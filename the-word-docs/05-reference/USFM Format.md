# USFM Format

> Status: Reference | Last Updated: 2026-06-22

USFM (Unfolding Scripture Format Marked) is a plain-text markup format for encoding Bible text. See [ubsicap.github.io/usfm/](https://ubsicap.github.io/usfm/) for the full specification.

## Common Markers (Supported)

| Marker | Meaning | Segment Type | Example |
|--------|---------|-------------|---------|
| `\id` | Book ID | (metadata) | `\id GEN` |
| `\h` | Header/abbrev title | (metadata) | `\h Gênesis` |
| `\mt1` | Major title level 1 | BookTitle | `\mt1 Gênesis` |
| `\c` | Chapter number | ChapterLabel | `\c 1` |
| `\s1`-`\s5` | Section heading | SectionHeading | `\s1 A Criação` |
| `\p` | Paragraph | ParagraphBreak | `\p` |
| `\q1`-`\q3` | Poetry line | PoetryLine | `\q1 Palavras do sábio` |
| `\v` | Verse number | (boundary) | `\v 16` |

## Markers (Ignored / Stripped)

| Marker | Meaning | Handling |
|--------|---------|----------|
| `\f ... \f*` | Footnote | Stripped entirely |
| `\add ... \add*` | Added words | Text kept, markers stripped |
| `\wj ... \wj*` | Words of Jesus | Text kept, markers stripped |
| `\r` | Parallel ref heading | Treated as SectionHeading level 1 |
| `\toc1`-`\toc3` | Table of contents | (metadata, ignored) |

## Example (Bíblia Livre, Genesis 1)

```
\id GEN Bíblia Livre - Textus Receptus 
\ide UTF-8
\h Gênesis 
\toc1 Gênesis 
\toc2 Gênesis 
\toc3 Gn 
\mt1 Gênesis  
\c 1  
\p
\v 1 No princípio criou Deus os céus e a terra.  
\v 2 E a terra estava desordenada e vazia...
\v 3 E disse Deus: Haja luz; e houve luz.  
```

This produces:
```cpp
ChapterData {
    bookId: "GEN",
    chapterNum: 1,
    words: [{id:0, verseId:1, text:"No"}, {id:1, verseId:1, text:"princípio"}, ...],
    segments: [
        {BookTitle,    level:1, text:"Gênesis"},
        {ChapterLabel, level:0, text:"1"},
        {ParagraphBreak},
        {VerseText,    verseStart:1, verseEnd:1, words[0..5]},
        {VerseText,    verseStart:2, verseEnd:2, words[6..10]},
        ...
    ]
};
```

## Data Source

The project uses **Bíblia Livre** (porbr2018, CC BY 4.0) as its primary USFM source:
- Available at [ebible.org/porbr2018/](https://ebible.org/porbr2018/)
- Download: [porbr2018_usfm.zip](https://ebible.org/Scriptures/porbr2018_usfm.zip)
- License: Creative Commons Attribution 4.0
- Attribution: "Bíblia Livre (BLIVRE), Copyright © Diego Santos, Mario Sérgio, e Marco Teles"

## USFM Parser vs BibleClient Output

Both the USFM Parser (offline) and BibleClient (online) produce identical `ChapterData` output. The LayoutEngine and Renderer do not care which source produced the data.
