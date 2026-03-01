---
phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment
plan: "02"
subsystem: docs
tags: [readme, deployment, raspberry-pi, systemd, eglfs, cmake, kiosk]

# Dependency graph
requires:
  - phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment
    plan: "01"
    provides: CMake install infrastructure, wxdash.service.in template, eglfs.json, install-kiosk target
provides:
  - README.md with project overview and comprehensive Pi kiosk deployment guide
  - SERVICE_USER CMake variable defaulting to $ENV{USER} at configure time
  - Parameterized systemd service template — no hardcoded username
affects:
  - Pi deployment users following README

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "SERVICE_USER CMake variable: defaults to $ENV{USER} at configure time, overridable with -DSERVICE_USER=<name>, falls back to 'nobody' with warning if env is empty"
    - "configure_file(@ONLY) with @SERVICE_USER@ — safe alongside @CMAKE_INSTALL_PREFIX@, systemd Environment= lines using ${VAR} left untouched"

key-files:
  created:
    - README.md
  modified:
    - assets/wxdash.service.in
    - CMakeLists.txt

key-decisions:
  - "SERVICE_USER defaults to $ENV{USER} (current user running CMake) rather than a hardcoded username — correct for any Pi deployment user, not just 'pi'"
  - "Dropped Group= directive from service template — SupplementaryGroups= covers all required group memberships; redundant Group= with a different value than User= can cause confusion"
  - "README documents -DSERVICE_USER=<name> override explicitly so users know how to configure non-default service users"

patterns-established:
  - "CMake configure-time user capture: if(NOT DEFINED SERVICE_USER OR SERVICE_USER STREQUAL \"\") set(SERVICE_USER \"$ENV{USER}\") endif() — portable, no shell subprocess needed"

requirements-completed: [KIOSK-05]

# Metrics
duration: 12min
completed: 2026-03-01
---

# Phase 5 Plan 02: README and Pi Deployment Documentation Summary

**README.md with full Pi kiosk deployment guide, plus SERVICE_USER CMake variable replacing hardcoded 'pi' in service template — deploy-ready for any Linux user account**

## Performance

- **Duration:** ~12 min (including checkpoint fix)
- **Started:** 2026-03-01T23:00:00Z
- **Completed:** 2026-03-01T23:12:04Z
- **Tasks:** 2 (Task 1 pre-completed; Task 2 completed including checkpoint fix)
- **Files modified:** 3

## Accomplishments

- Created README.md (155+ lines) covering project overview, requirements, standard desktop build, and full Raspberry Pi deployment guide (prerequisites, build/install, user permissions, EGLFS config, systemd service, hostname resolution, troubleshooting table)
- Fixed hardcoded `User=pi` in `assets/wxdash.service.in` — replaced with `@SERVICE_USER@` substitution variable
- Added `SERVICE_USER` CMake variable to `CMakeLists.txt`: defaults to `$ENV{USER}` at configure time, overridable via `-DSERVICE_USER=<name>`, falls back to `nobody` with warning if both are empty
- Updated README.md User Permissions section to use `$USER` shell variable and document the `-DSERVICE_USER` override

## Task Commits

Each task was committed atomically:

1. **Task 1: Create README.md with project overview and Pi deployment guide** - `e6bf5c0` (feat)
2. **Task 2 fix: Remove hardcoded pi user from service template and README** - `c5d5125` (fix)

## Files Created/Modified

- `/home/cisien/src/wxdash/README.md` - Project README with Pi kiosk deployment guide including EGLFS, systemd, group permissions, hostname resolution, and troubleshooting
- `/home/cisien/src/wxdash/assets/wxdash.service.in` - Replaced `User=pi` / `Group=pi` with `User=@SERVICE_USER@`; removed redundant `Group=` directive
- `/home/cisien/src/wxdash/CMakeLists.txt` - Added `SERVICE_USER` variable block with `$ENV{USER}` default before `configure_file` call

## Decisions Made

- **SERVICE_USER defaults to $ENV{USER}:** The correct value is the user running `cmake` on the Pi, not a hardcoded `pi`. This works for any Pi OS user account without requiring manual configuration.
- **Dropped Group= from service template:** `SupplementaryGroups=render video input` provides all needed group access. A separate `Group=` matching the user's primary group is redundant; removing it reduces configuration surface.
- **README uses $USER shell variable:** In user-facing documentation, `$USER` communicates "your current login" clearly without assuming any specific username.

## Deviations from Plan

### Checkpoint Fix Applied

**[User Feedback] Removed hardcoded 'pi' username from service template and README**
- **Found during:** Task 2 checkpoint review
- **Issue:** `assets/wxdash.service.in` had `User=pi` and `Group=pi` hardcoded; README.md User Permissions section and Troubleshooting table also referenced `pi` specifically
- **Fix:** Added `SERVICE_USER` CMake variable defaulting to `$ENV{USER}`; updated service template to use `@SERVICE_USER@`; updated README to use `$USER` and document the CMake override
- **Files modified:** assets/wxdash.service.in, CMakeLists.txt, README.md
- **Verification:** `cmake` configure output shows `-- wxdash service will run as user: cisien`; generated `build/wxdash.service` contains `User=cisien`; full install to `/tmp/wxdash-verify` succeeds with correct service content
- **Committed in:** c5d5125

---

**Total deviations:** 1 (user-directed fix at checkpoint)
**Impact on plan:** Necessary correctness fix — the service template must not assume a specific username. Zero scope creep; fix applies only to the three directly affected files.

## Issues Encountered

- Checkpoint human-verify revealed hardcoded `pi` username in service template — addressed immediately per user feedback before completing the plan.

## User Setup Required

None — no external service configuration required. Pi deployment instructions are in README.md.

## Next Phase Readiness

- Phase 5 complete — all CMake install infrastructure and deployment documentation delivered
- `cmake --install build --component kiosk` deploys binary, QML module, systemd service (with correct user), EGLFS config, and desktop entries
- README.md is complete and sufficient for Pi deployment without additional guidance
- No blockers or outstanding concerns

## Self-Check: PASSED

All claimed files verified:
- FOUND: /home/cisien/src/wxdash/README.md
- FOUND: /home/cisien/src/wxdash/assets/wxdash.service.in
- FOUND: /home/cisien/src/wxdash/CMakeLists.txt

All commits verified:
- e6bf5c0 — Task 1 (feat: README.md)
- c5d5125 — Task 2 fix (fix: remove hardcoded pi user)

---
*Phase: 05-cmake-install-kiosk-target-for-raspberry-pi-deployment*
*Completed: 2026-03-01*
