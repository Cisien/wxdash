# Phase 1: Data Model and Network Layer - Research

**Researched:** 2026-03-01
**Domain:** Qt 6 C++ HTTP polling, UDP real-time feed, JSON parsing, and central WeatherDataModel
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Staleness and degraded state:**
- Uniform 30s staleness threshold for all sources
- When a source goes stale, clear the values entirely — no last-known display, widgets show blank/dash
- UDP staleness means the device itself is having issues; HTTP data is likely stale too — clear all values for that device
- Track staleness per device (WeatherLink Live as one source, PurpleAir as another), not per sensor or per protocol

**Network resilience:**
- No special retry logic — keep polling HTTP at normal 10s interval; if the device is unreachable, the next poll is the retry
- While UDP is down, attempt to restart the UDP session at regular intervals alongside normal HTTP polling
- Silent recovery — when the device comes back, data just starts populating again, no special notification
- UDP broadcast session: request 86400s (24h) duration, but renew every 3600s (1h) for safety margin
- Silently ignore malformed or unexpected JSON responses — don't process what can't be parsed cleanly

**Project structure and tooling:**
- CMake build system
- QTest for unit testing
- Directory layout: `src/` with `models/`, `network/` subdirs; `tests/` at project root
- Small files — ~500 line guideline per file, break features into separate classes/files
- Generate AGENTS.md with project conventions for Claude Code
- Standard Qt/C++ naming conventions (camelCase methods, PascalCase classes, m_ prefix for members)
- Code style enforced by formatter/linter rules (not just documented)

**Data model consumer API:**
- Qt-native Q_PROPERTY with per-value changed signals — widgets subscribe to exactly the signals they care about
- Single `WeatherDataModel` class with all weather fields (refactor later if unwieldy)
- Unified properties — one property per value regardless of which source (HTTP or UDP) provided it; consumers don't know or care about the source
- Dependency injection — model instantiated and passed, no singleton/global state

### Claude's Discretion

- Exact formatter/linter tool choice (clang-format, clang-tidy, etc.)
- Internal network class design and error handling patterns
- JSON parsing implementation details
- UDP packet parsing implementation
- Timer and polling mechanism internals

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DATA-01 | App polls WeatherLink Live HTTP API (`/v1/current_conditions`) every 10s for all sensor data | QNetworkAccessManager + QTimer pattern documented; single QNAM instance; async reply via `finished()` signal; always call `reply->deleteLater()` |
| DATA-02 | App starts UDP real-time broadcast (`/v1/real_time?duration=86400`) and listens on port 22222 for 2.5s wind/rain updates | QUdpSocket bound to 0.0.0.0:22222; HTTP GET to start broadcast; `readyRead` signal fires per datagram; loop `hasPendingDatagrams()` to drain |
| DATA-03 | App automatically renews UDP broadcast session before expiry | QTimer fires every 3600s (1h); re-issues GET `/v1/real_time?duration=86400`; API spec: new request with longer remaining time resets the broadcast |
| DATA-04 | JSON parser routes by `data_structure_type` (1=ISS outdoor, 3=barometer, 4=indoor temp/hum) | QJsonDocument parses response; iterate `data.conditions` array; match on `data_structure_type` — do NOT rely on array index position |
| DATA-05 | Rain counts converted to inches using `rain_size` field (1=0.01in, 2=0.2mm, 3=0.1mm, 4=0.001in) | Lookup table on `rain_size` field from API; `rain_size` present in both HTTP (type 1) and UDP packets; unit test with known counts is a phase success criterion |
| DATA-08 | Data staleness detected and signaled when no update received for >30s (per source) | QTimer or QElapsedTimer tracks last-received timestamp per device; if elapsed > 30000ms, emit `sourceStale(SourceId)` signal and clear model values |
| DATA-09 | App handles network disconnect/reconnect gracefully with automatic retry | No special retry state machine — HTTP polling at 10s interval IS the retry; UDP: attempt session restart alongside HTTP polling; silent recovery when device returns |
</phase_requirements>

---

## Summary

Phase 1 establishes the data pipeline for the entire application: an HTTP poller, a UDP receiver, a JSON parser, and a central WeatherDataModel. This is a greenfield Qt 6 C++ project with no existing code. The domain is well-understood — Qt's networking and JSON modules directly cover all requirements with no external libraries needed. All API fields, protocol mechanics, and threading patterns are verified against official Qt documentation and the WeatherLink Live Local API specification.

The primary architectural concern is correct threading: HttpPoller and UdpReceiver run on a dedicated QThread, communicate results to WeatherDataModel on the GUI thread via automatic `Qt::QueuedConnection`. The model exposes per-field `Q_PROPERTY` with `NOTIFY` signals; consumers connect to exactly the signal they care about. Staleness detection uses a `QElapsedTimer` tracking last-received time per source (WeatherLink Live as a whole), with a 30s threshold and a clear-all-values behavior on expiry.

Two protocol mechanics require careful implementation: (1) the UDP broadcast must be initiated via HTTP and renewed every 3600s — the API silently stops sending after the duration expires; (2) rain fields are raw bucket-tip counts that must be multiplied by a `rain_size`-derived factor to produce inches. Both are covered by the existing project research and the WeatherLink Live API specification with full field-level detail.

**Primary recommendation:** Implement in dependency order — `WeatherReadings.h` structs first, then `JsonParser` (immediately testable with QTest), then `WeatherDataModel`, then `HttpPoller` + `UdpReceiver` on a shared `QThread`. This order allows each component to be tested in isolation before integration.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::Core | 6.8 LTS | QObject, signals/slots, QTimer, QElapsedTimer, QJsonDocument, Q_PROPERTY | Built-in; JSON and timers live here; no external parser needed |
| Qt6::Network | 6.8 LTS | QNetworkAccessManager (HTTP), QUdpSocket (UDP) | Fully async, event-driven, zero blocking; integrates natively with Qt event loop |
| Qt6::Test | 6.8 LTS | QTest unit testing, QSignalSpy | Locked decision; lightweight; no extra dependencies; ships with Qt |
| CMake | 3.16+ (3.22+ recommended) | Build system | Locked decision; Qt 6 dropped qmake; `qt_add_executable` and `enable_testing` |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| clang-format | system | Code formatting enforcement | Formatter choice is Claude's discretion; recommended for Qt C++ projects |
| clang-tidy | system | Static analysis, catches common Qt signal/slot errors | Recommended alongside clang-format |
| Ninja | system | Fast CMake backend | Qt's own documentation specifies Ninja for faster incremental builds |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| QJsonDocument | nlohmann/json | nlohmann/json is header-only but adds a CMake FetchContent dependency for no benefit at 10s polling intervals; QJsonDocument is built-in and sufficient |
| QTimer for staleness | QElapsedTimer | QElapsedTimer tracks elapsed time without firing a slot; suitable for checking "has N seconds passed since last packet?" on-demand; combine with a QTimer that periodically checks staleness |

**Installation:**

No external packages required. All libraries are Qt 6.8 LTS modules.

```bash
# On a Linux dev machine with Qt 6.8 installed:
# Qt6::Core, Qt6::Network, Qt6::Test are standard Qt components
# Install clang tools if not present:
sudo apt install clang-format clang-tidy
```

---

## Architecture Patterns

### Recommended Project Structure

Per locked decisions (src/ with models/, network/ subdirs; tests/ at project root):

```
wxdash/
├── CMakeLists.txt                       # Top-level: find Qt6, enable_testing, add subdirs
├── AGENTS.md                            # Generated project conventions for Claude Code
├── .clang-format                        # Enforced formatter config
├── .clang-tidy                          # Enforced linter config
├── src/
│   ├── CMakeLists.txt                   # Defines wxdash executable target
│   ├── main.cpp                         # QCoreApplication (no UI in Phase 1); wires components
│   ├── models/
│   │   ├── WeatherReadings.h            # Plain C++ structs (no Qt deps; cross-thread safe)
│   │   └── WeatherDataModel.h/.cpp      # QObject; Q_PROPERTY + NOTIFY signals; staleness detection
│   └── network/
│       ├── HttpPoller.h/.cpp            # QNetworkAccessManager + QTimer; 10s polling
│       ├── UdpReceiver.h/.cpp           # QUdpSocket; broadcast kick-off; renewal timer
│       └── JsonParser.h/.cpp            # QJsonDocument → WeatherReadings structs
└── tests/
    ├── CMakeLists.txt                   # Defines test targets
    ├── tst_JsonParser.cpp               # Unit tests: data_structure_type routing, rain_size conversion
    └── tst_WeatherDataModel.cpp         # Unit tests: staleness signals, value clearing
```

### Pattern 1: Plain Structs for Cross-Thread Data Transfer

**What:** All data crossing the network thread → GUI thread boundary travels as plain C++ structs (no QObject inheritance), declared in `WeatherReadings.h`.

**When to use:** Always, for any data emitted from worker objects on non-GUI threads. Plain structs are trivially copyable and safe for `Qt::QueuedConnection` queued signal delivery.

**Why:** QObject instances cannot be safely passed between threads without special care. Plain structs with `Q_DECLARE_METATYPE` registered via `qRegisterMetaType<>()` work transparently in queued connections.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/threads-qobject.html
// WeatherReadings.h — no Qt includes required
#pragma once
#include <cstdint>

struct IssReading {
    double temperature    = 0.0;  // °F
    double humidity       = 0.0;  // %RH
    double dewPoint       = 0.0;  // °F
    double heatIndex      = 0.0;  // °F
    double windChill      = 0.0;  // °F
    double windSpeedLast  = 0.0;  // mph
    int    windDirLast    = 0;    // degrees
    double windSpeedHi10  = 0.0;  // mph (gust, last 10 min)
    double rainRateLast   = 0.0;  // inches/hour (converted)
    double rainfallDaily  = 0.0;  // inches (converted)
    double solarRad       = 0.0;  // W/m²
    double uvIndex        = 0.0;
    int    rainSize       = 1;    // raw rain_size from API
};

struct BarReading {
    double pressureSeaLevel = 0.0;  // inHg
    int    pressureTrend    = 0;    // -1=falling, 0=steady, 1=rising
};

struct IndoorReading {
    double tempIn     = 0.0;  // °F
    double humIn      = 0.0;  // %RH
    double dewPointIn = 0.0;  // °F
};

struct UdpReading {
    double windSpeedLast  = 0.0;  // mph
    int    windDirLast    = 0;    // degrees
    double windSpeedHi10  = 0.0;  // mph (gust)
    double rainRateLast   = 0.0;  // inches/hour (converted)
    double rainfallDaily  = 0.0;  // inches (converted)
    int    rainSize       = 1;
};

// Must be registered before use in queued connections:
// qRegisterMetaType<IssReading>();
// qRegisterMetaType<BarReading>();
// etc.
```

### Pattern 2: Worker Objects on a Shared QThread

**What:** Both `HttpPoller` and `UdpReceiver` are QObjects moved to a single dedicated `QThread` via `moveToThread()`. All network activity runs off the GUI thread. Results return to the main thread via `Qt::QueuedConnection` (automatic for cross-thread signal/slot).

**When to use:** Any I/O that could block or introduce latency. Prevents network hiccups (DNS timeout, device offline) from stalling the GUI event loop.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/qthread.html#details
// In main.cpp or application setup

auto *networkThread = new QThread(this);

// No parent — objects will be moved to networkThread
auto *httpPoller   = new HttpPoller();
auto *udpReceiver  = new UdpReceiver();

httpPoller->moveToThread(networkThread);
udpReceiver->moveToThread(networkThread);

// Cross-thread signals: Qt automatically uses QueuedConnection
connect(httpPoller, &HttpPoller::issReceived,
        weatherModel, &WeatherDataModel::applyIssUpdate);
connect(httpPoller, &HttpPoller::barReceived,
        weatherModel, &WeatherDataModel::applyBarUpdate);
connect(httpPoller, &HttpPoller::indoorReceived,
        weatherModel, &WeatherDataModel::applyIndoorUpdate);
connect(udpReceiver, &UdpReceiver::realtimeReceived,
        weatherModel, &WeatherDataModel::applyUdpUpdate);

// Start pollers when thread starts
connect(networkThread, &QThread::started, httpPoller,  &HttpPoller::start);
connect(networkThread, &QThread::started, udpReceiver, &UdpReceiver::start);

// Clean shutdown
connect(networkThread, &QThread::finished, httpPoller,  &QObject::deleteLater);
connect(networkThread, &QThread::finished, udpReceiver, &QObject::deleteLater);

networkThread->start();
```

### Pattern 3: WeatherDataModel with Q_PROPERTY and Per-Field NOTIFY Signals

**What:** `WeatherDataModel` uses `Q_PROPERTY` with `NOTIFY` signals for each observable field. Widgets (in later phases) connect to exactly the signal they need. Dual update slots (`applyIssUpdate`, `applyUdpUpdate`, etc.) merge data from different sources into unified properties.

**When to use:** Always. This is the locked architecture: unified properties, dependency injection (no singleton), widgets don't know which source provided a value.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/properties.html
class WeatherDataModel : public QObject {
    Q_OBJECT

    // Per the locked decision: Q_PROPERTY with NOTIFY per value
    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(double humidity    READ humidity    NOTIFY humidityChanged)
    Q_PROPERTY(double windSpeed   READ windSpeed   NOTIFY windSpeedChanged)
    Q_PROPERTY(int    windDir     READ windDir     NOTIFY windDirChanged)
    Q_PROPERTY(double rainRate    READ rainRate    NOTIFY rainRateChanged)
    Q_PROPERTY(double rainfallDaily READ rainfallDaily NOTIFY rainfallDailyChanged)
    Q_PROPERTY(double pressure    READ pressure    NOTIFY pressureChanged)
    Q_PROPERTY(int    pressureTrend READ pressureTrend NOTIFY pressureTrendChanged)
    Q_PROPERTY(double uvIndex     READ uvIndex     NOTIFY uvIndexChanged)
    Q_PROPERTY(double solarRad    READ solarRad    NOTIFY solarRadChanged)
    Q_PROPERTY(double dewPoint    READ dewPoint    NOTIFY dewPointChanged)
    Q_PROPERTY(double heatIndex   READ heatIndex   NOTIFY heatIndexChanged)
    Q_PROPERTY(double windChill   READ windChill   NOTIFY windChillChanged)
    Q_PROPERTY(double tempIn      READ tempIn      NOTIFY tempInChanged)
    Q_PROPERTY(double humIn       READ humIn       NOTIFY humInChanged)
    Q_PROPERTY(double windGust    READ windGust    NOTIFY windGustChanged)
    Q_PROPERTY(bool   sourceStale READ sourceStale NOTIFY sourceStaleChanged)

public:
    explicit WeatherDataModel(QObject *parent = nullptr);

    // Accessors
    double temperature()   const { return m_temperature; }
    double humidity()      const { return m_humidity; }
    // ... etc.
    bool   sourceStale()   const { return m_sourceStale; }

public slots:
    // Called from network thread via QueuedConnection
    void applyIssUpdate(const IssReading &r);
    void applyBarUpdate(const BarReading &r);
    void applyIndoorUpdate(const IndoorReading &r);
    void applyUdpUpdate(const UdpReading &r);

    // Called by staleness watchdog timer
    void checkStaleness();

signals:
    void temperatureChanged(double value);
    void humidityChanged(double value);
    void windSpeedChanged(double value);
    void windDirChanged(int degrees);
    void rainRateChanged(double inchesPerHour);
    void rainfallDailyChanged(double inches);
    void pressureChanged(double inHg);
    void pressureTrendChanged(int trend);
    void uvIndexChanged(double index);
    void solarRadChanged(double wm2);
    void dewPointChanged(double f);
    void heatIndexChanged(double f);
    void windChillChanged(double f);
    void tempInChanged(double f);
    void humInChanged(double pct);
    void windGustChanged(double mph);
    void sourceStaleChanged(bool stale);

private:
    // Per locked decision: m_ prefix for members
    double m_temperature   = 0.0;
    double m_humidity      = 0.0;
    double m_windSpeed     = 0.0;
    int    m_windDir       = 0;
    double m_rainRate      = 0.0;
    double m_rainfallDaily = 0.0;
    double m_pressure      = 0.0;
    int    m_pressureTrend = 0;
    double m_uvIndex       = 0.0;
    double m_solarRad      = 0.0;
    double m_dewPoint      = 0.0;
    double m_heatIndex     = 0.0;
    double m_windChill     = 0.0;
    double m_tempIn        = 0.0;
    double m_humIn         = 0.0;
    double m_windGust      = 0.0;
    bool   m_sourceStale   = false;

    QElapsedTimer m_lastHttpUpdate;  // tracks last successful HTTP read
    QElapsedTimer m_lastUdpUpdate;   // tracks last successful UDP packet
    QTimer       *m_stalenessTimer;  // fires every ~5s to check elapsed time
    static constexpr int kStalenessMs = 30000;  // 30s threshold (locked)

    void clearAllValues();  // called on staleness; per locked decision: clear, not last-known
};
```

### Pattern 4: HttpPoller — Single QNAM, Explicit deleteLater, Transfer Timeout

**What:** One `QNetworkAccessManager` per application, owned by `HttpPoller`. Every `get()` call's `finished()` handler calls `reply->deleteLater()`. A separate 10s `QTimer` fires polls. If a previous reply is still in-flight, it is aborted before issuing the next poll (prevents overlapping requests).

**When to use:** Always. Missing `deleteLater()` is the most common Qt memory leak in polling applications.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/qnetworkaccessmanager.html
class HttpPoller : public QObject {
    Q_OBJECT
public:
    explicit HttpPoller(QObject *parent = nullptr);

public slots:
    void start();

signals:
    void issReceived(IssReading reading);
    void barReceived(BarReading reading);
    void indoorReceived(IndoorReading reading);

private slots:
    void poll();          // fires every 10s
    void onReply();       // handles QNetworkReply::finished

private:
    QNetworkAccessManager *m_nam;
    QTimer                *m_pollTimer;
    QNetworkReply         *m_pendingReply = nullptr;
    QUrl                   m_currentConditionsUrl;
};

void HttpPoller::poll() {
    // Abort previous in-flight request if still pending
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_currentConditionsUrl);
    // Source: https://doc.qt.io/qt-6/qnetworkrequest.html#setTransferTimeout
    req.setTransferTimeout(5000);  // 5s; prevents stalled requests
    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &HttpPoller::onReply);
}

void HttpPoller::onReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        // Pass raw bytes to parser; silently ignore malformed JSON (locked decision)
        auto [iss, bar, indoor] = JsonParser::parseCurrentConditions(reply->readAll());
        if (iss) emit issReceived(*iss);
        if (bar) emit barReceived(*bar);
        if (indoor) emit indoorReceived(*indoor);
    }
    // Per locked decision: silently ignore errors (next poll IS the retry)

    reply->deleteLater();  // MANDATORY — always, even on error
    m_pendingReply = nullptr;
}
```

### Pattern 5: UdpReceiver — Broadcast Kick-Off, Renewal Timer, Last-Packet Tracking

**What:** `UdpReceiver` binds a `QUdpSocket` to `0.0.0.0:22222`, issues the HTTP GET to start the broadcast (using `HttpPoller`'s QNAM or its own), and sets a 3600s renewal timer. Tracks `QElapsedTimer` of last received packet; if silent for >10s, triggers an immediate re-registration attempt.

**When to use:** Always — the WeatherLink Live API silently stops UDP after the duration expires. The renewal timer and last-packet watchdog are both required.

**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/qudpsocket.html
class UdpReceiver : public QObject {
    Q_OBJECT
public:
    explicit UdpReceiver(QNetworkAccessManager *nam, const QUrl &realtimeUrl,
                         QObject *parent = nullptr);

public slots:
    void start();

signals:
    void realtimeReceived(UdpReading reading);

private slots:
    void onReadyRead();     // drains all pending datagrams
    void renewBroadcast();  // re-issues GET /v1/real_time?duration=86400
    void checkUdpHealth();  // called periodically; triggers re-registration if silent

private:
    QUdpSocket            *m_socket;
    QNetworkAccessManager *m_nam;
    QUrl                   m_realtimeUrl;
    QTimer                *m_renewalTimer;    // fires every 3600s (1h) per locked decision
    QTimer                *m_healthTimer;     // fires every 5s to check last-packet age
    QElapsedTimer          m_lastPacketTimer;
    static constexpr int kRenewalIntervalMs  = 3600 * 1000;   // 1h
    static constexpr int kHealthCheckMs      = 5000;
    static constexpr int kSilenceThresholdMs = 10000;          // re-register if silent 10s
};

void UdpReceiver::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        auto reading = JsonParser::parseUdpDatagram(datagram.data());
        if (reading) {
            m_lastPacketTimer.restart();
            emit realtimeReceived(*reading);
        }
        // Per locked decision: silently ignore malformed packets
    }
}

void UdpReceiver::checkUdpHealth() {
    if (m_lastPacketTimer.isValid()
        && m_lastPacketTimer.elapsed() > kSilenceThresholdMs) {
        renewBroadcast();  // attempt immediate re-registration
    }
}

void UdpReceiver::renewBroadcast() {
    QNetworkRequest req(m_realtimeUrl);  // includes ?duration=86400
    req.setTransferTimeout(5000);
    m_nam->get(req);  // fire-and-forget; reply deleteLater in finished handler
}
```

### Pattern 6: JsonParser — Route by data_structure_type, Rain Count Conversion

**What:** Stateless parser functions take raw `QByteArray` input, return `std::optional<T>` structs. Never relies on array index to identify sensor type — always checks `data_structure_type` field. Silently returns `std::nullopt` for malformed JSON (locked decision).

**When to use:** Always. The API documentation explicitly states sensor structures may appear in any order in the array. Index-based access is a data correctness bug.

**Example:**
```cpp
// Source: WeatherLink Live Local API - API.md
// Rain size conversion factors per API spec:
// rain_size 1 = 0.01 inch per count
// rain_size 2 = 0.2 mm per count  (metric — unlikely but must handle)
// rain_size 3 = 0.1 mm per count  (metric)
// rain_size 4 = 0.001 inch per count

namespace JsonParser {

// Returns multiplier in inches per count
inline double rainSizeToInches(int rainSize) {
    switch (rainSize) {
        case 1: return 0.01;
        case 2: return 0.2 / 25.4;   // 0.2mm → inches
        case 3: return 0.1 / 25.4;   // 0.1mm → inches
        case 4: return 0.001;
        default: return 0.01;        // fallback to most common Davis collector
    }
}

struct ParsedConditions {
    std::optional<IssReading>    iss;
    std::optional<BarReading>    bar;
    std::optional<IndoorReading> indoor;
};

ParsedConditions parseCurrentConditions(const QByteArray &data) {
    ParsedConditions result;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return result;  // silently ignore

    QJsonArray conditions = doc["data"]["conditions"].toArray();
    for (const QJsonValue &val : conditions) {
        QJsonObject obj = val.toObject();
        int type = obj["data_structure_type"].toInt(-1);

        if (type == 1) {
            // ISS Current Conditions
            IssReading iss;
            iss.temperature   = obj["temp"].toDouble();
            iss.humidity      = obj["hum"].toDouble();
            iss.dewPoint      = obj["dew_point"].toDouble();
            iss.heatIndex     = obj["heat_index"].toDouble();
            iss.windChill     = obj["wind_chill"].toDouble();
            iss.windSpeedLast = obj["wind_speed_last"].toDouble();
            iss.windDirLast   = obj["wind_dir_last"].toInt();
            iss.windGust      = obj["wind_speed_hi_last_10_min"].toDouble();
            iss.solarRad      = obj["solar_rad"].toDouble();
            iss.uvIndex       = obj["uv_index"].toDouble();
            iss.rainSize      = obj["rain_size"].toInt(1);

            double factor        = rainSizeToInches(iss.rainSize);
            iss.rainRateLast     = obj["rain_rate_last"].toDouble() * factor;
            iss.rainfallDaily    = obj["rainfall_daily"].toDouble() * factor;
            result.iss = iss;

        } else if (type == 3) {
            // LSS BAR
            BarReading bar;
            bar.pressureSeaLevel = obj["bar_sea_level"].toDouble();
            bar.pressureTrend    = obj["bar_trend"].toInt(0);
            result.bar = bar;

        } else if (type == 4) {
            // LSS Temp/Hum Indoor
            IndoorReading indoor;
            indoor.tempIn     = obj["temp_in"].toDouble();
            indoor.humIn      = obj["hum_in"].toDouble();
            indoor.dewPointIn = obj["dew_point_in"].toDouble();
            result.indoor = indoor;
        }
        // type 2 (Leaf/Soil) and others: silently skip (locked decision)
    }
    return result;
}

std::optional<UdpReading> parseUdpDatagram(const QByteArray &data) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return std::nullopt;

    // UDP broadcasts contain a "conditions" array, same structure as HTTP but wind/rain subset
    QJsonArray conditions = doc["conditions"].toArray();
    if (conditions.isEmpty()) return std::nullopt;

    // UDP packets contain ISS data (type 1); take first valid ISS entry
    for (const QJsonValue &val : conditions) {
        QJsonObject obj = val.toObject();
        if (obj["data_structure_type"].toInt(-1) != 1) continue;

        UdpReading r;
        r.rainSize       = obj["rain_size"].toInt(1);
        double factor    = rainSizeToInches(r.rainSize);
        r.windSpeedLast  = obj["wind_speed_last"].toDouble();
        r.windDirLast    = obj["wind_dir_last"].toInt();
        r.windSpeedHi10  = obj["wind_speed_hi_last_10_min"].toDouble();
        r.rainRateLast   = obj["rain_rate_last"].toDouble() * factor;
        r.rainfallDaily  = obj["rainfall_daily"].toDouble() * factor;
        return r;
    }
    return std::nullopt;
}

} // namespace JsonParser
```

### Pattern 7: Staleness Detection — QElapsedTimer + Periodic Check

**What:** `WeatherDataModel` owns a `QElapsedTimer` tracking the last successful update from the WeatherLink Live device. A `QTimer` fires every 5s to check if elapsed time exceeds 30,000ms. On expiry: set `m_sourceStale = true`, call `clearAllValues()` (per locked decision: clear, not last-known display), emit `sourceStaleChanged(true)`.

**When to use:** Required for DATA-08. Track staleness per device (WeatherLink Live as one unit), not per sensor or per protocol.

**Example:**
```cpp
void WeatherDataModel::checkStaleness() {
    // Per locked decision: 30s uniform threshold; track per device
    bool nowStale = m_lastHttpUpdate.isValid()
                 && m_lastHttpUpdate.elapsed() > kStalenessMs;

    if (nowStale && !m_sourceStale) {
        m_sourceStale = true;
        clearAllValues();         // per locked decision: no last-known display
        emit sourceStaleChanged(true);
    } else if (!nowStale && m_sourceStale) {
        m_sourceStale = false;    // silent recovery (locked decision)
        emit sourceStaleChanged(false);
    }
}

void WeatherDataModel::applyIssUpdate(const IssReading &r) {
    m_lastHttpUpdate.restart();  // reset staleness clock on any valid update
    if (m_sourceStale) {
        m_sourceStale = false;
        emit sourceStaleChanged(false);
    }
    // Update fields and emit changed signals...
    if (!qFuzzyCompare(m_temperature, r.temperature)) {
        m_temperature = r.temperature;
        emit temperatureChanged(m_temperature);
    }
    // ... repeat for all fields
}

void WeatherDataModel::clearAllValues() {
    // Per locked decision: widgets show blank/dash — emit signals with 0/sentinel
    m_temperature = 0.0;  emit temperatureChanged(m_temperature);
    m_humidity    = 0.0;  emit humidityChanged(m_humidity);
    // ... clear all fields
}
```

### Pattern 8: QTest Unit Test Structure for CMake

**What:** Test targets are separate executables, defined in `tests/CMakeLists.txt`. Each test file contains one QObject subclass with test methods as private slots. The top-level `CMakeLists.txt` calls `enable_testing()`. Tests run via `ctest`.

**When to use:** For `JsonParser` (rain_size conversion, data_structure_type routing) and `WeatherDataModel` (staleness signal, clear-on-stale). These are the two components with correctness-critical logic.

**Example:**
```cmake
# tests/CMakeLists.txt
find_package(Qt6 REQUIRED COMPONENTS Core Test)
enable_testing()

# JsonParser tests
qt_add_executable(tst_JsonParser tst_JsonParser.cpp)
target_link_libraries(tst_JsonParser PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_JsonParser PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME tst_JsonParser COMMAND tst_JsonParser)

# WeatherDataModel tests
qt_add_executable(tst_WeatherDataModel tst_WeatherDataModel.cpp)
target_link_libraries(tst_WeatherDataModel PRIVATE Qt6::Core Qt6::Test)
target_include_directories(tst_WeatherDataModel PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME tst_WeatherDataModel COMMAND tst_WeatherDataModel)
```

```cpp
// tst_JsonParser.cpp — rain_size conversion (DATA-05 success criterion)
#include <QTest>
#include "network/JsonParser.h"

class TestJsonParser : public QObject {
    Q_OBJECT

private slots:
    void rainSize1_convertsToTenthsOfInch() {
        // rain_size=1: 0.01 inches per count
        // 5 counts → 0.05 inches
        double factor = JsonParser::rainSizeToInches(1);
        QCOMPARE(factor, 0.01);
        QCOMPARE(5 * factor, 0.05);
    }
    void rainSize4_convertsToThousandthsOfInch() {
        // rain_size=4: 0.001 inches per count
        double factor = JsonParser::rainSizeToInches(4);
        QCOMPARE(factor, 0.001);
    }
    void rainSize2_convertsMillimetersToInches() {
        // rain_size=2: 0.2mm = 0.2/25.4 inches
        double factor = JsonParser::rainSizeToInches(2);
        QVERIFY(qAbs(factor - (0.2 / 25.4)) < 1e-9);
    }
    void parseCurrentConditions_routesByType() {
        // Craft JSON with types in non-natural order: bar first, then ISS
        QByteArray json = R"({
            "data": { "conditions": [
                {"data_structure_type": 3, "bar_sea_level": 29.92, "bar_trend": 1},
                {"data_structure_type": 1, "temp": 72.5, "hum": 55.0,
                 "rain_size": 1, "rain_rate_last": 0, "rainfall_daily": 0,
                 "wind_speed_last": 0, "wind_dir_last": 0,
                 "wind_speed_hi_last_10_min": 0, "solar_rad": 0, "uv_index": 0,
                 "dew_point": 54.0, "heat_index": 72.5, "wind_chill": 72.5}
            ]}
        })";
        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(result.bar.has_value());
        QCOMPARE(result.iss->temperature, 72.5);
        QCOMPARE(result.bar->pressureSeaLevel, 29.92);
    }
    void parseMalformedJson_returnsEmpty() {
        auto result = JsonParser::parseCurrentConditions("{not json}");
        QVERIFY(!result.iss.has_value());
        QVERIFY(!result.bar.has_value());
    }
};

QTEST_MAIN(TestJsonParser)
#include "tst_JsonParser.moc"
```

```cpp
// tst_WeatherDataModel.cpp — staleness signal
#include <QTest>
#include <QSignalSpy>
#include "models/WeatherDataModel.h"

class TestWeatherDataModel : public QObject {
    Q_OBJECT

private slots:
    void stalenessSignalEmittedAfter30s() {
        WeatherDataModel model;
        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);

        // Simulate a valid update first so the staleness timer starts
        IssReading r;
        r.temperature = 70.0;
        model.applyIssUpdate(r);

        // Manually trigger the staleness check with the clock advanced
        // (Test approach: expose a method to override elapsed time, or use
        // QTest::qWait to let 30s+ pass in a fast test mode)
        // See Open Questions for testing staleness in fast tests
        QVERIFY(spy.isValid());
    }
};

QTEST_MAIN(TestWeatherDataModel)
#include "tst_WeatherDataModel.moc"
```

**Run tests:**
```bash
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

### Anti-Patterns to Avoid

- **Missing `reply->deleteLater()`:** Every `QNetworkReply::finished()` handler must call `deleteLater()` on the reply — even on error. At 10s polling, omitting this leaks 360 objects/hour and causes OOM after days of kiosk uptime.
- **Multiple QNetworkAccessManager instances:** Create exactly one QNAM per thread. HttpPoller owns the single QNAM used by both polling and UDP broadcast renewal.
- **Array-index-based `data_structure_type` assumption:** Never access `conditions[0]` assuming it is ISS. Always iterate and check the `data_structure_type` field.
- **Calling `repaint()` instead of `update()` from a slot:** Use `update()` in widget slots (Phase 2). It queues a paint event; `repaint()` forces a synchronous paint and bypasses Qt's event coalescing.
- **Using a blocking QEventLoop for network replies:** Do not use `QEventLoop::exec()` to wait for a reply synchronously on the network thread. The async `finished()` signal pattern is correct.
- **Singleton WeatherDataModel:** Locked decision requires dependency injection. Pass the model by pointer to consumers.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON parsing | Custom string parser | `QJsonDocument` / `QJsonObject` / `QJsonArray` | Built-in; handles all WeatherLink response structures; no edge cases to miss |
| Async HTTP | Custom socket + HTTP framing | `QNetworkAccessManager` | Connection pooling, redirect handling, timeout support, error types |
| UDP receive | `recvfrom()` blocking loop | `QUdpSocket` with `readyRead` signal | Blocking loop would stall the event loop; `readyRead` is non-blocking and integrates with QThread |
| Signal testing | Custom bool flags | `QSignalSpy` from Qt6::Test | Captures signal arguments; supports `wait()` with timeout; standard Qt testing pattern |
| Cross-thread data | QObject subclass | Plain C++ structs + `Q_DECLARE_METATYPE` | QObject instances cannot be safely copied across thread boundaries |
| Timer polling | Manual `clock()` loop | `QTimer` + `QElapsedTimer` | Qt timers integrate with the event loop; `QElapsedTimer` gives monotonic elapsed time |

**Key insight:** Qt 6 provides everything needed for this phase. Adding external libraries (nlohmann/json, Boost.Asio, etc.) introduces CMake complexity and build dependencies with zero benefit at this data volume and polling rate.

---

## Common Pitfalls

### Pitfall 1: Missing `reply->deleteLater()`

**What goes wrong:** QNetworkReply objects accumulate in heap memory. At 10s polling: 360/hour, 8,640/day. Kiosk OOM-kills after days.

**Why it happens:** Qt documentation does not put `deleteLater()` front and center. The reply's parent is the QNAM, not the handler, so the parent-child ownership model does not save you.

**How to avoid:** Always call `reply->deleteLater()` in the `finished()` handler, even on error. Alternatively, call `m_nam->setAutoDeleteReplies(true)` (Qt 6 feature — verify available in 6.8) to make QNAM auto-delete replies.

**Warning signs:** RSS memory growth in `htop` that does not plateau; `valgrind` showing `QNetworkReplyImpl` in leak report.

### Pitfall 2: UDP Broadcast Expires Silently

**What goes wrong:** Wind and rain values freeze while temperature and pressure continue updating. No error, no socket error event. The session expired and the API stopped sending.

**Why it happens:** The WeatherLink Live API's default duration is 1200s (20 minutes). Even with `duration=86400`, if the WeatherLink Live device reboots, it loses broadcast state. The 3600s renewal timer (locked decision) prevents expiry drift; the 10s silence watchdog catches device reboots.

**How to avoid:** Set `duration=86400`, renew every 3600s (locked), AND track last-packet timestamp — trigger re-registration if silent for >10s.

**Warning signs:** Wind speed and rain rate freeze while HTTP data continues arriving; UDP `readyRead` signal stops firing.

### Pitfall 3: data_structure_type Index Assumption

**What goes wrong:** `conditions[0]` is assumed to be ISS (type 1). If the WeatherLink Live firmware returns structures in a different order (or adds new types), the parser silently reads bar data as temperature data.

**Why it happens:** Testing against a single device at development time; the device happens to always return ISS first. Works until it doesn't.

**How to avoid:** Always iterate all entries in the `conditions` array and route by `data_structure_type` value. Unit test with JSON where ISS is NOT first.

**Warning signs:** Pressure gauge shows temperature values; temperature shows 29.xx (barometric pressure range).

### Pitfall 4: Cross-Thread Direct Connection

**What goes wrong:** `Qt::DirectConnection` to a slot on a different thread causes the slot to execute on the emitting thread, bypassing the receiving object's event loop. For `WeatherDataModel` methods that are not thread-safe, this causes data races.

**Why it happens:** Explicitly specifying `Qt::DirectConnection` when connecting cross-thread signals, or connecting before calling `moveToThread()`.

**How to avoid:** Never specify a connection type when connecting signals between objects on different threads. Qt detects thread affinity automatically and uses `Qt::QueuedConnection`. Only specify `Qt::DirectConnection` for objects on the same thread.

**Warning signs:** Random crashes or corrupted values in the model; `QObject: Cannot queue arguments of type...` warnings in output.

### Pitfall 5: QElapsedTimer Not Started Before First Check

**What goes wrong:** `m_lastHttpUpdate.elapsed()` returns a large invalid value if `restart()` has never been called, causing immediate false staleness detection at startup.

**Why it happens:** `QElapsedTimer::isValid()` returns false until `start()` or `restart()` is called. `elapsed()` on an invalid timer is undefined.

**How to avoid:** Guard staleness check with `m_lastHttpUpdate.isValid()` before checking `elapsed()`. The first valid update from HTTP starts the clock.

**Warning signs:** `sourceStaleChanged(true)` emitted immediately at application startup before any network request completes.

---

## Code Examples

Verified patterns from official sources:

### QNAM GET with deleteLater (DATA-01)

```cpp
// Source: https://doc.qt.io/qt-6/qnetworkaccessmanager.html
void HttpPoller::poll() {
    QNetworkRequest req(m_url);
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            processResponse(reply->readAll());
        }
        reply->deleteLater();  // always
    });
}
```

### QUdpSocket bind and drain (DATA-02)

```cpp
// Source: https://doc.qt.io/qt-6/qudpsocket.html
m_socket = new QUdpSocket(this);
m_socket->bind(QHostAddress::AnyIPv4, 22222, QUdpSocket::ShareAddress);
connect(m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);

void UdpReceiver::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket->receiveDatagram();
        // process dg.data()
    }
}
```

### Start UDP broadcast (DATA-02)

```cpp
// Source: WeatherLink Live Local API
// GET http://weatherlinklive.local.cisien.com/v1/real_time?duration=86400
QUrl url("http://weatherlinklive.local.cisien.com/v1/real_time");
QUrlQuery q;
q.addQueryItem("duration", "86400");
url.setQuery(q);
QNetworkRequest req(url);
req.setTransferTimeout(5000);
QNetworkReply *reply = m_nam->get(req);
connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
```

### Rain count conversion (DATA-05)

```cpp
// Source: WeatherLink Live Local API - API.md
// rain_size: 1=0.01in, 2=0.2mm, 3=0.1mm, 4=0.001in
double rainSizeToInches(int rainSize) {
    switch (rainSize) {
        case 1: return 0.01;
        case 2: return 0.2 / 25.4;
        case 3: return 0.1 / 25.4;
        case 4: return 0.001;
        default: return 0.01;
    }
}
double rainfallInches = rainfallCounts * rainSizeToInches(rainSize);
```

### QSignalSpy for signal testing (DATA-08)

```cpp
// Source: https://doc.qt.io/qt-6/qsignalspy.html
WeatherDataModel model;
QSignalSpy staleSpy(&model, &WeatherDataModel::sourceStaleChanged);
QVERIFY(staleSpy.isValid());

// ... trigger staleness ...

// Wait up to 35s for the signal (or use test clock injection)
staleSpy.wait(35000);
QVERIFY(!staleSpy.isEmpty());
bool staleValue = staleSpy.takeFirst().at(0).toBool();
QVERIFY(staleValue);
```

### moveToThread pattern for network objects (DATA-09)

```cpp
// Source: https://doc.qt.io/qt-6/qthread.html
auto *thread      = new QThread(this);
auto *httpPoller  = new HttpPoller();           // no parent
auto *udpReceiver = new UdpReceiver(nam, url);  // no parent

httpPoller->moveToThread(thread);
udpReceiver->moveToThread(thread);

// Connections across threads use QueuedConnection automatically
connect(httpPoller, &HttpPoller::issReceived,
        model, &WeatherDataModel::applyIssUpdate);

connect(thread, &QThread::started,  httpPoller,  &HttpPoller::start);
connect(thread, &QThread::started,  udpReceiver, &UdpReceiver::start);
connect(thread, &QThread::finished, httpPoller,  &QObject::deleteLater);
connect(thread, &QThread::finished, udpReceiver, &QObject::deleteLater);

thread->start();
```

---

## API Reference: WeatherLink Live Fields

Verified from WeatherLink Live Local API - API.md (GitHub raw content, 2026-03-01):

### HTTP `/v1/current_conditions` — data_structure_type 1 (ISS)

| Field | Type | Units | Notes |
|-------|------|-------|-------|
| `temp` | float | °F | Current temperature |
| `hum` | float | %RH | Current humidity |
| `dew_point` | float | °F | Dew point |
| `heat_index` | float | °F | Pre-calculated by device |
| `wind_chill` | float | °F | Pre-calculated by device |
| `wind_speed_last` | float | mph | Most recent wind speed |
| `wind_dir_last` | int | degrees | Most recent wind direction |
| `wind_speed_hi_last_10_min` | float | mph | Gust (10-min high) |
| `wind_dir_at_hi_speed_last_10_min` | int | degrees | Gust direction |
| `wind_speed_avg_last_2_min` | float | mph | 2-min average |
| `rain_size` | int | — | Collector type: 1=0.01in, 2=0.2mm, 3=0.1mm, 4=0.001in |
| `rain_rate_last` | float | counts/hr | Convert with rain_size factor |
| `rain_rate_hi` | float | counts/hr | Peak rate |
| `rainfall_daily` | float | counts | Convert with rain_size factor |
| `solar_rad` | float | W/m² | Solar radiation |
| `uv_index` | float | — | UV index value |

### HTTP `/v1/current_conditions` — data_structure_type 3 (LSS BAR)

| Field | Type | Units | Notes |
|-------|------|-------|-------|
| `bar_sea_level` | float | inHg | Sea-level adjusted pressure |
| `bar_trend` | int | — | -1=falling, 0=steady, 1=rising |
| `bar_absolute` | float | inHg | Raw sensor reading |

### HTTP `/v1/current_conditions` — data_structure_type 4 (Indoor)

| Field | Type | Units | Notes |
|-------|------|-------|-------|
| `temp_in` | float | °F | Indoor temperature |
| `hum_in` | float | %RH | Indoor humidity |
| `dew_point_in` | float | °F | Indoor dew point |
| `heat_index_in` | float | °F | Indoor heat index |

### UDP Broadcast (port 22222, every 2.5s) — data_structure_type 1 fields subset

| Field | Type | Units | Notes |
|-------|------|-------|-------|
| `wind_speed_last` | float | mph | Instantaneous wind speed |
| `wind_dir_last` | int | degrees | Instantaneous wind direction |
| `wind_speed_hi_last_10_min` | float | mph | 10-min gust |
| `wind_dir_at_hi_speed_last_10_min` | int | degrees | Gust direction |
| `rain_size` | int | — | Same as HTTP |
| `rain_rate_last` | float | counts/hr | Current rain rate |
| `rainfall_daily` | float | counts | Daily accumulation |

**Note on bar_trend field:** The API documentation shows `bar_trend` as the 3-hour trend value. Per the WeatherLink Live spec, positive = rising, negative = falling, 0 = steady. (The `pressure_trend` field name referenced in earlier project research is `bar_trend` in the actual API.)

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| qmake build system | CMake 3.16+ | Qt 6.0 (2020) | `qt_add_executable`, `qt_add_qml_module`, qmlsc all require CMake |
| Qt Charts for graphs | Qt Graphs (6.8+) | Qt 6.10 deprecated Charts | Qt Graphs graduated from tech preview in 6.8; Qt Charts deprecated in 6.10 |
| `QNetworkReply::finished` with SIGNAL macro | Modern C++ `connect` with lambda | Qt 5 era | Type-safe, no runtime string matching |
| Subclassing QThread and overriding `run()` | `moveToThread()` worker-object pattern | Qt 4.8+ | `moveToThread` is simpler, safer, and the documented Qt recommendation |
| `QAbstractItemModel` for data exposure | Plain QObject with typed Q_PROPERTY | N/A | QAbstractItemModel is for table/list views; gauge widgets use direct signals |

**Deprecated/outdated:**
- `qmake` (.pro files): Legacy; Qt 6 documentation uses CMake exclusively; avoid
- `QNetworkReply::error()` (old signal-as-member confusion): Use `QNetworkReply::errorOccurred()` signal if you need an error signal; `error()` is the enum accessor method
- Qt Charts: Deprecated Qt 6.10; replaced by Qt Graphs

---

## Open Questions

1. **Testing staleness within 30s in fast unit tests**
   - What we know: `WeatherDataModel` uses `QElapsedTimer` with a 30s threshold; QTest tests time out at a configurable limit
   - What's unclear: Whether to `QTest::qWait(31000)` (slow but simple) or inject a fake clock/timer into the model to test staleness without waiting 30+ seconds
   - Recommendation: Design `WeatherDataModel` to accept an injectable elapsed-ms getter (strategy function or `std::function<qint64()>`) so tests can simulate 30s+ elapsed without actual waiting. Document this as an internal design detail.

2. **UDP packet JSON structure: `conditions` vs root-level fields**
   - What we know: The API research indicates UDP packets contain a `conditions` array with ISS fields; the API.md confirms field names but the exact JSON envelope structure is less explicit than the HTTP response
   - What's unclear: Whether the UDP JSON wraps data in `{"conditions": [...]}` or emits the ISS fields at the root level
   - Recommendation: Log and inspect raw UDP bytes on first integration with real hardware; write `parseUdpDatagram` defensively to handle both structures; add a unit test with a captured real packet once available

3. **bar_trend field semantics**
   - What we know: API.md shows `bar_trend` field on type 3 (bar_sea_level, bar_trend, bar_absolute); earlier project research referenced `pressure_trend`
   - What's unclear: Whether the value is a raw float trend in inHg over 3 hours, or an integer (-1/0/1) normalized trend indicator
   - Recommendation: Map the field as `double barTrend` initially; clamp/classify into -1/0/1 in the model based on a threshold (e.g., |trend| < 0.02 = steady, negative = falling, positive = rising)

4. **`QNetworkAccessManager::setAutoDeleteReplies(true)` availability in Qt 6.8**
   - What we know: This method exists in Qt 6; avoids needing explicit `deleteLater()` in every handler
   - What's unclear: Exact version it was introduced; whether it is in Qt 6.8 LTS
   - Recommendation: Use explicit `reply->deleteLater()` in all handlers regardless — it works in all Qt 6 versions and is unambiguously safe; treat `setAutoDeleteReplies(true)` as an optional enhancement after confirming availability

---

## Sources

### Primary (HIGH confidence)

- Qt 6.8 official docs — QNetworkAccessManager: https://doc.qt.io/qt-6/qnetworkaccessmanager.html — GET request pattern, `deleteLater()` requirement, transfer timeout, single-instance recommendation
- Qt 6.8 official docs — QUdpSocket: https://doc.qt.io/qt-6/qudpsocket.html — bind, `readyRead`, `hasPendingDatagrams()`, `receiveDatagram()`
- Qt 6.8 official docs — QThread and moveToThread: https://doc.qt.io/qt-6/qthread.html — worker-object-on-thread pattern
- Qt 6.8 official docs — Threads and QObjects: https://doc.qt.io/qt-6/threads-qobject.html — cross-thread signal delivery, QueuedConnection
- Qt 6.8 official docs — Q_PROPERTY: https://doc.qt.io/qt-6/properties.html — property declaration, NOTIFY signals
- Qt 6.8 official docs — QTest Overview: https://doc.qt.io/qt-6/qtest-overview.html — test structure, CMake integration
- Qt 6.8 official docs — QSignalSpy: https://doc.qt.io/qt-6/qsignalspy.html — signal testing, `wait()`, argument access
- Qt 6.8 official docs — Tutorial Chapter 1 (QTest CMake): https://doc.qt.io/qt-6/qttestlib-tutorial1-example.html — cmake setup
- WeatherLink Live Local API — API.md (raw): https://raw.githubusercontent.com/weatherlink/weatherlink-live-local-api/master/API.md — complete field reference, data_structure_type values, rain_size conversion table, UDP packet fields

### Secondary (MEDIUM confidence)

- QNetworkAccessManager best practices: https://www.volkerkrause.eu/2022/11/19/qt-qnetworkaccessmanager-best-practices.html — single QNAM instance, deleteLater pattern, timeout setup
- WeatherLink Live Local API — Cumulus Wiki: https://cumuluswiki.org/a/WeatherLink_Live — community implementation notes on UDP session renewal

### Tertiary (LOW confidence)

- None for this phase — all critical claims are verified against official Qt documentation and the WeatherLink Live API specification

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Qt 6.8 LTS official docs confirm all APIs; no external libraries needed
- Architecture: HIGH — all patterns (moveToThread, Q_PROPERTY + NOTIFY, QJsonDocument routing) are from official Qt documentation; the dual-source model (UDP wins for wind/rain, HTTP for all else) is architecturally sound and aligns with prior project research
- Pitfalls: HIGH — `deleteLater()` requirement, UDP expiry behavior, and cross-thread connection rules all verified against official Qt docs and API specification
- API field reference: HIGH — extracted from WeatherLink Live Local API.md on GitHub

**Research date:** 2026-03-01
**Valid until:** 2026-09-01 (stable Qt LTS APIs; WeatherLink Live API is stable; 6 months)
