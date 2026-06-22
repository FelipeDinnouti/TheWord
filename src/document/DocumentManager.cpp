#include "DocumentManager.h"
#include "../core/BibleBooks.h"
#include <algorithm>
#include <cmath>

DocumentManager::DocumentManager(LayoutEngine& engine, float viewportHeight,
                                  ChapterProvider& provider, float contentTop)
    : layoutEngine(engine)
    , primaryProvider(provider)
    , scrollY(0.0f)
    , targetScrollY(0.0f)
    , viewportHeight(viewportHeight)
    , contentTop(contentTop) {}

void DocumentManager::loadInitialChapter(const std::string& chapterId) {
    chapters.clear();

    std::string book;
    int chapter;
    if (!ParseChapterRef(chapterId, book, chapter)) return;

    auto result = primaryProvider.LoadChapter(book, chapter);
    if (!result) return;

    ChapterLayout layout = layoutEngine.layoutChapter(chapterId, *result);

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(*result);
    lc.layout = std::move(layout);
    lc.startY = 0.0f;
    lc.height = lc.layout.totalHeight;

    chapters.push_back(std::move(lc));

    scrollY = 0.0f;
    targetScrollY = 0.0f;
}

void DocumentManager::update(float deltaTime) {
    float diff = targetScrollY - scrollY;
    if (std::abs(diff) > 0.5f) {
        scrollY += diff * SMOOTH_SPEED * deltaTime;
    } else {
        scrollY = targetScrollY;
    }

    if (scrollY <= 0.0f) {
        tryPrepend();
    }

    float maxScroll = getTotalHeight() - viewportHeight;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    if (scrollY >= maxScroll - AUTO_LOAD_MARGIN) {
        tryAppend();
    }
}

void DocumentManager::scrollBy(float delta) {
    float maxScroll = getTotalHeight() - viewportHeight;
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    float newTarget = targetScrollY + delta;
    if (newTarget < 0.0f) newTarget = 0.0f;
    if (newTarget > maxScroll) newTarget = maxScroll;

    targetScrollY = newTarget;
}

float DocumentManager::getScrollY() const {
    return scrollY;
}

float DocumentManager::getTotalHeight() const {
    if (chapters.empty()) return 0.0f;
    return chapters.back().startY + chapters.back().height;
}

float DocumentManager::getViewportHeight() const {
    return viewportHeight;
}

void DocumentManager::setViewportHeight(float height) {
    viewportHeight = height;
}

void DocumentManager::invalidateLayouts() {
    layoutEngine.invalidateCache();
    for (auto& chapter : chapters) {
        chapter.layout = layoutEngine.layoutChapter(chapter.chapterId, chapter.data);
        chapter.height = chapter.layout.totalHeight;
    }
    recalculateChapterPositions();
}

void DocumentManager::getVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const {
    docSpans.clear();

    float visibleTop = scrollY - viewportHeight;
    float visibleBottom = scrollY + viewportHeight * 2;

    for (const auto& chapter : chapters) {
        float chapterTop = chapter.startY;
        float chapterBottom = chapter.startY + chapter.height;

        if (chapterBottom < visibleTop || chapterTop > visibleBottom) continue;

        for (const auto& line : chapter.layout.lines) {
            float lineTop = chapter.startY + line.y;
            float lineBottom = lineTop + line.height;

            if (lineBottom < visibleTop || lineTop > visibleBottom) continue;

            float baseY = chapter.startY;
            for (const auto& span : line.spans) {
                float spanDocY = baseY + line.y + (line.height - span.height) / 2.0f;
                docSpans.push_back({span, spanDocY});
            }
        }
    }
}

int DocumentManager::hitTestWord(float screenX, float screenY, float scrollY) const {
    float docY = screenY + scrollY - contentTop;
    for (const auto& chapter : chapters) {
        if (docY < chapter.startY || docY > chapter.startY + chapter.height) continue;
        float relY = docY - chapter.startY;
        return layoutEngine.hitTestLine(chapter.layout, relY, screenX);
    }
    return -1;
}

const std::string& DocumentManager::getCurrentChapterId() const {
    static const std::string empty = "";
    if (chapters.empty()) return empty;
    return chapters.front().chapterId;
}

std::string DocumentManager::getChapterTitle() const {
    if (chapters.empty()) return "";
    return ChapterIdToTitle(chapters.front().chapterId);
}

void DocumentManager::recalculateChapterPositions() {
    float currentY = 0.0f;
    for (auto& chapter : chapters) {
        chapter.startY = currentY;
        currentY += chapter.height;
    }
}

bool DocumentManager::tryPrepend() {
    if (chapters.empty()) return false;

    const std::string& firstId = chapters.front().chapterId;
    std::string prev = GetPreviousChapter(firstId);
    if (prev.empty()) return false;

    std::string book;
    int chapter;
    if (!ParseChapterRef(prev, book, chapter)) return false;

    auto result = primaryProvider.LoadChapter(book, chapter);
    if (!result) return false;

    prependChapter(prev, std::move(*result));
    return true;
}

bool DocumentManager::tryAppend() {
    if (chapters.empty()) return false;

    const std::string& lastId = chapters.back().chapterId;
    std::string next = GetNextChapter(lastId);
    if (next.empty()) return false;

    std::string book;
    int chapter;
    if (!ParseChapterRef(next, book, chapter)) return false;

    auto result = primaryProvider.LoadChapter(book, chapter);
    if (!result) return false;

    appendChapter(next, std::move(*result));
    return true;
}

void DocumentManager::prependChapter(const std::string& chapterId, ChapterData&& data) {
    ChapterLayout layout = layoutEngine.layoutChapter(chapterId, data);
    float prependHeight = layout.totalHeight;

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(data);
    lc.layout = std::move(layout);
    lc.height = prependHeight;

    chapters.insert(chapters.begin(), std::move(lc));
    recalculateChapterPositions();

    targetScrollY += prependHeight;
    scrollY = targetScrollY;
}

void DocumentManager::appendChapter(const std::string& chapterId, ChapterData&& data) {
    ChapterLayout layout = layoutEngine.layoutChapter(chapterId, data);

    float lastEnd = 0.0f;
    if (!chapters.empty()) {
        const LoadedChapter& last = chapters.back();
        lastEnd = last.startY + last.height;
    }

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(data);
    lc.layout = std::move(layout);
    lc.startY = lastEnd;
    lc.height = lc.layout.totalHeight;

    chapters.push_back(std::move(lc));
}
