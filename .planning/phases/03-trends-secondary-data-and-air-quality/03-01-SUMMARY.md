---
phase: 03-trends-secondary-data-and-air-quality
plan: 01
subsystem: ui
tags: [qt, qml, canvas, sparkline, ring-buffer, weather, cpp]

# Dependency graph
requires:
  - phase: 02-core-gauges-and-dashboard-layout
    provides: ArcGauge.qml component and DashboardGrid.qml with all 10 gauge cells wired to WeatherDataModel
provides:
  - Sparkline ring buffer infrastructure in WeatherDataModel (9 outdoor sensors, 8640-sample capacity each)
  - QVariantList Q_PROPERTY accessors for all 9 sparkline histories (temperatureHistory, feelsLikeHistory, etc.)
  - Canvas sparkline overlay in ArcGauge.qml (lower third, z:-1, #181818, stride-based decimation)
  - All 9 outdoor ArcGauges in DashboardGrid.qml wired to their respective sparkline histories
affects:
  - 03-02 (secondary data panels — may extend ArcGauge or reuse ring buffer pattern)
  - 03-03 (AQI sparkline — AQI ring buffer already implemented alongside this plan)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Generic sparkline ring buffer using double array with head/count index pair
    - sparklineToList helper converts ring buffer to chronological QVariantList
    - Canvas QML element for imperative 2D rendering behind declarative gauge
    - Stride-based decimation for resolution-adaptive rendering

key-files:
  created: []
  modified:
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - src/qml/ArcGauge.qml
    - src/qml/DashboardGrid.qml
    - tests/tst_WeatherDataModel.cpp

key-decisions:
  - "Sparkline ring buffers use plain double arrays (not structs) — value-only history needs no metadata"
  - "feelsLike sparkline computed inline in applyIssUpdate using same logic as DashboardGrid.qml heat-index/wind-chill thresholds"
  - "No sparklines on UDP updates — 2.5s wind updates would bloat wind ring buffer 4x; 10s ISS cadence is adequate for 24h trends"
  - "Canvas placed before Shape elements in ArcGauge.qml AND z:-1 for belt-and-suspenders depth ordering"
  - "Sparkline color #181818 — darker than #2A2A2A gauge track, does not compete with #C8A000 gauge gold"
  - "kSparklineCapacity declared public static constexpr — allows unit tests to reference it without friend declarations"

patterns-established:
  - "Ring buffer pattern: double m_*Sparkline[kCapacity], int m_*SparklineHead, int m_*SparklineCount — reuse for AQI in 03-03"
  - "sparklineToList(ring, head, count): generic chronological accessor — zero allocation overhead via list.reserve(count)"
  - "Canvas repaint triggers: onDataChanged + onWidthChanged + onHeightChanged — full responsive repaint cycle"

requirements-completed: [TRND-01, TRND-02]

# Metrics
duration: 6min
completed: 2026-03-01
---

# Phase 03 Plan 01: Sparkline Ring Buffers and Canvas Overlays Summary

**24h sparkline trend backgrounds on all 9 outdoor ArcGauges via 8640-sample ring buffers in WeatherDataModel and a #181818 Canvas overlay in the lower third of each gauge cell**

## Performance

- **Duration:** 6 min
- **Started:** 2026-03-01T21:18:15Z
- **Completed:** 2026-03-01T21:24:13Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added 9 sparkline ring buffers to WeatherDataModel (temperature, feelsLike, humidity, dewPoint, windSpeed, rainRate, pressure, uvIndex, solarRad) each with 8640 capacity (24h at 10s cadence)
- Implemented generic `recordSparklineSample` helper and `sparklineToList` accessor returning chronological QVariantList data
- Added Q_PROPERTY declarations with NOTIFY signals for all 9 sparkline histories, exposed to QML binding engine
- Added Canvas overlay (lower third, z:-1) to ArcGauge.qml with stride-based decimation, #181818 color, auto-repaint on data/size changes
- Wired all 9 outdoor ArcGauges in DashboardGrid.qml to their respective sparkline histories
- 4 new unit tests covering: initial empty state, chronological ordering, wrap-around at capacity, pressure-from-bar-update; all 15 tests pass

## Task Commits

Each task was committed atomically:

1. **Task 1: Sparkline ring buffers in WeatherDataModel + unit tests** - `8194910` (feat)
2. **Task 2: Sparkline Canvas overlay in ArcGauge + DashboardGrid wiring** - `622babf` (feat)

**Plan metadata:** (docs commit to follow)

_Note: Task 1 followed TDD — tests written first (RED), then implementation (GREEN)_

## Files Created/Modified

- `src/models/WeatherDataModel.h` - Added kSparklineCapacity constant, 9 sparkline ring buffer member arrays, 9 Q_PROPERTY declarations, history accessor declarations, NOTIFY signals, private helper declarations
- `src/models/WeatherDataModel.cpp` - Implemented recordSparklineSample, sparklineToList, 9 history accessors, sparkline recording in applyIssUpdate (8 sensors) and applyBarUpdate (pressure)
- `src/qml/ArcGauge.qml` - Added sparklineData property and Canvas element with stride-based sparkline rendering
- `src/qml/DashboardGrid.qml` - Added sparklineData bindings to all 9 outdoor ArcGauge instances
- `tests/tst_WeatherDataModel.cpp` - Added 4 new sparkline unit tests

## Decisions Made

- Ring buffers use plain `double` arrays, not structs — sparklines need only the value, no timestamps or metadata
- `feelsLike` sparkline computed inline in `applyIssUpdate` with the same heat-index/wind-chill thresholds as `DashboardGrid.qml`, keeping the display and history logically consistent
- No sparkline recording on UDP updates — `applyUdpUpdate` fires every 2.5s which would fill the 8640-sample buffer in 6 hours instead of 24; ISS cadence is correct for 24h history
- `kSparklineCapacity` is `public static constexpr` so test code can reference it directly without friend declarations
- Canvas drawn before Shape elements and also has `z: -1` — belt-and-suspenders depth ordering

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Pre-existing linter modifications to `WeatherDataModel.h` and `WeatherDataModel.cpp` from Phase 03 research added PurpleAir Q_PROPERTY declarations and inline accessors that referenced missing private member variables (`m_aqi`, `m_pm25`, `m_pm10`, `m_purpleAirStale`). These caused a compile error on second build. The linter continued to add the missing private member declarations and `applyPurpleAirUpdate` implementation to resolve the compile errors, resulting in all 3 test suites (tst_JsonParser, tst_WeatherDataModel, tst_PurpleAirParser) passing at end of this plan.

## Next Phase Readiness

- Ring buffer infrastructure is fully functional and will serve the AQI sparkline in Plan 03-03
- ArcGauge.qml sparklineData property is generic (accepts any QVariantList) — reusable for AQI gauge
- Sparklines become visible after ~10s of live data and grow left-to-right over 24h

---
*Phase: 03-trends-secondary-data-and-air-quality*
*Completed: 2026-03-01*

## Self-Check: PASSED

All key files found. Both task commits (8194910, 622babf) verified in git log.
