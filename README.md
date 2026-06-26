# TheWord

A minimalist Bible study application built with Raylib. Read, highlight, and navigate Bible text with a clean, distraction-free interface.

> **Note:** Desktop proof-of-concept targeting a mobile-first viewport (450×800px). Android NDK and WebAssembly builds available.

---

## Features

- **Rich text rendering** — section headings, poetry indentation, paragraph spacing
- **Dual Bible source** — YouVersion API (online) + USFM files (offline Bíblia Livre)
- **All 66 books** with chapter navigation and go-to dialog with auto-complete
- **Per-word highlighting** — 5 colors (Yellow, Pink, Green, Blue, Orange), drag selection
- **Context menu** — delete or recolor highlights via long-press or right-click
- **Font size controls** — A–/A+ buttons (12–36 range), persisted across sessions
- **Bible version switching** — toggle between USFM offline and API online
- **Smooth scrolling** — momentum-based with mouse wheel, keyboard, or touch
- **Mobile-first aspect ratio** (450×800) with Android NDK support
- **SQLite persistence** — highlights and preferences survive app restart

---

## Quick Start

### Prerequisites

```bash
sudo apt install build-essential cmake git libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

libcurl is optional (needed for the online Bible API):
```bash
sudo apt install libcurl4-openssl-dev
```

### Build & Run

```bash
# Configure and build with the default preset
cmake --preset default
cmake --build --preset default

# Run
./build/theword

# Run tests
./build/theword_test
```

### Android

```bash
# Build APK for arm64 device (requires NDK + SDK — see Environment Setup)
./scripts/build-android.sh arm64-v8a
```

### Controls (Desktop)

| Input | Action |
|-------|--------|
| Mouse wheel | Scroll up/down |
| W / Arrow Up | Scroll up |
| S / Arrow Down | Scroll down |
| Click + drag | Select text to highlight |
| Right-click / long-press | Context menu (delete/recolor highlight) |
| G | Open go-to dialog |
| S | Open settings panel |
| A | Toggle about/credits overlay |
| Escape | Dismiss active dialog/menu |

---

## Build Platforms

| Platform | Command | Output |
|----------|---------|--------|
| Linux | `cmake --preset default && cmake --build --preset default` | `build/theword` |
| Linux (debug) | `cmake --preset debug && cmake --build --preset debug` | `build-debug/theword` |
| Android (arm64) | `./scripts/build-android.sh arm64-v8a` | `theword-arm64-v8a.apk` |
| Android (x86_64) | `./scripts/build-android.sh x86_64` | `theword-x86_64.apk` |
| WebAssembly | `./scripts/build-wasm.sh` | `build-wasm/theword.html` |
| Windows (cross) | `cmake --preset windows-mingw` | `build-windows/theword.exe` |

---

## Documentation

All design docs, architecture, and planning are in **`the-word-docs/`**. Start at `the-word-docs/00-INDEX.md`.

---

## License

This project is for educational purposes. Bible text is copyrighted by respective publishers.
