#ifndef EnvLoader_h
#define EnvLoader_h

#include <string>
#include <map>

class EnvLoader {
public:
    static void load(const std::string& filepath = ".env");
    static std::string get(const std::string& key);
    static std::string get(const std::string& key, const std::string& defaultValue);

private:
    static std::map<std::string, std::string> variables;
};

#endif // EnvLoader_h