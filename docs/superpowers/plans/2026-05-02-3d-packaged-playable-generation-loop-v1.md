# 3D Packaged Playable Generation Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make AI-generated packaged 3D games use reviewed source, scene, cook, package, manifest, runtime validation, and host-gated desktop package validation surfaces instead of staying limited to the fixed `sample_desktop_runtime_game` package proof.

**Architecture:** Reuse the reviewed command surfaces and manifest descriptors from the 2D packaged loop: `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `validate-runtime-scene-package`, `register-runtime-package-files`, and `run-validation-recipe`. Keep the first 3D uplift to one deterministic generated package with camera/controller/light/static mesh/material intent, explicit package files, `runtimeSceneValidationTargets`, and package smoke validation; keep renderer/RHI residency, package streaming, editor productization, material/shader graphs, live shader generation, native handles, Metal readiness, and general renderer quality out of the ready claim.

**Tech Stack:** C++23 generated game scaffolding, `tools/new-game.ps1`, `game.agent.json`, `mirakana_scene`, `mirakana_assets`, `mirakana_tools`, `mirakana_runtime`, `mirakana_runtime_scene`, `mirakana_scene_renderer`, existing desktop runtime package scripts, static checks, docs, and validation recipes.

---

## Goal

Raise practical 3D game-engine completeness by proving a generated packaged 3D workflow through existing reviewed contracts:

- scaffold or generate a packaged 3D game manifest with runtime package files and `runtimeSceneValidationTargets`
- register first-party source rows for static mesh, texture, material, and Scene v2 authoring inputs where applicable
- cook explicitly selected registered source assets into deterministic cooked artifacts and `.geindex` rows
- migrate supported Scene v2 camera/light/mesh rows to the current runtime-loadable Scene v1 package surface
- validate the explicit runtime `.geindex` plus scene asset through `validate-runtime-scene-package`
- run desktop runtime package validation on the D3D12 primary lane and keep Vulkan/Metal gates honest

## Context

The engine currently has a host-gated `3d-playable-desktop-package` proof centered on `sample_desktop_runtime_game`. The preceding 2D packaged loop should establish a repeatable generated-game workflow. This plan applies that pattern to a generated static-mesh 3D package before editor playtest/package review, renderer resource residency/upload execution, and material/shader authoring.

## Constraints

- Do not add third-party dependencies or assets.
- Do not parse source assets at runtime; gameplay consumes cooked packages only.
- Do not make broad dependency cooking ready. Dependencies must be selected explicitly until a later focused plan implements traversal.
- Do not claim renderer/RHI residency, package streaming, material graph, shader graph, live shader generation, editor productization, public native/RHI handles, Metal readiness, or general production renderer quality.
- Keep generated game code on public `mirakana::` APIs.
- Keep D3D12, Vulkan, and desktop runtime package proofs host-gated where runtime/toolchain support is required.

## Done When

- A focused RED -> GREEN record exists in this plan.
- A generated or sample packaged 3D game has a manifest, runtime package files, and `runtimeSceneValidationTargets`.
- Focused tests or static checks prove the generated game includes deterministic package files, explicit validation target rows, and no source authoring files in `runtimePackageFiles`.
- The workflow validates through `validate-runtime-scene-package` before package smoke.
- `engine/agent/manifest.json`, docs, static checks, generated-game guidance, and `agent-context` expose the workflow honestly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests/checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory The Existing 3D Package Paths

**Files:**
- Read: `tools/new-game.ps1`
- Read: `games/sample_desktop_runtime_game/game.agent.json`
- Read: `games/sample-generated-desktop-runtime-material-shader-package/game.agent.json`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/specs/generated-game-validation-scenarios.md`
- Read: `docs/ai-game-development.md`

- [x] Define the generated 3D packaged workflow boundary and list reused command surfaces.
- [x] Decide whether to extend `DesktopRuntimeMaterialShaderPackage` or add a separate generated 3D packaged template.
- [x] Define package files, scene asset key, validation target id, and validation recipe names.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- Boundary: add a separate `DesktopRuntime3DPackage` generated scaffold for the `3d-playable-desktop-package` recipe instead of reusing `DesktopRuntimeMaterialShaderPackage`. The material/shader template remains the lower-level source material/HLSL artifact scaffold; the 3D package template is the gameplay-facing package loop with camera/controller, static mesh, material, directional light, package files, `runtimeSceneValidationTargets`, and package smoke descriptors.
- Reused surfaces: `runtimePackageFiles`, `runtimeSceneValidationTargets`, `validate-runtime-scene-package`, `mirakana_add_desktop_runtime_game(... PACKAGE_FILES_FROM_MANIFEST)`, `mirakana_configure_desktop_runtime_scene_shader_artifacts`, `run-validation-recipe`, and the existing package target validation scripts. The authored flow is documented as `register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> validate-runtime-scene-package -> package/smoke`, but this slice only scaffolds deterministic first-party cooked fixtures and descriptors.
- Package files: `runtime/<game>.config`, `runtime/<game>.geindex`, `runtime/assets/3d/base_color.texture.geasset`, `runtime/assets/3d/triangle.mesh`, `runtime/assets/3d/lit.material`, and `runtime/assets/3d/packaged_scene.scene`. Source mirrors `source/materials/lit.material`, `shaders/runtime_scene.hlsl`, and `shaders/runtime_postprocess.hlsl` are authoring inputs only and must stay out of `runtimePackageFiles`.
- Validation target: id `packaged-3d-scene`, package index `runtime/<game>.geindex`, scene asset key `<game-name-with-hyphens>/scenes/packaged-3d-scene`, `contentRoot=runtime`, `validateAssetReferences=true`, and `requireUniqueNodeNames=true`.
- Validation recipes: `desktop-game-runtime`, `desktop-runtime-release-target`, `installed-d3d12-3d-package-smoke`, and `desktop-runtime-release-target-vulkan-toolchain-gated`.
- Non-goals: no broad dependency cooking, runtime source parsing, package streaming, renderer/RHI residency claim beyond host-owned package smoke, material/shader graph, live shader generation, editor productization, public native/RHI handles, Metal readiness, or general production renderer quality claim.

### Task 2: RED Checks For Generated 3D Package Workflow

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tests if a C++ helper is introduced
- Modify: this plan

- [x] Add failing static checks requiring the generated 3D packaged game manifest shape.
- [x] Add failing checks requiring `runtimeSceneValidationTargets` to match package files and scene asset key.
- [x] Add failing checks rejecting source files in `runtimePackageFiles`, runtime source parsing, broad dependency cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, editor productization, native handle, Metal, and general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Generate Or Scaffold The Packaged 3D Game Path

**Files:**
- Modify: `tools/new-game.ps1`
- Modify or create: `games/<selected-3d-package-game>/game.agent.json`
- Modify or create: `games/<selected-3d-package-game>/main.cpp`
- Modify or create: selected runtime package fixture files under `games/<selected-3d-package-game>/runtime/`
- Modify: `games/CMakeLists.txt` if a new sample target is added

- [x] Emit deterministic runtime config, `.geindex`, texture/material/mesh/scene package files, and validation target rows.
- [x] Keep source authoring inputs out of `runtimePackageFiles`.
- [x] Keep gameplay code on public `mirakana::` APIs and cooked package/runtime scene access.
- [x] Ensure package smoke args consume the generated package path and scene package path.

Implementation notes:

- Added `DesktopRuntime3DPackage` to `tools/new-game.ps1`.
- The scaffold emits `runtime/<game>.config`, `runtime/<game>.geindex`, `runtime/assets/3d/base_color.texture.geasset`, `runtime/assets/3d/triangle.mesh`, `runtime/assets/3d/lit.material`, and `runtime/assets/3d/packaged_scene.scene`.
- The generated manifest selects `3d-playable-desktop-package`, lists those runtime package files, and emits `runtimeSceneValidationTargets` id `packaged-3d-scene`.
- Source material/HLSL authoring inputs are generated under `source/materials/` and `shaders/` but are excluded from `runtimePackageFiles`.
- The generated executable keeps gameplay on public package/scene/renderer contracts and adds `--require-primary-camera-controller` smoke validation over the cooked primary camera node.

### Task 4: Validation Command Composition

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `engine/agent/manifest.json`

- [x] Document the ordered workflow: register source rows, cook selected rows, migrate Scene v2, validate runtime scene package, package/smoke.
- [x] Keep `validate-runtime-scene-package` as the reviewed non-mutating package/scene check before desktop smoke.
- [x] Keep D3D12/Vulkan package smoke host-gated and separate from package/scene validation.
- [x] Update static checks so stale generated-game guidance fails.

### Task 5: Plan Completion And Next Slice

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: this plan

- [x] Mark this 3D generated package workflow ready only inside its validated scope.
- [x] Keep renderer/RHI residency, package streaming, production material/shader authoring, editor productization, native handles, Metal, and general renderer quality planned or host-gated.
- [x] Create the next focused plan for editor playtest/package review or renderer resource residency/upload execution, based on validation evidence.
- [x] Run the full required validation set and record evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json commands.newGame must expose DesktopRuntime3DPackage`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `engine/agent/manifest.json` did not contain `DesktopRuntime3DPackage`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and exposed `currentActivePlan=docs/superpowers/plans/2026-05-02-editor-playtest-package-review-loop-v1.md` with `recommendedNextPlan=docs/superpowers/plans/2026-05-02-renderer-resource-residency-upload-execution-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed and verified `DesktopRuntime3DPackage` scaffold output, package files, validation recipes, and `runtimeSceneValidationTargets`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, CTest `28/28` passed).
- Diagnostic-only gates remain honest: Metal shader tools are missing on this host, Apple packaging requires macOS/Xcode, Android release signing/device smoke are not fully configured, and strict tidy analysis remains compile-database gated.
