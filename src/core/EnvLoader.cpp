#include "EnvLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace theword::core {

std::map<std::string, std::string> EnvLoader::variables;

namespace {
void ParseLine(const std::string& line, std::map<std::string, std::string>& vars) {
    std::string trimmed = line;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(),
        [](char c) { return c == '\r' || c == '\n'; }), trimmed.end());

    if (trimmed.empty() || trimmed[0] == '#') {
        return;
    }

    size_t eqPos = trimmed.find('=');
    if (eqPos == std::string::npos) {
        return;
    }

    std::string key = trimmed.substr(0, eqPos);
    std::string value = trimmed.substr(eqPos + 1);

    key.erase(std::remove_if(key.begin(), key.end(),
        [](char c) { return std::isspace(static_cast<unsigned char>(c)); }), key.end());

    while (!value.empty() && (value[0] == ' ' || value[0] == '"' || value[0] == '\'')) {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '"' || value.back() == '\'')) {
        value.pop_back();
    }

    vars[key] = value;
}
} // namespace

void EnvLoader::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        ParseLine(line, variables);
    }
}

void EnvLoader::LoadFromContent(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        ParseLine(line, variables);
    }
}

std::string EnvLoader::Get(const std::string& key) {
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

std::string EnvLoader::Get(const std::string& key, const std::string& defaultValue) {
    std::string value = Get(key);
    return value.empty() ? defaultValue : value;
}

} // namespace theword::core
