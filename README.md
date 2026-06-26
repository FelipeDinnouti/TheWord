# TheWord

A minimalist Bible study application built with Raylib. Read, highlight, and navigate Bible text with a clean, distraction-free interface.

> **Note:** Desktop proof-of-concept targeting a mobile-first viewport (450×800px). Android NDK build available. All documentation is in `the-word-docs/`.

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

## Prerequisites

### Linux
```bash
sudo apt install build-essential cmake git libcurl4-openssl-dev
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
```

---

## Quick Start

### 1. Get an API Key (optional)

1. Sign up at [platform.youversion.com](https://platform.youversion.com)
2. Create an app and copy your **App Key**
3. Create a `.env` file:
```env
YVP_APP_KEY=your_app_key_here
```
> Without an API key, the app uses offline USFM files (full Bible, no fallback text).

### 2. Build & Run
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
./build/theword
```

### 3. Run Tests
```bash
./build/theword_test
```

---

## Controls (Desktop)

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

## Documentation

All design docs, architecture, and planning are in **`the-word-docs/`**.

Start at `the-word-docs/00-INDEX.md` for a guided tour.

---

## Build Platforms

| Platform | Build Command | Output |
|----------|--------------|--------|
| Linux | `cmake -B build -G "Unix Makefiles" && cmake --build build` | `build/theword` |
| Windows (MSYS2) | `cmake -B build -G "MinGW Makefiles" && cmake --build build` | `build/theword.exe` |
| Android | `./scripts/build-android.sh` | `theword.apk` |
| WebAssembly | See `the-word-docs/06-ops/Build Guide.md` | `build-wasm/theword.html` |

---

## License

This project is for educational purposes. Bible text is copyrighted by respective publishers.
