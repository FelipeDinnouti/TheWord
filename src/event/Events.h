#ifndef EVENTS_H
#define EVENTS_H

#include <string>

namespace theword::event {

struct ScrollEvent       { float delta; bool direct = false; float velocity = 0.0f; };

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
struct BibleVersionSwitchEvent { int bibleId; };

struct DialogEvent {
    enum class Type { GoTo, Settings, About, ContextMenu } type;
    enum class Action { Show, Hide, Toggle } action;
};

struct KeyEvent          { int key; };
struct NavigateEvent     { std::string chapterRef; };

struct NavigateToHighlightEvent {
    std::string chapterRef;
    int wordId;
};

struct ChapterLoadedEvent {
    std::string chapterRef;
};

} // namespace theword::event
#endif
