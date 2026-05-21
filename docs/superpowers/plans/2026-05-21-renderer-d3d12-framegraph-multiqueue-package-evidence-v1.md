# Renderer D3D12 Framegraph Multi-Queue Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-d3d12-framegraph-multiqueue-package-evidence-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected D3D12 Frame Graph multi-queue package evidence independently from Vulkan without treating Vulkan package smoke as cross-backend proof.

## Context

- `frame-graph-multiqueue-package-evidence-v1` completed with `--require-framegraph-multiqueue-evidence` and backend-neutral counters.
- Vulkan promotion added `--require-vulkan-framegraph-multiqueue-evidence` with `vulkan_framegraph_multiqueue_evidence_ready/selected`.
- D3D12 used the same flag and counters but lacked per-backend ready/selected fields.
- Non-goal: production graph adoption, async overlap claims, or native queue/fence exposure.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing `execute_frame_graph_rhi_multi_queue_package_evidence` counters.
- Do not claim Vulkan proof transfers to D3D12.

## Phase 1: D3D12 Multi-Queue Package Fields

**Status:** Completed.

### Done When

- `d3d12_framegraph_multiqueue_evidence_ready/selected` exist on smoke stdout when `--require-framegraph-multiqueue-evidence` is set.
- Smoke exit uses a D3D12-selected backend gate.

## Phase 2: Package Validation And Agent Surfaces

**Status:** Completed.

### Done When

- `installed-d3d12-scene-gpu-smoke` already requests `--require-framegraph-multiqueue-evidence`.
- `tools/validate-installed-desktop-runtime.ps1` validates D3D12 policy fields when the flag is set.
- Agent surfaces stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal multi-queue package promotion (Apple-host gated).
- Treating Vulkan package smoke as D3D12 proof.
- Production render graph scheduling adoption.
