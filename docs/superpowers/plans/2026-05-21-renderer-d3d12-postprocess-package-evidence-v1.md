# Renderer D3D12 Postprocess Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-d3d12-postprocess-package-evidence-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected D3D12 postprocess execution package evidence independently from Vulkan without treating Vulkan package smoke as cross-backend proof.

## Context

- `renderer-vulkan-postprocess-v1` completed with `--require-vulkan-postprocess-evidence` on `installed-vulkan-scene-gpu-smoke`.
- D3D12 postprocess execution counters already existed but were validated whenever `--require-d3d12-renderer` was set.
- Non-goal: Metal postprocess parity or new postprocess shader families.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing postprocess policy and `postprocess_d3d12_execution_*` counters.
- Do not claim Vulkan proof transfers to D3D12.

## Phase 1: D3D12 Postprocess Package Flag

**Status:** Completed.

### Done When

- `--require-d3d12-postprocess-evidence` exists on `sample_desktop_runtime_game`.
- `evaluate_sdl_desktop_presentation_d3d12_postprocess_execution` gates on an explicit requested flag like Vulkan.

## Phase 2: D3D12 Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-d3d12-scene-gpu-smoke` requests `--require-d3d12-postprocess-evidence`.
- `installed-d3d12-scene-gpu-smoke` requests `--require-gpu-skinning` for README alignment.
- `tools/validate-installed-desktop-runtime.ps1` validates D3D12 postprocess fields only when the flag is set.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `MK_runtime_host_sdl3_tests` | Pass |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal postprocess package promotion (Apple-host gated).
- Treating Vulkan package smoke as D3D12 proof.
- New postprocess shader families beyond the existing package path.
