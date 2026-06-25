#ifndef EMSCRIPTENCLIENT_H
#define EMSCRIPTENCLIENT_H

#include "IHttpClient.h"
#include <string>

class EmscriptenClient : public IHttpClient {
public:
    EmscriptenClient();
    ~EmscriptenClient() override = default;

    std::string Get(const std::string& url) override;
    void SetAppKey(const std::string& key) override;
    std::string GetAppKey() const override;

private:
    std::string appKey;
};

#endif
