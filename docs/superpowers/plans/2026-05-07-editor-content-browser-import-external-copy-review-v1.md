# Editor Content Browser Import External Copy Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Let the Content Browser reviewed import-source dialog stage external first-party source documents into the project before rebuilding the import plan.

**Architecture:** Extend `MK_editor_core` with a GUI-independent external-source copy review model that receives caller-supplied source/target rows and emits retained `MK_ui` diagnostics. Keep native file dialog path conversion and filesystem copying in the optional `MK_editor` shell; after explicit copy success, reuse the existing first-party `AssetImportPlan` rebuild path.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_platform` file dialog contracts, `MK_assets` import metadata, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow editor-productization gap:

- Keep `Browse Import Sources` accepting first-party source document extensions.
- Continue accepting in-project `.texture`, `.mesh`, `.material`, `.scene`, and `.audio_source` selections without a copy step.
- For external selections with the same first-party extensions, build reviewed copy rows that stage files under a deterministic project-relative import source directory.
- Execute copying only from the visible `MK_editor` shell after the user explicitly chooses `Copy External Sources`.
- Rebuild `asset_import_plan_` only after all reviewed copies succeed.

## Context

- Editor Content Browser Import Native Dialog v1 added `EditorContentBrowserImportOpenDialogModel` and visible `Browse Import Sources`.
- The current visible shell rejects out-of-project selections even when the selected file is a first-party source document.
- `editor/core` already owns retained UI models and diagnostics, while `MK_editor` owns SDL3 dialogs, path conversion, `RootedFileSystem`, and import execution.

## Constraints

- Do not add PNG/glTF/common-audio codec import from native dialog selections.
- Do not copy arbitrary extensions; accept only `.texture`, `.mesh`, `.material`, `.scene`, and `.audio_source`.
- Do not overwrite an existing project file in this slice; report a blocked target instead.
- Do not move filesystem copying, OS paths, SDL3, Dear ImGui, native handles, or `RootedFileSystem` into `editor/core`.
- Do not execute import automatically after copy; `Import Assets` remains the explicit import execution action.
- Keep package scripts, arbitrary shell, validation recipe execution, shader compiler execution, renderer/RHI work, package streaming, hot reload execution, manifest mutation, dynamic game-module loading, and broad importer readiness unsupported.

## Done When

- RED `MK_editor_core_tests` proves the external copy review model, retained rows, and blocked target diagnostics are missing.
- `make_content_browser_import_external_source_copy_model` reviews caller-supplied copy rows, accepts only first-party source document source/target paths, reports existing target blockers, and distinguishes ready/copied/blocked/failed rows.
- `make_content_browser_import_external_source_copy_ui_model` emits retained `content_browser_import.external_copy` rows for status, copy count, selected sources, target project paths, and diagnostics.
- Visible `MK_editor` classifies accepted dialog selections as in-project or external, builds deterministic external copy targets under the project asset root, blocks existing target collisions, and renders `Copy External Sources`.
- Visible `MK_editor` copies external first-party source documents only after the explicit copy action, then rebuilds the import plan from copied project-relative source paths plus any already in-project selections.
- Docs, master plan, registry, manifest, skills, and static checks record external first-party source document copy review as implemented while codec adapters and broad importer readiness remain unsupported.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice-closing commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor content browser import external source copy review keeps copying explicit")`.
- [x] Assert a ready row for external `C:/drop/hero.texture` targeting `assets/imported_sources/hero.texture`.
- [x] Assert retained UI ids exist:
  - `content_browser_import.external_copy.status`,
  - `content_browser_import.external_copy.copy_count`,
  - `content_browser_import.external_copy.rows.1.source`,
  - `content_browser_import.external_copy.rows.1.target`.
- [x] Add blocked cases for unsupported source extension, unsafe target path, existing target, empty source, and failed copy diagnostics.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm it fails before implementation because the new model types/functions do not exist.

### Task 2: Editor-Core Copy Review Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`
- Modify: `editor/core/src/content_browser_import_panel.cpp`

- [x] Add `EditorContentBrowserImportExternalSourceCopyStatus`.
- [x] Add `EditorContentBrowserImportExternalSourceCopyInput`, `EditorContentBrowserImportExternalSourceCopyRow`, and `EditorContentBrowserImportExternalSourceCopyModel`.
- [x] Declare `make_content_browser_import_external_source_copy_model` and `make_content_browser_import_external_source_copy_ui_model`.
- [x] Reuse the existing first-party source extension policy for source and target paths.
- [x] Validate target project paths as safe relative paths with no absolute path, empty segment, parent segment, newline, carriage return, or `=`.
- [x] Report existing target collisions as blocked.
- [x] Preserve caller-supplied failed-copy diagnostics without throwing.
- [x] Emit retained UI rows under `content_browser_import.external_copy`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add transient state for pending external copy rows and the external copy review model.
- [x] Classify each accepted dialog selection:
  - in-project first-party source document -> keep existing project-relative path,
  - external first-party source document -> target `assets/imported_sources/<filename>`,
  - unsupported/malformed path -> block with a diagnostic.
- [x] Check external source existence with `std::filesystem::exists`.
- [x] Check project target collisions through `project_store_.exists`.
- [x] Render external copy review rows in the Assets panel below the open-dialog rows.
- [x] Add `Copy External Sources` and enable it only when the review model can copy.
- [x] Execute copying in `MK_editor` with `std::filesystem::create_directories` and `std::filesystem::copy_file` without overwrite.
- [x] After all copies succeed, rebuild `asset_import_plan_` from the staged project-relative paths plus already in-project selections.
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

- [x] Record `Editor Content Browser Import External Copy Review v1`, `EditorContentBrowserImportExternalSourceCopyModel`, `make_content_browser_import_external_source_copy_model`, `make_content_browser_import_external_source_copy_ui_model`, retained `content_browser_import.external_copy` ids, and visible `Copy External Sources`.
- [x] Update the editor-productization gap so external first-party source document copy review is no longer listed as missing.
- [x] Keep PNG/glTF/common-audio codec import, arbitrary file import, automatic import execution, package scripts, arbitrary shell, validation recipe execution, shader compiler execution, renderer/RHI/native handles, hot reload execution, package streaming, manifest mutation, dynamic game-module loading, and broad importer readiness unsupported.
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
| RED focused build/test | PASS expected failure | `cmake --build --preset dev --target MK_editor_core_tests` failed after the RED test because `EditorContentBrowserImportExternalSourceCopyInput`, `EditorContentBrowserImportExternalSourceCopyStatus`, `make_content_browser_import_external_source_copy_model`, and `make_content_browser_import_external_source_copy_ui_model` did not exist yet. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the editor-core model. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`, `MK_editor_core_tests`, and desktop GUI/runtime targets; GUI CTest reported 46/46 tests passing. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`; added the `currentEditorContentBrowserImportDiagnostics` external-copy assertions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static AI integration checks passed after docs/manifest/skills sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `editor-productization` remains `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial run caught clang-format drift in `content_browser_import_panel.hpp`; after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed for the new editor-core header model. |
| `git diff --check` | PASS | Whitespace check passed after trimming the manifest EOF. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed, including agent/json/production audit, toolchain, tidy, build, and 29/29 default CTest tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default `dev` build completed after validation. |
