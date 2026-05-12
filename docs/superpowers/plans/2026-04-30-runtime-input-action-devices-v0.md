# Runtime Input Action Devices v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand `mirakana_runtime` input action maps from keyboard-only bindings to deterministic keyboard, pointer, and gamepad button triggers.

**Architecture:** Keep device state in first-party `mirakana_platform` virtual input contracts and keep action evaluation in `mirakana_runtime`. The runtime document owns action names and trigger lists only; SDL3, OS device handles, editor APIs, UI middleware, RHI, analog axis bindings, rebinding UI, input glyphs, and per-device profile systems stay outside this slice.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_platform` virtual input contracts, focused `mirakana_runtime_tests`, docs/manifest/skills sync.

---

## Context

- `mirakana_platform` already exposes `VirtualInput`, `VirtualPointerInput`, and `VirtualGamepadInput`.
- `mirakana_runtime::RuntimeInputActionMap` currently stores `std::vector<Key>` per action, serializes `format=GameEngine.RuntimeInputActions.v1`, and evaluates only `VirtualInput`.
- Generated games need a first-party action layer that can read keyboard, pointer, and gamepad buttons without depending on SDL3, platform-native device handles, editor systems, or middleware.

## Constraints

- Do not add third-party dependencies.
- Keep `mirakana_runtime` dependent only on first-party public engine contracts.
- Do not expose SDL3, OS window/device handles, native gamepad IDs beyond first-party `GamepadId`, renderer, RHI, editor, or Dear ImGui APIs.
- Keep this as button-style digital action mapping only; analog axes, deadzones, rebinding UI, input glyphs, device profiles, and persistence migrations are follow-up work.
- This is a greenfield clean implementation. Prefer a canonical v2 action document over compatibility shims if the v1 shape is too narrow.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End the slice with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Failing Runtime Input Device Tests

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Add a test named `runtime input action map evaluates key pointer and gamepad triggers`.
- [x] Bind one action to `Key::space`, `primary_pointer_id`, and gamepad `GamepadId{1}` / `GamepadButton::south`.
- [x] Evaluate through a new `RuntimeInputStateView` that can hold `VirtualInput`, `VirtualPointerInput`, and `VirtualGamepadInput`.
- [x] Assert `action_down`, `action_pressed`, and `action_released` work for each device family.
- [x] Add a test named `runtime input action transitions use logical action state across devices` proving a second trigger press while another trigger remains down does not create a new action press, and releasing one trigger while another remains down does not create an action release.
- [x] Add a test named `runtime input action maps serialize canonical device triggers` proving stable v2 serialization order for `key:space`, `pointer:1`, and `gamepad:1:south`, plus deserialize/evaluate.
- [x] Add malformed document assertions for unsupported trigger kind, invalid pointer id, unknown gamepad button, duplicate action lines, intentional v1 rejection, and duplicate triggers.
- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests` and confirm the tests fail because the trigger types, state view, and binding APIs do not exist yet.

### Task 2: Trigger Model And Evaluation

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Replace the keyboard-only binding storage with a clean trigger model:
  - `RuntimeInputActionTriggerKind { key, pointer, gamepad_button }`
  - `RuntimeInputActionTrigger { kind, key, pointer_id, gamepad_id, gamepad_button }`
  - `RuntimeInputStateView { const VirtualInput* keyboard; const VirtualPointerInput* pointer; const VirtualGamepadInput* gamepad; }`
- [x] Add `bind_pointer(std::string action, PointerId pointer_id)`.
- [x] Add `bind_gamepad_button(std::string action, GamepadId gamepad_id, GamepadButton button)`.
- [x] Keep `bind_key` as the keyboard trigger entry point.
- [x] Evaluate `action_down`, `action_pressed`, and `action_released` against `RuntimeInputStateView` across all trigger kinds.
- [x] Preserve logical action semantics: `pressed` only when the action was previously up and is now down; `released` only when the action was previously down and is now up.
- [x] Reject empty actions, unsupported keys, pointer id `0`, gamepad id `0`, and `GamepadButton::unknown` / `count`.

### Task 3: Canonical v2 Serialization

**Files:**
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Change `serialize_runtime_input_actions` to emit `format=GameEngine.RuntimeInputActions.v2`.
- [x] Serialize each action as `bind.<action>=key:<name>,pointer:<id>,gamepad:<id>:<button>`.
- [x] Sort actions by action name and triggers by kind, then device id, then button/key name.
- [x] Make deserialization accept only v2 format for this clean slice.
- [x] Reject unsupported trigger kind, empty trigger values, invalid integer values, pointer id `0`, gamepad id `0`, unknown gamepad buttons, duplicate actions, and duplicate triggers.
- [x] Run focused `mirakana_runtime_tests` until the new and updated tests pass.

### Task 4: Docs, Manifest, Skills, And Plan Registry

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-04-30-runtime-input-action-devices-v0.md`

- [x] Document that runtime input action documents now support keyboard, pointer, and gamepad button triggers through first-party virtual input state.
- [x] State clearly that analog axes, deadzones, rebinding UI, input glyphs, native handles, SDL3, and per-device profiles remain follow-up adapter work.
- [x] Update manifest and generated-game guidance so agents use `RuntimeInputStateView`, `bind_key`, `bind_pointer`, `bind_gamepad_button`, and v2 action documents honestly.
- [x] Update static AI checks for `RuntimeInputStateView`, `bind_gamepad_button`, and the analog/deadzone/rebinding caveat.

### Task 5: Focused And Full Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests`.
- [x] Run `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_runtime_tests"`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Request cpp-reviewer after implementation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Move this plan to Completed in `docs/superpowers/plans/README.md` after evidence is recorded.

## Validation Results

- Red check: `cmake --build --preset dev --target mirakana_runtime_tests` failed before implementation because `RuntimeInputStateView`, `bind_pointer`, and `bind_gamepad_button` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `cmake --build --preset dev --target mirakana_runtime_tests`: PASS.
- `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_runtime_tests"`: PASS, 1/1 test passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS after manifest/guidance synchronization.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; diagnostic-only blocker remains `out/build/dev/compile_commands.json` missing for the Visual Studio generator.
- `cpp-reviewer`: no blocking implementation issue or public SDL3/native/GPU/editor/Dear ImGui leakage found; reviewer requested stronger canonical/v2-break/duplicate-trigger tests and plan honesty updates, which were fixed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, including 22/22 CTest tests. Known diagnostic-only host gates remain Metal `metal` / `metallib` missing, Apple packaging requiring macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict tidy compile database missing.

## Remaining Follow-Up

- Analog axis actions, deadzone tuning, input glyph metadata, runtime rebinding UI, per-device profiles, device hotplug policy beyond virtual state, multiplayer device assignment, touch gestures, action contexts/layers, and native platform device handles remain follow-up systems.

## Done When

- `RuntimeInputActionMap` can bind and evaluate keyboard keys, pointer buttons, and gamepad buttons through `RuntimeInputStateView`.
- Canonical v2 text documents serialize/deserialize those triggers deterministically and reject malformed trigger rows.
- Docs, manifest, skills, and subagent guidance distinguish this from analog/rebinding/device-profile work.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
