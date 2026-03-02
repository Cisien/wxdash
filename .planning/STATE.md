---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: unknown
last_updated: "2026-03-02T00:50:58.638Z"
progress:
  total_phases: 5
  completed_phases: 5
  total_plans: 12
  completed_plans: 12
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-01)

**Core value:** Display live weather conditions from the WeatherLink Live with real-time updates — always-on, always-current, at a glance.
**Current focus:** Phase 6 in progress — 3-day forecast backend complete (Plan 06-01), QML ForecastPanel pending (Plan 06-02)

## Current Position

Phase: 6 of 6 (3-day Forecast Panel with Weather Icons, High/Low Temps, and Precipitation Chance) — IN PROGRESS
Plan: 1 of 2 complete (06-01 complete, 06-02 pending)
Status: Plan 06-01 complete — NwsPoller, parseForecast, forecastData Q_PROPERTY, 17 SVG weather icons bundled
Last activity: 2026-03-01 — Completed Plan 06-01: NWS forecast backend

Progress: [█░░░░░░░░░] 50% of Phase 6 plans complete (1 of 2)

## Performance Metrics

**Velocity:**
- Total plans completed: 6
- Average duration: 8.2 min
- Total execution time: 54 min

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| Phase 1: Data Model and Network Layer | 3 | 11 min | 3.7 min |
| Phase 2: Core Gauges and Dashboard Layout | 2 | 38 min | 19 min |
| Phase 3: Trends, Secondary Data, Air Quality | 3 | 14 min | 4.7 min |

**Recent Trend:**
- Last 5 plans: 5 min, 5 min, 4 min, 5 min, 3 min
- Trend: All plans fast and well-scoped

*Updated after each plan completion*
| Phase 03 P03 | 4 | 1 tasks | 3 files |
| Phase 05 P01 | 3 | 2 tasks | 5 files |
| Phase 05 P02 | 12 | 2 tasks | 3 files |
| Phase 06 P01 | 5 | 2 tasks | 21 files |

## Accumulated Context

### Roadmap Evolution

- Phase 5 added: CMake install-kiosk target for Raspberry Pi deployment
- Phase 6 added: 3-day forecast panel with weather icons, high/low temps, and precipitation chance

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
- [Phase 03-03]: Single Shape with 6 ShapePaths for multi-ring AQI gauge — Qt recommended pattern for best GPU performance
- [Phase 03-03]: AQI ring gets 1/2 stroke width (outer prominence), PM2.5 and PM10 each get 1/4 (visual hierarchy)
- [Phase 03-03]: GAUG-13 (indoor panel) deferred — not implemented in Phase 3 per user decision
- [Phase 05-01]: Manual install(DIRECTORY) for QML module replaces qt_generate_deploy_qml_app_script — Qt's generic deploy tool RPATH-patches system plugins (kimg_ani.so) that have no ELF RPATH entry, causing fatal install error on Linux desktop; Pi uses system Qt so only user QML module needs shipping
- [Phase 05-01]: QML_IMPORT_PATH=@CMAKE_INSTALL_PREFIX@/qml in systemd service — QML module installs to prefix/qml/wxdash/, one level up from bin/, requiring explicit import path for runtime resolution
- [Phase 05-01]: COMPONENT kiosk on all install() rules — enables selective `cmake --install build --component kiosk` without Qt system library bundling
- [Phase 05]: SERVICE_USER defaults to $ENV{USER} at CMake configure time — correct for any Pi deployment user, not hardcoded to pi; overridable with -DSERVICE_USER=<name>
- [Phase 05]: Dropped Group= from systemd service template — SupplementaryGroups covers all required DRM/input group memberships; explicit Group= is redundant and potentially confusing
- [Phase 06-01]: NwsPoller shares existing networkThread (no new thread) — same pattern as PurpleAirPoller
- [Phase 06-01]: Forecast retained on API failure with no staleness clearing — forecast ages gracefully unlike real-time readings
- [Phase 06-01]: ForecastDay.high=-999 sentinel for tonight-only edge case (afternoon fetch) — QML checks for this to display '--'

### Pending Todos

Phase 6 Plan 02 — ForecastPanel QML component consuming weatherModel.forecastData and /icons/weather/ SVG resources

### Blockers/Concerns

- [Phase 2 resolved]: QML Shape+PathAngleArc approach validated — ArcGauge compiles and the render pipeline is confirmed. No need to benchmark Canvas vs QOpenGLWidget; QtQuick Shapes is the chosen approach.

## Session Continuity

Last session: 2026-03-01
Stopped at: Completed 06-01-PLAN.md — NWS forecast backend with NwsPoller, parseForecast, forecastData model property, and 17 SVG weather icons
Resume file: None
