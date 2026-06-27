#ifndef SCREEN_H
#define SCREEN_H

namespace theword::ui {

class Screen {
public:
    virtual ~Screen() = default;
    virtual void Draw() = 0;
    virtual bool HandleInput(float deltaTime) = 0;
    virtual const char* GetTitle() const = 0;
    virtual bool IsOverlay() const { return false; }
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
};

} // namespace theword::ui

#endif
