# YouVersion API

> Status: Reference | Last Updated: 2026-06-22

## Base URL

```
https://api.youversion.com/v1
```

## Authentication

Header: `X-YVP-App-Key: <your_key>`

Get an API key at [platform.youversion.com](https://platform.youversion.com).

## Key Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /bibles` | List available Bible versions |
| `GET /bibles/{id}/passages/{usfm}?format=html&include_headings=true` | Fetch passage as HTML |

## HTML Response Format

The API returns HTML with structured `<div>` and `<span>` elements:

```html
<div class="s1 yv-h">Section Heading Level 1</div>
<div class="s2 yv-h">Section Heading Level 2</div>
<div class="p">
  <span class="yv-v" v="1"></span>
  <span class="yv-vlbl">1</span>Verse text here...
  <span class="yv-n f">
    <span class="fr">1:3 </span>
    <span class="ft">footnote text</span>
  </span>
</div>
<div class="q1">
  <span class="yv-v" v="27"></span>
  <span class="yv-vlbl">27</span>Poetry line level 1
</div>
<div class="q2">Poetry line level 2 (continuation)</div>
```

### Element Reference

| HTML Element | Meaning | Parsed As |
|-------------|---------|-----------|
| `<div class="s1 yv-h">` | Section heading level 1 | `Segment{SectionHeading, level:1}` |
| `<div class="s2 yv-h">` | Section heading level 2 | `Segment{SectionHeading, level:2}` |
| `<div class="p">` | Paragraph | `Segment{ParagraphBreak}` then `VerseText` segments |
| `<div class="q1">` | Poetry line level 1 | `Segment{PoetryLine, level:1}` |
| `<div class="q2">` | Poetry line level 2 | `Segment{PoetryLine, level:2}` |
| `<span class="yv-v" v="N">` | Verse boundary marker | Current verse = N |
| `<span class="yv-vlbl">` | Verse number display | Skipped (text already in flow) |
| `<span class="yv-n ...">` | Footnote/note | **Stripped entirely** (not rendered) |

## Bible Versions

### Curated Public-Domain List

These are the versions the app supports as selectable options. All are public domain and work with a standard app key:

| ID | Abbreviation | Full Name | Notes |
|----|-------------|-----------|-------|
| 12 | ASV | American Standard Version | Public domain, English |
| 206 | WEBUS | World English Bible (US) | Public domain, English |
| 3034 | BSB | Berean Standard Bible | Public domain, English — **default** |

### Other Versions (for reference)

| ID | Abbreviation | Name | Notes |
|----|-------------|------|-------|
| 111 | NIV | New International Version | ❌ Requires special Biblica license |
| 1 | — | Legacy numbering | Outdated; see ID 12 for ASV |
| 129 | — | Legacy BSB numbering | Outdated; see ID 3034 |

## Usage in TheWord

The API is consumed by `BibleClient` (implementing `ChapterProvider`). The HTML is parsed into `ChapterData` with the same `Segment[]` + `Word[]` structure that `USFMParser` produces. This means the rendering pipeline is identical regardless of data source.

## .env Configuration

```env
YVP_APP_KEY=your_app_key_here
```

Required for `BibleClient` (online primary source). Without it, the app falls back to the offline USFM source (Bíblia Livre).
