# Roadmap: wxdash

## Milestones

- v1.0 MVP - Phases 1-6 (shipped 2026-03-02)
- v1.1 Wind Rose Refinement - Phases 7-8 (in progress)

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3...): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

<details>
<summary>v1.0 MVP (Phases 1-6) - SHIPPED 2026-03-02</summary>

- [x] **Phase 1: Data Model and Network Layer** - HTTP polling, UDP real-time feed, JSON parsing, and central WeatherDataModel
- [x] **Phase 2: Core Gauges and Dashboard Layout** - All outdoor weather gauges, compass rose, fullscreen frameless window, and responsive layout
- [x] **Phase 3: Trends, Secondary Data, and Air Quality** - Sparklines, PurpleAir AQI gauges, and animated needles
- [x] **Phase 4: Kiosk Hardening and Deployment** - systemd watchdog and staleness detection
- [x] **Phase 5: CMake install-kiosk target** - One-command Pi deployment with systemd auto-start
- [x] **Phase 6: 3-Day Forecast Panel** - NWS forecast with weather icons, high/low temps, precipitation

### Phase 1: Data Model and Network Layer
**Goal**: Live weather data from WeatherLink Live HTTP and UDP feeds flows into a central in-memory model, correctly parsed and tested, ready for widgets to consume
**Depends on**: Nothing (first phase)
**Requirements**: DATA-01, DATA-02, DATA-03, DATA-04, DATA-05, DATA-08, DATA-09
**Plans**: 3 plans

Plans:
- [x] 01-01-PLAN.md -- Project scaffolding, WeatherReadings structs, and JsonParser (TDD)
- [x] 01-02-PLAN.md -- WeatherDataModel with staleness detection (TDD)
- [x] 01-03-PLAN.md -- HttpPoller, UdpReceiver, and main.cpp integration wiring

### Phase 2: Core Gauges and Dashboard Layout
**Goal**: Users see all outdoor weather conditions at a glance on a full-screen, frameless, responsive dashboard with color-coded thresholds and real-time updates
**Depends on**: Phase 1
**Requirements**: GAUG-01, GAUG-02, GAUG-03, GAUG-04, GAUG-05, GAUG-06, GAUG-07, GAUG-08, GAUG-09, GAUG-10, GAUG-14, KIOSK-01, KIOSK-02, KIOSK-03
**Plans**: 2 plans

Plans:
- [x] 02-01-PLAN.md -- ArcGauge component and gauge cells
- [x] 02-02-PLAN.md -- CompassRose, DashboardGrid layout, fullscreen wiring

### Phase 3: Trends, Secondary Data, and Air Quality
**Goal**: Users see recent trend context via sparklines, indoor conditions, and air quality alongside the core weather display
**Depends on**: Phase 2
**Requirements**: DATA-06, DATA-07, GAUG-11, GAUG-12, GAUG-13, TRND-01, TRND-02
**Plans**: 3 plans

Plans:
- [x] 03-01-PLAN.md -- Sparkline component and ring buffer history
- [x] 03-02-PLAN.md -- PurpleAir poller, AQI calculation, gauge integration
- [x] 03-03-PLAN.md -- Animated needle transitions

### Phase 4: Kiosk Hardening and Deployment
**Goal**: The dashboard runs unattended and recovers automatically from crashes
**Depends on**: Phase 3
**Requirements**: KIOSK-04, KIOSK-05
**Plans**: 0 plans (cancelled -- KIOSK-04 dropped, KIOSK-05 moved to Phase 5)

### Phase 5: CMake install-kiosk target
**Goal**: One-command Pi kiosk deployment with systemd auto-start
**Depends on**: Phase 4
**Requirements**: KIOSK-05
**Plans**: 2 plans

Plans:
- [x] 05-01-PLAN.md -- CMake install() rules, systemd service template, eglfs.json
- [x] 05-02-PLAN.md -- README deployment documentation and human verification

### Phase 6: 3-Day Forecast Panel
**Goal**: Users see a 3-day weather forecast with weather icons, high/low temps, and precipitation chance
**Depends on**: Phase 5
**Requirements**: (new feature, no pre-existing requirement IDs)
**Plans**: 2 plans

Plans:
- [x] 06-01-PLAN.md -- NWS poller, forecast parser, model properties, SVG icon assets, unit tests
- [x] 06-02-PLAN.md -- ForecastPanel QML component, DashboardGrid wiring, human verification

</details>

### v1.1 Wind Rose Refinement (In Progress)

**Milestone Goal:** Refine the wind rose to handle calm conditions gracefully, use recent-average coloring for direction bars, maintain color consistency with the wind speed gauge, and scale forecast icons to fill their cells.

- [ ] **Phase 7: Wind Rose Calm and Color** - Calm indicator, idle sample tracking, recent-average bar coloring, and color threshold consistency
- [ ] **Phase 8: Forecast Icon Scaling** - Scale forecast weather icons to 95% of cell size

## Phase Details

### Phase 7: Wind Rose Calm and Color
**Goal**: The compass rose gracefully handles calm/idle wind conditions and colors direction bars by recent wind speed, consistent with the wind speed gauge
**Depends on**: Phase 6
**Requirements**: WIND-01, WIND-02, WIND-03, WIND-04
**Success Criteria** (what must be TRUE):
  1. When wind speed is zero, the compass rose displays a small center dot (smaller than the minimum bar length) instead of a directional needle (no false directional bias)
  2. Idle/calm readings (zero wind speed) occupy slots in the 720-entry rolling window and evict old samples normally, but no bar appears in any direction bin for those samples
  3. Each direction bar's color reflects the average wind speed from that bin's samples in the last ~60 seconds (not the lifetime average of the entire rolling window)
  4. Wind speed color thresholds in CompassRose produce the same colors as the ArcGauge windSpeedColor function for any given speed value
**Plans**: TBD

### Phase 8: Forecast Icon Scaling
**Goal**: Forecast weather icons fill their cells for better visibility on the kiosk display
**Depends on**: Phase 6 (no dependency on Phase 7)
**Requirements**: FCST-01
**Success Criteria** (what must be TRUE):
  1. Forecast weather icons render at 95% of their cell size (up from 60%)
  2. Icons remain centered and do not clip or overflow their cell boundaries
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 7 -> 8

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Data Model and Network Layer | v1.0 | 3/3 | Complete | 2026-03-01 |
| 2. Core Gauges and Dashboard Layout | v1.0 | 2/2 | Complete | 2026-03-01 |
| 3. Trends, Secondary Data, and Air Quality | v1.0 | 3/3 | Complete | 2026-03-01 |
| 4. Kiosk Hardening and Deployment | v1.0 | 0/0 | Complete | 2026-03-01 |
| 5. CMake install-kiosk target | v1.0 | 2/2 | Complete | 2026-03-01 |
| 6. 3-Day Forecast Panel | v1.0 | 2/2 | Complete | 2026-03-02 |
| 7. Wind Rose Calm and Color | v1.1 | 0/TBD | Not started | - |
| 8. Forecast Icon Scaling | v1.1 | 0/TBD | Not started | - |
