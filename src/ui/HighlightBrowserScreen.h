#ifndef HIGHLIGHT_BROWSER_SCREEN_H
#define HIGHLIGHT_BROWSER_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include <vector>
#include <string>
#include <memory>
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
    ~HighlightBrowserScreen();
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Highlights"; }

    struct ItemLayout {
        float height;
        std::vector<std::string> subtitleLines;
    };

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    const theword::highlight::Highlighter& highlighter_;
    const theword::core::UIScale& uiScale_;

    int activeColorId_ = 0;
    float scrollY_ = 0.0f;

    struct DisplayItem {
        const theword::highlight::Highlight* hl;
        std::string title;
        std::string subtitle;
    };

    mutable std::vector<DisplayItem> cachedItems_;
    mutable int cachedFilterColorId_ = -1;
    mutable size_t cachedHighlightsCount_ = 0;
    mutable std::vector<ItemLayout> layouts_;
    mutable float lastTextWrapWidth_ = 0;
    Vector2 pressStartPos_{};
    bool hasPendingPress_ = false;

    std::shared_ptr<bool> aliveGuard_ = std::make_shared<bool>(true);

    const std::vector<DisplayItem>& GetFilteredItems() const;
    void RebuildLayouts(float textWrapWidth) const;
    void OnItemTapped(int index);
    void NavigateToHighlight(const theword::highlight::Highlight& h);
};

} // namespace theword::ui

#endif
