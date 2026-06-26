#ifndef ANDROID_ASSET_PROVIDER_H
#define ANDROID_ASSET_PROVIDER_H

#include "IAssetProvider.h"

namespace theword::core {

class AndroidAssetProvider : public IAssetProvider {
public:
    // assetManager is AAssetManager* passed as void* to avoid NDK header dependency
    explicit AndroidAssetProvider(void* assetManager);
    std::optional<std::string> readFileText(const std::string& path) override;
    std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) override;
private:
    void* assetManager;
};

} // namespace theword::core

#endif
