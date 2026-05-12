# Runtime Session Services Foundation Plan (2026-04-27)

## Goal

Implement the P0 host-independent session-services slice in `mirakana_runtime`: save data, settings, localization catalogs, and keyboard input action mapping.

## Task List

- [x] Add RED tests in `tests/unit/runtime_tests.cpp` for deterministic save data round trip, settings load/malformed rejection, localization key fallback, `VirtualInput` action evaluation, and stable input-action serialization.
- [x] Add `engine/runtime/include/mirakana/runtime/session_services.hpp` with game-facing document/result/action-map contracts.
- [x] Add `engine/runtime/src/session_services.cpp` with strict deterministic text serialization/deserialization, relative path validation, schema version parsing, duplicate-key rejection, localization lookup, and `VirtualInput` action evaluation.
- [x] Register the new source in `engine/runtime/CMakeLists.txt`.
- [x] Update AI-facing and roadmap docs: `engine/agent/manifest.json`, `docs/roadmap.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `docs/superpowers/plans/2026-04-26-engine-excellence-roadmap.md`, and `.agents/skills/gameengine-game-development/SKILL.md`.
- [x] Run public API, agent, schema, and full validation checks.

## Non-Goals

- Pointer/gamepad action mapping.
- Binary save files, cloud saves, profile slots, encryption, compression, or save migration framework.
- Runtime diagnostics/profiling services.
- UI font/text shaping/image/IME/accessibility adapters.
- Editor rebinding UI or sample-game integration.
