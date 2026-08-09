#include "NavigationStack.h"

namespace theword::ui {

void NavigationStack::Push(std::unique_ptr<Screen> screen) {
    if (!stack_.empty()) {
        stack_.back()->OnDeactivate();
    }
    screen->OnActivate();
    stack_.push_back(std::move(screen));
}

void NavigationStack::Pop() {
    if (stack_.size() <= 1) return;
    stack_.back()->OnDeactivate();
    stack_.pop_back();
    stack_.back()->OnActivate();
}

void NavigationStack::PopAll() {
    while (stack_.size() > 1) {
        stack_.back()->OnDeactivate();
        stack_.pop_back();
    }
    stack_.back()->OnActivate();
}

Screen* NavigationStack::GetActive() {
    return stack_.empty() ? nullptr : stack_.back().get();
}

const Screen* NavigationStack::GetActive() const {
    return stack_.empty() ? nullptr : stack_.back().get();
}

void NavigationStack::DrawActive(theword::renderer::DrawContext& ctx) {
    if (stack_.empty()) return;

    auto* active = stack_.back().get();
    if (active->IsOverlay() && stack_.size() >= 2) {
        auto* below = stack_[stack_.size() - 2].get();
        below->Draw(ctx);
    }
    active->Draw(ctx);
}

bool NavigationStack::HandleInput(float deltaTime) {
    if (stack_.empty()) return false;
    return stack_.back()->HandleInput(deltaTime);
}

bool NavigationStack::IsOnRoot() const {
    return stack_.size() == 1;
}

} // namespace theword::ui
