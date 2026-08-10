# Active

> Current version: v1.9.1-alpha (code audit, in progress)
> Next: version bump + APK + device sign-off + tag → then v1.10.0-alpha
>
> Previous: `memory/archive/2026-07-20_v1.8.0-alpha-active.md`
>
> Recon audit: 2026-07-31 — all findings verified against current code, granular
> checklists below. See `memory/Architecture-Analysis.md` for finding details.

## Workstream: v1.9.1-alpha — Full Code Audit (fix-everything scope)

**Theme:** 7-phase audit: evidence → input consolidation → App slimming → event governance → data layer → tests → clean pass + release

### Implementation Order

| # | Feature | Scope | Status |
|---|---------|-------|--------|
| 0 | **Release planning + doc sync** | Roadmap, Release Plan, memory/ | ✅ Complete |
| 1 | **Evidence sweep** | dead code, layer map, input inventory, naming, ownership | ✅ Complete |
| 2 | **Input consolidation S1–S9** | InputFrame snapshot, single-poll, screens on HandleInput(ctx) | ✅ Complete |
| 3 | **App slimming** | Init split ×5, Highlighter statics, D6 dedup, onShortcut | ✅ Complete |
| 4 | **Event governance (D3/D4)** | KeyEvent + DialogEvent deleted, Event Bus.md | ✅ Complete |
| 5 | **Data layer (D-dead HasChapter)** | interface chain + cache maps deleted | ✅ Complete |
| 6 | **Tests** | 6 LayoutEngine FakeMeasure + 2 Highlighter cases | ✅ Complete |
| 7 | **Clean-code + release** | dead code sweep, docs, bump 1.9.1-alpha, APK, sign-off, tag | 🚧 In progress |

### Feature 1: A2 — LayoutTypes extraction ✅ Complete (2026-07-31)

Move layout-only types out of the data layer so Renderer (renderer → data is the A2
layer skip) only depends on text.

**Current state (verified):**
- `src/data/ChapterProvider.h:43-67` — `Span`, `Line`, `ChapterLayout` (layout types; `ChapterData` at :69 does NOT contain them)
- `SegmentType` (:22) is shared — used by `Segment` (data) and `Span.type` (layout). **Stays in `data/ChapterProvider.h`.**
- `text/LayoutTypes.h` includes `data/ChapterProvider.h` for `SegmentType` — legal (text → data). **Data must NOT include text.**

- [x] Create `src/text/LayoutTypes.h` — move `Span`, `Line`, `ChapterLayout` into namespace `theword::text`
- [x] `data/ChapterProvider.h` — remove the 3 moved structs; keep `SegmentType`, `Segment`, `Word`, `Footnote`, `ChapterData` + interface
- [x] `document/DocumentManager.h` — :25 `data::ChapterLayout` → `text::ChapterLayout`; :58, :67 `data::Span` → `text::Span`; add LayoutTypes.h include
- [x] `document/DocumentManager.cpp` — :381 (unqualified `Span`), :451 `data::ChapterLayout` → `text::ChapterLayout` (+ `using namespace theword::text` for :175/:206/:257)
- [x] `renderer/Renderer.h` — :33, :61 `data::Span` → `text::Span`; **swap `data/ChapterProvider.h` include for `text/LayoutTypes.h`** (the A2 fix itself)
- [x] `text/LayoutEngine.h` — :28, :39, :60, :66, :67 → native `text::` types (no qualification needed inside `theword::text`)
- [x] `text/LayoutEngine.cpp` — no changes needed (own namespace + `using theword::data` still resolves)
- [x] `ui/ReaderScreen.cpp` — :90-91 unqualified `Span` → `theword::text::Span`; `SegmentType` at :97 still `data::`
- [x] `ui/ReaderScreen.h` — :11 forward decl → `namespace theword::text { struct Span; }`
- [x] `app/App.cpp` — :237 `theword::data::Span` → `theword::text::Span` + explicit LayoutTypes.h include
- [x] `tests/test_main.cpp` — LayoutTypes.h include + `using namespace theword::text` (Span/Line/ChapterLayout tests)
- [x] Verify: Release build 0 compiler warnings; `theword_test` 78/80 (same baseline, 2 pre-existing locale failures); `-H` preprocess confirms Renderer.h → text/LayoutTypes.h → data/ChapterProvider.h (no direct renderer → data include)
- [x] GUI smoke test passed on device (2026-08-09, APK v1.9.0-alpha) — no regressions, no visual alteration

**Files touched:** new `text/LayoutTypes.h` + 9 modified.

### Feature 2: A5 — TextMeasureFn abstraction ✅ Complete (2026-07-31)

Remove raylib from the text layer so LayoutEngine is testable/portable.

**Current state (verified):**
- `text/LayoutEngine.h` includes `<raylib.h>` (:7), holds `const Font&` ×4 + 4 sizes (:44-51), ctor takes 4 fonts + 4 sizes (:20-26)
- 8 `MeasureTextEx` call sites in `LayoutEngine.cpp` (:155, :172, :230, :241, :312, :323, :346, + more)
- `LayoutHeading(..., const Font& useFont, float renderSize)` (:68) — font chosen per heading type

- [x] Define `TextMeasureFn` in `text/TextMeasure.h` — `FontKind` enum (Body/Heading/Large/Small) + `TextExtent{width,height}` + `std::function<TextExtent(FontKind, const std::string&, float)>`; **no raylib types**
- [x] `LayoutEngine` ctor: 4 `Font` refs + 4 sizes → `TextMeasureFn` + 4 sizes (sizes still owned by LayoutEngine for cache generation)
- [x] Rewire all `MeasureTextEx` sites in `LayoutEngine.cpp` through the fn with per-role `FontKind`
- [x] `LayoutHeading` now takes `FontKind kind` instead of `const Font& useFont`
- [x] Removed `<raylib.h>` include from `LayoutEngine.h/cpp` — **text/ layer is now raylib-free** (verified by grep)
- [x] `FontManager::Get(FontKind)` added (renderer → text include, legal layer edge)
- [x] `App.cpp` — measure fn built on `MeasureTextEx(fontManager_->Get(kind), ...)`; pruned now-unused `core/FontHelper.h` + `core/Theme.h` includes
- [x] Verify: Release build 0 warnings; `theword_test` 78/80 baseline
- [ ] GUI smoke test pending (no DISPLAY in this session)

**Files touched:** new `text/TextMeasure.h` + `LayoutEngine.h/cpp`, `FontManager.h/cpp`, `App.cpp`, `CMakeLists.txt`.

### Feature 3: A9.1 — selection-hop removal ✅ Complete (2026-07-31)

InputHandler re-emits SelectionEvent through App; only Highlighter listens.

**Current state (verified):**
- `App.cpp:716-737` — `OnDragStart/Update/End` emit `SelectionEvent` using `inputHandler_->GetPressStartHit()`
- `App.cpp:739-751` — `OnLongPress` emits Start (+ End on desktop for highlighted word)
- Sole listener: `Highlighter::OnSelection` (`highlighter/Highlighter.cpp:26-40`)
- `InputHandler` already owns `eventBus_` and emits `FontSizeEvent`/`ResizeEvent` itself (`InputHandler.cpp:287-299`) — precedent exists

- [x] `InputHandler` emits `SelectionEvent{Start}` at drag-start and long-press; `{Update}` at drag-update (both Dragging + LongPress paths); `{End}` at drag-end — using internal press-start hit
- [x] `suppressDragEnd_` mirrors old `longPressHandled_`: desktop long-press on highlighted word → immediate `{End}` + suppress next drag-end's `{End}` **and** its `onDragEnd` callback (radial menu already shown); reset in `ResetState()`
- [x] `App.cpp` — deleted `OnDragStart`/`OnDragUpdate`; `OnDragEnd` keeps only `ShowRadialMenu`; `OnLongPress` keeps radial-menu + desktop-highlighted check, drops both emissions; `WireInputCallbacks` trimmed
- [x] `App.h` — dropped 2 declarations + `longPressHandled_` member
- [x] Verify: Release build 0 warnings; `theword_test` 78/80 baseline
- [x] GUI smoke test passed on device (2026-08-09, APK v1.9.0-alpha) — reader rendering, scrolling, selection

**Files touched:** `input/InputHandler.h/cpp`, `app/App.cpp`, `app/App.h`.

### Feature 4: A9.2-3 — FontManager extraction ✅ Complete (2026-07-31)

App.cpp owns 5 raylib fonts, codepoints, sizes, and reload logic — extract.

**Current state (verified):**
- `App.h:48-54` — `bodyFont_`, `headingFont_`, `largeFont_`, `smallFont_`, `boldFont_`, `headingSize_`, `fontCodepoints_`; `App.h:43` — `ReloadFonts`
- `App.cpp:140` — `LoadFontCodepoints`; `:290-338` — `ReloadFonts` (LoadFontEx ×5, texture filters, swap+unload)
- `App.cpp:341-365` — FontSizeEvent handler: size clamp → ReloadFonts → `SetFontSizes` on LayoutEngine+Renderer → `InvalidateCache` + `InvalidateLayouts`
- Fonts distributed via ctor: `LayoutEngine` (:173), `Renderer` (:179), `UIManager` (:214), `ReaderScreen` (:252-257), `SettingsScreen` (:449-454), `CreditsOverlay` (:457-460), `FontDiagnostic` (:467-471)
- Unloads in dtor (:46-51); `App.cpp:408-410` ResizeEvent handler only touches `uiScale_`

- [x] Created `src/renderer/FontManager.h/cpp` — owns 5 `Font`s, codepoints, `currentFontSize_`, 4 scaled sizes, heading text size, dpi scale
- [x] `FontManager::Init(assets, savedFontSize)` — LoadFontCodepoints + ReloadSizes; dtor unloads all 5 fonts
- [x] `FontManager::ReloadSizes(float)` — clamped; same formulas/swap-unload pattern as old `ReloadFonts`; `Get(FontKind)` added for A5
- [x] `App.cpp` — font members + `ReloadFonts` deleted; all 7 consumer sites pull from `fontManager_`; dtor does `fontManager_.reset()` before `CloseWindow()` (GL context order)
- [x] **Decision (deviation from checklist): FontManager does NOT subscribe to FontSizeEvent** — App keeps orchestrating (delegates reload + reads sizes via getters). Lower risk, identical behavior; revisit when A4 lands
- [x] **Bug fix discovered**: SettingsScreen mutates `currentFontSize_` before emitting `FontSizeEvent`, so the old handler's `newSize == currentFontSize_` check always early-returned — settings font size never reloaded fonts. Now compared against `fontManager_->CurrentSize()`; `currentFontSize_` synced after reload. Settings +/- now actually applies
- [x] Verify: Release build 0 warnings; `theword_test` 78/80 baseline
- [x] GUI smoke test passed on device (2026-08-09, APK v1.9.0-alpha) — Settings font size +/- applies live; fonts render in all screens/overlays

**Files touched:** new `renderer/FontManager.h/cpp` + `CMakeLists.txt` (RENDERER_SOURCES) + `App.h/cpp`.

### Feature 5: A4 — DrawContext ✅ Complete (2026-07-31, device-verified 2026-08-09)

Screens bypass the renderer and call raylib directly (~80 `Draw*` calls across 13 files).

**Scope:** draw-side only. Input polling (`GetMousePosition`, `IsKeyPressed`, scissor…) stays
untouched — deferred to Feature 6 (input consolidation).

**Plan baked 2026-07-31 — all design decisions approved by user.**

**Current state (verified):**
- `ui/Screen.h:9` — `virtual void Draw() = 0;` (no params; screens capture `themeManager_`/fonts/`uiScale_`/`scale_`/`headingSize_` in ctor)
- Direct raylib calls per file: `components.cpp` 17, `FontDiagnostic` 12, `ChapterGridScreen` 7, `HighlightBrowserScreen` 6, `ReaderScreen` 6, `SettingsScreen` 5, `UIManager` 5, `BookListScreen` 4, `RadialMenu` 3, `Renderer` 3, `CenterMenu` 2, `App` 2
- `NavigationStack::DrawActive()` (`ui/NavigationStack.cpp`) drives draws
- **Key finding:** `core/UIScale` already carries `screenW/screenH/dpiScale/bottomInset` — DrawContext is a bundle of 3 existing refs, not new state
- 224 direct raylib calls total across 12 files (draw + input + viewport + scissor)
- `Renderer` ctor: 9 params (4 fonts + 4 sizes + contentTop + dpiScale + theme) — 4 of those now redundant with FontManager (A9.2-3)
- `Renderer::DrawFrame` sole caller is `ReaderScreen.cpp:114`; `RenderSizes` sizes are draw-time reads (no caching → safe to read live from FontManager)
- Font injection per screen (verify kinds at `App.cpp:385-410`): ReaderScreen=Heading, UIManager=Small, FontDiagnostic=all 5, others=`const Font& + float fontSize` (BookList, ChapterGrid, Settings, Credits, HighlightBrowser, CenterMenu)

**Design — new `src/renderer/DrawContext.h`:**

```cpp
namespace theword::renderer {
struct DrawContext {
    const core::ThemeManager& themeManager;   // → palette via Current()
    const FontManager& fonts;                 // → Get(FontKind), *Size() getters
    const core::UIScale& uiScale;             // screenW/H, dpiScale, dp()/vw()/vh()
    float scale;                              // dpi scale (raw)
};
}
```

**Decisions (approved):**

| # | Question | Decision |
|---|----------|----------|
| D1 | Ctor keep/remove | Remove `themeManager_`, `uiScale_`, font refs + sizes, `dpiScale_` where ctx covers them; **keep** state refs (`currentFontSize_`, `immersiveMode_`, `navStack_`, `contentTop_`, eventBus/doc/persistence deps) in ctors |
| D2 | Renderer slimming | `Renderer` ctor: 9 params → 3 (`const FontManager&`, `contentTop`, `const ThemeManager&`); reads sizes live via `fonts.Get(FontKind)` → **`SetFontSizes` dies**; App FontSizeEvent handler drops renderer call; `bodySize_`-style members deleted |
| D3 | Screen font mapping | Each screen maps old injected font → kind App actually passed; pulls `ctx.fonts.Get(kind)` in Draw |
| D4 | components helpers | First param `const DrawContext&` (palette/scale/screenW); per-helper font+fontSize params stay (helpers reused with different fonts) |
| D5 | Who builds ctx | **App, per frame** in render loop: `renderer::DrawContext ctx{*themeManager_, *fontManager_, uiScale_, scale_};` → `navStack_->DrawActive(ctx)`, `uiManager_->Draw* (ctx)`. Always fresh — no stale-size risk |
| D6 | Screen Draw() | `virtual void Draw(DrawContext& ctx)` (non-const param — extensible for clip-rect push/pop later); `HandleInput` unchanged |

**Implementation order (build + tests after each stage):**

- [x] S1: `DrawContext.h` (new) + `Screen.h` signature + `NavigationStack::DrawActive(DrawContext&)`
- [x] S2: `components.cpp/h` — 6 helpers take ctx (17 draw sites)
- [x] S3: Screens one at a time: BookListScreen (4) → CenterMenu (2) → CreditsOverlay (5) → SettingsScreen (5) → ChapterGridScreen (7) → HighlightBrowserScreen (6) → FontDiagnostic (12)
- [x] S3b: ReaderScreen (6) + Renderer (3) — biggest, done together
- [x] S4: UIManager/RadialMenu (8) — pulls fonts/palette from ctx
- [x] S5: `App.cpp` wiring (2 direct calls): build ctx, thread through; shrink ReaderScreen/SettingsScreen/Credits/FontDiagnostic ctors; FontSizeEvent handler drops `renderer_->SetFontSizes`
- [x] S6: Full pass — grep `GetScreenWidth|GetScreenHeight` in ui/ + renderer/ (only App.cpp + Renderer internals allowed); remove `SetFontSizes` everywhere; ctor members removed from headers
- [x] Verify: Release build 0 warnings; `theword_test` 78/80; GUI smoke test passed on device (2026-08-09) — theme toggle, font size, all 9 screens + overlays, reader interactions, no regressions
- [x] Tag: `git tag -am "v1.9.0-alpha" "v1.9.0-alpha"` (2026-08-09)

**Decision refinements (deviations from plan, all within approved intent):**
- D1-refined: `themeManager_` retained in **SettingsScreen** (HandleInput needs `IsDarkMode()` for theme-row hit-testing) and the forwarding chain **ReaderScreen → CenterMenu** (they construct SettingsScreen). Dropped everywhere else. `uiScale_` retained everywhere (HandleInput layout math). Fonts/sizes dropped from ctor where Draw-only: SettingsScreen, CreditsOverlay (font only — `fontSize_` kept for HandleInput contentH math), ChapterGridScreen, FontDiagnostic (all 5 fonts), UIManager. Kept where HandleInput/forwarding needs them: BookListScreen, CenterMenu, ReaderScreen, HighlightBrowserScreen (ScrollEvent → RebuildLayouts uses font).
- D2-refined: Renderer went further than planned — ctor **9 params → 1** (`Renderer(float contentTop)`), `DrawFrame(const DrawContext&, ...)`/`DrawSpan(ctx,...)`/`DrawScrollbar(ctx,...)`, all deps from ctx at call time. `SetFontSizes`/`GetFontSize`/`GetContentTop` deleted from Renderer (unused elsewhere); App FontSizeEvent handler only touches LayoutEngine now.
- D4-refined: `DrawPanel` keeps no ctx (no palette/scale deps). `DrawRadialMenu()` stays ctx-free — RadialMenu draws itself with no palette/screen deps (geometry precomputed at Show from `dpiScale_`).
- A4 is draw-side only, as scoped: remaining `GetScreenWidth/Height`/mouse calls in `ui/` are all in HandleInput/ScrollEvent/HandleBottomBarClick paths — deferred to Feature 6 (input consolidation).

**Files touched (14):** new `renderer/DrawContext.h` + `renderer/Renderer.h/cpp`, `renderer/UIManager.h/cpp`, `renderer/RadialMenu.h/cpp`, `ui/Screen.h`, `ui/NavigationStack.h/cpp`, `ui/components.h/cpp`, 9 screens, `app/App.cpp/h`.

### Feature 6: Code Quality Assurance (fresh full audit) ✅ Complete (as v1.9.1-alpha)

> Status: COMPLETE — v1.9.1-alpha (2026-08-09). Full-scope audit + fix.
> User decisions: fix everything found; full FrameState input consolidation;
> new LayoutEngine tests (A5 payoff).

**Phase 1 — Evidence sweep (done):**
- Baseline green: 0 warnings, 78/80 tests (2 locale-only failures)
- Layer compliance verify: only legal cross-layer includes remain
  (renderer→text/highlight, ui→renderer); no renderer/input/ui → data
- Dead code: `KeyEvent` (Events.h:33) — 0 emitters, 0 subscribers
- `DialogEvent` — self-subscribe only (InputHandler.cpp:27)
- No raw new/delete; no naming violations; `static` helpers in 5 files
  (convention prefers anonymous namespace — minor, Phase 6)
- `LoadFontCodepointsFromData` used internally (not dead)
- Input inventory (D1): GetScreenWidth/H ×8 files, GetMousePosition ×8,
  IsMouseButton* ×7, IsKeyPressed ×7, GetCharPressed ×1, GetMouseWheelMove ×3
- components.cpp hovers poll mouse at draw time (needs ctx.input)
- BookListScreen double-drains GetCharPressed (151+172) — latent bug, fixed by snapshot

**Phase 2 — Input consolidation design (approved 2026-08-09):**
- New `core/InputFrame.h` (portable, no raylib): mouseX/Y, wheel, leftPressed/Down/
  Released, rightPressed, keysPressed (drained), textInput (drained chars),
  `KeyPressed(int)` helper
- `InputHandler::BeginFrame()` — ONLY poller (input layer = authority): captures
  snapshot; `Poll()` consumes the same frame (consistency win); touch/held-key
  paths (GetTouchPosition, IsKeyDown arrows) stay as legal input-layer calls
- `DrawContext` gains `const InputFrame& input` + `PushClipRect/PopClipRect`
  (new DrawContext.cpp; replaces BeginScissorMode/EndScissorMode ×3 screens)
- `Screen::HandleInput(const DrawContext&, float)` + NavigationStack pass-through
- `TapDetector` → float x/y (drops raylib Vector2)
- components hover/click → ctx.input (CheckCollisionPointRec → inline rect test)
- Screens ×9: replace all polling; viewport reads → ctx.uiScale
- **Scope note**: Font/Color/Rectangle stay as draw-boundary vocabulary in
  components (A4 D4 decision; FontKind has no Bold — FontDiagnostic needs it).
  Zero-polling gate is about input+scissor, not resource types.

**Implementation order (build+test after each stage):**
- [x] S1: core/InputFrame.h (mouseX/Y, wheel, left/right edge booleans, keysPressed,
      textInput, touchActive, ctrlDown, KeyPressed helper)
- [x] S2: InputHandler BeginFrame/Frame/Poll rewire (single poll point; double-drain
      bug in BookListScreen fixed by snapshot semantics)
- [x] S3: DrawContext member + clip/cursor helpers + CMakeLists (new DrawContext.cpp)
- [x] S4: Screen.h + NavigationStack signatures
- [x] S5: TapDetector → floats
- [x] S6: components.cpp → ctx.input (InRect + public PointInRect helpers)
- [x] S7: screens ×9 (BookList→CenterMenu→Settings→Credits→ChapterGrid→
        HighlightBrowser→FontDiagnostic→ReaderScreen; UIManager/RadialMenu left as
        renderer-boundary polling per A4 D4 — same as held-key scroll in InputHandler)
- [x] S8: App loop wiring (BeginFrame + shared ctx + frame-based HandleShortcuts/
      footnote popup; ctrlDown added for Ctrl+C path)
- [x] S9: grep verify (zero polling in ui/; App.cpp only window-size capture) +
      build (0 warnings in our code; 14 in vendored raylib/doctest) + tests
      (78/78 non-locale; 80 total with the 2 known pt_BR failures)

After the refactor, run the same 8-step audit as v1.8 across all `src/`:

- [x] `-Wall -Wextra -Wpedantic` clean build (0 warnings)
- [x] Dead code / unused members removal (OpenURL, GetClipboard, onDragStart/onDragUpdate, ResizeEvent::prevScrollY, KeyEvent, DialogEvent, HasChapter chain)
- [x] Include pruning + forward declarations (esp. after LayoutTypes/FontManager/DrawContext land)
- [x] Const-correctness pass on refactored surfaces
- [x] Input consolidation (deferred from v1.8): TapDetector + mouse/press boilerplate across screens
- [x] Naming consistency (PascalCase convention)
- [x] Optional/pointer error-handling check on new APIs (TextMeasureFn, FontManager getters)

## Planned: v1.10.0-alpha — Animations & UI Polish + Web Deployment

- **Web Deployment**: verify WASM build in browser (never tested); add `scripts/serve-web.sh` static server with correct MIME types; IDBFS stays backlogged
- **Animations & UI Polish**: animation/transition pass across screens and overlays; polish items TBD

See `the-word-docs/04-planning/Release Plan.md` and `the-word-docs/04-planning/Roadmap.md`.

## v1.9.1-alpha — Remaining Phase 3–7 checklist

**Phase 3 (App slimming) — done:**
- [x] D6 dedup: `HandleRadialMenuResult` (copy/delete/highlight paths share one handler)
- [x] Selection statics `AssembleSelectedText`/`FindVerseRange` → Highlighter public statics
- [x] `Init()` split: InitWindow/InitDataServices/InitInput/InitNavigation/ApplyPreferences (+ `platInfo_`, `contentTop_` members)
- [x] Shortcuts → `InputHandler::onShortcut` (S/A/I/D + Ctrl+C drain in Poll after FSM; App.cpp 691→617 lines)
- [x] `key::C`/`key::D` constants in core/Config.h (KEY_C_ANDROID=31, KEY_D_ANDROID=32) — fixes latent Android raw-key bug
- [x] Live smoke (DISPLAY=:0): GEN load + S/D/A/I/Escape verified

**Phase 4 (event governance) — done:**
- [x] `KeyEvent` deleted (0 emitters/subscribers)
- [x] `DialogEvent` self-subscribe loop deleted (emitted+subscribed only by InputHandler behind unreachable `dialogActive_`); removed `IsDialogActive()`/`dialogActive_`
- [x] `hasUiOverlay` = radial + navstack only
- [x] New `the-word-docs/02-architecture/Event Bus.md` (9-event matrix + governance); 00-INDEX + Architecture Overview updated

**Phase 5 (data layer) — done:**
- [x] Zero-caller `ChapterProvider::HasChapter` deleted: interface + USFMParser/BibleClient (each had `cachedHasChapter` maps) + CompositeProvider + StubChapterProvider
- [x] 4 test cases converted to LoadChapter semantics
- [x] Data Structures.md / Data Source Architecture.md / USFM Parser.md updated
- [x] Note: BibleClient HasChapter did a full network fetch per boolean check — deletion is a correctness win

**Phase 6 (tests) — done (87 cases, 320 assertions; 85 pass, 2 known pt_BR locale failures):**
- [x] 6 LayoutEngine cases with deterministic FakeMeasure (char width ×10): wraps+verses, single line, HitTestLine, cache invalidation + generation, poetry indent, ResizeEvent invalidation (initial line-count assertion was wrong — engine was right, rewrote to span/wrap invariants)
- [x] 2 Highlighter cases: FindVerseRange + AssembleSelectedText (incl. Gênesis citation, bounds cases)

## v1.9.1-alpha — Phase 7: Clean-code sweep + Release

- [x] Dead code sweep: OpenURL + GetClipboard (Platform.cpp incl. Android JNI blocks, Platform.h) — verified 0 grep hits
- [x] `onDragStart`/`onDragUpdate` deleted (InputHandler callbacks + emit sites; SelectionEvent flow untouched)
- [x] `ResizeEvent::prevScrollY` deleted (had 0 consumers; emit site updated)
- [x] Docs sync: Progress Tracking.md + memory/State.md + Active.md
- [x] Version bump to 1.9.1-alpha (CMakeLists + regenerated Version.h, reconfigured)
- [x] Build + full test run (0 warnings ours, 87 cases / 85 pass / 2 known pt_BR locale)
- [x] Live smoke (DISPLAY=:0): GEN.1 load, s/d/a/i/Escape/g/q, EXO.1 navigation — all crash-free, no ASan, no regressions
- [x] APK build → `dist/theword-arm64-v8a-v1.9.1-alpha.apk` (19 MB, signed)
- [ ] Device sign-off (user installs APK, confirms no regressions)
- [ ] Annotated tag `v1.9.1-alpha`
