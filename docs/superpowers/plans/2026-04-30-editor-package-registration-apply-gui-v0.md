# Editor Package Registration Apply GUI v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed editor GUI apply workflow that writes approved scene package runtime files into `game.agent.json.runtimePackageFiles`.

**Architecture:** Keep durable package review and manifest update behavior in `MK_editor_core` over `ITextStore`, with a narrow `runtimePackageFiles` updater instead of a new JSON dependency or CMake invocation. Keep `MK_editor` as the Dear ImGui adapter that renders the reviewed plan, exposes an Apply button, records the result, and refreshes its in-memory package file list.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, existing `ITextStore`/`FileTextStore`, existing unit/GUI validation and static AI integration checks.

---

## Context

Editor Package Registration Draft Workflow v0 already classifies scene package candidates as `add_runtime_file`, `already_registered`, source-only, unsafe, or duplicate. Desktop Runtime Package Registration Apply Tool v0 already provides a PowerShell lane for reviewed manifest appends outside the editor. The remaining authoring-loop gap is a visible editor apply button that can apply only the already-reviewed draft additions while preserving the existing runtime/game API boundary.

This slice does not add CMake invocation, package building, file generation, third-party JSON parsing, renderer/RHI access, SDL3/Dear ImGui exposure to game APIs, or arbitrary manifest editing.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Sync after implementation: `docs/roadmap.md`, `docs/editor.md`, `docs/workflows.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, and relevant Codex/Claude subagent guidance if stale.

## Task 1: Failing Editor-Core Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Add tests for reviewed apply-plan filtering.**

Add a test near the existing package draft tests:

```cpp
MK_TEST("editor scene package registration apply plan filters reviewed additions") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> rows{
        {mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         "games/sample/runtime/scenes/start.scene",
         "runtime/scenes/start.scene",
         true,
         mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file,
         "add runtimePackageFiles entry"},
        {mirakana::editor::ScenePackageCandidateKind::package_index,
         "games/sample/runtime/start.geindex",
         "runtime/start.geindex",
         true,
         mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         "runtime file is already registered"},
    };

    const auto plan =
        mirakana::editor::make_scene_package_registration_apply_plan(rows, "games/sample", "game.agent.json");

    MK_REQUIRE(plan.can_apply);
    MK_REQUIRE(plan.game_manifest_path == "games/sample/game.agent.json");
    MK_REQUIRE(plan.runtime_package_files.size() == 1);
    MK_REQUIRE(plan.runtime_package_files[0] == "runtime/scenes/start.scene");
}
```

- [x] **Step 2: Add tests for manifest apply behavior.**

Add coverage that writes an in-memory game manifest, applies a plan, verifies `runtimePackageFiles` order/idempotence, and verifies non-array `runtimePackageFiles` is rejected without writing.

- [x] **Step 3: Run the focused test and confirm RED.**

Run: `ctest --preset dev --output-on-failure -R MK_editor_core_tests`

Expected before implementation: build/test fails because `make_scene_package_registration_apply_plan` and `apply_scene_package_registration_to_manifest` do not exist.

## Task 2: Editor-Core Apply Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] **Step 1: Add `ScenePackageRegistrationApplyPlan` and `ScenePackageRegistrationApplyResult`.**

Expose value types with `game_manifest_path`, reviewed `runtime_package_files`, `can_apply`, `applied`, `diagnostic`, and returned merged `runtime_package_files`.

- [x] **Step 2: Implement plan construction.**

The plan joins `project_root_path` with `game_manifest_path`, filters only `add_runtime_file` rows, keeps draft order, rejects unsafe manifest path fields, and reports `no runtime package files to apply` when there are no additions.

- [x] **Step 3: Implement narrow manifest update.**

Read manifest text through `ITextStore`, parse only the top-level `runtimePackageFiles` string array, append missing reviewed paths, create the property when absent, reject malformed/non-array values, and write deterministic JSON text back through the store. Do not edit unrelated manifest fields beyond adding/replacing the one property.

## Task 3: Visible Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] **Step 1: Render the apply status next to the draft table.**

Show the manifest path, count of reviewed additions, and the last apply diagnostic.

- [x] **Step 2: Add `Apply Package Registration` button.**

The button calls `apply_scene_package_registration_to_manifest(project_store_, plan)`, logs success/failure, and refreshes `manifest_runtime_package_files_` from the result when applied.

- [x] **Step 3: Keep the adapter scoped.**

Do not run CMake, package scripts, shader tools, or expose editor/Dear ImGui APIs to game code.

## Task 4: Static Checks And Guidance Sync

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/roadmap.md`
- Modify: `docs/editor.md`
- Modify: `docs/workflows.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Add static checks for the apply model and button.**

Require `ScenePackageRegistrationApplyPlan`, `apply_scene_package_registration_to_manifest`, and `Apply Package Registration` in the appropriate files.

- [x] **Step 2: Update docs and agent guidance.**

Mark GUI apply as implemented, keep the PowerShell tool documented as the headless/manual apply lane, and keep non-goals explicit: package build, CMake edits, broad JSON editing, and shader/material generation remain follow-up work.

## Task 5: Validation

- [x] **Step 1: Run focused editor-core validation.**

Run: `ctest --preset dev --output-on-failure -R MK_editor_core_tests`

- [x] **Step 2: Run GUI validation because `editor/src/main.cpp` changed.**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

- [x] **Step 3: Run API boundary check because an editor-core public header changed.**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`

- [x] **Step 4: Run static agent/schema/format checks if needed.**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

- [x] **Step 5: Run default validation.**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Done When

- [x] `MK_editor_core` exposes tested reviewed apply-plan and manifest update contracts.
- [x] `MK_editor` shows a package registration apply button and result diagnostics.
- [x] Static checks and agent/docs guidance distinguish GUI apply from package build, CMake mutation, and arbitrary manifest editing.
- [x] Focused editor-core, GUI, API-boundary, agent/static checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have fresh results or concrete blockers.

## Validation Evidence

- Focused editor-core build and CTest passed: `MK_editor_core_tests` built through the dev preset and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed.
- GUI validation passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: desktop-gui configure/build completed and 33/33 tests passed. The sandboxed run hit the known vcpkg 7zip stdin/CreateFileW permission issue, so the same command was rerun with approval outside the sandbox.
- Public editor-core header change passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Static guidance checks passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- Final default validation passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
