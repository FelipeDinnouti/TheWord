# Active

> Current version: v1.6.x -- VSYNC Investigation 🔄
>
> Archive: `memory/archive/2026-07-12_radial-menu-input-refactor.md`

## Workstream: Android VSYNC Verification & Fix

**Context**: `raylib 5.0` has `eglSwapInterval()` **commented out** in `rcore_android.c:715`. TheWord compensates with a direct `eglSwapInterval(eglGetCurrentDisplay(), 1)` call in `Platform.cpp:38`, but the display handle may be invalid at that point, causing the call to silently fail. Visible screen tearing on Android.

### Part 1 -- Platform.cpp: Logging (verify VSYNC is actually being set)

Add logging after the existing `eglSwapInterval` call at `Platform.cpp:38` to capture:
- `eglGetCurrentDisplay()` return value
- `eglSwapInterval()` result (EGL_TRUE/EGL_FALSE)
- `eglGetError()` error code
- `GetMonitorRefreshRate()` for reference

### Part 2 -- Extend existing raylib patch: `rcore_android.c`

Add two new hunks to `cmake/patches/raylib-5.0-android-char-input.patch`:

**Hunk A** -- Uncomment `eglSwapInterval` in `InitGraphicsDevice()` at line 715:
```
-    //eglSwapInterval(platform.device, 1);
+    eglSwapInterval(platform.device, 1);
```

**Hunk B** -- Add `eglSwapInterval` in `contextRebindRequired` path (after `eglMakeCurrent` at line 777):
```
+    eglSwapInterval(platform.device, 1);
```

This ensures vsync is re-enabled when the EGL context is re-created after resume from background.

### Part 3 -- Build, deploy, and observe logs

1. Delete `build/` and rebuild Android to apply the updated patch
2. Deploy to device
3. Read logs via `adb logcat -s "TheWord"` to verify VSYNC log lines

### Decision Tree (based on log results)

| Log result | Next step |
|---|---|
| `eglSwapInterval` returns EGL_TRUE | VSYNC is configured -- tear is downstream (SurfaceFlinger/triple-buffer). Move to ANativeWindow buffer count clamping. |
| `eglSwapInterval` returns EGL_FALSE | Direct call failing. Try `eglGetDisplay(EGL_DEFAULT_DISPLAY)` instead of `eglGetCurrentDisplay()`. |
| Display handle is 0/null | Timing issue -- move call later in init sequence. |

### Checklist

- [x] Part 1: Add logging to Platform.cpp
- [x] Part 2: Extend raylib patch (Hunk A + Hunk B)
- [ ] Part 3: Clean build + deploy + `logcat` analysis
- [ ] If needed: ANativeWindow buffer count clamp (double-buffer force)
- [ ] If needed: fallback `eglGetDisplay(EGL_DEFAULT_DISPLAY)`
- [ ] Update State.md on completion

## Release Checklist

- [ ] Build: desktop + Android clean
- [ ] Test: >= 70/76 pass (same locale failures)
- [ ] Manual: APK verification on device
- [ ] Update `State.md`
- [ ] Tag release

## Deferred / Backlog

- Radial Menu Phase B: Press-down feedback
- Radial Menu Phase C: Show/hide animation
- Non-contiguous verse selection
- Code quality audit beyond input system
- Copy Verse (half-implemented -- more polish needed)
