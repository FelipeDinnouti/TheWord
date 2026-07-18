# Active

> Current version: v1.8.0-alpha
>
> Previous: `memory/archive/2026-07-17_v1.7.0-alpha-active.md`

## Workstream: v1.8.0-alpha — Customization & Navigation

**Theme:** Code Quality Audit, ~~Architectural Analysis~~, Fuzzy Finder, Bible Version Switcher, Modular Theme System

### Implementation Order

| # | Feature | Scope | Status |
|---|---------|-------|--------|
| 0 | **Doc Fixes (release startup)** | 6 files | ✅ Complete |
| 1 | **Code Quality Audit** | All src/ (read + fix pass) | ✅ Complete (input consolidation deferred) |
| 1.5 | ~~**Architectural Analysis**~~ | All src/ (read-only analysis) | ✅ Complete (9 findings documented) |
| 2 | **Architectural Remediation (v1.8.0-alpha)** | Selected fixes from analysis | ✅ Complete (A1, A3, A6, A7, A8 done) |
| 3 | **Fuzzy Finder** | ~2 new files, ~1 modified | 🔲 Pending |
| 4 | **Bible Version Switcher** | ~5 files, curated list | 🔲 Pending |
| 5 | **Modular Theme System** | ~10 files, ThemeManager | 🔲 Pending |

### Immediate Remediation Plan (v1.8.0-alpha)

Priority-critical items from the architectural analysis:

| Finding | Action | Status |
|---------|--------|--------|
| A1 | Update architecture docs to include `ui/` module | ✅ Done |
| A3 | Move NavigationStack::HandleInput from InputHandler to App | ✅ Done |
| A6 | Replace raylib Color in Highlighter with portable SimpleColor | ✅ Done |
| A7 | Move `#ifdef _WIN32` to Platform::EnsureDirectoryExists | ✅ Done |
| A8 | Remove dead KeyEvent emission + add event governance to docs | ✅ Done |

All 5 immediate items complete. 75/77 tests pass (same 2 pre-existing locale failures).

See `memory/Architecture-Analysis.md` for full details on all 9 findings.

### Implementation Details

See `the-word-docs/04-planning/Release Plan.md` for full scope per feature.

## Doc Fixes (Completed — release startup)

- [x] Fix Agent Workflow.md — reading order + Starting a New Release procedure
- [x] Fix AGENTS.md Doc Hierarchy — add Progress Tracking.md
- [x] Archive old Active.md → `archive/2026-07-17_v1.7.0-alpha-active.md`
- [x] Update Release Plan.md, Roadmap.md, State.md for v1.8.0-alpha
- [x] Create new Active.md for v1.8.0-alpha
- [x] Archive Footnote-Plan.md → `archive/2026-07-17_footnote-plan.md`
- [x] Fix duplicate Active Decisions section in State.md

## Architectural Analysis ✅

See `memory/Architecture-Analysis.md` for the full document covering all 9 findings.

- [x] A1: Document `ui/` module gap in architecture docs
- [x] A2: Renderer → data/ChapterProvider.h layer skip (remediation planned)
- [x] A3: InputHandler → NavigationStack backward dependency (remediation planned)
- [x] A4: All screens bypass Renderer (remediation planned, large refactor)
- [x] A5: LayoutEngine depends on raylib Font (remediation planned)
- [x] A6: Highlighter depends on raylib Color (remediation planned)
- [x] A7: #ifdef _WIN32 in PersistenceManager (remediation planned)
- [x] A8: Event bus ad-hoc usage (remediation planned)
- [x] A9: App.cpp 757-line god orchestrator (remediation planned)

## Code Quality Audit ✅

- [x] Step 1: `-Wall -Wextra -Wpedantic` — already clean (0 warnings)
- [x] Step 2: Remove dead code — `GlobalId.h` (deleted), `DrawDebugSectors()` (deleted), `DrawDebugTap()` (deleted), `Platform.cpp` `(void)` casts (removed)
- [x] Step 3: Prune unused `#include`s — 26 removes across 17 files; `BibleClient.h` → forward declaration for `IHttpClient`; `UIScale.h` added to `CMakeLists.txt`
- [x] Step 4: `const`-correctness — 10 members made `const` (BibleClient::bibleId, Renderer::contentTop/dpiScale_, DocumentManager::contentTop, 6 LayoutEngine margin/gap/indent fields); `using namespace theword::core` removed where unused
- [x] Step 5: `std::optional` / pointer returns — all properly checked (0 findings in production code)
- [x] Step 6: Naming consistency — `EnvLoader::load/loadFromContent/get` → `Load/LoadFromContent/Get` (header + impl + 3 callers + tests)
- [ ] Step 7: Input pattern consolidation — **deferred** (identified 7 patterns; TapDetector boilerplate + Escape + back-button consolidation is a medium-term refactor)
- [x] Step 8: Full build + test pass — both targets build with 0 warnings, 75/77 tests pass (same 2 locale pre-existing failures), app starts and loads GEN.1
