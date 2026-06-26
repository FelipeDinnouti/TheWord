#ifndef MOCK_HTTP_CLIENT_H
#define MOCK_HTTP_CLIENT_H

#include "core/IHttpClient.h"
#include <string>

namespace theword::test {

class MockHttpClient : public theword::core::IHttpClient {
public:
    void SetResponse(const std::string& r) { response = r; }
    std::string Get(const std::string&) override { return response; }
    void SetAppKey(const std::string&) override {}
    std::string GetAppKey() const override { return ""; }
private:
    std::string response;
};

} // namespace theword::test

#endif
