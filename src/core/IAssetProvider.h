#ifndef I_ASSET_PROVIDER_H
#define I_ASSET_PROVIDER_H

#include <string>
#include <optional>
#include <vector>

namespace theword::core {

class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual std::optional<std::string> readFileText(const std::string& path) = 0;
    virtual std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) = 0;
};

} // namespace theword::core

#endif
