# Editor Content Browser Asset Identity v2 Rows v1 Implementation Plan (2026-05-12)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-content-browser-asset-identity-v2-rows-v1`

**Status:** Completed

**Goal:** Add read-only Asset Identity v2 key/source provenance rows to the GUI-independent Content Browser and retained Assets panel models.

**Architecture:** Keep `AssetRegistry` as the current browser population source and preserve `AssetId` selection semantics. Add optional `AssetIdentityDocumentV2` annotation to `ContentBrowserState`, then surface `asset_key` and `identity_source_path` through `EditorContentBrowserAssetRow` and retained `content_browser_import.*` UI rows.

**Tech Stack:** C++23, `mirakana_editor_core`, `MK_ui`, `MK_assets`, CMake `dev` preset, `MK_editor_core_tests`.

---

## Context

- The remaining `asset-identity-v2` blockers include `editor asset browser migration`.
- `AssetRegistry` rows intentionally stay `AssetId`/kind/path only; adding stable keys there would ripple into material preview, scene authoring, prefab variant diagnostics, and other unrelated editor systems.
- A read-only annotation slice narrows the editor browser blocker without claiming full source-registry-backed browser population or broader scene/render/UI/gameplay cleanup.

## Constraints

- Keep `editor/core` GUI-independent.
- Do not execute imports, recooks, package scripts, shell commands, or arbitrary manifest edits.
- Do not replace `AssetRegistry` or `AssetId` selection in this slice.
- Do not remove `editor asset browser migration` from `requiredBeforeReadyClaim` yet.
- Add tests before production behavior and verify the RED failure.

## Done When

- `ContentBrowserItem` carries optional Asset Identity v2 key/source provenance.
- `ContentBrowserState::refresh_from(const AssetRegistry&, const AssetIdentityDocumentV2&)` annotates registry rows by `asset_id_from_key_v2`.
- Text filtering can match `asset_key` and `identity_source_path`.
- `EditorContentBrowserAssetRow` and selected asset rows expose `asset_key`, `identity_source_path`, and an identity status label.
- Retained UI includes `content_browser_import.assets.<id>.asset_key`, `content_browser_import.assets.<id>.identity_source_path`, `content_browser_import.selection.asset_key`, and `content_browser_import.selection.identity_source_path`.
- Focused editor tests/checks and full validation evidence are recorded below.

## File Map

- Modify `editor/core/include/mirakana/editor/content_browser.hpp`: add Asset Identity v2 fields and overload.
- Modify `editor/core/src/content_browser.cpp`: implement annotation, filter matching, and selection preservation.
- Modify `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`: add row fields.
- Modify `editor/core/src/content_browser_import_panel.cpp`: populate model fields and retained UI rows.
- Modify `tests/unit/editor_core_tests.cpp`: add focused Content Browser and Assets panel identity row assertions.
- Update docs/manifest/static checks after implementation evidence is green.

## Tasks

### Task 1: RED Tests

- [x] Extend `editor content browser filters sorts and selects assets` or add a nearby test that:
  - builds an `AssetRegistry` with a material and texture;
  - builds an `AssetIdentityDocumentV2` with matching keys/source paths;
  - calls `browser.refresh_from(registry, identity)`;
  - asserts item `asset_key`, `identity_source_path`, and `identity_backed`;
  - filters by stable key text.

- [x] Extend retained UI tests to assert:

```cpp
MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.assets." + std::to_string(material_id.value) + ".asset_key"}) != nullptr);
MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.assets." + std::to_string(material_id.value) + ".identity_source_path"}) != nullptr);
MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.selection.asset_key"}) != nullptr);
MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.selection.identity_source_path"}) != nullptr);
```

- [x] Run:

```powershell
cmake --build --preset dev --target MK_editor_core_tests
```

Expected RED: compile failure because the overload and row fields do not exist.

### Task 2: GREEN Implementation

- [x] Add `AssetKeyV2 asset_key`, `std::string asset_key_label`, `std::string identity_source_path`, and `bool identity_backed` to `ContentBrowserItem`.
- [x] Implement `ContentBrowserState::refresh_from(const AssetRegistry&, const AssetIdentityDocumentV2&)`.
- [x] Preserve existing `refresh_from(const AssetRegistry&)` by populating unannotated rows.
- [x] Include key/source fields in `visible_items()` filter matching.
- [x] Add matching fields to `EditorContentBrowserAssetRow` and populate them from `ContentBrowserItem`.
- [x] Emit retained UI labels for list rows and selection rows.

### Task 3: Focused Verification

- [x] Run:

```powershell
cmake --build --preset dev --target MK_editor_core_tests
ctest --preset dev --output-on-failure -R MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/content_browser.cpp,editor/core/src/content_browser_import_panel.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

### Task 4: Docs, Manifest, Static Checks

- [x] Update plan registry, roadmap/current capability docs, and the manifest fragment to record read-only Content Browser identity rows.
- [x] Keep `editor asset browser migration` required-before-ready because source-registry-backed population and broader UX migration remain follow-up work.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

### Task 5: Slice Closeout

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] Update this plan status to `Completed` and record command evidence in `Validation Evidence`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_editor_core_tests` | RED then PASS | RED failed because the `ContentBrowserState::refresh_from(const AssetRegistry&, const AssetIdentityDocumentV2&)` overload and identity row fields did not exist; final focused build passed. |
| `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | PASS | Focused editor-core regression lane passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting gate passed after applying repository clang-format to changed C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/content_browser.cpp,editor/core/src/content_browser_import_panel.cpp,engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp` | PASS | Static C++ implementation check passed for the editor identity row files plus the adjacent formatted scene migration file. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public editor-core header changed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS | Manifest fragment composed to `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON/static contract check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing integration check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice-closing validation passed; 47/47 CTest tests passed. Metal/Apple checks remain diagnostic-only host gates on Windows. |
