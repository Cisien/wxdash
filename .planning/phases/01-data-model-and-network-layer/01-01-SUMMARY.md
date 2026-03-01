---
phase: 01-data-model-and-network-layer
plan: "01"
subsystem: data
tags: [qt6, cmake, ninja, qtest, json-parser, c++17, clang-format, clang-tidy]

requires: []

provides:
  - CMake + Ninja project scaffold with Qt6 (Core, Network, Test)
  - WeatherReadings.h plain C++ structs (IssReading, BarReading, IndoorReading, UdpReading)
  - JsonParser namespace with rainSizeToInches, parseCurrentConditions, parseUdpDatagram
  - 24 unit tests covering all behaviors via QTest
  - .clang-format and .clang-tidy enforcing project code style
  - AGENTS.md documenting all project conventions for Claude Code

affects:
  - 01-data-model-and-network-layer/01-02 (WeatherDataModel builds on WeatherReadings structs)
  - 01-data-model-and-network-layer/01-03 (HttpPoller/UdpReceiver use JsonParser)

tech-stack:
  added:
    - Qt 6.10.2 (Core, Network, Test modules)
    - CMake 4.2.3 with Ninja generator
    - clang-format 21.1.8
    - clang-tidy 21.1.8
  patterns:
    - TDD red-green: write failing tests first, then implement minimally
    - Plain C++ structs for cross-thread data (no QObject inheritance on data types)
    - Stateless namespace (JsonParser) for pure parsing functions
    - qt_add_library(STATIC) for wxdash_lib allows test linking without executable

key-files:
  created:
    - CMakeLists.txt
    - src/CMakeLists.txt
    - tests/CMakeLists.txt
    - src/main.cpp
    - src/models/WeatherReadings.h
    - src/network/JsonParser.h
    - src/network/JsonParser.cpp
    - tests/tst_JsonParser.cpp
    - .clang-format
    - .clang-tidy
    - AGENTS.md
  modified: []

key-decisions:
  - "JsonParser implemented as stateless namespace with free functions (not class) — no shared state, trivially testable"
  - "WeatherReadings.h has no Qt dependency — plain C++17 structs with Q_DECLARE_METATYPE at bottom"
  - "cmake not installed on system — installed via pacman (Rule 3 auto-fix)"
  - "Used LLVM clang-format style with Qt conventions (PointerAlignment Left, IndentWidth 4)"

patterns-established:
  - "Never assume array index for data_structure_type — always iterate and check field value"
  - "Rain counts multiplied by rainSizeToInches(rain_size) factor before storing in structs"
  - "Malformed JSON returns empty/nullopt silently — no logging, no throw"
  - "wxdash_lib STATIC library links to both executable and tests"

requirements-completed: [DATA-04, DATA-05]

duration: 6min
completed: 2026-03-01
---

# Phase 1 Plan 01: CMake Scaffold + JsonParser with TDD Summary

**Qt6 C++17 project scaffolded with CMake/Ninja, WeatherReadings structs, and JsonParser with 24 unit tests covering rain_size conversion and data_structure_type routing**

## Performance

- **Duration:** 6 min
- **Started:** 2026-03-01T18:18:07Z
- **Completed:** 2026-03-01T18:24:28Z
- **Tasks:** 2
- **Files modified:** 11 created

## Accomplishments

- CMake + Ninja build system scaffolded with Qt6 (Core, Network, Test), C++17, -Wall -Wextra
- WeatherReadings.h defines four plain C++ structs (IssReading, BarReading, IndoorReading, UdpReading) for safe cross-thread data transfer
- JsonParser implements stateless namespace with three functions: rainSizeToInches, parseCurrentConditions, parseUdpDatagram
- 24 unit tests pass covering all four rain_size values, array-order-independence, malformed JSON, empty inputs, UDP parsing, and rain conversion
- AGENTS.md documents all project conventions; .clang-format and .clang-tidy enforce code style

## Task Commits

Each task was committed atomically:

1. **Task 1: Scaffold CMake project** - `e3731e2` (chore)
2. **Task 2 RED: Failing tests for JsonParser behaviors** - `981a5ba` (test)
3. **Task 2 GREEN: JsonParser full implementation** - `df280ce` (feat)

_Note: TDD task has three commits: scaffold → test (RED) → feat (GREEN)_

## Files Created/Modified

- `/home/cisien/src/wxdash/CMakeLists.txt` - Top-level CMake: Qt6, C++17, -Wall -Wextra, enable_testing
- `/home/cisien/src/wxdash/src/CMakeLists.txt` - wxdash_lib STATIC + wxdash executable targets
- `/home/cisien/src/wxdash/tests/CMakeLists.txt` - tst_JsonParser test executable target
- `/home/cisien/src/wxdash/src/main.cpp` - Minimal QCoreApplication entry point
- `/home/cisien/src/wxdash/src/models/WeatherReadings.h` - IssReading, BarReading, IndoorReading, UdpReading structs with Q_DECLARE_METATYPE
- `/home/cisien/src/wxdash/src/network/JsonParser.h` - Public API: rainSizeToInches, ParsedConditions, parseCurrentConditions, parseUdpDatagram
- `/home/cisien/src/wxdash/src/network/JsonParser.cpp` - Full implementation with QJsonDocument parsing
- `/home/cisien/src/wxdash/tests/tst_JsonParser.cpp` - 24 QTest unit tests covering all behaviors
- `/home/cisien/src/wxdash/.clang-format` - LLVM style, 4-space indent, 100 col limit, Qt pointer alignment
- `/home/cisien/src/wxdash/.clang-tidy` - readability/modernize/bugprone/performance checks
- `/home/cisien/src/wxdash/AGENTS.md` - Full project conventions reference for Claude Code

## Decisions Made

- JsonParser is a stateless namespace (not a class) — pure functions are trivially unit-testable and have no shared state that could cause thread issues
- WeatherReadings.h avoids Qt headers in the struct definitions themselves — only QMetaType is included at the bottom for Q_DECLARE_METATYPE; the structs themselves have zero Qt dependency
- LLVM clang-format style chosen over WebKit — LLVM is the default for C++ projects and aligns better with Qt's own tooling recommendations

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Installed missing cmake package**
- **Found during:** Task 1 (CMake project scaffold)
- **Issue:** cmake was not installed on the development machine (only available via flatpak SDK)
- **Fix:** Ran `sudo pacman -S cmake ninja --noconfirm` to install cmake 4.2.3 and ninja 1.13.2
- **Files modified:** None (system package installation)
- **Verification:** `cmake --version` returns 4.2.3; `cmake -B build -G Ninja` succeeds
- **Committed in:** e3731e2 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (Rule 3 - blocking dependency)
**Impact on plan:** Required for build to work. No scope creep.

## Issues Encountered

None beyond the cmake installation above.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- WeatherReadings.h structs are ready for WeatherDataModel to consume (Plan 02)
- JsonParser is ready for HttpPoller and UdpReceiver to use (Plan 03)
- wxdash_lib STATIC library is correctly linked so both test and executable targets can share the implementation
- Build system configured with -Wall -Wextra; any warnings in future code will be caught at compile time

---
*Phase: 01-data-model-and-network-layer*
*Completed: 2026-03-01*
