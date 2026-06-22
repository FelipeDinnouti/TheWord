#ifndef DocumentManager_h
#define DocumentManager_h

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

    void loadInitialChapter(const std::string& chapterId);

    void update(float deltaTime);
    void scrollBy(float delta);

    float getScrollY() const;
    float getTotalHeight() const;
    float getViewportHeight() const;

    void setViewportHeight(float height);
    void invalidateLayouts();

    void getVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const;

    int hitTestWord(float screenX, float screenY, float scrollY) const;

    const std::string& getCurrentChapterId() const;
    std::string getChapterTitle() const;

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

    void recalculateChapterPositions();
    bool tryPrepend();
    bool tryAppend();
    void prependChapter(const std::string& chapterId, ChapterData&& data);
    void appendChapter(const std::string& chapterId, ChapterData&& data);
};

#endif
