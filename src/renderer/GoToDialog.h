#ifndef GO_TO_DIALOG_H
#define GO_TO_DIALOG_H

#include <raylib.h>
#include <string>
#include <vector>

namespace theword::event { class EventBus; }

namespace theword::renderer {

class GoToDialog {
public:
    GoToDialog(const Font& headingFont, float headingSize,
               theword::event::EventBus& eventBus, float scale);

    void Toggle();
    void Dismiss();
    bool IsActive() const;

    void Draw();
    void HandleKeyboardInput();
    bool HandleClick(Vector2 pos);

    int GetSelection() const { return selection; }

private:
    const Font& headingFont;
    float headingSize;
    theword::event::EventBus& eventBus_;
    float scale;

    bool active;
    std::string input;
    int selection;
    bool goToError;

    static constexpr float DIALOG_WIDTH = 300.0f;
    static constexpr float GO_TO_DIALOG_HEIGHT = 200.0f;
    static constexpr float SETTINGS_LABEL_X = 10.0f;
    static constexpr float SETTINGS_ROW1_Y = 40.0f;
    static constexpr float INPUT_BOX_H = 30.0f;
    static constexpr float INPUT_BOX_INSET = 4.0f;
    static constexpr float SUGGESTION_ITEM_H = 22.0f;
    static constexpr float SUGGESTION_LINE_H = 24.0f;
    static constexpr float CLOSE_SIZE = 18.0f;
    static constexpr float CLOSE_MARGIN = 6.0f;

    Rectangle GetDialogRect() const;
    Rectangle GetCloseButtonRect(Rectangle dlg) const;
    void DrawBackdrop() const;
    void DrawCloseButton(Rectangle dlg) const;
    void DrawSuggestions(Rectangle dlg, const std::vector<int>& suggestions) const;

    std::vector<int> GetSuggestions() const;
    static bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);
    std::string ParseGoToInput(const std::string& input) const;
    static std::string TryParseBookDotChapter(const std::string& input);
    static std::string TryParseFullNameThenChapter(const std::string& input);
    static std::string TryParseSpaceSeparated(const std::string& input);
};

} // namespace theword::renderer

#endif
