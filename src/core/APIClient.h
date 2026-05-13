#ifndef APIClient_h
#define APIClient_h

#include <string>
#include <curl/curl.h>

class APIClient {
public:
    APIClient();
    ~APIClient();

    std::string get(const std::string& url);

    void setAppKey(const std::string& key);
    std::string getAppKey() const;

private:
    CURL* curl;
    std::string appKey;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // APIClient_h