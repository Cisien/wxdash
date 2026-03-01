# Project Research Summary

**Project:** wxdash — Qt 6 C++ Real-Time Weather Dashboard Kiosk
**Domain:** Embedded kiosk display / IoT sensor data visualization (Raspberry Pi + Davis WeatherLink Live)
**Researched:** 2026-03-01
**Confidence:** MEDIUM-HIGH (stack and architecture HIGH from official Qt docs; Pi-specific GPU behavior MEDIUM from community consensus; dual-cadence aggregation LOW — no direct examples found)

---

## Executive Summary

wxdash is a Qt 6 C++ kiosk application that displays live weather data from a Davis WeatherLink Live station on a Raspberry Pi connected to an HDMI display. The domain is well-understood — real-time sensor dashboards are a canonical Qt embedded use case — but the specific combination of dual-cadence data ingestion (HTTP every 10 seconds + UDP every 2.5 seconds), EGLFS/DRM rendering without a compositor, and indefinite unattended operation on constrained hardware creates a cluster of integration hazards that are individually documented but rarely addressed together. Research finds strong consensus on all major technology choices, with Qt 6.8 LTS + QML scene graph + Mesa V3D on EGLFS as the proven path.

The recommended approach is a layered architecture: an isolated network layer (HttpPoller + UdpReceiver on a dedicated QThread), a central WeatherDataModel as single source of truth, and GPU-accelerated QML/Canvas widgets consuming model signals. The dual-cadence update path — UDP wins for wind/rain freshness, HTTP updates everything else and feeds sparkline history — must be designed from the start, not patched in later. Kiosk requirements (watchdog, staleness detection, burn-in mitigation, network recovery) are low-complexity but must be included before deployment.

The primary risk is rendering path selection: Qt Widgets with QPainter may silently fall back to CPU software rasterization on Pi's EGLFS, causing thermal throttling within hours. This must be validated on target hardware before any gauge code is written. Secondary risks are operational — UDP session expiry causes silent data freeze, QNetworkReply leaks cause eventual OOM, and mDNS startup failures must be handled gracefully. All of these are solvable with known patterns, but require deliberate inclusion in Phase 1 and 2 implementation.

---

## Key Findings

### Recommended Stack

Qt 6.8 LTS is the unambiguous choice: it is the current long-term support release (supported through October 2029), Qt Graphs graduated from tech preview in 6.8 making it safe for sparkline charting, and the 6.10 non-LTS introduces no meaningful advantages for this use case. The application is C++17 + CMake 3.16+ (3.22+ recommended), targeting EGLFS with the `eglfs_kms` DRM/KMS backend on Mesa V3D drivers. Qt's built-in modules cover the full stack: QNetworkAccessManager for HTTP, QUdpSocket for UDP, QJsonDocument for API parsing, Qt6::Sql + SQLite for optional persistent sparkline history. No external JSON or charting libraries are needed and none should be added.

Native compilation on the Pi is recommended over cross-compilation for a single-developer project. Cross-compilation sysroot symlink corruption is a well-documented time sink; Pi 5 native builds are fast enough. Build Qt 6.8 from source on the Pi following the TAL.org guide, then build the application natively. Set `gpu_mem=128` and `dtoverlay=vc4-kms-v3d` before first launch.

**Core technologies:**
- **Qt 6.8 LTS**: Application framework, event loop, networking, JSON — LTS until Oct 2029; only LTS with confirmed Mesa V3D support
- **Qt Quick / QML**: Primary UI rendering — GPU-accelerated scene graph via Mesa V3D; Widgets rejected because they use CPU-only rasterization
- **Qt Network (QNetworkAccessManager + QUdpSocket)**: HTTP polling (10s) and UDP real-time feed (2.5s) — fully async, event-driven, zero blocking
- **EGLFS with eglfs_kms**: Kiosk display backend — direct DRM/KMS framebuffer, no compositor overhead; mandatory for Pi 4/5 (Pi 1-3's `eglfs_brcm` backend is incompatible)
- **CMake 3.16+**: Build system — Qt 6 officially dropped qmake; `qt_add_qml_module` and qmlsc require CMake
- **C++17**: Language standard — Qt 6 minimum requirement; structured bindings, std::optional for reply handling
- **Qt6::Sql + SQLite**: Optional sparkline persistence — use explicit C++ schema, not QML LocalStorage

**What NOT to use:**
- Qt Widgets for gauge rendering (CPU rasterization causes thermal throttle on Pi)
- Qt Charts (deprecated as of Qt 6.10; replaced by Qt Graphs)
- QCustomPlot (GPL license; last Qt 6 support at 6.4; Widgets-only, no QML)
- Broadcom proprietary EGL blobs (dropped in Qt 6.8; use Mesa V3D)
- Cross-compilation unless native builds exceed 40 minutes (Pi 5 is fast enough)

### Expected Features

The WeatherLink Live local API delivers all sensor data needed for a complete weather display. Table stakes features are all available as direct API fields — no external services required. The UDP broadcast at port 22222 gives genuine 2.5-second wind/rain refresh, a meaningful differentiator over dashboards that poll at 30-60 second intervals. Rain accumulation requires a rain_size-aware count-to-inches conversion that must be implemented correctly from the start.

**Must have (table stakes — v1):**
- Current outdoor temperature, humidity, dew point, feels-like (heat index/wind chill per NWS rules)
- Barometric pressure + trend arrow (API provides `pressure_trend` field directly: -1/0/1)
- Wind speed + compass rose direction + gust — UDP-driven at 2.5s refresh
- Rain rate + daily accumulation — with rain_size-corrected conversion (counts to inches)
- UV Index with EPA 5-zone color coding (0-2 green, 3-5 yellow, 6-7 orange, 8-10 red, 11+ violet)
- Solar radiation numeric display
- Staleness/connection indicator — visually flag data older than 30 seconds
- Full-screen kiosk mode — Qt frameless, no chrome
- Last updated timestamp

**Should have (competitive — v1.x, after core validated):**
- Sparkline trend graphs per sensor (6h ring buffer at 10s cadence = 2160 points per channel)
- Indoor temperature + humidity panel (API type 4 data structure; low effort)
- Animated gauge needles (only if Pi GPU budget allows smooth animation)
- Pixel-shift burn-in prevention (before any long-term deployment)
- Watchdog / auto-restart via systemd Restart=always

**Defer (v2+):**
- Configurable threshold values via config file
- Wind gust dedicated compass ring (MVP shows both as numbers)
- Screen-off schedule for nighttime dimming

**Anti-features — explicitly out of scope:**
- Metric unit toggle (API is imperial-only; conversion doubles QA surface for no gain)
- Touch-driven interaction (kiosk is display-only; touch introduces settings drift)
- Weather forecast display (requires external API, contradicts local-only design)
- Multi-station support (multiplies API complexity; one station, done well)

### Architecture Approach

The architecture follows a strict three-layer separation: a data source layer (HttpPoller + UdpReceiver) on a dedicated QThread, a central WeatherDataModel QObject on the GUI thread, and presentation-layer widgets that know nothing about networking. The model acts as the single source of truth, maintaining per-field typed signals and in-memory ring buffers for sparklines. The critical design decision is the dual-cadence update path: UDP updates win for wind/rain (fresher, 2.5s), while HTTP updates populate all fields and feed sparkline history buffers (10s). Only the HTTP path appends to ring buffers.

**Major components:**
1. **HttpPoller** — QNetworkAccessManager + 10s QTimer on network thread; single QNAM instance for entire app; always calls `reply->deleteLater()`
2. **UdpReceiver** — QUdpSocket on network thread; manages keep-alive by re-POSTing `/v1/real_time?duration=86400` before expiry; tracks last-packet timestamp to detect silent expiry
3. **JsonParser** — Parses QJsonDocument; routes by `data_structure_type` (1=ISS outdoor, 3=LSS BAR pressure, 4=LSS Temp/Hum indoor); applies rain_size conversion
4. **WeatherDataModel** — Central QObject; per-field typed signals; ring buffers for sparklines; `applyUdpUpdate()` and `applyHttpUpdate()` as separate slots
5. **GaugeWidget / CompassWidget / SparklineWidget** — Custom QML Canvas items or QWidget subclasses; receive value via model signals; call `update()` (never `repaint()`); connect only to their specific model signal
6. **DashboardWindow** — Root window; sets up fullscreen EGLFS mode; assembles all widgets; handles resize for responsive scaling
7. **ThresholdConfig** — Plain C++ struct; maps field names to (warn, critical, color) triples; loaded at startup

**Build order derived from dependencies:**
WeatherReadings.h (plain structs) → JsonParser → WeatherDataModel → HttpPoller + UdpReceiver → individual widgets → DashboardWindow → main.cpp

### Critical Pitfalls

1. **Wrong EGLFS backend (`eglfs_brcm` on Pi 4/5)** — Results in black screen or "Could not find DRM device!" Pi 1-3 tutorials describe `eglfs_brcm`; Pi 4/5 requires `eglfs_kms`. Validate GPU is actually being used (GPU load > 0%) before writing any gauge code. Add user to `video` group; set `QT_QPA_EGLFS_KMS_CONFIG` JSON explicitly. This must be verified in Phase 0/1.

2. **QPainter software rasterization on Pi** — Qt Widgets' QPainter path may silently bypass GPU even under EGLFS, causing 30-80% CPU load and thermal throttling. Use QOpenGLWidget or QRhiWidget (Qt 6.6+) as gauge base, or commit to QML Canvas which uses the GPU scene graph. Profile on target hardware before building all gauges. Recovery requires rewriting all widget base classes — expensive if caught late.

3. **QNetworkReply memory leak** — Every `get()` call must have `reply->deleteLater()` in the `finished()` handler, always. At 10s polling, forgetting this leaks 360 objects/hour; 8,640/day. On a Pi this causes OOM after days. Write a 1,000-poll leak test in Phase 2 before proceeding.

4. **UDP session expiry (silent data freeze)** — The WeatherLink Live UDP broadcast expires after the requested duration. Default is 20 minutes; max is 86,400 seconds. Wind/rain values freeze silently with no error. Set `duration=86400` at startup, track last-packet timestamp, and trigger re-registration if no packet arrives within 10 seconds. This must be in the initial UDP implementation.

5. **data_structure_type parsing** — The API response contains an array of sensor structures; ISS is NOT guaranteed to be at index 0. Parse all entries and route by `data_structure_type` field (1, 3, 4). Hardcoding `data_structures[0]` as ISS will silently produce wrong data when structure order changes.

---

## Implications for Roadmap

Based on combined research findings, the build order is driven by three constraints: (1) hardware rendering validation must precede all UI work, (2) the network layer and data model must be stable before widgets are built, and (3) kiosk reliability features must be designed in, not added at the end.

### Phase 0: Build Environment and Hardware Validation

**Rationale:** Cross-compilation sysroot issues and wrong EGLFS backend are blocking pitfalls — they cannot be fixed after the fact without significant rework. Hardware validation is a prerequisite gating all subsequent work.

**Delivers:**
- Pi with Qt 6.8 LTS compiled natively or cross-compiled correctly
- Hello-world QML app running fullscreen via EGLFS eglfs_kms backend
- Confirmed GPU rendering (vcgencmd / htop shows GPU active, CPU < 10%)
- mDNS resolution working for `weatherlinklive.local.cisien.com`
- systemd service unit launching app at boot

**Avoids:** Pitfall 1 (wrong EGLFS backend), Pitfall 5 (cross-compilation sysroot corruption)

**Research flag:** SKIP deep research — patterns are well-documented in Qt official docs and TAL.org Pi guide. Follow the checklist.

---

### Phase 1: Core Data Model and Network Layer

**Rationale:** Architecture research is explicit that the model's API (header files) must exist before both widgets and data sources can be developed. Network layer correctness is the highest-risk functional area; building it first allows extended testing before UI complicates debugging.

**Delivers:**
- `WeatherReadings.h` plain C++ structs (no Qt deps; cross-thread safe)
- `JsonParser` with full data_structure_type routing (type 1/3/4) and rain_size conversion
- `WeatherDataModel` with per-field typed signals and ring buffers for sparklines
- `HttpPoller` on dedicated QThread with single QNAM instance, explicit deleteLater(), 5s request timeout, exponential backoff retry
- `UdpReceiver` on network thread with duration=86400 kick-off, last-packet watchdog, re-registration on silence
- Staleness detection — data older than 30s triggers a "stale" signal to UI
- Network disconnect/reconnect state machine (Connecting → Live → Stale/Error → Reconnecting)
- Unit tests: JsonParser corner cases, rain_size conversion table, ring buffer correctness, 1,000-poll QNAM leak test

**Implements:** HttpPoller, UdpReceiver, JsonParser, WeatherDataModel (full architecture)

**Avoids:** Pitfall 3 (QNAM memory leak), Pitfall 4 (UDP session expiry), Pitfall 7 (mDNS startup failure recovery), Pitfall 8 (unbounded sparkline buffer), Pitfall 9 (multiple QNAM instances), Pitfall 10 (rain count conversion)

**Research flag:** SKIP — patterns are thoroughly documented in Qt official docs and WeatherLink Live API documentation.

---

### Phase 2: Gauge Widgets and Dashboard Layout

**Rationale:** With a stable model layer in place, widget development can proceed with real data. Rendering path (QML Canvas vs. QWidget subclass) must be benchmarked on target hardware at the start of this phase, before all gauges are built on the wrong foundation.

**Delivers:**
- Rendering path benchmark on Pi target: QML Canvas vs. QOpenGLWidget (pick one, document result)
- All v1 gauge widgets:
  - GaugeWidget: temperature, humidity, barometric pressure, UV Index (with EPA color zones), solar radiation, wind speed, rain rate
  - CompassWidget: 8-point compass rose with direction needle, gust + average readouts
  - StatusWidget: staleness indicator, connection state, last-updated timestamp
- DashboardWindow: fullscreen layout, responsive scaling on QResizeEvent
- ThresholdConfig: color zones for UV (EPA), temperature, humidity, wind, rain rate (all standards-based)
- Feels-like display: heat index or wind chill per NWS validity rules (API provides both pre-calculated)

**Addresses:** All v1 table-stakes features from FEATURES.md

**Avoids:** Pitfall 2 (software rasterization fallback — validate early in this phase)

**Research flag:** MAY NEED research specifically for QML Canvas gauge rendering patterns and performance on Pi. QRhiWidget (Qt 6.6+) is a newer API with less community documentation on Pi.

---

### Phase 3: Sparklines and Secondary Data

**Rationale:** Sparklines require accumulated ring buffer data to be meaningful. Indoor temperature/humidity is low-effort after the outdoor data path is established. These features are v1.x additions that benefit from the full data model being stable.

**Delivers:**
- SparklineWidget reading from model ring buffers (renders after >= 2 data points)
- Sparklines for: temperature, pressure, humidity, wind speed, UV (configurable subset)
- Indoor temperature + humidity secondary panel (type 4 data structure — already parsed in Phase 1)
- Animated gauge needle polish pass (if GPU budget allows; benchmark first)

**Addresses:** v1.x features from FEATURES.md (sparklines, indoor panel, animated gauges)

**Research flag:** SKIP — sparkline rendering via QML Canvas polyline is straightforward. Ring buffer design is already established in Phase 1.

---

### Phase 4: Kiosk Hardening and Deployment

**Rationale:** Reliability requirements for unattended 24/7 operation must be verified before any long-term installation. These are individually simple but collectively critical.

**Delivers:**
- systemd service: `Restart=always`, `WatchdogSec`, `WantedBy=graphical.target`
- Hardware watchdog: periodic write to `/dev/watchdog` (Pi-enabled by default)
- Pixel-shift burn-in prevention: ±2px shift on 10-minute timer (imperceptible to users)
- Screen-off schedule: `vcgencmd display_power 0` for overnight dimming (if applicable)
- DPMS disabled: prevent auto-blank
- 24-hour stability test: flat RSS memory, CPU < 20%, temperature < 75°C on Pi 4

**Addresses:** Kiosk-specific feature notes from FEATURES.md; Pitfall 6 (burn-in)

**Research flag:** SKIP — all patterns are standard Linux kiosk patterns, well-documented.

---

### Phase Ordering Rationale

- **Phase 0 before Phase 1:** Hardware rendering validation gates all UI work. Discovering the wrong EGLFS backend after widgets are built is expensive.
- **Phase 1 before Phase 2:** The model API must exist before widgets can be wired to it. Network correctness (leak test, UDP renewal) is easier to validate in isolation without UI complexity.
- **Phase 2 before Phase 3:** Sparklines need the ring buffers established in Phase 1 to accumulate data; Phase 3 can only be tested meaningfully once Phase 2's continuous polling is running.
- **Phase 4 last:** Hardening is about long-term operation; validate it on a working dashboard, not during development.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2 (gauge rendering):** QML Canvas performance on Pi with anti-aliased arcs is not extensively documented with benchmarks. QRhiWidget (Qt 6.6+) is newer; community Pi-specific examples are sparse. Plan to benchmark both QML Canvas and QOpenGLWidget/QRhiWidget at the start of Phase 2 before committing to one approach.

Phases with standard patterns (skip research-phase):
- **Phase 0:** Qt embedded Linux setup for Pi is thoroughly documented (Qt official docs + TAL.org). Follow the checklist.
- **Phase 1:** Qt signal/slot threading model, QNAM best practices, and WeatherLink Live UDP protocol are all documented exhaustively.
- **Phase 3:** QML Canvas polyline sparklines are a standard pattern.
- **Phase 4:** Linux kiosk systemd hardening is a standard pattern.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Qt 6.8 LTS choice backed by official Qt release docs, confirmed Pi Mesa V3D support, LTS date confirmed. EGLFS backend choice backed by official docs + community consensus. |
| Features | MEDIUM-HIGH | Table stakes drawn from multiple commercial products (Davis, AcuRite) and NWS/EPA official standards. Rain_size conversion confirmed from WeatherLink Live API docs. Feels-like validity rules from NWS. |
| Architecture | MEDIUM-HIGH | Qt core patterns (signals/slots, moveToThread, QPainter) are HIGH from official docs. Dual-cadence HTTP+UDP aggregation pattern is inferred — no direct prior art found. Pattern is sound but unvalidated in this specific combination. |
| Pitfalls | MEDIUM | Most pitfalls are verified from multiple community sources and Qt forums. Software rasterization fallback on Pi EGLFS is confirmed by forum benchmarks. UDP session expiry is from API docs. Cross-compilation sysroot is from Qt wiki + community experience. |

**Overall confidence:** MEDIUM-HIGH

### Gaps to Address

- **Dual-cadence model update path:** The specific pattern of UDP wins for wind/rain while HTTP feeds history buffers is architecturally sound but has no directly comparable open-source example found. Validate the WeatherDataModel's `applyUdpUpdate()` / `applyHttpUpdate()` split against real hardware behavior early in Phase 1.

- **QML Canvas vs. QOpenGLWidget rendering performance on Pi:** Research documents the risk of software rasterization fallback but does not have benchmark numbers specifically for QML Canvas at 2.5s refresh with anti-aliased arc gauges on Pi 4/5 with Mesa V3D. This is the highest technical uncertainty and must be resolved by benchmarking on target hardware in Phase 2.

- **mDNS hostname format `weatherlinklive.local.cisien.com`:** The `.local` mDNS domain with a `.cisien.com` suffix is an unusual pattern (typical mDNS is just `.local`). Verify that Avahi resolves this correctly on Raspberry Pi OS Bookworm, or that the actual hostname is simpler (e.g., just `weatherlinklive.local`). May need fallback to hard-coded IP.

- **Qt Graphs for sparklines:** Qt Graphs graduated from tech preview in 6.8 and is the official replacement for Qt Charts, but community examples for simple 2D line sparklines on embedded Pi are absent. Custom QML Canvas sparklines (~100 LOC) are lower risk and zero additional dependency.

---

## Sources

### Primary (HIGH confidence)
- [Qt 6.8 LTS Release Blog (qt.io)](https://www.qt.io/blog/qt-6.8-released) — LTS dates, Qt Graphs GA status
- [Qt for Embedded Linux (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/embedded-linux.html) — EGLFS, Wayland, LinuxFB platform plugin documentation
- [QNetworkAccessManager (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qnetworkaccessmanager.html) — HTTP async API, best practices
- [QUdpSocket (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qudpsocket.html) — UDP socket API
- [QThread / moveToThread (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qthread.html) — Worker-object-on-thread pattern
- [Qt Graphs (doc.qt.io/qt-6)](https://doc.qt.io/qt-6/qtgraphs-index.html) — GA in 6.8, replaces Qt Charts
- [WeatherLink Live Local API (weatherlink.github.io)](https://weatherlink.github.io/weatherlink-live-local-api/) — HTTP/UDP protocol, data_structure_type, rain_size, UDP duration/renewal
- [EPA UV Index Scale](https://www.epa.gov/sunsafety/uv-index-scale-0) — 5-zone color standard
- [NWS Heat Index](https://www.weather.gov/arx/heat_index) — Validity conditions for feels-like
- [NWS Wind Chill](https://www.weather.gov/epz/wxcalc_windchill) — Validity conditions for feels-like

### Secondary (MEDIUM confidence)
- [Building Qt 6.8 for Raspberry Pi (tal.org)](https://www.tal.org/tutorials/building-qt-6-8-raspberry-pi) — Native Pi build process, Mesa V3D, dropped Broadcom blobs
- [Secure and efficient QNAM use (volkerkrause.eu)](https://www.volkerkrause.eu/2022/11/19/qt-qnetworkaccessmanager-best-practices.html) — Single instance, deleteLater(), request timeout
- [Cross-Compile Qt 6 for Raspberry Pi (Qt Wiki)](https://wiki.qt.io/Cross-Compile_Qt_6_for_Raspberry_Pi) — Sysroot symlink issues documented
- [raspberrypi/firmware GitHub Issue #1724](https://github.com/raspberrypi/firmware/issues/1724) — Confirmed brcm backend failure on Pi 4
- [Raspberry Pi Forums — burn-in / screen blanking](https://forums.raspberrypi.com/viewtopic.php?t=290974) — Pixel shift and brightness strategies
- [Raspberry Pi Forums — watchdog](https://forums.raspberrypi.com/viewtopic.php?t=376126) — Hardware watchdog default-enabled
- [WeatherLink Live — Cumulus Wiki](https://cumuluswiki.org/a/WeatherLink_Live) — UDP renewal community implementation notes

### Tertiary (LOW confidence)
- [RoniaKit (github.com/Roniasoft/RoniaKit)](https://github.com/Roniasoft/RoniaKit) — Open-source QML gauge components; unmaintained status unclear; custom Canvas preferred
- [Qt OpenGL vs QPainter on Pi (github.com/fhunleth/qt-rectangles)](https://github.com/fhunleth/qt-rectangles) — Performance comparison; older but pattern still applies

---
*Research completed: 2026-03-01*
*Ready for roadmap: yes*
