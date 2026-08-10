# Event Bus

> Message bus reference: every event, its emitters, its subscribers, and the
> governance rules that keep the graph acyclic. See
> [Architecture Overview](Architecture Overview.md) for the module graph.

## Principles

1. **Plain data payloads only.** Every event is a small struct in
   `src/event/Events.h` — no methods, no business logic.
2. **EventBus carries no logic.** It only dispatches: emitter → subscribers
   (synchronous, in order of subscription).
3. **One-way flow.** Emitters produce; subscribers react. A subscriber that
   re-emits the same event type is forbidden (see history).
4. **Input is not an event channel.** Since v1.9.1, raw input is polled once
   per frame by `InputHandler` into an `InputFrame` snapshot and consumed by
   screens/App through the shared `DrawContext` / frame reads. Keypresses that
   open screens travel through the `onShortcut` callback, not the bus.
   Bus events remain the channel for *semantic* actions (scroll, selection,
   navigation, settings).

## Event Matrix (v1.9.1)

| Event | Emitters | Subscribers | Payload notes |
|---|---|---|---|
| `ScrollEvent` | InputHandler (wheel / keyboard / touch, momentum) | DocumentManager, ReaderScreen, BookListScreen, HighlightBrowserScreen, ChapterGridScreen | `delta`, `direct`, `velocity` |
| `SelectionEvent` | InputHandler (drag FSM) | Highlighter | Start / Update / End / Cancel |
| `ResizeEvent` | InputHandler (window resize) | DocumentManager, LayoutEngine, App | `width`, `height`, `prevScrollY` |
| `FontSizeEvent` | SettingsScreen (buttons), InputHandler (pinch) | DocumentManager, App | `newSize` or `delta` |
| `BibleVersionSwitchEvent` | SettingsScreen | App | `bibleId` |
| `ThemeToggleEvent` | SettingsScreen | App | empty |
| `NavigateEvent` | ReaderScreen (arrow keys, bottom bar) | App | `chapterRef` |
| `NavigateToHighlightEvent` | HighlightBrowserScreen | ReaderScreen | `chapterRef`, `wordId` |
| `ChapterLoadedEvent` | DocumentManager | App | `chapterRef` |

## Governance Rules

- **Add an event only when a semantic action crosses module boundaries.**
  Screen-internal UI state or per-frame input should use the InputFrame /
  DrawContext path instead.
- **One emitter per event type** unless the semantics are truly identical
  (e.g. `FontSizeEvent` from both pinch and settings).
- **Never subscribe to your own event type.** Dialogs/screens that need to
  react to keypresses do it directly in `HandleInput`, not via a re-emitted
  loop.
- **Keep the matrix in this file in sync** when adding or removing events.

## History

- **1.8.0**: `DialogEvent` + `KeyEvent` existed. `DialogEvent` was emitted and
  subscribed *only* by InputHandler behind a `dialogActive_` flag that no other
  code could ever set — a pure self-loop with an unreachable guard branch.
- **1.9.1 audit**: both structs deleted; the dead dialog key block removed from
  `InputHandler::Poll`; shortcut keys (S/A/I/D, Ctrl+C) moved to the
  `onShortcut` callback so screens can consume their keys first without
  double-fire risk.