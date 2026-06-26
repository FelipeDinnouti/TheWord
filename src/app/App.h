#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::text { class LayoutEngine; }
namespace theword::document { class DocumentManager; }
namespace theword::renderer { class Renderer; class UIManager; }
namespace theword::input { class InputHandler; }
namespace theword::highlight { class Highlighter; }
namespace theword::data { class USFMParser; class BibleClient; class CompositeProvider; class ChapterProvider; }
namespace theword::persistence { class PersistenceManager; }

namespace theword::app {

class App {
public:
    App();
    ~App();
    bool Init(const std::string& title);
    void Run();

private:
    void WireEvents();

    float scale_ = 1.0f;

    Font bodyFont_{};
    Font headingFont_{};

    std::unique_ptr<theword::event::EventBus> eventBus_;
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

    theword::data::ChapterProvider* activeProv_ = nullptr;
    theword::data::ChapterProvider* onlineProv_ = nullptr;
    theword::data::ChapterProvider* offlineProv_ = nullptr;

    float currentFontSize_ = 24.0f;
    bool versionOnline_ = false;
};

} // namespace theword::app

#endif
