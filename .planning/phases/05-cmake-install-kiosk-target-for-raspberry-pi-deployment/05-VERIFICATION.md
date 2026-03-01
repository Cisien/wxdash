---
phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment
verified: 2026-03-01T23:30:00Z
status: passed
score: 4/4 must-haves verified
re_verification: null
gaps: []
human_verification:
  - test: "Run wxdash binary on an actual Raspberry Pi 4/5 with eglfs_kms backend"
    expected: "Dashboard renders fullscreen on HDMI output with live weather data"
    why_human: "x86-64 dev machine cannot exercise DRM/KMS/EGLFS stack; Pi hardware required to confirm QT_QPA_PLATFORM=eglfs and /dev/dri/card1 work correctly"
  - test: "Execute `sudo cmake --build build --target install-kiosk` on Pi, then `systemctl status wxdash.service`"
    expected: "Service is active and wxdash displays on screen; `systemctl enable` persisted the unit for boot"
    why_human: "systemctl commands require root on target hardware; cannot simulate on dev machine"
  - test: "Reboot Pi and verify wxdash auto-starts"
    expected: "wxdash appears on display within ~10 seconds of boot without manual intervention"
    why_human: "Boot-time auto-start can only be confirmed on real hardware after reboot"
---

# Phase 5: CMake Install-Kiosk Target for Raspberry Pi Deployment — Verification Report

**Phase Goal:** `cmake --install build --component kiosk` deploys the wxdash binary, QML module, systemd service, EGLFS config, and desktop files to FHS-compliant paths, enabling one-command Pi kiosk deployment with auto-start at boot
**Verified:** 2026-03-01T23:30:00Z
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `cmake --install build --component kiosk` places binary, QML module, systemd service, eglfs.json, desktop files, and icon at correct FHS paths | VERIFIED | Install ran to completion: bin/wxdash, lib/systemd/system/wxdash.service, share/wxdash/eglfs.json, share/applications/wxdash-kiosk.desktop, share/icons/hicolor/scalable/apps/wxdash.svg — all present at correct paths |
| 2 | Installed systemd service file has correct ExecStart path and EGLFS environment variables | VERIFIED | Configured service contains `ExecStart=/tmp/wxdash-verify/bin/wxdash --kiosk`, `QT_QPA_PLATFORM=eglfs`, `QT_QPA_EGLFS_INTEGRATION=eglfs_kms`, `QT_QPA_EGLFS_KMS_CONFIG=.../share/wxdash/eglfs.json` — all substituted from template |
| 3 | README documents the complete Pi deployment workflow from build through systemctl enable | VERIFIED | README.md (164 lines) covers prerequisites, build/install commands, user groups, EGLFS config, systemd service lifecycle, hostname resolution, and troubleshooting table |
| 4 | install-kiosk convenience target exists for one-command deploy + enable | VERIFIED | `add_custom_target(install-kiosk ...)` defined in CMakeLists.txt lines 76-82 with cmake --install, systemctl daemon-reload, and systemctl enable commands |

**Score:** 4/4 truths verified

---

## Required Artifacts

### Plan 05-01 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `CMakeLists.txt` | Top-level install rules: GNUInstallDirs, configure_file for service and desktop, install-kiosk target | VERIFIED | Lines 19-82: include(GNUInstallDirs), SERVICE_USER variable, configure_file(@ONLY) for both templates, install() rules for service/desktop/icon/eglfs.json, add_custom_target(install-kiosk) |
| `src/CMakeLists.txt` | Target install rules and QML deploy | VERIFIED | Lines 41-61: install(TARGETS wxdash RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT kiosk) + manual install(DIRECTORY) for QML module |
| `assets/wxdash.service.in` | systemd service template with @CMAKE_INSTALL_PREFIX@ placeholders | VERIFIED | Contains ExecStart=@CMAKE_INSTALL_PREFIX@/bin/wxdash, WorkingDirectory, QT_QPA_PLATFORM=eglfs, QT_QPA_EGLFS_INTEGRATION=eglfs_kms, QT_QPA_EGLFS_KMS_CONFIG, QML_IMPORT_PATH, Restart=always |
| `assets/wxdash-kiosk.desktop.in` | Desktop entry template with installed path | VERIFIED | Exec=@CMAKE_INSTALL_PREFIX@/bin/wxdash --kiosk correctly templated |
| `assets/eglfs.json` | Pi 4/5 EGLFS KMS device configuration | VERIFIED | Contains "device": "/dev/dri/card1" and HDMI1 primary output |

### Plan 05-02 Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `README.md` | Complete project README with Pi kiosk deployment documentation | VERIFIED | 164 lines; contains "Raspberry Pi Deployment" section header, systemctl commands (8+ occurrences), eglfs.json references, render,video,input group documentation |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `CMakeLists.txt` | `assets/wxdash.service.in` | configure_file(@ONLY) | VERIFIED | Line 35-39: `configure_file(${CMAKE_SOURCE_DIR}/assets/wxdash.service.in ${CMAKE_BINARY_DIR}/wxdash.service @ONLY)` — confirmed by install output showing substituted ExecStart path |
| `src/CMakeLists.txt` | `bin/wxdash` (installed) | install(TARGETS wxdash RUNTIME DESTINATION) | VERIFIED | Line 45-48 installs binary; confirmed by `/tmp/wxdash-verify/bin/wxdash` (ELF 64-bit, 1016248 bytes) |
| `CMakeLists.txt` | `assets/eglfs.json` | install(FILES) | VERIFIED | Line 69-72: `install(FILES assets/eglfs.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wxdash COMPONENT kiosk)` — confirmed at share/wxdash/eglfs.json |
| `README.md` | `assets/wxdash.service.in` | Documents service commands | VERIFIED | README contains `systemctl enable wxdash.service`, `systemctl start wxdash.service`, daemon-reload — matches service template's WantedBy=multi-user.target intent |
| `README.md` | `assets/eglfs.json` | Documents EGLFS config and device node | VERIFIED | README section "EGLFS Configuration" references `/usr/local/share/wxdash/eglfs.json`, explains card1 device node, documents how to override |
| `assets/wxdash.service.in` | `bin/wxdash` (runtime) | QML_IMPORT_PATH env var | VERIFIED | Service sets `QML_IMPORT_PATH=@CMAKE_INSTALL_PREFIX@/qml`; QML module installs to prefix/qml/wxdash/; qmldir present at correct path for URI wxdash resolution |

---

## Requirements Coverage

| Requirement | Source Plan(s) | Description | Status | Evidence |
|-------------|----------------|-------------|--------|----------|
| KIOSK-05 | 05-01-PLAN.md, 05-02-PLAN.md | systemd watchdog with auto-restart on crash | SATISFIED | `Restart=always` and `RestartSec=5` in wxdash.service.in (line 20-21); comment explicitly tags KIOSK-05; install infrastructure deploys the service unit |

**Note on requirement-to-phase mapping:** REQUIREMENTS.md maps KIOSK-05 to "Phase 4" in the coverage table, while ROADMAP.md assigns KIOSK-05 to Phase 5. No Phase 4 directory exists in .planning/phases/. The ROADMAP is the authoritative source for phase assignment; the REQUIREMENTS.md coverage table appears to contain a stale phase number. This is a documentation inconsistency only — the requirement itself is implemented and satisfied.

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `README.md` | 59 | `git clone <repo-url> wxdash` | Info | Placeholder repo URL — expected for a project without a published remote yet; does not block deployment workflow |
| `README.md` | 163 | `MIT License — see LICENSE file for details.` | Info | References a LICENSE file that does not exist in the repository | No LICENSE file at `/home/cisien/src/wxdash/LICENSE` | Does not affect kiosk deployment functionality |

Neither anti-pattern is a blocker for the phase goal. Both are informational notes about documentation completeness.

---

## Live Install Verification

The following was confirmed by executing `cmake --install build --prefix /tmp/wxdash-verify --component kiosk` on the development machine:

**Install output (all files present):**
```
/tmp/wxdash-verify/bin/wxdash
/tmp/wxdash-verify/lib/systemd/system/wxdash.service
/tmp/wxdash-verify/qml/wxdash/qmldir
/tmp/wxdash-verify/qml/wxdash/qml/AqiGauge.qml
/tmp/wxdash-verify/qml/wxdash/qml/ArcGauge.qml
/tmp/wxdash-verify/qml/wxdash/qml/CompassRose.qml
/tmp/wxdash-verify/qml/wxdash/qml/DashboardGrid.qml
/tmp/wxdash-verify/qml/wxdash/qml/Main.qml
/tmp/wxdash-verify/qml/wxdash/qml/ReservedCell.qml
/tmp/wxdash-verify/qml/wxdash/wxdash.qmltypes
/tmp/wxdash-verify/share/applications/wxdash.desktop
/tmp/wxdash-verify/share/applications/wxdash-kiosk.desktop
/tmp/wxdash-verify/share/icons/hicolor/scalable/apps/wxdash.svg
/tmp/wxdash-verify/share/wxdash/eglfs.json
```

**Configured service file (all substitutions applied):**
```ini
[Unit]
Description=wxdash Weather Kiosk
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=cisien
SupplementaryGroups=render video input
WorkingDirectory=/tmp/wxdash-verify/bin
ExecStart=/tmp/wxdash-verify/bin/wxdash --kiosk

Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_INTEGRATION=eglfs_kms
Environment=QT_QPA_EGLFS_KMS_CONFIG=/tmp/wxdash-verify/share/wxdash/eglfs.json
Environment=QML_IMPORT_PATH=/tmp/wxdash-verify/qml

Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

**CMake configure output confirms SERVICE_USER detection:**
```
-- wxdash service will run as user: cisien
```

---

## Human Verification Required

### 1. Pi Hardware: EGLFS/KMS Rendering

**Test:** On a Raspberry Pi 4 or 5 running Pi OS Bookworm, install with `sudo cmake --install build --component kiosk` then `sudo systemctl start wxdash.service`.
**Expected:** wxdash dashboard renders fullscreen on the HDMI display using the eglfs_kms backend without any DRM/KMS errors in journalctl.
**Why human:** The x86-64 development machine cannot exercise the EGLFS/KMS stack. The card1 device node and Mesa V3D driver path can only be confirmed on actual Pi hardware.

### 2. Pi Hardware: install-kiosk Target + Boot Auto-Start

**Test:** Run `sudo cmake --build build --target install-kiosk` on the Pi, then reboot.
**Expected:** `systemctl status wxdash.service` shows active (running) after reboot without manual intervention; dashboard displays within ~10 seconds of boot.
**Why human:** The `systemctl daemon-reload` and `systemctl enable` commands in the install-kiosk target require a running systemd instance. Boot persistence can only be confirmed by an actual reboot on target hardware.

### 3. QML Module Resolution at Runtime on Pi

**Test:** After kiosk install on Pi, check `journalctl -u wxdash.service` for any QML import errors.
**Expected:** No "module wxdash not found" or "QML_IMPORT_PATH" errors; all gauge components render correctly.
**Why human:** The QML module is embedded in the binary as Qt resources (prefer :/qt/qml/wxdash/) so it works without the installed module directory. The installed QML module in prefix/qml/wxdash/ serves as an override; runtime behavior on the Pi with system Qt packages needs human confirmation.

---

## Summary

Phase 5 goal is fully achieved at the code level. All four success criteria are met:

1. `cmake --install build --component kiosk` was executed live on this machine and deployed all required files to correct FHS paths — binary at bin/, QML module at qml/wxdash/, systemd service at lib/systemd/system/, eglfs.json at share/wxdash/, desktop files at share/applications/, icon at share/icons/hicolor/scalable/apps/.

2. The installed service file has all required substitutions applied: ExecStart with correct prefix, EGLFS environment variables (QT_QPA_PLATFORM, QT_QPA_EGLFS_INTEGRATION, QT_QPA_EGLFS_KMS_CONFIG), QML_IMPORT_PATH, and Restart=always for crash recovery.

3. README.md provides a complete Pi deployment guide covering prerequisites, build/install commands (including the --component kiosk one-liner and install-kiosk convenience target), user group configuration, EGLFS config, systemd service lifecycle, hostname resolution, and troubleshooting.

4. The install-kiosk custom CMake target is defined and chained correctly: cmake --install + systemctl daemon-reload + systemctl enable.

Two informational notes (placeholder repo URL and missing LICENSE file in README) do not block the deployment goal. Three items require human verification on Pi hardware since the EGLFS/KMS stack and systemd boot persistence cannot be exercised on an x86-64 development machine.

---

_Verified: 2026-03-01T23:30:00Z_
_Verifier: Claude (gsd-verifier)_
