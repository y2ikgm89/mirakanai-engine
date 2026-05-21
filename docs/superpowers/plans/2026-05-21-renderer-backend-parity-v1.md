# Renderer Backend Parity v1 (2026-05-21)

**Plan ID:** `renderer-backend-parity-v1`
**Status:** Completed.
**Current pointer rule:** Completed; `engine/agent/manifest.json.aiOperableProductionLoop` points at the production-completion master plan with `recommendedNextPlan.id=next-production-gap-selection`.

## Goal

Promote selected Vulkan and Metal renderer features independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `renderer-debug-profiling-v1` completed with backend-neutral debug profiling policy, package-visible counters, and selected D3D12 execution evidence.
- Advanced production track lists backend parity after debug profiling.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Keep manifest host gates explicit for Vulkan validation-layer/SPIR-V and Apple-host Metal evidence.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 0: Pointer Sync

**Status:** Completed.

### Done When

- Manifest, roadmap, and plan registry point `currentActivePlan` at this plan.
- `renderer-debug-profiling-v1` is recorded as completed.

## Phase 1: Backend Parity Policy and Per-Backend Evidence Rows

**Status:** Completed.

### Goal

Add `backend_renderer_parity_policy` cross-backend proof guards and `debug_profiling_policy_backend_evidence_ready` with per-backend criteria (D3D12 timestamps + markers; Vulkan markers + framegraph without timestamp ticks).

### Done When

- `backend_renderer_parity_policy.hpp/cpp` and `debug_profiling_policy_backend_evidence_ready` exist in `MK_renderer`.
- `MK_renderer_tests` cover D3D12/Vulkan evidence and cross-backend proof rejection.

## Phase 2: Vulkan SDL Presentation Counters

**Status:** Completed.

### Goal

Expose Vulkan debug profiling execution through `SdlDesktopPresentationReport` and `sample_desktop_runtime_game` status fields.

### Done When

- `evaluate_sdl_desktop_presentation_vulkan_debug_profiling_execution` exists.
- Vulkan debug profiling policy requests use `vulkan_debug_utils` without mandatory GPU timestamps.
- `MK_runtime_host_sdl3_tests` cover ready/blocked paths.
- `--require-vulkan-debug-profiling-evidence` exists on the sample game.

## Phase 3: Vulkan Package Smoke Promotion

**Status:** Completed.

### Goal

Validate installed Vulkan package smoke independently from the D3D12 default recipe.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests debug profiling policy and Vulkan execution evidence.
- `tools/validate-installed-desktop-runtime.ps1` validates Vulkan status fields.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `MK_renderer_tests` | Pass |
| `MK_runtime_host_sdl3_tests` | Pass |
| `tools/check-agents.ps1` | Pass (after compose) |
| `tools/check-ai-integration.ps1` | Pass (after compose) |
| `tools/validate.ps1` | Pass (70/70 slice gate) |

## Non-Goals

- Metal debug profiling package promotion (Apple-host gated).
- Vulkan GPU memory execution parity (separate milestone).
- Treating D3D12 package smoke as Vulkan/Metal proof.
