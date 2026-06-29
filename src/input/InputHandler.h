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
    bool HasMomentum() const { return std::abs(scrollVelocity) > MIN_VELOCITY; }

private:
    enum class PressState { Idle, Pending, Dragging, LongPress, Selecting };

    theword::event::EventBus& eventBus_;

    std::function<int(float, float)> hitTestFn;
    std::function<bool(int)> isHighlightedFn;
    bool dialogActive_ = false;

    float scrollVelocity;
    float touchVelocity;
    float deltaHistory[3] = {};
    int deltaHistoryIdx = 0;
    float slopAccumulator = 0.0f;
    static constexpr int DELTA_HISTORY_SIZE = 3;
    static constexpr float SCROLL_SENSITIVITY = 40.0f;
    static constexpr float KEYBOARD_SCROLL_FACTOR = 0.3f;
    static constexpr float VELOCITY_ALPHA = 0.18f;
    static constexpr float TOUCH_SLOP = 10.0f;
    static constexpr float MOMENTUM_TIME_CONSTANT = 0.500f;
    static constexpr float MIN_VELOCITY = 50.0f;
    static constexpr float MAX_MOMENTUM_VELOCITY = 3500.0f;
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
