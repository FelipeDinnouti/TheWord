#include "DocumentManager.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "text/LayoutEngine.h"
#include "core/BibleBooks.h"
#include "core/Logger.h"
#include <algorithm>
#include <cmath>
#include <chrono>

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

    if (e.direct) {
        scrollY += e.delta;
        if (scrollY < 0.0f) scrollY = 0.0f;
        if (scrollY > maxScroll) scrollY = maxScroll;
        targetScrollY = scrollY;
    } else {
        float newTarget = targetScrollY + e.delta;
        if (newTarget < 0.0f) newTarget = 0.0f;
        if (newTarget > maxScroll) newTarget = maxScroll;
        targetScrollY = newTarget;
    }

    float absDelta = std::abs(e.delta);
    avgScrollSpeed_ = avgScrollSpeed_ * 0.9f + absDelta * 0.1f;
}

bool DocumentManager::HasPendingLoads() const {
    if (initialLoadFuture_) return true;
    for (const auto& p : pendingLoads_) {
        if (!p.inserted) return true;
    }
    return false;
}

void DocumentManager::UpdateLoadTime(float ms) {
    recentLoadTimes_[loadTimeIndex_ % 5] = ms;
    loadTimeIndex_++;
    int count = std::min(loadTimeIndex_, 5);
    float sum = 0.0f;
    for (int i = 0; i < count; i++) sum += recentLoadTimes_[i];
    float margin = (sum / count) / 1000.0f * avgScrollSpeed_ * 1.5f;
    autoLoadMargin_ = std::clamp(margin, MIN_LOAD_MARGIN, MAX_LOAD_MARGIN);
}

void DocumentManager::OnResize(const theword::event::ResizeEvent& e) {
    float newContentWidth = static_cast<float>(e.width);
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
    // Move any in-flight initial load to graveyard
    if (initialLoadFuture_) {
        pendingGraveyard_.push_back(
            PendingLoad{visibleChapterId_, std::move(*initialLoadFuture_), false, false}
        );
        initialLoadFuture_.reset();
    }

    visibleChapterId_ = chapterId;
    chapters.clear();
    for (auto& p : pendingLoads_) {
        pendingGraveyard_.push_back(std::move(p));
    }
    pendingLoads_.clear();

    std::string book;
    int chapter;
    if (!ParseChapterRef(chapterId, book, chapter)) {
        Logger::Warning("LoadInitialChapter: failed to parse ref: " + chapterId);
        return;
    }

    auto& provider = primaryProvider;
    auto& mutex = providerMutex_;

    auto future = std::async(std::launch::async,
        [&provider, &mutex, book, chapter]()
            -> std::optional<ChapterData>
    {
        std::lock_guard<std::mutex> lock(mutex);
        return provider.LoadChapter(book, chapter);
    });

    initialLoadFuture_ = std::move(future);
}

void DocumentManager::LoadInitialChapterSync(const std::string& chapterId) {
    if (initialLoadFuture_) {
        pendingGraveyard_.push_back(
            PendingLoad{visibleChapterId_, std::move(*initialLoadFuture_), false, false}
        );
        initialLoadFuture_.reset();
    }

    visibleChapterId_ = chapterId;
    chapters.clear();
    for (auto& p : pendingLoads_) {
        pendingGraveyard_.push_back(std::move(p));
    }
    pendingLoads_.clear();

    std::string book;
    int chapter;
    if (!ParseChapterRef(chapterId, book, chapter)) {
        Logger::Warning("LoadInitialChapterSync: failed to parse ref: " + chapterId);
        return;
    }

    std::optional<ChapterData> data;
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        data = primaryProvider.LoadChapter(book, chapter);
    }
    if (!data) {
        Logger::Warning("LoadInitialChapterSync: provider returned null for " + chapterId);
        return;
    }

    ChapterLayout layout = layoutEngine.LayoutChapter(chapterId, *data);

    LoadedChapter lc;
    lc.chapterId = chapterId;
    lc.data = std::move(*data);
    lc.layout = std::move(layout);
    lc.startY = 0.0f;
    lc.height = lc.layout.totalHeight;

    chapters.push_back(std::move(lc));

    scrollY = 0.0f;
    targetScrollY = 0.0f;

    Logger::Info("Loaded chapter: " + chapterId);
}

void DocumentManager::Update(float deltaTime) {
    // Check if async initial chapter load has completed
    if (initialLoadFuture_) {
        if ((*initialLoadFuture_).wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto data = (*initialLoadFuture_).get();
            initialLoadFuture_.reset();
            if (data) {
                chapters.clear();
                for (auto& p : pendingLoads_) {
                    pendingGraveyard_.push_back(std::move(p));
                }
                pendingLoads_.clear();

                ChapterLayout layout = layoutEngine.LayoutChapter(visibleChapterId_, *data);
                LoadedChapter lc;
                lc.chapterId = visibleChapterId_;
                lc.data = std::move(*data);
                lc.layout = std::move(layout);
                lc.startY = 0.0f;
                lc.height = layout.totalHeight;
                chapters.push_back(std::move(lc));
                scrollY = 0.0f;
                targetScrollY = 0.0f;

                Logger::Info("Loaded chapter: " + visibleChapterId_);
                eventBus_.Emit(theword::event::ChapterLoadedEvent{visibleChapterId_});
            }
        }
    }

    float diff = targetScrollY - scrollY;
    if (std::abs(diff) > 0.5f) {
        scrollY += diff * (1.0f - std::exp(-SMOOTH_SPEED * deltaTime));
    } else {
        scrollY = targetScrollY;
    }

    // Process one completed pending load per frame
    bool loaded = false;
    for (auto& pending : pendingLoads_) {
        if (pending.inserted) continue;
        if (pending.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            continue;

        auto data = pending.future.get();
        pending.inserted = true;
        if (!data) break;

        auto t0 = std::chrono::steady_clock::now();

        ChapterLayout layout = layoutEngine.LayoutChapter(pending.chapterId, *data);
        LoadedChapter lc;
        lc.chapterId = pending.chapterId;
        lc.data = std::move(*data);
        lc.layout = std::move(layout);
        lc.height = layout.totalHeight;

        if (pending.prepend) {
            float h = lc.height;
            chapters.insert(chapters.begin(), std::move(lc));
            RecalculateChapterPositions();
            targetScrollY += h;
        } else {
            float lastEnd = chapters.empty() ? 0.0f
                : chapters.back().startY + chapters.back().height;
            lc.startY = lastEnd;
            chapters.push_back(std::move(lc));
        }

        auto t1 = std::chrono::steady_clock::now();
        float insertMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        UpdateLoadTime(insertMs);
        eventBus_.Emit(theword::event::ScrollStopEvent{});
        loaded = true;
        break;
    }

    // Clean up finished pendings
    pendingLoads_.erase(std::remove_if(pendingLoads_.begin(), pendingLoads_.end(),
        [](const auto& p) { return p.inserted; }), pendingLoads_.end());

    // Drain graveyard non-blockingly — only destroy futures that are already ready
    pendingGraveyard_.erase(
        std::remove_if(pendingGraveyard_.begin(), pendingGraveyard_.end(),
            [](const auto& p) {
                return p.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        pendingGraveyard_.end());

    // Only trigger new loads if no pending just completed
    if (!loaded) {
        if (scrollY <= autoLoadMargin_) {
            TryLoadAdjacent(true);
        }
        float maxScroll = GetTotalHeight() - viewportHeight;
        if (maxScroll < 0.0f) maxScroll = 0.0f;
        if (scrollY >= maxScroll - autoLoadMargin_) {
            TryLoadAdjacent(false);
        }
    }

    UpdateVisibleChapter();
}

void DocumentManager::UpdateVisibleChapter() {
    if (chapters.empty()) return;
    for (const auto& ch : chapters) {
        if (scrollY >= ch.startY - 0.5f && scrollY < ch.startY + ch.height - 0.5f) {
            if (visibleChapterId_ != ch.chapterId) {
                Logger::Info("Visible chapter: " + ch.chapterId);
                visibleChapterId_ = ch.chapterId;
                eventBus_.Emit(theword::event::ChapterLoadedEvent{visibleChapterId_});
            }
            return;
        }
    }
    if (chapters.size() == 1) {
        if (visibleChapterId_ != chapters.back().chapterId) {
            Logger::Info("Visible chapter: " + chapters.back().chapterId);
            visibleChapterId_ = chapters.back().chapterId;
        }
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

void DocumentManager::ScrollTo(float y) {
    float maxScroll = std::max(0.0f, GetTotalHeight() - viewportHeight);
    targetScrollY = std::clamp(y, 0.0f, maxScroll);
}

void DocumentManager::InvalidateLayouts() {
    if (initialLoadFuture_) {
        pendingGraveyard_.push_back(
            PendingLoad{visibleChapterId_, std::move(*initialLoadFuture_), false, false}
        );
        initialLoadFuture_.reset();
    }
    for (auto& p : pendingLoads_) {
        pendingGraveyard_.push_back(std::move(p));
    }
    pendingLoads_.clear();
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

const theword::data::ChapterLayout* DocumentManager::GetCurrentLayout() const {
    for (const auto& ch : chapters) {
        if (ch.chapterId == visibleChapterId_) {
            return &ch.layout;
        }
    }
    return nullptr;
}

const theword::data::ChapterData* DocumentManager::GetCurrentChapterData() const {
    for (const auto& ch : chapters) {
        if (ch.chapterId == visibleChapterId_) {
            return &ch.data;
        }
    }
    return nullptr;
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

    // Don't queue duplicate pending loads
    for (auto& p : pendingLoads_) {
        if (p.chapterId == adjacent && !p.inserted) return false;
    }

    std::string book;
    int chapter;
    if (!ParseChapterRef(adjacent, book, chapter)) return false;

    auto& provider = primaryProvider;
    auto& mutex = providerMutex_;

    auto future = std::async(std::launch::async,
        [&provider, &mutex, book, chapter]()
            -> std::optional<ChapterData>
    {
        std::lock_guard<std::mutex> lock(mutex);
        return provider.LoadChapter(book, chapter);
    });

    pendingLoads_.push_back({adjacent, std::move(future), false, prepend});
    return true;
}

} // namespace theword::document
