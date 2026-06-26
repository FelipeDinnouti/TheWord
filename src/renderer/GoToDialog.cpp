#include "GoToDialog.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "core/BibleBooks.h"
#include "core/Theme.h"
#include "core/Config.h"
#include <cctype>
#include <cstdlib>
#include <cmath>

namespace theword::renderer {

using namespace theword::core;

GoToDialog::GoToDialog(const Font& headingFont, float headingSize,
                       theword::event::EventBus& eventBus, float scale)
    : headingFont(headingFont), headingSize(headingSize),
      eventBus_(eventBus), scale(scale),
      active(false), selection(0), goToError(false) {}

void GoToDialog::Toggle() {
    active = !active;
    if (active) { input.clear(); selection = 0; goToError = false; }
}

void GoToDialog::Dismiss() { active = false; input.clear(); goToError = false; }
bool GoToDialog::IsActive() const { return active; }

Rectangle GoToDialog::GetDialogRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - DIALOG_WIDTH * scale) / 2.0f,
            (screenH - GO_TO_DIALOG_HEIGHT * scale) / 2.0f,
            DIALOG_WIDTH * scale, GO_TO_DIALOG_HEIGHT * scale};
}

Rectangle GoToDialog::GetCloseButtonRect(Rectangle dlg) const {
    return {dlg.x + dlg.width - CLOSE_SIZE * scale - CLOSE_MARGIN * scale,
            dlg.y + CLOSE_MARGIN * scale,
            CLOSE_SIZE * scale, CLOSE_SIZE * scale};
}

void GoToDialog::DrawBackdrop() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), theme::OVERLAY_BG);
}

void GoToDialog::DrawCloseButton(Rectangle dlg) const {
    Rectangle closeBtn = GetCloseButtonRect(dlg);
    DrawRectangleRec(closeBtn, theme::BUTTON_BG);
    DrawRectangleLinesEx(closeBtn, 1, theme::BUTTON_BORDER);
    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * theme::FONT_DETAIL, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1},
               headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void GoToDialog::DrawSuggestions(Rectangle dlg, const std::vector<int>& suggestions) const {
    float sy = dlg.y + 80 * scale;
    for (size_t i = 0; i < suggestions.size(); ++i) {
        const auto& book = BOOKS[suggestions[i]];
        Color bg = ((int)i == selection) ? theme::SELECTED_BG : theme::PANEL_BG;
        DrawRectangle(dlg.x + 10 * scale, sy, DIALOG_WIDTH * scale - 20 * scale,
                      SUGGESTION_ITEM_H * scale, bg);
        std::string label = std::string(book.code) + " - " + book.fullName;
        DrawTextEx(headingFont, label.c_str(), {dlg.x + 14 * scale, sy + 2 * scale},
                   headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
        sy += SUGGESTION_LINE_H * scale;
    }
}

void GoToDialog::Draw() {
    if (!active) return;

    DrawBackdrop();
    Rectangle dlg = GetDialogRect();

    DrawRectangleRec(dlg, theme::PANEL_BG);
    DrawRectangleLinesEx(dlg, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Go to:", {dlg.x + SETTINGS_LABEL_X * scale,
               dlg.y + SETTINGS_LABEL_X * scale}, headingSize, 1, theme::UI_TITLE);
    DrawCloseButton(dlg);

    Rectangle inputBox = {dlg.x + SETTINGS_LABEL_X * scale,
                          dlg.y + SETTINGS_ROW1_Y * scale,
                          DIALOG_WIDTH * scale - 20 * scale, INPUT_BOX_H * scale};
    DrawRectangleRec(inputBox, theme::INPUT_BG);
    DrawRectangleLinesEx(inputBox, 1, goToError ? theme::INPUT_BORDER_ERROR : theme::INPUT_BORDER);

    std::string display = input;
    if (fmod(GetTime() * 2.0, 1.0) < 0.5) display += "|";
    DrawTextEx(headingFont, display.c_str(), {inputBox.x + INPUT_BOX_INSET * scale,
               inputBox.y + INPUT_BOX_INSET * scale}, headingSize, 1, theme::UI_INPUT_TEXT);

    DrawSuggestions(dlg, GetSuggestions());
}

void GoToDialog::HandleKeyboardInput() {
    if (!active) return;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            input.push_back(static_cast<char>(ch));
            goToError = false;
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(key::BACKSPACE) && !input.empty()) {
        input.pop_back();
        goToError = false;
    }

    if (IsKeyPressed(key::DOWN)) {
        auto suggestions = GetSuggestions();
        selection = std::min(selection + 1, std::max(0, (int)suggestions.size() - 1));
    }
    if (IsKeyPressed(key::UP)) {
        selection = std::max(selection - 1, 0);
    }

    if (IsKeyPressed(key::TAB)) {
        auto suggestions = GetSuggestions();
        if (!suggestions.empty() && selection < (int)suggestions.size()) {
            input = BOOKS[suggestions[selection]].code;
        }
    }

    if (IsKeyPressed(key::ENTER) && !input.empty()) {
        std::string ref = ParseGoToInput(input);
        if (!ref.empty()) {
            eventBus_.Emit(theword::event::NavigateEvent{ref});
            Dismiss();
        } else {
            goToError = true;
        }
    }

    if (IsKeyPressed(key::ESCAPE)) {
        Dismiss();
    }
}

bool GoToDialog::HandleClick(Vector2 pos) {
    if (!active) return false;

    Rectangle dlg = GetDialogRect();
    if (!CheckCollisionPointRec(pos, dlg)) {
        Dismiss();
        return true;
    }

    Rectangle closeBtn = GetCloseButtonRect(dlg);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        Dismiss();
    }
    return true;
}

bool GoToDialog::StartsWithIgnoreCase(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

std::vector<int> GoToDialog::GetSuggestions() const {
    std::vector<int> results;
    if (input.empty()) return results;
    for (int i = 0; i < (int)BOOKS.size(); ++i) {
        if (StartsWithIgnoreCase(input, BOOKS[i].code) ||
            StartsWithIgnoreCase(input, BOOKS[i].fullName)) {
            results.push_back(i);
            if (results.size() >= 5) break;
        }
    }
    return results;
}

std::string GoToDialog::ParseGoToInput(const std::string& input) const {
    if (auto r = TryParseBookDotChapter(input); !r.empty()) return r;
    if (auto r = TryParseFullNameThenChapter(input); !r.empty()) return r;
    if (auto r = TryParseSpaceSeparated(input); !r.empty()) return r;
    return "";
}

std::string GoToDialog::TryParseBookDotChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string code = book.code;
        if (input.size() > code.size() + 1 &&
            StartsWithIgnoreCase(input, code) && input[code.size()] == '.') {
            int ch = 0; try { ch = std::stoi(input.substr(code.size() + 1)); } catch (...) { continue; }
            if (ch >= 1 && ch <= book.chapterCount)
                return code + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string GoToDialog::TryParseFullNameThenChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string name = book.fullName;
        if (input.size() > name.size() + 1 &&
            StartsWithIgnoreCase(input, name) && input[name.size()] == ' ') {
            int ch = 0; try { ch = std::stoi(input.substr(name.size() + 1)); } catch (...) { continue; }
            if (ch >= 1 && ch <= book.chapterCount)
                return std::string(book.code) + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string GoToDialog::TryParseSpaceSeparated(const std::string& input) {
    size_t space = input.rfind(' ');
    if (space != std::string::npos && space + 1 < input.size()) {
        int ch = 0; try { ch = std::stoi(input.substr(space + 1)); } catch (...) { return ""; }
        if (ch > 0) {
            std::string bookPart = input.substr(0, space);
            for (const auto& candidate : BOOKS) {
                if (StartsWithIgnoreCase(bookPart, candidate.code) ||
                    StartsWithIgnoreCase(bookPart, candidate.fullName)) {
                    if (ch <= candidate.chapterCount)
                        return std::string(candidate.code) + "." + std::to_string(ch);
                }
            }
        }
    }
    return "";
}

} // namespace theword::renderer
