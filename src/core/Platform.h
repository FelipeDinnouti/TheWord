#ifndef PLATFORM_H
#define PLATFORM_H

#include <memory>
#include <string>
#include "IAssetProvider.h"

namespace theword::core { class IHttpClient; }

namespace theword::core { namespace platform {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

struct Info {
    float dpiScale;
    std::string dbPath;
    std::unique_ptr<IAssetProvider> assets;
    int bottomInset = 0;
};

Info Init(const char* title);
std::unique_ptr<IHttpClient> CreateHttpClient();
bool ShouldQuit();
bool HasTouchInput();

bool OpenURL(const char* url);
void WriteLog(LogLevel level, const char* message);
std::string GetClipboard();
void SetClipboard(const std::string& text);

} } // namespace theword::core::platform

#endif
