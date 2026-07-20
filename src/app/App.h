#ifndef APP_H
#define APP_H

#include "core/UIScale.h"
#include <memory>
#include <string>
#include <vector>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::text { class LayoutEngine; }
namespace theword::document { class DocumentManager; }
namespace theword::renderer { class Renderer; class UIManager; }
namespace theword::input { struct HitInfo; class InputHandler; }
namespace theword::highlight { class Highlighter; }
namespace theword::core { class IHttpClient; class ThemeManager; }
namespace theword::data { struct ChapterData; class USFMParser; class BibleClient; class CompositeProvider; class ChapterProvider; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::app {

class App {
public:
    App();
    ~App();
    bool Init(const std::string& title);
    void Run();

private:
    void WireEvents();
    void WireInputCallbacks();
    void MainLoop();
    void HandleShortcuts();
    void OnTap(theword::input::HitInfo hit, Vector2 pos, bool isDouble);
    void OnTapEmpty(Vector2 pos);
    void OnDragStart(int startWord, Vector2 pos);
    void OnDragUpdate(int startWord, int currentWord, Vector2 pos);
    void OnDragEnd(int startWord, int endWord, Vector2 pos);
    void OnLongPress(int wordId, Vector2 pos);
    bool OnDismiss();
    void CopySelection(const theword::data::ChapterData& data, int startWord, int endWord);
    void ReloadFonts(float newFontSize, std::vector<int>& codepoints);

    float scale_ = 1.0f;
    theword::core::UIScale uiScale_{1.0f, 0, 0, 0};

    Font bodyFont_{};
    Font headingFont_{};
    Font largeFont_{};
    Font smallFont_{};
    Font boldFont_{};
    float headingSize_ = 0.0f;
    std::vector<int> fontCodepoints_;

    std::unique_ptr<theword::event::EventBus> eventBus_;
    std::unique_ptr<theword::core::IHttpClient> apiClient_;
    std::unique_ptr<theword::core::ThemeManager> themeManager_;
    std::unique_ptr<theword::data::USFMParser> usfmParser_;
    std::unique_ptr<theword::data::BibleClient> bibleClient_;
    std::unique_ptr<theword::data::CompositeProvider> compositeProv_;
    std::unique_ptr<theword::persistence::PersistenceManager> persistence_;
    std::unique_ptr<theword::highlight::Highlighter> highlighter_;
    std::unique_ptr<theword::text::LayoutEngine> layoutEngine_;
    std::unique_ptr<theword::renderer::Renderer> renderer_;
    std::unique_ptr<theword::document::DocumentManager> docManager_;
    std::unique_ptr<theword::renderer::UIManager> uiManager_;
    std::unique_ptr<theword::input::InputHandler> inputHandler_;
    std::unique_ptr<theword::ui::NavigationStack> navStack_;

    theword::data::ChapterProvider* activeProv_ = nullptr;

    float currentFontSize_ = 24.0f;
    int currentBibleId_ = 3034;
    bool immersiveMode_ = false;
    float pendingScrollY_ = -1.0f;
    int accumStartWord_ = -1;
    int accumEndWord_ = -1;
    bool longPressHandled_ = false;
};

} // namespace theword::app

#endif
