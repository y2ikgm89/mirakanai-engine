# Asset Identity v2 Placement Resolution v1 (2026-05-12)

**Plan ID:** `asset-identity-v2-placement-resolution-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `asset-identity-v2`

## Goal

Add a host-independent `MK_assets` placement-resolution planner that turns reviewed `AssetKeyV2` rows into deterministic `AssetId`/kind/source placement rows for production asset placement without migrating scene/render/UI/gameplay components or claiming renderer/RHI residency.

## Context

- Asset Identity v2 already provides stable keys, deterministic `AssetId` derivation, validation, and `GameEngine.AssetIdentity.v2` text IO.
- The remaining `asset-identity-v2` gap entered this slice with production asset placement, scene/render/UI/gameplay reference cleanup, editor asset browser migration, and broader command apply surfaces.
- A narrow placement planner can remove the production placement blocker while keeping broad component migration and editor UX as separate follow-ups.

## Constraints

- Keep the implementation in `MK_assets`; do not add dependencies on scene, renderer, runtime, editor, platform, SDL3, Dear ImGui, or native handles.
- Do not mutate files, cook packages, execute importers, or update `.geindex` rows.
- Do not replace existing scene/render/UI/gameplay `AssetId` fields in this slice.
- Keep `asset-identity-v2` in `unsupportedProductionGaps` until the remaining reference cleanup, editor migration, and apply-surface rows are resolved.

## Done When

- `AssetIdentityPlacementRequestV2`, `AssetIdentityPlacementRowV2`, diagnostics, and `plan_asset_identity_placements_v2` are public `MK_assets` APIs.
- Focused tests prove successful mesh/material placement resolution, invalid identity diagnostic carry-through, explicit placement grammar, all-or-nothing row clearing, and deterministic missing-key/kind-mismatch rejection.
- Manifest/docs/static checks record production asset placement as implemented while leaving scene/render/UI/gameplay reference cleanup, editor asset browser migration, and asset command apply surfaces as required-before-ready claims.
- Validation evidence is recorded for focused tests, public API boundary checks, manifest composition/static checks, and full repository validation.

## Validation Evidence

- RED: `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` failed because `plan_asset_identity_placements_v2`, `AssetIdentityPlacementRequestV2`, and `AssetIdentityPlacementDiagnosticCodeV2` were not defined.
- PASS: `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests`.
- PASS: `ctest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests`.
- PASS: post-review `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` after tightening placement grammar, adding `std::span` requests, and adding invalid-document/all-or-nothing tests.
- PASS: post-review `ctest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/asset_identity.cpp`.
- PASS: post-review `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (47/47 tests).

## Next Candidate After Validation

- Continue `asset-identity-v2` with scene/render/UI/gameplay reference cleanup or a reviewed asset command apply surface.
