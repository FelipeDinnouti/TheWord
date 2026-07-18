#ifndef FUZZY_MATCHER_H
#define FUZZY_MATCHER_H

#include <string>

namespace theword::core {

int FuzzyMatch(const std::string& query, const std::string& target);

std::string StripAccents(const std::string& input);

} // namespace theword::core

#endif
