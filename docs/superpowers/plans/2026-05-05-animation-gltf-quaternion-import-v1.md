# Animation glTF Quaternion Import v1 (2026-05-05)

**Plan ID:** `animation-gltf-quaternion-import-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Import glTF 2.0 node rotation animation channels as first-party normalized quaternion keyframes for 3D
`AnimationPose3d` workflows.

## Context

- `animation-quaternion-clip-sampling-application-v1` added `QuatKeyframe`, `QuatAnimationTrack`,
  `sample_quat_keyframes`, `AnimationJointTrack3dDesc`, and `sample_animation_local_pose_3d`.
- The existing `gltf-node-transform-animation-import-v1` path imports LINEAR node translation, z-axis-only rotation, and
  scale channels into ordinary 2D/Z-rotation tracks.
- The Khronos glTF 2.0 specification defines animation sampler inputs as strictly increasing seconds, rotation channel
  outputs as XYZW quaternions, clamped sampling outside the input range, and LINEAR rotation interpolation by shortest-path
  slerp.

## Constraints

- Keep `mirakana_tools` as the importer adapter and keep `mirakana_animation` / `mirakana_math` host-independent.
- Support `translation`, full quaternion `rotation`, and positive `scale` node channels for the 3D track import path.
- Accept LINEAR interpolation only in this slice, matching the existing node transform importer boundary.
- Reject duplicate node/path channels, missing target nodes, out-of-range indices, sparse accessors, non-finite times,
  non-increasing times, output count mismatches, non-finite quaternion components, and non-normalized quaternion outputs.
- Keep the existing z-axis-only `rotation_z_keyframes` importer behavior stable for scalar float clip and transform binding
  workflows; do not silently broaden those 2D/Z-rotation outputs.
- Do not add cooked quaternion clip byte formats, generated-game quaternion animation package smoke, animation graphs,
  3D local rotation limits/twist controls, renderer/RHI integration, or broad skeletal animation production readiness.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_tools` exposes a glTF node transform 3D import report whose rows include selected translation, quaternion rotation,
  and positive-scale keyframes suitable for `AnimationJointTrack3dDesc`.
- Full non-z-axis glTF rotation keyframes import as normalized `QuatKeyframe` rows without going through
  `rotation_z_keyframes`.
- Invalid duplicate channels, unsupported interpolation, invalid quaternion data, count mismatches, and invalid scale rows
  fail with deterministic diagnostics.
- Docs, manifest, registry, and AI integration checks describe glTF quaternion import while keeping cooked quaternion clip
  formats, generated-game package smoke, 3D local rotation limits/twist controls, animation graphs, and renderer/RHI
  integration as follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for full glTF quaternion node rotation import and deterministic rejection cases.
- [x] Add public `mirakana_tools` 3D node transform animation import report/API.
- [x] Implement LINEAR glTF translation, quaternion rotation, and scale channel import into first-party 3D track rows.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow glTF quaternion import surface.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- Official source checked: Khronos glTF 2.0 specifies animation sampler input times as strictly increasing seconds,
  rotation outputs as XYZW quaternions, clamped sampling outside the input range, and LINEAR rotation interpolation via
  shortest-path slerp.
- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests'` failed as
  expected before implementation because `import_gltf_node_transform_animation_tracks_3d` was missing.
- GREEN focused build: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests'`
  exited 0.
- GREEN focused tests: `out\build\dev\Debug\mirakana_tools_tests.exe` exited 0 and included full quaternion import plus
  invalid quaternion, unsupported interpolation, mismatched count, invalid scale, and duplicate rotation coverage.
- Static gates: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1` exited 0.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exited 0; CTest reported 29/29 passing. Metal shader tools and Apple packaging
  remain diagnostic-only host blockers on this Windows host.
