#ifndef PERSISTENCE_INTERFACE_H
#define PERSISTENCE_INTERFACE_H

#include <string>
#include <vector>

struct SimpleColor {
    unsigned char r, g, b, a;
};

struct Highlight {
    int id;
    int startWord;
    int endWord;
    int typeId;
    std::string providerName;
};

struct HighlightType {
    int id;
    std::string name;
    SimpleColor color;
};

class PersistenceInterface {
public:
    virtual ~PersistenceInterface() = default;
    virtual std::vector<Highlight> LoadHighlights() = 0;
    virtual void SaveHighlight(const Highlight& h) = 0;
    virtual void RemoveHighlight(int id) = 0;
    virtual std::vector<HighlightType> LoadHighlightTypes() = 0;
};

#endif
