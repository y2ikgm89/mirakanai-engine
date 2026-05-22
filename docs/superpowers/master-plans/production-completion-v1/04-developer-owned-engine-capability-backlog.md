# Production Completion v1 - Canonical Post-1.0 Capability Backlog

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or updating post-1.0 / 1.x engine capability work.

## Purpose

This is the canonical selection ledger for post-1.0 / 1.x capability work. The surrounding chapters project these rows into AI game creation, renderer, gameplay, high-freedom game creation, sprite, and 2D/3D coverage views; they must not carry their own competing backlog tables.

These rows are not 1.0 blockers while `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps = []`. A row becomes active only when a developer selects it through a dated capability, gap-cluster, or milestone plan. Game-creation agents may reference these ids in handoff/remediation rows, but they may not implement engine internals from the game-creation lane.

## Status Vocabulary

| Status | Meaning |
| --- | --- |
| `candidate` | Not selected. Needs a dated plan before implementation. |
| `foundation-ready` | A narrow supported foundation exists; broader 1.x productization remains available. |
| `implemented-1x-foundation` | A post-1.0 developer-owned foundation slice has landed; later breadth remains separate work. |
| `optional-adapter-candidate` | Requires dependency, legal, host-gate, and opaque-adapter planning before implementation. |
| `host-gated` | Requires backend, platform, SDK, device, or toolchain evidence before ready claims. |
| `excluded` | Intentionally outside the current engine product direction unless a future roadmap reopens it. |

## Selection Protocol

1. Select one coherent capability row or tightly related wave. Do not activate multiple independent rows in one plan.
2. Create a dated plan under `docs/superpowers/plans/YYYY-MM-DD-<capability>.md`.
3. Start the plan with official documentation review for the relevant SDKs, standards, or tools: CMake, vcpkg, SDL3, Direct3D 12, Vulkan, Metal, Android GameActivity, OpenAI Codex, Anthropic Claude Code, or middleware documentation as applicable.
4. Keep public game-facing contracts first-party and backend-neutral. Native handles, middleware types, editor APIs, Dear ImGui, SDL3, and renderer internals stay behind adapters or reviewed opaque boundaries.
5. Add tests and package-visible evidence for the smallest externally meaningful behavior. Promote manifest rows or validation recipes only after implementation evidence exists.
6. Update docs, skills, schemas, manifest fragments, and static checks only when the supported surface changes. Do not mark planned or host-gated work as ready.

## Canonical Capability Rows

### AI Game Creation Workflow

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `ai-game-design-spec-v1` | `foundational-unblocker` | `candidate` | Structured game design documents for gameplay family, camera, input, loop, assets, package targets, and quality gates. | Schema/static checks plus 2D and 3D examples that can drive supported generation recipes. |
| `ai-game-generation-orchestrator-v1` | `foundational-unblocker` | `candidate` | Reviewed dry-run/apply flow from design spec to `tools/new-game.ps1`, game-owned source assets, manifests, package rows, and validation recipes. | Deterministic file lists, recipe output, generated 2D/3D package evidence, and no arbitrary shell execution. |
| `ai-safe-content-mutation-ledger-v1` | `foundational-unblocker` | `candidate` | Per-game ledger of AI-owned source files, generated files, reviewed command surfaces, forbidden shared paths, and remediation actions. | `game.agent.json` schema rows, static checks, and failure on AI mutation-surface drift. |
| `ai-placeholder-asset-pipeline-v1` | `foundational-unblocker` | `foundation-ready` | AI-safe placeholder asset generation and replacement flows over first-party provenance/license rows. | Broader sprite, mesh, audio, UI, and scene-prop placeholder breadth needs selected package evidence. |
| `ai-generated-game-playtest-loop-v1` | `foundational-unblocker` | `candidate` | Supported validation/playtest recipe loop with evidence capture, failure classification, and remediation rows. | Recipe summaries, package smoke logs, gameplay counters, and supported-host evidence. |
| `ai-validation-remediation-recipes-v1` | `foundational-unblocker` | `candidate` | Machine-readable remediation for common generation failures without weakening validation. | Manifest remediation rows and tests for missing package files, invalid references, host gates, shader/tool gaps, and counter failures. |
| `ai-generated-game-quality-rubric-v1` | `foundational-unblocker` | `candidate` | Minimum quality gates beyond compile: objective, controls, feedback, fail/restart, deterministic package smoke, and budget evidence. | Headless/package assertions, sample rubric reports, and explicit unsupported rows for unmet qualities. |
| `ai-codex-claude-agent-surface-v1` | `foundational-unblocker` | `candidate` | Codex, Claude Code, and Cursor project surfaces stay aligned around game-owned write scopes, reviewed tools, and validation recipes. | `check-agents`, `check-ai-integration`, official docs review, and no broad permission grants. |
| `ai-engine-capability-handoff-v1` | `foundational-unblocker` | `candidate` | Typed handoff from game agents to developers when requested game behavior needs unsupported engine capability. | Handoff schema including requested capability, workaround, blocked feature, desired public contract, and required evidence. |
| `ai-gameplay-authoring-tools-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Value-only review rows for AI-authored gameplay feature requests, accepted/remediation/mutation-ledger rows, and package-visible counters. | Applying fixes, mutating cooked packages, autonomous commercial game design, and engine-internal edits remain separate work. |

### Game-Facing Systems

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `engine-gameplay-interaction-framework-v1` | `foundational-unblocker` | `foundation-ready` | Reusable interaction, trigger, damage/heal, pickup, objective, restart, win/loss, and feedback contracts. | Runtime tests, 2D/3D package counters, quality-rubric integration, and game-local authoring examples. |
| `engine-save-settings-profile-v1` | `foundational-unblocker` | `foundation-ready` | Versioned save/settings/profile documents with migration diagnostics and user-data path policy. | Richer save-slot/progression/package-resume evidence before broad generated-game save claims. |
| `engine-ui-game-menu-hud-v1` | `foundational-unblocker` | `foundation-ready` | Runtime UI contracts for HUD/menu intent rows, prompts, simple dialogue boxes, pause/restart, and rebinding surfaces. | Headless UI tests, package-visible menu/HUD counters, and explicit text/IME/accessibility adapter limits. |
| `engine-audio-gameplay-mixer-v1` | `foundational-unblocker` | `foundation-ready` | Gameplay cue, bus, fade, pause, loop, trigger, and optional spatial metadata planning. | Package counters, device-gated smoke, and codec diagnostics before broader audio production claims. |
| `engine-input-action-contexts-v1` | `foundational-unblocker` | `foundation-ready` | Runtime input contexts, capture, first-party rebinding rows, and profile overlays. | Broader UI capture/rebinding panels, glyph assets, localization, and device assignment remain separate evidence. |
| `engine-scene-gameplay-binding-v1` | `foundational-unblocker` | `foundation-ready` | Stable scene/component binding rows for gameplay systems without runtime reflection magic. | Scene validation tests, missing/duplicate binding diagnostics, and package counters. |
| `gameplay-simulation-orchestration-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Fixed-step simulation planning, ordered input-command playback, budget-limited frames, and replay-ready diagnostics. | Later rollback, networking, editor runtime host, thread, and frame-pacing claims need separate plans. |
| `engine-quest-dialogue-state-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Versioned quest/dialogue documents, deterministic transitions, rewards/actions, and package counters. | Cinematic tooling, voice pipeline, and full localization management remain separate work. |
| `engine-inventory-items-crafting-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Item catalogs, inventory state validation, deterministic add/remove/craft transitions, and package counters. | Economy simulation, equipment UI, network replication, and marketplace content remain separate work. |
| `engine-construction-placement-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Placement validation and reviewed runtime-scene placement intent rows. | Large-scale destruction, voxel terrain, multiplayer building replication, and editor-grade placement UX remain separate work. |

### Content Pipeline

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `engine-asset-placeholder-generation-v1` | `foundational-unblocker` | `foundation-ready` | Deterministic placeholder asset planning with provenance/license rows and package handoff. | Broader sprite/mesh/audio/UI placeholder generators and replacement workflows need selected evidence. |
| `sprite-atlas-authoring-v1` | `gameplay-family-enabler` | `foundation-ready` | Reviewed decoded-frame atlas source authoring, texture source rows, registry rows, and package handoff. | Production image decode/import breadth, atlas page policies, pivots/borders, and editor UX need a selected sprite plan. |
| `sprite-batching-renderer-v1` | `gameplay-family-enabler` | `foundation-ready` | Package-visible sprite batch counters and selected renderer-owned native sprite execution evidence. | High-density world/UI/effects batching with per-backend gates and budgets remains a 1.x stream. |
| `sprite-animation-flipbook-v1` | `gameplay-family-enabler` | `foundation-ready` | First-party cooked sprite animation rows and package smoke counters. | Direction sets, events, playback modes, and richer gameplay-state integration remain follow-up work. |
| `sprite-sorting-layer-v1` | `gameplay-family-enabler` | `candidate` | Explicit 2D sorting layers, order-in-layer, camera-space/world-space policy, and diagnostics. | Deterministic sorting tests and package-visible sorted draw counters. |
| `sprite-9slice-and-tiled-v1` | `gameplay-family-enabler` | `candidate` | 9-slice/tiled metadata and renderer/UI support for panels, bars, walls, floors, and reusable sprites. | Border/tile validation, atlas dependency checks, and package counters. |
| `sprite-collision-hitbox-v1` | `gameplay-family-enabler` | `candidate` | Frame-authored hitbox/hurtbox rows tied to sprite frames, layers, and gameplay interaction rows. | Hitbox schema tests, deterministic hit counters, and projectile/adventure package examples. |
| `sprite-effects-particles-v1` | `gameplay-family-enabler` | `candidate` | Budgeted sprite effects for particles, damage numbers, trails, hit flashes, and transient visuals. | Pool/budget counters, package-visible effect rows, and backend-specific renderer evidence. |
| `sprite-editor-preview-diagnostics-v1` | `gameplay-family-enabler` | `candidate` | Retained editor/game diagnostics for selected sprites, atlas pages, animation frames, sorting, hitboxes, and package dependencies. | Host-gated preview evidence and no renderer native handle exposure. |
| `material-shader-graph-production-v1` | `gameplay-family-enabler` | `foundation-ready` | Existing material graph authoring, package binding, shader export, and reviewed compile-request planning become production authoring workflows. | Shader graph execution, editor graph UX, live generation, and backend shader evidence require separate selected work. |

### Renderer Production

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `renderer-modern-materials-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Backend-gated modern material package evidence and material variant diagnostics. | IBL, richer PBR, full graph execution, and native cache APIs remain separate work. |
| `renderer-lighting-shadows-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Backend-neutral lighting/shadow policy diagnostics plus selected D3D12/Vulkan package evidence. | Multi-light budgets, clustered/Forward+ lists, GI, ray tracing, and broad parity need selected plans. |
| `renderer-postprocess-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Backend-neutral postprocess policy diagnostics plus selected D3D12/Vulkan package evidence. | Tone mapping, exposure, bloom, color grading, fog, AA selection, and subjective visual quality need evidence gates. |
| `renderer-scene-scale-v1` | `scale-enabler` | `implemented-1x-foundation` | Scene-scale policy diagnostics, package-visible counters, and selected instanced draw evidence. | GPU-driven rendering, full high-object-count performance readiness, and automatic content optimization remain separate. |
| `renderer-gpu-memory-v1` | `scale-enabler` | `implemented-1x-foundation` | Backend-neutral GPU memory policy diagnostics, package counters, and selected D3D12/Vulkan memory evidence. | Automatic eviction, broad residency budgets, and cross-backend parity need per-host proof. |
| `renderer-backend-parity-v1` | `scale-enabler` | `host-gated` | Promote selected Vulkan and Metal features independently from D3D12. | Strict Vulkan validation/SPIR-V evidence and Apple-host Metal/Xcode evidence per feature. |
| `renderer-debug-profiling-v1` | `scale-enabler` | `implemented-1x-foundation` | Backend-neutral debug profiling diagnostics and selected backend evidence. | Crash/telemetry backend publication, symbol-server workflows, and production flame graphs remain separate. |

### Gameplay, Physics, Navigation, And AI

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `physics-character-dynamics-v1` | `gameplay-family-enabler` | `foundation-ready` | Conservative character/controller movement, movement-policy diagnostics, and package counters. | Slope/step/platform polish, richer push/slide policies, and full production controller tuning remain 1.x work. |
| `physics-collision-query-v1` | `gameplay-family-enabler` | `foundation-ready` | Ray, shape, sweep, trigger, layer, and deterministic query foundations. | Oriented boxes, mesh/convex casts, editor visualization, and middleware-native query exposure remain separate. |
| `physics-constraints-and-joints-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Distance/fixed/linear-axis constraint rows, budgets, and package counters. | Full ragdolls, soft bodies, destructive physics, and persistent joint assets remain separate. |
| `physics-vehicles-and-kinematics-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Kinematic motion planning plus public simple vehicle policy and package-visible counters. | Broad vehicle dynamics, tire models, suspension tooling, and persistent vehicle simulation remain separate. |
| `navigation-navmesh-v1` | `gameplay-family-enabler` | `foundation-ready` | Scene-ref navmesh dynamic-obstacle routing and package evidence. | Broad navmesh import, automatic bake workflows, and streaming navmesh readiness remain separate. |
| `navigation-crowd-local-avoidance-v1` | `gameplay-family-enabler` | `foundation-ready` | Value-only crowd/local-avoidance planning and package evidence. | Massive crowd simulation, animation-aware steering, and networked crowd determinism remain separate. |
| `navigation-hierarchical-world-v1` | `scale-enabler` | `candidate` | Region graphs, portal links, streaming-safe nav data references, and path cache diagnostics. | Open-world streaming readiness and automatic large-world nav baking remain separate. |
| `ai-behavior-authoring-v1` | `gameplay-family-enabler` | `foundation-ready` | Behavior documents, blackboard/perception integration, validation rows, and deterministic traces. | Visual scripting, arbitrary code execution, ML inference, and networked AI replication remain separate. |
| `ai-perception-services-v1` | `gameplay-family-enabler` | `foundation-ready` | Sight/hearing classification, stable target selection, and blackboard projection. | Memory decay, teams/factions, event stimuli breadth, and performance budgets remain separate. |

### Scale And Persistence

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `engine-procedural-generation-v1` | `gameplay-family-enabler` | `implemented-1x-foundation` | Seeded generation rows for objects, encounters, loot, replay hashes, and package-visible adoption. | Quest/world-scale procedural content and subjective quality claims need selected validation. |
| `engine-world-region-streaming-v1` | `scale-enabler` | `implemented-1x-foundation` | Region catalogs, safe-point load/unload planning, budgets, reviewed package bridge evidence, and counters. | Seamless open-world readiness, background streaming, and unlimited world size remain separate. |
| `engine-entity-scale-and-culling-v1` | `scale-enabler` | `implemented-1x-foundation` | Deterministic entity rows, visibility/culling, LOD/update/draw budget intent, and package counters. | GPU/occlusion culling, full high-object-count performance readiness, and renderer-owned scheduling remain separate. |
| `simulation-persistence-v1` | `scale-enabler` | `candidate` | Stable entity/world state, save slots, versioned migrations, snapshots, and corrupted-save remediation. | Cloud saves, cross-device sync, multiplayer replication, and binary compatibility policy remain separate. |
| `world-streaming-and-large-scenes-v1` | `scale-enabler` | `foundation-ready` | Region/chunk packages, safe load/unload, resource budgets, and missing-region diagnostics. | Background streaming, open-world parity, and platform-specific async job systems need selected evidence. |
| `procedural-content-generation-v1` | `gameplay-family-enabler` | `foundation-ready` | Reproducible seed/input/generator rows and deterministic output summaries. | External services, unbounded AI-generated assets, and commercial content-quality claims remain separate. |

### Optional Adapters

| Capability id | Tier | Status | Ready boundary | Evidence before promotion |
| --- | --- | --- | --- | --- |
| `engine-scripting-sandbox-v1` | `optional-adapter` | `implemented-1x-foundation` | Optional-adapter scripting sandbox policy foundation and selected package evidence. | Runtime execution adapters, script APIs, replay guarantees, and dependency/legal updates need selected plans. |
| `scripting-and-mod-sandbox-v1` | `optional-adapter` | `optional-adapter-candidate` | Constrained gameplay scripting/modding with first-party host API facade and budgets. | Sandbox capability tests, no filesystem/network/process access by default, and package counters. |
| `engine-networking-foundation-v1` | `optional-adapter` | `implemented-1x-foundation` | Value-only networking foundation policy rows and selected package evidence. | Transport/session/replication/rollback, security model, and real network host gates remain separate. |
| `networking-and-multiplayer-v1` | `optional-adapter` | `optional-adapter-candidate` | Multiplayer after deterministic simulation and persistence mature. | Architecture decision, threat model, replay tests, host/network gates, and no broad multiplayer ready claim. |
| `native-physics-middleware-adapter-v1` | `optional-adapter` | `optional-adapter-candidate` | Optional Jolt/PhysX/Recast-style adapters behind first-party opaque APIs. | vcpkg feature gating, legal records, host gates, fallback diagnostics, and no middleware types in public APIs. |

## Projection Chapters

- [03-ai-autonomous-game-creation.md](03-ai-autonomous-game-creation.md) describes game-owned mutation boundaries and AI workflow rules for the `ai-*` rows.
- [05-2d-3d-capability-coverage-matrix.md](05-2d-3d-capability-coverage-matrix.md) maps supported 2D/3D scopes to these rows.
- [06-renderer-advanced-production-track.md](06-renderer-advanced-production-track.md) gives renderer-specific evidence rules for `renderer-*`, material, and sprite renderer rows.
- [07-gameplay-physics-nav-ai-advanced-track.md](07-gameplay-physics-nav-ai-advanced-track.md) gives deterministic simulation and middleware-boundary rules.
- [08-high-freedom-game-creation-track.md](08-high-freedom-game-creation-track.md) groups sandbox, persistence, procedural, scripting, and multiplayer rows.
- [09-sprite-production-pipeline-track.md](09-sprite-production-pipeline-track.md) groups sprite-specific authoring/import/runtime rows.
- [10-gameplay-archetype-validation.md](10-gameplay-archetype-validation.md) uses archetypes only as evidence probes; it must not introduce separate capability rows.

Promotion rule: add or change a capability here only when it is reusable across at least two gameplay families, has a stable first-party contract, can be validated outside one specific game, and can be exposed safely to AI through manifests, reviewed tools, schemas, or validation recipes.
