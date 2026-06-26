#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include "core/GlobalId.h"
#include "ChapterProvider.h"

namespace theword::data {

inline void TokenizeToWords(const std::string& text, int verseId,
                            std::vector<Word>& chapterWords,
                            std::vector<Word>& segmentWords) {
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        Word w;
        w.id = theword::core::GetNextWordId();
        w.verseId = verseId;
        w.text = word;
        chapterWords.push_back(w);
        segmentWords.push_back(w);
    }
}

inline std::string DecodeHtmlEntities(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '&' && input.substr(i, 5) == "&amp;") {
            result += '&';
            i += 4;
        } else if (input[i] == '&' && input.substr(i, 4) == "&lt;") {
            result += '<';
            i += 3;
        } else if (input[i] == '&' && input.substr(i, 4) == "&gt;") {
            result += '>';
            i += 3;
        } else if (input[i] == '&' && input.substr(i, 6) == "&quot;") {
            result += '\"';
            i += 5;
        } else if (input[i] == '&' && input.substr(i, 3) == "&#") {
            size_t semi = input.find(';', i);
            if (semi != std::string::npos) {
                i = semi;
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

} // namespace theword::data

#endif
