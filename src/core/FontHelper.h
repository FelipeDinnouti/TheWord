#ifndef FONT_HELPER_H
#define FONT_HELPER_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace theword::core { class IAssetProvider; }

namespace theword::core {

std::vector<int> LoadFontCodepoints(const char* fontPath);
std::vector<int> LoadFontCodepoints(IAssetProvider& assets, const std::string& fontPath);
std::vector<int> LoadFontCodepointsFromData(const uint8_t* data, size_t size);

} // namespace theword::core

#endif
