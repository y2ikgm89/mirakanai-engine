# Animation IK Angle Wrap Normalization v1 (2026-05-05)

**Plan ID:** `animation-ik-angle-wrap-normalization-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Normalize planar IK local Z angle deltas across the `-pi` / `pi` boundary so constraints treat equivalent rotations as
small local differences instead of near-`2pi` jumps.

## Context

- `animation-fabrik-integrated-rotation-constraints-v1` moved segment local Z limits into XY FABRIK root-fixed
  reconstruction.
- `AnimationPose` write-back already computes local Z deltas from segment world rotations and optional parent model
  rotations.
- Both paths currently subtract angles directly. A parent world rotation near `+pi` and a child world rotation near `-pi`
  should represent a small local delta, but direct subtraction reports a large negative delta.

## Constraints

- Keep the change internal to `mirakana_animation`; do not add public API unless tests prove it is needed.
- Keep the normalization deterministic and finite-input-only.
- Apply normalization only to local delta computation, not to stored `JointLocalTransform` values or public solution
  rotations beyond the existing `atan2` outputs.
- Do not claim pole vectors, full 3D IK orientation, quaternion orientation, animation graphs, or root-motion wrapping.
- Add RED coverage before production behavior.

## Done When

- Solver-integrated FABRIK segment limits handle parent/child segment rotations across the `-pi` / `pi` boundary.
- FABRIK solution-to-pose local rotation clamps handle parent/child rotations across the same boundary.
- Existing local limit, unreachable target, and unconstrained FABRIK tests remain green.
- Docs/manifest/registry move angular wrap normalization out of the current non-ready IK boundary while keeping broader
  3D orientation and pole-vector claims out of ready.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED tests for solver-integrated limit wrapping and pose-application wrapping.
- [x] Add a private signed-pi angle normalization helper in `chain_ik.cpp`.
- [x] Use the helper when computing segment-local solver deltas and pose-application local rotations.
- [x] Update docs/manifest/registry and record validation evidence.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  built the tests, then `out\build\dev\Debug\mirakana_core_tests.exe` failed the new solver wrap case at
  `solution.reached` and the new pose wrap case at the expected child local rotation assertion.
- GREEN focused build/test:
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` passed, and
  `out\build\dev\Debug\mirakana_core_tests.exe` passed with both wrap cases green.
- Format/agent/contracts: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `tools\check-json-contracts.ps1` passed
  after formatting the `std::numbers::pi_v<float>` include update.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` was attempted after the final C++ edit and timed out after 600 seconds while scanning broad
  existing repository warnings; the introduced `pi` literal warning was removed by switching to `std::numbers`.
- Full repository validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It included `tidy-check: ok (1 files)`, full build, and CTest
  `29/29` passing; Metal and Apple packaging remained diagnostic-only host blockers on this Windows host.
