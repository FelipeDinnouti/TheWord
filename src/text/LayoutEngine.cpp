#include "LayoutEngine.h"
#include "core/Theme.h"
#include <algorithm>
#include <cmath>

LayoutEngine::LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing, float scaleFactor)
    : maxWidth(maxWidth), font(font), fontSize(fontSize), lineSpacing(lineSpacing),
      leftMargin(10.0f * scaleFactor), rightMargin(10.0f * scaleFactor),
      paragraphGap(8.0f * scaleFactor), headingTopGap(12.0f * scaleFactor),
      headingBottomGap(6.0f * scaleFactor), poetryIndent(20.0f * scaleFactor) {}

float LayoutEngine::GetFontSize() const {
    return fontSize;
}

void LayoutEngine::SetFontSize(float size) {
    fontSize = size;
    InvalidateCache();
}

void LayoutEngine::SetMaxWidth(float width) {
    maxWidth = width;
    InvalidateCache();
}

float LayoutEngine::GetMaxWidth() const {
    return maxWidth;
}

void LayoutEngine::InvalidateCache() {
    cachedLayouts.clear();
}

int LayoutEngine::HitTestLine(const ChapterLayout& layout, float chapterRelativeY, float screenX) const {
    for (const auto& line : layout.lines) {
        if (chapterRelativeY >= line.y && chapterRelativeY < line.y + line.height) {
            for (const auto& span : line.spans) {
                if (screenX >= span.x && screenX <= span.x + span.width) {
                    return span.startWord >= 0 ? span.startWord : -1;
                }
            }
        }
    }
    return -1;
}

ChapterLayout LayoutEngine::LayoutChapter(const std::string& chapterId, const ChapterData& data) {
    ChapterLayout layout;
    layout.chapterId = chapterId;

    auto existing = std::find_if(cachedLayouts.begin(), cachedLayouts.end(),
        [&chapterId](const ChapterLayout& l) { return l.chapterId == chapterId; });

    if (existing != cachedLayouts.end()) {
        return *existing;
    }

    float currentY = 0.0f;

    for (const auto& seg : data.segments) {
        switch (seg.type) {
            case SegmentType::ParagraphBreak:
                currentY += paragraphGap;
                break;

            case SegmentType::SectionHeading:
                currentY += LayoutHeading(seg, currentY, layout.lines, theme::FONT_HEADING);
                break;

            case SegmentType::VerseText:
                currentY += LayoutWords(seg, data, currentY, layout.lines, 0.0f, SegmentType::VerseText);
                break;

            case SegmentType::PoetryLine:
                currentY += LayoutWords(seg, data, currentY, layout.lines,
                                        seg.level * poetryIndent, SegmentType::PoetryLine);
                break;

            case SegmentType::ChapterLabel:
            case SegmentType::BookTitle:
                currentY += LayoutHeading(seg, currentY, layout.lines, theme::FONT_LARGE_HEADING);
                break;
        }
    }

    layout.totalHeight = currentY;

    cachedLayouts.push_back(layout);

    return layout;
}

float LayoutEngine::LayoutWords(const Segment& seg, const ChapterData& data, float startY,
                                std::vector<Line>& lines, float indent, SegmentType spanType) {
    float lineHeight = fontSize * lineSpacing;
    float availableWidth = maxWidth - leftMargin - rightMargin - indent;
    float spaceWidth = MeasureTextEx(font, " ", fontSize, 1).x;
    float y = startY;
    float startX = leftMargin + indent;
    float x = startX;

    Line currentLine;
    currentLine.y = y;
    currentLine.height = lineHeight;

    size_t endIndex = seg.startWordIndex + seg.wordCount;
    for (size_t i = seg.startWordIndex; i < endIndex; ++i) {
        const Word& word = data.words[i];

        float wordWidth = MeasureTextEx(font, word.text.c_str(), fontSize, 1).x;
        float spaceAfterWord = wordWidth;

        if (!currentLine.spans.empty()) {
            spaceAfterWord += spaceWidth;
        }

        if (x + spaceAfterWord > startX + availableWidth && !currentLine.spans.empty()) {
            lines.push_back(currentLine);
            y += lineHeight;
            currentLine = Line();
            currentLine.y = y;
            currentLine.height = lineHeight;
            x = startX;
        }

        if (!currentLine.spans.empty()) {
            x += spaceWidth;
        }

        Span span;
        span.text = word.text;
        span.x = x;
        span.y = y;
        span.width = wordWidth;
        span.height = fontSize;
        span.verseId = word.verseId;
        span.startWord = word.id;
        span.endWord = word.id;
        span.type = spanType;

        currentLine.spans.push_back(span);
        x += wordWidth;
    }

    if (!currentLine.spans.empty()) {
        lines.push_back(currentLine);
        y += lineHeight;
    }

    return y - startY;
}

float LayoutEngine::LayoutHeading(const Segment& seg, float startY, std::vector<Line>& lines, float fontScale) {
    float lineHeight = fontSize * lineSpacing * fontScale;
    float y = startY;

    y += headingTopGap;

    float headingWidth = MeasureTextEx(font, seg.text.c_str(), fontSize * fontScale, 1).x;
    float x = (maxWidth - headingWidth) / 2.0f;

    Line headingLine;
    headingLine.y = y;
    headingLine.height = lineHeight;

    Span span;
    span.text = seg.text;
    span.x = x;
    span.y = y;
    span.width = headingWidth;
    span.height = fontSize * fontScale;
    span.verseId = 0;
    span.startWord = -1;
    span.endWord = -1;
    span.type = seg.type;

    headingLine.spans.push_back(span);
    lines.push_back(headingLine);

    y += lineHeight;
    y += headingBottomGap;

    return y - startY;
}
