# glTF Node Transform Animation Float Clip Bridge v1 (2026-05-05)

**Plan ID:** `gltf-node-transform-animation-float-clip-bridge-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Goal

Bridge generic glTF node TRS animation import into the existing first-party `GameEngine.AnimationFloatClipSource.v1` path without adding a new cooked asset kind.

## Context

- `gltf-node-transform-animation-import-v1` imports LINEAR node `translation`, z-axis-only `rotation`, and positive `scale` into `GltfNodeTransformAnimationTrack` rows.
- `cooked-animation-float-clip-v1` already provides scalar float source/cook/package/runtime payload rows.
- `gltf-animation-float-clip-bridge-v1` proved the same bridge shape for glTF morph weights.

## Constraints

- Reuse `AnimationFloatClipSourceDocument`; do not add a Vec3 clip asset kind in this slice.
- Emit stable scalar target names:
  - `gltf/node/<node>/translation/x|y|z`
  - `gltf/node/<node>/rotation_z`
  - `gltf/node/<node>/scale/x|y|z`
- Preserve the importer boundaries from `gltf-node-transform-animation-import-v1`: LINEAR only, z-axis-only rotation, positive scale, no `weights` channels here.
- Keep runtime playback and generated-game application out of scope; gameplay can sample float tracks through existing byte-row helpers.

## Done When

- `mirakana_tools` exposes a node transform animation -> scalar float clip source bridge.
- Unit tests import a glTF node transform animation, cook/package the source as `animation_float_clip`, load the runtime payload, convert it to `FloatAnimationTrack` rows, and sample deterministic values.
- Docs, plan registry, roadmap/master plan, manifest, and static AI integration checks describe the bridge and remaining boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_tools_tests` coverage for node transform float clip import/cook/runtime sampling.
- [x] Add the bridge report/API and implementation.
- [x] Update docs/manifest/static checks.
- [x] Run focused tests, formatting/API/agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build | PASS | Sanitized MSBuild `mirakana_tools_tests.vcxproj` failed before implementation because `mirakana::import_gltf_node_transform_animation_float_clip` was missing. |
| Focused `mirakana_tools_tests` | PASS | Sanitized MSBuild `mirakana_tools_tests.vcxproj` succeeded; `out\build\dev\Debug\mirakana_tools_tests.exe` passed including `gltf node transform animation imports as cooked float clip and samples at runtime`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Diagnostic-only host blockers remain Metal tools missing and Apple packaging unavailable on this Windows host. |
