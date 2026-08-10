#ifndef TAP_DETECTOR_H
#define TAP_DETECTOR_H

namespace theword::ui {

class TapDetector {
public:
    explicit TapDetector(float slopDp) : slopSq_(slopDp * slopDp) {}

    void OnPress(float x, float y) {
        pressStartX_ = x;
        pressStartY_ = y;
        hasPendingPress_ = true;
    }

    enum class Result { None, Drag, Tap };

    Result OnRelease(float releaseX, float releaseY, float& tapX, float& tapY) {
        if (!hasPendingPress_) return Result::None;
        hasPendingPress_ = false;
        float dx = releaseX - pressStartX_;
        float dy = releaseY - pressStartY_;
        if (dx * dx + dy * dy > slopSq_)
            return Result::Drag;
        tapX = releaseX;
        tapY = releaseY;
        return Result::Tap;
    }

    void Reset() { hasPendingPress_ = false; }

private:
    float pressStartX_ = 0.0f;
    float pressStartY_ = 0.0f;
    bool hasPendingPress_ = false;
    float slopSq_;
};

} // namespace theword::ui

#endif