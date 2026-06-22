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

    void load();
    const std::vector<Highlight>& getHighlights() const;

private:
    PersistenceInterface& persistence;
    std::vector<Highlight> highlights;
    std::vector<HighlightType> types;

    int selectionStart;
    int selectionEnd;
    bool selecting;
    int nextId;
};

#endif
