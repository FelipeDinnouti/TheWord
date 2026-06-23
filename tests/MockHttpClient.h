#ifndef MOCKHTTPCLIENT_H
#define MOCKHTTPCLIENT_H

#include "core/IHttpClient.h"
#include <string>

class MockHttpClient : public IHttpClient {
public:
    void setResponse(const std::string& r) { response = r; }
    std::string get(const std::string&) override { return response; }
    void setAppKey(const std::string&) override {}
    std::string getAppKey() const override { return ""; }
private:
    std::string response;
};

#endif
