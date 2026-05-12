# Animation Float Transform Application v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply sampled scalar float animation curves to caller-owned `Transform3D` rows through explicit, deterministic bindings.

**Architecture:** Keep this in `mirakana_animation` so generated games and runtime code can consume cooked `animation_float_clip` payload samples without depending on `mirakana_tools`, asset importers, scene runtime, or glTF parsing. The API remains target-string agnostic: glTF-derived targets such as `gltf/node/<node>/translation/x` are just strings bound by the caller to transform indices and components.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math::Transform3D`, existing `FloatAnimationCurveSample` rows.

---

**Plan ID:** `animation-float-transform-application-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `cooked-animation-float-clip-v1` provides cooked scalar float clip payload rows and byte-row sampling helpers.
- `gltf-node-transform-animation-float-clip-bridge-v1` emits stable scalar target names for glTF node TRS curves, but it intentionally does not apply those curves to scene or game transforms.
- This slice provides the minimal gameplay-side bridge: sampled scalar curves plus explicit bindings update caller-owned `Transform3D` values.

## Constraints

- Do not make `mirakana_animation` depend on `mirakana_tools`, `mirakana_assets`, `mirakana_runtime`, `mirakana_scene`, or glTF libraries.
- Do not parse glTF or assume a specific target prefix in the implementation.
- Do not add animation graph authoring, scene-node lookup, runtime package loading, or generated-game scaffolding in this slice.
- Reject duplicate sample targets, duplicate binding targets, missing bound samples, out-of-range transform indices, non-finite values, and non-positive scale values with deterministic diagnostics.

## Done When

- `mirakana_animation` exposes a small public binding API for applying sampled scalar curves to `Transform3D` rows.
- Unit tests prove successful translation/rotation_z/scale application using glTF-style target strings and deterministic rejection of invalid binding/sample input.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the new helper and remaining boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_core_tests` coverage for applying sampled scalar curves to `Transform3D`.
- [x] Add RED `mirakana_core_tests` coverage for deterministic invalid input rejection.
- [x] Add public API declarations to `engine/animation/include/mirakana/animation/keyframe_animation.hpp`.
- [x] Implement the binding application in `engine/animation/src/keyframe_animation.cpp`.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting/API/agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | Sanitized MSBuild `mirakana_core_tests.vcxproj` failed before implementation because `AnimationTransformCurveBinding`, `AnimationTransformComponent`, and `apply_float_animation_samples_to_transform3d` were missing. |
| Focused `mirakana_core_tests` | PASS | Sanitized MSBuild `mirakana_core_tests.vcxproj` succeeded; `out\build\dev\Debug\mirakana_core_tests.exe` passed including transform application and invalid-input tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Diagnostic-only host blockers remain Metal tools missing and Apple packaging unavailable on this Windows host. |
