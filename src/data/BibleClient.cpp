#include "BibleClient.h"
#include "core/GlobalId.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cctype>

BibleClient::BibleClient(APIClient& client, int bibleId)
    : apiClient(client), bibleId(bibleId) {
    baseUrl = "https://api.youversion.com/v1";
}

bool BibleClient::HasChapter(const std::string& bookId, int chapter) const {
    (void)bookId;
    (void)chapter;
    return true;
}

const char* BibleClient::ProviderName() const {
    return "BibleClient";
}

std::string BibleClient::extractJsonString(const std::string& json, const std::string& key) {
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

    std::string response = apiClient.get(url);
    if (response.empty()) return std::nullopt;

    std::string htmlContent = extractJsonString(response, "content");
    if (htmlContent.empty()) return std::nullopt;

    return parseHtmlChapter(htmlContent, bookId, chapter);
}

std::optional<ChapterData> BibleClient::parseHtmlChapter(const std::string& html,
        const std::string& bookId, int chapter) {
    ChapterData data;
    data.bookId = bookId;
    data.chapterNum = chapter;

    // Parse div blocks from HTML using character-by-character scanning
    // We're looking for <div class="..."> ... </div> blocks

    size_t pos = 0;
    std::vector<Word> currentWords;
    int currentVerse = 1;
    int segmentVerseStart = 1;
    size_t segmentStartWordIndex = 0;
    SegmentType currentSegType = SegmentType::VerseText;
    int currentSegLevel = 0;
    bool inParagraphContent = false;
    bool inFootnote = false;

    auto flushWordsToSegment = [&]() {
        if (currentWords.empty()) return;

        Segment seg;
        seg.type = currentSegType;
        seg.level = currentSegLevel;
        seg.verseStart = segmentVerseStart;
        seg.verseEnd = currentVerse;
        seg.startWordIndex = segmentStartWordIndex;
        seg.wordCount = currentWords.size();

        data.segments.push_back(seg);
    };

    while (pos < html.size()) {
        // Look for <div
        size_t divStart = html.find("<div", pos);
        if (divStart == std::string::npos) break;

        // Find the end of the opening div tag
        size_t tagEnd = html.find('>', divStart);
        if (tagEnd == std::string::npos) break;

        std::string openTag = html.substr(divStart, tagEnd - divStart);

        // Extract class from the opening tag
        std::string divClass;
        size_t classPos = openTag.find("class=\"");
        if (classPos != std::string::npos) {
            classPos += 7; // skip class="
            size_t classEnd = openTag.find('\"', classPos);
            if (classEnd != std::string::npos) {
                divClass = openTag.substr(classPos, classEnd - classPos);
            }
        }

        // Determine segment type from class
        SegmentType segType = SegmentType::VerseText;
        int segLevel = 0;
        bool isDivBlock = true;

        if (divClass.find("s1") != std::string::npos) {
            segType = SegmentType::SectionHeading;
            segLevel = 1;
        } else if (divClass.find("s2") != std::string::npos) {
            segType = SegmentType::SectionHeading;
            segLevel = 2;
        } else if (divClass.find("q1") != std::string::npos) {
            segType = SegmentType::PoetryLine;
            segLevel = 1;
        } else if (divClass.find("q2") != std::string::npos) {
            segType = SegmentType::PoetryLine;
            segLevel = 2;
        } else if (divClass.find("q3") != std::string::npos) {
            segType = SegmentType::PoetryLine;
            segLevel = 3;
        } else if (divClass.find("p") != std::string::npos ||
                   divClass.find("m") != std::string::npos) {
            segType = SegmentType::ParagraphBreak;
        } else {
            isDivBlock = false;
        }

        if (!isDivBlock) {
            pos = tagEnd + 1;
            continue;
        }

        // Find the matching closing div tag (simplified: find next </div>)
        size_t closeDiv = html.find("</div>", tagEnd);
        if (closeDiv == std::string::npos) break;

        std::string innerHtml = html.substr(tagEnd + 1, closeDiv - tagEnd - 1);

        pos = closeDiv + 6; // skip </div>

        if (segType == SegmentType::ParagraphBreak) {
            // Emit ParagraphBreak segment
            Segment pb;
            pb.type = SegmentType::ParagraphBreak;
            pb.level = 0;
            data.segments.push_back(pb);

            // Parse verse content within the paragraph
            size_t innerPos = 0;
            currentVerse = 1;
            currentWords.clear();
            segmentStartWordIndex = data.words.size();
            currentSegType = SegmentType::VerseText;
            currentSegLevel = 0;
            segmentVerseStart = 1;
            inFootnote = false;

            while (innerPos < innerHtml.size()) {
                // Check for tags
                if (innerHtml[innerPos] == '<') {
                    size_t closeTag = innerHtml.find('>', innerPos);
                    if (closeTag == std::string::npos) break;

                    std::string tag = innerHtml.substr(innerPos + 1, closeTag - innerPos - 1);

                    // Check if this is a verse number span
                    if (tag.find("yv-v") != std::string::npos) {
                        // Extract verse number from v="N" attribute
                        size_t vPos = tag.find("v=\"");
                        if (vPos != std::string::npos) {
                            vPos += 3;
                            size_t vEnd = tag.find('\"', vPos);
                            if (vEnd != std::string::npos) {
                                flushWordsToSegment();
                                currentVerse = std::stoi(tag.substr(vPos, vEnd - vPos));
                                segmentStartWordIndex = data.words.size();
                                segmentVerseStart = currentVerse;
                                currentWords.clear();
                            }
                        }
                    }

                    // Check for footnote start
                    if (tag.find("yv-n") != std::string::npos) {
                        inFootnote = true;
                    }

                    // Check for footnote end
                    if (tag[0] == '/' && tag.find("span") != std::string::npos) {
                        // Closing span might end footnote context
                        // We'll handle this differently — skip entire footnote spans
                    }

                    // Skip verse labels
                    if (tag.find("yv-vlbl") != std::string::npos) {
                        innerPos = closeTag + 1;
                        // Skip content until </span>
                        size_t spanEnd = innerHtml.find("</span>", innerPos);
                        if (spanEnd != std::string::npos) {
                            innerPos = spanEnd + 7;
                        }
                        continue;
                    }

                    innerPos = closeTag + 1;
                } else {
                    if (inFootnote) {
                        // Check if we're exiting the footnote
                        if (innerHtml.substr(innerPos, 7) == "</span>") {
                            inFootnote = false;
                            innerPos += 7;
                            continue;
                        }
                        innerPos++;
                        continue;
                    }

                    // Collect text until next tag or end
                    size_t nextTag = innerHtml.find('<', innerPos);
                    std::string textChunk;
                    if (nextTag == std::string::npos) {
                        textChunk = innerHtml.substr(innerPos);
                        innerPos = innerHtml.size();
                    } else {
                        textChunk = innerHtml.substr(innerPos, nextTag - innerPos);
                        innerPos = nextTag;
                    }

                    // Decode HTML entities
                    std::string decoded;
                    for (size_t ci = 0; ci < textChunk.size(); ++ci) {
                if (textChunk[ci] == '&' && textChunk.substr(ci, 5) == "&amp;") {
                    decoded += '&';
                    ci += 4;
                        } else if (textChunk[ci] == '&' && textChunk.substr(ci, 4) == "&lt;") {
                            decoded += '<';
                            ci += 3;
                        } else if (textChunk[ci] == '&' && textChunk.substr(ci, 4) == "&gt;") {
                            decoded += '>';
                            ci += 3;
                        } else if (textChunk[ci] == '&' && textChunk.substr(ci, 6) == "&quot;") {
                            decoded += '\"';
                            ci += 5;
                        } else if (textChunk[ci] == '&' && textChunk.substr(ci, 3) == "&#") {
                            // Skip numeric entities (rare)
                            size_t semi = textChunk.find(';', ci);
                            if (semi != std::string::npos) {
                                ci = semi;
                            }
                        } else {
                            decoded += textChunk[ci];
                        }
                    }

                    // Tokenize the text chunk into words
                    std::istringstream stream(decoded);
                    std::string word;
                    while (stream >> word) {
                        Word w;
                        w.id = GetNextWordId();
                        w.verseId = currentVerse;
                        w.text = word;
                        data.words.push_back(w);
                        currentWords.push_back(w);
                    }
                }
            }

            flushWordsToSegment();
        } else if (segType == SegmentType::SectionHeading) {
            // Extract text from inner HTML (strip tags)
            std::string headingText = stripHtml(innerHtml);

            // Remove leading verse labels if present
            while (!headingText.empty() && std::isdigit(static_cast<unsigned char>(headingText[0]))) {
                size_t spacePos = headingText.find(' ');
                if (spacePos == std::string::npos) break;
                // Check if this is a verse label like "1 "
                std::string prefix = headingText.substr(0, spacePos);
                bool allDigits = !prefix.empty() &&
                    std::all_of(prefix.begin(), prefix.end(),
                        [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
                if (allDigits) {
                    headingText = headingText.substr(spacePos + 1);
                } else {
                    break;
                }
            }

            // Trim whitespace
            while (!headingText.empty() && headingText[0] == ' ') {
                headingText = headingText.substr(1);
            }

            if (!headingText.empty()) {
                Segment seg;
                seg.type = SegmentType::SectionHeading;
                seg.level = segLevel;
                seg.text = headingText;
                data.segments.push_back(seg);
            }
        } else if (segType == SegmentType::PoetryLine) {
            // Parse text from inner html
            inFootnote = false;
            currentVerse = 1;
            currentWords.clear();
            segmentStartWordIndex = data.words.size();
            currentSegType = SegmentType::PoetryLine;
            currentSegLevel = segLevel;

            // Simple text extraction from inner html
            std::string poetryText;
            size_t innerPos = 0;
            while (innerPos < innerHtml.size()) {
                if (innerHtml[innerPos] == '<') {
                    size_t closeTag = innerHtml.find('>', innerPos);
                    if (closeTag == std::string::npos) break;
                    std::string tag = innerHtml.substr(innerPos + 1, closeTag - innerPos - 1);

                    if (tag.find("yv-n") != std::string::npos) {
                        inFootnote = true;
                    } else if (tag[0] == '/' && inFootnote) {
                        if (tag.find("span") != std::string::npos) {
                            inFootnote = false;
                        }
                    } else if (tag.find("yv-v") != std::string::npos) {
                        size_t vPos = tag.find("v=\"");
                        if (vPos != std::string::npos) {
                            vPos += 3;
                            size_t vEnd = tag.find('\"', vPos);
                            if (vEnd != std::string::npos) {
                                currentVerse = std::stoi(tag.substr(vPos, vEnd - vPos));
                            }
                        }
                    }

                    innerPos = closeTag + 1;
                } else if (inFootnote) {
                    innerPos++;
                } else {
                    size_t nextTag = innerHtml.find('<', innerPos);
                    std::string chunk;
                    if (nextTag == std::string::npos) {
                        chunk = innerHtml.substr(innerPos);
                        innerPos = innerHtml.size();
                    } else {
                        chunk = innerHtml.substr(innerPos, nextTag - innerPos);
                        innerPos = nextTag;
                    }
                    poetryText += chunk;
                }
            }

            // Tokenize poetry text
            std::istringstream stream(poetryText);
            std::string word;
            while (stream >> word) {
                Word w;
                w.id = GetNextWordId();
                w.verseId = currentVerse;
                w.text = word;
                data.words.push_back(w);
                currentWords.push_back(w);
            }

            if (!currentWords.empty()) {
                Segment seg;
                seg.type = SegmentType::PoetryLine;
                seg.level = segLevel;
                seg.verseStart = segmentVerseStart;
                seg.verseEnd = currentVerse;
                seg.startWordIndex = segmentStartWordIndex;
                seg.wordCount = currentWords.size();
                data.segments.push_back(seg);
                currentWords.clear();
            }
        }
    }

    return data;
}

std::string BibleClient::stripHtml(const std::string& html) {
    std::string result;
    bool inTag = false;
    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            inTag = true;
        } else if (html[i] == '>') {
            inTag = false;
        } else if (!inTag) {
            // Decode entities
            if (html[i] == '&' && html.substr(i, 5) == "&amp;") {
                result += '&';
                i += 4;
            } else if (html[i] == '&' && html.substr(i, 4) == "&lt;") {
                result += '<';
                i += 3;
            } else if (html[i] == '&' && html.substr(i, 4) == "&gt;") {
                result += '>';
                i += 3;
            } else if (html[i] == '&' && html.substr(i, 6) == "&quot;") {
                result += '\"';
                i += 5;
            } else {
                result += html[i];
            }
        }
    }
    return result;
}
