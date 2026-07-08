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

    using RadialMenuClickCallback = std::function<bool(Vector2)>;
    using RadialMenuShowCallback = std::function<void(int startWord, int endWord, Vector2 position, bool selectFullVerse)>;
    using ContextDismissHandler = std::function<bool()>;
    void Poll(float deltaTime, theword::ui::NavigationStack* navStack = nullptr,
              RadialMenuClickCallback radialClickHandler = nullptr,
              ContextDismissHandler contextDismissHandler = nullptr,
              RadialMenuShowCallback radialShowHandler = nullptr);
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
    static constexpr double DOUBLE_CLICK_TIME = 0.35;

    PressState pressState;
    double pressStartTime;
    Vector2 pressStartPos;
    int pressStartWord;
    int selectStartWord;
    bool touchActive;
    float touchLastY;
    float touchLaunchVelocity_ = 0.0f;
    float lastPinchDist;

    double lastClickTime_ = 0.0;
    int lastClickWord_ = -1;

    RadialMenuShowCallback showRadialCallback_;

    void FinishSelection(int startWord, int endWord, Vector2 position);
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
