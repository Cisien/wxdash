---
phase: 07-wind-rose-calm-and-color
verified: 2026-03-02T00:00:00Z
status: human_needed
score: 8/8 must-haves verified
human_verification:
  - test: "Calm dot is visible and correctly sized when wind speed is zero"
    expected: "A small gold center dot appears at the rose center, visibly smaller than any direction bar, with no directional needle"
    why_human: "Canvas rendering only verifiable at runtime; dot size relative to bars requires visual inspection"
  - test: "Directional needle renders when wind speed transitions above zero"
    expected: "Needle appears pointing in the correct direction with smooth rotation animation; dot disappears"
    why_human: "State transition and animation are QML runtime behaviors not verifiable statically"
  - test: "Bar colors reflect recent ~60s wind speed conditions vs lifetime average"
    expected: "Color of a bar changes within ~60 seconds after a speed change, rather than lagging the full 30-minute window"
    why_human: "Time-based behavior requires live weather data or manual simulation"
  - test: "Bar colors match Wind Speed arc gauge (Cell 5) for the same speed value"
    expected: "Green below 5 mph, yellow 5-15, orange 15-30, red 30+; identical in both rose bars and arc gauge"
    why_human: "Visual color matching between two canvas elements requires human eyes"
---

# Phase 7: Wind Rose Calm and Color Verification Report

**Phase Goal:** The compass rose gracefully handles calm/idle wind conditions and colors direction bars by recent wind speed, consistent with the wind speed gauge
**Verified:** 2026-03-02
**Status:** human_needed (all automated checks passed; visual behavior requires human confirmation)
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Calm/idle readings occupy ring slots and participate in eviction, producing no directional bin counts | VERIFIED | `recordWindSample`: `bin=-1` sentinel stored in ring; eviction guards `if (old.bin >= 0)` and new-sample update guards `if (bin >= 0)` — lines 42-66, `WeatherDataModel.cpp` |
| 2 | No bar appears in any direction bin for calm/idle samples | VERIFIED | `windRoseData()` only includes bins with `m_windBinCount[i] > 0` via the `barsCanvas` `if (!bin || bin.count <= 0) continue` guard; calm samples never increment `m_windBinCount`; test `windRose_calmSamplesNoBinCount` passes |
| 3 | Each direction bin's `windRoseData` includes `recentAvgSpeed` reflecting last ~60 s | VERIFIED | `windRoseData()` walks ring backwards collecting up to `kRecentSpeedSamples=24` per bin and emits `recentAvgSpeed`; tests `windRose_recentAvgSpeedUsesLast24Samples` and `windRose_recentAvgSpeedFewerThan24` pass |
| 4 | Existing non-calm wind sample recording and eviction behavior is preserved | VERIFIED | Tests `windRose_existingBehaviorPreserved` and `windRose_calmSamplesOccupyRingSlots` pass; all 29 tests pass including pre-existing 22 |
| 5 | Calm condition shows center dot, non-calm shows directional needle | VERIFIED (code) / UNCERTAIN (visual) | `pointerCanvas.onPaint`: branches on `root.windSpeed < 0.1`; dot drawn at `outerR * 0.04`; needle path follows for `>= 0.1` — lines 180-224, `CompassRose.qml` |
| 6 | Direction bars use `recentAvgSpeed` for their color | VERIFIED | `barsCanvas.onPaint` line 123: `root.windSpeedColorFn(bin.recentAvgSpeed !== undefined ? bin.recentAvgSpeed : bin.avgSpeed)` |
| 7 | Wind speed color thresholds are identical between `CompassRose` and `DashboardGrid` | VERIFIED | No local `windSpeedColor` function exists in `CompassRose.qml`; it calls `windSpeedColorFn` property which is injected from `DashboardGrid.windSpeedColor` — single source of truth |
| 8 | `pointerCanvas` repaints when `windSpeed` changes | VERIFIED | `property real currentWindSpeed: root.windSpeed` + `onCurrentWindSpeedChanged: requestPaint()` — line 166-167, `CompassRose.qml` |

**Score:** 8/8 truths verified (4 require additional human visual confirmation)

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/models/WeatherDataModel.h` | `WindSample` struct with `bin=-1` calm sentinel; `kRecentSpeedSamples=24` constant; `windRoseData` returns `recentAvgSpeed` per bin | VERIFIED | Line 241: `static constexpr int kRecentSpeedSamples = 24;`; `WindSample { int bin; double speed; }` at line 246; `windRoseData()` declared line 95 |
| `src/models/WeatherDataModel.cpp` | `recordWindSample` uses `bin=-1` for calm; `windRoseData` computes `recentAvgSpeed`; `loadSparklineData` guards `bin < 0` | VERIFIED | Lines 42-66 (`recordWindSample`); lines 70-98 (`windRoseData` with backward walk); lines 259-273 (`loadSparklineData` guards `if (bin >= 0)`) |
| `tests/tst_WeatherDataModel.cpp` | 7+ unit tests for calm tracking and recent-average speed | VERIFIED | 7 new test slots present (lines 501-720); all 29 tests pass (22 pre-existing + 7 new) |
| `src/qml/CompassRose.qml` | `windSpeed` property; `windSpeedColorFn` injected property; calm center dot; `recentAvgSpeed` bar coloring; no local `windSpeedColor` | VERIFIED | Line 9: `property real windSpeed: 0`; line 15: `property var windSpeedColorFn`; lines 180-187: calm dot branch; line 123: `recentAvgSpeed` coloring; no `windSpeedColor` function in file |
| `src/qml/DashboardGrid.qml` | `windSpeed` and `windSpeedColorFn` bindings in `CompassRose` instantiation | VERIFIED | Lines 162-170: `windSpeed: weatherModel.windSpeed`; `windSpeedColorFn: function(mph) { return dashboard.windSpeedColor(mph) }` |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `WeatherDataModel.cpp` | `WeatherDataModel.h` | `WindSample.bin == -1` sentinel | VERIFIED | Pattern `bin.*-1` found at lines 43, 50, 63 in `recordWindSample`; `loadSparklineData` uses `if (bin >= 0)` at line 265 and 269 |
| `WeatherDataModel.cpp` | QML `windRoseData` consumers | `recentAvgSpeed` field in `QVariantMap` | VERIFIED | Line 93: `bin[QStringLiteral("recentAvgSpeed")] = recentCount[i] > 0 ? ...` |
| `DashboardGrid.qml` | `CompassRose.qml` | `windSpeed` property binding | VERIFIED | Line 164: `windSpeed: weatherModel.windSpeed` in `CompassRose` instantiation |
| `CompassRose.qml` | `DashboardGrid.qml` | `windSpeedColorFn` function injection | VERIFIED | Line 167: `windSpeedColorFn: function(mph) { return dashboard.windSpeedColor(mph) }` — single source of truth enforced |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| WIND-01 | 07-02 | Direction needle becomes center dot when wind speed is zero | VERIFIED (code) | `pointerCanvas.onPaint` branches on `root.windSpeed < 0.1`; dot at `outerR * 0.04` radius |
| WIND-02 | 07-01 | Idle/calm readings tracked in 720-entry rolling window, no directional bar | VERIFIED | `recordWindSample` bin=-1 path; all 4 calm-related unit tests pass |
| WIND-03 | 07-01 | Bar color reflects rolling avg speed from last ~60 seconds per bin | VERIFIED | `windRoseData` computes `recentAvgSpeed` from last 24 samples per bin; bar coloring uses `recentAvgSpeed` |
| WIND-04 | 07-02 | Wind speed color thresholds in CompassRose match ArcGauge windSpeedColor | VERIFIED | No local color function in `CompassRose.qml`; `windSpeedColorFn` injected from `DashboardGrid.windSpeedColor` |

All 4 requirements declared across plans 07-01 and 07-02 are satisfied. No orphaned requirements found for Phase 7.

---

### Anti-Patterns Found

No anti-patterns detected in any of the 4 modified files:
- No TODO / FIXME / HACK / placeholder comments
- No empty implementations or stub returns
- No console.log-only handlers
- Calm dot radius (`outerR * 0.04`) is substantively smaller than minimum bar length (`outerR * 0.08`) — design intent confirmed in code

---

### Human Verification Required

The following items require a running application to confirm. All automated (structural + unit test) checks have passed.

#### 1. Calm center dot visual appearance

**Test:** Launch app during calm conditions (or momentarily set `windSpeed` to 0 in a test build). Observe the compass rose pointer area.
**Expected:** A small gold (#C8A000) dot appears at the exact center of the rose. No directional needle is visible. The dot is noticeably smaller than the shortest direction bar.
**Why human:** Canvas `arc()` drawing at `outerR * 0.04` vs bar minimum at `outerR * 0.08` is a pixel-level visual comparison; QML canvas rendering is not verifiable statically.

#### 2. Needle transition when wind picks up from calm

**Test:** With wind speed at 0, observe the calm dot. Then observe when speed rises above 0.1 mph (may require waiting for real wind or simulated data change).
**Expected:** Dot disappears and directional needle appears, pointing in the correct compass direction with smooth 400ms rotation animation.
**Why human:** State transition across the 0.1 mph threshold and the RotationAnimation behavior require runtime observation.

#### 3. Bar color responsiveness to recent speed (~60 second window)

**Test:** Observe a direction bin during sustained wind, then note the bar color. Wait for wind speed to change significantly. Monitor whether bar color updates within ~60 seconds.
**Expected:** Bar color reflects recent conditions rather than lagging the full 30-minute window. A strong wind gust lasting 60 seconds should shift bar color toward orange/red and return toward green within a minute of calming.
**Why human:** Time-based behavior requires live weather data or manual simulation over multiple minutes.

#### 4. Color consistency between compass rose bars and wind speed arc gauge

**Test:** At the same moment, compare the color of a compass rose direction bar with the arc gauge (Cell 5 Wind Speed gauge).
**Expected:** Both show the same color family for the same numeric wind speed: green below 5 mph, yellow 5-14 mph, orange 15-29 mph, red 30+ mph.
**Why human:** Visual cross-component color matching requires side-by-side human observation.

---

### Build and Test Summary

| Check | Result |
|-------|--------|
| `cmake --build build --target tst_WeatherDataModel` | ninja: no work to do (already built) |
| `./build/tests/tst_WeatherDataModel` | 29 passed, 0 failed, 0 skipped |
| `cmake --build build` (full project) | ninja: no work to do (already built) |
| New wind rose tests (7 total) | All pass: `windRose_calmSamplesNoBinCount`, `windRose_calmSamplesOccupyRingSlots`, `windRose_recentAvgSpeedField`, `windRose_recentAvgSpeedUsesLast24Samples`, `windRose_recentAvgSpeedFewerThan24`, `windRose_mixedCalmAndDirectional`, `windRose_existingBehaviorPreserved` |
| Regression tests (22 pre-existing) | All pass |

---

*Verified: 2026-03-02*
*Verifier: Claude (gsd-verifier)*
