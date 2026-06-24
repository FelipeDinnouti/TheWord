#ifndef USFM_PARSER_H
#define USFM_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "ChapterProvider.h"
#include "core/IAssetProvider.h"

class USFMParser : public ChapterProvider {
public:
    explicit USFMParser(const std::string& usfmDir, IAssetProvider* assets = nullptr);

    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;

private:
    std::string usfmDir;
    IAssetProvider* assets;
    mutable std::unordered_map<std::string, bool> cachedHasChapter;
    mutable std::unordered_map<std::string, std::vector<ChapterData>> bookCache;

    std::vector<ChapterData> parseBook(const std::string& bookId) const;
    std::string loadFile(const std::string& filepath) const;
    std::string stripFootnotes(const std::string& text) const;
    std::string stripInlineMarkers(const std::string& text) const;
    std::string extractBookCodeFromId(const std::string& line) const;
};

#endif
