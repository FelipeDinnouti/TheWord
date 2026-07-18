#include "LayoutEngine.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>

namespace theword::text {

using namespace theword::data;

LayoutEngine::LayoutEngine(theword::event::EventBus& eventBus,
                           float maxWidth,
                           const Font& bodyFont, float bodySize,
                           const Font& headingFont, float headingSize,
                           const Font& largeFont, float largeSize,
                           const Font& smallFont, float smallSize,
                           float lineSpacing, float scaleFactor)
    : eventBus_(eventBus), maxWidth(maxWidth),
      bodyFont_(bodyFont), headingFont_(headingFont),
      largeFont_(largeFont), smallFont_(smallFont),
      bodySize_(bodySize), headingSize_(headingSize),
      largeSize_(largeSize), smallSize_(smallSize),
      lineSpacing(lineSpacing),
      leftMargin(20.0f * scaleFactor), rightMargin(20.0f * scaleFactor),
      paragraphGap(8.0f * scaleFactor), headingTopGap(12.0f * scaleFactor),
      headingBottomGap(6.0f * scaleFactor), poetryIndent(20.0f * scaleFactor) {

    eventBus_.On<theword::event::ResizeEvent>([this](const auto& e) { OnResize(e); });
}

void LayoutEngine::OnResize(const theword::event::ResizeEvent& e) {
    maxWidth = static_cast<float>(e.width);
    InvalidateCache();
}

float LayoutEngine::GetFontSize() const {
    return bodySize_;
}

void LayoutEngine::SetFontSizes(float body, float heading, float large, float small) {
    bodySize_ = body;
    headingSize_ = heading;
    largeSize_ = large;
    smallSize_ = small;
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
    layoutGeneration_++;
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

ChapterLayout LayoutEngine::LayoutChapter(const std::string& chapterId, const ChapterData& data, bool skipCache) {
    if (!skipCache) {
        auto existing = std::find_if(cachedLayouts.begin(), cachedLayouts.end(),
            [&chapterId](const ChapterLayout& l) { return l.chapterId == chapterId; });
        if (existing != cachedLayouts.end()) {
            return *existing;
        }
    }

    ChapterLayout layout;
    layout.chapterId = chapterId;

    float currentY = 0.0f;

    size_t vtStartIdx = 0;
    size_t vtWordCount = 0;
    bool inVt = false;

    auto flushVt = [&]() {
        if (!inVt) return;
        Segment merged;
        merged.type = SegmentType::VerseText;
        merged.startWordIndex = vtStartIdx;
        merged.wordCount = vtWordCount;
        currentY += LayoutWords(merged, data, currentY, layout.lines, 0.0f, SegmentType::VerseText);
        inVt = false;
    };

    for (const auto& seg : data.segments) {
        switch (seg.type) {
            case SegmentType::ParagraphBreak:
                flushVt();
                currentY += paragraphGap;
                break;

            case SegmentType::SectionHeading:
                flushVt();
                currentY += LayoutHeading(seg, currentY, layout.lines, headingFont_, headingSize_);
                break;

            case SegmentType::VerseText:
                if (!inVt) {
                    inVt = true;
                    vtStartIdx = seg.startWordIndex;
                    vtWordCount = 0;
                }
                vtWordCount += seg.wordCount;
                break;

            case SegmentType::PoetryLine:
                flushVt();
                currentY += LayoutWords(seg, data, currentY, layout.lines,
                                        seg.level * poetryIndent, SegmentType::PoetryLine);
                break;

            case SegmentType::ChapterLabel:
            case SegmentType::BookTitle:
                flushVt();
                currentY += LayoutHeading(seg, currentY, layout.lines, largeFont_, largeSize_);
                break;

            case SegmentType::VerseNumber:
            case SegmentType::FootnoteMarker:
                break;
        }
    }
    flushVt();

    layout.totalHeight = currentY;

    if (!skipCache) {
        cachedLayouts.push_back(layout);
    }

    return layout;
}

float LayoutEngine::LayoutWords(const Segment& seg, const ChapterData& data, float startY,
                                std::vector<Line>& lines, float indent, SegmentType spanType) {
    float lineHeight = bodySize_ * lineSpacing;
    float availableWidth = maxWidth - leftMargin - rightMargin - indent;
    float spaceWidth = MeasureTextEx(bodyFont_, " ", bodySize_, 1).x;
    float y = startY;
    float startX = leftMargin + indent;
    float x = startX;

    Line currentLine;
    currentLine.y = y;
    currentLine.height = lineHeight;

    size_t endIndex = seg.startWordIndex + seg.wordCount;
    int currentVerse = 0;
    for (size_t i = seg.startWordIndex; i < endIndex; ++i) {
        const Word& word = data.words[i];

        if (spanType == SegmentType::VerseText && word.verseId != currentVerse) {
            currentVerse = word.verseId;
            std::string vnText = std::to_string(word.verseId);
            float vnWidth = MeasureTextEx(smallFont_, vnText.c_str(), smallSize_, 1).x;

            float vnWithSpace = vnWidth;
            if (!currentLine.spans.empty()) {
                vnWithSpace += spaceWidth;
            }

            if (x + vnWithSpace > startX + availableWidth && !currentLine.spans.empty()) {
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

            Span vnSpan;
            vnSpan.text = vnText;
            vnSpan.x = x;
            vnSpan.y = y;
            vnSpan.width = vnWidth;
            vnSpan.height = smallSize_;
            vnSpan.verseId = word.verseId;
            vnSpan.startWord = -1;
            vnSpan.endWord = -1;
            vnSpan.bookId = data.bookId;
            vnSpan.chapterNum = data.chapterNum;
            vnSpan.type = SegmentType::VerseNumber;

            currentLine.spans.push_back(vnSpan);
            x += vnWidth;

            // Insert footnote markers for this verse
            bool hasFnForVerse = false;
            for (size_t fi = 0; fi < data.footnotes.size(); ++fi) {
                if (data.footnotes[fi].verseId == currentVerse) {
                    hasFnForVerse = true;
                    break;
                }
            }
            if (hasFnForVerse) {
                x += spaceWidth * 0.3f;
            }
            for (size_t fi = 0; fi < data.footnotes.size(); ++fi) {
                if (data.footnotes[fi].verseId == currentVerse) {
                    Span fnSpan;
                    fnSpan.text = "[" + std::to_string(fi + 1) + "]";
                    fnSpan.type = SegmentType::FootnoteMarker;
                    fnSpan.footnoteIndex = static_cast<int>(fi);
                    fnSpan.verseId = currentVerse;
                    fnSpan.bookId = data.bookId;
                    fnSpan.chapterNum = data.chapterNum;
                    fnSpan.startWord = -1;
                    fnSpan.endWord = -1;
                    float fnWidth = MeasureTextEx(smallFont_, fnSpan.text.c_str(), smallSize_, 1).x;
                    fnSpan.x = x;
                    fnSpan.y = y;
                    fnSpan.width = fnWidth;
                    fnSpan.height = smallSize_;
                    currentLine.spans.push_back(fnSpan);
                    x += fnWidth;
                }
            }
        }

        float wordWidth = MeasureTextEx(bodyFont_, word.text.c_str(), bodySize_, 1).x;
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
        span.height = bodySize_;
        span.verseId = word.verseId;
        span.startWord = word.id;
        span.endWord = word.id;
        span.bookId = data.bookId;
        span.chapterNum = data.chapterNum;
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

float LayoutEngine::LayoutHeading(const Segment& seg, float startY, std::vector<Line>& lines,
                                   const Font& useFont, float renderSize) {
    float lineHeight = renderSize * lineSpacing;
    float y = startY + headingTopGap;

    // Split text into words
    std::string text = seg.text;
    std::vector<std::string> words;
    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && text[pos] == ' ') pos++;
        if (pos >= text.size()) break;
        size_t space = text.find(' ', pos);
        if (space == std::string::npos) {
            words.push_back(text.substr(pos));
            break;
        }
        words.push_back(text.substr(pos, space - pos));
        pos = space + 1;
    }

    if (words.empty()) {
        y += headingBottomGap;
        return y - startY;
    }

    float spaceWidth = MeasureTextEx(useFont, " ", renderSize, 1).x;
    float availableWidth = maxWidth - leftMargin - rightMargin;

    // Wrap words into lines
    struct WrappedLine {
        std::vector<std::string> wordTexts;
        float totalWidth = 0.0f;
    };
    std::vector<WrappedLine> wrappedLines;
    WrappedLine current;
    for (const auto& word : words) {
        float wordWidth = MeasureTextEx(useFont, word.c_str(), renderSize, 1).x;
        float addedGap = current.wordTexts.empty() ? 0.0f : spaceWidth;
        if (current.totalWidth + addedGap + wordWidth > availableWidth && !current.wordTexts.empty()) {
            wrappedLines.push_back(current);
            current = WrappedLine();
            current.wordTexts.push_back(word);
            current.totalWidth = wordWidth;
        } else {
            current.wordTexts.push_back(word);
            current.totalWidth += addedGap + wordWidth;
        }
    }
    if (!current.wordTexts.empty()) wrappedLines.push_back(current);

    // Render each wrapped line centered
    for (const auto& wl : wrappedLines) {
        Line headingLine;
        headingLine.y = y;
        headingLine.height = lineHeight;

        float x = (maxWidth - wl.totalWidth) / 2.0f;
        float wordX = x;
        for (const auto& wt : wl.wordTexts) {
            float ww = MeasureTextEx(useFont, wt.c_str(), renderSize, 1).x;

            Span span;
            span.text = wt;
            span.x = wordX;
            span.y = y;
            span.width = ww;
            span.height = renderSize;
            span.verseId = 0;
            span.startWord = -1;
            span.endWord = -1;
            span.type = seg.type;

            headingLine.spans.push_back(span);
            wordX += ww + spaceWidth;
        }

        lines.push_back(headingLine);
        y += lineHeight;
    }

    y += headingBottomGap;
    return y - startY;
}

} // namespace theword::text
