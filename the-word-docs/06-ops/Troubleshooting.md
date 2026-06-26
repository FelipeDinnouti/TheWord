# Troubleshooting

> Status: Updated for all platforms | Last Updated: 2026-06-26

## Linux Desktop

| Issue | Solution |
|-------|----------|
| `CMAKE_CXX_COMPILE_OBJECT not set` | Add `CXX` to project languages: `project(theword C CXX)` |
| New .cpp files not compiled | Delete `build/` folder and reconfigure CMake |
| libcurl not found | Install `libcurl4-openssl-dev`, ensure `find_package(CURL)` comes after raylib in CMakeLists.txt |
| API returns "Access denied" | Use Bible ID 3034 (BSB) instead of 111 (NIV) |
| API key not working | Verify `.env` file exists and `YVP_APP_KEY` is set correctly |
| `libraylib.so: cannot open shared object file` | Run `sudo ldconfig` or set `LD_LIBRARY_PATH` |
| Raylib not found by CMake | Use `-DCMAKE_PREFIX_PATH=/usr/local` or delete `build/` and reconfigure |
| Missing OpenGL references | Install `libgl1-mesa-dev libx11-dev` |
| Assets not found at runtime | Build dir must have `assets/` and `shaders/` copied in (done by `file(COPY)` in CMakeLists.txt) |

## CMake Generator Issues

On Linux, always use `-G "Unix Makefiles"`. The default or Ninja generators may fail with Raylib's FetchContent configuration.

## Windows

| Issue | Solution |
|-------|----------|
| MSYS2: compiler not found | Open **MSYS2 MINGW64** terminal, not MSYS or UCRT64 |
| Raylib not found via pacman | Run `pacman -Syu` to update, then `pacman -S mingw-w64-x86_64-raylib` |
| `opengl32` link error | Ensure MinGW packages are up to date; Raylib provides OpenGL |
| `.env` not read | Place `.env` in the same directory as `theword.exe` (project root) |

## Android

| Issue | Solution |
|-------|----------|
| NDK toolchain not found | Set `ANDROID_NDK` env var or edit `scripts/build-android.sh` |
| `android_native_app_glue.c` not found | Pass `-DANDROID_NDK=...` to CMake so raylib can find it |
| APK installs but crashes on launch | Check `adb logcat` for `TheWord` tag; ensure `libtheword.so` is in APK |
| Fonts not rendering | `AAssetManager` paths are relative to APK assets — no `assets/` prefix. Config.h handles this via `#ifdef __ANDROID__` |
| Keyboard input not working | Android keycodes are raw AKEYCODE values, not raylib KEY_* constants. Apply `patches/raylib-android-keycodes.patch` to raylib |
| Back button doesn't dismiss dialogs | Same keycode patch needed; `Config.h` has fallback `key::ESCAPE` mapping |
| Touch scroll feels wrong | Touch input is behind `#ifdef __EMSCRIPTEN__`; Android uses desktop mouse path — being addressed in Phase 4 |
| Database path wrong | `main.cpp` uses `/data/data/com.theword.app/app_storage/` — verify app package name matches manifest |
| `javac` not found for Java compilation | Install JDK 11+: `sudo apt install openjdk-11-jdk` (the script falls back gracefully to NativeActivity-only) |
| `d8` not found for `.class` → `.dex` | Install Android SDK build-tools 34.0.0 via sdkmanager: `sdkmanager "build-tools;34.0.0"` |
| APK packaging fails with "aapt: command not found" | Ensure `ANDROID_SDK` is set and `$ANDROID_SDK/build-tools/34.0.0/` exists |
| Debug keystore error during signing | Create it manually: `keytool -genkey -v -keystore ~/.android/debug.keystore -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -storepass android -keypass android -dname "CN=, OU=, O=, L=, S=, C="` |

## WebAssembly

| Issue | Solution |
|-------|----------|
| Emscripten toolchain not found | Run `source emsdk_env.sh` before CMake configure |
| `theword.html` loads but canvas blank | Serve with `python3 -m http.server` (or any static server); `file://` protocol won't load `.data` |
| Browser console: "Failed to load asset file" | Assets are preloaded via `--preload-file assets` in link flags; ensure `build-wasm/assets/` exists |
| Network API calls fail | WASM uses `-sFETCH=1` flag; check browser console for CORS errors |
| Highlights not persisting | IDBFS not yet mounted (Phase 4); highlights are session-only in WASM for now |

## macOS

Not currently supported. Raylib works on macOS, but the CMake build has not been tested on Apple platforms.
