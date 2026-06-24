#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <string>
#include <map>

class EnvLoader {
public:
    static void load(const std::string& filepath = ".env");
    static void loadFromContent(const std::string& content);
    static std::string get(const std::string& key);
    static std::string get(const std::string& key, const std::string& defaultValue);

private:
    static std::map<std::string, std::string> variables;
};

#endif // ENVLOADER_H