# Production Completion v1 - Gameplay, Physics, Navigation, And AI Advanced Track

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or reviewing deterministic gameplay, physics, navigation, or AI post-1.0 work.

## Purpose

This chapter is the gameplay projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It keeps deterministic simulation and middleware-boundary rules in one place while the canonical row list stays in `04`.

## Gameplay System Rules

- Fixed-timestep orchestration, explicit input/command rows, stable update order, replayable diagnostics, and package-visible counters are required before readiness claims.
- Variable frame-rate presentation must not drive authoritative simulation state.
- Middleware is optional and opaque. Jolt, PhysX, Recast/Detour, Havok, or similar dependencies require optional-adapter plans, vcpkg feature gating, dependency/legal records, host gates, and first-party public APIs that do not expose middleware handles.
- Navmesh, collision, behavior, perception, and simulation data must be reviewed scene/package assets or first-party runtime rows. Runtime code must not silently import, generate, mutate, or execute broad content outside reviewed surfaces.
- 2D and 3D evidence must be proven separately when a claim affects both lanes.
- Performance and scale claims need budgets for agent counts, body counts, casts, path latency, broadphase cost, behavior ticks, and failure diagnostics.

## Gameplay Projection

| Concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Core gameplay interactions | `engine-gameplay-interaction-framework-v1`, `engine-scene-gameplay-binding-v1` | Reusable interaction/state rows, scene binding diagnostics, 2D/3D package counters, and no game-specific engine shortcuts. |
| Simulation orchestration | `gameplay-simulation-orchestration-v1` | Fixed-step planning, input-command playback, replay diagnostics, and package counters before rollback/network claims. |
| Character and movement physics | `physics-character-dynamics-v1`, `physics-collision-query-v1` | Deterministic movement/query tests, package-visible counters, replay evidence, and first-party public contracts. |
| Constraints, joints, and vehicles | `physics-constraints-and-joints-v1`, `physics-vehicles-and-kinematics-v1` | Stable solver order, bounded iterations, deterministic diagnostics, and explicit non-goals for ragdolls and broad vehicle simulation. |
| Optional physics middleware | `native-physics-middleware-adapter-v1` | Opaque adapter boundary, dependency/legal records, host gates, fallback diagnostics, and no middleware types in public APIs. |
| Navigation and crowds | `navigation-navmesh-v1`, `navigation-crowd-local-avoidance-v1`, `navigation-hierarchical-world-v1` | Reviewed nav assets, deterministic path/crowd tests, dynamic obstacle diagnostics, region/portal evidence, and package counters. |
| AI behavior and perception | `ai-behavior-authoring-v1`, `ai-perception-services-v1` | Behavior asset validation, blackboard schema checks, perception ordering tests, deterministic decision traces, and package-visible counters. |

When a gameplay row is selected, the dated plan must state whether it changes a release-ready definition or remains post-1.0 work, cite official middleware/platform documentation when dependencies are considered, and keep manifest fragments, validation recipes, generated-game guidance, and static checks aligned with the supported surface.
