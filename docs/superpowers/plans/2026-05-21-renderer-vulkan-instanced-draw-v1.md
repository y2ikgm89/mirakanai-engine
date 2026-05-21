# Renderer Vulkan Instanced Draw v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-instanced-draw-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan instanced draw execution evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `renderer-scene-scale-v1` completed with backend-neutral scene-scale policy, package-visible counters, and selected D3D12 instanced draw execution evidence.
- `renderer-vulkan-gpu-memory-v1` completed with per-backend GPU memory execution evidence on Vulkan.
- Non-goal from backend parity: Vulkan instanced draw execution parity (this plan).

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Use backend-neutral `RhiStats` instanced draw counters for evidence; do not expose native handles.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Backend Instancing Evidence Criteria

**Status:** Completed.

### Done When

- `SceneScaleBackendInstancingEvidenceDesc` and `scene_scale_policy_backend_instancing_evidence_ready` exist in `MK_renderer`.
- `MK_renderer_tests` cover D3D12/Vulkan evidence and cross-backend rejection.

## Phase 2: Vulkan SDL Presentation Counters

**Status:** Completed.

### Done When

- `evaluate_sdl_desktop_presentation_vulkan_instanced_draw_execution` exists.
- `MK_runtime_host_sdl3_tests` cover ready/blocked paths.
- `--require-vulkan-instanced-draw-evidence` exists on the sample game.

## Phase 3: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests scene-scale policy and Vulkan instanced draw evidence.
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

- Metal instanced draw package promotion (Apple-host gated).
- Vulkan/Metal postprocess execution parity.
- Treating D3D12 package smoke as Vulkan/Metal proof.
