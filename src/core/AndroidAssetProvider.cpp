#include "AndroidAssetProvider.h"
#include <android/asset_manager.h>

AndroidAssetProvider::AndroidAssetProvider(void* assetManager)
    : assetManager(assetManager) {}

std::optional<std::string> AndroidAssetProvider::readFileText(const std::string& path) {
    AAssetManager* mgr = static_cast<AAssetManager*>(assetManager);
    AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) return std::nullopt;
    const char* data = static_cast<const char*>(AAsset_getBuffer(asset));
    off_t size = AAsset_getLength(asset);
    std::string result(data, static_cast<size_t>(size));
    AAsset_close(asset);
    return result;
}

std::optional<std::vector<uint8_t>> AndroidAssetProvider::readFileBinary(const std::string& path) {
    AAssetManager* mgr = static_cast<AAssetManager*>(assetManager);
    AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) return std::nullopt;
    const uint8_t* data = static_cast<const uint8_t*>(AAsset_getBuffer(asset));
    off_t size = AAsset_getLength(asset);
    std::vector<uint8_t> result(data, data + size);
    AAsset_close(asset);
    return result;
}
