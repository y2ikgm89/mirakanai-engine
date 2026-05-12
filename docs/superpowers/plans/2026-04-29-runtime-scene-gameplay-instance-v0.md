# Runtime Scene Gameplay Instance v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let gameplay instantiate cooked `RuntimeAssetPackage` scene payloads into a safe `mirakana::Scene` instance without depending on renderer, RHI, editor, platform windows, SDL3, Dear ImGui, or native handles.

**Architecture:** Add a new `mirakana_runtime_scene` bridge module that depends only on `mirakana_runtime` and `mirakana_scene`. It owns deserialization, scene asset lookup, scene reference enumeration, package-reference validation, node-name lookup, and deterministic diagnostics. `mirakana_scene_renderer` keeps renderer-specific material palette and render packet behavior, but delegates package scene instantiation to `mirakana_runtime_scene`.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_scene`, `mirakana_scene_renderer`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_runtime` already loads cooked package indexes, validates package records, exposes typed scene payload metadata, and resolves material dependencies without depending on `mirakana_scene`.
- `mirakana_scene` already owns scene hierarchy, components, serialization, deserialization, and renderer-neutral render packet extraction.
- `mirakana_scene_renderer` currently owns both gameplay-neutral runtime scene deserialization and renderer-facing material palette/render packet construction. Splitting the scene instantiation bridge gives generated games a renderer-free way to load authored/cooked scene state.

## Constraints

- Keep `engine/runtime` independent from `mirakana_scene`, renderer, RHI, editor, platform windows, SDL3, Dear ImGui, and native handles.
- Keep `engine/scene` independent from `mirakana_runtime`, renderer, RHI, editor, and platform code.
- `mirakana_runtime_scene` may depend on `mirakana_runtime` and `mirakana_scene` only.
- `mirakana_scene_renderer` may depend on `mirakana_runtime_scene`, but renderer/RHI-specific behavior must remain outside `mirakana_runtime_scene`.
- Do not add source import, scripting, ECS scheduling, physics integration, navigation integration, play-in-editor isolation, asset cooking, RHI upload, GPU binding, or editor visualization to this slice.
- Keep public APIs under `mirakana::` and avoid native/backend handles.

## Done When

- [x] `mirakana_runtime_scene` is an exported SDK target with installed public headers.
- [x] `mirakana::runtime_scene::instantiate_runtime_scene` loads a cooked scene payload into `mirakana::Scene` and reports deterministic diagnostics for missing scene asset, wrong kind, malformed payload/deserialization, missing references, wrong reference kinds, and duplicate node names when requested.
- [x] `mirakana::runtime_scene::RuntimeSceneInstance` records the scene asset, runtime handle, deserialized scene, and mesh/material/sprite references discovered from scene components.
- [x] `mirakana::runtime_scene::find_runtime_scene_nodes_by_name` returns all matching node ids in scene order without requiring unique names.
- [x] `mirakana_scene_renderer` delegates runtime scene deserialization/reference validation to `mirakana_runtime_scene` while preserving material palette resolution, render packet construction, and existing diagnostics.
- [x] Focused runtime-scene and scene-renderer tests, public API boundary, schema/agent/format checks, tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Runtime Scene Tests

**Files:**
- Create: `tests/unit/runtime_scene_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add tests that include `mirakana/runtime_scene/runtime_scene.hpp` before the production header exists.
- [x] Register `mirakana_runtime_scene_tests` in the default CTest lane.
- [x] Verify the focused build fails because the new public header/target is missing.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_runtime_scene_tests
```

Expected failure: compiler cannot open `mirakana/runtime_scene/runtime_scene.hpp` or CMake cannot resolve the not-yet-created runtime scene target.

### Task 2: Runtime Scene Bridge Module

**Files:**
- Create: `engine/runtime_scene/CMakeLists.txt`
- Create: `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- Create: `engine/runtime_scene/src/runtime_scene.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `mirakana_runtime_scene` as a target that links publicly to `mirakana_runtime` and `mirakana_scene`.
- [x] Export and install the target/header through the root package configuration.
- [x] Implement value-type diagnostics and load result contracts in namespace `mirakana::runtime_scene`.
- [x] Implement `instantiate_runtime_scene` with scene asset lookup, kind validation, typed payload validation, `deserialize_scene` exception capture, reference enumeration, optional package-reference validation, and optional duplicate node-name diagnostics.
- [x] Implement `find_runtime_scene_nodes_by_name`.

### Task 3: Runtime Scene Test Coverage

**Files:**
- Modify: `tests/unit/runtime_scene_tests.cpp`

- [x] Cover successful scene instantiation from a cooked package.
- [x] Cover missing scene asset.
- [x] Cover wrong asset kind.
- [x] Cover malformed scene payload and deserialization errors.
- [x] Cover missing mesh/material/sprite references while still returning a parsed scene instance.
- [x] Cover wrong mesh/material/sprite reference kinds.
- [x] Cover duplicate node-name diagnostics only when `require_unique_node_names` is enabled.
- [x] Cover deterministic replay and node-name lookup returning all matches in scene order.

### Task 4: Scene Renderer Delegation

**Files:**
- Modify: `engine/scene_renderer/CMakeLists.txt`
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`

- [x] Link `mirakana_scene_renderer` against `mirakana_runtime_scene`.
- [x] Convert `mirakana::runtime_scene::RuntimeSceneDiagnostic` rows into existing `RuntimeSceneRenderLoadFailure` rows.
- [x] Load material payloads from material references discovered by `mirakana_runtime_scene`.
- [x] Keep `RuntimeSceneRenderLoadResult` and `RuntimeSceneRenderInstance` behavior compatible for existing package render paths.
- [x] Add or update focused tests proving malformed scene, missing reference, material palette, and render packet behavior still work through the delegated bridge.

### Task 5: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] Describe `mirakana_runtime_scene` as a renderer-free cooked scene instantiation bridge, not an editor, ECS, physics, navigation, scripting, or RHI integration layer.
- [x] Keep `mirakana_scene_renderer` documented as the renderer-facing render packet/material palette bridge.
- [x] Keep manifest capability status honest: implemented after validation only; runtime gameplay scene loading is separate from authored scene editing and packaging.

### Task 6: Verification

- [x] Run focused runtime-scene build and CTest.
- [x] Run focused scene-renderer build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED: after adding `mirakana_runtime_scene_tests` before the production header/target existed, `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_runtime_scene_tests` failed as expected because `mirakana/runtime_scene/runtime_scene.hpp` was missing.
- Review-driven RED: `cpp-reviewer` found that diagnostics for repeated bad references on different nodes were collapsed and duplicate-name diagnostics did not point at the source scene asset. Added tests reproduced both failures before the implementation fix.
- Focused validation: `mirakana_runtime_scene_tests`, `mirakana_scene_renderer_tests`, and `mirakana_runtime_scene_rhi_tests` build and pass through CTest.
- Boundary and docs validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` passed or recorded the known strict-tidy compile database diagnostic-only gate.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `mirakana_runtime_scene_tests` included in the default CTest lane. Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode, Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy analysis lacks a compile database for the Visual Studio generator.
