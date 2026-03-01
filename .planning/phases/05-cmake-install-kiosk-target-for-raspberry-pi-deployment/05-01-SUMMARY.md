---
phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment
plan: "01"
subsystem: infra
tags: [cmake, kiosk, raspberry-pi, systemd, eglfs, qml, deployment]

# Dependency graph
requires:
  - phase: 02-core-gauges-and-dashboard-layout
    provides: wxdash QML module with URI wxdash and OUTPUT_DIRECTORY in build tree
  - phase: 03-trends-secondary-data-air-quality
    provides: complete wxdash binary with AqiGauge, SparkLines, all components
provides:
  - CMake install() rules with COMPONENT kiosk targeting FHS-standard paths
  - systemd service template assets/wxdash.service.in with @CMAKE_INSTALL_PREFIX@ substitution
  - EGLFS KMS device config assets/eglfs.json targeting /dev/dri/card1 for Pi 4/5
  - Desktop entry template assets/wxdash-kiosk.desktop.in with installed Exec= path
  - install-kiosk custom CMake target (cmake --build build --target install-kiosk)
  - QML module install to prefix/qml/wxdash/ alongside binary
affects:
  - Pi deployment documentation (README section on EGLFS, gpu_mem, mDNS)

# Tech tracking
tech-stack:
  added: [GNUInstallDirs, configure_file @ONLY, cmake --install --component kiosk]
  patterns:
    - "Manual QML module install(DIRECTORY) instead of qt_generate_deploy_qml_app_script to avoid RPATH-patching failures on Linux desktop systems"
    - "configure_file(@ONLY) for service/desktop templates — only substitutes @VAR@ not ${VAR} so systemd Environment= lines are safe"
    - "COMPONENT kiosk on all install() rules enables selective install via cmake --install build --component kiosk"

key-files:
  created:
    - assets/wxdash.service.in
    - assets/eglfs.json
    - assets/wxdash-kiosk.desktop.in
  modified:
    - CMakeLists.txt
    - src/CMakeLists.txt

key-decisions:
  - "Manual install(DIRECTORY) for QML module replaces qt_generate_deploy_qml_app_script — the generic Qt deploy tool fails RPATH-patching on Linux desktop (kimg_ani.so has no RPATH entry). Pi deployment uses system Qt packages so only the user QML module needs shipping."
  - "QML_IMPORT_PATH=@CMAKE_INSTALL_PREFIX@/qml added to systemd service so installed binary resolves wxdash QML module from prefix/qml/wxdash/"
  - "wxdash-kiosk.desktop (hardcoded build path) kept as developer convenience; wxdash-kiosk.desktop.in is the separate install-time template"

patterns-established:
  - "configure_file(@ONLY): safe for systemd service templates — @-substitution only, ${VAR} left untouched"
  - "COMPONENT kiosk: all install() rules use this component for selective deployment without Qt system libraries"

requirements-completed: [KIOSK-05]

# Metrics
duration: 3min
completed: 2026-03-01
---

# Phase 5 Plan 01: CMake Install and Kiosk Deployment Assets Summary

**CMake install rules with COMPONENT kiosk targeting FHS paths, systemd service template with @CMAKE_INSTALL_PREFIX@ substitution, Pi 4/5 EGLFS KMS config, and desktop entry template — enabling `cmake --install build --component kiosk` for one-command Pi deployment**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-01T22:57:48Z
- **Completed:** 2026-03-01T23:00:17Z
- **Tasks:** 2
- **Files modified:** 5 (2 modified, 3 created)

## Accomplishments

- Added `install(TARGETS wxdash RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT kiosk)` in src/CMakeLists.txt
- Added `configure_file(@ONLY)` for service and desktop templates in top-level CMakeLists.txt, with install() rules for service file, desktop entries, SVG icon, and eglfs.json at FHS-standard paths
- QML module (all .qml files, qmldir, .qmltypes) installs to `prefix/qml/wxdash/` alongside binary with `QML_IMPORT_PATH` in systemd service
- `install-kiosk` custom CMake target added (runs cmake --install + daemon-reload + systemctl enable)
- New assets: wxdash.service.in (systemd unit with Pi EGLFS env vars), eglfs.json (/dev/dri/card1), wxdash-kiosk.desktop.in (installed Exec= path)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add CMake install rules, systemd service template, eglfs.json, and desktop template** - `0bf1d14` (feat)
2. **Task 2: Verify QML module deploys alongside binary** - verified via Task 1 commit (no additional code changes needed)

## Files Created/Modified

- `/home/cisien/src/wxdash/CMakeLists.txt` - Added install rules block: GNUInstallDirs, configure_file for service/desktop templates, install() for service/desktop/icon/eglfs.json, install-kiosk target
- `/home/cisien/src/wxdash/src/CMakeLists.txt` - Added install(TARGETS wxdash RUNTIME), manual install(DIRECTORY) for QML module
- `/home/cisien/src/wxdash/assets/wxdash.service.in` - systemd unit template with @CMAKE_INSTALL_PREFIX@ in ExecStart, WorkingDirectory, EGLFS env vars, QML_IMPORT_PATH; Restart=always for kiosk resilience
- `/home/cisien/src/wxdash/assets/eglfs.json` - Pi 4/5 EGLFS KMS device config with /dev/dri/card1 and HDMI1 output
- `/home/cisien/src/wxdash/assets/wxdash-kiosk.desktop.in` - Desktop entry template with Exec=@CMAKE_INSTALL_PREFIX@/bin/wxdash --kiosk

## Decisions Made

- **Manual QML install over qt_generate_deploy_qml_app_script:** The Qt generic deploy script on Linux desktop systems tries to copy and re-RPATH all system libraries, which fails for plugins like kimg_ani.so that have no RPATH entry. On the Pi, Qt comes from system packages — only the user QML module (wxdash URI) needs to be shipped. Manual `install(DIRECTORY)` is the correct solution.
- **QML_IMPORT_PATH in systemd service:** Since the QML module installs to `prefix/qml/wxdash/` (one level up from binary), the service needs `QML_IMPORT_PATH=@CMAKE_INSTALL_PREFIX@/qml` so Qt can resolve the wxdash URI at runtime without relying on rpath-relative discovery.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Replaced qt_generate_deploy_qml_app_script with manual QML module install**
- **Found during:** Task 1 (initial install verification)
- **Issue:** `qt_generate_deploy_qml_app_script` generates a deploy script that on Linux desktop systems tries to RPATH-patch all copied Qt plugins (including KDE image plugins like kimg_ani.so). kimg_ani.so has no ELF RPATH/RUNPATH entry, causing `file RPATH_SET could not write new RPATH` fatal error that aborted the install before service/desktop/icon files were reached.
- **Fix:** Replaced `qt_generate_deploy_qml_app_script` block with `install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash/ ...)` targeting only the user QML module files (*.qml, qmldir, *.qmltypes). Added `QML_IMPORT_PATH=@CMAKE_INSTALL_PREFIX@/qml` to service template.
- **Files modified:** src/CMakeLists.txt, assets/wxdash.service.in
- **Verification:** `cmake --install build --prefix /tmp/wxdash-install-test --component kiosk` exits 0, all expected files present, ExecStart path substituted correctly
- **Committed in:** 0bf1d14 (Task 1 commit — fix applied before first commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 - Bug)
**Impact on plan:** Auto-fix necessary for correctness — the deploy script approach is fundamentally incompatible with Linux desktop development systems. The manual install approach is cleaner and more appropriate for Pi deployment where system Qt packages provide Qt runtime.

## Issues Encountered

- `qt_generate_deploy_qml_app_script` with `NO_UNSUPPORTED_PLATFORM_ERROR` and `DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM` still ran the full generic Qt deploy tool on Linux, which attempted to bundle and re-RPATH all system Qt plugins. The `NO_UNSUPPORTED_PLATFORM_ERROR` flag only prevents a compile-time error, not the runtime RPATH behavior. Manual QML module install is the correct approach for a Pi deployment that relies on system Qt.

## User Setup Required

None - no external service configuration required. Pi deployment instructions will be added to README (tracked in STATE.md Pending Todos).

## Next Phase Readiness

- CMake install infrastructure complete — `cmake --install build --component kiosk` deploys binary, QML module, systemd service, EGLFS config, and desktop entries to any prefix
- Pi deployment: copy install tree to Pi, run `systemctl enable --now wxdash.service`; Pi needs Qt6 packages, mesa-utils (V3D driver), and `gpu_mem=128` in /boot/firmware/config.txt
- README section documenting Pi-specific deployment steps is the remaining pending todo

## Self-Check: PASSED

All claimed files verified to exist:
- FOUND: CMakeLists.txt
- FOUND: src/CMakeLists.txt
- FOUND: assets/wxdash.service.in
- FOUND: assets/eglfs.json
- FOUND: assets/wxdash-kiosk.desktop.in
- FOUND: 05-01-SUMMARY.md

All commits verified:
- FOUND: 0bf1d14 (feat(05-01): add CMake install rules and kiosk deployment assets)

---
*Phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment*
*Completed: 2026-03-01*
