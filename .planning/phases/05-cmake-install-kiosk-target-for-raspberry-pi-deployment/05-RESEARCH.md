# Phase 5: CMake install-kiosk target for Raspberry Pi deployment - Research

**Researched:** 2026-03-01
**Domain:** CMake install rules, Qt6 Linux deployment, systemd service files, Raspberry Pi EGLFS/KMS runtime configuration
**Confidence:** HIGH (CMake install API, Qt6 deploy API, EGLFS env vars) / MEDIUM (Pi-specific group memberships, eglfs_kms card selection) / LOW (Bookworm-specific kiosk pitfalls — single community source)

---

## Summary

Phase 5 installs the wxdash application and a systemd unit into a filesystem layout that lets the Raspberry Pi auto-start the dashboard as a kiosk at boot. There are three distinct sub-problems: (1) adding CMake `install()` rules so `cmake --install` places the binary, QML plugins, assets, and the .desktop file at correct FHS-compliant paths; (2) generating a systemd service file from a template, installing it, and optionally running `systemctl enable`; (3) documenting the Pi-side runtime prerequisites (groups, eglfs.json, gpu_mem) in the README (per the existing STATE.md pending todo).

Qt 6.5+ introduced a Linux deployment API (`qt_generate_deploy_qml_app_script`), but it is officially unsupported on Linux embedded targets — it will produce a fatal CMake error unless `NO_UNSUPPORTED_PLATFORM_ERROR` is passed, and even then it does nothing useful beyond deploying user-built QML modules. Since wxdash is compiled natively on the Pi and the Pi's system Qt6 packages supply the shared libraries system-wide, the correct deployment strategy is: install the binary + QML module + assets via standard `install()` rules, and skip the Qt deploy script (or use it with `NO_UNSUPPORTED_PLATFORM_ERROR` + `DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM` to ensure the `wxdash` QML module's type registration files land in the right place). The resulting install prefix is `/usr/local` by default, which is fine for a Pi that compiles natively.

The systemd service needs `QT_QPA_PLATFORM=eglfs` and the user in the `render` and `video` groups to access `/dev/dri/cardN`. The Pi 4/5 uses Mesa V3D rather than the Broadcom proprietary blob; the correct DRM device is typically `/dev/dri/card1` (the render node, not card0 which is the 3D-only node). Input events (keyboard/mouse/touch) require the `input` group for `/dev/input/*` access. The service runs as a system service with `Type=simple`, `Restart=always`, and `WantedBy=graphical.target` (or `multi-user.target`).

**Primary recommendation:** Three plans — (1) CMake `install()` rules + Release build configuration, (2) systemd service file (template → configure_file → install → enable), (3) README section documenting Pi runtime prerequisites. The CMake side is straightforward standard CMake. The operational complexity is all on the Pi side.

---

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| CMake `install()` | 3.22+ (project minimum) | Place binary, assets, QML module at install prefix | CMake built-in; GNUInstallDirs provides FHS-compliant destination variables |
| `include(GNUInstallDirs)` | CMake 3.22+ | Provides `CMAKE_INSTALL_BINDIR` (bin), `CMAKE_INSTALL_DATAROOTDIR` (share), etc. | GNU/FHS standard; required for correct paths on Linux |
| `configure_file()` | CMake 3.22+ | Substitute `@CMAKE_INSTALL_PREFIX@` into systemd service template at configure time | Standard pattern for service files that embed the installed binary path |
| `install(TARGETS)` | CMake 3.22+ | Install the `wxdash` executable | Canonical CMake target install |
| `install(FILES)` | CMake 3.22+ | Install .desktop, icons, systemd unit | Standard CMake file install |
| `qt_generate_deploy_qml_app_script` | Qt 6.5+ | Deploy user-built QML modules (NO_UNSUPPORTED_PLATFORM_ERROR) | Qt's official deploy API for QML apps; Linux embedded is "unsupported" but project QML modules still deploy |
| systemd service unit | N/A | Auto-start kiosk at boot with Restart=always | Standard Linux service management on Pi OS |

### Supporting

| Tool / Variable | Version | Purpose | When to Use |
|-----------------|---------|---------|-------------|
| `CMAKE_INSTALL_BINDIR` | GNUInstallDirs | Resolves to `bin` (prefix-relative) | Destination for wxdash executable |
| `CMAKE_INSTALL_DATAROOTDIR` | GNUInstallDirs | Resolves to `share` | Base for .desktop and icon destinations |
| `CMAKE_BUILD_TYPE=Release` | CMake | Strip debug symbols, enable -O2 | Required for Pi deployment — Debug builds are 3-5x larger and slower |
| `pkg-config systemd --variable=systemdsystemunitdir` | pkg-config | Returns `/usr/lib/systemd/system` | Canonical way to discover systemd unit install dir without hardcoding |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| configure_file for service template | install(CODE) to write service at install time | configure_file at cmake time is simpler but bakes the prefix at configure time (not install time). For native Pi builds this is fine since the prefix is known. install(CODE) handles DESTDIR correctly but is significantly more complex. configure_file is the correct choice for this project. |
| System-level service (`/etc/systemd/system/`) | User-level service (`~/.config/systemd/user/`) | System service runs as a dedicated user or `pi`, starts before login, correct for kiosk. User service requires lingering enabled. System service is the standard kiosk pattern. |
| `WantedBy=graphical.target` | `WantedBy=multi-user.target` | `graphical.target` is semantically correct for display apps. `multi-user.target` works but starts the app even without a display. Either works on Pi OS; `multi-user.target` is safer for headless fallback. |
| qt_generate_deploy_qml_app_script | Manually install QML files | Qt's deploy script handles type registrations and QML plugin directories correctly. Even on "unsupported" Linux, DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM ensures the wxdash QML module lands in the right place relative to the binary. |

**No new CMake packages, Qt modules, or system libraries needed.** GNUInstallDirs is bundled with CMake. Qt6's deploy API is already available through Qt6::Core (qt_standard_project_setup enables it).

---

## Architecture Patterns

### Recommended Project Structure (files to add/modify)

```
wxdash/
├── CMakeLists.txt                     # Add: include(GNUInstallDirs), top-level install rules
├── src/
│   └── CMakeLists.txt                 # Add: install(TARGETS wxdash ...), qt_generate_deploy_qml_app_script
└── assets/
    ├── wxdash.desktop                 # Update: Exec= to use installed path (or keep as template)
    ├── wxdash-kiosk.desktop           # Update: Exec= to use installed path
    ├── wxdash.svg                     # Already exists — just needs install(FILES) rule
    └── wxdash.service.in              # NEW: systemd unit template
```

### Pattern 1: CMake install() Rules in src/CMakeLists.txt

**What:** Install the wxdash executable and let qt_generate_deploy_qml_app_script handle QML module deployment. Install assets and .desktop separately.

**When to use:** All Qt6 QML applications targeting Linux.

**Example:**
```cmake
# Source: Qt6 cmake-deployment docs + GNUInstallDirs standard
include(GNUInstallDirs)

# Install the executable
install(TARGETS wxdash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Deploy QML module (user-built wxdash QML module only — no runtime deps on Linux)
qt_generate_deploy_qml_app_script(
    TARGET wxdash
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})
```

**Note:** `NO_UNSUPPORTED_PLATFORM_ERROR` prevents a fatal CMake error on Linux embedded. `DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM` ensures the `wxdash` QML module (Main.qml, ArcGauge.qml, etc.) is deployed alongside the binary.

### Pattern 2: Asset Installation

**What:** Install the .desktop files and SVG icon to FHS-standard paths.

**Example (in CMakeLists.txt, top-level or assets subdirectory):**
```cmake
# .desktop files → share/applications/
install(FILES
    assets/wxdash.desktop
    assets/wxdash-kiosk.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

# SVG icon → share/icons/hicolor/scalable/apps/
install(FILES
    assets/wxdash.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
)
```

**Note on .desktop Exec= path:** The existing `wxdash-kiosk.desktop` hardcodes the build directory path. Before installing, the Exec= line must reflect the installed path. Two options:
1. Treat it as a template (rename to `wxdash-kiosk.desktop.in`), use `configure_file()` to substitute `${CMAKE_INSTALL_PREFIX}/bin/wxdash --kiosk`, then install the configured output.
2. Or simply install and document that users run `systemctl` directly (desktop files are less important for a kiosk).

### Pattern 3: systemd Service File (configure_file → install)

**What:** A service file template with `@CMAKE_INSTALL_PREFIX@` placeholder, configured at CMake configure time, installed via `install(FILES)`.

**Template file — `assets/wxdash.service.in`:**
```ini
[Unit]
Description=wxdash Weather Kiosk
Documentation=https://github.com/user/wxdash
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pi
Group=pi
SupplementaryGroups=render video input
WorkingDirectory=@CMAKE_INSTALL_PREFIX@/bin
ExecStart=@CMAKE_INSTALL_PREFIX@/bin/wxdash --kiosk

# EGLFS/KMS environment — Pi 4/5 with Mesa V3D driver
Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_INTEGRATION=eglfs_kms
Environment=QT_QPA_EGLFS_KMS_CONFIG=@CMAKE_INSTALL_PREFIX@/share/wxdash/eglfs.json
Environment=QT_QPA_KMS_DEVICE=/dev/dri/card1

# Qt paths for non-system installs (adjust if Qt is system-installed)
# Environment=LD_LIBRARY_PATH=@CMAKE_INSTALL_PREFIX@/lib
# Environment=QT_QPA_PLATFORM_PLUGIN_PATH=@CMAKE_INSTALL_PREFIX@/plugins/platforms

# Restart on crash (KIOSK-05)
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

**CMakeLists.txt — configure_file + install:**
```cmake
# Configure the service file (substitutes @CMAKE_INSTALL_PREFIX@)
configure_file(
    ${CMAKE_SOURCE_DIR}/assets/wxdash.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/wxdash.service
    @ONLY
)

# Install to systemd system unit directory
# Use pkg-config to find the correct path, or default to /usr/lib/systemd/system
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/wxdash.service
    DESTINATION lib/systemd/system
)
```

**Note on systemd unit path:** Hardcoding `lib/systemd/system` as the destination is correct when `CMAKE_INSTALL_PREFIX=/usr` (giving `/usr/lib/systemd/system`). With prefix `/usr/local`, the unit lands at `/usr/local/lib/systemd/system` — which systemd does scan. Verify with `systemd-analyze unit-paths`. Alternatively, use `install(CODE)` with `pkg-config --variable=systemdsystemunitdir systemd` to find the canonical path.

### Pattern 4: Custom `install-kiosk` CMake Target

**What:** A top-level CMake custom target that runs `cmake --install` with a kiosk-specific component and then enables the systemd unit. This is the "install-kiosk" target the phase name references.

**Example:**
```cmake
# In top-level CMakeLists.txt
add_custom_target(install-kiosk
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
            --config Release
            --component kiosk
    COMMAND systemctl daemon-reload
    COMMAND systemctl enable wxdash.service
    COMMENT "Installing wxdash kiosk and enabling systemd service"
    VERBATIM
)
```

**Critical issue:** `add_custom_target` with `systemctl` commands requires sudo/root. The target is a convenience shortcut, not a replacement for `cmake --install`. The standard pattern in production is:
```bash
sudo cmake --install build --config Release
sudo systemctl enable wxdash.service
```
The custom target is optional convenience; the install rules themselves are mandatory.

### Pattern 5: CMake COMPONENT Scoping (optional)

**What:** Use CMake components to allow selective installation.

```cmake
install(TARGETS wxdash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT kiosk
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wxdash.service
    DESTINATION lib/systemd/system
    COMPONENT kiosk
)

install(FILES assets/wxdash.desktop assets/wxdash-kiosk.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    COMPONENT kiosk
)
```

Then `cmake --install build --component kiosk` installs only kiosk files.

### Anti-Patterns to Avoid

- **Hardcoding absolute paths in service ExecStart=:** Always use `@CMAKE_INSTALL_PREFIX@` via configure_file. Hardcoded paths break when deployed to a different prefix.
- **Using qt_generate_deploy_app_script without NO_UNSUPPORTED_PLATFORM_ERROR on Linux:** This causes a CMake fatal error. Always pass the flag.
- **Deploying shared Qt libs manually on Pi with system Qt6:** The Pi OS Qt6 packages are system-wide — there is no need to bundle Qt .so files. The deploy script on Linux doesn't copy system libs anyway.
- **Running systemd GUI apps as root:** The service should run as `User=pi` (or whichever user account owns the display). Root is unnecessary and a security risk.
- **Omitting SupplementaryGroups from the service:** Without `render` and `video` groups, `/dev/dri/cardN` is inaccessible and EGLFS silently falls back to software rendering or crashes.
- **Using card0 instead of card1 on Pi 4/5:** The Pi 4/5 GPU exposes two DRM nodes: `card0` (3D-only, no DRIVER_RENDER feature) and `card1` (render node). Qt EGLFS checks for `DRIVER_RENDER` and will fail or misbehave on card0.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Finding systemd unit install dir | Hardcoded `/etc/systemd/system` | `pkg-config systemd --variable=systemdsystemunitdir` or convention `lib/systemd/system` with correct prefix | Hardcoded paths are distribution-specific and wrong under DESTDIR |
| Service file path substitution | Manual string replacement in CMake | `configure_file(@ONLY)` | configure_file is the standard CMake mechanism; handles all edge cases |
| Qt QML module deployment | Manually copying .qmltypes and type registration files | `qt_generate_deploy_qml_app_script(... DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM)` | Qt knows exactly what files the QML module needs; manual copy misses type registrations |
| Release build stripping | Custom post-build strip commands | `cmake -DCMAKE_BUILD_TYPE=Release` | CMake handles strip flags via CMAKE_BUILD_TYPE; no custom commands needed |
| Icon theme registration | Custom gtk-update-icon-cache calls | `install(FILES ... DESTINATION share/icons/hicolor/...)` | Package manager or `update-icon-caches` handles cache; CMake install just places files correctly |

**Key insight:** Phase 5 is almost entirely configuration — correct CMake `install()` paths and a valid systemd template. The complexity is operational (Pi groups, eglfs.json), not implementation.

---

## Common Pitfalls

### Pitfall 1: configure_file bakes the PREFIX at configure time, not install time

**What goes wrong:** `configure_file()` substitutes `CMAKE_INSTALL_PREFIX` at `cmake -B build` time. If the user configures with prefix `/usr/local` but installs with `cmake --install build --prefix /opt/wxdash`, the service file still points to `/usr/local/bin/wxdash`.

**Why it happens:** configure_file runs at CMake configuration time. DESTDIR and `--prefix` overrides are install-time concepts.

**How to avoid:** For native Pi builds (where developer runs cmake + make + install locally), this is not a problem — just document that the configured prefix must match the installed prefix. If install-time prefix override is needed, use `install(CODE ...)` with CMake scripting or CPack. For this project's use case (native Pi, fixed prefix), configure_file is correct and sufficient.

**Warning signs:** App refuses to start from systemd service; `ExecStart` in the installed unit points to the wrong path.

### Pitfall 2: Pi 4/5 EGLFS uses card1, not card0

**What goes wrong:** `QT_QPA_EGLFS_INTEGRATION=eglfs_kms` without `QT_QPA_KMS_DEVICE=/dev/dri/card1` may pick up card0 (the 3D-only DRM node), which lacks `DRIVER_RENDER`. Qt's device probe fails silently or produces a black screen.

**Why it happens:** The Pi 4/5 VideoCore VI GPU exposes two DRM nodes: card0 is the HEVC/V4L2 side; card1 is the V3D render node. Qt EGLFS probes both and looks for a node with `DRIVER_RENDER` capability — but automatic discovery can be unreliable.

**How to avoid:** Set `QT_QPA_KMS_DEVICE=/dev/dri/card1` explicitly in the service file. Alternatively, use an eglfs.json config: `{"device": "/dev/dri/card1"}` pointed to by `QT_QPA_EGLFS_KMS_CONFIG`.

**Warning signs:** Black screen at EGLFS start; Qt log shows "Could not find a suitable DRM device" or "Cannot perform DRM page flip".

**Confidence:** MEDIUM — confirmed by multiple community sources (Qt forum, GitHub firmware issues) but exact node numbering varies by Pi OS version and GPU firmware.

### Pitfall 3: User not in render/video/input groups

**What goes wrong:** EGLFS crashes with "Permission denied" on `/dev/dri/card1`. Input events from keyboard/mouse never arrive.

**Why it happens:** The Pi's EGLFS/DRM path requires group membership: `render` for `/dev/dri/renderD128` and direct DRM access, `video` for framebuffer legacy, `input` for `/dev/input/event*` libinput access.

**How to avoid:** Service file uses `SupplementaryGroups=render video input`. If running as a real user account (not a dedicated kiosk user), also: `sudo usermod -aG render,video,input pi`.

**Warning signs:** `[drm] failed to open DRM device: Permission denied` in systemd journal; no keyboard/touch input in the kiosk.

### Pitfall 4: QML module not found at runtime after install

**What goes wrong:** wxdash crashes on startup with "module 'wxdash' is not installed" or similar QML error, even though the binary installed successfully.

**Why it happens:** `qt_add_qml_module` generates type registrations and a QML module directory that must be discoverable at runtime via `QT2_QML_IMPORT_PATH` or relative to the binary. Qt uses `qt.conf` or `$ORIGIN/../qml` conventions to locate QML modules. Without the deploy script, the QML module files may not land in the right place.

**How to avoid:** Use `qt_generate_deploy_qml_app_script(... DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM)`. This places the wxdash QML module relative to the installed binary where Qt's QML import scanner expects it. Alternatively, set `QML2_IMPORT_PATH` or `QML_IMPORT_PATH` in the service file to point to the module directory.

**Warning signs:** Application exits immediately after launch; `journalctl -u wxdash` shows QML import errors.

### Pitfall 5: Service running before network is available (mDNS hostname)

**What goes wrong:** wxdash starts before the network is up; `weatherlinklive.local.cisien.com` fails to resolve. The app continues with empty data but never recovers if mDNS was unavailable at the first DNS query.

**Why it happens:** The main.cpp hardcodes the hostname in the QUrl. The UDP and HTTP pollers retry, so recovery IS built in (DATA-09). But if Avahi/mDNS doesn't start until after wxdash, the first round of retries may all fail.

**How to avoid:** Add `After=network-online.target` and `Wants=network-online.target` in the service `[Unit]` section. On Raspberry Pi OS, also ensure `systemctl enable NetworkManager-wait-online.service` is active (Bookworm uses NetworkManager by default).

**Note on mDNS:** The STATE.md pending todo flags that `weatherlinklive.local.cisien.com` uses an unusual `.local.cisien.com` pattern. Avahi on Pi OS Bookworm handles `.local` mDNS; `.local.cisien.com` is NOT a plain mDNS name — it requires standard DNS resolution (or /etc/hosts entry). Verify DNS resolution on the Pi before assuming Avahi will handle it. This is a Pi deployment operational concern, not a CMake issue.

**Warning signs:** Weather data never appears; systemd journal shows connection refused or hostname resolution failures in the first 30 seconds.

### Pitfall 6: Debug build deployed to Pi

**What goes wrong:** Debug binaries are much larger (3-5x) and have no compiler optimizations. Qt Quick animations are sluggish; memory usage is higher. wxdash may trigger Pi thermal throttling with Debug Qt.

**Why it happens:** The current CMake cache shows `CMAKE_BUILD_TYPE=Debug`. Deployment should always use `Release`.

**How to avoid:** Reconfigure with `cmake -B build -DCMAKE_BUILD_TYPE=Release` before building for deployment. Document this in the README. The `install-kiosk` custom target could enforce this by passing `--config Release` to the install command.

**Warning signs:** Binary size > 10 MB; high CPU idle load on Pi.

---

## Code Examples

Verified patterns from CMake documentation and project analysis:

### Complete src/CMakeLists.txt install additions

```cmake
# Source: Qt6 cmake-deployment.html + GNUInstallDirs CMake module
include(GNUInstallDirs)

# Install executable
install(TARGETS wxdash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT kiosk
)

# Deploy user-built QML module (wxdash URI → Main.qml, ArcGauge.qml, etc.)
qt_generate_deploy_qml_app_script(
    TARGET wxdash
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script}
    COMPONENT kiosk
)
```

### Top-level CMakeLists.txt additions

```cmake
# Source: CMake install() documentation
include(GNUInstallDirs)

# Configure systemd service (bakes CMAKE_INSTALL_PREFIX)
configure_file(
    ${CMAKE_SOURCE_DIR}/assets/wxdash.service.in
    ${CMAKE_BINARY_DIR}/wxdash.service
    @ONLY
)

# Install systemd unit
install(FILES ${CMAKE_BINARY_DIR}/wxdash.service
    DESTINATION lib/systemd/system
    COMPONENT kiosk
)

# Install desktop entry files
configure_file(
    ${CMAKE_SOURCE_DIR}/assets/wxdash-kiosk.desktop.in
    ${CMAKE_BINARY_DIR}/wxdash-kiosk.desktop
    @ONLY
)
install(FILES
    assets/wxdash.desktop
    ${CMAKE_BINARY_DIR}/wxdash-kiosk.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    COMPONENT kiosk
)

# Install icon
install(FILES assets/wxdash.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    COMPONENT kiosk
)

# Convenience install-kiosk target
add_custom_target(install-kiosk
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config Release --component kiosk
    COMMAND systemctl daemon-reload
    COMMAND systemctl enable wxdash.service
    COMMENT "Installing wxdash kiosk + enabling systemd service (requires sudo)"
    VERBATIM
)
```

### wxdash-kiosk.desktop.in template

```ini
[Desktop Entry]
Type=Application
Name=wxdash (Kiosk)
Comment=Weather dashboard for WeatherLink Live — fullscreen kiosk mode
Exec=@CMAKE_INSTALL_PREFIX@/bin/wxdash --kiosk
Icon=wxdash
Terminal=false
Categories=Utility;Monitor;
StartupWMClass=wxdash
```

### eglfs.json for Pi 4/5 (to install to share/wxdash/)

```json
{
  "device": "/dev/dri/card1",
  "outputs": [
    {
      "name": "HDMI1",
      "primary": true
    }
  ]
}
```

Install with:
```cmake
install(FILES assets/eglfs.json
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wxdash
    COMPONENT kiosk
)
```

### Pi deployment command sequence (README content)

```bash
# On the Raspberry Pi, in the wxdash source directory:

# 1. Configure Release build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local

# 2. Build
cmake --build build

# 3. Install (requires sudo for /usr/local/bin)
sudo cmake --install build --component kiosk

# 4. Add user to required groups (if not already done)
sudo usermod -aG render,video,input pi

# 5. Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable wxdash.service
sudo systemctl start wxdash.service

# Or use the convenience target (also requires sudo):
sudo cmake --build build --target install-kiosk
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| linuxdeployqt (external tool) | qt_generate_deploy_qml_app_script (built-in) | Qt 6.5 (2023) | No external tooling needed; but Linux support is partial |
| EGLFS brcm (Broadcom blob) | EGLFS KMS + Mesa V3D | Pi 4 (2019) / mainstream Qt6 | eglfs_brcm is Pi 1-3 only; Pi 4/5 must use eglfs_kms with Mesa |
| Manual service file copy | configure_file + install(FILES) | N/A (CMake standard) | configure_file correctly substitutes installation paths |
| `WantedBy=default.target` | `WantedBy=multi-user.target` or `graphical.target` | systemd modern | multi-user.target is the correct kiosk target for headless EGLFS apps |

**Deprecated/outdated:**
- linuxdeployqt: No Qt6 support, abandoned. Do not use.
- `eglfs_brcm` platform plugin: Pi 4+ uses Mesa V3D; eglfs_brcm targets the legacy Broadcom EGL blob which is no longer in Pi OS.
- `/dev/fb0` via `QT_QPA_EGLFS_FB`: Works with linuxfb platform but not with eglfs_kms. Use the DRM path instead.

---

## Open Questions

1. **Does `qt_generate_deploy_qml_app_script` with `DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM` correctly deploy the wxdash QML module directory on Linux?**
   - What we know: Qt docs say user QML modules deploy with this flag. The wxdash module uses `qt_add_qml_module` with `OUTPUT_DIRECTORY` set to `${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash`. The deploy script should mirror this relative path next to the installed binary.
   - What's unclear: The exact relative path the deploy script places the module vs. where Qt's import scanner looks. May need `QML2_IMPORT_PATH` in the service if the auto-discovery fails.
   - Recommendation: Test `cmake --install build` to a temp prefix and verify the QML module directory exists relative to the binary. If not, set `QML2_IMPORT_PATH=${CMAKE_INSTALL_PREFIX}/qml` in the service file.

2. **Does the mDNS hostname `weatherlinklive.local.cisien.com` resolve on Pi OS Bookworm without special configuration?**
   - What we know: STATE.md flags this as a pending concern. `.local.cisien.com` is not a pure mDNS name (`.local` suffix only). It requires a DNS server that knows this name — either a local router/DNS entry or `/etc/hosts` fallback.
   - What's unclear: Whether the user's router serves this hostname via DNS. Avahi will NOT resolve `.local.cisien.com` — only `.local` names.
   - Recommendation: Document in README: verify `nslookup weatherlinklive.local.cisien.com` on the Pi. If unresolved, add to `/etc/hosts` or configure as local DNS entry. This is a deployment operational item, not a CMake implementation item.

3. **Should `install-kiosk` be a custom target or an install component?**
   - What we know: CMake custom targets can run arbitrary commands; CMake install components partition install artifacts.
   - What's unclear: Whether the user wants `cmake --build --target install-kiosk` (runs as part of build) or `cmake --install --component kiosk` (runs as part of install phase).
   - Recommendation: Both. Use `COMPONENT kiosk` on all install() rules for selective installation. Add an `install-kiosk` custom target as a convenience that runs install + systemctl. Document both approaches. The custom target is developer convenience; the component is the robust mechanism.

4. **Pi 4 vs Pi 5 DRM device node numbering**
   - What we know: Pi 4 typically has card0 (V3D 3D-only) and card1 (render). Pi 5 uses a different GPU (VideoCore VII) — device node numbering may differ.
   - What's unclear: Whether Pi 5 also uses card1 as the primary render node, or has different numbering.
   - Recommendation: The eglfs.json approach is correct: set the device explicitly and document that the user may need to check `ls /dev/dri/` and run `sudo journalctl -u wxdash` to identify the correct node. Default to card1 in the template; document how to override.

---

## Validation Architecture

> `workflow.nyquist_validation` is not present in `.planning/config.json` — skip automated test mapping. Phase 5 is deployment infrastructure with no new C++ logic; validation is operational (deploy to Pi, confirm kiosk starts).

This phase has no new C++ or QML code. Validation consists of:
- `cmake --install build --prefix /tmp/wxdash-test` — verify directory structure correct
- On Pi: `systemctl status wxdash` — service active and kiosk visible on screen
- Manual/operational: not automatable in CI

---

## Sources

### Primary (HIGH confidence)
- [Qt6 CMake Deployment docs](https://doc.qt.io/qt-6/cmake-deployment.html) — install(TARGETS), qt_generate_deploy_qml_app_script patterns, Linux deploy notes
- [Qt6 qt_generate_deploy_qml_app_script reference](https://doc.qt.io/qt-6/qt-generate-deploy-qml-app-script.html) — argument list, NO_UNSUPPORTED_PLATFORM_ERROR behavior, DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
- [Qt6 Embedded Linux docs](https://doc.qt.io/qt-6/embedded-linux.html) — EGLFS backends, env vars, eglfs.json format, device nodes
- [CMake install() command documentation](https://cmake.org/cmake/help/latest/command/install.html) — TARGETS, FILES, CODE, SCRIPT, COMPONENT
- [CMake GNUInstallDirs documentation](https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html) — CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_DATAROOTDIR, etc.
- Project codebase (src/CMakeLists.txt, assets/wxdash-kiosk.desktop) — existing structure, .desktop paths, build patterns

### Secondary (MEDIUM confidence)
- [Qt Deploying to Linux with CMake blog](https://www.qt.io/blog/deploying-to-linux-with-cmake) — Linux-specific deployment behavior, qt.conf, $ORIGIN rpath setup
- Community finding (multiple Qt forums, GitHub firmware issues): Pi 4/5 uses card1 (render node, DRIVER_RENDER present) vs card0 (3D-only); render+video groups required for DRM access
- [Interelectronix: Raspberry Pi 4 Qt autostart at boot](https://www.interelectronix.com/raspberry-pi-4-autostart-qt-application-during-boot.html) — systemd service file pattern (After=graphical.target, User=pi, WantedBy=multi-user.target)
- `pkg-config systemd` on dev machine: returns `/usr/lib/systemd/system` for systemdsystemunitdir — standard path for system units

### Tertiary (LOW confidence)
- Various Raspberry Pi forum threads (2023-2024): SupplementaryGroups=render video in service file pattern — seen mentioned but not from an authoritative Qt or Raspberry Pi official source; requires Pi-side verification
- Qt forum discussion: Pi 5 DRM device numbering — conflicting/absent information; defer to runtime discovery

---

## Metadata

**Confidence breakdown:**
- Standard Stack (CMake install rules, GNUInstallDirs): HIGH — CMake official docs, well-established patterns
- Qt deploy API (NO_UNSUPPORTED_PLATFORM_ERROR): HIGH — Qt official docs, behavior clearly documented
- EGLFS env vars (QT_QPA_PLATFORM, QT_QPA_EGLFS_KMS_CONFIG): HIGH — Qt official embedded-linux docs
- EGLFS device (card1 on Pi 4/5): MEDIUM — multiple community sources agree but no single authoritative source; validate on Pi hardware
- User groups (render, video, input in SupplementaryGroups): MEDIUM — community-confirmed, aligns with DRM permissions model; validate on Pi
- Pi 5 DRM specifics: LOW — insufficient data; defer to runtime discovery and README documentation

**Research date:** 2026-03-01
**Valid until:** 2026-04-01 (CMake and Qt APIs stable; Pi OS Bookworm specifics valid for ~30 days given active kernel/firmware updates)
