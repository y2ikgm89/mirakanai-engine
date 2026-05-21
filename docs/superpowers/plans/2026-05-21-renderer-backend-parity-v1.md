# Renderer Backend Parity v1 (2026-05-21)

**Plan ID:** `renderer-backend-parity-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

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

**Status:** Pending.

### Done When

- Manifest, roadmap, and plan registry point `currentActivePlan` at this plan.
- `renderer-debug-profiling-v1` is recorded as completed.

## Validation Evidence

- Phase 0 pending.
