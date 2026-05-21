# Renderer Vulkan Postprocess Execution v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-postprocess-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan postprocess execution evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `renderer-postprocess-v1` completed with backend-neutral postprocess policy and selected D3D12 postprocess execution evidence.
- `renderer-vulkan-instanced-draw-v1` and `renderer-vulkan-gpu-memory-v1` established per-backend Vulkan execution report patterns on `installed-vulkan-scene-gpu-smoke`.
- Non-goal from backend parity: Vulkan postprocess execution parity (this plan).

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing postprocess policy and renderer stats counters; do not expose native handles.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Vulkan SDL Presentation Execution Report

**Status:** Completed.

### Done When

- `SdlDesktopPresentationVulkanPostprocessExecutionReport` and `evaluate_sdl_desktop_presentation_vulkan_postprocess_execution` exist.
- `MK_runtime_host_sdl3_tests` cover ready/blocked paths.
- `--require-vulkan-postprocess-evidence` exists on the sample game.

## Phase 2: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests Vulkan postprocess execution evidence.
- `tools/validate-installed-desktop-runtime.ps1` validates Vulkan status fields.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `MK_runtime_host_sdl3_tests` | Pass |
| `tools/check-agents.ps1` | Pass |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal postprocess execution parity (Apple-host gated).
- Treating D3D12 package smoke as Vulkan/Metal proof.
- New postprocess shader families beyond existing Vulkan depth/postprocess package path.
