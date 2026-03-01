# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-01)

**Core value:** Display live weather conditions from the WeatherLink Live with real-time updates — always-on, always-current, at a glance.
**Current focus:** Phase 1 — Data Model and Network Layer

## Current Position

Phase: 1 of 4 (Data Model and Network Layer)
Plan: 3 of 3 in current phase
Status: Phase 1 complete
Last activity: 2026-03-01 — Completed Plan 01-03: HttpPoller, UdpReceiver, and main.cpp wiring

Progress: [███░░░░░░░] 25%

## Performance Metrics

**Velocity:**
- Total plans completed: 3
- Average duration: 3.7 min
- Total execution time: 11 min

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| Phase 1: Data Model and Network Layer | 3 | 11 min | 3.7 min |

**Recent Trend:**
- Last 5 plans: 6 min, 3 min, 2 min
- Trend: Accelerating

*Updated after each plan completion*

## Accumulated Context

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

### Pending Todos

- Add a README section documenting Pi-specific deployment steps: EGLFS backend selection (eglfs_kms for Pi 4/5), Mesa V3D, gpu_mem setting, mDNS hostname resolution for `weatherlinklive.local.cisien.com` (unusual `.local.cisien.com` pattern — verify Avahi resolves on Pi OS Bookworm, may need fallback to IP).

### Blockers/Concerns

- [Phase 2]: QML Canvas rendering performance for anti-aliased arc gauges at 2.5s refresh is unvalidated on all targets — benchmark both QML Canvas and QOpenGLWidget at start of Phase 2 before committing to one approach.

## Session Continuity

Last session: 2026-03-01
Stopped at: Completed 01-03-PLAN.md — HttpPoller, UdpReceiver, and main.cpp wiring (Phase 1 complete)
Resume file: None
