#ifndef CHAPTER_GRID_SCREEN_H
#define CHAPTER_GRID_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "TapDetector.h"
#include <string>
#include <memory>
#include <raylib.h>

namespace theword::event { class EventBus; struct ScrollEvent; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class ChapterGridScreen : public Screen {
public:
    ChapterGridScreen(NavigationStack& navStack,
                      theword::event::EventBus& eventBus,
                      const std::string& bookCode, const std::string& bookName, int chapterCount,
                      const theword::core::UIScale& uiScale, int currentChapter);
    ~ChapterGridScreen() override;
    void Draw(theword::renderer::DrawContext& ctx) override;
    bool HandleInput(const theword::renderer::DrawContext& ctx, float deltaTime) override;
    const char* GetTitle() const override { return "ChapterGrid"; }

private:
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    std::string bookCode_;
    std::string bookName_;
    int chapterCount_;
    const theword::core::UIScale& uiScale_;

    int selectedChapter_ = 1;
    float gridScrollY_ = 0.0f;

    void KeepSelectionVisible(int columns, float rowH, float visibleH);

    TapDetector tapDetector_;

    std::shared_ptr<bool> aliveGuard_ = std::make_shared<bool>(true);
};

} // namespace theword::ui

#endif
