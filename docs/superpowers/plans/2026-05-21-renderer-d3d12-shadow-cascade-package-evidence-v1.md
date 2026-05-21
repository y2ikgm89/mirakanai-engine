# Renderer D3D12 Shadow Cascade Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-d3d12-shadow-cascade-package-evidence-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected D3D12 shadow cascade package evidence independently from Vulkan without treating Vulkan package smoke as cross-backend proof.

## Context

- `renderer-vulkan-shadow-cascade-v1` completed with `--require-vulkan-shadow-cascade-policy` and `vulkan_shadow_cascade_policy_ready/selected`.
- D3D12 already uses `--require-d3d12-shadow-cascade-policy` with cascade/atlas counters but lacks per-backend ready/selected fields.
- Non-goal: new cascade shaders, Metal parity, or production shadow quality.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing cascade/atlas counters; do not expose native handles.
- Do not claim Vulkan proof transfers to D3D12.

## Phase 1: D3D12 Shadow Cascade Package Fields

**Status:** In progress.

### Done When

- `d3d12_shadow_cascade_policy_ready/selected` exist on smoke stdout when `--require-d3d12-shadow-cascade-policy` is set.
- Smoke exit uses a D3D12-selected backend gate.

## Phase 2: Package Smoke And Validation

**Status:** Completed.

### Done When

- `installed-d3d12-scene-gpu-smoke` already requests `--require-d3d12-shadow-cascade-policy`.
- `tools/validate-installed-desktop-runtime.ps1` validates D3D12 policy fields when the flag is set.
- Agent surfaces stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal shadow cascade package promotion (Apple-host gated).
- Treating Vulkan package smoke as D3D12 proof.
