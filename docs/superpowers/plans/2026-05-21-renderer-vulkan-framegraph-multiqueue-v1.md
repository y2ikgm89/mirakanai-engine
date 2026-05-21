# Renderer Vulkan Frame Graph Multi-Queue Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-framegraph-multiqueue-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan Frame Graph multi-queue package evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `frame-graph-multiqueue-package-evidence-v1` completed with `--require-framegraph-multiqueue-evidence` on the selected D3D12 package lane.
- `execute_frame_graph_rhi_multi_queue_package_evidence` is backend-neutral over `IRhiDevice`; Vulkan scene setup already exposes `scene_pbr_frame_rhi_device()`.
- Non-goal: production graph adoption, async overlap claims, Metal parity, or native queue/fence exposure.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing `FrameGraphRhiMultiQueuePackageEvidence` counters; do not expose native handles.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Vulkan Multi-Queue Package Flag

**Status:** Completed.

### Done When

- `--require-vulkan-framegraph-multiqueue-evidence` exists on `sample_desktop_runtime_game`.
- Smoke exit validates multi-queue counters on the selected Vulkan backend.

## Phase 2: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests `--require-vulkan-framegraph-multiqueue-evidence`.
- `tools/validate-installed-desktop-runtime.ps1` validates multi-queue fields when the flag is set.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `sample_desktop_runtime_game` build | Pass |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal multi-queue package promotion (Apple-host gated).
- Treating D3D12 package smoke as Vulkan/Metal proof.
- Production render-graph scheduling or async overlap evidence.
