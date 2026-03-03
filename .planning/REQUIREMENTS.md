# Requirements: wxdash

**Defined:** 2026-03-01
**Core Value:** Display live weather and air quality conditions with real-time updates — always-on, always-current, at a glance.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Data Acquisition

- [x] **DATA-01**: App polls WeatherLink Live HTTP API (`/v1/current_conditions`) every 10s for all sensor data
- [x] **DATA-02**: App starts UDP real-time broadcast (`/v1/real_time?duration=86400`) and listens on port 22222 for 2.5s wind/rain updates
- [x] **DATA-03**: App automatically renews UDP broadcast session before expiry
- [x] **DATA-04**: JSON parser routes by `data_structure_type` (1=ISS outdoor, 3=barometer, 4=indoor temp/hum)
- [x] **DATA-05**: Rain counts converted to inches using `rain_size` field (1=0.01in, 2=0.2mm, 3=0.1mm, 4=0.001in)
- [x] **DATA-06**: App polls PurpleAir sensor (`http://10.1.255.41/json?live=false`) for PM2.5 data
- [x] **DATA-07**: PurpleAir channels A and B are averaged (PM2.5 averaged, then AQI calculated from average via EPA breakpoint table)
- [x] **DATA-08**: Data staleness detected and signaled when no update received for >30s (per source)
- [x] **DATA-09**: App handles network disconnect/reconnect gracefully with automatic retry

### Gauges & Display

- [x] **GAUG-01**: Temperature gauge with color thresholds
- [x] **GAUG-02**: Humidity gauge
- [x] **GAUG-03**: Barometric pressure gauge with trend arrow (rising/falling/steady)
- [x] **GAUG-04**: Wind speed gauge with gust readout
- [x] **GAUG-05**: Compass rose for wind direction
- [x] **GAUG-06**: Rain rate gauge + daily accumulation display
- [x] **GAUG-07**: UV Index gauge with EPA 5-zone color coding (Green 0-2, Yellow 3-5, Orange 6-7, Red 8-10, Violet 11+)
- [x] **GAUG-08**: Solar radiation numeric display
- [x] **GAUG-09**: Feels-like display (heat index when temp>=80F+RH>=40%; wind chill when temp<=50F+wind>=3mph)
- [x] **GAUG-10**: Dew point display
- [x] **GAUG-11**: AQI gauge with EPA 6-zone color coding (Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+)
- [x] **GAUG-12**: PM2.5 gauge showing averaged A+B sensor value
- [ ] **GAUG-13**: Indoor temperature and humidity panel (WeatherLink type 4 data)
- [x] **GAUG-14**: Animated gauge needle transitions (smooth movement between values)

### Trends

- [x] **TRND-01**: Sparkline mini-graphs showing last few hours of data for key sensors
- [x] **TRND-02**: In-memory ring buffer storage for sparkline history (10s cadence)

### Kiosk & Layout

- [x] **KIOSK-01**: Full-screen frameless window (via `-platform eglfs` on embedded Linux, or fullscreen window flag on desktop Linux)
- [x] **KIOSK-02**: Responsive layout targeting 720p, scales to larger displays without code changes
- [ ] **KIOSK-03**: ~~Last-updated timestamp visible on dashboard~~ (Dropped per user decision)
- [ ] **KIOSK-04**: Connection/staleness status indicator per data source
- [x] **KIOSK-05**: systemd watchdog with auto-restart on crash

## v1.1 Requirements

Requirements for Wind Rose Refinement milestone. Each maps to roadmap phases.

### Wind Rose

- [ ] **WIND-01**: Direction needle becomes a neutral circle indicator when wind speed is zero (no directional bias)
- [ ] **WIND-02**: Idle/calm readings are tracked as samples in the 720-entry rolling window and participate in eviction, but render no bar in any direction bin
- [ ] **WIND-03**: Bar color for each direction bin reflects the rolling average wind speed from the last ~60 seconds of samples in that bin (not the lifetime average)
- [ ] **WIND-04**: Wind speed color thresholds in CompassRose match the ArcGauge windSpeedColor function (verify and maintain)

### Forecast

- [ ] **FCST-01**: Forecast weather icons scaled to 95% of cell size (up from current 60%)

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Display Enhancements

- **DISP-01**: Pixel-shift burn-in prevention (2px shift every 10 min)
- **DISP-02**: Screen dimming schedule for nighttime
- **DISP-03**: Configurable color thresholds via config file
- **DISP-04**: Wind gust dedicated compass ring

### Unit Support

- **UNIT-01**: User-toggleable imperial/metric units

## Out of Scope

| Feature | Reason |
|---------|--------|
| Metric unit conversion | Imperial only — API returns imperial, no conversion needed |
| Weather forecast display | Requires external API, contradicts local-only design |
| Multi-station support | Single WeatherLink Live, done well |
| Touch interaction | Display-only kiosk, touch introduces settings drift |
| Cloud/remote API access | Local network only |
| Full scrollable history charts | Sparklines sufficient for at-a-glance use |
| Text-based alert labels | Color thresholds convey severity without clutter |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DATA-01 | Phase 1 | Complete |
| DATA-02 | Phase 1 | Complete |
| DATA-03 | Phase 1 | Complete |
| DATA-04 | Phase 1 | Complete (01-01) |
| DATA-05 | Phase 1 | Complete (01-01) |
| DATA-06 | Phase 3 | Complete |
| DATA-07 | Phase 3 | Complete |
| DATA-08 | Phase 1 | Complete |
| DATA-09 | Phase 1 | Complete |
| GAUG-01 | Phase 2 | Complete |
| GAUG-02 | Phase 2 | Complete |
| GAUG-03 | Phase 2 | Complete |
| GAUG-04 | Phase 2 | Complete |
| GAUG-05 | Phase 2 | Complete |
| GAUG-06 | Phase 2 | Complete |
| GAUG-07 | Phase 2 | Complete |
| GAUG-08 | Phase 2 | Complete |
| GAUG-09 | Phase 2 | Complete |
| GAUG-10 | Phase 2 | Complete |
| GAUG-11 | Phase 3 | Complete |
| GAUG-12 | Phase 3 | Complete |
| GAUG-13 | Phase 3 | Pending |
| GAUG-14 | Phase 2 | Complete |
| TRND-01 | Phase 3 | Complete |
| TRND-02 | Phase 3 | Complete |
| KIOSK-01 | Phase 2 | Complete |
| KIOSK-02 | Phase 2 | Complete |
| KIOSK-03 | Phase 2 | Dropped |
| KIOSK-04 | Phase 4 | Pending |
| KIOSK-05 | Phase 4 | Complete |

**v1 Coverage:**
- v1 requirements: 30 total
- Mapped to phases: 30
- Unmapped: 0

**v1.1 Coverage:**
- v1.1 requirements: 5 total
- Mapped to phases: 0
- Unmapped: 5 ⚠️

---
*Requirements defined: 2026-03-01*
*Last updated: 2026-03-02 — v1.1 Wind Rose Refinement requirements added*
