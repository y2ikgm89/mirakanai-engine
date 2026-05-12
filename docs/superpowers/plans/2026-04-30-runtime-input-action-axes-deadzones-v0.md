# Runtime Input Action Axes And Deadzones v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand `mirakana_runtime` input action documents from digital triggers to deterministic scalar axis actions with key-pair and gamepad-axis sources plus deadzone handling.

**Architecture:** Keep hardware state in first-party `mirakana_platform` virtual input contracts and keep game-facing action/axis documents in `mirakana_runtime`. This is a greenfield clean document upgrade to `GameEngine.RuntimeInputActions.v3`; do not preserve v2 compatibility shims.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_platform` virtual input contracts, focused `mirakana_runtime_tests`, docs/manifest/skills sync.

---

## Context

- `Runtime Input Action Devices v0` implemented `RuntimeInputStateView`, `bind_key`, `bind_pointer`, `bind_gamepad_button`, and `GameEngine.RuntimeInputActions.v2` digital trigger documents.
- `mirakana_platform` already exposes `VirtualGamepadInput::axis_value`, `GamepadAxis`, and `VirtualInput::digital_axis`.
- Generated games still lack a runtime-owned, serializable analog action layer for movement, camera, vehicle, and UI navigation axes.

## Constraints

- No third-party dependencies.
- Keep `mirakana_runtime` dependent only on first-party public engine contracts.
- Do not expose SDL3, OS handles, native gamepad handles, renderer, RHI, editor, or Dear ImGui APIs.
- Scope is scalar axis actions only: key-pair axes and gamepad axes.
- Use deterministic source selection: evaluate all bound axis sources and choose the value with greatest absolute magnitude; canonical source order wins ties.
- Apply deterministic deadzone only after clamping input to `[-1, 1]`: if `abs(value) <= deadzone`, output `0`; otherwise rescale to `(abs(value) - deadzone) / (1 - deadzone)` with the original sign.
- Reject invalid public input: empty action names, invalid keys, invalid gamepad id `0`, invalid `GamepadAxis::unknown` / `count`, non-finite scale/deadzone, zero scale, scale outside `[-1, 1]`, and deadzone outside `[0, 1)`.
- Non-goals: pointer delta axes, touch gestures, radial stick/vector deadzones, response curves, smoothing, latest-device arbitration, rebinding UI, input glyphs, action contexts/layers, per-device profiles, haptics, multiplayer device assignment, and v2 migration compatibility.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End the slice with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Planned v3 Document Shape

```text
format=GameEngine.RuntimeInputActions.v3
bind.jump=key:space,pointer:1,gamepad:1:south
axis.move_x=keys:left:right,gamepad:1:left_x:1:0.25
axis.look_y=gamepad:1:right_y:-1:0.2
```

- `bind.*` rows keep the v2 digital trigger grammar.
- `axis.*` rows contain comma-separated axis sources.
- `keys:<negative>:<positive>` produces `-1`, `0`, or `1`; pressing both keys produces `0`.
- `gamepad:<id>:<axis>:<scale>:<deadzone>` reads a first-party `VirtualGamepadInput` axis, applies `scale`, clamps to `[-1, 1]`, then applies deadzone.

## Tasks

### Task 1: Failing Runtime Axis Tests

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Add a test named `runtime input action axes evaluate key pairs and gamepad axes`.
- [x] Create `RuntimeInputActionMap actions`.
- [x] Call the intended API:
  - `actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);`
  - `actions.bind_gamepad_axis("move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F);`
  - `actions.bind_gamepad_axis("look_y", mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, -1.0F, 0.2F);`
- [x] Assert key pairs evaluate to `-1`, `0`, and `1`, including the both-keys-down neutral case.
- [x] Assert gamepad `left_x=0.5` with deadzone `0.25` evaluates to approximately `0.33333334`.
- [x] Assert inverted gamepad `right_y=0.6` with scale `-1` and deadzone `0.2` evaluates to approximately `-0.5`.
- [x] Add a test named `runtime input action axes choose strongest source deterministically`.
- [x] Bind a key-pair source and two gamepad sources to one axis; assert the greatest absolute value wins and canonical source order wins equal-magnitude ties.
- [x] Add a test named `runtime input action maps serialize canonical v3 axis documents`.
- [x] Expect exact serialization:

```text
format=GameEngine.RuntimeInputActions.v3
bind.jump=key:space
axis.look_y=gamepad:1:right_y:-1:0.2
axis.move_x=keys:left:right,gamepad:1:left_x:1:0.25
```

- [x] Update existing v2 serialization tests to expect `GameEngine.RuntimeInputActions.v3`.
- [x] Add malformed document assertions for unsupported v2 format, unsupported axis source kind, invalid key-pair key, gamepad id `0`, unknown gamepad axis, zero scale, deadzone `1`, duplicate axis action rows, and duplicate axis sources.
- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests` and confirm the tests fail because the axis binding model and v3 format do not exist yet.

### Task 2: Axis Binding Public Model

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Add `RuntimeInputAxisSourceKind { key_pair, gamepad_axis }`.
- [x] Add `RuntimeInputAxisSource` with fields:
  - `RuntimeInputAxisSourceKind kind`
  - `Key negative_key`
  - `Key positive_key`
  - `GamepadId gamepad_id`
  - `GamepadAxis gamepad_axis`
  - `float scale`
  - `float deadzone`
- [x] Add `RuntimeInputAxisBinding { std::string action; std::vector<RuntimeInputAxisSource> sources; }`.
- [x] Add public methods:
  - `bind_key_axis(std::string action, Key negative_key, Key positive_key)`
  - `bind_gamepad_axis(std::string action, GamepadId gamepad_id, GamepadAxis axis, float scale = 1.0F, float deadzone = 0.0F)`
  - `axis_value(std::string_view action, RuntimeInputStateView state) const noexcept`
  - `find_axis(std::string_view action) const noexcept`
  - `axis_bindings() const noexcept`
- [x] Keep digital trigger APIs unchanged except for document format v3.
- [x] Store digital bindings and axis bindings in separate sorted vectors.
- [x] Ignore duplicate `bind_*` calls at runtime as the digital trigger path already does, but keep document deserialization strict and reject duplicate sources.

### Task 3: Axis Evaluation And Deadzone Policy

**Files:**
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Validate key-pair sources reject invalid keys and identical negative/positive keys.
- [x] Validate gamepad-axis sources reject gamepad id `0`, invalid axes, non-finite scale/deadzone, zero scale, scale outside `[-1, 1]`, and deadzone outside `[0, 1)`.
- [x] Implement key-pair axis evaluation:
  - negative key down only => `-1`
  - positive key down only => `1`
  - both down or both up => `0`
- [x] Implement gamepad-axis evaluation using `RuntimeInputStateView::gamepad`; missing gamepad state returns `0`.
- [x] Implement deadzone as the deterministic scalar formula in Constraints.
- [x] Implement source arbitration by greatest absolute value, with canonical source order as tie-breaker.

### Task 4: Canonical v3 Serialization

**Files:**
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Change `input_actions_format` to `GameEngine.RuntimeInputActions.v3`.
- [x] Keep digital trigger serialization as `bind.<action>=key:<name>,pointer:<id>,gamepad:<id>:<button>`.
- [x] Add axis serialization as:
  - `axis.<action>=keys:<negative>:<positive>`
  - `axis.<action>=gamepad:<id>:<axis>:<scale>:<deadzone>`
- [x] Add canonical gamepad axis names: `left_x`, `left_y`, `right_x`, `right_y`, `left_trigger`, `right_trigger`.
- [x] Sort axis action rows by action name.
- [x] Sort axis sources by kind, then key names, then gamepad id, then axis name, then scale, then deadzone.
- [x] Use locale-independent `std::to_chars` / `std::from_chars` float conversion for scale/deadzone.
- [x] Make deserialization accept only v3 format.
- [x] Reject unsupported axis source kinds, malformed source field counts, empty source values, invalid floats, duplicate axis actions, and duplicate axis sources.
- [x] Run focused `mirakana_runtime_tests` until the new and updated tests pass.

### Task 5: Docs, Manifest, Skills, And Plan Registry

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-04-30-runtime-input-action-axes-deadzones-v0.md`

- [x] Document that runtime input action documents are now v3 and include digital triggers plus scalar axes.
- [x] State clearly that pointer delta axes, touch gestures, radial deadzones, response curves, rebinding UI, input glyphs, action contexts/layers, native handles, SDL3, and per-device profiles remain follow-up adapter work.
- [x] Update manifest and generated-game guidance so agents use `bind_key_axis`, `bind_gamepad_axis`, `axis_value`, and v3 action documents honestly.
- [x] Update static AI checks for `GameEngine.RuntimeInputActions.v3`, `bind_gamepad_axis`, `axis_value`, and the radial/rebinding/glyph caveat.

### Task 6: Focused And Full Validation

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

- Red check: `cmake --build --preset dev --target mirakana_runtime_tests` failed before implementation because `bind_key_axis`, `bind_gamepad_axis`, and `axis_value` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `cmake --build --preset dev --target mirakana_runtime_tests`: PASS.
- `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_runtime_tests"`: PASS, 1/1 test passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS after manifest/guidance synchronization and strengthened caveat checks.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; diagnostic-only blocker remains `out/build/dev/compile_commands.json` missing for the Visual Studio generator.
- `cpp-reviewer`: no blocking implementation issue or public SDL3/native/GPU/editor/Dear ImGui leakage found; reviewer requested plan honesty and stronger caveat synchronization for `per-device profiles` / `action contexts/layers`, which were fixed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, including 22/22 CTest tests. Known diagnostic-only host gates remain Metal `metal` / `metallib` missing, Apple packaging requiring macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict tidy compile database missing.

## Remaining Follow-Up

- Pointer delta axes, touch gesture axes, radial stick/vector deadzones, response curves, smoothing, latest-device arbitration, rebinding UI, input glyph metadata, action contexts/layers, per-device profiles, haptics, multiplayer device assignment, native platform device handles, and sample-game axis consumption remain follow-up systems.

## Done When

- `RuntimeInputActionMap` can bind and evaluate scalar axes from key pairs and gamepad axes through `RuntimeInputStateView`.
- Canonical v3 text documents serialize/deserialize digital triggers and scalar axes deterministically and reject malformed rows.
- Docs, manifest, skills, and subagent guidance distinguish this from radial deadzones, rebinding, glyphs, native handles, and per-device profile work.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
