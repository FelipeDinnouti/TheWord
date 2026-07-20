#include "UIManager.h"
#include "RadialMenu.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"
#include "core/ThemeManager.h"

namespace theword::renderer {

using namespace theword::core;

UIManager::UIManager(theword::highlight::Highlighter& highlighter, const Font& uiFont,
                     const theword::core::ThemeManager& themeManager, float dpiScale)
    : highlighter_(highlighter), uiFont_(uiFont), themeManager_(themeManager), dpiScale_(dpiScale) {}

UIManager::~UIManager() = default;

void UIManager::ShowToast(const std::string& text) {
    toastText_ = text;
    toastStartTime_ = GetTime();
}

void UIManager::DrawToast() {
    if (toastText_.empty()) return;
    double elapsed = GetTime() - toastStartTime_;
    if (elapsed > TOAST_DURATION) {
        toastText_.clear();
        return;
    }

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float fontSize = 24.0f * dpiScale_;
    float alpha = (elapsed > TOAST_DURATION - 0.3f)
        ? static_cast<float>((TOAST_DURATION - elapsed) / 0.3) : 1.0f;

    Vector2 dims = MeasureTextEx(uiFont_, toastText_.c_str(), fontSize, 1);
    float pad = 12.0f * dpiScale_;
    float x = (sw - dims.x) / 2.0f;
    float y = sh * 0.7f;
    Rectangle bg = {x - pad, y - pad, dims.x + pad * 2.0f, dims.y + pad * 2.0f};

    DrawRectangleRounded(bg, 0.3f, 6, Color{0, 0, 0, static_cast<unsigned char>(180 * alpha)});
    DrawTextEx(uiFont_, toastText_.c_str(), {x, y}, fontSize, 1,
               Color{255, 255, 255, static_cast<unsigned char>(255 * alpha)});
}

void UIManager::ShowFootnotePopup(const std::string& text, Vector2 position) {
    footnoteText_ = text;
    footnotePos_ = position;
    footnotePopupActive_ = true;
}

void UIManager::DrawFootnotePopup() {
    if (!footnotePopupActive_ || footnoteText_.empty()) return;

    float fontSize = 14.0f * dpiScale_;
    float maxWidth = GetScreenWidth() * 0.5f;

    // Word-wrap the text
    std::vector<std::string> lines;
    std::string currentLine;
    std::string remaining = footnoteText_;
    while (!remaining.empty()) {
        size_t spacePos = remaining.find(' ');
        std::string word = (spacePos == std::string::npos) ? remaining : remaining.substr(0, spacePos);

        float testWidth = currentLine.empty()
            ? MeasureTextEx(uiFont_, word.c_str(), fontSize, 1).x
            : MeasureTextEx(uiFont_, (currentLine + " " + word).c_str(), fontSize, 1).x;

        if (testWidth > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = currentLine.empty() ? word : currentLine + " " + word;
        }

        if (spacePos == std::string::npos) break;
        remaining = remaining.substr(spacePos + 1);
    }
    if (!currentLine.empty()) lines.push_back(currentLine);

    // Calculate dimensions
    float lineHeight = fontSize * 1.4f;
    float pad = 16.0f * dpiScale_;
    float totalWidth = maxWidth + pad;
    float maxPopupHeight = GetScreenHeight() * 0.4f;
    float contentHeight = lines.size() * lineHeight;
    float totalHeight = contentHeight + pad;

    // Clamp height — truncate lines if taller than 40% of screen
    if (totalHeight > maxPopupHeight) {
        int maxLines = static_cast<int>((maxPopupHeight - pad) / lineHeight);
        if (maxLines < 1) maxLines = 1;
        lines.resize(maxLines);
        contentHeight = maxLines * lineHeight;
        totalHeight = contentHeight + pad;
    }

    // Position popup near tap, clamping to screen edges
    float margin = 10.0f * dpiScale_;
    float px = footnotePos_.x + margin;
    if (px + totalWidth > GetScreenWidth())
        px = footnotePos_.x - totalWidth - margin;
    if (px < margin) px = margin;

    float py = footnotePos_.y - totalHeight / 2.0f;
    if (py < margin) py = margin;
    if (py + totalHeight > GetScreenHeight() - margin)
        py = GetScreenHeight() - totalHeight - margin;
    if (py < margin) py = margin;

    const auto& palette = themeManager_.Current();
    Rectangle bg = {px, py, totalWidth, totalHeight};
    DrawRectangleRounded(bg, 0.2f, 6, palette.docFootnotePopupBg);
    DrawRectangleRoundedLines(bg, 0.2f, 6, 1.5f, palette.docFootnotePopupBorder);

    float textX = px + 8.0f * dpiScale_;
    float textY = py + 8.0f * dpiScale_;
    for (const auto& line : lines) {
        DrawTextEx(uiFont_, line.c_str(), {textX, textY}, fontSize, 1, palette.docFootnotePopupText);
        textY += lineHeight;
    }
}

void UIManager::HideFootnotePopup() {
    footnoteText_.clear();
    footnotePopupActive_ = false;
}

bool UIManager::IsFootnotePopupActive() const {
    return footnotePopupActive_;
}

void UIManager::ShowRadialMenu(Vector2 position, int startWord, int endWord,
                               const std::string& bookId, int chapterNum) {
    if (!radialMenu) {
        radialMenu = std::make_unique<RadialMenu>(dpiScale_);
    }
    radialMenu->Show(position, startWord, endWord, bookId, chapterNum, highlighter_.GetTypes());
}

void UIManager::HideRadialMenu() {
    if (radialMenu) radialMenu->Hide();
    highlighter_.ClearCommittedSelection();
}

bool UIManager::IsRadialMenuActive() const {
    return radialMenu && radialMenu->IsActive();
}

void UIManager::DrawRadialMenu() {
    if (radialMenu) radialMenu->Draw();
}

RadialMenuActionResult UIManager::HandleRadialMenuClick(Vector2 pos) {
    RadialMenuActionResult result;
    if (!radialMenu || !radialMenu->IsActive()) return result;

    auto [action, colorIndex] = radialMenu->HandleClick(pos);
    result.startWord = radialMenu->GetStartWord();
    result.endWord = radialMenu->GetEndWord();
    result.bookId = radialMenu->GetBookId();
    result.chapterNum = radialMenu->GetChapterNum();

    switch (action) {
        case RadialMenu::Action::Highlight:
            result.consumed = true;
            result.isHighlight = true;
            result.colorIndex = colorIndex;
            break;

        case RadialMenu::Action::Copy:
            result.consumed = true;
            result.isCopy = true;
            HideRadialMenu();
            break;

        case RadialMenu::Action::Delete:
            result.consumed = true;
            result.isDelete = true;
            HideRadialMenu();
            break;

        case RadialMenu::Action::None:
            result.consumed = false;
            break;
    }

    return result;
}

} // namespace theword::renderer
