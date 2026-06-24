#ifndef FILEASSETPROVIDER_H
#define FILEASSETPROVIDER_H

#include "IAssetProvider.h"

class FileAssetProvider : public IAssetProvider {
public:
    std::optional<std::string> readFileText(const std::string& path) override;
    std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) override;
};

#endif
