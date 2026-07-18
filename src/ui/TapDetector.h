#ifndef TAP_DETECTOR_H
#define TAP_DETECTOR_H

#include <raylib.h>

namespace theword::ui {

class TapDetector {
public:
    explicit TapDetector(float slopDp) : slopSq_(slopDp * slopDp) {}

    void OnPress(Vector2 pos) {
        pressStartPos_ = pos;
        hasPendingPress_ = true;
    }

    enum class Result { None, Drag, Tap };

    Result OnRelease(Vector2 releasePos, Vector2& tapPos) {
        if (!hasPendingPress_) return Result::None;
        hasPendingPress_ = false;
        float dx = releasePos.x - pressStartPos_.x;
        float dy = releasePos.y - pressStartPos_.y;
        if (dx * dx + dy * dy > slopSq_)
            return Result::Drag;
        tapPos = releasePos;
        return Result::Tap;
    }

    void Reset() { hasPendingPress_ = false; }

private:
    Vector2 pressStartPos_{};
    bool hasPendingPress_ = false;
    float slopSq_;
};

} // namespace theword::ui

#endif
