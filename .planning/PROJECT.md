# wxdash — Real-Time Weather Dashboard

## What This Is

A full-screen Qt 6 (C++) weather dashboard for Raspberry Pi kiosks that displays real-time atmospheric data from a Davis Instruments WeatherLink Live station via its local HTTP/UDP API. Gauge-based UI with sparkline trends, compass rose wind direction, and color-coded threshold alerts. Designed for 720p minimum resolution with responsive scaling.

## Core Value

Display live weather conditions from the WeatherLink Live with real-time updates — always-on, always-current, at a glance.

## Requirements

### Validated

- ✓ Connect to WeatherLink Live HTTP API for initial current conditions — v1.0
- ✓ Start and maintain UDP real-time broadcast (2.5s wind/rain updates) — v1.0
- ✓ Poll HTTP current_conditions every ~10s for temp, humidity, pressure, solar, UV — v1.0
- ✓ Gauge displays for temperature, humidity, barometric pressure, solar radiation, UV index, wind speed, rain rate/amount — v1.0
- ✓ Compass rose for wind direction — v1.0
- ✓ Sparkline mini-trend graphs showing recent hours of data — v1.0
- ✓ Color threshold indicators on gauges for extreme/concerning conditions — v1.0
- ✓ Full-screen kiosk mode (no window chrome) — v1.0
- ✓ 720p target resolution with responsive scaling to larger displays — v1.0
- ✓ Smart layout prioritizing weather data by importance — v1.0
- ✓ PurpleAir AQI integration with EPA color zones — v1.0
- ✓ CMake install-kiosk one-command Pi deployment — v1.0
- ✓ NWS 3-day forecast panel with weather icons — v1.0

### Active

- [ ] Calm wind indicator (small center dot) replaces direction needle when wind is idle
- [ ] Idle/calm samples tracked in rolling window but render no bars
- [ ] Bar color reflects recent ~60s average speed per direction bin
- [ ] Wind speed color thresholds consistent between CompassRose and ArcGauge
- [ ] Forecast weather icons scaled to 95% of cell (up from 60%)

### Out of Scope

- Metric unit conversion — imperial only (°F, mph, inHg)
- Full scrollable history charts — sparklines only
- Text-based alert labels — color thresholds only
- Remote/cloud API access — local network only
- Touch interaction — display-only kiosk
- Multi-station support — single WeatherLink Live

## Context

**API Details:**
- Current conditions endpoint: `http://weatherlinklive.local.cisien.com/v1/current_conditions`
- Real-time broadcast: Hit `/v1/real_time?duration=N` to start, listen UDP port 22222
- API docs: https://weatherlink.github.io/weatherlink-live-local-api/
- No authentication required
- Device and client must be on same local network

**Data Architecture:**
- Data structure type 1 (ISS): temperature, humidity, dew point, heat index, wind chill, wind speed/direction, rain, solar radiation, UV index
- Data structure type 3 (LSS BAR): barometric pressure (sea level), pressure trend
- Data structure type 4 (LSS Temp/Hum): indoor temp/humidity from console
- UDP real-time only broadcasts wind and rain data at 2.5s intervals
- HTTP current_conditions provides all sensor data, pollable every 10s

**Units (imperial, raw from API):**
- Temperature: °F
- Humidity: %RH
- Wind speed: mph
- Wind direction: degrees (0-360)
- Pressure: inches Hg
- Solar radiation: W/m²
- UV: index value
- Rain: counts (collector-size dependent, rain_size field indicates cup size)

**Rain cup sizes (rain_size field):**
- 1 = 0.01 inches
- 2 = 0.2 mm
- 3 = 0.1 mm
- 4 = 0.001 inches

## Constraints

- **Hardware**: Raspberry Pi (ARM SBC, limited GPU/memory) — must be lightweight
- **Framework**: Qt 6, C++ — native performance required for smooth gauge rendering
- **Display**: Full-screen kiosk, 720p minimum, scales up gracefully
- **Network**: WeatherLink Live accessible on local network via mDNS hostname
- **Update cadence**: Wind/rain at 2.5s (UDP), all other sensors at ~10s (HTTP)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| C++ over Python/PySide6 | Pi has limited resources; native C++ gives best gauge rendering performance | — Pending |
| Imperial units only | API returns imperial; user doesn't need conversion | — Pending |
| Sparklines over full charts | Keeps UI focused on current conditions; few hours of context is enough | — Pending |
| Compass rose for wind | Traditional, intuitive for weather data | — Pending |
| Color thresholds (no text alerts) | Subtle visual cues without cluttering the dashboard | — Pending |
| Mixed update cadence | UDP for fast wind/rain, HTTP poll for everything else — matches API design | — Pending |

---
## Current Milestone: v1.1 Wind Rose Refinement

**Goal:** Refine the wind rose to handle calm conditions gracefully, use recent-average coloring, and track idle samples in the rolling window.

**Target features:**
- Calm wind indicator (circle/neutral) replacing the direction needle when wind is idle
- Idle sample tracking in rolling window (participate in eviction, no bars rendered)
- Bar color based on recent ~60-second average speed per direction bin
- Color consistency maintained between CompassRose and ArcGauge
- Forecast weather icons scaled to 95% of cell

## Current State

Shipped v1.0 with 2,955 LOC (C++/QML). Tech stack: Qt 6, C++17, QML, CMake/Ninja.
12-cell dashboard with real-time weather gauges, sparkline trends, AQI integration, NWS forecast, and Pi kiosk deployment.

*Last updated: 2026-03-02 after v1.1 milestone started*
