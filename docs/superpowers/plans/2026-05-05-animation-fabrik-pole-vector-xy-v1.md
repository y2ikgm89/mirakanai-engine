# Animation FABRIK Pole Vector XY v1 (2026-05-05)

**Plan ID:** `animation-fabrik-pole-vector-xy-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add deterministic planar branch selection to the multi-segment XY FABRIK solver using an explicit bend side and optional
pole vector guide, without expanding the solver into full 3D orientation IK.

## Context

- `animation-fabrik-chain-ik-v1` added deterministic multi-segment XY FABRIK positions and segment rotations.
- `animation-fabrik-integrated-rotation-constraints-v1` added solver-integrated segment local Z rotation limits.
- `animation-ik-angle-wrap-normalization-v1` made those local Z clamps robust across the signed-pi boundary.
- `animation-two-bone-ik-pole-vector-xy-v1` added a clean planar pole-vector branch selector to the analytic two-bone
  solver.
- The master plan still lists multi-segment pole vectors as an IK follow-up. This slice applies the same narrow planar
  branch-selection idea to the multi-segment FABRIK chain.

## Constraints

- Keep this in `mirakana_animation` + `mirakana_math`; do not add asset, scene, renderer, RHI, editor, or third-party dependencies.
- Keep the model XY-only and Z-rotation-only, matching the current `JointLocalTransform` limits.
- Use a global root-to-target branch selector. The pole vector chooses the mirrored chain branch relative to the
  root-to-target axis; it does not impose per-joint half-plane constraints or solve a 3D pole plane.
- Reject non-finite, zero-length, or root-to-target-collinear pole vectors with deterministic diagnostics.
- Preserve segment lengths and keep local rotation limits applied during root-fixed reconstruction.
- Do not claim full 3D IK orientation, quaternion/twist extraction, IK assets, glTF IK import, animation graphs, or broad
  skeletal animation production readiness.
- Add RED coverage before production behavior.

## Design

- Add `AnimationFabrikIkXyBendSide` with `positive` and `negative` values. The side is measured by the sign of
  `cross(target_xy - root_xy, joint_xy - root_xy)` for the first non-collinear internal joint.
- Add optional `AnimationFabrikIkXyDesc::bend_side` and optional `AnimationFabrikIkXyDesc::pole_vector_xy`. When neither
  is set, the solver keeps its existing unguided FABRIK branch behavior.
- When a pole vector is present, derive the requested side from `cross(target_xy - root_xy, pole_vector_xy)`, overriding
  the explicit bend side.
- After FABRIK moves the chain and after root-fixed reconstruction applies length and local rotation constraints, mirror
  all internal joints across the root-to-target axis if the first non-collinear internal joint is on the opposite side.
  Mirroring all internal joints keeps segment lengths unchanged because root and target lie on the mirror axis.
- Skip mirroring when the solved chain is collinear with the root-to-target axis; this remains a valid degenerate branch
  for straight or fully folded chains.

## Done When

- Tests prove explicit positive and negative FABRIK bend sides select mirrored branches for the same reachable target.
- Tests prove `pole_vector_xy` overrides `bend_side` and selects the requested branch.
- Tests reject non-finite, zero-length, zero root-to-target axis, and collinear pole vectors.
- Existing FABRIK reachability, local-rotation-limit, signed-pi, and pose-application coverage remains passing.
- Docs/manifest/registry distinguish planar multi-segment branch selection from full 3D IK orientation and animation
  graph workflows.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED tests for explicit FABRIK bend-side selection and pole-vector validation.
- [x] Add the public bend-side enum and optional pole-vector fields to `chain_ik.hpp`.
- [x] Implement finite/non-collinear validation and branch-side resolution in `chain_ik.cpp`.
- [x] Apply branch mirroring before root-fixed reconstruction without breaking local rotation limit behavior.
- [x] Update manifest/docs/registry and record validation evidence.
- [x] Run focused tests, format/API/agent checks, `tidy-check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed as expected because `AnimationFabrikIkXyBendSide`, `AnimationFabrikIkXyDesc::bend_side`, and
  `AnimationFabrikIkXyDesc::pole_vector_xy` did not exist.
- GREEN focused build/test:
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` passed, and
  `out\build\dev\Debug\mirakana_core_tests.exe` passed with the new explicit bend-side and pole-vector FABRIK tests.
- During implementation, `mirakana_core_tests` caught a regression in the existing signed-pi local rotation limit case when
  `bend_side` was applied by default. The API was corrected to `std::optional<AnimationFabrikIkXyBendSide>` so unguided
  FABRIK retains the existing branch behavior unless a bend side or pole vector is explicitly supplied.
- Format/API/agent/contracts: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and
  `tools\check-json-contracts.ps1` passed after formatting.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` was attempted and timed out after 600 seconds while scanning broad existing repository
  warnings. The direct-include warnings reported for `chain_ik.cpp` were fixed by explicitly including
  `mirakana/animation/skeleton.hpp` and `mirakana/math/vec.hpp`; `tools\check-tidy.ps1 -MaxFiles 1` then passed.
- Full repository validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It included `tidy-check: ok (1 files)`, full build, and CTest
  `29/29` passing; Metal and Apple packaging remained diagnostic-only host blockers on this Windows host.
