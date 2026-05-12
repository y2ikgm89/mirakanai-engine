# Editor Input Rebinding Action Capture Panel v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the visible editor Input Rebinding panel from read-only diagnostics to reviewed in-memory digital action capture using the existing first-party runtime rebinding capture contract. Validated in `MK_editor_core_tests`.

**Architecture:** Keep capture validation and candidate profile construction in GUI-independent `mirakana_editor_core` over `mirakana_runtime` and first-party `RuntimeInputStateView`. Wire `mirakana_editor` as a thin optional shell adapter that feeds `VirtualInput`, `VirtualPointerInput`, and `VirtualGamepadInput` updated through `SdlWindow::handle_event`, applies captured candidates only to the in-memory profile, and avoids file IO, command execution, native handles, glyph generation, runtime UI focus consumption, and multiplayer device assignment.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, `mirakana_runtime`, `mirakana_platform` virtual input contracts, Dear ImGui shell adapter, repository CMake/CTest plus PowerShell 7 scripts under `tools/`.

---

## Goal

Close the current editor productization loophole after Runtime Input Rebinding Capture Contract v1: the editor can review rebinding profiles and the runtime can build safe capture candidates, but the visible Input Rebinding panel still cannot arm action capture or apply a captured candidate to the edited profile.

## Context

- `mirakana_runtime` now exposes `RuntimeInputRebindingCaptureRequest`, `RuntimeInputRebindingCaptureResult`, and `capture_runtime_input_rebinding_action`.
- `mirakana_editor_core` already exposes read-only `EditorInputRebindingProfilePanelModel` and retained `input_rebinding` UI rows.
- `mirakana_editor` currently builds a sample base action map/profile and renders the panel without any capture state.
- `SdlWindow::handle_event` can already update `VirtualInput`, `VirtualPointerInput`, and `VirtualGamepadInput`; the editor shell currently calls it without passing those first-party input states.

## Constraints

- Capture digital action triggers only: keyboard keys, pointer ids, and gamepad buttons. Axis capture remains unsupported.
- Do not add file mutation, cloud/binary saves, command execution, validation recipe execution, package scripts, runtime UI focus consumption/bubbling, input glyph generation, multiplayer device assignment, native device handles, public SDL3 input APIs, editor-private runtime dependencies, or middleware.
- Keep `editor/core` GUI-independent and free of SDL3, Dear ImGui, OS handles, renderer/RHI handles, and process execution.
- Treat editor profile mutation as an explicit in-memory candidate apply only; saving remains a separate reviewed workflow.
- In the visible shell, avoid capturing the mouse click that arms capture by skipping capture evaluation until the frame after a target is armed.

## Done When

- `mirakana_editor_core` exposes an action capture model and retained `mirakana_ui` rows that can return waiting/captured/blocked status, candidate profile, trigger label, diagnostics, and unsupported-claim rows.
- Unit tests prove pressed key capture updates a candidate profile, no input waits without mutation, unsupported file/command/native/focus/glyph/device claims block, and retained UI rows are stable.
- `mirakana_editor` updates first-party virtual input state per frame and lets the Input Rebinding panel arm/cancel action capture, then applies captured candidates to the in-memory profile only.
- Docs, plan registry, master plan, manifest, skills, and static contract checks describe the narrow editor action capture path while preserving unsupported claims for axis capture, glyphs, runtime UI focus/consumption, device assignment, native handles, file/cloud/binary saves, and broad input middleware.
- Focused `mirakana_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing this slice.

## Files

- Modify: `editor/core/include/mirakana/editor/input_rebinding.hpp`
- Modify: `editor/core/src/input_rebinding.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/testing.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add `editor input rebinding capture model captures pressed key candidate` to `tests/unit/editor_core_tests.cpp`.
- [x] Add `editor input rebinding capture model waits without mutating profile`.
- [x] Add `editor input rebinding capture model blocks unsupported file command and native claims`.
- [x] Add retained UI row assertions for `input_rebinding.capture`, `.status`, `.trigger`, and `.diagnostics`.
- [x] Run focused `mirakana_editor_core_tests` and confirm the expected compile failure before the new editor-core API exists.

### Task 2: Editor-Core Capture Contract

- [x] Add `EditorInputRebindingCaptureStatus` with `waiting`, `captured`, and `blocked`.
- [x] Add `EditorInputRebindingCaptureDesc` with base actions, current profile, selected context/action, `RuntimeInputStateView`, capture allow flags, and unsupported request flags.
- [x] Add `EditorInputRebindingCaptureModel` with status, labels, optional trigger, candidate profile, diagnostics, unsupported claims, `mutates_profile`, `mutates_files=false`, and `executes=false`.
- [x] Implement `make_editor_input_rebinding_capture_action_model` by rejecting unsupported claims, delegating to `capture_runtime_input_rebinding_action`, and preserving the current profile while waiting or blocked.
- [x] Implement `make_input_rebinding_capture_action_ui_model` with retained `input_rebinding.capture` rows.

### Task 3: Visible Editor Wiring

- [x] Add `VirtualInput`, `VirtualPointerInput`, and `VirtualGamepadInput` members to `EditorState`, begin-frame them once per frame, and pass them to `SdlWindow::handle_event`.
- [x] Add an optional capture target and one-frame arm guard to the editor Input Rebinding panel state.
- [x] Render action-row `Capture` buttons and a `Cancel Capture` action without adding capture for axis rows.
- [x] While armed after the guard frame, call the editor-core capture model and replace `input_rebinding_profile_` with the captured candidate profile only when the model reports `captured`.
- [x] Show waiting/blocked/captured status and diagnostics without writing files or executing commands.

### Task 4: Documentation And Contract Checks

- [x] Update `engine/agent/manifest.json` current active plan to this plan while active, then return to the master plan when complete.
- [x] Update docs and skills to describe the narrow editor action capture path and unsupported follow-ups.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to assert the new API, docs, manifest, GUI wiring, and tests.

### Task 5: Validation And Commit

- [x] Run focused `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Stage only this slice and commit as `feat: add editor input rebinding action capture panel`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| Direct `cmake --build --preset dev --target mirakana_editor_core_tests` | Blocked | Local MSBuild invocation failed before compilation with a duplicate `Path`/`PATH` environment issue; repository wrapper was used for focused compiler evidence. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests` | Expected RED | Failed on missing `EditorInputRebindingCaptureDesc`, `EditorInputRebindingCaptureStatus`, `make_editor_input_rebinding_capture_action_model`, and `make_input_rebinding_capture_action_ui_model` before implementation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests` | Passed | Editor-core capture model and UI model compiled after implementation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Diagnostic RED | Initial success test used `escape`, which correctly conflicted with an existing `cancel` binding; the test was corrected to press `Key::right`. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests` | Passed | Focused editor-core build passed after test correction. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Passed | Focused `mirakana_editor_core_tests` passed after test correction. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | GUI preset built and desktop-gui CTest reported 46/46 tests passed, including `mirakana_editor_core_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | JSON contract checks include the completed plan, docs, and `input_rebinding.capture` evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration checks include the editor capture API names, visible `mirakana_editor` wiring, manifest guidance, and updated editor-productization gap notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit still reports 11 known unsupported gaps and keeps `editor-productization` partly ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public editor-core header changes stayed within the allowed API boundary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Applied clang-format after the initial format check flagged `editor/core/src/input_rebinding.cpp`; final format check passed. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests` | Passed | Focused editor-core build passed again after formatting. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Passed | Focused `mirakana_editor_core_tests` passed again after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | GUI preset built again after formatting and desktop-gui CTest reported 46/46 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed with known diagnostic-only Metal/Apple host gates and Android host readiness reported honestly. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Default dev build completed successfully. |

## Status

**Status:** Completed.
