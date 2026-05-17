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
- [ ] Phase 2: Select the next implementation boundary from the Phase 1 output, likely package-visible production graph ownership beyond runtime uploads, broad/background package streaming integration, Vulkan/Metal memory alias allocation, or production multi-queue graph adoption.
- [ ] Phase 3: Close or reclassify the milestone with manifest/docs/static guards and package-visible evidence.

## Done When

- Phase 1 API and tests pass.
- `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, renderer guidance, manifest fragments, composed manifest, and static integration checks describe the new boundary planner without broadening ready claims.
- Focused renderer validation and full validation evidence are recorded in this plan.

## Validation Evidence

- 2026-05-18 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed because `FrameGraphProductionOwnershipCandidate`, `FrameGraphProductionOwnershipCapability`, `FrameGraphProductionOwnershipBoundary`, and `plan_frame_graph_production_ownership_boundary` were not yet defined.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed.
- 2026-05-18 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed.
- 2026-05-18 AGENT SYNC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/static guard synchronization.
- 2026-05-18 REVIEW: read-only `cpp-reviewer` found missing coverage for `runtime_host_owned`, `host_gated`, empty ids, duplicate ids, and invalid capabilities. Tests were extended to cover those cases, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed again.
- 2026-05-18 PHASE GATE: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the review fixes.
