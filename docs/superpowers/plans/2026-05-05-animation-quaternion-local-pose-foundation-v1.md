# Animation Quaternion Local Pose Foundation v1 (2026-05-05)

**Plan ID:** `animation-quaternion-local-pose-foundation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a deterministic, host-independent quaternion local-pose foundation so later 3D IK pose write-back, quaternion clip
sampling, and authored 3D animation workflows can use first-party rotation storage without renderer, RHI, runtime host,
platform, editor, glTF importer, or native handles.

## Context

- Existing `AnimationSkeletonDesc` / `AnimationPose` intentionally store translation, positive scale, and
  Z-rotation-only radians.
- `animation-two-bone-ik-3d-orientation-v1` provides orthonormal two-bone 3D basis output, and
  `animation-fabrik-3d-chain-ik-v1` provides root-fixed 3D joint positions, but neither can write a 3D local pose.
- The clean prerequisite is a first-party unit-quaternion math contract plus a parallel 3D skeleton/pose value contract,
  not immediate glTF quaternion import, IK write-back, local 3D constraints, graph authoring, or renderer integration.

## Constraints

- Keep `mirakana_math` and `mirakana_animation` standard-library-only and host-independent.
- Add a focused `Quat` value type and explicit quaternion helpers; reject non-finite and non-unit rotations in animation
  validation rather than silently accepting malformed poses.
- Add a parallel 3D pose contract instead of migrating every existing Z-only animation API in this slice.
- Do not add quaternion keyframe sampling, glTF quaternion import, 3D IK pose write-back, 3D local rotation limits,
  skinning palette migration, generated-game animation production, animation graph authoring, renderer/RHI integration,
  or cooked asset formats in this slice.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_math` exposes `Quat`, unit validation/normalization helpers, axis-angle construction, quaternion multiplication,
  vector rotation, and `Mat4::rotation_quat`.
- `mirakana_animation` exposes `AnimationJointLocalTransform3d`, `AnimationSkeleton3dDesc`, `AnimationPose3d`,
  `AnimationModelPose3d`, validation helpers, rest-pose construction, and model-pose matrix construction.
- Tests prove quaternion rotation, composition, matrix conversion, valid 3D skeleton/model pose construction, parent
  hierarchy application, non-Z-axis rotation behavior, and invalid quaternion/scale/parent diagnostics.
- Docs, manifest, registry, and static checks describe this as a 3D pose-storage/model-matrix foundation only, while
  quaternion clip sampling, glTF quaternion import, 3D IK pose write-back/local rotation limits, authored IK assets,
  animation graphs, skinning palette migration, and generated-game production readiness remain follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for quaternion math and 3D animation pose APIs.
- [x] Add public `mirakana_math` quaternion API and matrix conversion.
- [x] Add public `mirakana_animation` 3D skeleton/pose descriptor/result API.
- [x] Implement 3D skeleton/pose validation, rest-pose construction, and model-pose matrix construction.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow quaternion pose foundation.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed while `mirakana/math/quat.hpp`, `Quat`, `Mat4::rotation_quat`, `AnimationSkeleton3dDesc`,
  `AnimationPose3d`, and `build_animation_model_pose_3d` were still missing.
- Focused GREEN: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  passed.
- Focused GREEN: `out\build\dev\Debug\mirakana_core_tests.exe` passed, including
  `quat rotates vectors and converts to matrices deterministically`,
  `animation 3D skeleton validates quaternion rest pose and builds model pose`, and
  `animation 3D skeleton rejects invalid quaternion pose inputs`.
- Static checks passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1`.
- Full validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (CTest 29/29 passed; diagnostic-only blockers remained Metal tools
  and Apple packaging host tools).
