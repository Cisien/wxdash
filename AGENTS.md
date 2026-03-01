# wxdash — Project Conventions for Claude Code

## Project Overview

wxdash is a full-screen Qt 6 (C++) weather dashboard for displaying real-time data from a Davis
Instruments WeatherLink Live station. The app is designed to run as a kiosk on Linux (including
Raspberry Pi) with Qt's EGLFS backend.

## Stack

- **Language:** C++17
- **Framework:** Qt 6 (Qt6::Core, Qt6::Network, Qt6::Test; QML/Quick added in Phase 2)
- **Build system:** CMake (3.22+) with Ninja generator
- **Unit testing:** QTest (Qt6::Test)
- **Formatter:** clang-format (config in .clang-format)
- **Linter:** clang-tidy (config in .clang-tidy)

## Directory Layout

```
wxdash/
├── CMakeLists.txt                # Top-level: find Qt6, enable_testing, add subdirs
├── AGENTS.md                     # This file — project conventions
├── .clang-format                 # Code formatter config
├── .clang-tidy                   # Static analysis config
├── src/
│   ├── CMakeLists.txt            # Defines wxdash_lib (STATIC) and wxdash executable
│   ├── main.cpp                  # QCoreApplication entry point (Phase 1: no UI)
│   ├── models/
│   │   ├── WeatherReadings.h     # Plain C++ structs for cross-thread data transfer
│   │   └── WeatherDataModel.h    # QObject with Q_PROPERTY + NOTIFY signals (Phase 1+)
│   └── network/
│       ├── JsonParser.h/.cpp     # Stateless JSON parsing (QByteArray → structs)
│       ├── HttpPoller.h/.cpp     # QNetworkAccessManager + QTimer; 10s polling
│       └── UdpReceiver.h/.cpp    # QUdpSocket; UDP broadcast management
└── tests/
    ├── CMakeLists.txt            # Defines test executable targets
    └── tst_JsonParser.cpp        # Unit tests for JsonParser
```

## Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `WeatherDataModel`, `HttpPoller` |
| Methods | camelCase | `parseCurrentConditions()`, `rainSizeToInches()` |
| Member variables | `m_` prefix | `m_temperature`, `m_pollTimer` |
| Constants | `k` prefix or `kConstantName` | `kStalenessMs`, `kRenewalIntervalMs` |
| Namespaces | PascalCase | `JsonParser` |
| Files | PascalCase (matches class) | `WeatherReadings.h`, `JsonParser.cpp` |

## File Size Guideline

~500 lines per file. When a file is approaching 500 lines, break it into separate
classes/files. This keeps files reviewable and helps Claude stay within context.

## Architecture Principles

### Plain Structs for Cross-Thread Data Transfer

All data crossing thread boundaries uses plain C++ structs (no QObject inheritance),
defined in `WeatherReadings.h`. Register with `qRegisterMetaType<T>()` in `main()` for
use in queued connections.

```cpp
// Good — plain struct, safely queued
struct IssReading { double temperature = 0.0; /* ... */ };

// Bad — QObject cannot be safely queued across threads
class IssData : public QObject { /* ... */ };
```

### Worker Objects on a Shared QThread

`HttpPoller` and `UdpReceiver` are moved to a dedicated `QThread` via `moveToThread()`.
Connect signals/slots between threads without specifying connection type — Qt
automatically uses `QueuedConnection` for cross-thread connections.

### Q_PROPERTY with NOTIFY Signals

Every observable field in `WeatherDataModel` has a `Q_PROPERTY` with a `NOTIFY` signal.
Widgets connect to exactly the signal they care about — no polling, no god-object
coupling.

```cpp
Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
```

### Dependency Injection — No Singletons

Pass dependencies (model, QNAM) as constructor parameters. Never use global state,
`qApp->property()`, or static singletons.

## Critical Rules

### 1. Always call `reply->deleteLater()`

Every `QNetworkReply::finished` handler MUST call `reply->deleteLater()`. Missing this
is the most common Qt memory leak in polling applications.

```cpp
void HttpPoller::onReply() {
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    // ... process reply ...
    reply->deleteLater();  // MANDATORY — always, even on error
}
```

### 2. Never assume array index for `data_structure_type`

The WeatherLink Live API may return sensor structures in any order. Always iterate the
conditions array and check the `data_structure_type` field:

```cpp
// Good
for (const auto &val : conditionsArray) {
    int type = val.toObject()["data_structure_type"].toInt();
    if (type == 1) { /* ISS */ }
    else if (type == 3) { /* barometer */ }
    else if (type == 4) { /* indoor */ }
}

// Bad — assumes index 0 is always ISS
auto issObj = conditionsArray[0].toObject();
```

### 3. Silently ignore malformed JSON

Per locked decision: if JSON is malformed or fields are missing, return empty/nullopt
and continue. Do not throw, do not log errors, do not retry — the next poll is the
retry.

### 4. Rain counts require unit conversion

Rain fields from the API are raw bucket-tip counts. Always apply the `rain_size` factor:

```
rain_size 1 → 0.01 inches per count
rain_size 2 → 0.2/25.4 inches per count  (0.2mm)
rain_size 3 → 0.1/25.4 inches per count  (0.1mm)
rain_size 4 → 0.001 inches per count
```

Use `JsonParser::rainSizeToInches(rain_size)` to get the multiplier.

## Build Commands

```bash
# Configure (first time or after CMakeLists changes)
cmake -B build -G Ninja

# Build
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Build and test together
cmake --build build && ctest --test-dir build --output-on-failure
```

## Network Architecture

### HTTP Polling (DATA-01)
- `QNetworkAccessManager` owned by `HttpPoller` (one QNAM per app)
- `QTimer` fires every 10s to trigger polls
- If a reply is still in-flight when the timer fires, abort the previous reply first
- Transfer timeout: 5000ms on each request

### UDP Real-Time (DATA-02, DATA-03)
- `QUdpSocket` bound to `0.0.0.0:22222`
- Broadcast started by HTTP GET to `/v1/real_time?duration=86400`
- Renewal timer fires every 3600s (1h) to re-issue the start request
- Health check timer (every 5s) detects silence >10s and triggers immediate re-registration

### Staleness Detection (DATA-08)
- Uniform 30s threshold for all sources
- When stale: clear all values (no last-known display)
- Track per device (WeatherLink Live as one source), not per sensor

## API Reference

**Base URL:** `http://weatherlinklive.local.cisien.com`

**HTTP endpoint:** `GET /v1/current_conditions`

Response structure:
```json
{
  "data": {
    "conditions": [
      { "data_structure_type": 1, /* ISS fields */ },
      { "data_structure_type": 3, /* barometer fields */ },
      { "data_structure_type": 4, /* indoor fields */ }
    ]
  }
}
```

**UDP endpoint:** `GET /v1/real_time?duration=86400` (starts broadcast on port 22222)

UDP datagram structure:
```json
{
  "conditions": [
    { "data_structure_type": 1, /* wind/rain fields */ }
  ]
}
```

### ISS Fields (type 1)
| API Field | Struct Field | Unit | Notes |
|-----------|-------------|------|-------|
| `temp` | `temperature` | °F | |
| `hum` | `humidity` | %RH | |
| `dew_point` | `dewPoint` | °F | |
| `heat_index` | `heatIndex` | °F | |
| `wind_chill` | `windChill` | °F | |
| `wind_speed_last` | `windSpeedLast` | mph | |
| `wind_dir_last` | `windDirLast` | degrees | |
| `wind_speed_hi_last_10_min` | `windSpeedHi10` | mph | gust |
| `solar_rad` | `solarRad` | W/m² | |
| `uv_index` | `uvIndex` | — | |
| `rain_size` | `rainSize` | — | 1/2/3/4 |
| `rain_rate_last` | `rainRateLast` | counts/hr | multiply by rain_size factor |
| `rainfall_daily` | `rainfallDaily` | counts | multiply by rain_size factor |

### Barometer Fields (type 3)
| API Field | Struct Field | Unit |
|-----------|-------------|------|
| `bar_sea_level` | `pressureSeaLevel` | inHg |
| `bar_trend` | `pressureTrend` | — |

### Indoor Fields (type 4)
| API Field | Struct Field | Unit |
|-----------|-------------|------|
| `temp_in` | `tempIn` | °F |
| `hum_in` | `humIn` | %RH |
| `dew_point_in` | `dewPointIn` | °F |
