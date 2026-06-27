#include "UIManager.h"
#include "ContextMenu.h"
#include "SettingsPanel.h"
#include "GoToDialog.h"
#include "AboutOverlay.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "core/Config.h"
#include "core/Theme.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"

namespace theword::renderer {

using namespace theword::core;

UIManager::UIManager(theword::event::EventBus& eventBus,
                     const Font& headingFont, float headingSize,
                     theword::highlight::Highlighter& highlighter,
                     theword::persistence::PersistenceManager& persistence, float initialFontSize,
                     bool initialVersionOnline, float scaleFactor)
    : eventBus_(eventBus),
      headingFont(headingFont), headingSize(headingSize),
      persistence(persistence), highlighter(highlighter),
      currentFontSize(initialFontSize), scale(scaleFactor),
      versionOnline(initialVersionOnline) {
    contextMenu = new ContextMenu(headingFont, headingSize, highlighter, scaleFactor);
    settingsPanel = new SettingsPanel(headingFont, headingSize, highlighter, scaleFactor);
    goToDialog = new GoToDialog(headingFont, headingSize, eventBus_, scaleFactor);
    aboutOverlay = new AboutOverlay(headingFont, headingSize, scaleFactor);

    eventBus_.On<theword::event::KeyEvent>([this](const auto& e) { OnKey(e); });
    eventBus_.On<theword::event::DialogEvent>([this](const auto& e) { OnDialog(e); });
    eventBus_.On<theword::event::FontSizeEvent>([this](const auto& e) { OnFontSize(e); });
}

UIManager::~UIManager() {
    delete contextMenu;
    delete settingsPanel;
    delete goToDialog;
    delete aboutOverlay;
}

void UIManager::UpdateActiveDialog() {
    Vector2 pos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (aboutOverlay->IsActive()) {
        if (mousePressed) aboutOverlay->HandleClick(pos);
        return;
    }
    if (goToDialog->IsActive()) {
        if (mousePressed) goToDialog->HandleClick(pos);
        goToDialog->HandleKeyboardInput();
        return;
    }
    if (settingsPanel->IsActive()) {
        if (mousePressed) {
            auto action = settingsPanel->HandleClick(pos);
            switch (action) {
                case SettingsPanel::Action::Dismiss:
                    settingsPanel->Dismiss();
                    break;
                case SettingsPanel::Action::FontDecrease:
                    ChangeFontSize(-config::FONT_SIZE_STEP);
                    break;
                case SettingsPanel::Action::FontIncrease:
                    ChangeFontSize(config::FONT_SIZE_STEP);
                    break;
                case SettingsPanel::Action::SourceOffline:
                    SwitchSource(false);
                    break;
                case SettingsPanel::Action::SourceOnline:
                    SwitchSource(true);
                    break;
                default:
                    break;
            }
        }
        return;
    }
    if (contextMenu->IsActive() && mousePressed) {
        contextMenu->HandleClick(pos);
        eventBus_.Emit(theword::event::DialogEvent{
            theword::event::DialogEvent::Type::ContextMenu,
            theword::event::DialogEvent::Action::Hide});
        return;
    }
}

void UIManager::OnKey(const theword::event::KeyEvent& e) {
    if (e.key == key::ESCAPE) {
        DismissActiveDialog();
    }
}

void UIManager::OnDialog(const theword::event::DialogEvent& e) {
    switch (e.type) {
        case theword::event::DialogEvent::Type::GoTo:
            ToggleGoToDialog();
            break;
        case theword::event::DialogEvent::Type::Settings:
            ToggleSettings();
            break;
        case theword::event::DialogEvent::Type::About:
            ToggleAbout();
            break;
        case theword::event::DialogEvent::Type::ContextMenu:
            break;
    }
}

void UIManager::OnFontSize(const theword::event::FontSizeEvent& e) {
    // Handle pinch-initiated font size changes (delta != 0)
    if (e.delta != 0.0f && e.newSize == 0.0f) {
        ChangeFontSize(e.delta);
    }
}

float UIManager::GetContentTop() const { return TOP_BAR_HEIGHT; }
float UIManager::GetFontSize() const { return currentFontSize; }

void UIManager::DrawTopBar(const std::string& chapterTitle) {
    if (!chapterTitle.empty()) {
        float pad = 20.0f * scale;
        DrawTextEx(headingFont, chapterTitle.c_str(), {pad, pad}, headingSize, 1, theme::UI_TEXT);
    }
}

void UIManager::DrawContextMenu() { contextMenu->Draw(); }
void UIManager::DrawSettingsPanel() { settingsPanel->Draw(currentFontSize, versionOnline); }
void UIManager::DrawGoToDialog() { goToDialog->Draw(); }
void UIManager::DrawAbout() { aboutOverlay->Draw(); }

void UIManager::ShowContextMenu(Vector2 position, int highlightId, int typeId) {
    contextMenu->Show(position, highlightId, typeId);
    eventBus_.Emit(theword::event::DialogEvent{
        theword::event::DialogEvent::Type::ContextMenu,
        theword::event::DialogEvent::Action::Show});
}
void UIManager::HideContextMenu() {
    contextMenu->Hide();
    eventBus_.Emit(theword::event::DialogEvent{
        theword::event::DialogEvent::Type::ContextMenu,
        theword::event::DialogEvent::Action::Hide});
}
bool UIManager::IsContextMenuActive() const { return contextMenu->IsActive(); }

void UIManager::ToggleGoToDialog() {
    goToDialog->Toggle();
    eventBus_.Emit(theword::event::DialogEvent{
        theword::event::DialogEvent::Type::GoTo,
        goToDialog->IsActive() ? theword::event::DialogEvent::Action::Show
                               : theword::event::DialogEvent::Action::Hide});
}
void UIManager::DismissGoToDialog() { goToDialog->Dismiss(); }
bool UIManager::IsGoToDialogActive() const { return goToDialog->IsActive(); }

void UIManager::ToggleAbout() {
    aboutOverlay->Toggle();
    eventBus_.Emit(theword::event::DialogEvent{
        theword::event::DialogEvent::Type::About,
        aboutOverlay->IsActive() ? theword::event::DialogEvent::Action::Show
                                 : theword::event::DialogEvent::Action::Hide});
}
void UIManager::DismissAbout() { aboutOverlay->Dismiss(); }
bool UIManager::IsAboutActive() const { return aboutOverlay->IsActive(); }

void UIManager::ToggleSettings() {
    settingsPanel->Toggle();
    eventBus_.Emit(theword::event::DialogEvent{
        theword::event::DialogEvent::Type::Settings,
        settingsPanel->IsActive() ? theword::event::DialogEvent::Action::Show
                                  : theword::event::DialogEvent::Action::Hide});
}
void UIManager::DismissSettings() { settingsPanel->Dismiss(); }
bool UIManager::IsSettingsActive() const { return settingsPanel->IsActive(); }

void UIManager::ChangeFontSize(float delta) {
    float newSize = currentFontSize + delta;
    newSize = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, newSize));
    if (newSize != currentFontSize) {
        currentFontSize = newSize;
        eventBus_.Emit(theword::event::FontSizeEvent{newSize * scale, 0.0f});
        persistence.SetPreference("font_size", std::to_string((int)newSize));
    }
}

void UIManager::OnFontSizeApplied(float newSize) {
    currentFontSize = newSize;
}

void UIManager::SwitchSource(bool online) {
    if (online == versionOnline) return;
    versionOnline = online;
    persistence.SetPreference("active_version", online ? "online" : "offline");
    eventBus_.Emit(theword::event::SourceSwitchEvent{online});
}

void UIManager::DismissActiveDialog() {
    if (aboutOverlay->IsActive()) DismissAbout();
    else if (goToDialog->IsActive()) DismissGoToDialog();
    else if (settingsPanel->IsActive()) DismissSettings();
    else if (contextMenu->IsActive()) HideContextMenu();
}

} // namespace theword::renderer
