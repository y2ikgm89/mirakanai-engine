# Animation FABRIK 3D Pose Application v1 (2026-05-05)

**Plan ID:** `animation-fabrik-3d-pose-application-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Apply deterministic 3D FABRIK joint-position solutions back into first-party 3D local animation poses by writing
unit-quaternion local rotations for ordered skeleton chains.

## Context

- `animation-fabrik-3d-chain-ik-v1` produces root-fixed 3D joint positions but does not mutate a skeleton pose.
- `animation-quaternion-local-pose-foundation-v1` adds `AnimationSkeleton3dDesc`, `AnimationPose3d`, and
  `build_animation_model_pose_3d`, giving this slice a host-independent pose target.
- The existing XY helper `apply_animation_fabrik_ik_xy_solution_to_pose` validates an ordered parent-child chain,
  writes local rotations only, and leaves translation/scale ownership with the pose.

## Constraints

- Keep `mirakana_animation` standard-library-only and host-independent.
- Write only local quaternion rotations for segment-controlling joints; preserve translations, scales, unrelated joints,
  and end-effector local rotation.
- Require a valid 3D skeleton, valid 3D pose, finite solution positions, ordered parent-child chain, no duplicate joints,
  matching solution shape, finite non-zero bone offsets, and pose bone lengths that match solution segment lengths within
  a small tolerance.
- Do not add quaternion clip sampling/application, glTF quaternion import, twist controls, 3D local rotation limits, pole
  vector branch controls, authored IK assets, animation graph authoring, generated-game animation wiring, renderer/RHI
  integration, or cooked asset formats in this slice.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_animation` exposes `apply_animation_fabrik_ik_3d_solution_to_pose` for ordered 3D skeleton chains.
- Applying a valid 3D FABRIK solution to a rest `AnimationPose3d` rotates chain joints so
  `build_animation_model_pose_3d` places chain origins at the solved joint positions.
- The helper rejects invalid chain order, duplicate/out-of-range joints, mismatched solution shape, non-finite positions,
  invalid poses, zero-length pose bones, and length mismatches without mutating the input pose.
- Docs, manifest, registry, and AI integration checks describe this as 3D pose write-back only, while 3D local rotation
  limits, quaternion clip sampling/application, glTF quaternion import, authored IK assets, animation graphs, and
  generated-game production readiness remain follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for valid 3D FABRIK pose application and invalid input non-mutation.
- [x] Add the public `apply_animation_fabrik_ik_3d_solution_to_pose` API.
- [x] Implement validation and deterministic quaternion local-rotation write-back.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow 3D pose application surface.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED verified with `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`: failed before production implementation because `mirakana::apply_animation_fabrik_ik_3d_solution_to_pose` was not declared.
- Focused GREEN verified with `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` and `out\build\dev\Debug\mirakana_core_tests.exe`.
- Static gates passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1`.
- Full validation passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` on 2026-05-05; CTest reported 29/29 passing, while Metal/Apple host checks remained diagnostic-only host blockers.
