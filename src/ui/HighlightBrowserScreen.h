#ifndef HIGHLIGHT_BROWSER_SCREEN_H
#define HIGHLIGHT_BROWSER_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include <vector>
#include <string>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; struct Highlight; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class HighlightBrowserScreen : public Screen {
public:
    HighlightBrowserScreen(const Font& font, float fontSize,
                           NavigationStack& navStack,
                           theword::event::EventBus& eventBus,
                           const theword::highlight::Highlighter& highlighter,
                           const theword::core::UIScale& uiScale);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Highlights"; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    const theword::highlight::Highlighter& highlighter_;
    const theword::core::UIScale& uiScale_;

    int activeColorId_ = 0;
    float scrollOffset_ = 0.0f;

    struct DisplayItem {
        const theword::highlight::Highlight* hl;
        std::string title;
        std::string subtitle;
    };

    mutable std::vector<DisplayItem> cachedItems_;
    mutable int cachedFilterColorId_ = -1;
    mutable size_t cachedHighlightsCount_ = 0;

    const std::vector<DisplayItem>& GetFilteredItems() const;
    void OnItemTapped(int index);
    void NavigateToHighlight(const theword::highlight::Highlight& h);
};

} // namespace theword::ui

#endif
