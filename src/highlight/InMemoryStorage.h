#ifndef IN_MEMORY_STORAGE_H
#define IN_MEMORY_STORAGE_H

#include "PersistenceInterface.h"
#include <vector>

namespace theword::highlight {

class InMemoryStorage : public PersistenceInterface {
public:
    std::vector<Highlight> LoadHighlights() override;
    void SaveHighlight(const Highlight& h) override;
    void RemoveHighlight(int id) override;
    std::vector<HighlightType> LoadHighlightTypes() override;

private:
    std::vector<Highlight> highlights;
    std::vector<HighlightType> types;
};

} // namespace theword::highlight

#endif
