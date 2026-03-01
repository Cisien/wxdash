---
phase: 01-data-model-and-network-layer
plan: "02"
subsystem: data
tags: [qt6, qobject, qproperty, qsignalspy, qtest, c++17, staleness, tdd, injectable-clock]

requires:
  - phase: 01-data-model-and-network-layer/01-01
    provides: WeatherReadings.h plain C++ structs (IssReading, BarReading, IndoorReading, UdpReading) used as slot parameter types

provides:
  - WeatherDataModel QObject with 18 Q_PROPERTY fields + NOTIFY signals
  - Staleness detection: sourceStaleChanged(true) after 30s silence, clearAllValues on stale
  - Silent recovery: sourceStaleChanged(false) when data resumes after staleness
  - Injectable std::function<qint64()> elapsed provider for fast unit tests (no 30s waits)
  - 18 unit tests covering all apply slots, staleness, clear-on-stale, recovery, and no-false-positive

affects:
  - 01-data-model-and-network-layer/01-03 (HttpPoller/UdpReceiver emit readings to WeatherDataModel slots)
  - 02-ui-layer (all widgets bind to WeatherDataModel Q_PROPERTY via NOTIFY signals)

tech-stack:
  added:
    - QElapsedTimer (wall clock for production staleness timing)
    - QSignalSpy (signal assertion in all test cases)
  patterns:
    - Change-check pattern: qFuzzyCompare for doubles, == for ints before emitting NOTIFY signals
    - Injectable clock: std::function<qint64()> passed to constructor enables deterministic time in tests
    - clearAllValues: emit changed signals only for non-zero fields (avoids spurious signals)
    - markUpdated: single place to record last-update timestamp and start staleness timer on first update
    - TDD red-green-refactor: stub implementation fails tests, full impl passes, clang-format applied

key-files:
  created:
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - tests/tst_WeatherDataModel.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Injectable clock (std::function<qint64()>) accepted in constructor — avoids real 30s waits in tests while preserving QElapsedTimer for production"
  - "clearAllValues emits changed signals only for non-zero fields — prevents spurious signals when values are already 0"
  - "Staleness timer starts on first received update, not on construction — prevents false positive at startup"
  - "Recovery from staleness handled in applyIssUpdate/applyUdpUpdate, not in checkStaleness — requires real data to recover"

patterns-established:
  - "qFuzzyCompare(x, y) for all double field change-checks in apply slots"
  - "Emit changed signal passing the stored member value, not the incoming value"
  - "m_hasReceivedUpdate guard prevents false staleness before any data arrives"

requirements-completed: [DATA-08]

duration: 3min
completed: 2026-03-01
---

# Phase 1 Plan 02: WeatherDataModel with TDD Summary

**WeatherDataModel QObject with 18 Q_PROPERTY+NOTIFY fields, injectable-clock staleness detection (30s threshold), clear-on-stale zeroing, silent recovery, and 18 unit tests passing in 1ms**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-01T18:27:31Z
- **Completed:** 2026-03-01T18:31:18Z
- **Tasks:** 1 (TDD: RED + GREEN + REFACTOR commits)
- **Files modified:** 5

## Accomplishments

- WeatherDataModel QObject implements 18 Q_PROPERTY fields covering all ISS, barometer, indoor, and sourceStale properties
- Injectable `std::function<qint64()>` clock means staleness tests run in 1ms instead of 30 seconds (zero real-time waits)
- Four typed update slots: `applyIssUpdate`, `applyBarUpdate`, `applyIndoorUpdate`, `applyUdpUpdate` each with change-check pattern
- Staleness detection: 30s threshold, `clearAllValues()` zeroes all fields and emits per-field signals, `sourceStaleChanged(true)`
- Silent recovery: `applyIssUpdate`/`applyUdpUpdate` emit `sourceStaleChanged(false)` when called during stale state
- No false positive at startup: `m_hasReceivedUpdate` guard ensures `checkStaleness()` is a no-op before first data arrives

## Task Commits

TDD task has three commits:

1. **Task 1 RED: Failing tests for WeatherDataModel** - `ec80230` (test)
2. **Task 1 GREEN: Full WeatherDataModel implementation** - `ab549df` (feat)

_Note: clang-format applied in GREEN commit. No separate REFACTOR commit needed (no logic changes)._

## Files Created/Modified

- `/home/cisien/src/wxdash/src/models/WeatherDataModel.h` - QObject with 18 Q_PROPERTY+NOTIFY fields, injectable clock constructor
- `/home/cisien/src/wxdash/src/models/WeatherDataModel.cpp` - apply slots, checkStaleness, clearAllValues, markUpdated
- `/home/cisien/src/wxdash/tests/tst_WeatherDataModel.cpp` - 18 QTest cases using QSignalSpy and fake clock
- `/home/cisien/src/wxdash/src/CMakeLists.txt` - Added WeatherDataModel.cpp to wxdash_lib sources
- `/home/cisien/src/wxdash/tests/CMakeLists.txt` - Added tst_WeatherDataModel test target

## Decisions Made

- Injectable clock via `std::function<qint64()>` in constructor: tests inject a `qint64*` lambda returning a controllable value; production uses `QElapsedTimer`. This avoids 30-second real waits without adding a separate test-only API.
- `clearAllValues()` only emits changed signals for fields that are actually non-zero: avoids spurious signals when clearing an already-cleared model.
- Staleness timer (`QTimer`, 5s interval) starts only after the first update is received (`m_hasReceivedUpdate` flag), ensuring no false positive at startup.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- WeatherDataModel is fully tested and ready for HttpPoller and UdpReceiver to connect their signals to its apply slots (Plan 03)
- All Q_PROPERTY+NOTIFY fields are established — UI layer (Phase 2) can bind widgets directly
- Injectable clock pattern established as the project's standard for testing time-dependent behavior

---
*Phase: 01-data-model-and-network-layer*
*Completed: 2026-03-01*
