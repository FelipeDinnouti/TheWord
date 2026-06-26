#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "PersistenceInterface.h"
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

    void StartSelection(int wordId);
    void UpdateSelection(int wordId);
    void EndSelection();
};

} // namespace theword::highlight

#endif
