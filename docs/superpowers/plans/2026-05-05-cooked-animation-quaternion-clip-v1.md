# Cooked Animation Quaternion Clip v1 (2026-05-05)

**Plan ID:** `cooked-animation-quaternion-clip-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a first-party source/cooked/runtime payload path for quaternion animation clips that can carry selected 3D node or
joint TRS tracks into runtime/gameplay code without reparsing glTF.

## Context

- `cooked-animation-float-clip-v1` already provides `GameEngine.AnimationFloatClipSource.v1`,
  `GameEngine.CookedAnimationFloatClip.v1`, `AssetKind::animation_float_clip`, and runtime byte-row sampling helpers for
  scalar tracks.
- `animation-quaternion-clip-sampling-application-v1` added validated `QuatKeyframe` rows and
  `sample_animation_local_pose_3d`.
- `animation-gltf-quaternion-import-v1` added `import_gltf_node_transform_animation_tracks_3d`, which can produce
  sorted translation, unit-quaternion rotation, and positive-scale rows from LINEAR glTF node animation data.

## Constraints

- Keep the cooked format first-party, deterministic text/hex payload based, and independent from glTF at runtime.
- Add a new quaternion/3D TRS clip asset kind rather than broadening scalar `animation_float_clip` semantics.
- Store times plus translation xyz, quaternion xyzw, and scale xyz rows in little-endian float32 byte payloads, with
  validation requiring finite non-negative strictly increasing times, normalized quaternions, and positive scales.
- Keep runtime/game code on `mirakana_runtime` typed cooked payload access and `mirakana_animation` sampling; do not require
  `mirakana_tools`, fastgltf, or source files at runtime.
- Do not add generated-game package smoke, graph authoring, blend trees over quaternion tracks, retargeting, 3D local
  rotation limits/twist controls, renderer/RHI integration, or broad skeletal animation production readiness in this
  slice.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_assets` exposes `GameEngine.AnimationQuaternionClipSource.v1` and paired cooked payload validation/serialization
  for selected 3D tracks.
- `mirakana_runtime` exposes a typed `runtime_animation_quaternion_clip_payload` reader for cooked package records.
- `mirakana_animation` or `mirakana_assets` bridge helpers can turn cooked byte rows into validated `AnimationJointTrack3dDesc` rows
  suitable for `sample_animation_local_pose_3d`.
- `mirakana_tools` can bridge `GltfNodeTransformAnimationTrack3d` rows into the new source/cooked clip path without runtime
  glTF parsing.
- Docs, manifest, registry, and AI integration checks describe the cooked quaternion clip path while keeping
  generated-game package smoke, graphs, retargeting, 3D local rotation limits/twist controls, and renderer/RHI
  integration as follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for source/cooked/runtime quaternion clip documents and package payload access.
- [x] Add first-party source/cooked quaternion clip document types, validation, and deterministic text IO.
- [x] Add runtime typed payload access and byte-row-to-`AnimationJointTrack3dDesc` conversion.
- [x] Add `mirakana_tools` bridge from glTF 3D node tracks into the source document.
- [x] Update docs, manifest, registry, and AI integration checks for the cooked quaternion clip path.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed before implementation because `AnimationJointTrack3dByteSource` and
  `make_animation_joint_tracks_3d_from_f32_bytes` were missing.
- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests'`
  failed before implementation because `AssetKind::animation_quaternion_clip`,
  `runtime_animation_quaternion_clip_payload`, `AnimationQuaternionClipSourceDocument`, and
  `import_gltf_node_transform_animation_quaternion_clip` were missing.
- Focused GREEN: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`.
- Focused GREEN: `out\build\dev\Debug\mirakana_core_tests.exe`.
- Focused GREEN: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests'`.
- Focused GREEN: `out\build\dev\Debug\mirakana_tools_tests.exe`.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after rerunning with local approval because the first sandboxed attempt failed with
  `Operation not permitted` before repository validation executed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1`.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
