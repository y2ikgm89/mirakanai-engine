# Production Completion v1 - 2D Sprite Production Pipeline Track

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when its scope is needed; do not bulk-read the whole split plan by default.

## 2D Sprite Production Pipeline Track (developer-owned 2D engine capability)

Sprites are a mandatory 2D engine capability and a shared dependency for dense 2D scenes, map-based adventures, projectile-pattern action, UI, effects, and generated placeholder assets. Current engine evidence covers selected 2D package proof, atlas/image foundations, sprite animation counters, and D3D12 primary package lanes, but production 2D games need a stronger sprite pipeline. This track is developer-owned: game-creation agents may generate game-owned sprite documents/assets and use supported package recipes, but sprite renderer/atlas/import/runtime internals are engine-feature work.

Official-practice anchors:

- Unity treats Sprite Atlases as a way to pack many textures into one atlas so renderers can avoid many texture switches/draw overhead; this engine should expose equivalent first-party atlas package rows, atlas/material discipline, and package-visible draw/atlas counters.
- Unity 2D sorting uses sorting layers/order/camera transparency policy; this engine should expose deterministic layer/order/camera-space sorting contracts and diagnostics rather than relying on incidental scene order.
- Unity 9-slicing and tiled sprite draw modes are standard 2D/UI reuse patterns; this engine should support first-party 9-slice/tiled sprite metadata before claiming production UI/world sprite readiness.
- Unreal Paper2D Flipbooks model sprite animation as keyframes with sprite frame and duration; this engine should expose deterministic flipbook/sprite-sheet animation rows with package-visible frame/counter evidence.
- D3D12 sprite batching and atlas use must respect official resource binding concepts: descriptors/tables/heaps/root signatures, resource-state transitions, upload/staging ownership, and no public native handle exposure.

Recommended sprite pipeline streams:

| Stream | Goal | Official-practice evidence gate | Non-goals until separately planned |
| --- | --- | --- | --- |
| `sprite-atlas-authoring-v1` | Make sprite atlas authoring production-ready for game-owned source images, atlas pages, sprite rects, pivots, borders, padding, material/texture dependencies, and replacement flows. | Source image/provenance rows, deterministic packing tests, atlas metadata validation, package registration proof, D3D12 upload evidence, and fallback diagnostics for missing/invalid images. | Arbitrary web asset import, full DCC tool parity, runtime silent atlas generation. |
| `sprite-batching-renderer-v1` | Add high-density sprite batching for world/UI/effects with atlas/material grouping, instance data, draw-call budgets, and package-visible counters. | Draw-call/sprite/instance/atlas/material counters, D3D12 package proof, Vulkan/Metal separate host-gated proofs, framegraph/render-pass evidence, and over-budget diagnostics. | General renderer rewrite, GPU-driven rendering readiness, backend parity without per-host evidence. |
| `sprite-animation-flipbook-v1` | Add deterministic sprite-sheet/flipbook animation contracts: frame ids, durations, events, playback modes, direction sets, and package counters. | Animation document/schema tests, frame timing/repeat diagnostics, generated 2D package evidence, and integration with gameplay state rows. | Full animation graph, skeletal 2D animation, timeline/cinematic tooling. |
| `sprite-sorting-layer-v1` | Add explicit 2D sorting layers, order-in-layer, camera-space/world-space policy, y-sort or custom sort keys, and diagnostics. | Deterministic sorting tests, invalid layer/order diagnostics, package-visible sorted draw counters, and no incidental scene-order claim. | Full editor visual sorting tools, arbitrary renderer overrides. |
| `sprite-9slice-and-tiled-v1` | Add 9-slice/tiled sprite metadata and renderer/UI support for scalable panels, bars, windows, walls, floors, and reusable UI/world sprites. | Border/tile validation, package-visible 9-slice/tiled draw counters, atlas dependency checks, and runtime UI/menu examples. | Full layout engine parity, vector UI, arbitrary shader effects. |
| `sprite-collision-hitbox-v1` | Add sprite-authored collision shapes/hitboxes/hurtboxes tied to frame ids, layers, and gameplay interaction rows. | Hitbox schema tests, frame-bound hitbox validation, deterministic collision/hit counters, and generated projectile-pattern and content-rich adventure examples. | Full physics editor, polygon authoring UX parity, middleware collider exposure. |
| `sprite-effects-particles-v1` | Add first-party sprite effect rows for particles, damage numbers, hit flashes, trails, explosions, and budgeted transient visuals. | Pool/budget counters, package-visible effect rows, D3D12 render evidence, and no unbounded allocation. | Full Niagara/VFX Graph parity, GPU particle simulation, editor effect graph. |
| `sprite-placeholder-generation-v1` | Generate legal placeholder sprites, portraits, icons, tiles, projectiles, enemies, and UI panels with provenance and replacement instructions. | Deterministic generator ids/seeds, license/provenance rows, package proof, replacement workflow docs, and AI mutation-ledger integration. | Photoreal art generation, external asset scraping, untracked AI art provenance. |
| `sprite-editor-preview-diagnostics-v1` | Provide editor/game diagnostics for selected sprites, atlas pages, animation frames, sorting, hitboxes, and package dependencies. | Retained UI rows, package/atlas diagnostics, no renderer native handle exposure, and host-gated preview evidence for non-D3D12 backends. | Full Unity Sprite Editor parity, visual atlas packing UI, DCC integration. |

Archetype coverage gates:

- Dense 2D arena scenarios require sprite batching, pooling/effect counters, atlas discipline, and damage/XP/projectile visual rows before high-density ready claims.
- Content-rich 2D adventure scenarios require tile/character/NPC layering, direction-based walk animations, portraits/icons, dialogue/window 9-slice sprites, and map-transition visuals before polished content-rich adventure readiness claims.
- Projectile-pattern action scenarios require projectile/bullet sprite batching, hitbox-per-frame rows, bullet/effect budget counters, and clear visibility/sorting evidence before bullet-density ready claims.

When this track is selected, create or update a dated sprite capability/milestone plan. Keep game-owned sprite asset generation separate from engine-owned renderer/importer/atlas implementation, update manifest fragments and validation recipes only after evidence exists, and keep unsupported sprite claims explicit for game-creation agents.

Gap closeout requires:

- `production-readiness-audit-check` reports only the active gameplay/physics/navigation/package-evidence rows until they close, and later reports `unsupported_gaps=0`; any selected gap that remains outside the ready surface is explicitly host-gated or excluded with machine-readable notes.
- `engine/agent/manifest.json` `unsupportedProductionGaps` (composed from fragments), this master plan, plan registry, current capability docs, roadmap, skills, and static checks describe the same ready surface.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or a concrete host/tool blocker is recorded in the closing child plan.


Sprite pipeline generic rule: implement sprite features as renderer/asset/runtime primitives. Do not add a sprite feature solely for one archetype unless it generalizes into batching, animation, atlas management, hitbox metadata, effects, UI, tooling, or package validation.
