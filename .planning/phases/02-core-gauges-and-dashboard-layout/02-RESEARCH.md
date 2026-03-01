# Phase 2: Core Gauges and Dashboard Layout - Research

**Researched:** 2026-03-01
**Domain:** Qt6 QML/Quick — arc gauges, compass rose, responsive layout, kiosk windowing, C++ model binding
**Confidence:** HIGH (stack verified against Qt 6.10.2 docs and local system)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Gauge Visual Style**
- 80% arc (~290 degree sweep) with large numeric readout centered inside the arc
- Arc fill represents value position in range (min to max) — gives spatial "where am I in the range" reading
- Dim/muted track always visible behind filled portion showing full range
- Fill only, no needle — the leading edge of the fill is the indicator
- Fill animates smoothly between values (satisfies GAUG-14 animated transitions)

**Dashboard Layout**
- 3x4 equal grid (3 rows, 4 columns)
- Fixed layout:
  - Row 1: Temperature | Feels-like | Humidity | Dew Point
  - Row 2: Wind Speed | Compass Rose | Rain Rate | Pressure
  - Row 3: UV Index | Solar Rad | [Reserved: AQI Phase 3] | [Reserved: Future]
- 2 reserved cells: AQI gauge (Phase 3 fills with data + UI), one future placeholder
- Reserved cells should be empty styled slots (not invisible — maintain grid structure)
- KIOSK-03 (last-updated timestamp) dropped — user does not want it

**Compass Rose**
- Traditional compass rose with full N/S/E/W and intercardinal markings
- 16-point coarse cardinal direction text label displayed (N, NNE, NE, ENE, etc.) — no numeric degrees
- Smooth rotation animation when wind direction changes
- Must update within 2.5s of UDP packet (GAUG-05 requirement)

**Color Thresholds**
- Temperature: Blue <32F, Light blue 32-50F, Green 50-70F, Yellow 70-85F, Orange 85-100F, Red 100F+
- Humidity: Red <25%, Orange 25-30%, Green 30-60%, Yellow 60-70%, Red 70%+
- Wind speed: Green 0-5 mph, Yellow 5-15 mph, Orange 15-30 mph, Red 30+ mph
- UV Index: EPA 5-zone (Green 0-2, Yellow 3-5, Orange 6-7, Red 8-10, Violet 11+)
- All other gauges (pressure, solar rad, rain rate, dew point, feels-like): no threshold colors

**Color Theme**
- Dark background throughout the dashboard
- Dark yellow for default arc fill on non-threshold gauges
- Subdued yellow for all gauge value text — always yellow, even on threshold gauges
- Subdued yellow for gauge markings, tick marks, compass rose markings/arrow, cardinal labels
- Threshold gauge arcs use subdued version of threshold color (~70-80% saturation, pulled slightly darker than pure)
- "Subdued" means clearly recognizable as that color, not full saturation — toned down, not muted to gray

**Unit Display**
- Barometric pressure displayed in millibars (API returns inHg; convert via inHg x 33.8639 = mbar)
- All other values remain imperial (Fahrenheit, mph, inches)

**Kiosk & Window Modes**
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

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| GAUG-01 | Temperature gauge with color thresholds | Arc gauge pattern + threshold color binding in QML |
| GAUG-02 | Humidity gauge | Arc gauge pattern; same component, different thresholds |
| GAUG-03 | Barometric pressure gauge with trend arrow (rising/falling/steady) | Arc gauge + pressureTrend int (-1/0/1) drives arrow icon or text label |
| GAUG-04 | Wind speed gauge with gust readout | Arc gauge + secondary Text binding windGust property |
| GAUG-05 | Compass rose for wind direction | Canvas or Image Item with rotation + RotationAnimation.Shortest, driven by windDir UDP (2.5s SLA) |
| GAUG-06 | Rain rate gauge + daily accumulation display | Arc gauge for rainRate + secondary Text for rainfallDaily |
| GAUG-07 | UV Index gauge with EPA 5-zone color coding | Arc gauge with threshold color function (5 zones) |
| GAUG-08 | Solar radiation numeric display | Arc gauge (no thresholds) |
| GAUG-09 | Feels-like display (heat index >=80F+RH>=40%; wind chill <=50F+wind>=3mph) | QML property binding to heatIndex/windChill with formula selector logic |
| GAUG-10 | Dew point display | Arc gauge (no thresholds) |
| GAUG-14 | Animated gauge needle transitions (smooth movement between values) | SmoothedAnimation on ShapePath.sweepAngle — built into arc gauge component |
| KIOSK-01 | Full-screen frameless window | Window.FullScreen + Qt.FramelessWindowHint via --kiosk flag, QCommandLineParser |
| KIOSK-02 | Responsive layout targeting 720p, scales to larger displays | GridLayout with Layout.fillWidth/fillHeight + font.pixelSize bound to cell height |
| KIOSK-03 | Last-updated timestamp visible on dashboard | DROPPED by user — do not implement |
</phase_requirements>

---

## Summary

Phase 2 transforms the headless Qt6 data pipeline from Phase 1 into a visible dashboard. The work is purely in the presentation layer: switching `QCoreApplication` to `QGuiApplication`, adding a `QQmlApplicationEngine`, exposing the existing `WeatherDataModel` as a QML context property, then building QML components for the gauges and layout.

The confirmed Qt 6 approach for arc gauges is `QtQuick.Shapes` — a `Shape` containing two `ShapePath` elements with `PathAngleArc` (one static track, one animated fill). `SmoothedAnimation` on `sweepAngle` provides the smooth value transitions required by GAUG-14. This approach avoids software rasterization entirely (unlike `Canvas.Image` which uploads textures on every paint) and supports GPU acceleration on all targets including EGLFS.

The compass rose is a drawn `Canvas` item that rotates as a whole using `RotationAnimation` with `direction: RotationAnimation.Shortest`, which handles the 0/359 wraparound that trips up numeric animations. The dashboard grid is a `GridLayout` with `uniformCellWidths: true` and `uniformCellHeights: true` (Qt 6.6+, available on the system's Qt 6.10.2), anchored to fill the root `Window`.

**Primary recommendation:** Use `Shape` + `PathAngleArc` for all arc gauges with `SmoothedAnimation` on `sweepAngle`; use a rotating `Canvas` item for the compass rose; use `GridLayout` with uniform cells for the 3x4 layout; expose `WeatherDataModel` via `setContextProperty` (valid, not deprecated in Qt 6.10.2 official docs).

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::Quick | 6.10.2 (system) | QML engine, scene graph, all QML types | Official Qt rendering infrastructure; GPU-accelerated scene graph |
| Qt6::Qml | 6.10.2 (system) | QQmlApplicationEngine, context properties, type registration | Required for C++↔QML bridge |
| QtQuick.Shapes (QML import) | bundled with Qt6::Quick | Shape, ShapePath, PathAngleArc | GPU path rendering without texture upload overhead |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt Quick GridLayout | bundled | 3x4 responsive dashboard grid | Fixed-column grid that scales to fill any window size |
| RotationAnimation | bundled | Smooth compass rose rotation with wraparound | When animating a rotation property that crosses 0/360 boundary |
| SmoothedAnimation | bundled | Smooth arc fill transition on value change | Gauge sweepAngle changes — handles rapid updates by splicing curves |
| QCommandLineParser | Qt6::Core | `--kiosk` flag parsing | Parsing custom CLI flags after QGuiApplication construction |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Shape + PathAngleArc | Canvas (2D API) | Canvas.Image render target (the only fully supported Qt6 option) incurs texture upload on every repaint — bad for frequent updates. Shape avoids rasterization entirely. Canvas is only preferable for complex freehand drawing (compass rose interior detail). |
| SmoothedAnimation | NumberAnimation | SmoothedAnimation handles rapid value changes by splicing easing curves — maintains visual continuity if UDP updates arrive mid-animation. NumberAnimation would snap to new target abruptly. |
| setContextProperty | QML_SINGLETON + foreign wrapper | Singleton approach is idiomatic Qt 6.4+ and better for tooling. setContextProperty is simpler for a pre-existing QObject* that can't be modified. Both are valid; setContextProperty is undeprecated in Qt 6.10.2 docs. |
| GridLayout | Manual anchors / Repeater | GridLayout with uniformCellWidths/Heights gives equal-size cells that resize with the window automatically. Manual anchoring requires hardcoded math. |

**Installation:** Qt 6.10.2 already installed. No new packages needed.

CMakeLists changes required:

```cmake
# Top-level CMakeLists.txt — add Quick to find_package
find_package(Qt6 REQUIRED COMPONENTS Core Network Test Quick Qml)

# src/CMakeLists.txt — replace qt_add_executable + add qt_add_qml_module
qt_add_executable(wxdash main.cpp)

qt_add_qml_module(wxdash
    URI wxdash
    QML_FILES
        qml/Main.qml
        qml/ArcGauge.qml
        qml/CompassRose.qml
        qml/DashboardGrid.qml
)

target_link_libraries(wxdash PRIVATE wxdash_lib Qt6::Quick)
```

---

## Architecture Patterns

### Recommended Project Structure

```
src/
├── CMakeLists.txt            # Updated with Qt6::Quick + qt_add_qml_module
├── main.cpp                  # QGuiApplication + QQmlApplicationEngine, setContextProperty
├── models/
│   ├── WeatherReadings.h     # Unchanged (Phase 1)
│   └── WeatherDataModel.h    # Unchanged (Phase 1)
├── network/                  # Unchanged (Phase 1)
│   └── ...
└── qml/
    ├── Main.qml              # Root Window, kiosk flag handling, F11 handler
    ├── DashboardGrid.qml     # GridLayout 3x4, instantiates all gauge cells
    ├── ArcGauge.qml          # Reusable arc gauge component (Shape + PathAngleArc)
    ├── CompassRose.qml       # Canvas-drawn rose + RotationAnimation
    └── ReservedCell.qml      # Styled empty placeholder (AQI slot, future slot)
```

### Pattern 1: Arc Gauge Component (Shape + PathAngleArc)

**What:** A `Shape` containing two `ShapePath + PathAngleArc` — one dim track (full sweep), one animated fill (value-proportional sweep). Centered `Text` shows the numeric value.

**When to use:** All 9 arc gauges (temperature, feels-like, humidity, dew point, wind speed, rain rate, pressure, UV index, solar rad).

**Example:**
```qml
// Source: https://doc.qt.io/qt-6/qml-qtquick-pathanglearc.html
//         https://doc.qt.io/qt-6/qml-qtquick-shapes-shape.html

import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property real value: 0.0
    property real minValue: 0.0
    property real maxValue: 100.0
    property color arcColor: "#B8860B"   // default dark yellow

    // Gauge geometry: 80% arc = 288 degrees, starting from ~216 degrees (bottom-left)
    // PathAngleArc: 0=right(3 o'clock), clockwise positive
    // 80% arc: startAngle=135, sweepAngle=270 covers the ~288-degree arc nicely
    readonly property real startAngle: 135
    readonly property real fullSweep: 270   // degrees for full range (80% of circle)
    readonly property real valueSweep: fullSweep * Math.max(0, Math.min(1, (value - minValue) / (maxValue - minValue)))

    Shape {
        anchors.fill: parent

        // Track (dim background arc — full range)
        ShapePath {
            strokeColor: "transparent"
            fillColor: "#3A3A3A"          // muted track color
            strokeWidth: 0
            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: Math.min(root.width, root.height) * 0.38
                radiusY: Math.min(root.width, root.height) * 0.38
                startAngle: root.startAngle
                sweepAngle: root.fullSweep
            }
        }

        // Fill arc (animated value indicator)
        ShapePath {
            strokeColor: "transparent"
            fillColor: root.arcColor
            strokeWidth: 0

            property real animatedSweep: 0
            SmoothedAnimation on animatedSweep {
                to: root.valueSweep
                velocity: 180   // degrees per second
            }

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: Math.min(root.width, root.height) * 0.38
                radiusY: Math.min(root.width, root.height) * 0.38
                startAngle: root.startAngle
                sweepAngle: parent.animatedSweep
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: root.value.toFixed(1)
        color: "#C8A000"                  // subdued yellow
        font.pixelSize: Math.min(root.width, root.height) * 0.22
        font.bold: true
    }
}
```

**Key insight for arc fill shape:** `PathAngleArc` with `fillColor` fills the entire pie sector from center. To get a ring/donut look (filled arc strip, not pie), use two concentric arcs with opposite windings, or use `strokeColor` with a thick `strokeWidth` and `fillColor: "transparent"`. The stroke approach is simpler — set `strokeWidth` to the desired arc thickness and `fillColor: "transparent"`:

```qml
// Ring/strip arc using stroke (preferred for gauge track)
ShapePath {
    strokeColor: root.arcColor
    fillColor: "transparent"
    strokeWidth: Math.min(root.width, root.height) * 0.08   // arc thickness
    capStyle: ShapePath.RoundCap
    PathAngleArc {
        centerX: root.width / 2; centerY: root.height / 2
        radiusX: Math.min(root.width, root.height) * 0.38
        radiusY: Math.min(root.width, root.height) * 0.38
        startAngle: root.startAngle
        sweepAngle: root.fullSweep
    }
}
```

### Pattern 2: Compass Rose with Rotation Animation

**What:** A `Canvas` item draws the rose image once (or on size change). The entire `Canvas` item is rotated using a `Behavior` on `rotation` with `RotationAnimation { direction: RotationAnimation.Shortest }`. Driven by `model.windDir`.

**When to use:** The single compass rose cell (GAUG-05).

**Example:**
```qml
// Source: https://doc.qt.io/qt-6/qml-qtquick-rotationanimation.html
import QtQuick

Item {
    id: root
    property int windDir: 0    // bound to model.windDir

    Canvas {
        id: roseCanvas
        anchors.fill: parent
        rotation: root.windDir  // rotated by the model value

        Behavior on rotation {
            RotationAnimation {
                duration: 400
                direction: RotationAnimation.Shortest  // handles 359→1 wraparound
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            // Draw N/S/E/W arms, intercardinal tick marks, compass arrow
            // ... drawing code uses ctx.arc, ctx.lineTo, ctx.fillText, etc.
        }

        // Redraw only when size changes (not every rotation update)
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Text {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        text: directionLabel(root.windDir)
        color: "#C8A000"

        function directionLabel(deg) {
            var dirs = ["N","NNE","NE","ENE","E","ESE","SE","SSE",
                        "S","SSW","SW","WSW","W","WNW","NW","NNW"]
            return dirs[Math.round(deg / 22.5) % 16]
        }
    }
}
```

**Why Canvas for the rose drawing (not Shape):** The compass rose involves many freehand arcs, lines, and text labels that are drawn once and cached. Canvas.Image render target is acceptable here because the rose image is painted once on `onPaint` (or on resize), not on every UDP update — only the `rotation` property changes, which is handled by Qt's scene graph transform without any repainting.

### Pattern 3: Dashboard Grid Layout

**What:** A `GridLayout` with fixed `columns: 4` and `rows: 3`, `uniformCellWidths: true`, `uniformCellHeights: true`, anchored to fill the root `Window`.

**When to use:** The top-level dashboard.

**Example:**
```qml
// Source: https://doc.qt.io/qt-6/qml-qtquick-layouts-gridlayout.html
import QtQuick.Layouts

GridLayout {
    anchors.fill: parent
    anchors.margins: 4
    columns: 4
    rows: 3
    uniformCellWidths: true    // Qt 6.6+ — available in 6.10.2
    uniformCellHeights: true
    columnSpacing: 4
    rowSpacing: 4

    // Row 1
    ArcGauge { /* temperature */ Layout.fillWidth: true; Layout.fillHeight: true }
    ArcGauge { /* feels-like  */ Layout.fillWidth: true; Layout.fillHeight: true }
    ArcGauge { /* humidity    */ Layout.fillWidth: true; Layout.fillHeight: true }
    ArcGauge { /* dew point   */ Layout.fillWidth: true; Layout.fillHeight: true }
    // Row 2
    ArcGauge  { /* wind speed  */ ... }
    CompassRose { /* compass   */ ... }
    ArcGauge  { /* rain rate   */ ... }
    ArcGauge  { /* pressure    */ ... }
    // Row 3
    ArcGauge  { /* UV index    */ ... }
    ArcGauge  { /* solar rad   */ ... }
    ReservedCell { /* AQI slot */ ... }
    ReservedCell { /* future   */ ... }
}
```

### Pattern 4: C++ Model Binding — main.cpp Migration

**What:** Replace `QCoreApplication` with `QGuiApplication`, create `QQmlApplicationEngine`, expose `WeatherDataModel*` via `setContextProperty`, parse `--kiosk` flag with `QCommandLineParser`, conditionally set `Window.FullScreen` and `Qt.FramelessWindowHint`.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/qtqml-cppintegration-contextproperties.html
//         https://doc.qt.io/qt-6/qcommandlineparser.html

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Parse --kiosk before loading QML
    QCommandLineParser parser;
    QCommandLineOption kioskOption("kiosk", "Launch fullscreen frameless");
    parser.addOption(kioskOption);
    parser.process(app);
    bool kioskMode = parser.isSet(kioskOption);

    // Register cross-thread types (unchanged from Phase 1)
    qRegisterMetaType<IssReading>();
    // ... etc

    auto *model = new WeatherDataModel(&app);
    // ... set up network thread (unchanged) ...

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("weatherModel", model);
    engine.rootContext()->setContextProperty("kioskMode", kioskMode);

    engine.loadFromModule("wxdash", "Main");
    // OR: engine.load(QUrl(QStringLiteral("qrc:/qt/qml/wxdash/Main.qml")));

    return app.exec();
}
```

**QML Window using kioskMode:**
```qml
// Main.qml
import QtQuick

Window {
    id: root
    width: 1280
    height: 720
    visible: true
    title: "wxdash"

    visibility: kioskMode ? Window.FullScreen : Window.Windowed
    flags: kioskMode ? Qt.Window | Qt.FramelessWindowHint : Qt.Window

    // F11 toggles fullscreen at runtime
    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_F11) {
                if (root.visibility === Window.FullScreen)
                    root.visibility = Window.Windowed
                else
                    root.visibility = Window.FullScreen
            }
        }
    }

    DashboardGrid {
        anchors.fill: parent
    }
}
```

### Pattern 5: Threshold Color Logic

**What:** A QML JS function in each gauge that maps value to arc color. Called as a property binding — reactive to value changes.

**Example:**
```qml
// Inside ArcGauge or caller
property color arcColor: temperatureColor(value)

function temperatureColor(tempF) {
    if (tempF < 32)  return "#5B8DD9"   // subdued blue
    if (tempF < 50)  return "#7BB8E8"   // subdued light blue
    if (tempF < 70)  return "#5CA85C"   // subdued green
    if (tempF < 85)  return "#C8A000"   // subdued yellow (same as default)
    if (tempF < 100) return "#C87C2A"   // subdued orange
    return "#C84040"                    // subdued red
}
```

**UV Index (EPA 5-zone):**
```qml
function uvColor(uv) {
    if (uv <= 2)  return "#5CA85C"   // green
    if (uv <= 5)  return "#C8A000"   // yellow
    if (uv <= 7)  return "#C87C2A"   // orange
    if (uv <= 10) return "#C84040"   // red
    return "#8B5CA8"                 // violet
}
```

### Pattern 6: Pressure Trend Indicator

**What:** `pressureTrend` is an `int` (-1=falling, 0=steady, 1=rising). Use a `Text` item with a Unicode arrow or a conditional `Image`.

**Example:**
```qml
// pressureTrend: -1 / 0 / 1
Text {
    text: {
        if (weatherModel.pressureTrend === 1)  return "\u2191"  // ↑ rising
        if (weatherModel.pressureTrend === -1) return "\u2193"  // ↓ falling
        return "\u2192"                                          // → steady
    }
    color: "#C8A000"
    font.pixelSize: parent.height * 0.12
}
```

### Pattern 7: Feels-Like Display

**What:** Shows `heatIndex` when `temperature >= 80F && humidity >= 40%`, shows `windChill` when `temperature <= 50F && windSpeed >= 3mph`, otherwise shows raw temperature. All values already in `WeatherDataModel`.

**Example:**
```qml
// ArcGauge for feels-like
property real feelsLikeValue: {
    if (weatherModel.temperature >= 80 && weatherModel.humidity >= 40)
        return weatherModel.heatIndex
    if (weatherModel.temperature <= 50 && weatherModel.windSpeed >= 3)
        return weatherModel.windChill
    return weatherModel.temperature
}

// Optional: label showing active formula
property string feelsLikeLabel: {
    if (weatherModel.temperature >= 80 && weatherModel.humidity >= 40)
        return "Heat Index"
    if (weatherModel.temperature <= 50 && weatherModel.windSpeed >= 3)
        return "Wind Chill"
    return "Apparent"
}
```

### Pattern 8: Pressure Unit Conversion

**What:** `WeatherDataModel.pressure` is in inHg (from API). Display in millibars per user decision.

```qml
// Inline in QML — no C++ change needed
property real pressureMbar: weatherModel.pressure * 33.8639
```

### Anti-Patterns to Avoid

- **Canvas.Image for animated gauges:** Canvas's only supported render target in Qt 6 is `Canvas.Image`, which uploads a new texture on every `requestPaint()`. Calling `requestPaint()` on every UDP update at 2.5s cadence creates constant texture uploads. Use `Shape` + `PathAngleArc` instead — only the `sweepAngle` property changes, handled as a scene graph transform with no rasterization.
- **`FramebufferObject` render target:** Ignored/removed in Qt 6.0. Do not specify it.
- **Hardcoded pixel sizes for fonts and margins:** Everything must be relative to cell height/width so the layout scales from 720p to larger displays. Use `font.pixelSize: parent.height * 0.22`.
- **`setInterval`/`setTimeout` in QML Canvas:** Replace with `requestAnimationFrame()` or `Timer` item.
- **QML type `rotation` with `NumberAnimation`:** Numerical interpolation on rotation wraps through 359 degrees (e.g., 350→10 rotates 340 degrees the wrong way). Always use `RotationAnimation { direction: RotationAnimation.Shortest }` for compass/rotation properties.
- **Loading QML before `setContextProperty`:** Context properties must be set before calling `engine.load()` / `engine.loadFromModule()`. Setting them afterward is an expensive operation per Qt docs.
- **`QCoreApplication` with QML:** Qt Quick requires `QGuiApplication` (or `QApplication` for widget mixing). Keeping `QCoreApplication` will crash at runtime when the QML scene graph tries to initialize.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Arc fill animation | Custom timer + manual sweep update | SmoothedAnimation on sweepAngle | Handles rapid value changes, curve splicing, no timer management |
| Compass wraparound | Manual mod-360 + shortest-path math | RotationAnimation.Shortest | Qt calculates minimal rotation including 359→1 path automatically |
| CLI argument parsing | Manual `strcmp(argv[i], "--kiosk")` | QCommandLineParser | Handles `-h`/`--help`, long/short options, multiple names, error messages |
| Responsive font sizes | Separate QML files per resolution | `font.pixelSize: parent.height * N` | Single binding scales across all resolutions |
| Equal-size grid cells | Manual `width: parent.width / 4` | GridLayout uniformCellWidths/Heights | Handles margins, spacing, rounding automatically |
| Pressure mbar conversion | C++ property in WeatherDataModel | QML inline multiply | One-liner in QML, no C++ churn, no extra Q_PROPERTY |

**Key insight:** SmoothedAnimation is specifically designed for value tracking where the target changes frequently (e.g., every 2.5s from UDP). It maintains velocity continuity when the target changes mid-animation, preventing the jarring resets that NumberAnimation produces.

---

## Common Pitfalls

### Pitfall 1: Canvas Repaint on Every Data Update

**What goes wrong:** Developer calls `requestPaint()` in a `Connections` handler for every model property change. On a 2.5s update cadence across 10 gauges, this creates 4 texture uploads per second, accumulating GPU memory pressure and causing visible frame drops.

**Why it happens:** Canvas is the familiar "drawing API" that looks easy. The texture upload cost only becomes visible under realistic update rates.

**How to avoid:** Use `Shape` + `PathAngleArc` for all arc gauges. Only use `Canvas` for the compass rose where the image content is redrawn on size change only — rotation is handled by the scene graph `rotation` property, not by repainting.

**Warning signs:** `onPaint:` handler that reads model values directly; `requestPaint()` called from a `Connections` block.

---

### Pitfall 2: Rotation Animation Wraps Wrong Direction

**What goes wrong:** Wind direction changes from 350° to 10°. The compass rose spins 340 degrees clockwise (the long way around) instead of 20 degrees counterclockwise.

**Why it happens:** `NumberAnimation` interpolates numerically: 350→10 = -340 change, which Qt renders as 340-degree counterclockwise spin. The "short way" is 20 degrees.

**How to avoid:** Always use `RotationAnimation { direction: RotationAnimation.Shortest }` inside a `Behavior on rotation` block.

**Warning signs:** Compass spin duration seems very long on direction changes near North.

---

### Pitfall 3: QML Property Binding Breaks on JS Statements

**What goes wrong:** Developer writes `property color arcColor: { if (v < 32) return "blue"; return "green" }` — this silently breaks the binding and `arcColor` gets `undefined`.

**Why it happens:** In QML, `property x: { ... }` with curly braces is a property object definition, not a function body. The colon-assignment syntax only accepts expressions, not imperative JS statements.

**How to avoid:** Wrap conditional logic in a function and call it: `property color arcColor: tempColor(value)` where `tempColor` is a `function tempColor(v) { if (v < 32) return "#5B8DD9"; ... }` declared elsewhere in the component. Alternatively use the ternary expression form for simple two-value cases.

**Warning signs:** `arcColor` always shows the default/last-assigned color regardless of value changes; `qmlWarning` about undefined binding value.

---

### Pitfall 4: SmoothedAnimation Target Property Mismatch

**What goes wrong:** `SmoothedAnimation on sweepAngle` is placed on the `PathAngleArc` element but `sweepAngle` is a property of `PathAngleArc`, not `ShapePath`. Animation doesn't animate anything visible.

**Why it happens:** The arc geometry is on `PathAngleArc` but `SmoothedAnimation` needs to be a child of the object whose property it animates.

**How to avoid:** Declare an intermediate `property real animatedSweep` on the `ShapePath`, animate it, and bind `PathAngleArc.sweepAngle: parent.animatedSweep`. See Pattern 1 example above.

**Warning signs:** Gauge fill changes instantly instead of animating.

---

### Pitfall 5: Forgetting QGuiApplication Migration

**What goes wrong:** `QCoreApplication` remains in `main.cpp`. The `QQmlApplicationEngine` attempts to load a QML file that uses Qt Quick types, which require the Qt Quick scene graph, which requires a GUI application. Crash at runtime: `QQuickView requires a QGuiApplication`.

**How to avoid:** Replace `#include <QCoreApplication>` and `QCoreApplication app` with `#include <QGuiApplication>` and `QGuiApplication app` as the first change in main.cpp.

**Warning signs:** Immediate crash on launch with `QQuickView requires a QGuiApplication` or segfault in scene graph initialization.

---

### Pitfall 6: qt_add_qml_module URI and Load Path Mismatch

**What goes wrong:** CMake's `qt_add_qml_module(wxdash URI wxdash ...)` embeds QML at `qrc:/qt/qml/wxdash/`. Calling `engine.load(QUrl("qrc:/Main.qml"))` fails because the file is at the `qt/qml/wxdash` subpath.

**Why it happens:** Qt 6's `qt_standard_project_setup` enables QTP0001 which changes the QML resource path from the old `qrc:/` root to `qrc:/qt/qml/<URI>/`.

**How to avoid:** Use `engine.loadFromModule("wxdash", "Main")` which resolves the URI automatically — no hardcoded path needed. This is the modern Qt 6.4+ approach.

**Warning signs:** `QML module not found` or `file not found` error when loading QML.

---

### Pitfall 7: uniformCellWidths/Heights Qt Version Requirement

**What goes wrong:** Developer targets Qt 6.2 or 6.5 LTS for deployment and uses `uniformCellWidths: true`. The property was added in Qt Quick Layouts 6.6 — it silently falls back to non-uniform sizing on older Qt.

**Why it happens:** API was added in 6.6, not available in LTS releases before that.

**How to avoid:** Not a concern for this project — system has Qt 6.10.2 (confirmed), which includes uniformCellWidths. For Pi deployment: verify Pi OS Qt version or install Qt 6.6+ manually. If targeting older Qt, fall back to `Layout.preferredWidth: (parent.width - spacing * 3) / 4` calculations.

**Warning signs:** Grid cells not equal size; no warning from Qt about the property.

---

## Code Examples

Verified patterns from Qt 6.10.2 official documentation:

### PathAngleArc Properties (source: https://doc.qt.io/qt-6/qml-qtquick-pathanglearc.html)

```
centerX, centerY   — center of the arc
radiusX, radiusY   — ellipse radii (equal for circular arc)
startAngle         — start in degrees, clockwise from 3 o'clock (0 = right)
sweepAngle         — arc length in degrees, positive=clockwise, negative=counterclockwise
moveToStart        — false = connects to previous path element with a line
```

For 80% arc (~290°): `startAngle: 135`, `sweepAngle: 270` starts bottom-left and sweeps to bottom-right.

### SmoothedAnimation (source: https://doc.qt.io/qt-6/qml-qtquick-smoothedanimation.html)

```qml
SmoothedAnimation on targetProperty {
    to: someValue
    velocity: 200   // units/sec (default); or set duration: 300
}
```
When `to` changes while animating, the easing curves are spliced — no jump, continuous motion.

### RotationAnimation Shortest (source: https://doc.qt.io/qt-6/qml-qtquick-rotationanimation.html)

```qml
Behavior on rotation {
    RotationAnimation {
        duration: 400
        direction: RotationAnimation.Shortest
    }
}
```
`Shortest` automatically picks clockwise or counterclockwise to minimize the rotation angle.

### QCommandLineParser (source: https://doc.qt.io/qt-6/qcommandlineparser.html)

```cpp
QGuiApplication app(argc, argv);
QCommandLineParser parser;
QCommandLineOption kioskOption("kiosk", "Launch fullscreen frameless");
parser.addOption(kioskOption);
parser.process(app);
bool kiosk = parser.isSet(kioskOption);
```

### Window Fullscreen + Frameless (source: https://doc.qt.io/qt-6/qml-qtquick-window.html)

```qml
Window {
    visibility: kioskMode ? Window.FullScreen : Window.Windowed
    flags: kioskMode ? Qt.Window | Qt.FramelessWindowHint : Qt.Window
}
```
Use both `visibility` and `flags` together for fully frameless fullscreen.

### Font Scaling (source: https://doc.qt.io/qt-6/scalability.html, Qt forum patterns)

```qml
Text {
    font.pixelSize: parent.height * 0.22   // scales with cell height
    font.bold: true
}
```

### GridLayout Equal Cells (source: https://doc.qt.io/qt-6/qml-qtquick-layouts-gridlayout.html)

```qml
GridLayout {
    anchors.fill: parent
    columns: 4
    rows: 3
    uniformCellWidths: true    // Qt 6.6+
    uniformCellHeights: true
}
```

### QML Context Property (source: https://doc.qt.io/qt-6/qtqml-cppintegration-contextproperties.html)

```cpp
QQmlApplicationEngine engine;
engine.rootContext()->setContextProperty("weatherModel", model);
engine.rootContext()->setContextProperty("kioskMode", kioskMode);
// MUST be set before engine.loadFromModule(...)
engine.loadFromModule("wxdash", "Main");
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `Canvas.FramebufferObject` render target | `Canvas.Image` is the only supported option | Qt 6.0 | Do not specify renderTarget — default Image is correct |
| `CircularGauge` from Qt Quick Extras | Hand-built `Shape` + `PathAngleArc` | Qt 6.0 (Extras not ported) | No pre-built gauge widget; build from Shape primitives |
| `qRegisterMetaType<T>()` for cross-thread | Still required for custom types in queued connections | Qt 6 | No change — Phase 1 already does this correctly |
| `setContextProperty` (Qt 5 idiom) | Still valid in Qt 6; `QML_SINGLETON` + foreign wrapper is the "modern" approach | Qt 6.4+ | `setContextProperty` is simpler for pre-existing instances — use it |
| `NumberAnimation` on rotation | `RotationAnimation { direction: Shortest }` | Qt 5+ | Important for compass wraparound correctness |
| `GeometryRenderer` (default) | `CurveRenderer` (opt-in, tech preview Qt 6.6, stable in Qt 6.7+) | Qt 6.7 | CurveRenderer gives anti-aliased arcs without MSAA; acceptable to use default GeometryRenderer for gauges at their display size |

**Deprecated/outdated:**
- `qt_quick_extras_circulargauge`: Removed in Qt 6 — not available, do not reference
- `Canvas.FramebufferObject`: Ignored since Qt 6.0 — do not specify renderTarget at all
- `qmlRegisterType` imperative calls: Replaced by `QML_ELEMENT` macro approach — but for this project `setContextProperty` avoids needing either

---

## Open Questions

1. **Canvas drawing detail for compass rose**
   - What we know: Canvas 2D API works well for static drawing; the rose is drawn once on size change and rotated via scene graph.
   - What's unclear: Exact line weights, tick mark sizes, and compass arrow shape — these are under "Claude's Discretion" but need concrete values before implementation.
   - Recommendation: Decide compass rose drawing constants during the planning/implementation of the CompassRose component task. A simple 16-arm star with N/S/E/W arms longer, intercardinals shorter, and a triangular north pointer is sufficient.

2. **Arc gauge stroke width vs fill rendering**
   - What we know: `ShapePath` supports both `strokeColor` (outline) and `fillColor` (filled area). For a ring/strip gauge, stroke approach is simpler (one arc per track/fill layer). For a solid pie-wedge, fill approach is used.
   - What's unclear: Which rendering approach is more visually faithful to the "fitness tracker ring" inspiration — stroke creates a ring, fill creates a pie wedge. The user specified "fill only, no needle", which implies a ring/strip arc style (stroke approach).
   - Recommendation: Use the stroke approach (`strokeColor` with thick `strokeWidth`, `fillColor: "transparent"`) to achieve a ring/strip that matches the fitness tracker ring aesthetic. Width approximately 8-10% of the smaller dimension.

3. **`uniformCellWidths` on Pi deployment**
   - What we know: Available in Qt 6.6+; system has 6.10.2.
   - What's unclear: Pi OS Bookworm ships Qt 6.4 in apt. If deploying via system Qt on Pi, this property won't exist.
   - Recommendation: For Phase 2 development on Linux desktop (confirmed approach), use `uniformCellWidths`. Add a README note for Pi: install Qt 6.6+ via Qt's official installer, not `apt`.

4. **`engine.loadFromModule` availability**
   - What we know: `loadFromModule(URI, type)` was added in Qt 6.4.
   - What's unclear: Whether the system's Qt 6.10.2 build includes it (should, as 6.10.2 > 6.4).
   - Recommendation: Use `loadFromModule` — confirmed available on 6.10.2. Fallback `engine.load(QUrl("qrc:/qt/qml/wxdash/Main.qml"))` if it doesn't link.

---

## Sources

### Primary (HIGH confidence)

- https://doc.qt.io/qt-6/qml-qtquick-pathanglearc.html — PathAngleArc properties: centerX, centerY, radiusX, radiusY, startAngle, sweepAngle
- https://doc.qt.io/qt-6/qml-qtquick-shapes-shape.html — Shape renderers, preferredRendererType, performance characteristics
- https://doc.qt.io/qt-6/qml-qtquick-shapes-shapepath.html — ShapePath stroke/fill properties
- https://doc.qt.io/qt-6/qml-qtquick-smoothedanimation.html — velocity, duration, curve splicing behavior
- https://doc.qt.io/qt-6/qml-qtquick-rotationanimation.html — direction: Shortest for compass wraparound
- https://doc.qt.io/qt-6/qml-qtquick-layouts-gridlayout.html — uniformCellWidths, uniformCellHeights, Layout.fillWidth/fillHeight
- https://doc.qt.io/qt-6/qml-qtquick-window.html — visibility (FullScreen/Windowed), flags (FramelessWindowHint)
- https://doc.qt.io/qt-6/qcommandlineparser.html — --kiosk flag parsing, isSet()
- https://doc.qt.io/qt-6/cmake-build-qml-application.html — qt_add_qml_module CMake pattern
- https://doc.qt.io/qt-6/qtqml-cppintegration-contextproperties.html — setContextProperty for QObject* model
- https://doc.qt.io/qt-6/scalability.html — font.pixelSize binding to parent dimensions
- `pkg-config --modversion Qt6Core Qt6Quick Qt6Qml` — confirms Qt 6.10.2 on system with Quick, Qml, Shapes all available

### Secondary (MEDIUM confidence)

- https://www.qt.io/blog/smooth-vector-graphics-in-qt-quick — CurveRenderer vs GeometryRenderer quality/performance comparison (blog post, official Qt source)
- https://www.qt.io/product/qt6/qml-book/ch09-shapes-animations — ShapePath animation patterns (The Qt 6 Book, official Qt resource)

### Tertiary (LOW confidence)

- Qt forum discussions on gauge implementations — multiple sources confirm PathAngleArc + Shape approach; specific examples not from official docs

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — verified against Qt 6.10.2 installed on system (pkg-config confirmed) and official docs
- Architecture: HIGH — all patterns cite official Qt 6 documentation
- Pitfalls: HIGH — Canvas.Image texture cost confirmed by official Canvas docs; rotation wraparound confirmed by RotationAnimation docs; QGuiApplication requirement is a hard Qt constraint
- Color values: MEDIUM — hex values for "subdued" colors are Claude's discretion; no objective spec. Chosen values are reasonable starting points.

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (Qt 6.10.x stable series — 30 days)
