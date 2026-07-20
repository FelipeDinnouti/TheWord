#ifndef BOOK_LIST_SCREEN_H
#define BOOK_LIST_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include "core/ThemeManager.h"
#include "TapDetector.h"
#include <string>
#include <vector>
#include <memory>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class BookListScreen : public Screen {
public:
    BookListScreen(const Font& font, float fontSize,
                   NavigationStack& navStack,
                   theword::event::EventBus& eventBus,
                   const theword::core::UIScale& uiScale,
                   const theword::core::ThemeManager& themeManager,
                   const std::string& currentChapterRef = "");
    ~BookListScreen() override;
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Books"); }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    const theword::core::UIScale& uiScale_;
    const theword::core::ThemeManager& themeManager_;

    std::string search_;
    std::string currentChapterRef_;
    int scrollOffset_ = 0;
    int selection_ = 0;
    bool selectionInitialized_ = false;

    std::vector<int> GetFilteredIndices() const;
    int GetVisibleCount(float listHeight) const;

    float scrollAccumulator_ = 0.0f;

    TapDetector tapDetector_;

    std::shared_ptr<bool> aliveGuard_ = std::make_shared<bool>(true);
};

} // namespace theword::ui

#endif
