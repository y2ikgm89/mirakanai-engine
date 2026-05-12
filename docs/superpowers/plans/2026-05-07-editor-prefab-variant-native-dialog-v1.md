# Editor Prefab Variant Native Dialog v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-prefab-variant-native-dialog-v1`
**Status:** Completed
**Goal:** Add a narrow native open/save dialog review path for visible Prefab Variant authoring so `mirakana_editor` can browse `.prefabvariant` files outside the Profiler-only dialog path.
**Architecture:** Keep request/result review in `mirakana_editor_core` as GUI-independent `mirakana::FileDialogRequest` / `mirakana::FileDialogResult` models with retained `mirakana_ui` rows. Keep SDL3/Dear ImGui polling and project-store path conversion inside the optional `mirakana_editor` shell, then reuse existing `load_prefab_variant_authoring_document` / `save_prefab_variant_authoring_document`.
**Tech Stack:** C++23, `mirakana_platform` file dialog contracts, `mirakana_editor_core`, retained `mirakana_ui`, optional `mirakana_editor` SDL3/Dear ImGui adapter, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- The Profiler now has native Trace JSON open/save dialogs, but broader editor native save/open dialogs remain unsupported.
- Prefab Variant authoring already has visible `Variant Path`, `Load Variant`, and `Save Variant` controls over first-party `.prefabvariant` text-store IO.

## Constraints

- Do not move file IO, SDL3, Dear ImGui, OS paths, or native handles into `editor/core`.
- Do not add arbitrary project browsing, project migration, dynamic package scripts, validation execution, shader tool execution, or runtime host execution.
- Accept only one selected `.prefabvariant` path; canceled, failed, empty, multi-select, and wrong-extension results must produce deterministic model diagnostics.
- The visible editor must convert accepted native paths back to safe project-store relative paths before existing load/save helpers are called.
- Do not mutate or load files from the editor-core model.

## Done When

- `mirakana_editor_core` exposes `EditorPrefabVariantFileDialogModel`, `make_prefab_variant_open_dialog_request`, `make_prefab_variant_save_dialog_request`, `make_prefab_variant_open_dialog_model`, `make_prefab_variant_save_dialog_model`, and `make_prefab_variant_file_dialog_ui_model`.
- `mirakana_editor` exposes visible Prefab Variant browse-open and browse-save controls that use `mirakana::IFileDialogService` / `mirakana::SdlFileDialogService`, update `Variant Path`, and reuse existing load/save document paths.
- Docs, master plan, plan registry, manifest, editor skill guidance, and static checks describe this as the first non-Profiler native document dialog slice without broad editor file dialog claims.
- Validation evidence is recorded below.

## Tasks

- [x] Add RED `mirakana_editor_core` tests for prefab variant open/save dialog request/review models and retained UI rows.
- [x] Implement prefab variant dialog model/request/review helpers in `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp` and `editor/core/src/prefab_variant_authoring.cpp`.
- [x] Wire visible `mirakana_editor` Prefab Variant `Browse Load Variant` / `Browse Save Variant` controls through `mirakana::IFileDialogService` polling.
- [x] Update docs, master plan, plan registry, manifest, editor skill guidance, and AI integration static checks.
- [x] Run focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, formatting/static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | PASS | RED failed first on missing prefab variant dialog APIs, then GREEN focused build passed after implementation and formatting. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | PASS | Focused editor-core test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | `mirakana_editor` built and desktop-gui CTest passed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Failed once before `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; passed after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public `mirakana_editor_core` header changes accepted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Manifest/docs/static sync accepted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap audit still reports `editor-productization` partly-ready. |
| `git diff --check` | PASS | Whitespace check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Slice-closing gate passed; CTest reported 29/29 tests passing. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit gate passed after validation. |
