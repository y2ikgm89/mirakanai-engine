# Asset Identity v2 Scene Runtime Reference Placement Evidence v1 Implementation Plan (2026-05-12)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `asset-identity-v2-scene-runtime-reference-placement-evidence-v1`

**Status:** Completed

**Goal:** Add deterministic Asset Identity v2 placement evidence to the reviewed Scene v2 runtime package migration surface so scene-side mesh/material/sprite references prove which stable `AssetKeyV2` rows produced the runtime `AssetId` values.

**Architecture:** Keep the existing `plan_scene_v2_runtime_package_migration` / `apply_scene_v2_runtime_package_migration` command boundary and reuse `project_source_asset_registry_identity_v2` plus `plan_asset_identity_placements_v2`. The migration continues to emit Scene v1 content and `.geindex` rows through the existing scene package helper, while `SceneV2RuntimePackageMigrationModelMutation` gains read-only `AssetIdentityPlacementRowV2` evidence rows for the scene asset and referenced mesh/material/sprite component placements.

**Tech Stack:** C++23, `MK_tools`, `MK_assets`, `MK_scene`, CMake `dev` preset, `MK_tools_tests`, repository PowerShell validation scripts.

---

## Context

- `asset-identity-v2` still requires scene/render/UI/gameplay reference cleanup and editor asset browser migration before a ready claim.
- `plan_asset_identity_placements_v2` already maps reviewed `AssetKeyV2` rows into deterministic `AssetId`/kind/source placement rows.
- Scene v2 runtime package migration currently resolves component properties directly to `AssetId` and emits dependency rows, but the model mutation does not preserve placement evidence tying those ids to stable keys and source paths.
- This slice is scene-side evidence only. It does not complete the full `scene/render/UI/gameplay reference cleanup` claim.

## Constraints

- Do not add renderer, RHI, runtime host, editor, importer execution, package cooking, package streaming, or material/shader graph behavior.
- Do not add compatibility aliases or new command kinds.
- Do not hand-edit `engine/agent/manifest.json`; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Add or update tests before production behavior and verify the RED failure.

## Done When

- `SceneV2RuntimePackageMigrationModelMutation` exposes deterministic `placement_rows`.
- `plan_scene_v2_runtime_package_migration` and `apply_scene_v2_runtime_package_migration` populate placement rows for:
  - `scene.runtime_package`
  - `scene.component.mesh_renderer.mesh`
  - `scene.component.mesh_renderer.material`
  - `scene.component.sprite_renderer.sprite`
  - `scene.component.sprite_renderer.material`
- Distinct mesh-renderer and sprite-renderer material placements remain separate even when they resolve to the same material key.
- Focused `MK_tools_tests`, tidy/public API checks, manifest/static checks, and full `tools/validate.ps1` evidence are recorded below.
- Docs/manifest/static checks state that scene-side placement evidence is implemented while broader scene/render/UI/gameplay cleanup remains open.

## File Map

- Modify `engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp`: add `std::vector<AssetIdentityPlacementRowV2> placement_rows` to `SceneV2RuntimePackageMigrationModelMutation`.
- Modify `engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp`: collect placement requests while resolving scene component asset keys, plan rows through `plan_asset_identity_placements_v2`, and pass rows into the model mutation.
- Modify `tests/unit/tools_tests.cpp`: extend the Scene v2 migration dry-run test with deterministic placement evidence assertions.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`: record completed scene-side placement evidence without closing the full gap.
- Modify `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1`: enforce the new manifest wording.
- Modify docs/plan index/current capability guidance to record this slice and remaining claims.

## Tasks

### Task 1: RED Test

- [ ] Add assertions to `tests/unit/tools_tests.cpp` in `scene v2 runtime package migration dry-runs scene and package index changes`:

```cpp
const auto& mutation = result.model_mutations[0];
MK_REQUIRE(mutation.placement_rows.size() == 5);
MK_REQUIRE(mutation.placement_rows[0].placement == "scene.component.mesh_renderer.material");
MK_REQUIRE(mutation.placement_rows[0].key.value == "assets/materials/base");
MK_REQUIRE(mutation.placement_rows[0].id == material);
MK_REQUIRE(mutation.placement_rows[0].kind == mirakana::AssetKind::material);
MK_REQUIRE(mutation.placement_rows[0].source_path == "source/materials/base.material");
MK_REQUIRE(mutation.placement_rows[1].placement == "scene.component.mesh_renderer.mesh");
MK_REQUIRE(mutation.placement_rows[1].key.value == "assets/meshes/cube");
MK_REQUIRE(mutation.placement_rows[2].placement == "scene.component.sprite_renderer.material");
MK_REQUIRE(mutation.placement_rows[2].key.value == "assets/materials/base");
MK_REQUIRE(mutation.placement_rows[3].placement == "scene.component.sprite_renderer.sprite");
MK_REQUIRE(mutation.placement_rows[3].key.value == "assets/textures/hero");
MK_REQUIRE(mutation.placement_rows[4].placement == "scene.runtime_package");
MK_REQUIRE(mutation.placement_rows[4].key.value == request.scene_asset_key.value);
MK_REQUIRE(mutation.placement_rows[3].kind == mirakana::AssetKind::scene);
```

- [ ] Run:

```powershell
cmake --build --preset dev --target MK_tools_tests
```

Expected RED: compile failure because `SceneV2RuntimePackageMigrationModelMutation` has no `placement_rows` member.

### Task 2: GREEN Implementation

- [ ] Add `placement_rows` to `SceneV2RuntimePackageMigrationModelMutation` after `scene_asset` and before `dependency_rows`.
- [ ] Add placement request collection in `engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp`:
  - component mesh reference uses placement `scene.component.mesh_renderer.mesh`;
  - mesh material reference uses placement `scene.component.mesh_renderer.material`;
  - sprite reference uses placement `scene.component.sprite_renderer.sprite`;
  - sprite material uses the same material placement, so duplicate request suppression leaves one material row;
  - scene package row uses placement `scene.runtime_package`.
- [ ] Build an `AssetIdentityDocumentV2` with `project_source_asset_registry_identity_v2(registry)`, append the scene key as a scene row with `source_path = request.scene_v2_path`, then call `plan_asset_identity_placements_v2`.
- [ ] On placement diagnostics, add fail-closed `asset_identity_placement_failed` migration diagnostics and do not emit changed files.
- [ ] Pass `placement_rows` through `PreparedSceneV2RuntimePackageMigration` into `append_model_mutation`.

### Task 3: Focused Verification

- [ ] Run:

```powershell
cmake --build --preset dev --target MK_tools_tests
ctest --preset dev --output-on-failure -R MK_tools_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

### Task 4: Docs, Manifest, Static Checks

- [ ] Update the plan registry, roadmap/current capability docs, and manifest fragment to name `asset-identity-v2-scene-runtime-reference-placement-evidence-v1`.
- [ ] Keep `scene/render/UI/gameplay reference cleanup` and `editor asset browser migration` required-before-ready claims.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

### Task 5: Slice Closeout

- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Update this plan status to `Completed` and record command evidence in `Validation Evidence`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_tools_tests` | RED then PASS | RED failed because `SceneV2RuntimePackageMigrationModelMutation` had no `placement_rows` member; final focused build passed. |
| `ctest --preset dev --output-on-failure -R MK_tools_tests` | PASS | `MK_tools_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp` | PASS | `tidy-check: ok (1 files)` after direct includes and `std::ranges::unique` cleanup. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS | Wrote composed `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 47/47 CTest passed. Metal/Apple host checks remain diagnostic-only/host-gated on Windows. |
