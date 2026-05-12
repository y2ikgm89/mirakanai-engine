# Editor Content Browser Import Codec Adapter Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the Content Browser reviewed import-source dialog build explicit import plans for supported PNG, glTF, and common-audio codec sources through the existing optional `MK_tools` importer adapters.

**Architecture:** Extend the existing `MK_editor_core` import dialog/copy review contracts so supported codec source paths are reviewed with the same retained `MK_ui` rows as first-party source documents. Keep adapter execution in the optional `MK_editor` shell by passing `mirakana::ExternalAssetImportAdapters` into the existing `execute_asset_import_plan` path; when the optional `asset-importers` build feature is unavailable, the adapters surface stable failure diagnostics instead of false readiness.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_tools` optional `asset-importers` adapters, `MK_assets` import metadata, `MK_platform` file dialog contracts, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow editor-productization gap:

- Extend `Browse Import Sources` beyond first-party `.texture`, `.mesh`, `.material`, `.scene`, and `.audio_source` documents to accepted codec source files:
  - `.png` as texture import input,
  - `.gltf` and `.glb` as mesh import input,
  - `.wav`, `.mp3`, and `.flac` as audio import input.
- Continue staging external selections into the project before import-plan rebuild, preserving explicit `Copy External Sources`.
- Keep `Import Assets` as the only import execution action.
- Execute codec imports only through the existing audited `MK_tools` `ExternalAssetImportAdapters` adapter set, never through ad hoc shell commands.

## Context

- Editor Content Browser Import Native Dialog v1 added reviewed file-dialog selection rows under `content_browser_import.open_dialog`.
- Editor Content Browser Import External Copy Review v1 added reviewed external-copy rows under `content_browser_import.external_copy`.
- `MK_tools` already implements optional `PngTextureExternalAssetImporter`, `GltfMeshExternalAssetImporter`, and `AudioExternalAssetImporter` adapters behind `MK_ENABLE_ASSET_IMPORTERS`.
- The visible editor currently rebuilds import plans only from first-party source document extensions and calls `execute_asset_import_plan` without adapter options.

## Constraints

- Do not add arbitrary importer adapters or arbitrary file import.
- Do not accept `.ogg` in this slice because the existing audited audio adapter supports `.wav`, `.mp3`, and `.flac`.
- Do not auto-execute import after selection or copy.
- Do not move adapter execution, filesystem copying, OS paths, SDL3, Dear ImGui, or native handles into `editor/core`.
- Do not claim broad importer readiness, dependency cooking, package scripts, validation execution, shader compiler execution, renderer/RHI/native handles, hot reload readiness, package streaming, manifest mutation, dynamic game-module loading, or in-process runtime-host embedding.

## Done When

- RED `MK_editor_core_tests` proves codec-source request filters, dialog acceptance, and external-copy review for codec source extensions are missing.
- `make_content_browser_import_open_dialog_request` includes codec filters for PNG, glTF/GLB, and common audio.
- `make_content_browser_import_open_dialog_model` accepts reviewed `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac` selections and still rejects unsupported paths.
- `make_content_browser_import_external_source_copy_model` stages supported codec source paths with safe project-relative targets without allowing arbitrary extensions.
- Visible `MK_editor` maps codec selections to deterministic `AssetImportMetadataRegistry` rows and output artifacts:
  - `.png` -> `AssetImportActionKind::texture` -> `imported/<stem>.texture`,
  - `.gltf` / `.glb` -> `AssetImportActionKind::mesh` -> `imported/<stem>.mesh`,
  - `.wav` / `.mp3` / `.flac` -> `AssetImportActionKind::audio` -> `imported/<stem>.audio`.
- Visible `MK_editor` calls `execute_asset_import_plan` with `mirakana::ExternalAssetImportAdapters::options()` for import and recook plan execution.
- Docs, master plan, registry, manifest, skills, and static checks record the narrow reviewed codec adapter path while arbitrary importer adapters and broad importer readiness remain unsupported.
- Focused editor-core tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` when local dependencies are available, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host/dependency blockers.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor content browser import codec adapter review accepts supported source formats")`.
- [x] Assert `make_content_browser_import_open_dialog_request("assets/source")` includes codec filters:
  - `mirakana::FileDialogFilter{"PNG Texture Source", "png"}`,
  - `mirakana::FileDialogFilter{"glTF Mesh Source", "gltf;glb"}`,
  - `mirakana::FileDialogFilter{"Common Audio Source", "wav;mp3;flac"}`.
- [x] Assert accepted paths `assets/source/hero.png`, `assets/source/ship.gltf`, and `assets/source/hit.wav` produce `accepted=true`, `status_label = "Asset import open dialog accepted"`, and retained `content_browser_import.open_dialog.paths.1` rows.
- [x] Assert `.ogg` still blocks with a diagnostic.
- [x] Assert `make_content_browser_import_external_source_copy_model` marks external `C:/drop/hero.png` targeting `assets/imported_sources/hero.png` as ready.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm it fails before implementation because codec filters/source extensions are not accepted yet.

### Task 2: Editor-Core Source Review

**Files:**
- Modify: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`
- Modify: `editor/core/src/content_browser_import_panel.cpp`

- [x] Replace the first-party-only dialog/copy extension helper with a supported import source policy that covers first-party documents plus `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac`.
- [x] Keep `.ogg`, unknown extensions, empty paths, malformed dialog results, line separators, and unsafe target paths blocked.
- [x] Add codec filters to `make_content_browser_import_open_dialog_request`.
- [x] Keep retained ids under `content_browser_import.open_dialog` and `content_browser_import.external_copy` unchanged.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Adapter Execution

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Include `mirakana/tools/asset_import_adapters.hpp`.
- [x] Extend `asset_import_kind_for_source_path` to map `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac` to texture, mesh, or audio actions.
- [x] Keep `.ogg` and unsupported extensions mapped to `AssetImportActionKind::unknown`.
- [x] Reuse the existing deterministic output path helper so codec sources write first-party cooked artifacts under `imported/`.
- [x] Keep external codec selections staged through `Copy External Sources` before import-plan rebuild.
- [x] Construct `mirakana::ExternalAssetImportAdapters` in the visible shell import and recook execution paths and pass `adapters.options()` into `execute_asset_import_plan`.
- [x] Preserve existing first-party source document behavior.
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

- [x] Record `Editor Content Browser Import Codec Adapter Review v1`, accepted `.png` / `.gltf` / `.glb` / `.wav` / `.mp3` / `.flac` selections, `ExternalAssetImportAdapters`, and visible adapter-backed `Import Assets`.
- [x] Update `unsupportedProductionGaps.editor-productization` notes so PNG/glTF/common-audio codec import from reviewed Content Browser selections is no longer listed as missing.
- [x] Keep arbitrary importer adapters, arbitrary file import, automatic import execution, package scripts, arbitrary shell, validation execution, shader compiler execution, renderer/RHI/native handles, hot reload readiness, package streaming, manifest mutation, dynamic game-module loading, and broad importer readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Optional Adapter Lane Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`.
- [x] No dependency-lane blocker was recorded because the optional asset-importers lane passed after fixing current fastgltf API and fixture compatibility.

### Task 6: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | `cmake --build --preset dev --target MK_editor_core_tests` built the new test, then `ctest --preset dev --output-on-failure -R MK_editor_core_tests` failed as expected on `request.filters.size() == 8U` before implementation. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests; ctest --preset dev --output-on-failure -R MK_editor_core_tests` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor` and reported `100% tests passed, 0 tests failed out of 46`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` | PASS | Fixed current fastgltf API/test fixture drift, then asset-importers CTest reported `100% tests passed, 0 tests failed out of 29`; installed asset-importers consumer reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap audit remained at 11 rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` after clang-format reported drift, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` reported `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported only existing LF-to-CRLF working-tree warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; dev CTest reported `100% tests passed, 0 tests failed out of 29`; shader/mobile/Apple host-only lanes remained diagnostic or host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev configure/build completed successfully for engine, editor-core tests, tools tests, and sample targets. |
