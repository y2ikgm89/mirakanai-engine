# Editor Runtime Host Playtest Launch v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed Play-In-Editor runtime-host launch path so the visible editor can execute a selected desktop runtime host as an external process and record evidence without loading dynamic game modules into editor core.

**Architecture:** Extend `MK_editor_core` Play-In-Editor models with a deterministic launch-review contract over caller-supplied argv and host gates. The model only validates and exposes a safe `mirakana::ProcessCommand` plus retained `MK_ui` rows; the optional `MK_editor` shell owns process execution and transient evidence rows through the existing platform process runner.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_platform` process contracts, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow editor-productization gap:

- Keep Play-In-Editor source/simulation scene isolation and visible viewport controls intact.
- Add a reviewed runtime-host playtest launch model for selected desktop runtime commands.
- Allow host-gated commands only after explicit host-gate acknowledgement.
- Let the visible `MK_editor` shell execute ready launch commands through `mirakana::Win32ProcessRunner` on Windows and record transient evidence.
- Keep dynamic game-module loading, embedded runtime-host execution inside `editor/core`, package script execution, arbitrary shell execution, raw manifest command evaluation, free-form manifest edits, renderer/RHI handle exposure, package streaming, hot reload, and broad editor/runtime-host readiness unsupported.

## Context

- `EditorPlaySession` already snapshots source scenes into isolated simulation scenes, ticks them, and renders the simulation scene in the viewport.
- `EditorAiReviewedValidationExecutionModel` already proves the safe pattern for reviewed argv to `mirakana::ProcessCommand` plus visible-shell execution evidence.
- The master plan still lists dynamic game-module/runtime-host Play-In-Editor execution as a follow-up editor-productization gap.
- This slice chooses the host-feasible external runtime-host process path first; dynamic in-process game modules remain out of scope.

## Constraints

- Do not make `editor/core` depend on SDL3, Dear ImGui, `MK_runtime_host_sdl3`, RHI backends, native handles, or runtime game modules.
- Do not execute processes from `editor/core`; it may only review launch intent and build a safe command model.
- Do not parse or evaluate raw manifest command strings. Accept only caller-supplied reviewed argv tokens.
- Do not bypass host gates. Host-gated rows require explicit acknowledgement for every reviewed gate before `can_execute=true`.
- Keep retained UI ids stable under `play_in_editor.runtime_host`.
- Update docs, manifest, plan registry, and static checks so the ready claim stays narrow.

## Done When

- RED `MK_editor_core_tests` proves the runtime-host launch review contract is missing.
- `mirakana::editor::make_editor_runtime_host_playtest_launch_model` rejects unsafe, dynamic-module, editor-core-execution, native-handle, package-script, raw-manifest, and unacknowledged host-gated requests.
- Ready launch rows expose a safe `mirakana::ProcessCommand` and retained `play_in_editor.runtime_host.*` UI rows.
- The visible `MK_editor` Run/Viewport UI can execute ready runtime-host launch commands from the optional shell and record transient evidence without mutating editor-core state.
- Docs, master plan, registry, manifest, skills, and static checks record that external runtime-host launch evidence is implemented while dynamic modules and embedded runtime-host execution remain unsupported.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor runtime host playtest launch reviews safe external process commands")`.
- [x] Create a ready descriptor with:
  - `id = "sample_desktop_runtime_game"`,
  - `label = "Sample Desktop Runtime Game"`,
  - `working_directory = "."`,
  - `argv = {"out/install/desktop-runtime-release/bin/sample_desktop_runtime_game.exe", "--smoke", "--require-config", "runtime/sample_desktop_runtime_game.config", "--require-scene-package", "runtime/sample_desktop_runtime_game.geindex"}`.
- [x] Assert the model is `ready`, `can_execute=true`, and exposes a safe command with the first argv token as executable and the rest as arguments.
- [x] Assert retained UI ids exist:
  - `play_in_editor.runtime_host.sample_desktop_runtime_game.status`,
  - `play_in_editor.runtime_host.sample_desktop_runtime_game.command`,
  - `play_in_editor.runtime_host.sample_desktop_runtime_game.diagnostic`.
- [x] Add host-gated cases proving an unacknowledged `d3d12-windows-primary` gate is `host_gated`, then acknowledged gates become executable.
- [x] Add blocked cases for dynamic game module loading, editor-core execution, package scripts, raw manifest evaluation, native handle exposure, unsafe argv tokens, and empty argv.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm the build fails before implementation because the new model types/functions do not exist.

### Task 2: Editor-Core Launch Review Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/play_in_editor.hpp`
- Modify: `editor/core/src/play_in_editor.cpp`

- [x] Add `EditorRuntimeHostPlaytestLaunchStatus`.
- [x] Add `EditorRuntimeHostPlaytestLaunchDesc` with id, label, working directory, reviewed argv, host gates, acknowledged host gates, acknowledgement flag, and unsupported-claim booleans.
- [x] Add `EditorRuntimeHostPlaytestLaunchModel` with status, status label, `can_execute`, host-gate fields, blockers, unsupported claims, diagnostics, and `mirakana::ProcessCommand`.
- [x] Implement `make_editor_runtime_host_playtest_launch_model`.
- [x] Validate safe tokens, non-empty argv, matching host-gate acknowledgements, and `mirakana::is_safe_process_command`.
- [x] Reject dynamic game modules, editor-core execution, package scripts, arbitrary shell/raw manifest evaluation, free-form manifest edits, renderer/RHI/native handles, and package streaming claims.
- [x] Implement `make_editor_runtime_host_playtest_launch_ui_model` with retained `play_in_editor.runtime_host` rows.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Runtime-Host Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add transient editor state for runtime-host launch evidence and host-gate acknowledgement.
- [x] Build one reviewed launch descriptor for the current `sample_desktop_runtime_game` desktop runtime validation path using existing reviewed command metadata, but keep it as an external runtime-host process command rather than raw shell text.
- [x] Render a compact Run/Viewport runtime-host launch section with status, command, diagnostics, acknowledgement checkbox for host-gated rows, and an `Execute Runtime Host` button only when `can_execute=true`.
- [x] Execute ready commands through `mirakana::Win32ProcessRunner` on Windows and record pass/fail/blocked evidence from `mirakana::ProcessResult`.
- [x] On non-Windows hosts, record host-gated evidence instead of launching.
- [x] Keep execution in `MK_editor` only; do not route process execution through `editor/core`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record `Editor Runtime Host Playtest Launch v1`, `EditorRuntimeHostPlaytestLaunchDesc`, `EditorRuntimeHostPlaytestLaunchModel`, `make_editor_runtime_host_playtest_launch_model`, and retained `play_in_editor.runtime_host` ids.
- [x] Update the editor-productization gap to remove the broad claim that no runtime-host Play-In-Editor launch evidence exists.
- [x] Keep dynamic game-module loading, in-process runtime-host embedding, package scripts, arbitrary shell, raw manifest evaluation, free-form manifest edits, renderer/RHI/native handles, hot reload, package streaming, Vulkan/Metal material-preview display parity, automatic host-gated AI command execution, nested prefab propagation/merge UX, and broad editor productization unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `EditorRuntimeHostPlaytestLaunchDesc`, `EditorRuntimeHostPlaytestLaunchStatus`, `make_editor_runtime_host_playtest_launch_model`, and `make_editor_runtime_host_playtest_launch_ui_model`. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the editor-core launch review model and retained `play_in_editor.runtime_host` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`, `MK_editor_core_tests`, runtime-host SDL3 targets, and passed 46/46 desktop GUI CTest entries with the visible runtime-host playtest launch controls. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after manifest/docs/check updates. |
| `tools/check-json-contracts.ps1` | PASS | Contract checks passed after adding the runtime-host playtest launch guidance string requirement. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; editor-productization remains `partly-ready` with dynamic modules and in-process embedding unsupported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Passed after formatting the touched C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `git diff --check` | PASS | No whitespace errors in the final slice diff; repository line-ending warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported 29/29 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev configure/build completed with MSBuild exit 0. |
