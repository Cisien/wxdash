# Architecture Research

**Domain:** Qt 6 C++ real-time weather dashboard (kiosk display)
**Researched:** 2026-03-01
**Confidence:** MEDIUM-HIGH (Qt core patterns HIGH from official docs; Pi-specific rendering MEDIUM from community sources; dual-cadence aggregation LOW — no direct examples found)

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         PRESENTATION LAYER                           │
│  ┌──────────────┐ ┌──────────────┐ ┌───────────────┐ ┌───────────┐ │
│  │ GaugeWidget  │ │CompassWidget │ │SparklineWidget│ │LayoutMgr  │ │
│  │(temp/hum/etc)│ │(wind dir)    │ │(trend graphs) │ │(QWidget)  │ │
│  └──────┬───────┘ └──────┬───────┘ └───────┬───────┘ └─────┬─────┘ │
│         │                │                 │               │        │
├─────────┴────────────────┴─────────────────┴───────────────┴────────┤
│                           MODEL LAYER                                │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                      WeatherDataModel                          │  │
│  │  (QObject subclass — owns all current readings + ring buffers) │  │
│  └────────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                         DATA SOURCE LAYER                            │
│  ┌──────────────────────────┐   ┌────────────────────────────────┐  │
│  │   HttpPoller             │   │   UdpReceiver                  │  │
│  │   (QNetworkAccessManager │   │   (QUdpSocket, async           │  │
│  │    + QTimer 10s)         │   │    readyRead signal, 2.5s)     │  │
│  └──────────────────────────┘   └────────────────────────────────┘  │
│                  │                              │                    │
│  ┌───────────────┴──────────────────────────────┴─────────────────┐  │
│  │              JsonParser / DataMapper                           │  │
│  │   (QJsonDocument parsing → typed structs)                      │  │
│  └────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Communicates With |
|-----------|----------------|-------------------|
| `HttpPoller` | Fires GET /v1/current_conditions every 10s via QTimer; manages QNetworkAccessManager; calls deleteLater() on replies | JsonParser (raw JSON bytes out), WeatherDataModel (via signal) |
| `UdpReceiver` | Binds QUdpSocket to port 22222; emits readyRead every ~2.5s; manages keep-alive by re-POSTing /v1/real_time?duration=N before expiry | JsonParser (raw JSON bytes out), WeatherDataModel (via signal) |
| `JsonParser` | Parses QJsonDocument; maps data_structure_type 1/3/4 to typed C++ structs; translates rain_size counts to inches | HttpPoller + UdpReceiver (caller), WeatherDataModel (output) |
| `WeatherDataModel` | Central QObject holding all current readings; maintains ring buffers (e.g., QVector of last 3 hours at 10s cadence) for sparklines; emits fine-grained signals per field changed | All widgets (signals consumed), DataSources (slots receive updates) |
| `GaugeWidget` | Custom QWidget subclass; overrides paintEvent(); draws arc gauge with QPainter; holds single scalar value + threshold config; calls update() when value changes | WeatherDataModel (connects to per-field signals) |
| `CompassWidget` | Custom QWidget subclass; draws compass rose + wind needle via QPainter; updates at UDP cadence (2.5s) | WeatherDataModel (wind direction signal) |
| `SparklineWidget` | Custom QWidget subclass; draws polyline from ring buffer snapshot; repaints on HTTP cadence (10s); owns no data — reads from model | WeatherDataModel (ring buffer accessor) |
| `DashboardWindow` | QMainWindow/QWidget root; sets up full-screen, owns all child widgets; handles QResizeEvent for responsive scaling; applies Qt::WA_OpaquePaintEvent to reduce overhead | Platform (eglfs), all child widgets |
| `ThresholdConfig` | Plain C++ struct / config singleton; maps field name → (warn level, critical level, color); loaded at startup | GaugeWidget (reads thresholds), DashboardWindow (owns config) |

---

## Recommended Project Structure

```
wxdash/
├── CMakeLists.txt                  # Qt6 find_package, target_link_libraries
├── src/
│   ├── main.cpp                    # QApplication, DashboardWindow, fullscreen
│   ├── datasource/
│   │   ├── HttpPoller.h/.cpp       # QNetworkAccessManager + QTimer
│   │   ├── UdpReceiver.h/.cpp      # QUdpSocket + keep-alive timer
│   │   └── JsonParser.h/.cpp       # QJsonDocument → typed structs
│   ├── model/
│   │   ├── WeatherDataModel.h/.cpp # QObject, signals, ring buffers
│   │   └── WeatherReadings.h       # Plain C++ structs (no Qt deps)
│   ├── widgets/
│   │   ├── DashboardWindow.h/.cpp  # Root window, layout
│   │   ├── GaugeWidget.h/.cpp      # Analog arc gauge
│   │   ├── CompassWidget.h/.cpp    # Wind direction compass rose
│   │   └── SparklineWidget.h/.cpp  # Trend mini-chart
│   └── config/
│       └── ThresholdConfig.h/.cpp  # Warn/critical levels, colors
├── resources/
│   └── (fonts, if needed)
└── tests/
    ├── test_JsonParser.cpp          # Unit test parsing corner cases
    └── test_WeatherDataModel.cpp    # Ring buffer correctness
```

### Structure Rationale

- **datasource/:** Network I/O isolated from everything else; testable by injecting raw JSON bytes
- **model/:** The single source of truth; no Qt widget headers required — testable without display
- **widgets/:** Pure presentation; receive values via signals, know nothing about network
- **WeatherReadings.h:** Plain C++ structs (no QObject) so they can cross thread boundaries safely via Qt::QueuedConnection
- **tests/:** Parser and model are the most logic-dense components; widget tests are lower priority for a kiosk

---

## Architectural Patterns

### Pattern 1: Signal-Per-Field on WeatherDataModel

**What:** WeatherDataModel emits separate typed signals for each observable field rather than a single generic "data changed" signal.

**When to use:** When different widgets update at different cadences (compass at 2.5s, temperature at 10s) — each widget connects only to the signal it needs.

**Trade-offs:** More signals to declare; avoids unnecessary widget repaints for unrelated data changes.

**Example:**
```cpp
class WeatherDataModel : public QObject {
    Q_OBJECT
public:
    // Wind/rain fields — emitted from UDP path (~2.5s)
    Q_SIGNAL void windSpeedChanged(double mph);
    Q_SIGNAL void windDirectionChanged(int degrees);
    Q_SIGNAL void rainRateChanged(double inchesPerHour);

    // Full-conditions fields — emitted from HTTP path (~10s)
    Q_SIGNAL void temperatureChanged(double fahrenheit);
    Q_SIGNAL void humidityChanged(double percent);
    Q_SIGNAL void pressureChanged(double inHg);
    Q_SIGNAL void solarRadiationChanged(double wm2);
    Q_SIGNAL void uvIndexChanged(double index);

    // Ring buffer accessor for sparklines
    QVector<double> temperatureHistory() const;
};
```

### Pattern 2: Worker-Object-on-Thread for Network I/O

**What:** HttpPoller and UdpReceiver are QObjects moved to a dedicated QThread using `moveToThread()`. All network activity happens off the GUI thread. Results cross the thread boundary via Qt::QueuedConnection (automatic for cross-thread signal/slot).

**When to use:** Any I/O that could block or introduce latency. On a Pi with limited CPU, preventing GUI jank from network stalls is essential.

**Trade-offs:** Adds thread management complexity; worth it because network operations are the most likely source of GUI freeze.

**Example:**
```cpp
// In main.cpp or DashboardWindow::init()
auto *networkThread = new QThread(this);
auto *httpPoller = new HttpPoller(); // no parent — will be moved
auto *udpReceiver = new UdpReceiver();

httpPoller->moveToThread(networkThread);
udpReceiver->moveToThread(networkThread);

// Cross-thread: Qt automatically uses QueuedConnection
connect(httpPoller, &HttpPoller::conditionsReceived,
        weatherModel, &WeatherDataModel::applyHttpUpdate);
connect(udpReceiver, &UdpReceiver::realtimeReceived,
        weatherModel, &WeatherDataModel::applyUdpUpdate);

connect(networkThread, &QThread::started, httpPoller, &HttpPoller::start);
connect(networkThread, &QThread::started, udpReceiver, &UdpReceiver::start);
networkThread->start();
```

### Pattern 3: Dual-Cadence Update Paths in the Model

**What:** WeatherDataModel has two distinct update slots: `applyUdpUpdate()` for fast wind/rain data (2.5s) and `applyHttpUpdate()` for full sensor snapshots (10s). Each slot emits only the signals relevant to the data it received.

**When to use:** When two data sources overlap in fields but run at different rates. The HTTP response also contains wind/rain — use it to reconcile/reset baseline, but let UDP dominate for live wind display.

**Trade-offs:** Requires explicitly deciding which source "wins" per field. Decision: UDP wins for wind/rain (fresher); HTTP wins for everything else. HTTP also used to confirm UDP baseline on startup.

**Example:**
```cpp
void WeatherDataModel::applyUdpUpdate(const UdpReading &r) {
    m_windSpeed = r.windSpeed;
    m_windDirection = r.windDirection;
    m_rainRate = r.rainRate;
    emit windSpeedChanged(m_windSpeed);
    emit windDirectionChanged(m_windDirection);
    emit rainRateChanged(m_rainRate);
    // Do NOT append to history buffer here — only HTTP does that
}

void WeatherDataModel::applyHttpUpdate(const HttpReading &r) {
    m_temperature = r.temperature;
    // ... update all fields
    // Also update wind/rain baseline from HTTP (lower priority)
    appendToHistory(m_temperature, m_tempHistory);
    emit temperatureChanged(m_temperature);
    // ... emit all other signals
}
```

### Pattern 4: QPainter-Based Custom Widgets with OpaquePaintEvent

**What:** All visual components subclass QWidget and override `paintEvent()`. Declare `Qt::WA_OpaquePaintEvent` to skip the background erase pass. Use `update()` (not `repaint()`) to queue paints via the event loop.

**When to use:** Always for custom gauge rendering on Pi. QPainter with anti-aliasing produces good quality gauges without requiring OpenGL. QWidget double-buffers automatically — no manual double-buffer needed.

**Trade-offs:** Pure software rendering; adequate for 720p dashboard with ~8 gauge-type widgets updating at 2.5–10s. Would need QOpenGLWidget if targeting 60fps animations or many widgets.

**Example:**
```cpp
GaugeWidget::GaugeWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent); // skip background erase
    setMinimumSize(150, 150);
}

void GaugeWidget::setValue(double value) {
    m_value = value;
    update(); // schedules paintEvent via event loop — no direct call
}

void GaugeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawArcBackground(p);
    drawArcValue(p, m_value);
    drawNeedle(p, m_value);
    drawLabel(p);
}
```

### Pattern 5: Ring Buffer in Model for Sparkline History

**What:** WeatherDataModel owns `QVector<double>` ring buffers (one per sparkline-able field), sized for the history window (e.g., 1080 samples = 3 hours at 10s intervals). SparklineWidget reads a snapshot of the buffer at paint time.

**When to use:** Sparklines need historical data but the widget shouldn't own or query historical storage — keeping it in the model maintains single-source-of-truth.

**Trade-offs:** Model holds all history in memory. At 10s intervals, 3h = 1080 doubles per field = ~8.6 KB per field (negligible on Pi). Cap buffer size at construction.

**Example:**
```cpp
// In WeatherDataModel
void WeatherDataModel::appendToHistory(double value, QVector<double> &buf) {
    if (buf.size() >= HISTORY_CAPACITY) {
        buf.removeFirst(); // O(n) but buffer is small; use QDeque if profiling shows cost
    }
    buf.append(value);
}

// SparklineWidget reads via const accessor
void SparklineWidget::paintEvent(QPaintEvent *) {
    QVector<double> snap = m_model->temperatureHistory(); // snapshot
    // ... draw polyline from snap
}
```

---

## Data Flow

### HTTP Polling Flow (10-second cadence)

```
QTimer::timeout() (10s)
    |
    v
HttpPoller::fetchCurrentConditions()
    |  (QNetworkAccessManager::get)
    |  [network thread]
    v
QNetworkReply::finished()
    |
    v
JsonParser::parseHttpResponse(QByteArray)
    |  → data_structure_type 1: ISS (temp, hum, wind baseline, UV, solar, rain baseline)
    |  → data_structure_type 3: LSS BAR (pressure)
    |  → data_structure_type 4: LSS Temp/Hum (indoor)
    v
HttpPoller emits: conditionsReceived(HttpReading)
    |  [Qt::QueuedConnection crosses to main thread]
    v
WeatherDataModel::applyHttpUpdate(HttpReading)
    |  → updates all fields
    |  → appends to history ring buffers
    v
Emits: temperatureChanged, humidityChanged, pressureChanged,
       solarRadiationChanged, uvIndexChanged, ...
    |
    v
Each widget's slot → widget.update() → paintEvent() redraws
```

### UDP Broadcast Flow (2.5-second cadence)

```
UdpReceiver starts keep-alive: POST /v1/real_time?duration=N
QUdpSocket::readyRead() fires every ~2.5s
    |
    v
UdpReceiver::onReadyRead()
    |  → readDatagram() loop while hasPendingDatagrams()
    v
JsonParser::parseUdpDatagram(QByteArray)
    |  → extracts wind_speed_last, wind_dir_last, rain_rate_last
    |  → applies rain_size conversion (counts → inches)
    v
UdpReceiver emits: realtimeReceived(UdpReading)
    |  [Qt::QueuedConnection crosses to main thread]
    v
WeatherDataModel::applyUdpUpdate(UdpReading)
    v
Emits: windSpeedChanged, windDirectionChanged, rainRateChanged
    |
    v
CompassWidget, WindGaugeWidget, RainGaugeWidget → update() → paintEvent()
```

### State Management

```
WeatherDataModel (single source of truth)
    |
    | signals (per-field, typed)
    |
    +-- GaugeWidget (temperature)    [slot: setValue, calls update()]
    +-- GaugeWidget (humidity)       [slot: setValue, calls update()]
    +-- GaugeWidget (pressure)       [slot: setValue, calls update()]
    +-- GaugeWidget (UV)             [slot: setValue, calls update()]
    +-- GaugeWidget (solar)          [slot: setValue, calls update()]
    +-- GaugeWidget (wind speed)     [slot: setValue, calls update()]
    +-- GaugeWidget (rain rate)      [slot: setValue, calls update()]
    +-- CompassWidget                [slot: setDirection, calls update()]
    +-- SparklineWidget (temp)       [slot: historyUpdated, calls update()]
    +-- SparklineWidget (pressure)   [slot: historyUpdated, calls update()]
    +-- (other sparklines)
```

---

## Scaling Considerations

This is a single-display kiosk with a fixed data source — scaling in the user/multi-tenant sense does not apply. The relevant "scaling" is performance under load on constrained hardware.

| Concern | Pi Zero 2W (worst case) | Pi 4 (target-ish) | Pi 5 (headroom) |
|---------|-------------------------|-------------------|-----------------|
| Widget repaint at 2.5s | Software QPainter OK for ~8 widgets at 720p | No issue | No issue |
| JSON parsing overhead | Negligible (small payloads, infrequent) | No issue | No issue |
| History ring buffer memory | ~1 MB for 3h of all fields | No issue | No issue |
| eglfs vs X11 overhead | Use eglfs-gbm, avoid X11 entirely | eglfs-gbm recommended | Either fine |
| UDP keep-alive reliability | Re-POST /v1/real_time before duration expires | Same | Same |

### Performance Priorities

1. **First bottleneck:** Repaint frequency. Default: widgets repaint only when their model signal fires. Never start a QTimer in a widget to poll for value changes — always use model signals.
2. **Second bottleneck:** Paint complexity. Keep paintEvent() fast by pre-computing static geometry (arc paths, scale markers) in resizeEvent() and caching in member variables. Only recompute value-dependent drawing in paintEvent().

---

## Anti-Patterns

### Anti-Pattern 1: Polling the Model from Widget Timers

**What people do:** Give each widget its own QTimer that calls model->getValue() on an interval and compares to last known value.

**Why it's wrong:** Decouples repaint from actual data arrival; wastes CPU; creates N timers where signal/slot connections are free. On a Pi this matters.

**Do this instead:** Widget connects to WeatherDataModel's signal for its specific field. Widget only repaints when its data actually changes.

### Anti-Pattern 2: Doing Network I/O on the GUI Thread

**What people do:** Call QNetworkAccessManager::get() in a slot on the main thread and block with QEventLoop::exec() or use a synchronous reply approach.

**Why it's wrong:** Any network hiccup (DNS timeout, TCP retry, API slowness) stalls the entire GUI. On a Pi with a potentially flaky mDNS hostname, this is a real risk.

**Do this instead:** Move HttpPoller and UdpReceiver to a QThread via moveToThread(). Communicate results back with signals (automatically queued cross-thread).

### Anti-Pattern 3: Leaking QNetworkReply Objects

**What people do:** Connect to QNetworkReply::finished() but forget to call reply->deleteLater() inside the handler.

**Why it's wrong:** Each HTTP poll (every 10s) leaks a QNetworkReply. Over 24h of kiosk uptime this accumulates 8640 leaked objects — eventual memory exhaustion on a Pi with limited RAM.

**Do this instead:**
```cpp
connect(reply, &QNetworkReply::finished, this, [reply, this]() {
    if (reply->error() == QNetworkReply::NoError) {
        processData(reply->readAll());
    }
    reply->deleteLater(); // always, even on error
});
```

### Anti-Pattern 4: Calling repaint() Instead of update()

**What people do:** Call `this->repaint()` immediately after setting a value — triggers synchronous paint.

**Why it's wrong:** Bypasses Qt's event loop coalescing. If multiple signals fire in rapid succession (e.g., both UDP and HTTP arrive close together), each call triggers a full paint pass.

**Do this instead:** Call `update()`, which queues a paint event. Qt coalesces multiple update() calls into a single paintEvent() per event loop cycle.

### Anti-Pattern 5: Using XCB/X11 Platform Plugin on Pi Kiosk

**What people do:** Run with default `-platform xcb` on a Pi with a desktop OS installed.

**Why it's wrong:** Adds X server overhead, requires window management, and the X11 path in Qt does not use hardware-accelerated rendering via EGL on Pi 4's VC4/V3D GPU.

**Do this instead:** Run with `-platform eglfs` (Pi 3) or configure eglfs with GBM backend for Pi 4 (fake KMS mode). Pi 4 requires `dtoverlay=vc4-kms-v3d` in `/boot/config.txt` and the `eglfs-gbm` backend. This gives direct GPU scanout without X11.

### Anti-Pattern 6: QAbstractItemModel for a Non-Table Data Domain

**What people do:** Subclass QAbstractItemModel to expose weather readings, then connect a QTableView or similar to display gauges.

**Why it's wrong:** QAbstractItemModel is designed for table/tree/list view widgets. Gauge widgets are custom-painted — they don't use Qt's model/view infrastructure. Forcing weather data through QAbstractItemModel adds unnecessary complexity.

**Do this instead:** Use a plain QObject subclass (WeatherDataModel) with typed signals. Reserve QAbstractItemModel for actual list/table views if any are needed.

---

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| WeatherLink Live HTTP `/v1/current_conditions` | QNetworkAccessManager GET, 10s QTimer, async reply signal | mDNS hostname `weatherlinklive.local.cisien.com` — add QTimer-based request timeout (e.g., 5s) since QNAM has no built-in timeout |
| WeatherLink Live UDP port 22222 | QUdpSocket bound to 0.0.0.0:22222, readyRead signal | Must POST `/v1/real_time?duration=N` to activate broadcast; re-POST before `N` seconds expire (use ~N-30s keepalive timer); N max is 1800s |
| WeatherLink Live HTTP `/v1/real_time` | POST to start UDP stream, re-POST to keep alive | Separate from the polling timer; use a second QTimer with interval = (duration - 30) * 1000 ms |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Network thread → Main thread | Qt::QueuedConnection (automatic when objects are on different threads) | Do not use Qt::DirectConnection across threads for model updates |
| HttpPoller → WeatherDataModel | Signal `conditionsReceived(HttpReading)` carrying a plain struct (copyable, no QObject) | HttpReading must be registered with Q_DECLARE_METATYPE if used in queued connections |
| UdpReceiver → WeatherDataModel | Signal `realtimeReceived(UdpReading)` carrying a plain struct | Same metatype registration needed |
| WeatherDataModel → Widgets | Per-field typed signals; widgets on GUI thread | Direct connections (same thread) — paintEvent triggered via update() |
| SparklineWidget → WeatherDataModel | SparklineWidget holds a const pointer/reference to model; reads ring buffer in paintEvent() | Read-only access; model must not resize/reallocate buffer during paint. Guard with a QReadWriteLock if model runs on a thread. Since model is on GUI thread, this is safe as-is. |

---

## Build Order Implications

The component dependency graph determines what to build first:

```
1. WeatherReadings.h (plain structs) — no dependencies, needed by everything
2. JsonParser — depends on WeatherReadings structs only; testable immediately
3. WeatherDataModel — depends on WeatherReadings; testable with fake data
4. HttpPoller + UdpReceiver — depend on JsonParser; testable with mock server
5. GaugeWidget (single field, hard-coded value) — no model dependency to start
6. CompassWidget — same: initially standalone
7. SparklineWidget — needs WeatherDataModel ring buffers; build after model
8. DashboardWindow — assembles all widgets + wires to model
9. main.cpp — starts QApplication, launches network thread, shows window
```

**Key insight:** Widgets and data sources can be developed in parallel because both depend only on `WeatherReadings.h` and `WeatherDataModel`'s interface. Build the model's API (header) first, even before the implementation.

---

## Sources

- Qt 6 Model/View Programming (official): https://doc.qt.io/qt-6/model-view-programming.html [HIGH confidence]
- QAbstractItemModel Class (official): https://doc.qt.io/qt-6/qabstractitemmodel.html [HIGH confidence]
- QUdpSocket Class (official): https://doc.qt.io/qt-6/qudpsocket.html [HIGH confidence]
- QNetworkAccessManager Class (official): https://doc.qt.io/qt-6/qnetworkaccessmanager.html [HIGH confidence]
- QNetworkAccessManager best practices: https://www.volkerkrause.eu/2022/11/19/qt-qnetworkaccessmanager-best-practices.html [MEDIUM confidence]
- QThread / moveToThread pattern (official): https://doc.qt.io/qt-6/qthread.html [HIGH confidence]
- Threads and QObjects (official): https://doc.qt.io/qt-6/threads-qobject.html [HIGH confidence]
- QWidget paintEvent / OpaquePaintEvent (official): https://doc.qt.io/qt-6/qwidget.html [HIGH confidence]
- Qt for Embedded Linux / eglfs (official): https://doc.qt.io/qt-6/embedded-linux.html [HIGH confidence]
- Qt 6 on Raspberry Pi EGLFS (community): https://bugfreeblog.duckdns.org/2021/09/qt-6-on-raspberry-pi-on-eglfs.html [MEDIUM confidence]
- Qt OpenGL vs QPainter performance on Pi: https://github.com/fhunleth/qt-rectangles [MEDIUM confidence]
- WeatherLink Live Local API: https://weatherlink.github.io/weatherlink-live-local-api/ [HIGH confidence]
- WeatherLink Live Local API source: https://github.com/weatherlink/weatherlink-live-local-api [HIGH confidence]
- Qwt Qt6 compatibility: https://qwt.sourceforge.io/ [MEDIUM confidence — library confirmed Qt6 compatible but no sparkline component; custom implementation required]
- Qt Weather Terminal example (reTerminal): https://raymii.org/s/tutorials/Qt_QML_WeatherTerminal_app_for_the_Seeed_reTerminal.html [MEDIUM confidence — QML-based, architectural patterns applicable]

---
*Architecture research for: Qt 6 C++ real-time weather dashboard (kiosk, Raspberry Pi)*
*Researched: 2026-03-01*
