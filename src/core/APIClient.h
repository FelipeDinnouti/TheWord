#ifndef APICLIENT_H
#define APICLIENT_H

#include <string>
#include <curl/curl.h>

class APIClient {
public:
    APIClient();
    ~APIClient();
    APIClient(const APIClient&) = delete;
    APIClient& operator=(const APIClient&) = delete;
    APIClient(APIClient&&) = default;
    APIClient& operator=(APIClient&&) = default;

    std::string get(const std::string& url);

    void setAppKey(const std::string& key);
    std::string getAppKey() const;

private:
    CURL* curl;
    std::string appKey;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // APICLIENT_H