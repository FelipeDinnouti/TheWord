#ifndef CHAPTER_GRID_SCREEN_H
#define CHAPTER_GRID_SCREEN_H

#include "Screen.h"
#include <string>
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
                      int chapterCount);
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

    static constexpr float HEADER_HEIGHT = 60.0f;
    static constexpr float GRID_PADDING = 16.0f;
    static constexpr float CELL_WIDTH = 60.0f;
    static constexpr float CELL_HEIGHT = 44.0f;
    static constexpr float CELL_GAP = 8.0f;
    static constexpr int GRID_COLUMNS = 5;
    static constexpr float BACK_AREA_WIDTH = 80.0f;

    int selectedChapter_ = 1;
};

} // namespace theword::ui

#endif
