---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: unknown
last_updated: "2026-03-01T21:26:02.068Z"
progress:
  total_phases: 4
  completed_phases: 2
  total_plans: 8
  completed_plans: 7
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-01)

**Core value:** Display live weather conditions from the WeatherLink Live with real-time updates — always-on, always-current, at a glance.
**Current focus:** Phase 2 complete — moving to Phase 3 or Phase 4

## Current Position

Phase: 3 of 4 (Trends, Secondary Data, and Air Quality) — IN PROGRESS
Plan: 2 of 3 in phase (03-01 and 03-02 complete, 03-03 remaining)
Status: Plan 03-02 complete — PurpleAir data acquisition with 2024 EPA AQI, WeatherDataModel integration
Last activity: 2026-03-01 — Completed Plan 03-02: PurpleAir poller, AQI calculation, model properties

Progress: [███████░░░] 70% of phase 3 plans complete (2/3 plans)

## Performance Metrics

**Velocity:**
- Total plans completed: 7
- Average duration: 8.7 min
- Total execution time: 60 min

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| Phase 1: Data Model and Network Layer | 3 | 11 min | 3.7 min |
| Phase 2: Core Gauges and Dashboard Layout | 2 | 38 min | 19 min |
| Phase 3: Trends, Secondary Data, Air Quality | 2 complete | 11 min | 5.5 min |

**Recent Trend:**
- Last 5 plans: 6 min, 3 min, 2 min, 3 min, 35 min
- Trend: 02-02 took longer due to iterative visual verification checkpoint with multiple design refinements

| Phase | Plan | Duration | Tasks | Files |
|-------|------|----------|-------|-------|
| Phase 03-trends-secondary-data-and-air-quality | P01 | 5 min | 2 tasks | 5 files |
| Phase 03-trends-secondary-data-and-air-quality | P02 | 6 min | 2 tasks | 8 files |

*Updated after each plan completion*

## Accumulated Context

### Roadmap Evolution

- Phase 5 added: CMake install-kiosk target for Raspberry Pi deployment

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Init]: Use Qt 6 C++ with QML/Canvas for GPU-accelerated gauge rendering (Qt Widgets rejected — CPU software rasterization on Pi causes thermal throttle)
- [Init]: Native compilation on Pi recommended over cross-compilation (sysroot symlink issues documented as time sink)
- [Init]: EGLFS eglfs_kms backend required for Pi 4/5 on embedded deployments (eglfs_brcm is Pi 1-3 only — black screen on Pi 4/5)
- [Revise-2026-03-01]: Development and testing happens on local Linux machine, not the Pi. Pi is a deployment target. Pi-specific deployment notes go in README, not a build validation phase.
- [Revise-2026-03-01]: App targets Linux broadly. Pi-specific requirements (EGLFS, Mesa V3D, gpu_mem) are README documentation items, not v1 requirements.
- [01-01]: JsonParser implemented as stateless namespace — pure functions, trivially testable, no shared state
- [01-01]: WeatherReadings.h has zero Qt dependency in struct definitions — plain C++17 structs, Q_DECLARE_METATYPE added at bottom
- [01-01]: wxdash_lib STATIC library allows both executable and test targets to link the same implementation
- [01-02]: Injectable std::function<qint64()> clock in WeatherDataModel constructor — enables deterministic staleness tests without real-time waits
- [01-02]: clearAllValues emits changed signals only for non-zero fields — avoids spurious signals when clearing an already-cleared model
- [01-02]: Staleness timer starts on first received update (m_hasReceivedUpdate guard) — no false positive at startup
- [01-03]: HttpPoller creates QNAM in start() slot (not constructor) — QNAM must be owned by network thread after moveToThread
- [01-03]: UdpReceiver creates its own QNAM — simpler than sharing across thread boundary for fire-and-forget broadcast requests
- [01-03]: No connection type specified in cross-thread connects — Qt auto-detects QueuedConnection
- [01-03]: DATA-09 — no retry in HttpPoller; 10s poll cadence is the retry mechanism
- [01-03]: DATA-03 — renewal timer fires every 3600s (belt and suspenders, even though 86400s requested)
- [02-01]: OUTPUT_DIRECTORY required in qt_add_qml_module when URI matches executable name to avoid linker collision (wxdash URI creates wxdash/ dir, conflicts with wxdash binary)
- [02-01]: animatedSweep property placed on ShapePath (not PathAngleArc) to avoid Behavior binding pitfall; Binding{} element drives it from root.targetSweep
- [02-01]: KIOSK-03 (last-updated timestamp) intentionally omitted per user decision
- [02-01]: strokeColor ring-style arc (not filled pie wedge) for fitness-tracker-ring aesthetic
- [02-02]: Wind rose implemented as radial bar chart histogram — shows directional frequency over time, more informative than instantaneous pointer for always-on display
- [02-02]: Calm wind readings (windDir=0, windSpeed=0) filtered from wind histogram to avoid false north-heavy data
- [02-02]: Rolling window in WeatherDataModel wind histogram prevents stale data from dominating the rose
- [02-02]: RESOURCE_PREFIX /qt/qml required in qt_add_qml_module when URI matches binary name (Qt 6.5+ loadFromModule path collision)
- [02-02]: ArcGauge value+unit split to two Text elements in Column — prevents text overlap with arc at various window sizes
- [Phase 03-01]: Sparkline ring buffers use plain double arrays (not structs) — value-only history needs no metadata
- [Phase 03-01]: feelsLike sparkline computed inline in applyIssUpdate using same heat-index/wind-chill thresholds as DashboardGrid.qml
- [Phase 03-01]: No sparkline recording on UDP updates — 2.5s cadence would fill 24h ring buffer in 6h; 10s ISS cadence is correct
- [Phase 03]: PurpleAir staleness uses same 30s kStalenessMs threshold, independent m_purpleAirStale flag
- [Phase 03]: kAqiSparklineCapacity=2880 (not kSparklineCapacity) since PurpleAir polls at 30s not 10s
- [Phase 03]: PurpleAirPoller shares networkThread with HttpPoller and UdpReceiver — no new thread
- [Phase 03]: calculateAqi uses 2024 EPA breakpoints: Good threshold 9.0 (not pre-2024 12.0)

### Pending Todos

- Add a README section documenting Pi-specific deployment steps: EGLFS backend selection (eglfs_kms for Pi 4/5), Mesa V3D, gpu_mem setting, mDNS hostname resolution for `weatherlinklive.local.cisien.com` (unusual `.local.cisien.com` pattern — verify Avahi resolves on Pi OS Bookworm, may need fallback to IP).

### Blockers/Concerns

- [Phase 2 resolved]: QML Shape+PathAngleArc approach validated — ArcGauge compiles and the render pipeline is confirmed. No need to benchmark Canvas vs QOpenGLWidget; QtQuick Shapes is the chosen approach.

## Session Continuity

Last session: 2026-03-01
Stopped at: Completed 03-02-PLAN.md — PurpleAir data acquisition: PurpleAirReading struct, calculateAqi (2024 EPA), parsePurpleAirJson, PurpleAirPoller, WeatherDataModel aqi/pm25/pm10/purpleAirStale properties, main.cpp wiring
Resume file: None
