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
    Color GetHighlightForWord(int wordId) const;
    const Highlight* HighlightAtWord(int wordId) const;

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

    void StartSelection(int wordId);
    void UpdateSelection(int wordId);
    void EndSelection();
};

} // namespace theword::highlight

#endif
