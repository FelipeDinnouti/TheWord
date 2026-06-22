#ifndef IN_MEMORY_STORAGE_H
#define IN_MEMORY_STORAGE_H

#include "PersistenceInterface.h"
#include <vector>

class InMemoryStorage : public PersistenceInterface {
public:
    std::vector<Highlight> loadHighlights() override;
    void saveHighlight(const Highlight& h) override;
    void removeHighlight(int id) override;

private:
    std::vector<Highlight> highlights;
};

#endif
