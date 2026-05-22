# Production Completion v1 - 2D Sprite Production Pipeline Track

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or reviewing sprite-specific 2D engine capability work.

## Purpose

This chapter is the sprite projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It keeps sprite authoring/import/runtime rules together while canonical row ownership stays in `04`.

## Sprite Rules

- Sprite work is engine-owned when it changes atlas, renderer, importer, package, metadata, or runtime primitives. Game-creation agents may generate game-owned sprite documents/assets only through supported surfaces.
- Atlas work must track source image/provenance rows, deterministic packing, package registration, material/texture dependencies, and replacement flows.
- Renderer work must keep native handles private and prove draw-call, sprite, instance, atlas, material, render-pass, and budget counters per backend.
- Animation work must use deterministic frame ids, durations, playback policy, and invalid-frame diagnostics before broad production claims.
- Sorting, 9-slice/tiled drawing, hitboxes, effects, and editor diagnostics are separate rows. Do not hide them inside one generic "sprite ready" claim.
- D3D12 evidence does not imply Vulkan or Metal readiness; promote backend claims separately.

## Sprite Projection

| Sprite concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Atlas authoring and package handoff | `sprite-atlas-authoring-v1`, `engine-asset-placeholder-generation-v1` | Source/provenance rows, packing tests, atlas metadata validation, package registration, and invalid-image diagnostics. |
| High-density batching | `sprite-batching-renderer-v1`, `renderer-scene-scale-v1`, `renderer-gpu-memory-v1` | Draw-call/sprite/instance counters, atlas/material grouping, package proof, and over-budget diagnostics. |
| Flipbook and sprite-sheet animation | `sprite-animation-flipbook-v1` | Animation document tests, frame timing/repeat diagnostics, generated 2D package evidence, and gameplay-state integration rows. |
| Sorting and camera policy | `sprite-sorting-layer-v1` | Layer/order/y-sort/custom-key tests, invalid diagnostics, package-visible sorted draw counters, and no incidental scene-order claim. |
| 9-slice and tiled sprites | `sprite-9slice-and-tiled-v1`, `engine-ui-game-menu-hud-v1` | Border/tile validation, atlas dependency checks, package-visible draw counters, and runtime UI/menu examples. |
| Hitboxes and gameplay collisions | `sprite-collision-hitbox-v1`, `physics-collision-query-v1`, `engine-gameplay-interaction-framework-v1` | Frame-bound hitbox validation, deterministic hit counters, layer policy, and projectile/adventure examples. |
| Particles and transient effects | `sprite-effects-particles-v1` | Pool/budget counters, package-visible effect rows, renderer evidence, and no unbounded allocation. |
| Editor and diagnostics | `sprite-editor-preview-diagnostics-v1`, `renderer-debug-profiling-v1` | Retained UI rows, package/atlas diagnostics, host-gated preview evidence, and no renderer native handle exposure. |

When a sprite row is selected, create or update one dated sprite capability/milestone plan. Keep art/content generation separate from engine-owned renderer/importer/atlas implementation, and keep unsupported sprite claims explicit for game-creation agents.
