#ifndef CURL_HTTP_CLIENT_H
#define CURL_HTTP_CLIENT_H

#include "IHttpClient.h"
#include <memory>
#include <string>

namespace theword::core {

struct CurlHandleDeleter {
    void operator()(void* h) const noexcept;
};

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
    std::unique_ptr<void, CurlHandleDeleter> curl;
    std::string appKey;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // namespace theword::core

#endif
