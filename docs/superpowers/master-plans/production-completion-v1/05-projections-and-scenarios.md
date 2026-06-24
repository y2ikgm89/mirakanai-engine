# Production Completion v1 - Projections, Official Gates, And Scenarios

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). This file consolidates the former thin projection chapters so agents load one short projection surface instead of many routing files.

Canonical capability rows remain in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). This file owns cross-cutting reading rules, official-practice gates, projections, and scenario probes only.

## Official Practice Review Gates

Each child plan that touches the listed surface must include a short "official practice check" before implementation and record the source in its validation evidence. These checks are planning gates, not broad ready claims:

- **CMake / install / package:** keep project-wide presets in tracked `CMakePresets.json`, local choices in ignored `CMakeUserPresets.json`, target-based usage requirements, installed headers/modules, `install(EXPORT ...)`, `find_package(Mirakanai)` consumer validation, CTest, and CPack in the repository wrappers. C++ module work must keep `FILE_SET CXX_MODULES` install/export behavior explicit.
- **vcpkg / dependencies:** stay in manifest mode with `builtin-baseline`, additive manifest features, audited optional dependencies, and user/CI-local binary cache configuration. Dependency installation belongs to `tools/bootstrap-deps.ps1`; CMake configure must not restore or download packages.
- **Asset/import:** keep Khronos glTF/KTX2/Basis validation, optional codec/importer dependencies, shader compiler invocation, and package mutation as reviewed gates. A broad import claim needs explicit source roots, importer ids, selected extensions, package rows, license/provenance/hash evidence, validator/dependency/legal rows, exact `vcpkg.ktx` dependency evidence before KTX2/Basis readiness, exact `toolchain.dxc` plus `toolchain.spirv-tools` dependency evidence before shader rows are ready, offline shader command review rows, host gates, and execution-readiness rows that separate reviewed execution from dependency-gated, host-gated, package-mutation, and unsupported-claim evidence before promotion.
- **Desktop platform adapters:** keep Win32, Raw Input, clipboard/dialog, DXGI/D3D12 presentation, and WASAPI calls inside first-party platform/runtime-host/audio adapters, copy OS-owned text or message payloads before retention, enable text input only for the active text target/window, and keep platform APIs out of `MK_ui`, gameplay, and `engine/core`.
- **D3D12 / Vulkan / Metal:** require backend-private resource ownership, barriers/synchronization, queue/fence timelines, upload/staging retirement, debug names/markers, and validation-layer or host-tool evidence before promotion. D3D12 may be the Windows primary lane; Vulkan and Metal must not inherit D3D12 ready claims without their own strict host evidence.
- **Android / Apple release lanes:** Android work must use the GameActivity/Prefab/NDK path accepted by the Android docs and must not store signing keys or SDK caches in the repo. Apple/iOS/Metal readiness requires macOS/Xcode/Simulator or device evidence; Windows diagnostics can only record host-gated blockers for those lanes.
- **AI/operator command surfaces:** every convenience feature must be a reviewed dry-run/apply or reviewed execution surface with typed diagnostics, host-gate acknowledgement where needed, undo/remediation rows when it mutates content, and manifest/static-check synchronization.
- **General-purpose production surfaces:** large-world, addressable-content, data-oriented scale, and runtime UI work may cite vendor engine guidance for architecture comparison, but implementation must stay first-party, backend-neutral, and free of Unity/Unreal/Godot API vocabulary unless a first-party contract explicitly adopts it.

Source anchors for these checks: [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html), [CMake packages](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html), [CMake install/export](https://cmake.org/cmake/help/latest/command/install.html), [Microsoft vcpkg manifest mode](https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode), [vcpkg CMake integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration), [vcpkg binary caching](https://learn.microsoft.com/en-us/vcpkg/reference/binarycaching), [Microsoft Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input), [Microsoft WASAPI](https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi), [Microsoft DXGI](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dx-graphics-dxgi), [Microsoft Direct3D 12 resource barriers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12), [Microsoft Direct3D 12 multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization), [Khronos Vulkan synchronization guide](https://docs.vulkan.org/guide/latest/synchronization.html), [Khronos Vulkan validation overview](https://docs.vulkan.org/guide/latest/validation_overview.html), [Apple Metal resource synchronization](https://developer.apple.com/documentation/metal/resource-synchronization), [Apple Metal feature set tables](https://developer.apple.com/metal/capabilities/), [Android GameActivity](https://developer.android.com/games/agdk/game-activity), [Unreal Engine World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine), [Unity Addressables](https://docs.unity3d.com/Packages/com.unity.addressables@latest), [Unity Entities](https://docs.unity3d.com/Packages/com.unity.entities@latest), and [Unity UI Toolkit](https://docs.unity3d.com/Manual/UIElements.html). Use newer official docs when a child plan is authored after this ledger edit.

Concrete evidence requirements:

- **CMake / package:** cite the exact preset, install/export, package config/version, CTest/CPack, and C++ module file-set behavior used by the slice; avoid user-package-registry side effects unless explicitly reviewed.
- **vcpkg:** keep dependency state reproducible through manifest mode and pinned baselines, keep binary cache configuration user/CI-local, and keep all vcpkg-affecting CMake variables set before `project()`.
- **Asset/import:** use `mirakana::review_asset_import_production_readiness` as a value-only promotion gate when broad asset/import evidence is being reviewed; treat dependency-gated, host-gated, package-mutation, and unsupported-claim readiness rows as non-ready evidence, and do not infer importer execution, KTX2/Basis adapter readiness, broad codec support, runtime source parsing, external downloads, package mutation, or live shader generation from descriptor rows.
- **Desktop platform adapters:** keep Win32 message/input, clipboard, file dialog, DXGI surface, and WASAPI stream/device ownership inside platform/runtime-host/audio adapters; copy OS-owned text before retention; expose only first-party value rows and opaque handles across module boundaries.
- **D3D12:** record explicit resource-state, aliasing, queue, fence, and present-state evidence; use common-state promotion/decay only when the plan explains why it is valid, and avoid excessive transitions back to `COMMON`.
- **Vulkan:** require `VK_LAYER_KHRONOS_validation` or explicit host-gated validation evidence for promoted claims, and record synchronization2 / image-layout / queue-family assumptions when renderer or upload work changes.
- **Metal / Apple:** require Apple-host evidence, resource synchronization reasoning, and feature-family/capability checks before any Metal ready claim; Windows-only evidence can only leave a host-gated blocker.
- **Android:** require the GameActivity/Prefab/NDK path, `game-activity` CMake target evidence, Android manifest/library-name alignment, and no repository-stored signing keys, SDKs, Gradle caches, or device images.

## Documentation authority stack

Do **not** rely on bulk edits to this ledger or to completed dated child plans as the primary way to track physical tree rules. Use the current stack: manifest and this master plan for **what** is in focus and **what** is ready; specs/ADRs for **invariants**; the plan registry for active navigation; archived historical plan snapshots for **evidence**, not living path catalogs.

## AI Autonomous Game Creation

## Purpose

This chapter is the AI game-creation projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It owns game-creation boundaries, official agent-surface rules, and remediation behavior. It does not own a separate backlog table.

The autonomous game-creation lane is game-surface only by default: agents may generate and update game files, game code, game manifests, source assets, placeholder assets, package registration rows, validation evidence, and repo-owned operation commands. They must not modify engine internals, renderer/RHI/backend code, platform adapters, build-system contracts, toolchain policy, or shared agent surfaces unless the operator starts a separate engine-feature task.

## Official Agent-Surface Anchors

- OpenAI Codex and Codex CLI behavior must be checked against official OpenAI documentation when changing Codex-facing workflows.
- Claude Code memory, subagents, settings, permissions, hooks, and skills must be checked against official Anthropic documentation when changing Claude-facing workflows.
- Cursor-facing behavior must stay aligned with repository `.cursor/` rules, skills, agents, and the shared AGENTS baseline.
- Repository automation must stay on reviewed `tools/*.ps1` entrypoints. Autonomous workflows must not rely on broad shell permissions, untracked user settings, or hidden local state.

## AI Game-Creation Rules

- Start from `engine/agent/manifest.json`, `game.agent.json`, and `tools/agent-context.ps1 -ContextProfile Minimal|Standard`, not broad source-tree exploration.
- Keep game-owned mutation under `games/<game_name>/`, game-local source asset roots, game-local docs/evidence, and reviewed generated/runtime package registration rows.
- Treat `engine/`, `editor/`, shared `tools/`, schemas, skills, rules, subagents, CMake/vcpkg, and validation policy as read-only unless a separate engine-change plan is selected.
- Create or change game content through reviewed dry-run/apply tools, typed command rows, or documented file formats.
- Require `game.agent.json.aiWorkflow.gameDesignSpec` before generated 2D/3D package expansion: gameplay family, template, camera, input map, core loop, win/loss/restart, scene list, asset requests, systems, package targets, validation recipes, quality gates, and unsupported claims.
- Use first-party or reviewed placeholder assets with provenance/license rows. Do not copy web, store, sample, blog, or repository assets without explicit license evidence.
- Record validation/playtest evidence, classify failures, and repair through remediation recipes. Do not loosen validation or delete evidence to pass.
- If a requested game needs unsupported engine capability, emit a typed developer handoff that references a canonical row in `04`.

## AI Projection

| Workflow concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Design before generation | `ai-game-design-spec-v1` | Fail-closed `game.agent.json.aiWorkflow.gameDesignSpec` schema/static checks, examples for 2D/3D templates, and validation that required gameplay/package fields and same-manifest package/recipe references exist. |
| Generation orchestration | `ai-game-generation-orchestrator-v1` | Dry-run/apply rows, deterministic file lists, generated 2D/3D package evidence, and no arbitrary shell. |
| Safe mutation boundaries | `ai-safe-content-mutation-ledger-v1` | `game.agent.json.aiWorkflow.contentMutationLedger` schema/static checks for generated 2D/3D package games, including game-local roots, generated files, reviewed command surfaces, forbidden shared paths, remediation rows, and failure when AI-owned mutation surfaces drift. |
| Placeholder assets | `ai-placeholder-asset-pipeline-v1`, `engine-asset-placeholder-generation-v1` | Fail-closed `game.agent.json.aiWorkflow.placeholderAssetPipeline` rows, provenance/license status, package handoff rows, validation recipe ids, runtime package files, hash/eol checks, and package-visible placeholder evidence. |
| Playtest and remediation | `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1` | Fail-closed `game.agent.json.aiWorkflow.generatedGamePlaytestLoop` and `game.agent.json.aiWorkflow.validationRemediationRecipes` rows, reviewed validation recipe selection, recipe output summaries, gameplay counters, package smoke logs, failure classifications, required remediation evidence, stop conditions, mutation-ledger remediation rows, rerun selected validation recipes, and no validation weakening. |
| Quality gate | `ai-generated-game-quality-rubric-v1` | Fail-closed `game.agent.json.aiWorkflow.generatedGameQualityRubric` rows link same-manifest design/playtest/remediation/validation/report ids to objective, controls, feedback, fail/restart, deterministic package smoke, and budget evidence gates with unsupported rows for qualities not proven. |
| Agent-surface alignment | `ai-codex-claude-agent-surface-v1` | Implemented `engine/agent/manifest.json.aiSurfaces.crossToolAlignment` with OpenAI Codex, Claude Code, and Cursor official docs anchors, game-owned write scopes, reviewed command surfaces, validation guards, required read-only roles, forbidden broad grants, and unsupported claims. |
| Developer handoff | `ai-engine-capability-handoff-v1` | `EngineCapabilityHandoffRequestRow` / `review_engine_capability_handoff_request` and optional `game.agent.json.aiWorkflow.engineCapabilityHandoffs` rows include requested capability, current workaround, blocked feature, affected game files, desired public contract, and required evidence. |

## Missing-Capability Handoff

When a game-creation agent hits a missing reusable engine capability, it must stop at a handoff/remediation row. The row should name the requested game feature, the closest supported workaround, the affected game files, the desired first-party API or data contract, and the evidence needed before future agents can use the capability. The AI Engine Capability Handoff v1 review API fails closed if the row invents a non-canonical capability id, omits required handoff fields, or leaks native handles, SDL3, Dear ImGui, renderer/RHI, backend, platform, or middleware types into the desired public contract.

Only a developer-owned engine-feature task may change engine internals and promote the new capability back through the manifest, schema, docs, skills, and validation recipes.

## 2D / 3D Capability Coverage Matrix

## Purpose

This chapter is a projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It does not own capability rows. Add or change capability ids in `04`, then update this matrix only when the 2D/3D coverage view changes.

Coverage statuses:

| Status | Meaning |
| --- | --- |
| `covered-foundation` | Current engine evidence supports a narrow, validated use. |
| `post-1x-row` | Use the named canonical backlog row before broad ready claims. |
| `host-gated` | Requires backend, platform, SDK, device, or toolchain evidence. |
| `game-owned` | Belongs in `games/<game_name>/` unless it becomes a reusable engine primitive. |

## Current Recommended Implementation Sequence

[First-Party UI Editor Production Stack v1](../../plans/2026-05-31-first-party-ui-editor-production-stack-v1.md) is completed; `recommendedNextPlan.id = next-production-gap-selection` after the closeout returned execution to the production-completion master-plan selection gate with `unsupportedProductionGaps = []`. It follows [First-Party Editor Shell v1](../../plans/2026-05-31-first-party-editor-shell-v1.md), which replaced the Dear ImGui-backed editor shell with a first-party retained editor shell. Completed editor/UI scope includes first-party editor docking, rich text, Windows DirectWrite/TSF/UIA adapter evidence, D3D12 viewport/material texture display, AI-operable editor UX rows, reviewed rich-text copy commands, and cross-platform adapter future gates without compatibility shims, UI middleware, or public native handles. Broad editable rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, native IME candidate UI, reconversion, full UIA control pattern/event parity, cross-platform editor/accessibility parity, Vulkan/Metal texture-display parity, broader material-preview parity, production font fallback, and runtime UI OS accessibility publication remain future selections. Generic 2D Sandbox Production Lane v1 completed through [Sandbox World Package Validation And Performance Budgets v1](../../plans/2026-05-30-sandbox-world-package-validation-performance-budgets-v1.md) / PR #313 after Generic 2D Sandbox Streaming Residency v1, Generic 2D Sandbox Persistence Snapshot v1, Generic 2D Sandbox Tile Simulation Primitives v1, Generic 2D Sandbox Mutation Execution v1, [Renderer Backend Parity Host Recipe Proof v1](../../plans/2026-05-29-renderer-backend-parity-host-recipe-proof-v1.md), [Generic 2D Sandbox Runtime Foundation v1](../../plans/2026-05-29-generic-2d-sandbox-runtime-foundation-v1.md), [Renderer Postprocess Tone Mapping Evidence v1](../../plans/2026-05-29-renderer-postprocess-tone-mapping-evidence-v1.md), [Renderer Backend Parity Metal Apple Evidence v1](../../plans/2026-05-29-renderer-backend-parity-metal-apple-evidence-v1.md), [Physics Navigation Commercial Coverage v1](../../plans/2026-05-29-physics-navigation-commercial-coverage-v1.md), [First-Party Desktop Platform And SDL3 Removal v1](../../plans/2026-05-27-first-party-desktop-platform-sdl3-removal-v1.md), and [Reviewed Importers Codecs And Shader Generation v1](../../plans/2026-05-27-reviewed-importers-codecs-shader-generation-v1.md), and returned to the selection gate before the editor replacement was selected. [Engine 1.0 Gap Matrix v1](../../plans/2026-05-26-engine-1-0-gap-matrix-v1.md), [Renderer Production VFX Profiling Host Gate v1](../../plans/2026-05-26-renderer-production-vfx-profiling-host-gate-v1.md), [Engine General Production Quality Expansion v1](../../plans/2026-05-26-engine-general-production-quality-expansion-v1.md), [Renderer Production Quality And Backend Parity v1](../../plans/2026-05-27-renderer-production-quality-backend-parity-v1.md), [Runtime UI Text Platform Stack v1](../../plans/2026-05-27-runtime-ui-text-platform-stack-v1.md), [First-Party Desktop Platform And SDL3 Removal v1](../../plans/2026-05-27-first-party-desktop-platform-sdl3-removal-v1.md), and [Reviewed Importers Codecs And Shader Generation v1](../../plans/2026-05-27-reviewed-importers-codecs-shader-generation-v1.md) remain completed evidence.

1. Keep `sprite-collision-hitbox-v1`, `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, and `sprite-effects-particles-v1` out of new implementation selection unless a future row intentionally expands them beyond `implemented-1x-foundation`.
2. No concrete `renderer-backend-parity-v1` Apple/Metal child step is currently selected. Retained Apple/Metal evidence includes [Renderer Backend Parity Apple Evidence Closeout v1](../../plans/2026-06-21-renderer-backend-parity-apple-evidence-closeout-v1.md), [Renderer Backend Parity Apple Memory Profiling Proof Rows v1](../../plans/2026-06-24-renderer-backend-parity-apple-memory-profiling-proof-rows-v1.md), and [Renderer Metal Memory Profiling Apple Host Artifacts v1](../../plans/2026-06-24-renderer-metal-memory-profiling-apple-host-artifacts-v1.md). Any future Metal parity proof row must name `host_validation_recipe_id` with a reviewed validation recipe from `shader-toolchain`, `mobile-packaging`, `renderer-metal-apple-host-evidence`, or `ios-simulator-smoke`; missing recipes fail closed with `missing_host_validation_recipe`, unreviewed valid ids fail closed with `unreviewed_host_validation_recipe`, and Windows/Vulkan proof must not promote Metal readiness.
3. Treat `Engine General Production Quality Expansion v1` as completed evidence for renderer quality matrix, runtime UI text/font/IME/accessibility contract rows, broad reviewed import/cook, physics/navigation breadth, networking execution/security, audio production, cross-domain package evidence, and final agent-surface closeout. Future work uses the clean-break master plan child sequence rather than reopening that milestone.
4. Select the next post-1.0 capability from the production-completion master plan through a new dated plan; do not reopen `first-party-ui-editor-production-stack-v1` for unrelated follow-up work. Do not treat the completed editor milestone as ready evidence for broad editable rich text, cross-platform editor parity, public native handle access, Vulkan/Metal texture-display parity, broader material-preview parity, or broad runtime UI parity until focused phases validate those claims.

## 2D Coverage Projection

| Capability area | Current boundary | Canonical rows to select when broader support is needed |
| --- | --- | --- |
| 2D scaffold/package | `covered-foundation`: generated 2D package recipes create, validate, and package game-owned files without engine edits. | `ai-game-generation-orchestrator-v1`, `ai-generated-game-quality-rubric-v1` |
| Sprite atlas, animation, and batching | `covered-foundation`: selected package proof, atlas/image foundations, sprite animation counters, and native sprite execution counters exist. | `sprite-atlas-authoring-v1`, `sprite-batching-renderer-v1`, `sprite-animation-flipbook-v1`, `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, `sprite-effects-particles-v1` |
| Tilemaps and 2D levels | `covered-foundation`: tilemap metadata and package counters exist for the supported generated path. | `engine-scene-gameplay-binding-v1`, `engine-gameplay-interaction-framework-v1`, `world-streaming-and-large-scenes-v1`, `generic-2d-sandbox-streaming-residency-v1` |
| 2D collision, hitboxes, and triggers | `covered-foundation`: frame-authored hitbox/hurtbox planning, physics query foundations, and gameplay interaction rows exist for selected generated/package paths. | `sprite-collision-hitbox-v1`, `physics-collision-query-v1`, `engine-gameplay-interaction-framework-v1` |
| HUD, menus, dialogue, and rebinding | `covered-foundation`: runtime UI and input context foundations exist, but polished game menus remain a selected 1.x feature. | `engine-ui-game-menu-hud-v1`, `engine-input-action-contexts-v1`, `engine-quest-dialogue-state-v1` |
| Audio, feedback, and SFX | `covered-foundation`: gameplay audio mix planning exists without broad codec claims. | `engine-audio-gameplay-mixer-v1`, `sprite-effects-particles-v1` |
| Save, progression, and reset | `covered-foundation`: session/profile documents plus selected package resume evidence exist; broader simulation persistence, real save UX, cloud/binary saves, and migration execution remain selected rows. | `engine-save-settings-profile-v1`, `simulation-persistence-v1` |
| Dense 2D arena or projectile games | `covered-foundation`: use archetypes only as evidence probes unless future rows add broader performance or gameplay execution claims. | `engine-entity-scale-and-culling-v1`, `sprite-batching-renderer-v1`, `sprite-collision-hitbox-v1`, `gameplay-simulation-orchestration-v1` |

## 3D Coverage Projection

| Capability area | Current boundary | Canonical rows to select when broader support is needed |
| --- | --- | --- |
| 3D scaffold/package | `covered-foundation`: generated 3D package recipes create, validate, and package scene/mesh/material/game-owned files. | `ai-game-generation-orchestrator-v1`, `ai-generated-game-quality-rubric-v1` |
| Mesh, material, and shader package path | `covered-foundation`: current proof is package-scoped. | `renderer-modern-materials-v1`, `material-shader-graph-production-v1`, `renderer-backend-parity-v1` |
| Lighting, shadows, and postprocess | `covered-foundation` on selected D3D12/Vulkan lanes; tone-mapping evidence rows are completed through `Renderer Postprocess Tone Mapping Evidence v1`; `host-gated` for broader backend claims. | `renderer-lighting-shadows-v1`, `renderer-postprocess-v1`, `renderer-backend-parity-v1` |
| 3D camera, controller, and movement | `covered-foundation`: movement, controller, and policy counters exist for selected scenarios. | `physics-character-dynamics-v1`, `engine-input-action-contexts-v1`, `engine-scene-gameplay-binding-v1` |
| 3D physics and collision queries | `covered-foundation`: deterministic query, joint, constraint, kinematic, and simple vehicle policy foundations exist. | `physics-collision-query-v1`, `physics-constraints-and-joints-v1`, `physics-vehicles-and-kinematics-v1`, `native-physics-middleware-adapter-v1` |
| 3D navigation and AI movement | `covered-foundation`: navmesh/crowd foundations exist with selected package evidence. | `navigation-navmesh-v1`, `navigation-crowd-local-avoidance-v1`, `navigation-hierarchical-world-v1`, `ai-behavior-authoring-v1`, `ai-perception-services-v1` |
| Animation, skeletal, and morph basics | `covered-foundation`: package/proof-scoped animation, skinning, morph, IK, and root-motion foundations exist. | `renderer-backend-parity-v1`, `material-shader-graph-production-v1`, `engine-scene-gameplay-binding-v1` |
| Open-ended 3D worlds | `post-1x-row`: no open-world claim from single-scene proof. | `engine-world-region-streaming-v1`, `engine-entity-scale-and-culling-v1`, `world-streaming-and-large-scenes-v1`, `simulation-persistence-v1`, `navigation-hierarchical-world-v1` |
| Multiplayer or networked 3D | `post-1x-row`: local gameplay readiness is not network readiness. | `engine-networking-foundation-v1`, `networking-and-multiplayer-v1`, `gameplay-simulation-orchestration-v1`, `simulation-persistence-v1` |

## Editing Rule

This matrix should stay short. If a future review finds a missing reusable engine capability, add or update one row in `04-developer-owned-engine-capability-backlog.md`, then point this matrix at that id. Do not add genre-specific backlog tables here.

## Renderer Advanced Production Track

## Purpose

This chapter is the renderer projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It owns renderer implementation rules and evidence boundaries, not the canonical list of capability rows.

## Renderer Rules

- Keep public gameplay, scene, material, UI, and package APIs backend-neutral.
- Keep D3D12, Vulkan, Metal, PIX, RenderDoc, Xcode/Metal capture, descriptor heaps, command queues, command buffers, fences, semaphores, heaps, argument buffers, and native handles behind backend-private adapters or an accepted opaque interop design.
- Treat each backend independently. D3D12 package proof does not imply Vulkan or Metal readiness.
- Cite current official backend documentation in the selected dated plan before changing synchronization, memory, shader, or presentation behavior.
- Performance claims require timestamp/profile evidence, deterministic budgets, and failure diagnostics.
- Keep runtime package renderer proof, editor preview, viewport tooling, capture handoff, and material-authoring UX as separate claims.

## Renderer Projection

| Renderer concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Modern materials and shader authoring | `renderer-modern-materials-v1`, `material-shader-graph-production-v1` | Backend-specific shader compiler evidence, package-visible material counters, pipeline-cache diagnostics, and reviewed compile requests. |
| Lighting and shadows | `renderer-lighting-shadows-v1` | Backend-neutral light-list contracts, per-backend package proof, GPU counters, and explicit resource-state/layout evidence. |
| Postprocess and image quality | `renderer-postprocess-v1` | Frame graph pass rows, render-pass/load-store policy, texture lifetime/aliasing evidence, package-visible quality counters, backend shader validation, and selected tone-mapping evidence rows through `PostprocessToneMappingEvidencePlan` / `plan_postprocess_tone_mapping_evidence`; actual tone-mapping execution and subjective quality remain separate evidence gates. |
| Scene scale and draw throughput | `renderer-scene-scale-v1`, `engine-entity-scale-and-culling-v1` | Deterministic culling/LOD tests, draw-call/instance counters, CPU/GPU timing rows, and no public native handles. |
| GPU memory and residency | `renderer-gpu-memory-v1`, `world-streaming-and-large-scenes-v1` | Heap/resource evidence per backend, package-visible budget diagnostics, and safe failure/remediation rows. |
| Backend parity | `renderer-backend-parity-v1` | Strict Vulkan validation/SPIR-V evidence and Apple-host Metal/Xcode evidence per promoted feature. |
| Debug and profiling | `renderer-debug-profiling-v1` | PIX, Vulkan debug utils / validation layers, Apple Metal/Xcode capture evidence, and first-party trace/profile export rows. |
| 2D sprite renderer production | `sprite-batching-renderer-v1`, `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, `sprite-effects-particles-v1` | Atlas/material grouping, draw/instance budgets, render-pass evidence, and per-backend package counters. |

When a renderer row is selected, create or update a dated capability/milestone plan. Do not broaden ready claims, expose native handles, or mark host-gated backend work as ready without focused validation evidence.

## Gameplay, Physics, Navigation, And AI Advanced Track

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

## General-Purpose / High-Freedom Game Creation Track

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
| AI-authored games | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-safe-content-mutation-ledger-v1`, `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1`, `ai-generated-game-quality-rubric-v1`, `ai-codex-claude-agent-surface-v1`, `ai-engine-capability-handoff-v1`, `generated-game-studio-v1` | Design-spec to game-owned content flow, mutation ledger, validation recipes, remediation rows, quality-rubric gates, studio-loop evidence aggregation, cross-tool agent-surface alignment, and no engine-internal edits. |
| Scripting and modding | `engine-scripting-sandbox-v1`, `scripting-and-mod-sandbox-v1` | Reviewed sandbox policy rows, caller-owned adapter dispatch, execution budgets, deterministic replay signatures, package counters, no filesystem/network/process access by default, and no bundled VM/runtime dependency until a separate adapter decision. |
| Multiplayer | `engine-networking-foundation-v1`, `networking-and-multiplayer-v1`, `gameplay-simulation-orchestration-v1`, `simulation-persistence-v1` | Separate architecture/security plan, threat model, replay/determinism proof, transport/session gates, and no broad multiplayer claim from local gameplay. |

When a high-freedom wave is selected, prefer one dated milestone plan that states the target freedom level, supported gameplay families, AI-operable authoring surface, persistence model, validation recipes, and explicit non-goals.

## Generated Game Studio Track

## Purpose

This chapter is the active AI game-creation studio track. It projects the canonical AI game workflow rows into one operator-visible milestone that can iterate generated 2D and 3D package games without granting engine-internal write scope to game-creation agents.

The track is an orchestration and review surface, not a new renderer, runtime, package, or editor-productization claim. It must use existing first-party descriptors, package review rows, validation preflight rows, evidence import rows, remediation queue rows, quality-rubric rows, and command-panel rows before adding broader capability.

## Generated Studio Rules

- Start every session from a selected `game.agent.json.aiWorkflow.gameDesignSpec` row and same-manifest package/validation recipe ids.
- Keep all game-content mutation under reviewed command surfaces and mutation-ledger rows; shared `engine/`, `editor/`, `tools/`, schemas, docs, and agent surfaces require a separate developer-owned task.
- Treat host-gated D3D12/Vulkan/Metal/mobile/desktop package evidence as external operator evidence until the relevant validation recipe passes.
- Route failed, blocked, missing, or host-gated evidence into remediation queue and handoff rows; do not weaken validation or delete evidence.
- Keep the editor-core studio model read-only and deterministic. Execution stays in reviewed validation runners and editor-shell host controls.
- If a requested generated-game behavior needs a missing reusable engine capability, emit `ai-engine-capability-handoff-v1` evidence against a canonical backlog row.

## Generated Studio Projection

| Studio concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Track selection and governance | `generated-game-studio-v1` | Dated milestone plan, manifest active pointer, updated AI game-creation/backlog/projection docs, static checks, and explicit non-goals. |
| 2D generated package loop | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1`, `ai-generated-game-quality-rubric-v1` | Reviewed 2D design/spec rows, `DesktopRuntime2DPackage` scaffold/package rows, validation recipe preflight, imported evidence rows, remediation/handoff rows, and quality gates without engine internals or arbitrary shell. |
| 3D generated package loop | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `production-rendering-vfx-profiling-v1`, `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1`, `ai-generated-game-quality-rubric-v1` | Reviewed 3D design/spec rows, selected 3D package rows, host-gated renderer evidence rows, imported package-smoke evidence, remediation/handoff rows, and explicit Metal/broad-renderer non-goals. |
| Editor/operator workflow | `generated-game-studio-v1`, existing `MK_editor_core` AI playtest package review and command panel models | GUI-independent editor-core value contract plus retained `generated_game_studio` UI/editor-shell evidence that summarizes session status, package readiness, validation preflight, reviewed commands, evidence, remediation, handoff, unsupported claims, and diagnostics without executing commands from editor core. |

## General-Purpose Game Production Track

## Purpose

This chapter is the completed fifth production track. It projects the `General-Purpose Game Production` canonical rows from [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md) into a post-foundation milestone that can support RPG, sandbox, simulation, and broad 2D/3D game creation without reopening completed foundation rows.

This track is not a genre-specific backlog. It promotes only reusable production primitives: scheduler, world/entity model, addressable content, authoring workflows, runtime UI, genre-pack validation surfaces, network replication, and renderer/VFX/profiling evidence.

## General-Purpose Production Rules

- Keep completed `implemented-1x-foundation` rows closed. New breadth belongs to `production-candidate`, `selected-production-slice`, `implemented-production-surface`, or `host-gated-production` rows.
- Prefer clean breaking public contracts over compatibility shims when a foundation-era value-only API blocks a production data model.
- Keep production systems first-party and backend-neutral. Public APIs must not expose SDL3, Dear ImGui, native OS handles, renderer internals, RHI handles, backend types, or middleware types.
- Split reusable engine production primitives from genre content. Stats, schedules, inventory, jobs, chunks, and dashboards can be engine primitives; story, balance, enemy names, biome rules, and economy numbers stay game-owned.
- Use vendor engine documentation as comparison evidence only. Unreal World Partition, Unity Addressables, Unity Entities, and Unity UI Toolkit should inform boundaries and validation expectations, not dictate API names.
- Each production phase must prove at least one generated or sample package counter set before making a broader ready claim.

## General-Purpose Production Projection

| Production concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Track selection and governance | `general-purpose-game-production-v1` | Dated milestone plan, manifest active pointer, updated backlog/projection docs, official practice anchors, and docs/static checks. |
| Authoritative game loop | `gameplay-runtime-scheduler-production-v1` | Implemented first-party `MK_runtime` scheduler value contract for fixed-tick system order, command playback, replay hashes, pause/step policy, budget diagnostics, and selected 2D/3D package counters; rollback/network hooks remain design extension points without transport claims. |
| World and entity ownership | `world-entity-model-production-v1` | Implemented first-party `MK_runtime` world/entity value contract for stable ids, component schema rows, region ownership, lifecycle diagnostics, persistence bridge rows, streaming bridge rows, and selected 2D/3D package counters; scene mutation, lifecycle execution, package IO, streaming execution, renderer/RHI work, threads, and native handles remain outside the claim. |
| Addressable content and streaming | `addressable-content-streaming-production-v1` | Implemented first-party `MK_runtime` address/dependency/refcount/load/release plans over reviewed package rows, resident budget diagnostics, selected 2D/3D package counters, and host-owned async/IO execution boundaries. |
| Production authoring | `production-authoring-workflows-v1` | Implemented first-party `MK_tools` production workflow review with accepted workflow rows, mutation-ledger rows, one selected validation-repair row, selected 2D package counters, and fail-closed shared-surface/arbitrary-shell/cooked-package/native-backend/non-game-target diagnostics without file mutation, package IO, command execution, native handles, or autonomous content generation. |
| Runtime UI workbench | `production-runtime-ui-workbench-v1` | Implemented first-party `MK_ui` workbench value contract for dense menu, inventory, equipment, shop, simulation dashboard, table, graph, focus, text-input, localization, and accessibility identity rows with selected 2D package counters and adapter-boundary evidence. |
| RPG production surface | `genre-rpg-systems-pack-v1` | Implemented first-party `MK_runtime` `plan_runtime_rpg_systems` value contract for generic stats/progression/skills/equipment/combat/reward/save-validation evidence across selected 2D/3D `rpg_systems_*` package smokes without engine-owned story, balance, combat execution, reward application, save IO, package/world mutation, renderer/RHI work, or native handles. |
| Sandbox production surface | `genre-sandbox-world-pack-v1`, `generic-2d-sandbox-streaming-residency-v1` | Implemented first-party `MK_runtime` `plan_runtime_sandbox_world_mutation` value contract for chunk/cell ownership, placement/destruction intent, construction costs, generic tile-drop/tool/spawn/day-night/trigger hook rows, mutation rows, persistence review, repairable persistence evidence, unsafe mutation rejection, positive replay hash, and selected 2D/3D `sandbox_world_*` package counters. `Generic 2D Sandbox Gameplay Integration Rows v1` adds `RuntimeSandboxTileDropRow`, `RuntimeSandboxConstructionCostConsumptionRow`, `RuntimeSandboxToolEffectivenessRow`, `RuntimeSandboxSpawnRegionRow`, `RuntimeSandboxDayNightEventRow`, and `RuntimeSandboxTriggerRow` with boss/NPC/damage-formula/economy hook rejection for game-owned inventory, crafting, AI, spawning, and interaction consumers. `Generic 2D Sandbox Runtime Foundation v1` adds `build_runtime_sandbox_world`, `sample_runtime_sandbox_cell`, and `snapshot_runtime_sandbox_world` for deterministic in-memory runtime world snapshots over reviewed chunk/cell rows. `Generic 2D Sandbox Mutation Execution v1` adds `RuntimeSandboxWorldMutationExecutionStatus`, `RuntimeSandboxWorldDirtyRegion`, `RuntimeSandboxWorldMutationExecutionResult`, and `apply_runtime_sandbox_world_mutations` for copied-world placement/destruction execution, dirty regions, replay hashes, applied/rejected counts, zero side-effect flags, and invalid-plan input preservation. `Generic 2D Sandbox Persistence Snapshot v1` adds `RuntimeSandboxWorldPersistenceDocumentPlan`, `RuntimeSandboxWorldMigrationReviewPlan`, `RuntimeSandboxWorldAtomicSavePlan`, `plan_runtime_sandbox_world_persistence_document`, `plan_runtime_sandbox_world_snapshot_diff`, `review_runtime_sandbox_world_migration`, and `plan_runtime_sandbox_world_atomic_save` for canonical snapshot rows, dirty-chunk diffs, schema migration/recovery review, safe project-relative atomic-save operation intent, replay hashes, and zero filesystem/platform/thread side-effect flags. `Generic 2D Sandbox Streaming Residency v1` adds `RuntimeSandboxWorldStreamingPlan`, `plan_runtime_sandbox_world_streaming`, and `execute_runtime_sandbox_world_streaming_safe_point` for sandbox-specific source-range chunk selection, dirty resident chunk pins, addressable dependencies, resident budget diagnostics, and reviewed world-region safe-point adoption. The surface still excludes actual persistence IO, broad/background streaming, async addressable execution, automatic LRU eviction, biome/block-art ownership, inventory/crafting/spawn/dialogue/AI execution, renderer/platform/editor APIs, threads, native handles, legacy desktop middleware, and game-specific content rules. |
| Sandbox tile simulation primitives | `generic-2d-sandbox-tile-simulation-primitives-v1` | Implemented first-party `MK_runtime` `RuntimeSandboxTileSimulationPlan` / `plan_runtime_sandbox_tile_simulation` value contract for tile material rows, deterministic solid/platform collision spans, liquid and trigger cells, light propagation rows, bounded liquid-flow candidates, scheduled tile update rows, replay hashes, fail-closed diagnostics, and zero physics/renderer/platform/thread side-effect flags over already-built runtime sandbox worlds. Physics upload, renderer lightmap upload, full fluid simulation quality, persistence IO, package loading, streaming execution, renderer/platform/editor APIs, native handles, legacy desktop middleware, and game-specific content rules remain outside the claim. |
| Simulation production surface | `genre-simulation-management-pack-v1` | Implemented first-party `MK_runtime` `plan_runtime_simulation_management` value contract for jobs, resources, logistics transfers, economy summaries, population needs, schedules, deterministic long-run replay, save-review rows, and dashboard handoff counters across selected 2D/3D `simulation_management_*` package smokes without economy/job/logistics execution, save IO, UI rendering, package/world/scene mutation, renderer/platform/editor APIs, threads, native handles, or game balance/content ownership. |
| Networked production surface | `production-network-replication-v1` | Implemented first-party `MK_runtime` value contract through `plan_runtime_network_replication` for authoritative replication session review, object/input/snapshot/rollback rows, replay hashes, `host_evidence_required` package counters, and optional transport evidence rows; socket IO, rollback/prediction execution, secure transport execution, matchmaking/NAT traversal, native handles, and broad multiplayer readiness remain separate host-gated work. |
| Rendering/VFX/profiling production breadth | `production-rendering-vfx-profiling-v1` | Implemented first-party `MK_renderer` `plan_renderer_production_vfx_profiling` value contract for GPU particle budget rows, postprocess rows, backend timing rows, crash/telemetry handoff rows, backend parity diagnostics, host-gated Metal evidence, selected generated 3D `rendering_vfx_profiling_*` package counters, selected debug profiling CPU profile zone / trace handoff / package counter evidence, and selected GPU memory budget / residency pressure / package counter evidence without GPU command recording, native capture execution, crash upload execution, public native handles, backend performance parity, Metal readiness, or broad renderer quality. |
| Renderer General Quality Matrix v1 production matrix | `renderer-general-quality-matrix-v1` | Implemented first-party `MK_renderer` `plan_renderer_quality_matrix` value contract for backend-local renderer quality rows across materials, lighting/shadows, postprocess, sprite/UI, scene scale, GPU memory/residency, and profiling/capture, with explicit ready/host-gated/dependency-gated/unsupported row taxonomy, selected generated 3D `renderer_quality_matrix_status=host_evidence_required`, `renderer_quality_matrix_dependency_gated_rows=0`, and `renderer_quality_matrix_unsupported_rows=0` package counters, D3D12/Vulkan readiness, Metal host-gated rows, `renderer_quality_matrix_general_renderer_quality_ready=0`, and no native capture/crash upload/public native handle/broad renderer quality claim. |

When this track is active, the dated milestone plan owns sequencing. Keep each implementation PR focused on one production surface, but keep shared governance, docs, manifest, and validation context in the milestone rather than creating unrelated micro-plans.

## 2D Sprite Production Pipeline Track

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

## Gameplay Archetype Validation Scenarios

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
