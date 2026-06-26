#ifndef I_HTTP_CLIENT_H
#define I_HTTP_CLIENT_H

#include <string>

namespace theword::core {

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::string Get(const std::string& url) = 0;
    virtual void SetAppKey(const std::string& key) = 0;
    virtual std::string GetAppKey() const = 0;
};

} // namespace theword::core

#endif
