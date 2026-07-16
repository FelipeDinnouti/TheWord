#include "InputHandler.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>
#include "core/Config.h"
#include "core/Platform.h"
#include "ui/NavigationStack.h"
#include <raylib.h>
#include <cmath>

namespace theword::input {

using namespace theword::core;

InputHandler::InputHandler(theword::event::EventBus& eventBus,
                           std::function<HitInfo(float, float)> hitTestFn,
                           std::function<bool(int)> isHighlightedFn,
                           std::function<int(float, float)> hitTestFootnoteFn)
    : eventBus_(eventBus),
      hitTestFn(std::move(hitTestFn)),
      isHighlightedFn(std::move(isHighlightedFn)),
      hitTestFootnoteFn(std::move(hitTestFootnoteFn)),
      slopAccumulator(0.0f),
      pressState(PressState::Idle), pressStartTime(0.0),
      pressStartPos{0, 0}, pressStartHit{}, selectStartWord(-1),
      touchLastY(0.0f), lastPinchDist(0.0f) {

    eventBus_.On<theword::event::DialogEvent>([this](const theword::event::DialogEvent& e) {
        dialogActive_ = (e.action != theword::event::DialogEvent::Action::Hide);
    });
}

void InputHandler::ResetState() {
    pressState = PressState::Idle;
    pressStartTime = 0.0;
    pressStartPos = {0, 0};
    pressStartHit = {};
    selectStartWord = -1;
    lastDragWord_ = -1;
    touchLastY = 0.0f;
    touchLaunchVelocity_ = 0.0f;
    lastPinchDist = 0.0f;
    lastTouchPos_ = {0, 0};
    scrollActive_ = false;
    didScroll_ = false;
    prevPressed_ = false;
    slopAccumulator = 0.0f;
    // Preserve double-click tracking (lastClickTime_, lastClickPos_)
    // Preserve dialogActive_ (controlled by DialogEvent subscriber)
}

void InputHandler::Poll(float deltaTime, theword::ui::NavigationStack* navStack) {
    // Capture press state at top so early-return paths update prevPressed_
    // correctly, preventing stale-justPressed on the next frame.
    bool isPressed = platform::HasTouchInput() ? (GetTouchPointCount() >= 1)
                                               : IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool justPressed = isPressed && !prevPressed_;
    bool justReleased = !isPressed && prevPressed_;

    // ── Poll execution order ───────────────────────────────────────────────
    //  1. Escape → onDismiss (radial menu / context dismiss, consumed)
    //  2. Active screen HandleInput (buttons / list items, consumed)
    //  3. Escape → KeyEvent (unconsumed, fallthrough)
    //  4. Dialog guard (G/S/A hotkeys only, resets FSM)
    //  5. Scroll / pinch / right-click (platform-specific)
    //  6. RunUnifiedFSM (word tap / drag / long-press)
    //  7. HandleWindowResize (viewport change)
    //
    // Screens run before the FSM (stage 2 vs 6). This means screens consume
    // button-area presses before the FSM can see them. Currently safe because
    // screens handle UI (not text), but a screen that consumed a word-area
    // press would starve the FSM. (P12)
    // ────────────────────────────────────────────────────────────────────────

    // Escape → onDismiss (radial menu dismiss)
    if (IsKeyPressed(key::ESCAPE) && onDismiss && onDismiss()) {
        prevPressed_ = isPressed;
        return;
    }

    // Let the active screen process input first
    if (navStack && navStack->HandleInput(deltaTime)) {
        prevPressed_ = isPressed;
        return;
    }

    if (IsKeyPressed(key::ESCAPE)) {
        prevPressed_ = isPressed;
        eventBus_.Emit(theword::event::KeyEvent{key::ESCAPE});
        return;
    }

    if (dialogActive_) {
        // Dialog opened mid-gesture — reset FSM to prevent stuck state (P7)
        pressState = PressState::Idle;
        prevPressed_ = false;
        if (IsKeyPressed(key::G)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::GoTo, theword::event::DialogEvent::Action::Toggle}); return; }
        if (IsKeyPressed(key::S)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::Settings, theword::event::DialogEvent::Action::Toggle}); return; }
        if (IsKeyPressed(key::A)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::About, theword::event::DialogEvent::Action::Toggle}); return; }
        return;
    }

    if (platform::HasTouchInput()) {
        HandlePinch();
        HandleTouchScroll();
    } else {
        HandleScroll();
        HandleRightClick();
    }

    RunUnifiedFSM(isPressed, justPressed, justReleased);
    prevPressed_ = isPressed;
    HandleWindowResize();
}

void InputHandler::HandleScroll() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        eventBus_.Emit(theword::event::ScrollEvent{-wheel * SCROLL_SENSITIVITY});
        return;
    }

    if (IsKeyDown(key::DOWN)) {
        eventBus_.Emit(theword::event::ScrollEvent{SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR});
        return;
    }
    if (IsKeyDown(key::UP)) {
        eventBus_.Emit(theword::event::ScrollEvent{-SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR});
        return;
    }
}

void InputHandler::HandleRightClick() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePos = GetMousePosition();
        HitInfo hi = hitTestFn ? hitTestFn(mousePos.x, mousePos.y) : HitInfo{};
        if (hi.wordId >= 0 && onTap)
            onTap(hi, mousePos, false);
    }
}

void InputHandler::HandleTouchScroll() {
    int touchCount = GetTouchPointCount();
    if (touchCount == 1) {
        Vector2 pos = GetTouchPosition(0);
        lastTouchPos_ = pos;
        if (!scrollActive_) {
            scrollActive_ = true;
            touchLastY = pos.y;
            slopAccumulator = 0.0f;
            touchLaunchVelocity_ = 0.0f;
            didScroll_ = false;
            return;
        }

        float deltaY = pos.y - touchLastY;
        touchLastY = pos.y;

        float dt = GetFrameTime();
        if (dt > 0.0f && std::abs(deltaY) > 0.5f) {
            touchLaunchVelocity_ = -deltaY / dt;
        }

        slopAccumulator += deltaY;
        if (std::abs(slopAccumulator) < TOUCH_SLOP) return;

        // Suppress scroll while a text-selection drag is active
        if (pressState == PressState::Dragging || pressState == PressState::LongPress)
            return;

        didScroll_ = true;
        float effectiveDelta = slopAccumulator;
        slopAccumulator = 0.0f;

        eventBus_.Emit(theword::event::ScrollEvent{-effectiveDelta, true});
    } else if (scrollActive_) {
        scrollActive_ = false;
        // Don't emit scroll momentum after a text-selection gesture
        if (pressState != PressState::Dragging && pressState != PressState::LongPress)
            eventBus_.Emit(theword::event::ScrollEvent{0.0f, false, touchLaunchVelocity_});
    }
}

void InputHandler::RunUnifiedFSM(bool isPressed, bool justPressed, bool justReleased) {
    Vector2 pos = platform::HasTouchInput() ? GetTouchPosition(0) : GetMousePosition();
    if (platform::HasTouchInput() && isPressed)
        lastTouchPos_ = pos;
    Vector2 cbPos = platform::HasTouchInput() ? lastTouchPos_ : pos;

    switch (pressState) {
        case PressState::Idle:
            if (justPressed) {
                pressStartTime = GetTime();
                pressStartPos = pos;
                if (platform::HasTouchInput())
                    lastTouchPos_ = pos;
                pressStartHit = hitTestFn ? hitTestFn(pressStartPos.x, pressStartPos.y) : HitInfo{};
                lastDragWord_ = pressStartHit.wordId;
                didScroll_ = false;
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (justReleased) {
                // Check footnote hit first (higher z-order)
                if (!didScroll_ && hitTestFootnoteFn) {
                    int fi = hitTestFootnoteFn(pressStartPos.x, pressStartPos.y);
                    if (fi >= 0 && onFootnoteTap) {
                        onFootnoteTap(fi);
                        pressState = PressState::Idle;
                        return;
                    }
                }
                if (pressStartHit.wordId >= 0 && !didScroll_) {
                    float tapDist = std::sqrt(
                        (pressStartPos.x - lastClickPos_.x) * (pressStartPos.x - lastClickPos_.x) +
                        (pressStartPos.y - lastClickPos_.y) * (pressStartPos.y - lastClickPos_.y));
                    bool isDouble = tapDist < DOUBLE_CLICK_DISTANCE
                        && (GetTime() - lastClickTime_) < DOUBLE_CLICK_TIME;
                    lastClickTime_ = GetTime();
                    lastClickPos_ = cbPos;
                    if (onTap) onTap(pressStartHit, cbPos, isDouble);
                } else if (pressStartHit.wordId < 0 && !didScroll_) {
                    if (onTapEmpty) onTapEmpty(cbPos);
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                if (pressStartHit.wordId >= 0) {
                    selectStartWord = pressStartHit.wordId;
                    if (onLongPress) onLongPress(pressStartHit.wordId, cbPos);
                    pressState = PressState::LongPress;
                } else {
                    pressState = PressState::Idle;
                }
            } else {
                float dx = pos.x - pressStartPos.x;
                float dy = pos.y - pressStartPos.y;
                if (dx * dx + dy * dy > LONG_PRESS_MOVE_THRESHOLD * LONG_PRESS_MOVE_THRESHOLD
                    || (platform::HasTouchInput() && didScroll_)) {
                    if (pressStartHit.wordId >= 0) {
                        if (platform::HasTouchInput()) {
                            // On touch, drag-start within the long-press window is a scroll
                            // gesture, not a text selection. Desktop keeps the drag
                            // immediately (mouse drag always means select).
                            pressState = PressState::Idle;
                        } else {
                            if (onDragStart) onDragStart(pressStartHit.wordId, cbPos);
                            pressState = PressState::Dragging;
                        }
                    } else {
                        pressState = PressState::Idle;
                    }
                }
            }
            break;

        case PressState::Dragging:
            if (isPressed) {
                HitInfo hi = hitTestFn ? hitTestFn(pos.x, pos.y) : HitInfo{};
                if (hi.wordId >= 0 && onDragUpdate) {
                    onDragUpdate(pressStartHit.wordId, hi.wordId, cbPos);
                    lastDragWord_ = hi.wordId;
                }
            }
            if (justReleased) {
                int endWord = hitTestFn ? hitTestFn(pos.x, pos.y).wordId : lastDragWord_;
                if (endWord < 0) endWord = lastDragWord_;
                if (onDragEnd) onDragEnd(pressStartHit.wordId, endWord, cbPos);
                pressState = PressState::Idle;
            }
            break;

        case PressState::LongPress:
            if (isPressed && hitTestFn) {
                HitInfo hi = hitTestFn(pos.x, pos.y);
                if (hi.wordId >= 0 && hi.wordId != pressStartHit.wordId) {
                    pressStartHit = hi;
                    if (onDragUpdate) onDragUpdate(selectStartWord, hi.wordId, cbPos);
                }
            }
            if (justReleased) {
                if (onDragEnd) onDragEnd(selectStartWord, pressStartHit.wordId, cbPos);
                pressState = PressState::Idle;
            }
            break;

        default:
            break;
    }
}

void InputHandler::HandlePinch() {
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        Vector2 t1 = GetTouchPosition(0);
        Vector2 t2 = GetTouchPosition(1);
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (lastPinchDist > 0.0f) {
            float delta = dist - lastPinchDist;
            if (delta > 5.0f) eventBus_.Emit(theword::event::FontSizeEvent{0.0f, config::FONT_SIZE_STEP});
            if (delta < -5.0f) eventBus_.Emit(theword::event::FontSizeEvent{0.0f, -config::FONT_SIZE_STEP});
        }
        lastPinchDist = dist;
        pressState = PressState::Idle;
    } else {
        lastPinchDist = 0.0f;
    }
}

void InputHandler::HandleWindowResize() {
    if (IsWindowResized()) {
        eventBus_.Emit(theword::event::ResizeEvent{GetScreenWidth(), GetScreenHeight(), 0.0f});
    }
}

} // namespace theword::input
