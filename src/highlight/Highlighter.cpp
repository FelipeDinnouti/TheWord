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
    types = persistence.LoadHighlightTypes();
    if (types.empty()) {
        types.push_back(DEFAULT_HIGHLIGHT_TYPE);
    }
    activeTypeId = types[0].id;
    Load();
}

void Highlighter::SetProvider(const std::string& name) {
    currentProvider = name;
    Load();
}

const std::string& Highlighter::GetProvider() const {
    return currentProvider;
}

void Highlighter::Load() {
    highlights = persistence.LoadHighlights();
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

void Highlighter::StartSelection(int wordId) {
    selecting = true;
    selectionStart = wordId;
    selectionEnd = wordId;
}

void Highlighter::UpdateSelection(int wordId) {
    if (!selecting) return;
    selectionEnd = wordId;
}

void Highlighter::EndSelection() {
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
    persistence.SaveHighlight(h);
}

bool Highlighter::IsWordHighlighted(int wordId) const {
    for (const auto& h : highlights) {
        if (wordId >= h.startWord && wordId <= h.endWord) return true;
    }
    return false;
}

Color Highlighter::GetHighlightForWord(int wordId) const {
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

const Highlight* Highlighter::HighlightAtWord(int wordId) const {
    for (const auto& h : highlights) {
        if (wordId >= h.startWord && wordId <= h.endWord) {
            return &h;
        }
    }
    return nullptr;
}

void Highlighter::RemoveHighlight(int id) {
    highlights.erase(
        std::remove_if(highlights.begin(), highlights.end(),
            [id](const Highlight& h) { return h.id == id; }),
        highlights.end());
    persistence.RemoveHighlight(id);
}

void Highlighter::RecolorHighlight(int highlightId, int newTypeId) {
    for (auto& h : highlights) {
        if (h.id == highlightId) {
            h.typeId = newTypeId;
            persistence.SaveHighlight(h);
            return;
        }
    }
}

void Highlighter::SetActiveTypeId(int typeId) {
    activeTypeId = typeId;
}

int Highlighter::GetActiveTypeId() const {
    return activeTypeId;
}

const std::vector<Highlight>& Highlighter::GetHighlights() const {
    return highlights;
}

const std::vector<HighlightType>& Highlighter::GetTypes() const {
    return types;
}
