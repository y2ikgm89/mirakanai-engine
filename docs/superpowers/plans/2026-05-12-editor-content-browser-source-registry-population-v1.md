# Editor Content Browser Source Registry Population v1 Implementation Plan (2026-05-12)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-content-browser-source-registry-population-v1`

**Status:** Completed

**Goal:** Let the GUI-independent editor Content Browser populate rows directly from `GameEngine.SourceAssetRegistry.v1` while preserving existing `AssetId` selection and retained Assets panel behavior.

**Architecture:** Add a `ContentBrowserState::refresh_from(const SourceAssetRegistryDocumentV1&)` overload that maps each registry row to the existing `ContentBrowserItem` shape: `id = asset_id_from_key_v2(row.key)`, `kind = row.kind`, `path = row.imported_path`, `asset_key_label = row.key.value`, and `identity_source_path = row.source_path`. Keep the prior `AssetRegistry` refresh overloads intact and add key-based selection as a convenience over the derived `AssetId`.

**Tech Stack:** C++23, `mirakana_editor_core`, `MK_assets`, `MK_ui`, CMake `dev` preset, `MK_editor_core_tests`.

---

## Context

- Read-only Asset Identity v2 annotation rows are complete in [2026-05-12-editor-content-browser-asset-identity-v2-rows-v1.md](2026-05-12-editor-content-browser-asset-identity-v2-rows-v1.md).
- The remaining `asset-identity-v2` gap still includes `editor asset browser migration`.
- `SourceAssetRegistryDocumentV1` is already the reviewed source identity/import intent surface for first-party texture, mesh, audio, material, and scene sources.
- This slice should make the Content Browser able to view those reviewed source rows without executing importers or replacing import/recook flows.

## Constraints

- Keep `editor/core` GUI-independent.
- Do not execute imports, recooks, package scripts, validation recipes, shell commands, or arbitrary manifest edits.
- Do not mutate `.geassets`, `.geindex`, packages, or runtime state.
- Do not remove `editor asset browser migration` from `requiredBeforeReadyClaim`; this closes source-registry population only, not visible UX polish or broader asset workflow migration.
- Add tests before production behavior and verify the RED failure.

## Done When

- `ContentBrowserState::refresh_from(const SourceAssetRegistryDocumentV1&)` builds deterministic rows from source registry entries.
- Source-registry browser rows expose stable key labels, source paths, derived `AssetId` values, imported-path display names/directories, and `identity_backed=true`.
- Text filtering works for key, source path, and imported path.
- `ContentBrowserState::select(AssetKeyV2)` selects the derived `AssetId`.
- `EditorContentBrowserImportPanelModel` and retained Assets panel rows work unchanged over source-registry-backed browser state.
- Focused editor tests/checks and full validation evidence are recorded below.

## File Map

- Modify `editor/core/include/mirakana/editor/content_browser.hpp`: include `source_asset_registry.hpp`, add source-registry refresh overload, and add key-based `select`.
- Modify `editor/core/src/content_browser.cpp`: build `ContentBrowserItem` rows from `SourceAssetRegistryRowV1` and preserve existing selection clearing.
- Modify `tests/unit/editor_core_tests.cpp`: add RED tests for source-registry population, key selection, filtering, and Assets panel model propagation.
- Update current docs, manifest fragment, and static integration checks after focused tests are green.

## Tasks

### Task 1: RED Tests

- [x] Add `editor content browser populates source registry rows` near the existing Content Browser tests.
- [x] The test should construct a `SourceAssetRegistryDocumentV1` with at least a material row and texture row, call `browser.refresh_from(source_registry)`, then assert derived ids, `path == imported_path`, `asset_key_label`, `identity_source_path`, and `identity_backed`.
- [x] The same test should filter by source path and key text, call `browser.select(material_key)`, and assert the selected row is the material.
- [x] Add or extend an Assets panel model test so `make_editor_content_browser_import_panel_model` receives a source-registry-backed `ContentBrowserState` and still exposes key/source labels for the selected asset.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.

Expected RED: compile failure because the source-registry refresh overload and `select(AssetKeyV2)` do not exist.

### Task 2: GREEN Implementation

- [x] Add the source-registry overload to `ContentBrowserState`.
- [x] Add a file-local `make_content_browser_item(const SourceAssetRegistryRowV1&)`.
- [x] Sort source-registry-backed items by `path` using the same ordering as `AssetRegistry` rows.
- [x] Add `bool ContentBrowserState::select(const AssetKeyV2& key) noexcept` that delegates to `select(asset_id_from_key_v2(key))`.
- [x] Keep existing `refresh_from(const AssetRegistry&)` and `refresh_from(const AssetRegistry&, const AssetIdentityDocumentV2&)` behavior unchanged.

### Task 3: Focused Verification

- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/content_browser.cpp,editor/core/src/content_browser_import_panel.cpp`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

### Task 4: Docs, Manifest, Static Checks

- [x] Update plan registry, roadmap/current capability docs, and `docs/ai-game-development.md` to record source-registry-backed Content Browser population.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and keep remaining `asset-identity-v2` blockers explicit.
- [x] Update `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` needles for the new source-registry population claim.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

### Task 5: Slice Closeout

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Update this plan status to `Completed` and record command evidence in `Validation Evidence`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_editor_core_tests` | RED then PASS | RED failed on missing `ContentBrowserState::refresh_from(SourceAssetRegistryDocumentV1)` and `select(AssetKeyV2)` overloads; PASS after implementation and validator-backed fixture cleanup. |
| `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | PASS | Focused editor-core regression lane passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/content_browser.cpp,editor/core/src/content_browser_import_panel.cpp` | PASS | Static C++ implementation check passed after changing key selection to `const AssetKeyV2&`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public editor-core header changed; boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS | Manifest fragment composed into `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON/static contract check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing integration check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface consistency check passed after static needle updates. |
| `Invoke-ScriptAnalyzer -Path tools/check-json-contracts.ps1,tools/check-ai-integration.ps1 -Severity Warning,Error` | PASS with existing warnings | Existing analyzer warnings were emitted; command exited successfully. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting gate passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; 47/47 CTest tests passed. Metal/Apple diagnostics remain host-gated on Windows. |
