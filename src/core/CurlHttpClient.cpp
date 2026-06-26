#include "CurlHttpClient.h"
#include "Logger.h"
#include <curl/curl.h>

namespace theword::core {

void CurlHandleDeleter::operator()(void* h) const noexcept {
    if (h) curl_easy_cleanup(h);
}

CurlHttpClient::CurlHttpClient()
    : curl(curl_easy_init(), CurlHandleDeleter{}) {
    if (curl) {
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);
    }
}

CurlHttpClient::~CurlHttpClient() = default;

void CurlHttpClient::SetAppKey(const std::string& key) {
    appKey = key;
}

std::string CurlHttpClient::GetAppKey() const {
    return appKey;
}

size_t CurlHttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), realsize);
    return realsize;
}

std::string CurlHttpClient::Get(const std::string& url) {
    std::string response;
    if (!curl) return response;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    if (!appKey.empty()) {
        std::string header = "X-YVP-App-Key: " + appKey;
        headers = curl_slist_append(headers, header.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        Logger::Error("API request failed: " + std::string(curl_easy_strerror(res)));
        response.clear();
    }

    if (headers) {
        curl_slist_free_all(headers);
    }

    return response;
}

} // namespace theword::core
