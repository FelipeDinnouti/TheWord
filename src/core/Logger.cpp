#include "Logger.h"
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#endif

void Logger::Info(const std::string& msg) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "TheWord", "%s", msg.c_str());
#else
    std::fprintf(stdout, "%s\n", msg.c_str());
#endif
}

#ifndef NDEBUG
void Logger::Debug(const std::string& msg) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "TheWord", "%s", msg.c_str());
#else
    std::fprintf(stdout, "%s\n", msg.c_str());
#endif
}
#else
void Logger::Debug(const std::string&) {}
#endif

void Logger::Warning(const std::string& msg) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_WARN, "TheWord", "%s", msg.c_str());
#else
    std::fprintf(stderr, "%s\n", msg.c_str());
#endif
}

void Logger::Error(const std::string& msg) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, "TheWord", "%s", msg.c_str());
#else
    std::fprintf(stderr, "%s\n", msg.c_str());
#endif
}
