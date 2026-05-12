# Scene Package Apply Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `material-instance-apply-tooling-v1`. Do not append this work to completed scene schema, authored scene cook, generated package, material, UI atlas, or editor plans.

**Goal:** Add a reviewed dry-run/apply tooling surface for first-party scene package authoring that can create or update explicit `.scene` content and matching cooked `.geindex` `AssetKind::scene` rows with scene mesh/material/sprite dependency edges, without claiming broad editor productization, package streaming, renderer/RHI residency, material graphs, shader graphs, live shader generation, or native handle access.

**Architecture:** Keep scene contracts in `mirakana_scene` and deterministic package mutation helpers in `mirakana_tools`. The command surface must reconcile the existing authored `create-scene` descriptor with the runtime package `.scene` payload contract before promotion; if the reviewed surface is package-only, expose it as a narrow package update command instead of overloading broader editor scene authoring. Gameplay-facing APIs must stay free of SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI, descriptor, shader module, swapchain, or native handles.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_assets`, `mirakana_tools`, PowerShell 7 validation entrypoints under `tools/`, `engine/agent/manifest.json`, schema/static checks, desktop runtime sample package validation.

---

## Goal

Make scene package edits AI-operable through a narrow reviewed path:

- dry-run a scene `.scene` payload from explicit scene data, output path, package index path, and dependency declarations
- validate stable scene/node/component ids where applicable, finite transforms, supported component/reference rows, package-safe paths, duplicate asset/resource rows, package hash updates, and dependency rows before writing
- validate referenced mesh/material/sprite texture rows exist and have the expected `AssetKind`
- apply the `.scene` and `.geindex` updates only after the same dry-run validation succeeds
- emit deterministic diagnostics and changed-file lists for command-surface consumers
- keep editor productization, prefab mutation, runtime source import, production renderer claims, package streaming, material graphs, shader graphs, live shader generation, and native handles unavailable

## Context

- `create-scene` and related scene command surfaces are currently planned or blocked in `engine/agent/manifest.json`.
- `mirakana_scene` already owns deterministic scene serialization, Schema v2 authoring contracts, prefab contracts, and validation helpers.
- `mirakana_assets` already owns `AssetKind::scene`, cooked package index serialization, content hashes, and scene dependency edge kinds: `scene_mesh`, `scene_material`, and `scene_sprite`.
- `mirakana_tools` already cooks first-party `GameEngine.Scene.v1` source through the asset import path and now has reviewed dry-run/apply patterns for runtime package file registration, UI atlas metadata package updates, and material instance package updates.
- This slice is scene metadata/package-row authoring only. It is not a general editor scene workflow, prefab authoring workflow, source asset importer, renderer residency system, or package streaming feature.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not parse external scene formats, glTF scene graphs, PNG/JPEG files, or source image formats.
- Do not implement material graphs, shader graphs, live shader generation, shader hot reload, shader compilation execution, or runtime shader authoring.
- Do not implement renderer/RHI residency, package streaming, GPU upload ownership, descriptor allocation, native pipeline creation, or broad renderer quality.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_scene`, `mirakana_assets`, or `mirakana_ui` depend on renderer, RHI, platform, SDL3, Dear ImGui, or backend APIs.
- Keep existing `sample_desktop_runtime_game` and generated cooked-scene/material-shader package proofs metadata-driven and host-gated.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A deterministic dry-run/apply path exists for scene `.scene` and matching package index/dependency rows, or the plan explicitly records a narrower command-surface split if `create-scene` must remain authored-only.
- Static checks keep the promoted command surface narrow, reviewed, and limited to scene metadata/package rows with unsupported renderer/editor/package-streaming claims explicit.
- Existing desktop runtime and selected package validations still pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation where relevant, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Contract Selection

**Files:**
- Read: `engine/scene/include/mirakana/scene/scene.hpp`
- Read: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Read: `engine/tools/include/mirakana/tools/material_tool.hpp`
- Read: `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`
- Read: `engine/tools/src/asset_import_tool.cpp`
- Read: `engine/tools/src/asset_package_tool.cpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `engine/agent/manifest.json`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `games/sample_desktop_runtime_game/runtime/`
- Read: `games/sample-generated-desktop-runtime-cooked-scene-package/runtime/`

- [x] Decide whether this slice promotes `create-scene` directly or introduces/uses a narrower scene package update command.
  - Decision: keep `create-scene` as the planned authored Scene/Component/Prefab Schema v2 descriptor and add a narrower ready `update-scene-package` command for cooked `GameEngine.Scene.v1` `.scene` plus `.geindex` updates.
- [x] Define dry-run and apply request/result shapes, including changed files, diagnostics, package hash updates, scene dependency rows, and unsupported claims.
  - C++ helper shape: `ScenePackageUpdateDesc`, `ScenePackageApplyDesc`, `ScenePackageUpdateResult`, `ScenePackageChangedFile`, `ScenePackageUpdateFailure`, `plan_scene_package_update`, and `apply_scene_package_update` in `mirakana_tools`.
  - Manifest command shape: `GameEngine.AiCommand.UpdateScenePackage.Request.v1` / `Result.v1` with `packageIndexPath`, `sceneAsset`, `sceneOutputPath`, `sourceRevision`, explicit mesh/material/sprite dependencies, deterministic changed files, diagnostics, `sceneContent`, and `packageIndexContent`.
- [x] Define exact overwrite, duplicate asset id/resource path, missing dependency, wrong dependency kind, malformed scene payload, and package-external path policies.
  - Package paths must be package-relative safe paths. Existing same-asset scene rows may be overwritten only when `AssetKind::scene`; output paths used by another asset are rejected. Referenced mesh/material/sprite rows must exist and match `AssetKind::mesh`, `AssetKind::material`, and current sprite-as-texture package convention. The helper validates/canonicalizes through `mirakana_scene` serialization/deserialization, rejects non-finite transforms, and replaces only the updated scene asset's dependency rows with `scene_mesh`, `scene_material`, and `scene_sprite` edges whose paths match referenced package entries.
- [x] Record which parts remain manual, planned, blocked, or host-gated.
  - `runtimePackageFiles` manifest registration remains separate through `register-runtime-package-files`. Authored Scene v2 creation, editor productization, prefab mutation, runtime source import, package cooking execution, renderer/RHI residency, package streaming, material/shader graphs, live shader generation, native handles, structured undo, Metal readiness, and broad renderer quality remain planned, blocked, unsupported, or host-gated.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tooling tests

- [x] Add failing checks for a reviewed scene package dry-run/apply surface and docs that keep editor productization, prefab mutation, runtime source import, renderer/RHI residency, package streaming, material/shader graphs, and live shader generation unsupported.
- [x] Add failing tests for deterministic dry-run output and changed-file diagnostics.
- [x] Add failing tests for package hash/dependency updates and rejection of inconsistent `.geindex` rows.
- [x] Add failing tests for duplicate scene asset/resource paths, missing mesh/material/sprite rows, wrong-kind dependency rows, unsafe paths, malformed scene payloads, and unsupported claims.
- [x] Record RED evidence in this plan.

### Task 3: Apply Tooling Implementation

**Files:**
- Modify only the selected tooling modules from Task 1.

- [x] Implement deterministic dry-run for scene `.scene` and package index updates.
- [x] Implement apply only after validation passes.
- [x] Reuse existing `mirakana_scene` and `mirakana_assets` validation/serialization instead of duplicating scene or package rules.
- [x] Preserve existing runtime scene package consumption and desktop package proof.
- [x] Keep editor productization, prefab mutation, runtime source import, renderer/RHI residency, package streaming, native handles, material/shader graphs, and live shader generation unavailable.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` only if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated scene package apply capability.
- [x] Update docs so agents know when to use dry-run/apply instead of hand-editing scene package rows.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED 2026-05-01:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected while compiling `mirakana_tools_tests`: `tests/unit/tools_tests.cpp(24,10): error C1083: include file not found: 'mirakana/tools/scene_tool.hpp'`. This proves the new scene package API tests are not passing against existing code.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected: `engine/agent/manifest.json aiOperableProductionLoop must expose one ready update-scene-package command surface`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected: `engine manifest aiOperableProductionLoop must expose one ready update-scene-package command surface`.
  - Direct `cmake --build --preset dev --target mirakana_tools_tests` was not usable from the current PowerShell PATH (`cmake` not found); repository scripts resolved Visual Studio bundled CMake and produced the focused compile RED through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- GREEN 2026-05-01:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed, 28/28 tests.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after promoting only the narrow ready `update-scene-package` command surface.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with the same manifest contract.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and exposed the updated production loop.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding explicit public-header boundary checks that keep `mirakana_scene`, `mirakana_assets`, and `mirakana_ui` independent from renderer/RHI/platform/editor/tool/backend/native APIs.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only: D3D12 DXIL and Vulkan SPIR-V were ready; Metal `metal`/`metallib` remained missing host gates.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, 14/14 tests.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the D3D12 selected package with `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `ui_overlay_status=ready`, `ui_atlas_metadata_status=ready`, `ui_texture_overlay_status=ready`, and `framegraph_passes=3`.
  - `& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_desktop_runtime_game.config', '--require-scene-package', 'runtime/sample_desktop_runtime_game.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-directional-shadow', '--require-directional-shadow-filtering', '--require-native-ui-overlay', '--require-native-ui-textured-sprite-atlas')` passed for the strict Vulkan selected package with `renderer=vulkan`, scene GPU, postprocess depth, directional shadow filtering, native UI overlay, UI atlas metadata, textured UI atlas overlay, and `framegraph_passes=3`.
  - The equivalent nested `pwsh -File ... -SmokeArgs @(...)` form failed before script execution because `-File` did not evaluate the array literal; the current-session PowerShell invocation above is the verified strict Vulkan command for this host.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after registry, roadmap, manifest, active plan, and next-plan updates. Known diagnostic-only gates remained Metal tools, Apple packaging/Xcode, Android release signing/device smoke, and tidy compile database availability.

Completed on 2026-05-01. The next focused plan is [Validation Recipe Runner Tooling v1](2026-05-01-validation-recipe-runner-tooling-v1.md).
