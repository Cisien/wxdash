# Phase 2: Core Gauges and Dashboard Layout - Context

**Gathered:** 2026-03-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Full-screen weather dashboard displaying all outdoor weather conditions at a glance. Delivers 10 gauges (temperature, feels-like, humidity, dew point, wind speed, compass rose, rain rate, barometric pressure, UV index, solar radiation), a responsive 3x4 grid layout, fullscreen/windowed modes, color-coded thresholds, and animated gauge fill transitions. AQI, indoor panel, and sparklines are Phase 3. Staleness indicators are Phase 4.

</domain>

<decisions>
## Implementation Decisions

### Gauge Visual Style
- 80% arc (~290 degree sweep) with large numeric readout centered inside the arc
- Arc fill represents value position in range (min to max) — gives spatial "where am I in the range" reading
- Dim/muted track always visible behind filled portion showing full range
- Fill only, no needle — the leading edge of the fill is the indicator
- Fill animates smoothly between values (satisfies GAUG-14 animated transitions)

### Dashboard Layout
- 3x4 equal grid (3 rows, 4 columns)
- Fixed layout:
  - Row 1: Temperature | Feels-like | Humidity | Dew Point
  - Row 2: Wind Speed | Compass Rose | Rain Rate | Pressure
  - Row 3: UV Index | Solar Rad | [Reserved: AQI Phase 3] | [Reserved: Future]
- 2 reserved cells: AQI gauge (Phase 3 fills with data + UI), one future placeholder
- Reserved cells should be empty styled slots (not invisible — maintain grid structure)
- KIOSK-03 (last-updated timestamp) dropped — user does not want it

### Compass Rose
- Traditional compass rose with full N/S/E/W and intercardinal markings
- 16-point coarse cardinal direction text label displayed (N, NNE, NE, ENE, etc.) — no numeric degrees
- Smooth rotation animation when wind direction changes
- Must update within 2.5s of UDP packet (GAUG-05 requirement)

### Color Thresholds
- Temperature: Blue <32F, Light blue 32-50F, Green 50-70F, Yellow 70-85F, Orange 85-100F, Red 100F+
- Humidity: Red <25%, Orange 25-30%, Green 30-60%, Yellow 60-70%, Red 70%+
- Wind speed: Green 0-5 mph, Yellow 5-15 mph, Orange 15-30 mph, Red 30+ mph
- UV Index: EPA 5-zone (Green 0-2, Yellow 3-5, Orange 6-7, Red 8-10, Violet 11+)
- All other gauges (pressure, solar rad, rain rate, dew point, feels-like): no threshold colors

### Color Theme
- Dark background throughout the dashboard
- Dark yellow for default arc fill on non-threshold gauges
- Subdued yellow for all gauge value text — always yellow, even on threshold gauges
- Subdued yellow for gauge markings, tick marks, compass rose markings/arrow, cardinal labels
- Threshold gauge arcs use subdued version of threshold color (~70-80% saturation, pulled slightly darker than pure)
- "Subdued" means clearly recognizable as that color, not full saturation — toned down, not muted to gray

### Unit Display
- Barometric pressure displayed in millibars (API returns inHg; convert via inHg x 33.8639 = mbar)
- All other values remain imperial (Fahrenheit, mph, inches)

### Kiosk & Window Modes
- Default to windowed mode with window chrome visible
- `--kiosk` command-line flag launches fullscreen frameless
- F11 keyboard shortcut toggles fullscreen at runtime
- Same 3x4 grid layout in both modes — scales to fill whatever window size
- No minimum window size constraint

### Claude's Discretion
- Exact dark background color value
- Exact yellow/subdued color hex values (within the described intent)
- Arc gauge min/max ranges for each gauge type
- Spacing and padding between grid cells
- Font choice and sizing for gauge readouts
- Compass rose visual detail level (line weights, tick styling)
- Reserved cell placeholder styling
- Rain rate vs daily accumulation presentation within the rain gauge cell
- Wind gust readout presentation within the wind speed cell
- Pressure trend arrow styling within the pressure gauge cell
- Feels-like logic display (which formula is active: heat index vs wind chill)

</decisions>

<specifics>
## Specific Ideas

- Dashboard should be readable from across a room — large numbers, clear color contrast against dark background
- The gauge arc style is inspired by fitness tracker rings / car fuel gauges — partial arc with fill, not full circular dials
- Compass rose should feel like a traditional weather instrument, not a minimalist arrow
- Color system: think "slightly darkened" versions of pure colors, not "washed out" or "pastel"

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `WeatherDataModel` (src/models/WeatherDataModel.h): 18 Q_PROPERTY/NOTIFY signals ready for QML binding — temperature, humidity, dewPoint, heatIndex, windChill, windSpeed, windDir, windGust, rainRate, rainfallDaily, pressure, pressureTrend, uvIndex, solarRad, tempIn, humIn, dewPointIn, sourceStale
- `WeatherReadings.h`: Plain C++ structs (IssReading, BarReading, IndoorReading, UdpReading) with Q_DECLARE_METATYPE

### Established Patterns
- Q_PROPERTY with NOTIFY signals — widgets connect to exactly the signal they care about
- Worker objects on shared QThread via moveToThread() — HttpPoller and UdpReceiver
- Cross-thread signals use Qt auto-detected QueuedConnection
- Dependency injection, no singletons — pass model as constructor param
- pressureTrend is an int field (from API bar_trend: -1/0/1 for falling/steady/rising)

### Integration Points
- `main.cpp` currently uses QCoreApplication — must become QGuiApplication + QQmlApplicationEngine
- AGENTS.md notes "QML/Quick added in Phase 2" — add Qt6::Quick, Qt6::Qml to CMakeLists
- Model registered as QML context property — QML gauges bind directly to model properties
- 42 existing unit tests (JsonParser + WeatherDataModel) must continue passing

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-core-gauges-and-dashboard-layout*
*Context gathered: 2026-03-01*
