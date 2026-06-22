#include "Highlighter.h"
#include <algorithm>

static const HighlightType DEFAULT_HIGHLIGHT_TYPE { 1, "Yellow", {255, 255, 0, 100} };

Highlighter::Highlighter(PersistenceInterface& persistence)
    : persistence(persistence)
    , selectionStart(-1)
    , selectionEnd(-1)
    , selecting(false)
    , nextId(1) {
    types.push_back(DEFAULT_HIGHLIGHT_TYPE);
    load();
}

void Highlighter::load() {
    highlights = persistence.loadHighlights();
    for (const auto& h : highlights) {
        if (h.id >= nextId) nextId = h.id + 1;
    }
}

void Highlighter::startSelection(int wordId) {
    selecting = true;
    selectionStart = wordId;
    selectionEnd = wordId;
}

void Highlighter::updateSelection(int wordId) {
    if (!selecting) return;
    selectionEnd = wordId;
}

void Highlighter::endSelection() {
    if (!selecting) return;
    selecting = false;

    int start = (std::min)(selectionStart, selectionEnd);
    int end = (std::max)(selectionStart, selectionEnd);
    if (start < 0) return;

    Highlight h;
    h.id = nextId++;
    h.startWord = start;
    h.endWord = end;
    h.typeId = types.empty() ? 1 : types[0].id;

    highlights.push_back(h);
    persistence.saveHighlight(h);
}

bool Highlighter::isWordHighlighted(int wordId) const {
    for (const auto& h : highlights) {
        if (wordId >= h.startWord && wordId <= h.endWord) return true;
    }
    return false;
}

Color Highlighter::getHighlightForWord(int wordId) const {
    for (const auto& h : highlights) {
        if (wordId >= h.startWord && wordId <= h.endWord) {
            for (const auto& t : types) {
                if (t.id == h.typeId) return t.color;
            }
            return DEFAULT_HIGHLIGHT_TYPE.color;
        }
    }
    return {0, 0, 0, 0};
}

const std::vector<Highlight>& Highlighter::getHighlights() const {
    return highlights;
}
