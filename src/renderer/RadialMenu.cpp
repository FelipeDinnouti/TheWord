#include "RadialMenu.h"
#include "highlight/PersistenceInterface.h"
#include "core/Theme.h"
#include <cmath>
#include <algorithm>

namespace theword::renderer {

using namespace theword::core;

RadialMenu::RadialMenu()
    : center_{0, 0}, startWord_(-1), endWord_(-1), active_(false), hoveredIndex_(-1) {}

void RadialMenu::Show(Vector2 center, int startWord, int endWord,
                       const std::vector<theword::highlight::HighlightType>& types) {
    center_ = center;
    startWord_ = startWord;
    endWord_ = endWord;
    active_ = true;
    hoveredIndex_ = -1;
    LayoutButtons(types);
}

void RadialMenu::Hide() {
    active_ = false;
    buttons_.clear();
    hoveredIndex_ = -1;
}

bool RadialMenu::IsActive() const { return active_; }

void RadialMenu::LayoutButtons(const std::vector<theword::highlight::HighlightType>& types) {
    buttons_.clear();

    int numColors = static_cast<int>(types.size());
    int count = numColors + 2; // colors + Copy + Delete
    float ringRadius = 56.0f;
    float btnRadius = 18.0f;

    float startAngle = -PI / 2.0f;

    for (int i = 0; i < count; ++i) {
        float angle = startAngle + (static_cast<float>(i) / count) * 2.0f * PI;
        Button btn;
        btn.center = {
            center_.x + cosf(angle) * ringRadius,
            center_.y + sinf(angle) * ringRadius
        };
        btn.radius = btnRadius;

        if (i < numColors) {
            btn.action = Action::Highlight;
            btn.colorIndex = i;
            btn.fillColor = {types[i].color.r, types[i].color.g, types[i].color.b, 230};
            btn.borderColor = GRAY;
            btn.label = "";
        } else if (i == numColors) {
            btn.action = Action::Copy;
            btn.fillColor = {70, 130, 180, 220};
            btn.borderColor = GRAY;
            btn.label = "C";
        } else {
            btn.action = Action::Delete;
            btn.fillColor = {200, 60, 60, 220};
            btn.borderColor = GRAY;
            btn.label = "X";
        }

        buttons_.push_back(btn);
    }
}

void RadialMenu::UpdateHover(Vector2 mousePos) {
    hoveredIndex_ = -1;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        float dx = mousePos.x - buttons_[i].center.x;
        float dy = mousePos.y - buttons_[i].center.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= buttons_[i].radius) {
            hoveredIndex_ = static_cast<int>(i);
            return;
        }
    }
}

void RadialMenu::Draw() {
    if (!active_) return;

    float mx = static_cast<float>(GetMouseX());
    float my = static_cast<float>(GetMouseY());
    UpdateHover({mx, my});

    for (size_t i = 0; i < buttons_.size(); ++i) {
        const auto& btn = buttons_[i];
        bool hovered = (static_cast<int>(i) == hoveredIndex_);

        Color fill = btn.fillColor;
        Color border = btn.borderColor;
        if (hovered) {
            fill.a = static_cast<unsigned char>(std::min(255, static_cast<int>(fill.a) + 25));
            border = WHITE;
        }

        DrawCircleV(btn.center, btn.radius, fill);
        DrawCircleLinesV(btn.center, btn.radius, border);

        if (!btn.label.empty()) {
            float fontSize = btn.radius * 1.1f;
            Vector2 textSize = MeasureTextEx(GetFontDefault(), btn.label.c_str(), fontSize, 1);
            DrawTextEx(GetFontDefault(), btn.label.c_str(),
                       {btn.center.x - textSize.x / 2.0f,
                        btn.center.y - textSize.y / 2.0f},
                       fontSize, 1, WHITE);
        }
    }
}

std::pair<RadialMenu::Action, int> RadialMenu::HandleClick(Vector2 pos) {
    if (!active_) return {Action::None, -1};

    for (const auto& btn : buttons_) {
        float dx = pos.x - btn.center.x;
        float dy = pos.y - btn.center.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= btn.radius) {
            return {btn.action, btn.colorIndex};
        }
    }

    return {Action::None, -1};
}

} // namespace theword::renderer
