#ifndef TEXT_MEASURE_H
#define TEXT_MEASURE_H

#include <functional>
#include <string>

namespace theword::text {

enum class FontKind {
    Body,
    Heading,
    Large,
    Small
};

struct TextExtent {
    float width = 0.0f;
    float height = 0.0f;
};

using TextMeasureFn = std::function<TextExtent(FontKind kind, const std::string& text, float size)>;

} // namespace theword::text

#endif // TEXT_MEASURE_H
