# Production Completion v1 - 2D / 3D Capability Coverage Matrix

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when its scope is needed; do not bulk-read the whole split plan by default.

## 2D / 3D Core and Advanced Capability Coverage Matrix

This matrix records whether the master plan covers the major capabilities expected from practical 2D and 3D game engines. It is an index over existing tracks, not a second backlog. Add new capability rows only when no existing track owns the gap.

Official comparison anchors: Unity documents [2D development](https://docs.unity.cn/Manual/Unity2D.html) around sprites, sprite atlases, tilemaps, SpriteShape, 2D sorting, animation, and 2D physics; Unreal Engine documents [World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine), [Behavior Trees](https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-trees-in-unreal-engine), [Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/input-overview-in-unreal-engine), UMG, animation, rendering, Niagara, and save-game workflows. GameEngine should not copy Unity/Unreal breadth, but its plans must cover equivalent **first-party, AI-operable contracts** for the supported 2D/3D scope.

Coverage statuses:

| Status | Meaning |
| --- | --- |
| `covered-foundation` | Current engine/plan already has a minimum supported or active closeout path. |
| `active-gap` | Current 1.0 gap cluster must close before zero-gap ready claims. No current row uses this while `unsupportedProductionGaps = []`. |
| `developer-backlog` | Game-creation agents may request this through handoff; developer implements separately. |
| `advanced-track` | Post-1.0 / 1.x high-functionality stream. |
| `host-gated` | Requires backend/platform host evidence before ready claim. |

### 2D core coverage

| Capability | Coverage status | Owning track / row | Ready boundary |
| --- | --- | --- | --- |
| 2D game scaffold/package | `covered-foundation` | `2d-playable-vertical-slice`; `ai-game-generation-orchestrator-v1` | Generated 2D package recipe creates, validates, and packages game-owned files without engine edits. |
| Sprites / atlas / animation | `developer-backlog` + `advanced-track` | `2D Sprite Production Pipeline Track`; `sprite-atlas-authoring-v1`; `sprite-batching-renderer-v1`; `sprite-animation-flipbook-v1` | Production claim requires atlas rows, deterministic flipbooks, batching counters, and package evidence. |
| Tilemaps / 2D levels | `covered-foundation` + `developer-backlog` | `tilemap-metadata-authoring-tooling-v1`; `content-rich-2d-adventure-archetype` | Map-based interaction claim requires walkability/interaction/collision layers and transition diagnostics. |
| 2D camera / bounds / screen-space policy | `developer-backlog` | `engine-scene-gameplay-binding-v1`; `projectile-pattern-action-archetype` | Camera bounds, spawn-safe regions, y/screen sorting, and despawn rules need package counters. |
| Input / rebinding / UI capture | `covered-foundation` + `developer-backlog` | `engine-input-action-contexts-v1`; `gameplay-simulation-orchestration-v1` | Runtime input context stack planning is implemented; broader package/UI capture/rebinding flows remain follow-up evidence. |
| 2D collision / hitboxes / triggers | `developer-backlog` | `sprite-collision-hitbox-v1`; `physics-collision-query-v1`; `engine-gameplay-interaction-framework-v1` | Frame/layer hitbox rows, deterministic query ordering, and package counters before broad gameplay-family readiness claims. |
| HUD / menus / runtime UI | `covered-foundation` + `developer-backlog` | `engine-ui-game-menu-hud-v1`; `sprite-9slice-and-tiled-v1` | Runtime menu/HUD intent rows are implemented; richer dialogue/9-slice/package UX remains follow-up evidence. |
| Audio / music / SFX | `covered-foundation` + `developer-backlog` | `engine-audio-gameplay-mixer-v1` | Audio Gameplay Mix Planner v1 is implemented for cue/bus/trigger planning; SDL3/audio device ownership remains behind adapters and broader package counters remain future evidence. |
| Save/settings/progression | `covered-foundation` + `developer-backlog` | `engine-save-settings-profile-v1`; `simulation-persistence-v1` | Runtime session profile path/document bundle primitives are implemented; richer save slots/progression/package resume evidence remains follow-up work. |
| AI-generated game workflow | `advanced-track` | `AI Autonomous Game Creation Track` | AI writes game-owned files only, uses design spec/mutation ledger/remediation recipes, and handoffs missing engine features. |

### 2D advanced coverage

| Capability | Coverage status | Owning track / row | Ready boundary |
| --- | --- | --- | --- |
| High-density sprites / bullets / effects | `advanced-track` | `sprite-batching-renderer-v1`; `sprite-effects-particles-v1`; `projectile-pattern-action-archetype` | Requires pooling, draw-call/instance/atlas counters, frame budget diagnostics, and backend-specific renderer evidence. |
| dense 2D arena arena systems | `advanced-track` | `dense-2d-arena-archetype` | Complete only after waves, upgrades, pickups, HUD, audio, save/progression, and performance budgets are package-proven. |
| content-rich 2D adventure content systems | `advanced-track` | `content-rich-2d-adventure-archetype`; `engine-quest-dialogue-state-v1`; `engine-inventory-items-crafting-v1` | Complete only after maps, dialogue, quests, inventory/equipment, battle/non-combat loop, localization, save/load evidence. |
| Projectile-pattern action systems | `advanced-track` | `projectile-pattern-action-archetype` | Complete only after pattern documents, hit layers, pooling, score/combo, VFX/audio feedback, and package evidence. |
| Scripting/mod-like 2D gameplay | `optional-adapter` | `scripting-and-mod-sandbox-v1`; `engine-scripting-sandbox-v1` | Optional only; requires dependency/legal records, sandbox capability tests, deterministic replay, and no filesystem/network/process access by default. |
| Large 2D worlds / streaming | `scale-enabler` | `world-streaming-and-large-scenes-v1`; `engine-world-region-streaming-v1` | Region/chunk packages, safe load/unload, nav/physics/resource partitioning, and package counters required. |

### 3D core coverage

| Capability | Coverage status | Owning track / row | Ready boundary |
| --- | --- | --- | --- |
| 3D game scaffold/package | `covered-foundation` | `3d-playable-vertical-slice`; `ai-game-generation-orchestrator-v1` | Generated 3D package recipe creates, validates, and packages scene/mesh/material/game-owned files without engine edits. |
| Mesh/material/shader package path | `covered-foundation` + `advanced-track` | `3d-playable-vertical-slice`; `renderer-modern-materials-v1` | Current 3D proof is narrow; advanced material/PBR/IBL/shader graph claims require backend-specific evidence. |
| Lighting/shadows/postprocess | `covered-foundation` + `advanced-track` | `renderer-lighting-shadows-v1`; `renderer-postprocess-v1` | D3D12 package proof is primary; Vulkan/Metal must prove separately through host gates. |
| 3D camera/controller/movement | `covered-foundation` + `advanced-track` | `gameplay-physics-navigation-ai-foundation-v1`; `engine-advanced-physics-controller-v1`; `physics-character-dynamics-v1`; `engine-input-action-contexts-v1` | Advanced controller foundations exist; broader character/controller policy, camera/input policy, and richer package counters remain post-1.0 follow-up evidence. |
| 3D physics/collision queries | `covered-foundation` + `advanced-track` | `physics-collision-query-v1`; `physics-joints-foundation-v1`; `physics-constraints-and-joints-v1`; `physics-vehicles-and-kinematics-v1` | Deterministic query and distance-joint foundations exist; constraints, vehicles, and richer body interaction remain post-1.0 follow-up work. |
| 3D navigation / AI movement | `covered-foundation` + `advanced-track` | `gameplay-physics-navigation-ai-foundation-v1`; `engine-navmesh-crowd-v1`; `navigation-hierarchical-world-v1` | Navmesh/crowd foundations exist; region/portal hierarchy, streaming-safe nav references, and larger-world package evidence remain post-1.0 follow-up work. |
| Animation / skeletal / morph basics | `covered-foundation` + `advanced-track` | existing animation/gltf/skinning/morph closeout evidence; `renderer-modern-materials-v1`; `gameplay-systems-framework-v1` | Broad animation graph/retarget/cinematics remain future; current claim is package/proof-scoped. |
| Runtime UI/audio/save | `covered-foundation` + `developer-backlog` | `engine-ui-game-menu-hud-v1`; `engine-audio-gameplay-mixer-v1`; `engine-save-settings-profile-v1` | Shared game-owned UI/audio/save foundations are implemented; richer 3D package-specific evidence remains follow-up work. |

### 3D advanced coverage

| Capability | Coverage status | Owning track / row | Ready boundary |
| --- | --- | --- | --- |
| Renderer quality / scale | `advanced-track` | `Renderer Advanced Production Track`; `renderer-scene-scale-v1`; `renderer-gpu-memory-v1`; `renderer-debug-profiling-v1` | Requires draw/instance/LOD/culling/GPU memory/timestamp evidence and backend-specific gates. |
| Open-ended 3D worlds | `scale-enabler` | `world-streaming-and-large-scenes-v1`; `engine-world-region-streaming-v1`; `engine-entity-scale-and-culling-v1` | Region streaming, culling/LOD, nav/physics partitioning, and package evidence required; no open-world claim from single-scene proof. |
| Advanced 3D physics | `advanced-track` | `physics-constraints-and-joints-v1`; `physics-vehicles-and-kinematics-v1`; optional middleware adapter plans | Constraints/vehicles/ragdoll-like systems need first-party contracts or optional opaque adapters, deterministic tests, and legal/dependency updates. |
| Advanced AI/NPC behavior | `advanced-track` | `ai-behavior-authoring-v1`; `ai-perception-services-v1`; `gameplay-systems-framework-v1` | Behavior assets, blackboard/perception traces, and package-visible decision counters required. |
| Effects / particles / feedback | `advanced-track` | `renderer-postprocess-v1`; `sprite-effects-particles-v1` for 2D-style effects; future 3D VFX rows if selected | Budgeted effect rows and renderer evidence required; no Niagara/VFX Graph parity claim. |
| Multiplayer/networking | `optional-adapter` | `networking-and-multiplayer-v1`; `engine-networking-foundation-v1` | Separate architecture/security plan, replay/determinism prerequisites, and host/network gates required. |

Coverage verdict: the master plan covers the expected 2D and 3D **foundations and advanced directions**, and the composed manifest currently keeps `unsupportedProductionGaps = []`. `post-1-0-capability-program-v1` is the active milestone for selected post-1.0 / 1.x implementation, starting with `physics-constraints-and-joints-v1`. Broader Unity/Unreal-like 2D/3D functionality remains tracked as developer-owned or post-1.0 / 1.x work so game-creation agents do not mistake planned features for supported engine behavior.

When future reviews find a missing 2D/3D capability, first map it to this matrix. If an existing row owns it, update that row's dated plan when selected; if no row owns it, add one developer-owned capability row and one AI-operable evidence gate rather than scattering duplicate archetype-specific requirements.


Generic coverage rule: every row should name a reusable engine capability, not a single game genre. Archetype references are evidence probes only; the owning implementation remains an engine primitive or a game-owned sample.
