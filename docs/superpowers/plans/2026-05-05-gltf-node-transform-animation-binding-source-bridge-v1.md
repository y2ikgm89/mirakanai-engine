# glTF Node Transform Animation Binding Source Bridge v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Import authored transform-binding source rows for glTF node TRS animation targets.

**Architecture:** Keep the bridge in `mirakana_tools`, reusing the existing glTF node transform animation importer and the first-party `GameEngine.AnimationTransformBindingSource.v1` source document. The bridge emits target-to-node/component rows only; it does not cook a runtime payload, mutate runtime scenes, add generated-game scaffolds, or expand glTF support beyond the existing LINEAR TRS restrictions.

**Tech Stack:** C++23, `mirakana_tools`, `mirakana_assets`, `mirakana_tools_tests`.

---

**Plan ID:** `gltf-node-transform-animation-binding-source-bridge-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `gltf-node-transform-animation-float-clip-bridge-v1` emits stable scalar curve targets such as `gltf/node/<index>/translation/x`.
- `animation-transform-binding-source-v1` defines the first-party authored binding source document for mapping those targets to named transform components.
- `runtime-scene-animation-transform-binding-v1` can resolve the binding source against runtime scene node names and apply caller-provided scalar samples.
- The remaining authoring gap is producing the binding source rows from the same glTF animation import path so agents do not have to hand-author target/component rows.

## Constraints

- Preserve existing importer unsupported-shape boundaries: LINEAR-only, ordinary-node translation, z-axis-only rotation, positive scale, no weights channels, no sparse accessors.
- Use glTF node `name` as the authored `node_name`; use a deterministic `gltf_node_<index>` fallback for unnamed nodes.
- Keep binding target names aligned with `import_gltf_node_transform_animation_float_clip`.
- Do not resolve runtime scenes, apply animation, cook a payload, edit generated games, or add full 3D orientation.

## Done When

- `mirakana_tools` exposes `import_gltf_node_transform_animation_binding_source`.
- The report contains a validated `AnimationTransformBindingSourceDocument` with rows matching the float clip bridge target naming and glTF node names.
- Unit tests prove translation/rotation/scale rows and deterministic unnamed-node fallback.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the bridge and boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_tools_tests` coverage for binding source bridge rows and fallback names.
- [x] Extend glTF node transform import rows with deterministic node names.
- [x] Add bridge report/API declarations to `gltf_node_animation_import.hpp`.
- [x] Implement binding source document generation in `gltf_node_animation_import.cpp`.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting/API/agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | Sanitized `mirakana_tools_tests.vcxproj` build failed before implementation because `mirakana::import_gltf_node_transform_animation_binding_source` was missing. |
| Focused `mirakana_tools_tests` | PASS | Sanitized `mirakana_tools_tests.vcxproj` build passed with 0 warnings/0 errors; `out\build\dev\Debug\mirakana_tools_tests.exe` passed including `gltf node transform animation imports transform binding source rows`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Metal shader tools and Apple packaging remain diagnostic-only host blockers on this Windows host. |
