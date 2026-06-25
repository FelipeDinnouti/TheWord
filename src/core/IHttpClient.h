#ifndef IHTTPCLIENT_H
#define IHTTPCLIENT_H

#include <string>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::string Get(const std::string& url) = 0;
    virtual void SetAppKey(const std::string& key) = 0;
    virtual std::string GetAppKey() const = 0;
};

#endif
