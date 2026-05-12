# Animation 3D Local Rotation Limits Twist Controls v1 (2026-05-05)

**Plan ID:** `animation-3d-local-rotation-limits-twist-controls-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add host-independent 3D local rotation limit and twist-control contracts for quaternion `AnimationPose3d` workflows so IK and sampled 3D joint tracks can be constrained without renderer/RHI, animation graph, or skeletal deformation claims.

## Context

- `animation-quaternion-local-pose-foundation-v1` added unit-quaternion `AnimationPose3d` and model-pose construction.
- `animation-fabrik-3d-pose-application-v1` can write validated 3D FABRIK joint positions back into local quaternion rotations.
- `animation-quaternion-clip-sampling-application-v1`, `cooked-animation-quaternion-clip-v1`, and `generated-3d-quaternion-animation-package-smoke-v1` now prove sampled/cooked/generated package quaternion data flow into `AnimationPose3d`.
- Existing local rotation limits are planar Z-only rows for `AnimationPose`; they do not constrain 3D quaternion swing/twist.

## Constraints

- Keep the slice inside `mirakana_math` / `mirakana_animation` host-independent code and unit tests.
- Use explicit value types, finite validation, deterministic diagnostics, and unit-quaternion outputs.
- Do not add animation graphs, retargeting, authored IK assets, renderer/RHI upload/execution, skeletal renderer deformation, runtime scene playback, or generated package UI.
- Do not mutate existing 2D/Z-only limit APIs except where shared validation helpers are clearly reusable.
- Add RED tests before production implementation.

## Done When

- `mirakana_animation` exposes public 3D local rotation limit/twist-control descriptors with deterministic validation diagnostics.
- Quaternion pose/IK helpers can clamp or report constrained 3D local rotations without leaving unit-quaternion space.
- Existing 3D pose, quaternion clip sampling, and FABRIK 3D tests keep passing.
- Docs, manifest, registry, and AI guidance describe this as a narrow host-independent animation constraint foundation, not broad animation graph or renderer readiness.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for invalid 3D local rotation limit/twist descriptors and expected clamp behavior.
- [x] Design minimal public value types and diagnostics consistent with existing `mirakana_animation` style.
- [x] Implement unit-quaternion swing/twist decomposition or equivalent deterministic clamp helpers.
- [x] Integrate the helper with selected 3D pose/IK application paths where the contract is unambiguous.
- [x] Update docs, manifest, static checks, and validation evidence.

## Validation Evidence

- RED focused build: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests"`
  failed before implementation because `AnimationIkLocalRotationLimit3d`,
  `validate_animation_ik_local_rotation_limits_3d`, `apply_animation_local_rotation_limits_3d`, and the local-limit
  overload of `apply_animation_fabrik_ik_3d_solution_to_pose` were missing.
- GREEN focused build: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests"`
  passed after implementation.
- GREEN focused test: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_core_tests --output-on-failure"`
  passed with `mirakana_core_tests`.
- Format check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- API boundary check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Tidy smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- Agent/static guidance check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including `ctest --preset dev` with 29/29 tests passing.
