#include "InMemoryStorage.h"
#include <algorithm>

std::vector<Highlight> InMemoryStorage::loadHighlights() {
    return highlights;
}

void InMemoryStorage::saveHighlight(const Highlight& h) {
    highlights.push_back(h);
}

void InMemoryStorage::removeHighlight(int id) {
    highlights.erase(
        std::remove_if(highlights.begin(), highlights.end(),
            [id](const Highlight& h) { return h.id == id; }),
        highlights.end());
}


