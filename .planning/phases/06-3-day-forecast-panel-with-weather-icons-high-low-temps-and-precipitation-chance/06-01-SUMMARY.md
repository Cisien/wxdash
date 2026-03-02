---
phase: 06-3-day-forecast-panel-with-weather-icons-high-low-temps-and-precipitation-chance
plan: 01
subsystem: network, data-model
tags: [nws-api, forecast, svg-icons, qt-network, qvector, qvariantlist, tdd]

requires:
  - phase: 01-data-model-and-network-layer
    provides: WeatherReadings.h struct pattern, JsonParser namespace, WeatherDataModel Q_PROPERTY pattern, PurpleAirPoller class structure for NwsPoller to mirror
  - phase: 03-trends-secondary-data-air-quality
    provides: PurpleAirPoller wiring pattern in main.cpp for NwsPoller to follow

provides:
  - ForecastDay struct in WeatherReadings.h (high, low, precip, iconCode)
  - parseForecast() in JsonParser namespace parsing NWS forecast API response into QVector<ForecastDay>
  - NwsPoller class polling NWS every 30 minutes with User-Agent header and HTTP 200 check
  - WeatherDataModel.forecastData Q_PROPERTY returning QVariantList of QVariantMaps for QML
  - WeatherDataModel.applyForecastUpdate slot connected to NwsPoller.forecastReceived
  - 17 monochrome gold (#C8A000) SVG weather icons bundled as Qt resources under /icons/weather/ prefix
  - main.cpp NwsPoller wiring sharing existing networkThread

affects:
  - 06-02: ForecastPanel QML component uses forecastData property and /icons/weather/ resources

tech-stack:
  added: NWS public forecast API (no key required), QUrl for icon code extraction
  patterns:
    - NwsPoller mirrors PurpleAirPoller exactly (QNAM+QTimer in start(), abort pending on poll, User-Agent header)
    - extractIconCode file-static helper strips probability suffix (,40) and query string (?size=medium) from NWS icon URLs via QUrl::path().section()
    - Forecast retained indefinitely on API failure (no staleness clearing) per user decision
    - ForecastDay.high=-999 sentinel for afternoon fetch where first period is nighttime
    - Precip per day = max(daytime precip, nighttime precip)

key-files:
  created:
    - src/network/NwsPoller.h
    - src/network/NwsPoller.cpp
    - assets/icons/weather/sun.svg
    - assets/icons/weather/sun_cloud.svg
    - assets/icons/weather/partly_cloudy.svg
    - assets/icons/weather/mostly_cloudy.svg
    - assets/icons/weather/cloudy.svg
    - assets/icons/weather/rain.svg
    - assets/icons/weather/drizzle.svg
    - assets/icons/weather/rain_showers.svg
    - assets/icons/weather/snow.svg
    - assets/icons/weather/blizzard.svg
    - assets/icons/weather/rain_snow.svg
    - assets/icons/weather/freezing_rain.svg
    - assets/icons/weather/sleet.svg
    - assets/icons/weather/thunderstorm.svg
    - assets/icons/weather/windy.svg
    - assets/icons/weather/fog.svg
    - assets/icons/weather/unknown.svg
  modified:
    - src/models/WeatherReadings.h
    - src/network/JsonParser.h
    - src/network/JsonParser.cpp
    - src/models/WeatherDataModel.h
    - src/models/WeatherDataModel.cpp
    - src/main.cpp
    - src/CMakeLists.txt
    - tests/tst_JsonParser.cpp

key-decisions:
  - "NwsPoller shares existing networkThread (no new thread) — same pattern as PurpleAirPoller"
  - "30-minute NWS poll interval — NWS updates every few hours; more frequent polling wastes bandwidth and risks IP blocks"
  - "extractIconCode uses QUrl::path().section() to strip both probability suffix and query string in one pass"
  - "Forecast retained on API failure with no staleness clearing — forecast ages gracefully unlike real-time readings"
  - "ForecastDay.high=-999 sentinel for tonight-only edge case (afternoon fetch) — QML checks for this to display '--'"
  - "NWS grid point SEW/137,72 hardcoded for Duvall WA 98019 — consistent with other hardcoded local endpoints"
  - "QVector<ForecastDay> registered as metatype for cross-thread queued signal delivery"

patterns-established:
  - "icon-code-extraction: QUrl::path().section('/', -1).section(',', 0, 0) strips last path segment and probability suffix"
  - "nws-afternoon-edge: loop checks !hasHigh before first nighttime period to detect tonight-only scenario"

requirements-completed: []

duration: 5min
completed: 2026-03-01
---

# Phase 6 Plan 01: NWS Forecast Backend Summary

**NWS 30-minute forecast polling pipeline: parseForecast() with TDD, NwsPoller with User-Agent, WeatherDataModel.forecastData Q_PROPERTY, and 17 monochrome gold SVG weather icons bundled as Qt resources**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-01T23:18:07Z
- **Completed:** 2026-03-01T23:23:07Z
- **Tasks:** 2
- **Files modified:** 21 (8 modified, 20 created including 17 SVG icons)

## Accomplishments
- parseForecast() with full TDD: tests written first (RED), implementation written to pass (GREEN); handles morning fetch, afternoon sentinel, null precip, empty input, precip max, partial data, icon code extraction
- NwsPoller class mirroring PurpleAirPoller with 30min interval, User-Agent header (required to avoid NWS 403), HTTP 200 status check beyond QNetworkReply::NoError
- WeatherDataModel gains forecastData Q_PROPERTY exposing QVector<ForecastDay> as QVariantList of QVariantMaps ready for QML binding
- 17 monochrome gold (#C8A000) SVG weather icons created and bundled as Qt resources under /icons/weather/ prefix

## Task Commits

Each task was committed atomically:

1. **Task 1: ForecastDay struct, parseForecast() parser with TDD** - `72ba172` (feat)
2. **Task 2: NwsPoller, forecastData model, main.cpp wiring, SVG icons, CMake** - `a0696fd` (feat)

**Plan metadata:** (docs commit below)

_Note: Task 1 used TDD — tests written before implementation_

## Files Created/Modified
- `src/models/WeatherReadings.h` - Added ForecastDay struct with high/low/precip/iconCode and Q_DECLARE_METATYPE
- `src/network/JsonParser.h` - Added parseForecast() declaration and QVector include
- `src/network/JsonParser.cpp` - Implemented parseForecast() and extractIconCode() helper
- `tests/tst_JsonParser.cpp` - 7 new parseForecast test cases
- `src/network/NwsPoller.h` - New NwsPoller class declaration
- `src/network/NwsPoller.cpp` - New NwsPoller implementation
- `src/models/WeatherDataModel.h` - Added forecastData Q_PROPERTY, forecastDataChanged signal, applyForecastUpdate slot, m_forecast member
- `src/models/WeatherDataModel.cpp` - Implemented forecastData() and applyForecastUpdate()
- `src/main.cpp` - NwsPoller instantiation, moveToThread, signal wiring, metatype registration
- `src/CMakeLists.txt` - NwsPoller sources added to wxdash_lib; weather icons qt_add_resources block
- `assets/icons/weather/*.svg` - 17 new monochrome gold weather icon SVGs

## Decisions Made
- NwsPoller shares existing networkThread (no new thread) — same architecture as PurpleAirPoller
- NWS grid SEW/137,72 hardcoded for Duvall WA 98019, consistent with other hardcoded endpoints
- Forecast retained indefinitely on API failure — weather forecasts age gracefully unlike real-time readings
- ForecastDay.high=-999 sentinel distinguishes "no daytime high available" from a real -999 degree reading
- extractIconCode uses QUrl::path().section() for clean icon code extraction in one step

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed explicit namespace qualification in parseForecast implementation**
- **Found during:** Task 1 (parseForecast implementation)
- **Issue:** Writing `QVector<ForecastDay> JsonParser::parseForecast(...)` inside the `namespace JsonParser { }` block produced a compile error: "explicit qualification in declaration"
- **Fix:** Removed explicit `JsonParser::` prefix since function is defined inside the namespace block
- **Files modified:** src/network/JsonParser.cpp
- **Verification:** Build succeeded after fix
- **Committed in:** 72ba172 (Task 1 commit, caught before commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 - Bug)
**Impact on plan:** One-line fix for C++ namespace scoping rule. No scope creep.

## Issues Encountered
- C++ explicit namespace qualification inside namespace body is an error — caught immediately during build, fixed in one line

## User Setup Required
None - no external service configuration required beyond the hardcoded NWS endpoint already in main.cpp.

## Next Phase Readiness
- forecastData Q_PROPERTY is live on the model and will update every 30 minutes
- 17 SVG icons are bundled at /icons/weather/{iconCode}.svg
- Plan 06-02 can immediately start building the ForecastPanel QML component consuming weatherModel.forecastData
- ForecastDay.high=-999 sentinel is documented — QML should display "--" when high == -999

## Self-Check: PASSED

- NwsPoller.h: FOUND
- NwsPoller.cpp: FOUND
- 17 SVG icons in assets/icons/weather/: FOUND (17 count confirmed)
- Commit 72ba172 (Task 1): FOUND
- Commit a0696fd (Task 2): FOUND
- forecastData Q_PROPERTY in WeatherDataModel.h: FOUND
- NwsPoller in main.cpp: FOUND
- weather_icons in CMakeLists.txt: FOUND
- All 3 test suites: PASSED

---
*Phase: 06-3-day-forecast-panel-with-weather-icons-high-low-temps-and-precipitation-chance*
*Completed: 2026-03-01*
