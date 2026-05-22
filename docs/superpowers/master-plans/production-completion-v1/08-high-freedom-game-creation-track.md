# Production Completion v1 - General-Purpose / High-Freedom Game Creation Track

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or reviewing sandbox, persistence, procedural, scripting, multiplayer, or broad AI game-creation work.

## Purpose

This chapter is the high-freedom game creation projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It describes how to combine canonical rows for broader game shapes without creating a second backlog.

## High-Freedom Rules

- Data-driven systems must be reviewed. Quests, dialogue, inventory, interaction rules, world regions, saves, procedural seeds, and scripts need first-party documents or reviewed command rows with validation.
- Game-creation agents stay game-surface scoped. Missing engine capability becomes a typed handoff to a developer-owned row in `04`.
- Persistent entities, regions, quests, inventory items, dialogue flags, generated chunks, and asset references need stable identity, versioning, migration diagnostics, and save/load validation before ready claims.
- Large maps require region/chunk ownership, streaming boundaries, resource budgets, nav/physics partitioning, LOD/culling evidence, and missing-region diagnostics.
- Scripting is optional and sandboxed. It requires dependency/legal records, capability restrictions, execution budgets, and replay/validation evidence.
- Procedural generation must be reproducible through explicit seeds, inputs, generator ids, deterministic output summaries, and remediation paths.

## High-Freedom Projection

| Game shape or workflow | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Content-rich adventure | `engine-quest-dialogue-state-v1`, `engine-inventory-items-crafting-v1`, `engine-save-settings-profile-v1`, `engine-ui-game-menu-hud-v1` | Graph/schema tests, localization key checks, save/load evidence, and package counters. |
| Sandbox construction/crafting | `engine-construction-placement-v1`, `engine-inventory-items-crafting-v1`, `engine-procedural-generation-v1`, `simulation-persistence-v1` | Placement validation, physics/nav update diagnostics, persistence tests, and package evidence. |
| Large worlds | `engine-world-region-streaming-v1`, `world-streaming-and-large-scenes-v1`, `engine-entity-scale-and-culling-v1`, `navigation-hierarchical-world-v1` | Region packages, safe-point load/unload, resource budgets, nav/physics partitioning, and explicit host-gated streaming claims. |
| Procedural worlds and encounters | `procedural-content-generation-v1`, `engine-procedural-generation-v1`, `ai-gameplay-authoring-tools-v1` | Generator ids, seeds, deterministic output hashes/summaries, validation of generated scenes/assets, and quality-rubric rows. |
| AI-authored games | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-safe-content-mutation-ledger-v1`, `ai-generated-game-playtest-loop-v1`, `ai-generated-game-quality-rubric-v1`, `ai-engine-capability-handoff-v1` | Design-spec to game-owned content flow, mutation ledger, validation recipes, remediation rows, and no engine-internal edits. |
| Scripting and modding | `engine-scripting-sandbox-v1`, `scripting-and-mod-sandbox-v1` | Sandbox capability tests, dependency/legal updates, execution budgets, package counters, and no filesystem/network/process access by default. |
| Multiplayer | `engine-networking-foundation-v1`, `networking-and-multiplayer-v1`, `gameplay-simulation-orchestration-v1`, `simulation-persistence-v1` | Separate architecture/security plan, threat model, replay/determinism proof, transport/session gates, and no broad multiplayer claim from local gameplay. |

When a high-freedom wave is selected, prefer one dated milestone plan that states the target freedom level, supported gameplay families, AI-operable authoring surface, persistence model, validation recipes, and explicit non-goals.
