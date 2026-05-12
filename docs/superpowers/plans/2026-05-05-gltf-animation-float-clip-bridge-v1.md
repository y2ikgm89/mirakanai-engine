# glTF Animation Float Clip Bridge v1 (2026-05-05)

**Plan ID:** `gltf-animation-float-clip-bridge-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-05.

## Goal

Bridge existing glTF morph-weight animation import into the first-party cooked scalar float animation clip path, and expose a deterministic runtime/gameplay helper that turns little-endian cooked clip rows into `mirakana::FloatAnimationTrack` samples.

## Context

- `cooked-animation-float-clip-v1` introduced `GameEngine.AnimationFloatClipSource.v1`, `GameEngine.CookedAnimationFloatClip.v1`, `AssetKind::animation_float_clip`, and runtime payload access, but intentionally left glTF import and `MK_animation` sampling bridges for a later slice.
- `gltf-morph-mesh-and-weights-animation-import-v1` imports glTF 2.0 `path: "weights"` channels into `AnimationMorphWeightsTrackDesc`, but does not emit cooked float clip source documents.
- Khronos glTF 2.0 animation samplers define scalar time inputs and channel target paths; this slice stays on the already-supported `LINEAR` morph-weight path and does not claim translation, rotation, scale, STEP, or CUBICSPLINE support.

## Constraints

- Keep runtime package parsing in `MK_runtime`; keep animation sampling in `MK_animation`; do not make `MK_animation` depend on `MK_assets` or `MK_runtime`.
- Add no third-party dependency and copy no external code.
- Keep the glTF path in `MK_tools` behind the existing `MK_HAS_ASSET_IMPORTERS` gate and reuse the current stable diagnostics style.
- Public API additions must stay in `mirakana::`, avoid native handles, and pass `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Behavior changes use RED -> GREEN tests before production code.

## Done When

- [x] `MK_animation` exposes a helper that converts little-endian float32 time/value byte rows into validated `FloatAnimationTrack` rows and samples them with existing `sample_float_animation_tracks`.
- [x] `MK_tools` exposes a glTF morph-weight animation -> `AnimationFloatClipSourceDocument` bridge that emits one scalar track per morph target using stable `gltf/node/<node>/weights/<index>` target names.
- [x] `MK_core_tests` covers byte-row to `FloatAnimationTrack` conversion and deterministic sample ordering.
- [x] `MK_tools_tests` covers the glTF -> float clip source bridge and the cook/package/runtime/sampling path.
- [x] Master plan, plan registry, `docs/roadmap.md`, and `engine/agent/manifest.json` record the completed boundary without claiming broad animation graph, GPU morph, or generated 3D production readiness.
- [x] Focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` are recorded below, or concrete local blockers are documented.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| RED: build `MK_core_tests` after adding the byte-row conversion test only | Failed as expected | Missing `mirakana::FloatAnimationTrackByteSource` / `mirakana::make_float_animation_tracks_from_f32_bytes` symbols proved the new behavior was not already implemented. |
| `cmake --build --preset dev --target MK_core_tests MK_tools_tests` | Host environment failure | MSBuild hit the known local `Path` / `PATH` duplicate environment issue (`MSB6001`) before completing targeted compilation. |
| Sanitized MSBuild `MK_core_tests.vcxproj` and `MK_tools_tests.vcxproj` | Passed | Rebuilt both changed test executables after normalizing `PATH` for MSBuild. |
| `out\build\dev\Debug\MK_core_tests.exe` | Passed | Includes `animation float clip byte sources convert to sampled tracks`. |
| `out\build\dev\Debug\MK_tools_tests.exe` | Passed | Includes `gltf morph weights animation imports as cooked float clip and samples at runtime`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | `format: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`; all 29 CTest tests passed. Metal and Apple checks remained diagnostic-only host gates. |

## Non-Goals

- glTF translation, rotation, or scale channel cook/import.
- STEP or CUBICSPLINE animation sampler support.
- Animation graph authoring, retargeted graph playback, GPU morph execution, or generated 3D production-ready animation.
- Broad package dependency cooking or automatic asset dependency traversal beyond the selected cooked clip source path.
