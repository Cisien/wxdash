# Phase 6: 3-day forecast panel with weather icons, high/low temps, and precipitation chance - Context

**Gathered:** 2026-03-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Display a 3-day weather forecast panel in the existing dashboard grid. The panel shows weather icons, high/low temperatures, and precipitation chance for the next 3 days using NWS forecast data. This is the first external API integration — all previous data sources are local network only.

</domain>

<decisions>
## Implementation Decisions

### Forecast Data Source
- Use the NWS (National Weather Service) API — free, no API key required
- Hardcode the NWS grid point for zip code 98019 (Duvall, WA) — consistent with how WeatherLink and PurpleAir endpoints are hardcoded
- Poll the NWS forecast endpoint every 30 minutes
- On API failure, display the last successfully fetched forecast — forecast data ages gracefully unlike real-time readings, so no staleness clearing

### Panel Layout and Placement
- Forecast panel occupies the single ReservedCell (cell 12, row 3, col 4) in the existing 3x4 GridLayout
- 3 day columns arranged horizontally (side-by-side) within the single cell
- Same visual styling as all other cells: #222222 background, rounded corners, subtle border (#2A2A2A)
- No header label or title — maximize space for content

### Weather Icons
- Monochrome gold/amber SVG icons matching the #C8A000 text color used throughout the dashboard
- Simple weather silhouettes / clean outlines (sun, clouds, rain, snow, etc.)
- Comprehensive mapping of all NWS weather conditions to unique icons — every distinct NWS condition gets its own icon

### Day Card Content
- Each column shows (top to bottom): weather icon, high/low temp, precipitation chance
- No day name labels — content is self-explanatory from position (today, tomorrow, day after)
- High temp in red, low temp in blue — departure from all-gold scheme for instant readability
- High/low on one line ("72/55") if space allows, two separate lines if not — adaptive layout
- Precipitation chance displayed as plain percentage number in gold/amber, no droplet symbol or decoration
- Font colors and sizes consistent with the existing gauge aesthetic (#C8A000 for standard text)

### Claude's Discretion
- Exact red/blue color values for high/low temps (should be subdued to match dashboard aesthetic, not bright primary colors)
- NWS grid point resolution (lat/lon for 98019 to NWS grid coordinates)
- Icon sizing and spacing within the cell
- How to handle NWS day/night forecast period mapping to daily high/low
- SVG icon design details and exact condition-to-icon mapping table
- Network request implementation (reuse HttpPoller pattern or new component)
- Forecast data model structure (new struct or extend existing)

</decisions>

<specifics>
## Specific Ideas

- The 3 forecast days are: today, tomorrow, and day after tomorrow
- Red/blue high/low color scheme should feel "subdued" like the existing threshold colors in DashboardGrid.qml (e.g., "#C84040" red, "#5B8DD9" blue are already in the codebase)
- Icons should be recognizable at small sizes since they share a single grid cell with two other data points per column

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ReservedCell.qml`: Placeholder cell at grid position 12 — will be replaced by ForecastPanel
- `HttpPoller`: Existing HTTP polling pattern (10s cadence, QNetworkAccessManager, worker thread) — forecast poller can follow same architecture at 30min cadence
- `JsonParser`: Routes JSON by structure type — can be extended or a new parser created for NWS response format
- `WeatherDataModel`: Central Q_PROPERTY model consumed by QML — forecast data needs similar exposure
- `icons.qrc`: Qt resource file for bundled assets — forecast SVG icons will be added here

### Established Patterns
- All network I/O happens on worker threads with typed structs emitted via signals to the model
- Data staleness is tracked per-source with independent timers
- QML components consume model properties via direct binding (e.g., `weatherModel.temperature`)
- Color threshold functions defined in DashboardGrid.qml (temperatureColor, humidityColor, etc.)
- Dark theme: #1A1A1A background, #222222 cell background, #C8A000 gold text, #2A2A2A borders

### Integration Points
- `DashboardGrid.qml` GridLayout cell 12 — replace ReservedCell with new ForecastPanel component
- `main.cpp` — wire up new NWS poller to forecast model (same pattern as HttpPoller + PurpleAirPoller)
- `WeatherDataModel` or new `ForecastModel` — expose forecast data as Q_PROPERTYs for QML binding

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-3-day-forecast-panel-with-weather-icons-high-low-temps-and-precipitation-chance*
*Context gathered: 2026-03-01*
