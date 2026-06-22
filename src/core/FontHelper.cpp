#include "FontHelper.h"
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <string>

static uint16_t ReadU16(const uint8_t* data, size_t offset) {
    return (uint16_t)data[offset] << 8 | (uint16_t)data[offset + 1];
}

static uint32_t ReadU32(const uint8_t* data, size_t offset) {
    return (uint32_t)data[offset] << 24 |
           (uint32_t)data[offset + 1] << 16 |
           (uint32_t)data[offset + 2] << 8 |
           (uint32_t)data[offset + 3];
}

std::vector<int> LoadFontCodepoints(const char* fontPath) {
    std::vector<int> result;

    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) return result;

    size_t size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read((char*)data.data(), size)) return result;

    // Parse TrueType/OpenType offset table
    uint16_t numTables = ReadU16(data.data(), 4);

    size_t cmapOffset = 0;
    for (uint16_t i = 0; i < numTables; i++) {
        size_t entry = 12 + i * 16;
        char tag[5] = {};
        tag[0] = (char)data[entry];
        tag[1] = (char)data[entry + 1];
        tag[2] = (char)data[entry + 2];
        tag[3] = (char)data[entry + 3];
        if (std::string(tag) == "cmap") {
            cmapOffset = ReadU32(data.data(), entry + 8);
            break;
        }
    }
    if (cmapOffset == 0) return result;

    uint16_t numCmapTables = ReadU16(data.data(), cmapOffset + 2);

    for (uint16_t i = 0; i < numCmapTables; i++) {
        size_t entry = cmapOffset + 4 + i * 8;
        uint16_t platformID = ReadU16(data.data(), entry);
        uint16_t encodingID = ReadU16(data.data(), entry + 2);
        uint32_t subOffset = ReadU32(data.data(), entry + 4);

        size_t sub = cmapOffset + subOffset;
        uint16_t format = ReadU16(data.data(), sub);

        if (format == 4 && (platformID == 0 || platformID == 3)) {
            uint16_t segCount = ReadU16(data.data(), sub + 6) / 2;
            size_t endCodes = sub + 14;
            size_t startCodes = endCodes + segCount * 2 + 2;

            for (uint16_t j = 0; j < segCount; j++) {
                uint16_t endCode = ReadU16(data.data(), endCodes + j * 2);
                if (endCode == 0xFFFF) break;
                uint16_t startCode = ReadU16(data.data(), startCodes + j * 2);
                for (uint32_t c = startCode; c <= endCode; c++) {
                    result.push_back((int)c);
                }
            }
        } else if (format == 12 && (platformID == 0 || platformID == 3)) {
            uint32_t nGroups = ReadU32(data.data(), sub + 12);
            for (uint32_t j = 0; j < nGroups; j++) {
                size_t g = sub + 16 + j * 12;
                uint32_t startCode = ReadU32(data.data(), g);
                uint32_t endCode = ReadU32(data.data(), g + 4);
                for (uint32_t c = startCode; c <= endCode; c++) {
                    result.push_back((int)c);
                }
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}
