#ifndef EMSCRIPTEN_CLIENT_H
#define EMSCRIPTEN_CLIENT_H

#include "IHttpClient.h"
#include <string>

namespace theword::core {

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

} // namespace theword::core

#endif
