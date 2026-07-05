#ifndef LOCALE_H
#define LOCALE_H

namespace theword::core {

class Locale {
public:
    static const char* Get(const char* key);
};

} // namespace theword::core

#endif
