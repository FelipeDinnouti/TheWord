# TheWord

A minimalist Bible study application built with Raylib. Read and highlight Bible text with a clean, distraction-free interface.

> **Note:** This is a proof-of-concept targeting mobile-first desktop viewing (450x800px). Full Bible navigation and infinite scroll are in development.

---

## Features

- **Clean text rendering** with word wrapping
- **Live Bible text** via YouVersion API (BSB translation)
- **Per-word highlighting** (in development)
- **Smooth scrolling** with mouse wheel or keyboard
- **Mobile-first aspect ratio** for previewing mobile experience on desktop

---

## Prerequisites

### Linux

```bash
sudo apt install build-essential cmake git
sudo apt install libcurl4-openssl-dev
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
3. Create a `.env` file in the project root:

```env
YVP_APP_KEY=your_app_key_here
```

> Without an API key, the app displays a fallback verse (John 3:16) so you can test the layout engine.

### 2. Build

```bash
# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel

# Windows (MSYS2 MINGW64)
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### 3. Run

```bash
# Linux
./build/theword

# Windows
./build/theword.exe
```

---

## Controls

| Input | Action |
|-------|--------|
| Mouse wheel | Scroll up/down |
| W / Arrow Up | Scroll up |
| S / Arrow Down | Scroll down |

---

## Project Structure

```
TheWord/
├── CMakeLists.txt          # Build configuration
├── AGENTS.md               # Developer guide (for contributors)
├── the-word-docs/          # Design documents
│   ├── DEVELOPMENT_PLAN.md # Full development roadmap
│   ├── SPEC.md             # Project specification
│   └── ai-docs/            # Implementation notes
├── src/                    # Source code
│   ├── main.cpp            # Application entry point
│   ├── core/               # Config, HTTP client, .env loader
│   ├── data/               # Bible API client, data structures
│   └── text/               # Layout engine (word wrapping)
├── shaders/                # Raylib shaders
└── .env                    # API key (gitignored)
```

---

## Development Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1. Foundation | ✅ Done | Build system, modular structure, window setup |
| 2. Text Layout Engine | ✅ Done | Word wrapping, line breaking, API integration |
| 3. Document Manager | 🔜 Next | Infinite scroll with anchor-fixed behavior |
| 4. Highlighting | ⬜ Todo | Per-word selection and highlight rendering |
| 5. USFM Parser | ⬜ Todo | Local Bible file parsing |
| 6. SQLite Persistence | ⬜ Todo | Save highlights to database |
| 7. UI Layer | ⬜ Todo | Polish rendering and controls |
| 8. Android | ⬜ Todo | Mobile build preparation |

---

## API Reference

The app uses the **YouVersion Platform API**.

| Endpoint | Description |
|----------|-------------|
| `GET /bibles?language_ranges[]=en` | List available Bible versions |
| `GET /bibles/3034/passages/JHN.3.16?format=text` | Fetch verse text |

**Required header:** `X-YVP-App-Key: <your_key>`

**Bible versions:**
- **BSB (3034)** — Works with standard app key ✅
- **NIV (111)** — Requires special license ❌

---

## Configuration

Edit `src/core/Config.h` to customize:

```cpp
constexpr int WINDOW_WIDTH = 450;        // Window width
constexpr int WINDOW_HEIGHT = 800;       // Window height
constexpr float FONT_SIZE = 20.0f;       // Text size
constexpr float LINE_SPACING = 1.5f;     // Line height multiplier
constexpr int DEFAULT_BIBLE_ID = 3034;   // BSB translation
constexpr const char* DEFAULT_VERSE = "JHN.3.16";  // Starting verse
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "CMAKE_CXX_COMPILE_OBJECT not set" | Add `CXX` to project: `project(theword C CXX)` |
| "libcurl not found" | Install `libcurl4-openssl-dev` |
| API returns "Access denied" | Use Bible ID 3034 (BSB), not 111 (NIV) |
| API key not working | Check `.env` file exists and `YVP_APP_KEY` is set |
| New .cpp files not compiled | Delete `build/` folder and reconfigure CMake |

---

## License

This project is for educational purposes. Bible text is copyrighted by respective publishers. See YouVersion API for attribution requirements.

---

## Resources

- [YouVersion Developer Portal](https://developers.youversion.com)
- [Raylib Documentation](https://raylib.com)
- [USFM Format Reference](https://paratext.org/usfm/)