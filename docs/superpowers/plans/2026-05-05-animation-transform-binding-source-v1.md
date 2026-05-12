# Animation Transform Binding Source v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party source document that maps sampled scalar animation curve targets to named transform components.

**Architecture:** Keep the authored binding document in `mirakana_assets` as text-only first-party asset data. It stores curve targets, caller-resolved node names, and scalar transform components without depending on `mirakana_animation`, `mirakana_scene`, `mirakana_runtime`, or glTF tooling.

**Tech Stack:** C++23, `mirakana_assets` source document validation/serialization patterns, `mirakana_core_tests`.

---

**Plan ID:** `animation-transform-binding-source-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `animation-float-transform-application-v1` can already apply sampled scalar curves to `Transform3D` rows through explicit bindings.
- `gltf-node-transform-animation-float-clip-bridge-v1` emits stable scalar curve targets, but there is no first-party authored data file for mapping those targets to game/scene transform names.
- This slice fills only the authored data contract, so later runtime scene or generated-game slices can resolve `node_name` to actual transform rows.

## Constraints

- Keep `mirakana_assets` dependency-free; do not include `mirakana_animation`, `mirakana_scene`, `mirakana_runtime`, or glTF headers.
- Store component names as stable text tokens: `translation_x`, `translation_y`, `translation_z`, `rotation_z`, `scale_x`, `scale_y`, `scale_z`.
- Validate non-empty documents, safe target and node-name tokens, recognized components, duplicate curve targets, and duplicate `(node_name, component)` bindings.
- Do not cook a runtime payload, resolve scene nodes, apply transforms, or update generated-game scaffolds in this slice.

## Done When

- `mirakana_assets` exposes `AnimationTransformBindingSourceDocument` with validation, deterministic text serialization, and deserialization.
- Unit tests prove round-trip behavior and deterministic rejection of duplicate/invalid documents.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the new authored binding contract and remaining boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_core_tests` coverage for binding source document round-trip and rejection.
- [x] Add public document types and parser/serializer declarations to `asset_source_format.hpp`.
- [x] Implement validation, component token parsing, deterministic serialization, and deserialization in `asset_source_format.cpp`.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting/API/agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | Sanitized MSBuild for `mirakana_core_tests.vcxproj` failed before implementation on missing `AnimationTransformBindingSourceDocument`, row/component types, and binding source validation/serialize/deserialize APIs. |
| Focused `mirakana_core_tests` | PASS | Sanitized MSBuild for `mirakana_core_tests.vcxproj` succeeded; `out\build\dev\Debug\mirakana_core_tests.exe` passed including the binding source round-trip/rejection tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Metal shader tools and Apple packaging remain diagnostic-only host blockers on this Windows host. |
