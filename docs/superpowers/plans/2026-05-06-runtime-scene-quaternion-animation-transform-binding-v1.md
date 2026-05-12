# Runtime Scene Quaternion Animation Transform Binding v1 (2026-05-06)

**Plan ID:** `runtime-scene-quaternion-animation-transform-binding-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote cooked quaternion animation clips from package decode/sample counters into a narrow runtime-scene transform
application path: selected unit-quaternion local rotation samples can update named scene node transforms through
first-party binding rows and refresh scene render packets without exposing renderer, RHI, or native handles.

## Context

- `cooked-animation-quaternion-clip-v1` and `generated-3d-quaternion-animation-package-smoke-v1` proved
  `animation_quaternion_clip` package rows, little-endian byte-row decode, and deterministic `AnimationPose3d`
  sampling counters.
- `runtime-scene-animation-transform-binding-v1` and `generated-3d-transform-animation-scaffold-v1` already apply
  scalar float samples to runtime-scene node transforms and rebuild generated 3D render packets.
- The remaining gap is scene-facing quaternion rotation consumption: generated 3D games can sample quaternion clips,
  but the sampled local rotations are not yet applied to scene node transforms through the same first-party package
  workflow.

## Constraints

- Keep this host-independent and renderer-free at the runtime-scene/scene-renderer boundary; do not expose
  `IRhiDevice`, renderer command lists, native platform handles, or SDL3/editor APIs to gameplay.
- Limit the first slice to named-node local transform application and deterministic packet refresh evidence. Do not
  claim skeletal animation production readiness, animation graphs, blending/state machines over quaternion clips,
  retargeting, skinning, root-motion integration, editor authoring UX, or broad generated 3D production readiness.
- Reuse existing cooked `animation_quaternion_clip`, `AnimationJointTrack3dDesc`, scene node-name lookup, and
  `RuntimeSceneRenderInstance` patterns before adding new abstractions.
- Add or update focused RED tests before production implementation.

## Done When

- A focused unit test fails first for applying sampled quaternion rotation rows to a named runtime scene node.
- Runtime-scene or scene-renderer code can consume selected quaternion animation samples, update local scene transform
  rotation deterministically, and rebuild a render packet that reflects the updated transform.
- Generated 3D package guidance and manifest text distinguish this from existing quaternion pose smoke counters and
  keep unsupported animation-graph/skeletal-production claims out of `ready`.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` when public contracts change, validate-scoped
  tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing scalar transform binding rows, runtime-scene node resolution, and quaternion clip package smoke.
- [x] Add RED coverage for named-node quaternion local rotation application and render packet refresh.
- [x] Implement the minimal runtime-scene/scene-renderer quaternion transform binding path.
- [x] Update generated scaffold smoke fields, docs, manifest, AI guidance, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_scene_tests"` failed after adding the named-node pose test because `mirakana::runtime_scene::apply_runtime_scene_animation_pose_3d` did not exist.
- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_scene_renderer_tests"` failed after adding the render-instance pose test because `mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding static expectations because `games/sample_desktop_runtime_game/main.cpp` did not yet contain `sample_and_apply_runtime_scene_render_animation_pose_3d`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_scene_tests"`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_scene_tests --output-on-failure"`: 1/1 passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_scene_renderer_tests"`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_scene_renderer_tests --output-on-failure"`: 1/1 passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target sample_desktop_runtime_game"`.
- GREEN: `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_game\sample_desktop_runtime_game.exe --smoke --max-frames 2 --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex --require-quaternion-animation` exited 0 and reported `quaternion_animation=1`, `quaternion_animation_scene_applied=2`, and `quaternion_animation_scene_rotation_z=3.14159`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: `format-check: ok`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: `public-api-boundary-check: ok`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`: `tidy-check: ok (1 files)`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: `validate: ok` with 29/29 CTest tests passed.
