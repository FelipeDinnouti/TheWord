#include "Logger.h"
#include "Platform.h"
#include <cstdio>

namespace theword::core {

void Logger::Info(const std::string& msg) {
    platform::WriteLog(platform::LogLevel::INFO, msg.c_str());
}

#ifndef NDEBUG
void Logger::Debug(const std::string& msg) {
    platform::WriteLog(platform::LogLevel::DEBUG, msg.c_str());
}
#else
void Logger::Debug(const std::string&) {}
#endif

void Logger::Warning(const std::string& msg) {
    platform::WriteLog(platform::LogLevel::WARN, msg.c_str());
}

void Logger::Error(const std::string& msg) {
    platform::WriteLog(platform::LogLevel::ERROR, msg.c_str());
}

} // namespace theword::core
