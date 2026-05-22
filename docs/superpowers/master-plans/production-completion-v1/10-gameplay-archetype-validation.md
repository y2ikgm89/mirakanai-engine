# Production Completion v1 - Gameplay Archetype Validation Scenarios

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when its scope is needed; do not bulk-read the whole split plan by default.

## Purpose

This chapter does not define genre-specific engine goals. It defines reusable validation scenarios that prove the engine can support broad gameplay families through generic systems.

Archetypes are coverage probes. They should never force engine APIs to adopt one game's vocabulary. Promote only reusable primitives into the engine; keep one-off rules, balance, content, and presentation in game-owned code/data.

## AI reading rules

- Read this chapter after the parent index and only when broad gameplay coverage is being reviewed.
- Treat each archetype as a stress test for reusable systems, not as a required sample game or product template.
- If an archetype needs a missing reusable capability, create or update the canonical post-1.0 row in `04-developer-owned-engine-capability-backlog.md`.
- If the need is specific to one game concept, keep it under `games/<game_name>/` and do not change engine internals.
- Do not claim an archetype is ready unless the underlying generic capabilities have tests, package evidence, manifest exposure where needed, and validation recipes.

## Generic promotion criteria

A capability may move from game-owned implementation into the engine only when all are true:

- It is reusable across at least two gameplay families.
- It can be described as an engine primitive such as input, simulation, scene, asset, renderer, audio, UI, persistence, tooling, validation, packaging, or AI-operable workflow.
- It has a stable public C++ or data contract.
- It can be validated without relying on one specific game's balance or content.
- It can be exposed safely to AI through `game.agent.json`, manifest fragments, reviewed tools, or validation recipes.

## Archetype coverage table

| Archetype | Generic capability stress | Engine-owned candidates | Game-owned examples |
| --- | --- | --- | --- |
| `dense-2d-arena-archetype` | Entity density, collision/query throughput, spawn/despawn policy, pickups, progression counters, HUD, audio feedback, 2D renderer budgets. | Entity pooling/culling, collision query policy, sprite batching, gameplay counters, runtime HUD primitives, package-visible performance evidence. | Enemy waves, upgrade lists, damage formulas, item drop tables, arena rules, difficulty curves. |
| `content-rich-2d-adventure-archetype` | Map layers, transitions, interaction verbs, dialogue/objectives, inventory/equipment, text-heavy UI, save/load, localization, authored content validation. | Tilemap/scene interaction contracts, quest/objective state, inventory/item documents, save/profile systems, localization tables, menu/text UI models. | Story, NPC lines, quests, item names/stats, encounter design, town/dungeon layouts. |
| `projectile-pattern-action-archetype` | Precise input, projectile schedules, collision layers, scoring events, transient object pooling, VFX/audio feedback, deterministic replay, renderer/effect budgets. | Projectile pattern data contracts, hit layer policy, pooling diagnostics, effect budgets, deterministic event traces, package-visible counters. | Weapon behavior, boss patterns, stage timelines, scoring/combo formulas, enemy formations. |
| `navigation-crowd-archetype` | Pathfinding, dynamic obstacles, local avoidance, agent state, route diagnostics, 2D/3D scene integration. | Navmesh/crowd APIs or adapter boundary, obstacle updates, path query diagnostics, AI movement counters. | Level-specific routes, patrol schedules, encounter placement, NPC personality rules. |
| `physics-movement-archetype` | Character movement, sweeps, triggers, constraints, moving platforms, replayable collisions, controller tuning. | Character controller/query contracts, deterministic physics diagnostics, collision layer schemas, replay evidence. | Movement tuning, player abilities, level hazards, puzzle rules. |
| `systemic-sandbox-archetype` | Placement, persistence, procedural generation, item economy, construction rules, world mutation validation. | Placement/rule contracts, seed-driven generation, inventory/crafting schemas, persistent world-state groups, validation/remediation rows. | Building recipes, biome definitions, economy balance, object catalogs. |

## Ready claim boundary

An archetype is ready only when a generated or sample game package proves the relevant generic capability family end to end. The claim must name the reusable systems proven, validation commands, package counters, and explicit unsupported rows.

Do not write `the engine supports RPGs`, `the engine supports bullet hell`, or similar broad genre claims. Write precise capability claims such as `the engine has package-proven tile interaction, dialogue state, inventory save/load, and text UI evidence for content-rich 2D adventure scenarios`.

## Recommended implementation sequence

1. Close the 1.0 active gameplay/physics/navigation gaps first.
2. Add missing reusable primitives to the canonical backlog in `04-developer-owned-engine-capability-backlog.md` instead of expanding archetype text.
3. For each selected primitive, create a dated capability/gap-cluster plan with tests, package evidence, docs, manifest fragments, and validation recipes.
4. Use one small archetype package as proof after the primitive is implemented.
5. Archive long completed evidence only when it no longer needs to be read for current planning.
