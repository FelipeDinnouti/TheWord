#ifndef CURLHTTPCLIENT_H
#define CURLHTTPCLIENT_H

#include "IHttpClient.h"
#include <string>

class CurlHttpClient : public IHttpClient {
public:
    CurlHttpClient();
    ~CurlHttpClient() override;
    CurlHttpClient(const CurlHttpClient&) = delete;
    CurlHttpClient& operator=(const CurlHttpClient&) = delete;
    CurlHttpClient(CurlHttpClient&&) = default;
    CurlHttpClient& operator=(CurlHttpClient&&) = default;

    std::string Get(const std::string& url) override;
    void SetAppKey(const std::string& key) override;
    std::string GetAppKey() const override;

private:
    void* curl;
    std::string appKey;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif
