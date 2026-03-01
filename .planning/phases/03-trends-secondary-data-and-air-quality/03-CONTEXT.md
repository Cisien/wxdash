# Phase 3: Trends, Secondary Data, and Air Quality - Context

**Gathered:** 2026-03-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Add sparkline trend graphs to all outdoor gauges, add a PurpleAir AQI/PM2.5/PM10 multi-ring gauge, and wire up PurpleAir data polling. The existing 3x4 grid layout is preserved. Indoor readings (GAUG-13) are deferred to a future phase.

</domain>

<decisions>
## Implementation Decisions

### Dashboard Layout
- 3x4 grid stays unchanged — no new rows or columns
- Cell 11 (currently ReservedCell labeled "AQI — Phase 3"): AQI multi-ring gauge
- Cell 12 (currently ReservedCell): Remains reserved for future forecast widget
- GAUG-13 (indoor panel) removed from this phase

### Sparkline Design
- 24-hour time window
- Sample resolution adapts dynamically to available horizontal pixel count — lean toward high resolution
- Line only — no fill, no bars
- Dim neutral color, darker than the gauge track (#2A2A2A) so it never competes with gauge readings
- Renders in the lower third of each gauge cell, behind the existing gauge content — gauges do not resize
- Every outdoor ArcGauge gets a sparkline (temperature, feels-like, humidity, dew point, wind, rain rate, pressure, UV, solar, AQI)
- AQI gauge sparkline tracks only the AQI value (not PM2.5 or PM10)

### AQI Gauge (Cell 11)
- Single gauge cell with three concentric arc rings sharing the same total stroke width as other gauges:
  - AQI (outermost): 1/2 of total arc width, colored by EPA 6-zone scheme (Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+)
  - PM2.5 (middle): 1/4 of total arc width, neutral color
  - PM10 (innermost): 1/4 of total arc width, neutral color
- Center text: AQI number displayed prominently
- PM2.5 value in smaller text at bottom-left of gauge
- PM10 value in smaller text at bottom-right of gauge
- When PurpleAir is offline: same staleness behavior as existing weather station (values cleared, stale signal emitted)

### PurpleAir Data Acquisition
- Poll PurpleAir local sensor API (`http://10.1.255.41/json?live=false`) for PM2.5, PM10, and calculated AQI
- Average channels A and B for PM2.5, then calculate AQI from EPA breakpoint table
- Pull PM10 from the same API response (not in original requirements but same sensor, natural fit)

### Claude's Discretion
- Ring buffer implementation details for sparkline history storage
- Exact sparkline neutral color value (must be darker than #2A2A2A gauge track)
- PurpleAir polling interval
- AQI EPA breakpoint calculation implementation
- Sparkline sampling/decimation algorithm for adapting to pixel width

</decisions>

<specifics>
## Specific Ideas

- Sparklines should be unobtrusive background elements — the gauge readings remain the primary visual
- The AQI multi-ring arc is a new pattern: concentric rings within a single gauge cell, not three separate gauges
- PM2.5 and PM10 sub-arcs use neutral color to keep AQI as the visually dominant reading

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- ArcGauge.qml: Animated arc gauge with value, label, unit, color, secondary text — sparklines and AQI gauge will extend this
- CompassRose.qml: Custom QML drawing pattern — reference for custom Shape rendering
- DashboardGrid.qml: 3x4 GridLayout with threshold color functions — new gauge slots into existing grid
- ReservedCell.qml: Placeholder cells 11 and 12 — cell 11 replaced by AQI gauge

### Established Patterns
- Q_PROPERTY with NOTIFY signals for all weather values — PurpleAir data will follow same pattern
- Reading structs (IssReading, BarReading, etc.) for cross-thread data transfer — PurpleAir needs equivalent
- Threshold color functions defined in DashboardGrid.qml — AQI EPA color function follows same pattern
- SmoothedAnimation on arc sweep for smooth gauge transitions

### Integration Points
- WeatherDataModel: Add PurpleAir properties (aqi, pm25, pm10) and sparkline history ring buffers
- HttpPoller pattern: PurpleAir polling follows same HTTP GET + JSON parse + signal emit pattern
- DashboardGrid.qml: Replace ReservedCell in cell 11 with new AQI gauge component
- main.cpp: Wire PurpleAir poller to WeatherDataModel slots

</code_context>

<deferred>
## Deferred Ideas

- GAUG-13 (indoor temperature and humidity panel) — moved to future phase
- Cell 12 forecast widget — future phase
- PM10 was not in original requirements — added naturally from same PurpleAir API response

</deferred>

---

*Phase: 03-trends-secondary-data-and-air-quality*
*Context gathered: 2026-03-01*
