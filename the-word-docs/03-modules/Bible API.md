# Bible API Module

> Status: Implemented, wired | Last Updated: 2026-06-22

Files: `src/data/BibleClient.h/cpp`

## Overview

`BibleClient` is an online `ChapterProvider` implementation. It fetches Bible passages from the YouVersion API, parses the HTML response, and produces `ChapterData` (identical format to `USFMParser`).

## Interface

```cpp
class BibleClient : public ChapterProvider {
public:
    BibleClient(APIClient& apiClient, int bibleId);
    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;
};
```

## API Endpoint

```
GET /bibles/{bibleId}/passages/{usfm}?format=html&include_headings=true
```

## HTML Parsing Strategy

The API returns HTML like:

```html
<div class="s1 yv-h">The Creation</div>
<div class="p">
  <span class="yv-v" v="1"></span>
  <span class="yv-vlbl">1</span>In the beginning God created...
  <span class="yv-n f"><span class="fr">1:3 </span><span class="ft">footnote text</span></span>
</div>
<div class="q1">
  <span class="yv-v" v="27"></span>
  <span class="yv-vlbl">27</span>So God created man...
</div>
<div class="q2">in the image of God He created him;</div>
```

| HTML Element | Segment Type | Level |
|-------------|--------------|-------|
| `<div class="s1 yv-h">` | SectionHeading | 1 |
| `<div class="s2 yv-h">` | SectionHeading | 2 |
| `<div class="p">` | ParagraphBreak | 0 |
| `<div class="q1">` | PoetryLine | 1 |
| `<div class="q2">` | PoetryLine | 2 |
| `<span class="yv-v" v="N">` | VerseText (marker) | verse N |

**Parsing rules:**
1. Split on `<div class="...">` boundaries
2. For `s1`/`s2` divs: extract text content as heading, emit `SectionHeading` segment
3. For `p` divs: emit `ParagraphBreak` segment, then parse verse spans
4. For `q1`/`q2` divs: emit `PoetryLine` segment
5. For `<span class="yv-v" v="N">`: set current verse number
6. For `<span class="yv-vlbl">`: skip (verse number label, already counted above)
7. For `<span class="yv-n ...">`: **strip entirely** (footnotes not supported in MVP)
8. Text outside verse spans (e.g., mid-chapter refrains) is assigned to the nearest preceding verse

## Output

Both `BibleClient` and `USFMParser` produce identical `ChapterData`:

```cpp
ChapterData {
    bookId: "GEN",
    chapterNum: 1,
    words: [{id:0, verseId:1, text:"No"}, {id:1, verseId:1, text:"princípio"}, ...],
    segments: [
        {SectionHeading, level:1, text:"A Criação"},
        {ParagraphBreak, level:0},
        {VerseText, verseStart:1, verseEnd:5, startWordIndex:0, wordCount:38},
        ...
    ]
};
```

## Bible Versions

| Version | ID | Notes |
|---------|-----|-------|
| BSB (Berean Standard Bible) | 3034 | Public domain, works with standard app key |
| NVI (Nova Versão Internacional) | — | Requires special license from Biblica |

## Runtime Behavior

BibleClient is always compiled and present in every build. At startup, `main.cpp` loads `.env` and checks for a `YVP_APP_KEY`. If found, a `CompositeProvider` wraps BibleClient (primary) and USFMParser (fallback). If no key is found, USFMParser is used directly — no API calls are made, no errors logged.

## Dependencies

- `APIClient` (libcurl) — hard dependency, always compiled. Both online and offline providers are present in every build.
- `ChapterProvider.h` — interface definition
