#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <raylib.h>
#include <functional>
#include <cmath>

namespace theword::event { class EventBus; }
namespace theword::ui { class NavigationStack; }

namespace theword::input {

class InputHandler {
public:
    InputHandler(theword::event::EventBus& eventBus,
                 std::function<int(float, float)> hitTestFn = nullptr,
                 std::function<bool(int)> isHighlightedFn = nullptr);

    using ContextMenuHandler = std::function<bool(Vector2)>;
    using ContextDismissHandler = std::function<bool()>;
    void Poll(float deltaTime, theword::ui::NavigationStack* navStack = nullptr,
              ContextMenuHandler contextMenuHandler = nullptr,
              ContextDismissHandler contextDismissHandler = nullptr);
    bool IsDialogActive() const { return dialogActive_; }
    bool HasMomentum() const { return false; }

private:
    enum class PressState { Idle, Pending, Dragging, LongPress, Selecting };

    theword::event::EventBus& eventBus_;

    std::function<int(float, float)> hitTestFn;
    std::function<bool(int)> isHighlightedFn;
    bool dialogActive_ = false;

    float slopAccumulator = 0.0f;
    static constexpr float SCROLL_SENSITIVITY = 40.0f;
    static constexpr float KEYBOARD_SCROLL_FACTOR = 0.3f;
    static constexpr float TOUCH_SLOP = 10.0f;
    static constexpr double LONG_PRESS_TIME = 0.5;
    static constexpr float LONG_PRESS_MOVE_THRESHOLD = 10.0f;

    PressState pressState;
    double pressStartTime;
    Vector2 pressStartPos;
    int pressStartWord;
    int selectStartWord;
    bool touchActive;
    float touchLastY;
    float lastTouchDelta;
    float lastPinchDist;

    void HandleScroll();
    void HandlePressFSM();
    void HandleWindowResize();
    void HandleRightClick();
    void HandleTouchScroll();
    void HandleTouchPressFSM();
    void HandlePinch();
};

} // namespace theword::input

#endif
