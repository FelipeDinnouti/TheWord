#include "AndroidClient.h"
#include "Logger.h"
#include <curl/curl.h>
#include <cstring>

AndroidClient::AndroidClient() {
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    }
}

AndroidClient::~AndroidClient() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

void AndroidClient::SetAppKey(const std::string& key) {
    appKey = key;
}

std::string AndroidClient::GetAppKey() const {
    return appKey;
}

size_t AndroidClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), realsize);
    return realsize;
}

std::string AndroidClient::Get(const std::string& url) {
    std::string response;
    CURL* ch = static_cast<CURL*>(curl);

    if (!ch) {
        return response;
    }

    curl_easy_setopt(ch, CURLOPT_URL, url.c_str());
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    if (!appKey.empty()) {
        std::string header = "X-YVP-App-Key: " + appKey;
        headers = curl_slist_append(headers, header.c_str());
        curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(ch);

    if (res != CURLE_OK) {
        Logger::Error("API request failed: " + std::string(curl_easy_strerror(res)));
        response.clear();
    }

    if (headers) {
        curl_slist_free_all(headers);
    }

    return response;
}
