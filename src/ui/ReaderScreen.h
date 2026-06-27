#ifndef READER_SCREEN_H
#define READER_SCREEN_H

#include "Screen.h"
#include <vector>
#include <string>
#include <utility>
#include <raylib.h>

namespace theword::data { struct Span; }
namespace theword::event { class EventBus; struct ScrollEvent; }
namespace theword::renderer { class Renderer; struct HighlightRect; }
namespace theword::document { class DocumentManager; }
namespace theword::highlight { class Highlighter; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class ReaderScreen : public Screen {
public:
    ReaderScreen(theword::event::EventBus& eventBus,
                 theword::document::DocumentManager& docManager,
                 theword::renderer::Renderer& renderer,
                 theword::highlight::Highlighter& highlighter,
                 theword::persistence::PersistenceManager& persistence,
                 const Font& uiFont, float uiFontSize,
                 float contentTop,
                 NavigationStack& navStack,
                 float scale, float& currentFontSize, bool& versionOnline);
    ~ReaderScreen() override;
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Reader"; }

private:
    theword::event::EventBus& eventBus_;
    theword::document::DocumentManager& docManager_;
    theword::renderer::Renderer& renderer_;
    theword::highlight::Highlighter& highlighter_;
    theword::persistence::PersistenceManager& persistence_;
    const Font& uiFont_;
    float uiFontSize_;
    float contentTop_;
    NavigationStack& navStack_;
    float scale_;
    float& currentFontSize_;
    bool& versionOnline_;

    // Bottom bar
    static constexpr float BOTTOM_BAR_HEIGHT = 50.0f;
    static constexpr float SHOW_HIDE_THRESHOLD = 30.0f;
    static constexpr float ANIMATION_SPEED = 10.0f;
    bool showBottomBar_ = true;
    float barAnimation_ = 0.0f;
    float scrollAccumulator_ = 0.0f;

    void OnScroll(const theword::event::ScrollEvent& e);
    void UpdateBottomBar(float deltaTime);
    void DrawBottomBarContent();
    bool HandleBottomBarClick();
    void OpenCenterMenu();
};

} // namespace theword::ui

#endif
