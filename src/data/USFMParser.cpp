#include "USFMParser.h"
#include "DataUtils.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iostream>

USFMParser::USFMParser(const std::string& usfmDir, IAssetProvider* assets)
    : usfmDir(usfmDir), assets(assets) {}

const char* USFMParser::ProviderName() const {
    return "USFMParser";
}

std::string USFMParser::extractBookCodeFromId(const std::string& line) const {
    // \id GEN Bíblia Livre - Textus Receptus
    if (line.size() < 5) return "";
    size_t start = 4; // skip "\id "
    size_t end = line.find_first_of(" \t", start);
    if (end == std::string::npos) return "";
    return line.substr(start, end - start);
}

std::string USFMParser::loadFile(const std::string& filepath) const {
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

std::string USFMParser::stripFootnotes(const std::string& text) const {
    std::string result;
    size_t pos = 0;
    bool inFootnote = false;

    while (pos < text.size()) {
        if (!inFootnote) {
            // Look for \f marker
            if (pos + 1 < text.size() && text[pos] == '\\' && text[pos + 1] == 'f' &&
                (pos + 2 >= text.size() || text[pos + 2] == ' ' || text[pos + 2] == '+' ||
                 text[pos + 2] == '*' || text[pos + 2] == '\\' || text[pos + 2] == '\r' || text[pos + 2] == '\n')) {
                // Check it's \f (not \fr, \ft, \fq, etc.)
                if (pos + 2 < text.size() && (text[pos + 2] == ' ' || text[pos + 2] == '+' ||
                     text[pos + 2] == '\r' || text[pos + 2] == '\n')) {
                    inFootnote = true;
                    pos += 2;
                    continue;
                }
            }
            result += text[pos];
            pos++;
        } else {
            // Look for \f* to end footnote
            if (pos + 2 < text.size() && text[pos] == '\\' && text[pos + 1] == 'f' && text[pos + 2] == '*') {
                inFootnote = false;
                pos += 3;
                continue;
            }
            pos++;
        }
    }

    // Collapse multiple spaces
    std::string cleaned;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == ' ' && i + 1 < result.size() && result[i + 1] == ' ') continue;
        if (result[i] == '\r') continue;
        cleaned += result[i];
    }

    return cleaned;
}

std::string USFMParser::stripInlineMarkers(const std::string& text) const {
    std::string result = text;

    // Strip \add ... \add* — keep text between, remove markers
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

    // Strip standalone markers that should be removed entirely
    // \wj ... \wj* (words of Jesus)
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

    return result;
}

std::vector<ChapterData> USFMParser::parseBook(const std::string& bookId) const {
    auto cached = bookCache.find(bookId);
    if (cached != bookCache.end()) {
        return cached->second;
    }

    // Try to find and load the file
    std::string filepath = usfmDir + "/" + bookId + ".usfm";
    std::string content = loadFile(filepath);

    if (content.empty()) {
        bookCache[bookId] = {};
        return {};
    }

    // Pre-process: strip footnotes
    content = stripFootnotes(content);

    // Pre-process: strip inline markers (\add, \wj, etc.)
    content = stripInlineMarkers(content);

    std::vector<ChapterData> chapters;
    ChapterData currentChapter;
    currentChapter.bookId = bookId;
    currentChapter.chapterNum = 0;

    std::vector<Word> currentWords;
    int currentVerse = 0;
    size_t segmentStartWordIndex = 0;
    int segmentVerseStart = 0;
    SegmentType currentSegType = SegmentType::VerseText;
    int currentSegLevel = 0;
    bool inDescription = false;

    auto flushSegment = [&]() {
        if (!currentWords.empty()) {
            Segment seg;
            seg.type = currentSegType;
            seg.level = currentSegLevel;
            seg.verseStart = segmentVerseStart;
            seg.verseEnd = currentVerse;
            seg.startWordIndex = segmentStartWordIndex;
            seg.wordCount = currentWords.size();
            currentChapter.segments.push_back(seg);
            currentWords.clear();
        }
    };

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim trailing whitespace
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        // Check for markers at the start of the line
        if (line[0] == '\\') {
            size_t markerEnd = line.find_first_of(" \t\r");
            std::string marker = (markerEnd == std::string::npos) ? line.substr(1) : line.substr(1, markerEnd - 1);
            std::string rest = (markerEnd == std::string::npos) ? "" : line.substr(markerEnd + 1);

            // Trim rest
            while (!rest.empty() && (rest[0] == ' ' || rest[0] == '\t')) {
                rest = rest.substr(1);
            }

            if (marker == "id") {
                // Book ID — skip
                continue;
            }

            if (marker == "h" || marker == "toc1" || marker == "toc2" || marker == "toc3" ||
                marker == "ide") {
                // Metadata — skip
                continue;
            }

            if (marker == "c") {
                // Chapter marker: \c N
                flushSegment();
                inDescription = false;

                if (currentChapter.chapterNum > 0) {
                    // Finalize previous chapter
                    chapters.push_back(std::move(currentChapter));

                    ChapterData newChapter;
                    newChapter.bookId = bookId;
                    newChapter.chapterNum = std::stoi(rest);
                    currentChapter = std::move(newChapter);
                } else {
                    // First chapter — update the existing chapter's number
                    currentChapter.chapterNum = std::stoi(rest);
                }

                // Parse chapter label from rest
                size_t numEnd = 0;
                while (numEnd < rest.size() && std::isdigit(static_cast<unsigned char>(rest[numEnd]))) {
                    numEnd++;
                }
                std::string chapterNumStr = rest.substr(0, numEnd);

                Segment label;
                label.type = SegmentType::ChapterLabel;
                label.level = 0;
                label.text = chapterNumStr;
                currentChapter.segments.push_back(label);

                currentVerse = 0;
                segmentStartWordIndex = 0;
                segmentVerseStart = 0;
                currentSegType = SegmentType::VerseText;
                currentSegLevel = 0;
                continue;
            }

            if (marker == "v") {
                // Verse marker: \v N
                flushSegment();

                // Extract verse number from rest
                size_t numEnd = 0;
                while (numEnd < rest.size() && std::isdigit(static_cast<unsigned char>(rest[numEnd]))) {
                    numEnd++;
                }
                if (numEnd > 0) {
                    currentVerse = std::stoi(rest.substr(0, numEnd));
                }

                // Rest after verse number is text
                std::string verseText = (numEnd < rest.size()) ? rest.substr(numEnd) : "";
                while (!verseText.empty() && verseText[0] == ' ') {
                    verseText = verseText.substr(1);
                }

                segmentStartWordIndex = currentChapter.words.size();
                segmentVerseStart = currentVerse;
                currentSegType = SegmentType::VerseText;
                currentSegLevel = 0;
                inDescription = false;

                if (!verseText.empty()) {
                    TokenizeToWords(verseText, currentVerse, currentChapter.words, currentWords);
                }
                continue;
            }

            if (marker == "p" || marker == "m") {
                // Paragraph break
                flushSegment();
                inDescription = false;

                Segment pb;
                pb.type = SegmentType::ParagraphBreak;
                pb.level = 0;
                currentChapter.segments.push_back(pb);

                if (!rest.empty()) {
                    segmentStartWordIndex = currentChapter.words.size();
                    segmentVerseStart = currentVerse;
                    currentSegType = SegmentType::VerseText;
                    currentSegLevel = 0;
                    TokenizeToWords(rest, currentVerse, currentChapter.words, currentWords);
                }
                continue;
            }

            if (marker == "s1" || marker == "s2" || marker == "s3" || marker == "s4" || marker == "s5") {
                flushSegment();
                inDescription = false;

                int level = marker[1] - '0';

                Segment heading;
                heading.type = SegmentType::SectionHeading;
                heading.level = level;
                heading.text = rest;
                currentChapter.segments.push_back(heading);
                continue;
            }

            if (marker == "d") {
                // Descriptive title (Psalm headings) → SectionHeading level 1
                flushSegment();
                inDescription = true;

                Segment heading;
                heading.type = SegmentType::SectionHeading;
                heading.level = 1;
                heading.text = rest;
                currentChapter.segments.push_back(heading);
                continue;
            }

            if (marker == "r") {
                // Parallel reference heading → SectionHeading level 1
                flushSegment();

                Segment heading;
                heading.type = SegmentType::SectionHeading;
                heading.level = 1;
                heading.text = rest;
                currentChapter.segments.push_back(heading);
                continue;
            }

            if (marker == "q" || marker == "q1" || marker == "q2" || marker == "q3") {
                flushSegment();
                inDescription = false;

                int level = 1;
                if (marker.size() > 1) {
                    level = marker[1] - '0';
                    if (level < 1 || level > 3) level = 1;
                }

                segmentStartWordIndex = currentChapter.words.size();
                segmentVerseStart = currentVerse;
                currentSegType = SegmentType::PoetryLine;
                currentSegLevel = level;

                if (!rest.empty()) {
                    TokenizeToWords(rest, currentVerse, currentChapter.words, currentWords);
                }
                continue;
            }

            if (marker == "mt1" || marker == "mt2" || marker == "mt3" || marker == "mt4") {
                flushSegment();
                inDescription = false;

                int level = marker[2] - '0';

                Segment title;
                title.type = SegmentType::BookTitle;
                title.level = level;
                title.text = rest;
                currentChapter.segments.push_back(title);
                continue;
            }

            // Unknown marker — skip line (with warning for known ones we don't support)
            continue;
        }

        // Plain text line (continuation of current segment)
        if (!inDescription && currentChapter.chapterNum > 0 && currentVerse > 0) {
            TokenizeToWords(line, currentVerse, currentChapter.words, currentWords);
        }
    }

    // Flush last segment and chapter
    flushSegment();
    if (currentChapter.chapterNum > 0) {
        chapters.push_back(std::move(currentChapter));
    }

    bookCache[bookId] = chapters;
    return chapters;
}

bool USFMParser::HasChapter(const std::string& bookId, int chapter) const {
    auto key = bookId + "." + std::to_string(chapter);
    auto cached = cachedHasChapter.find(key);
    if (cached != cachedHasChapter.end()) {
        return cached->second;
    }

    auto chapters = parseBook(bookId);
    for (const auto& ch : chapters) {
        if (ch.chapterNum == chapter) {
            cachedHasChapter[key] = true;
            return true;
        }
    }

    cachedHasChapter[key] = false;
    return false;
}

std::optional<ChapterData> USFMParser::LoadChapter(
        const std::string& bookId, int chapter) {
    auto chapters = parseBook(bookId);
    for (auto& ch : chapters) {
        if (ch.chapterNum == chapter) {
            return std::move(ch);
        }
    }
    return std::nullopt;
}
