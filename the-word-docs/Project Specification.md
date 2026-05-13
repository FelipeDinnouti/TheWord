I want a simple and minimalist Bible app that let's me have configurable reading and highlighting, being a tool for studying the Bible, no more. 

Essentially a mini text engine + UI framework, using Raylib as the base to render text and handle window management. The goal is be an mobile app, but for now we keep it simple and make a proof-of-concept for desktop in a mobile window-ratio.

**Rendering:**
- Text layout (word wrapping, line breaking)
- Word indexing
- Highlight rectangles aligned to text
- Infinite scrolling document with window

**Interaction:**
- Users may interact with text to highlight, there needs to be per-word hit detection, and this is the foundation to implement drag selection to highlight
## Foundations

**Text Engine:** Handling text is not easy and it should be done in a robust and clear manner, handling word wrapping, line breaks, color changes, etc. Simply a text renderer.

**[[Document]] Manager:** The text engine merely handles what the document contains. Document is responsible for containing the structures that define words, line breaks, titles, etc. It is also what handles prepending and appending of content through infinite scrolling

**Highlighting:** The whole motivation for this project. The goal is to be able to highlight per-word sections of verses with different colors, and later be able to query only verses with a certain color. Colors are like tags, and if the user wants, they could be named.

## Implementation Details

Core structures (only ideas, not final)

```cpp
typedef struct {
	char* name;
	int id;
	Color color;
} HighlightType;
```

```cpp
typedef struct {
    int start_word;
    int end_word;
    int highlight_type_id;
} Highlight;
```

```cpp
typedef struct {
    int word_index;
    float x, y;
    float width, height;
} WordLayout;
```

```cpp
typedef struct {
    int id;
    char* text;
} Word;
```

```cpp
typedef struct {
    std::vector<Word> words;
} Verse;
```

```cpp
typedef struct {
	std::vector<Verse> verses;
	int scroll_position;
} Document;
```