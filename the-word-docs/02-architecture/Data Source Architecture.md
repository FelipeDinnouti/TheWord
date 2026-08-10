# Data Source Architecture

> Status: Active Design | Last Updated: 2026-06-22

## Dual-Source Design

The app supports two data sources that produce identical output formats. The `DocumentManager` never knows which source it is reading from — it only talks to a `ChapterProvider` interface.

```
┌──────────────────────────────────────────────────┐
│                 UI / Renderer                     │
├──────────────────────────────────────────────────┤
│               DocumentManager                     │
│            (holds ChapterProvider&)               │
├──────────────────────────────────────────────────┤
│              ChapterProvider                      │
│            (abstract interface)                   │
│                ┌────┴────┐                        │
│                │         │                        │
│          USFMParser  BibleClient                  │
│         (offline .usfm)  (online HTML API)        │
└──────────────────────────────────────────────────┘
```

## ChapterProvider Interface

```cpp
class ChapterProvider {
public:
    virtual ~ChapterProvider() = default;
    virtual std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) = 0;
    virtual const char* ProviderName() const = 0;
};
```

Both `USFMParser` and `BibleClient` implement this interface. `DocumentManager` receives a `ChapterProvider&` at construction time and calls `LoadChapter()` when it needs text.

## Output Format: ChapterData

Both providers produce `ChapterData`, which contains:
- A flat array of all words in the chapter (with global word IDs and verse IDs)
- A `segments` array describing the rich-text structure (headings, poetry, paragraphs)

This is the single format the LayoutEngine consumes. Neither its inner loop nor the renderer cares which provider produced the data.

## Offline Source: USFMParser

- Reads `assets/usfm/*.usfm` files (one per book, 66 files)
- Parses USFM markers into `Segment[]` + `Word[]`
- Always available, no network required
- **Fallback** when the API is unavailable or the device is offline

## Online Source: BibleClient

- Fetches `GET /bibles/{id}/passages/{usfm}?format=html`
- Parses HTML `<div>` classes (`p`, `q1`, `q2`, `s1`, `s2`) into `Segment[]` + `Word[]`
- Strips footnotes/notes from the HTML
- Requires network + API key
- **Primary source** when online

## Source Selection Strategy

| Scenario | Source Used |
|----------|-------------|
| Development (no API key) | USFMParser only |
| Production (API key present, online) | BibleClient (primary), USFMParser fallback |
| Production (API key present, offline) | USFMParser fallback only |

The `DocumentManager` can be configured with a preferred `ChapterProvider`. If that provider returns `nullopt` for a chapter, it falls back to the secondary provider.

## Why Dual Source?

1. **Online-first**: Rich API provides licensed versions (NVI, NAA) when online; USFM fallback ensures the app is never empty
2. **Development convenience**: USFM files let us iterate offline without API calls during development
3. **Resilience**: If the API is unreachable, the app degrades gracefully to the offline USFM source
4. **Same pipeline**: Both sources produce `ChapterData` — no code duplication in the layout/rendering path
