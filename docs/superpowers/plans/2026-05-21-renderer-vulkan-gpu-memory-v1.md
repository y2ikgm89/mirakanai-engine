# Renderer Vulkan GPU Memory v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-gpu-memory-v1`
**Status:** Completed.
**Current pointer rule:** Completed; `engine/agent/manifest.json.aiOperableProductionLoop` points at the production-completion master plan with `recommendedNextPlan.id=next-production-gap-selection`.

## Goal

Promote selected Vulkan GPU memory execution evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `renderer-gpu-memory-v1` completed with backend-neutral GPU memory policy, package-visible counters, and selected D3D12 execution evidence.
- `renderer-backend-parity-v1` completed with cross-backend proof guards and Vulkan debug profiling execution evidence.
- Non-goal from backend parity: Vulkan GPU memory execution parity (this plan).

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Do not require DXGI `QueryVideoMemoryInfo` on Vulkan; use committed-byte estimates, upload bytes, and framegraph/transient pressure instead.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Backend Memory Evidence Criteria

**Status:** Completed.

### Done When

- `GpuMemoryBackendEvidenceDesc` and `gpu_memory_policy_backend_evidence_ready` exist in `MK_renderer`.
- `MK_renderer_tests` cover D3D12 OS-budget and Vulkan framegraph-pressure criteria.

## Phase 2: Vulkan SDL Presentation Counters

**Status:** Completed.

### Done When

- `evaluate_sdl_desktop_presentation_vulkan_gpu_memory_execution` exists.
- SDL GPU memory policy uses per-backend backend evidence without requiring OS video memory on Vulkan.
- `MK_runtime_host_sdl3_tests` cover ready/blocked paths.
- `--require-vulkan-gpu-memory-evidence` exists on the sample game.

## Phase 3: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests GPU memory policy and Vulkan execution evidence.
- `tools/validate-installed-desktop-runtime.ps1` validates Vulkan status fields.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `MK_renderer_tests` | Pass |
| `MK_runtime_host_sdl3_tests` | Pass |
| `tools/check-agents.ps1` | Pass (after compose) |
| `tools/check-ai-integration.ps1` | Pass (after compose) |

## Non-Goals

- Metal GPU memory package promotion (Apple-host gated).
- `VK_EXT_memory_budget` host wiring (separate milestone).
- Treating D3D12 package smoke as Vulkan/Metal proof.
