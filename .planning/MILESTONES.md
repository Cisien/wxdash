# Milestones

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

### Known Gaps
- **GAUG-13**: Indoor temperature and humidity panel (deferred per user decision)
- **KIOSK-03**: Last-updated timestamp (dropped per user decision)
- **KIOSK-04**: Connection/staleness status indicator (not implemented)

---

