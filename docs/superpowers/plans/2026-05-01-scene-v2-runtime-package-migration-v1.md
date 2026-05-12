# Scene v2 Runtime Package Migration v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed AI-safe dry-run/apply bridge from authored `GameEngine.Scene.v2` documents plus `GameEngine.SourceAssetRegistry.v1` source asset rows into the existing runtime-loadable `GameEngine.Scene.v1` scene package update surface.

**Architecture:** Keep `mirakana_scene` as the owner of authored scene validation and renderer-neutral scene values, keep `mirakana_assets` as the source asset identity/metadata owner, and put the command planner/apply helper in `mirakana_tools`. Reuse the existing reviewed `plan_scene_package_update` / `apply_scene_package_update` package row surface instead of adding broad package cooking, importer execution, renderer/RHI residency, package streaming, or native-handle exposure.

**Tech Stack:** C++23 `mirakana_scene`/`mirakana_assets`/`mirakana_tools`, focused C++ tests, `engine/agent/manifest.json`, schema/static checks, docs, and existing validation recipes.

---

## Goal

Make the authored Scene v2 data spine usable by the current cooked runtime scene lane through a narrow reviewed command:

- dry-run a Scene v2 runtime package migration and report deterministic changed files, model mutations, diagnostics, validation recipes, unsupported gap ids, and placeholder undo metadata
- apply only validated writes through existing filesystem abstractions after rereading source documents from safe repository-relative paths
- resolve mesh/material/sprite references through `GameEngine.SourceAssetRegistry.v1` and stable `AssetKeyV2` projection before producing `GameEngine.Scene.v1` package update input
- keep dependent texture/mesh/audio/material cooking, `.geindex` package assembly beyond the existing scene row update, renderer/RHI residency, package streaming, material/shader graphs, live shader generation, editor productization, Metal readiness, and public native/RHI handles outside the ready claim

## Context

- `scene-prefab-authoring-command-tooling-v1` made authored `GameEngine.Scene.v2` and `GameEngine.Prefab.v2` mutation AI-operable.
- `source-asset-registration-command-tooling-v1` made `GameEngine.SourceAssetRegistry.v1` source asset identity and deterministic import intent AI-operable.
- `scene-package-apply-tooling-v1` already owns the reviewed first-party `GameEngine.Scene.v1` `.scene` plus `.geindex` scene dependency row update surface.
- The missing bridge is a reviewed migration planner that converts Scene v2 component property rows into runtime `mirakana::Scene` values using only registered source asset keys.

## Constraints

- Do not add third-party dependencies.
- Do not execute importers, shell commands, raw command strings, arbitrary scripts, or free-form edits.
- Do not broaden package cooking, package streaming, renderer/RHI residency, material graph, shader graph, live shader generation, editor productization, public native/RHI handles, or Metal readiness.
- Do not parse external PNG, glTF, common-audio, shader, or script sources in this command.
- Do not make `mirakana_scene`, `mirakana_assets`, `mirakana_ui`, or gameplay-facing APIs depend on renderer, RHI, platform backend, SDL3, Dear ImGui, OS, GPU, or native handles.
- Keep dependent asset package rows on existing reviewed package surfaces until a focused package-cooking plan promotes more.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A reviewed `migrate-scene-v2-runtime-package` dry-run/apply API exists for the initial operation allowlist.
- Static checks require the reviewed descriptor and reject broad importer/package/renderer/native readiness claims.
- Docs tell agents when to use Scene/Prefab v2 authoring, SourceAssetRegistry v1 registration, Scene v2 runtime package migration, and existing cooked package update tooling.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused C++ tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, selected cheap runner execute test, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## API Sketch

Initial operation allowlist: `migrate_scene_v2_runtime_package` only.

Request fields:

```cpp
struct SceneV2RuntimePackageMigrationRequest {
    SceneV2RuntimePackageMigrationCommandKind kind;
    std::string scene_v2_path;
    std::string scene_v2_content;
    std::string source_registry_path;
    std::string source_registry_content;
    std::string package_index_path;
    std::string package_index_content;
    std::string output_scene_path;
    AssetKeyV2 scene_asset_key;
    std::uint64_t source_revision;
    std::string package_cooking{"unsupported"};
    std::string dependent_asset_cooking{"unsupported"};
    std::string external_importer_execution{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string editor_productization{"unsupported"};
    std::string metal_readiness{"unsupported"};
    std::string public_native_rhi_handles{"unsupported"};
    std::string general_production_renderer_quality{"unsupported"};
    std::string arbitrary_shell{"unsupported"};
    std::string free_form_edit{"unsupported"};
};
```

Result fields:

```cpp
struct SceneV2RuntimePackageMigrationResult {
    std::string scene_v1_content;
    std::string package_index_content;
    std::vector<SceneV2RuntimePackageMigrationChangedFile> changed_files;
    std::vector<SceneV2RuntimePackageMigrationModelMutation> model_mutations;
    std::vector<SceneV2RuntimePackageMigrationDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};
};
```

Property mapping for the first slice:

- `camera`: `projection`, `primary`, `vertical_fov_radians`, `orthographic_height`, `near_plane`, `far_plane`
- `light`: `type`, `color`, `intensity`, `range`, `casts_shadows`
- `mesh_renderer`: `mesh`, `material`, `visible`
- `sprite_renderer`: `sprite`, `material`, `size`, `tint`, `visible`

Asset reference rules:

- `mesh_renderer.mesh`, `mesh_renderer.material`, `sprite_renderer.sprite`, and `sprite_renderer.material` property values are `AssetKeyV2` strings.
- Keys must exist in the loaded `GameEngine.SourceAssetRegistry.v1` document.
- Referenced rows must match the required source asset kind: `mesh`, `material`, or `texture` for sprite texture keys.
- The migration derives runtime `AssetId` values through canonical `AssetKeyV2` projection and records dependency lists for `plan_scene_package_update`.
- Duplicate nodes/components, missing asset keys, wrong-kind asset rows, non-finite numeric values, malformed colors/vectors, unsafe paths, unsupported component types for runtime migration, and unsupported claim sentinels produce deterministic diagnostics.

Task 1 contract decisions:

- Helper names are `mirakana::plan_scene_v2_runtime_package_migration` and `mirakana::apply_scene_v2_runtime_package_migration`, declared in `engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp` and implemented in `engine/tools/src/scene_v2_runtime_package_migration_tool.cpp`.
- C++ command kind is `SceneV2RuntimePackageMigrationCommandKind::migrate_scene_v2_runtime_package`; `free_form_edit` exists only to reject unsupported free-form mutation requests.
- The projection lives in `mirakana_tools` for v1 because it combines `mirakana_scene`, `mirakana_assets`, `mirakana_platform`, and the existing scene package update helper. `mirakana_scene` remains dependency-free and does not learn about source registries or package indexes in this slice.
- Source inputs use safe repository-relative paths: no empty paths, absolute roots, drive colons, backslashes, semicolons, control characters, or `.` / `..` path segments. `scene_v2_path` and `output_scene_path` must end in `.scene`, `source_registry_path` in `.geassets`, and `package_index_path` in `.geindex`.
- Package writes additionally use package-relative rules before calling `plan_scene_package_update`: no empty paths, absolute roots, colons, backslashes, NUL/LF/CR, or `.` / `..` segments.
- `output_scene_path` must not alias `scene_v2_path`, `source_registry_path`, or `package_index_path`; the migration must never overwrite authored Scene v2 input content with runtime Scene v1 package content.
- `scene_asset_key` must use the same safe `AssetKeyV2` segment policy as source registry keys even though the scene key does not need a source registry row.
- Runtime-supported Scene v2 component types are exactly `camera`, `light`, `mesh_renderer`, and `sprite_renderer`; other Scene v2 component types stay authoring-only for this migration.
- Scene v2 component vector properties use comma-separated values: `color=r,g,b`, `size=x,y`, and `tint=r,g,b,a`. Scalars must parse as finite floats and booleans must be `true` or `false`.
- `camera.near_plane` / `camera.far_plane` map to the existing Scene v1 serializer fields `camera.near` / `camera.far`.
- Asset reference properties are key strings resolved through `GameEngine.SourceAssetRegistry.v1`; `mesh_renderer.mesh` requires `AssetKind::mesh`, material properties require `AssetKind::material`, and `sprite_renderer.sprite` requires `AssetKind::texture`. The scene asset id is derived from `scene_asset_key` without requiring a Scene v1 source registry row, avoiding the current `GameEngine.Scene.v1` source-format constraint for scene rows.
- Deterministic ordering is document node order for runtime `Scene` node ids, sorted dependency/model mutation rows by dependency kind name then key, and downstream package index ordering from `plan_scene_package_update`.
- Diagnostic codes are snake_case and include `unsafe_path`, `unsupported_operation`, `unsupported_free_form_edit`, `invalid_scene_v2`, `invalid_source_registry`, `missing_source_asset`, `wrong_source_asset_kind`, `unsupported_component_type`, `unsupported_component_property`, `invalid_component_value`, `duplicate_component_id`, `duplicate_runtime_component`, and `unsupported_<capability>` claim codes.
- Model mutation row kind is `migrate_scene_v2_runtime_package`, with target output path, scene key/id, source scene path, package index path, and resolved scene dependency rows.
- Apply reuses `apply_scene_package_update` for the final `.scene` and `.geindex` writes after rereading Scene v2 and source registry inputs. It inherits the existing scene package helper write behavior instead of adding a new transaction protocol to `IFileSystem`.
- Explicit non-goals remain broad package cooking, dependent asset cooking, importer execution, renderer/RHI residency, package streaming, material/shader graphs, live shader generation, editor productization, public native/RHI handles, Metal readiness, and general production renderer quality.

## Implementation Tasks

### Task 1: Inventory And Contract

**Files:**
- Read: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Read: `engine/scene/include/mirakana/scene/scene.hpp`
- Read: `engine/scene/include/mirakana/scene/components.hpp`
- Read: `engine/assets/include/mirakana/assets/source_asset_registry.hpp`
- Read: `engine/tools/include/mirakana/tools/scene_tool.hpp`
- Read: `engine/tools/include/mirakana/tools/source_asset_registration_tool.hpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/ai-game-development.md`
- Read: `docs/testing.md`

- [x] Decide exact helper names, request/result field names, diagnostic codes, and model mutation row names.
- [x] Confirm whether projection logic belongs entirely in `mirakana_tools` or whether `mirakana_scene` needs a dependency-free `SceneDocumentV2` to `Scene` projection helper that takes resolved `AssetId` values.
- [x] Define safe path rules for `scene_v2_path`, `source_registry_path`, `package_index_path`, and `output_scene_path`.
- [x] Define runtime-supported component property names, scalar/vector parsing rules, and deterministic sort order.
- [x] Record explicit non-goals before RED tests are added.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing focused tests for dry-run migration from Scene v2 plus SourceAssetRegistry v1 into Scene v1 content and package index changes.
- [x] Add failing focused tests for apply rereading validated paths and writing only deterministic changed files.
- [x] Add failing focused tests for missing source registry rows, wrong asset kinds, duplicate component ids, malformed numeric values, unsupported component property names, unsafe paths, and unsupported claim sentinels.
- [x] Add failing static checks requiring a reviewed `migrate-scene-v2-runtime-package` descriptor.
- [x] Add failing static checks rejecting external importer execution, broad package cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, editor productization, Metal readiness, and public native/RHI handle claims.
- [x] Record RED evidence before production implementation.

### Task 3: Production Implementation

**Files:**
- Create or modify: `engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp`
- Create or modify: `engine/tools/src/scene_v2_runtime_package_migration_tool.cpp`
- Modify: `engine/tools/CMakeLists.txt`
- Modify: `tests/unit/tools_tests.cpp`

- [x] Implement typed request/result value APIs with deterministic diagnostics.
- [x] Implement dry-run planning without filesystem mutation.
- [x] Implement apply through validated file reads and writes only.
- [x] Reuse `validate_scene_document_v2`, source registry document validation through its parser, stable `AssetKeyV2` projection, and existing `plan_scene_package_update` / `apply_scene_package_update` behavior.
- [x] Keep dependency cooking, package assembly beyond existing scene package row updates, renderer/RHI residency, and package streaming outside this helper.

### Task 4: Manifest, Docs, And Agent Context Sync

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the reviewed Scene v2 runtime package migration subset to ready.
- [x] Keep broad package cooking, dependent asset cooking, renderer/RHI residency, package streaming, editor productization, material/shader graphs, live shader generation, public native/RHI handles, Metal readiness, and general production renderer quality planned or host-gated.
- [x] Update generated-game guidance so agents use four distinct steps: register source assets, author Scene/Prefab v2, migrate a Scene v2 runtime package, then run reviewed validation recipes.
- [x] Keep `currentActivePlan` and `recommendedNextPlan` synchronized with the next focused plan and the plan registry after this slice completes.

### Task 5: Validation

**Files:**
- Modify: this plan's Validation Evidence section
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`

- [x] Run focused Scene v2 runtime package migration tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run a cheap validation runner execute test, for example `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks because `engine/agent/manifest.json aiOperableProductionLoop` did not yet expose one ready `migrate-scene-v2-runtime-package` command surface.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected for the same missing ready `migrate-scene-v2-runtime-package` command surface.
- RED: direct `cmake --build --preset dev --target mirakana_tools_tests` could not run from the shell because `cmake` was not on PATH, so focused compile RED was re-run through the repository script.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed in `mirakana_tools_tests` compile as expected because `mirakana/tools/scene_v2_runtime_package_migration_tool.hpp` did not exist yet.
- RED: after code-review hardening tests were added, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed in `mirakana_tools_tests` for output/input path aliasing, invalid `scene_asset_key`, and centralized write-failure reporting gaps.
- RED: after requiring the migration implementation to call both existing scene package helpers, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed until the helper source reused `apply_scene_package_update`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed with 28/28 tests after implementing `mirakana::plan_scene_v2_runtime_package_migration`, `mirakana::apply_scene_v2_runtime_package_migration`, path alias rejection, `AssetKeyV2` validation, and `apply_scene_package_update` reuse.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after manifest/static-check synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/static-check synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `currentActivePlan=docs/superpowers/plans/2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md` after next-plan handoff.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding the public `mirakana/tools/scene_v2_runtime_package_migration_tool.hpp` API.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only: D3D12 DXIL and Vulkan SPIR-V tooling are ready; Metal `metal`/`metallib` remain missing host-gated diagnostics.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed and executed `agent-check` plus `schema-check`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 tests. Existing diagnostic-only gates remain: Metal tools missing, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.
- GREEN: created `docs/superpowers/plans/2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md`, moved the plan registry active slice to it, and updated `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` / `recommendedNextPlan`.
