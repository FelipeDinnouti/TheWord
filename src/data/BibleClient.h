#ifndef BibleClient_h
#define BibleClient_h

#include <string>
#include "../core/APIClient.h"
#include "BibleVerse.h"

class BibleClient {
public:
    BibleClient(const std::string& appKey);

    BiblePassage getPassage(int bibleId, const std::string& usfmReference, const std::string& format = "text");

private:
    APIClient apiClient;
    std::string baseUrl;

    BiblePassage parsePassageResponse(const std::string& json);
    std::string extractTextFromJson(const std::string& json, const std::string& key);
};

#endif // BibleClient_h