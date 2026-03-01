---
phase: 02-core-gauges-and-dashboard-layout
plan: 01
subsystem: ui
tags: [qt6, qml, qtquick, qtshapes, arcgauge, kiosk, cmake]

# Dependency graph
requires:
  - phase: 01-data-model-and-network-layer
    provides: WeatherDataModel with 18 Q_PROPERTY/NOTIFY signals ready for QML binding
provides:
  - QGuiApplication + QQmlApplicationEngine replacing headless QCoreApplication
  - ArcGauge.qml reusable Shape+PathAngleArc component with SmoothedAnimation fill
  - Main.qml root Window with kiosk/windowed mode and F11 toggle
  - qt_add_qml_module registered wxdash QML module with compiled QML cache
  - --kiosk command-line flag parsed and exposed as QML context property
affects:
  - 02-02 (DashboardGrid replaces placeholder ArcGauge in Main.qml)
  - any future phase adding QML components (follows qt_add_qml_module pattern)

# Tech tracking
tech-stack:
  added: [Qt6::Quick, Qt6::Qml (transitive), QtQuick.Shapes, qt_add_qml_module, QQmlApplicationEngine, QCommandLineParser]
  patterns: [QML context property injection (weatherModel + kioskMode), qt_add_qml_module with OUTPUT_DIRECTORY to avoid exe/module name collision, Behavior+SmoothedAnimation on ShapePath property for arc animation, responsive font sizing via Math.min(width,height)*factor]

key-files:
  created:
    - src/qml/ArcGauge.qml
    - src/qml/Main.qml
  modified:
    - CMakeLists.txt
    - src/CMakeLists.txt
    - src/main.cpp

key-decisions:
  - "OUTPUT_DIRECTORY set to build/src/qml/wxdash in qt_add_qml_module to avoid linker collision between wxdash executable and default wxdash/ QML module output directory"
  - "animatedSweep property lives on ShapePath (not PathAngleArc) — avoids Behavior+PathAngleArc binding pitfall; Binding{} element drives it from root.targetSweep"
  - "KIOSK-03 (last-updated timestamp) intentionally not implemented per user decision documented in CONTEXT.md"
  - "strokeWidth stroke-style arc (ring aesthetic, no pie wedge fill) chosen per design decision for fitness-tracker-ring look"
  - "SmoothedAnimation velocity:200 deg/s chosen for smooth animation without excessive lag on 2.5s data updates"

patterns-established:
  - "ArcGauge pattern: expose value/minValue/maxValue as root properties, compute targetSweep inline, use Binding element to drive animated property"
  - "QML module pattern: qt_add_qml_module with explicit OUTPUT_DIRECTORY when URI matches executable name"
  - "Context property pattern: set all context properties before engine.loadFromModule()"
  - "Responsive sizing pattern: font.pixelSize bound to Math.min(width,height)*factor throughout — no hardcoded pixel sizes"

requirements-completed: [GAUG-14, KIOSK-01, KIOSK-02, KIOSK-03]

# Metrics
duration: 3min
completed: 2026-03-01
---

# Phase 2 Plan 01: QGuiApplication + ArcGauge component + kiosk window foundation

**QGuiApplication/QQmlApplicationEngine migration with reusable Shape-based ArcGauge.qml (290-degree animated arc, SmoothedAnimation fill) and kiosk-capable Main.qml root window**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-01T19:28:17Z
- **Completed:** 2026-03-01T19:31:14Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Migrated from headless QCoreApplication to QGuiApplication + QQmlApplicationEngine with weatherModel and kioskMode context properties
- Built reusable ArcGauge.qml with 290-degree arc geometry, track+fill ShapePaths, SmoothedAnimation velocity-based transition, and responsive font sizing
- Created Main.qml with --kiosk/F11 fullscreen toggle and placeholder ArcGauge verifying the full render pipeline
- Resolved qt_add_qml_module/executable name collision via OUTPUT_DIRECTORY
- All 42 unit tests (24 JsonParser + 18 WeatherDataModel) continue passing

## Task Commits

Each task was committed atomically:

1. **Task 1: CMake migration + main.cpp rewrite for QML engine** - `429e1c8` (feat)
2. **Task 2: ArcGauge.qml reusable component + Main.qml root window** - `ec2ef37` (feat)

## Files Created/Modified
- `CMakeLists.txt` - Added Qt6::Quick and Qt6::Qml to find_package
- `src/CMakeLists.txt` - Added qt_add_qml_module with OUTPUT_DIRECTORY, Qt6::Quick linkage
- `src/main.cpp` - QGuiApplication, QQmlApplicationEngine, QCommandLineParser, kioskMode context property
- `src/qml/ArcGauge.qml` - Reusable 290-degree arc gauge with Shape+PathAngleArc+SmoothedAnimation
- `src/qml/Main.qml` - Root Window with kiosk/windowed mode, F11 toggle, placeholder ArcGauge

## Decisions Made
- **OUTPUT_DIRECTORY for QML module:** `qt_add_qml_module` by default creates an output directory matching the URI (`wxdash/`), which collides with the executable named `wxdash`. Fixed with `OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash`.
- **animatedSweep on ShapePath:** The animated property lives on the ShapePath item, not on PathAngleArc, to avoid the Behavior binding pitfall documented in Phase 2 research. A `Binding {}` element drives it from root.targetSweep.
- **KIOSK-03 intentionally omitted:** User explicitly requested this requirement be dropped. Satisfied by non-implementation per CONTEXT.md decision.
- **strokeColor ring style:** Arc fill uses stroke (ring aesthetic) not area fill (pie wedge), consistent with fitness-tracker-ring design intent.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] OUTPUT_DIRECTORY added to resolve qt_add_qml_module/executable name collision**
- **Found during:** Task 1 (first build attempt)
- **Issue:** `qt_add_qml_module(wxdash URI wxdash ...)` creates a directory `build/src/wxdash/` for QML module output. The linker then cannot write the `wxdash` executable because a directory with that name already exists at the same path.
- **Fix:** Added `OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash` to redirect QML module output to `build/src/qml/wxdash/`
- **Files modified:** src/CMakeLists.txt
- **Verification:** Clean rebuild succeeded; executable links; `./build/src/wxdash` exists as a file
- **Committed in:** `429e1c8` (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Fix was necessary for the build to succeed. No scope creep.

## Issues Encountered
- Stale build directory from Phase 1 (old `build/src/wxdash` binary) required `rm -rf build` before fresh reconfigure. Resolved immediately.
- QML module / executable name collision (documented above as deviation).

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- ArcGauge component is fully implemented and ready for Plan 02-02 instantiation in DashboardGrid
- Main.qml has a placeholder ArcGauge that Plan 02-02 will replace with the full 3x4 grid
- weatherModel context property is live in QML — all 18 Q_PROPERTY/NOTIFY signals available for binding
- kioskMode context property is live — fullscreen/windowed behavior verified
- All 42 Phase 1 tests continue passing — no regressions

## Self-Check: PASSED

- FOUND: src/qml/ArcGauge.qml
- FOUND: src/qml/Main.qml
- FOUND: src/main.cpp
- FOUND: CMakeLists.txt
- FOUND: src/CMakeLists.txt
- FOUND: build/src/wxdash (executable)
- FOUND: commit 429e1c8 (Task 1)
- FOUND: commit ec2ef37 (Task 2)

---
*Phase: 02-core-gauges-and-dashboard-layout*
*Completed: 2026-03-01*
