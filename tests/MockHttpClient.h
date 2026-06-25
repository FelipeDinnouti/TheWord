#ifndef MOCKHTTPCLIENT_H
#define MOCKHTTPCLIENT_H

#include "core/IHttpClient.h"
#include <string>

class MockHttpClient : public IHttpClient {
public:
    void SetResponse(const std::string& r) { response = r; }
    std::string Get(const std::string&) override { return response; }
    void SetAppKey(const std::string&) override {}
    std::string GetAppKey() const override { return ""; }
private:
    std::string response;
};

#endif
