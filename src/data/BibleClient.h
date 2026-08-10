#ifndef BIBLE_CLIENT_H
#define BIBLE_CLIENT_H

#include <string>
#include <optional>
#include "ChapterProvider.h"

namespace theword::core { class IHttpClient; }

class BibleClientTest;  // test helper, defined in test_main.cpp

namespace theword::data {

class BibleClient : public ChapterProvider {
public:
    BibleClient(theword::core::IHttpClient& apiClient, int bibleId);

    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;

private:
    friend class ::BibleClientTest;

    theword::core::IHttpClient& apiClient;
    const int bibleId;
    const std::string baseUrl;

    static std::string ExtractJsonString(const std::string& json, const std::string& key);
    std::optional<ChapterData> ParseHtmlChapter(const std::string& html,
                                                  const std::string& bookId, int chapter) const;
    static std::string StripHtml(const std::string& html);
};

} // namespace theword::data

#endif // BIBLE_CLIENT_H
