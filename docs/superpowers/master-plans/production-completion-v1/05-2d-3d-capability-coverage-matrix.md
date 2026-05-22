# Production Completion v1 - 2D / 3D Capability Coverage Matrix

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter when reviewing whether a 2D or 3D game request maps to the current ready surface or to the canonical post-1.0 backlog.

## Purpose

This chapter is a projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It does not own capability rows. Add or change capability ids in `04`, then update this matrix only when the 2D/3D coverage view changes.

Coverage statuses:

| Status | Meaning |
| --- | --- |
| `covered-foundation` | Current engine evidence supports a narrow, validated use. |
| `post-1x-row` | Use the named canonical backlog row before broad ready claims. |
| `host-gated` | Requires backend, platform, SDK, device, or toolchain evidence. |
| `game-owned` | Belongs in `games/<game_name>/` unless it becomes a reusable engine primitive. |

## 2D Coverage Projection

| Capability area | Current boundary | Canonical rows to select when broader support is needed |
| --- | --- | --- |
| 2D scaffold/package | `covered-foundation`: generated 2D package recipes create, validate, and package game-owned files without engine edits. | `ai-game-generation-orchestrator-v1`, `ai-generated-game-quality-rubric-v1` |
| Sprite atlas, animation, and batching | `covered-foundation`: selected package proof, atlas/image foundations, sprite animation counters, and native sprite execution counters exist. | `sprite-atlas-authoring-v1`, `sprite-batching-renderer-v1`, `sprite-animation-flipbook-v1`, `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, `sprite-effects-particles-v1` |
| Tilemaps and 2D levels | `covered-foundation`: tilemap metadata and package counters exist for the supported generated path. | `engine-scene-gameplay-binding-v1`, `engine-gameplay-interaction-framework-v1`, `world-streaming-and-large-scenes-v1` |
| 2D collision, hitboxes, and triggers | `post-1x-row`: physics/query foundations exist, but frame-authored hitbox families need a selected row. | `sprite-collision-hitbox-v1`, `physics-collision-query-v1`, `engine-gameplay-interaction-framework-v1` |
| HUD, menus, dialogue, and rebinding | `covered-foundation`: runtime UI and input context foundations exist, but polished game menus remain a selected 1.x feature. | `engine-ui-game-menu-hud-v1`, `engine-input-action-contexts-v1`, `engine-quest-dialogue-state-v1` |
| Audio, feedback, and SFX | `covered-foundation`: gameplay audio mix planning exists without broad codec claims. | `engine-audio-gameplay-mixer-v1`, `sprite-effects-particles-v1` |
| Save, progression, and reset | `post-1x-row`: session/profile primitives exist, but richer generated-game save slots and migration evidence need a selected row. | `engine-save-settings-profile-v1`, `simulation-persistence-v1` |
| Dense 2D arena or projectile games | `post-1x-row`: use archetypes only as evidence probes. | `engine-entity-scale-and-culling-v1`, `sprite-batching-renderer-v1`, `sprite-collision-hitbox-v1`, `gameplay-simulation-orchestration-v1` |

## 3D Coverage Projection

| Capability area | Current boundary | Canonical rows to select when broader support is needed |
| --- | --- | --- |
| 3D scaffold/package | `covered-foundation`: generated 3D package recipes create, validate, and package scene/mesh/material/game-owned files. | `ai-game-generation-orchestrator-v1`, `ai-generated-game-quality-rubric-v1` |
| Mesh, material, and shader package path | `covered-foundation`: current proof is package-scoped. | `renderer-modern-materials-v1`, `material-shader-graph-production-v1`, `renderer-backend-parity-v1` |
| Lighting, shadows, and postprocess | `covered-foundation` on selected D3D12/Vulkan lanes; `host-gated` for broader backend claims. | `renderer-lighting-shadows-v1`, `renderer-postprocess-v1`, `renderer-backend-parity-v1` |
| 3D camera, controller, and movement | `covered-foundation`: movement, controller, and policy counters exist for selected scenarios. | `physics-character-dynamics-v1`, `engine-input-action-contexts-v1`, `engine-scene-gameplay-binding-v1` |
| 3D physics and collision queries | `covered-foundation`: deterministic query, joint, constraint, kinematic, and simple vehicle policy foundations exist. | `physics-collision-query-v1`, `physics-constraints-and-joints-v1`, `physics-vehicles-and-kinematics-v1`, `native-physics-middleware-adapter-v1` |
| 3D navigation and AI movement | `covered-foundation`: navmesh/crowd foundations exist with selected package evidence. | `navigation-navmesh-v1`, `navigation-crowd-local-avoidance-v1`, `navigation-hierarchical-world-v1`, `ai-behavior-authoring-v1`, `ai-perception-services-v1` |
| Animation, skeletal, and morph basics | `covered-foundation`: package/proof-scoped animation, skinning, morph, IK, and root-motion foundations exist. | `renderer-backend-parity-v1`, `material-shader-graph-production-v1`, `engine-scene-gameplay-binding-v1` |
| Open-ended 3D worlds | `post-1x-row`: no open-world claim from single-scene proof. | `engine-world-region-streaming-v1`, `engine-entity-scale-and-culling-v1`, `world-streaming-and-large-scenes-v1`, `simulation-persistence-v1`, `navigation-hierarchical-world-v1` |
| Multiplayer or networked 3D | `post-1x-row`: local gameplay readiness is not network readiness. | `engine-networking-foundation-v1`, `networking-and-multiplayer-v1`, `gameplay-simulation-orchestration-v1`, `simulation-persistence-v1` |

## Editing Rule

This matrix should stay short. If a future review finds a missing reusable engine capability, add or update one row in `04-developer-owned-engine-capability-backlog.md`, then point this matrix at that id. Do not add genre-specific backlog tables here.
