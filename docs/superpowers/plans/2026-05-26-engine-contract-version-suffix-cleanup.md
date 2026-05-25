# Engine Contract Version Suffix Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended when linked worktrees are available) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove removable pre-release `vN` suffixes from current MIRAIKANAI engine-owned saved formats, public C++ contracts, editor ABI names, docs, schemas, manifests, and static checks without breaking the engine.

**Architecture:** Treat the cleanup as a clean-breaking pre-release contract reset. Current repository-owned contracts become canonical unsuffixed names, while historical plan IDs, external standards, numeric version fields, and coordinate names remain untouched. Each candidate updates the owning code, tests, samples, templates, docs, manifest fragments, schemas, and static checks together so no stale mixed contract remains.

**Tech Stack:** C++23, CMake target wrappers under `tools/*.ps1`, PowerShell 7 validation scripts, repository text `format=GameEngine.*` contracts, generated `engine/agent/manifest.json` from manifest fragments.

---

**Plan ID:** `engine-contract-version-suffix-cleanup`

**Status:** Active.

Candidate 2 is validated locally on linked worktree `G:\workspace\development\GameEngine\.worktrees\versionless-saved-formats` branch `codex/versionless-saved-formats`. Candidate 1 is published as draft PR #242.

**Design:** [Engine Contract Version Suffix Cleanup Design](../../specs/2026-05-26-engine-contract-version-suffix-cleanup-design.md)

**Context:** A previous session could not create a linked worktree because Git metadata writes failed with `Permission denied` for `.git/refs/heads/*.lock` and `.git/worktrees/*`. This session confirmed Git metadata writes are available by creating `codex/version-suffix-cleanup` at `.worktrees/version-suffix-cleanup`. Do not bypass GitHub Flow through GitHub REST/MCP object writes.

**Constraints:**

- Clean breaking changes are allowed; compatibility aliases, dual writers, fallback parsers, and migration shims are not allowed unless a later release policy explicitly requires them.
- Keep historical plan/spec IDs and evidence filenames stable.
- Preserve numeric `schemaVersion` and ABI layout version fields.
- Preserve external standard/version names and UV coordinate names.
- Update agent surfaces in the same candidate as contract changes.
- Use worktree/branch isolation before implementation. If Git metadata writes are still blocked, stop implementation and record the blocker instead of editing default-branch checkout code.

**Done When:** All candidates are implemented, validated, published through reviewable PRs, merged, and main is synchronized; current source/docs/manifest/static checks prove that removable current-contract `vN` suffixes are gone and the engine still validates.

---

## Candidate Split

1. `version-suffix-cleanup-planning`: design, plan, registry pointers, and branch/worktree readiness. No runtime contract code changes.
2. `versionless-saved-formats`: canonicalize `GameEngine.*.vN` saved formats and repository-owned saved data/templates.
3. `versionless-cpp-contracts`: canonicalize public C++ `*VN` and `*_vN` types/functions/files.
4. `versionless-editor-abi-tool-surfaces`: canonicalize editor dynamic ABI names and current tool/evidence surface IDs while keeping numeric ABI version checks.
5. `versionless-agent-docs-closeout`: reconcile docs, manifest fragments, schemas, static checks, skills, validation evidence, PR state, and final audit.

Execute candidates serially by default. Subagents may work only on disjoint write sets, and branch/worktree publishing must remain available before implementation candidates begin.

## File Structure

Create:

- `docs/specs/2026-05-26-engine-contract-version-suffix-cleanup-design.md`
- `docs/superpowers/plans/2026-05-26-engine-contract-version-suffix-cleanup.md`

Modify during planning:

- `docs/specs/README.md`
- `docs/superpowers/plans/README.md`
- `docs/roadmap.md`

Likely modify during implementation:

- `engine/assets/include/mirakana/assets/*.hpp`
- `engine/assets/src/*.cpp`
- `engine/scene/include/mirakana/scene/schema_v2.hpp`
- `engine/scene/src/schema_v2.cpp`
- `engine/scene/src/scene_io.cpp`
- `engine/scene/src/prefab.cpp`
- `engine/scene/src/prefab_overrides.cpp`
- `engine/runtime/include/mirakana/runtime/*.hpp`
- `engine/runtime/src/*.cpp`
- `engine/renderer/include/mirakana/renderer/*.hpp`
- `engine/renderer/src/frame_graph.cpp`
- `engine/tools/{asset,scene,shader}/*.cpp`
- `engine/tools/include/mirakana/tools/*.hpp`
- `editor/core/include/mirakana/editor/game_module_driver.hpp`
- `editor/core/src/*.cpp`
- `editor/src/main.cpp`
- `tests/unit/*.cpp`
- `games/*/runtime/**`
- `games/*/source/**`
- `games/*/game.agent.json`
- `tools/new-game-templates.ps1`
- `tools/check-ai-integration*.ps1`
- `tools/check-json-contracts*.ps1`
- `schemas/*.json`
- `engine/agent/manifest.fragments/*.json`
- generated `engine/agent/manifest.json`
- current-truth docs: `docs/architecture.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/ai-integration.md`, `docs/editor.md`, `docs/testing.md`, `docs/legal-and-licensing.md`

## Candidate 1: Planning And Worktree Readiness

**Files:**

- Create: `docs/specs/2026-05-26-engine-contract-version-suffix-cleanup-design.md`
- Create: `docs/superpowers/plans/2026-05-26-engine-contract-version-suffix-cleanup.md`
- Modify: `docs/specs/README.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`

- [x] **Step 1: Confirm the branch/worktree blocker is gone**

Run:

```powershell
git status --short --branch
git worktree add .worktrees/version-suffix-cleanup -b codex/version-suffix-cleanup
```

Expected: linked worktree is created. If this still fails with `.git/refs/heads/*.lock` or `.git/worktrees/*` permission errors, stop implementation and fix host/session Git metadata access before continuing.

- [x] **Step 2: Enter the linked worktree and prepare it**

Run:

```powershell
Set-Location .worktrees/version-suffix-cleanup
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1
git status --short --branch
```

Expected: the worktree is on `codex/version-suffix-cleanup`, prepared, and clean except for the Candidate 1 planning files.

- [x] **Step 3: Run the planning-only checks**

Run:

```powershell
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: all pass, or the output identifies only pre-existing blockers that are recorded before implementation.

- [x] **Step 4: Commit planning evidence**

Run:

```powershell
git add docs/specs/2026-05-26-engine-contract-version-suffix-cleanup-design.md docs/specs/README.md docs/superpowers/plans/2026-05-26-engine-contract-version-suffix-cleanup.md docs/superpowers/plans/README.md docs/roadmap.md
git diff --cached --check
git commit -m "docs: plan version suffix cleanup"
```

Expected: one planning commit containing only design, plan, and registry pointer files.

## Candidate 2: Saved-Format Canonicalization

**Files:**

- Modify: `engine/assets/src/source_asset_registry.cpp`
- Modify: `engine/assets/src/asset_source_format.cpp`
- Modify: `engine/assets/src/asset_identity.cpp`
- Modify: `engine/assets/src/asset_package.cpp`
- Modify: `engine/assets/src/material.cpp`
- Modify: `engine/assets/src/material_graph.cpp`
- Modify: `engine/assets/src/material_graph_shader_export.cpp`
- Modify: `engine/assets/src/ui_atlas_metadata.cpp`
- Modify: `engine/assets/src/tilemap_metadata.cpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `engine/scene/src/scene_io.cpp`
- Modify: `engine/scene/src/prefab.cpp`
- Modify: `engine/scene/src/prefab_overrides.cpp`
- Modify: `engine/scene/src/schema_v2.cpp`
- Modify: `engine/tools/asset/*.cpp`
- Modify: `engine/tools/scene/*.cpp`
- Modify: `engine/tools/shader/*.cpp`
- Modify: `editor/core/src/project.cpp`
- Modify: `editor/core/src/workspace.cpp`
- Modify: `editor/core/src/ui_model.cpp`
- Modify: `editor/core/src/playtest_package_review.cpp`
- Modify: `games/*/runtime/**`
- Modify: `games/*/source/**`
- Modify: `tools/new-game-templates.ps1`
- Modify: `schemas/*.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify/generated: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration*.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: current docs that name saved formats, including `docs/ai-game-development.md`, `docs/architecture.md`, `docs/current-capabilities.md`, `docs/editor.md`, `docs/roadmap.md`, and `docs/testing.md`
- Modify: `tests/unit/*.cpp`

- [x] **Step 1: Build the exact saved-format map**

Run:

```powershell
rg -o --no-line-number --no-filename "GameEngine\.[A-Za-z0-9_.]+\.v[0-9]+" engine games tests editor tools schemas docs |
    Sort-Object |
    Group-Object |
    Sort-Object Name |
    ForEach-Object { "{0} {1}" -f $_.Count, $_.Name }
```

Expected: a complete pre-change inventory. Save the output in the PR description, not in source files.

- [x] **Step 2: Update canonical format constants**

Replace current engine-owned constants according to this map:

```text
GameEngine.AssetIdentity.v2 -> GameEngine.AssetIdentity
GameEngine.SourceAssetRegistry.v1 -> GameEngine.SourceAssetRegistry
GameEngine.TextureSource.v1 -> GameEngine.TextureSource
GameEngine.MeshSource.v2 -> GameEngine.MeshSource
GameEngine.AudioSource.v1 -> GameEngine.AudioSource
GameEngine.Material.v1 -> GameEngine.Material
GameEngine.MaterialGraph.v1 -> GameEngine.MaterialGraph
GameEngine.MaterialGraphGeneratedHlsl.v0 -> GameEngine.MaterialGraphGeneratedHlsl
GameEngine.MaterialGraphShaderExport.v0 -> GameEngine.MaterialGraphShaderExport
GameEngine.MaterialInstance.v1 -> GameEngine.MaterialInstance
GameEngine.MorphMeshCpuSource.v1 -> GameEngine.MorphMeshCpuSource
GameEngine.AnimationFloatClipSource.v1 -> GameEngine.AnimationFloatClipSource
GameEngine.AnimationQuaternionClipSource.v1 -> GameEngine.AnimationQuaternionClipSource
GameEngine.AnimationTransformBindingSource.v1 -> GameEngine.AnimationTransformBindingSource
GameEngine.CookedTexture.v1 -> GameEngine.CookedTexture
GameEngine.CookedMesh.v2 -> GameEngine.CookedMesh
GameEngine.CookedMorphMeshCpu.v1 -> GameEngine.CookedMorphMeshCpu
GameEngine.CookedAnimationFloatClip.v1 -> GameEngine.CookedAnimationFloatClip
GameEngine.CookedAnimationQuaternionClip.v1 -> GameEngine.CookedAnimationQuaternionClip
GameEngine.CookedAudio.v1 -> GameEngine.CookedAudio
GameEngine.CookedSkinnedMesh.v1 -> GameEngine.CookedSkinnedMesh
GameEngine.CookedSpriteAnimation.v1 -> GameEngine.CookedSpriteAnimation
GameEngine.CookedPackageIndex.v1 -> GameEngine.CookedPackageIndex
GameEngine.PhysicsCollisionScene3D.v1 -> GameEngine.PhysicsCollisionScene3D
GameEngine.UiAtlas.v1 -> GameEngine.UiAtlas
GameEngine.Tilemap.v1 -> GameEngine.Tilemap
GameEngine.Scene.v0 -> GameEngine.Scene
GameEngine.Scene.v1 -> GameEngine.Scene
GameEngine.Scene.v2 -> GameEngine.Scene
GameEngine.Prefab.v0 -> GameEngine.Prefab
GameEngine.Prefab.v1 -> GameEngine.Prefab
GameEngine.Prefab.v2 -> GameEngine.Prefab
GameEngine.PrefabVariant.v1 -> GameEngine.PrefabVariant
GameEngine.Project.v0 -> GameEngine.Project
GameEngine.Project.v1 -> GameEngine.Project
GameEngine.Project.v2 -> GameEngine.Project
GameEngine.Project.v3 -> GameEngine.Project
GameEngine.Project.v4 -> GameEngine.Project
GameEngine.Workspace.v0 -> GameEngine.Workspace
GameEngine.Workspace.v1 -> GameEngine.Workspace
GameEngine.RuntimeSaveData.v0 -> GameEngine.RuntimeSaveData
GameEngine.RuntimeSaveData.v1 -> GameEngine.RuntimeSaveData
GameEngine.RuntimeSettings.v1 -> GameEngine.RuntimeSettings
GameEngine.RuntimeLocalizationCatalog.v1 -> GameEngine.RuntimeLocalizationCatalog
GameEngine.RuntimeInputActions.v1 -> GameEngine.RuntimeInputActions
GameEngine.RuntimeInputActions.v2 -> GameEngine.RuntimeInputActions
GameEngine.RuntimeInputActions.v3 -> GameEngine.RuntimeInputActions
GameEngine.RuntimeInputActions.v4 -> GameEngine.RuntimeInputActions
GameEngine.RuntimeInputRebindingProfile.v0 -> GameEngine.RuntimeInputRebindingProfile
GameEngine.RuntimeInputRebindingProfile.v1 -> GameEngine.RuntimeInputRebindingProfile
GameEngine.ShaderArtifacts.v1 -> GameEngine.ShaderArtifacts
GameEngine.ShaderArtifact.v1 -> GameEngine.ShaderArtifact
GameEngine.ShaderArtifactCacheIndex.v1 -> GameEngine.ShaderArtifactCacheIndex
GameEngine.ShaderArtifactProvenance.v1 -> GameEngine.ShaderArtifactProvenance
GameEngine.EditorAiPlaytestEvidence.v1 -> GameEngine.EditorAiPlaytestEvidence
GameEngine.EditorUiModel.v1 -> GameEngine.EditorUiModel
```

Generated package config names also lose `.v1`, for example `GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1` becomes `GameEngine.GeneratedDesktopRuntime3DPackage.Config`.

```text
GameEngine.GeneratedDesktopRuntime2DPackage.Config.v1 -> GameEngine.GeneratedDesktopRuntime2DPackage.Config
GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1 -> GameEngine.GeneratedDesktopRuntime3DPackage.Config
GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1 -> GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config
GameEngine.GeneratedDesktopRuntimePackage.Config.v1 -> GameEngine.GeneratedDesktopRuntimePackage.Config
```

Sample package config names lose `.v1` too:

```text
GameEngine.Sample2DDesktopRuntimePackage.Config.v1 -> GameEngine.Sample2DDesktopRuntimePackage.Config
GameEngine.SampleDesktopRuntimeGame.Config.v1 -> GameEngine.SampleDesktopRuntimeGame.Config
```

AI command schema names in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, generated manifest output, command tools, and static checks also lose `.v1`:

```text
GameEngine.AiCommand.AddOrUpdateComponent.Request.v1 -> GameEngine.AiCommand.AddOrUpdateComponent.Request
GameEngine.AiCommand.AddOrUpdateComponent.Result.v1 -> GameEngine.AiCommand.AddOrUpdateComponent.Result
GameEngine.AiCommand.AddSceneNode.Request.v1 -> GameEngine.AiCommand.AddSceneNode.Request
GameEngine.AiCommand.AddSceneNode.Result.v1 -> GameEngine.AiCommand.AddSceneNode.Result
GameEngine.AiCommand.CookRegisteredSourceAssets.Request.v1 -> GameEngine.AiCommand.CookRegisteredSourceAssets.Request
GameEngine.AiCommand.CookRegisteredSourceAssets.Result.v1 -> GameEngine.AiCommand.CookRegisteredSourceAssets.Result
GameEngine.AiCommand.CookRuntimePackage.Request.v1 -> GameEngine.AiCommand.CookRuntimePackage.Request
GameEngine.AiCommand.CookRuntimePackage.Result.v1 -> GameEngine.AiCommand.CookRuntimePackage.Result
GameEngine.AiCommand.CreateGameRecipe.Request.v1 -> GameEngine.AiCommand.CreateGameRecipe.Request
GameEngine.AiCommand.CreateGameRecipe.Result.v1 -> GameEngine.AiCommand.CreateGameRecipe.Result
GameEngine.AiCommand.CreateMaterialFromGraph.Request.v1 -> GameEngine.AiCommand.CreateMaterialFromGraph.Request
GameEngine.AiCommand.CreateMaterialFromGraph.Result.v1 -> GameEngine.AiCommand.CreateMaterialFromGraph.Result
GameEngine.AiCommand.CreateMaterialInstance.Request.v1 -> GameEngine.AiCommand.CreateMaterialInstance.Request
GameEngine.AiCommand.CreateMaterialInstance.Result.v1 -> GameEngine.AiCommand.CreateMaterialInstance.Result
GameEngine.AiCommand.CreatePrefab.Request.v1 -> GameEngine.AiCommand.CreatePrefab.Request
GameEngine.AiCommand.CreatePrefab.Result.v1 -> GameEngine.AiCommand.CreatePrefab.Result
GameEngine.AiCommand.CreateScene.Request.v1 -> GameEngine.AiCommand.CreateScene.Request
GameEngine.AiCommand.CreateScene.Result.v1 -> GameEngine.AiCommand.CreateScene.Result
GameEngine.AiCommand.InstantiatePrefab.Request.v1 -> GameEngine.AiCommand.InstantiatePrefab.Request
GameEngine.AiCommand.InstantiatePrefab.Result.v1 -> GameEngine.AiCommand.InstantiatePrefab.Result
GameEngine.AiCommand.MigrateSceneV2RuntimePackage.Request.v1 -> GameEngine.AiCommand.MigrateSceneV2RuntimePackage.Request
GameEngine.AiCommand.MigrateSceneV2RuntimePackage.Result.v1 -> GameEngine.AiCommand.MigrateSceneV2RuntimePackage.Result
GameEngine.AiCommand.RefreshPrefabInstance.Request.v1 -> GameEngine.AiCommand.RefreshPrefabInstance.Request
GameEngine.AiCommand.RefreshPrefabInstance.Result.v1 -> GameEngine.AiCommand.RefreshPrefabInstance.Result
GameEngine.AiCommand.RegisterRuntimePackageFiles.Request.v1 -> GameEngine.AiCommand.RegisterRuntimePackageFiles.Request
GameEngine.AiCommand.RegisterRuntimePackageFiles.Result.v1 -> GameEngine.AiCommand.RegisterRuntimePackageFiles.Result
GameEngine.AiCommand.RegisterSourceAsset.Request.v1 -> GameEngine.AiCommand.RegisterSourceAsset.Request
GameEngine.AiCommand.RegisterSourceAsset.Result.v1 -> GameEngine.AiCommand.RegisterSourceAsset.Result
GameEngine.AiCommand.RunValidationRecipe.Request.v1 -> GameEngine.AiCommand.RunValidationRecipe.Request
GameEngine.AiCommand.RunValidationRecipe.Result.v1 -> GameEngine.AiCommand.RunValidationRecipe.Result
GameEngine.AiCommand.UpdateGameAgentManifest.Request.v1 -> GameEngine.AiCommand.UpdateGameAgentManifest.Request
GameEngine.AiCommand.UpdateGameAgentManifest.Result.v1 -> GameEngine.AiCommand.UpdateGameAgentManifest.Result
GameEngine.AiCommand.UpdateScenePackage.Request.v1 -> GameEngine.AiCommand.UpdateScenePackage.Request
GameEngine.AiCommand.UpdateScenePackage.Result.v1 -> GameEngine.AiCommand.UpdateScenePackage.Result
GameEngine.AiCommand.UpdateUiAtlasMetadataPackage.Request.v1 -> GameEngine.AiCommand.UpdateUiAtlasMetadataPackage.Request
GameEngine.AiCommand.UpdateUiAtlasMetadataPackage.Result.v1 -> GameEngine.AiCommand.UpdateUiAtlasMetadataPackage.Result
GameEngine.AiCommand.ValidateRuntimeScenePackage.Request.v1 -> GameEngine.AiCommand.ValidateRuntimeScenePackage.Request
GameEngine.AiCommand.ValidateRuntimeScenePackage.Result.v1 -> GameEngine.AiCommand.ValidateRuntimeScenePackage.Result
```

`GameEngine.EditorGameModuleDriver.v1` is owned by Candidate 4 and intentionally remains after Candidate 2. `GameEngine.Unknown.v1` is not a current contract. Keep it only as an explicit stale/unknown-format rejection fixture if a test still needs it; otherwise replace it with `GameEngine.Unknown`.

- [x] **Step 3: Remove stale parser migrations**

For each parser that currently accepts multiple `GameEngine.*.vN` values, keep only the canonical value. Add or update rejection tests for stale pre-release values where the behavior matters externally.

- [x] **Step 4: Update repository-owned saved files and templates**

Run targeted searches after editing:

```powershell
rg -n "format=GameEngine\.[A-Za-z0-9_.]+\.v[0-9]+" games engine tests editor tools schemas
rg -n "GameEngine\.[A-Za-z0-9_.]+\.v[0-9]+" tools/new-game-templates.ps1 games
```

Expected: no current sample/template saved format requires a pre-release suffix. Remaining hits must be historical docs or explicitly exempt text.

- [x] **Step 5: Run focused saved-format tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_scene_schema_v2_tests MK_runtime_tests MK_runtime_scene_tests MK_tools_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(asset_identity_runtime_resource_tests|scene_schema_v2_tests|runtime_tests|runtime_scene_tests|tools_tests|editor_core_tests)"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: targeted tests and the owning manifest/static checks pass.

Candidate 2 evidence:

- Focused build and CTest passed for saved-format/runtime/editor/package targets after the canonicalization edits.
- Suffix audit leaves only stale rejection fixtures, `GameEngine.Unknown.v1`, and Candidate 4 `GameEngine.EditorGameModuleDriver.v1` hits.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-26 after package index content hashes were regenerated for the updated runtime payload text.

- [x] **Step 6: Commit saved-format canonicalization**

Run:

```powershell
git add engine assets editor games tests tools schemas
git diff --cached --check
git commit -m "refactor: canonicalize saved contract formats"
```

Expected: one validated commit with saved-format code/data/test changes only. If docs/static checks also had to change to keep tests green, record that coupling in the commit body.

## Candidate 3: Public C++ Contract Canonicalization

**Files:**

- Modify/rename: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Modify/rename: `engine/scene/src/schema_v2.cpp`
- Modify/rename: `tests/unit/scene_schema_v2_tests.cpp`
- Modify: public headers and sources under `engine/assets`, `engine/runtime`, `engine/renderer`, `engine/tools`, `editor/core`
- Modify: dependent tests under `tests/unit`

- [ ] **Step 1: Build the exact C++ suffix map**

Run:

```powershell
rg -o --no-line-number --no-filename "\b[A-Za-z][A-Za-z0-9]*V[0-9]+\b|\b[a-z][a-z0-9_]*_v[0-9]+\b" engine games tests editor tools schemas --glob "!out/**" |
    Sort-Object |
    Group-Object |
    Sort-Object Count -Descending |
    ForEach-Object { "{0} {1}" -f $_.Count, $_.Name }
```

Expected: an inventory used to drive the rename. Do not mechanically change `uv.v1`, `u1`, `v1`, `schemaVersion`, or external version fields.

- [ ] **Step 2: Rename the highest-blast-radius families first**

Apply canonical renames in this order:

```text
AssetKeyV2 -> AssetKey
AssetIdentityDocumentV2 -> AssetIdentityDocument
AssetIdentityRowV2 -> AssetIdentityRow
AssetIdentityDiagnosticCodeV2 -> AssetIdentityDiagnosticCode
asset_id_from_key_v2 -> asset_id_from_key
plan_asset_identity_placements_v2 -> plan_asset_identity_placements
SourceAssetRegistryDocumentV1 -> SourceAssetRegistryDocument
SourceAssetRegistryRowV1 -> SourceAssetRegistryRow
SourceAssetDependencyRowV1 -> SourceAssetDependencyRow
SceneDocumentV2 -> SceneDocument
PrefabDocumentV2 -> PrefabDocument
SceneNodeDocumentV2 -> SceneNodeDocument
SceneComponentDocumentV2 -> SceneComponentDocument
serialize_scene_document_v2 -> serialize_scene_document
deserialize_scene_document_v2 -> deserialize_scene_document
validate_scene_document_v2 -> validate_scene_document
serialize_prefab_document_v2 -> serialize_prefab_document
deserialize_prefab_document_v2 -> deserialize_prefab_document
validate_prefab_document_v2 -> validate_prefab_document
RuntimeResourceCatalogV2 -> RuntimeResourceCatalog
RuntimeResourceHandleV2 -> RuntimeResourceHandle
RuntimeResidentPackageMountSetV2 -> RuntimeResidentPackageMountSet
RuntimeResidentPackageMountIdV2 -> RuntimeResidentPackageMountId
find_runtime_resource_v2 -> find_runtime_resource
build_runtime_resource_catalog_v2 -> build_runtime_resource_catalog
runtime_resource_record_v2 -> runtime_resource_record
compile_frame_graph_v1 -> compile_frame_graph
```

Keep numeric value fields such as `abi_version`, `schemaVersion`, and `generation` unchanged.

Do not stop after the high-blast-radius map above. The inventory also shows current runtime package/resource families that must lose their `V2` / `_v2` markers in the same C++ contract candidate:

```text
RuntimePackageCandidateLoad*
RuntimePackageCandidateResidentMount*
RuntimePackageCandidateResidentReplace*
RuntimePackageCandidateResidentMountReviewedEvictions*
RuntimePackageCandidateResidentReplaceReviewedEvictions*
RuntimePackageDiscoveryResidentCommit*
RuntimePackageDiscoveryResidentMountReviewedEvictions*
RuntimePackageDiscoveryResidentReplaceReviewedEvictions*
RuntimePackageHotReloadCandidateReview*
RuntimePackageHotReloadRecookChangeReview*
RuntimePackageHotReloadReplacementIntentReview*
RuntimePackageHotReloadRecookReplacement*
RuntimePackageIndexDiscovery*
RuntimeResidentCatalogCache*
RuntimeResidentPackageEvictionPlan*
RuntimeResidentPackageMount*
RuntimeResidentPackageReviewedEvictionCommit*
RuntimeResidentPackageReplaceCommit*
RuntimeResidentPackageUnmountCommit*
RuntimeResourceCatalogBuild*
RuntimeResourceHandle*
RuntimeResourceRecord*
RuntimeResourceResidencyBudget*
AssetIdentityDiagnostic*
AssetIdentityPlacement*
PrefabOverride*
PrefabVariant*
SceneComponentPrefabSource*
SceneNodePrefabSource*
ScenePrefabInstanceRefresh*
SourceAssetRegistry*
SourceAssetDependency*
ResourceStateV1 -> ResourceState
ResourceUseV1 -> ResourceUse
add_scene_prefab_instance_refresh_row_v2 -> add_scene_prefab_instance_refresh_row
apply_scene_prefab_instance_refresh_v2 -> apply_scene_prefab_instance_refresh
collect_scene_subtree_node_ids_v2 -> collect_scene_subtree_node_ids
compose_prefab_variant_v2 -> compose_prefab_variant
deserialize_asset_identity_document_v2 -> deserialize_asset_identity_document
emit_material_graph_reviewed_hlsl_v0 -> emit_material_graph_reviewed_hlsl
expected_source_asset_format_v1 -> expected_source_asset_format
is_supported_source_asset_kind_v1 -> is_supported_source_asset_kind
parse_source_asset_dependency_kind_v1 -> parse_source_asset_dependency_kind
parse_source_asset_registry_document_unvalidated_v1 -> parse_source_asset_registry_document_unvalidated
plan_scene_prefab_instance_refresh_v2 -> plan_scene_prefab_instance_refresh
project_source_asset_registry_identity_v2 -> project_source_asset_registry_identity
commit_runtime_package_candidate_resident_mount_v2 -> commit_runtime_package_candidate_resident_mount
commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2 -> commit_runtime_package_candidate_resident_mount_with_reviewed_evictions
commit_runtime_package_candidate_resident_replace_v2 -> commit_runtime_package_candidate_resident_replace
commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2 -> commit_runtime_package_candidate_resident_replace_with_reviewed_evictions
commit_runtime_package_discovery_resident_v2 -> commit_runtime_package_discovery_resident
commit_runtime_package_discovery_resident_mount_with_reviewed_evictions_v2 -> commit_runtime_package_discovery_resident_mount_with_reviewed_evictions
commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2 -> commit_runtime_package_discovery_resident_replace_with_reviewed_evictions
commit_runtime_package_hot_reload_recook_replacement_v2 -> commit_runtime_package_hot_reload_recook_replacement
commit_runtime_resident_package_replace_v2 -> commit_runtime_resident_package_replace
commit_runtime_resident_package_reviewed_evictions_v2 -> commit_runtime_resident_package_reviewed_evictions
commit_runtime_resident_package_unmount_v2 -> commit_runtime_resident_package_unmount
discover_runtime_package_indexes_v2 -> discover_runtime_package_indexes
is_runtime_resource_handle_live_v2 -> is_runtime_resource_handle_live
load_runtime_package_candidate_v2 -> load_runtime_package_candidate
plan_runtime_package_hot_reload_candidate_review_v2 -> plan_runtime_package_hot_reload_candidate_review
plan_runtime_package_hot_reload_recook_change_review_v2 -> plan_runtime_package_hot_reload_recook_change_review
plan_runtime_package_hot_reload_replacement_intent_review_v2 -> plan_runtime_package_hot_reload_replacement_intent_review
plan_runtime_resident_package_evictions_v2 -> plan_runtime_resident_package_evictions
```

Stale `compile_frame_graph_v0` helpers are not a new canonical API. Remove them or convert their coverage into explicit stale-schedule rejection tests while keeping `compile_frame_graph` as the only current compile entrypoint.

- [ ] **Step 3: Rename files and build references**

Use native Git moves when Git metadata writes are available:

```powershell
git mv engine/scene/include/mirakana/scene/schema_v2.hpp engine/scene/include/mirakana/scene/schema.hpp
git mv engine/scene/src/schema_v2.cpp engine/scene/src/schema.cpp
git mv tests/unit/scene_schema_v2_tests.cpp tests/unit/scene_schema_tests.cpp
```

Then update `CMakeLists.txt`, includes, manifest public header lists, and static-check needles.

- [ ] **Step 4: Run focused public API checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_scene_schema_tests MK_runtime_tests MK_runtime_scene_tests MK_tools_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(asset_identity_runtime_resource_tests|scene_schema_tests|runtime_tests|runtime_scene_tests|tools_tests|editor_core_tests)"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: targeted tests and public API checks pass.

- [ ] **Step 5: Commit public API canonicalization**

Run:

```powershell
git add engine editor tests tools games CMakeLists.txt
git diff --cached --check
git commit -m "refactor: canonicalize current C++ contracts"
```

Expected: one validated commit with API/file/caller updates.

## Candidate 4: Editor ABI And Tool-Surface Canonicalization

**Files:**

- Modify: `editor/core/include/mirakana/editor/game_module_driver.hpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/editor_game_module_driver_load_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify/generated: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-040-agent-surfaces.ps1`
- Modify: `tools/check-json-contracts-010-engine-manifest.ps1`
- Modify: `docs/architecture.md`, `docs/editor.md`, `docs/current-capabilities.md`, `docs/legal-and-licensing.md`, `docs/testing.md`
- Modify: `docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md` only where current ABI guidance must name the canonical symbol

- [ ] **Step 1: Canonicalize editor ABI names**

Apply these renames:

```text
editor_game_module_driver_abi_name_v1 -> editor_game_module_driver_abi_name
editor_game_module_driver_factory_symbol_v1 -> editor_game_module_driver_factory_symbol
editor_game_module_driver_abi_version_v1 -> editor_game_module_driver_abi_version
editor_game_module_driver_contract_v1 -> editor_game_module_driver_contract
editor_game_module_driver_host_session_contract_v1 -> editor_game_module_driver_host_session_contract
editor_game_module_driver_host_session_dll_barriers_contract_v1 -> editor_game_module_driver_host_session_dll_barriers_contract
editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1 -> editor_game_module_driver_reload_transaction_recipe_evidence_contract
mirakana_create_editor_game_module_driver_v1 -> mirakana_create_editor_game_module_driver
GameEngine.EditorGameModuleDriver.v1 -> GameEngine.EditorGameModuleDriver
```

Keep the numeric `editor_game_module_driver_abi_version` value at `1`.

- [ ] **Step 2: Update probe/export tests and docs**

Update all probe/load tests to resolve `mirakana_create_editor_game_module_driver`. Update current docs to describe the canonical factory symbol and numeric ABI version separately.

- [ ] **Step 3: Run editor ABI tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_game_module_driver_load_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(editor_core_tests|editor_game_module_driver_load_tests)"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: editor-core, dynamic probe, and owning manifest/static checks pass.

- [ ] **Step 4: Commit editor ABI canonicalization**

Run:

```powershell
git add editor tests tools docs
git diff --cached --check
git commit -m "refactor: canonicalize editor module ABI names"
```

Expected: one validated commit with ABI names and docs/static checks aligned.

## Candidate 5: Agent Surface, Docs, And Final Closeout

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/ai-integration.md`
- Modify: `docs/editor.md`
- Modify: `docs/testing.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify/generated: `engine/agent/manifest.json`
- Modify: `schemas/game-agent.schema.json`
- Modify: `tools/check-ai-integration*.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify if current guidance changes: `.agents/skills/*.md`, `.claude/skills/*/SKILL.md`, `.cursor/skills/*/SKILL.md`

Candidate 5 is the final audit and lifecycle closeout. It must not be used to postpone manifest, schema, static-check, docs, or test updates that belong to the contract-owning Candidates 2-4.

- [ ] **Step 1: Switch and close out the active plan deliberately**

When Candidate 2 begins on a branch/worktree, update `docs/superpowers/plans/README.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` so `currentActivePlan` points to this plan. Do not claim completion until Candidates 2-5 validate.

When Candidate 5 closes, update `currentActivePlan`, `recommendedNextPlan`, registry status, and roadmap wording in the same change. If `Generated Game Studio v1` is still the broader active milestone, return this cleanup from active child status to completed docs/governance evidence under that milestone; otherwise point back to the production-completion master plan and record the selected next plan explicitly.

- [ ] **Step 2: Compose the manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: `engine/agent/manifest.json` is regenerated from fragments.

- [ ] **Step 3: Run canonical-name audits**

Run:

```powershell
rg -n "GameEngine\.[A-Za-z0-9_.]+\.v[0-9]+" engine games tests editor tools schemas docs
rg -n "\b[A-Za-z][A-Za-z0-9]*V[0-9]+\b|\b[a-z][a-z0-9_]*_v[0-9]+\b" engine games tests editor tools schemas --glob "!out/**"
rg -n "mirakana_create_editor_game_module_driver_v1|GameEngine\.EditorGameModuleDriver\.v1" .
```

Expected: remaining hits are only approved historical plan/spec/evidence text, external standards, numeric fields, or coordinate-style false positives. Record the approved remaining-hit classes in the PR.

- [ ] **Step 4: Run agent and schema checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: all pass.

- [ ] **Step 5: Run full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full validation passes, or host/toolchain blockers are concrete and recorded.

- [ ] **Step 6: Publish through GitHub Flow**

Run after a clean validated worktree:

```powershell
git status --short --branch
$branch = git branch --show-current
if ($branch -eq "main" -or [string]::IsNullOrWhiteSpace($branch)) { throw "publish requires a non-main branch" }
git log --oneline origin/main..HEAD
gh auth status
git push -u origin $branch
gh pr create --draft --fill
```

Expected: the branch is not `main`, the branch is pushed, and a draft PR exists for the candidate or tightly coupled candidate group. Do not push directly to `main`.

## Verification Matrix

| Requirement | Evidence |
| --- | --- |
| Saved format suffixes removed from current contracts | `rg "GameEngine\.[A-Za-z0-9_.]+\.v[0-9]+" engine games tests editor tools schemas docs` with only approved historical/external hits |
| Public C++ contract suffixes removed | `rg "\b[A-Za-z][A-Za-z0-9]*V[0-9]+\b|\b[a-z][a-z0-9_]*_v[0-9]+\b" engine games tests editor tools schemas --glob "!out/**"` with only approved false positives |
| Old parser branches removed | stale-format rejection tests in the owning unit tests |
| Samples/templates canonicalized | searches over `games/**` and `tools/new-game-templates.ps1` |
| Editor ABI canonicalized | `MK_editor_game_module_driver_load_tests` and `MK_editor_core_tests` |
| Agent surface aligned | `check-ai-integration.ps1`, `check-json-contracts.ps1`, composed manifest diff |
| Whole repository still works | `tools/validate.ps1` |
