# Runtime Input Rebinding Capture Contract v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent runtime input rebinding capture contract that turns the next first-party key, pointer, or gamepad-button press into a validated `RuntimeInputRebindingProfile` action override candidate.

**Architecture:** Keep capture in `mirakana_runtime` over `RuntimeInputActionMap`, `RuntimeInputRebindingProfile`, and first-party `VirtualInput` / `VirtualPointerInput` / `VirtualGamepadInput` state only. The helper does not open devices, mutate files, run editor UI, generate glyphs, consume UI focus, or expose native handles; it reuses existing rebinding profile validation for missing base rows, duplicate overrides, and trigger conflicts.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_platform` virtual input contracts, existing `MK_runtime_tests`, repository CMake/CTest plus PowerShell 7 scripts under `tools/`.

---

## Goal

Close the current input rebinding loophole after Input Rebinding Profile UX v1 and Editor Input Rebinding Profile Panel v1: profiles can be reviewed and persisted, but runtime/editor code has no reviewed first-party helper for capturing the next input press into a safe action override candidate.

## Context

- `RuntimeInputActionMap` already evaluates keyboard, pointer, and gamepad button actions through first-party virtual input state.
- `RuntimeInputRebindingProfile` already validates and applies full-row action and axis overrides over a base map.
- The editor currently exposes a read-only rebinding panel; this slice adds the host-independent digital capture helper only, while visible interactive panels, UI focus consumption, input glyphs, multiplayer device assignment, SDL3/native handles, and cloud/binary saves remain unsupported.
- The new helper should be usable by runtime gameplay, tools, or editor UI models without depending on SDL3, Dear ImGui, editor-private APIs, platform device handles, or middleware.

## Constraints

- Do not add dependencies, SDL3 APIs, OS input APIs, editor dependencies, native handles, UI focus routing, input consumption/bubbling, glyph generation, device assignment, binary saves, or cloud saves.
- Capture digital action triggers only: keyboard keys, pointer ids, and gamepad buttons. Axis capture remains a later explicit slice because thresholding, deadzones, response curves, and latest-device arbitration are still open.
- Detect simultaneous presses deterministically in this order: keyboard key enum order, pointer id order, then gamepad id and button enum order.
- If no allowed new press exists, return a waiting result without diagnostics and without changing the candidate profile.
- If a candidate press exists but existing rebinding validation fails, return a blocked result with the candidate trigger and diagnostics, but do not report success.

## Done When

- `mirakana_runtime` exposes capture request/result/status types and `capture_runtime_input_rebinding_action`.
- Unit tests prove key capture creates a one-row action override candidate, no input waits without mutation, deterministic source priority works, and missing base/conflicting triggers block through existing diagnostics.
- Docs, plan registry, master plan, manifest, and static contract checks record the capture contract while keeping UI focus consumption, glyphs, axis capture, multiplayer device assignment, native handles, SDL3, editor-private runtime APIs, and cloud/binary saves unsupported.
- Focused `MK_runtime_tests`, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing this slice.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/check-ai-integration.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add `runtime input rebinding capture creates action override from pressed key` to `tests/unit/runtime_tests.cpp`.
- [x] Add `runtime input rebinding capture waits when no allowed input is pressed` to prove no mutation.
- [x] Add `runtime input rebinding capture uses deterministic source priority` to prove keyboard-before-pointer and pointer fallback when keyboard capture is disabled.
- [x] Add `runtime input rebinding capture blocks missing base and conflicting trigger candidates` to prove existing diagnostics are reused.
- [x] Run focused `MK_runtime_tests` and confirm the expected compile failure before the new API exists.

### Task 2: Public API

- [x] Add `RuntimeInputRebindingCaptureStatus` with `waiting`, `captured`, and `blocked`.
- [x] Add `RuntimeInputRebindingCaptureRequest` with `context`, `action`, `RuntimeInputStateView state`, and booleans for keyboard, pointer, and gamepad-button capture.
- [x] Add `RuntimeInputRebindingCaptureResult` with status, optional trigger, action override, candidate profile, diagnostics, and `captured()` / `waiting()` helpers.
- [x] Declare `capture_runtime_input_rebinding_action`.

### Task 3: Implementation

- [x] Validate capture request context/action against the existing base map and return blocked diagnostics on invalid or missing base rows.
- [x] Scan pressed keyboard keys, pointer ids, and gamepad buttons deterministically according to the request allow flags.
- [x] Return waiting with the current profile unchanged when no candidate trigger is found.
- [x] Replace or append a single action override in a candidate profile, then call `validate_runtime_input_rebinding_profile`.
- [x] Return captured only when validation succeeds; otherwise return blocked with the candidate trigger and diagnostics.

### Task 4: Documentation And Contract Checks

- [x] Mark this plan completed with validation evidence.
- [x] Update plan registry and master plan current verdict/completed slice lists.
- [x] Update docs and `engine/agent/manifest.json` to describe the capture contract while preserving unsupported claims.
- [x] Update `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` to assert the new API, docs, and manifest evidence.

### Task 5: Validation And Commit

- [x] Run focused `MK_runtime_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add runtime input rebinding capture contract`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_tests` | Expected failure | RED confirmed: `RuntimeInputRebindingCaptureRequest`, `RuntimeInputRebindingCaptureStatus`, and `capture_runtime_input_rebinding_action` were not defined yet. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_runtime_tests` | Passed | Focused runtime test target built after capture API implementation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_runtime_tests` | Passed | `MK_runtime_tests` passed, including the new capture contract tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `json-contract-check: ok` after manifest and static contract updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok` after docs and manifest evidence were synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Production audit accepted the updated unsupported-gap language. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary accepted the new runtime capture contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | Applied clang-format to the touched runtime C++ files after the first format check found drift. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Repository formatting check passed after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; CTest reported 29/29 tests passing. Metal and Apple lanes remained diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Full dev preset build passed after validation. |

## Status

**Status:** Completed.
