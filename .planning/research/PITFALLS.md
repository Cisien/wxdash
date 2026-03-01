# Pitfalls Research

**Domain:** Qt 6 C++ real-time weather dashboard / Raspberry Pi kiosk
**Researched:** 2026-03-01
**Confidence:** MEDIUM (most findings verified across multiple sources; some Pi-specific GPU behavior is HIGH confidence from community consensus)

---

## Critical Pitfalls

### Pitfall 1: EGLFS Platform Choice Mismatch for Raspberry Pi 4/5

**What goes wrong:**

The legacy `brcm` EGLFS backend (used on Pi 1–3) does not work on Raspberry Pi 4 or 5 because the VideoCore VI GPU uses a completely different driver stack. Developers following old tutorials configure the wrong EGLFS backend and get either a black screen or "Could not find DRM device!" errors at launch. The correct backend for Pi 4/5 is `eglfs_kms` (DRM/KMS via GBM), not `eglfs_brcm`.

**Why it happens:**

Most search results, blog posts, and forum answers for "Qt EGLFS Raspberry Pi" are from the Pi 1–3 era. Official documentation describes both backends but does not prominently flag the Pi 4 break. Developers assume continuity.

**How to avoid:**

- Use `eglfs_kms` on Pi 4/5. Verify with `QT_QPA_EGLFS_INTEGRATION=eglfs_kms`.
- Ensure `vc4` and `v3d` kernel modules are loaded: `modprobe vc4 && modprobe v3d`.
- Add the runtime user to the `video` group: `usermod -aG video <username>` — without this, `/dev/dri/card*` access is denied with "Operation not permitted."
- Create a JSON KMS config file and point to it with `QT_QPA_EGLFS_KMS_CONFIG=/path/to/eglfs.json` to explicitly select the correct DRM device if multiple `/dev/dri/card*` nodes exist.
- Enable debug logging during setup: `QT_LOGGING_RULES="qt.qpa.eglfs.kms=true"`.

**Warning signs:**

- "Could not find DRM device!" at startup.
- "EGLFS Raspberry Pi ... no" in the Qt configure summary (means wrong backend compiled in).
- Black screen with no error output (usually wrong backend + silent fallback to software rendering).
- Application launches but GPU load stays at 0% (software rasterization fallback).

**Phase to address:** Environment setup / Phase 1 (foundation). Validate display initialization before writing a single line of UI code. A "hello world" full-screen Qt app with EGLFS must be confirmed working on the target hardware before any gauge code is written.

---

### Pitfall 2: QPainter Widget Rendering Falls Back to Software Rasterization

**What goes wrong:**

Qt Widgets with QPainter on EGLFS may silently fall back to software (CPU) rasterization rather than using the GPU. This is particularly acute on Raspberry Pi where EGLFS internally may bypass true hardware acceleration for the 2D widget paint path. The result is gauge redraws consuming 30–80% CPU for a display that should be nearly idle, causing thermal throttling on the Pi's ARM cores within hours of operation.

**Why it happens:**

Qt's EGLFS uses OpenGL ES for compositing, but QPainter's default raster engine bypasses OpenGL for widget painting. Antialiased arcs, radial gradients, and thick stroked paths — all common in analog gauge widgets — are rendered entirely on the CPU. The GPU sits idle while the CPU heats up.

**How to avoid:**

- Profile with `htop` and `vcgencmd measure_temp` during a full-screen animated gauge test before finalizing widget implementation.
- Prefer `QOpenGLWidget` as the base for custom gauges, which routes painting through OpenGL ES and the GPU.
- Alternatively use `QRhiWidget` (Qt 6.6+) which uses Qt's RHI abstraction layer and is the modern recommended approach.
- Minimize `update()` calls: only repaint the gauge widget when its data value actually changes. Do not use a blanket timer that repaints all gauges at 2.5 Hz regardless of value changes.
- Avoid `QLinearGradient` / `QRadialGradient` on the software raster path — these are expensive. If using software raster, pre-render static gradient backgrounds to a `QPixmap` once and blit it.
- Do not use `QPainter::Antialiasing` hint everywhere blindly — benchmark without it first on the Pi target.

**Warning signs:**

- `top` or `htop` shows Qt process at 30%+ CPU during normal operation.
- `vcgencmd measure_temp` shows temperature rising toward 70°C+ during idle display.
- Frame rate of gauge redraws feels choppy or drops below 30fps.
- GPU usage tools (e.g., `sudo vcgencmd get_mem gpu`) show GPU memory barely used.

**Phase to address:** Phase 1 (gauge widget foundation). Pick the rendering path and benchmark it before building all 8+ gauges on the wrong foundation.

---

### Pitfall 3: QNetworkReply Memory Leak from Missing deleteLater()

**What goes wrong:**

Every `QNetworkAccessManager::get()` call returns a `QNetworkReply*`. If `deleteLater()` is not called on the reply after processing the `finished()` signal, the reply object accumulates in heap memory. For a dashboard polling HTTP every 10 seconds, this is 360 leaked reply objects per hour, 8,640 per day. On a Raspberry Pi with 1–8 GB RAM, this causes RSS memory growth of several MB per hour and eventually an OOM kill after days of continuous operation.

**Why it happens:**

Qt documentation does not make `deleteLater()` the first thing you see. Developers write `connect(reply, &QNetworkReply::finished, this, &MyClass::onData)` in `onData`, parse the JSON, and return — forgetting that the `QNetworkReply` object is never freed. The parent-child ownership model does not save you here because the reply's parent is the QNAM, not the handler.

**How to avoid:**

```cpp
// Correct pattern — always call deleteLater in the finished handler
void WeatherPoller::onCurrentConditionsReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        processCurrentConditions(data);
    } else {
        handleNetworkError(reply->error());
    }
    reply->deleteLater();  // MANDATORY — must be last operation on reply
}
```

- Use a single `QNetworkAccessManager` instance for the entire application. Do not create a new QNAM per poll cycle. QNAM maintains connection pools and SSL sessions; multiple instances waste resources and lose these optimizations.
- Wait for the previous reply's `finished()` signal before issuing the next poll request. Do not fire overlapping requests on a 10-second timer if a previous request is still in flight.
- Watch for timeout replies: if WeatherLink Live goes offline, QNAM may hold the reply open until `QNetworkRequest::setTransferTimeout()` fires. Set an explicit timeout (e.g., 5000ms) so stalled requests don't block the poll cycle.

**Warning signs:**

- RSS memory in `htop` grows continuously over hours without plateauing.
- `valgrind --leak-check=full` on a development build shows `QNetworkReplyImpl` in leak report.
- QNAM request queue grows (observable by logging request count vs. reply count).

**Phase to address:** Phase 2 (HTTP data ingestion). Write a QNAM leak test that runs 1000 polls in a tight loop and checks RSS memory before and after.

---

### Pitfall 4: UDP Broadcast Session Expiry — Silent Data Loss

**What goes wrong:**

The WeatherLink Live UDP real-time broadcast has a configurable duration; the default is 20 minutes (1200 seconds), maximum 86400 seconds. If the application does not periodically re-request the broadcast session before it expires, UDP packets simply stop arriving. The application continues running, the HTTP poll still works, but wind and rain data freeze at their last values. This is completely silent — no error, no socket error event, no WeatherLink notification.

**Why it happens:**

The WeatherLink Live local API documentation describes the renewal requirement, but it is easy to miss. Developers implement the initial `GET /v1/real_time?duration=N` call at startup and assume the broadcast persists. It does not.

**How to avoid:**

- Set `duration=86400` (24 hours) at startup. This is the maximum and reduces renewal frequency to once per day.
- Even with 86400-second duration, implement a keep-alive that re-calls `/v1/real_time?duration=86400` at a conservative interval (e.g., every 12 hours), in case the WeatherLink Live reboots and loses broadcast state.
- Track the timestamp of the last received UDP packet. If no UDP packet arrives within 10 seconds (4 missed 2.5s intervals), trigger an immediate re-registration of the UDP broadcast.
- Distinguish "WeatherLink offline" (HTTP also failing) from "UDP session expired" (HTTP succeeds but UDP silent). Different recovery paths are needed.

**Warning signs:**

- Wind speed and rain rate values freeze while temperature, humidity, and pressure continue updating.
- UDP socket's `readyRead` signal stops firing but no socket error is emitted.
- Time since last UDP packet exceeds 10 seconds during normal WeatherLink Live operation.

**Phase to address:** Phase 2 (UDP ingestion). The UDP session renewal logic must be in the initial UDP implementation, not added later as a bug fix.

---

### Pitfall 5: Cross-Compilation Sysroot Symlink Corruption

**What goes wrong:**

When setting up a cross-compilation sysroot by rsyncing `/lib`, `/usr/lib`, etc. from the Raspberry Pi, the absolute symlinks on the Pi become dangling symlinks in the host sysroot. CMake's toolchain file cannot resolve library locations during Qt configuration. The build fails with errors like "cannot find -lEGL", "cannot find -lGLESv2", or DBus link failures, even though the files visually exist in the sysroot.

**Why it happens:**

Raspberry Pi OS uses absolute symlinks (e.g., `/lib -> /usr/lib`). rsync copies these as-is, making them point to paths that don't exist on the build host. The standard `sysroot-relativelinks.py` script from official Qt documentation sometimes fails to fix all cases correctly, particularly for the aarch64 multilib paths.

**How to avoid:**

- Use `rsync -aHAXv --rsync-path="sudo rsync"` from the Pi directly (not from host) to preserve symlinks correctly as relative links.
- Alternatively use Docker-based cross-compilation (e.g., the [PhysicsX/QTonRaspberryPi](https://github.com/PhysicsX/QTonRaspberryPi) approach) which encapsulates the sysroot and toolchain in a reproducible container.
- After rsyncing, run the relativelinks script AND manually verify EGL and GLESv2 symlink chains: `readlink -f sysroot/usr/lib/aarch64-linux-gnu/libEGL.so` must resolve to an actual file, not a broken chain.
- Add `-Wl,-fuse-ld=gold` to `QT_LINKER_FLAGS` in the toolchain file as a workaround for DBus linking failures.
- On Pi 4/5 (aarch64), use the system GCC cross-compiler from `apt` (`gcc-aarch64-linux-gnu`), not Linaro's toolchain — the CMake toolchain file is incompatible with Linaro.

**Warning signs:**

- Qt `cmake` configure step shows "EGL ... no" or "OpenGLES ... no" despite the files being in the sysroot directory.
- Linker errors referencing libraries that `ls` shows exist at that path.
- Build succeeds but binary crashes immediately on Pi with "cannot open shared object file."

**Phase to address:** Phase 0 (build environment). This must be solved before any application code. Consider native compilation on Pi 5 (significantly faster than Pi 4) as an alternative to cross-compilation if the team lacks embedded Linux cross-compile experience.

---

## Moderate Pitfalls

### Pitfall 6: Always-On Display Burn-In

**What goes wrong:**

A static weather dashboard displaying the same gauge layout 24/7 will cause image retention ("burn-in") on IPS/VA LCD panels within weeks to months, and on OLED panels within days to weeks. The static gauge arcs, needle resting positions, and label positions leave permanent ghost images that are visible when displaying other content.

**Prevention:**

- IPS/LCD panels: Run at 60–70% brightness (not 100%). Implement a configurable "overnight dim" schedule using `vcgencmd display_power 0` (Raspberry Pi specific) or by sending DPMS off signal. For a weather station this is natural: dim from midnight to 6am.
- OLED panels: Use pixel-shift — shift the entire rendered widget area by ±2px on a slow timer (every 30 minutes). Implement a dynamic background that subtly changes (e.g., sky color shifting with time of day) rather than a static black or white background.
- Regardless of panel type: Reduce gauge needle and arc line brightness rather than rendering full-white/full-red on a black background.
- Phase to address: Phase 3 (polish/deployment). Not urgent for development, but must be in place before any long-term installation.

---

### Pitfall 7: mDNS Resolution Failure Breaking Startup

**What goes wrong:**

The WeatherLink Live is configured to be reached via `weatherlinklive.local.cisien.com`. If mDNS resolution fails at startup (network not yet up, mDNS daemon not running, DHCP lease not yet assigned), `QNetworkAccessManager` returns an error immediately. An application that treats the first failed request as a fatal error will crash or show a broken UI at every boot.

**Prevention:**

- Use a retry loop with exponential backoff for the initial HTTP connection. Do not treat startup network failure as fatal.
- Show a "Connecting..." state on the dashboard rather than leaving gauges in an undefined display state.
- Implement a watchdog QTimer that re-attempts connection every 30 seconds when in disconnected state.
- Consider hard-coding the IP address as a fallback alongside the mDNS hostname, in case Avahi/mDNS is not reliably available on the Pi's target OS configuration.
- Phase to address: Phase 2 (network ingestion). The disconnected/reconnecting state machine must be part of the initial network layer, not retrofitted.

---

### Pitfall 8: Unbounded Sparkline History Buffer

**What goes wrong:**

Sparklines showing "recent hours" of data sound simple, but without a fixed-size circular buffer, the history array grows indefinitely. At one data point per 10 seconds for temperature/humidity/pressure, that is 360 points/hour, 8,640/day. After a week of operation, iterating over thousands of unnecessary history points on every sparkline repaint slows the UI and wastes RAM.

**Prevention:**

- Define maximum sparkline history at construction time (e.g., 2 hours = 720 points for 10s poll rate).
- Use a fixed-size circular/ring buffer (`std::deque` with max-size enforcement, or a fixed `std::array` with a head index). `QVector` with `removeFirst()` is an O(n) anti-pattern for this use case.
- `QCircularBuffer` from Qt3D is in Qt's private API — do not use it. Implement a simple fixed-size ring buffer or use `boost::circular_buffer` if Boost is available.
- Phase to address: Phase 2 (data model). Design the circular buffer as a first-class data structure from the beginning.

---

### Pitfall 9: Multiple QNetworkAccessManager Instances

**What goes wrong:**

Creating a new `QNetworkAccessManager` in each class that needs network access (e.g., one in `WeatherPoller`, one in `UDPSessionManager`) bypasses QNAM's internal connection pooling, multiplies SSL handshake overhead, and increases the risk of misconfigured instances (e.g., missing timeouts on one instance).

**Prevention:**

- Create exactly one `QNetworkAccessManager` for the entire application and pass it by pointer to components that need it. This is explicitly documented Qt best practice.
- Phase to address: Phase 2 (architecture foundation). Establish the singleton QNAM before writing any polling code.

---

### Pitfall 10: Rain Count-to-Inches Conversion Error

**What goes wrong:**

The WeatherLink Live reports rain data as raw bucket-tip counts, not inches. The conversion factor depends on the `rain_size` field (1 = 0.01 in, 2 = 0.2 mm, 3 = 0.1 mm, 4 = 0.001 in). Ignoring `rain_size` and hardcoding 0.01 produces wrong rain totals for non-standard collectors and is a silent data corruption bug — the gauge shows values that look plausible but are wrong.

**Prevention:**

- Parse `rain_size` from the API response and apply the correct multiplier in the data model layer.
- Validate: a typical Davis tipping spoon reports rain_size=1. Log the rain_size value at startup so it can be confirmed correct.
- Phase to address: Phase 2 (data parsing). Include rain_size handling in the initial JSON parsing unit tests.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Native compile on Pi instead of cross-compile | Eliminates sysroot/toolchain complexity | Build times 20–40 min instead of 2–5 min; impractical for iteration | Acceptable for Pi 5 (fast enough); never for Pi 4 or Pi Zero |
| Polling UDP "manually" with a blocking recvfrom loop | Simpler code, avoids Qt socket setup | Blocks event loop; breaks all Qt timers and signals during receive | Never — always use QUdpSocket with readyRead signal |
| Use X11 via `startx` instead of EGLFS | Easier to debug; supports multiple windows | X11 overhead eats ~100MB RAM; display server adds latency; GPU path differs | MVP only if EGLFS cannot be made to work; must be replaced before deployment |
| Single QTimer driving all updates at 2.5Hz | Simple | Repaints gauges with unchanged values; unnecessary CPU/GPU load every 2.5s | Never — only repaint when data changes |
| Hardcode WeatherLink IP instead of mDNS | Simpler connection logic | Breaks when DHCP lease changes; not portable | Acceptable as a fallback alongside mDNS, never as sole method |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| WeatherLink Live UDP | Bind UDP socket to `0.0.0.0:22222` and assume packets arrive forever | Register broadcast session with `duration=86400`; track last-packet timestamp; re-register on silence |
| WeatherLink Live HTTP | Create QNAM per request; forget `deleteLater()` | One QNAM singleton; always call `reply->deleteLater()` in `finished()` handler |
| WeatherLink Live HTTP | Parse only `data_structures[0]` assuming ISS is always first | Iterate all `data_structures` and match by `data_structure_type` field (1=ISS, 3=LSS BAR, 4=LSS Temp/Hum) |
| EGLFS + KMS | Assume application auto-detects correct DRM device | Set `QT_QPA_EGLFS_KMS_CONFIG` JSON with explicit `"device": "/dev/dri/card1"` if multiple GPU devices exist |
| mDNS hostname | Use `QHostInfo::lookupHost` blocking path | Use `QNetworkRequest` directly with mDNS hostname; QNAM handles DNS asynchronously |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Software rasterization for gauge arcs | CPU at 40-80% during normal display; Pi thermally throttles | Use QOpenGLWidget or QRhiWidget as gauge base; profile on target hardware early | Immediately visible on Pi 4/5 with complex gauges at 30Hz refresh |
| `update()` called unconditionally at 2.5Hz on all widgets | CPU usage higher than expected; unnecessary GPU/CPU wake | Only call `update()` on widgets whose underlying data value changed | Not immediately visible at low gauge count; worsens as gauge count grows |
| `QVector::removeFirst()` for sparkline history | Frame time spikes during sparkline render when history is long | Fixed-size ring buffer with head/tail index | Noticeable after ~4 hours of data accumulation |
| Antialiasing on software raster path | Smooth gauges at 10fps instead of 60fps | Profile raster vs. OpenGL paths; disable antialiasing on software path | Immediately visible on Pi with Qt Widgets + EGLFS software fallback |
| QTimer without elapsed compensation | Cumulative drift: 10s poll becomes 10.1s over hours | Use `QElapsedTimer` to compute actual elapsed time; adjust next interval to compensate | Subtle; only matters if rain totals or trend calculations are time-sensitive |

---

## "Looks Done But Isn't" Checklist

- [ ] **UDP data pipeline:** Verify that UDP session renewal is implemented — the stream stopping after 20 minutes is not a network failure, it is expected expiry behavior.
- [ ] **Network disconnect recovery:** Simulate unplugging the Ethernet cable for 60 seconds and re-plugging. The dashboard must recover and resume showing live data without a manual restart.
- [ ] **Memory stability:** Run for 24 hours and check RSS memory at start vs. end. Growth of more than 10MB indicates a leak.
- [ ] **Thermal stability:** Let the dashboard run for 1 hour and check `vcgencmd measure_temp`. Values above 75°C on a Pi 4 without a heatsink indicate excessive CPU load from software rendering.
- [ ] **Rain rain_size parsing:** Verify the rain_size field is read and the correct multiplier is applied. Log rain_size at startup.
- [ ] **EGLFS hardware acceleration:** Confirm GPU is being used. GPU load should be nonzero during gauge animation; CPU should be below 20% during idle display.
- [ ] **`deleteLater()` on all replies:** Verify every `QNetworkReply` created by QNAM has a corresponding `deleteLater()` call in the finished handler — search the codebase for `QNetworkReply` to audit.
- [ ] **data_structure_type parsing:** Verify ISS (type 1), LSS BAR (type 3), and LSS Temp/Hum (type 4) are all correctly identified by type, not by array index position.

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Wrong EGLFS backend (brcm vs kms) | LOW | Set `QT_QPA_EGLFS_INTEGRATION=eglfs_kms`, add user to video group, confirm kernel modules loaded |
| QNetworkReply memory leak discovered after deployment | MEDIUM | Add `reply->deleteLater()` to all finished handlers; audit all QNAM usage; redeploy |
| UDP session expiry (silent freeze) | LOW | Add last-packet watchdog timer; call re-register endpoint; no refactor needed |
| Sysroot symlink corruption | HIGH | Rebuild sysroot from scratch using rsync with elevated privileges; switch to Docker-based build if recurring |
| Software rasterization (gauges too slow) | HIGH | Requires rewriting gauge widgets on QOpenGLWidget/QRhiWidget base; cannot be patched around QPainter |
| Rain count conversion bug | LOW | Fix multiplier lookup table in data model; existing data is wrong but forward data is correct after fix |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Wrong EGLFS backend (brcm vs eglfs_kms) | Phase 1: Display environment setup | Hello-world Qt app renders full-screen at 60fps on Pi hardware with GPU load > 0% |
| QPainter software rasterization | Phase 1: Gauge widget foundation | Profiler shows CPU below 15% with all gauges animating; temperature stable |
| QNetworkReply memory leak | Phase 2: HTTP polling implementation | 1-hour test with 360 requests shows flat RSS memory |
| UDP session expiry | Phase 2: UDP real-time ingestion | Simulate 21-minute run; confirm stream continues after renewal; confirm re-registration on silence |
| Cross-compilation sysroot symlinks | Phase 0: Build environment | Binary produced on host runs without shared library errors on Pi |
| mDNS startup failure recovery | Phase 2: Network layer | Boot Pi with WeatherLink offline; confirm dashboard shows "connecting" not crash; confirm recovery when network appears |
| Sparkline unbounded buffer | Phase 2: Data model | 48-hour RSS memory check shows no growth from sparkline history |
| Multiple QNAM instances | Phase 2: Architecture | Code review — grep for `new QNetworkAccessManager` confirms single instance |
| Rain count conversion | Phase 2: Data parsing | Unit test: rain_size=1, raw_count=5 → 0.05 inches; rain_size=4, raw_count=5 → 0.005 inches |
| Always-on burn-in | Phase 3: Deployment hardening | Overnight dim schedule configured; pixel-shift enabled on OLED if applicable |

---

## Sources

- [Qt for Embedded Linux — Qt 6.10 Official Docs](https://doc.qt.io/qt-6/embedded-linux.html) — EGLFS platform plugin documentation
- [Cross-Compile Qt 6 for Raspberry Pi — Qt Wiki](https://wiki.qt.io/Cross-Compile_Qt_6_for_Raspberry_Pi) — official sysroot and toolchain instructions
- [QT6 eglfs not working on Raspberry Pi 4 — raspberrypi/firmware GitHub Issue #1724](https://github.com/raspberrypi/firmware/issues/1724) — confirmed brcm backend failure on Pi 4
- [Cross-compiling Qt 6.5 for armhf and aarch64 — Qt Wiki](https://wiki.qt.io/Cross-compiling_Qt_6.5_for_both_armhf_and_aarch64_architectures_for_Raspberry_Pi_OS) — architecture-specific build instructions
- [Qt Forum: running Qt on Raspberry Pi using EGLFS](https://forum.qt.io/topic/156510/running-qt-on-raspberry-pi-using-eglfs-or-install-a-minimal-kde-de) — community experience on EGLFS vs desktop compositor
- [QCustomPlot performance improvement docs](https://www.qcustomplot.com/documentation/performanceimprovement.html) — OpenGL acceleration for plotting
- [Raspberry Pi Qt plotting with high CPU usage — Qt Forum](https://forum.qt.io/topic/64777/raspberry-pi-qt-plotting-with-high-cpu-usage) — software rasterization performance data
- [Qt Forum: Memory leak on QNetworkAccessManager](https://forum.qt.io/topic/93081/memory-leak-on-qnetworkaccessmanager) — deleteLater() requirement
- [Secure and efficient QNetworkAccessManager use — volkerkrause.eu](https://www.volkerkrause.eu/2022/11/19/qt-qnetworkaccessmanager-best-practices.html) — single instance best practice
- [Qt Forum: QUdpSocket in QThread critical errors](https://forum.qt.io/topic/21218/qudpsocket-in-the-qthread-critical-errors) — event loop requirements for UDP
- [WeatherLink Live Local API documentation](https://weatherlink.github.io/weatherlink-live-local-api/) — UDP broadcast duration and renewal requirements
- [WeatherLink Live — Cumulus Wiki](https://cumuluswiki.org/a/WeatherLink_Live) — community implementation notes including UDP renewal
- [QNanoPainter issue #45 — Raspberry Pi performance](https://github.com/QUItCoding/qnanopainter/issues/45) — EGLFS rendering performance data
- [Failed to take device /dev/dri/card0 — Arch Linux Forums](https://bbs.archlinux.org/viewtopic.php?id=250684) — video group permission requirement
- [Raspberry Pi Forums: screen burn-in prevention](https://forums.raspberrypi.com/viewtopic.php?t=290974) — pixel shift and brightness strategies
- [Qt Forum: Circular buffer in Qt](https://forum.qt.io/topic/121622/circular-buffer-in-qt) — ring buffer implementation guidance

---
*Pitfalls research for: Qt 6 C++ real-time weather dashboard on Raspberry Pi kiosk*
*Researched: 2026-03-01*
