# Editor Asset Browser Production v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a clean-break, first-party, production-quality `MK_editor` Assets panel that turns project source assets, import/cook/package state, preview evidence, hot-reload readiness, and legal provenance into one deterministic AI-operable asset browser.

**Architecture:** `MK_editor_core` owns value-only asset browser documents, query/filter planning, command planning, provenance/legal review rows, and retained `mirakana::ui` output. The native `MK_editor` shell owns Win32 file dialogs, explicit reviewed import/copy/cook execution, private preview cache execution, clipboard/accessibility publication, and D3D12 presentation evidence without exposing native handles. Runtime/package systems consume only validated `AssetKeyV2`, cooked artifacts, and package registration rows.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_ui`, `GameEngine.SourceAssetRegistry.v1`, `GameEngine.AssetIdentity.v2`, existing `MK_tools` asset import adapters, optional `asset-importers` vcpkg features, Win32 Common Item Dialog, Windows path canonicalization APIs, Windows UI Automation, repository PowerShell validation wrappers.

---

## Status

- **Plan ID:** `editor-asset-browser-production-v1`
- **Status:** Candidate plan, not selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- **Date:** 2026-06-28
- **Scope:** Generic editor `Content Browser` / visible `Assets` panel productionization. This does not replace `EnvironmentArtistWorkflowAssetBrowser*`; that environment-specific asset browser remains a specialized read-only workflow model.
- **Non-goal:** Backward-compatible support for the current hard-coded `EditorAssetListRow` visible shell path. This plan intentionally removes that path after replacement evidence is green.

## Research And Source Use

The following sources are approved only as factual category/API/legal context. They are not implementation sources, design-expression sources, sample-code sources, icon sources, asset sources, UI-layout sources, or compatibility targets.

| Source | Use allowed in this plan | Use forbidden in this plan |
| --- | --- | --- |
| Context7 `/microsoftdocs/cpp-docs` | Confirms C++ standard-library value containers and RAII as the public editor-core data-model baseline. | Copying examples, adopting MFC/Windows handles in `editor/core`, or using platform APIs in public core types. |
| Context7 `/academysoftwarefoundation/openexr` | Confirms EXR review/cook metadata should include header, data/display window, channels, pixel types, frame-buffer/read policy, multipart/deep/tiled policy, and explicit color-intent rows. | Treating any EXR as scene-linear without declared metadata, copying sample code, or making OpenEXR a default dependency outside the optional importer feature. |
| Context7 `/khronosgroup/ktx-software` | Confirms KTX2/Basis review/cook metadata should include `ktxTexture2_NeedsTranscoding`, selected transcode target, backend format support evidence, dimensions, levels, layers, faces, color model, and payload byte count. | Runtime upload from editor core, unconditional BC/ASTC/ETC inference, copying sample code, or making KTX default outside the optional importer feature. |
| Unity Project window documentation | Category taxonomy: project files, assets, navigation/search/import expectations. | Copying Unity layouts, shortcuts, UI names beyond descriptive nominative references, icons, assets, project schema, package formats, workflows, or compatibility claims. |
| Unreal Engine Content Browser documentation | Category taxonomy: asset browsing, folder/source organization, import/management/preview expectations. | Copying Content Browser expression, drawer layout, terminology as product branding, collection behavior, public APIs, assets, samples, icons, or compatibility claims. |
| Godot FileSystem/import documentation | Category taxonomy: filesystem visibility, import/reimport expectations, generated import artifacts. | Copying Godot dock workflow, filesystem labels, import formats as schema, editor visual expression, project metadata, code, samples, icons, or compatibility claims. |
| Microsoft Common Item Dialog / `IFileDialog` documentation | Native shell file-open/save routing for explicit import-source selection. | Moving native dialogs or path conversion into `editor/core`; using deprecated common-dialog APIs as the selected Windows path. |
| Microsoft path naming and `PathCch*` documentation | Native shell path canonicalization and project-root containment checks before external copy/import execution. | Accepting device paths, uncanonicalized traversal, shell-expanded paths, or arbitrary absolute paths inside editor-core models. |
| Microsoft UI Automation documentation | Accessibility publication requirements for tree/list/grid, selection, invoke, scroll, and item metadata. | Claiming full UIA parity or cross-platform accessibility from Windows-only evidence. |
| Unity legal/trademark, Epic trademark/EULA, Godot license/trademark documentation | Clean-room and trademark guardrails. | Using logos, marks, sample assets, marketplace/Fab/Asset Store material, proprietary docs text, engine-specific schema, API names, or compatibility/equivalence/parity marketing. |

This plan is not legal advice. Before shipping third-party assets, code, models, fonts, images, codecs, or sample content through the asset browser, update `THIRD_PARTY_NOTICES.md`, `docs/legal-and-licensing.md`, `docs/dependencies.md`, and `vcpkg.json` as applicable, then run the dependency-policy checks.

## Current Project Baseline

- `ContentBrowserState` already refreshes deterministic flat rows from `AssetRegistry`, `AssetIdentityDocumentV2`, or `SourceAssetRegistryDocumentV1`.
- `refresh_content_browser_from_project_source_registry` already reads `ProjectDocument::source_registry_path`, validates `GameEngine.SourceAssetRegistry.v1`, refreshes browser rows, and projects an `AssetImportPlan` without writing `.geassets` or executing imports.
- `EditorContentBrowserImportPanelModel` already combines visible assets, selected asset details, import queue/progress, diagnostics, dependency rows, thumbnail requests, material preview rows, and hot-reload summaries.
- `EditorContentBrowserImportOpenDialogModel` and `EditorContentBrowserImportExternalSourceCopyModel` already review source selections and external copy staging without executing automatically.
- `EditorMaterialAssetPreviewPanelModel` already reports selected material metadata, texture payload rows, shader readiness, host-owned GPU preview evidence, and no native handles.
- The native shell still exposes hard-coded `EditorAssetListRow` values through `NativeEditorApp::asset_rows()` / `make_asset_list_ui_model`; this is the clean-break replacement target.

## Clean-Room Product Direction

The production asset browser must be a MIRAIKANAI-first tool, not a clone of Unity, Unreal Engine, or Godot. The selected product concept is **Source Pulse**:

- One row per project-owned source/cooked/package artifact identity, keyed by `AssetKeyV2`.
- Every row exposes source path, imported/cooked path, kind, package visibility, provenance/license status, import plan status, dependency status, preview status, hot-reload status, and blocked/host-gated reasons.
- The browser answers "can this asset safely enter the runtime package?" instead of only "where is this file?"
- Search is a first-party query grammar: `kind=<asset-kind>`, `scope=source|cooked|package|hot_reload`, `state=ready|blocked|host_gated|missing`, `key=<asset-key-prefix>`, `path=<substring>`, and plain text. Do not implement Unity `t:` / `l:` filters, Unreal collection syntax, or Godot dock terms.
- The visible panel may use tree/list/detail/preview regions because those are generic UI categories, but row ids, labels, layout metrics, commands, icons, shortcuts, and user-facing names must be first-party.

## Non-Negotiable Claim Rules

| Claim | Required evidence | Explicit non-evidence |
| --- | --- | --- |
| `editor_asset_browser_core_ready` | `MK_editor_core` tests prove deterministic Source Pulse rows, query/filter semantics, source-registry generation safety, retained UI ids, clean-room/legal rows, and fail-closed mutation/execution/native-handle flags. | Existing flat `ContentBrowserState` rows alone. |
| `editor_asset_browser_visible_ready` | `MK_editor` shell tests prove the visible Assets panel consumes the production model, no hard-coded asset rows remain, native dialogs are shell-owned, accessibility rows are published, and panel smoke counters are positive. | A future-doc statement or retained core model without shell binding. |
| `editor_asset_browser_import_ready` | Explicit reviewed command plan, user-confirmed shell execution, optional adapter diagnostics, source/copy/cook evidence, package candidate rows, rollback diagnostics, and no automatic import. | File selection, copied external source rows, or codec availability alone. |
| `editor_asset_browser_preview_ready` | Host-owned preview cache evidence for selected material/glTF/audio/texture thumbnail rows, with backend scope, frame/hash/counter evidence, and native handles hidden. | Renderer/RHI execution from `editor/core`, selected-material rows without host-owned preview evidence, or D3D12 evidence inferred to Vulkan/Metal. |
| `editor_asset_browser_legal_clean_room_ready` | Retained source summaries, notices, license/provenance rows for shipped third-party material, and explicit external-engine rejection rows. | Reading Unity/Unreal/Godot public docs, using search-result snippets, or "inspired by" claims. |

## File Structure

| File | Responsibility |
| --- | --- |
| `editor/core/include/mirakana/editor/asset_browser_production.hpp` | Public value-only Source Pulse model, query, command, legal/provenance, and readiness row types. |
| `editor/core/src/asset_browser_production.cpp` | Deterministic model/query/command/readiness implementation. |
| `editor/core/include/mirakana/editor/content_browser.hpp` | Keep only low-level browser row storage and selection; add generation counters only if the production model needs stale-command rejection. |
| `editor/core/src/content_browser.cpp` | Deterministic refresh/query support shared by production model. |
| `editor/core/include/mirakana/editor/content_browser_import_panel.hpp` | Either re-export production panel compatibility during the same clean-break slice or retire it after `asset_browser_production.hpp` owns the visible Assets panel contract. |
| `editor/core/src/content_browser_import_panel.cpp` | Retire duplicated UI assembly once production model emits the retained UI document. |
| `editor/src/native_editor_app.hpp` | Expose `asset_browser()` production model and reviewed asset-browser command plans. Remove `asset_rows()`. |
| `editor/src/native_editor_app.cpp` | Build Source Pulse model from project/source registry/import/material preview state; route reviewed shell execution evidence into the model. |
| `editor/src/first_party_editor_document.cpp` | Render the Assets panel from the production retained UI document; remove `make_asset_list_ui_model` use. |
| `editor/core/include/mirakana/editor/ui_model.hpp` | Remove `EditorAssetListRow` only after shell tests prove the production replacement. |
| `editor/core/src/ui_model.cpp` | Remove `make_asset_list_ui_model` only after replacement. |
| `tests/unit/editor_core_tests.cpp` | Core production model, query, command, legal, provenance, retained UI, and fail-closed tests. |
| `tests/unit/editor_native_shell_tests.cpp` | Visible shell replacement, dialog routing, accessibility, smoke counter, and no hard-coded-row tests. |
| `docs/editor.md` | Current product docs after each completed phase. |
| `docs/current-capabilities.md` | Capability rows and explicit non-claims after implementation evidence exists. |
| `docs/roadmap.md` | Roadmap summary and follow-up blockers. |
| `docs/superpowers/plans/README.md` | Registry entry for this candidate plan and Task 11 closeout evidence. |
| `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` | Update only if this plan becomes `currentActivePlan` or changes machine-readable production claims. |
| `tools/check-ai-integration.ps1` | Update only when new retained ids become agent-contract needles. |

## Task 1: Core Source Pulse Model

**Files:**
- Create: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Create: `editor/core/src/asset_browser_production.cpp`
- Modify: `editor/core/CMakeLists.txt`
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Write failing core model tests**

Add a test section named `editor_asset_browser_production_model_builds_source_pulse_rows` to `tests/unit/editor_core_tests.cpp`. The test must build a `SourceAssetRegistryDocumentV1` with texture, material, mesh, audio, scene, shader, and UI atlas rows; refresh `ContentBrowserState`; create one import plan; and assert:

```cpp
const auto model = mirakana::editor::make_editor_asset_browser_production_model(
    mirakana::editor::EditorAssetBrowserProductionDesc{
        .browser = browser,
        .import_plan = import_plan,
        .project_root = ".",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
    });
MK_REQUIRE(model.status_label == "Asset browser ready");
MK_REQUIRE(model.source_registry_path == "source/assets/package.geassets");
MK_REQUIRE(model.rows.size() == 7U);
MK_REQUIRE(model.rows.front().asset_key_label.starts_with("asset."));
MK_REQUIRE(model.mutates == false);
MK_REQUIRE(model.executes == false);
MK_REQUIRE(model.exposes_native_handles == false);
```

- [ ] **Step 2: Verify the test fails**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`

Expected: compile failure because `asset_browser_production.hpp` and `make_editor_asset_browser_production_model` do not exist.

- [ ] **Step 3: Add the public value model**

Create `editor/core/include/mirakana/editor/asset_browser_production.hpp` with these exact first-pass public types:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetBrowserProductionStatus : std::uint8_t { empty, ready, attention };

struct EditorAssetBrowserSourcePulseRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string row_id;
    std::string kind_label;
    std::string asset_key_label;
    std::string source_path;
    std::string imported_path;
    std::string display_name;
    std::string scope_label;
    std::string state_label;
    std::string import_status_label;
    std::string package_status_label;
    std::string provenance_status_label;
    std::string preview_status_label;
    std::string hot_reload_status_label;
    bool selected{false};
    bool identity_backed{false};
    bool source_visible{false};
    bool package_visible{false};
    bool blocked{false};
    bool host_gated{false};
};

struct EditorAssetBrowserProductionDesc {
    const ContentBrowserState* browser{nullptr};
    const AssetImportPlan* import_plan{nullptr};
    std::string project_root{"."};
    std::string asset_root{"assets"};
    std::string source_registry_path;
};

struct EditorAssetBrowserProductionModel {
    EditorAssetBrowserProductionStatus status{EditorAssetBrowserProductionStatus::empty};
    std::string status_label{"Asset browser empty"};
    std::string project_root;
    std::string asset_root;
    std::string source_registry_path;
    std::size_t total_row_count{0};
    std::size_t visible_row_count{0};
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
};

[[nodiscard]] std::string_view
editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept;
[[nodiscard]] EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model);

} // namespace mirakana::editor
```

- [ ] **Step 4: Implement deterministic row building**

Create `editor/core/src/asset_browser_production.cpp`. Use `std::vector`, `std::ranges::sort`, explicit strings, no OS APIs, no renderer/RHI types, no native handles, and no exceptions for ordinary missing optional input. Row ids must be `asset_browser.source_pulse.<AssetId value>`. Sort rows by `imported_path`, then `asset_key_label`, then `row_id`.

- [ ] **Step 5: Register the files in CMake**

Modify `editor/core/CMakeLists.txt` to add the new header/source to `MK_editor_core` using the existing local source listing style. Do not create a new target.

- [ ] **Step 6: Run the focused test**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`

Expected: build succeeds.

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`

Expected: the new core model test passes.

## Task 2: Query Grammar And Stale-Command Safety

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Add failing query tests**

Add tests for `kind=texture`, `scope=source`, `state=ready`, `key=asset.environment`, `path=clouds`, plain text, invalid key, and a Unity/Unreal/Godot syntax rejection case. The invalid external-engine syntax test must assert diagnostics for `t:texture`, `collection:`, and `res://` rather than silently accepting those as first-party query operators.

- [ ] **Step 2: Add query types**

Extend the header with:

```cpp
enum class EditorAssetBrowserQueryStatus : std::uint8_t { empty, ready, blocked };

struct EditorAssetBrowserQueryTokenRow {
    std::string id;
    std::string key;
    std::string value;
    std::string status_label;
    bool active{false};
    bool blocked{false};
};

struct EditorAssetBrowserQueryDesc {
    std::string query_text;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
};

struct EditorAssetBrowserQueryResult {
    EditorAssetBrowserQueryStatus status{EditorAssetBrowserQueryStatus::empty};
    std::string status_label{"Asset browser query empty"};
    std::string normalized_query;
    std::vector<EditorAssetBrowserQueryTokenRow> tokens;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] EditorAssetBrowserQueryResult
plan_editor_asset_browser_query(const EditorAssetBrowserQueryDesc& desc);
```

- [ ] **Step 3: Implement first-party query parsing**

Support only these operators: `kind=`, `scope=`, `state=`, `key=`, and `path=`. Treat all other `name=value` operators as blocked diagnostics. Treat plain tokens as ASCII case-insensitive substring matches over display name, source path, imported path, and asset key. Do not implement fuzzy matching, tag collections, saved searches, Unity labels, Unreal collections, or Godot `res://` path semantics in this plan.

- [ ] **Step 4: Add generation fields for command safety**

Add `std::uint64_t generation{1};` to `EditorAssetBrowserProductionModel`. Command requests introduced in Task 3 must include `expected_generation`; stale generations must produce `rejected_stale_generation`.

- [ ] **Step 5: Run focused validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`

Expected: query tests pass and invalid external-engine syntax stays blocked.

## Task 3: Reviewed Asset Browser Commands

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Add failing command-plan tests**

Add tests for:

- `asset_browser.source_registry.reload`
- `asset_browser.import.review_sources`
- `asset_browser.import.copy_external_sources`
- `asset_browser.import.execute_reviewed_plan`
- `asset_browser.cook.package_preview`
- `asset_browser.hot_reload.stage_recook`
- `asset_browser.selection.inspect`
- `asset_browser.package.apply_registration`

Each test must assert dry-run rows, `expected_generation`, `requires_user_confirmation` for mutating/executing shell-owned work, stale-generation rejection, no editor-core execution, no package scripts, no validation recipes, and no native handles.

- [ ] **Step 2: Add command public types**

Add:

```cpp
enum class EditorAssetBrowserCommandKind : std::uint8_t {
    reload_source_registry,
    review_import_sources,
    copy_external_sources,
    execute_reviewed_import_plan,
    preview_cooked_package,
    stage_hot_reload_recook,
    inspect_selection,
    apply_package_registration
};

enum class EditorAssetBrowserCommandMode : std::uint8_t { dry_run, apply };
enum class EditorAssetBrowserCommandStatus : std::uint8_t { ready, blocked, rejected_stale_generation };

struct EditorAssetBrowserCommandRequest {
    EditorAssetBrowserCommandKind kind{EditorAssetBrowserCommandKind::reload_source_registry};
    EditorAssetBrowserCommandMode mode{EditorAssetBrowserCommandMode::dry_run};
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool user_confirmed{false};
};

struct EditorAssetBrowserCommandPlan {
    std::string command_id;
    std::string label;
    EditorAssetBrowserCommandStatus status{EditorAssetBrowserCommandStatus::blocked};
    std::string status_label;
    bool requires_user_confirmation{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> report_rows;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view editor_asset_browser_command_id(EditorAssetBrowserCommandKind kind) noexcept;
[[nodiscard]] EditorAssetBrowserCommandPlan
plan_editor_asset_browser_command(const EditorAssetBrowserCommandRequest& request);
```

- [ ] **Step 3: Implement command planning**

`editor/core` must only plan. The only plans allowed to set `mutates_project_files=true` or `executes_import_tools=true` are apply-mode plans with matching generation and `user_confirmed=true`; they still do not execute. `executes_package_scripts`, `executes_validation_recipes`, and `exposes_native_handles` must always remain `false`.

- [ ] **Step 4: Run focused validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`

Expected: command-plan tests pass.

## Task 4: Legal, Provenance, And Clean-Room Rows

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Modify: `docs/legal-and-licensing.md` only if new third-party material is actually added
- Modify: `THIRD_PARTY_NOTICES.md` only if new third-party material is actually shipped
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Add failing legal/provenance tests**

Add tests proving:

- Rows with third-party source paths require `license_id`, `source_url`, `retrieved_date`, `copyright_holder`, and `distribution_target`.
- License-less material is blocked.
- `CC-NC` and `CC-ND` assets are blocked for production package use.
- Unity Asset Store, Unreal Marketplace/Fab, engine sample content, engine logos/trademarks, and copied editor screenshots/icons/layout descriptions are rejected as `external_engine_material_rejected`.
- Godot MIT material may be accepted only when notice rows are complete and the asset is not copied editor UI expression or trademark usage.

- [ ] **Step 2: Add legal row types**

Add:

```cpp
struct EditorAssetBrowserLegalProvenanceRow {
    std::string id;
    std::string asset_key_label;
    std::string source_url;
    std::string retrieved_date;
    std::string version_or_commit;
    std::string copyright_holder;
    std::string license_id;
    std::string modification_status;
    std::string distribution_target;
    std::string status_label;
    bool notice_complete{false};
    bool external_engine_material{false};
    bool accepted_for_package{false};
    bool blocked{false};
};
```

- [ ] **Step 3: Implement fail-closed provenance review**

No asset with incomplete provenance can become `package_visible=true` in the production model. Public-doc category references from Unity, Unreal Engine, and Godot must be represented only as `reference_only` rows and must never become asset rows.

- [ ] **Step 4: Run legal validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`

Expected: passes if no dependency metadata changed; otherwise reports the exact missing notice/dependency records to fix before continuing.

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`

Expected: legal/provenance tests pass.

## Task 5: Native Shell Replacement For The Assets Panel

**Files:**
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/core/include/mirakana/editor/ui_model.hpp`
- Modify: `editor/core/src/ui_model.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Add failing native shell tests**

Add tests that assert:

- `NativeEditorApp::asset_browser()` exists and returns a production model.
- `NativeEditorApp::asset_rows()` no longer exists after this task.
- `make_first_party_editor_document(app)` contains `asset_browser.source_pulse` retained rows, not legacy `assets.row.*` hard-coded rows.
- Smoke counters include `editor_asset_browser_visible=1`, `editor_asset_browser_source_pulse_rows>0`, `editor_asset_browser_hardcoded_rows=0`, and `editor_asset_browser_native_handles_exposed=0`.

- [ ] **Step 2: Replace the app surface**

Remove `asset_rows()` and `EditorAssetListRow` usage from `NativeEditorApp`. Add:

```cpp
[[nodiscard]] const EditorAssetBrowserProductionModel& asset_browser() const noexcept;
[[nodiscard]] std::span<const EditorAssetBrowserCommandPlan> asset_browser_command_plans() const noexcept;
```

- [ ] **Step 3: Build the model in `NativeEditorApp`**

Use existing default project/source registry/import/material preview state. Do not add hard-coded visual rows. Hard-coded test fixture source data is acceptable only as in-memory sample project data used to exercise the model until real project load is wired.

- [ ] **Step 4: Render retained UI**

In `first_party_editor_document.cpp`, render the Assets panel by cloning `make_editor_asset_browser_production_ui_model(app.asset_browser())` into the shell document under the existing `assets` panel root. Keep `mirakana::ui` ids stable and never expose Win32/D3D12 handles.

- [ ] **Step 5: Delete legacy row model**

Remove `EditorAssetListRow` and `make_asset_list_ui_model` only after tests in Step 1 are updated to the production model. If another panel still depends on them, migrate that panel in the same task rather than adding compatibility aliases.

- [ ] **Step 6: Run shell validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1`

Expected: editor lane builds and native shell tests pass.

## Task 6: Dialog, Path, Import, And External Copy Execution Handoff

**Files:**
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/win32_first_party_editor_host.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `docs/editor.md`

- [ ] **Step 1: Add failing handoff tests**

Tests must prove:

- Browse Import Sources uses the native shell file-dialog service.
- Accepted selections are canonicalized by the shell, converted to safe project-relative paths, and rejected when outside the project root.
- External copy writes only under `ProjectDocument::asset_root/imported_sources/<filename>`.
- Existing targets, empty source paths, line separators, device paths, path traversal, and unsupported extensions are blocked.
- Import execution requires `asset_browser.import.execute_reviewed_plan`, matching generation, and user confirmation.

- [ ] **Step 2: Implement shell-owned path review**

Use the existing `IFileDialogService` boundary. On Windows host code, use the Common Item Dialog / `IFileDialog` adapter already present in the project service layer; do not move native path APIs into `editor/core`. The shell path review must canonicalize and compare paths against the project root before converting to store-relative paths.

- [ ] **Step 3: Implement explicit import execution handoff**

The shell may pass reviewed options into existing `execute_asset_import_plan` / `ExternalAssetImportAdapters::options()` only after command review and user confirmation. The shell must record result rows back into the production model; `editor/core` remains non-executing.

- [ ] **Step 4: Run validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1`

Expected: dialog/path/import handoff tests pass.

If optional codec adapters are touched, also run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`

Expected: optional importer lane passes or reports an explicit host/dependency blocker.

## Task 7: Preview, Inspect, Thumbnail, And Hot-Reload Evidence

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/native_material_preview_cache.hpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

- [ ] **Step 1: Add failing preview evidence tests**

Add tests for material, texture thumbnail request, glTF/glB inspect, audio summary, scene reference diagnostics, and hot-reload staged recook rows. Each test must prove preview evidence is host-owned and that `editor/core` does not create renderer/RHI resources, decode arbitrary files for display, execute shader compilers, stream packages, expose native handles, or mutate manifests.

- [ ] **Step 2: Add preview evidence row types**

Add:

```cpp
struct EditorAssetBrowserPreviewEvidenceRow {
    std::string id;
    std::string asset_key_label;
    std::string preview_kind;
    std::string backend_label;
    std::string display_path_label;
    std::string status_label;
    std::string diagnostic;
    std::uint64_t frame_or_sample_count{0};
    bool host_owned{true};
    bool ready{false};
    bool exposes_native_handles{false};
};
```

- [ ] **Step 3: Wire existing preview sources**

Reuse `EditorMaterialAssetPreviewPanelModel`, `make_editor_asset_thumbnail_requests`, `inspect_gltf_mesh_primitives` review rows, and `AssetHotReloadRecookScheduler` summaries as inputs. Do not introduce a new image library, audio decoder, or GPU upload path in this task.

- [ ] **Step 4: Run validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1`

Expected: preview and shell evidence tests pass.

## Task 8: Accessibility, Keyboard, And AI-Operable Retained Rows

**Files:**
- Modify: `editor/core/src/asset_browser_production.cpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/src/native_editor_uia_provider.*`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `tools/check-ai-integration.ps1` only if retained ids become contract needles

- [ ] **Step 1: Add failing retained UI and accessibility tests**

Tests must prove retained rows for:

- `asset_browser.status`
- `asset_browser.query`
- `asset_browser.source_registry.path`
- `asset_browser.source_pulse.<id>.asset_key`
- `asset_browser.source_pulse.<id>.source_path`
- `asset_browser.source_pulse.<id>.state`
- `asset_browser.source_pulse.<id>.package_status`
- `asset_browser.preview.<id>.status`
- `asset_browser.legal.<id>.status`
- command rows under `asset_browser.commands.<command_id>`

Native shell tests must prove UIA publication for the asset tree/list items, selection rows, command buttons, and status rows with no hidden/unsupported-pattern counters.

- [ ] **Step 2: Implement retained UI model**

`make_editor_asset_browser_production_ui_model` must produce a deterministic `mirakana::ui::UiDocument` with semantic roles: panel, text input/search field, tree/list, list item, label, button, and status. Use existing `mirakana::ui` roles; add new roles only if existing roles cannot represent the control.

- [ ] **Step 3: Publish accessibility rows**

Map the visible retained rows through the existing private Windows UIA provider. Use UI Automation tree/control-pattern guidance as the Windows host contract, but keep cross-platform accessibility unclaimed.

- [ ] **Step 4: Run validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1`

Expected: retained UI and UIA tests pass.

If new retained ids become static needles, run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

Expected: agent-contract needles pass.

## Task 9: Package Candidate And Runtime Registration Review

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp` only if existing package candidate rows need one narrow hook
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `docs/editor.md`

- [ ] **Step 1: Add failing package review tests**

Tests must prove:

- Package candidates are derived from source registry/import/cook evidence and caller-supplied runtime package files.
- Rows classify `add`, `already_registered`, `source_only`, `unsafe_path`, `duplicate`, `missing_cooked_artifact`, and `blocked_license`.
- `asset_browser.package.apply_registration` plans only a narrow `game.agent.json.runtimePackageFiles` edit through `ITextStore`.
- The model does not execute package scripts, run validation recipes, stream packages, or load runtime game modules.

- [ ] **Step 2: Implement package review rows**

Add package rows to the production model rather than inventing a parallel package panel. Reuse existing scene/package candidate draft patterns where possible, but keep asset-browser-specific row ids.

- [ ] **Step 3: Run focused validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`

Expected: package review tests pass.

## Task 10: Documentation, Manifest, And Static Contract Synchronization

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only if this plan becomes active or ready claims change
- Modify: `engine/agent/manifest.json` only by running compose if fragments changed
- Modify: `tools/check-ai-integration.ps1` only if durable agent-contract needles change

- [ ] **Step 1: Update docs with exact evidence**

When Tasks 1-9 have passing evidence, update docs to say exactly what is ready and what remains unsupported. Use these non-claims unless a named follow-up plan proves them:

- arbitrary importer adapters
- automatic import execution
- broad runtime package streaming
- renderer/RHI execution from editor core
- public native handles
- Vulkan/Metal preview display parity
- cross-platform accessibility parity
- external-engine compatibility/equivalence/parity
- Unity/Unreal/Godot asset/project/schema import compatibility

- [ ] **Step 2: Update manifest only if required**

If `currentActivePlan` is intentionally changed to this plan or production claim rows change, edit the relevant fragment under `engine/agent/manifest.fragments/`, then run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`

Expected: composed `engine/agent/manifest.json` changes only through the compose script.

- [ ] **Step 3: Run docs/agent checks**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`

Expected: all pass, or report exact stale contract rows to fix.

## Task 11: Final Validation And Closeout

**Files:**
- Modify: this plan with validation evidence
- Modify: docs from Task 10 if validation changes capability wording

- [ ] **Step 1: Run focused editor checks**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1`

Expected: editor lane passes.

- [ ] **Step 2: Run full repository validation**

Run:

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

Expected: full validation passes. If a host/toolchain blocker occurs, record the exact command, blocker, and unaffected focused checks in this plan before stopping.

- [ ] **Step 3: Record final evidence**

Add a closeout evidence subsection to this plan with exact test commands, pass/fail status, hosted PR evidence if available, retained smoke counters, legal/dependency check status, and explicit non-claims.

## Acceptance Checklist

- [ ] The visible `MK_editor` Assets panel no longer uses hard-coded `EditorAssetListRow` rows.
- [ ] `MK_editor_core` owns all persistent production asset browser state as value models.
- [ ] Native dialogs, path canonicalization, external copy, import execution, preview execution, and accessibility publication stay shell-owned.
- [ ] Every command has a dry-run/apply review path, stale-generation rejection, and user-confirmation requirements where mutation/execution is possible.
- [ ] Every source/cooked/package row is key-first through `AssetKeyV2`.
- [ ] Legal/provenance rows block license-less, NC/ND, external-engine sample, trademark, marketplace, and copied UI-expression material.
- [ ] Official Unity/Unreal/Godot docs are retained only as category/legal research inputs.
- [ ] Context7 C++/OpenEXR/KTX evidence is used only to shape validation and import/cook metadata, not copied implementation.
- [ ] Focused editor validation and full `tools/validate.ps1` have run or exact host blockers are recorded.
