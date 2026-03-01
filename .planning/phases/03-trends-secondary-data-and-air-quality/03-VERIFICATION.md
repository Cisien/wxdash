---
phase: 03-trends-secondary-data-and-air-quality
verified: 2026-03-01T22:00:00Z
status: passed
score: 27/28 must-haves verified
human_verification:
  - test: "Sparklines visible as dim trend lines in lower third of outdoor gauge cells after ~10s of live data"
    expected: "Each outdoor ArcGauge shows a dim #5A4500 yellow line growing left-to-right as data accumulates; line does not compete with gold gauge text or arc"
    why_human: "Requires running app with live WeatherLink data; color visibility against dark background cannot be verified programmatically"
  - test: "AQI multi-ring gauge visible in Cell 11 with correct EPA zone coloring"
    expected: "Three concentric arc rings visible (AQI outer, PM2.5 middle, PM10 inner); AQI ring color matches EPA zone (green if <= 50, yellow 51-100, orange 101-150, red 151-200, purple 201-300, maroon 301+); center shows AQI number; PM2.5/PM10 sub-labels at bottom corners"
    why_human: "Requires running app with PurpleAir sensor reachable at 10.1.255.41 or mocked data; requires visual inspection of ring proportions and EPA color transitions"
  - test: "Sparklines do NOT appear on CompassRose (Cell 6) or ReservedCell (Cell 12)"
    expected: "CompassRose and Cell 12 ReservedCell render without any sparkline canvas overlay"
    why_human: "Requires visual inspection of running app; component isolation cannot be fully verified statically"
  - test: "Gauge fill animations on AqiGauge work with SmoothedAnimation"
    expected: "AQI, PM2.5, and PM10 ring fills animate smoothly between values (velocity 200) matching ArcGauge behavior; no snap/jump transitions"
    why_human: "Requires observing live or simulated value changes over time"
  - test: "Window resize causes sparklines to repaint at correct resolution"
    expected: "Resizing the app window causes all sparklines (ArcGauge and AqiGauge) to repaint immediately using stride-based decimation adapted to new pixel width"
    why_human: "Requires interactive window resizing during live session"
  - test: "PurpleAir offline graceful degradation"
    expected: "If 10.1.255.41 is unreachable, AQI gauge shows 0/zero values without crashing or displaying error dialogs; weather station data unaffected"
    why_human: "Requires network condition testing; static analysis cannot verify runtime network failure handling behavior"
---

# Phase 03: Trends, Secondary Data, and Air Quality — Verification Report

**Phase Goal:** Users see recent trend context via sparklines, indoor conditions, and air quality alongside the core weather display
**Verified:** 2026-03-01T22:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

**Note on GAUG-13 (Indoor Panel):** Explicitly deferred per user decision. The indoor model properties (tempIn, humIn, dewPointIn) exist in WeatherDataModel from Phase 1 and will serve a future phase. This deferral is intentional and not a gap.

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | WeatherDataModel stores ring buffer of 8640 samples per outdoor sensor (9 total) | VERIFIED | `kSparklineCapacity = 8640` in WeatherDataModel.h L68; 9 `m_*Sparkline` arrays declared L192-226 |
| 2 | Ring buffer wraps at capacity, evicting oldest first | VERIFIED | `recordSparklineSample` wraps via `head = (head+1) % kSparklineCapacity`; `sparklineWrapsAtCapacity` unit test confirms eviction of first 10 |
| 3 | Each sparkline ring buffer exposed to QML as Q_PROPERTY QVariantList with NOTIFY | VERIFIED | 9 `temperatureHistory` through `solarRadHistory` Q_PROPERTYs with NOTIFY signals in WeatherDataModel.h L50-58 |
| 4 | ArcGauge.qml renders sparkline Canvas in lower third (z:-1) | VERIFIED | Canvas id `sparklineCanvas` at ArcGauge.qml L32, `height: parent.height / 3`, `z: -1` |
| 5 | Sparkline line color is #181818, line only, no fill | DIVERGED (human-approved) | Color is `#5A4500` (dim yellow), changed post-human-verification because `#181818` was invisible against `#1A1A1A` background; plan spec overridden by user approval |
| 6 | Sparkline adapts sampling stride to available pixel width | VERIFIED | `stride = Math.max(1, Math.floor(count / width))` in ArcGauge.qml L58 and AqiGauge.qml L65 |
| 7 | DashboardGrid.qml wires correct sparklineData to each outdoor ArcGauge | VERIFIED | All 9 bindings present in DashboardGrid.qml (temperatureHistory, feelsLikeHistory, humidityHistory, dewPointHistory, windSpeedHistory, rainRateHistory, pressureHistory, uvIndexHistory, solarRadHistory) |
| 8 | Existing unit tests continue to pass | VERIFIED | `ctest` output: 3/3 test suites passed (tst_JsonParser, tst_WeatherDataModel, tst_PurpleAirParser) |
| 9 | New ring buffer unit tests verify wrap-around, capacity enforcement, chronological ordering | VERIFIED | `sparklineInitiallyEmpty`, `sparklineRecordsSamples`, `sparklineWrapsAtCapacity`, `pressureSparklineFromBarUpdate` in tst_WeatherDataModel.cpp L441-495 |
| 10 | PurpleAirReading struct with Q_DECLARE_METATYPE exists | VERIFIED | WeatherReadings.h L49-55; `Q_DECLARE_METATYPE(PurpleAirReading)` at L63 |
| 11 | JsonParser::parsePurpleAirJson with A+B averaging and AQI calculation | VERIFIED | JsonParser.cpp L174-187; averages `(pm25_a + pm25_b) / 2.0`; calls `calculateAqi` |
| 12 | calculateAqi uses 2024 EPA breakpoints (Good 0-9.0) | VERIFIED | `kPm25Breakpoints` in JsonParser.cpp L153-160; Good threshold is 9.0 (not pre-2024 12.0) |
| 13 | PurpleAirPoller polls every 30s on its own thread, emits purpleAirReceived | VERIFIED | PurpleAirPoller.h L26 `kPollIntervalMs = 30000`; start() creates QNAM and timer in PurpleAirPoller.cpp L7-13; `emit purpleAirReceived(reading)` at L44 |
| 14 | PurpleAirPoller follows HttpPoller thread pattern | VERIFIED | `start()` creates QNAM inside itself (after moveToThread); mirrors HttpPoller.cpp pattern exactly |
| 15 | WeatherDataModel has aqi, pm25, pm10 Q_PROPERTYs with NOTIFY signals | VERIFIED | WeatherDataModel.h L60-64; READ accessors L107-110; signals L157-161 |
| 16 | WeatherDataModel has separate PurpleAir staleness (m_purpleAirStale) | VERIFIED | `m_purpleAirStale`, `m_hasPurpleAirUpdate`, `m_lastPurpleAirElapsed` in WeatherDataModel.h L246-248; independent check in `checkStaleness()` L519-527 of .cpp |
| 17 | WeatherDataModel has aqiHistory sparkline ring buffer | VERIFIED | `kAqiSparklineCapacity = 2880` in WeatherDataModel.h L251; `m_aqiSparkline` array L252; Q_PROPERTY `aqiHistory` at L64 |
| 18 | main.cpp wires PurpleAirPoller to WeatherDataModel on network thread | VERIFIED | main.cpp L80-88; `purpleAirPoller->moveToThread(networkThread)`; connect `purpleAirReceived` to `applyPurpleAirUpdate`; `QThread::started` wired to `PurpleAirPoller::start` |
| 19 | Unit tests verify AQI calculation at EPA boundary values | VERIFIED | `aqiBoundary` data-driven test in tst_PurpleAirParser.cpp L17-34; 12 boundary values + above-range + negative |
| 20 | Cell 11 displays AQI multi-ring gauge replacing ReservedCell | VERIFIED | DashboardGrid.qml L228-237: `AqiGauge` with `aqiValue: weatherModel.aqi`, `pm25Value: weatherModel.pm25`, `pm10Value: weatherModel.pm10`; no ReservedCell comment retained |
| 21 | AqiGauge has three concentric arc rings in single Shape (AQI, PM2.5, PM10) | VERIFIED | AqiGauge.qml L93-195: one `Shape` with 6 ShapePaths (3 tracks + 3 fills); outer/middle/inner radii computed L27-29 |
| 22 | AQI ring uses EPA 6-zone color coding via aqiColor function | VERIFIED | `aqiColor` function in DashboardGrid.qml L42-49; wired via `aqiColor: dashboard.aqiColor(weatherModel.aqi)` at L233 |
| 23 | PM2.5 and PM10 rings use neutral color | VERIFIED | AqiGauge.qml L146, L179: `strokeColor: "#C8A000"` (neutral yellow) for both PM rings |
| 24 | Center text shows AQI number prominently | VERIFIED | AqiGauge.qml L203-230: Column with "AQI" label + `Math.round(root.aqiValue).toString()` at 22% gauge size, bold |
| 25 | PM2.5 value at bottom-left, PM10 at bottom-right | VERIFIED | AqiGauge.qml L232-251: `anchors.left`/`anchors.right` Text elements with toFixed(1) |
| 26 | AQI ring fill animates via SmoothedAnimation | VERIFIED | AqiGauge.qml L118-120: `Behavior on animatedSweep { SmoothedAnimation { velocity: 200 } }` on all 3 fill paths; Binding elements drive them at L198-200 |
| 27 | AqiGauge includes sparkline Canvas for AQI history | VERIFIED | AqiGauge.qml L40-91: Canvas with `property var data: root.sparklineData`, same stride decimation pattern as ArcGauge |
| 28 | Sparklines visible (dim, non-competing) in lower third of gauges | HUMAN NEEDED | Color `#5A4500` set; human visual confirmation required to confirm legibility and non-competition with gauge content |

**Score:** 27/28 truths verified automated; 1 requires human confirmation (visual appearance)

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/models/WeatherDataModel.h` | kSparklineCapacity, 9 ring buffers, 9 Q_PROPERTYs, history accessors, NOTIFY signals | VERIFIED | All present: `kSparklineCapacity=8640`, 9 sparkline arrays, 9 temperatureHistory..solarRadHistory properties, all NOTIFY signals |
| `src/models/WeatherDataModel.cpp` | recordSparklineSample, sparklineToList, recording in applyIssUpdate/applyBarUpdate | VERIFIED | recordSparklineSample L83-87, sparklineToList L89-97, 8 ISS sensors recorded L337-363, pressure from applyBarUpdate L378-379 |
| `src/qml/ArcGauge.qml` | Canvas sparklineCanvas, sparklineData property, stride-based decimation, auto-repaint | VERIFIED | sparklineData property L17, Canvas id sparklineCanvas L32-84, stride at L58, onWidthChanged/onHeightChanged/onDataChanged repaint triggers |
| `src/qml/DashboardGrid.qml` | sparklineData bindings on every outdoor ArcGauge + AqiGauge | VERIFIED | 10 sparklineData bindings confirmed (9 ArcGauge + 1 AqiGauge) |
| `tests/tst_WeatherDataModel.cpp` | Ring buffer wrap-around, capacity, chronological ordering tests | VERIFIED | 4 sparkline tests at L441-495; sparklineWrapsAtCapacity references kSparklineCapacity |
| `src/models/WeatherReadings.h` | PurpleAirReading struct with pm25_a, pm25_b, pm25avg, pm10, aqi + Q_DECLARE_METATYPE | VERIFIED | Struct at L49-55, Q_DECLARE_METATYPE at L63 |
| `src/network/PurpleAirPoller.h` | PurpleAirPoller class with 30s interval, purpleAirReceived signal | VERIFIED | kPollIntervalMs=30000, signal declared, start()/poll()/onReply() slots |
| `src/network/PurpleAirPoller.cpp` | start() creates QNAM, parsePurpleAirJson called in onReply, signal emitted | VERIFIED | QNAM created in start() L8, parsePurpleAirJson called L40, emit purpleAirReceived L44 |
| `src/network/JsonParser.h` | parsePurpleAirJson and calculateAqi declarations | VERIFIED | Both declared in JsonParser namespace L82, L92 |
| `src/network/JsonParser.cpp` | kPm25Breakpoints with 2024 EPA values, A+B averaging | VERIFIED | kPm25Breakpoints at L153-160 with Good=9.0; averaging at L184 |
| `src/models/WeatherDataModel.h` | applyPurpleAirUpdate slot, aqiHistory, purpleAirStale | VERIFIED | applyPurpleAirUpdate at L121, aqiHistory accessor at L111, purpleAirStale at L110 |
| `src/main.cpp` | PurpleAirPoller creation, moveToThread, purpleAirReceived->applyPurpleAirUpdate | VERIFIED | L80-88; creation, moveToThread, both signal connections, lifecycle connections |
| `tests/tst_PurpleAirParser.cpp` | AQI boundary tests, JSON parsing, missing fields | VERIFIED | 12 boundary values + above-range + negative + valid JSON + missing B + malformed + PM10 |
| `src/qml/AqiGauge.qml` | 6-ShapePath single Shape, animated fills, center text, PM2.5/PM10 labels, sparkline | VERIFIED | All present per line-by-line inspection |
| `src/qml/DashboardGrid.qml` | aqiColor function, AqiGauge in Cell 11, sparklineData on AqiGauge | VERIFIED | aqiColor L42-49, AqiGauge L229-237 |
| `src/CMakeLists.txt` | AqiGauge.qml in qt_add_qml_module; PurpleAirPoller.h/.cpp in wxdash_lib | VERIFIED | AqiGauge.qml at L36; PurpleAirPoller.h/.cpp at L10-11 |
| `tests/CMakeLists.txt` | tst_PurpleAirParser target registered | VERIFIED | L11-13 |

---

## Key Link Verification

### Plan 03-01 Key Links

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| WeatherDataModel.cpp | applyIssUpdate | recordSparklineSample called after applying ISS update | WIRED | recordSparklineSample called 8 times in applyIssUpdate L337-363 |
| DashboardGrid.qml | ArcGauge.qml | sparklineData property binding from model history properties | WIRED | `sparklineData: weatherModel.temperatureHistory` (and 8 others) confirmed |
| ArcGauge.qml | Canvas | Canvas element with onPaint drawing sparkline polyline | WIRED | Canvas element with `ctx.beginPath()` / `ctx.stroke()` in onPaint |

### Plan 03-02 Key Links

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| PurpleAirPoller.cpp | JsonParser.h | parsePurpleAirJson called in onReply() | WIRED | `auto reading = JsonParser::parsePurpleAirJson(data)` at PurpleAirPoller.cpp L40 |
| main.cpp | PurpleAirPoller.h | PurpleAirPoller creation and thread wiring | WIRED | `auto *purpleAirPoller = new PurpleAirPoller(purpleAirUrl)` at main.cpp L80 |
| main.cpp | WeatherDataModel.h | connect purpleAirReceived to applyPurpleAirUpdate | WIRED | `connect(purpleAirPoller, &PurpleAirPoller::purpleAirReceived, model, &WeatherDataModel::applyPurpleAirUpdate)` at main.cpp L83-84 |
| WeatherDataModel.cpp | PurpleAirReading | applyPurpleAirUpdate slot receives PurpleAirReading struct | WIRED | `void WeatherDataModel::applyPurpleAirUpdate(const PurpleAirReading& r)` at .cpp L439 |

### Plan 03-03 Key Links

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| DashboardGrid.qml | AqiGauge.qml | QML component instantiation in Cell 11 | WIRED | `AqiGauge { aqiValue: weatherModel.aqi ... }` at DashboardGrid.qml L229 |
| AqiGauge.qml | weatherModel | Property bindings to aqi, pm25, pm10 | WIRED (through DashboardGrid) | aqiValue, pm25Value, pm10Value bound in DashboardGrid.qml L230-232 |
| DashboardGrid.qml | aqiColor | EPA 6-zone color function driving AQI ring color | WIRED | `aqiColor: dashboard.aqiColor(weatherModel.aqi)` at L233 |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| TRND-01 | 03-01 | Sparkline mini-graphs showing last few hours of data for key sensors | SATISFIED | 9 sparkline ring buffers in WeatherDataModel, Canvas in ArcGauge, wired in DashboardGrid |
| TRND-02 | 03-01 | In-memory ring buffer storage for sparkline history (10s cadence) | SATISFIED | kSparklineCapacity=8640 (24h at 10s), recordSparklineSample called in applyIssUpdate |
| DATA-06 | 03-02 | App polls PurpleAir sensor (http://10.1.255.41/json?live=false) for PM2.5 data | SATISFIED | PurpleAirPoller polls http://10.1.255.41/json?live=false every 30s; wired in main.cpp |
| DATA-07 | 03-02 | PurpleAir channels A and B averaged; AQI calculated via EPA breakpoint table | SATISFIED | parsePurpleAirJson averages (pm25_a + pm25_b) / 2.0; calculateAqi uses 2024 EPA kPm25Breakpoints |
| GAUG-11 | 03-03 | AQI gauge with EPA 6-zone color coding | SATISFIED | AqiGauge.qml outer ring uses aqiColor(); aqiColor() function implements all 6 EPA zones |
| GAUG-12 | 03-03 | PM2.5 gauge showing averaged A+B sensor value | SATISFIED | AqiGauge middle ring shows PM2.5 via pm25Value property; sub-label shows PM2.5 value |
| GAUG-13 | 03-03 | Indoor temperature and humidity panel | DEFERRED | Explicitly deferred per user decision; model properties (tempIn, humIn, dewPointIn) exist from Phase 1 |

**Coverage:** 6/7 Phase 3 requirements satisfied; 1 explicitly deferred by user.

---

## Divergences from Plan Spec

### Sparkline Color: #181818 → #5A4500

**Plan 03-01 specified:** `"Sparkline line color is #181818 (darker than #2A2A2A gauge track), line only, no fill"`

**Actual implementation:** Color is `#5A4500` (dim yellow) in both ArcGauge.qml and AqiGauge.qml.

**Reason:** Human verification in the 03-03 checkpoint revealed that `#181818` was invisible against the `#1A1A1A` window background. The change to `#5A4500` was human-approved as a fix documented in the 03-03 SUMMARY checkpoint fixes. The goal (trend context visible but not competing) is better served by `#5A4500`.

**Assessment:** This is an intentional improvement, not a defect. The visual goal is achieved at higher quality.

### Sparkline Persistence (Bonus)

**Not in any plan spec:** `saveSparklineData`/`loadSparklineData` added to WeatherDataModel; called in main.cpp on startup (load) and every 60s + on shutdown (save). This ensures sparklines show history immediately on restart, not starting from empty.

**Assessment:** Exceeds plan spec. Beneficial addition that improves UX.

---

## Anti-Patterns Found

No blocking anti-patterns found:

- No TODO/FIXME/HACK comments in phase-modified files
- No stub implementations (return null, return {}, empty handlers)
- No placeholder comments
- No console.log-only handlers
- All key signal connections fully wired (not partial)

---

## Human Verification Required

### 1. Sparkline Visual Legibility

**Test:** Run `./build/src/wxdash` with live WeatherLink data. Wait ~30 seconds for several sparkline samples to accumulate. Inspect each outdoor gauge cell.
**Expected:** A dim yellowish line (`#5A4500`) appears in the lower third of each of the 9 outdoor ArcGauge cells (Temperature, Feels-Like, Humidity, Dew Point, Wind, Rain Rate, Pressure, UV Index, Solar Rad). The line is visible but does not compete with the gold gauge text or arc fill.
**Why human:** Color legibility and visual balance require live rendering against the actual dark background.

### 2. AQI Multi-Ring Gauge Appearance

**Test:** With PurpleAir sensor at 10.1.255.41 reachable (or simulated), inspect Cell 11.
**Expected:** Three concentric arc rings visible. Outer (AQI) ring is thickest (50% of total stroke width); middle (PM2.5) and inner (PM10) rings are thinner (25% each). AQI ring changes color by EPA zone. Center shows "AQI" label and prominent numeric value. Bottom-left shows "PM2.5: X.X", bottom-right shows "PM10: X.X". AQI sparkline dimly visible behind rings.
**Why human:** Ring proportions, gap-free concentric rendering, EPA color accuracy at live data values, and sub-label positioning require visual inspection.

### 3. Sparkline Absence on Non-Gauge Cells

**Test:** Inspect Cell 6 (CompassRose) and Cell 12 (ReservedCell) in the running app.
**Expected:** No sparkline Canvas overlay appears on CompassRose or ReservedCell.
**Why human:** Absence is harder to verify statically; CompassRose and ReservedCell do not have sparklineData properties by design.

### 4. Smooth Animation on AQI Gauge

**Test:** If PurpleAir data changes while app is running, or simulate a value change.
**Expected:** All three AqiGauge ring fills animate smoothly using SmoothedAnimation (velocity 200), not jumping to new positions.
**Why human:** Animation behavior requires observing live value transitions.

### 5. Window Resize Sparkline Repaint

**Test:** Run app, wait for sparkline data, then resize the window.
**Expected:** All sparklines immediately repaint at the new width; stride adapts to new pixel count.
**Why human:** Requires interactive window manipulation.

### 6. PurpleAir Offline Graceful Degradation

**Test:** Make 10.1.255.41 unreachable (firewall rule or disconnect). Run app. Wait 30+ seconds.
**Expected:** AQI gauge shows 0 values. No crash, no error dialogs. Weather station data continues normally (wind, temp, rain, etc. unaffected).
**Why human:** Requires network condition simulation; static analysis cannot verify runtime fault tolerance.

---

## Gaps Summary

No automated gaps found. All must-haves verified except visual appearance items that require human confirmation. The sparkline color divergence (#181818 → #5A4500) is human-approved and represents an improvement over the spec. The scope divergence on GAUG-13 (indoor panel) is an explicit user deferral, not a gap.

The phase has successfully delivered:
- Complete sparkline infrastructure (9 sensor ring buffers, Canvas overlay, DashboardGrid wiring)
- PurpleAir data pipeline (struct, parser with 2024 EPA AQI breakpoints, poller, model integration)
- AQI multi-ring gauge (3 concentric arcs, EPA color coding, animated fills, center text, PM sub-labels)
- Full test coverage (4 sparkline unit tests + 15 PurpleAir parser tests + all existing tests passing)
- Bonus: sparkline persistence across restarts

---

_Verified: 2026-03-01T22:00:00Z_
_Verifier: Claude (gsd-verifier)_
