# Asset Identity v2 Command Apply Surface Evidence v1 (2026-05-12)

**Plan ID:** `asset-identity-v2-command-apply-surface-evidence-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `asset-identity-v2`

## Goal

Close the `asset command apply surfaces` blocker for Asset Identity v2 by recording the existing reviewed command-owned mutation surfaces as sufficient evidence, without adding a new raw `GameEngine.AssetIdentity.v2` write command or widening renderer/editor readiness claims.

## Context

- [2026-05-12-asset-identity-v2-placement-resolution-v1.md](2026-05-12-asset-identity-v2-placement-resolution-v1.md) added `plan_asset_identity_placements_v2` for deterministic reviewed key-to-asset placement evidence.
- Existing `MK_tools` command surfaces already cover reviewed source identity registration, selected source cook/package updates, Scene v2 runtime package migration, and scene/material/UI package row writes.
- Raw `GameEngine.AssetIdentity.v2` is a declarative identity document. It cannot safely replace `GameEngine.SourceAssetRegistry.v1` or package update requests because those command-owned mutation targets carry source paths, import formats, package rows, dependency declarations, and apply validation context.

## Constraints

- Do not add a new C++ public API in this slice.
- Do not claim scene/render/UI/gameplay reference cleanup, editor asset browser migration, renderer/RHI residency, package streaming, broad package cooking, or 2D/3D playable vertical-slice readiness.
- Keep `asset-identity-v2` in `unsupportedProductionGaps` until scene/render/UI/gameplay reference cleanup and editor asset browser migration are closed or explicitly excluded.
- Keep static checks honest: remove only the `asset command apply surfaces` required-before-ready claim.

## File Plan

- Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and regenerate `engine/agent/manifest.json`.
- Update `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` to enforce the narrowed `asset-identity-v2` blocker list and the reviewed command-owned apply-surface evidence wording.
- Update current docs and plan registries so the live plan stack returns to the master plan after this closeout.

## Tasks

- [x] Replace the stale keyed scene package child plan with this evidence closeout plan.
- [x] Remove `asset command apply surfaces` from `asset-identity-v2.requiredBeforeReadyClaim`.
- [x] Record the reviewed command-owned apply surfaces as existing evidence: `apply_source_asset_registration`, `apply_registered_source_asset_cook_package`, `apply_scene_v2_runtime_package_migration`, `apply_scene_package_update`, `apply_material_instance_package_update`, `apply_material_graph_package_update`, `apply_cooked_ui_atlas_package_update`, `apply_packed_ui_atlas_package_update`, and `apply_packed_ui_glyph_atlas_package_update`.
- [x] Keep scene/render/UI/gameplay reference cleanup and editor asset browser migration as required-before-ready claims.
- [x] Run focused `MK_tools_tests` and `MK_asset_identity_runtime_resource_tests`, then static checks and full validation.

## Done When

- Manifest, docs, and static checks agree that command-owned apply surfaces are evidenced and no longer block `asset-identity-v2`.
- `asset-identity-v2` still honestly blocks on scene/render/UI/gameplay reference cleanup and editor asset browser migration.
- Full repository validation passes.

## Validation Evidence

- PASS: `cmake --build --preset dev --target MK_tools_tests`.
- PASS: `ctest --preset dev --output-on-failure -R MK_tools_tests`.
- PASS: `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests`.
- PASS: `ctest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (47/47 tests; Metal and Apple lanes remain diagnostic-only host-gated on this Windows host).
