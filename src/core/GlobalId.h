#ifndef GLOBAL_ID_H
#define GLOBAL_ID_H

#include <atomic>

namespace theword::core {

inline int GetNextWordId() {
    static std::atomic<int> id{0};
    return id++;
}

} // namespace theword::core

#endif
