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
                           std::function<int(float, float)> hitTestFn,
                           std::function<bool(int)> isHighlightedFn)
    : eventBus_(eventBus),
      hitTestFn(std::move(hitTestFn)),
      isHighlightedFn(std::move(isHighlightedFn)),
      scrollVelocity(0.0f),
      touchVelocity(0.0f),
      deltaHistory{},
      deltaHistoryIdx(0),
      slopAccumulator(0.0f),
      pressState(PressState::Idle), pressStartTime(0.0),
      pressStartPos{0, 0}, pressStartWord(-1), selectStartWord(-1),
      touchActive(false), touchLastY(0.0f), lastTouchDelta(0.0f), lastPinchDist(0.0f) {

    eventBus_.On<theword::event::DialogEvent>([this](const theword::event::DialogEvent& e) {
        dialogActive_ = (e.action != theword::event::DialogEvent::Action::Hide);
    });

    eventBus_.On<theword::event::ScrollStopEvent>([this](const auto&) {
        scrollVelocity = 0.0f;
    });
}

void InputHandler::Poll(float deltaTime, theword::ui::NavigationStack* navStack,
                        ContextMenuHandler contextMenuHandler,
                        ContextDismissHandler contextDismissHandler) {
    // Context menu gets first chance to consume clicks and escape
    if (IsKeyPressed(key::ESCAPE) && contextDismissHandler && contextDismissHandler()) {
        return;
    }
    if (contextMenuHandler && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (contextMenuHandler(GetMousePosition())) {
            return;
        }
    }

    // Let the active screen process input first — if it consumes it, skip normal handling
    if (navStack && navStack->HandleInput(deltaTime)) return;

    if (IsKeyPressed(key::ESCAPE)) {
        eventBus_.Emit(theword::event::KeyEvent{key::ESCAPE});
        return;
    }

    if (dialogActive_) {
        if (IsKeyPressed(key::G)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::GoTo, theword::event::DialogEvent::Action::Toggle}); return; }
        if (IsKeyPressed(key::S)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::Settings, theword::event::DialogEvent::Action::Toggle}); return; }
        if (IsKeyPressed(key::A)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::About, theword::event::DialogEvent::Action::Toggle}); return; }
        return;
    }

    if (platform::HasTouchInput()) {
        HandlePinch();
        HandleTouchScroll();
        HandleTouchPressFSM();
    } else {
        HandleScroll();
        HandleRightClick();
        HandlePressFSM();
    }

    HandleWindowResize();
}

void InputHandler::HandleScroll() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scrollVelocity = 0.0f;
        eventBus_.Emit(theword::event::ScrollEvent{-wheel * SCROLL_SENSITIVITY});
        return;
    }

    if (IsKeyDown(key::DOWN)) {
        scrollVelocity = 0.0f;
        eventBus_.Emit(theword::event::ScrollEvent{SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR});
        return;
    }
    if (IsKeyDown(key::UP)) {
        scrollVelocity = 0.0f;
        eventBus_.Emit(theword::event::ScrollEvent{-SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR});
        return;
    }
}

void InputHandler::HandlePressFSM() {
    switch (pressState) {
        case PressState::Idle:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                pressStartTime = GetTime();
                pressStartPos = GetMousePosition();
                if (hitTestFn) {
                    pressStartWord = hitTestFn(pressStartPos.x, pressStartPos.y);
                } else {
                    pressStartWord = -1;
                }
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (pressStartWord >= 0) {
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord, {}, 0});
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::End, pressStartWord, pressStartWord, {}, 0});
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                eventBus_.Emit(theword::event::RightClickEvent{pressStartPos.x, pressStartPos.y});
            } else {
                Vector2 m = GetMousePosition();
                float dx = m.x - pressStartPos.x;
                float dy = m.y - pressStartPos.y;
                if (dx * dx + dy * dy > LONG_PRESS_MOVE_THRESHOLD * LONG_PRESS_MOVE_THRESHOLD) {
                    pressState = PressState::Dragging;
                    if (pressStartWord >= 0) {
                        eventBus_.Emit(theword::event::SelectionEvent{
                            theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord, {}, 0});
                    }
                    if (hitTestFn) {
                        int wordId = hitTestFn(m.x, m.y);
                        if (wordId >= 0) {
                            eventBus_.Emit(theword::event::SelectionEvent{
                                theword::event::SelectionEvent::Action::Update, pressStartWord, wordId, {}, 0});
                        }
                    }
                }
            }
            break;

        case PressState::Dragging:
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (hitTestFn) {
                    Vector2 m = GetMousePosition();
                    int wordId = hitTestFn(m.x, m.y);
                    if (wordId >= 0) {
                        eventBus_.Emit(theword::event::SelectionEvent{
                            theword::event::SelectionEvent::Action::Update, pressStartWord, wordId, {}, 0});
                    }
                }
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                eventBus_.Emit(theword::event::SelectionEvent{
                    theword::event::SelectionEvent::Action::End, pressStartWord, pressStartWord, {}, 0});
                pressState = PressState::Idle;
            }
            break;

        case PressState::LongPress:
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                pressState = PressState::Idle;
            }
            break;

        default:
            break;
    }
}

void InputHandler::HandleRightClick() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePos = GetMousePosition();
        eventBus_.Emit(theword::event::RightClickEvent{mousePos.x, mousePos.y});
    }
}

void InputHandler::HandleTouchScroll() {
    int touchCount = GetTouchPointCount();
    if (touchCount == 1) {
        if (pressState == PressState::Selecting) return;

        Vector2 pos = GetTouchPosition(0);
        if (!touchActive) {
            touchActive = true;
            touchLastY = pos.y;
            lastTouchDelta = 0.0f;
            touchVelocity = 0.0f;
            deltaHistoryIdx = 0;
            slopAccumulator = 0.0f;
            return;
        }

        float deltaY = pos.y - touchLastY;
        touchLastY = pos.y;
        lastTouchDelta = deltaY;

        slopAccumulator += deltaY;
        if (std::abs(slopAccumulator) < TOUCH_SLOP) {
            touchVelocity = 0.0f;
            return;
        }

        float effectiveDelta = slopAccumulator;
        slopAccumulator = 0.0f;

        deltaHistory[deltaHistoryIdx % DELTA_HISTORY_SIZE] = effectiveDelta;
        deltaHistoryIdx++;
        int count = std::min(deltaHistoryIdx, DELTA_HISTORY_SIZE);
        float sumDelta = 0.0f;
        for (int i = 0; i < count; i++) sumDelta += deltaHistory[i];

        float dt = GetFrameTime();
        if (dt > 0.0f && dt < 0.1f) {
            float smoothVel = sumDelta / (count * dt);
            touchVelocity = smoothVel * VELOCITY_ALPHA + touchVelocity * (1.0f - VELOCITY_ALPHA);
        }

        eventBus_.Emit(theword::event::ScrollEvent{-effectiveDelta, true});
    } else {
        if (touchActive) {
            touchActive = false;
            scrollVelocity = -touchVelocity;
            if (scrollVelocity > MAX_MOMENTUM_VELOCITY)
                scrollVelocity = MAX_MOMENTUM_VELOCITY;
            if (scrollVelocity < -MAX_MOMENTUM_VELOCITY)
                scrollVelocity = -MAX_MOMENTUM_VELOCITY;
            eventBus_.Emit(theword::event::ScrollEvent{scrollVelocity * GetFrameTime()});
            return;
        }

        if (scrollVelocity != 0.0f) {
            float dt = GetFrameTime();
            scrollVelocity *= std::exp(-dt / MOMENTUM_TIME_CONSTANT);

            if (std::abs(scrollVelocity) < MIN_VELOCITY) {
                scrollVelocity = 0.0f;
            } else {
                eventBus_.Emit(theword::event::ScrollEvent{scrollVelocity * dt});
            }
        }
    }
}

void InputHandler::HandleTouchPressFSM() {
    int touchCount = GetTouchPointCount();
    Vector2 touchPos = {};
    if (touchCount >= 1) {
        touchPos = GetTouchPosition(0);
    }

    switch (pressState) {
        case PressState::Idle:
            if (touchCount == 1) {
                pressStartTime = GetTime();
                pressStartPos = touchPos;
                if (hitTestFn) {
                    pressStartWord = hitTestFn(pressStartPos.x, pressStartPos.y);
                } else {
                    pressStartWord = -1;
                }
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (touchCount == 0) {
                // Tap: show context menu if word is highlighted, otherwise do nothing
                if (pressStartWord >= 0 && isHighlightedFn && isHighlightedFn(pressStartWord)) {
                    eventBus_.Emit(theword::event::RightClickEvent{pressStartPos.x, pressStartPos.y});
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                // Longpress: start highlighting selection
                if (pressStartWord >= 0) {
                    selectStartWord = pressStartWord;
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord, {}, 0});
                    pressState = PressState::Selecting;
                } else {
                    pressState = PressState::Idle;
                }
            } else {
                float dy = touchPos.y - pressStartPos.y;
                if (std::abs(dy) > LONG_PRESS_MOVE_THRESHOLD) {
                    pressState = PressState::Idle;
                }
            }
            break;

        case PressState::Selecting:
            if (touchCount == 0) {
                // Release: end selection and save highlight
                eventBus_.Emit(theword::event::SelectionEvent{
                    theword::event::SelectionEvent::Action::End, selectStartWord, pressStartWord, {}, 0});
                pressState = PressState::Idle;
            } else {
                // Drag: extend selection range
                if (hitTestFn) {
                    int wordId = hitTestFn(touchPos.x, touchPos.y);
                    if (wordId >= 0 && wordId != pressStartWord) {
                        pressStartWord = wordId;
                        eventBus_.Emit(theword::event::SelectionEvent{
                            theword::event::SelectionEvent::Action::Update, selectStartWord, wordId, {}, 0});
                    }
                }
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
        touchActive = false;
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
