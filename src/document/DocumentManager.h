#ifndef DOCUMENTMANAGER_H
#define DOCUMENTMANAGER_H

#include <string>
#include <vector>
#include "../text/LayoutEngine.h"
#include "../data/ChapterProvider.h"

class LayoutEngine;
class ChapterProvider;

struct LoadedChapter {
    std::string chapterId;
    ChapterData data;
    ChapterLayout layout;
    float startY;
    float height;
};

class DocumentManager {
public:
    DocumentManager(LayoutEngine& layoutEngine, float viewportHeight,
                    ChapterProvider& primaryProvider, float contentTop = 60.0f);

    void LoadInitialChapter(const std::string& chapterId);

    void Update(float deltaTime);
    void ScrollBy(float delta);
    void ScrollTo(float y);

    float GetScrollY() const;
    float GetTotalHeight() const;
    float GetViewportHeight() const;

    void SetViewportHeight(float height);
    void InvalidateLayouts();

    void GetVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const;

    int HitTestWord(float screenX, float screenY, float scrollY) const;

    const std::string& GetCurrentChapterId() const;
    std::string GetChapterTitle() const;

private:
    LayoutEngine& layoutEngine;
    ChapterProvider& primaryProvider;

    std::vector<LoadedChapter> chapters;

    float scrollY;
    float targetScrollY;
    float viewportHeight;
    float contentTop;

    static constexpr float SMOOTH_SPEED = 8.0f;
    static constexpr float AUTO_LOAD_MARGIN = 50.0f;

    void RecalculateChapterPositions();
    bool TryLoadAdjacent(bool prepend);
    void PrependChapter(const std::string& chapterId, ChapterData&& data);
    void AppendChapter(const std::string& chapterId, ChapterData&& data);
};

#endif // DOCUMENTMANAGER_H
