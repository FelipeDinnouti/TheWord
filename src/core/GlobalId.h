#ifndef GLOBAL_ID_H
#define GLOBAL_ID_H

inline int GetNextWordId() {
    static int id = 0;
    return id++;
}

#endif
