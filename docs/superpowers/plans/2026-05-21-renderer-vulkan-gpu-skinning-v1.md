# Renderer Vulkan GPU Skinning Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-gpu-skinning-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan GPU skinning package evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- D3D12 GPU Skinning Upload And Rendering v1 completed with `--require-gpu-skinning` on the selected D3D12 package lane.
- `installed-vulkan-scene-gpu-smoke` already exercises the cooked skinned mesh scene on Vulkan but did not require GPU skinning counters.
- Non-goal: broad skeletal animation production, Metal parity, or public native/RHI handles.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing `scene_gpu_stats` and `RendererStats` GPU skinning counters; do not expose native handles.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Vulkan GPU Skinning Package Flag

**Status:** Completed.

### Done When

- `--require-vulkan-gpu-skinning-evidence` exists on `sample_desktop_runtime_game`.
- Smoke exit validates GPU skinning counters on the selected Vulkan backend.

## Phase 2: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests `--require-vulkan-gpu-skinning-evidence`.
- `tools/validate-installed-desktop-runtime.ps1` validates GPU skinning and Vulkan fields when the flag is set.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `sample_desktop_runtime_game` build | Pass |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal GPU skinning package promotion (Apple-host gated).
- Treating D3D12 package smoke as Vulkan/Metal proof.
- Broad skeletal animation or skin+morph composition production readiness.
