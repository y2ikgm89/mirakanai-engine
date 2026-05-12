# Editor Source Registry Visible Content Browser v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the visible `MK_editor` Assets panel load the project-owned `GameEngine.SourceAssetRegistry.v1` file and show source-registry-backed Content Browser rows without falling back to hard-coded cooked registry rows when a reviewed registry is available.

**Architecture:** Add an explicit `ProjectDocument::source_registry_path` so the editor does not guess `.geassets` locations from manifests or asset roots. Keep parsing, validation, Content Browser refresh, and import-plan projection in GUI-independent `MK_editor_core`; keep Dear ImGui limited to displaying status and invoking an explicit reload. Missing or invalid registries fail closed without package mutation, import execution, renderer/RHI residency, source edits, or free-form manifest updates.

**Tech Stack:** C++23, `MK_editor_core`, `MK_assets` `SourceAssetRegistryDocumentV1`, `ITextStore`, Dear ImGui `MK_editor`, CMake `dev` preset, `MK_editor_core_tests`, repository PowerShell validation.

---

## Scope

- In scope:
  - Add `project.source_registry` to `GameEngine.Project.v4`.
  - Default and migrate older project files to `source/assets/package.geassets`.
  - Validate that the configured path is safe project-relative and ends in `.geassets`.
  - Add an editor-core source-registry browser refresh helper that reads `ITextStore`, validates the registry, refreshes `ContentBrowserState`, and returns an `AssetImportPlan`.
  - Wire `MK_editor` startup, project open/create, import/recook refresh, and an explicit Assets panel reload button to prefer loaded source-registry rows.
  - Update docs, manifest pointers, and static integration checks.
- Out of scope:
  - Writing or repairing `.geassets` from the editor.
  - Automatic import execution, package mutation, cooked package writes, package streaming, renderer/RHI residency, raw manifest edits, or arbitrary shell execution.
  - Completing the broader scene/render/UI/gameplay reference cleanup blocker.

## File Map

- Create `editor/core/include/mirakana/editor/source_registry_browser.hpp`: public editor-core result model and refresh helper.
- Create `editor/core/src/source_registry_browser.cpp`: `ITextStore` read/parse/validate, `ContentBrowserState` refresh, and `AssetImportPlan` projection.
- Modify `editor/core/include/mirakana/editor/project.hpp`: `ProjectDocument::source_registry_path`.
- Modify `editor/core/src/project.cpp`: `GameEngine.Project.v4`, validation, serialization, deserialization, and migration defaults.
- Modify `editor/core/include/mirakana/editor/project_wizard.hpp` and `editor/core/src/project_wizard.cpp`: wizard draft/default/setter/validation.
- Modify `editor/CMakeLists.txt`: add the new editor-core source.
- Modify `editor/src/main.cpp`: include the helper, display project/Assets status, explicit reload, and source-registry-preserving refresh behavior.
- Modify `tests/unit/editor_core_tests.cpp`: RED/GREEN coverage for project serialization/migration/validation, wizard default, helper loaded/missing/invalid behavior.
- Modify `docs/editor.md`, `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`: align the ready surface and active plan truth.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` only if new literals need static needles.

## Tasks

### Task 1: Project contract RED/GREEN

- [x] Add failing `MK_editor_core_tests` cases:
  - serialized project includes `format=GameEngine.Project.v4` and `project.source_registry=source/assets/package.geassets`;
  - v3 migration defaults `source_registry_path`;
  - validation rejects `../package.geassets`, `C:/package.geassets`, and non-`.geassets` paths;
  - project wizard defaults the same source registry path.
- [x] Run:
  - `cmake --build --preset dev --target MK_editor_core_tests`
  - Expected RED: compile/test failure for missing `source_registry_path`.
- [x] Implement the minimal project/wizard changes.
- [x] Run:
  - `cmake --build --preset dev --target MK_editor_core_tests`
  - `ctest --preset dev --output-on-failure -R MK_editor_core_tests`
  - Expected GREEN.

### Task 2: Editor-core browser helper RED/GREEN

- [x] Add failing `MK_editor_core_tests` cases for `refresh_content_browser_from_project_source_registry`:
  - valid registry content loads `ContentBrowserState` from `SourceAssetRegistryDocumentV1`, returns `loaded`, asset count, registry path, and an import plan whose action ids match `asset_id_from_key_v2`;
  - missing registry returns `missing`, diagnostics, and does not mutate an existing browser selection/list;
  - invalid registry returns `blocked`, diagnostics, and does not mutate the browser.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests` and confirm RED for the missing helper/header.
- [x] Create `source_registry_browser.hpp/.cpp`, add it to `editor/CMakeLists.txt`, and implement only read/validate/refresh/import-plan projection.
- [x] Run:
  - `cmake --build --preset dev --target MK_editor_core_tests`
  - `ctest --preset dev --output-on-failure -R MK_editor_core_tests`

### Task 3: Visible `MK_editor` wiring

- [x] Add `source_registry_browser.hpp` include and shell state for status, diagnostics, loaded flag, and reload result.
- [x] Replace direct Content Browser refreshes after startup/project load/create/import/recook with a helper that:
  - starts from cooked built-in fallback rows;
  - attempts the configured project source registry;
  - preserves source-registry rows after import/recook while the registry is still loadable;
  - falls back to cooked rows only when no source registry is loaded.
- [x] Render in Assets panel:
  - registry path/status/asset count;
  - `Reload Source Registry` button;
  - bounded diagnostics from the helper.
- [x] Render `project.source_registry_path` in Project Settings and the New Project wizard.
- [x] Run:
  - `cmake --build --preset dev --target MK_editor`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`

### Task 4: Docs, manifest, and static checks

- [x] Update plan registry and master plan to name this slice as the active/completed visible editor browser migration work.
- [x] Update the manifest fragment, compose output, and current capability docs so `asset-identity-v2` truth says visible editor browser migration is implemented while broader scene/render/UI/gameplay reference cleanup remains.
- [x] Add static needles only for durable new literals such as `project.source_registry`, `source/assets/package.geassets`, `refresh_content_browser_from_project_source_registry`, and `Reload Source Registry`.
- [x] Run:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`

### Task 5: Slice validation and closeout

- [x] Run focused static checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/project.cpp,editor/core/src/project_wizard.cpp,editor/core/src/source_registry_browser.cpp,editor/src/main.cpp`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- [x] Run full gate:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- [x] Record validation evidence in this plan.
- [x] Set `currentActivePlan` back to the master plan and `recommendedNextPlan.id` back to `next-production-gap-selection` with completed context.

## Validation Evidence

| Check | Status | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_editor_core_tests` RED | Passed | Initial RED failed on missing `source_registry_browser.hpp`; the source-registry browser helper did not exist yet. |
| `cmake --build --preset dev --target MK_editor_core_tests` GREEN | Passed | Passed after implementation and again after formatting. |
| `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | Passed | 1/1 test passed after source-registry, project v4, and quaternion clip mapping fixes. |
| `cmake --build --preset dev --target MK_editor` | Passed | Passed after visible `MK_editor` source registry wiring. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | Built desktop GUI preset and passed 47/47 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/project.cpp,editor/core/src/project_wizard.cpp,editor/core/src/source_registry_browser.cpp,editor/src/main.cpp` | Passed | Expanded focused file set also included source registry, content-browser labels, asset pipeline, import panel, and editor-core tests; command ended with `tidy-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Composed `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Passed after targeted clang-format on touched C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository gate passed; shader Metal and Apple host checks remained diagnostic/host-gated on Windows. |
