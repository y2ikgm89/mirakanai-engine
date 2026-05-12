# Input Rebinding Profile UX v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent, persisted runtime input rebinding profile contract plus editor-core review diagnostics so generated games can expose controller/key rebinding without SDL3, editor-private APIs, native handles, or middleware.

**Status:** Completed

**Architecture:** Keep canonical gameplay defaults in `GameEngine.RuntimeInputActions.v4` and add a separate `GameEngine.RuntimeInputRebindingProfile.v1` overlay that can only replace bindings for actions/axes already declared by a base `RuntimeInputActionMap`. Runtime validation and application live in `mirakana_runtime`; editor UX is a read-only `mirakana_editor_core` review model over the same diagnostics. The slice is intentionally not a visible ImGui panel, glyph system, UI focus/consumption system, multiplayer device assignment system, native platform input API, or cloud/binary save format.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_platform` virtual input contracts, `mirakana_editor_core`, focused `mirakana_runtime_tests` and `mirakana_editor_core_tests`, docs/manifest/skills/static AI guidance sync.

---

## Context

- Runtime input action foundations are already complete through `GameEngine.RuntimeInputActions.v4`: keyboard, pointer, gamepad button triggers, scalar key-pair/gamepad axes, and deterministic context-stack lookup.
- The master plan still lists input rebinding and persisted input profiles as required runtime-system minimums before a 1.0 AI-operable production claim.
- Generated games need a reviewed way to store player/operator rebinding choices and apply them to a default action map without depending on SDL3, Dear ImGui, editor storage, native handles, or game-local input frameworks.

## Constraints

- Do not add third-party dependencies.
- Keep runtime gameplay code on public `mirakana::runtime` / `mirakana_platform` contracts only.
- Keep editor-core GUI-independent; do not add visible `mirakana_editor` panel wiring in this slice.
- Do not parse or execute arbitrary shell, command strings, or raw manifest commands.
- Rebinding profiles must be deterministic text documents and must not create new action/axis names that are absent from the base map.
- Rebinding diagnostics must distinguish invalid profile data, missing base actions, duplicate overrides, duplicate triggers/sources, and same-context digital trigger conflicts.
- Existing `GameEngine.RuntimeInputActions.v4` remains the default action-map format; no v1-v3 compatibility shims are added.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Planned Runtime Document Shape

```text
format=GameEngine.RuntimeInputRebindingProfile.v1
profile.id=player_one
bind.gameplay.confirm=key:space,gamepad:1:south
axis.gameplay.move_x=keys:left:right,gamepad:1:left_x:1:0.25
```

- `bind.<context>.<action>` replaces the full digital trigger list for an existing base action binding.
- `axis.<context>.<action>` replaces the full scalar axis source list for an existing base axis binding.
- The profile is an overlay: it is valid only with a caller-supplied base `RuntimeInputActionMap`.
- The profile keeps action names and device abstractions first-party and value-type only.

## Tasks

### Task 1: RED Runtime Profile Tests

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Add a test named `runtime input rebinding profile applies digital and axis overrides`.
- [x] Build a base action map with `gameplay.confirm` on `GamepadButton::south` and `gameplay.move_x` on `left/right`.
- [x] Build a profile with `gameplay.confirm` on `Key::space` and `gameplay.move_x` on `GamepadAxis::left_x`.
- [x] Assert `apply_runtime_input_rebinding_profile` succeeds, reports one digital and one axis override, and the returned map evaluates the new bindings.
- [x] Add exact serialization/deserialization coverage for `GameEngine.RuntimeInputRebindingProfile.v1`.
- [x] Add malformed/diagnostic coverage for old/unsupported formats, missing base action, duplicate override, duplicate trigger/source, empty override values, invalid profile id, invalid context/action names, and same-context digital trigger conflicts.
- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests` and confirm RED because the profile types/functions do not exist.

### Task 2: Runtime Rebinding Profile Contract

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Add value types for `RuntimeInputRebindingProfile`, digital override rows, axis override rows, diagnostics, and load/apply results.
- [x] Add public functions:
  - `validate_runtime_input_rebinding_profile(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile)`
  - `apply_runtime_input_rebinding_profile(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile)`
  - `serialize_runtime_input_rebinding_profile(const RuntimeInputRebindingProfile& profile)`
  - `deserialize_runtime_input_rebinding_profile(std::string_view text)`
  - `load_runtime_input_rebinding_profile(IFileSystem& filesystem, std::string_view path)`
  - `write_runtime_input_rebinding_profile(IFileSystem& filesystem, std::string_view path, const RuntimeInputRebindingProfile& profile)`
- [x] Reuse existing first-party trigger/source grammar from `RuntimeInputActions.v4`.
- [x] Keep validation strict and deterministic.
- [x] Apply profiles by rebuilding a new `RuntimeInputActionMap` from the base rows with selected full-row replacements.

### Task 3: GREEN Runtime Tests

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests` until the new runtime tests pass.
- [x] Run `ctest --preset dev -R "mirakana_runtime_tests" --output-on-failure`.
- [x] Refactor only after the focused runtime tests are green.

### Task 4: Editor-Core Review Model

**Files:**
- Create: `editor/core/include/mirakana/editor/input_rebinding.hpp`
- Create: `editor/core/src/input_rebinding.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `EditorInputRebindingProfileReviewModel` with read-only row diagnostics, `ready_for_save`, `has_blocking_diagnostics`, `has_conflicts`, `mutates=false`, and `executes=false`.
- [x] Add `make_editor_input_rebinding_profile_review_model` over a base action map and a `RuntimeInputRebindingProfile`.
- [x] Report unsupported requests for native handles, SDL3 input APIs, Dear ImGui/editor-private runtime dependencies, UI focus/consumption, multiplayer device assignment, and glyph generation.
- [x] Add tests proving a clean profile is ready, conflict diagnostics are surfaced, and unsupported execution/mutation-style claims are blocked without mutating or executing.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev -R "mirakana_editor_core_tests" --output-on-failure`.

### Task 5: Docs, Manifest, Skills, And Static Guidance

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/editor.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-05-06-input-rebinding-profile-ux-v1.md`

- [x] Mark this plan as the active slice in the plan registry and manifest before implementation.
- [x] After implementation, document `GameEngine.RuntimeInputRebindingProfile.v1`, the runtime apply/diagnostic helpers, and the editor-core review model.
- [x] Keep ready claims narrow: no visible rebinding panel, glyphs, UI focus consumption, multiplayer assignment, native handles, SDL3/editor runtime dependency, cloud/binary saves, or broad input middleware.
- [x] Update static AI checks so the new profile contract and non-goals stay synchronized across Codex and Claude guidance.
- [x] Return `currentActivePlan` to the master plan and `recommendedNextPlan` to `next-production-gap-selection` after validation passes.

### Task 6: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused runtime/editor-core CTest commands from Tasks 3 and 4.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and move this plan to Recent Completed in the registry.

## Done When

- `GameEngine.RuntimeInputRebindingProfile.v1` serializes/deserializes deterministic digital and axis override rows.
- `apply_runtime_input_rebinding_profile` validates a profile against a base `RuntimeInputActionMap` and returns a new action map with safe, full-row overrides.
- Editor-core exposes a GUI-independent review model for rebinding profile readiness/conflicts/unsupported claims.
- Docs, manifest, Codex/Claude skills, and static checks state the new ready boundary and non-goals honestly.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete local-tool blocker is recorded.

## Validation Results

- `cmake --build --preset dev --target mirakana_runtime_tests`: RED first on missing `RuntimeInputRebindingProfile` types/functions after test insertion; GREEN after runtime implementation.
- `ctest --preset dev -R "mirakana_runtime_tests" --output-on-failure`: PASS.
- `cmake --build --preset dev --target mirakana_editor_core_tests`: RED first on missing `mirakana/editor/input_rebinding.hpp` after test insertion; GREEN after editor-core model implementation.
- `ctest --preset dev -R "mirakana_editor_core_tests" --output-on-failure`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `ctest --preset dev -R "mirakana_runtime_tests|mirakana_editor_core_tests" --output-on-failure`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only host gates remain for missing Apple/Metal tools on this Windows host.
