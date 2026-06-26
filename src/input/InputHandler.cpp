#include "InputHandler.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "core/Config.h"
#include "core/Platform.h"
#include <raylib.h>
#include <cmath>

namespace theword::input {

using namespace theword::core;

InputHandler::InputHandler(theword::event::EventBus& eventBus,
                           float contentTop, float scale,
                           std::function<int(float, float)> hitTestFn)
    : eventBus_(eventBus), contentTop(contentTop), scale(scale),
      hitTestFn(std::move(hitTestFn)), scrollVelocity(0.0f),
      pressState(PressState::Idle), pressStartTime(0.0),
      pressStartPos{0, 0}, pressStartWord(-1),
      touchActive(false), touchLastY(0.0f), lastTouchDelta(0.0f), lastPinchDist(0.0f) {

    eventBus_.On<theword::event::DialogEvent>([this](const theword::event::DialogEvent& e) {
        dialogActive_ = (e.action != theword::event::DialogEvent::Action::Hide);
    });
}

void InputHandler::Poll(float /*deltaTime*/) {
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

    if (IsKeyPressed(key::G)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::GoTo, theword::event::DialogEvent::Action::Toggle}); return; }
    if (IsKeyPressed(key::S)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::Settings, theword::event::DialogEvent::Action::Toggle}); return; }
    if (IsKeyPressed(key::A)) { eventBus_.Emit(theword::event::DialogEvent{theword::event::DialogEvent::Type::About, theword::event::DialogEvent::Action::Toggle}); return; }

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
        scrollVelocity -= wheel * SCROLL_SENSITIVITY;
    }

    if (IsKeyDown(key::DOWN)) {
        scrollVelocity += SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR;
    }
    if (IsKeyDown(key::UP)) {
        scrollVelocity -= SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR;
    }

    scrollVelocity *= FRICTION;
    if (std::abs(scrollVelocity) < MIN_VELOCITY) {
        scrollVelocity = 0.0f;
    }

    eventBus_.Emit(theword::event::ScrollEvent{scrollVelocity});
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
                        theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord});
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::End, pressStartWord, pressStartWord});
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
                            theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord});
                    }
                    if (hitTestFn) {
                        int wordId = hitTestFn(m.x, m.y);
                        if (wordId >= 0) {
                            eventBus_.Emit(theword::event::SelectionEvent{
                                theword::event::SelectionEvent::Action::Update, pressStartWord, wordId});
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
                            theword::event::SelectionEvent::Action::Update, pressStartWord, wordId});
                    }
                }
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                eventBus_.Emit(theword::event::SelectionEvent{
                    theword::event::SelectionEvent::Action::End, pressStartWord, pressStartWord});
                pressState = PressState::Idle;
            }
            break;

        case PressState::LongPress:
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                pressState = PressState::Idle;
            }
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
        Vector2 pos = GetTouchPosition(0);
        if (!touchActive) {
            touchActive = true;
            touchLastY = pos.y;
            lastTouchDelta = 0.0f;
        } else {
            float deltaY = pos.y - touchLastY;
            touchLastY = pos.y;
            lastTouchDelta = deltaY;
            eventBus_.Emit(theword::event::ScrollEvent{deltaY});
        }
    } else {
        if (touchActive) {
            touchActive = false;
            scrollVelocity = lastTouchDelta * 10.0f;
        }
    }

    scrollVelocity *= FRICTION;
    if (std::abs(scrollVelocity) < MIN_VELOCITY) {
        scrollVelocity = 0.0f;
    }
    eventBus_.Emit(theword::event::ScrollEvent{scrollVelocity});
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
                if (pressStartWord >= 0) {
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::Start, pressStartWord, pressStartWord});
                    eventBus_.Emit(theword::event::SelectionEvent{
                        theword::event::SelectionEvent::Action::End, pressStartWord, pressStartWord});
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                eventBus_.Emit(theword::event::RightClickEvent{pressStartPos.x, pressStartPos.y});
            } else {
                float dy = touchPos.y - pressStartPos.y;
                if (std::abs(dy) > LONG_PRESS_MOVE_THRESHOLD) {
                    pressState = PressState::Idle;
                }
            }
            break;

        case PressState::LongPress:
            if (touchCount == 0) {
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
