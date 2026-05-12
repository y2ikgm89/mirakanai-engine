# Editor Game Module Driver Contract Metadata Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the editor game-module driver ABI contract machine-readable and visible as retained metadata rows so agents and editor users can distinguish the supported same-engine-build handoff from unsupported hot reload or stable third-party ABI claims.

**Architecture:** Keep contract metadata in `mirakana_editor_core` as a deterministic, non-mutating retained model. The optional `mirakana_editor` shell may display the same metadata beside load/reload controls, but dynamic-library execution, Play-In-Editor runtime handoff, and host-owned module lifetime stay in the existing reviewed paths.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-game-module-driver-contract-metadata-review-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-game-module-driver-safe-reload-review-v1.md`.
- Dynamic driver load and stopped-state reload are implemented, but the contract details are currently spread across constants, load/reload rows, docs, and validation checks.
- This slice exposes the contract as explicit metadata: ABI name/version, factory symbol, required callbacks, host-owned module lifetime, same-engine-build compatibility, and unsupported hot reload / stable third-party ABI / `DesktopGameRunner` embedding boundaries.

## Constraints

- Do not add active-session hot reload, automatic reload, or unload while Play-In-Editor is active.
- Do not claim stable third-party binary ABI; this remains a same-engine-build reviewed editor handoff.
- Do not add `DesktopGameRunner` embedding, package scripts, validation recipes, arbitrary shell, renderer/RHI uploads or handles, native handles, package streaming, or broad editor productization.
- Keep `mirakana_editor_core` free of OS dynamic-library calls and renderer/RHI dependencies.
- Keep retained row ids stable and deterministic under `play_in_editor.game_module_driver.contract`.

## Files

- Modify: `editor/core/include/mirakana/editor/game_module_driver.hpp`
  - Add `EditorGameModuleDriverContractMetadataRow`.
  - Add `EditorGameModuleDriverContractMetadataModel`.
  - Add `make_editor_game_module_driver_contract_metadata_model`.
  - Add `make_editor_game_module_driver_contract_metadata_ui_model`.
- Modify: `editor/core/src/game_module_driver.cpp`
  - Build deterministic contract metadata rows.
  - Reuse the same ABI constants and factory-symbol value used by load/reload review.
  - Add retained `play_in_editor.game_module_driver.contract` rows.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for contract rows, unsupported-boundary flags, and retained UI ids.
- Modify: `editor/src/main.cpp`
  - Display contract metadata beside visible Game Module Driver controls without changing execution semantics.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the contract metadata rows and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Keep this plan active and add contract metadata evidence under editor-productization without promoting the gap to ready.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinels for the new contract metadata symbols, retained ids, docs, skills, manifest, and plan.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update the current selected-gap pointer and verdict.

## Done When

- `mirakana_editor_core` exposes a deterministic contract metadata model for `GameEngine.EditorGameModuleDriver.v1`.
- Metadata rows identify the ABI name/version, factory symbol, required `tick`/`destroy` callbacks, optional `begin`/`end` callbacks, host-owned dynamic-library lifetime, same-engine-build requirement, and unsupported hot reload / stable third-party ABI / `DesktopGameRunner` embedding claims.
- Retained UI rows include `play_in_editor.game_module_driver.contract`.
- The visible editor displays contract metadata without executing new work or broadening load/reload behavior.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing contract metadata coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `editor game module driver contract metadata documents same engine ABI boundary`.
- [x] Assert ABI contract/version/factory symbol match the public constants.
- [x] Assert callback rows classify `tick` and `destroy` as required and `begin` / `end` as optional.
- [x] Assert same-engine-build is required while hot reload, stable third-party ABI, active-session reload, and `DesktopGameRunner` embedding are unsupported.
- [x] Assert retained UI rows include `play_in_editor.game_module_driver.contract`.
- [x] Build `mirakana_editor_core_tests` and verify the new test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new contract metadata model/functions.

### Task 2: Implement editor-core contract metadata

**Files:**
- Modify: `editor/core/include/mirakana/editor/game_module_driver.hpp`
- Modify: `editor/core/src/game_module_driver.cpp`

- [x] Add public metadata row/model structs.
- [x] Add `make_editor_game_module_driver_contract_metadata_model`.
- [x] Add deterministic row construction from existing ABI constants.
- [x] Add retained `play_in_editor.game_module_driver.contract` UI rows.
- [x] Keep the model non-mutating and non-executing.

### Task 3: Surface metadata in the visible editor

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Render the contract name/version and factory symbol in the Game Module Driver panel.
- [x] Render same-engine-build and unsupported stable ABI/hot reload messaging without changing load/reload execution behavior.

### Task 4: Synchronize docs, manifest, skills, and static checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Document contract metadata as reviewed evidence.
- [x] Keep unsupported claims explicit.
- [x] Add static sentinels for symbols, retained rows, docs, skills, manifest, and this plan.

### Task 5: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation with missing `make_editor_game_module_driver_contract_metadata_model`, `EditorGameModuleDriverContractMetadataRow`, and `make_editor_game_module_driver_contract_metadata_ui_model`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `cmake --build --preset dev --target mirakana_editor_core_tests`.
- PASS: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` (`1/1` tests passed).
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` (`46/46` GUI-preset tests passed).
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `git diff --check` after removing the extra trailing blank line from `engine/agent/manifest.json`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (`29/29` default-preset tests passed; diagnostic-only host gates remain Metal/Apple on this Windows host).

## Status

Completed.
