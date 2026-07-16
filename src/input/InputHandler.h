#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <raylib.h>
#include <functional>
#include <cmath>
#include <string>

namespace theword::event { class EventBus; }
namespace theword::ui { class NavigationStack; }

namespace theword::input {

struct HitInfo {
    int wordId = -1;
    std::string bookId;
    int chapterNum = 0;
};

class InputHandler {
public:
    InputHandler(theword::event::EventBus& eventBus,
                 std::function<HitInfo(float, float)> hitTestFn = nullptr,
                 std::function<bool(int)> isHighlightedFn = nullptr,
                 std::function<int(float, float)> hitTestFootnoteFn = nullptr);

    void Poll(float deltaTime, theword::ui::NavigationStack* navStack = nullptr);
    void ResetState();
    bool IsDialogActive() const { return dialogActive_; }
    bool HasMomentum() const { return false; }

    // Semantic callbacks — set before the Poll loop
    std::function<void(HitInfo hit, Vector2 pos, bool isDouble)> onTap;
    std::function<void(Vector2 pos)> onTapEmpty;
    std::function<void(int startWord, Vector2 pos)> onDragStart;
    std::function<void(int startWord, int currentWord, Vector2 pos)> onDragUpdate;
    std::function<void(int startWord, int endWord, Vector2 pos)> onDragEnd;
    std::function<void(int wordId, Vector2 pos)> onLongPress;
    std::function<void(int footnoteIndex)> onFootnoteTap;
    std::function<bool()> onDismiss;

    const HitInfo& GetPressStartHit() const { return pressStartHit; }

private:
    enum class PressState { Idle, Pending, Dragging, LongPress };

    theword::event::EventBus& eventBus_;

    std::function<HitInfo(float, float)> hitTestFn;
    std::function<bool(int)> isHighlightedFn;
    std::function<int(float, float)> hitTestFootnoteFn;
    bool dialogActive_ = false;

    float slopAccumulator = 0.0f;
    static constexpr float SCROLL_SENSITIVITY = 40.0f;
    static constexpr float KEYBOARD_SCROLL_FACTOR = 0.3f;
    static constexpr float TOUCH_SLOP = 10.0f;
    static constexpr double LONG_PRESS_TIME = 0.3;
    static constexpr float LONG_PRESS_MOVE_THRESHOLD = 10.0f;
    static constexpr double DOUBLE_CLICK_TIME = 0.35;
    static constexpr float DOUBLE_CLICK_DISTANCE = 40.0f;

    PressState pressState;
    double pressStartTime;
    Vector2 pressStartPos;
    HitInfo pressStartHit;
    int selectStartWord;
    int lastDragWord_;

    float touchLastY;
    float touchLaunchVelocity_ = 0.0f;
    float lastPinchDist;
    Vector2 lastTouchPos_{0, 0};
    bool scrollActive_ = false;
    bool didScroll_ = false;
    bool prevPressed_ = false;

    double lastClickTime_ = 0.0;
    Vector2 lastClickPos_{0, 0};

    void HandleScroll();
    void HandleWindowResize();
    void HandleRightClick();
    void HandleTouchScroll();
    void HandlePinch();
    void RunUnifiedFSM(bool isPressed, bool justPressed, bool justReleased);
};

} // namespace theword::input

#endif
