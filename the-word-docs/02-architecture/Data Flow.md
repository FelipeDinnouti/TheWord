# Data Flow

> Status: Updated 2026-06-22

## Offline Startup Flow (USFM)

```
main()
  ├── InitWindow(450, 800, "TheWord")
  ├── Build codepoint vector (ASCII 32-126 + Latin-1 160-255)
  ├── LoadFontEx(bodyFont at FONT_SIZE=24, codepoints) + SetTextureFilter(POINT)
  ├── LoadFontEx(headingFont at FONT_HEADING_SIZE=31, codepoints) + SetTextureFilter(POINT)
  ├── Create LayoutEngine(maxWidth, bodyFont, fontSize, lineSpacing)
  ├── Create USFMParser(assets/usfm/)
  ├── Create DocumentManager(layoutEngine, viewportHeight, usfmParser)
  ├── Load initial chapter (GEN.1)
  │     └── DocumentManager::loadInitialChapter("GEN.1")
  │           └── USFMParser::LoadChapter("GEN", 1) → ChapterData
  │                 ├── words: [{id:0, verseId:1, text:"No"}, {id:1, verseId:1, text:"princípio"}, ...]
  │                 └── segments: [VerseText(1-18)]
  │           └── LayoutEngine::layoutChapter("GEN.1", chapterData)
  │                 ├── Iterate segments
  │                 ├── For VerseText: wrap words into lines
  │                 ├── For SectionHeading: center bold text
  │                 └── return ChapterLayout
  └── Enter render loop
```

## Online Startup Flow (API)

```
main()
  ├── ...
  ├── Create APIClient(appKey)
  ├── Create BibleClient(apiClient)
  ├── Create DocumentManager(layoutEngine, viewportHeight, bibleClient)
  ├── Load initial chapter (JHN.3)
  │     └── BibleClient::LoadChapter("JHN", 3) → ChapterData
  │           ├── GET /bibles/3034/passages/JHN.3?format=html&include_headings=true
  │           ├── Parse HTML:
  │           │     <div class="s1 yv-h"> → Segment{SectionHeading, level:1}
  │           │     <div class="p">       → Segment{ParagraphBreak}
  │           │     <span class="yv-v">  → Segment{VerseText}
  │           │     <span class="yv-n">  → stripped (footnotes)
  │           ├── words: [{id:0, verseId:1}, {id:1, verseId:1}, ...]
  │           └── segments: [SectionHeading("The Creation"), ParagraphBreak, VerseText(1-18)]
  │           └── LayoutEngine::layoutChapter("JHN.3", chapterData)
  └── Enter render loop
```

## Render Loop Flow (per frame — desktop)

```
while (!WindowShouldClose())
  ├── Handle input
  │     ├── GetMouseWheelMove() → update scrollVelocity
  │     ├── Keyboard input → update scrollVelocity
  │     ├── Touch gestures (Android/WASM) → HandleTouchScroll, HandleTouchPressFSM, HandlePinch
  │     └── Apply friction to scrollVelocity
  │
  ├── DocumentManager::scrollBy(scrollVelocity)
  │     └── Update targetScrollY, clamp to bounds
  │
  ├── Auto-load chapters (near scroll boundaries)
  │     ├── scrollY near 0 → prepend previous chapter
  │     │     ├── ChapterProvider::LoadChapter(prevBook, prevChapter) → ChapterData
  │     │     └── DocumentManager::prependChapter(chapterData)
  │     │           └── LayoutEngine::layoutChapter(chapterData)
  │     └── scrollY near end → append next chapter
  │           ├── ChapterProvider::LoadChapter(nextBook, nextChapter) → ChapterData
  │           └── DocumentManager::appendChapter(chapterData)
  │                 └── LayoutEngine::layoutChapter(chapterData)
  │
  ├── DocumentManager::update(deltaTime)
  │     └── Lerp scrollY toward targetScrollY
  │
  ├── Handle window resize
  │     ├── Recalculate maxWidth
  │     ├── LayoutEngine::setMaxWidth(newWidth)
  │     └── DocumentManager::setViewportHeight(newHeight)
  │
  ├── BeginDrawing()
  │     ├── ClearBackground(RAYWHITE)
  │     ├── Draw chapter title with headingFont via DrawTextEx(...)
  │     ├── DocumentManager::getVisibleSpans(...)
  │     │     └── For each loaded chapter:
  │     │           └── For each line:
  │     │                 └── For each span:
  │     │                       └── Convert (spanY → screenY)
  │     │                       └── Collect (span, screenY)
  │     ├── Draw each visible span (selects font by span type)
  │     │     ├── VerseText/PoetryLine → bodyFont (atlas 24px, 1:1)
  │     │     ├── SectionHeading/ChapterLabel/BookTitle → headingFont (atlas 31px, 1:1)
  │     │     └── DrawTextEx(selectedFont, span.text, {x, screenY}, ...)
  │     ├── Draw highlight rectangles (query Highlighter for visible word range)
  │     ├── Draw UI overlays (context menu, go-to dialog, settings, about)
  │     ├── Draw scrollbar
  │     └── DrawFPS(...) (debug builds only)
  └── EndDrawing()
```

## Render Loop Flow (per frame — Android)

```
while (!WindowShouldClose())
  ├── Check Android lifecycle (GetAndroidApp()->window not null)
  ├── Handle input
  │     ├── Touch scroll (single-finger drag) → HandleTouchScroll
  │     ├── Touch press FSM (Idle→Pending→Dragging/LongPress) → HandleTouchPressFSM
  │     ├── Pinch zoom → HandlePinch (future use)
  │     └── Mouse events (ChromeOS with mouse) → desktop mouse path
  │
  ├── ... (same as desktop: DocumentManager, auto-load, BeginDrawing, EndDrawing)
  └── EndDrawing()
```

On Android, keyboard events come as raw AKEYCODE values (not raylib KEY_* constants).
A patch (`patches/raylib-android-keycodes.patch`) adds translation in the raylib platform layer.
Without the patch, `Config.h` provides fallback constants in `namespace key`.

## Prepend Flow (detailed)

```
User scrolls past top of first loaded chapter
  → DocumentManager::canPrepend() → true
  → getPreviousChapter(currentChapter) → "JHN.2"
  → ChapterProvider::LoadChapter("JHN", 2) → ChapterData
  → DocumentManager::prependChapter(chapterData)
      1. LayoutEngine::layoutChapter("JHN.2", chapterData) → ChapterLayout{totalHeight: 500}
      2. Create LoadedChapter{chapterId: "JHN.2", ...}
      3. Insert at chapters.begin()
      4. Recalculate all chapter positions (shift existing chapters down)
      5. scrollY += 500  → anchor-fixed: visible content stays in place
```

## Segment-Aware Layout Diagram

```
ChapterData::segments = [
    {SectionHeading, level:1, text: "The Creation"},   → centered, bold, 24px
    {ParagraphBreak},                                    → 8px vertical gap
    {VerseText, verseStart:1, verseEnd:5, words[0..30]}, → left-aligned wrapped text
    {SectionHeading, level:2, text: "The First Day"},   → centered, semi-bold, 20px
    {VerseText, verseStart:3, verseEnd:5, words[30..45]}, → continues verse text
    {PoetryLine, level:1, verseStart:27, words[100..105]}, → 20px left indent
    {PoetryLine, level:2, verseStart:27, words[105..110]}, → 40px left indent
]
```

## Resize Flow

```
Window width changes
  → LayoutEngine::setMaxWidth(newWidth)
  → LayoutEngine::invalidateCache()  → clear cached layouts
  → For each loaded chapter:
      → LayoutEngine::layoutChapter(chapterId, chapterData)
  → Recalculate chapter positions
  → Clamp scrollY to valid range
```
