# Animation Two Bone IK 3D Orientation v1 (2026-05-05)

**Plan ID:** `animation-two-bone-ik-3d-orientation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a deterministic, host-independent analytic two-bone IK 3D orientation basis so gameplay and later animation graph
work can solve a reachable 3D target with a finite pole guide without depending on renderer, runtime host, platform,
editor, glTF, or native handles.

## Context

- `animation-two-bone-ik-xy-plane-v1` and follow-up pole-vector slices cover planar Z-rotation-only IK.
- The master plan still lists full 3D IK orientation as a Phase 6 gap.
- `JointLocalTransform` is still translation + Z-rotation + scale only, so the clean next step is an engine-owned 3D
  basis result, not a broad quaternion pose model.

## Constraints

- Keep this in `mirakana_animation` plus existing `mirakana_math::Vec3` primitives.
- Do not introduce quaternion/Euler pose storage, skeleton pose application, retargeting, glTF IK import, IK assets,
  animation graph authoring, renderer/RHI integration, or generated-game production readiness.
- Reject non-finite, zero-length, unreachable, and target-collinear pole inputs with stable diagnostics.
- Add RED tests before production API and implementation.

## Done When

- `mirakana_animation` exposes `AnimationTwoBoneIk3dDesc`, `AnimationTwoBoneIk3dJointFrame`,
  `AnimationTwoBoneIk3dSolution`, and `solve_animation_two_bone_ik_3d_orientation`.
- The solver returns deterministic root, parent, and child positions plus orthonormal parent/child frames whose x axes
  point along the solved parent and child bones.
- Tests prove a reachable 3D target is reached, the pole guide selects the bend side/plane, and invalid poles or
  unreachable targets are rejected without mutating stale output.
- Docs, manifest, registry, and static checks describe this as a 3D two-bone orientation basis only, while quaternion
  pose storage/application, FABRIK 3D, authored IK assets, animation graphs, and generated-game production readiness
  remain follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks,
  and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for reachable 3D two-bone IK orientation and invalid pole/unreachable diagnostics.
- [x] Add public `mirakana_animation` 3D two-bone IK descriptor/result/frame API.
- [x] Implement the analytic 3D solver with finite input checks, reach annulus checks, stable pole-plane construction,
  and orthonormal frame output.
- [x] Update docs, manifest, registry, and AI integration checks for the narrow 3D orientation basis.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED: after adding the focused tests, the sanitized MSBuild lane
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed on missing `AnimationTwoBoneIk3d*` API and `solve_animation_two_bone_ik_3d_orientation`; the unsanitized
  direct build first exposed the known duplicate `Path`/`PATH` host issue.
- Focused build: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  passed after implementation.
- Focused tests: `out\build\dev\Debug\mirakana_core_tests.exe` passed, including the two new 3D IK orientation tests.
- Static gates: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` passed after closure updates.
- Tidy: full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` was attempted after the shared C++ implementation change and timed out after 300s
  while scanning broad existing warning surfaces; the validate-scoped
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -MaxFiles 1` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after completion metadata update; CTest reported 29/29 tests passed.
