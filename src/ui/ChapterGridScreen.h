#ifndef CHAPTER_GRID_SCREEN_H
#define CHAPTER_GRID_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include <string>
#include <memory>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class ChapterGridScreen : public Screen {
public:
    ChapterGridScreen(const Font& font, float fontSize,
                      NavigationStack& navStack,
                      theword::event::EventBus& eventBus,
                      const std::string& bookCode,
                      const std::string& bookName,
                      int chapterCount,
                      const theword::core::UIScale& uiScale,
                      int currentChapter = 1);
    ~ChapterGridScreen() override;
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return bookName_.c_str(); }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    std::string bookCode_;
    std::string bookName_;
    int chapterCount_;
    const theword::core::UIScale& uiScale_;

    int selectedChapter_ = 1;
    bool hasCurrent_ = false;
    float gridScrollY_ = 0.0f;

    void KeepSelectionVisible(int columns, float rowH, float visibleH);

    Vector2 pressStartPos_{};
    bool hasPendingPress_ = false;

    std::shared_ptr<bool> aliveGuard_ = std::make_shared<bool>(true);
};

} // namespace theword::ui

#endif
