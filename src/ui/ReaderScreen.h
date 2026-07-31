#ifndef READER_SCREEN_H
#define READER_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/ThemeManager.h"
#include <vector>
#include <string>
#include <raylib.h>

namespace theword::text { struct Span; }
namespace theword::event { class EventBus; struct ScrollEvent; struct NavigateToHighlightEvent; }
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
                   const theword::core::UIScale& uiScale, float& currentFontSize, int& currentBibleId, bool& immersiveMode,
                   const theword::core::ThemeManager& themeManager);
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
    const theword::core::UIScale& uiScale_;
    float& currentFontSize_;
    int& currentBibleId_;
    bool& immersiveMode_;
    const theword::core::ThemeManager& themeManager_;

    float bottomBarHeight_;
    float bottomMargin_;
    static constexpr float SHOW_HIDE_THRESHOLD = 30.0f;
    static constexpr float ANIMATION_SPEED = 10.0f;
    bool showBottomBar_ = true;
    float barAnimation_ = 0.0f;
    float scrollAccumulator_ = 0.0f;

    int pendingNavigateWordId_ = -1;

    void OnScroll(const theword::event::ScrollEvent& e);
    void OnNavigateToHighlight(const theword::event::NavigateToHighlightEvent& e);
    float FindLineYForWord(int wordId) const;
    void UpdateBottomBar(float deltaTime);
    void DrawBottomBarContent();
    bool HandleBottomBarClick();
    void OpenCenterMenu();
};

} // namespace theword::ui

#endif
