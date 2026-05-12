# Generated 3D Mesh Telemetry Scaffold v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend generated `DesktopRuntime3DPackage` games so they emit the existing `scene_mesh_plan_*` counters during package smoke validation, matching the committed 3D package proof without claiming broad generated 3D production readiness.

**Architecture:** Reuse the completed `mirakana::plan_scene_mesh_draws` contract from `mirakana_scene_renderer`. Keep the change in `tools/new-game.ps1` generated C++ and scaffold static checks; do not add renderer execution, new runtime package mutation, new dependencies, or native/RHI handle exposure.

**Tech Stack:** C++23 generated game scaffold text, PowerShell `tools/new-game.ps1`, `tools/check-ai-integration.ps1`, docs/manifest synchronization, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Post-3D Scene Mesh Package Telemetry Review

- Latest completed slice: `docs/superpowers/plans/2026-05-02-3d-scene-mesh-package-telemetry-v1.md`.
- Validation evidence from that slice records `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with `validate: ok` and 28/28 CTest entries passed, plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS with 16/16 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and Vulkan package claims remain strict host/runtime/toolchain-gated outside verified hosts.
- Non-goals remain non-ready: renderer execution, culling, sorting, batching, skeletal animation, GPU skinning, material/shader graphs, live shader generation, runtime source parsing, broad package cooking, package streaming, public native/RHI handles, Metal readiness, broad generated 3D production readiness, production text/font/image/atlas/accessibility, editor productization, and renderer quality.
- Remaining manifest gaps are broad: `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`. This plan selects only a narrow generated-scaffold telemetry follow-up from the 3D gap.

## Goal

Make generated `DesktopRuntime3DPackage` scaffolds produce the same package-visible mesh/material planning counters that the committed 3D sample already produces.

## Context

- `mirakana::plan_scene_mesh_draws` is already implemented and tested in `mirakana_scene_renderer`.
- `sample_desktop_runtime_game` already emits `scene_mesh_plan_meshes`, `scene_mesh_plan_draws`, `scene_mesh_plan_unique_meshes`, `scene_mesh_plan_unique_materials`, and `scene_mesh_plan_diagnostics`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` currently generates camera/controller package smoke and D3D12/Vulkan shader artifact metadata, but the generated executable does not yet emit the mesh telemetry counters.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Keep gameplay public APIs free of native OS/GPU/RHI handles.
- Do not introduce third-party dependencies.
- Do not change renderer execution, culling, sorting, batching, shader/material graph authoring, GPU skinning, importer execution, package streaming, runtime source parsing, or package mutation behavior.
- Do not claim broad generated 3D production readiness, Metal readiness, public native/RHI handle access, editor productization, production UI/importer/platform adapters, or renderer quality.

## Done When

- [x] RED static check is added before implementation and fails because generated `DesktopRuntime3DPackage` output lacks `scene_mesh_plan_*` counters.
- [x] `tools/new-game.ps1 -Template DesktopRuntime3DPackage` generated `main.cpp` calls `mirakana::plan_scene_mesh_draws` over the rebuilt `SceneRenderPacket`.
- [x] Generated 3D package smoke output includes `scene_mesh_plan_meshes`, `scene_mesh_plan_draws`, `scene_mesh_plan_unique_meshes`, `scene_mesh_plan_unique_materials`, and `scene_mesh_plan_diagnostics`.
- [x] Generated packaged-scene smoke pass/fail logic requires successful mesh plan telemetry whenever `--require-scene-package` is selected.
- [x] Static scaffold checks verify the generated telemetry fields and keep source authoring files out of `runtimePackageFiles`.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` are synchronized with the limited generated-scaffold ready claim.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add a failing generated-scaffold assertion to `tools/check-ai-integration.ps1` requiring `plan_scene_mesh_draws` and the five `scene_mesh_plan_*` generated output fields in `DesktopRuntime3DPackage` `main.cpp`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and record the expected RED failure.
- [x] Update `New-DesktopRuntime3DMainCpp` in `tools/new-game.ps1` to accumulate mesh telemetry counters and include them in generated smoke output and `packaged_scene_passed`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm the generated scaffold check passes.
- [x] Update current capability docs, roadmap, registry, and manifest for Generated 3D Mesh Telemetry Scaffold v1.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` because the changed generator feeds the desktop-runtime scaffold lane.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and the next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after RED static check | Expected FAIL | Failed at `tools/check-ai-integration.ps1` because generated `DesktopRuntime3DPackage` `main.cpp` did not contain `plan_scene_mesh_draws`; this confirmed the missing scaffold telemetry before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after implementation | PASS | Generated dry-run/scaffold games through `DesktopRuntime3DPackage`; `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok` after manifest/docs synchronization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | PASS | Desktop runtime configure/build completed and 16/16 CTest entries passed, including generated desktop runtime package smoke tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final rerun after manifest/docs were synchronized to `recommendedNextPlan=next-production-gap-selection`; `license-check`, `agent-config-check`, `ai-integration-check`, `json-contract-check`, `validation-recipe-runner-check`, `dependency-policy-check`, `vcpkg-environment-check`, `toolchain-check`, `cpp-standard-policy-check`, public API boundary, and dev build/test passed; 28/28 CTest entries passed; `validate: ok`. Diagnostic-only host gates remain: Metal shader/library tools missing, Apple packaging blocked on macOS/Xcode, strict clang-tidy lacks `compile_commands.json` for the active Visual Studio generator. |

## Non-Goals

- Renderer execution changes, renderer batching, sorting, culling, or production renderer quality.
- Broad generated 3D game production readiness.
- Skeletal animation production path, GPU skinning, glTF animation/skin import, IK, or animation graph authoring.
- Material graph, shader graph, live shader generation, or broad material/shader pipeline authoring.
- Runtime source asset parsing, broad/dependent package cooking, importer execution, package streaming, or renderer/RHI teardown execution.
- Public gameplay access to native OS, GPU, RHI, SDL3, or editor handles.
- Metal/iOS readiness.
- Production text shaping, font rasterization, image decoding, atlas packing, IME, or OS accessibility.
- Editor productization or play-in-editor isolation.

## Next-Step Decision

Registry and manifest were re-read after validation. The post-review found no additional Windows-verifiable focused production slice selected by the registry or manifest, so `recommendedNextPlan` is now the blocked selection gate `next-production-gap-selection`. Remaining manifest gaps are still broad (`3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`) or host-gated (Apple/iOS/Metal, strict clang-tidy compile database). Stop here until a new dated focused plan narrows one of those gaps with explicit non-goals and validation evidence.
