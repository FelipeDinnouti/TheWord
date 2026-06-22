#include "Highlighter.h"
#include <algorithm>

static const HighlightType DEFAULT_HIGHLIGHT_TYPE { 1, "Yellow", {255, 255, 0, 100} };
static Color ToColor(const SimpleColor& c) { return {c.r, c.g, c.b, c.a}; }

Highlighter::Highlighter(PersistenceInterface& persistence)
    : persistence(persistence)
    , selectionStart(-1)
    , selectionEnd(-1)
    , selecting(false)
    , nextId(1)
    , activeTypeId(1) {
    types = persistence.loadHighlightTypes();
    if (types.empty()) {
        types.push_back(DEFAULT_HIGHLIGHT_TYPE);
    }
    activeTypeId = types[0].id;
    load();
}

void Highlighter::setProvider(const std::string& name) {
    currentProvider = name;
    load();
}

const std::string& Highlighter::getProvider() const {
    return currentProvider;
}

void Highlighter::load() {
    highlights = persistence.loadHighlights();
    highlights.erase(
        std::remove_if(highlights.begin(), highlights.end(),
            [this](const Highlight& h) {
                return !h.providerName.empty() && h.providerName != currentProvider;
            }),
        highlights.end());
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
    h.typeId = activeTypeId;
    h.providerName = currentProvider;

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
                if (t.id == h.typeId) return ToColor(t.color);
            }
            return ToColor(DEFAULT_HIGHLIGHT_TYPE.color);
        }
    }
    return {0, 0, 0, 0};
}

const Highlight* Highlighter::highlightAtWord(int wordId) const {
    for (const auto& h : highlights) {
        if (wordId >= h.startWord && wordId <= h.endWord) {
            return &h;
        }
    }
    return nullptr;
}

void Highlighter::removeHighlight(int id) {
    highlights.erase(
        std::remove_if(highlights.begin(), highlights.end(),
            [id](const Highlight& h) { return h.id == id; }),
        highlights.end());
    persistence.removeHighlight(id);
}

void Highlighter::recolorHighlight(int highlightId, int newTypeId) {
    for (auto& h : highlights) {
        if (h.id == highlightId) {
            h.typeId = newTypeId;
            persistence.saveHighlight(h);
            return;
        }
    }
}

void Highlighter::setActiveTypeId(int typeId) {
    activeTypeId = typeId;
}

int Highlighter::getActiveTypeId() const {
    return activeTypeId;
}

const std::vector<Highlight>& Highlighter::getHighlights() const {
    return highlights;
}

const std::vector<HighlightType>& Highlighter::getTypes() const {
    return types;
}
