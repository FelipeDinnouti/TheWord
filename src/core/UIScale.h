#ifndef UISCALE_H
#define UISCALE_H

#include <algorithm>

namespace theword::core {

struct UIScale {
    float dpiScale;
    float screenW;
    float screenH;
    float bottomInset;

    UIScale(float dpi, float w, float h, float bottom = 0)
        : dpiScale(dpi), screenW(w), screenH(h), bottomInset(bottom) {}

    void OnResize(float w, float h) { screenW = w; screenH = h; }

    float dp(float n) const { return n * dpiScale; }
    float vw(float percent) const { return screenW * percent / 100.0f; }
    float vh(float percent) const { return screenH * percent / 100.0f; }

    float fitScreen(float vwPct, float dpMax) const {
        return std::min(vw(vwPct), dp(dpMax));
    }
};

} // namespace theword::core

#endif
