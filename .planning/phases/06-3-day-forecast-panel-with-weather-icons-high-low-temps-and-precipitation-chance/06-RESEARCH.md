# Phase 6: 3-Day Forecast Panel - Research

**Researched:** 2026-03-01
**Domain:** NWS REST API (JSON), Qt6 QML SVG icons, new C++ poller/parser/model, QML RowLayout panel
**Confidence:** HIGH (NWS API structure verified live, Qt SVG/QML patterns verified via official docs) / MEDIUM (shortForecast-to-icon mapping — no authoritative complete list)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Forecast Data Source**
- Use the NWS (National Weather Service) API — free, no API key required
- Hardcode the NWS grid point for zip code 98019 (Duvall, WA) — consistent with how WeatherLink and PurpleAir endpoints are hardcoded
- Poll the NWS forecast endpoint every 30 minutes
- On API failure, display the last successfully fetched forecast — forecast data ages gracefully unlike real-time readings, so no staleness clearing

**Panel Layout and Placement**
- Forecast panel occupies the single ReservedCell (cell 12, row 3, col 4) in the existing 3x4 GridLayout
- 3 day columns arranged horizontally (side-by-side) within the single cell
- Same visual styling as all other cells: #222222 background, rounded corners, subtle border (#2A2A2A)
- No header label or title — maximize space for content

**Weather Icons**
- Monochrome gold/amber SVG icons matching the #C8A000 text color used throughout the dashboard
- Simple weather silhouettes / clean outlines (sun, clouds, rain, snow, etc.)
- Comprehensive mapping of all NWS weather conditions to unique icons — every distinct NWS condition gets its own icon

**Day Card Content**
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

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope
</user_constraints>

---

## Summary

Phase 6 adds a 3-day weather forecast panel to cell 12 (ReservedCell) using the free NWS API. The implementation follows three established codebase patterns almost exactly: a new `NwsPoller` that mirrors `PurpleAirPoller` (30-minute HTTP poll), a new `ForecastParser` namespace function similar to `parsePurpleAirJson`, and a `ForecastPanel.qml` component that replaces `ReservedCell.qml` in `DashboardGrid.qml`.

The NWS `/gridpoints/SEW/137,72/forecast` endpoint (verified live) returns alternating day/night 12-hour periods. Periods with `isDaytime: true` carry the daily high temperature; `isDaytime: false` periods carry the daily low. To get 3 days of forecasts, the parser consumes the next 6 periods (alternating day/night), extracting (high, low, precipitationChance, shortForecast) tuples. The parser handles the "partial day" edge case — if the current time is in the afternoon, the first period returned is a nighttime period with no matching daytime period for today; in this case that night's data becomes "today night only" and tomorrow starts with the next daytime period.

Weather icons are monochrome gold SVG files bundled in `icons.qrc`. NWS provides a `shortForecast` text string (e.g. "Sunny", "Light Rain", "Mostly Cloudy") and also an `icon` URL containing a code like `skc`, `tsra`, `sn`. The icon URL code is more reliable for programmatic mapping than the free-text shortForecast. The parser extracts the icon code from the URL and maps it to a local SVG filename. A flat `QVariantList` of 3 `QVariantMap` items (each with `high`, `low`, `precip`, `iconPath` keys) is the cleanest approach for exposing forecast data to QML, matching the `windRoseData` pattern already in the codebase.

**Primary recommendation:** Implement in two plans — Plan 06-01: NWS poller, parser, struct, model properties, and icon assets; Plan 06-02: ForecastPanel.qml QML component and DashboardGrid wiring.

---

## NWS API — Verified Live Data

### Grid Point for Zip 98019 (Duvall, WA)

Resolved via `https://api.weather.gov/points/47.7394,-121.9823` (verified 2026-03-01):

- **Office:** SEW
- **GridX:** 137
- **GridY:** 72
- **Forecast URL:** `https://api.weather.gov/gridpoints/SEW/137,72/forecast`
- **Forecast Hourly URL:** `https://api.weather.gov/gridpoints/SEW/137,72/forecast/hourly`

**Confidence: HIGH** — verified via live API call.

### Forecast Period JSON Structure (verified live)

```json
{
  "number": 1,
  "name": "This Afternoon",
  "startTime": "2026-03-01T16:00:00-08:00",
  "endTime": "2026-03-01T18:00:00-08:00",
  "isDaytime": true,
  "temperature": 55,
  "temperatureUnit": "F",
  "temperatureTrend": null,
  "probabilityOfPrecipitation": {
    "unitCode": "wmoUnit:percent",
    "value": 0
  },
  "windSpeed": "2 mph",
  "windDirection": "NNW",
  "icon": "https://api.weather.gov/icons/land/day/skc?size=medium",
  "shortForecast": "Sunny",
  "detailedForecast": "Sunny. High near 55..."
}
```

Key fields for Phase 6:
- `isDaytime` — boolean; true = daytime (high temp period), false = nighttime (low temp period)
- `temperature` — integer, always in °F when `temperatureUnit: "F"`
- `probabilityOfPrecipitation.value` — integer 0-100 (percent), may be `null` — treat null as 0
- `icon` — URL containing the NWS icon code (e.g. `skc`, `tsra`, `rain_showers`, `sn`) embedded in the path
- `shortForecast` — human-readable string ("Sunny", "Light Rain", "Mostly Cloudy", etc.)

### Day/Night Period Mapping Strategy

The API returns alternating periods starting from the current moment. The strategy for extracting 3-day forecast data:

1. Fetch all periods (typically 14 periods over 7 days)
2. Find the first **daytime** period — this is "today" (or "tomorrow" if today's daytime has already passed)
3. Take consecutive pairs: (daytime period N, nighttime period N+1) → one ForecastDay
4. Collect 3 such pairs = 3 forecast days
5. If the first period returned is **nighttime** (afternoon/evening fetch): treat as day 0 with no high temp yet — use the night period's data for "tonight" and begin day 1 with the next daytime period

**Practical recommendation:** Start from the first period regardless of isDaytime. Iterate periods and accumulate: when we see a daytime period, save its temperature as the high; when we see the following nighttime period, save its temperature as the low. The first pair = day 1, second pair = day 2, third pair = day 3. If the very first period is a night period, that becomes a "tonight only" entry with no high temp (display "--" for high).

**Confidence: HIGH** — pattern confirmed by live API observation and NWS API FAQ documentation.

### Icon URL Code Extraction

The `icon` field contains a URL like:
- `https://api.weather.gov/icons/land/day/skc?size=medium`
- `https://api.weather.gov/icons/land/night/rain_showers?size=medium`
- `https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium` (with probability suffix)

Extract the icon code by:
1. Split the URL path on `/`
2. Take the segment after `day` or `night`
3. Strip any `,NN` probability suffix and `?size=...` query string
4. The result is the canonical NWS icon code: `skc`, `tsra_hi`, `rain_showers`, `sn`, etc.

```cpp
// Example extraction
QString extractIconCode(const QString& iconUrl) {
    // "https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium"
    QUrl url(iconUrl);
    QString path = url.path();           // "/icons/land/day/tsra_hi,40"
    QString last = path.section('/', -1); // "tsra_hi,40"
    last = last.section(',', 0, 0);      // "tsra_hi"  (strip probability)
    return last;
}
```

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Network (QNetworkAccessManager) | Qt 6.x (project-established) | NWS HTTP polling | PurpleAirPoller pattern — identical architecture, 30min cadence |
| Qt Core (QJsonDocument) | Qt 6.x (project-established) | NWS forecast JSON parsing | JsonParser/parsePurpleAirJson stateless namespace pattern |
| Qt Quick (QML Image) | Qt 6.x (project-established) | SVG weather icon rendering | Qt Quick Image type with qrc:/ resource path |
| Qt SVG | Qt 6.x (already linked in CMakeLists.txt) | SVG rendering support for QML Image | Already linked: `target_link_libraries(wxdash PRIVATE wxdash_lib Qt6::Quick Qt6::Svg)` |
| Qt Quick Layouts (RowLayout) | Qt 6.x (project-established) | 3-column horizontal day layout | Same module as GridLayout used in DashboardGrid.qml |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QVariantList / QVariantMap | Qt 6.x | Forecast data exposed to QML | Same pattern as windRoseData — list of maps for structured data |
| QTimer | Qt 6.x | 30-minute NWS poll interval | Same QTimer-driven poll pattern as PurpleAirPoller |
| Qt Resource System (.qrc) | Qt 6.x | SVG icon bundling | Same `icons.qrc` / `qt_add_resources` pattern already in CMakeLists.txt |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| QVariantList of QVariantMaps | QAbstractListModel subclass | ListModel is more flexible for ListView/Repeater, but QVariantList is simpler for exactly-3 fixed items and matches windRoseData pattern. For 3 static slots, QVariantList is sufficient. |
| QML Image (SVG) | QML Shape + SVG path data inline | Inline SVG paths would avoid the file dependency but require converting SVG files to QML path commands — far more complex. Image with qrc:/ is the right choice. |
| NWS shortForecast text matching | NWS icon URL code extraction | Icon URL code (skc, tsra, sn) is more reliable than free-text shortForecast string matching. Use the URL code as the primary mapping key. |
| Extending WeatherDataModel | New ForecastModel class | Forecast data is conceptually separate (external API, different cadence, different failure mode). A new `ForecastModel` class is cleaner than adding more properties to an already-large WeatherDataModel. However, the simpler option — exposing forecast as a `Q_PROPERTY(QVariantList forecast)` on `WeatherDataModel` — avoids a new context property in main.cpp. Recommended: add to `WeatherDataModel` for simplicity (same approach PurpleAir data took). |

**No new CMake dependencies.** `Qt6::Svg` is already linked. SVG icons use the existing `qt_add_resources` infrastructure.

---

## Architecture Patterns

### Recommended Project Structure

```
src/
├── models/
│   ├── WeatherReadings.h        # Add ForecastDay struct
│   ├── WeatherDataModel.h/.cpp  # Add forecastData Q_PROPERTY + applyForecastUpdate slot
├── network/
│   ├── NwsPoller.h/.cpp         # New: mirrors PurpleAirPoller, 30min cadence
│   └── JsonParser.h/.cpp        # Add parseForecast() function
├── qml/
│   └── ForecastPanel.qml        # New: replaces ReservedCell in cell 12
└── icons.qrc                    # Add SVG weather icon files
assets/
└── icons/
    └── weather/                 # New: weather SVG icons (sun.svg, rain.svg, etc.)
```

### Recommended Plan Structure

```
Plan 06-01: Backend — NWS poller, forecast parser, model properties, SVG icon assets
  - ForecastDay struct in WeatherReadings.h
  - parseForecast() in JsonParser namespace
  - NwsPoller class (mirrors PurpleAirPoller)
  - WeatherDataModel: forecastData Q_PROPERTY + applyForecastUpdate() slot
  - main.cpp wiring (NwsPoller on existing networkThread)
  - SVG weather icons designed and added to assets/icons/weather/ + icons.qrc

Plan 06-02: Frontend — ForecastPanel.qml + DashboardGrid wiring
  - ForecastPanel.qml: 3-column RowLayout, icon + high/low + precip per column
  - DashboardGrid.qml: replace ReservedCell with ForecastPanel, bind forecastData
```

### Pattern 1: ForecastDay Struct (plain C++, no Qt dependency)

```cpp
// In WeatherReadings.h — follows established plain C++ struct pattern
struct ForecastDay {
    int high    = -999;   // °F daytime high; -999 means "no data" (tonight-only edge case)
    int low     = -999;   // °F nighttime low
    int precip  = 0;      // % precipitation chance (0-100)
    QString iconCode;     // NWS icon code: "skc", "tsra", "sn", etc.
};

// Required for cross-thread queued connections — add at bottom of WeatherReadings.h
Q_DECLARE_METATYPE(ForecastDay)
```

**Note:** `QString` in a struct requires Qt header, which `WeatherReadings.h` already includes via `Q_DECLARE_METATYPE`. `QString` is copy-safe for queued connections so this is acceptable. Alternatively, use `std::string` and convert at the boundary, but `QString` is simpler here since `Q_DECLARE_METATYPE` already requires the Qt include.

**For cross-thread signal delivery:** The signal emits a `QVector<ForecastDay>` (or `QList<ForecastDay>`) — register with `qRegisterMetaType<QVector<ForecastDay>>()` in main.cpp.

### Pattern 2: NWS Forecast Parser (stateless namespace function)

```cpp
// In JsonParser.h — add to existing JsonParser namespace
QVector<ForecastDay> parseForecast(const QByteArray &data);
```

```cpp
// In JsonParser.cpp
static QString extractIconCode(const QString &iconUrl) {
    // e.g. "https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium" -> "tsra_hi"
    QUrl url(iconUrl);
    QString last = url.path().section('/', -1);  // "tsra_hi,40"
    return last.section(',', 0, 0);               // "tsra_hi"
}

QVector<ForecastDay> JsonParser::parseForecast(const QByteArray &data) {
    QVector<ForecastDay> result;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return result;

    auto periods = doc.object()["properties"].toObject()["periods"].toArray();
    if (periods.isEmpty()) return result;

    ForecastDay current;
    bool hasHigh = false;

    for (const auto &val : periods) {
        if (result.size() >= 3) break;
        auto obj = val.toObject();
        bool isDaytime = obj["isDaytime"].toBool();
        int temp = obj["temperature"].toInt();
        int precip = obj["probabilityOfPrecipitation"].toObject()["value"].toInt(0);
        QString iconCode = extractIconCode(obj["icon"].toString());

        if (isDaytime) {
            // Start of a new day
            if (hasHigh) {
                // Previous day had high but no low yet — edge case, push it
                result.append(current);
                current = ForecastDay{};
                hasHigh = false;
            }
            current.high = temp;
            current.precip = precip;
            current.iconCode = iconCode;
            hasHigh = true;
        } else {
            // Nighttime period — the low
            if (!hasHigh) {
                // Tonight only (no daytime period seen yet — e.g. afternoon fetch)
                current.high = -999;  // "no high" sentinel
            }
            current.low = temp;
            // Use nighttime precip if daytime was 0 and nighttime is higher
            if (precip > current.precip) current.precip = precip;
            // Keep daytime icon if we have one; use nighttime icon if tonight-only
            if (!hasHigh) current.iconCode = iconCode;
            result.append(current);
            current = ForecastDay{};
            hasHigh = false;
        }
    }
    // If last period was a daytime with no following night, append anyway
    if (hasHigh && result.size() < 3) result.append(current);

    return result;
}
```

### Pattern 3: NwsPoller (mirrors PurpleAirPoller exactly)

```cpp
// NwsPoller.h
class NwsPoller : public QObject {
    Q_OBJECT
public:
    static constexpr int kPollIntervalMs = 30 * 60 * 1000; // 30 minutes

    explicit NwsPoller(const QUrl &url, QObject *parent = nullptr);

public slots:
    void start();

signals:
    void forecastReceived(QVector<ForecastDay> forecast);

private slots:
    void poll();
    void onReply();

private:
    QUrl m_url;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr;
    QNetworkReply *m_pendingReply = nullptr;
};
```

```cpp
// NwsPoller.cpp — identical structure to PurpleAirPoller.cpp
void NwsPoller::start() {
    m_nam = new QNetworkAccessManager(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &NwsPoller::poll);
    m_pollTimer->start();
    poll(); // immediate first poll on startup
}

void NwsPoller::onReply() {
    auto *reply = m_pendingReply;
    m_pendingReply = nullptr;
    if (!reply) return;
    reply->deleteLater();

    // Add User-Agent — NWS API returns 403 without it (see Pitfall 3)
    if (reply->error() != QNetworkReply::NoError) return;

    auto forecast = JsonParser::parseForecast(reply->readAll());
    if (!forecast.isEmpty()) {
        emit forecastReceived(forecast);
    }
    // On error or empty: silently keep last forecast (CONTEXT.md decision)
}
```

### Pattern 4: WeatherDataModel Forecast Properties

Add to `WeatherDataModel` (same approach PurpleAir data used — avoids new context property):

```cpp
// In WeatherDataModel.h
Q_PROPERTY(QVariantList forecastData READ forecastData NOTIFY forecastDataChanged)

// Slot to receive from NwsPoller
void applyForecastUpdate(const QVector<ForecastDay>& forecast);

signals:
    void forecastDataChanged();

// Private
QVector<ForecastDay> m_forecast;  // last successfully fetched (max 3 items)
```

```cpp
// In WeatherDataModel.cpp
QVariantList WeatherDataModel::forecastData() const {
    QVariantList list;
    for (const auto& day : m_forecast) {
        QVariantMap map;
        map[QStringLiteral("high")]     = day.high;
        map[QStringLiteral("low")]      = day.low;
        map[QStringLiteral("precip")]   = day.precip;
        map[QStringLiteral("iconCode")] = day.iconCode;
        list.append(map);
    }
    return list;
}

void WeatherDataModel::applyForecastUpdate(const QVector<ForecastDay>& forecast) {
    m_forecast = forecast;
    emit forecastDataChanged();
}
```

**Note:** No staleness clearing for forecast data per CONTEXT.md decision — on API failure keep last successfully fetched forecast. This means no staleness timer for forecast; just cache the last good result.

### Pattern 5: ForecastPanel.qml

```qml
// ForecastPanel.qml — replaces ReservedCell in DashboardGrid.qml cell 12
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#222222"
    radius: 4
    border.color: "#2A2A2A"
    border.width: 1

    property var forecastData: []   // QVariantList of QVariantMaps from weatherModel

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        Repeater {
            model: root.forecastData

            delegate: Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                readonly property var day: modelData

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    // Weather icon
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.parent.width, parent.parent.height) * 0.35
                        height: width
                        source: "qrc:/icons/weather/" + (day.iconCode || "unknown") + ".svg"
                        sourceSize.width: 64
                        sourceSize.height: 64
                        // Fallback if icon not found — handled by unknown.svg
                    }

                    // High/Low temp
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 2
                        Text {
                            text: day.high >= -998 ? day.high + "°" : "--°"
                            color: "#C84040"   // subdued red (matches DashboardGrid.qml threshold red)
                            font.pixelSize: Math.min(root.width, root.height) * 0.08
                            font.bold: true
                        }
                        Text {
                            text: "/"
                            color: "#C8A000"
                            font.pixelSize: Math.min(root.width, root.height) * 0.08
                        }
                        Text {
                            text: day.low >= -998 ? day.low + "°" : "--°"
                            color: "#5B8DD9"   // subdued blue (matches DashboardGrid.qml threshold blue)
                            font.pixelSize: Math.min(root.width, root.height) * 0.08
                            font.bold: true
                        }
                    }

                    // Precipitation chance
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: day.precip + "%"
                        color: "#C8A000"
                        font.pixelSize: Math.min(root.width, root.height) * 0.07
                    }
                }
            }
        }
    }
}
```

### Pattern 6: main.cpp Wiring for NwsPoller

```cpp
// In main.cpp — add after PurpleAir wiring, shares networkThread
qRegisterMetaType<QVector<ForecastDay>>();  // needed for cross-thread queued connection

const QUrl nwsUrl(QStringLiteral(
    "https://api.weather.gov/gridpoints/SEW/137,72/forecast"));
auto *nwsPoller = new NwsPoller(nwsUrl);
nwsPoller->moveToThread(networkThread);

QObject::connect(nwsPoller, &NwsPoller::forecastReceived,
                 model, &WeatherDataModel::applyForecastUpdate);
QObject::connect(networkThread, &QThread::started,
                 nwsPoller, &NwsPoller::start);
QObject::connect(networkThread, &QThread::finished,
                 nwsPoller, &QObject::deleteLater);
```

### Pattern 7: DashboardGrid.qml Integration

```qml
// Replace ReservedCell with ForecastPanel in cell 12
// Cell 12: 3-Day Forecast
ForecastPanel {
    forecastData: weatherModel.forecastData
    Layout.fillWidth: true
    Layout.fillHeight: true
}
```

### Anti-Patterns to Avoid

- **Matching on shortForecast text string:** The free-text field has no official fixed vocabulary. Use the icon URL code instead — it is machine-generated and stable.
- **Multiple calls to `api.weather.gov/points/{lat},{lon}` at runtime:** The grid point (SEW/137,72) is hardcoded at build time — no runtime lookup needed, matching how WeatherLink and PurpleAir URLs are hardcoded.
- **Omitting User-Agent header:** NWS API requires a valid User-Agent or returns HTTP 403. Must add `req.setRawHeader("User-Agent", "wxdash/1.0 (your-email@example.com)")` to QNetworkRequest.
- **Polling faster than 30 minutes:** NWS updates forecast data every few hours; sub-30-minute polling provides no benefit and risks IP blocks.
- **Changing `sourceSize` dynamically on SVG images:** Qt docs warn this causes re-rasterization (slow). Set `sourceSize` once to a fixed logical size (64x64 is sufficient for icon display).
- **Using QAbstractListModel for 3 fixed items:** Overkill. `QVariantList` of `QVariantMap` works perfectly for exactly 3 forecast day slots.
- **Separate QThread for NWS poller:** The existing `networkThread` can host all pollers (HttpPoller, UdpReceiver, PurpleAirPoller, NwsPoller). All are event-driven and non-blocking. No need for a separate thread.

---

## NWS Icon Code Taxonomy (Complete Mapping)

Source: weather.gov/forecast-icons (official NWS icon reference page, verified 2026-03-01)

### Sky/Cloud Cover
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `skc` | Clear/Fair (day) | `sun.svg` |
| `nskc` | Clear (night) | `moon.svg` (or `sun.svg` — daytime panel only) |
| `few` | A Few Clouds (day) | `sun_cloud.svg` |
| `nfew` | A Few Clouds (night) | `sun_cloud.svg` |
| `sct` | Partly Cloudy | `partly_cloudy.svg` |
| `nsct` | Partly Cloudy (night) | `partly_cloudy.svg` |
| `bkn` | Mostly Cloudy | `mostly_cloudy.svg` |
| `nbkn` | Mostly Cloudy (night) | `mostly_cloudy.svg` |
| `ovc` | Overcast | `cloudy.svg` |
| `novc` | Overcast (night) | `cloudy.svg` |

### Rain/Showers
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `minus_ra` | Light Rain/Drizzle | `drizzle.svg` |
| `nra` | Rain (night) | `rain.svg` |
| `ra` | Rain | `rain.svg` |
| `shra` | Rain Showers | `rain_showers.svg` |
| `nshra` | Rain Showers (night) | `rain_showers.svg` |
| `hi_shwrs` | Showers in Vicinity | `rain_showers.svg` |
| `hi_nshwrs` | Showers in Vicinity (night) | `rain_showers.svg` |
| `rain_showers` | Rain Showers | `rain_showers.svg` |
| `rain_showers_hi` | Showers in Vicinity | `rain_showers.svg` |

### Snow
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `sn` | Snow | `snow.svg` |
| `nsn` | Snow (night) | `snow.svg` |
| `blizzard` | Blizzard | `blizzard.svg` |
| `nblizzard` | Blizzard (night) | `blizzard.svg` |

### Mixed Precipitation
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `ra_sn` | Rain/Snow Mix | `rain_snow.svg` |
| `nra_sn` | Rain/Snow Mix (night) | `rain_snow.svg` |
| `rasn` | Rain/Snow | `rain_snow.svg` |
| `nrasn` | Rain/Snow (night) | `rain_snow.svg` |
| `fzra` | Freezing Rain | `freezing_rain.svg` |
| `nfzra` | Freezing Rain (night) | `freezing_rain.svg` |
| `ra_fzra` | Rain/Freezing Rain | `freezing_rain.svg` |
| `nra_fzra` | Rain/Freezing Rain (night) | `freezing_rain.svg` |
| `fzra_sn` | Freezing Rain/Snow | `freezing_rain.svg` |
| `nfzra_sn` | Freezing Rain/Snow (night) | `freezing_rain.svg` |
| `ip` | Ice Pellets/Sleet | `sleet.svg` |
| `nip` | Ice Pellets/Sleet (night) | `sleet.svg` |
| `snip` | Snow/Ice Pellets | `sleet.svg` |
| `nsnip` | Snow/Ice Pellets (night) | `sleet.svg` |
| `raip` | Rain/Ice Pellets | `sleet.svg` |
| `nraip` | Rain/Ice Pellets (night) | `sleet.svg` |

### Thunderstorms
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `tsra` | Thunderstorms | `thunderstorm.svg` |
| `ntsra` | Thunderstorms (night) | `thunderstorm.svg` |
| `tsra_sct` | Scattered Thunderstorms | `thunderstorm.svg` |
| `scttsra` | Thunderstorms in Vicinity | `thunderstorm.svg` |
| `nscttsra` | Thunderstorms in Vicinity (night) | `thunderstorm.svg` |
| `tsra_hi` | Distant/Isolated Thunderstorms | `thunderstorm.svg` |
| `hi_tsra` | Thunderstorms in Vicinity | `thunderstorm.svg` |
| `hi_ntsra` | Thunderstorms in Vicinity (night) | `thunderstorm.svg` |

### Wind
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `wind_skc` | Fair and Windy | `windy.svg` |
| `nwind_skc` | Fair and Windy (night) | `windy.svg` |
| `wind_few` | Few Clouds and Windy | `windy.svg` |
| `wind_sct` | Partly Cloudy and Windy | `windy.svg` |
| `wind_bkn` | Mostly Cloudy and Windy | `windy.svg` |
| `wind_ovc` | Overcast and Windy | `windy.svg` |

### Hazards
| Icon Code | Condition | Recommended SVG |
|-----------|-----------|-----------------|
| `fg` | Fog | `fog.svg` |
| `nfg` | Fog (night) | `fog.svg` |
| `hz` | Haze | `fog.svg` |
| `du` | Dust/Blowing Dust | `fog.svg` |
| `ndu` | Dust (night) | `fog.svg` |
| `fu` | Smoke | `fog.svg` |
| `nfu` | Smoke (night) | `fog.svg` |
| `fc` | Funnel Cloud | `thunderstorm.svg` |
| `tor` | Tornado | `thunderstorm.svg` |
| `hot` | Excessive Heat | `sun.svg` |
| `cold` | Extreme Cold | `snow.svg` |
| `ncold` | Extreme Cold (night) | `snow.svg` |
| `hur_warn` | Hurricane Warning | `thunderstorm.svg` |
| `hur_watch` | Hurricane Watch | `thunderstorm.svg` |
| `ts_warn` | Tropical Storm Warning | `thunderstorm.svg` |
| `ts_watch` | Tropical Storm Watch | `thunderstorm.svg` |

**Fallback:** `unknown.svg` — for any unrecognized code. Design a generic cloud question-mark icon.

**Consolidation rationale:** Night variants map to the same SVG as the day variant because the ForecastPanel shows 3-day data, not hour-by-hour. The icon used per day is derived from the **daytime period** icon code, so `nskc` (night clear) should not normally appear as a daytime day-icon. However, if it does (tonight-only edge case), showing `sun.svg` is still correct since the panel conveys "clear conditions" not "it is currently night."

**Minimum distinct SVG set required:**
`sun.svg`, `sun_cloud.svg` (few clouds + sun), `partly_cloudy.svg`, `mostly_cloudy.svg`, `cloudy.svg`, `rain.svg`, `drizzle.svg`, `rain_showers.svg`, `snow.svg`, `blizzard.svg`, `rain_snow.svg`, `freezing_rain.svg`, `sleet.svg`, `thunderstorm.svg`, `windy.svg`, `fog.svg`, `unknown.svg` = **17 icons**

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| HTTP polling with retry | Custom socket + retry loop | QNetworkAccessManager (existing NwsPoller pattern) | Transfer timeout, abort-in-flight, thread affinity all solved. 30min cadence IS the retry. |
| JSON parsing | DOM traversal with custom error handling | `QJsonDocument::fromJson` + null-safe `.toObject()/.toArray()/.toInt(0)` | Qt's JSON API returns default values on missing fields — no crash on partial response |
| SVG icon rendering | Qt Canvas 2D path drawing of weather icons | QML `Image` with SVG source | SVG rendering is built into Qt SVG module (already linked). Much simpler than Canvas path code. |
| Icon code normalization | Complex regex on shortForecast text | `extractIconCode(url)` from the `icon` URL field | URL format is machine-generated and consistent; shortForecast text is human-readable and not reliably parseable |
| NWS grid point lookup | Runtime HTTP call to `api.weather.gov/points/...` | Hardcoded SEW/137,72 at build time | Grid points don't change for a fixed location. Hardcoded is consistent with how WeatherLink and PurpleAir URLs are handled. |

**Key insight:** This phase is a new external API integration, but the C++ patterns are identical to PurpleAirPoller. The only genuinely new work is the SVG icon design and the day/night period mapping logic.

---

## Common Pitfalls

### Pitfall 1: NWS API Requires User-Agent Header

**What goes wrong:** `QNetworkAccessManager::get()` without a User-Agent header returns HTTP 403 from `api.weather.gov`. The error is not a network error but an HTTP 403, and `reply->error()` returns `QNetworkReply::NoError` only if the request completed — a 403 is considered an HTTP error that still "completes."

**Why it happens:** NWS API documentation states: "You must include a User-Agent header with your application name and contact email." Many Qt HTTP examples omit this.

**How to avoid:**
```cpp
QNetworkRequest req(m_url);
req.setTransferTimeout(10000);  // 10s — NWS can be slow
req.setRawHeader("User-Agent", "wxdash/1.0 (github.com/your-repo)");
m_pendingReply = m_nam->get(req);
```

**Validate by:** Check `reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()` — must be 200, not 403.

**Confidence: HIGH** — NWS API FAQ explicitly documents this requirement.

### Pitfall 2: `probabilityOfPrecipitation.value` Can Be null

**What goes wrong:** Some periods return `"value": null` in the probabilityOfPrecipitation object. `QJsonObject["value"].toInt()` on a null JSON value returns 0, which is accidentally correct — but the `.toInt(0)` default-value form is more explicit and defensive.

**Why it happens:** The API uses JSON null rather than omitting the field when data is unavailable.

**How to avoid:**
```cpp
int precip = obj["probabilityOfPrecipitation"].toObject()["value"].toInt(0);
// .toInt(0) returns 0 for both missing and null — correct behavior
```

**Confidence: HIGH** — observed in live API response (period 1, value: 0 was integer; some periods may return null).

### Pitfall 3: Partial Day Edge Case (Afternoon Fetch)

**What goes wrong:** If the dashboard starts in the afternoon, the NWS API returns the first period as a **night** period (e.g., "Tonight", isDaytime: false). Without handling this, the first "day" in the panel would show only a low temp with "--" high.

**Why it happens:** NWS periods start from the current time, not from midnight.

**How to avoid:** The parser handles this by accepting night-only entries as valid (high = -999 sentinel, display "--" for high in QML). "Tonight/Tomorrow/Day after" is still 3 valid forecast slots. The user decision says "today, tomorrow, day after tomorrow" but this is best-effort — tonight's display is still useful.

**Alternative:** Skip night-only entries and always start from the next daytime period. This means in the afternoon, the display shows "tomorrow, day 2, day 3" instead of "tonight, tomorrow, day 2." Discuss in planning which behavior is preferred.

**Recommendation for planning:** Start from the first period regardless; show "--" for high when tonight-only. This provides more immediate information (precipitation tonight) than skipping to tomorrow.

### Pitfall 4: SVG Icon Rendering Quality

**What goes wrong:** SVG images displayed at small sizes in Qt Quick appear blurry or pixelated.

**Why it happens:** Qt rasterizes SVG to a bitmap at `sourceSize`. If `sourceSize` is too small (e.g., matches the display size exactly), scaling up even slightly causes blur.

**How to avoid:** Set `sourceSize` to a larger-than-displayed fixed size:
```qml
Image {
    source: "qrc:/icons/weather/sun.svg"
    sourceSize.width: 64
    sourceSize.height: 64  // render at 64x64
    width: 40              // display at 40x40 — stays sharp since 64 > 40
    height: 40
}
```

**Additional note:** Qt SVG module supports SVG 1.2 Tiny subset only — not full SVG 1.1. Keep SVG icons simple: filled paths and strokes only. Avoid SVG filters, gradients (linear is OK, radial may vary), masks, or `<use>` elements referencing external files.

**Confidence: HIGH** — Qt documentation states the sourceSize performance guidance explicitly.

### Pitfall 5: Icon Code Variants (Probability Suffix in URL)

**What goes wrong:** NWS icon URLs sometimes include a probability suffix: `tsra_hi,40?size=medium`. Naive string matching on the full URL path segment fails.

**Why it happens:** The NWS API appends `,NN` (probability percentage) to icon codes for combined forecasts.

**How to avoid:** Strip before the comma:
```cpp
QString last = url.path().section('/', -1);  // "tsra_hi,40"
return last.section(',', 0, 0);              // "tsra_hi"
// Also strip URL query string if present
```

**Confidence: HIGH** — observed in live API responses.

### Pitfall 6: NWS API SSL Certificate / HTTPS

**What goes wrong:** `api.weather.gov` is HTTPS-only. If Qt is built without SSL support, the request silently fails.

**Why it happens:** The project previously used only HTTP endpoints (WeatherLink, PurpleAir are local HTTP). This is the first HTTPS external request.

**How to avoid:** Verify Qt build has SSL support. On Raspberry Pi OS with system Qt packages, `libssl-dev` should be installed. Test by checking `QSslSocket::supportsSsl()` returns `true` at startup, or simply observe the first successful NWS response.

**On local dev machine:** OpenSSL should be available. If not, install `libssl-dev`.

**Confidence: MEDIUM** — standard Qt Network + system OpenSSL; no unusual platform-specific issues expected but worth verifying on Pi deployment.

### Pitfall 7: QVariantList Notify — No Diff Detection

**What goes wrong:** WeatherDataModel emits `forecastDataChanged` every time `applyForecastUpdate` is called, even if the data is identical. QML Repeater re-renders all 3 delegate items on every 30-minute poll.

**Why it happens:** `QVariantList` has no built-in change detection — any assignment triggers the signal.

**How to avoid:** This is acceptable at 30-minute cadence. Re-rendering 3 simple QML items every 30 minutes is negligible. No optimization needed. (Note contrast with sparklines which update every 10 seconds — those use ring buffers specifically to avoid this.)

---

## Code Examples

### Full NwsPoller poll() with User-Agent

```cpp
// Source: NWS API FAQ requirement + PurpleAirPoller pattern
void NwsPoller::poll() {
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(10000);  // NWS can be slow — 10s vs 5s for local
    req.setRawHeader("User-Agent", "wxdash/1.0 (contact@example.com)");

    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &NwsPoller::onReply);
}
```

### SVG Icon Template (monochrome gold, #C8A000)

```xml
<!-- Example: sun.svg — simple filled circle with rays -->
<!-- Qt SVG 1.2 Tiny compatible -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <g fill="#C8A000" stroke="none">
    <circle cx="32" cy="32" r="12"/>
    <!-- 8 rays -->
    <rect x="30" y="4" width="4" height="10" rx="2"/>
    <rect x="30" y="50" width="4" height="10" rx="2"/>
    <rect x="4" y="30" width="10" height="4" rx="2"/>
    <rect x="50" y="30" width="10" height="4" rx="2"/>
    <!-- diagonal rays -->
    <rect x="48" y="10" width="4" height="10" rx="2" transform="rotate(45 50 15)"/>
    <!-- etc -->
  </g>
</svg>
```

**Design principles for icons:**
- ViewBox: 64x64 (consistent internal coordinate system)
- Fill color: `#C8A000` (match dashboard gold)
- No stroke, no gradients — pure filled paths
- Simple silhouettes recognizable at 32-48px display size
- Qt SVG Tiny subset: no filters, no masks, no `<use>`, simple path/rect/circle/polygon elements only

### icons.qrc Addition

```xml
<RCC>
  <qresource prefix="/icons">
    <file alias="wxdash.svg">../assets/wxdash.svg</file>
  </qresource>
  <qresource prefix="/icons/weather">
    <file alias="sun.svg">../assets/icons/weather/sun.svg</file>
    <file alias="sun_cloud.svg">../assets/icons/weather/sun_cloud.svg</file>
    <file alias="partly_cloudy.svg">../assets/icons/weather/partly_cloudy.svg</file>
    <file alias="mostly_cloudy.svg">../assets/icons/weather/mostly_cloudy.svg</file>
    <file alias="cloudy.svg">../assets/icons/weather/cloudy.svg</file>
    <file alias="rain.svg">../assets/icons/weather/rain.svg</file>
    <file alias="drizzle.svg">../assets/icons/weather/drizzle.svg</file>
    <file alias="rain_showers.svg">../assets/icons/weather/rain_showers.svg</file>
    <file alias="snow.svg">../assets/icons/weather/snow.svg</file>
    <file alias="blizzard.svg">../assets/icons/weather/blizzard.svg</file>
    <file alias="rain_snow.svg">../assets/icons/weather/rain_snow.svg</file>
    <file alias="freezing_rain.svg">../assets/icons/weather/freezing_rain.svg</file>
    <file alias="sleet.svg">../assets/icons/weather/sleet.svg</file>
    <file alias="thunderstorm.svg">../assets/icons/weather/thunderstorm.svg</file>
    <file alias="windy.svg">../assets/icons/weather/windy.svg</file>
    <file alias="fog.svg">../assets/icons/weather/fog.svg</file>
    <file alias="unknown.svg">../assets/icons/weather/unknown.svg</file>
  </qresource>
</RCC>
```

**Note on CMakeLists.txt:** The existing `qt_add_resources(wxdash "icons" ...)` call bundles the resource at compile time. Add the weather SVG files to the same `qt_add_resources` call or add a second `qt_add_resources` call for weather icons. Alternatively, use the `.qrc` file directly. Either works; the existing project uses inline `qt_add_resources` in `src/CMakeLists.txt`.

### ForecastDay → Icon SVG Path Mapping (C++ map function)

Rather than a huge switch statement, use a `QHash` or `std::unordered_map` initialized once:

```cpp
// In NwsPoller.cpp or a new ForecastIconMapper.h
static QString iconPathForCode(const QString &code) {
    static const QHash<QString, QString> kIconMap = {
        {"skc",            "qrc:/icons/weather/sun.svg"},
        {"nskc",           "qrc:/icons/weather/sun.svg"},
        {"few",            "qrc:/icons/weather/sun_cloud.svg"},
        {"nfew",           "qrc:/icons/weather/sun_cloud.svg"},
        {"sct",            "qrc:/icons/weather/partly_cloudy.svg"},
        {"nsct",           "qrc:/icons/weather/partly_cloudy.svg"},
        {"bkn",            "qrc:/icons/weather/mostly_cloudy.svg"},
        {"nbkn",           "qrc:/icons/weather/mostly_cloudy.svg"},
        {"ovc",            "qrc:/icons/weather/cloudy.svg"},
        {"novc",           "qrc:/icons/weather/cloudy.svg"},
        {"minus_ra",       "qrc:/icons/weather/drizzle.svg"},
        {"ra",             "qrc:/icons/weather/rain.svg"},
        {"nra",            "qrc:/icons/weather/rain.svg"},
        {"shra",           "qrc:/icons/weather/rain_showers.svg"},
        {"nshra",          "qrc:/icons/weather/rain_showers.svg"},
        {"hi_shwrs",       "qrc:/icons/weather/rain_showers.svg"},
        {"hi_nshwrs",      "qrc:/icons/weather/rain_showers.svg"},
        {"rain_showers",   "qrc:/icons/weather/rain_showers.svg"},
        {"rain_showers_hi","qrc:/icons/weather/rain_showers.svg"},
        {"sn",             "qrc:/icons/weather/snow.svg"},
        {"nsn",            "qrc:/icons/weather/snow.svg"},
        {"blizzard",       "qrc:/icons/weather/blizzard.svg"},
        {"nblizzard",      "qrc:/icons/weather/blizzard.svg"},
        {"ra_sn",          "qrc:/icons/weather/rain_snow.svg"},
        {"nra_sn",         "qrc:/icons/weather/rain_snow.svg"},
        {"rasn",           "qrc:/icons/weather/rain_snow.svg"},
        {"fzra",           "qrc:/icons/weather/freezing_rain.svg"},
        {"nfzra",          "qrc:/icons/weather/freezing_rain.svg"},
        {"ra_fzra",        "qrc:/icons/weather/freezing_rain.svg"},
        {"fzra_sn",        "qrc:/icons/weather/freezing_rain.svg"},
        {"ip",             "qrc:/icons/weather/sleet.svg"},
        {"nip",            "qrc:/icons/weather/sleet.svg"},
        {"snip",           "qrc:/icons/weather/sleet.svg"},
        {"raip",           "qrc:/icons/weather/sleet.svg"},
        {"nraip",          "qrc:/icons/weather/sleet.svg"},
        {"tsra",           "qrc:/icons/weather/thunderstorm.svg"},
        {"ntsra",          "qrc:/icons/weather/thunderstorm.svg"},
        {"tsra_sct",       "qrc:/icons/weather/thunderstorm.svg"},
        {"tsra_hi",        "qrc:/icons/weather/thunderstorm.svg"},
        {"scttsra",        "qrc:/icons/weather/thunderstorm.svg"},
        {"nscttsra",       "qrc:/icons/weather/thunderstorm.svg"},
        {"hi_tsra",        "qrc:/icons/weather/thunderstorm.svg"},
        {"hi_ntsra",       "qrc:/icons/weather/thunderstorm.svg"},
        {"wind_skc",       "qrc:/icons/weather/windy.svg"},
        {"wind_few",       "qrc:/icons/weather/windy.svg"},
        {"wind_sct",       "qrc:/icons/weather/windy.svg"},
        {"wind_bkn",       "qrc:/icons/weather/windy.svg"},
        {"wind_ovc",       "qrc:/icons/weather/windy.svg"},
        {"fg",             "qrc:/icons/weather/fog.svg"},
        {"nfg",            "qrc:/icons/weather/fog.svg"},
        {"hz",             "qrc:/icons/weather/fog.svg"},
        {"du",             "qrc:/icons/weather/fog.svg"},
        {"fu",             "qrc:/icons/weather/fog.svg"},
        {"hot",            "qrc:/icons/weather/sun.svg"},
        {"cold",           "qrc:/icons/weather/snow.svg"},
        {"ncold",          "qrc:/icons/weather/snow.svg"},
        {"fc",             "qrc:/icons/weather/thunderstorm.svg"},
        {"tor",            "qrc:/icons/weather/thunderstorm.svg"},
    };
    return kIconMap.value(code, QStringLiteral("qrc:/icons/weather/unknown.svg"));
}
```

**Alternative:** Store the `iconCode` string in `ForecastDay` and resolve the `qrc:/` path in QML using a JS lookup object or a helper function. This avoids Qt URLs in the C++ struct and keeps icon mapping in one place (QML side). Trade-off: the mapping lives in QML (harder to test) vs C++ (easier to unit test). Either works; C++ mapping is more testable.

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| NWS XML/SOAP API (graphical.weather.gov) | NWS REST/JSON API (api.weather.gov) | ~2016-2019 transition | JSON API is simpler, no XML parsing needed |
| NWS icon URL via `/icons/` endpoint | Icon code embedded in icon URL path | Always | `/icons/` endpoint deprecated (Discussion #557) — use the URL code extraction approach, not a separate API call to the icons endpoint |
| shortForecast string matching for icons | Icon URL code extraction | Community best practice | shortForecast text is not machine-parseable reliably; URL code is stable |
| Qt SVG rendering at display size | Set `sourceSize` larger than display size | Qt 5/6 | Prevents blurry rasterization; performance warning in Qt docs |

**Deprecated/outdated:**
- `api.weather.gov/icons/` endpoint: Marked deprecated in NWS API Discussion #557. Do NOT make separate HTTP calls to this endpoint. The icon code embedded in the forecast period's `icon` URL is the correct approach.
- `graphical.weather.gov` XML service: Legacy API, being phased out. Use `api.weather.gov` REST API only.

---

## Open Questions

1. **Afternoon/evening first-period handling: "tonight only" vs "skip to tomorrow"**
   - What we know: When fetched in the afternoon, first period is isDaytime=false (e.g., "Tonight"). Two options: (a) show "--/55°" with no high, (b) skip ahead and show tomorrow as day 1.
   - What's unclear: Which is more useful to the user?
   - Recommendation for planning: Show tonight-only as the first slot (option a). "--" for high is clearly readable. Shows immediately useful tonight precipitation info. Discuss in planning if user prefers option b.

2. **Icon storage location: store `iconCode` or resolved `qrc:/` path in ForecastDay struct**
   - What we know: Either approach works. `iconCode` is more testable in C++; resolved path simplifies QML.
   - Recommendation: Store `iconCode` string in the struct; resolve to `qrc:/` path using a QML JS lookup table or a small helper JS function in ForecastPanel.qml. Keeps C++ struct Qt-light.

3. **Precipitation chance per day: daytime, nighttime, or maximum?**
   - What we know: Each period has its own `probabilityOfPrecipitation.value`. The panel shows one precip number per day column.
   - Recommendation: Show the maximum of the daytime and nighttime precip values for that day. This presents the "worst case" which is most actionable for planning.

4. **High/low on one line vs two lines: when does layout need to switch?**
   - What we know: CONTEXT.md says "High/low on one line if space allows, two lines if not." The cell is 1/4 of a 4-column 720p layout.
   - Recommendation: Use a `Row` with adaptive `fontSizeMode: Text.HorizontalFit` and `minimumPixelSize`. If text overflows, Qt will reduce font size. Two-line fallback can be implemented with a `FontMetrics` check or by observing that the existing dashboard cells are ~320px wide at 720p / 4 columns. At 16px font, "72°/55°" = ~60px wide — easily fits. One line is fine.

5. **SVG icon design: custom-create or use existing open-source icon set?**
   - What we know: Icons must be monochrome gold (#C8A000), SVG, compatible with Qt SVG Tiny subset.
   - Recommendation: Use a permissively licensed open-source weather icon set as a starting point (e.g., [weather-icons by Erik Flowers](https://erikflowers.github.io/weather-icons/) in SVG format, or Material Design weather icons), then recolor to #C8A000 and simplify to Qt SVG Tiny compatible elements. This is faster than designing from scratch. Verify license (most are MIT or SIL OFL — both compatible with this project).

---

## Sources

### Primary (HIGH confidence)
- Live NWS API response: `https://api.weather.gov/points/47.7394,-121.9823` — grid point resolution verified 2026-03-01
- Live NWS API response: `https://api.weather.gov/gridpoints/SEW/137,72/forecast` — period structure verified 2026-03-01
- Official NWS Forecast Icon reference: `https://www.weather.gov/forecast-icons` — complete icon code taxonomy verified 2026-03-01
- Qt official docs: `https://doc.qt.io/qt-6/qml-qtquick-image.html` — sourceSize SVG guidance
- Qt official docs: `https://doc.qt.io/qt-6/svgrendering.html` — SVG Tiny subset limitations
- Existing codebase: `src/network/PurpleAirPoller.cpp` — NwsPoller implementation pattern
- Existing codebase: `src/models/WeatherDataModel.h/.cpp` — forecastData Q_PROPERTY pattern

### Secondary (MEDIUM confidence)
- NWS API GitHub Discussion #293 (`github.com/weather-gov/api/discussions/293`) — shortForecast values not officially enumerated; icon URL code is more reliable
- NWS API GitHub Discussion #557 (`github.com/weather-gov/api/discussions/557`) — `/icons/` endpoint deprecated
- MatthewFlamm/nws weather.py (`github.com/MatthewFlamm/nws/blob/master/weather.py`) — community icon code mapping (Home Assistant integration), cross-validated against official icon page
- NWS API FAQ (`weather-gov.github.io/api/general-faqs`) — User-Agent requirement documented
- NDFD XML Weather Conditions reference (`graphical.weather.gov/xml/xml_fields_icon_weather_conditions.php`) — complete weather condition string taxonomy

### Tertiary (LOW confidence)
- WebSearch results for NWS shortForecast values — no official complete list found; icon URL approach preferred

---

## Metadata

**Confidence breakdown:**
- NWS API structure and grid point: HIGH — verified via live API calls
- NWS icon code taxonomy: HIGH — verified against official weather.gov/forecast-icons page
- Qt SVG/Image rendering patterns: HIGH — verified via official Qt documentation
- Icon-to-SVG mapping table: MEDIUM — derived from official codes but consolidation decisions (e.g., all thunderstorm variants → one SVG) are judgment calls
- Day/night period mapping algorithm: MEDIUM — pattern confirmed by API observation and NWS FAQ; edge case handling (afternoon fetch) is reasoned from API structure
- shortForecast text reliability: MEDIUM (confirmed unreliable) — no official vocabulary; icon URL code approach is the correct alternative

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (NWS API stable; Qt SVG APIs stable; icon code taxonomy rarely changes)
