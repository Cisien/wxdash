# Milestones

## v1.1 Wind Rose Refinement (Shipped: 2026-03-03)

**Phases completed:** 2 phases (7-8), 2 plans + 1 direct change
**Files modified:** 12 (534 insertions, 52 deletions)
**Lines of code:** 2,996 (C++/QML)
**Timeline:** 2026-03-03

**Key accomplishments:**
- Calm wind samples tracked in ring buffer with bin=-1 sentinel (no false directional bars)
- Gold center dot replaces directional needle during calm/idle conditions
- Direction bars colored by recent ~60s average speed per bin (not lifetime average)
- Wind speed color function unified to single source of truth via function property injection
- Forecast icons scaled from 60% to 95% of cell size for kiosk visibility

---

## v1.0 MVP (Shipped: 2026-03-02)

**Phases completed:** 6 phases, 12 plans
**Lines of code:** 2,955 (C++/QML)
**Timeline:** 2026-03-01

**Key accomplishments:**
- WeatherLink Live HTTP/UDP polling pipeline with staleness detection and auto-renewal
- 12-cell gauge dashboard with animated arcs, compass rose, threshold color coding
- 24h sparkline trends on all outdoor gauges via 8640-sample ring buffers
- PurpleAir AQI integration with 3-ring concentric gauge and EPA color zones
- CMake install-kiosk target for one-command Pi deployment with systemd
- NWS 3-day forecast panel with weather icons, color-coded temps, precipitation

### Cancelled Requirements
- **GAUG-13**: Indoor temperature and humidity panel — cancelled
- **KIOSK-03**: Last-updated timestamp — cancelled
- **KIOSK-04**: Connection/staleness status indicator — cancelled

---

