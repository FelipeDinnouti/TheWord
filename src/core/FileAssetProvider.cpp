#include "FileAssetProvider.h"
#include <fstream>
#include <sstream>

namespace theword::core {

std::optional<std::string> FileAssetProvider::readFileText(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::optional<std::vector<uint8_t>> FileAssetProvider::readFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return std::nullopt;
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return std::nullopt;
    return data;
}

} // namespace theword::core
