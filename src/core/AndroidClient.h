#ifndef ANDROIDCLIENT_H
#define ANDROIDCLIENT_H

#include "IHttpClient.h"
#include <string>

class AndroidClient : public IHttpClient {
public:
    AndroidClient();
    ~AndroidClient() override;
    AndroidClient(const AndroidClient&) = delete;
    AndroidClient& operator=(const AndroidClient&) = delete;
    AndroidClient(AndroidClient&&) = default;
    AndroidClient& operator=(AndroidClient&&) = default;

    std::string get(const std::string& url) override;
    void setAppKey(const std::string& key) override;
    std::string getAppKey() const override;

private:
    void* curl;
    std::string appKey;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif
