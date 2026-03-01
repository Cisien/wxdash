---
phase: 03-trends-secondary-data-and-air-quality
plan: "03"
subsystem: ui
tags: [qml, qtquick-shapes, aqi, air-quality, gauge, purpleair, animation]

# Dependency graph
requires:
  - phase: 03-01
    provides: ArcGauge sparkline Canvas pattern and ring buffer infrastructure
  - phase: 03-02
    provides: WeatherDataModel AQI/PM2.5/PM10/aqiHistory properties from PurpleAir integration

provides:
  - AqiGauge.qml — three-ring concentric arc gauge for AQI, PM2.5, and PM10 in a single Shape
  - aqiColor() function in DashboardGrid.qml — EPA 6-zone color lookup for AQI ring
  - Cell 11 in 3x4 grid now shows AqiGauge instead of ReservedCell placeholder

affects:
  - phase-04-polish
  - pi-deployment

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Multi-ring concentric arc gauge using single QML Shape with 6 ShapePaths (3 tracks + 3 fills)
    - Stroke partition: outer ring 1/2 total width, middle and inner rings 1/4 each
    - Center-radius calculation accounts for half-strokes to eliminate gaps/overlaps between rings
    - EPA 6-zone AQI color lookup function pattern (matches uvColor/humidityColor style)

key-files:
  created:
    - src/qml/AqiGauge.qml
  modified:
    - src/qml/DashboardGrid.qml
    - src/CMakeLists.txt

key-decisions:
  - "Single Shape with 6 ShapePaths (3 tracks + 3 fills) — Qt recommends all shape paths in one Shape for best performance"
  - "Ring partition ratios: AQI 1/2 stroke, PM2.5 1/4, PM10 1/4 — AQI gets visual prominence as primary metric"
  - "PM2.5 and PM10 rings use neutral yellow (#C8A000) — consistent with dashboard palette, only AQI ring uses EPA zone colors"
  - "GAUG-13 (indoor panel) deferred per user decision — indoor model properties exist in WeatherDataModel from Phase 1"
  - "AQI sparkline uses same Canvas pattern as ArcGauge — consistent sparkline behavior across all gauges"

patterns-established:
  - "Multi-ring gauge pattern: compute center radii from outer ring inward, each ring = previous_outer - half_strokes"
  - "EPA AQI color zones: Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+"

requirements-completed: [GAUG-11, GAUG-12]

# Metrics
duration: 4min
completed: 2026-03-01
---

# Phase 3 Plan 03: AQI Multi-Ring Gauge Summary

**Three-ring concentric AQI/PM2.5/PM10 gauge in Cell 11 with EPA 6-zone color coding, animated fills via SmoothedAnimation, and AQI sparkline background**

## Performance

- **Duration:** 4 min
- **Started:** 2026-03-01T21:29:57Z
- **Completed:** 2026-03-01T21:33:01Z
- **Tasks:** 1 (auto) + 1 (checkpoint:human-verify)
- **Files modified:** 3

## Accomplishments

- Created AqiGauge.qml with three concentric arc rings (AQI/PM2.5/PM10) in a single Shape element
- AQI outer ring uses EPA 6-zone colors driven by aqiColor() property from DashboardGrid
- All three rings animate with SmoothedAnimation (velocity: 200) matching ArcGauge behavior
- Center text shows AQI label + prominent numeric value; bottom corners show PM2.5/PM10 sub-labels
- Sparkline Canvas (same pattern as ArcGauge) renders AQI history in lower third behind arcs
- DashboardGrid Cell 11 now uses AqiGauge bound to weatherModel.aqi/pm25/pm10/aqiHistory
- Added aqiColor() function to DashboardGrid.qml with 6-zone EPA thresholds
- AqiGauge.qml registered in qt_add_qml_module in src/CMakeLists.txt
- All 3 existing tests pass (JsonParser, WeatherDataModel, PurpleAirParser)

## Task Commits

Each task was committed atomically:

1. **Task 1: AqiGauge.qml + DashboardGrid wiring + CMake registration** - `ecc13b9` (feat)

2. **Checkpoint fixes (post human-verify):** `295b7d2` (fix)

## Files Created/Modified

- `src/qml/AqiGauge.qml` — Multi-ring AQI gauge: 6 ShapePaths in one Shape, sparkline Canvas, center text, PM2.5/PM10 sub-labels
- `src/qml/ArcGauge.qml` — Sparkline color and min/max fixes
- `src/qml/DashboardGrid.qml` — Added aqiColor() function, replaced Cell 11 ReservedCell with AqiGauge, qualified aqiColor call
- `src/CMakeLists.txt` — Registered AqiGauge.qml, switched icon resource to qt_add_resources
- `src/models/WeatherDataModel.h/.cpp` — Added sparkline persistence (save/load to disk)
- `src/main.cpp` — Wired sparkline persistence (load on startup, save every 60s + on shutdown)

## Decisions Made

- Single Shape with 6 ShapePaths is Qt's recommended pattern for best GPU performance — track and fill for each ring in the same Shape
- Stroke partitioning (AQI 50%, PM2.5 25%, PM10 25%) gives AQI visual dominance as the primary air quality index
- Only the AQI ring uses EPA color coding; PM2.5 and PM10 rings use neutral yellow (#C8A000) matching dashboard palette
- GAUG-13 (indoor temperature/humidity panel) deferred per explicit user decision at planning time

## Checkpoint Fixes

Human verification revealed these issues (fixed in `295b7d2`):
- Sparkline color `#181818` invisible against `#1A1A1A` window background → `#5A4500` (dim yellow)
- Sparklines auto-scaled only from data range → now use gauge min/max as baseline
- Sparklines empty on startup → ring buffers persisted to `~/.local/share/wxdash/sparklines.bin`
- PM2.5/PM10 ring fill gray → neutral yellow (`#C8A000`)
- PM2.5/PM10 text overlapping arc → margins pushed from 10% to 2% from edges
- `aqiColor` property shadowing DashboardGrid function → qualified call `dashboard.aqiColor()`
- Icon SVG not loading → switched from `.qrc` to `qt_add_resources()` for Qt6

## Next Phase Readiness

- Phase 3 is now visually complete: sparklines on all outdoor gauges (03-01), PurpleAir AQI data (03-02), AQI gauge in Cell 11 (03-03)
- Cell 12 remains ReservedCell (future forecast widget — planned for a later phase)
- Phase 4 (polish/UX) can begin

---
*Phase: 03-trends-secondary-data-and-air-quality*
*Completed: 2026-03-01*
