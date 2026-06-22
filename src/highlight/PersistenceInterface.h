#ifndef PERSISTENCE_INTERFACE_H
#define PERSISTENCE_INTERFACE_H

#include <string>
#include <vector>
#include <raylib.h>

struct Highlight {
    int id;
    int startWord;
    int endWord;
    int typeId;
};

struct HighlightType {
    int id;
    std::string name;
    Color color;
};

class PersistenceInterface {
public:
    virtual ~PersistenceInterface() = default;
    virtual std::vector<Highlight> loadHighlights() = 0;
    virtual void saveHighlight(const Highlight& h) = 0;
    virtual void removeHighlight(int id) = 0;
};

#endif
