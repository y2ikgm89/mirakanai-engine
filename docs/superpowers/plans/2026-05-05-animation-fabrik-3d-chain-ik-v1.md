# Animation FABRIK 3D Chain IK v1 (2026-05-05)

**Plan ID:** `animation-fabrik-3d-chain-ik-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a deterministic, host-independent 3D FABRIK chain IK position solver in `mirakana_animation` so gameplay and later
animation graph work can solve multi-segment 3D chains without renderer, runtime host, platform, editor, glTF, or
native handles.

## Context

- `animation-fabrik-chain-ik-v1` and follow-up slices cover XY-plane FABRIK positions, Z-rotation pose write-back,
  pole-vector branch selection, and planar local Z limits.
- `animation-two-bone-ik-3d-orientation-v1` covers a narrow analytic two-bone 3D orientation basis only.
- The clean next step is 3D FABRIK joint positions, not quaternion pose storage/application, authored IK assets, or
  skeleton pose write-back.

## Constraints

- Keep the implementation in `mirakana_animation` and use existing `mirakana_math::Vec3` primitives.
- Do not introduce quaternion/Euler pose storage, skeleton pose application, local 3D rotation constraints, pole-vector
  branch control, retargeting, glTF IK import, IK assets, animation graph authoring, renderer/RHI integration, or
  generated-game production readiness.
- Preserve existing XY FABRIK behavior and API.
- Reject invalid finite inputs, empty/oversized chains, non-positive segment lengths, invalid tolerance/iteration count,
  and malformed initial joint rows with stable diagnostics.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_animation` exposes `AnimationFabrikIk3dDesc`, `AnimationFabrikIk3dSolution`, and
  `solve_animation_fabrik_ik_3d_chain`.
- The solver returns deterministic root-fixed 3D joint positions, end-effector error, iteration count, `reached`, and
  `target_was_unreachable`.
- Tests prove reachable 3D target convergence, outer unreachable target clamping, inner minimum-reach clamping, invalid
  input diagnostics, and stale-output clearing on failure.
- Docs, manifest, registry, and static checks describe this as a 3D position solver only, while quaternion pose
  storage/application, 3D pose write-back, 3D local rotation limits, authored IK assets, glTF IK import, animation
  graphs, and generated-game production readiness remain follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for reachable 3D FABRIK chain solving, unreachable clamping, minimum-reach clamping, and invalid
  diagnostics.
- [x] Add public `mirakana_animation` 3D FABRIK descriptor/result API.
- [x] Implement the 3D FABRIK solver with finite input checks, deterministic default/initial joint construction,
  outer/inner reach handling, root-fixed forward/backward iteration, and stale-output clearing.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow 3D position solver.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed as expected while the new tests referenced missing `AnimationFabrikIk3dDesc`,
  `AnimationFabrikIk3dSolution`, and `solve_animation_fabrik_ik_3d_chain`.
- GREEN/focused: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  passed after implementation.
- Focused tests: `out\build\dev\Debug\mirakana_core_tests.exe` passed, including
  `animation FABRIK 3D chain IK solves reachable targets`,
  `animation FABRIK 3D chain IK clamps unreachable targets deterministically`, and
  `animation FABRIK 3D chain IK reports invalid descriptions`.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` passed.
- Tidy smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1` passed
  (`tidy-check: ok (1 files)`; broad existing warning output remains outside this slice).
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after closing the docs/manifest ledger; CTest reported
  `100% tests passed, 0 tests failed out of 29`.
