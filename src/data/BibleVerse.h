#ifndef BibleVerse_h
#define BibleVerse_h

#include <string>
#include <vector>

struct BibleVerse {
    std::string id;
    std::string content;
    int bibleId;
};

struct BiblePassage {
    std::string id;
    std::string content;
    std::string reference;
};

#endif // BibleVerse_h