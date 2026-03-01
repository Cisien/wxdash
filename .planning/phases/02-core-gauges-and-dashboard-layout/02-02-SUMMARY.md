---
phase: 02-core-gauges-and-dashboard-layout
plan: "02"
subsystem: ui
tags: [qml, qt6, canvas, gridlayout, compass-rose, arc-gauge, wind-rose, threshold-colors]

# Dependency graph
requires:
  - phase: 02-01
    provides: ArcGauge.qml reusable component, QML engine wired to WeatherDataModel context property
  - phase: 01-01
    provides: WeatherDataModel Q_PROPERTYs (temperature, humidity, windDir, windSpeed, etc.)

provides:
  - DashboardGrid.qml — 3x4 GridLayout assembling all 10 gauge cells + 2 reserved cells
  - CompassRose.qml — Canvas wind rose with RotationAnimation, showing wind histogram via radial bar chart
  - ReservedCell.qml — Styled placeholder cell for future gauges (AQI, etc.)
  - Threshold color functions in DashboardGrid: temperatureColor, humidityColor, windSpeedColor, uvColor
  - Feels-like logic (heat index / wind chill / raw temp switching) in DashboardGrid
  - Pressure conversion: inHg to mbar inline binding
  - Wind gust and daily rainfall accumulation as secondary gauge text

affects:
  - 02-03 (future plans extending the grid layout or adding gauge variants)
  - 03-historical-data (any chart overlay atop the dashboard)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Canvas-based wind rose using radial bar chart for direction histogram with rolling window
    - GridLayout uniform cell sizing for responsive dashboard scaling
    - Qt 6.5+ RESOURCE_PREFIX /qt/qml required when URI matches executable name
    - Threshold color JS functions co-located in DashboardGrid for single source of truth
    - QML property binding block expressions ({ if ... return }) for computed derived properties
    - RotationAnimation.Shortest for smooth compass wrap-around (359->1)
    - WeatherDataModel wind histogram rolling window filtering calm (0/0) readings

key-files:
  created:
    - src/qml/DashboardGrid.qml
    - src/qml/CompassRose.qml
    - src/qml/ReservedCell.qml
  modified:
    - src/qml/Main.qml
    - src/CMakeLists.txt
    - src/qml/ArcGauge.qml
    - src/models/WeatherDataModel.cpp
    - src/models/WeatherDataModel.h

key-decisions:
  - "Wind rose implemented as radial bar chart histogram (not traditional compass rose with pointer) — better suited for showing predominant wind direction over time vs instantaneous direction"
  - "CompassRose title and direction readout removed to maximize rose size — matches visual weight of ArcGauge cells"
  - "Cardinal labels removed from wind rose to avoid crowding and let the arc shape speak for itself"
  - "Calm wind readings (windDir=0, windSpeed=0) filtered from histogram to avoid false north-heavy data"
  - "Rolling window added to WeatherDataModel wind histogram — prevents stale historical data from dominating display"
  - "ArcGauge value/unit text split into two separate Text elements (Column layout) to prevent overlap with arc"
  - "RESOURCE_PREFIX set to /qt/qml in qt_add_qml_module — required for Qt 6.5+ loadFromModule when URI matches binary name"
  - "CompassRose redesigned from traditional pointer compass to wind rose during visual verification based on user feedback"

patterns-established:
  - "Pattern 1: Threshold color functions as plain JS functions in parent grid component — keeps gauge instances declarative"
  - "Pattern 2: Secondary display text as ArcGauge.secondaryText property — consistent pattern for gust, daily rain, trend arrow"
  - "Pattern 3: QML block expression property bindings for multi-condition derived values (feelsLikeValue, feelsLikeLabel)"

requirements-completed: [GAUG-01, GAUG-02, GAUG-03, GAUG-04, GAUG-05, GAUG-06, GAUG-07, GAUG-08, GAUG-09, GAUG-10]

# Metrics
duration: 35min
completed: 2026-03-01
---

# Phase 2 Plan 02: Core Gauges and Dashboard Layout Summary

**3x4 QML dashboard grid with 10 live weather gauges, threshold color coding, Canvas wind rose with rolling-window histogram, and all secondary displays (gust, daily rain, trend arrow, feels-like)**

## Performance

- **Duration:** ~35 min (including multiple visual verification iterations)
- **Started:** 2026-03-01T11:35:01Z
- **Completed:** 2026-03-01T12:10:17Z
- **Tasks:** 3 (2 auto + 1 checkpoint, approved after iterations)
- **Files modified:** 8

## Accomplishments

- Full 3x4 dashboard grid (`DashboardGrid.qml`) assembling all 10 weather gauges plus 2 styled reserved cells
- Compass rose redesigned during visual review from traditional pointer rose to a radial bar chart wind histogram showing directional frequency over time
- Threshold color functions for temperature (6 zones), humidity (5 zones), wind speed (4 zones), UV index (5 EPA zones)
- Feels-like switching logic: heat index (>=80F + >=40% RH), wind chill (<=50F + >=3mph), otherwise raw temperature
- Pressure displayed in mbar (converted from inHg via `* 33.8639`)
- Wind gust and daily rainfall shown as secondary text on their respective gauges
- Rolling window filtering in WeatherDataModel prevents calm-wind noise from skewing histogram
- App confirmed compiling and visually verified by user

## Task Commits

Each task was committed atomically with additional fix commits during visual verification:

1. **Task 1: DashboardGrid.qml + ReservedCell.qml + Main.qml update** - `58bca20` (feat)
2. **Task 2: CompassRose.qml full implementation** - `defb347` (feat)
3. **Fix: Qt 6.5+ RESOURCE_PREFIX for loadFromModule** - `b79250e` (fix)
4. **Fix: Redesign CompassRose as stylized wind rose** - `93f271c` (fix)
5. **Fix: Split ArcGauge value/unit text to Column layout** - `1cf592b` (fix)
6. **Fix: Wind rose radial bar chart + ArcGauge text layout** - `4503bbe` (fix)
7. **Fix: Remove wind rose title/readout, enlarge to match gauges** - `ca7e252` (fix)
8. **Fix: Remove cardinal labels from wind rose, enlarge circle** - `4079bad` (fix)
9. **Fix: Filter calm wind 0/0 readings, add rolling window** - `1816fbc` (fix)

**Task 3 (checkpoint:human-verify):** APPROVED by user

## Files Created/Modified

- `src/qml/DashboardGrid.qml` - 3x4 GridLayout with 10 ArcGauge instances, 2 ReservedCells, threshold color functions, feels-like logic, and pressure conversion
- `src/qml/CompassRose.qml` - Canvas-based wind rose radial bar chart with rolling window histogram, RotationAnimation.Shortest for smooth direction updates
- `src/qml/ReservedCell.qml` - Styled dark placeholder Rectangle for future gauge slots
- `src/qml/Main.qml` - Updated to instantiate DashboardGrid instead of standalone ArcGauge placeholder
- `src/qml/ArcGauge.qml` - Fixed value/unit text layout (Column with two Text elements to prevent arc overlap)
- `src/CMakeLists.txt` - Added DashboardGrid.qml, CompassRose.qml, ReservedCell.qml to qt_add_qml_module; set RESOURCE_PREFIX to /qt/qml
- `src/models/WeatherDataModel.cpp` - Added rolling window to wind direction histogram, filter calm readings
- `src/models/WeatherDataModel.h` - Rolling window data members for wind histogram

## Decisions Made

- **Wind rose as histogram, not pointer:** The plan called for a traditional pointer-style compass rose. During visual verification the user preferred a wind rose radial bar chart that shows directional frequency — more informative for an always-on weather display.
- **No title/readout on wind rose:** Removing the "Wind Dir" label and numeric readout at bottom gives the histogram more visual real estate, matching the ArcGauge cells' visual weight.
- **No cardinal labels on wind rose:** Text labels on the radial bars crowded the design; the directional shape of the bars conveys orientation.
- **Calm wind filtering:** Readings where both windDir=0 and windSpeed=0 are treated as "calm" or no-data and excluded from the histogram to avoid north-bias artifacts.
- **Rolling window:** WeatherDataModel maintains a bounded window of recent readings so the histogram reflects recent conditions rather than accumulating all historical data.
- **RESOURCE_PREFIX /qt/qml:** Qt 6.5+ requires this when using `loadFromModule` and the QML module URI matches the executable name (both "wxdash") to avoid file path collisions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Qt 6.5+ RESOURCE_PREFIX required for loadFromModule**
- **Found during:** Task 2 (first visual verification run)
- **Issue:** `QQmlApplicationEngine::loadFromModule("wxdash", "Main")` failed at runtime because the QML module resources were registered under the wrong prefix when URI equals the binary name
- **Fix:** Added `RESOURCE_PREFIX /qt/qml` to `qt_add_qml_module` in CMakeLists.txt
- **Files modified:** src/CMakeLists.txt
- **Verification:** App launched successfully
- **Committed in:** b79250e

**2. [Rule 1 - Bug] ArcGauge value and unit text overlapped the arc**
- **Found during:** Visual verification checkpoint
- **Issue:** Single Text element for value+unit positioned at arc center caused the text string to overlap the arc track at certain font sizes and window sizes
- **Fix:** Split into two separate Text elements (value on one line, unit below) inside a Column with centerIn anchor
- **Files modified:** src/qml/ArcGauge.qml
- **Verification:** Text displays cleanly within arc bounds across window sizes
- **Committed in:** 1cf592b

**3. [Rule 1 - Bug] Wind rose showed north-biased histogram due to calm readings**
- **Found during:** Visual verification checkpoint (user-observed behavior)
- **Issue:** Station sends windDir=0, windSpeed=0 during calm conditions, polluting the north bucket of the histogram
- **Fix:** Filter out readings where windSpeed <= 0.1 mph before adding to histogram; added rolling window to WeatherDataModel
- **Files modified:** src/models/WeatherDataModel.cpp, src/models/WeatherDataModel.h, src/qml/CompassRose.qml
- **Verification:** Histogram shows only meaningful directional data
- **Committed in:** 1816fbc

---

**Total deviations:** 3 auto-fixed (1 blocking, 2 bugs) + iterative visual design refinements during checkpoint review
**Impact on plan:** Auto-fixes were necessary for correctness and visual quality. Design refinements (wind rose style, label removal) were user-directed during the checkpoint verification phase — not unplanned scope, but responsive to user feedback at the intended review gate.

## Issues Encountered

- CompassRose went through 5 iterations of design during the visual verification checkpoint: initial pointer compass → stylized wind rose → radial bar chart → remove labels/title → filter calm readings. Each iteration was a targeted fix commit. The checkpoint mechanism worked as intended — the user could evaluate and provide specific feedback without requiring a new plan.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Full dashboard grid is assembled and visually verified — ready for Phase 3 (historical data) or Phase 4 (deployment)
- Two reserved cells (Row 3, columns 3-4) hold slots for future gauges (AQI from Phase 3 planned for one slot)
- WeatherDataModel wind histogram infrastructure is in place; any Phase 3 historical data work can extend the same rolling window pattern
- No blockers

---
*Phase: 02-core-gauges-and-dashboard-layout*
*Completed: 2026-03-01*
