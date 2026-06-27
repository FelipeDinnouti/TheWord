#include "DocumentManager.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "text/LayoutEngine.h"
#include "core/BibleBooks.h"
#include "core/Logger.h"
#include <algorithm>
#include <cmath>

namespace theword::document {

using namespace theword::core;
using namespace theword::data;

DocumentManager::DocumentManager(theword::event::EventBus& eventBus,
                                  theword::text::LayoutEngine& engine, float viewportHeight,
                                  theword::data::ChapterProvider& provider, float contentTop)
    : eventBus_(eventBus)
    , layoutEngine(engine)
    , primaryProvider(provider)
    , scrollY(0.0f)
    , targetScrollY(0.0f)
    , viewportHeight(viewportHeight)
    , contentTop(contentTop) {

    eventBus_.On<theword::event::ScrollEvent>([this](const auto& e) { OnScroll(e); });
    eventBus_.On<theword::event::ResizeEvent>([this](const auto& e) { OnResize(e); });
    eventBus_.On<theword::event::FontSizeEvent>([this](const auto& e) { OnFontSize(e); });
    eventBus_.On<theword::event::SourceSwitchEvent>([this](const auto& e) { OnSourceSwitch(e); });
}

void DocumentManager::OnScroll(const theword::event::ScrollEvent& e) {
    float maxScroll = GetTotalHeight() - viewportHeight;
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    float newTarget = targetScrollY + e.delta;
    if (newTarget < 0.0f) newTarget = 0.0f;
    if (newTarget > maxScroll) newTarget = maxScroll;

    targetScrollY = newTarget;
}

void DocumentManager::OnResize(const theword::event::ResizeEvent& e) {
    float newContentWidth = e.width - 60.0f;
    layoutEngine.SetMaxWidth(newContentWidth);
    layoutEngine.InvalidateCache();

    float scrollFraction = 0.0f;
    if (GetTotalHeight() > 0.0f) {
        scrollFraction = scrollY / GetTotalHeight();
    }

    viewportHeight = e.height - contentTop;
    InvalidateLayouts();

    float newTotal = GetTotalHeight();
    scrollY = scrollFraction * newTotal;
    targetScrollY = scrollY;
}

void DocumentManager::OnFontSize(const theword::event::FontSizeEvent& /*e*/) {
    InvalidateLayouts();
}

void DocumentManager::OnSourceSwitch(const theword::event::SourceSwitchEvent& /*e*/) {
}

void DocumentManager::LoadInitialChapter(const std::string& chapterId) {
    visibleChapterId_ = chapterId;
    chapters.clear();

    std::string book;
    int chapter;
    if (!ParseChapterRef(chapterId, book, chapter)) {
        Logger::Warning("LoadInitialChapter: failed to parse ref: " + chapterId);
        return;
    }

    auto result = primaryProvider.LoadChapter(book, chapter);
    if (!result) {
        Logger::Warning("LoadInitialChapter: provider returned null for " + chapterId);
        return;
    }

    ChapterLayout layout = layoutEngine.LayoutChapter(chapterId, *result);

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(*result);
    lc.layout = std::move(layout);
    lc.startY = 0.0f;
    lc.height = lc.layout.totalHeight;

    chapters.push_back(std::move(lc));

    scrollY = 0.0f;
    targetScrollY = 0.0f;

    Logger::Info("Loaded chapter: " + chapterId);
}

void DocumentManager::Update(float deltaTime) {
    float diff = targetScrollY - scrollY;
    if (std::abs(diff) > 0.5f) {
        scrollY += diff * (1.0f - std::exp(-SMOOTH_SPEED * deltaTime));
    } else {
        scrollY = targetScrollY;
    }

    if (scrollY <= 0.0f) {
        TryLoadAdjacent(true);
    }

    float maxScroll = GetTotalHeight() - viewportHeight;
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    if (scrollY >= maxScroll - AUTO_LOAD_MARGIN) {
        TryLoadAdjacent(false);
    }

    UpdateVisibleChapter();
}

void DocumentManager::UpdateVisibleChapter() {
    if (chapters.empty()) return;
    for (const auto& ch : chapters) {
        if (scrollY >= ch.startY && scrollY < ch.startY + ch.height) {
            if (visibleChapterId_ != ch.chapterId) {
                Logger::Info("Visible chapter: " + ch.chapterId);
                visibleChapterId_ = ch.chapterId;
            }
            return;
        }
    }
    if (visibleChapterId_ != chapters.back().chapterId) {
        Logger::Info("Visible chapter: " + chapters.back().chapterId);
        visibleChapterId_ = chapters.back().chapterId;
    }
}

float DocumentManager::GetScrollY() const {
    return scrollY;
}

float DocumentManager::GetTotalHeight() const {
    if (chapters.empty()) return 0.0f;
    return chapters.back().startY + chapters.back().height;
}

float DocumentManager::GetViewportHeight() const {
    return viewportHeight;
}

void DocumentManager::SetViewportHeight(float height) {
    viewportHeight = height;
}

void DocumentManager::InvalidateLayouts() {
    layoutEngine.InvalidateCache();
    for (auto& chapter : chapters) {
        chapter.layout = layoutEngine.LayoutChapter(chapter.chapterId, chapter.data);
        chapter.height = chapter.layout.totalHeight;
    }
    RecalculateChapterPositions();
}

void DocumentManager::GetVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const {
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

int DocumentManager::HitTestWord(float screenX, float screenY, float scrollY) const {
    float docY = screenY + scrollY - contentTop;
    for (const auto& chapter : chapters) {
        if (docY < chapter.startY || docY > chapter.startY + chapter.height) continue;
        float relY = docY - chapter.startY;
        return layoutEngine.HitTestLine(chapter.layout, relY, screenX);
    }
    return -1;
}

const std::string& DocumentManager::GetCurrentChapterId() const {
    if (!visibleChapterId_.empty()) return visibleChapterId_;
    static const std::string empty = "";
    if (chapters.empty()) return empty;
    return chapters.front().chapterId;
}

std::string DocumentManager::GetChapterTitle() const {
    if (chapters.empty()) return "";
    return ChapterIdToTitle(chapters.front().chapterId);
}

void DocumentManager::RecalculateChapterPositions() {
    float currentY = 0.0f;
    for (auto& chapter : chapters) {
        chapter.startY = currentY;
        currentY += chapter.height;
    }
}

bool DocumentManager::TryLoadAdjacent(bool prepend) {
    if (chapters.empty()) return false;

    const std::string& currentId = prepend ? chapters.front().chapterId : chapters.back().chapterId;
    std::string adjacent = prepend ? GetPreviousChapter(currentId) : GetNextChapter(currentId);
    if (adjacent.empty()) return false;

    std::string book;
    int chapter;
    if (!ParseChapterRef(adjacent, book, chapter)) return false;

    auto result = primaryProvider.LoadChapter(book, chapter);
    if (!result) return false;

    if (prepend) {
        PrependChapter(adjacent, std::move(*result));
    } else {
        AppendChapter(adjacent, std::move(*result));
    }
    return true;
}

void DocumentManager::PrependChapter(const std::string& chapterId, ChapterData&& data) {
    ChapterLayout layout = layoutEngine.LayoutChapter(chapterId, data);
    float prependHeight = layout.totalHeight;

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(data);
    lc.layout = std::move(layout);
    lc.height = prependHeight;

    chapters.insert(chapters.begin(), std::move(lc));
    RecalculateChapterPositions();

    targetScrollY += prependHeight;
    scrollY = targetScrollY;
}

void DocumentManager::AppendChapter(const std::string& chapterId, theword::data::ChapterData&& data) {
    ChapterLayout layout = layoutEngine.LayoutChapter(chapterId, data);

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

} // namespace theword::document
