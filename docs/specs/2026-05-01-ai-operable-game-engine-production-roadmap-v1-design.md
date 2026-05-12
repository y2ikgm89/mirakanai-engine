# AI-Operable Game Engine Production Roadmap v1 Design

## Goal

Make GameEngine a clean C++23 engine that can be operated by Codex or similar coding agents to create both 2D and 3D games. The target is not a Unity or Unreal clone. The target is a first-party engine whose scene, prefab, asset, package, gameplay, renderer, validation, and release contracts are explicit enough that an AI agent can understand the engine state, choose supported capabilities, place authored content, write game code, and run validation without guessing.

This roadmap intentionally does not preserve backward compatibility. GameEngine is still greenfield, and the next production phase should make clean breaking changes where the current MVP-era contracts would block a coherent production engine.

## Design Inputs

- User requirement from the planning thread: complete the engine so AI can operate it and build 2D and 3D games, including asset placement and C++ gameplay implementation.
- User preference: official best practices, clean implementation, no backward compatibility burden.
- Current repository state: `core-first-mvp` is closed as an MVP scope, not as commercial engine completeness.
- Local evidence from `docs/roadmap.md`, `docs/architecture.md`, `docs/ai-game-development.md`, `engine/agent/manifest.json`, `tools/agent-context.ps1`, sample games, editor-core authoring headers, runtime package code, and renderer/RHI code.
- Read-only subagent reviews:
  - `explorer`: AI contract and generated game workflow are strong, but the end-to-end game-spec-to-scene-to-package-to-visible-smoke recipe is still thin.
  - `engine-architect`: the next clean step is a production data spine with Scene/Component/Prefab v2, asset identity separation, AI command schemas, and resource handles.
  - `rendering-auditor`: D3D12 is the strongest lane, Vulkan is a gated proof, Metal is Apple-host-gated, and production renderer work still needs resource lifetime, frame graph, upload/staging, markers, and broader material/shader systems.

## Official Practice Anchors

Use these references as design anchors, not as implementations to copy:

- Unity Prefabs: reusable authored object templates and instance overrides should be first-class. Reference: <https://docs.unity.cn/2023.2/Documentation/Manual/Prefabs.html>
- Unity asset workflow: imported source assets should flow into engine-owned runtime assets through a documented import/cook pipeline. Reference: <https://docs.unity.cn/2021.1/Documentation/Manual/AssetWorkflow.html>
- Unreal Actors and Components: scene objects should be composed from components, with gameplay intent separate from renderer/backend ownership. References: <https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine> and <https://dev.epicgames.com/documentation/en-us/unreal-engine/basic-components-in-unreal-engine>
- CMake: keep target-based usage requirements, `PUBLIC`/`PRIVATE`/`INTERFACE` scopes, install/export targets, and `FILE_SET CXX_MODULES` policy centralized. Reference: <https://cmake.org/cmake/help/latest/>
- SDL3: SDL belongs behind platform/runtime-host adapters that own windows, events, audio streams, and native handles. Reference: <https://wiki.libsdl.org/SDL3/FrontPage>
- Vulkan: explicit synchronization, descriptors, resource ownership, memory limits, and validation must be reflected in RHI design instead of hidden behind ad hoc backend state. Reference: <https://docs.vulkan.org/>
- vcpkg: optional dependencies should stay in manifest features with a pinned baseline and matching legal records. Reference: <https://learn.microsoft.com/vcpkg/>

## Current Verdict

GameEngine is a strong MVP foundation, but it is not yet an AI-operable production engine.

Current strengths:

- `engine/agent/manifest.json` and `tools/agent-context.ps1` give AI agents a real machine-readable starting point.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1` can scaffold headless games and multiple desktop-runtime package variants.
- Desktop runtime packaging, selected target metadata, D3D12 package proof, and strict host-gated Vulkan package proof are concrete.
- `mirakana_runtime`, `mirakana_runtime_scene`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_scene_renderer`, and `mirakana_runtime_host_sdl3_presentation` form a usable cooked-package-to-renderer path.
- Editor-core has GUI-independent models for scene, prefab, material, and package authoring.
- Deterministic headless foundations exist for input, physics, navigation, behavior trees, animation, audio, runtime UI contracts, and diagnostics.

Current blockers:

- There is no single AI-safe production loop from game prompt to game recipe to scene/prefab/material/assets to package to gameplay code to validation.
- `mirakana_scene` is still a fixed optional-component model. Production 2D/3D games need stable ids, schema-driven components, extension points, and AI-friendly diffs.
- Prefab overrides are too index-oriented for nested prefab propagation, merge/conflict UX, and reliable AI editing.
- Asset identity is not separated enough from broader asset/import/cook concerns.
- Runtime resource lifetime and GPU resource lifetime are split across package stores, upload helpers, renderer state, and backend state.
- Frame graph, upload/staging, descriptor/resource lifetime, GPU markers, memory budgets, and allocator diagnostics are not production-grade.
- 2D production authoring is not complete: sprite import/cook, atlas, tilemap, 2D camera, sprite animation, and visible package validation need a first-class path.
- 3D production authoring is not complete: glTF skin/animation import, material variants, multi-light, PBR basics, GPU skinning, camera/controller, and 3D vertical slice authoring need a first-class path.
- Runtime UI has a good first-party contract, but production text, fonts, IME, accessibility, image decoding, atlas, and texture upload remain adapter work.
- Editor UX is still behind the underlying model work: hierarchy, inspector, viewport tools, asset browser, play-in-editor isolation, package review/apply, profiling/resource views, and visualization need productization.

## Strategic Order

Use the order `A -> C -> B`.

### A: AI-First Production Loop

Build the machine-readable contracts and command surfaces first. The AI should know what the engine supports, what is blocked, what validation proves a claim, and how to make safe dry-run/apply edits.

Required results:

- AI game recipe schema.
- AI command schema with dry-run/apply/undo/result diagnostics.
- Machine-readable production loop in `engine/agent/manifest.json`.
- Agent context output that exposes current recipes, gaps, validation recipes, and host gates.
- Static checks that reject stale or false capability claims.

### C: Playable 2D And 3D Vertical Slices

After the production loop contract is honest, build one 2D and one 3D vertical slice that prove AI can create real games through the engine instead of only headless samples.

Required results:

- 2D slice: camera2D, sprite/tilemap/atlas path, input actions, HUD, audio cue, cooked package, desktop runtime package smoke.
- 3D slice: static mesh scene, material instance, camera/controller, light/shadow/postprocess, cooked package, D3D12 primary package smoke, Vulkan strict gated package smoke.

### B: Editor Productization

The editor should productize the same first-party model that AI and headless tools use. It should not become the only way to author games.

Required results:

- Hierarchy, inspector, viewport gizmos, asset browser, package review/apply, play-in-editor isolation, profiler/resource panels.
- Dear ImGui remains an optional developer shell adapter.
- Runtime game UI stays on `mirakana_ui` contracts, not editor or Dear ImGui APIs.

## Clean Architecture Rules

- `engine/core` remains independent from OS APIs, GPU APIs, asset formats, editor code, SDL3, and Dear ImGui.
- Platform-specific behavior remains behind `engine/platform` and runtime-host adapters.
- Graphics APIs remain behind renderer/RHI interfaces and backend implementations.
- Gameplay-facing APIs must stay RHI-free and native-handle-free. GPU bindings belong to runtime/renderer/host adapters.
- Source assets are imported and cooked by tools/editor workflows. Runtime game code consumes cooked packages and first-party runtime handles.
- Editor-core owns GUI-independent authoring models and command semantics. Dear ImGui adapts those models.
- Optional dependencies must be dependency-gated in vcpkg features and documented in dependency and legal records.
- AI-facing capability claims must be machine-readable, validated, and conservative. Planned, blocked, and host-gated capabilities must not be described as ready.

## Target Architecture

### Production Data Spine

Create a clean spine that every authoring surface can share:

- `Scene/Component/Prefab Schema v2`
  - stable authoring ids for nodes and components
  - schema-driven component rows using `ComponentTypeId`
  - built-in data-only components for transform, camera, light, mesh, sprite, tilemap, audio cue, rigid body, collider, nav agent, animation, and UI attachment
  - non-throwing validation diagnostics
  - deterministic serialization and AI-friendly diffs
  - nested prefab and variant override paths based on stable ids, not indices
- `Asset Identity v2`
  - small identity layer for `AssetId`, `AssetKind`, asset URI, package reference rows, and dependency keys
  - `mirakana_scene`, renderer intent, UI image refs, and gameplay components depend on identity only
  - import/cook/material/shader/package logic stays in assets/tools/editor modules
- `Runtime Resource v2`
  - generational resource handles
  - package mounts
  - resident resource cache
  - safe-point release and hot reload
  - memory budgets and diagnostics
  - clear ownership between runtime packages, CPU resources, GPU resources, and backend resources

### AI Command Layer

Build an AI command layer above editor-core and tools:

- `CommandDescriptor`: machine-readable command id, schema version, inputs, outputs, required modules, validation recipes, host gates, and capability gates.
- `CommandRequest`: structured request with project/game path, dry-run/apply mode, and typed payload.
- `CommandDryRunResult`: planned file/model mutations, warnings, errors, validation recipes, and unsupported capability rows.
- `CommandApplyResult`: applied mutations, undo token, diagnostics, and follow-up validation commands.
- `UndoToken`: editor-core undo integration without exposing UI implementation.
- Structured diagnostic codes that tests and AI can match.

Initial commands should cover:

- create or update game recipe
- create scene
- add node
- add/update component
- create prefab
- instantiate prefab
- create material instance
- register source asset for import
- cook package
- register runtime package files
- update `game.agent.json`
- run selected validation recipe

### Renderer And GPU Foundation

Renderer production work should follow this order:

1. Resource lifetime foundation: persistent destroy, deferred deletion, owner registry, residency counters, debug names, and marker adapter hooks.
2. Frame Graph v1: pass execution callbacks, resource declarations, barrier planning, transient allocation, aliasing policy, and queue scheduling model.
3. Upload/Staging v1: upload ring, staging pool, batched copies, fence retirement, partial updates, texture mips/formats/compression metadata, and async-ready design.
4. Material/Shader Authoring v1: cook-time binding metadata, shader artifact validation, pipeline layout policy, sampler/texture slots, pipeline cache, and hot reload.
5. Backend promotion: D3D12 production hardening, Vulkan multi-draw/multi-pass/multi-present constraints removed, Metal validated on Apple hosts before any ready claim.

### Memory And Diagnostics Foundation

Memory work should be first-party and dependency-light:

- allocator policy and subsystem budget model
- bounded diagnostics for allocations, resource residency, package memory, upload memory, and transient GPU memory
- leak and lifetime checks in tests
- trace export rows for resource and upload events
- no global mutable allocator singleton in public gameplay APIs

### 2D Production Spine

2D should become a first-class engine lane:

- `SpriteComponent`, `SpriteAnimationComponent`, `TilemapComponent`, `Camera2DComponent`, and 2D layer/sort metadata
- sprite source import/cook path
- texture atlas and sprite frame metadata
- tilemap source/cooked contract
- `SpriteBatch` or scene-renderer path that stays RHI-free at gameplay level
- desktop runtime package validation with visible sprite output or strict renderer command/readback proof

### 3D Production Spine

3D should become a first-class engine lane:

- static mesh scene authoring with material instances
- 3D camera/controller path
- light/shadow/postprocess authoring to package
- skeletal animation import/rendering path after static slice is stable
- GPU skinning/upload path after resource lifetime and upload/staging foundations
- D3D12 primary, Vulkan strict gated, Metal Apple-host-gated until validated

## Production Roadmap Waves

### Wave 0: Governance And Truthfulness

Goal: keep docs, manifests, schemas, skills, subagents, and validation recipes synchronized.

Done when:

- stale AI guidance is rejected by static checks
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass
- host-gated items stay explicit

### Wave 1: AI-Operable Production Loop Contracts

Goal: make the engine's production workflow visible to AI as data.

Done when:

- `engine/agent/manifest.json` exposes an `aiOperableProductionLoop` or equivalent machine-readable section
- agent context includes production recipes, gaps, host gates, command surfaces, and validation recipes
- game prompt pack and generated-game scenarios point to supported recipes
- first checks reject unsupported recipe claims

### Wave 2: Scene/Component/Prefab Schema v2

Goal: replace fixed optional scene components and index-based prefab overrides with stable, schema-driven authoring.

Done when:

- scene nodes and components have stable authoring ids
- built-in component schemas cover 2D/3D starter needs
- serialization is deterministic
- prefab variants use stable override paths
- editor-core and samples are migrated without compatibility shims

### Wave 3: Asset Identity And Runtime Resource v2

Goal: separate asset identity from import/cook systems and give runtime resources explicit ownership.

Done when:

- scene/render/UI/gameplay references depend on asset identity only
- runtime resource handles are generational
- package mount, residency, safe-point unload, and hot reload state are test-covered
- memory/resource diagnostics are visible to agent context and editor-core

### Wave 4: Renderer/RHI Resource Foundation

Goal: make GPU resource lifetime explicit and auditable.

Done when:

- persistent resource destruction and deferred deletion are implemented
- GPU resource ownership is registry-backed
- debug names, markers, budgets, and residency counters exist behind backend-neutral contracts
- D3D12 tests prove the primary lane and Vulkan/Metal remain gated honestly

### Wave 5: Frame Graph v1 And Upload/Staging v1

Goal: move fixed smoke sequencing into a production render graph and replace immediate waits with upload infrastructure.

Done when:

- frame graph owns pass execution, resource declaration, barriers, transient allocation, and diagnostics
- upload/staging supports batching and fence retirement
- current postprocess/shadow samples run through the graph without exposing backend handles

### Wave 6: 2D Playable Vertical Slice

Goal: prove AI can create a small 2D game through first-party authoring, runtime, and package contracts.

Done when:

- scaffold or command recipe creates a 2D game with sprites/tilemap, input, HUD, audio cue, and package files
- D3D12 or renderer command/readback proof validates visible 2D output
- `game.agent.json` mirrors the selected backend/import/package capabilities

### Wave 7: 3D Playable Vertical Slice

Goal: prove AI can create a small 3D game through first-party authoring, runtime, renderer, and package contracts.

Done when:

- scaffold or command recipe creates a 3D game with static meshes, material instances, camera/controller, lights, shadow/postprocess, and package files
- D3D12 package validation passes
- Vulkan strict validation is available only when host/toolchain gates pass

### Wave 8: Editor Productization

Goal: make editor UX a product layer over the same AI/headless authoring model.

Done when:

- hierarchy, inspector, viewport gizmos, content browser, package review/apply, play-in-editor isolation, profiler, and resource panels operate on editor-core models
- editor actions expose the same command/dry-run/apply diagnostics as AI tooling
- runtime UI remains separate from Dear ImGui

### Wave 9: Production Adapters

Goal: add dependency-gated adapters after first-party contracts are stable.

Done when:

- text shaping, font rasterization, glyph atlas, image decode, accessibility bridge, codecs, glTF skin/animation import, and optional physics middleware are added only through audited dependencies
- `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` are updated before use

### Wave 10: Platform Release And Operations

Goal: make release, diagnostics, and platform readiness honest.

Done when:

- Android, iOS, desktop packaging, crash/telemetry adapters, GPU markers, allocator diagnostics, and profiling workflows have per-host validation recipes
- Apple/iOS/Metal readiness is claimed only from macOS/Xcode or CI evidence

## Validation Policy

Default validation for planning or docs:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

When public headers or backend interop surfaces change:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

When renderer, RHI, shader, material, or package-visible GPU behavior changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

When desktop runtime packages change:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
tools/package-desktop-runtime.ps1 -GameTarget <target>
```

When dependency-gated work changes:

```powershell
tools/check-dependency-policy.ps1
```

## Subagent And Context7 Policy

Use subagents when the user explicitly authorizes parallel or delegated work. Recommended roles for this roadmap:

- `explorer`: read-only current-state tracing
- `engine-architect`: clean architecture sequencing
- `rendering-auditor`: renderer/RHI/GPU resource risk review
- `cpp-reviewer`: final code review
- `build-fixer`: build/test failure investigation
- `gameplay-builder`: scoped game/sample implementation

Use Context7 or official documentation before making design claims about CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, C++ tooling, or platform SDK behavior. Repository architecture rules still win when official samples expose native handles in places that GameEngine intentionally hides behind adapters.

## First Focused Plan

The first implementation slice is:

- `docs/superpowers/plans/2026-05-01-ai-operable-production-loop-foundation-v1.md`

That slice should be narrow: docs, manifest/schema, agent-context output, checks, and prompt/scenario guidance. It should not yet rewrite Scene v2, Resource v2, Frame Graph v1, or playable vertical slices.

## Reusable Next-Chat Prompt

Use the dedicated handoff prompt:

- `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`

