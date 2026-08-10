#include "Highlighter.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "core/BibleBooks.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>

namespace theword::highlight {
namespace {
const HighlightType DEFAULT_HIGHLIGHT_TYPE { 1, "Amarelo", {255, 255, 0, 100} };
} // namespace

std::string Highlighter::AssembleSelectedText(const theword::data::ChapterData& data,
                                              int startWord, int endWord) {
    int s = (std::min)(startWord, endWord);
    int e = (std::max)(startWord, endWord);
    if (s < 0 || e >= static_cast<int>(data.words.size())) return {};

    // Determine verse range
    int firstVerse = data.words[s].verseId;
    int lastVerse = data.words[e].verseId;

    // Build citation prefix
    int idx = theword::core::FindBookIndex(data.bookId);
    std::string bookName = (idx >= 0) ? theword::core::BOOK_NAMES_PT[idx] : data.bookId;
    std::ostringstream citation;
    citation << bookName << " " << data.chapterNum << ":" << firstVerse;
    if (lastVerse > firstVerse) citation << "-" << lastVerse;
    citation << "\n\n";

    // Build body text
    std::ostringstream body;
    for (const auto& w : data.words) {
        if (w.id >= s && w.id <= e) {
            if (body.tellp() > 0) body << " ";
            body << w.text;
        }
    }
    return citation.str() + body.str();
}

void Highlighter::FindVerseRange(const std::vector<theword::data::Word>& words,
                                 int anchorWord, int& verseStart, int& verseEnd) {
    int targetVerse = -1;
    for (const auto& w : words) {
        if (w.id == anchorWord) {
            targetVerse = w.verseId;
            break;
        }
    }
    if (targetVerse < 0) { verseStart = anchorWord; verseEnd = anchorWord; return; }

    verseStart = anchorWord;
    verseEnd = anchorWord;
    for (const auto& w : words) {
        if (w.verseId == targetVerse) {
            if (w.id < verseStart) verseStart = w.id;
            if (w.id > verseEnd) verseEnd = w.id;
        }
    }
}

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
            StartSelection(e.startWordId, e.bookId, e.chapterNum);
            break;
        case theword::event::SelectionEvent::Action::Update:
            UpdateSelection(e.endWordId);
            break;
        case theword::event::SelectionEvent::Action::End:
            EndSelection(e.bookId, e.chapterNum);
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

void Highlighter::StartSelection(int wordId, const std::string& bookId, int chapterNum) {
    selecting = true;
    selectionStart = wordId;
    selectionEnd = wordId;
    selectBookId_ = bookId;
    selectChapterNum_ = chapterNum;
}

void Highlighter::UpdateSelection(int wordId) {
    if (!selecting) return;
    selectionEnd = wordId;
}

void Highlighter::EndSelection(const std::string& bookId, int chapterNum) {
    if (!selecting) return;
    selecting = false;

    int start = (std::min)(selectionStart, selectionEnd);
    int end = (std::max)(selectionStart, selectionEnd);
    if (start < 0) return;

    CommitSelection(start, end, bookId, chapterNum);
}

void Highlighter::ClearCommittedSelection() {
    committedStart_ = -1;
    committedEnd_ = -1;
    committedBookId_.clear();
    committedChapterNum_ = 0;
}

void Highlighter::CommitSelection(int startWord, int endWord, const std::string& bookId, int chapterNum) {
    committedStart_ = startWord;
    committedEnd_ = endWord;
    committedBookId_ = bookId;
    committedChapterNum_ = chapterNum;
}

void Highlighter::CreateHighlight(int startWord, int endWord, int typeId,
                                   const std::string& bookId, int chapterNum,
                                   const std::vector<theword::data::Word>* words) {
    int start = (std::min)(startWord, endWord);
    int end = (std::max)(startWord, endWord);
    if (start < 0) return;

    Highlight h;
    h.id = nextId++;
    h.startWord = start;
    h.endWord = end;
    h.typeId = typeId;
    h.providerName = currentProvider;
    h.bookId = bookId;
    h.chapterNum = chapterNum;

    if (words && !words->empty()) {
        int vStart = 9999;
        int vEnd = 0;
        std::string snippet;
        for (const auto& w : *words) {
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
    ClearCommittedSelection();
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

SimpleColor Highlighter::GetHighlightForWord(int wordId) const {
    for (const auto& h : highlights) {
        if (h.bookId == currentBookId_ && h.chapterNum == currentChapterNum_
            && wordId >= h.startWord && wordId <= h.endWord) {
            for (const auto& t : types) {
                if (t.id == h.typeId) return t.color;
            }
            return DEFAULT_HIGHLIGHT_TYPE.color;
        }
    }
    return {0, 0, 0, 0};
}

bool Highlighter::IsWordHighlighted(int wordId, const std::string& bookId, int chapterNum) const {
    for (const auto& h : highlights) {
        if (h.bookId == bookId && h.chapterNum == chapterNum
            && wordId >= h.startWord && wordId <= h.endWord) return true;
    }
    return false;
}

SimpleColor Highlighter::GetHighlightForWord(int wordId, const std::string& bookId, int chapterNum) const {
    for (const auto& h : highlights) {
        if (h.bookId == bookId && h.chapterNum == chapterNum
            && wordId >= h.startWord && wordId <= h.endWord) {
            for (const auto& t : types) {
                if (t.id == h.typeId) return t.color;
            }
            return DEFAULT_HIGHLIGHT_TYPE.color;
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

const Highlight* Highlighter::HighlightAtWord(int wordId, const std::string& bookId, int chapterNum) const {
    for (const auto& h : highlights) {
        if (h.bookId == bookId && h.chapterNum == chapterNum
            && wordId >= h.startWord && wordId <= h.endWord) {
            return &h;
        }
    }
    return nullptr;
}

const Highlight* Highlighter::HighlightOverlapping(int startWord, int endWord,
                                                    const std::string& bookId, int chapterNum) const {
    for (const auto& h : highlights) {
        if (h.bookId == bookId && h.chapterNum == chapterNum
            && h.startWord <= endWord && h.endWord >= startWord) {
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
