#ifndef FONT_HELPER_H
#define FONT_HELPER_H

#include <vector>
#include <cstddef>
#include <cstdint>

std::vector<int> LoadFontCodepoints(const char* fontPath);
std::vector<int> LoadFontCodepointsFromData(const uint8_t* data, size_t size);

#endif
