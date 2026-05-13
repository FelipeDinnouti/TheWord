#include "EnvLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::map<std::string, std::string> EnvLoader::variables;

void EnvLoader::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove_if(line.begin(), line.end(),
            [](char c) { return c == '\r' || c == '\n'; }), line.end());

        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            key.erase(std::remove_if(key.begin(), key.end(),
                [](char c) { return std::isspace(static_cast<unsigned char>(c)); }), key.end());

            while (!value.empty() && (value[0] == ' ' || value[0] == '"' || value[0] == '\'')) {
                value.erase(value.begin());
            }
            while (!value.empty() && (value.back() == ' ' || value.back() == '"' || value.back() == '\'')) {
                value.pop_back();
            }

            variables[key] = value;
        }
    }
}

std::string EnvLoader::get(const std::string& key) {
    auto it = variables.find(key);
    if (it != variables.end()) {
        return it->second;
    }

    char* envValue = std::getenv(key.c_str());
    if (envValue != nullptr) {
        return std::string(envValue);
    }

    return "";
}

std::string EnvLoader::get(const std::string& key, const std::string& defaultValue) {
    std::string value = get(key);
    return value.empty() ? defaultValue : value;
}