#ifndef EMSCRIPTENCLIENT_H
#define EMSCRIPTENCLIENT_H

#include "IHttpClient.h"
#include <string>

class EmscriptenClient : public IHttpClient {
public:
    EmscriptenClient();
    ~EmscriptenClient() override = default;

    std::string get(const std::string& url) override;
    void setAppKey(const std::string& key) override;
    std::string getAppKey() const override;

private:
    std::string appKey;
};

#endif
