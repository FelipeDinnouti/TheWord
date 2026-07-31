#include "FontManager.h"
#include "core/Config.h"
#include "core/FontHelper.h"
#include "core/Theme.h"
#include <algorithm>

namespace theword::renderer {

using namespace theword::core;
using namespace theword::core::theme;

FontManager::FontManager(float dpiScale)
    : scale_(dpiScale) {
}

FontManager::~FontManager() {
    UnloadFont(bodyFont_);
    UnloadFont(headingFont_);
    UnloadFont(largeFont_);
    UnloadFont(smallFont_);
    UnloadFont(boldFont_);
}

void FontManager::Init(theword::core::IAssetProvider& assets, float savedFontSize) {
    codepoints_ = LoadFontCodepoints(assets, config::FONT_REGULAR);
    ReloadSizes(savedFontSize);
}

void FontManager::ReloadSizes(float newFontSize) {
    newFontSize = std::clamp(newFontSize, config::FONT_SIZE_MIN, config::FONT_SIZE_MAX);
    currentFontSize_ = newFontSize;

    int scaledFontSize = (int)(newFontSize * scale_);
    scaledFontSize = std::max(1, scaledFontSize);
    int scaledHeadingSize = (int)(newFontSize * FONT_HEADING * scale_);
    scaledHeadingSize = std::max(1, scaledHeadingSize);
    int scaledLargeSize = (int)(newFontSize * FONT_LARGE_HEADING * scale_);
    scaledLargeSize = std::max(1, scaledLargeSize);
    int scaledSmallSize = (int)(newFontSize * FONT_VERSE_NUMBER * scale_);
    scaledSmallSize = std::max(1, scaledSmallSize);

    Font newBody = LoadFontEx(config::FONT_REGULAR, scaledFontSize,
                              codepoints_.data(), (int)codepoints_.size());
    SetTextureFilter(newBody.texture, TEXTURE_FILTER_POINT);

    Font newHeading = LoadFontEx(config::FONT_REGULAR, scaledHeadingSize,
                                 codepoints_.data(), (int)codepoints_.size());
    SetTextureFilter(newHeading.texture, TEXTURE_FILTER_BILINEAR);

    Font newLarge = LoadFontEx(config::FONT_REGULAR, scaledLargeSize,
                               codepoints_.data(), (int)codepoints_.size());
    SetTextureFilter(newLarge.texture, TEXTURE_FILTER_POINT);

    Font newSmall = LoadFontEx(config::FONT_REGULAR, scaledSmallSize,
                               codepoints_.data(), (int)codepoints_.size());
    SetTextureFilter(newSmall.texture, TEXTURE_FILTER_POINT);

    Font newBold = LoadFontEx(config::FONT_BOLD, scaledFontSize,
                              codepoints_.data(), (int)codepoints_.size());
    SetTextureFilter(newBold.texture, TEXTURE_FILTER_POINT);

    bodySize_ = (float)scaledFontSize;
    headingFontSize_ = (float)scaledHeadingSize;
    largeFontSize_ = (float)scaledLargeSize;
    smallFontSize_ = (float)scaledSmallSize;
    headingSize_ = config::FONT_HEADING_SIZE / config::FONT_SIZE * newFontSize * scale_;

    Font oldBody = bodyFont_;
    Font oldHeading = headingFont_;
    Font oldLarge = largeFont_;
    Font oldSmall = smallFont_;
    Font oldBold = boldFont_;
    bodyFont_ = newBody;
    headingFont_ = newHeading;
    largeFont_ = newLarge;
    smallFont_ = newSmall;
    boldFont_ = newBold;

    UnloadFont(oldBody);
    UnloadFont(oldHeading);
    UnloadFont(oldLarge);
    UnloadFont(oldSmall);
    UnloadFont(oldBold);
}

const Font& FontManager::Get(const theword::text::FontKind kind) const {
    switch (kind) {
        case theword::text::FontKind::Body: return bodyFont_;
        case theword::text::FontKind::Heading: return headingFont_;
        case theword::text::FontKind::Large: return largeFont_;
        case theword::text::FontKind::Small: return smallFont_;
    }
    return bodyFont_;
}

} // namespace theword::renderer
