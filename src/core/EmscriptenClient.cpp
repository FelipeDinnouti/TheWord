#include "EmscriptenClient.h"
#include <emscripten/fetch.h>
#include <cstring>
#include <iostream>

EmscriptenClient::EmscriptenClient() {}

void EmscriptenClient::setAppKey(const std::string& key) {
    appKey = key;
}

std::string EmscriptenClient::getAppKey() const {
    return appKey;
}

struct FetchBuffer {
    std::string data;
    bool done;
};

static void onSuccess(emscripten_fetch_t* fetch) {
    auto* buf = static_cast<FetchBuffer*>(fetch->userData);
    buf->data.assign(fetch->data, fetch->numBytes);
    buf->done = true;
    emscripten_fetch_close(fetch);
}

static void onError(emscripten_fetch_t* fetch) {
    auto* buf = static_cast<FetchBuffer*>(fetch->userData);
    std::cerr << "WASM fetch failed: " << fetch->statusText << std::endl;
    buf->done = true;
    emscripten_fetch_close(fetch);
}

std::string EmscriptenClient::get(const std::string& url) {
    FetchBuffer buf;
    buf.done = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_SYNCHRONOUS;
    attr.userData = &buf;
    attr.onsuccess = onSuccess;
    attr.onerror = onError;

    if (!appKey.empty()) {
        const char* headers[] = {"X-YVP-App-Key", appKey.c_str(), nullptr};
        attr.requestHeaders = headers;
    }

    emscripten_fetch(&attr, url.c_str());
    return buf.data;
}
