# Gameplay Physics / AI Navigation / 2D-3D Essential Systems Expansion Master v1 (2026-05-19)

**Plan ID:** `gameplay-physics-navigation-ai-foundation-v1`
**Status:** Completed.
**Current pointer rule:** Set machine-readable progress through `engine/agent/manifest.json.aiOperableProductionLoop` (`currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps`). Keep `currentActivePlan` aligned with this plan while this roadmap is active.

## Goal

Close the next production gap cluster for essential gameplay systems so generated 2D/3D titles can use a coherent physics + nav + AI loop through reviewed headless and package paths without middleware dependencies.

## Context

- Recent closeouts proved individual gameplay foundations (`MK_physics`, `MK_navigation`, `MK_ai`) in isolation and selected headless/package smokes.
- Scene/physics binding, robust movement/control semantics, navmesh/crowd production paths, and 3D/2D integrated gameplay loop determinism remain explicit follow-up work.
- The next cluster is intentionally constrained: gameplay simulation integration, movement correctness, and execution evidence in generated-game package flows.

## Constraints

- Preserve engine clean-break policy and architecture boundaries (`engine/core` independent from OS/GPU/asset/editor).
- Avoid third-party runtime middleware integration at this stage; keep native backends and native handles out of gameplay public APIs.
- No broad quality claims (renderer parity, full streaming, async services) without proof rows.
- Use focused slices and avoid tiny single-function plans.

## Scope

This plan clusters four gaps required for shipped gameplay loops:

1. `gameplay-runtime-integration-foundation` (foundation-follow-up)
2. `navigation-navmesh-and-dynamic-obstacle-follow-up` (foundation-follow-up)
3. `physics-advanced-dynamics-follow-up` (foundation-follow-up)
4. `gameplay-2d-3d-package-evidence` (package-evidence)

A future sibling or child plan should own long-tail AI authoring UX and full editor-level runtime diagnostics after these slices close.

## Phase 1: Gameplay Runtime Integration Foundation

### Goal

Define a deterministic gameplay simulation slice that composes physics, navigation, and AI-perception/blackboard/behavior updates on a stable tick order and command surface.

### Context

- Current public APIs expose pieces of each system.
- `sample_gameplay_foundation` now has a deterministic scene-authoritative gameplay-tick contract (`physics.apply_force`, `physics.step`, `physics.resolve_contacts`, `animation.update`, `navigation_and_ai.update`) captured in output and gate checks, so ordering proof is implemented and repeatable in headless execution.

### Constraints

- Keep API value-oriented and scene-authoring/review contracts non-middleware.
- No middleware adapters, no public native handles, no runtime module embedding in editor.

### Done When

- A first-party API lane demonstrates `PhysicsWorld3D/2D` stepping, `NavigationGridAgentState` updates, and AI blackboard-driven behavior ticks in one reviewed flow.
- Unsupported behavior (scene-physics coupling assumptions, deterministic step ordering, failure diagnostics) is documented as boundary rows.
- Phase 1 is complete for this plan: reproducible integration order is now instrumented in `sample_gameplay_foundation` with deterministic tick-order proof output fields and a stable-order gate.

## Phase 2: Navigation + Physics + AI Closeout for 2D/3D Movement

### Goal

Add missing production foundations for traversal and obstacle behavior beyond the current grid-only pathfinding baseline.

### Context

- Explicit exclusions in current roadmap include navmesh assets, crowd simulation, and broad scene integration.
- Gameplay loops now depend on path replacement and avoidance behavior in both 2D and 3D scenes.

### Constraints

- Keep navmesh introduction first-party and deterministic; avoid engine middleware commitments.
- No global scene graph or world-stepping bypass of reviewed command/surface ownership.

### Done When

- Deterministic scene-referenced movement obstacles can participate in path planning and replanning.
- Local avoidance and movement completion/abort transitions are proven with reviewed diagnostic coverage.
- No external pathing middleware appears in public gameplay contracts.
- Phase 2 is complete for this plan: `NavigationNavmeshPathRequest` / `plan_navigation_navmesh_path` provide deterministic scene-ref polygon routing with dynamic obstacle evidence, and `sample_gameplay_foundation` now gates navmesh, grid replan, local avoidance, and physics movement-policy counters.

## Phase 3: Package-evidence and production slice proof

### Goal

Prove selected generated 2D and 3D package workflows execute an integrated gameplay loop through reviewed diagnostics.

### Context

- Existing package proofs cover individual systems but not full runtime gameplay composition.
- This phase moves from foundation slices to runtime-package confidence.

### Constraints

- Keep package evidence tied to reviewed desktop package recipes.
- Avoid broad renderer- or platform-parity claims in this phase.

### Done When

- Selected sample package recipes report the new integration counters/rows from physics+navigation+AI composition.
- The 2D and 3D gameplay sample loop path remains headless-testable and deterministic.
- Host-gated items remain explicitly blocked until validated in this same manifest-backed plan.
- Phase 3 is complete for this plan: `sample_2d_desktop_runtime_package` and generated `DesktopRuntime2DPackage` expose `--require-gameplay-systems` counters for 2D physics/navigation/AI, and `sample_generated_desktop_runtime_3d_package` plus generated `DesktopRuntime3DPackage` expose package-visible navmesh dynamic-obstacle, local-avoidance, and physics movement-policy counters.

## Initial Unsupported Gaps for this Plan

- `gameplay-runtime-integration-foundation`
- `navigation-navmesh-and-dynamic-obstacle-follow-up`
- `physics-advanced-dynamics-follow-up`
- `gameplay-2d-3d-package-evidence`

These row IDs were recorded in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` while this plan was active.
The full gap cluster is now closed for the Engine 1.0 Windows-default ready surface: `gameplay-runtime-integration-foundation`, `navigation-navmesh-and-dynamic-obstacle-follow-up`, `physics-advanced-dynamics-follow-up`, and `gameplay-2d-3d-package-evidence` have focused evidence, and the composed manifest returns to the master plan with `unsupportedProductionGaps` empty.
