# Android Verification & Fix Plan

> Generated from full codebase audit. Organized by priority.

## Blocker (must fix to launch)

### 1. Touch input on Android
- `InputHandler.cpp:69-77` uses `#else` (desktop mouse path) for Android
- Touch gestures (scroll, tap, pinch) behind `#ifdef __EMSCRIPTEN__` only
- **Fix**: Add `#elif defined(__ANDROID__)` using same touch paths
- **Files**: `src/input/InputHandler.cpp`

### 2. Back button → Escape / dialog dismiss
- `rcore_android.c:1040-1043` eats AKEYCODE_BACK (return 1)
- All keyboard input broken on Android because raylib stores raw Android keycodes (AKEYCODE_A=29) but `IsKeyPressed(KEY_A=65)` checks raylib keycodes
- **Fix**: Patch raylib's `AndroidInputCallback()` to translate Android keycodes to raylib keycodes (or check raw keycodes)
- **Files**: raylib `rcore_android.c` (via build script patch), `src/input/InputHandler.cpp`

## High (app launches but broken UX)

### 3. Resolution independence
- Config.h hardcodes WINDOW_WIDTH=450, WINDOW_HEIGHT=800
- On Android, use actual screen size: `GetScreenWidth()/GetScreenHeight()`
- Also affects splash screen centering, contentWidth calculation, viewportHeight
- **Fix**: Android branch in main.cpp for initial window size
- **Files**: `src/main.cpp`, `src/core/Config.h`

### 4. Font loading verification
- `LoadFontEx()` uses raylib's `InitAssetManager()` → AAssetManager on Android
- Paths in Config.h use `fonts/...` (no `assets/` prefix, correct for AAssetManager)
- **Fix**: Verify fonts render; if not, extract TTFs to internal storage on first launch
- **Files**: `src/main.cpp`

### 5. Lifecycle save/restore
- No scroll position save on pause/resume
- Raylib handles EGL surface in AndroidCommandCallback but our state is lost
- **Fix**: Save scroll position/font size/highlights to SQLite on `APP_CMD_PAUSE`/`APP_CMD_STOP`
- **Files**: `src/main.cpp`

### 6. Soft keyboard for go-to dialog
- `GetCharPressed()` reads raylib's char queue (may work if Android delivers key events)
- IME must be explicitly shown via JNI when go-to dialog opens
- **Fix**: Add JNI call to `showSoftInput()` in `UIManager`
- **Files**: `src/renderer/UIManager.h/cpp`

## Medium (polish)

### 7. Immersive mode
- Hide status bar / navigation bar for full-screen reading
- `AManageActivity_setWindowFlags(AWINDOW_FLAG_FULLSCREEN)` called by raylib at init
- **Fix**: Add `SYSTEM_UI_FLAG_IMMERSIVE | SYSTEM_UI_FLAG_HIDE_NAVIGATION` via JNI
- **Files**: `src/main.cpp`

### 8. Orientation handling
- `APP_CMD_CONFIG_CHANGED` currently no-op in raylib
- Manifest allows rotation; we need to verify re-layout on orientation change
- **Fix**: Already handled by `InputHandler::handleWindowResize()` - verify correctness
- **Files**: `src/input/InputHandler.cpp` (verify)

### 9. Splash screen
- Text-only splash drawn after InitWindow but before font loading
- On Android, there's a visible delay - add native splash via manifest or Java
- **Fix**: Add `<meta-data android:name="android.app.splash_screen">` or brief Java Activity
- **Files**: `AndroidManifest.xml`

## Low (future)

### 10. Multi-ABI APK
- Package both `lib/arm64-v8a/libtheword.so` and `lib/x86_64/libtheword.so`
- **Files**: `scripts/build-android.sh`

### 11. Release signing
- Production keystore for Play Store distribution
- **Files**: `scripts/build-android.sh`

### 12. Deep linking
- Intent filter for `theword://{book}.{chapter}` scheme
- **Files**: `AndroidManifest.xml`, `src/main.cpp`

### 13. Network status
- Check `ConnectivityManager` before API calls
- **Files**: `src/core/AndroidClient.cpp`
