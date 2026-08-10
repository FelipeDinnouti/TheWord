#ifndef INPUT_FRAME_H
#define INPUT_FRAME_H

#include <string>
#include <vector>

namespace theword::core {

struct InputFrame {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float wheel = 0.0f;
    bool leftPressed = false;
    bool leftDown = false;
    bool leftReleased = false;
    bool rightPressed = false;
    bool touchActive = false;
    bool ctrlDown = false;
    std::vector<int> keysPressed;
    std::string textInput;

    bool KeyPressed(int key) const;
};

} // namespace theword::core

#endif