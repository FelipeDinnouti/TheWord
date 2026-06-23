#ifndef IHTTPCLIENT_H
#define IHTTPCLIENT_H

#include <string>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::string get(const std::string& url) = 0;
    virtual void setAppKey(const std::string& key) = 0;
    virtual std::string getAppKey() const = 0;
};

#endif
