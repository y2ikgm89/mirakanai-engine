# Renderer Vulkan Shadow Cascade Package Evidence v1 (2026-05-21)

**Plan ID:** `renderer-vulkan-shadow-cascade-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`.

## Goal

Promote selected Vulkan directional shadow cascade/atlas/light-space package evidence independently from D3D12 without treating Windows D3D12 package smoke as cross-backend proof.

## Context

- `renderer-lighting-shadows-v1` completed with `--require-d3d12-shadow-cascade-policy` and first-party cascade/atlas counters on the selected D3D12 package lane.
- `installed-vulkan-scene-gpu-smoke` already requests directional shadow and filtering; host-owned shadow planning already uses four cascades for Vulkan scene setup.
- Non-goal: new cascade selection shaders, Metal parity, or production shadow quality.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing `SdlDesktopPresentationReport` cascade/atlas counters; do not expose native handles.
- Do not claim D3D12 proof transfers to Vulkan/Metal.

## Phase 1: Vulkan Shadow Cascade Package Flag

**Status:** Completed.

### Done When

- `--require-vulkan-shadow-cascade-policy` exists on `sample_desktop_runtime_game`.
- Smoke exit validates cascade/atlas counters on the selected Vulkan backend.

## Phase 2: Vulkan Package Smoke Promotion

**Status:** Completed.

### Done When

- `installed-vulkan-scene-gpu-smoke` requests `--require-vulkan-shadow-cascade-policy`.
- `tools/validate-installed-desktop-runtime.ps1` validates cascade fields when the flag is set.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `sample_desktop_runtime_game` build | Pass |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Metal shadow cascade package promotion (Apple-host gated).
- Treating D3D12 package smoke as Vulkan/Metal proof.
- Shader-side cascade selection expansion or editor shadow authoring.
