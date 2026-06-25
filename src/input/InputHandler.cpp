#include "InputHandler.h"
#include "core/Config.h"
#include "document/DocumentManager.h"
#include "highlight/Highlighter.h"
#include "renderer/UIManager.h"
#include "text/LayoutEngine.h"
#include <raylib.h>
#include <cmath>

InputHandler::InputHandler(DocumentManager& docManager, Highlighter& highlighter,
                           LayoutEngine& layoutEngine, UIManager& uiManager,
                           float contentTop, float scale)
    : docManager(docManager), highlighter(highlighter), layoutEngine(layoutEngine),
      uiManager(uiManager), contentTop(contentTop), scale(scale), scrollVelocity(0.0f),
      pressState(PressState::Idle), pressStartTime(0.0),
      pressStartPos{0, 0}, pressStartWord(-1),
      touchActive(false), touchLastY(0.0f), lastTouchDelta(0.0f), lastPinchDist(0.0f) {}

void InputHandler::HandleInput(float deltaTime) {
    if (IsKeyPressed(key::ESCAPE)) {
        uiManager.DismissActiveDialog();
        return;
    }

    if (uiManager.IsAboutActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.HandleAboutClick(GetMousePosition());
        }
        return;
    }

    if (uiManager.IsGoToDialogActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.HandleGoToClick(GetMousePosition());
        }
        uiManager.HandleGoToKeyboardInput();
        return;
    }

    if (uiManager.IsSettingsActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.HandleSettingsClick(GetMousePosition());
        }
        return;
    }

    if (uiManager.IsContextMenuActive() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        uiManager.HandleContextMenuClick(GetMousePosition());
        return;
    }

    if (IsKeyPressed(key::G)) {
        if (uiManager.IsContextMenuActive()) uiManager.HideContextMenu();
        uiManager.ToggleGoToDialog();
        return;
    }

    if (IsKeyPressed(key::S)) {
        if (uiManager.IsContextMenuActive()) uiManager.HideContextMenu();
        uiManager.ToggleSettings();
        return;
    }

    if (IsKeyPressed(key::A)) {
        if (uiManager.IsContextMenuActive()) uiManager.HideContextMenu();
        uiManager.ToggleAbout();
        return;
    }

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    HandlePinch();
    HandleTouchScroll();
    HandleTouchPressFSM();
#else
    HandleScroll();
    HandleRightClick();
    HandlePressFSM();
#endif

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

    docManager.ScrollBy(scrollVelocity);
}

void InputHandler::HandlePressFSM() {
    switch (pressState) {
        case PressState::Idle:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                pressStartTime = GetTime();
                pressStartPos = GetMousePosition();
                pressStartWord = docManager.HitTestWord(
                    pressStartPos.x, pressStartPos.y, docManager.GetScrollY());
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (pressStartWord >= 0) {
                    highlighter.StartSelection(pressStartWord);
                    highlighter.EndSelection();
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                if (pressStartWord >= 0) {
                    const Highlight* hl = highlighter.HighlightAtWord(pressStartWord);
                    if (hl) {
                        uiManager.ShowContextMenu(pressStartPos, hl->id, hl->typeId);
                    }
                }
            } else {
                Vector2 m = GetMousePosition();
                float dx = m.x - pressStartPos.x;
                float dy = m.y - pressStartPos.y;
                if (dx * dx + dy * dy > LONG_PRESS_MOVE_THRESHOLD * LONG_PRESS_MOVE_THRESHOLD) {
                    pressState = PressState::Dragging;
                    if (pressStartWord >= 0) highlighter.StartSelection(pressStartWord);
                    int wordId = docManager.HitTestWord(m.x, m.y, docManager.GetScrollY());
                    if (wordId >= 0) highlighter.UpdateSelection(wordId);
                }
            }
            break;

        case PressState::Dragging:
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 m = GetMousePosition();
                int wordId = docManager.HitTestWord(m.x, m.y, docManager.GetScrollY());
                if (wordId >= 0) highlighter.UpdateSelection(wordId);
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                highlighter.EndSelection();
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
        int wordId = docManager.HitTestWord(mousePos.x, mousePos.y, docManager.GetScrollY());
        if (wordId >= 0) {
            const Highlight* hl = highlighter.HighlightAtWord(wordId);
            if (hl) {
                uiManager.ShowContextMenu(mousePos, hl->id, hl->typeId);
            }
        }
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
            docManager.ScrollBy(deltaY);
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
    docManager.ScrollBy(scrollVelocity);
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
                pressStartWord = docManager.HitTestWord(
                    pressStartPos.x, pressStartPos.y, docManager.GetScrollY());
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (touchCount == 0) {
                if (pressStartWord >= 0) {
                    highlighter.StartSelection(pressStartWord);
                    highlighter.EndSelection();
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                if (pressStartWord >= 0) {
                    const Highlight* hl = highlighter.HighlightAtWord(pressStartWord);
                    if (hl) {
                        uiManager.ShowContextMenu(pressStartPos, hl->id, hl->typeId);
                    }
                }
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
            if (delta > 5.0f) uiManager.ChangeFontSize(config::FONT_SIZE_STEP);
            if (delta < -5.0f) uiManager.ChangeFontSize(-config::FONT_SIZE_STEP);
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
        float newContentWidth = GetScreenWidth() - config::CONTENT_PADDING;
        layoutEngine.SetMaxWidth(newContentWidth);
        layoutEngine.InvalidateCache();

        float scrollFraction = 0.0f;
        float totalHeight = docManager.GetTotalHeight();
        if (totalHeight > 0.0f) {
            scrollFraction = docManager.GetScrollY() / totalHeight;
        }

        docManager.InvalidateLayouts();
        docManager.SetViewportHeight(GetScreenHeight() - contentTop);

        float newTotal = docManager.GetTotalHeight();
        docManager.ScrollTo(scrollFraction * newTotal);
    }
}
