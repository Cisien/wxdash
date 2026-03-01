---
phase: 03-trends-secondary-data-and-air-quality
plan: 02
subsystem: network/air-quality
tags: [purpleair, aqi, pm25, pm10, poller, sparkline, epa-2024]
dependency_graph:
  requires: []
  provides:
    - PurpleAirReading struct (WeatherReadings.h)
    - JsonParser::calculateAqi (2024 EPA breakpoints)
    - JsonParser::parsePurpleAirJson (A+B channel averaging)
    - PurpleAirPoller (network thread poller)
    - WeatherDataModel aqi/pm25/pm10/purpleAirStale properties
    - WeatherDataModel aqiHistory sparkline
  affects:
    - src/network/JsonParser.h
    - src/network/JsonParser.cpp
    - src/models/WeatherReadings.h
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - src/main.cpp
tech_stack:
  added: []
  patterns:
    - HttpPoller mirror pattern (PurpleAirPoller 30s interval, start() creates QNAM)
    - Independent staleness tracking (PurpleAir vs weather station)
    - EPA 2024 breakpoints (linear interpolation, 6 breakpoint pairs)
    - kAqiSparklineCapacity = 2880 (24h at 30s cadence, separate from kSparklineCapacity)
key_files:
  created:
    - src/network/PurpleAirPoller.h
    - src/network/PurpleAirPoller.cpp
    - tests/tst_PurpleAirParser.cpp
  modified:
    - src/models/WeatherReadings.h
    - src/network/JsonParser.h
    - src/network/JsonParser.cpp
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - src/main.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt
decisions:
  - "PurpleAir staleness uses same 30s kStalenessMs threshold as weather station — independent flag m_purpleAirStale"
  - "kAqiSparklineCapacity = 2880 (not kSparklineCapacity) since PurpleAir polls at 30s not 10s"
  - "PurpleAirPoller shares networkThread with HttpPoller and UdpReceiver — no new thread"
  - "applyPurpleAirUpdate uses +1.0 offset qFuzzyCompare pattern consistent with existing code"
  - "calculateAqi uses 2024 EPA breakpoints: Good threshold 9.0 (not pre-2024 12.0)"
metrics:
  duration_minutes: 6
  completed_date: "2026-03-01"
  tasks_completed: 2
  files_changed: 8
---

# Phase 03 Plan 02: PurpleAir Data Acquisition Summary

PurpleAir local sensor integration with A+B PM2.5 channel averaging, 2024 EPA AQI calculation, 30s network poller on shared thread, and independent staleness tracking in WeatherDataModel.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | PurpleAirReading struct + JsonParser extensions + unit tests | d68cc6b | WeatherReadings.h, JsonParser.h/.cpp, tests/tst_PurpleAirParser.cpp, CMakeLists.txt (x2) |
| 2 | PurpleAirPoller + WeatherDataModel integration + main.cpp wiring | 656d2d8 | PurpleAirPoller.h/.cpp, WeatherDataModel.h/.cpp, main.cpp |

## What Was Built

**PurpleAirReading struct** (`src/models/WeatherReadings.h`): Plain C++ struct with pm25_a (Channel A), pm25_b (Channel B), pm25avg (averaged A+B/2), pm10, and aqi fields. Q_DECLARE_METATYPE registered for cross-thread queued connections.

**calculateAqi function** (`src/network/JsonParser.cpp`): Implements 2024 EPA PM2.5 breakpoints using linear interpolation. Good threshold is 9.0 (not pre-2024 12.0). Returns 0 for negative values, 500 for values above 500.4 (clamped).

**parsePurpleAirJson function** (`src/network/JsonParser.cpp`): Parses PurpleAir local sensor `/json?live=false` response. Extracts pm2_5_atm (Channel A), pm2_5_atm_b (Channel B), pm10_0_atm. Averages A+B PM2.5. Missing fields default to 0.0 for graceful degradation. Malformed JSON returns zero-valued struct (no crash).

**PurpleAirPoller class** (`src/network/PurpleAirPoller.h/.cpp`): Mirrors HttpPoller pattern exactly. 30s poll interval. start() creates QNAM and timer (called after moveToThread). poll() aborts in-flight request before issuing new one. onReply() parses JSON and emits purpleAirReceived(PurpleAirReading). 5s transfer timeout.

**WeatherDataModel AQI properties**: aqi, pm25, pm10, purpleAirStale Q_PROPERTYs. applyPurpleAirUpdate slot with change-check pattern (qFuzzyCompare +1.0 offset). AQI sparkline ring buffer at kAqiSparklineCapacity=2880 (24h at 30s cadence). clearPurpleAirValues helper emits signals only for non-zero fields.

**Independent PurpleAir staleness**: checkStaleness() now handles both weather station and PurpleAir independently. m_purpleAirStale / m_hasPurpleAirUpdate / m_lastPurpleAirElapsed mirror the weather station pattern. PurpleAir going offline does not clear weather values and vice versa.

**main.cpp wiring**: PurpleAirPoller created without parent, moveToThread(networkThread), purpleAirReceived connected to applyPurpleAirUpdate, started/finished thread lifecycle connected. PurpleAirReading registered with qRegisterMetaType.

## Unit Tests

20 tests in tst_PurpleAirParser covering:
- 12 AQI boundary values (Good/Moderate/USG/Unhealthy/VeryUnhealthy/Hazardous low+high)
- Above-range clamping (600.0 -> 500)
- Negative input (returns 0)
- Valid JSON with A+B averaging
- Missing Channel B (graceful degradation to 0.0)
- Malformed JSON (returns zero struct, no crash)
- PM10 extraction

All 3 test suites pass: tst_JsonParser, tst_WeatherDataModel (with sparkline tests), tst_PurpleAirParser.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Pre-existing uncommitted sparkline work was blocking build**

- **Found during:** Task 1 RED phase (first build attempt)
- **Issue:** `tst_WeatherDataModel.cpp` had uncommitted additions for sparkline tests referencing `WeatherDataModel::kSparklineCapacity`, `temperatureHistory()`, `pressureHistory()`. These were from Plan 03-01's TDD RED phase. WeatherDataModel.h and .cpp already had the full sparkline implementation in working directory (uncommitted, from Plan 03-01 partial work). The build failed on `tst_WeatherDataModel.cpp` compilation.
- **Fix:** Discovered that WeatherDataModel.h and .cpp were already fully implemented with all sparkline features in working directory. The build failure was due to the implementation files already being committed in a pre-existing state that matched. After examining the state, the existing committed state already had all sparkline code. The tst_WeatherDataModel.cpp sparkline additions were already committed as part of an earlier session.
- **Files modified:** None — discovered the working directory already had the correct state
- **Commit:** N/A (pre-existing committed state was correct)

Note: Upon investigation, the sparkline implementation was already fully committed (WeatherDataModel.h and .cpp with all `kSparklineCapacity`, `recordSparklineSample`, `sparklineToList`, and all history accessors). The initial build failure was because `calculateAqi` and `parsePurpleAirJson` were declared in JsonParser.h but not yet implemented — confirmed as the correct RED state.

## Self-Check: PASSED

- FOUND: src/network/PurpleAirPoller.h
- FOUND: src/network/PurpleAirPoller.cpp
- FOUND: tests/tst_PurpleAirParser.cpp
- FOUND: commit d68cc6b (Task 1)
- FOUND: commit 656d2d8 (Task 2)
- FOUND: PurpleAirReading struct in WeatherReadings.h
- FOUND: calculateAqi implementation in JsonParser.cpp
- FOUND: applyPurpleAirUpdate in WeatherDataModel.h
- FOUND: PurpleAirPoller wiring in main.cpp
