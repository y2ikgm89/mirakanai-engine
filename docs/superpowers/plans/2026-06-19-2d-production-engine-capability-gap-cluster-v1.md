# 2D Production Engine Capability Gap Cluster v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `gameengine-feature` before implementation. Use `gameengine-rendering` before renderer/RHI edits, `gameengine-editor` before editor-core or visible editor edits, `gameengine-performance-optimization` before performance-budget or benchmark edits, and `gameengine-cmake-build-system` before CMake or validation-script edits.

**Plan ID:** `2d-production-engine-capability-gap-cluster-v1`
**Status:** Active.

**Goal:** Define a non-overlapping candidate milestone for the remaining reusable 2D game-engine capabilities needed to move from selected 2D package/source foundations to a production-grade 2D runtime creation loop.

**Architecture:** This plan is the active isolated 2D milestone for branch `codex/2d-engine-gap-plan`. Each implementation phase must use first-party, backend-neutral C++23 contracts, package-visible counters, clean-room IP provenance, and fail-closed validation before any ready claim.

**Tech Stack:** C++23, `MK_runtime`, `MK_renderer`, `MK_scene_renderer`, `MK_runtime_scene`, `MK_runtime_scene_rhi`, `MK_physics`, `MK_tools`, `MK_editor_core`, PowerShell 7 repository scripts, CMake presets, `game.agent.json`, `engine/agent/manifest.fragments/`.

---

## Status

- Plan id: `2d-production-engine-capability-gap-cluster-v1`
- Date: `2026-06-19`
- Re-audited: `2026-06-21`
- Status: `active-milestone` in branch `codex/2d-engine-gap-plan`; selected by `currentActivePlan`
- Authored in isolated worktree: `.worktrees/codex-2d-engine-gap-plan`
- Branch: `codex/2d-engine-gap-plan`
- Parallel external plan: `environment-highest-commercial-readiness-v1` remains separate work outside this isolated branch.
- Phase 0 selection gate updated this plan, the plan registry, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, and the composed manifest. Implementation phases may edit only their task-owned runtime/tests/package/docs/manifest/static-check surfaces and must not mutate active environment files or infer environment readiness.

## Phase 0 Selection Gate

**Goal:** Select this existing 2D capability gap-cluster plan as the active isolated milestone before writing production runtime code.

**Context:** The main checkout may still contain independent environment/MAVG work. This branch owns only the 2D plan and selected 2D implementation phases.

**Constraints:**

- Do not edit the main checkout.
- Do not mark any 2D phase ready from selection alone.
- Keep `unsupportedProductionGaps = []` unchanged.
- Keep environment commercial, all-platform, Vulkan, Metal, optimization, weather, asset-pipeline, artist-workflow, and broad `environment_ready` claims untouched.

**Done When:**

- `currentActivePlan` points at this plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`.
- `engine/agent/manifest.json` is regenerated from fragments.
- The plan registry marks this plan as the active milestone for this branch.
- Phase 1 has a RED test before production code.

**Validation Evidence:**

- 2026-06-21 RED: `tools/cmake.ps1 --preset dev` configured successfully, then `tools/cmake.ps1 --build --preset dev --target MK_runtime_2d_gameplay_execution_loop_tests` failed as expected because `mirakana/runtime/gameplay_execution_loop_2d.hpp` does not exist yet.
- IP provenance: `external_code_copied=0`, `external_assets_copied=0`, `third_party_api_surface_copied=0`, `third_party_ui_layout_copied=0`, `third_party_trademark_public_api=0`, `license_records_updated=not_applicable`, `official_docs_used_for_category_mapping_only=1`.

## Investigation Sources

### Context7 Evidence

| Source | Query purpose | Findings applied here |
| --- | --- | --- |
| `/godotengine/godot-docs` | Current Godot 2D engine surface. | Godot exposes first-class `Sprite2D`, `AnimatedSprite2D`, `TileMap`, `InputMap`, `PhysicsServer2D`, `RenderingServer`, `ResourceLoader`, `ResourceSaver`, `Performance`, and `WorkerThreadPool` surfaces. This supports treating 2D creation as a cross-module engine loop, not only a sample game feature. |
| `/websites/unity3d_6000_0_manual` | Current Unity 6.0 2D production workflow. | Unity's 2D workflow groups Fundamentals, Scripting, Sprites, in-game environments, character animation, graphics, Physics 2D, Audio, UI, profiling/optimization/testing, and publishing. This supports a deduplicated plan around sprite/atlas execution, physics, input UX, performance evidence, and package playtest loops. |

### Official Documentation Evidence

| Source | Findings applied here |
| --- | --- |
| Unity Manual 2D game creation workflow: `https://docs.unity3d.com/6000.0/Documentation/Manual/2d-game-creation-wokflow.html` | Official Unity 2D workflow includes fundamentals, scripting, sprites, in-game environments, character animation, graphics, Physics 2D, Audio, UI, profiling/optimization/testing, and publishing. Sprite Renderer, sorting layers/groups, Sprite Atlas, Tilemap, 9-slicing, SpriteShape, 2D lighting, particles, postprocess, Rigidbody2D, Collider2D, triggers, joints, and effectors are called out in that workflow. |
| Unity Sprite Atlas workflow: `https://docs.unity.cn/Manual/SpriteAtlasWorkflow.html` | Production atlas usage needs atlas creation, pack-object selection, build inclusion, variants/distribution decisions, and atlas grouping by common usage to reduce runtime overhead. |
| Unity Rigidbody 2D: `https://docs.unity3d.com/6000.0/Documentation/Manual/2d-physics/rigidbody/introduction-to-rigidbody-2d.html` | 2D physics readiness includes Rigidbody2D-driven movement, collider interaction, and transform synchronization. This plan therefore separates physics execution from value-only query rows. |
| Unity Input System: `https://docs.unity3d.com/Packages/com.unity.inputsystem@latest/` | Production input needs device-flexible actions and bindings. This plan keeps UI panels separate while adding runtime device/profile semantics and evidence. |
| Godot TileMap class: `https://docs.godotengine.org/en/stable/classes/class_tilemap.html` | Tile maps are a first-class 2D map surface with TileSet data, multiple layers, and batched updates. Existing sandbox/tile plans already own this production path, so this plan excludes duplicate tilemap implementation. |
| Godot PhysicsServer2D: `https://docs.godotengine.org/en/stable/classes/class_physicsserver2d.html` | Godot's 2D physics server owns low-level bodies, areas, shapes, joints, and direct queries. This supports a focused 2D physics execution-extension phase instead of only collision/hitbox planning. |
| Godot Input examples: `https://docs.godotengine.org/en/stable/tutorials/inputs/input_examples.html` | Godot recommends named input actions through `InputMap`, supports events vs polling, keyboard/mouse/gamepad/touch, and runtime action processing. This supports gesture-to-action and per-device input evidence. |
| GameMaker tile set manual: `https://manual.gamemaker.io/beta/en/Quick_Start_Guide/Creating_Tile_Sets.htm` | Tile sets and tile maps are optimized static-level building blocks. Existing sandbox/tile production lanes already cover this category. |
| Defold manuals: `https://defold.com/manuals/` and `https://defold.com/manuals/physics-shapes/` | Defold documents resource management, sprites, animation, sound, GUI, tile maps, profiling/memory, and tilemap collision shapes. This supports using tilemap collision and profiling as evidence categories while keeping Defold-specific APIs out of public contracts. |
| Unity 2D feature page: `https://unity.com/features/2d` | Unity positions Tilemaps/Level Authoring plus 2D Physics colliders/rigidbodies/joints/effectors as core 2D engine features. This informs the physics and level-authoring non-overlap boundaries. |

## Official-Practice Traceability

This plan is not a claim that Unity, Godot, GameMaker, or Defold APIs should be copied. It uses official engine documentation only to identify production-grade 2D capability categories, then maps those categories to first-party `mirakana` contracts, existing completed plans, or explicit exclusions.

| Official category | Official anchor | Repository truth | Plan treatment |
| --- | --- | --- | --- |
| Fundamentals, scene objects, transform, camera, scripting/gameplay logic | Unity 2D workflow fundamentals/scripting; Godot `Node2D`/`CanvasItem` inheritance in `Sprite2D`, `TileMap`, and physics nodes. | `2d-playable-source-tree`, `engine-scene-gameplay-binding-v1`, `gameplay-runtime-scheduler-production-v1`, `world-entity-model-production-v1`, and `gameplay-simulation-orchestration-v1` already provide foundation/value contracts. | Phase 1 only connects those value contracts into an executable selected 2D runtime loop. It must not create a scripting VM, new ECS framework, or editor module ABI. |
| Sprites, Sprite Renderer, sorting layers/groups, sprite-sheet animation | Unity Sprite Renderer/sorting workflow; Godot `Sprite2D` and `AnimatedSprite2D`. | `sprite-sorting-layer-v1`, `sprite-animation-flipbook-v1`, `sprite-editor-preview-diagnostics-v1`, and selected package counters are implemented foundations. | Phases 2-3 strengthen runtime atlas residency and dense sprite throughput. They do not reopen basic sorting or flipbook animation evidence. |
| Sprite Atlas packing, build inclusion, distribution, and performance optimization | Unity Sprite Atlas workflow; Godot texture region and atlas-capable `Sprite2D`. | `sprite-atlas-authoring-v1` supports reviewed decoded-frame atlas authoring, but runtime source decode, renderer/RHI residency, multi-page/padded runtime atlas residency, and broader atlas/editor/runtime integration remain non-ready claims. | Phase 2 owns cooked-only multi-page/padded runtime atlas residency and upload handoff rows. Runtime PNG/JPEG parsing, arbitrary imports, and production atlas editor UX remain excluded. |
| Tilemaps, level authoring, tile collision, batched tile updates | Unity Tilemap workflow; Godot TileMap/TileMapLayer and batched tile updates; GameMaker and Defold tilemap/tile-source docs. | `generic-2d-sandbox-production-lane-v1` and children already own sandbox world runtime, tile simulation, persistence, streaming safe points, tile chunk renderer, and sandbox authoring. | No tilemap/sandbox rewrite. Phase 6 may use sandbox/tile packages as workload evidence only. |
| Character animation, particles, 2D lighting, shadows, postprocess, visual polish | Unity character animation, particle, 2D lighting, shadows, and postprocess guidance; Godot `AnimatedSprite2D` and particle material docs. | `sprite-animation-flipbook-v1`, `sprite-effects-particles-v1`, `renderer-lighting-shadows-v1`, `renderer-postprocess-v1`, and environment/renderer plans own these rows. 2D skeletal/cutout animation beyond existing animation foundations is not selected by this plan. | This plan does not duplicate visual-polish systems. Dense sprite throughput may measure existing particle/effects counters as workload ingredients, but new lighting/postprocess/skeletal animation features require their own selected plan. |
| Physics 2D bodies, areas/triggers, joints, effectors, physics-server style execution | Unity Rigidbody2D/Collider2D/triggers/joints/effectors; Godot `PhysicsServer2D` spaces, bodies, areas, joints, direct queries, and transform callbacks. | `PhysicsWorld2D` already has force integration, broadphase, contacts, triggers, raycasts, sweeps, and discrete steps. Manifest non-ready claims include 2D CCD, dynamic-vs-dynamic TOI, persistent joint assets, and timing benchmarks. | Phase 4 owns exact 2D execution extensions: deterministic sweeps/TOI, kinematic contact resolution, trigger/area rows, first-party 2D joints, and package counters. Middleware/native handles remain excluded. |
| Input actions, device abstraction, gestures, rebinding, glyphs, keyboard layout, multiplayer device assignment | Unity Input System/action binding guidance; Godot `InputMap` named input actions and input-event examples. | `engine-input-action-contexts-v1` already provides contexts, profile overlays, capture, and symbolic glyph lookup keys. Real glyph assets/rendering, keyboard-layout localization, multiplayer device assignment, gesture-to-action binding, and per-device profiles remain follow-up work. | Phase 5 owns engine-level input semantics only. Runtime UI panels, text/IME/accessibility, and visible rebinding UX remain in the UI/editor roadmap. |
| Audio, UI, accessibility, resource management, profiling/optimization/testing, publishing | Unity workflow audio/UI/profiling/testing/publishing; Defold manuals for sound/GUI/resource management/profiling/memory. | Audio/UI/text/accessibility/resource/package foundations are implemented or host-gated in existing plans; broad production UI/text and accessibility are owned by the UI roadmap. Packaging/playtest foundations exist but broad productized 2D package iteration remains non-ready. | Phases 6-7 own fixed workload evidence and reviewed 2D package playtest productization. They do not implement broad audio, UI widgets, accessibility, publishing-store workflows, or platform certification. |

## Highest-Level Implementation Standard

Any phase selected from this candidate plan must satisfy all of these criteria before it can claim readiness:

1. Refresh Context7 and official documentation for that phase's SDKs, engine concepts, and toolchain on the implementation date.
2. Map official categories to first-party `mirakana` APIs; do not import Unity, Godot, GameMaker, Defold, SDL3, Dear ImGui, native OS, renderer backend, or middleware concepts into public contracts.
3. Start with focused RED tests for the smallest externally meaningful runtime/API guarantee.
4. Provide deterministic diagnostics for every fail-closed path: missing rows, invalid ids, over-budget rows, stale handles, host-gated backend evidence, unsupported broad claims, and native-handle attempts.
5. Add selected package-visible counters and, for performance claims, retained trace/profile artifacts with exact metric names.
6. Keep existing completed plans closed; use them as dependencies only, never as places to append new work.
7. Update docs, manifest fragments, schemas, static checks, and validation recipes only for the exact supported surface being promoted.
8. Run focused build/CTest loops plus full `tools/validate.ps1` for any runtime/build/public-contract slice.
9. Preserve all non-claims unless a phase explicitly implements and validates the exact missing evidence.

## Legal And Clean-Room Constraints

This plan is designed for independent implementation. It is not a clone plan for Unity, Unreal Engine, Godot, GameMaker, Defold, or any other engine.

### Legal anchors used for this planning boundary

| Anchor | Official source | Planning consequence |
| --- | --- | --- |
| U.S. Copyright Office / USPTO baseline: copyright protects expression, not ideas, procedures, processes, systems, methods, concepts, principles, or discoveries. | `https://www.copyright.gov/help/faq/faq-protect.html`, `https://www.copyright.gov/what-is-copyright/`, `https://www.copyright.gov/circs/circ31.pdf`, `https://www.copyright.gov/circs/circ33.pdf` | Official documentation may be used to identify broad feature categories such as sprites, tilemaps, input actions, physics joints, profiling, or package publishing. Implementation must not copy protected expression: source code, documentation prose, sample projects, UI layouts, asset content, shader text, diagrams, or distinctive naming/API structures. |
| Unity legal/trademark guidance: Unity marks and logos are controlled by Unity. | `https://unity.com/legal/branding-trademarks`, `https://unity.com/legal/terms-of-service`, `https://docs.unity3d.com/6000.4/Documentation/Manual/TermsOfUse.html` | Do not use Unity branding, product names, package names, asset names, or UI terminology as public `mirakana` API names except in documentation citations or comparative evidence. |
| Unreal Engine EULA and Epic trademark guidance: Unreal Engine code, Examples, and Starter Content are licensed technology, and Unreal trademarks require approved usage. | `https://www.unrealengine.com/eula/unreal`, `https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license` | Do not inspect, copy, translate, decompile, derive from, or reimplement from Unreal Engine source, Examples, Starter Content, Marketplace/Fab content, Blueprints, shaders, editor UI, or sample project assets. Official public docs may be cited only for category-level comparison. |
| Godot official license and compliance guidance: Godot is MIT/Expat-licensed and redistribution of Godot or derivatives carries notice obligations. | `https://godotengine.org/license/`, `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html` | Even where permissive licensing exists, this plan does not copy Godot engine code, docs prose, demos, icons, node names as API surface, or scene/resource formats. If any future phase intentionally imports third-party code or assets, it must go through dependency/license records first. |
| Repository policy in `AGENTS.md`, `docs/legal-and-licensing.md`, and `gameengine-license-audit`: license-less material is unusable; third-party code must be isolated; notices and dependency records are required before distribution. | `docs/legal-and-licensing.md`, `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md`, `.agents/skills/gameengine-license-audit/SKILL.md` | The selected implementation must remain first-party unless a separate dependency decision updates `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md`. |

### Anti-clone implementation rules

1. Use official docs only as feature taxonomy and interoperability context, not as implementation instructions to copy line-by-line.
2. Do not copy source code, pseudocode, class hierarchies, public API names, serialization formats, sample scenes, assets, editor panel layout, shaders, test data, screenshots, icons, or documentation prose from Unity, Unreal Engine, Godot, GameMaker, Defold, or their marketplaces/samples.
3. Do not make public APIs named after third-party concepts such as `GameObject`, `MonoBehaviour`, `SpriteRenderer`, `TileMapLayer`, `PhysicsServer2D`, `Blueprint`, `Actor`, `Component`, `WorldPartition`, or similar engine-specific terms unless a separate architecture decision approves a generic term with a first-party meaning. Prefer existing `mirakana` terms such as runtime package, scene binding, sprite atlas residency, package-visible counters, gameplay execution loop, and first-party value rows.
4. Do not reproduce third-party editor workflows visually. Any future editor/productization phase must use first-party `mirakana::ui` layouts, command ids, retained rows, and AI-operable evidence rather than cloning Unity Inspector/Project/Hierarchy, Unreal Content Browser/Blueprint Editor, or Godot Scene/Inspector workflows.
5. Do not ingest third-party sample assets, templates, Starter Content, Marketplace/Fab assets, Unity sample project content, Godot demo assets, Defold examples, screenshots, logos, or trademarked names into repository tests or packages.
6. Do not use reverse engineering, decompilation, binary inspection, source-available proprietary code, or marketplace packages as implementation input.
7. If a future phase wants to use any third-party library, code, asset, data format, shader, benchmark scene, or sample file, stop and run a dedicated dependency/license decision before implementation. Required records include source URL, retrieved date, version/commit, copyright holder, SPDX or custom license reference, modification status, notices, distribution target, and dependency metadata.
8. Keep all implementation evidence reproducible from first-party tests, generated fixtures, authored minimal data, or officially documented standards. When an official standard is not available, write a first-party spec under `docs/specs/` before adding code.

### Legal readiness gate for every selected phase

Each selected phase must include a short "IP provenance" subsection in its implementation notes or final validation evidence:

- `external_code_copied=0`
- `external_assets_copied=0`
- `third_party_api_surface_copied=0`
- `third_party_ui_layout_copied=0`
- `third_party_trademark_public_api=0`
- `license_records_updated=not_applicable` unless a separately approved dependency or asset is added
- `official_docs_used_for_category_mapping_only=1`

## Local Evidence Reviewed

| Local source | Current truth used |
| --- | --- |
| `engine/agent/manifest.json` via `tools/agent-context.ps1 -ContextProfile Minimal` | Pre-selection review found stage `mvp-closed-not-commercial-complete`, `unsupportedProductionGaps = []`, `2d-playable-source-tree` ready, and `2d-desktop-runtime-package` host-gated with selected sprite, tile, UI, audio, performance, long-run, and D3D12 native sprite evidence. Phase 0 selection now makes this plan the active milestone in this isolated branch while leaving environment commercial-readiness work external. |
| `docs/ai-game-development.md` | 2D source and package foundations are ready/host-gated, but runtime source parsing, renderer/RHI residency, broad package streaming, broad hot reload productization, and broad production readiness remain explicit non-claims. |
| `docs/current-capabilities.md` | Explicit non-ready claims include broad package cooking/streaming, broad production sprite batching readiness, runtime image decoding beyond reviewed adapters, broader atlas/editor/runtime integration, full editor productization, hot reload, active-session reload/unload, and broad high-object-count performance readiness. |
| `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md` | Canonical rows already exist for the selected 2D follow-up space. This plan creates no new canonical backlog row until a later implementation phase proves a missing reusable capability cannot be expressed by existing ids. |
| `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md` | 2D coverage is currently foundation-covered; dense 2D arena/projectile games still point to `engine-entity-scale-and-culling-v1`, `sprite-batching-renderer-v1`, `sprite-collision-hitbox-v1`, and `gameplay-simulation-orchestration-v1` for broader support. |

## Non-Overlap Matrix

| Existing plan or row | Already owns | This plan excludes | This plan may add only if selected later |
| --- | --- | --- | --- |
| `2026-06-18-environment-highest-commercial-readiness-v1.md` | Strict Vulkan, Apple Metal, backend parity, broad environment optimization, AAA environment assets, OpenEXR/KTX2/Basis, physical weather, production artist workflow. | No environment feature, backend commercial gate, or broad renderer parity work. | None. This branch must not change environment implementation files, environment evidence, or environment readiness claims. |
| `2026-06-12-first-party-ui-runtime-editor-production-roadmap-v1.md` | Runtime UI widgets, UI binding/input routing, UI authoring documents, UI renderer execution, Windows text/font adapters, text editing/IME, accessibility, editor UI builder, cross-platform UI adapter gates. | No UI widget, text shaping, IME, accessibility, runtime UI panel, or editor UI builder work. | Input rows may expose device/profile metadata that UI can present later, but presentation belongs to the UI roadmap. |
| `2026-05-27-generic-2d-sandbox-production-lane-v1.md` and children | Sandbox world model, mutation execution, tile simulation primitives, persistence snapshot, streaming safe points, tile chunk renderer, authoring/cook, sandbox validation/performance budgets. | No generic sandbox/tilemap engine rewrite, no block/world-specific mutation, no duplicate tile chunk renderer. | Use the sandbox package only as one workload in performance/playtest validation. |
| `2026-05-25-general-purpose-game-production-v1.md` | Value-only production scheduler, world/entity model, addressable content streaming, authoring workflows, runtime UI workbench, genre packs, network replication value rows, renderer/VFX/profiling value rows. | No duplicate scheduler/world/addressable value contract. | Connect existing value contracts into executable selected 2D runtime loops and package counters. |
| `2026-05-24-foundation-ready-burn-down-v1.md` rows | `engine-input-action-contexts-v1`, `sprite-atlas-authoring-v1`, `sprite-batching-renderer-v1`, `sprite-animation-flipbook-v1`, `physics-collision-query-v1`, `world-streaming-and-large-scenes-v1`, and other foundation promotions. | No reopen of foundation-ready proof or basic sprite/input/physics rows. | Promote only stronger production evidence named below. |
| Runtime Resource v2 / hot-reload helpers | Reviewed resident mount/replace, reviewed evictions, candidate discovery, recook-to-resident replacement safe points, registered asset watch tick helpers. | No new native watcher, automatic target inference, broad background streaming, package scripts, arbitrary/LRU eviction, renderer/RHI ownership, or native handles. | 2D package playtest productization may compose existing helpers into a reviewed recipe and counters. |
| `renderer-production-quality-backend-parity-v1`, `renderer-general-quality-matrix-v1`, MAVG plans | Broad renderer/RHI quality, backend evidence, GPU memory/residency rows, virtualized geometry. | No broad renderer quality, no MAVG, no 3D renderer scope. | 2D sprite phases may add sprite-specific counters while keeping backend parity separate. |
| Audio/network/scripting plans | Audio production review rows, optional network transport, scripting/mod sandbox. | No broad audio codec/device/DSP/HRTF work, no multiplayer/rollback transport, no scripting runtime. | Selected 2D package counters may consume already reviewed audio/input/script surfaces as workload ingredients only. |

## Deduplicated Gap Cluster

The project can already scaffold and validate selected 2D packages. The remaining non-overlapping 2D engine gaps are these production connectors:

| Candidate phase | Canonical ids consumed | Gap closed | Explicit non-claims |
| --- | --- | --- | --- |
| `2d-gameplay-execution-loop-v1` | `gameplay-runtime-scheduler-production-v1`, `world-entity-model-production-v1`, `engine-scene-gameplay-binding-v1`, `gameplay-simulation-orchestration-v1`, `engine-gameplay-interaction-framework-v1` | Execute selected 2D scene bindings, input commands, deterministic fixed steps, entity lifecycle rows, and gameplay interaction rows in one runtime loop with replay diagnostics and package counters. | No networking rollback/prediction, no scripting VM, no editor module ABI change, no package mutation, no renderer/RHI handles. |
| `2d-sprite-atlas-runtime-residency-v1` | `sprite-atlas-authoring-v1`, `sprite-animation-flipbook-v1`, `addressable-content-streaming-production-v1`, Runtime Resource v2 rows | Convert reviewed cooked sprite atlas/page metadata into runtime resident atlas pages, stale-handle-safe lookup, and renderer upload handoff evidence for selected 2D packages. | No runtime PNG/JPEG source parsing, no production atlas editor UX, no arbitrary source asset import, no broad renderer quality, no Metal readiness by inference. |
| `2d-dense-sprite-throughput-v1` | `sprite-batching-renderer-v1`, `engine-entity-scale-and-culling-v1`, `performance-baseline-v1`, `renderer-general-quality-matrix-v1` | Add high-density 2D sprite workload rows, sorting/material/atlas grouping diagnostics, draw/instance/upload counters, and retained timing artifacts for selected backends. | No broad all-game optimization claim, no cross-backend parity inference, no GPU/occlusion culling claim, no subjective visual-quality gate. |
| `2d-physics-runtime-extension-v1` | `physics-collision-query-v1`, `physics-character-dynamics-v1`, `physics-constraints-and-joints-v1`, `sprite-collision-hitbox-v1` | Add 2D runtime physics execution evidence for AABB/circle bodies, deterministic sweeps/time-of-impact, kinematic controller contact resolution, trigger/area callbacks, and first-party 2D joint rows. | No optional middleware public API, no ragdoll, no 3D vehicle claim, no native physics handles, no broad physics parity. |
| `2d-input-device-production-ux-v1` | `engine-input-action-contexts-v1`, `engine-ui-game-menu-hud-v1` | Add gesture-to-`RuntimeInputActions` binding, radial/response-curve metadata, platform glyph asset lookup rows, keyboard-layout localization rows, per-device profile rows, and multiplayer device assignment semantics. | No runtime UI panels, no text/IME/accessibility work, no native input handle exposure, no SDL3/input middleware, no full visible rebinding UX. |
| `2d-production-workload-matrix-v1` | `performance-baseline-v1`, `long-running-performance-readiness-v1`, `engine-entity-scale-and-culling-v1`, `sprite-batching-renderer-v1`, `generic-2d-sandbox-production-lane-v1` | Add a fixed 2D workload matrix covering dense arena/projectiles, tile sandbox, sprite animation, input, UI overlay counters, audio triggers, resident packages, and frame/memory/draw/upload budgets. | No broad optimization until all required artifacts exist, no vendor parity inference, no default 108000-frame soak requirement. |
| `2d-package-playtest-productization-v1` | `ai-generated-game-playtest-loop-v1`, Runtime Resource v2 hot-reload helpers, editor runtime-host playtest launch rows, editor game-module driver rows | Compose selected 2D package build, external runtime launch, evidence import, safe recook-to-resident replacement, and failure classification into a reviewed recipe. | No active-session in-editor hot reload, no stable third-party ABI, no package scripts, no editor-executed validation recipes, no arbitrary shell, no runtime host embedding unless separately selected. |

## Implementation Sequence When Selected

### Phase 0: Selection Gate And Drift Freeze

**Files to edit only when operator selects this milestone:**

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- `docs/superpowers/plans/README.md`
- `docs/roadmap.md`
- `docs/current-capabilities.md`
- `docs/ai-game-development.md`

**Steps:**

1. Re-run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal`.
2. Confirm `currentActivePlan` is no longer owned by a conflicting environment/UI/editor milestone, or explicitly coordinate a separate branch/worktree.
3. Promote only this plan id to the active candidate/selected plan fields. Do not mark any phase ready.
4. Re-read Context7 and official docs for the phase being selected. Use updated docs if Unity, Godot, platform SDK, CMake, or vcpkg pages changed after this plan date.
5. Update only affected manifest fragments, compose with `tools/compose-agent-manifest.ps1 -Write`, then run `tools/check-json-contracts.ps1`.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

### Phase 1: `2d-gameplay-execution-loop-v1`

**Target modules:**

- `engine/runtime/include/mirakana/runtime/gameplay_runtime_scheduler.hpp`
- `engine/runtime/include/mirakana/runtime/world_entity_model.hpp`
- `engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp`
- New `engine/runtime/include/mirakana/runtime/gameplay_execution_loop_2d.hpp`
- New `engine/runtime/src/gameplay_execution_loop_2d.cpp`
- `tests/unit/runtime_2d_gameplay_execution_loop_tests.cpp`
- `CMakeLists.txt` target `MK_runtime_2d_gameplay_execution_loop_tests`
- `games/sample_2d_desktop_runtime_package/`
- `tools/package-desktop-runtime.ps1`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

**Contract to add:**

- `RuntimeGameplayExecutionLoop2DDesc`
- `RuntimeGameplayExecutionLoop2DFrameInput`
- `RuntimeGameplayExecutionLoop2DStepResult`
- `RuntimeGameplayExecutionLoop2DDiagnostic`
- `execute_runtime_gameplay_loop_2d_step`

**Steps:**

1. Add a RED compile/test case requiring deterministic fixed-step execution over two input-command frames and stable entity ids.
2. Compose existing scheduler, scene gameplay binding, input action state, entity lifecycle intent, and gameplay interaction rows without changing their existing value-only contracts.
3. Reject variable `delta` authority, duplicate system order, replay-unsafe random rows, missing binding rows, and over-budget command counts with typed diagnostics.
4. Emit package counters: `2d_gameplay_execution_loop_status`, `2d_gameplay_execution_loop_steps`, `2d_gameplay_execution_loop_replay_hash`, `2d_gameplay_execution_loop_diagnostics`, `2d_gameplay_execution_loop_side_effects`.
5. Add `--require-2d-gameplay-execution-loop` to the selected 2D package smoke only after unit tests pass.
6. Update docs, manifest recipe rows, static checks, and generated-game handoff language.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_2d_gameplay_execution_loop_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_2d_gameplay_execution_loop_tests
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-2d-gameplay-execution-loop') }"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** focused tests, package smoke, manifest rows, docs, static checks, and full validation prove deterministic replay, positive step count, zero side-effect counters outside caller-owned runtime state, and zero diagnostics.

**Implementation Evidence (2026-06-21):**

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev; if ($LASTEXITCODE -eq 0) { pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_2d_gameplay_execution_loop_tests }` configured successfully, then failed because `mirakana/runtime/gameplay_execution_loop_2d.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_2d_gameplay_execution_loop_tests` passed after adding `RuntimeGameplayExecutionLoop2D*` contracts and implementation.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_2d_gameplay_execution_loop_tests` passed, including deterministic replay and missing entity binding diagnostics.
- Package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-2d-gameplay-execution-loop') }"` passed; installed smoke reported `2d_gameplay_execution_loop_status=ready`, one fixed step, one scheduler step, three scheduler system rows, one scheduler command row, two world entity rows, one interaction row, positive replay hash, `diagnostics=0`, and `side_effects=0`.
- Default package lane: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` passed after wiring the flag into `games/CMakeLists.txt`; installed smoke reported `2d_gameplay_execution_loop_status=ready`, positive replay hash, `diagnostics=0`, and `side_effects=0`.
- Manifest/static sync: `games/sample_2d_desktop_runtime_package/game.agent.json` now has `installed-2d-gameplay-execution-loop-smoke`, a `gameplay-execution-loop-2d` quality gate, and `gameplayExecutionLoop2D` contract text; `tools/check-ai-integration-080-scaffold-smokes.ps1` guards the flag, API call, status field, recipe, and CMake smoke args.
- Static/public checks: `git diff --check`, `tools/check-text-format.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-toolchain.ps1`, and `tools/check-public-api-boundaries.ps1` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-06-21; static checks were ok and CTest reported `100% tests passed, 0 tests failed out of 132`.
- IP provenance: `external_code_copied=0`, `external_assets_copied=0`, `third_party_api_surface_copied=0`, `third_party_ui_layout_copied=0`, `third_party_trademark_public_api=0`, `license_records_updated=not_applicable`, `official_docs_used_for_category_mapping_only=1`.
- Publication: PR #716 / merge commit `0123baffe8aa436d151fc623fe9a37b3890893a6` completed Phase 1 publication and main synchronization evidence for this branch baseline.

### Phase 2: `2d-sprite-atlas-runtime-residency-v1`

**Target modules:**

- `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`
- New `engine/runtime/include/mirakana/runtime/sprite_atlas_residency.hpp`
- New `engine/runtime/src/sprite_atlas_residency.cpp`
- `engine/tools/include/mirakana/tools/sprite_atlas_tool.hpp`
- `engine/renderer/include/mirakana/renderer/sprite_batch.hpp`
- `tests/unit/runtime_sprite_atlas_residency_tests.cpp`
- `tests/unit/assets_sprite_atlas_packing_tests.cpp`
- `CMakeLists.txt` targets `MK_runtime_sprite_atlas_residency_tests` and existing `MK_assets_tests`
- `games/sample_2d_desktop_runtime_package/runtime/`

**Contract to add:**

- `RuntimeSpriteAtlasPageRow`
- `RuntimeSpriteAtlasResidencyRequest`
- `RuntimeSpriteAtlasResidencyPlan`
- `RuntimeSpriteAtlasResidencyUploadHandoffRow`
- `plan_runtime_sprite_atlas_residency`

**Steps:**

1. Add RED tests for multi-page atlas metadata, duplicate page ids, missing cooked texture payloads, invalid UV/padding rectangles, stale resource handles, and resident-byte budget failure.
2. Keep source image decoding out of runtime. Consume only reviewed cooked texture/runtime package payloads.
3. Add multi-page/padded atlas page metadata to package-visible rows while preserving existing single-page source authoring helper behavior.
4. Produce renderer upload handoff rows without taking renderer/RHI ownership in `MK_runtime`.
5. Add selected D3D12 package smoke counters. Vulkan/Metal counters stay host-gated until their phase-specific backend evidence lands.
6. Update explicit non-claims for runtime source decode, production atlas editor UX, renderer residency ownership, and backend parity.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sprite_atlas_residency_tests MK_assets_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_sprite_atlas_residency_tests|MK_assets_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','3','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-2d-sprite-atlas-residency') }"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** the selected package reports ready atlas residency, positive page rows, positive upload-handoff rows, clean diagnostics, no runtime source parsing, no renderer/RHI native handle exposure, and no backend readiness inference.

**Implementation Evidence (2026-06-22):**

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev; if ($LASTEXITCODE -eq 0) { pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sprite_atlas_residency_tests }` configured successfully, then failed because `mirakana/runtime/sprite_atlas_residency.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sprite_atlas_residency_tests; if ($LASTEXITCODE -eq 0) { pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_sprite_atlas_residency_tests }` passed after adding `RuntimeSpriteAtlasResidency*` value contracts, cooked payload validation, stale/mismatched resource handle rejection, budget diagnostics, and upload-handoff descriptor rows.
- Source package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime; if ($LASTEXITCODE -eq 0) { pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package }` passed, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package.*smoke"` passed 4/4.
- Installed package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','3','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-2d-sprite-atlas-residency') }"` passed and reported `2d_sprite_atlas_residency_status=ready`, `2d_sprite_atlas_residency_ready=1`, `2d_sprite_atlas_residency_page_rows=1`, `2d_sprite_atlas_residency_upload_handoff_rows=1`, `2d_sprite_atlas_residency_resident_bytes=4`, `2d_sprite_atlas_residency_diagnostics=0`, `2d_sprite_atlas_residency_invoked_runtime_source_decode=0`, `2d_sprite_atlas_residency_requested_renderer_residency_ownership=0`, `2d_sprite_atlas_residency_requested_public_native_handle=0`, and `2d_sprite_atlas_residency_invoked_renderer_upload=0`.
- Manifest/static sync: `games/sample_2d_desktop_runtime_package/game.agent.json` now records `spriteAtlasRuntimeResidency` and `installed-2d-sprite-atlas-residency-smoke`; `tools/validate-installed-desktop-runtime.ps1` validates the required counters; `tools/check-ai-integration-080-scaffold-smokes.ps1` guards the flag, API call, counter, CMake smoke args, and manifest recipe.
- Static/public checks: `git diff --check`, `tools/check-text-format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-toolchain.ps1`, and `tools/check-agents.ps1` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-06-22; static checks were ok and CTest reported `100% tests passed, 0 tests failed out of 135`.
- IP provenance: `external_code_copied=0`, `external_assets_copied=0`, `third_party_api_surface_copied=0`, `third_party_ui_layout_copied=0`, `third_party_trademark_public_api=0`, `license_records_updated=not_applicable`, `official_docs_used_for_category_mapping_only=1`.
- Publication: `tools/check-publication-preflight.ps1`, branch push, PR #721, and publication closeout PR #724 are complete.
- Slice close: PR #721 merged to `origin/main` at merge commit `3b4fe82e`; PR #724 merged publication evidence closeout at merge commit `23e56be8`; Phase 3 branch `codex/2d-dense-sprite-throughput` has merged that baseline.

### Phase 3: `2d-dense-sprite-throughput-v1`

**Target modules:**

- `engine/renderer/include/mirakana/renderer/sprite_batch.hpp`
- `engine/renderer/src/sprite_batch.cpp`
- `engine/runtime/include/mirakana/runtime/entity_scale_culling.hpp`
- `engine/scene_renderer/`
- `games/sample_2d_desktop_runtime_package/`
- New `tools/validate-2d-sprite-throughput.ps1`
- `tests/unit/renderer_sprite_batch_tests.cpp`
- `tests/unit/runtime_entity_scale_culling_tests.cpp`
- `CMakeLists.txt` target `MK_renderer_2d_sprite_throughput_tests` plus existing `MK_runtime_entity_scale_culling_tests`

**Contract to add or extend:**

- `SpriteBatchProductionThroughputDesc`
- `SpriteBatchProductionThroughputPlan`
- `SpriteBatchProductionWorkloadRow`
- `plan_sprite_batch_production_throughput`

**Workload rows:**

- `dense_arena_512`: 512 visible sprites, 1 atlas page, 1 material lane.
- `dense_arena_4096`: 4096 visible sprites, up to 4 atlas pages, 3 material lanes.
- `projectile_storm_12000`: 12000 logical projectile rows, culling enabled, draw-intent rows capped by package budget.

**Steps:**

1. Add RED tests for stable sorting, material/atlas grouping, budget rejection, lane overflow, and replay-stable workload summaries.
2. Connect entity scale/culling rows to sprite draw-intent rows without scene mutation or renderer ownership in `MK_runtime`.
3. Emit CPU frame, draw row, instance row, upload byte, atlas page, and over-budget counters.
4. Store retained trace/profile artifact paths only when host measurement exists; otherwise report host-gated rows without ready promotion.
5. Keep visual quality, subjective art approval, Metal parity, and broad renderer quality outside the claim.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_2d_sprite_throughput_tests MK_runtime_entity_scale_culling_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_2d_sprite_throughput_tests|MK_runtime_entity_scale_culling_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-sprite-throughput.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** required workload rows exist, the selected package stays within declared budgets, diagnostics are zero, retained timing artifacts exist for any claimed measured row, and backend-specific rows do not infer cross-backend parity.

**Evidence as of 2026-06-22:**

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` followed by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_2d_sprite_throughput_tests MK_runtime_entity_scale_culling_tests` failed as expected before production code because `SpriteBatchProduction*` and runtime sprite draw-intent contracts did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_2d_sprite_throughput_tests MK_runtime_entity_scale_culling_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_2d_sprite_throughput_tests|MK_runtime_entity_scale_culling_tests"` passed after adding `SpriteBatchProductionThroughput*`, `plan_sprite_batch_production_throughput`, `RuntimeEntityScaleCullingSpriteDrawIntent*`, and `plan_runtime_entity_scale_culling_sprite_draw_intents`.
- Package validator: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-sprite-throughput.ps1 -RequireReady` passed, including `desktop-runtime-release` package build, installed SDK validation, installed `sample_2d_desktop_runtime_package --require-2d-sprite-throughput` smoke, and CPack. The installed smoke proved `2d_sprite_throughput_status=ready`, `2d_sprite_throughput_ready=1`, 3 workload rows, dense 512/4096 visible sprite rows, 12000 logical projectile rows, 768 visible projectile rows, 11232 culled projectile rows, 14 draw rows, 5376 instance rows, 319488 upload bytes, 6 atlas-page rows, 5 material-lane rows, 1 measured workload row, 2 host-gated workload rows, 1 retained timing artifact, retained hash 9817, `diagnostics=0`, `over_budget_rows=0`, clean culling rows, and zero scene mutation, renderer ownership, native handle access, broad optimization, cross-backend parity, and Metal readiness counters.
- Manifest/static sync completed locally: `games/sample_2d_desktop_runtime_package/game.agent.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, composed `engine/agent/manifest.json`, `tools/validate-installed-desktop-runtime.ps1`, `tools/check-ai-integration-080-scaffold-smokes.ps1`, and `tools/check-ai-integration-020-engine-manifest.ps1` carry `installed-2d-sprite-throughput-smoke`, `--require-2d-sprite-throughput`, and selected counter needles.
- Slice-close local gates: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-text-format.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-toolchain.ps1`, and full `tools/validate.ps1` passed on 2026-06-22 with CTest 139/139 passing.
- Publication: PR #725 head `ba7dbc55` passed hosted PR Gate, Windows MSVC, Linux CMake/Coverage/ASan/Vulkan, static shards, iOS simulator, iOS Metal, macOS Metal CMake, and CodeQL, then merged to `origin/main` at merge commit `ee5c8ebf`. PR #728 closed the Phase 3 publication evidence on `origin/main` at merge commit `bcc027d5`; latest `origin/main` was merged into the Phase 4 branch before Phase 4 publication.

### Phase 4: `2d-physics-runtime-extension-v1`

**Target modules:**

- `engine/physics/include/mirakana/physics/physics2d.hpp`
- `engine/physics/src/physics2d.cpp`
- `engine/runtime/include/mirakana/runtime/physics_collision_runtime.hpp`
- `engine/runtime/src/physics_collision_runtime.cpp`
- `tests/unit/physics2d_runtime_extension_tests.cpp`
- `tests/unit/runtime_tests.cpp`
- `CMakeLists.txt` target `MK_physics2d_runtime_extension_tests` plus existing `MK_runtime_tests`
- `games/sample_2d_desktop_runtime_package/`

**Contract to add or extend:**

- `Physics2DContinuousCollisionRequest`
- `Physics2DTimeOfImpactRow`
- `Physics2DKinematicContactResolutionRow`
- `Physics2DJointRow`
- `Physics2DAreaTriggerEventRow`
- `simulate_physics2d_step`

**Steps:**

1. Add RED tests for circle-vs-circle, circle-vs-AABB, AABB-vs-AABB sweeps, no-hit rows, overlapping start rows, and row-budget rejection.
2. Add deterministic kinematic controller contact resolution over already authored shapes.
3. Add first-party 2D joint rows for distance, hinge, prismatic, and spring constraints with bounded iterations and deterministic diagnostics.
4. Add area/trigger callback rows as data rows, not arbitrary callbacks into gameplay code.
5. Wire selected 2D package counters behind `--require-2d-physics-runtime-extension`.
6. Keep optional Jolt or other middleware behind existing optional adapter boundaries; do not expose middleware/native handles.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics2d_runtime_extension_tests MK_runtime_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_physics2d_runtime_extension_tests|MK_runtime_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--max-frames','3','--require-2d-physics-runtime-extension') }"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** exact shape sweep/TOI rows, deterministic contact rows, joint rows, trigger rows, package counters, and zero native/middleware handle exposure are all validated.

**Phase 4 evidence (2026-06-22):**

- RED/GREEN tests added `MK_physics2d_runtime_extension_tests` for circle-vs-circle, circle-vs-AABB, AABB-vs-AABB exact sweeps, no-hit rows, initial-overlap rows, row-budget rejection, simulation-after-motion trigger enter/exit rows with rollback on trigger-row budget rejection, kinematic contact resolution with secondary slide blocker iteration, distance/hinge/prismatic/spring joint rows, trigger enter/exit data rows, and zero native/middleware dispatch counters.
- Package-visible evidence added `sample_2d_desktop_runtime_package --require-2d-physics-runtime-extension`, `installed-2d-physics-runtime-extension-smoke`, installed validator expectations, CMake smoke args, game manifest rows, and static AI integration needles. The selected package smoke proves `2d_physics_runtime_extension_status=ready`, `simulate_status=simulated`, 2 simulation runs, 5 TOI rows, 3 exact sweep shape-pair rows, 4 hit rows, 1 no-hit row, 1 initial-overlap row, 1 kinematic contact row, 4 joint rows across distance/hinge/prismatic/spring, 2 trigger event rows, clean diagnostics, `native_handle_exposure=0`, `middleware_dispatches=0`, and `dynamic_vs_dynamic_ccd_claimed=0`.
- Focused package validation: `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--max-frames','3','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-2d-physics-runtime-extension') }"` passed; CPack produced `Mirakanai-0.1.0-Windows-AMD64.zip` and installed desktop runtime validation reported `installed-desktop-runtime-validation: ok`.
- Slice-close local gates: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-text-format.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-toolchain.ps1`, focused `tools/check-tidy.ps1 -Files engine/physics/src/physics2d.cpp,tests/unit/physics2d_runtime_extension_tests.cpp`, focused `tools/check-tidy.ps1 -Preset desktop-runtime-release -Configuration Release -Files games/sample_2d_desktop_runtime_package/main.cpp`, focused build/CTest, package smoke, and full `tools/validate.ps1` passed on 2026-06-22 with CTest 141/141 passing after merging latest `origin/main` and rerunning the strengthened physics extension tests. Publication: PR #730 head `3f51abf6` passed hosted PR Gate, Windows MSVC, Linux CMake/Coverage/ASan/Vulkan, static shards, iOS simulator, iOS Metal, macOS Metal CMake, and CodeQL, then merged to `origin/main` at merge commit `6a8e9c6c`.

### Phase 5: `2d-input-device-production-ux-v1`

**Target modules:**

- `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`
- `engine/platform/include/mirakana/platform/input.hpp`
- `engine/platform/src/input.cpp`
- `editor/core/include/mirakana/editor/input_rebinding.hpp`
- `editor/core/src/input_rebinding.cpp`
- `tests/unit/runtime_tests.cpp`
- `tests/unit/editor_core_tests.cpp`
- `games/sample_2d_desktop_runtime_package/`

**Contract to add or extend:**

- `RuntimeInputGestureBindingRow`
- `RuntimeInputDeviceAssignmentRow`
- `RuntimeInputPerDeviceProfileRow`
- `RuntimeInputGlyphAssetLookupRow`
- `RuntimeInputKeyboardLayoutLabelRow`
- `plan_runtime_input_device_production_ux`

**Steps:**

1. Add RED tests for tap/double-tap/long-press/pan/swipe/pinch/rotate gesture mapping to `RuntimeInputActions`.
2. Add radial stick deadzone and response-curve metadata with deterministic clamping diagnostics.
3. Add symbolic platform glyph asset lookup rows without rendering glyphs or creating UI widgets.
4. Add keyboard-layout label rows that distinguish physical key, logical key, and display label.
5. Add multiplayer device assignment and per-device profile rows without native input handles.
6. Keep visible runtime/game rebinding panels, text/IME, accessibility, and UI focus rendering in the UI/editor roadmap.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_tests|editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--max-frames','3','--require-2d-input-device-production-ux') }"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** gesture/action rows, glyph lookup rows, keyboard layout rows, device assignment rows, and per-device profile rows are deterministic and package-visible, with UI rendering and native handles still unclaimed.

**Local evidence:**

- RED/GREEN tests added `MK_runtime_tests` coverage for tap, double-tap, long-press, pan, swipe, pinch, rotate gesture-to-action rows, duplicate rejection, device assignment rows, per-device profile rows with deadzone clamping diagnostics, symbolic glyph lookup rows, keyboard physical/logical/display label rows, invalid glyph/keyboard diagnostics, and explicit non-claims for native handles, UI rendering, input middleware, glyph rendering, and UI widgets.
- Runtime/package surface adds `RuntimeInputDeviceProductionUxPlan`, `RuntimeInputGestureBindingRow`, `RuntimeInputDeviceAssignmentRow`, `RuntimeInputPerDeviceProfileRow`, `RuntimeInputGlyphAssetLookupRow`, `RuntimeInputKeyboardLayoutLabelRow`, and `plan_runtime_input_device_production_ux`, plus `sample_2d_desktop_runtime_package --require-2d-input-device-production-ux`, installed validator expectations, CMake smoke args, game manifest recipe rows, and static AI integration needles.
- Focused validation on 2026-06-22 passed `tools/cmake.ps1 --build --preset dev --target MK_runtime_tests`, `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_runtime_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_runtime_tests$|^MK_editor_core_tests$"`, `tools/check-text-format.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` with `--require-2d-input-device-production-ux`. The package-visible counters prove `2d_input_device_production_ux_status=ready`, ready `1`, 7 gesture binding rows, 7 gesture event rows, 7 gesture action rows, 2 device assignments, 1 per-device profile, 2 glyph asset lookup rows, 2 keyboard layout label rows, zero diagnostics, zero clamped deadzones in the ready lane, and zero native-handle/UI/middleware/glyph-render/widget counters.
- Slice-close local gates passed after fast-forwarding to latest `origin/main`: composed manifest verification, `check-json-contracts`, `check-ai-integration`, `check-agents`, `check-text-format`, `check-format`, `check-public-api-boundaries`, focused clang-tidy for runtime/test/package files, focused build/CTest, package validation, and full `tools/validate.ps1` on 2026-06-22 with CTest 143/143 passing. Publication completed through PR #733 / merge commit `a199416c` after head `c870194b` passed hosted PR Gate, Windows MSVC, Linux CMake/Coverage/ASan/Vulkan, static shards, iOS simulator, iOS Metal, macOS Metal CMake, and CodeQL. Local `origin/main` synchronization verified the Phase 5 head reaches `origin/main`.

### Phase 6: `2d-production-workload-matrix-v1`

**Target modules:**

- `docs/specs/2026-06-19-2d-production-workload-matrix-v1.md`
- `game.agent.json` samples under `games/sample_2d_desktop_runtime_package/`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- New `tools/validate-2d-production-workloads.ps1`
- `tools/package-desktop-runtime.ps1`
- `docs/ai-game-development.md`

**Workloads:**

- `2d_dense_arena`: sprite throughput, projectiles, input actions, audio triggers.
- `2d_sandbox_tilemap`: existing sandbox/tile package, tile chunk renderer, safe-point streaming.
- `2d_ui_overlay`: runtime UI overlay counters only; no UI text/IME/accessibility promotion.
- `2d_hot_reload_package`: reviewed recook-to-resident replacement and external playtest evidence only.
- `2d_long_run_selected`: selected short long-run evidence plus optional `host-2d-long-run-readiness-soak`.

**Steps:**

1. Create a dated spec with exact budget metric names, artifact paths, required counters, and host-gated rows.
2. Add `game.agent.json.performanceBudgets` rows for all required workloads.
3. Add a fail-closed validator that reports missing artifacts, missing counters, over-budget rows, and unsupported broad-optimization claims.
4. Require retained trace/profile artifacts only for rows that claim measured timing evidence.
5. Keep broad optimization false unless every required row and artifact exists.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** the validator passes in `-RequireReady`, all required workload rows exist, all ready timing rows have retained artifacts, no over-budget row exists, and broad optimization remains unclaimed unless specifically proven.

- Local Phase 6 implementation adds [2D Production Workload Matrix v1](../../specs/2026-06-19-2d-production-workload-matrix-v1.md), `tools/validate-2d-production-workloads.ps1`, and `sample_2d_desktop_runtime_package` `performanceBudgets.workloadRows` for `2d-dense-arena`, `2d-sandbox-tilemap`, `2d-ui-overlay`, `2d-hot-reload-package`, and `2d-long-run-selected`.
- Initial focused package validation passed `tools/validate-2d-production-workloads.ps1` and `tools/validate-2d-production-workloads.ps1 -RequireReady` with selected package-visible counters for sprite throughput, sandbox/tilemap, runtime UI atlas handoff, performance baseline, long-run readiness, and 2D input-device production UX. Representative counters include `2d_sprite_throughput_workload_rows=3`, `2d_sprite_throughput_dense_arena_4096_visible_sprites=4096`, `2d_sprite_throughput_projectile_storm_logical_sprites=12000`, `2d_sprite_throughput_upload_bytes=319488`, `runtime_ui_renderer_atlas_handoff_atlas_budget_rows=2`, positive `long_run_readiness_memory_high_water_bytes`, and clean broad-optimization/backend/Metal/native-handle non-claim counters.
- Local slice-close gates passed on 2026-06-22, including composed manifest verification, `check-json-contracts`, `check-ai-integration`, `check-agents`, `check-text-format`, `check-format`, `tools/validate-2d-production-workloads.ps1 -RequireReady`, and full `tools/validate.ps1` with CTest 143/143 passing. Remaining publication gates are publication preflight, candidate commit, push, PR, hosted checks, merge, and local main synchronization.

### Phase 7: `2d-package-playtest-productization-v1`

**Target modules:**

- `engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp`
- `engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp`
- `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`
- `editor/core/include/mirakana/editor/play_in_editor.hpp`
- `editor/core/include/mirakana/editor/game_module_driver.hpp`
- `tests/unit/tools_runtime_hot_reload_package_tests.cpp`
- `tests/unit/runtime_package_hot_reload_*_tests.cpp`
- `tests/unit/editor_core_tests.cpp`
- `games/sample_2d_desktop_runtime_package/game.agent.json`
- New `tools/validate-2d-package-playtest-productization.ps1`

**Contract to add or extend:**

- `Runtime2DPackagePlaytestRecipeRow`
- `Runtime2DPackagePlaytestEvidenceRow`
- `Runtime2DPackagePlaytestFailureClassification`
- `plan_runtime_2d_package_playtest_productization`

**Steps:**

1. Compose existing generated-game playtest loop rows, package validation recipes, external runtime-host launch model rows, and hot-reload safe-point helper rows into one reviewed 2D package recipe.
2. Add failure classifications for missing package file, invalid scene binding, package load failure, shader/tool gap, counter mismatch, hot-reload recook failure, runtime replacement failure, and host-gated backend.
3. Add evidence import rows for stdout/stderr summaries, package-smoke counters, profile artifacts, and remediation handoff ids.
4. Keep editor-core as review/model only. Do not execute validation recipes from editor core, do not run arbitrary shell, and do not add active-session in-process hot reload.
5. Package this as a selected recipe only after the sample 2D package proves it.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_runtime_hot_reload_package_tests MK_runtime_package_hot_reload_candidate_review_tests MK_rt_pkg_hot_reload_repl_intent_review_tests MK_runtime_package_hot_reload_recook_change_review_tests MK_runtime_package_hot_reload_recook_replacement_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_tools_runtime_hot_reload_package_tests|MK_runtime_package_hot_reload_candidate_review_tests|MK_rt_pkg_hot_reload_repl_intent_review_tests|MK_runtime_package_hot_reload_recook_change_review_tests|MK_runtime_package_hot_reload_recook_replacement_tests|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Ready only when:** a selected 2D package can run the reviewed playtest recipe, classify failures deterministically, import evidence rows, execute only reviewed hot-reload safe points, and preserve all existing non-claims around native watchers, active-session hot reload, package scripts, and runtime-host embedding.

## Cross-Phase Validation Rules

- Every C++ behavior phase starts with a focused RED test or compile failure.
- Every phase ends with focused build/CTest, docs/manifest/static-check sync, and one full `tools/validate.ps1` unless the change is docs-only.
- Every package recipe must be fail-closed: missing counter, missing artifact, unsupported claim, or host-gated backend evidence keeps the row not ready.
- Every public API must stay first-party and backend-neutral.
- `engine/core` remains independent from OS APIs, GPU APIs, asset formats, and editor code.
- No phase may infer D3D12 readiness from Vulkan, Vulkan readiness from D3D12, or Metal readiness without Apple-host validation.
- No phase may mutate or delete user work in the main checkout. Use a branch/worktree per selected implementation phase.

## Completion Definition

This candidate milestone is complete only when all selected phases that remain in scope have:

- focused tests and package-visible counters,
- docs/spec/manifest/static-check updates,
- validation evidence recorded in the phase table,
- explicit non-claims preserved,
- no overlap with the active environment/UI/sandbox plans,
- `unsupportedProductionGaps` kept honest,
- full `tools/validate.ps1` evidence for any runtime/build/public-contract slice.

Until then, the correct answer for broad 2D production readiness is:

- selected 2D source/package foundations: ready or host-gated as currently documented,
- general tile/sandbox/UI/audio/input/sprite foundations: implemented as documented by their completed plans,
- broad production 2D engine loop: candidate work in this plan, not ready.
