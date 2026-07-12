#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "PersistenceInterface.h"
#include "data/ChapterProvider.h"
#include <vector>
#include <raylib.h>

namespace theword::event {
    class EventBus;
    struct SelectionEvent;
}

namespace theword::highlight {

class Highlighter {
public:
    Highlighter(theword::event::EventBus& eventBus, PersistenceInterface& persistence);

    void OnSelection(const theword::event::SelectionEvent& e);

    bool IsWordHighlighted(int wordId) const;
    bool IsWordHighlighted(int wordId, const std::string& bookId, int chapterNum) const;
    Color GetHighlightForWord(int wordId) const;
    Color GetHighlightForWord(int wordId, const std::string& bookId, int chapterNum) const;
    const Highlight* HighlightAtWord(int wordId) const;
    const Highlight* HighlightAtWord(int wordId, const std::string& bookId, int chapterNum) const;
    const Highlight* HighlightOverlapping(int startWord, int endWord,
                                          const std::string& bookId, int chapterNum) const;

    void RemoveHighlight(int id);
    void RecolorHighlight(int highlightId, int newTypeId);

    void SetActiveTypeId(int typeId);
    int GetActiveTypeId() const;

    void SetProvider(const std::string& name);
    const std::string& GetProvider() const;

    void Load();
    const std::vector<Highlight>& GetHighlights() const;
    const std::vector<HighlightType>& GetTypes() const;

    std::vector<const Highlight*> GetHighlightsByType(int typeId) const;

    void SetChapterContext(const std::string& bookId, int chapterNum,
                           const std::vector<theword::data::Word>* words);
    const std::string& GetChapterBookId() const { return currentBookId_; }
    int GetChapterNum() const { return currentChapterNum_; }

    bool IsSelecting() const { return selecting; }
    int GetSelectionStart() const { return selectionStart; }
    int GetSelectionEnd() const { return selectionEnd; }
    const std::string& GetSelectBookId() const { return selectBookId_; }
    int GetSelectChapterNum() const { return selectChapterNum_; }

    bool HasCommittedSelection() const { return committedStart_ >= 0; }
    int GetCommittedStart() const { return committedStart_; }
    int GetCommittedEnd() const { return committedEnd_; }
    const std::string& GetCommittedBookId() const { return committedBookId_; }
    int GetCommittedChapterNum() const { return committedChapterNum_; }
    void ClearCommittedSelection();
    void CommitSelection(int startWord, int endWord, const std::string& bookId, int chapterNum);
    void CreateHighlight(int startWord, int endWord, int typeId,
                         const std::string& bookId, int chapterNum,
                         const std::vector<theword::data::Word>* words = nullptr);

private:
    theword::event::EventBus& eventBus_;
    PersistenceInterface& persistence;
    std::vector<Highlight> highlights;
    std::vector<HighlightType> types;

    int selectionStart;
    int selectionEnd;
    bool selecting;
    int nextId;
    int activeTypeId;
    std::string currentProvider;

    std::string currentBookId_;
    int currentChapterNum_ = 0;
    std::vector<theword::data::Word> currentWords_;

    std::string selectBookId_;
    int selectChapterNum_ = 0;

    int committedStart_ = -1;
    int committedEnd_ = -1;
    std::string committedBookId_;
    int committedChapterNum_ = 0;

    void StartSelection(int wordId, const std::string& bookId, int chapterNum);
    void UpdateSelection(int wordId);
    void EndSelection(const std::string& bookId, int chapterNum);
};

} // namespace theword::highlight

#endif
