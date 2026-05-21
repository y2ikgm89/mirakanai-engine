# Generated 3D Vulkan Visible Production Proof v1 (2026-05-21)

**Plan ID:** `generated-3d-vulkan-visible-production-proof-v1`
**Status:** Completed.
**Current pointer rule:** Keep `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` on the production master plan after closeout. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan visible generated 3D production-style package evidence without treating D3D12 proof as cross-backend evidence.

## Context

- D3D12 visible proof already existed through `--require-visible-3d-production-proof` and `visible_3d_d3d12_selected`.
- Vulkan hosts needed an explicit `--require-vulkan-visible-3d-production-proof` path with `visible_3d_vulkan_selected`.
- Non-goal: Metal parity, native/RHI handle exposure, or broad generated 3D production readiness.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Require toolchain-gated SPIR-V artifacts for Vulkan package smokes.
- Do not claim D3D12 proof transfers to Vulkan.

## Phase 1: Vulkan Visible Package Flag

**Status:** Completed.

### Done When

- `--require-vulkan-visible-3d-production-proof` exists on `sample_generated_desktop_runtime_3d_package`.
- Smoke stdout reports `visible_3d_vulkan_selected=1` when the flag is set.
- Smoke exit requires visible aggregate readiness on the Vulkan-selected path.

## Phase 2: Package Smoke And Validation

**Status:** Completed.

### Done When

- `desktop-runtime-release-target-vulkan-visible-production-proof-toolchain-gated` exists.
- `tools/validate-installed-desktop-runtime.ps1` validates Vulkan visible fields when the flag is set.
- Agent surfaces stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal visible production proof.
- Treating D3D12 visible proof as Vulkan proof.
- Broad generated 3D production readiness.
