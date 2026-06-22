# TheWord

A minimalist Bible study application built with Raylib. Read and highlight Bible text with a clean, distraction-free interface.

> **Note:** Desktop proof-of-concept targeting a mobile-first viewport (450×800px). All documentation is in `the-word-docs/`.

---

## Features

- Clean text rendering with word wrapping
- Live Bible text via YouVersion API (BSB translation)
- Per-word highlighting (in development)
- Smooth scrolling with mouse wheel or keyboard
- Mobile-first aspect ratio

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

### 1. Get an API Key

1. Sign up at [platform.youversion.com](https://platform.youversion.com)
2. Create an app and copy your **App Key**
3. Create a `.env` file:
```env
YVP_APP_KEY=your_app_key_here
```
> Without an API key, the app shows John 3:16-18 fallback text.

### 2. Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
```

### 3. Run

```bash
./build/theword
```

---

## Controls

| Input | Action |
|-------|--------|
| Mouse wheel | Scroll up/down |
| W / Arrow Up | Scroll up |
| S / Arrow Down | Scroll down |

---

## Documentation

All design docs, architecture, and planning are in **`the-word-docs/`**.

Start at `the-word-docs/00-INDEX.md` for a guided tour.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "CMAKE_CXX_COMPILE_OBJECT not set" | Add `CXX` to `project(theword C CXX)` |
| New .cpp files not compiled | Delete `build/` folder and reconfigure |
| libcurl not found | Install `libcurl4-openssl-dev` |
| API returns "Access denied" | Use Bible ID 3034 (BSB), not 111 (NIV) |

---

## License

This project is for educational purposes. Bible text is copyrighted by respective publishers.
