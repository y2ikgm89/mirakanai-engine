# Editor Content Browser Import Native Dialog v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed native open-file dialog path for Content Browser asset import source selection without moving file dialogs, filesystem path conversion, or import execution into `editor/core`.

**Architecture:** Extend `MK_editor_core` Content Browser import panel contracts with an import-source native dialog request/review model over `mirakana::FileDialogResult` plus retained `MK_ui` rows. The optional `MK_editor` shell owns `mirakana::SdlFileDialogService`, project-store path conversion, and rebuilding a first-party source-document import plan from accepted in-project selections.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_platform` file dialog contracts, `MK_assets` import metadata, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow editor-productization gap:

- Add native file-open review for Content Browser import source selection.
- Allow multiple selected source files only when every selection is a supported first-party source document extension.
- Let visible `MK_editor` convert accepted in-project selections into a deterministic `AssetImportPlan` for `.texture`, `.mesh`, `.material`, `.scene`, and `.audio_source` files.
- Keep external file copying, PNG/glTF/audio codec import, package scripts, validation recipe execution, arbitrary shell, shader compiler execution, renderer/RHI work, package streaming, hot reload execution, manifest mutation, and broad editor/importer readiness unsupported.

## Context

- `EditorContentBrowserImportPanelModel` already summarizes assets, import queue rows, diagnostics, dependencies, thumbnail requests, material preview rows, and hot-reload summaries.
- Scene, Project, Prefab Variant, and Profiler dialog slices already establish the boundary: `editor/core` builds/reviews dialog data; `MK_editor` launches SDL3 dialogs and converts accepted paths.
- Current Assets panel import execution uses a built-in deterministic `AssetImportPlan`; this slice adds reviewed native source selection and plan replacement for first-party source documents only.

## Constraints

- Do not make `editor/core` depend on SDL3, Dear ImGui, filesystem path conversion, OS handles, `RootedFileSystem`, import execution, or third-party importer adapters.
- Do not read or copy selected files from `editor/core`.
- Do not accept external/out-of-project source paths in the visible shell until an explicit copy/import design exists.
- Do not infer PNG/glTF/common-audio imports from native dialog selections in this slice; use only first-party source document extensions.
- Keep retained UI ids stable under `content_browser_import.open_dialog`.
- Update docs, manifest, plan registry, skills, and static checks so the ready claim stays narrow.

## Done When

- RED `MK_editor_core_tests` proves the Content Browser import native dialog model and retained rows are missing.
- `make_content_browser_import_open_dialog_request` returns an open-file, multi-select request with first-party source document filters.
- `make_content_browser_import_open_dialog_model` accepts one or more `.texture`, `.mesh`, `.material`, `.scene`, or `.audio_source` selections and rejects empty accepted results, wrong extensions, canceled, failed, and malformed dialog results.
- `make_content_browser_import_open_dialog_ui_model` emits retained `content_browser_import.open_dialog` rows for status, selected count/filter, selected paths, and diagnostics.
- Visible `MK_editor` exposes `Browse Import Sources`, launches `mirakana::SdlFileDialogService`, converts accepted in-project selections to safe project-relative paths, rebuilds `asset_import_plan_` and `AssetPipelineState`, and reports blocked selections without executing imports automatically.
- Docs, master plan, registry, manifest, skills, and static checks record native Content Browser import dialog review as implemented while external copying, dependency importer execution, package scripts, shell, renderer/RHI, package streaming, hot reload execution, and broad editor/importer readiness remain unsupported.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor content browser import native file dialog reviews source selections")`.
- [x] Assert `make_content_browser_import_open_dialog_request("assets/source")` has:
  - `kind = mirakana::FileDialogKind::open_file`,
  - `title = "Import Assets"`,
  - `default_location = "assets/source"`,
  - `allow_many = true`,
  - filters for `texture`, `mesh`, `material`, `scene`, and `audio_source`.
- [x] Assert accepted multi-select paths such as `assets/source/hero.texture` and `assets/source/level.scene` produce `accepted=true`, `status_label = "Asset import open dialog accepted"`, and two selected path rows.
- [x] Assert retained UI ids exist:
  - `content_browser_import.open_dialog.status`,
  - `content_browser_import.open_dialog.selected_count`,
  - `content_browser_import.open_dialog.paths.1`,
  - `content_browser_import.open_dialog.paths.2`.
- [x] Add blocked cases for accepted empty selections, wrong extension, and malformed dialog results, plus canceled and failed cases.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm it fails before implementation because the new model types/functions do not exist.

### Task 2: Editor-Core Dialog Review Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`
- Modify: `editor/core/src/content_browser_import_panel.cpp`

- [x] Include `mirakana/platform/file_dialog.hpp`.
- [x] Add `EditorContentBrowserImportOpenDialogRow` and `EditorContentBrowserImportOpenDialogModel`.
- [x] Add declarations for `make_content_browser_import_open_dialog_request`, `make_content_browser_import_open_dialog_model`, and `make_content_browser_import_open_dialog_ui_model`.
- [x] Implement first-party source extension checks for `.texture`, `.mesh`, `.material`, `.scene`, and `.audio_source`.
- [x] Review `mirakana::FileDialogResult` with `mirakana::validate_file_dialog_result`; preserve canceled/failed diagnostics.
- [x] Accept multiple selected paths only when every path has a supported first-party extension.
- [x] Emit retained UI rows under `content_browser_import.open_dialog`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add transient dialog state:
  - `std::optional<mirakana::FileDialogId> asset_import_open_dialog_id_`,
  - `mirakana::editor::EditorContentBrowserImportOpenDialogModel asset_import_open_dialog_`,
  - import dialog status text.
- [x] Poll the asset import dialog each frame alongside the existing scene/project/profiler/prefab dialog pollers.
- [x] Add `Browse Import Sources` in the Assets panel near `Import Assets`.
- [x] Launch `mirakana::SdlFileDialogService` with `make_content_browser_import_open_dialog_request(project_.asset_root)`.
- [x] Convert each accepted selection to a safe project-relative path inside the project store and reject out-of-project selections.
- [x] Rebuild a deterministic `AssetImportMetadataRegistry` from accepted first-party source paths:
  - `.texture` -> `TextureImportMetadata` output under `imported/<stem>.texture`,
  - `.mesh` -> `MeshImportMetadata` output under `imported/<stem>.mesh`,
  - `.material` -> `MaterialImportMetadata` output under `imported/<stem>.material`,
  - `.scene` -> `SceneImportMetadata` output under `imported/<stem>.scene`,
  - `.audio_source` -> `AudioImportMetadata` output under `imported/<stem>.audio`.
- [x] Replace `asset_import_plan_`, call `asset_pipeline_.set_import_plan`, and report the reviewed selection without executing import automatically.
- [x] Keep `Import Assets` as the explicit execution button.
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

- [x] Record `Editor Content Browser Import Native Dialog v1`, `EditorContentBrowserImportOpenDialogModel`, `make_content_browser_import_open_dialog_request`, `make_content_browser_import_open_dialog_model`, `make_content_browser_import_open_dialog_ui_model`, and retained `content_browser_import.open_dialog` ids.
- [x] Update the editor-productization gap to remove the broad claim that no Content Browser native import dialog exists.
- [x] Keep external file copy/import, PNG/glTF/audio codec import from dialog selections, package scripts, arbitrary shell, validation recipe execution, shader compiler execution, renderer/RHI/native handles, hot reload execution, package streaming, manifest mutation, dynamic game-module loading, and broad editor/importer productization unsupported.
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
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation because `EditorContentBrowserImportOpenDialogModel`, `EditorContentBrowserImportOpenDialogRow`, `make_content_browser_import_open_dialog_request`, `make_content_browser_import_open_dialog_model`, and `make_content_browser_import_open_dialog_ui_model` were not defined. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the Content Browser import open dialog review model and retained `content_browser_import.open_dialog` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`, `MK_editor_core_tests`, SDL3 runtime host targets, and passed 46/46 GUI CTest entries with the visible `Browse Import Sources` adapter and import-plan replacement path. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static AI integration checks passed after syncing docs, manifest, skills, plan registry, and the new Content Browser import native dialog contract checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap audit reported 11 known gaps and preserved `editor-productization` as `partly-ready` without broad 1.0 readiness claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON contract check passed after adding the manifest guidance assertion for `currentEditorContentBrowserImportDiagnostics`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Failed once on `content_browser_import_panel.cpp` formatting, then passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core dialog model declarations. |
| `git diff --check` | PASS | Whitespace check passed; Git emitted only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed, including JSON contracts, AI integration, production-readiness audit, toolchain checks, tidy-check, full dev build, and 29/29 CTest tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit gate build passed for the `dev` preset after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. |
