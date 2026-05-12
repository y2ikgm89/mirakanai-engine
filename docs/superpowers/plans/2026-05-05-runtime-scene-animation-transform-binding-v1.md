# Runtime Scene Animation Transform Binding v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve authored animation transform binding source rows against a runtime scene instance and optionally apply sampled scalar curves to scene node transforms.

**Architecture:** Keep authored data in `mirakana_assets`, scalar sampling/application in `mirakana_animation`, and runtime scene node lookup/application in `mirakana_runtime_scene`. `mirakana_runtime_scene` may depend on `mirakana_animation` for the bridge API, but this slice must not add generated-game scaffold changes, package format changes, renderer/RHI work, or glTF-specific behavior.

**Tech Stack:** C++23, `mirakana_runtime_scene`, `mirakana_assets`, `mirakana_animation`, `mirakana_runtime_scene_tests`.

---

**Plan ID:** `runtime-scene-animation-transform-binding-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `animation-transform-binding-source-v1` records curve-target to `node_name` / transform-component rows as `GameEngine.AnimationTransformBindingSource.v1`.
- `animation-float-transform-application-v1` applies sampled scalar curves to caller-owned `Transform3D` arrays through explicit `AnimationTransformCurveBinding` rows.
- Runtime scene instances already support deterministic node-name lookup and optional unique-name validation.
- The missing bridge is resolving authored `node_name` rows into runtime scene transform indexes and applying sampled scalar rows without exposing scene internals to generated games.

## Constraints

- Keep `mirakana_assets` dependency-free and do not move scene lookup into source-document parsing.
- Keep `mirakana_animation` independent from `mirakana_scene` and `mirakana_runtime_scene`.
- Fail deterministically for invalid binding documents, missing node names, and duplicate node names.
- Validate all bindings and samples before mutating scene node transforms.
- Do not add cooked package payloads, generated-game scaffold wiring, glTF-specific special cases, renderer/RHI behavior, full 3D orientation, or animation graph authoring.

## Done When

- `mirakana_runtime_scene` exposes a resolver from `AnimationTransformBindingSourceDocument` to runtime-scene `AnimationTransformCurveBinding` rows.
- `mirakana_runtime_scene` exposes an apply helper that samples already-provided scalar curve values into scene node transforms only after validation succeeds.
- Unit tests prove successful resolve/apply behavior plus deterministic missing/duplicate-node rejection without partial mutation.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the new bridge and remaining boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_runtime_scene_tests` coverage for resolve/apply and missing/duplicate node rejection.
- [x] Add public bridge types and function declarations to `runtime_scene.hpp`.
- [x] Implement deterministic resolution/application in `runtime_scene.cpp`.
- [x] Update CMake dependency direction for `mirakana_runtime_scene -> mirakana_animation`.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting/API/agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | Sanitized MSBuild for `mirakana_runtime_scene_tests.vcxproj` failed before implementation on missing `resolve_runtime_scene_animation_transform_bindings`, `apply_runtime_scene_animation_transform_samples`, animation sample/binding types in the runtime scene surface, and diagnostics. |
| Focused `mirakana_runtime_scene_tests` | PASS | Sanitized MSBuild for `mirakana_runtime_scene_tests.vcxproj` succeeded after CMake regenerate; `out\build\dev\Debug\mirakana_runtime_scene_tests.exe` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Metal shader tools and Apple packaging remain diagnostic-only host blockers on this Windows host. |
