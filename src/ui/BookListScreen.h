#ifndef BOOK_LIST_SCREEN_H
#define BOOK_LIST_SCREEN_H

#include "Screen.h"
#include <string>
#include <vector>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class BookListScreen : public Screen {
public:
    BookListScreen(const Font& font, float fontSize,
                   NavigationStack& navStack,
                   theword::event::EventBus& eventBus);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Books"; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;

    std::string search_;
    int scrollOffset_ = 0;
    int selection_ = 0;
    static constexpr float HEADER_HEIGHT = 60.0f;
    static constexpr float SEARCH_HEIGHT = 40.0f;
    static constexpr float ITEM_HEIGHT = 36.0f;
    static constexpr float BACK_AREA_WIDTH = 80.0f;

    std::vector<int> GetFilteredIndices() const;
    int GetVisibleCount(float listHeight) const;
};

} // namespace theword::ui

#endif
