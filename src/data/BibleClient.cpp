#include "BibleClient.h"
#include "DataUtils.h"
#include <algorithm>
#include <cctype>

namespace theword::data {

BibleClient::BibleClient(theword::core::IHttpClient& client, int bibleId)
    : apiClient(client), bibleId(bibleId), baseUrl("https://api.youversion.com/v1") {
}

bool BibleClient::HasChapter(const std::string& bookId, int chapter) {
    auto key = bookId + "." + std::to_string(chapter);
    auto cached = cachedHasChapter.find(key);
    if (cached != cachedHasChapter.end()) return cached->second;

    auto result = LoadChapter(bookId, chapter);
    cachedHasChapter[key] = result.has_value();
    return result.has_value();
}

const char* BibleClient::ProviderName() const {
    return "BibleClient";
}

std::string BibleClient::ExtractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return "";

    size_t valueStart = json.find('\"', colonPos);
    if (valueStart == std::string::npos) return "";
    valueStart++;

    std::string result;
    for (size_t i = valueStart; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            if (json[i + 1] == 'n') result += '\n';
            else if (json[i + 1] == 't') result += '\t';
            else if (json[i + 1] == '\\') result += '\\';
            else if (json[i + 1] == '\"') result += '\"';
            else result += json[i + 1];
            i++;
        } else if (json[i] == '\"') {
            break;
        } else {
            result += json[i];
        }
    }

    return result;
}

std::optional<ChapterData> BibleClient::LoadChapter(
        const std::string& bookId, int chapter) {
    std::string usfmRef = bookId + "." + std::to_string(chapter);
    std::string url = baseUrl + "/bibles/" + std::to_string(bibleId) +
                      "/passages/" + usfmRef + "?format=html&include_headings=true";

    std::string response = apiClient.Get(url);
    if (response.empty()) return std::nullopt;

    std::string htmlContent = ExtractJsonString(response, "content");
    if (htmlContent.empty()) return std::nullopt;

    return ParseHtmlChapter(htmlContent, bookId, chapter);
}

namespace {

std::string StripHtmlImpl(const std::string& html) {
    std::string result;
    bool inTag = false;
    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            inTag = true;
        } else if (html[i] == '>') {
            inTag = false;
        } else if (!inTag) {
            result += html[i];
        }
    }
    return DecodeHtmlEntities(result);
}

int ParseVerseNumber(const std::string& tag); // forward decl — defined below

std::string ExtractFootnotes(const std::string& html, ChapterData& data, int startVerse) {
    std::string result;
    size_t pos = 0;
    int currentVerse = startVerse;
    // Track footnote block extraction
    bool extractingFootnote = false;
    std::string fnHtml;
    int fnDepth = 0;

    while (pos < html.size()) {
        if (html[pos] == '<') {
            size_t tagEnd = html.find('>', pos);
            if (tagEnd == std::string::npos) break;
            std::string tag = html.substr(pos + 1, tagEnd - pos - 1);

            // Track verse numbers even inside yv-n blocks
            if (tag.find("yv-v") != std::string::npos) {
                int v = ParseVerseNumber(tag);
                if (v > 0) currentVerse = v;
            }

            if (tag.find("yv-n") != std::string::npos && !extractingFootnote) {
                extractingFootnote = true;
                fnDepth = 1;
                fnHtml.clear();
                pos = tagEnd + 1;
                continue;
            }

            if (extractingFootnote) {
                if (tag[0] == '/' && tag.find("span") != std::string::npos) {
                    fnDepth--;
                    if (fnDepth == 0) {
                        // End of footnote — extract it
                        extractingFootnote = false;

                        // Parse fr and ft spans from the captured inner HTML
                        std::string callerRef;
                        std::string fnText;

                        size_t frStart = fnHtml.find("<span class=\"fr\">");
                        if (frStart != std::string::npos) {
                            frStart += 17; // len of "<span class=\"fr\">" = 17
                            size_t frEnd = fnHtml.find("</span>", frStart);
                            if (frEnd != std::string::npos) {
                                callerRef = StripHtmlImpl(fnHtml.substr(frStart, frEnd - frStart));
                            }
                        }

                        size_t ftStart = fnHtml.find("<span class=\"ft\">");
                        if (ftStart != std::string::npos) {
                            ftStart += 17; // len of "<span class=\"ft\">" = 17
                            size_t ftEnd = fnHtml.find("</span>", ftStart);
                            if (ftEnd != std::string::npos) {
                                fnText = StripHtmlImpl(fnHtml.substr(ftStart, ftEnd - ftStart));
                            }
                        }

                        if (fnText.empty()) {
                            // No explicit ft — use all content as text
                            fnText = StripHtmlImpl(fnHtml);
                        }

                        Footnote footnote;
                        footnote.verseId = currentVerse;
                        footnote.callerRef = callerRef;
                        footnote.text = fnText;
                        data.footnotes.push_back(footnote);

                        pos = tagEnd + 1;
                        continue;
                    }
                } else if (tag.find("span") != std::string::npos && tag[0] != '/') {
                    fnDepth++;
                }
                fnHtml += html.substr(pos, tagEnd - pos + 1);
                pos = tagEnd + 1;
                continue;
            }

            result += html.substr(pos, tagEnd - pos + 1);
            pos = tagEnd + 1;
        } else {
            if (!extractingFootnote) {
                result += html[pos];
            }
            if (extractingFootnote) {
                fnHtml += html[pos];
            }
            pos++;
        }
    }
    return result;
}

void FlushWordsToSegment(ChapterData& data, SegmentType segType, int segLevel,
                         int verseStart, int verseEnd,
                         const std::vector<Word>& words, size_t startWordIndex) {
    if (words.empty()) return;
    Segment seg;
    seg.type = segType;
    seg.level = segLevel;
    seg.verseStart = verseStart;
    seg.verseEnd = verseEnd;
    seg.startWordIndex = startWordIndex;
    seg.wordCount = words.size();
    data.segments.push_back(seg);
}

int ParseVerseNumber(const std::string& tag) {
    size_t vPos = tag.find("v=\"");
    if (vPos == std::string::npos) return -1;
    vPos += 3;
    size_t vEnd = tag.find('\"', vPos);
    if (vEnd == std::string::npos) return -1;
    return std::stoi(tag.substr(vPos, vEnd - vPos));
}

SegmentType ClassifyDiv(const std::string& divClass, int& outLevel) {
    if (divClass.find("s1") != std::string::npos) { outLevel = 1; return SegmentType::SectionHeading; }
    if (divClass.find("s2") != std::string::npos) { outLevel = 2; return SegmentType::SectionHeading; }
    if (divClass.find("q1") != std::string::npos) { outLevel = 1; return SegmentType::PoetryLine; }
    if (divClass.find("q2") != std::string::npos) { outLevel = 2; return SegmentType::PoetryLine; }
    if (divClass.find("q3") != std::string::npos) { outLevel = 3; return SegmentType::PoetryLine; }
    if (divClass.find("p") != std::string::npos || divClass.find("m") != std::string::npos) {
        outLevel = 0;
        return SegmentType::ParagraphBreak;
    }
    return SegmentType::VerseText;
}

void ParseParagraphContent(const std::string& html, ChapterData& data) {
    Segment pb;
    pb.type = SegmentType::ParagraphBreak;
    pb.level = 0;
    data.segments.push_back(pb);

    std::string cleaned = ExtractFootnotes(html, data, 1);
    size_t pos = 0;
    int currentVerse = 1;
    std::vector<Word> currentWords;
    int segmentVerseStart = 1;
    int nextWordId = static_cast<int>(data.words.size());
    size_t segmentStartWordIndex = data.words.size();

    while (pos < cleaned.size()) {
        if (cleaned[pos] != '<') {
            size_t nextTag = cleaned.find('<', pos);
            std::string chunk = (nextTag == std::string::npos)
                ? cleaned.substr(pos) : cleaned.substr(pos, nextTag - pos);
            std::string decoded = DecodeHtmlEntities(chunk);
            TokenizeToWords(decoded, currentVerse, data.words, currentWords, nextWordId);
            pos = (nextTag == std::string::npos) ? cleaned.size() : nextTag;
            continue;
        }

        size_t closeTag = cleaned.find('>', pos);
        if (closeTag == std::string::npos) break;
        std::string tag = cleaned.substr(pos + 1, closeTag - pos - 1);

        if (tag.find("yv-v") != std::string::npos) {
            int newVerse = ParseVerseNumber(tag);
            if (newVerse > 0) {
                FlushWordsToSegment(data, SegmentType::VerseText, 0,
                                    segmentVerseStart, currentVerse,
                                    currentWords, segmentStartWordIndex);
                currentVerse = newVerse;
                segmentStartWordIndex = data.words.size();
                segmentVerseStart = currentVerse;
                currentWords.clear();
            }
        }

        if (tag.find("yv-vlbl") != std::string::npos) {
            size_t spanEnd = cleaned.find("</span>", closeTag);
            pos = (spanEnd != std::string::npos) ? spanEnd + 7 : closeTag + 1;
            continue;
        }

        pos = closeTag + 1;
    }

    FlushWordsToSegment(data, SegmentType::VerseText, 0,
                        segmentVerseStart, currentVerse,
                        currentWords, segmentStartWordIndex);
}

void ParseSectionHeading(const std::string& innerHtml, int segLevel, ChapterData& data) {
    std::string text = StripHtmlImpl(innerHtml);

    while (!text.empty() && std::isdigit(static_cast<unsigned char>(text[0]))) {
        size_t spacePos = text.find(' ');
        if (spacePos == std::string::npos) break;
        std::string prefix = text.substr(0, spacePos);
        bool allDigits = !prefix.empty() &&
            std::all_of(prefix.begin(), prefix.end(),
                [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        if (!allDigits) break;
        text = text.substr(spacePos + 1);
    }

    while (!text.empty() && text[0] == ' ') {
        text = text.substr(1);
    }

    if (text.empty()) return;

    Segment seg;
    seg.type = SegmentType::SectionHeading;
    seg.level = segLevel;
    seg.text = text;
    data.segments.push_back(seg);
}

void ParsePoetryLine(const std::string& innerHtml, int segLevel, ChapterData& data) {
    std::string cleaned = ExtractFootnotes(innerHtml, data, 1);
    std::string text;
    size_t pos = 0;
    int verse = 1;

    while (pos < cleaned.size()) {
        if (cleaned[pos] != '<') {
            size_t nextTag = cleaned.find('<', pos);
            std::string chunk = (nextTag == std::string::npos)
                ? cleaned.substr(pos) : cleaned.substr(pos, nextTag - pos);
            text += chunk;
            pos = (nextTag == std::string::npos) ? cleaned.size() : nextTag;
            continue;
        }

        size_t closeTag = cleaned.find('>', pos);
        if (closeTag == std::string::npos) break;
        std::string tag = cleaned.substr(pos + 1, closeTag - pos - 1);

        if (tag.find("yv-v") != std::string::npos) {
            int v = ParseVerseNumber(tag);
            if (v > 0) verse = v;
        }

        pos = closeTag + 1;
    }

    std::vector<Word> words;
    int nextWordId = static_cast<int>(data.words.size());
    size_t startWordIndex = data.words.size();
    std::string decoded = DecodeHtmlEntities(text);
    TokenizeToWords(decoded, verse, data.words, words, nextWordId);

    if (words.empty()) return;

    Segment seg;
    seg.type = SegmentType::PoetryLine;
    seg.level = segLevel;
    seg.verseStart = verse;
    seg.verseEnd = verse;
    seg.startWordIndex = startWordIndex;
    seg.wordCount = words.size();
    data.segments.push_back(seg);
}

} // anonymous namespace

std::optional<ChapterData> BibleClient::ParseHtmlChapter(const std::string& html,
        const std::string& bookId, int chapter) const {
    ChapterData data;
    data.bookId = bookId;
    data.chapterNum = chapter;

    size_t pos = 0;
    while (pos < html.size()) {
        size_t divStart = html.find("<div", pos);
        if (divStart == std::string::npos) break;

        size_t tagEnd = html.find('>', divStart);
        if (tagEnd == std::string::npos) break;

        std::string openTag = html.substr(divStart, tagEnd - divStart);

        std::string divClass;
        size_t classPos = openTag.find("class=\"");
        if (classPos != std::string::npos) {
            classPos += 7;
            size_t classEnd = openTag.find('\"', classPos);
            if (classEnd != std::string::npos) {
                divClass = openTag.substr(classPos, classEnd - classPos);
            }
        }

        int segLevel = 0;
        SegmentType segType = ClassifyDiv(divClass, segLevel);
        if (segType == SegmentType::VerseText) {
            pos = tagEnd + 1;
            continue;
        }

        size_t closeDiv = html.find("</div>", tagEnd);
        if (closeDiv == std::string::npos) break;

        std::string innerHtml = html.substr(tagEnd + 1, closeDiv - tagEnd - 1);
        pos = closeDiv + 6;

        if (segType == SegmentType::ParagraphBreak) {
            ParseParagraphContent(innerHtml, data);
        } else if (segType == SegmentType::SectionHeading) {
            ParseSectionHeading(innerHtml, segLevel, data);
        } else if (segType == SegmentType::PoetryLine) {
            ParsePoetryLine(innerHtml, segLevel, data);
        }
    }

    return data;
}

std::string BibleClient::StripHtml(const std::string& html) {
    return StripHtmlImpl(html);
}

} // namespace theword::data
