---
phase: 01-data-model-and-network-layer
plan: "03"
subsystem: network
tags: [qt6, qnetworkaccessmanager, qudpsocket, qthread, movetothread, metatype, c++17, http-polling, udp-broadcast]

requires:
  - phase: 01-data-model-and-network-layer/01-01
    provides: JsonParser namespace (parseCurrentConditions, parseUdpDatagram) and WeatherReadings.h structs
  - phase: 01-data-model-and-network-layer/01-02
    provides: WeatherDataModel QObject with applyIssUpdate/applyBarUpdate/applyIndoorUpdate/applyUdpUpdate slots

provides:
  - HttpPoller QObject with 10s poll timer, abort-before-poll, 5s transfer timeout, always-deleteLater reply handling
  - UdpReceiver QObject with port 22222 bind, 1h renewal timer, 10s health check, own QNAM for broadcast requests
  - main.cpp wiring all components on a dedicated network thread with cross-thread queued signal connections
  - qRegisterMetaType for all four reading structs (IssReading, BarReading, IndoorReading, UdpReading)
  - Diagnostic qDebug connections proving end-to-end data flow in Phase 1

affects:
  - 02-ui-layer (QML widgets bind to WeatherDataModel Q_PROPERTY NOTIFY signals populated by this pipeline)

tech-stack:
  added:
    - QNetworkAccessManager (HTTP polling in HttpPoller and broadcast requests in UdpReceiver)
    - QUdpSocket (UDP datagram reception on port 22222)
    - QThread with moveToThread (dedicated network thread pattern)
    - QUrlQuery (building /v1/real_time?duration=86400 URL)
    - qRegisterMetaType (cross-thread struct passing via queued connections)
  patterns:
    - Worker-object-on-QThread: create without parent, moveToThread, start in QThread::started slot
    - start() slot pattern: defer QNAM/socket/timer creation until after moveToThread so they own the right thread
    - Abort-before-poll: m_pendingReply->abort() + deleteLater() before issuing next request
    - Fire-and-forget HTTP: connect reply->finished to reply->deleteLater for responses we don't inspect
    - Health-check + renewal dual timers: renewal every 1h, health check every 5s detecting 10s silence

key-files:
  created:
    - src/network/HttpPoller.h
    - src/network/HttpPoller.cpp
    - src/network/UdpReceiver.h
    - src/network/UdpReceiver.cpp
  modified:
    - src/main.cpp
    - src/CMakeLists.txt

key-decisions:
  - "HttpPoller creates QNAM in start() (not constructor) — ensures QNAM is owned by network thread after moveToThread"
  - "UdpReceiver creates its own QNAM (not shared with HttpPoller) — simpler than cross-slot coordination for broadcast requests"
  - "No cross-thread connection type specified — Qt auto-detects QueuedConnection when sender and receiver live on different threads"
  - "DATA-09: no retry logic in HttpPoller — 10s poll cadence is the retry mechanism"
  - "DATA-03: renewal timer fires every 3600s even though 86400s was requested — belt and suspenders"

patterns-established:
  - "start() slot pattern: all resource creation deferred to start() invoked after moveToThread via QThread::started signal"
  - "Worker cleanup: QThread::finished -> QObject::deleteLater on both workers"
  - "Clean shutdown: app.aboutToQuit lambda calls networkThread->quit() + networkThread->wait()"

requirements-completed: [DATA-01, DATA-02, DATA-03, DATA-09]

duration: 2min
completed: 2026-03-01
---

# Phase 1 Plan 03: HttpPoller, UdpReceiver, and main.cpp Wiring Summary

**HttpPoller (10s HTTP polling) + UdpReceiver (2.5s UDP with 1h renewal) wired on a dedicated QThread with cross-thread queued signal delivery to WeatherDataModel**

## Performance

- **Duration:** 2 min
- **Started:** 2026-03-01T18:33:34Z
- **Completed:** 2026-03-01T18:35:38Z
- **Tasks:** 3
- **Files modified:** 6 (4 created, 2 modified)

## Accomplishments

- HttpPoller polls `/v1/current_conditions` every 10s with abort-before-poll, 5s transfer timeout, and mandatory reply->deleteLater on every code path
- UdpReceiver binds to port 22222, drains all pending datagrams, fires renewBroadcast() every 3600s and on 10s silence via health check
- main.cpp registers all four reading metatypes, instantiates WeatherDataModel on main thread, moves HttpPoller and UdpReceiver to a shared network thread, wires cross-thread signals, and handles clean shutdown

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement HttpPoller** - `27d8b56` (feat)
2. **Task 2: Implement UdpReceiver with broadcast renewal** - `229579f` (feat)
3. **Task 3: Wire components in main.cpp with QThread** - `408a4b8` (feat)

## Files Created/Modified

- `/home/cisien/src/wxdash/src/network/HttpPoller.h` - QObject subclass: start() slot, issReceived/barReceived/indoorReceived signals, poll() and onReply() private slots, kPollIntervalMs constant
- `/home/cisien/src/wxdash/src/network/HttpPoller.cpp` - QNAM in start(), abort-before-poll, 5s transfer timeout, JsonParser call, always deleteLater
- `/home/cisien/src/wxdash/src/network/UdpReceiver.h` - QObject subclass: start() slot, realtimeReceived signal, renewBroadcast/checkUdpHealth private slots, three k-constants
- `/home/cisien/src/wxdash/src/network/UdpReceiver.cpp` - Own QNAM in start(), ShareAddress bind, datagram drain loop, 1h renewal timer, 5s health check with 10s silence threshold
- `/home/cisien/src/wxdash/src/main.cpp` - Full wiring: metatypes, model on main thread, workers on networkThread, cross-thread signals, lifecycle, clean shutdown, diagnostic qDebug connections
- `/home/cisien/src/wxdash/src/CMakeLists.txt` - Added HttpPoller.cpp and UdpReceiver.cpp to wxdash_lib sources

## Decisions Made

- `HttpPoller::start()` creates QNAM rather than the constructor — QNAM must be owned by the network thread, and moveToThread hasn't happened yet at construction time
- `UdpReceiver` creates its own QNAM (not shared with HttpPoller) — sharing would require a pointer/signal dance across the thread boundary; a second QNAM for fire-and-forget broadcast requests on the same thread is simpler and correct
- No `Qt::QueuedConnection` specified in any connect() call — Qt detects it automatically when sender and receiver live on different threads

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Added missing QNetworkDatagram and QNetworkReply includes in UdpReceiver.cpp**
- **Found during:** Task 2 (UdpReceiver compile)
- **Issue:** `QNetworkDatagram` is forward-declared in `<QUdpSocket>` (not fully defined), causing "incomplete type" compile errors in `onReadyRead()` and `renewBroadcast()`
- **Fix:** Added `#include <QNetworkDatagram>` and `#include <QNetworkReply>` to UdpReceiver.cpp
- **Files modified:** src/network/UdpReceiver.cpp
- **Verification:** cmake --build build completes without errors
- **Committed in:** 229579f (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 - missing includes causing compile failure)
**Impact on plan:** Necessary correction. No scope creep. Forward declarations in Qt headers require explicit includes in implementation files.

## Issues Encountered

None beyond the missing includes above.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Complete data pipeline is now in place: WeatherLink Live HTTP endpoint → HttpPoller → JsonParser → WeatherDataModel; UDP broadcast → UdpReceiver → JsonParser → WeatherDataModel
- Phase 2 UI layer (QML widgets) can bind directly to WeatherDataModel Q_PROPERTY NOTIFY signals on the main thread
- All cross-thread infrastructure is established — Phase 2 only needs to add a QQuickView/QQmlApplicationEngine and connect QML to the model
- Existing 42 unit tests (24 JsonParser + 18 WeatherDataModel) continue to pass

---
*Phase: 01-data-model-and-network-layer*
*Completed: 2026-03-01*
