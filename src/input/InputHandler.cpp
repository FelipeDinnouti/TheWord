#include "InputHandler.h"
#include "core/Config.h"
#include "document/DocumentManager.h"
#include "highlight/Highlighter.h"
#include "renderer/UIManager.h"
#include "text/LayoutEngine.h"
#include <raylib.h>
#include <cmath>

InputHandler::InputHandler(DocumentManager& docManager, Highlighter& highlighter,
                           LayoutEngine& layoutEngine, UIManager& uiManager, float contentTop)
    : docManager(docManager), highlighter(highlighter), layoutEngine(layoutEngine),
      uiManager(uiManager), contentTop(contentTop), scrollVelocity(0.0f),
      pressState(PressState::Idle), pressStartTime(0.0),
      pressStartPos{0, 0}, pressStartWord(-1),
      touchActive(false), touchLastY(0.0f), lastTouchDelta(0.0f), lastPinchDist(0.0f) {}

void InputHandler::handleInput(float deltaTime) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        uiManager.dismissActiveDialog();
        return;
    }

    if (uiManager.isAboutActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.handleAboutClick(GetMousePosition());
        }
        return;
    }

    if (uiManager.isGoToDialogActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.handleGoToClick(GetMousePosition());
        }
        uiManager.handleGoToKeyboardInput();
        return;
    }

    if (uiManager.isSettingsActive()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            uiManager.handleSettingsClick(GetMousePosition());
        }
        return;
    }

    if (uiManager.isContextMenuActive() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        uiManager.handleContextMenuClick(GetMousePosition());
        return;
    }

    if (IsKeyPressed(KEY_G)) {
        if (uiManager.isContextMenuActive()) uiManager.hideContextMenu();
        uiManager.toggleGoToDialog();
        return;
    }

    if (IsKeyPressed(KEY_S)) {
        if (uiManager.isContextMenuActive()) uiManager.hideContextMenu();
        uiManager.toggleSettings();
        return;
    }

    if (IsKeyPressed(KEY_A)) {
        if (uiManager.isContextMenuActive()) uiManager.hideContextMenu();
        uiManager.toggleAbout();
        return;
    }

#ifdef __EMSCRIPTEN__
    handlePinch();
    handleTouchScroll();
    handleTouchPressFSM();
#else
    handleScroll();
    handleRightClick();
    handlePressFSM();
#endif

    handleWindowResize();
}

void InputHandler::handleScroll() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scrollVelocity -= wheel * SCROLL_SENSITIVITY;
    }

    if (IsKeyDown(KEY_DOWN)) {
        scrollVelocity += SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR;
    }
    if (IsKeyDown(KEY_UP)) {
        scrollVelocity -= SCROLL_SENSITIVITY * KEYBOARD_SCROLL_FACTOR;
    }

    scrollVelocity *= FRICTION;
    if (std::abs(scrollVelocity) < MIN_VELOCITY) {
        scrollVelocity = 0.0f;
    }

    docManager.scrollBy(scrollVelocity);
}

void InputHandler::handlePressFSM() {
    switch (pressState) {
        case PressState::Idle:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                pressStartTime = GetTime();
                pressStartPos = GetMousePosition();
                pressStartWord = docManager.hitTestWord(
                    pressStartPos.x, pressStartPos.y, docManager.getScrollY());
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (pressStartWord >= 0) {
                    highlighter.startSelection(pressStartWord);
                    highlighter.endSelection();
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                if (pressStartWord >= 0) {
                    const Highlight* hl = highlighter.highlightAtWord(pressStartWord);
                    if (hl) {
                        uiManager.showContextMenu(pressStartPos, hl->id, hl->typeId);
                    }
                }
            } else {
                Vector2 m = GetMousePosition();
                float dx = m.x - pressStartPos.x;
                float dy = m.y - pressStartPos.y;
                if (dx * dx + dy * dy > LONG_PRESS_MOVE_THRESHOLD * LONG_PRESS_MOVE_THRESHOLD) {
                    pressState = PressState::Dragging;
                    if (pressStartWord >= 0) highlighter.startSelection(pressStartWord);
                    int wordId = docManager.hitTestWord(m.x, m.y, docManager.getScrollY());
                    if (wordId >= 0) highlighter.updateSelection(wordId);
                }
            }
            break;

        case PressState::Dragging:
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 m = GetMousePosition();
                int wordId = docManager.hitTestWord(m.x, m.y, docManager.getScrollY());
                if (wordId >= 0) highlighter.updateSelection(wordId);
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                highlighter.endSelection();
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

void InputHandler::handleRightClick() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePos = GetMousePosition();
        int wordId = docManager.hitTestWord(mousePos.x, mousePos.y, docManager.getScrollY());
        if (wordId >= 0) {
            const Highlight* hl = highlighter.highlightAtWord(wordId);
            if (hl) {
                uiManager.showContextMenu(mousePos, hl->id, hl->typeId);
            }
        }
    }
}

void InputHandler::handleTouchScroll() {
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
            docManager.scrollBy(deltaY);
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
    docManager.scrollBy(scrollVelocity);
}

void InputHandler::handleTouchPressFSM() {
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
                pressStartWord = docManager.hitTestWord(
                    pressStartPos.x, pressStartPos.y, docManager.getScrollY());
                pressState = PressState::Pending;
            }
            break;

        case PressState::Pending:
            if (touchCount == 0) {
                if (pressStartWord >= 0) {
                    highlighter.startSelection(pressStartWord);
                    highlighter.endSelection();
                }
                pressState = PressState::Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                pressState = PressState::LongPress;
                if (pressStartWord >= 0) {
                    const Highlight* hl = highlighter.highlightAtWord(pressStartWord);
                    if (hl) {
                        uiManager.showContextMenu(pressStartPos, hl->id, hl->typeId);
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

void InputHandler::handlePinch() {
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        Vector2 t1 = GetTouchPosition(0);
        Vector2 t2 = GetTouchPosition(1);
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (lastPinchDist > 0.0f) {
            float delta = dist - lastPinchDist;
            if (delta > 5.0f) uiManager.changeFontSize(config::FONT_SIZE_STEP);
            if (delta < -5.0f) uiManager.changeFontSize(-config::FONT_SIZE_STEP);
        }
        lastPinchDist = dist;
        touchActive = false;
        pressState = PressState::Idle;
    } else {
        lastPinchDist = 0.0f;
    }
}

void InputHandler::handleWindowResize() {
    if (IsWindowResized()) {
        float newContentWidth = GetScreenWidth() - 40.0f;
        layoutEngine.setMaxWidth(newContentWidth);
        layoutEngine.invalidateCache();

        float scrollFraction = 0.0f;
        float totalHeight = docManager.getTotalHeight();
        if (totalHeight > 0.0f) {
            scrollFraction = docManager.getScrollY() / totalHeight;
        }

        docManager.invalidateLayouts();
        docManager.setViewportHeight(GetScreenHeight() - contentTop);

        float newTotal = docManager.getTotalHeight();
        docManager.scrollTo(scrollFraction * newTotal);
    }
}
