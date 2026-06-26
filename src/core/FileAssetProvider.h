#ifndef FILE_ASSET_PROVIDER_H
#define FILE_ASSET_PROVIDER_H

#include "IAssetProvider.h"

namespace theword::core {

class FileAssetProvider : public IAssetProvider {
public:
    std::optional<std::string> readFileText(const std::string& path) override;
    std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) override;
};

} // namespace theword::core

#endif
