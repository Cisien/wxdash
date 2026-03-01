# Feature Research

**Domain:** Real-time weather dashboard / always-on kiosk display (Davis WeatherLink Live)
**Researched:** 2026-03-01
**Confidence:** MEDIUM-HIGH — table stakes drawn from multiple commercial products and standards bodies; kiosk patterns from Raspberry Pi community; derived-value formulas from NWS/EPA official sources.

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Current outdoor temperature display | Core weather datum; every display shows it | LOW | Raw from API (ISS type 1); imperial only (°F) |
| Current humidity display | Standard since first digital weather consoles | LOW | Raw from API (ISS type 1); %RH |
| Barometric pressure display | Standard since aneroid barometers | LOW | Sea-level pressure from LSS BAR (type 3); inHg |
| Pressure trend indicator (rising/falling/steady) | AcuRite, Davis, La Crosse all show this; users expect it | LOW | API provides pressure_trend field directly; render as arrow icon |
| Wind speed display | Core datum; UDP at 2.5s means it feels "live" | LOW | Raw from API; mph |
| Wind direction (compass rose) | Standard visual for wind direction since nautical charts | MEDIUM | Render as rotatable rose or needle; degrees 0-360 from API |
| Wind gust display | Distinct from average; every station console shows it | LOW | Separate field in API (wind_speed_hi_last_10_min) |
| Rain rate display (current intensity) | Users need to know if it is raining hard right now | LOW | rain_rate_hi from API (counts/hr); must convert using rain_size |
| Rain accumulation (daily total) | Standard; answers "how much has fallen today?" | LOW | rainfall_daily from API; convert counts to inches using rain_size |
| Dew point display | Expected on any serious weather display | LOW | API provides it directly (dew_point field on ISS type 1) |
| UV Index display | Public health awareness; color-coded scale is standard | LOW | API provides uv_index directly; use EPA 5-zone color scale |
| Solar radiation display | Present on WeatherLink Live sensor suite; omitting it wastes the sensor | LOW | sol_radiation in W/m² from API |
| Feels-like / apparent temperature | Weather apps all surface this; users think in "feels like" not dry bulb | MEDIUM | Derive from heat_index (>80°F, RH>40%) or wind_chill (<50°F, wind>3mph); API also provides heat_index and wind_chill directly |
| "Last updated" timestamp | Users need confidence the data is fresh | LOW | Show wall-clock time of last successful read |
| Full-screen / no window chrome | Kiosk context; bezels and title bars waste screen real-estate | LOW | Qt::FramelessWindowHint + showFullScreen() |
| Connection error / stale data indication | If network drops, user must not be shown stale data without warning | MEDIUM | Color wash, icon, or overlay when data age > threshold (e.g., 30s) |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Sparkline trend graphs per sensor | Gives temporal context without cluttering; commercial consoles show 24h graphs but sparklines are compact | MEDIUM | Ring buffer ~6h at 10s intervals (~2160 points per channel); render as QCustomPlot or QPainter polyline |
| Animated gauge needles | Makes data feel alive; smooth needle sweep signals real-time data vs. static screenshot | MEDIUM | Use QPropertyAnimation or custom QPainter with interpolated angle; cap at ~60fps for Pi GPU |
| Color-coded threshold alerts on gauges | Subtle visual communication without text; Davis WeatherLink Console does this for UV | LOW | Define threshold bands per sensor (e.g., UV green/yellow/orange/red/purple per EPA; temp extremes) |
| UDP-driven wind update at 2.5s | Most dashboards poll at 30–60s; 2.5s wind feels genuinely live | MEDIUM | Start UDP broadcast via /v1/real_time endpoint; parse JSON on UDP port 22222; update only wind/rain widgets |
| Indoor temp/humidity panel | API provides LSS Temp/Hum (type 4) from the console sensor; few kiosk displays surface indoor comfort data | LOW | Secondary panel, smaller than outdoor; direct from type 4 data structure |
| Compass rose with cardinal labels | More intuitive than a plain degree number; traditional weather instrument aesthetic | MEDIUM | N/NE/E/SE/S/SW/W/NW labels; needle or rotating rose; 16-point or 8-point |
| Rain cup size-aware unit conversion | Davis uses 4 different cup sizes (rain_size field); naive dashboards get wrong totals | LOW | Lookup table: rain_size 1=0.01in, 2=0.2mm (skip, we're imperial), 3=0.1mm (skip), 4=0.001in |
| Distinct display for wind gust vs average | Gust and average communicate different risk levels; most kiosks show only one | LOW | Show both on wind gauge or as separate labeled readouts |
| Pixel-shift / slow pan burn-in mitigation | OLED and LCD panels burn in with static kiosk content; professional kiosks use subtle drift | LOW | Shift entire scene by ±2px on slow timer (~10 min cycle); imperceptible to users, prevents static phosphor burn |
| Watchdog / auto-restart on crash | Unattended kiosk must self-heal; Raspberry Pi has hardware watchdog enabled by default | LOW | systemd service with Restart=always + WatchdogSec; or write watchdog /dev/watchdog periodically |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Metric unit toggle (°C, km/h, hPa) | International users; flexibility feels professional | API returns imperial raw values; conversion layer adds complexity, doubles QA surface, and the scope is explicitly imperial-only | Keep imperial. Document that metric is out of scope in README. |
| Full scrollable history charts | Curiosity about past data; charting looks impressive | Requires persistent storage, a DB schema, chart zoom/pan interaction (touch-unfriendly on kiosk), and long-term data retention logic | Sparklines showing recent 6h are enough for kiosk context; full history belongs in WeatherLink Cloud |
| Touch-driven interaction / settings UI | Feels modern; tap to configure thresholds | Kiosk is display-only; touch introduces accidental dismissal, settings drift, and accessibility complexity | Config file or env vars for threshold customization; no runtime UI |
| Multi-station support | "What if I add another sensor?" | Single WeatherLink Live is the defined hardware; multi-station multiplies API complexity, layout decisions, and failure modes | One station, done well, is the product |
| Weather forecast display | Users ask for "what's the forecast?" | Forecast requires external API (NWS, OpenWeather), authentication, rate limiting, and a network beyond the local LAN; contradicts local-only design | Out of scope. Display current conditions only. |
| Text alert notifications / push alerts | "Alert me when UV is high" | Notification delivery (email, SMS, push) requires external services, credentials management, and reliability infrastructure; this is a display, not an alerting system | Color thresholds on gauges provide in-situ visual alerting without external services |
| Auto-brightness based on ambient light | Power saving; looks polished | Requires photosensor hardware not on the Pi; DPMS/backlight control is inconsistent across display hardware | Fix screen at full brightness; disable DPMS for kiosk use |
| 5-day forecast panel | Common in weather app UIs | Requires cloud API, violates local-only constraint; forecast data has no bearing on the sensor data being displayed | Show today's accumulations (rain daily, UV dose) instead |

---

## Feature Dependencies

```
[UDP real-time broadcast]
    └──requires──> [HTTP /v1/real_time?duration=N kick-off]
                       └──requires──> [Network layer / API client]

[Wind gust display]
    └──requires──> [API client polling HTTP /v1/current_conditions]

[Sparkline trend graphs]
    └──requires──> [In-memory ring buffer per sensor channel]
                       └──requires──> [HTTP polling loop (10s interval)]

[Feels-like temperature]
    └──requires──> [Temp + Humidity + Wind Speed values available simultaneously]
    └──depends-on──> [Heat index (API) OR wind chill (API) per condition rules]

[Rain accumulation (inches)]
    └──requires──> [rain_size field parsed from ISS type 1 data structure]
    └──requires──> [Rain count conversion lookup table]

[Color threshold alerts]
    └──enhances──> [Any gauge widget]
    └──requires──> [Configurable threshold values per sensor]

[Compass rose]
    └──requires──> [Wind direction degrees from API]

[Pressure trend arrow]
    └──requires──> [pressure_trend field from LSS BAR type 3 data structure]

[UDP wind/rain at 2.5s]
    ──conflicts-with──> [Displaying wind from HTTP poll only (10s is too slow)]
```

### Dependency Notes

- **Rain accumulation requires rain_size:** The WeatherLink Live API returns rain as bucket-tip counts. Without the rain_size field, the displayed inch value will be wrong. Parse rain_size from every ISS type 1 response and propagate it to all rain conversion logic.
- **UDP kick-off requires HTTP first:** The WeatherLink Live local API requires an HTTP GET to `/v1/real_time?duration=N` to start the UDP broadcast. Without this call, port 22222 will be silent. Duration must be renewed before it expires.
- **Feels-like requires both heat index and wind chill logic:** The switch between heat index (hot/humid) and wind chill (cold/windy) must respect the NWS validity conditions. The API provides both fields pre-calculated, which is simpler than recomputing, but the display logic still needs to choose which to show (or show "feels like" as a unified derived field).
- **Sparklines require ring buffer before display:** Ring buffers need to be seeded with initial data before the first sparkline renders. On startup, the first HTTP poll populates the buffers; sparklines should not render until at least 2 data points exist.

---

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept.

- [ ] Current outdoor temperature — the primary datum; everything else is secondary
- [ ] Current humidity — required for feels-like derivation
- [ ] Barometric pressure + trend arrow — pressure trend is one of the most actionable weather indicators
- [ ] Wind speed + direction (compass rose) + gust — UDP-driven at 2.5s refresh
- [ ] Rain rate + daily accumulation — with correct rain_size-based conversion
- [ ] UV Index with EPA 5-zone color coding — sensor is present; display it
- [ ] Solar radiation — sensor is present; numeric or gauge readout
- [ ] Dew point — provided by API, expected by weather-aware users
- [ ] Feels-like temperature — derived from API heat_index / wind_chill fields with NWS validity rules
- [ ] Connection/staleness indicator — kiosk must show when data is old
- [ ] Full-screen kiosk mode — Qt frameless window, no chrome
- [ ] Last updated timestamp — basic data freshness signal

### Add After Validation (v1.x)

Features to add once core is working.

- [ ] Sparkline trend graphs — add after ring buffer infrastructure is validated; data must accumulate before graphs are meaningful
- [ ] Indoor temperature + humidity panel — API provides it (type 4); low effort after outdoor is working
- [ ] Animated gauge needles — polish pass; only add if Pi GPU budget allows smooth animation at target framerate
- [ ] Pixel-shift burn-in prevention — add before long-term deployment; trivial to implement

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] Configurable threshold values via config file — currently hardcoded thresholds are fine; make configurable when users report wanting different limits
- [ ] Wind gust vs average visual separation on compass — MVP shows both as numbers; a dedicated gust ring on the compass is polish
- [ ] Screen-off schedule (nighttime dimming) — only relevant if the display is not in a 24/7 environment

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Temperature display | HIGH | LOW | P1 |
| Humidity display | HIGH | LOW | P1 |
| Barometric pressure + trend | HIGH | LOW | P1 |
| Wind speed / direction / gust | HIGH | MEDIUM | P1 |
| Rain rate + daily accumulation | HIGH | MEDIUM | P1 |
| Dew point | MEDIUM | LOW | P1 |
| Feels-like temperature | HIGH | LOW | P1 |
| UV Index + color coding | MEDIUM | LOW | P1 |
| Solar radiation | LOW | LOW | P1 |
| Staleness / connection indicator | HIGH | MEDIUM | P1 |
| Full-screen kiosk mode | HIGH | LOW | P1 |
| Last updated timestamp | MEDIUM | LOW | P1 |
| Sparkline trend graphs | MEDIUM | MEDIUM | P2 |
| Indoor temp/humidity | MEDIUM | LOW | P2 |
| Animated gauge needles | LOW | MEDIUM | P2 |
| Burn-in pixel shift | LOW | LOW | P2 |
| Watchdog / auto-restart | HIGH | LOW | P2 |
| Configurable thresholds | LOW | MEDIUM | P3 |
| Wind gust compass ring | LOW | MEDIUM | P3 |
| Screen-off schedule | LOW | LOW | P3 |

---

## Competitor Feature Analysis

| Feature | Davis WeatherLink Console | AcuRite consumer displays | Weather Underground PWS dashboard | Our Approach |
|---------|--------------------------|--------------------------|----------------------------------|--------------|
| Temperature | Yes, digital readout | Yes, large digital | Yes, digital | Analog gauge with digital center readout |
| Humidity | Yes | Yes | Yes | Semicircular gauge |
| Barometric pressure + trend | Yes with trend arrow | Yes with up/down arrow | Yes with trend | Numeric + arrow icon; trend from API field |
| Wind speed + direction | Yes, compass + speed | Yes | Yes, separate wind/gust | Compass rose + speed gauge; 2.5s UDP refresh |
| Rain rate + accumulation | Yes (rate + daily/storm/monthly) | Yes (rate + daily) | Yes (rate + accumulation) | Rate + daily total; rain_size corrected |
| Dew point | Yes | Yes | Yes | Numeric display |
| Feels-like | Yes ("feels like" field) | Yes | Yes | Heat index or wind chill per NWS rules |
| UV Index | Yes with color bar | Not universal | Yes | EPA 5-zone color scale on gauge |
| Solar radiation | Yes (W/m²) | No (most models) | Yes | Numeric or semicircular gauge |
| Sparklines / trend graphs | 24h full charts | Rarely | 24h charts | Compact sparklines (~6h) — faster to read at a glance |
| Indoor temp/humidity | Yes (console sensor) | Yes | No (outdoor only) | Secondary panel from type 4 data structure |
| Color threshold alerts | Limited (UV bar) | Limited | No | Color zones on all gauges per sensor-appropriate scale |
| Kiosk auto-recovery | N/A (embedded device) | N/A | N/A (web browser) | systemd Restart=always + hardware watchdog |
| Burn-in prevention | N/A | N/A | N/A | Pixel-shift on slow timer |

---

## Derived Values Reference

The API provides several of these pre-calculated. Prefer API values where available; recompute only when not present.

| Derived Value | API Field Available? | Validity Conditions | Formula Reference |
|--------------|---------------------|--------------------|--------------------|
| Dew point | YES (dew_point on ISS type 1) | Always | N/A — use API value |
| Heat index | YES (heat_index on ISS type 1) | Temp >= 80°F AND RH >= 40% | NWS Rothfusz equation |
| Wind chill | YES (wind_chill on ISS type 1) | Temp <= 50°F AND wind >= 3 mph | NWS 2001 formula |
| Feels-like | Derive from above | Show heat index OR wind chill OR dry-bulb depending on conditions | Show heat_index when HI conditions met; show wind_chill when WC conditions met; otherwise show dry-bulb temp |
| Rain in inches | NO — API gives counts | Always | counts * cup_size_in_inches (lookup on rain_size field) |
| Pressure trend label | YES (pressure_trend on LSS BAR type 3) | Always | -1=falling, 0=steady, 1=rising |

---

## Color Coding Standards

Use established standards, not invented ones. Users recognize these.

| Sensor | Scale | Color Zones | Source |
|--------|-------|-------------|--------|
| UV Index | 0–11+ | 0-2 Green, 3-5 Yellow, 6-7 Orange, 8-10 Red, 11+ Violet | EPA UV Index Scale |
| Temperature | Context-dependent | Below 0°F = deep blue; 0-32°F = blue; 32-50°F = light blue; 50-70°F = green; 70-90°F = yellow; 90-100°F = orange; 100°F+ = red | Weather convention (no single standard; NWS color convention adapted) |
| Humidity | Comfort-based | <30% = dry/orange; 30-60% = comfortable/green; >60% = humid/yellow; >80% = oppressive/red | Comfort-zone convention |
| Wind speed | Beaufort-inspired | 0-5 mph = green; 5-25 mph = yellow; 25-45 mph = orange; 45+ mph = red | NWS wind advisory thresholds |
| Rain rate | Light/moderate/heavy | <0.1 in/hr = light blue; 0.1-0.3 = blue; 0.3-2.0 = dark blue; >2.0 = red/flash flood | NWS rain intensity classification |

---

## Kiosk-Specific Feature Notes

These are not weather features but are required for unattended operation.

| Feature | Requirement | Implementation Approach | Confidence |
|---------|------------|------------------------|------------|
| Screen blank prevention | Must never blank | Disable DPMS; set xset s off or Wayfire idle_timeout=0 | HIGH — Raspberry Pi Forum confirmed |
| Watchdog / crash recovery | Must self-restart without human intervention | systemd Restart=always + WatchdogSec; Pi has /dev/watchdog enabled by default | HIGH — RPi Forums confirmed watchdog default-enabled |
| Network loss recovery | Must reconnect and resume without restart | HTTP client retry loop with exponential backoff; UDP broadcast re-kick on reconnect | MEDIUM — standard pattern, not Pi-specific |
| Stale data display warning | Must not show old data as current | Track timestamp of last successful HTTP poll; visually flag data older than 30s | MEDIUM — design decision, no external standard |
| Burn-in pixel shift | Recommended for long-term OLED/LCD deployment | Shift entire QWidget coordinate origin by ±2px on 10-minute timer; imperceptible | MEDIUM — RPi kiosk forum community pattern |
| Boot-to-dashboard | Must start automatically on power-on | systemd service unit with WantedBy=graphical.target | HIGH — standard Linux kiosk pattern |

---

## Sources

- [Davis Instruments WeatherLink Console features](https://www.davisinstruments.com/pages/weatherlink-console)
- [WeatherLink Console display templates (manula.com)](https://www.manula.com/manuals/pws/davis-kb/1/en/topic/weatherlink-console-display-templates)
- [Davis Instruments — Meteorology 101: Understanding Indexes](https://www.davisinstruments.com/blogs/newsletter/meteorology-101-understanding-indexes)
- [NWS Heat Index page](https://www.weather.gov/arx/heat_index)
- [NWS Wind Chill formula](https://www.weather.gov/epz/wxcalc_windchill)
- [EPA UV Index Scale](https://www.epa.gov/sunsafety/uv-index-scale-0)
- [Cumulus Wiki — Feels Like](https://cumuluswiki.org/a/Feels_Like)
- [Ambient Weather rain increment definitions](https://ambientweather.com/faqs/question/view/id/1454/)
- [BARANI Design — rain rate intensity classification](https://www.baranidesign.com/faq-articles/2020/1/19/rain-rate-intensity-classification)
- [AcuRite — Trend Arrow Icon](https://support.acurite.com/hc/en-us/articles/360009604553-Trend-Arrow-Icon)
- [Raspberry Pi Forums — burn-in / screen blanking](https://forums.raspberrypi.com/viewtopic.php?t=290974)
- [Raspberry Pi Forums — watchdog](https://forums.raspberrypi.com/viewtopic.php?t=376126)
- [chilipie-kiosk (unattended kiosk patterns)](https://github.com/jareware/chilipie-kiosk)
- [QCustomPlot — Qt C++ plotting widget](https://www.qcustomplot.com/)
- [GitHub — Qt real-time plot on embedded Linux](https://github.com/deeplyembeddedWP/Plot-Real-Time-Graphs-Qt-Embedded-Linux)
- [Weather Underground PWS dashboard](https://www.wunderground.com/pws/overview)

---
*Feature research for: real-time weather dashboard kiosk (Davis WeatherLink Live via Qt 6 C++)*
*Researched: 2026-03-01*
