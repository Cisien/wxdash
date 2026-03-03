---
phase: 07-wind-rose-calm-and-color
plan: "01"
subsystem: data-model
tags: [wind-rose, ring-buffer, calm-wind, recent-average, qt-cpp]

# Dependency graph
requires:
  - phase: 02-gauges
    provides: "Wind rose histogram with 16-bin rolling window in WeatherDataModel"
provides:
  - "Calm wind samples tracked in ring buffer with bin=-1 sentinel"
  - "recentAvgSpeed field in windRoseData (last ~60s per bin)"
  - "loadSparklineData guards for calm samples on restore"
affects: [07-02-PLAN, wind-rose-qml-consumers]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "bin=-1 sentinel for calm/idle wind samples in WindSample ring buffer"
    - "kRecentSpeedSamples=24 (~60s) window for recent speed averaging"

key-files:
  created: []
  modified:
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - tests/tst_WeatherDataModel.cpp

key-decisions:
  - "Used bin=-1 sentinel for calm samples rather than a separate bool flag -- keeps WindSample struct unchanged"
  - "recentAvgSpeed computed by walking ring backwards collecting last 24 per-bin samples -- O(720) per call, negligible"

patterns-established:
  - "Calm sentinel: WindSample.bin == -1 means calm/idle, skip for bin count/speed aggregation"
  - "Recent window: kRecentSpeedSamples=24 per-bin samples for time-sensitive averaging"

requirements-completed: [WIND-02, WIND-03]

# Metrics
duration: 3min
completed: 2026-03-03
---

# Phase 07 Plan 01: Calm Sample Tracking Summary

**Calm wind samples stored with bin=-1 sentinel in rolling window, plus recentAvgSpeed per direction bin from last ~60s of samples**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-03T01:26:23Z
- **Completed:** 2026-03-03T01:29:36Z
- **Tasks:** 1 (TDD: RED + GREEN)
- **Files modified:** 3

## Accomplishments
- Calm/idle readings (dir=0, speed=0) now occupy ring buffer slots and participate in eviction instead of being discarded
- Calm samples produce no directional bin counts (bin=-1 sentinel skipped in aggregation)
- windRoseData() returns recentAvgSpeed per bin computed from the last 24 samples per bin (~60s at 2.5s UDP rate)
- loadSparklineData correctly restores calm samples without array-out-of-bounds
- All 29 tests pass (22 existing + 7 new)

## Task Commits

Each task was committed atomically:

1. **Task 1 (RED): Failing tests** - `416d5e3` (test)
2. **Task 1 (GREEN): Implementation** - `e76d5d0` (feat)

_TDD task with RED + GREEN commits. No refactor needed -- implementation was clean._

## Files Created/Modified
- `src/models/WeatherDataModel.h` - Added kRecentSpeedSamples=24 constant
- `src/models/WeatherDataModel.cpp` - recordWindSample uses bin=-1 for calm, windRoseData computes recentAvgSpeed, loadSparklineData guards bin=-1
- `tests/tst_WeatherDataModel.cpp` - 7 new tests for calm tracking and recent-average speed

## Decisions Made
- Used bin=-1 sentinel for calm samples rather than adding a boolean flag to WindSample -- keeps the struct unchanged and the sentinel approach is consistent with how bins are already used
- recentAvgSpeed walks the ring buffer backwards collecting the last 24 samples per bin -- O(720) worst case per windRoseData() call, which is negligible for a function called at 2.5s cadence

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- windRoseData now exposes recentAvgSpeed per bin, ready for plan 07-02 to wire up QML bar coloring
- Calm samples tracked in ring buffer -- sample counts will be more accurate for rose proportions

---
*Phase: 07-wind-rose-calm-and-color*
*Completed: 2026-03-03*
