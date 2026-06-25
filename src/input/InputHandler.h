#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <raylib.h>

class DocumentManager;
class Highlighter;
class LayoutEngine;
class UIManager;

class InputHandler {
public:
    InputHandler(DocumentManager& docManager, Highlighter& highlighter,
                 LayoutEngine& layoutEngine, UIManager& uiManager, float contentTop);

    void HandleInput(float deltaTime);

private:
    enum class PressState { Idle, Pending, Dragging, LongPress };

    DocumentManager& docManager;
    Highlighter& highlighter;
    LayoutEngine& layoutEngine;
    UIManager& uiManager;
    float contentTop;

    float scrollVelocity;
    static constexpr float SCROLL_SENSITIVITY = 30.0f;
    static constexpr float KEYBOARD_SCROLL_FACTOR = 0.16f;
    static constexpr float FRICTION = 0.92f;
    static constexpr float MIN_VELOCITY = 0.1f;
    static constexpr double LONG_PRESS_TIME = 0.5;
    static constexpr float LONG_PRESS_MOVE_THRESHOLD = 10.0f;

    PressState pressState;
    double pressStartTime;
    Vector2 pressStartPos;
    int pressStartWord;
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

#endif
