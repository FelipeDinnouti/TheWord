#ifndef CENTER_MENU_H
#define CENTER_MENU_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include "core/ThemeManager.h"
#include "TapDetector.h"
#include <memory>
#include <string>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class CenterMenu : public Screen {
public:
    CenterMenu(const Font& font, float fontSize,
               NavigationStack& navStack,
               theword::event::EventBus& eventBus,
               theword::highlight::Highlighter& highlighter,
               theword::persistence::PersistenceManager& persistence,
               const theword::core::UIScale& uiScale,
               float& currentFontSize, int& currentBibleId, bool& immersiveMode,
               const theword::core::ThemeManager& themeManager,
               const std::string& currentChapterRef = "");
    void Draw(theword::renderer::DrawContext& ctx) override;
    bool HandleInput(const theword::renderer::DrawContext& ctx, float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Menu"); }
    bool IsOverlay() const override { return true; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    theword::highlight::Highlighter& highlighter_;
    theword::persistence::PersistenceManager& persistence_;
    const theword::core::UIScale& uiScale_;
    float& currentFontSize_;
    int& currentBibleId_;
    bool& immersiveMode_;
    const theword::core::ThemeManager& themeManager_;

    void HandleAction(int action);
    static const char* ItemLabel(int idx);

    int selectedIndex_ = 0;
    double showTime_;
    double fadeOutStartTime_ = 0;
    bool fadingOut_ = false;
    bool popPending_ = false;
    std::string currentChapterRef_;
    static constexpr float FADE_DURATION = 0.1f;

    TapDetector tapDetector_;
};

} // namespace theword::ui

#endif
