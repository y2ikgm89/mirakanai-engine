# Renderer D3D12 GPU Skinning Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-d3d12-gpu-skinning-package-evidence-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected D3D12 GPU skinning package evidence independently from Vulkan without treating Vulkan package smoke as cross-backend proof.

## Context

- `renderer-vulkan-gpu-skinning-v1` completed with `--require-vulkan-gpu-skinning-evidence` and `vulkan_gpu_skinning_evidence_ready/selected`.
- D3D12 used `--require-gpu-skinning` with shared counters but lacked per-backend ready/selected fields.
- Non-goal: broad skeletal animation production, Metal parity, or public native/RHI handles.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing GPU skinning counters; do not expose native handles.
- Do not claim Vulkan proof transfers to D3D12.

## Phase 1: D3D12 GPU Skinning Package Flag

**Status:** Completed.

### Done When

- `--require-d3d12-gpu-skinning-evidence` exists on `sample_desktop_runtime_game`.
- `d3d12_gpu_skinning_evidence_ready/selected` exist on smoke stdout when the flag is set.
- Smoke exit uses a D3D12-selected backend gate.

## Phase 2: Package Smoke And Validation

**Status:** Completed.

### Done When

- `installed-d3d12-scene-gpu-smoke` requests `--require-d3d12-gpu-skinning-evidence`.
- `tools/validate-installed-desktop-runtime.ps1` validates D3D12 GPU skinning fields when the flag is set.
- Agent surfaces stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal GPU skinning package promotion (Apple-host gated).
- Treating Vulkan package smoke as D3D12 proof.
- Broad skeletal animation production readiness.
