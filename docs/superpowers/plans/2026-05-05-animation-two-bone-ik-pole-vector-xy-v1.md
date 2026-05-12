# Animation Two-Bone IK Pole Vector XY v1 (2026-05-05)

**Plan ID:** `animation-two-bone-ik-pole-vector-xy-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a deterministic planar pole-vector branch selection contract to the existing two-bone XY IK solver so callers can
choose the elbow side from an authored guide vector instead of exposing the solver's internal sine sign.

## Context

- `animation-two-bone-ik-xy-plane-v1` added a minimal analytic XY two-bone solver with a
  `prefer_positive_elbow_solution` bool.
- The bool names the cosine-law branch sign, not the authored bend side. For a target on the positive X axis, a positive
  elbow sine places the elbow on the negative Y side.
- The master plan still lists pole vectors as an IK follow-up. A planar two-bone pole vector is the smallest
  host-independent step before any full 3D orientation or multi-segment pole solver work.

## Constraints

- Keep this in `mirakana_animation` + `mirakana_math`; do not add asset, scene, renderer, RHI, editor, or third-party dependencies.
- Prefer a clean public API over preserving the MVP-era bool name.
- Keep the solver analytic and deterministic; reject non-finite or collinear pole vectors with stable diagnostics.
- Do not claim multi-segment FABRIK pole vectors, full 3D IK orientation, quaternion/twist extraction, IK assets, glTF IK
  import, animation graphs, or generated-game production readiness.
- Add RED coverage before production behavior.

## Design

- Replace `AnimationTwoBoneIkXyDesc::prefer_positive_elbow_solution` with
  `AnimationTwoBoneIkXyBendSide bend_side`, where `positive` and `negative` refer to the side of the root-to-target axis
  on which the intermediate joint should land.
- Add `std::optional<Vec2> pole_vector_xy`; when present, it overrides `bend_side` by using the sign of
  `cross(target_xy, pole_vector_xy)`.
- Keep the root at the local origin, matching the existing two-bone solver contract. The pole vector is a local XY guide
  direction from that root, not a world-space scene point.

## Done When

- Two-bone IK tests prove explicit positive and negative bend sides place the intermediate joint on the expected side of
  the target axis.
- Two-bone IK tests prove `pole_vector_xy` selects the same side deterministically and rejects collinear or non-finite
  poles.
- Existing reachability diagnostics remain covered.
- Docs/manifest/registry distinguish planar two-bone pole vectors from multi-segment/full-3D IK.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED tests for bend-side and pole-vector branch selection.
- [x] Replace the bool public branch selector with a bend-side enum and optional pole vector.
- [x] Implement finite/non-collinear validation and side selection in `two_bone_ik.cpp`.
- [x] Update docs/manifest/registry and record validation evidence.
- [x] Run focused tests, format/API/agent checks, `tidy-check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Evidence

- RED: `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'`
  failed as expected because `AnimationTwoBoneIkXyBendSide`, `AnimationTwoBoneIkXyDesc::bend_side`, and
  `AnimationTwoBoneIkXyDesc::pole_vector_xy` did not exist.
- GREEN focused build/test:
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` passed, and
  `out\build\dev\Debug\mirakana_core_tests.exe` passed with the new explicit bend-side and pole-vector tests.
- Format/API/agent/contracts: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and
  `tools\check-json-contracts.ps1` passed after formatting. The second `api-boundary-check` run required escalation
  because the sandbox returned `Operation not permitted` for its transient cwd.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` was attempted after the final C++ edit and timed out after 600 seconds while scanning broad
  existing repository warnings; the introduced `two_bone_ik.cpp` direct-include warning was removed by explicitly
  including `mirakana/math/vec.hpp`.
- Full repository validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It included `tidy-check: ok (1 files)`, full build, and CTest
  `29/29` passing; Metal and Apple packaging remained diagnostic-only host blockers on this Windows host.
