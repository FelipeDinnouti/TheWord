#ifndef ENV_LOADER_H
#define ENV_LOADER_H

#include <string>
#include <map>

namespace theword::core {

class EnvLoader {
public:
    static void load(const std::string& filepath = ".env");
    static void loadFromContent(const std::string& content);
    static std::string get(const std::string& key);
    static std::string get(const std::string& key, const std::string& defaultValue);

private:
    static std::map<std::string, std::string> variables;
};

} // namespace theword::core

#endif // ENVLOADER_H