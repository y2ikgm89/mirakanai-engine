# 2026-05-18 Frame Graph v1 Production Ownership Milestone v1

> **For agentic workers:** Active `frame-graph-v1` phase-gated milestone. Keep phases reviewable and evidence-backed. Do not split one-function child plans out of this milestone unless a later phase has a distinct architecture, validation, or review boundary.

**Status:** Active.

## Goal

Close the next coherent `frame-graph-v1` foundation gap by selecting and enforcing production graph ownership boundaries before moving more renderer/runtime work into the executor.

## Context

Recent `frame-graph-v1` slices already moved texture transitions, pass target-state preparation, render pass envelopes, selected renderer pass callbacks, runtime texture upload transitions, runtime mesh/skinned/morph upload command-list execution, and package-visible render-pass/multi-queue evidence into backend-neutral Frame Graph executor contracts.

The remaining risk is overclaiming production graph ownership. Swapchain acquire/present, viewport display/readback, native overlay preparation, package residency, Vulkan/Metal memory aliasing, broad/background package streaming, production multi-queue graph adoption, data preservation, async overlap/performance, public native handles, and broader renderer quality still need explicit boundaries.

## Constraints

- Keep Frame Graph and RHI contracts backend-neutral; do not expose native handles, native queues/fences/semaphores, or backend layout/barrier structs.
- Use clean-break public API names; do not add compatibility aliases.
- Keep `frame-graph-v1` status `implemented-foundation-only` until production graph ownership has external package/renderer evidence beyond a boundary planner.
- Run focused renderer build/test/static loops during implementation, then full `tools/validate.ps1` at the coherent C++/manifest/docs phase gate.
- Update manifest fragments and compose output; do not hand-edit `engine/agent/manifest.json`.

## Phases

- [x] Phase 1: Production graph ownership boundary selection.
  - Add a deterministic fail-closed planner that classifies reviewed capabilities into `frame_graph_owned`, `renderer_owned`, `runtime_host_owned`, `host_gated`, or `unsupported`.
  - Lock the planner with RED/GREEN `MK_renderer_tests`.
  - Keep the planner as boundary evidence only; do not claim renderer migration, broad production graph ownership, async overlap/performance, or broad renderer readiness.
- [x] Phase 2: Runtime material-factor upload Frame Graph command evidence.
  - Select `runtime_upload_commands` from the Phase 1 output and close the remaining material-factor upload exception inside that boundary.
  - Route the material-factor uniform copy in `create_runtime_material_gpu_binding` through `execute_frame_graph_rhi_multi_queue_schedule`.
  - Report one submitted command list, zero queue waits, zero texture barriers, and one pass callback on `RuntimeMaterialGpuBinding`.
  - Keep buffer barriers, staging rings, native async upload execution, production graph ownership, async overlap/performance, public native handles, and broad renderer readiness unsupported.
- [ ] Phase 3: Close or reclassify the milestone with manifest/docs/static guards and package-visible evidence.

## Done When

- Phase 1 and Phase 2 APIs and tests pass.
- `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, renderer guidance, manifest fragments, composed manifest, and static integration checks describe the new boundary planner and material-factor upload evidence without broadening ready claims.
- Focused renderer validation and full validation evidence are recorded in this plan.

## Validation Evidence

- 2026-05-18 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed because `FrameGraphProductionOwnershipCandidate`, `FrameGraphProductionOwnershipCapability`, `FrameGraphProductionOwnershipBoundary`, and `plan_frame_graph_production_ownership_boundary` were not yet defined.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed.
- 2026-05-18 AGENT SYNC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/static guard synchronization.
- 2026-05-18 REVIEW: read-only `cpp-reviewer` found missing coverage for `runtime_host_owned`, `host_gated`, empty ids, duplicate ids, and invalid capabilities. Tests were extended to cover those cases, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed again.
- 2026-05-18 PHASE GATE: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the review fixes.
- 2026-05-18 OFFICIAL PRACTICE CHECK: Microsoft Direct3D 12 documentation for command-list execution/synchronization and resource barriers confirms that command lists record GPU work, submitted work is synchronized through fences/queue waits, and explicit resource-state barriers are the D3D12 promotion path when resource states change. Phase 2 reuses the existing backend-neutral Frame Graph multi-queue executor for one buffer copy and does not add new D3D12/Vulkan/Metal backend behavior or backend ready claims. Source: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists> and <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>.
- 2026-05-18 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed because `RuntimeMaterialGpuBinding` did not yet expose `frame_graph_command_lists_submitted`, `frame_graph_queue_waits_recorded`, `frame_graph_barriers_recorded`, or `frame_graph_pass_callbacks_invoked`.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` passed.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_tests --output-on-failure` passed.
- 2026-05-18 PHASE 2 AGENT SYNC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after material-factor manifest/static guard synchronization.
- 2026-05-18 PHASE 2 FULL GATE: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after Phase 2 code, docs, manifest, static guard, and rendering guidance updates.
