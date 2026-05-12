# Runtime Input Action Context Layers v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic input action contexts so gameplay, menu, pause, and debug layers can select different digital and scalar axis bindings without native device handles.

**Architecture:** Keep device state in first-party `mirakana_platform` virtual input contracts and keep context filtering in `mirakana_runtime`. This is a clean `GameEngine.RuntimeInputActions.v4` document upgrade: bindings are keyed by context and action, active context stacks are explicit value data, and v3 compatibility shims are intentionally not added.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_platform` virtual input contracts, focused `mirakana_runtime_tests`, docs/manifest/skills sync.

---

## Context

- `Runtime Input Action Devices v0` added keyboard, pointer, and gamepad button triggers.
- `Runtime Input Action Axes And Deadzones v0` added scalar key-pair and gamepad-axis sources with deterministic clamp/deadzone handling.
- Generated games still need a first-party way to distinguish gameplay, menu, pause, and debug action layers without SDL3, editor APIs, input middleware, or per-device native handles.

## Constraints

- No third-party dependencies.
- Keep `mirakana_runtime` dependent only on first-party public engine contracts.
- Do not expose SDL3, OS handles, native gamepad handles, renderer, RHI, editor, or Dear ImGui APIs.
- Use deterministic active context stack semantics: evaluate the first active context that declares the requested action; if the active stack is empty, evaluate only the default context.
- Context matching is action-specific. Global consumption, bubbling, modal focus, UI focus integration, and multiplayer device assignment are follow-up work.
- Context and action names must be non-empty document keys without `.`, `=`, `,`, control characters, or whitespace.
- Keep existing device/axis semantics from v3.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End the slice with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Planned v4 Document Shape

```text
format=GameEngine.RuntimeInputActions.v4
bind.default.jump=key:space
bind.gameplay.confirm=gamepad:1:south
bind.menu.confirm=key:space
axis.gameplay.move_x=keys:left:right,gamepad:1:left_x:1:0.25
```

- `bind.<context>.<action>` rows contain v3 digital trigger values.
- `axis.<context>.<action>` rows contain v3 scalar axis source values.
- `default` is the implicit context for existing convenience bind/evaluate calls.

## Tasks

### Task 1: Failing Runtime Context Tests

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Add `runtime input action contexts select first active layer`.
- [x] Bind `confirm` to `Key::space` in `menu` and `GamepadButton::south` in `gameplay`.
- [x] Create `RuntimeInputContextStack` with active contexts `menu`, then `gameplay`.
- [x] Assert `action_down("confirm", state, stack)` follows `menu` while `menu` declares the action.
- [x] Assert a gameplay-only action still evaluates through the lower-priority `gameplay` context.
- [x] Add `runtime input action context axes select first active layer`.
- [x] Bind `move_x` axis differently in `menu` and `gameplay`; assert active context order selects the first declaring context.
- [x] Add explicit empty-stack default-context and non-empty-stack no-default-fallback coverage.
- [x] Add duplicate runtime bind call coverage for digital triggers and scalar axis sources.
- [x] Update exact serialization tests to expect `GameEngine.RuntimeInputActions.v4` plus `bind.default.*` / `axis.default.*` rows.
- [x] Add exact context serialization/deserialization coverage for `bind.gameplay.confirm`, `bind.menu.confirm`, and `axis.gameplay.move_x`.
- [x] Add malformed document assertions for unsupported v3 format, missing context/action separators, duplicate context+action rows, invalid context names, invalid action names, and duplicate trigger/source rows.
- [x] Run `cmake --build --preset dev --target mirakana_runtime_tests` and confirm the tests fail because context APIs and v4 row grammar do not exist yet.

### Task 2: Public Context Model

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Add `inline constexpr std::string_view runtime_input_default_context = "default"`.
- [x] Add `RuntimeInputContextStack { std::vector<std::string> active_contexts; }`.
- [x] Add `std::string context` to `RuntimeInputActionBinding` and `RuntimeInputAxisBinding`.
- [x] Keep existing `bind_key`, `bind_pointer`, `bind_gamepad_button`, `bind_key_axis`, and `bind_gamepad_axis` as default-context convenience methods.
- [x] Add explicit context methods:
  - `bind_key_in_context(std::string context, std::string action, Key key)`
  - `bind_pointer_in_context(std::string context, std::string action, PointerId pointer_id)`
  - `bind_gamepad_button_in_context(std::string context, std::string action, GamepadId gamepad_id, GamepadButton button)`
  - `bind_key_axis_in_context(std::string context, std::string action, Key negative_key, Key positive_key)`
  - `bind_gamepad_axis_in_context(std::string context, std::string action, GamepadId gamepad_id, GamepadAxis axis, float scale = 1.0F, float deadzone = 0.0F)`
- [x] Add `find(std::string_view context, std::string_view action)` and `find_axis(std::string_view context, std::string_view action)`.
- [x] Add context-aware `action_down`, `action_pressed`, `action_released`, and `axis_value` overloads that accept `const RuntimeInputContextStack&`.

### Task 3: Context Evaluation And Validation

**Files:**
- Modify: `engine/runtime/src/session_services.cpp`

- [x] Validate context and action names with the v4 name rules.
- [x] Sort bindings by context, then action.
- [x] Implement active context lookup:
  - empty stack uses `default`
  - otherwise scan `active_contexts` in order
  - return the first binding whose context declares the action
  - if none match, return no binding / zero / false
- [x] Preserve current digital transition semantics within the selected binding only.
- [x] Preserve current scalar source arbitration within the selected axis binding only.
- [x] Ignore duplicate runtime bind calls for the same context/action/source, but keep document deserialization strict.

### Task 4: Canonical v4 Serialization

**Files:**
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] Change `input_actions_format` to `GameEngine.RuntimeInputActions.v4`.
- [x] Serialize digital rows as `bind.<context>.<action>=...`.
- [x] Serialize axis rows as `axis.<context>.<action>=...`.
- [x] Deserialize v4 rows only and reject v1/v2/v3.
- [x] Reject unsupported row keys, missing or empty context/action, invalid context/action characters, duplicate context+action rows, empty trigger/source values, duplicate trigger/source rows, and malformed trigger/source values.
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
- Modify: `docs/superpowers/plans/2026-04-30-runtime-input-action-context-layers-v0.md`

- [x] Document `GameEngine.RuntimeInputActions.v4`, context row grammar, and `RuntimeInputContextStack`.
- [x] State clearly that UI focus integration, global input consumption/bubbling, rebinding UI, glyphs, per-device profiles, multiplayer device assignment, SDL3, and native handles remain follow-up work.
- [x] Update manifest and generated-game guidance so agents use all context-specific bind methods and context-aware evaluation honestly.
- [x] Update static AI checks for `GameEngine.RuntimeInputActions.v4`, `RuntimeInputContextStack`, all explicit context bind methods, and the input consumption/rebinding/glyph caveat.

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

- Red check: `cmake --build --preset dev --target mirakana_runtime_tests` failed before implementation because `bind_key_in_context`, `bind_gamepad_button_in_context`, `RuntimeInputContextStack`, context-aware `action_down`, `bind_key_axis_in_context`, `bind_gamepad_axis_in_context`, and context-aware `axis_value` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `cmake --build --preset dev --target mirakana_runtime_tests`: PASS.
- `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_runtime_tests"`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; diagnostic-only because Visual Studio `dev` preset did not emit `out/build/dev/compile_commands.json`.
- `cpp-reviewer`: no runtime implementation blocker; follow-ups for plan registry, complete context bind guidance, non-default fallback coverage, and duplicate runtime bind coverage were fixed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS with known diagnostic-only host gates for missing `metal` / `metallib`, Apple macOS/Xcode tools (`xcodebuild` / `xcrun`), Android release signing, Android device smoke, and strict tidy compile database.

## Remaining Follow-Up

- UI focus integration, global input consumption/bubbling, modal routing, rebinding UI, input glyph metadata, per-device profiles, multiplayer device assignment, pointer delta axes, touch gestures, radial stick/vector deadzones, response curves, haptics, native platform device handles, and sample-game context consumption remain follow-up systems.

## Done When

- `RuntimeInputActionMap` can bind and evaluate digital and scalar axis actions in named contexts through `RuntimeInputContextStack`.
- Canonical v4 text documents serialize/deserialize context-scoped digital triggers and scalar axes deterministically and reject malformed rows.
- Docs, manifest, skills, and subagent guidance distinguish context layers from UI focus, global input consumption, rebinding, glyphs, profiles, and native handles.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
