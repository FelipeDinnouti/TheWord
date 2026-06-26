#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace theword::core {

class Logger {
public:
    static void Info(const std::string& msg);
    static void Debug(const std::string& msg);
    static void Warning(const std::string& msg);
    static void Error(const std::string& msg);
};

} // namespace theword::core

#endif
