#include "RadialMenu.h"
#include "highlight/PersistenceInterface.h"
#include <cmath>
#include <algorithm>

namespace theword::renderer {

static constexpr float HIT_SCALE = 1.8f;

RadialMenu::RadialMenu(float dpiScale)
    : center_{0, 0}, startWord_(-1), endWord_(-1), bookId_{}, chapterNum_(0), active_(false), dpiScale_(dpiScale), hoveredIndex_(-1) {}

void RadialMenu::Show(Vector2 center, int startWord, int endWord,
                       const std::string& bookId, int chapterNum,
                       const std::vector<theword::highlight::HighlightType>& types) {
    center_ = center;
    startWord_ = startWord;
    endWord_ = endWord;
    bookId_ = bookId;
    chapterNum_ = chapterNum;
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
    float ringRadius = 56.0f * dpiScale_;
    float btnRadius = 18.0f * dpiScale_;

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

int RadialMenu::GetSectorIndex(Vector2 pos) const {
    if (!active_ || buttons_.empty()) return -1;

    float dx = pos.x - center_.x;
    float dy = pos.y - center_.y;
    float dist = sqrtf(dx * dx + dy * dy);

    float ringR = 56.0f * dpiScale_;
    float btnR = 18.0f * dpiScale_;

    if (dist > ringR + btnR * HIT_SCALE) return -1;
    if (dist < ringR * 0.35f) return -1;

    int count = static_cast<int>(buttons_.size());
    float sectorAngle = 2.0f * PI / static_cast<float>(count);
    float startAngle = -PI / 2.0f;
    float tapAngle = atan2f(dy, dx);
    float offset = tapAngle - (startAngle - sectorAngle * 0.5f);
    offset = fmodf(offset, 2.0f * PI);
    if (offset < 0.0f) offset += 2.0f * PI;

    int idx = static_cast<int>(offset / sectorAngle);
    return (idx >= 0 && idx < count) ? idx : -1;
}

void RadialMenu::UpdateHover(Vector2 mousePos) {
    hoveredIndex_ = GetSectorIndex(mousePos);
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

    int idx = GetSectorIndex(pos);
    if (idx >= 0) {
        return {buttons_[idx].action, buttons_[idx].colorIndex};
    }

    return {Action::None, -1};
}

} // namespace theword::renderer
