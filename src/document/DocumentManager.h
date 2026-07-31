#ifndef DOCUMENT_MANAGER_H
#define DOCUMENT_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <future>
#include <optional>
#include "data/ChapterProvider.h"
#include "text/LayoutTypes.h"

namespace theword::text { class LayoutEngine; }

namespace theword::event {
    class EventBus;
    struct ScrollEvent;
    struct ResizeEvent;
    struct FontSizeEvent;
}

namespace theword::document {

struct LoadedChapter {
    std::string chapterId;
    theword::data::ChapterData data;
    theword::text::ChapterLayout layout;
    float startY;
    float height;
};

struct PendingLoad {
    std::string chapterId;
    std::future<std::optional<theword::data::ChapterData>> future;
    bool inserted = false;
    bool prepend;
};

class DocumentManager {
public:
    DocumentManager(theword::event::EventBus& eventBus,
                    theword::text::LayoutEngine& layoutEngine, float viewportHeight,
                    theword::data::ChapterProvider& primaryProvider, float contentTop = 60.0f);

    void LoadInitialChapter(const std::string& chapterId);
    void LoadInitialChapterSync(const std::string& chapterId);

    void Update(float deltaTime);

    float GetScrollY() const;
    float GetTotalHeight() const;
    float GetViewportHeight() const;

    void SetViewportHeight(float height);
    void ScrollTo(float y);
    void InvalidateLayouts();
    bool HasMomentum() const { return momentumActive_; }
    bool HasPendingLoads() const;

    void GetVisibleSpans(std::vector<std::pair<theword::text::Span, float>>& docSpans) const;

    struct HitResult { int wordId = -1; const theword::data::ChapterData* chapterData = nullptr; };
    int HitTestWord(float screenX, float screenY, float scrollY) const;
    HitResult HitTestWithChapter(float screenX, float screenY) const;
    const theword::data::ChapterData* GetChapterDataAtPosition(float screenY) const;

    const std::string& GetCurrentChapterId() const;
    std::string GetChapterTitle() const;
    const theword::text::ChapterLayout* GetCurrentLayout() const;
    const theword::data::ChapterData* GetCurrentChapterData() const;
    const theword::data::ChapterData* GetChapterData(const std::string& bookId, int chapterNum) const;

private:
    theword::event::EventBus& eventBus_;
    theword::text::LayoutEngine& layoutEngine;
    theword::data::ChapterProvider& primaryProvider;

    std::vector<LoadedChapter> chapters;

    std::string visibleChapterId_;
    std::string navigationChapterId_;
    bool autoScrollActive_ = false;
    bool momentumActive_ = false;
    float scrollY;
    float scrollVelocity_ = 0.0f;
    float momentumStartY_ = 0.0f;
    float momentumStartVelocity_ = 0.0f;
    float momentumDuration_ = 0.0f;
    float momentumDistance_ = 0.0f;
    float momentumElapsed_ = 0.0f;
    float viewportHeight;
    const float contentTop;

    static constexpr float DECEL_RATE = 2.358f;   // ln(0.78)/ln(0.9) — Android spline exponent
    static constexpr float INFLEXION = 0.35f;      // spline inflection point
    static constexpr float PHYSICAL_COEFF = 51890.0f; // g * in/m * px/in * 0.84
    static constexpr float FRICTION = 0.008f;      // main tuning knob (higher = shorter scroll)
    static constexpr float MAX_VELOCITY = 7500.0f;
    static constexpr float MIN_LOAD_MARGIN = 30.0f;
    static constexpr float MAX_LOAD_MARGIN = 300.0f;

    float autoLoadMargin_ = 50.0f;
    float avgScrollSpeed_ = 100.0f;
    float recentLoadTimes_[5] = {};
    int loadTimeIndex_ = 0;

    std::vector<PendingLoad> pendingLoads_;
    std::vector<PendingLoad> pendingGraveyard_;
    std::optional<std::future<std::optional<theword::data::ChapterData>>> initialLoadFuture_;
    std::mutex providerMutex_;

    void OnScroll(const theword::event::ScrollEvent& e);
    void OnResize(const theword::event::ResizeEvent& e);
    void OnFontSize(const theword::event::FontSizeEvent& e);

    void RecalculateChapterPositions();
    bool TryLoadAdjacent(bool prepend);
    void UpdateVisibleChapter();
    void UpdateLoadTime(float ms);
};

} // namespace theword::document

#endif // DOCUMENTMANAGER_H
