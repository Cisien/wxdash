# Roadmap: wxdash

## Milestones

- v1.0 MVP - Phases 1-6 (shipped 2026-03-02)
- v1.1 Wind Rose Refinement - Phases 7-8 (shipped 2026-03-03)

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

</details>

<details>
<summary>v1.1 Wind Rose Refinement (Phases 7-8) - SHIPPED 2026-03-03</summary>

- [x] **Phase 7: Wind Rose Calm and Color** - Calm indicator, idle sample tracking, recent-average bar coloring, and color threshold consistency
- [x] **Phase 8: Forecast Icon Scaling** - Scale forecast weather icons to 95% of cell size

</details>

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Data Model and Network Layer | v1.0 | 3/3 | Complete | 2026-03-01 |
| 2. Core Gauges and Dashboard Layout | v1.0 | 2/2 | Complete | 2026-03-01 |
| 3. Trends, Secondary Data, and Air Quality | v1.0 | 3/3 | Complete | 2026-03-01 |
| 4. Kiosk Hardening and Deployment | v1.0 | 0/0 | Complete | 2026-03-01 |
| 5. CMake install-kiosk target | v1.0 | 2/2 | Complete | 2026-03-01 |
| 6. 3-Day Forecast Panel | v1.0 | 2/2 | Complete | 2026-03-02 |
| 7. Wind Rose Calm and Color | v1.1 | 2/2 | Complete | 2026-03-03 |
| 8. Forecast Icon Scaling | v1.1 | 0/0 | Complete | 2026-03-03 |
