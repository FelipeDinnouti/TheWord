#ifndef ANDROIDASSETPROVIDER_H
#define ANDROIDASSETPROVIDER_H

#include "IAssetProvider.h"

class AndroidAssetProvider : public IAssetProvider {
public:
    // assetManager is AAssetManager* passed as void* to avoid NDK header dependency
    explicit AndroidAssetProvider(void* assetManager);
    std::optional<std::string> readFileText(const std::string& path) override;
    std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) override;
private:
    void* assetManager;
};

#endif
