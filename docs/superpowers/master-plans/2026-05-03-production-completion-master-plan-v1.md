# Production Completion Master Plan v1 (2026-05-03)

Plan ID: `production-completion-master-plan-v1`
Status: Active lightweight index.
Detailed split index: [production-completion-v1/README.md](production-completion-v1/README.md)
Active execution pointer: `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Purpose

This file is the master plan index. It intentionally stays small so Codex, Claude Code, Cursor, and other agents can find the right production-completion chapter without spending context on the full historical plan.

Use the split chapter that matches the current decision. Do not bulk-read every chapter unless the task requires a full production readiness audit.

## Current Verdict

- 1.0 closeout readiness remains manifest-led: every `unsupportedProductionGaps` row in `engine/agent/manifest.json.aiOperableProductionLoop` must be implemented, host-gated with evidence, or explicitly excluded with evidence before a ready claim; the current composed manifest has no remaining rows.
- Current execution remains manifest-led. Do not hand-edit `engine/agent/manifest.json`; edit `engine/agent/manifest.fragments/*.json` and run the compose script when manifest state changes.
- Current active plan: `docs/superpowers/plans/2026-05-21-engine-procedural-generation-v1.md`.
- Current manifest state: `unsupportedProductionGaps = []`; `recommendedNextPlan` points at `engine-procedural-generation-v1` as the current developer-owned gameplay-family-enabler capability, not a reopened Engine 1.0 production gap. `renderer-scene-scale-v1` is completed for backend-neutral scene-scale policy diagnostics, package-visible scene-scale counters, selected D3D12 instanced draw execution evidence, PR #159, hosted checks, and full validation evidence.
- Current gap cluster: none selected after the 2026-05-19 gameplay/physics/navigation/AI foundation closeout.

## Plan map

| Need | Read |
| --- | --- |
| Start here / choose a chapter | [production-completion-v1/README.md](production-completion-v1/README.md) |
| 1.0 readiness ledger, Current Verdict, user-scenario mapping | [01-one-dot-zero-readiness-ledger.md](production-completion-v1/01-one-dot-zero-readiness-ledger.md) |
| Official Practice Review Gates and official documentation anchors | [02-official-practice-gates.md](production-completion-v1/02-official-practice-gates.md) |
| AI Autonomous Game Creation Track for Codex / Claude Code / Cursor | [03-ai-autonomous-game-creation.md](production-completion-v1/03-ai-autonomous-game-creation.md) |
| Developer-Owned Engine Capability Backlog for missing engine features | [04-developer-owned-engine-capability-backlog.md](production-completion-v1/04-developer-owned-engine-capability-backlog.md) |
| 2D / 3D Core and Advanced Capability Coverage Matrix | [05-2d-3d-capability-coverage-matrix.md](production-completion-v1/05-2d-3d-capability-coverage-matrix.md) |
| Renderer Advanced Production Track | [06-renderer-advanced-production-track.md](production-completion-v1/06-renderer-advanced-production-track.md) |
| Physics / Navigation / AI Advanced Gameplay Track | [07-gameplay-physics-nav-ai-advanced-track.md](production-completion-v1/07-gameplay-physics-nav-ai-advanced-track.md) |
| General-Purpose / High-Freedom Game Creation Track | [08-high-freedom-game-creation-track.md](production-completion-v1/08-high-freedom-game-creation-track.md) |
| 2D Sprite Production Pipeline Track | [09-sprite-production-pipeline-track.md](production-completion-v1/09-sprite-production-pipeline-track.md) |
| Gameplay archetype validation scenarios | [10-gameplay-archetype-validation.md](production-completion-v1/10-gameplay-archetype-validation.md) |
| Historical verdict archive and retained static-check evidence | [99-historical-verdict-archive.md](production-completion-v1/99-historical-verdict-archive.md) |

## Official implementation rule

Prefer official documentation and SDK guidance first: CMake, vcpkg, SDL3, Direct3D 12, Vulkan, Metal, Android GameActivity, OpenAI Codex docs, Anthropic Claude Code docs, and vendor engine references where explicitly cited in the chapter.

This is a greenfield engine. Use clean breaking changes when they remove duplicated contracts, stale compatibility layers, or ambiguous API ownership.

## AI readability contract

- This plan describes a general-purpose game engine, not a genre-specific engine.
- Read the parent index first, then load only the chapter that owns the current decision.
- Treat gameplay archetypes as validation scenarios, not product templates or mandatory engine feature bundles.
- Prefer reusable engine primitives: input, simulation, scene, assets, rendering, audio, UI, persistence, networking, tooling, validation, and packaging.
- Keep game-specific rules, balance, content, and presentation in game-owned files unless the same capability is reusable across multiple gameplay families.
- Missing reusable engine capability becomes a developer-owned backlog row; game-creation agents must not patch engine internals.

## Generic engine decision rule

Promote a feature into the engine only when it is reusable across at least two gameplay families, has a stable public contract, can be validated without one specific game, and can be exposed safely to AI through manifests or reviewed tools. Otherwise keep it in sample/game code.
## AI game-creation boundary

Game-creation agents may generate and edit game-owned files, code, and assets under `games/<game_name>/`. They must not mutate engine internals, editor internals, shared tools, schemas, agent policy, CMake/vcpkg contracts, or validation policy while acting as game creators.

When a generated game needs an engine capability that is missing, record a developer-owned handoff in the capability backlog instead of patching engine internals from the game-creation flow.

## Maintenance contract

- Keep this index small.
- Put durable details in the split chapter that owns them.
- Keep static checks pointed at the index plus split corpus.
- Update `AGENTS.md`, skills, rules, subagents, manifest fragments, schemas, and validation checks when durable workflow or AI-operable contracts change.
