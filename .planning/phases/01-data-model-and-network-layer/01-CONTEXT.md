# Phase 1: Data Model and Network Layer - Context

**Gathered:** 2026-03-01
**Status:** Ready for planning

<domain>
## Phase Boundary

HTTP polling, UDP real-time feed, JSON parsing, and a central WeatherDataModel. Live weather data from WeatherLink Live flows into a central in-memory model, correctly parsed and tested, ready for Phase 2 widgets to consume. No UI in this phase.

</domain>

<decisions>
## Implementation Decisions

### Staleness & degraded state
- Uniform 30s staleness threshold for all sources
- When a source goes stale, clear the values entirely — no last-known display, widgets show blank/dash
- UDP staleness means the device itself is having issues; HTTP data is likely stale too — clear all values for that device
- Track staleness per device (WeatherLink Live as one source, PurpleAir as another), not per sensor or per protocol

### Network resilience
- No special retry logic — keep polling HTTP at normal 10s interval; if the device is unreachable, the next poll is the retry
- While UDP is down, attempt to restart the UDP session at regular intervals alongside normal HTTP polling
- Silent recovery — when the device comes back, data just starts populating again, no special notification
- UDP broadcast session: request 86400s (24h) duration, but renew every 3600s (1h) for safety margin
- Silently ignore malformed or unexpected JSON responses — don't process what can't be parsed cleanly

### Project structure & tooling
- CMake build system
- QTest for unit testing
- Directory layout: `src/` with `models/`, `network/` subdirs; `tests/` at project root
- Small files — ~500 line guideline per file, break features into separate classes/files
- Generate AGENTS.md with project conventions for Claude Code
- Standard Qt/C++ naming conventions (camelCase methods, PascalCase classes, m_ prefix for members)
- Code style enforced by formatter/linter rules (not just documented)

### Data model consumer API
- Qt-native Q_PROPERTY with per-value changed signals — widgets subscribe to exactly the signals they care about
- Single `WeatherDataModel` class with all weather fields (refactor later if unwieldy)
- Unified properties — one property per value regardless of which source (HTTP or UDP) provided it; consumers don't know or care about the source
- Dependency injection — model instantiated and passed, no singleton/global state

### Claude's Discretion
- Exact formatter/linter tool choice (clang-format, clang-tidy, etc.)
- Internal network class design and error handling patterns
- JSON parsing implementation details
- UDP packet parsing implementation
- Timer and polling mechanism internals

</decisions>

<specifics>
## Specific Ideas

- UDP session should be renewed well before expiry (86400s requested, renew at 3600s) — belt and suspenders approach
- The polling cadence IS the retry mechanism — no backoff, no special retry state machine
- Files should stay small and well-factored; if a file approaches 500 lines, break it apart

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- None — greenfield project, no existing code

### Established Patterns
- None yet — this phase establishes the foundational patterns for the entire project

### Integration Points
- WeatherLink Live HTTP API: `http://weatherlinklive.local.cisien.com/v1/current_conditions`
- WeatherLink Live UDP broadcast: `/v1/real_time?duration=N`, listen on UDP port 22222
- PurpleAir sensor: `http://10.1.255.41/json?live=false` (Phase 3, but device staleness tracking established here)
- Data structure types: 1=ISS outdoor, 3=barometer, 4=indoor temp/hum

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-data-model-and-network-layer*
*Context gathered: 2026-03-01*
