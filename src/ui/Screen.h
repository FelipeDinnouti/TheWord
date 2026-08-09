#ifndef SCREEN_H
#define SCREEN_H

#include "renderer/DrawContext.h"

namespace theword::ui {

class Screen {
public:
    virtual ~Screen() = default;
    virtual void Draw(theword::renderer::DrawContext& ctx) = 0;
    virtual bool HandleInput(float deltaTime) = 0;
    virtual const char* GetTitle() const = 0;
    virtual bool IsOverlay() const { return false; }
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
};

} // namespace theword::ui

#endif
