#ifndef EVENTS_H
#define EVENTS_H

#include <string>

namespace theword::event {

struct ScrollEvent       { float delta; bool direct = false; };

struct SelectionEvent {
    enum class Action { Start, Update, End, Cancel } action;
    int startWordId;
    int endWordId;
    std::string bookId;
    int chapterNum = 0;
};

struct ResizeEvent {
    int width;
    int height;
    float prevScrollY;
};

struct FontSizeEvent     { float newSize; float delta; };
struct SourceSwitchEvent { bool online; };

struct DialogEvent {
    enum class Type { GoTo, Settings, About, ContextMenu } type;
    enum class Action { Show, Hide, Toggle } action;
};

struct KeyEvent          { int key; };
struct NavigateEvent     { std::string chapterRef; };

struct RightClickEvent   { float x; float y; };
struct ScrollStopEvent   {};

struct NavigateToHighlightEvent {
    std::string chapterRef;
    int wordId;
};

} // namespace theword::event
#endif
