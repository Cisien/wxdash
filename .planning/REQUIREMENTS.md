# Requirements: wxdash

**Defined:** 2026-03-01
**Core Value:** Display live weather and air quality conditions with real-time updates — always-on, always-current, at a glance.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Data Acquisition

- [ ] **DATA-01**: App polls WeatherLink Live HTTP API (`/v1/current_conditions`) every 10s for all sensor data
- [ ] **DATA-02**: App starts UDP real-time broadcast (`/v1/real_time?duration=86400`) and listens on port 22222 for 2.5s wind/rain updates
- [ ] **DATA-03**: App automatically renews UDP broadcast session before expiry
- [ ] **DATA-04**: JSON parser routes by `data_structure_type` (1=ISS outdoor, 3=barometer, 4=indoor temp/hum)
- [ ] **DATA-05**: Rain counts converted to inches using `rain_size` field (1=0.01in, 2=0.2mm, 3=0.1mm, 4=0.001in)
- [ ] **DATA-06**: App polls PurpleAir sensor (`http://10.1.255.41/json?live=false`) for PM2.5 data
- [ ] **DATA-07**: PurpleAir channels A and B are averaged (PM2.5 averaged, then AQI calculated from average via EPA breakpoint table)
- [ ] **DATA-08**: Data staleness detected and signaled when no update received for >30s (per source)
- [ ] **DATA-09**: App handles network disconnect/reconnect gracefully with automatic retry

### Gauges & Display

- [ ] **GAUG-01**: Temperature gauge with color thresholds
- [ ] **GAUG-02**: Humidity gauge
- [ ] **GAUG-03**: Barometric pressure gauge with trend arrow (rising/falling/steady)
- [ ] **GAUG-04**: Wind speed gauge with gust readout
- [ ] **GAUG-05**: Compass rose for wind direction
- [ ] **GAUG-06**: Rain rate gauge + daily accumulation display
- [ ] **GAUG-07**: UV Index gauge with EPA 5-zone color coding (Green 0-2, Yellow 3-5, Orange 6-7, Red 8-10, Violet 11+)
- [ ] **GAUG-08**: Solar radiation numeric display
- [ ] **GAUG-09**: Feels-like display (heat index when temp>=80F+RH>=40%; wind chill when temp<=50F+wind>=3mph)
- [ ] **GAUG-10**: Dew point display
- [ ] **GAUG-11**: AQI gauge with EPA 6-zone color coding (Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+)
- [ ] **GAUG-12**: PM2.5 gauge showing averaged A+B sensor value
- [ ] **GAUG-13**: Indoor temperature and humidity panel (WeatherLink type 4 data)
- [ ] **GAUG-14**: Animated gauge needle transitions (smooth movement between values)

### Trends

- [ ] **TRND-01**: Sparkline mini-graphs showing last few hours of data for key sensors
- [ ] **TRND-02**: In-memory ring buffer storage for sparkline history (10s cadence)

### Kiosk & Layout

- [ ] **KIOSK-01**: Full-screen frameless window via EGLFS
- [ ] **KIOSK-02**: Responsive layout targeting 720p, scales to larger displays
- [ ] **KIOSK-03**: Last-updated timestamp visible on dashboard
- [ ] **KIOSK-04**: Connection/staleness status indicator per data source
- [ ] **KIOSK-05**: systemd watchdog with auto-restart on crash

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
| DATA-01 | — | Pending |
| DATA-02 | — | Pending |
| DATA-03 | — | Pending |
| DATA-04 | — | Pending |
| DATA-05 | — | Pending |
| DATA-06 | — | Pending |
| DATA-07 | — | Pending |
| DATA-08 | — | Pending |
| DATA-09 | — | Pending |
| GAUG-01 | — | Pending |
| GAUG-02 | — | Pending |
| GAUG-03 | — | Pending |
| GAUG-04 | — | Pending |
| GAUG-05 | — | Pending |
| GAUG-06 | — | Pending |
| GAUG-07 | — | Pending |
| GAUG-08 | — | Pending |
| GAUG-09 | — | Pending |
| GAUG-10 | — | Pending |
| GAUG-11 | — | Pending |
| GAUG-12 | — | Pending |
| GAUG-13 | — | Pending |
| GAUG-14 | — | Pending |
| TRND-01 | — | Pending |
| TRND-02 | — | Pending |
| KIOSK-01 | — | Pending |
| KIOSK-02 | — | Pending |
| KIOSK-03 | — | Pending |
| KIOSK-04 | — | Pending |
| KIOSK-05 | — | Pending |

**Coverage:**
- v1 requirements: 30 total
- Mapped to phases: 0
- Unmapped: 30

---
*Requirements defined: 2026-03-01*
*Last updated: 2026-03-01 after initial definition*
