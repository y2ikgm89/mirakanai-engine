# Registered Source Asset Cook Package Command Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed AI-safe dry-run/apply command surface that takes explicit `GameEngine.SourceAssetRegistry.v1` rows and updates deterministic cooked artifacts plus a runtime-loadable `.geindex` package through existing import/package helpers.

**Architecture:** Keep source identity in `mirakana_assets`, keep runtime package loading in `mirakana_runtime`, and put the reviewed command planner/apply helper in `mirakana_tools`. Reuse existing `build_asset_import_plan`, `execute_asset_import_plan`, `assemble_asset_cooked_package`, package index validation/serialization, and first-party source decoders. Do not turn this into broad dependency cooking, renderer/RHI residency, package streaming, editor productization, or runtime source parsing.

**Tech Stack:** C++23 `mirakana_assets`/`mirakana_tools` plus focused tests, `engine/agent/manifest.json`, schema/static checks, docs, and existing validation recipes.

---

## Goal

Make the source asset registration surface practically package-producing for explicit, reviewed rows:

- dry-run a selected source registry cook/package update and report deterministic changed files, model mutations, diagnostics, validation recipes, unsupported gap ids, and placeholder undo metadata
- apply only validated writes through existing filesystem abstractions after rereading source registry and source asset files from safe repository-relative paths
- reuse existing first-party import planning/execution and cooked package assembly behavior instead of inventing a second package writer
- keep broad dependency traversal, package streaming, renderer/RHI residency, material/shader graphs, live shader generation, editor productization, Metal readiness, public native/RHI handles, and general production renderer quality outside the ready claim

## Context

- `source-asset-registration-command-tooling-v1` records source identity/import intent but intentionally does not execute importers or mutate `.geindex`.
- `scene-v2-runtime-package-migration-v1` can bridge authored Scene v2 rows into the existing Scene v1 package update surface once mesh/material/texture package rows already exist.
- `mirakana_assets` and `mirakana_tools` already contain first-party import planning/execution, deterministic cooked artifact writes, and package assembly helpers that need a reviewed AI command wrapper before they can be treated as an apply surface.

## Constraints

- Do not add third-party dependencies.
- Do not evaluate arbitrary shell, raw command strings, scripts, or free-form edits.
- Do not parse source assets at game runtime; this command is tools/editor-side only.
- Keep optional external PNG/glTF/audio importer adapters behind existing feature/host gates and do not make them required for the first ready subset.
- Do not claim broad package cooking, recursive dependency cooking, package streaming, renderer/RHI residency, native GPU upload, material graph, shader graph, live shader generation, editor productization, public native/RHI handles, Metal readiness, or general renderer quality.
- Keep gameplay-facing APIs on cooked package/runtime contracts only.

## Done When

- A focused RED -> GREEN record exists in this plan.
- A reviewed command surface, tentatively `cook-registered-source-assets`, exists for an initial explicit-row allowlist.
- Tests cover dry-run, apply reread/write determinism, missing source rows, unsafe paths, unsupported kinds/formats, importer failure diagnostics, package index conflicts, unsupported claim sentinels, and no-write validation failures.
- Static checks require the descriptor and reject broad package/importer/renderer/native/Metal/productization claims.
- Docs distinguish source registration, explicit source cook/package update, Scene v2 migration, and runtime package loading.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, selected validation runner execute test, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Initial API Sketch

Initial operation allowlist: `cook_registered_source_assets`.

Request fields:

```cpp
struct RegisteredSourceAssetCookPackageRequest {
    RegisteredSourceAssetCookPackageCommandKind kind;
    std::string source_registry_path;
    std::string source_registry_content;
    std::string package_index_path;
    std::string package_index_content;
    std::vector<AssetKeyV2> selected_asset_keys;
    std::uint64_t source_revision;
    std::string dependency_cooking{"unsupported"};
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

Result fields should mirror prior reviewed command surfaces: changed files, model mutations, diagnostics, validation recipes, unsupported gap ids, and placeholder undo token.

## Implementation Tasks

### Task 1: Inventory And Contract

**Files:**
- Read: `engine/assets/include/mirakana/assets/source_asset_registry.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
- Read: `engine/tools/include/mirakana/tools/asset_import_tool.hpp`
- Read: `engine/tools/include/mirakana/tools/asset_package_tool.hpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/ai-game-development.md`
- Read: `docs/testing.md`

- [x] Decide exact helper names, request/result field names, diagnostic codes, and model mutation row names.
- [x] Define initial supported row kinds/formats and whether optional external importer adapters are excluded or host-gated.
- [x] Define safe path rules for registry path, source files, cooked output paths, and package index path.
- [x] Define package-index conflict behavior and deterministic changed-file ordering.
- [x] Record explicit non-goals before RED tests are added.

Task 1 inventory decisions:

- Helper/API names: add `RegisteredSourceAssetCookPackageRequest`, `RegisteredSourceAssetCookPackageResult`, `RegisteredSourceAssetCookPackageChangedFile`, `RegisteredSourceAssetCookPackageModelMutation`, `RegisteredSourceAssetCookPackageDiagnostic`, `RegisteredSourceAssetCookPackageSourceFile`, `plan_registered_source_asset_cook_package`, and `apply_registered_source_asset_cook_package`.
- Command id / operation: expose descriptor id `cook-registered-source-assets`; C++ command kind is `cook_registered_source_assets`, plus `free_form_edit` rejection.
- Result fields mirror prior reviewed command surfaces: `package_index_content`, `changed_files`, `model_mutations`, `diagnostics`, `validation_recipes`, `unsupported_gap_ids`, and `undo_token`.
- Model mutation rows are one per selected cooked row, with kind `cook_registered_source_asset`, target cooked path, registry path, package index path, `AssetKeyV2`, `AssetId`, `AssetKind`, source path/format, imported path, and dependency rows.
- Diagnostic codes use existing string style: `unsafe_source_registry_path`, `unsafe_package_index_path`, `unsafe_source_path`, `unsafe_imported_path`, `aliased_output_path`, `duplicate_selected_asset_key`, `missing_source_asset_key`, `missing_source_file`, `unsupported_source_format`, `unselected_dependency`, `invalid_source_registry`, `invalid_package_index`, `asset_import_failed`, `asset_package_assembly_failed`, `package_index_conflict`, `filesystem_read_failed`, `filesystem_write_failed`, `unsupported_*`, and `unsupported_free_form_edit`.
- Initial supported rows are first-party `GameEngine.TextureSource.v1`, `GameEngine.MeshSource.v2`, `GameEngine.AudioSource.v1`, `GameEngine.Material.v1`, and `GameEngine.Scene.v1` rows already accepted by `SourceAssetRegistry.v1`, but dependency rows must be explicitly selected in the same request. Optional PNG/glTF/audio external importer adapters remain excluded/host-gated from this reviewed command.
- Safe path rules: registry path must be safe repository-relative `.geassets`; package index path must be safe package-relative `.geindex`; source paths from rows and dry-run payloads must be safe repository-relative; cooked imported paths must be safe package-relative; cooked output paths must not alias the registry path, package index path, or selected source file paths.
- Dry-run uses inline `RegisteredSourceAssetCookPackageSourceFile` payloads in a scratch filesystem to avoid mutating the caller filesystem. Apply rereads registry, package index, and selected source files from `IFileSystem`; inline registry/package/source content is ignored for apply after path validation.
- Package merge behavior: selected cooked entries replace existing entries for the same asset when kind/path are compatible; existing entries for unselected assets remain; any selected cooked path already owned by a different asset, or any existing same-asset incompatible kind/path collision, is a `package_index_conflict`. Dependency edges owned by selected assets are replaced by the selected assembly result.
- Deterministic changed-file ordering is selected cooked artifact files sorted by cooked output path, followed by the package index file.
- Explicit non-goals: no broad/dependent package cooking, no automatic dependency traversal beyond explicitly selected rows, no optional external importer execution, no runtime source parsing, no renderer/RHI residency, no package streaming, no material/shader graph, no live shader generation, no editor productization, no public native/RHI handles, no Metal readiness, no general production renderer quality, no arbitrary shell, and no free-form edits.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing focused tests for dry-run selected source rows into cooked artifacts plus package index changes.
- [x] Add failing focused tests for apply rereading registry/source/package inputs and writing deterministic changed files.
- [x] Add failing focused tests for missing source rows, unsupported row kinds/formats, unsafe paths, malformed source payloads, package conflicts, unsupported claim sentinels, and free-form edit rejection.
- [x] Add failing static checks requiring a reviewed `cook-registered-source-assets` descriptor.
- [x] Add failing static checks rejecting broad dependency cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, editor productization, Metal readiness, and public native/RHI handle claims.
- [x] Record RED evidence before production implementation.

### Task 3: Production Implementation

**Files:**
- Create or modify: `engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp`
- Create or modify: `engine/tools/src/registered_source_asset_cook_package_tool.cpp`
- Modify: `engine/tools/CMakeLists.txt`
- Modify: `tests/unit/tools_tests.cpp`

- [x] Implement typed request/result value APIs with deterministic diagnostics.
- [x] Implement dry-run planning without filesystem mutation.
- [x] Implement apply through validated file reads and writes only.
- [x] Reuse existing import planning/execution and package assembly helpers.
- [x] Keep broad dependency cooking, renderer/RHI residency, package streaming, and editor productization outside this helper.

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

- [x] Promote only the reviewed explicit source-row cook/package subset to ready.
- [x] Keep broad/dependent package cooking, renderer/RHI residency, package streaming, editor productization, material/shader graphs, live shader generation, public native/RHI handles, Metal readiness, and general production renderer quality planned or host-gated.
- [x] Update generated-game guidance so agents use distinct steps: register source assets, cook selected registered source assets, author/migrate Scene v2 when needed, then run reviewed validation recipes.
- [x] Keep `currentActivePlan` and `recommendedNextPlan` synchronized with this plan and the plan registry.

### Task 5: Validation

**Files:**
- Modify: this plan's Validation Evidence section
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`

- [x] Run focused registered source asset cook/package tests.
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

- 2026-05-01 RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: FAIL as expected before production implementation. MSVC compile fails in `tests/unit/tools_tests.cpp` because `mirakana/tools/registered_source_asset_cook_package_tool.hpp` does not exist yet.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: FAIL as expected before manifest sync. `tools/check-ai-integration.ps1` reports `engine/agent/manifest.json aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: FAIL as expected before manifest sync. `tools/check-json-contracts.ps1` reports `engine manifest aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface`.
- 2026-05-01 GREEN:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, 28/28 tests. Initial implementation failure in `mirakana_tools_tests` was traced to validated source registry parsing hiding `unsupported_source_format`; fixed by adding `parse_source_asset_registry_document_unvalidated_v1` and using it in the reviewed cook/package planner before validation diagnostics are mapped.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: PASS, diagnostic-only; Metal tools are still missing on this host.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 28/28 tests; diagnostic-only host gates remain Metal tools, Apple packaging on non-macOS, Android release signing/device smoke configuration, and tidy compile database availability before configure.
