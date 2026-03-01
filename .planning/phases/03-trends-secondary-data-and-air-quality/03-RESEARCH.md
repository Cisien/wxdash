# Phase 3: Trends, Secondary Data, and Air Quality - Research

**Researched:** 2026-03-01
**Domain:** Qt6 QML Canvas/Shape rendering, ring buffers in C++, PurpleAir local HTTP API, EPA AQI calculation
**Confidence:** HIGH (architecture, Qt patterns, AQI math) / MEDIUM (PurpleAir field names confirmed by community but no official schema doc)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Dashboard Layout**
- 3x4 grid stays unchanged — no new rows or columns
- Cell 11 (currently ReservedCell labeled "AQI — Phase 3"): AQI multi-ring gauge
- Cell 12 (currently ReservedCell): Remains reserved for future forecast widget
- GAUG-13 (indoor panel) removed from this phase

**Sparkline Design**
- 24-hour time window
- Sample resolution adapts dynamically to available horizontal pixel count — lean toward high resolution
- Line only — no fill, no bars
- Dim neutral color, darker than the gauge track (#2A2A2A) so it never competes with gauge readings
- Renders in the lower third of each gauge cell, behind the existing gauge content — gauges do not resize
- Every outdoor ArcGauge gets a sparkline (temperature, feels-like, humidity, dew point, wind, rain rate, pressure, UV, solar, AQI)
- AQI gauge sparkline tracks only the AQI value (not PM2.5 or PM10)

**AQI Gauge (Cell 11)**
- Single gauge cell with three concentric arc rings sharing the same total stroke width as other gauges:
  - AQI (outermost): 1/2 of total arc width, colored by EPA 6-zone scheme (Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+)
  - PM2.5 (middle): 1/4 of total arc width, neutral color
  - PM10 (innermost): 1/4 of total arc width, neutral color
- Center text: AQI number displayed prominently
- PM2.5 value in smaller text at bottom-left of gauge
- PM10 value in smaller text at bottom-right of gauge
- When PurpleAir is offline: same staleness behavior as existing weather station (values cleared, stale signal emitted)

**PurpleAir Data Acquisition**
- Poll PurpleAir local sensor API (`http://10.1.255.41/json?live=false`) for PM2.5, PM10, and calculated AQI
- Average channels A and B for PM2.5, then calculate AQI from EPA breakpoint table
- Pull PM10 from the same API response (not in original requirements but same sensor, natural fit)

### Claude's Discretion

- Ring buffer implementation details for sparkline history storage
- Exact sparkline neutral color value (must be darker than #2A2A2A gauge track)
- PurpleAir polling interval
- AQI EPA breakpoint calculation implementation
- Sparkline sampling/decimation algorithm for adapting to pixel width

### Deferred Ideas (OUT OF SCOPE)

- GAUG-13 (indoor temperature and humidity panel) — moved to future phase
- Cell 12 forecast widget — future phase
- PM10 was not in original requirements — added naturally from same PurpleAir API response (PM10 IS in scope per the AQI gauge design, but GAUG-13 indoor panel is NOT)
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DATA-06 | App polls PurpleAir sensor (`http://10.1.255.41/json?live=false`) for PM2.5 data | PurpleAir local HTTP API fields documented; HttpPoller pattern from Phase 1 is the implementation vehicle |
| DATA-07 | PurpleAir channels A and B averaged (PM2.5 averaged, then AQI calculated via EPA breakpoint table) | EPA 2024 breakpoint table documented; linear interpolation formula established; field names `pm2_5_atm` and `pm2_5_atm_b` confirmed |
| GAUG-11 | AQI gauge with EPA 6-zone color coding (Green 0-50, Yellow 51-100, Orange 101-150, Red 151-200, Purple 201-300, Maroon 301+) | Multi-ring Shape/ShapePath arc pattern confirmed; concentric arcs in single Shape recommended by Qt docs |
| GAUG-12 | PM2.5 gauge showing averaged A+B sensor value | Implemented as middle ring of AQI gauge per CONTEXT.md decision; same PurpleAir data source as DATA-06 |
| GAUG-13 | Indoor temperature and humidity panel | **DEFERRED** — removed from this phase per user decision |
| TRND-01 | Sparkline mini-graphs showing last few hours of data for key sensors | Canvas QML or Shape PathPolyline confirmed; Canvas is simpler and already used in CompassRose |
| TRND-02 | In-memory ring buffer storage for sparkline history (10s cadence) | Established C++17 ring buffer pattern using fixed-size array + head/count indices, mirrors wind rose pattern in WeatherDataModel |
</phase_requirements>

---

## Summary

Phase 3 adds three distinct capabilities to the existing dashboard: sparkline overlays on every outdoor ArcGauge, a PurpleAir data poller and parser, and a three-ring AQI/PM2.5/PM10 gauge in Cell 11. All three fit cleanly into the patterns established in Phases 1 and 2.

The sparklines are background Canvas elements rendered in the lower third of each gauge cell, below the existing QML gauge content using QML's z-order stacking (lower z renders behind). The ring buffer for sparkline data follows the exact pattern already in WeatherDataModel for the wind rose histogram — a fixed-size C++ array with head/count indices, exposed to QML as a QVariantList via Q_PROPERTY. The Canvas draws a polyline by iterating the data array and calling lineTo() for each sample.

The PurpleAir integration mirrors the HttpPoller pattern from Phase 1 almost exactly: a new PurpleAirPoller class lives on the network thread, polls the local sensor's `/json` endpoint, parses the JSON for `pm2_5_atm`, `pm2_5_atm_b`, `pm10_0_atm`, and `pm10_0_atm_b` fields, averages the A/B PM2.5 channels, calculates AQI using the 2024 EPA breakpoint table with linear interpolation, and emits a signal that WeatherDataModel receives on the main thread. A new PurpleAirReading struct follows the zero-Qt-dependency plain C++ struct pattern from WeatherReadings.h.

The AQI gauge uses a single Shape with three ShapePath elements at different radii — Qt documentation explicitly recommends one Shape with multiple ShapePaths over multiple Shapes for performance. The concentric ring layout (AQI outer, PM2.5 middle, PM10 inner) partitions the existing total strokeWidth used by ArcGauge, keeping visual consistency.

**Primary recommendation:** Implement as three distinct plans: (1) ring buffer infrastructure + sparkline Canvas overlay, (2) PurpleAir poller + parser + model properties, (3) AQI multi-ring gauge in Cell 11.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick (Canvas) | Qt 6.x (project-established) | Sparkline 2D line drawing | Already used in CompassRose.qml; Canvas Context2D is the established project pattern for custom 2D drawing |
| Qt Quick Shapes | Qt 6.x (project-established) | AQI concentric arc rings | Already used in ArcGauge.qml; single Shape with multiple ShapePath elements is Qt's recommended approach |
| Qt Network (QNetworkAccessManager) | Qt 6.x (project-established) | PurpleAir HTTP polling | Already used in HttpPoller; same pattern reused for PurpleAirPoller |
| Qt Core (QJsonDocument) | Qt 6.x (project-established) | PurpleAir JSON parsing | Already used in JsonParser; same stateless namespace parsing approach |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| QTimer | Qt 6.x | PurpleAir poll interval | Same QTimer-driven poll cadence as HttpPoller |
| QVariantList | Qt 6.x | Sparkline history exposed to QML | Same pattern as windRoseData Q_PROPERTY |
| QPolygonF | Qt 6.x | PathPolyline path binding (alternative to Canvas) | Only if Canvas performance is insufficient — Canvas approach preferred for consistency with CompassRose |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Canvas (sparklines) | Shape + PathPolyline | PathPolyline is GPU-native, never rasterized; but requires QPolygonF C++ type and adds QML complexity. Canvas is simpler and already established in CompassRose. Either works at this scale. |
| Canvas (sparklines) | Qt Charts (LineSeries + ChartView) | Qt Charts requires an additional module link (Qt6::Charts), adds significant overhead for a simple background sparkline. Rejected — overkill. |
| Separate PurpleAirPoller class | Extending HttpPoller | Extension would complicate HttpPoller beyond its responsibility. New class with same pattern is cleaner. |

**No new npm/CMake packages needed.** All required Qt modules are already linked (`Qt6::Core`, `Qt6::Network`, `Qt6::Quick`, `Qt6::Svg`). `Qt6::Quick` includes QtQuick.Shapes.

**No installation required** — all modules already in CMakeLists.txt.

---

## Architecture Patterns

### Recommended Plan Structure

```
Plan 03-01: Sparkline ring buffers + Canvas overlay in ArcGauge
  - C++ ring buffer per sensor in WeatherDataModel
  - Sparkline Canvas element added to ArcGauge.qml
  - Sparkline data wired from model properties to ArcGauge

Plan 03-02: PurpleAir poller, parser, and model properties
  - PurpleAirReading struct in WeatherReadings.h
  - PurpleAirPoller class (mirrors HttpPoller pattern)
  - WeatherDataModel: aqi, pm25, pm10 Q_PROPERTYs + ring buffers
  - main.cpp wiring (new thread or shared network thread)
  - tst_PurpleAirParser unit tests

Plan 03-03: AQI multi-ring gauge in Cell 11
  - AqiGauge.qml: single Shape, three ShapePaths at different radii
  - DashboardGrid.qml: replace ReservedCell in Cell 11 with AqiGauge
  - aqiColor() threshold function in DashboardGrid.qml
```

### Pattern 1: Ring Buffer for Sparkline History (C++)

**What:** Fixed-size circular buffer of double values stored in WeatherDataModel, one per sparkline sensor. Head/count tracking matches the wind rose ring buffer pattern already in the model.

**When to use:** Any time-series data needed for sparklines.

**Example (mirrors existing wind rose pattern):**
```cpp
// In WeatherDataModel.h — same pattern as wind rose ring buffer
static constexpr int kSparklineCapacity = 8640; // 24h at 10s cadence

struct SparklineSample { double value; };
SparklineSample m_tempHistory[kSparklineCapacity] = {};
int m_tempHistoryHead = 0;
int m_tempHistoryCount = 0;

// Accessor for QML — same pattern as windRoseData()
Q_PROPERTY(QVariantList temperatureHistory READ temperatureHistory NOTIFY temperatureHistoryChanged)
QVariantList temperatureHistory() const;
```

```cpp
// In WeatherDataModel.cpp
void WeatherDataModel::recordSample(SparklineSample* ring, int& head, int& count, double value) {
    ring[head] = {value};
    head = (head + 1) % kSparklineCapacity;
    if (count < kSparklineCapacity) count++;
}
```

**Key decision (Claude's discretion):** Capacity of 8640 samples = 24h at 10s ISS poll cadence. UDP wind updates arrive at 2.5s but wind sparkline can sample at 10s cadence off the ISS update path to avoid 4x storage cost.

**AQI sparkline storage:** Stored in WeatherDataModel alongside aqi/pm25/pm10 properties, sampled at PurpleAir poll interval. 24h capacity at 30s interval = 2880 samples.

### Pattern 2: Sparkline Canvas Overlay in ArcGauge

**What:** A Canvas element is added to ArcGauge.qml as a background layer (z lower than the existing Shape content). The Canvas receives the sparkline data array as a property from DashboardGrid.qml and redraws when data changes.

**When to use:** Every outdoor ArcGauge.

**Example:**
```qml
// In ArcGauge.qml — added below the track arc Shape, above nothing (background)
property var sparklineData: []      // QVariantList from model
property int sparklineCount: 0      // number of valid samples

Canvas {
    id: sparklineCanvas
    anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
    }
    height: parent.height * 0.25   // lower third of cell

    property var data: root.sparklineData
    property int count: root.sparklineCount

    onDataChanged: requestPaint()
    onCountChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        if (!data || count < 2) return

        // Decimation: stride through ring buffer to fit pixel width
        var stride = Math.max(1, Math.floor(count / width))

        ctx.beginPath()
        ctx.strokeStyle = "#1A1A1A"   // darker than #2A2A2A gauge track
        ctx.lineWidth = 1

        // Find min/max for normalization
        var minV = data[0], maxV = data[0]
        for (var i = 1; i < count; i++) {
            if (data[i] < minV) minV = data[i]
            if (data[i] > maxV) maxV = data[i]
        }
        var range = maxV - minV
        if (range === 0) range = 1

        var first = true
        for (var j = 0; j < count; j += stride) {
            var x = (j / count) * width
            var y = height - ((data[j] - minV) / range) * height * 0.9
            if (first) { ctx.moveTo(x, y); first = false }
            else ctx.lineTo(x, y)
        }
        ctx.stroke()
    }
}
```

**Important:** Canvas z-ordering — the Canvas is declared before the existing gauge Shape elements in ArcGauge.qml, so it renders behind them. Or set explicit `z: -1` on the Canvas.

**QML data binding:** `sparklineData` is a `QVariantList` property on WeatherDataModel, returned as a flat list of doubles. ArcGauge accepts it as `property var sparklineData: []`. DashboardGrid wires the correct model property to each ArcGauge instance.

### Pattern 3: PurpleAir Poller (mirrors HttpPoller exactly)

**What:** A new `PurpleAirPoller` class on the network thread, identical architecture to HttpPoller. Polls `/json?live=false`, parses JSON, emits a `PurpleAirReading` signal.

**Example:**
```cpp
// WeatherReadings.h — new struct, plain C++, no Qt dependencies
struct PurpleAirReading {
    double pm25_a  = 0.0;  // pm2_5_atm Channel A (ug/m3)
    double pm25_b  = 0.0;  // pm2_5_atm_b Channel B (ug/m3)
    double pm25avg = 0.0;  // averaged A+B
    double pm10    = 0.0;  // pm10_0_atm (Channel A, no B averaging needed)
    double aqi     = 0.0;  // calculated from pm25avg via EPA breakpoints
};
Q_DECLARE_METATYPE(PurpleAirReading)
```

```cpp
// PurpleAirPoller.h — same pattern as HttpPoller
class PurpleAirPoller : public QObject {
    Q_OBJECT
public:
    static constexpr int kPollIntervalMs = 30000; // 30s (Claude's discretion)
    explicit PurpleAirPoller(const QUrl &url, QObject *parent = nullptr);
public slots:
    void start();
signals:
    void purpleAirReceived(PurpleAirReading reading);
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

**Thread strategy:** PurpleAirPoller can share the existing `networkThread` from main.cpp. The existing networkThread starts both HttpPoller and UdpReceiver; adding PurpleAirPoller follows the same `moveToThread` + `QThread::started` → `start()` pattern.

### Pattern 4: AQI Multi-Ring Gauge (single Shape, three ShapePaths)

**What:** `AqiGauge.qml` contains a single Shape with three ShapePath/PathAngleArc elements at different radii, partitioning the total strokeWidth from ArcGauge.

**Qt recommendation:** "Prefer using one Shape item with multiple ShapePath elements over multiple Shape items" (Qt docs, Shape QML Type). A property change in one ShapePath only reprocesses that path.

**Concentric ring radius calculation:**
```qml
// Total arc geometry matches ArcGauge constants exactly
readonly property real totalStrokeWidth: Math.min(width, height) * 0.08  // same as ArcGauge
// AQI: outermost, half total width
readonly property real aqiStroke: totalStrokeWidth * 0.5
// PM2.5: middle, quarter total width
readonly property real pm25Stroke: totalStrokeWidth * 0.25
// PM10: innermost, quarter total width
readonly property real pm10Stroke: totalStrokeWidth * 0.25

// Radius for each ring — center of its stroke band
readonly property real outerRadius:  (Math.min(width, height) / 2) - (aqiStroke / 2)
readonly property real middleRadius: outerRadius - aqiStroke/2 - pm25Stroke/2
readonly property real innerRadius:  middleRadius - pm25Stroke/2 - pm10Stroke/2
```

**Example ShapePath structure:**
```qml
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property real aqiValue:  0.0   // 0-500
    property real pm25Value: 0.0   // ug/m3
    property real pm10Value: 0.0   // ug/m3

    Shape {
        anchors.fill: parent

        // Track arc — AQI (outer)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.aqiStroke
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: 125; sweepAngle: 290
            }
        }

        // Fill arc — AQI (outer, EPA color)
        ShapePath {
            strokeColor: root.aqiColor(root.aqiValue)
            fillColor: "transparent"
            strokeWidth: root.aqiStroke
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: 125
                sweepAngle: root.aqiSweep   // animated, same Binding pattern
            }
        }

        // Track arc — PM2.5 (middle)
        ShapePath { /* similar, neutral color, middleRadius */ }

        // Fill arc — PM2.5 (middle)
        ShapePath { /* similar, middleRadius */ }

        // Track arc — PM10 (inner)
        ShapePath { /* similar, neutral color, innerRadius */ }

        // Fill arc — PM10 (inner)
        ShapePath { /* similar, innerRadius */ }
    }
}
```

**Animation:** Same `Behavior on animatedSweep` + `Binding{}` pattern as ArcGauge.qml — one animatedSweep property per ShapePath fill arc. Three `SmoothedAnimation` instances in total.

### Pattern 5: EPA AQI Calculation (C++, stateless)

**What:** Pure C++ function in JsonParser namespace (or a new AqiCalculator namespace) that maps averaged PM2.5 to AQI using linear interpolation between breakpoint pairs.

**2024 EPA Breakpoints (effective May 6, 2024):**
```cpp
// Source: IQAir/EPA May 2024 update
// Each row: {concLo, concHi, aqiLo, aqiHi}
struct AqiBreakpoint { double concLo, concHi; int aqiLo, aqiHi; };

static constexpr AqiBreakpoint kPm25Breakpoints[] = {
    {  0.0,   9.0,   0,  50 },   // Good (2024 update: was 12.0)
    {  9.1,  35.4,  51, 100 },   // Moderate
    { 35.5,  55.4, 101, 150 },   // Unhealthy for Sensitive Groups
    { 55.5, 150.4, 151, 200 },   // Unhealthy (2024 update: was 125.4)
    {150.5, 250.4, 201, 300 },   // Very Unhealthy (2024 update: was 225.4)
    {250.5, 500.4, 301, 500 },   // Hazardous (2024 update: was 325.4)
};

int calculateAqi(double pm25) {
    for (const auto& bp : kPm25Breakpoints) {
        if (pm25 >= bp.concLo && pm25 <= bp.concHi) {
            return qRound(
                ((pm25 - bp.concLo) / (bp.concHi - bp.concLo))
                * (bp.aqiHi - bp.aqiLo) + bp.aqiLo
            );
        }
    }
    return (pm25 > 500.4) ? 500 : 0;
}
```

**Note on 2024 vs pre-2024 breakpoints:** The EPA updated PM2.5 breakpoints effective May 6, 2024. The "Good" threshold dropped from 12.0 to 9.0 µg/m³. Use the 2024 breakpoints shown above. Many older implementations (including the `simonw/til` AQI calculation) use pre-2024 values — those are now outdated.

**MEDIUM confidence caveat:** The IQAir source confirmed 0-9.0 as "Good" and the May 2024 effective date. The intermediate breakpoints (35.4, 55.4, 150.4, 250.4, 500.4) align with the original EPA TAD document upper boundaries. The EPA TAD PDF was inaccessible for direct verification. Validate against the official EPA Technical Assistance Document before shipping.

### Pattern 6: AQI Color Function in DashboardGrid.qml

**What:** New `aqiColor(aqi)` function in DashboardGrid.qml, same style as `temperatureColor()`, `humidityColor()`, `uvColor()`.

```qml
function aqiColor(aqi) {
    if (aqi <= 50)  return "#5CA85C"    // Green (Good) — matches uvColor green
    if (aqi <= 100) return "#C8A000"    // Yellow (Moderate) — matches existing yellow
    if (aqi <= 150) return "#C87C2A"    // Orange (USG) — matches existing orange
    if (aqi <= 200) return "#C84040"    // Red (Unhealthy) — matches existing red
    if (aqi <= 300) return "#8B5CA8"    // Purple (Very Unhealthy) — matches uvColor violet
    return "#7B2828"                     // Maroon (Hazardous) — new, subdued maroon
}
```

**Note:** Color choices follow the existing subdued palette convention. Maroon (#7B2828) is new — darker/less saturated red-brown. The exact hex is Claude's discretion but must be visually distinct from the red and fit the subdued theme.

### Anti-Patterns to Avoid

- **Multiple Shape elements for concentric rings:** Qt explicitly recommends one Shape with multiple ShapePaths. Using multiple Shapes adds unnecessary rendering overhead.
- **Qt Charts (ChartView/LineSeries) for sparklines:** Massive overkill for a background sparkline. Adds a whole module link and heavyweight rendering pipeline.
- **Storing sparkline history as QVariantList (resizable):** Use fixed-size C++ arrays like the wind rose ring buffer. Growing Qt containers on every update creates heap churn.
- **Sharing sparkline Canvas between multiple gauge cells:** Each ArcGauge gets its own Canvas. The Canvas element is lightweight; sharing would require complex coordination.
- **Polling PurpleAir faster than 30s:** The sensor hardware has limited processing; over-polling provides no additional data and risks sensor instability.
- **Using the pre-2024 EPA breakpoints:** The old "Good" threshold was 12.0 µg/m³; the 2024 update lowered it to 9.0 µg/m³. The old table is wrong as of May 2024.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Circular buffer | Custom linked list or vector-based deque | Fixed-size array + head/count (as in wind rose) | Already pattern-matched in the codebase; simpler, no heap allocation |
| HTTP polling | Custom socket management | QNetworkAccessManager (existing pattern) | Thread affinity, transfer timeout, abort-in-flight already solved in HttpPoller |
| Canvas line drawing | Custom GPU shaders | Canvas Context2D `beginPath/lineTo/stroke` | Simple 2D API, sufficient for sparklines, already used in CompassRose |
| AQI EPA color mapping | Dynamic color computation | Simple if/else ladder (as in uvColor()) | EPA zones are discrete fixed breakpoints, not a continuous gradient |

**Key insight:** This phase is almost entirely pattern reuse. The wind rose ring buffer, HttpPoller, Canvas drawing, and DashboardGrid threshold functions established in Phases 1 and 2 all have direct counterparts in Phase 3.

---

## Common Pitfalls

### Pitfall 1: Ring Buffer QVariantList Return — Copy on Every Call

**What goes wrong:** If `sparklineHistory()` accessor builds a new QVariantList from the raw array on every call, QML's property binding system can invoke it frequently, causing unnecessary O(n) copies.

**Why it happens:** QML bindings re-evaluate when NOTIFY signals fire. If Canvas connects to the model property directly and the model emits on every 10s ISS update, it rebuilds the list 8640 times per day.

**How to avoid:** Emit `sparklineHistoryChanged` only when the sparkline data actually changes (it always changes on each 10s poll, so this is inherent). Consider: the Canvas `onPaint` reads the data array once per paint; the QVariantList accessor is called once per paint, not continuously. Performance concern is LOW given the actual usage pattern, but keep the accessor simple.

**Warning signs:** High CPU usage at idle. Profile if concerned.

### Pitfall 2: Canvas z-Ordering in ArcGauge

**What goes wrong:** Canvas declared after the existing Shape elements renders on top of the gauge, obscuring the arc and text.

**Why it happens:** QML renders children in declaration order (later = on top). z property overrides this.

**How to avoid:** Declare the sparkline Canvas as the first child of ArcGauge's root Item, OR set `z: -1` on the Canvas explicitly. The existing Shape elements keep their default z:0.

**Warning signs:** Gauge arc or text invisible; sparkline line visible over gauge face.

### Pitfall 3: PurpleAir JSON Field Names — pm2_5_atm vs pm2.5_aqi

**What goes wrong:** The PurpleAir local `/json` endpoint has inconsistent field naming documented in firmware history. Some fields use dots (like `pm2.5_aqi`) and others use underscores. Channel B fields use the `_b` suffix.

**Why it happens:** PurpleAir's field documentation is community-maintained and evolving. The sensor at `10.1.255.41` is running whatever firmware version is installed.

**How to avoid:** Before writing the parser, GET the actual sensor response once: `curl http://10.1.255.41/json?live=false | python3 -m json.tool`. Verify field names `pm2_5_atm`, `pm2_5_atm_b`, `pm10_0_atm`, `pm10_0_atm_b` exist. Fall back gracefully if any field is missing (return 0.0, not crash).

**Warning signs:** AQI gauge shows 0 even when sensor is online. Log parsed field values on first successful parse.

**Confirmed field names (MEDIUM confidence — community-verified, firmware 7.04):**
- Channel A PM2.5 atmospheric: `pm2_5_atm`
- Channel B PM2.5 atmospheric: `pm2_5_atm_b`
- Channel A PM10 atmospheric: `pm10_0_atm`
- Channel B PM10 atmospheric: `pm10_0_atm_b` (presence not confirmed for PM10, may be `pm10_0_atm` only)

### Pitfall 4: Staleness for Two Independent Sources

**What goes wrong:** WeatherDataModel currently has a single staleness timer and a single `m_sourceStale` flag. PurpleAir is a separate data source that can go offline independently of the WeatherLink.

**Why it happens:** The current model conflates "any data source" into one stale flag. Phase 4 adds per-source indicators (KIOSK-04), but Phase 3 still needs AQI values cleared when PurpleAir is offline.

**How to avoid:** WeatherDataModel needs a second staleness mechanism for PurpleAir: separate `m_purpleAirStale` bool, separate last-update timestamp, separate check in `checkStaleness()`. When PurpleAir is stale, clear `m_aqi`, `m_pm25`, `m_pm10` to 0.0. Emit `purpleAirStaleChanged(bool)` signal. AqiGauge.qml can use this to dim or indicate offline state. **Critically:** the existing `sourceStale` property and timer remain unchanged — don't merge PurpleAir into the weather station staleness.

**Warning signs:** AQI gauge shows stale values hours after PurpleAir goes offline.

### Pitfall 5: AQI Sparkline vs PM2.5/PM10 Sparklines — Data Source

**What goes wrong:** CONTEXT.md says "AQI gauge sparkline tracks only the AQI value." Storing separate sparkline histories for AQI, PM2.5, and PM10 in WeatherDataModel requires three ring buffers for the PurpleAir source alone.

**Why it happens:** The AQI gauge has a sparkline for AQI only — not PM2.5 or PM10. So only one PurpleAir ring buffer is needed (AQI values over time), not three.

**How to avoid:** Only create one PurpleAir sparkline ring buffer: AQI history. PM2.5 and PM10 arc fills are driven by current values, not history.

### Pitfall 6: Concentric Ring Stroke Width Math

**What goes wrong:** Concentric arcs overlap or leave gaps if radius adjustment doesn't account for half-strokes.

**Why it happens:** PathAngleArc radius is the center of the stroke. Two adjacent strokes of width W touch exactly when their center radii differ by W. If AQI stroke is W/2 and PM2.5 stroke is W/4, the center radius of PM2.5 must be `outerRadius - (W/2)/2 - (W/4)/2` = `outerRadius - W/4 - W/8`.

**How to avoid:** Calculate each ring's center radius precisely:
```
aqiCenterRadius  = outerR - aqiStroke/2
pm25CenterRadius = aqiCenterRadius - aqiStroke/2 - pm25Stroke/2
pm10CenterRadius = pm25CenterRadius - pm25Stroke/2 - pm10Stroke/2
```
Where `outerR = (Math.min(width, height) / 2) - aqiStroke/2` (to fit the outermost stroke inside the cell boundary).

**Warning signs:** Gaps or overlaps between rings visible at the edges of the strokes.

---

## Code Examples

### PurpleAir JSON Parsing

```cpp
// In JsonParser.cpp or a new PurpleAirParser.cpp (same stateless namespace)
// Source: PurpleAir community documentation, firmware 7.04
PurpleAirReading parsePurpleAirJson(const QByteArray &data) {
    PurpleAirReading result;
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return result;

    auto root = doc.object();
    result.pm25_a  = root.value("pm2_5_atm").toDouble(0.0);
    result.pm25_b  = root.value("pm2_5_atm_b").toDouble(0.0);
    result.pm10    = root.value("pm10_0_atm").toDouble(0.0);
    result.pm25avg = (result.pm25_a + result.pm25_b) / 2.0;
    result.aqi     = calculateAqi(result.pm25avg);
    return result;
}
```

### Sparkline Data Accessor

```cpp
// In WeatherDataModel.cpp — same pattern as windRoseData()
// Source: mirrors existing wind rose accessor in codebase
QVariantList WeatherDataModel::temperatureHistory() const {
    QVariantList list;
    list.reserve(m_tempHistoryCount);
    // Return in chronological order (oldest first)
    for (int i = 0; i < m_tempHistoryCount; i++) {
        int idx = (m_tempHistoryHead - m_tempHistoryCount + i + kSparklineCapacity) % kSparklineCapacity;
        list.append(m_tempHistory[idx].value);
    }
    return list;
}
```

### DashboardGrid.qml — wiring sparkline to ArcGauge

```qml
// In DashboardGrid.qml — adding sparklineData to existing ArcGauge
ArcGauge {
    value: weatherModel.temperature
    minValue: -20
    maxValue: 120
    label: "Temperature"
    unit: "\u00B0F"
    arcColor: temperatureColor(weatherModel.temperature)
    sparklineData: weatherModel.temperatureHistory   // NEW
    sparklineCount: weatherModel.temperatureHistoryCount  // NEW
    Layout.fillWidth: true
    Layout.fillHeight: true
}
```

### main.cpp wiring for PurpleAirPoller

```cpp
// In main.cpp — add after existing network setup
const QUrl purpleAirUrl("http://10.1.255.41/json?live=false");
auto *purpleAirPoller = new PurpleAirPoller(purpleAirUrl);
purpleAirPoller->moveToThread(networkThread);

QObject::connect(purpleAirPoller, &PurpleAirPoller::purpleAirReceived,
                 model, &WeatherDataModel::applyPurpleAirUpdate);
QObject::connect(networkThread, &QThread::started,
                 purpleAirPoller, &PurpleAirPoller::start);
QObject::connect(networkThread, &QThread::finished,
                 purpleAirPoller, &QObject::deleteLater);
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| EPA PM2.5 "Good" ≤ 12.0 µg/m³ | EPA PM2.5 "Good" ≤ 9.0 µg/m³ | May 6, 2024 | Old breakpoints still widely used in open-source AQI libraries — verify all references |
| Multiple Shape items for complex gauges | Single Shape with multiple ShapePaths | Qt 6 recommendation | Better GPU cacheability, one Shape processes only changed ShapePaths |
| QQuickPaintedItem for complex 2D | Canvas QML (for simple cases) | N/A | Canvas is simpler to wire to QML properties; QQuickPaintedItem preferred for large/animated areas |
| Qt Charts (Qt 5 era) for line graphs | Canvas Context2D for simple sparklines | Qt 6 era | Qt Charts overkill; Canvas sufficient for mini background sparklines |

**Deprecated/outdated:**
- Pre-2024 EPA AQI breakpoints: Still used in many open-source tools (purpleairpy, Home Assistant integrations, simonw/til). The breakpoints shown in those tools are outdated as of May 2024.
- `Qt.Charts.LineSeries` for sparklines: Works but heavyweight for this use case.

---

## Open Questions

1. **PurpleAir PM10 Channel B field name**
   - What we know: `pm10_0_atm` (Channel A) confirmed by community and Home Assistant examples. `pm2_5_atm_b` (Channel B PM2.5) confirmed. PM10 channel B field name unclear.
   - What's unclear: Is `pm10_0_atm_b` a real field? Or is PM10 only reported as Channel A?
   - Recommendation: `curl http://10.1.255.41/json?live=false` before writing the parser. If `pm10_0_atm_b` exists, average it. If not, use `pm10_0_atm` alone. The gauge shows PM10 for context only — averaging is a nice-to-have, not a requirement.

2. **PurpleAir polling interval (Claude's discretion)**
   - What we know: User left this to Claude's discretion. HttpPoller uses 10s; PurpleAir is a local sensor with slower-changing values.
   - Recommendation: **30 seconds.** PurpleAir hardware typically averages over 2-minute windows anyway; 30s is sufficient and avoids over-polling the embedded sensor's HTTP stack. This gives 2880 samples per 24h (manageable ring buffer size).

3. **Sparkline neutral color value (Claude's discretion)**
   - What we know: Must be darker than #2A2A2A (gauge track color). ArcGauge track is #2A2A2A.
   - Recommendation: **#181818** — noticeably darker than the track, still visible as a line on the #222222 cell background (from ReservedCell pattern), doesn't compete with the gold gauge content.

4. **Sparkline sampling/decimation for pixel width (Claude's discretion)**
   - What we know: "Sample resolution adapts dynamically to available horizontal pixel count — lean toward high resolution."
   - Recommendation: `stride = Math.max(1, Math.floor(count / width))`. At 24h × 10s = 8640 samples, a 150px-wide sparkline uses stride=57 (~150 points drawn). At 300px, stride=28 (~300 points). This naturally leans toward high resolution at wider cells.

5. **Sparkline data flow: push vs poll**
   - What we know: windRoseData is a Q_PROPERTY that returns a QVariantList and fires `windRoseDataChanged`. QML Canvas listens to `onDataChanged`.
   - Recommendation: Same push pattern. WeatherDataModel emits `temperatureHistoryChanged()` each time a new sample is recorded (every 10s ISS poll). Canvas calls `requestPaint()` in response. No polling.

---

## Validation Architecture

> `workflow.nyquist_validation` not present in `.planning/config.json` — section included because the project has established test infrastructure (Qt Test, two test executables) and new C++ logic (AQI calculation, parser, ring buffer) should be unit tested.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Qt Test (Qt6::Test) |
| Config file | `tests/CMakeLists.txt` |
| Quick run command | `cd build && ctest -R tst_ --output-on-failure` |
| Full suite command | `cd build && ctest --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DATA-06 | PurpleAir JSON parsed correctly from raw bytes | unit | `ctest -R tst_PurpleAirParser -x` | ❌ Wave 0 |
| DATA-07 | PM2.5 A+B averaged; AQI calculated from known concentration | unit | `ctest -R tst_PurpleAirParser -x` | ❌ Wave 0 |
| DATA-07 | AQI boundary values (0, 9.0, 9.1, 35.4, 35.5, 55.4, 500) | unit | `ctest -R tst_PurpleAirParser -x` | ❌ Wave 0 |
| GAUG-11 | AQI gauge renders in correct EPA color at boundary values | manual/visual | Visual inspection on running app | N/A |
| GAUG-12 | PM2.5 ring shows averaged value | manual/visual | Visual inspection on running app | N/A |
| TRND-01 | Sparklines appear after data accumulation | manual/visual | Visual inspection on running app | N/A |
| TRND-02 | Ring buffer wraps correctly at capacity; oldest samples evicted | unit | `ctest -R tst_WeatherDataModel -x` | ❌ Wave 0 new test cases |

### Wave 0 Gaps

- [ ] `tests/tst_PurpleAirParser.cpp` — covers DATA-06, DATA-07 (AQI calculation, JSON parsing, boundary values, missing fields)
- [ ] New test cases in `tests/tst_WeatherDataModel.cpp` — ring buffer wrap-around, staleness clear of aqi/pm25/pm10

---

## Sources

### Primary (HIGH confidence)
- Qt official docs (doc.qt.io/qt-6/qml-qtquick-canvas.html) — Canvas rendering strategy, requestPaint, performance notes
- Qt official docs (doc.qt.io/qt-6/qml-qtquick-shapes-shape.html) — Single Shape with multiple ShapePaths recommendation
- Qt official docs (doc.qt.io/qt-6/qml-qtquick-pathpolyline.html) — PathPolyline path property types (QPolygonF, QList<QPointF>)
- Qt official docs (doc.qt.io/qt-6/qml-qtquick-shapes-shapepath.html) — ShapePath properties
- Existing codebase (WeatherDataModel.cpp, wind rose ring buffer) — ring buffer pattern directly reused

### Secondary (MEDIUM confidence)
- IQAir Knowledge Base (iqair.com/us/support/knowledge-base/KA-05074-US) — 2024 EPA PM2.5 breakpoints (0-9.0 Good, 9.1-35.4 Moderate, 35.5-55.4 USG, 55.5-150.4 Unhealthy, 150.5-250.4 Very Unhealthy, 250.5+ Hazardous)
- EPA search results confirming May 6, 2024 effective date for PM2.5 NAAQS revision
- AirNow.gov (airnow.gov/aqi/aqi-basics/) — AQI 6-category table confirmed (Good/Moderate/USG/Unhealthy/Very Unhealthy/Hazardous)
- PurpleAir Community (community.purpleair.com/t/sensor-json-documentation/6917) — field names `pm2_5_atm`, `pm2_5_atm_b`, `pm10_0_atm`, `current_temp_f`, channel A/B naming conventions
- ICS blog (ics.com/blog/creating-qml-controls-scratch-linechart) — Canvas polyline rendering pattern (beginPath/lineTo/stroke)

### Tertiary (LOW confidence)
- simonw/til (til.simonwillison.net/purpleair/purple-air-aqi) — Pre-2024 AQI breakpoints shown; useful for formula structure but breakpoints now outdated
- PurpleAir Home Assistant community — pm2_5_atm and pm10_0_atm field names in configuration examples (not official schema)

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all technology choices are project-established; no new dependencies
- Architecture: HIGH — all patterns directly mirror Phase 1/2 implementations with verified Qt docs support
- PurpleAir field names: MEDIUM — community-confirmed (firmware 7.04) but no official machine-readable schema; validate against actual sensor before writing parser
- AQI breakpoints (2024): MEDIUM — IQAir source confirmed, EPA search results confirmed, but EPA TAD PDF could not be directly loaded; formula structure verified against pre-2024 sources
- Pitfalls: HIGH — staleness, z-ordering, and stroke math pitfalls are derived from direct code analysis of existing implementation

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (stable Qt APIs; AQI breakpoints stable post-2024 update)
