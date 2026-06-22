#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "PersistenceInterface.h"
#include <vector>
#include <raylib.h>

class Highlighter {
public:
    explicit Highlighter(PersistenceInterface& persistence);

    void startSelection(int wordId);
    void updateSelection(int wordId);
    void endSelection();

    bool isWordHighlighted(int wordId) const;
    Color getHighlightForWord(int wordId) const;
    const Highlight* highlightAtWord(int wordId) const;

    void removeHighlight(int id);
    void recolorHighlight(int highlightId, int newTypeId);

    void setActiveTypeId(int typeId);
    int getActiveTypeId() const;

    void setProvider(const std::string& name);
    const std::string& getProvider() const;

    void load();
    const std::vector<Highlight>& getHighlights() const;
    const std::vector<HighlightType>& getTypes() const;

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
