# glTF Node Transform Animation Import v1 (2026-05-05)

**Plan ID:** `gltf-node-transform-animation-import-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Goal

Import glTF 2.0 `animations[].channels` with `translation`, z-axis-only `rotation`, and `scale` paths for ordinary nodes into deterministic `mirakana_animation` keyframe tracks.

## Context

- `gltf-animation-skin-import-v1` already imports LINEAR TRS channels for nodes listed in a selected skin.
- Non-skin/generated gameplay still lacks a generic node transform animation importer.
- `keyframe_animation.hpp` already owns `Vec3Keyframe`, `FloatKeyframe`, `Vec3AnimationTrack`, and `FloatAnimationTrack`.

## Constraints

- Keep the slice host-independent and inside `mirakana_tools` / `mirakana_animation` public contracts.
- Support only LINEAR interpolation.
- Reject non-finite or non-strictly-increasing time keys.
- Map glTF quaternion rotation only when it is z-axis-only; full 3D orientation remains a follow-up.
- Keep glTF `weights` channels on the existing morph-weight importer path.
- Do not add third-party dependencies, cook formats, or generated-game runtime behavior in this slice.

## Done When

- `mirakana_tools` exposes a generic node transform animation import report with one row per animated node.
- Imported rows are sorted by node index and contain validated translation, z-rotation, and/or scale keyframes.
- Unit tests cover a successful multi-channel import and at least one unsupported/duplicate-channel rejection.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the new capability and remaining boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_tools_tests` coverage for generic glTF node TRS animation import.
- [x] Add `mirakana/tools/gltf_node_animation_import.hpp` and implementation.
- [x] Register the new source in `engine/tools/CMakeLists.txt`.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting checks, agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build | Passed as expected | `mirakana_tools_tests` failed before implementation with missing `mirakana/tools/gltf_node_animation_import.hpp` after the tests included the new public API. A first attempt hit the known local `Path`/`PATH` MSBuild blocker, then the sanitized MSBuild run produced the expected missing-header failure. |
| Focused `mirakana_tools_tests` | Passed | Sanitized MSBuild for `mirakana_tools_tests` succeeded, and `out\build\dev\Debug\mirakana_tools_tests.exe` passed including the two new glTF node transform animation tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Ran after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; repository formatting check reported `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | RED failed before docs/manifest sync because `docs/current-capabilities.md` lacked the new capability; GREEN passed after adding docs, manifest public header/purpose text, and static checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Required because the slice adds `engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed: 29/29 CTest tests passed. Metal and Apple packaging remained diagnostic-only host blockers; Android readiness was reported. |
