#include "core/InputFrame.h"

#include <algorithm>

namespace theword::core {

bool InputFrame::KeyPressed(int key) const {
    return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
}

} // namespace theword::core