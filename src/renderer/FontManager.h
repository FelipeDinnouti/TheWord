#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <vector>
#include <raylib.h>
#include "text/TextMeasure.h"

namespace theword::core { class IAssetProvider; }

namespace theword::renderer {

class FontManager {
public:
    FontManager(float dpiScale);
    ~FontManager();

    void Init(theword::core::IAssetProvider& assets, float savedFontSize);
    void ReloadSizes(float newFontSize);
    const Font& Body() const { return bodyFont_; }
    const Font& Heading() const { return headingFont_; }
    const Font& Large() const { return largeFont_; }
    const Font& Small() const { return smallFont_; }
    const Font& Bold() const { return boldFont_; }
    const Font& Get(const theword::text::FontKind kind) const;

    float BodySize() const { return bodySize_; }
    float HeadingFontSize() const { return headingFontSize_; }
    float LargeSize() const { return largeFontSize_; }
    float SmallSize() const { return smallFontSize_; }
    float HeadingSize() const { return headingSize_; }
    float CurrentSize() const { return currentFontSize_; }

private:
    float scale_;
    float currentFontSize_ = 24.0f;
    Font bodyFont_{};
    Font headingFont_{};
    Font largeFont_{};
    Font smallFont_{};
    Font boldFont_{};
    std::vector<int> codepoints_;
    float bodySize_ = 0.0f;
    float headingFontSize_ = 0.0f;
    float largeFontSize_ = 0.0f;
    float smallFontSize_ = 0.0f;
    float headingSize_ = 0.0f;
};

} // namespace theword::renderer

#endif // FONT_MANAGER_H
