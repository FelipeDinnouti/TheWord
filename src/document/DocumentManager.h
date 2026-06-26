#ifndef DOCUMENT_MANAGER_H
#define DOCUMENT_MANAGER_H

#include <string>
#include <vector>
#include "data/ChapterProvider.h"

namespace theword::text { class LayoutEngine; }

namespace theword::event {
    class EventBus;
    struct ScrollEvent;
    struct ResizeEvent;
    struct FontSizeEvent;
    struct SourceSwitchEvent;
}

namespace theword::document {

struct LoadedChapter {
    std::string chapterId;
    theword::data::ChapterData data;
    theword::data::ChapterLayout layout;
    float startY;
    float height;
};

class DocumentManager {
public:
    DocumentManager(theword::event::EventBus& eventBus,
                    theword::text::LayoutEngine& layoutEngine, float viewportHeight,
                    theword::data::ChapterProvider& primaryProvider, float contentTop = 60.0f);

    void LoadInitialChapter(const std::string& chapterId);

    void Update(float deltaTime);

    float GetScrollY() const;
    float GetTotalHeight() const;
    float GetViewportHeight() const;

    void SetViewportHeight(float height);
    void InvalidateLayouts();

    void GetVisibleSpans(std::vector<std::pair<theword::data::Span, float>>& docSpans) const;

    int HitTestWord(float screenX, float screenY, float scrollY) const;

    const std::string& GetCurrentChapterId() const;
    std::string GetChapterTitle() const;

private:
    theword::event::EventBus& eventBus_;
    theword::text::LayoutEngine& layoutEngine;
    theword::data::ChapterProvider& primaryProvider;

    std::vector<LoadedChapter> chapters;

    float scrollY;
    float targetScrollY;
    float viewportHeight;
    float contentTop;

    static constexpr float SMOOTH_SPEED = 8.0f;
    static constexpr float AUTO_LOAD_MARGIN = 50.0f;

    void OnScroll(const theword::event::ScrollEvent& e);
    void OnResize(const theword::event::ResizeEvent& e);
    void OnFontSize(const theword::event::FontSizeEvent& e);
    void OnSourceSwitch(const theword::event::SourceSwitchEvent& e);

    void RecalculateChapterPositions();
    bool TryLoadAdjacent(bool prepend);
    void PrependChapter(const std::string& chapterId, theword::data::ChapterData&& data);
    void AppendChapter(const std::string& chapterId, theword::data::ChapterData&& data);
};

} // namespace theword::document

#endif // DOCUMENTMANAGER_H
