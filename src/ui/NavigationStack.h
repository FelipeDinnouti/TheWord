#ifndef NAVIGATION_STACK_H
#define NAVIGATION_STACK_H

#include <memory>
#include <vector>
#include "Screen.h"

namespace theword::ui {

class NavigationStack {
public:
    void Push(std::unique_ptr<Screen> screen);
    void Pop();
    void PopAll();
    Screen* GetActive();
    const Screen* GetActive() const;
    void DrawActive();
    bool HandleInput(float deltaTime);
    bool IsOnRoot() const;
    int Size() const { return static_cast<int>(stack_.size()); }

private:
    std::vector<std::unique_ptr<Screen>> stack_;
};

} // namespace theword::ui

#endif
