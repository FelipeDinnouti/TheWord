#ifndef USFM_PARSER_H
#define USFM_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>
#include "ChapterProvider.h"
#include "core/IAssetProvider.h"

namespace theword::data {

class USFMParser : public ChapterProvider {
public:
    explicit USFMParser(const std::string& usfmDir, std::unique_ptr<theword::core::IAssetProvider> assets = nullptr);

    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;

private:
    std::string usfmDir;
    std::unique_ptr<theword::core::IAssetProvider> assets;
    mutable std::unordered_map<std::string, std::vector<ChapterData>> bookCache;

    std::vector<ChapterData> ParseBook(const std::string& bookId) const;
    std::string LoadFile(const std::string& filepath) const;
    std::string ExtractFootnotes(const std::string& text, std::vector<std::pair<int, Footnote>>& outFootnotes) const;
    std::string StripInlineMarkers(const std::string& text) const;
    std::string ExtractBookCodeFromId(const std::string& line) const;
};

} // namespace theword::data

#endif
