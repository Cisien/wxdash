# Stack Research

**Domain:** Qt 6 C++ real-time weather dashboard kiosk on Raspberry Pi
**Researched:** 2026-03-01
**Confidence:** MEDIUM (Qt 6 core — HIGH; Pi-specific deployment — MEDIUM; charting — MEDIUM)

---

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Qt 6 (qtbase) | 6.8.x LTS | Application framework, event loop, networking, JSON | Only LTS with 5-year support (until Oct 2029); Mesa V3D driver support confirmed; the 6.10 non-LTS is not worth the instability risk on Pi |
| Qt Quick / QML | 6.8.x LTS | UI rendering layer | GPU-accelerated scene graph via Mesa V3D on Pi 4/5; smooth gauge animations at 2.5s intervals without CPU rasterization overhead; Widgets use CPU-only rasterization — wrong choice for animated gauges |
| Qt Quick Controls 2 | 6.8.x LTS | Base control set (used minimally — mostly custom components) | Provides ApplicationWindow, mouse event handling, and theme baseline; the actual gauges, compass, and sparklines will be custom Canvas/Shape items |
| Qt Network | 6.8.x LTS | HTTP polling + UDP receive | QNetworkAccessManager for HTTP current_conditions polling; QUdpSocket for 22222/UDP real-time wind/rain; both are async/event-driven with Qt signals — zero blocking |
| CMake | 3.22+ | Build system | Qt 6 officially dropped qmake as the canonical build tool; all official Qt 6 documentation uses CMake; required minimum is CMake 3.22 for Qt 6.9+, 3.16 for Qt 6.8 |
| C++17 | — | Language standard | Qt 6 requires C++17 minimum; use it throughout — structured bindings, std::optional for QRestReply JSON, if-constexpr |
| Ninja | latest | CMake build backend | Qt's own CMake documentation specifies Ninja; significantly faster incremental builds than Make on Pi |

### Qt Modules Required

| Module | CMake Target | Purpose | Notes |
|--------|-------------|---------|-------|
| Qt6::Core | always linked | QObject, signals/slots, timers, QJsonDocument, QString | JSON parsing lives here — no external library needed |
| Qt6::Network | Qt6::Network | QNetworkAccessManager, QUdpSocket, QNetworkReply | HTTP + UDP transport layer |
| Qt6::Quick | Qt6::Quick | QML scene graph, Canvas, Shape, animations | Primary UI rendering engine |
| Qt6::QuickControls2 | Qt6::QuickControls2 | ApplicationWindow, basic controls | Minimal use — only for window/application shell |
| Qt6::Qml | Qt6::Qml | QML engine, C++ type registration | Bridges C++ data model to QML |
| Qt6::Graphs | Qt6::Graphs | 2D sparkline/line charts (optional) | Graduated out of tech preview in 6.8; Qt Charts deprecated in 6.10; use for sparklines OR use custom Canvas |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Avahi (libavahi-client) | system | mDNS resolution of `weatherlinklive.local.cisien.com` | Required on Pi; Linux does not resolve `.local` mDNS by default without either avahi-daemon or systemd-resolved with mDNS enabled; Qt's QDnsLookup does NOT do mDNS — you need the OS to handle it |
| nlohmann/json | 3.11.x | Optional secondary JSON parser | Do NOT use for this project — Qt's QJsonDocument handles the WeatherLink JSON responses natively and avoids an extra dependency; only add if JSON parsing becomes a bottleneck (unlikely at 10s poll intervals) |
| SQLite (via Qt6::Sql) | system SQLite | Sparkline historical data ring buffer | Qt6::Sql + SQLite for persisting ~2 hours of readings; simple FIFO table with timestamp + sensor columns; do NOT use QML LocalStorage (QQmlEngine::offlineStoragePath) — access it from C++ side for control |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Qt Creator | IDE with QML designer, CMake integration, deploy-to-Pi | Best IDE for Qt QML work; version 13+ supports Qt 6.8; not mandatory but speeds QML layout iteration |
| qmllint | QML static analysis | Ships with Qt; run as part of CMake via `qt_add_qml_module`; catches binding loops and type errors before runtime |
| qmlsc | QML script compiler | Ships with Qt 6.8; ahead-of-time compiles QML to C++; significantly reduces startup time and runtime overhead on Pi |
| GDB + gdbserver | Remote debugging on Pi | Cross-debug from host to Pi over SSH; essential during sensor integration |
| clang-tidy / clang-format | Static analysis + formatting | Use with Qt's `.clang-tidy` presets; catches common Qt signal/slot mistakes |

---

## Qt Module Configuration (CMakeLists.txt skeleton)

```cmake
cmake_minimum_required(VERSION 3.22)
project(wxdash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Network
    Qml
    Quick
    QuickControls2
    Sql
    # Graphs  # add if using Qt Graphs for sparklines
)

qt_add_executable(wxdash
    main.cpp
    src/weatherclient.cpp
    src/weathermodel.cpp
)

qt_add_qml_module(wxdash
    URI wxdash
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/GaugeWidget.qml
        qml/CompassRose.qml
        qml/SparklineChart.qml
)

target_link_libraries(wxdash PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
)
```

---

## Raspberry Pi Deployment

### Display Backend Decision

**Use EGLFS with DRM/KMS backend.** This is the correct choice for a kiosk with no window manager.

| Backend | Use When | Notes |
|---------|----------|-------|
| EGLFS (eglfs_kms) | Kiosk, no compositor needed | Direct framebuffer via DRM/KMS + Mesa V3D; Qt forces first QQuickWindow fullscreen automatically; **recommended for this project** |
| Wayland (wayland) | If Pi runs labwc/wayfire/weston | Requires compositor process; adds ~30-50MB RAM overhead; unnecessary for single-app kiosk |
| LinuxFB | Framebuffer fallback, no GPU | Software rasterization only; do not use — poor performance for animated gauges |
| XCB (X11) | Desktop development only | Not for embedded kiosk |

**Critical Pi 4/5 driver note (HIGH confidence):** Qt 6.8 dropped support for Broadcom proprietary EGL blobs. You MUST use Mesa V3D drivers (vc4-v3d for Pi 4, v3d for Pi 5). Confirm `gpu_mem=128` or more in `/boot/config.txt` and that DRM/KMS overlay is active (`dtoverlay=vc4-kms-v3d`).

**Launch command for kiosk:**
```bash
QT_QPA_PLATFORM=eglfs \
QT_QPA_EGLFS_KMS_CONFIG=/etc/wxdash-kms.json \
./wxdash
```

**KMS config for 720p output (`/etc/wxdash-kms.json`):**
```json
{
  "device": "/dev/dri/card1",
  "outputs": [
    { "name": "HDMI1", "mode": "1280x720" }
  ]
}
```

### mDNS Resolution

The WeatherLink Live hostname `weatherlinklive.local.cisien.com` requires mDNS. On Raspberry Pi OS Bookworm:

```bash
# Option A: Use avahi-daemon (most reliable for .local)
sudo apt install avahi-daemon

# Option B: Enable mDNS in systemd-resolved
# Edit /etc/systemd/resolved.conf: MulticastDNS=yes
# Then: sudo systemctl restart systemd-resolved
```

Qt's `QNetworkAccessManager` will use the OS resolver once one of the above is active. No Qt-level mDNS library is needed for this project since you just need hostname resolution (not service discovery).

### Build Approach: Build on Pi vs Cross-Compile

**Recommendation: Build directly on Pi 4/5 (native compilation) for this project.**

| Approach | Pro | Con |
|----------|-----|-----|
| Native on Pi | Simple toolchain, no sysroot complexity, easy debug | Slower compile (~10-20 min for Qt app on Pi 4); fine for a single app |
| Cross-compile on x86_64 | Fast compile (~2 min) | Sysroot management is fragile; toolchain version mismatches cause subtle bugs; documented extensively but error-prone in practice |

For a single-developer kiosk project, cross-compilation complexity is not worth it. Build Qt 6.8 from source on the Pi following the TAL.org guide, then native-compile the application.

**Pi packages required before Qt build:**

```bash
sudo apt update && sudo apt install -y \
  build-essential cmake ninja-build git \
  libfontconfig1-dev libfreetype6-dev libharfbuzz-dev \
  libdbus-1-dev libicu-dev libinput-dev libxkbcommon-dev \
  libsqlite3-dev libssl-dev libpng-dev libjpeg-dev \
  libglib2.0-dev libdrm-dev libgbm-dev \
  libegl1-mesa-dev libgles2-mesa-dev
```

---

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| UI Technology | Qt Quick / QML | Qt Widgets | Widgets use CPU rasterization — animated gauge needle at 2.5s intervals will tear or stutter on Pi; QML uses GPU scene graph via Mesa V3D |
| UI Technology | Qt Quick / QML | Dear ImGui | No data binding, no property animation system; would require manual redraw loop; better for game UIs than dashboards |
| Charting | Custom QML Canvas or Qt Graphs | QCustomPlot | QCustomPlot is GPL (viral); last confirmed Qt 6 support is 6.4; no QML integration — Widgets-based only |
| Charting | Custom QML Canvas or Qt Graphs | Qt Charts | Deprecated as of Qt 6.10; being replaced by Qt Graphs; avoid starting new projects with it |
| Charting | Custom QML Canvas | Qt Graphs | Qt Graphs requires Qt Quick Scene Graph (good) but adds a full 2D/3D graph engine dependency for what are 3-4 sparklines; custom QML Canvas sparkline is ~100 LOC and zero dependency |
| JSON Parsing | Qt's QJsonDocument | nlohmann/json | QJsonDocument is built-in and handles WeatherLink's response structure; nlohmann/json is header-only but adds a CMake FetchContent dependency for no benefit at 10s polling intervals |
| Display Backend | EGLFS (eglfs_kms) | Wayland + Weston | Wayland requires a compositor process; adds RAM overhead; unnecessary for single full-screen Qt app kiosk |
| Build System | CMake | qmake | qmake is legacy; Qt 6's own build uses CMake; qt_add_qml_module and qmlsc integration require CMake |
| Data Language | C++ | Python/PySide6 | Pi has limited RAM/CPU; Python GIL and interpreter overhead are real at 2.5s UDP burst processing; C++ gives deterministic latency |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Qt Widgets for gauges | CPU rasterization; no property animation; will cause frame drops on Pi's GPU-capable but CPU-limited SoC | Qt Quick Canvas or Shape items with GPU rendering |
| Qt Charts | Deprecated in Qt 6.10; not worth starting new projects | Qt Graphs (Qt 6.8+) or custom QML Canvas sparkline |
| QCustomPlot | GPL license (viral); last known Qt 6 support is 6.4; Widgets-only, no QML | Custom QML Canvas; 3-4 sparklines don't need a full plotting engine |
| Broadcom proprietary EGL blobs | Dropped in Qt 6.8; will fail to configure | Mesa V3D driver (dtoverlay=vc4-kms-v3d) |
| Qt 6.10 or 6.9 | Non-LTS; fewer pre-built Pi community guides; support ends sooner | Qt 6.8 LTS (supported to Oct 2029) |
| X11 / XCB on Pi kiosk | Unnecessary display server overhead; EGLFS goes direct to hardware | EGLFS with DRM/KMS |
| QTimer for UDP "heartbeat" keep-alive | The WeatherLink UDP broadcast requires periodic re-request via HTTP (`/v1/real_time?duration=N`); using a QTimer correctly for this is fine, but don't assume UDP is self-sustaining | QTimer calling HTTP re-request every N-30 seconds |
| nlohmann/json | Redundant dependency; Qt's QJsonDocument is built-in and sufficient | Qt6::Core QJsonDocument + QJsonObject |
| QML LocalStorage | QQmlEngine path is opaque; harder to query from C++; less control over schema | Qt6::Sql + SQLite with explicit schema for ring buffer |

---

## Stack Patterns by Variant

**If targeting Pi 4 (Cortex-A72, 2-4GB RAM):**
- Use `QT_QPA_EGLFS_KMS_CONFIG` to lock 1280x720 mode — avoids mode detection latency at startup
- Set `QSG_RENDER_LOOP=threaded` to keep UI responsive during HTTP polling
- Allocate `gpu_mem=128` in `/boot/config.txt`

**If targeting Pi 5 (Cortex-A76, 4-8GB RAM):**
- All of the above applies; Pi 5 uses a different DRM device path (`/dev/dri/card0` vs `card1` — verify at runtime)
- Pi 5 supports HDMI0 and HDMI1 on separate DRM planes; specify output explicitly in KMS config

**If sparklines need >2 hours of history:**
- Add Qt6::Sql and SQLite; use a rolling DELETE WHERE timestamp < (NOW - interval) pattern rather than a fixed-size ring table — simpler and safer
- If no persistence needed (display-only, no history across reboots): use a `std::deque<float>` in C++ exposed as a QML-accessible list model — zero overhead

**If build time on Pi is unacceptable (>20 min per full rebuild):**
- Set up cross-compilation using Docker + QEMU sysroot on x86_64 host; follow the interelectronix.com Qt 6.8 cross-compile guide
- Still use Qt 6.8 LTS as the target version

---

## Version Compatibility Matrix

| Package | Version | Compatible With | Notes |
|---------|---------|-----------------|-------|
| Qt 6.8 LTS | 6.8.x | CMake 3.16+ | Use 3.22+ for best experience; 6.8.3 released 2025-03-26 |
| Mesa / V3D | system (Bookworm) | Pi 4 + Pi 5 | Raspberry Pi OS Bookworm ships Mesa 23.x; adequate for OpenGL ES 3.1 |
| Qt Graphs | 6.8+ only | Qt Quick 6.8+ | Moved out of tech preview in 6.8; do not use with Qt 6.7 or earlier |
| avahi-daemon | system | All Pi OS | Installed by default on Pi OS Desktop; may not be on Lite — verify |
| SQLite | system | Qt6::Sql | Qt links against system libsqlite3; install `libsqlite3-dev` before Qt build |
| C++ standard | C++17 | Qt 6.8 | Qt 6 requires C++17; use `set(CMAKE_CXX_STANDARD 17)` |

---

## Sources

- [Qt 6.8 LTS Released (qt.io)](https://www.qt.io/blog/qt-6.8-released) — Version confirmation, LTS dates, Qt Graphs GA status — HIGH confidence
- [Building Qt 6.8 LTS for Raspberry Pi (tal.org)](https://www.tal.org/tutorials/building-qt-6-8-raspberry-pi) — Pi build process, Mesa V3D requirement, dropped Broadcom blobs — MEDIUM confidence
- [Qt for Embedded Linux (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/embedded-linux.html) — EGLFS, Wayland, LinuxFB platform plugin documentation — HIGH confidence
- [Qt Releases (doc.qt.io)](https://doc.qt.io/qt-6/qt-releases.html) — Qt 6.10 is latest non-LTS; 6.8 is current LTS — HIGH confidence
- [Build System Changes in Qt 6 (doc.qt.io)](https://doc.qt.io/qt-6/qt6-buildsystem.html) — CMake requirement, qmake legacy status — HIGH confidence
- [QNetworkAccessManager (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qnetworkaccessmanager.html) — HTTP async API — HIGH confidence
- [QUdpSocket (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qudpsocket.html) — UDP socket API — HIGH confidence
- [Qt Graphs (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qtgraphs-index.html) — GA in 6.8, replaces Qt Charts — HIGH confidence
- [Qt Charts deprecation note](https://doc.qt.io/qt-6/qtcharts-index.html) — "Deprecated since Qt 6.10, use Qt Graphs" — HIGH confidence
- [Cross-Compile Qt 6.8 for Raspberry Pi (interelectronix.com)](https://www.interelectronix.com/qt-68-cross-compilation-raspberry-pi.html) — Cross-compile approach details — MEDIUM confidence
- [WeatherLink Live Local API (weatherlink.github.io)](https://weatherlink.github.io/weatherlink-live-local-api/) — API structure: HTTP every 10s, UDP port 22222, re-request required — HIGH confidence
- [QCustomPlot license and Qt 6 support research](https://www.qcustomplot.com/) — GPL license confirmed, Qt 6.4 last known compatibility — MEDIUM confidence
- [RoniaKit (github.com/Roniasoft/RoniaKit)](https://github.com/Roniasoft/RoniaKit) — Open-source QML gauge components; viable alternative to custom Canvas — LOW confidence (unmaintained status unclear)
- [QtZeroConf (github.com/jbagg/QtZeroConf)](https://github.com/jbagg/QtZeroConf) — mDNS wrapper; not needed if OS avahi/systemd-resolved handles .local — LOW confidence

---

*Stack research for: wxdash — Qt 6 C++ real-time weather dashboard on Raspberry Pi kiosk*
*Researched: 2026-03-01*
