#ifndef IASSETPROVIDER_H
#define IASSETPROVIDER_H

#include <string>
#include <optional>
#include <vector>

class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual std::optional<std::string> readFileText(const std::string& path) = 0;
    virtual std::optional<std::vector<uint8_t>> readFileBinary(const std::string& path) = 0;
};

#endif
