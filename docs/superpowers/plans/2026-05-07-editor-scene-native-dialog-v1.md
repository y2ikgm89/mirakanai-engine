# Editor Scene Native Dialog v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-scene-native-dialog-v1`
**Status:** Completed
**Goal:** Add a narrow native open/save dialog review path for visible Scene authoring so `mirakana_editor` can browse `.scene` files through reviewed first-party contracts.
**Architecture:** Keep `mirakana_editor_core` responsible only for deterministic `mirakana::FileDialogRequest` / `mirakana::FileDialogResult` review models and retained `mirakana_ui` rows. Keep SDL3, Dear ImGui, native dialog polling, project-store path conversion, and actual `SceneAuthoringDocument` load/save in the optional `mirakana_editor` shell.
**Tech Stack:** C++23, `mirakana_platform` file dialog contracts, `mirakana_editor_core`, retained `mirakana_ui`, optional `mirakana_editor` SDL3/Dear ImGui adapter, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Existing scene authoring already uses `save_scene_authoring_document` and `load_scene_authoring_document` over safe project-store paths.
- Profiler and Prefab Variant now have native document dialogs; broader editor native save/open remains incomplete for Scene files.

## Constraints

- Do not move file IO, SDL3, Dear ImGui, OS paths, or native handles into `editor/core`.
- Do not add arbitrary project browsing, package scripts, validation recipe execution, runtime host execution, dynamic module loading, renderer/RHI uploads, or manifest mutation.
- Accept only one selected `.scene` path; canceled, failed, empty, multi-select, and wrong-extension results must produce deterministic diagnostics.
- The visible editor must convert accepted native paths back to safe project-store relative paths before existing scene load/save helpers are called.
- Replacing the active scene must stop active Play-In-Editor through the existing `replace_scene_document` path.

## Done When

- `mirakana_editor_core` exposes `EditorSceneFileDialogModel`, `make_scene_open_dialog_request`, `make_scene_save_dialog_request`, `make_scene_open_dialog_model`, `make_scene_save_dialog_model`, and `make_scene_file_dialog_ui_model`.
- `mirakana_editor` exposes visible File menu scene browse-open and browse-save controls that use `mirakana::IFileDialogService` / `mirakana::SdlFileDialogService`, update the current scene path, and reuse `load_scene_authoring_document` / `save_scene_authoring_document`.
- Docs, master plan, plan registry, manifest, editor skill guidance, and static checks describe this as a narrow Scene native dialog slice without broad editor file dialog claims.
- Validation evidence is recorded below.

## Tasks

- [x] Add RED `mirakana_editor_core` tests for scene open/save dialog request/review models and retained UI rows.
- [x] Implement scene dialog model/request/review helpers in `editor/core/include/mirakana/editor/scene_authoring.hpp` and `editor/core/src/scene_authoring.cpp`.
- [x] Wire visible `mirakana_editor` File menu `Open Scene...` / `Save Scene As...` controls through `mirakana::IFileDialogService` polling.
- [x] Update docs, master plan, plan registry, manifest, editor skill guidance, and AI integration static checks.
- [x] Run focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, formatting/static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | PASS | Initial RED failed on missing scene dialog APIs and `SceneAuthoringDocument::set_scene_path`; GREEN build passed after implementation. Fresh post-format build passed. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | PASS | `mirakana_editor_core_tests` 1/1 passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | `mirakana_editor` built and desktop-gui CTest passed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` corrected `scene_authoring.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public editor-core header boundary accepted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Manifest/docs/static sync accepted with Scene native dialog checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | 11 known non-ready gaps preserved; audit passed. |
| `git diff --check` | PASS | Whitespace gate passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Slice-closing gate passed; default CTest passed 29/29. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit gate passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. |
