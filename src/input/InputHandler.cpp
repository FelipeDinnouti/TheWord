#include "InputHandler.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>
#include "core/Config.h"
#include "core/Platform.h"
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
    suppressDragEnd_ = false;
    // Preserve double-click tracking (lastClickTime_, lastClickPos_)
}

void InputHandler::BeginFrame() {
    bool isPressed = platform::HasTouchInput() ? (GetTouchPointCount() >= 1)
                                               : IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    frame_.touchActive = platform::HasTouchInput() && isPressed;
    frame_.leftPressed = isPressed && !prevPressed_;
    frame_.leftReleased = !isPressed && prevPressed_;
    frame_.leftDown = isPressed;
    prevPressed_ = isPressed;

    Vector2 mouse = GetMousePosition();
    frame_.mouseX = mouse.x;
    frame_.mouseY = mouse.y;
    frame_.wheel = GetMouseWheelMove();
    frame_.rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    frame_.ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    frame_.keysPressed.clear();
    int key = GetKeyPressed();
    while (key != 0) {
        frame_.keysPressed.push_back(key);
        key = GetKeyPressed();
    }

    frame_.textInput.clear();
    int ch = GetCharPressed();
    while (ch != 0) {
        frame_.textInput.push_back(static_cast<char>(ch));
        ch = GetCharPressed();
    }
}

void InputHandler::Poll(float /*deltaTime*/) {
    // Press edges are captured by BeginFrame() into the frame snapshot.
    bool isPressed = frame_.leftDown;
    bool justPressed = frame_.leftPressed;
    bool justReleased = frame_.leftReleased;

    // ── Poll execution order ───────────────────────────────────────────────
    //  1. Escape → onDismiss (radial menu / context dismiss, consumed)
    //  2. Scroll / pinch / right-click (platform-specific)
    //  3. RunUnifiedFSM (word tap / drag / long-press)
    //  4. HandleWindowResize (viewport change)
    //
    // Screens handle input before this point (NavigationStack::HandleInput runs
    // in App's main loop prior to Poll). This means screens consume button-area
    // presses before the FSM can see them. (P12)
    // ────────────────────────────────────────────────────────────────────────

    // Escape → onDismiss (radial menu dismiss)
    if (frame_.KeyPressed(key::ESCAPE) && onDismiss && onDismiss()) {
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
    HandleWindowResize();

    // App shortcuts (S/A/I/D/C). Screens already consumed their keys in
    // NavigationStack::HandleInput before Poll ran; overlaps cannot double-fire.
    if (onShortcut) {
        for (int k : frame_.keysPressed) {
            if (k == key::S || k == key::A || k == key::I || k == key::D || k == key::C) {
                onShortcut(k, frame_.ctrlDown);
            }
        }
    }
}

void InputHandler::HandleScroll() {
    float wheel = frame_.wheel;
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
    if (!frame_.rightPressed) return;
    HitInfo hi = hitTestFn ? hitTestFn(frame_.mouseX, frame_.mouseY) : HitInfo{};
    if (hi.wordId >= 0 && onTap)
        onTap(hi, Vector2{frame_.mouseX, frame_.mouseY}, false);
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
    Vector2 pos = platform::HasTouchInput() ? GetTouchPosition(0)
                                            : Vector2{frame_.mouseX, frame_.mouseY};
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
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Start, selectStartWord, selectStartWord,
                        pressStartHit.bookId, pressStartHit.chapterNum});
                    if (!platform::HasTouchInput() && isHighlightedFn
                        && isHighlightedFn(pressStartHit.wordId)) {
                        eventBus_.Emit(theword::event::SelectionEvent{
                            theword::event::SelectionEvent::Action::End, selectStartWord, selectStartWord,
                            pressStartHit.bookId, pressStartHit.chapterNum});
                        suppressDragEnd_ = true;
                    } else {
                        suppressDragEnd_ = false;
                    }
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
                            eventBus_.Emit(theword::event::SelectionEvent{
                                theword::event::SelectionEvent::Action::Start,
                                pressStartHit.wordId, pressStartHit.wordId,
                                pressStartHit.bookId, pressStartHit.chapterNum});
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
                if (hi.wordId >= 0) {
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Update,
                        pressStartHit.wordId, hi.wordId,
                        pressStartHit.bookId, pressStartHit.chapterNum});
                    lastDragWord_ = hi.wordId;
                }
            }
            if (justReleased) {
                int endWord = hitTestFn ? hitTestFn(pos.x, pos.y).wordId : lastDragWord_;
                if (endWord < 0) endWord = lastDragWord_;
                if (suppressDragEnd_) {
                    suppressDragEnd_ = false;
                } else {
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::End,
                        pressStartHit.wordId, endWord,
                        pressStartHit.bookId, pressStartHit.chapterNum});
                    if (onDragEnd) onDragEnd(pressStartHit.wordId, endWord, cbPos);
                }
                pressState = PressState::Idle;
            }
            break;

        case PressState::LongPress:
            if (isPressed && hitTestFn) {
                HitInfo hi = hitTestFn(pos.x, pos.y);
                if (hi.wordId >= 0 && hi.wordId != pressStartHit.wordId) {
                    pressStartHit = hi;
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Update,
                        selectStartWord, hi.wordId, hi.bookId, hi.chapterNum});
                }
            }
            if (justReleased) {
                if (suppressDragEnd_) {
                    suppressDragEnd_ = false;
                } else {
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::End,
                        selectStartWord, pressStartHit.wordId,
                        pressStartHit.bookId, pressStartHit.chapterNum});
                    if (onDragEnd) onDragEnd(selectStartWord, pressStartHit.wordId, cbPos);
                }
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
        eventBus_.Emit(theword::event::ResizeEvent{GetScreenWidth(), GetScreenHeight()});
    }
}

} // namespace theword::input
