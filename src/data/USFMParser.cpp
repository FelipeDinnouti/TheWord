#include "USFMParser.h"
#include "DataUtils.h"
#include <fstream>
#include <sstream>
#include <cctype>

namespace theword::data {

USFMParser::USFMParser(const std::string& usfmDir, std::unique_ptr<theword::core::IAssetProvider> assets)
    : usfmDir(usfmDir), assets(std::move(assets)) {}

const char* USFMParser::ProviderName() const {
    return "USFMParser";
}

std::string USFMParser::ExtractBookCodeFromId(const std::string& line) const {
    if (line.size() < 5) return "";
    size_t start = 4;
    size_t end = line.find_first_of(" \t", start);
    if (end == std::string::npos) return "";
    return line.substr(start, end - start);
}

std::string USFMParser::LoadFile(const std::string& filepath) const {
    if (assets) {
        auto content = assets->readFileText(filepath);
        return content.value_or("");
    }
    std::ifstream file(filepath);
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string USFMParser::ExtractFootnotes(
    const std::string& text,
    std::vector<std::pair<int, Footnote>>& outFootnotes) const {

    std::string result;
    size_t pos = 0;
    bool inFootnote = false;
    bool inRq = false;
    int currentChapter = 0;
    int currentVerse = 0;
    std::string fnContent;
    std::string fnCallerRef;
    std::string rqContent;

    while (pos < text.size()) {
        if (!inFootnote && !inRq) {
            // Track \c N (chapter) and \v M (verse) in raw text
            if (pos + 2 < text.size() && text[pos] == '\\' && text[pos + 1] == 'c' &&
                (text[pos + 2] == ' ' || text[pos + 2] == '\t')) {
                size_t numStart = pos + 2;
                while (numStart < text.size() && (text[numStart] == ' ' || text[numStart] == '\t'))
                    numStart++;
                size_t numEnd = numStart;
                while (numEnd < text.size() && std::isdigit(static_cast<unsigned char>(text[numEnd])))
                    numEnd++;
                if (numEnd > numStart) {
                    currentChapter = std::stoi(text.substr(numStart, numEnd - numStart));
                    currentVerse = 0;
                }
            }
            if (pos + 2 < text.size() && text[pos] == '\\' && text[pos + 1] == 'v' &&
                (text[pos + 2] == ' ' || text[pos + 2] == '\t')) {
                size_t numStart = pos + 2;
                while (numStart < text.size() && (text[numStart] == ' ' || text[numStart] == '\t'))
                    numStart++;
                size_t numEnd = numStart;
                while (numEnd < text.size() && std::isdigit(static_cast<unsigned char>(text[numEnd])))
                    numEnd++;
                if (numEnd > numStart)
                    currentVerse = std::stoi(text.substr(numStart, numEnd - numStart));
            }

            if (pos + 1 < text.size() && text[pos] == '\\' && text[pos + 1] == 'f' &&
                (pos + 2 >= text.size() || text[pos + 2] == ' ' || text[pos + 2] == '+' ||
                 text[pos + 2] == '*' || text[pos + 2] == '\\' || text[pos + 2] == '\r' || text[pos + 2] == '\n')) {
                if (pos + 2 < text.size() && (text[pos + 2] == ' ' || text[pos + 2] == '+' ||
                     text[pos + 2] == '\r' || text[pos + 2] == '\n')) {
                    inFootnote = true;
                    fnContent.clear();
                    fnCallerRef.clear();
                    pos += 2;
                    continue;
                }
            }

            if (pos + 3 < text.size() && text.substr(pos, 4) == "\\rq ") {
                inRq = true;
                rqContent.clear();
                pos += 4;
                continue;
            }

            result += text[pos];
            pos++;
        } else if (inFootnote) {
            if (pos + 2 < text.size() && text[pos] == '\\' && text[pos + 1] == 'f' && text[pos + 2] == '*') {
                inFootnote = false;
                pos += 3;

                // Process extracted footnote content
                std::string fnText = StripInlineMarkers(fnContent);

                // Parse \fr and \ft within the footnote
                size_t frPos = fnText.find("\\fr ");
                size_t ftPos = fnText.find("\\ft ");
                if (frPos != std::string::npos) {
                    size_t frEnd = (ftPos != std::string::npos && ftPos > frPos)
                        ? ftPos : fnText.size();
                    fnCallerRef = fnText.substr(frPos + 4, frEnd - frPos - 4);
                    while (!fnCallerRef.empty() && fnCallerRef.back() == ' ')
                        fnCallerRef.pop_back();
                }
                std::string bodyText;
                if (ftPos != std::string::npos) {
                    bodyText = fnText.substr(ftPos + 4);
                } else if (frPos != std::string::npos) {
                    bodyText = fnText.substr(frPos + 4);
                } else {
                    bodyText = fnText;
                }

                if (currentChapter > 0) {
                    Footnote fn;
                    fn.verseId = currentVerse;
                    fn.callerRef = fnCallerRef;
                    fn.text = bodyText;
                    outFootnotes.emplace_back(currentChapter, fn);
                }

                continue;
            }

            if (pos + 1 < text.size() && text[pos] == '\\' && text[pos + 1] == 'f' &&
                pos + 2 < text.size() && text[pos + 2] == '+') {
                pos += 3;
                continue;
            }
            fnContent += text[pos];
            pos++;
        } else { // inRq
            if (pos + 3 < text.size() && text.substr(pos, 4) == "\\rq*") {
                inRq = false;
                pos += 4;

                // Trim whitespace from captured content
                while (!rqContent.empty() && (rqContent.back() == ' ' || rqContent.back() == '\t'))
                    rqContent.pop_back();
                while (!rqContent.empty() && (rqContent.front() == ' ' || rqContent.front() == '\t'))
                    rqContent.erase(0, 1);

                if (currentChapter > 0 && !rqContent.empty()) {
                    Footnote fn;
                    fn.verseId = currentVerse;
                    fn.callerRef = "";
                    fn.text = rqContent;
                    outFootnotes.emplace_back(currentChapter, fn);
                }

                continue;
            }
            rqContent += text[pos];
            pos++;
        }
    }

    // Post-process: collapse whitespace
    std::string cleaned;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == ' ' && i + 1 < result.size() && result[i + 1] == ' ') continue;
        if (result[i] == '\r') continue;
        cleaned += result[i];
    }

    return cleaned;
}

std::string USFMParser::StripInlineMarkers(const std::string& text) const {
    std::string result = text;

    {
        std::string temp;
        size_t pos = 0;
        bool inAdd = false;
        while (pos < result.size()) {
            if (!inAdd) {
                if (pos + 4 < result.size() && result.substr(pos, 5) == "\\add ") {
                    inAdd = true;
                    pos += 5;
                    continue;
                }
                if (pos + 4 < result.size() && result.substr(pos, 5) == "\\add*") {
                    pos += 5;
                    continue;
                }
                if (pos + 4 < result.size() && result.substr(pos, 5) == "\\add\r") {
                    inAdd = true;
                    pos += 5;
                    continue;
                }
                if (pos + 4 < result.size() && result.substr(pos, 5) == "\\add\n") {
                    inAdd = true;
                    pos += 5;
                    continue;
                }
                temp += result[pos];
                pos++;
            } else {
                if (pos + 4 < result.size() && result.substr(pos, 5) == "\\add*") {
                    inAdd = false;
                    pos += 5;
                    continue;
                }
                temp += result[pos];
                pos++;
            }
        }
        result = temp;
    }

    {
        std::string temp;
        size_t pos = 0;
        bool inWj = false;
        while (pos < result.size()) {
            if (!inWj) {
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\wj ") {
                    inWj = true;
                    pos += 4;
                    continue;
                }
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\wj*") {
                    pos += 4;
                    continue;
                }
                temp += result[pos];
                pos++;
            } else {
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\wj*") {
                    inWj = false;
                    pos += 4;
                    continue;
                }
                temp += result[pos];
                pos++;
            }
        }
        result = temp;
    }

    {
        std::string temp;
        size_t pos = 0;
        bool inRq = false;
        while (pos < result.size()) {
            if (!inRq) {
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\rq ") {
                    inRq = true;
                    pos += 4;
                    continue;
                }
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\rq*") {
                    pos += 4;
                    continue;
                }
                temp += result[pos];
                pos++;
            } else {
                if (pos + 3 < result.size() && result.substr(pos, 4) == "\\rq*") {
                    inRq = false;
                    pos += 4;
                    continue;
                }
                pos++;
            }
        }
        result = temp;
    }

    return result;
}

namespace {

struct ParseState {
    ChapterData chapter;
    std::vector<Word> currentWords;
    int nextWordId = 0;
    int currentVerse = 0;
    size_t segmentStartWordIndex = 0;
    int segmentVerseStart = 0;
    SegmentType currentSegType = SegmentType::VerseText;
    int currentSegLevel = 0;
    bool inDescription = false;
};

void FlushSegment(ParseState& st) {
    if (st.currentWords.empty()) return;
    Segment seg;
    seg.type = st.currentSegType;
    seg.level = st.currentSegLevel;
    seg.verseStart = st.segmentVerseStart;
    seg.verseEnd = st.currentVerse;
    seg.startWordIndex = st.segmentStartWordIndex;
    seg.wordCount = st.currentWords.size();
    st.chapter.segments.push_back(seg);
    st.currentWords.clear();
}

void HandleChapter(const std::string& rest, const std::string& bookId,
                   std::vector<ChapterData>& chapters, ParseState& st) {
    FlushSegment(st);
    st.inDescription = false;

    size_t numEnd = 0;
    while (numEnd < rest.size() && std::isdigit(static_cast<unsigned char>(rest[numEnd]))) {
        numEnd++;
    }
    if (numEnd == 0) return;

    int chapterNum = std::stoi(rest.substr(0, numEnd));

    if (st.chapter.chapterNum > 0) {
        chapters.push_back(std::move(st.chapter));
        ParseState newSt;
        newSt.chapter.bookId = bookId;
        newSt.chapter.chapterNum = chapterNum;
        st = std::move(newSt);
    } else {
        st.chapter.chapterNum = chapterNum;
    }

    std::string chapterNumStr = rest.substr(0, numEnd);

    Segment label;
    label.type = SegmentType::ChapterLabel;
    label.level = 0;
    label.text = chapterNumStr;
    st.chapter.segments.push_back(label);

    st.currentVerse = 0;
    st.segmentStartWordIndex = 0;
    st.segmentVerseStart = 0;
    st.currentSegType = SegmentType::VerseText;
    st.currentSegLevel = 0;
}

void HandleVerse(const std::string& rest, ParseState& st) {
    FlushSegment(st);

    size_t numEnd = 0;
    while (numEnd < rest.size() && std::isdigit(static_cast<unsigned char>(rest[numEnd]))) {
        numEnd++;
    }
    if (numEnd > 0) {
        st.currentVerse = std::stoi(rest.substr(0, numEnd));
    }

    std::string verseText = (numEnd < rest.size()) ? rest.substr(numEnd) : "";
    while (!verseText.empty() && verseText[0] == ' ') {
        verseText = verseText.substr(1);
    }

    st.segmentStartWordIndex = st.chapter.words.size();
    st.segmentVerseStart = st.currentVerse;
    st.currentSegType = SegmentType::VerseText;
    st.currentSegLevel = 0;
    st.inDescription = false;

    if (!verseText.empty()) {
        TokenizeToWords(verseText, st.currentVerse, st.chapter.words, st.currentWords, st.nextWordId);
    }
}

void HandleParagraph(const std::string& rest, ParseState& st) {
    FlushSegment(st);
    st.inDescription = false;

    Segment pb;
    pb.type = SegmentType::ParagraphBreak;
    pb.level = 0;
    st.chapter.segments.push_back(pb);

    if (!rest.empty()) {
        st.segmentStartWordIndex = st.chapter.words.size();
        st.segmentVerseStart = st.currentVerse;
        st.currentSegType = SegmentType::VerseText;
        st.currentSegLevel = 0;
        TokenizeToWords(rest, st.currentVerse, st.chapter.words, st.currentWords, st.nextWordId);
    }
}

void HandleSectionHeading(const std::string& marker, const std::string& rest, ParseState& st) {
    FlushSegment(st);
    st.inDescription = false;

    int level = marker[1] - '0';
    Segment heading;
    heading.type = SegmentType::SectionHeading;
    heading.level = level;
    heading.text = rest;
    st.chapter.segments.push_back(heading);
}

void HandleDescription(const std::string& rest, ParseState& st) {
    FlushSegment(st);
    st.inDescription = true;

    Segment heading;
    heading.type = SegmentType::SectionHeading;
    heading.level = 1;
    heading.text = rest;
    st.chapter.segments.push_back(heading);
}

void HandleRemark(const std::string& rest, ParseState& st) {
    FlushSegment(st);

    Segment heading;
    heading.type = SegmentType::SectionHeading;
    heading.level = 1;
    heading.text = rest;
    st.chapter.segments.push_back(heading);
}

void HandlePoetryLine(const std::string& marker, const std::string& rest, ParseState& st) {
    FlushSegment(st);
    st.inDescription = false;

    int level = 1;
    if (marker.size() > 1) {
        level = marker[1] - '0';
        if (level < 1 || level > 3) level = 1;
    }

    st.segmentStartWordIndex = st.chapter.words.size();
    st.segmentVerseStart = st.currentVerse;
    st.currentSegType = SegmentType::PoetryLine;
    st.currentSegLevel = level;

    if (!rest.empty()) {
        TokenizeToWords(rest, st.currentVerse, st.chapter.words, st.currentWords, st.nextWordId);
    }
}

void HandleBookTitle(const std::string& marker, const std::string& rest, ParseState& st) {
    FlushSegment(st);
    st.inDescription = false;

    int level = marker[2] - '0';
    Segment title;
    title.type = SegmentType::BookTitle;
    title.level = level;
    title.text = rest;
    st.chapter.segments.push_back(title);
}

} // anonymous namespace

std::vector<ChapterData> USFMParser::ParseBook(const std::string& bookId) const {
    auto cached = bookCache.find(bookId);
    if (cached != bookCache.end()) {
        return cached->second;
    }

    std::string filepath = usfmDir + "/" + bookId + ".usfm";
    std::string content = LoadFile(filepath);

    if (content.empty()) {
        bookCache[bookId] = {};
        return {};
    }

    std::vector<std::pair<int, Footnote>> footnoteBuf;
    content = ExtractFootnotes(content, footnoteBuf);
    content = StripInlineMarkers(content);

    std::vector<ChapterData> chapters;
    ParseState st;
    st.chapter.bookId = bookId;
    st.chapter.chapterNum = 0;

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        if (line[0] != '\\') {
            if (!st.inDescription && st.chapter.chapterNum > 0 && st.currentVerse > 0) {
                TokenizeToWords(line, st.currentVerse, st.chapter.words, st.currentWords, st.nextWordId);
            }
            continue;
        }

        size_t markerEnd = line.find_first_of(" \t\r");
        std::string marker = (markerEnd == std::string::npos) ? line.substr(1) : line.substr(1, markerEnd - 1);
        std::string rest = (markerEnd == std::string::npos) ? "" : line.substr(markerEnd + 1);

        while (!rest.empty() && (rest[0] == ' ' || rest[0] == '\t')) {
            rest = rest.substr(1);
        }

        if (marker == "id" || marker == "h" || marker == "toc1" ||
            marker == "toc2" || marker == "toc3" || marker == "ide") {
            continue;
        }

        if (marker == "c") {
            HandleChapter(rest, bookId, chapters, st);
        } else if (marker == "v") {
            HandleVerse(rest, st);
        } else if (marker == "p" || marker == "m") {
            HandleParagraph(rest, st);
        } else if (marker == "s1" || marker == "s2" || marker == "s3" || marker == "s4" || marker == "s5") {
            HandleSectionHeading(marker, rest, st);
        } else if (marker == "d") {
            HandleDescription(rest, st);
        } else if (marker == "r") {
            HandleRemark(rest, st);
        } else if (marker == "q" || marker == "q1" || marker == "q2" || marker == "q3") {
            HandlePoetryLine(marker, rest, st);
        } else if (marker == "mt1" || marker == "mt2" || marker == "mt3" || marker == "mt4") {
            HandleBookTitle(marker, rest, st);
        }
    }

    FlushSegment(st);
    if (st.chapter.chapterNum > 0) {
        chapters.push_back(std::move(st.chapter));
    }

    // Distribute footnotes to chapters
    for (auto& ch : chapters) {
        for (auto& [chNum, fn] : footnoteBuf) {
            if (chNum == ch.chapterNum) {
                ch.footnotes.push_back(std::move(fn));
            }
        }
    }

    bookCache[bookId] = chapters;
    return chapters;
}

std::optional<ChapterData> USFMParser::LoadChapter(
        const std::string& bookId, int chapter) {
    auto chapters = ParseBook(bookId);
    for (auto& ch : chapters) {
        if (ch.chapterNum == chapter) {
            return std::move(ch);
        }
    }
    return std::nullopt;
}

} // namespace theword::data
