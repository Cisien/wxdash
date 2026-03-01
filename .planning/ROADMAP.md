# Roadmap: wxdash

## Overview

Build a Qt 6 C++ weather dashboard for kiosk display in four phases. The app targets Linux broadly (developed and tested on a local Linux machine; Raspberry Pi is a supported deployment target). Phase 1 builds the network and data model layer that everything else depends on. Phase 2 delivers all core outdoor weather gauges, fullscreen layout, and responsive scaling. Phase 3 adds sparkline trends, indoor panel, and air quality display. Phase 4 hardens the app for unattended 24/7 kiosk operation.

Pi-specific deployment notes (EGLFS, Mesa V3D, gpu_mem, eglfs_kms, native vs. cross-compilation) are documented in the project README, not tracked as build validation phases.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3, 4): Planned milestone work
- Decimal phases (1.1, 2.1): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Data Model and Network Layer** - HTTP polling, UDP real-time feed, JSON parsing, and central WeatherDataModel
- [x] **Phase 2: Core Gauges and Dashboard Layout** - All outdoor weather gauges, compass rose, fullscreen frameless window, and responsive layout (completed 2026-03-01)
- [x] **Phase 3: Trends, Secondary Data, and Air Quality** - Sparklines, indoor panel, PurpleAir AQI gauges, and animated needles (completed 2026-03-01)
- [ ] **Phase 4: Kiosk Hardening and Deployment** - systemd watchdog, staleness indicators, and 24-hour stability validation

## Phase Details

### Phase 1: Data Model and Network Layer
**Goal**: Live weather data from WeatherLink Live HTTP and UDP feeds flows into a central in-memory model, correctly parsed and tested, ready for widgets to consume
**Depends on**: Nothing (first phase)
**Requirements**: DATA-01, DATA-02, DATA-03, DATA-04, DATA-05, DATA-08, DATA-09
**Success Criteria** (what must be TRUE):
  1. The app polls `/v1/current_conditions` every 10s and receives outdoor ISS, barometer, and indoor sensor data
  2. UDP real-time packets arrive at 2.5s intervals; the session auto-renews before expiry with no manual intervention
  3. Rain counts are converted to inches correctly for all four rain_size values (verified by unit test against known counts)
  4. Staleness is detected within 30s of a data source going silent and a stale signal is emitted
  5. After a network disconnect, the app reconnects automatically without requiring a restart
**Plans**: 3 plans
  - [x] 01-01-PLAN.md — Project scaffolding, WeatherReadings structs, and JsonParser (TDD)
  - [x] 01-02-PLAN.md — WeatherDataModel with staleness detection (TDD)
  - [x] 01-03-PLAN.md — HttpPoller, UdpReceiver, and main.cpp integration wiring

### Phase 2: Core Gauges and Dashboard Layout
**Goal**: Users see all outdoor weather conditions at a glance on a full-screen, frameless, responsive dashboard with color-coded thresholds and real-time updates — running on any Linux system
**Depends on**: Phase 1
**Requirements**: GAUG-01, GAUG-02, GAUG-03, GAUG-04, GAUG-05, GAUG-06, GAUG-07, GAUG-08, GAUG-09, GAUG-10, GAUG-14, KIOSK-01, KIOSK-02, KIOSK-03
**Success Criteria** (what must be TRUE):
  1. Temperature, humidity, barometric pressure, wind speed, UV index, solar radiation, rain rate, dew point, and feels-like are all visible simultaneously on one screen
  2. The app launches fullscreen and frameless with no window chrome on any Linux desktop or embedded target
  3. The layout fills the display at 720p and scales correctly to larger resolutions without code changes
  4. The compass rose shows current wind direction and updates within 2.5 seconds of a new UDP packet
  5. UV Index gauge shows the correct EPA 5-zone color (green/yellow/orange/red/violet) for the current value
  6. Barometric pressure gauge displays a trend arrow (rising/falling/steady) that matches the API pressure_trend field
  7. A last-updated timestamp is visible and updates with each successful data refresh
**Plans**: TBD

### Phase 3: Trends, Secondary Data, and Air Quality
**Goal**: Users see recent trend context via sparklines, indoor conditions, and air quality alongside the core weather display
**Depends on**: Phase 2
**Requirements**: DATA-06, DATA-07, GAUG-11, GAUG-12, GAUG-13, TRND-01, TRND-02
**Success Criteria** (what must be TRUE):
  1. Sparkline mini-graphs for key sensors (temperature, pressure, humidity, wind speed) display after a few minutes of data accumulation and update continuously
  2. The AQI gauge shows the correct EPA 6-zone color (green/yellow/orange/red/purple/maroon) calculated from averaged PurpleAir A+B PM2.5 channels
  3. Indoor temperature and humidity from the WeatherLink console are visible in a separate panel
  4. PM2.5 gauge shows the averaged A+B channel value from PurpleAir
**Plans**: TBD

### Phase 4: Kiosk Hardening and Deployment
**Goal**: The dashboard runs unattended and recovers automatically from crashes, with connection status visible so a viewer knows when data may be stale
**Depends on**: Phase 3
**Requirements**: KIOSK-04, KIOSK-05
**Success Criteria** (what must be TRUE):
  1. A per-source connection/staleness indicator is visible on the dashboard and turns to a stale state when a data source has not updated for 30s
  2. After an intentional app crash, systemd restarts the application within 10 seconds without human intervention
  3. The app runs continuously for 24 hours with flat memory usage (no RSS growth) and CPU below 20%
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Data Model and Network Layer | 3/3 | Complete | 2026-03-01 |
| 2. Core Gauges and Dashboard Layout | 2/2 | Complete   | 2026-03-01 |
| 3. Trends, Secondary Data, and Air Quality | 3/3 | Complete   | 2026-03-01 |
| 4. Kiosk Hardening and Deployment | 0/TBD | Not started | - |
| 5. CMake install-kiosk target | 1/2 | In Progress|  |

### Phase 5: CMake install-kiosk target for Raspberry Pi deployment

**Goal:** `cmake --install build --component kiosk` deploys the wxdash binary, QML module, systemd service, EGLFS config, and desktop files to FHS-compliant paths, enabling one-command Pi kiosk deployment with auto-start at boot
**Requirements**: KIOSK-05
**Depends on:** Phase 4
**Success Criteria** (what must be TRUE):
  1. `cmake --install build --component kiosk` places the binary, QML module, systemd service, eglfs.json, desktop files, and icon at correct FHS paths
  2. The installed systemd service file has correct ExecStart path and EGLFS environment variables
  3. A README documents the complete Pi deployment workflow from build through systemctl enable
  4. The install-kiosk convenience target exists for one-command deploy + enable
**Plans**: 2 plans

Plans:
- [ ] 05-01-PLAN.md — CMake install() rules, systemd service template, eglfs.json, desktop entry template
- [ ] 05-02-PLAN.md — README deployment documentation and human verification
