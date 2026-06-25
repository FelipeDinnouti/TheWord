#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "PersistenceInterface.h"
#include <vector>
#include <raylib.h>

class Highlighter {
public:
    explicit Highlighter(PersistenceInterface& persistence);

    void StartSelection(int wordId);
    void UpdateSelection(int wordId);
    void EndSelection();

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
    PersistenceInterface& persistence;
    std::vector<Highlight> highlights;
    std::vector<HighlightType> types;

    int selectionStart;
    int selectionEnd;
    bool selecting;
    int nextId;
    int activeTypeId;
    std::string currentProvider;
};

#endif
