#include "renderer/DrawContext.h"

#include <raylib.h>

namespace theword::renderer {

void DrawContext::PushClipRect(float x, float y, float w, float h) {
    BeginScissorMode(static_cast<int>(x), static_cast<int>(y),
                     static_cast<int>(w), static_cast<int>(h));
}

void DrawContext::PopClipRect() {
    EndScissorMode();
}

void DrawContext::SetCursor(int cursorKind) {
    SetMouseCursor(cursorKind);
}

} // namespace theword::renderer