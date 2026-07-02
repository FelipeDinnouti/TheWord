#include "Highlighter.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "core/Logger.h"
#include <algorithm>

namespace theword::highlight {
namespace {
const HighlightType DEFAULT_HIGHLIGHT_TYPE { 1, "Yellow", {255, 255, 0, 100} };
Color ToColor(const SimpleColor& c) { return {c.r, c.g, c.b, c.a}; }
} // namespace

Highlighter::Highlighter(theword::event::EventBus& eventBus, PersistenceInterface& persistence)
    : eventBus_(eventBus), persistence(persistence)
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

    eventBus_.On<theword::event::SelectionEvent>([this](const auto& e) { OnSelection(e); });
}

void Highlighter::OnSelection(const theword::event::SelectionEvent& e) {
    switch (e.action) {
        case theword::event::SelectionEvent::Action::Start:
            StartSelection(e.startWordId);
            break;
        case theword::event::SelectionEvent::Action::Update:
            UpdateSelection(e.endWordId);
            break;
        case theword::event::SelectionEvent::Action::End:
            EndSelection();
            break;
        case theword::event::SelectionEvent::Action::Cancel:
            selecting = false;
            break;
    }
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
    h.bookId = currentBookId_;
    h.chapterNum = currentChapterNum_;

    if (!currentWords_.empty()) {
        int vStart = 9999;
        int vEnd = 0;
        std::string snippet;
        for (const auto& w : currentWords_) {
            if (w.id >= start && w.id <= end) {
                if (w.verseId < vStart) vStart = w.verseId;
                if (w.verseId > vEnd) vEnd = w.verseId;
                if (!snippet.empty()) snippet += " ";
                snippet += w.text;
            }
        }
        h.verseStart = (vStart < 9999) ? vStart : 0;
        h.verseEnd = vEnd;
        if (snippet.length() > 80) {
            snippet = snippet.substr(0, 80) + "...";
        }
        h.verseText = snippet;
    }

    highlights.push_back(h);
    persistence.SaveHighlight(h);
    theword::core::Logger::Debug("Highlight saved: " + h.bookId + "." + std::to_string(h.chapterNum)
        + " words " + std::to_string(start) + "-" + std::to_string(end));
}

bool Highlighter::IsWordHighlighted(int wordId) const {
    for (const auto& h : highlights) {
        if (h.bookId == currentBookId_ && h.chapterNum == currentChapterNum_
            && wordId >= h.startWord && wordId <= h.endWord) return true;
    }
    return false;
}

Color Highlighter::GetHighlightForWord(int wordId) const {
    for (const auto& h : highlights) {
        if (h.bookId == currentBookId_ && h.chapterNum == currentChapterNum_
            && wordId >= h.startWord && wordId <= h.endWord) {
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
        if (h.bookId == currentBookId_ && h.chapterNum == currentChapterNum_
            && wordId >= h.startWord && wordId <= h.endWord) {
            return &h;
        }
    }
    return nullptr;
}

void Highlighter::RemoveHighlight(int id) {
    theword::core::Logger::Debug("Highlight deleted: id " + std::to_string(id));
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

std::vector<const Highlight*> Highlighter::GetHighlightsByType(int typeId) const {
    std::vector<const Highlight*> result;
    for (const auto& h : highlights) {
        if (h.typeId == typeId) {
            result.push_back(&h);
        }
    }
    return result;
}

void Highlighter::SetChapterContext(const std::string& bookId, int chapterNum,
                                     const std::vector<theword::data::Word>* words) {
    currentBookId_ = bookId;
    currentChapterNum_ = chapterNum;
    if (words) {
        currentWords_ = *words;
    } else {
        currentWords_.clear();
    }
}

} // namespace theword::highlight
