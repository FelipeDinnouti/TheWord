#include "LayoutEngine.h"
#include <sstream>
#include <algorithm>

LayoutEngine::LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing)
    : maxWidth(maxWidth), font(font), fontSize(fontSize), lineSpacing(lineSpacing) {}

void LayoutEngine::setMaxWidth(float width) {
    maxWidth = width;
    invalidateCache();
}

float LayoutEngine::getMaxWidth() const {
    return maxWidth;
}

void LayoutEngine::invalidateCache() {
    cachedLayouts.clear();
}

std::vector<Word> LayoutEngine::tokenize(const std::string& text) {
    std::vector<Word> words;
    int globalWordId = 0;

    std::istringstream stream(text);
    std::string line;
    int verseId = 1;

    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string word;

        while (lineStream >> word) {
            Word w;
            w.id = globalWordId++;
            w.verseId = verseId;
            w.text = word;
            words.push_back(w);
        }

        if (!line.empty()) {
            verseId++;
        }
    }

    return words;
}

std::vector<Line> LayoutEngine::wrapText(const std::vector<Word>& words) {
    std::vector<Line> lines;
    float lineHeight = fontSize * lineSpacing;
    float currentY = 0.0f;
    float x = 10.0f;

    Line currentLine;
    currentLine.y = currentY;
    currentLine.height = lineHeight;

    for (size_t i = 0; i < words.size(); ++i) {
        const Word& word = words[i];

        float wordWidth = MeasureTextEx(font, word.text.c_str(), fontSize, 1).x;

        float spaceWidth = MeasureTextEx(font, " ", fontSize, 1).x;

        if (x + wordWidth > maxWidth - 10 && !currentLine.spans.empty()) {
            lines.push_back(currentLine);

            currentY += lineHeight;
            currentLine = Line();
            currentLine.y = currentY;
            currentLine.height = lineHeight;
            x = 10.0f;
        }

        if (!currentLine.spans.empty()) {
            x += spaceWidth;
        }

        Span span;
        span.text = word.text;
        span.x = x;
        span.y = currentY;
        span.width = wordWidth;
        span.height = fontSize;
        span.verseId = word.verseId;
        span.startWord = word.id;
        span.endWord = word.id;

        currentLine.spans.push_back(span);
        x += wordWidth;
    }

    if (!currentLine.spans.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}

std::vector<Span> LayoutEngine::createSpansForLine(const Line& line, const std::vector<Word>& words) {
    (void)words;
    return line.spans;
}

std::string LayoutEngine::getWordAtPosition(float x, float y, float scrollY) {
    float documentY = y + scrollY;

    for (const auto& line : [&]() {
        std::vector<Line> allLines;
        for (const auto& layout : cachedLayouts) {
            for (const auto& l : layout.lines) {
                allLines.push_back(l);
            }
        }
        return allLines;
    }()) {
        if (documentY >= line.y && documentY < line.y + line.height) {
            for (const auto& span : line.spans) {
                if (x >= span.x && x <= span.x + span.width) {
                    return span.text;
                }
            }
        }
    }

    return "";
}

ChapterLayout LayoutEngine::layoutChapter(const std::string& chapterId, const std::string& text) {
    ChapterLayout layout;
    layout.chapterId = chapterId;

    auto existing = std::find_if(cachedLayouts.begin(), cachedLayouts.end(),
        [&chapterId](const ChapterLayout& l) { return l.chapterId == chapterId; });

    if (existing != cachedLayouts.end()) {
        return *existing;
    }

    std::vector<Word> words = tokenize(text);
    layout.lines = wrapText(words);

    float totalHeight = 0.0f;
    for (const auto& line : layout.lines) {
        totalHeight += line.height;
    }
    layout.totalHeight = totalHeight;

    cachedLayouts.push_back(layout);

    return layout;
}