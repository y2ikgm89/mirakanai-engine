# 3D Scene Mesh Package Telemetry v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add package-visible, execution-free 3D scene mesh planning telemetry for `sample_desktop_runtime_game` so Windows package smokes can prove cooked scene mesh/material intent without claiming broader 3D production readiness.

**Architecture:** Keep `mirakana_scene` renderer-neutral and keep all native/RHI ownership in host adapters. Add a small `mirakana_scene_renderer` planning helper over `SceneRenderPacket::meshes`, then emit the resulting counters from the existing 3D desktop package smoke status line and installed validation scripts.

**Tech Stack:** C++23, `mirakana_scene_renderer`, `sample_desktop_runtime_game`, PowerShell static/package checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Post-2D Sprite Batch Package Telemetry Review

- Latest completed slice: `docs/superpowers/plans/2026-05-02-2d-sprite-batch-package-telemetry-v1.md`.
- Validation evidence from that slice records `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with `validate: ok` and 28/28 CTest entries passed, plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS with 16/16 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and Vulkan package claims remain strict host/runtime/toolchain-gated outside verified hosts.
- Non-goals remain non-ready: native GPU sprite batch execution, production sprite batching readiness, source/runtime image decoding, production atlas packing, tilemap editor UX, package streaming execution, renderer/RHI teardown execution, public native/RHI handles, Metal/iOS readiness, broad renderer quality, material/shader graphs, live shader generation, and production text/font/IME/accessibility.
- Remaining manifest gaps are broad: `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`. This plan selects only a narrow Windows-verifiable 3D telemetry slice from the first gap.

## Goal

Implement `3d-scene-mesh-package-telemetry-v1` as a focused 3D package telemetry slice. The ready claim is limited to deterministic scene mesh/material reference planning over already-built `SceneRenderPacket` data plus package smoke counters.

## Context

- `sample_desktop_runtime_game` already loads a cooked 3D scene package, submits scene meshes, reports primary camera/controller counters, and exposes host-gated D3D12/Vulkan scene GPU, postprocess, shadow, and UI overlay smoke fields.
- `mirakana_scene_renderer` already owns `make_scene_mesh_command`, material color resolution, GPU binding palette lookup, and runtime scene render packet instantiation.
- The manifest still keeps broader generated 3D production readiness, skeletal animation, GPU skinning, material/shader graphs, package streaming, public native/RHI handles, Metal, and renderer quality out of the ready claim.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Keep `mirakana_scene` independent from renderer and RHI APIs.
- Do not expose native OS/GPU/RHI handles through gameplay APIs.
- Do not introduce third-party dependencies.
- Do not add renderer execution, sorting, batching, culling, material/shader graph authoring, GPU skinning, importer execution, package streaming, or runtime source parsing.
- Do not claim production 3D readiness, production renderer quality, Metal readiness, public native/RHI handle access, production text/font/image/atlas/accessibility, or broad editor productization.

## Done When

- [x] RED test/static check is added before implementation and fails for missing 3D scene mesh package telemetry.
- [x] `mirakana_scene_renderer` exposes a host-independent `plan_scene_mesh_draws` helper over `SceneRenderPacket`.
- [x] Unit coverage proves visible mesh rows produce deterministic mesh/material/draw counters and invalid mesh/material asset ids produce diagnostics.
- [x] `sample_desktop_runtime_game` smoke output reports `scene_mesh_plan_*` counters for the loaded cooked 3D package.
- [x] Installed desktop runtime validation requires the new 3D package telemetry fields for `sample_desktop_runtime_game`.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` are synchronized with the limited telemetry ready claim.
- [x] Static checks keep broad generated 3D production readiness, skeletal animation, GPU skinning, material/shader graphs, package streaming, public native/RHI handles, Metal, and renderer quality claims non-ready.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add RED checks to `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` requiring `plan_scene_mesh_draws`, `scene_mesh_plan_draws`, `scene_mesh_plan_unique_materials`, and this plan/documentation text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and record the expected failure.
- [x] Add `SceneMeshDrawPlan`, `SceneMeshDrawPlanDiagnostic`, and `plan_scene_mesh_draws(const SceneRenderPacket&)` to `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp` and `engine/scene_renderer/src/scene_renderer.cpp`.
- [x] Add unit tests in `tests/unit/scene_renderer_tests.cpp` for deterministic 3D scene mesh telemetry and invalid mesh/material diagnostics.
- [x] Update `games/sample_desktop_runtime_game/main.cpp` to accumulate and print the mesh-plan counters.
- [x] Update `tools/validate-installed-desktop-runtime.ps1` to require the new fields for the installed 3D package smoke.
- [x] Update docs, registry, manifest, and static checks for the limited ready claim and remaining non-goals.
- [x] Run focused checks, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | RED FAIL (expected) | Before GREEN implementation, failed on `games/sample_desktop_runtime_game/main.cpp missing 3D smoke field or HUD contract: plan_scene_mesh_draws`. |
| `cmake --build --preset dev --target mirakana_scene_renderer_tests` | HOST ENV BLOCKED during RED compile check | Direct shell build hit MSBuild `Path`/`PATH` duplicate environment key failure before compiling the changed test; repository wrappers normalize process environment. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Configured and built dev preset with updated `mirakana_scene_renderer`, `mirakana_scene_renderer_tests`, and samples. |
| `ctest --preset dev -R mirakana_scene_renderer_tests --output-on-failure` | PASS | 1/1 `mirakana_scene_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | PASS | Desktop runtime lane passed 16/16 CTest entries, including `sample_desktop_runtime_game_smoke`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 CTest entries passed. Host gates remained diagnostic-only for Apple/iOS/Metal and strict clang-tidy compile database availability. |

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

Registry and manifest were re-read after validation. `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this completed telemetry slice and `recommendedNextPlan` is `post-3d-scene-mesh-package-telemetry-review`. Remaining broad manifest gaps are still `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`, plus Apple/iOS/Metal host gates. No next concrete dated focused slice is selected yet; continue only if the post-review can cut another Windows-verifiable narrow slice without broadening into unplanned renderer execution, skeletal animation/GPU skinning, material/shader graphs, importer/platform adapters, editor productization, Metal, native-handle, package-streaming, or renderer-quality claims.
