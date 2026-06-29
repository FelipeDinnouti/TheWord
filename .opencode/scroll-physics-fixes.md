# Scroll Physics Fixes (June 2026)

## Diagnostics

Three root causes of "stiff, unnatural" feel:

| Issue | Current | Platform norm | Fix |
|-------|---------|---------------|-----|
| Lerp fights touch drag | `SMOOTH_SPEED=20` filters ALL scroll events, adding ~50ms lag during active drag. Content swims behind finger. | iOS/Android apply touch deltas immediately during drag (no lerp). Lerp only for programmatic scroll. | Bypass lerp for touch drag — apply delta directly to `scrollY` |
| Momentum decays too fast | τ = 325ms. After 1s only 4.6% remains — 3x faster than iOS. | iOS normal τ ≈ 500ms (`0.998^1000` per ms). After 1s, 13.5% remains. | 0.325 → **0.500** |
| No touch slop | 0px — any micro-movement immediately scrolls. | Android: 8dp (~16px). iOS: ~5pt implicit. | Add **10px** slop accumulator |
| Velocity cap too low | `MAX_MOMENTUM_VELOCITY = 1200`. Fast flicks clipped. | iOS: ~3000. Flutter: 4000. Android: 8000+. | 1200 → **3500** |
| EMA too slow | α = 0.08 over 5 frames (~200ms total lag) | ~0.2 over 3 frames (~50ms) | α = 0.08 → **0.18**, history 5 → **3** |

## Platform Reference Values

| Parameter | iOS (normal) | Android | Flutter |
|-----------|-------------|---------|---------|
| Touch slop | ~5pt | 8dp (16px@2x) | ~18px |
| Drag ratio | 1:1 | 1:1 | 1:1 |
| Momentum τ | 500ms | ~350-400ms (spline) | ~350-500ms |
| Min velocity | ~60 pt/s | 50 dp/s (~100 px/s) | 50 px/s |
| Max velocity | ~3000 pt/s | 8000 dp/s (~16000 px/s) | 4000 px/s |
| Deceleration model | `v(t) = v₀ · 0.998^(t·ms)` | Spline curve | Clamping/Bouncing sim |

## Changes

### 1. InputHandler.h — constants + members

```cpp
float scrollVelocity;
float touchVelocity;
float deltaHistory[3] = {};          // WAS 5
int deltaHistoryIdx = 0;
float slopAccumulator = 0.0f;        // NEW

static constexpr int DELTA_HISTORY_SIZE = 3;        // WAS 5
static constexpr float SCROLL_SENSITIVITY = 40.0f;
static constexpr float KEYBOARD_SCROLL_FACTOR = 0.3f;
static constexpr float VELOCITY_ALPHA = 0.18f;      // WAS 0.08
static constexpr float TOUCH_SLOP = 10.0f;           // NEW
static constexpr float MOMENTUM_TIME_CONSTANT = 0.500f;  // WAS 0.325
static constexpr float MIN_VELOCITY = 50.0f;         // WAS 10.0
static constexpr float MAX_MOMENTUM_VELOCITY = 3500.0f; // WAS 1200
```

### 2. InputHandler.cpp — HandleTouchScroll

- First touch frame: reset slopAccumulator
- Accumulate deltas, only emit once past TOUCH_SLOP
- Once past slop, emit accumulated + subsequent deltas directly
- Bypass EMA for 3-frame history (was 5)
- Remove MIN_FLING_VELOCITY check (was already removed)

### 3. Events.h — ScrollEvent

Add bool to allow DocumentManager to distinguish touch drag from momentum/programmatic:

```cpp
struct ScrollEvent { float delta; bool direct = false; };
```

Touch drag emits `ScrollEvent{-delta, true}`. Momentum emits `ScrollEvent{scrollVelocity * dt, false}`. Mouse wheel and keyboard also emit `false`.

### 4. DocumentManager.cpp — OnScroll

When `direct`, apply to `scrollY` immediately and sync `targetScrollY`:

```cpp
void DocumentManager::OnScroll(const theword::event::ScrollEvent& e) {
    float maxScroll = GetTotalHeight() - viewportHeight;
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    if (e.direct) {
        scrollY += e.delta;
        if (scrollY < 0.0f) scrollY = 0.0f;
        if (scrollY > maxScroll) scrollY = maxScroll;
        targetScrollY = scrollY;
    } else {
        float newTarget = targetScrollY + e.delta;
        if (newTarget < 0.0f) newTarget = 0.0f;
        if (newTarget > maxScroll) newTarget = maxScroll;
        targetScrollY = newTarget;
    }

    float absDelta = std::abs(e.delta);
    avgScrollSpeed_ = avgScrollSpeed_ * 0.9f + absDelta * 0.1f;
}
```

## Files Touched

| File | Changes |
|------|---------|
| `src/event/Events.h` | Add `bool direct` to `ScrollEvent` |
| `src/input/InputHandler.h` | Add TOUCH_SLOP, slopAccumulator, tune constants |
| `src/input/InputHandler.cpp` | Implement slop + direct flag in HandleTouchScroll |
| `src/document/DocumentManager.cpp` | Direct path in OnScroll bypasses lerp |
