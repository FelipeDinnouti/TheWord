#ifndef BIBLECLIENT_H
#define BIBLECLIENT_H

#include <string>
#include <optional>
#include "../core/IHttpClient.h"
#include "ChapterProvider.h"

class BibleClient : public ChapterProvider {
public:
    BibleClient(IHttpClient& apiClient, int bibleId);

    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;

private:
    friend class BibleClientTest;

    IHttpClient& apiClient;
    int bibleId;
    const std::string baseUrl;

    static std::string ExtractJsonString(const std::string& json, const std::string& key);
    std::optional<ChapterData> ParseHtmlChapter(const std::string& html,
                                                  const std::string& bookId, int chapter);
    static std::string StripHtml(const std::string& html);
};

#endif // BIBLECLIENT_H
