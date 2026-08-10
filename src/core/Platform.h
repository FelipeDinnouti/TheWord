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
bool IsWindowAvailable();
bool HasTouchInput();

void WriteLog(LogLevel level, const char* message);
void SetClipboard(const std::string& text);

void EnsureDirectoryExists(const std::string& path);
void ShowKeyboard();
void HideKeyboard();

} } // namespace theword::core::platform

#endif
