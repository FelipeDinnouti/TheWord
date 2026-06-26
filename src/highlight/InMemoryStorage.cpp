#include "InMemoryStorage.h"
#include <algorithm>

namespace theword::highlight {

std::vector<Highlight> InMemoryStorage::LoadHighlights() {
    return highlights;
}

void InMemoryStorage::SaveHighlight(const Highlight& h) {
    for (auto& existing : highlights) {
        if (existing.id == h.id) {
            existing = h;
            return;
        }
    }
    highlights.push_back(h);
}

void InMemoryStorage::RemoveHighlight(int id) {
    highlights.erase(
        std::remove_if(highlights.begin(), highlights.end(),
            [id](const Highlight& h) { return h.id == id; }),
        highlights.end());
}

std::vector<HighlightType> InMemoryStorage::LoadHighlightTypes() {
    return types;
}

} // namespace theword::highlight
