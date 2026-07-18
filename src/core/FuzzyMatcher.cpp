#include "FuzzyMatcher.h"
#include <algorithm>
#include <cctype>

namespace theword::core {

std::string StripAccents(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c == 0xc3 && i + 1 < input.size()) {
            unsigned char next = static_cast<unsigned char>(input[i + 1]);
            switch (next) {
                case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
                    result += 'A'; break;
                case 0x87: result += 'C'; break;
                case 0x88: case 0x89: case 0x8a: case 0x8b:
                    result += 'E'; break;
                case 0x8c: case 0x8d: case 0x8e: case 0x8f:
                    result += 'I'; break;
                case 0x91: result += 'N'; break;
                case 0x92: case 0x93: case 0x94: case 0x95: case 0x96:
                    result += 'O'; break;
                case 0x99: case 0x9a: case 0x9b: case 0x9c:
                    result += 'U'; break;
                case 0x9d: result += 'Y'; break;
                case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5:
                    result += 'a'; break;
                case 0xa7: result += 'c'; break;
                case 0xa8: case 0xa9: case 0xaa: case 0xab:
                    result += 'e'; break;
                case 0xac: case 0xad: case 0xae: case 0xaf:
                    result += 'i'; break;
                case 0xb1: result += 'n'; break;
                case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6:
                    result += 'o'; break;
                case 0xb9: case 0xba: case 0xbb: case 0xbc:
                    result += 'u'; break;
                case 0xbd: case 0xbf: result += 'y'; break;
                default: result += static_cast<char>(c); result += static_cast<char>(next); break;
            }
            ++i;
        } else {
            result += static_cast<char>(c);
        }
    }
    return result;
}

int FuzzyMatch(const std::string& queryRaw, const std::string& targetRaw) {
    if (queryRaw.empty()) return 0;

    std::string query = StripAccents(queryRaw);
    std::string target = StripAccents(targetRaw);
    for (auto& c : query) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (auto& c : target) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    int score = 0;
    size_t qi = 0;
    int lastMatchPos = -1;

    for (size_t ti = 0; ti < target.size() && qi < query.size(); ++ti) {
        if (target[ti] == query[qi]) {
            score += 1;
            if (qi == 0 && ti == 0) score += 3;
            if (lastMatchPos >= 0 && static_cast<int>(ti) == lastMatchPos + 1) score += 2;
            lastMatchPos = static_cast<int>(ti);
            ++qi;
        }
    }

    if (qi < query.size()) return -1;

    if (target.size() >= query.size() &&
        std::equal(query.begin(), query.end(), target.begin()))
        score += 10;

    return score;
}

} // namespace theword::core
