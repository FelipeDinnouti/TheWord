#include "BibleClient.h"
#include <iostream>
#include <algorithm>

BibleClient::BibleClient(const std::string& appKey) {
    apiClient.setAppKey(appKey);
    baseUrl = "https://api.youversion.com/v1";
}

std::string BibleClient::extractTextFromJson(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }

    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\"')) {
        valueStart++;
    }

    size_t valueEnd = valueStart;
    if (json[valueStart - 1] == '\"') {
        valueEnd = json.find('\"', valueStart);
    } else {
        while (valueEnd < json.size() && json[valueEnd] != ',' && json[valueEnd] != '}') {
            valueEnd++;
        }
    }

    if (valueEnd == std::string::npos) {
        valueEnd = json.size();
    }

    std::string result = json.substr(valueStart, valueEnd - valueStart);
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\\'), result.end());

    return result;
}

BiblePassage BibleClient::parsePassageResponse(const std::string& json) {
    BiblePassage passage;

    passage.id = extractTextFromJson(json, "id");
    passage.content = extractTextFromJson(json, "content");
    passage.reference = extractTextFromJson(json, "reference");

    return passage;
}

BiblePassage BibleClient::getPassage(int bibleId, const std::string& usfmReference, const std::string& format) {
    std::string url = baseUrl + "/bibles/" + std::to_string(bibleId) +
                      "/passages/" + usfmReference + "?format=" + format;

    std::string response = apiClient.get(url);

    if (response.empty()) {
        return BiblePassage();
    }

    return parsePassageResponse(response);
}