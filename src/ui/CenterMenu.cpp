#include "CenterMenu.h"
#include "BookListScreen.h"
#include "SettingsScreen.h"
#include "CreditsOverlay.h"
#include "HighlightBrowserScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Locale.h"

namespace theword::ui {

using namespace theword::core;

static constexpr int ITEM_COUNT = 4;

static const char* ITEM_LABELS[ITEM_COUNT] = {
    "Books",
    "Settings",
    "Highlights",
    "Credits"
};

const char* CenterMenu::ItemLabel(int idx) {
    if (idx >= 0 && idx < ITEM_COUNT) return Locale::Get(ITEM_LABELS[idx]);
    return "";
}

CenterMenu::CenterMenu(const Font& font, float fontSize,
                       NavigationStack& navStack,
                       theword::event::EventBus& eventBus,
                       theword::highlight::Highlighter& highlighter,
                       theword::persistence::PersistenceManager& persistence,
                       const theword::core::UIScale& uiScale,
                       float& currentFontSize, int& currentBibleId, bool& immersiveMode,
                       const theword::core::ThemeManager& themeManager,
                       const std::string& currentChapterRef)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), currentBibleId_(currentBibleId), immersiveMode_(immersiveMode),
      themeManager_(themeManager),
      showTime_(GetTime()), currentChapterRef_(currentChapterRef),
      tapDetector_(uiScale_.dp(10)) {}

void CenterMenu::Draw(theword::renderer::DrawContext& ctx) {
    const auto& palette = ctx.themeManager.Current();
    float screenW = ctx.uiScale.screenW;
    float screenH = ctx.uiScale.screenH;

    float now = static_cast<float>(GetTime());
    float fadeAlpha;
    if (fadingOut_) {
        float elapsed = now - static_cast<float>(fadeOutStartTime_);
        fadeAlpha = std::max(0.0f, 1.0f - elapsed / FADE_DURATION);
        if (fadeAlpha <= 0.0f) {
            popPending_ = true;
            return;
        }
    } else {
        float elapsed = now - static_cast<float>(showTime_);
        fadeAlpha = std::min(1.0f, elapsed / FADE_DURATION);
    }

    Color overlayColor = palette.overlayBg;
    overlayColor.a = static_cast<unsigned char>(palette.overlayBg.a * fadeAlpha);
    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), overlayColor);

    float menuW = uiScale_.fitScreen(85, 320);
    float padding = uiScale_.dp(12);
    float itemH = std::max(uiScale_.dp(48), fontSize_ * 0.7f + uiScale_.dp(12));
    float totalHeight = ITEM_COUNT * itemH + padding * 2;
    float panelX = (screenW - menuW) / 2.0f;
    float panelY = (screenH - totalHeight) / 2.0f;

    DrawPanel({panelX, panelY, menuW, totalHeight},
              Fade(palette.panelBg, fadeAlpha),
              Fade(palette.panelBorder, fadeAlpha));

    float itemY = panelY + padding;
    float labelSize = fontSize_ * 0.7f;

    for (int i = 0; i < ITEM_COUNT; ++i) {
        Rectangle itemRect = {panelX + padding, itemY, menuW - padding * 2, itemH};
        DrawTextItem(ctx, itemRect, ItemLabel(i), font_, labelSize,
                     i == selectedIndex_,
                     Fade(palette.uiText, fadeAlpha), Fade(palette.uiTitle, fadeAlpha));
        itemY += itemH;
    }

    bool overPanel = PointInRect(ctx.input.mouseX, ctx.input.mouseY,
                                 {panelX, panelY, menuW, totalHeight});
    ctx.SetCursor(overPanel ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool CenterMenu::HandleInput(const theword::renderer::DrawContext& ctx, float /*deltaTime*/) {
    if (fadingOut_) {
        if (popPending_) {
            navStack_.Pop();
            return true;
        }
        return true;
    }

    if (ctx.input.KeyPressed(key::ESCAPE)) {
        fadingOut_ = true;
        fadeOutStartTime_ = GetTime();
        return true;
    }

    if (ctx.input.KeyPressed(key::UP)) {
        selectedIndex_ = (selectedIndex_ - 1 + ITEM_COUNT) % ITEM_COUNT;
        return true;
    }

    if (ctx.input.KeyPressed(key::DOWN)) {
        selectedIndex_ = (selectedIndex_ + 1) % ITEM_COUNT;
        return true;
    }

    if (ctx.input.KeyPressed(key::ENTER)) {
        HandleAction(selectedIndex_);
        return true;
    }

    if (ctx.input.leftPressed)
        tapDetector_.OnPress(ctx.input.mouseX, ctx.input.mouseY);

    if (ctx.input.leftReleased) {
        float tapX, tapY;
        auto tr = tapDetector_.OnRelease(ctx.input.mouseX, ctx.input.mouseY, tapX, tapY);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float screenW = ctx.uiScale.screenW;
        float screenH = ctx.uiScale.screenH;

        float menuW = uiScale_.fitScreen(85, 320);
        float padding = uiScale_.dp(12);
        float itemH = std::max(uiScale_.dp(48), fontSize_ * 0.7f + uiScale_.dp(12));
        float totalHeight = ITEM_COUNT * itemH + padding * 2;
        float panelX = (screenW - menuW) / 2.0f;
        float panelY = (screenH - totalHeight) / 2.0f;
        Rectangle panelRect = {panelX, panelY, menuW, totalHeight};

        if (!PointInRect(tapX, tapY, panelRect)) {
            fadingOut_ = true;
            fadeOutStartTime_ = GetTime();
            return true;
        }

        float itemY = panelY + padding;
        for (int i = 0; i < ITEM_COUNT; ++i) {
            Rectangle itemRect = {panelX + padding, itemY,
                                  menuW - padding * 2, itemH};
            if (PointInRect(tapX, tapY, itemRect)) {
                HandleAction(i);
                return true;
            }
            itemY += itemH;
        }
    }
    }

    return false;
}

void CenterMenu::HandleAction(int action) {
    switch (action) {
        case 0: // Books
            navStack_.Push(std::make_unique<BookListScreen>(
                font_, fontSize_, navStack_, eventBus_, uiScale_, currentChapterRef_
            ));
            return;
        case 1: // Settings
            navStack_.Push(std::make_unique<SettingsScreen>(
                navStack_, eventBus_,
                persistence_,
                uiScale_, currentFontSize_, currentBibleId_, immersiveMode_,
                themeManager_
            ));
            return;
        case 2: // Highlights
            navStack_.Push(std::make_unique<HighlightBrowserScreen>(
                font_, fontSize_, navStack_, eventBus_,
                highlighter_, uiScale_
            ));
            return;
        case 3: // Credits
            navStack_.Push(std::make_unique<CreditsOverlay>(
                fontSize_, navStack_, uiScale_
            ));
            return;
    }
    navStack_.Pop();
}

} // namespace theword::ui
