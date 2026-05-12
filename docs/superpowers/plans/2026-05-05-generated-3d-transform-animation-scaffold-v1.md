# Generated 3D Transform Animation Scaffold v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend generated `DesktopRuntime3DPackage` games so the packaged 3D smoke applies a cooked scalar animation float clip to a scene node transform through first-party transform binding rows.

**Architecture:** Keep scalar clip decoding in `mirakana_runtime`, curve sampling in `mirakana_animation`, scene-node binding in `mirakana_runtime_scene`, and render-packet refresh in `mirakana_scene_renderer`. The generated 3D game should use a renderer-level helper for `RuntimeSceneRenderInstance` instead of hand-editing transform indexes, and it should ship a tiny first-party cooked animation fixture without adding a new runtime asset kind for binding-source rows.

**Tech Stack:** C++23, `mirakana_scene_renderer`, `mirakana_runtime_scene`, `mirakana_animation`, `tools/new-game.ps1`, `mirakana_scene_renderer_tests`, `tools/check-ai-integration.ps1`.

---

**Plan ID:** `generated-3d-transform-animation-scaffold-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `AnimationFloatClipSourceDocument` / `GameEngine.CookedAnimationFloatClip.v1` already provide scalar float clip runtime payload rows.
- `AnimationTransformBindingSourceDocument` maps scalar curve targets to named scene-node transform components.
- `mirakana_runtime_scene` already resolves binding rows and applies sampled scalar values to a `RuntimeSceneInstance`.
- Generated `DesktopRuntime3DPackage` currently proves packaged scene load, material resolution, mesh telemetry, selected shader artifacts, postprocess, and a primary camera controller, but it does not consume animation clip rows or binding rows in its smoke path.

## Constraints

- Do not introduce third-party dependencies or new package formats.
- Do not claim broad authored animation workflow, animation graph authoring, GPU morph, GPU skinning parity, full 3D orientation, or glTF runtime loading.
- Do not ship source authoring files in `runtimePackageFiles`; keep generated runtime payloads cooked and deterministic.
- Keep generated game code on public `mirakana::` headers only.
- Preserve host gates: source-tree smoke must remain `NullRenderer`/dummy-video capable; D3D12/Vulkan package smokes remain host/toolchain-gated.
- Add tests before production behavior and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` because a public helper is added.

## Done When

- `mirakana_scene_renderer` exposes and tests a helper that samples an `AnimationFloatClipSourceDocument`, applies `AnimationTransformBindingSourceDocument` rows to a `RuntimeSceneRenderInstance`, and rebuilds the render packet after successful mutation.
- `DesktopRuntime3DPackage` generated games include a tiny cooked `animation_float_clip` package asset and a binding document that animates `PackagedMesh` translation through the runtime scene binding path.
- Generated 3D source-tree and package smoke arguments can require transform animation, and smoke output includes deterministic transform animation counters.
- Static AI integration checks validate the generated runtime file, manifest entries, CMake smoke args, generated `main.cpp` wiring, and `.geindex` package rows.
- Docs, plan registry, roadmap/master plan, manifest, and current-capability guidance describe the narrow capability and its boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_scene_renderer_tests` coverage for successful render-instance transform animation application and no-scene failure.
- [x] Add public scene-renderer helper types and declaration.
- [x] Implement clip byte-row conversion, sampling, runtime-scene binding application, and render-packet rebuild in `mirakana_scene_renderer`.
- [x] Update `DesktopRuntime3DPackage` generated cooked package files with a deterministic `animation_float_clip` record.
- [x] Update generated 3D main/CMake/manifest smoke wiring for `--require-transform-animation`.
- [x] Update static AI integration checks for the new generated scaffold contract.
- [x] Update docs, roadmap/master plan, manifest, and this plan with validation evidence.
- [x] Run focused tests, formatting, API boundary, agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | Sanitized MSBuild for `mirakana_scene_renderer_tests.vcxproj` failed before implementation because `mirakana::sample_and_apply_runtime_scene_render_animation_float_clip` was missing. |
| Focused `mirakana_scene_renderer_tests` | PASS | Sanitized MSBuild for `mirakana_scene_renderer_tests.vcxproj` succeeded; `out\build\dev\Debug\mirakana_scene_renderer_tests.exe` passed, including render-instance transform animation and no-scene failure coverage. |
| Scaffold/static check | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` generated `DesktopRuntime3DPackage` and verified the animation clip runtime file, manifest rows, CMake smoke args, generated `main.cpp` wiring, and `.geindex` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | `format: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Metal shader tools and Apple packaging remain diagnostic-only host blockers on this Windows host. |
