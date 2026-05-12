# Animation Quaternion Clip Sampling Application v1 (2026-05-05)

**Plan ID:** `animation-quaternion-clip-sampling-application-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add deterministic quaternion keyframe sampling and apply sampled quaternion rotation tracks into first-party
`AnimationPose3d` local joint rotations.

## Context

- `animation-quaternion-local-pose-foundation-v1` added `mirakana::Quat`, `Mat4::rotation_quat`,
  `AnimationSkeleton3dDesc`, `AnimationPose3d`, and `build_animation_model_pose_3d`.
- `animation-fabrik-3d-pose-application-v1` proved validated writes into local unit-quaternion pose rotations.
- Existing float and `Vec3` animation tracks validate sorted finite keyframes, clamp sample times, and let missing joint
  tracks fall back to rest pose.

## Constraints

- Keep `mirakana_animation` and `mirakana_math` standard-library-only and host-independent.
- Require finite non-negative strictly increasing keyframe times and finite normalized quaternion values.
- Sample quaternion tracks with deterministic shortest-path interpolation and normalized output.
- Apply only explicitly provided 3D joint tracks; missing translation, rotation, or scale tracks fall back to each joint's
  rest pose component.
- Preserve positive scale validation and existing sorted/duplicate joint-track diagnostics.
- Do not add glTF quaternion import, cooked quaternion clip byte formats, blend trees/graphs over quaternion tracks,
  animation graph authoring, 3D local rotation limits/twist controls, generated-game animation wiring, renderer/RHI
  integration, or broad skeletal animation production readiness in this slice.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_animation` exposes quaternion keyframe/track validation and sampling APIs with normalized shortest-path output.
- `mirakana_animation` exposes 3D joint-track validation and `AnimationPose3d` local-pose sampling that can apply sampled
  quaternion rotations to selected joints.
- Invalid quaternion keyframes, invalid scale tracks, duplicate/out-of-range 3D joint tracks, and non-finite sample times
  are rejected deterministically.
- Docs, manifest, registry, and AI integration checks describe quaternion clip sampling/application while keeping glTF
  quaternion import, cooked quaternion clip formats, 3D local rotation limits/twist controls, animation graphs, generated
  game animation wiring, and renderer/RHI integration as follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for quaternion keyframe sampling and 3D pose track application.
- [x] Add public quaternion keyframe/track APIs.
- [x] Add 3D joint-track validation and `AnimationPose3d` sampling.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow quaternion clip surface.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed as expected before implementation because `QuatKeyframe`, `QuatAnimationTrack`,
  `sample_quat_keyframes`, `AnimationJointTrack3dDesc`, and `sample_animation_local_pose_3d` were missing.
- GREEN focused build: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  exited 0.
- GREEN focused tests: `out\build\dev\Debug\mirakana_core_tests.exe` exited 0 and included quaternion keyframe,
  3D local-pose sampling, and invalid joint-track coverage.
- Static gates: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1` exited 0.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exited 0; CTest reported 29/29 passing. Metal shader tools and Apple packaging
  remain diagnostic-only host blockers on this Windows host.
