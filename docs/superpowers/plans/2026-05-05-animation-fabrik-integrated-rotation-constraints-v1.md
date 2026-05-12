# Animation FABRIK Integrated Rotation Constraints v1 (2026-05-05)

**Plan ID:** `animation-fabrik-integrated-rotation-constraints-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Make the XY FABRIK solver honor deterministic per-segment local Z rotation limits during solving, instead of only
clamping pose output after an unconstrained solve.

## Context

- `animation-fabrik-chain-ik-v1` added deterministic multi-segment XY FABRIK positions and segment rotations.
- `animation-ik-pose-application-v1` writes FABRIK segment rotations into local `AnimationPose` Z rotations for ordered
  skeleton chains.
- `animation-ik-local-rotation-limits-v1` clamps local pose rotations during pose application, but that clamp does not
  change the solver's returned joint positions or end-effector error.
- At slice selection time, the master plan still listed solver-integrated IK constraints, pole vectors, and full 3D IK
  orientation as follow-up systems. This slice covers only the first XY local-rotation constraint step.

## Constraints

- Keep the API in `mirakana_animation`; do not add scene, renderer, runtime, or editor dependencies.
- Keep the model XY-only and Z-rotation-only, matching the current `JointLocalTransform` limits.
- Use explicit segment-indexed solver limits, not skeleton joint ids, because the solver can run without a skeleton.
- Preserve deterministic value-type behavior and diagnostics for invalid limits.
- Do not claim pole vectors, solver weight blending, full 3D orientation, angular wrapping policy, or authored animation
  graph support.
- Add RED coverage before production behavior.

## Done When

- `AnimationFabrikIkXyDesc` can carry per-segment local Z rotation limits.
- `solve_animation_fabrik_ik_xy_chain` validates finite ranges, segment index bounds, and duplicate segment limits.
- The solver applies those limits while reconstructing the chain from the fixed root so returned joint positions,
  `segment_rotation_z_radians`, `end_effector_error`, and `reached` reflect constrained motion.
- Existing unconstrained FABRIK behavior and pose-application local rotation limits remain covered.
- Docs/manifest/registry distinguish this XY constraint slice from pole vectors, 3D IK orientation, and animation graphs.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Add RED tests for constrained FABRIK output and invalid integrated rotation limits.
- [x] Add a public segment local-rotation limit row to `chain_ik.hpp`.
- [x] Validate integrated solver limits in `chain_ik.cpp`.
- [x] Apply limits during root-fixed reconstruction and unreachable target reconstruction.
- [x] Update manifest/docs/registry and record validation evidence.
- [x] Run focused tests, format/API/agent checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and record evidence.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_core_tests` first hit the known Windows MSBuild `Path`/`PATH`
  duplicate environment issue, then the sanitized command
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` failed as
  expected because `AnimationFabrikIkXyDesc::local_rotation_limits` and
  `AnimationFabrikIkXySegmentRotationLimit` did not exist.
- GREEN focused build/test:
  `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_core_tests'` passed, and
  `out\build\dev\Debug\mirakana_core_tests.exe` passed with the new integrated-limit and invalid-limit cases.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `tools\check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: attempted after the shared C++ solver change; timed out after 300 seconds while producing broad
  existing clang-tidy warnings. The repository `validate` tidy smoke below still passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. The run included license/config/json/recipe/AI/dependency/toolchain/API checks, shader and
  mobile diagnostic-only host gates, tidy smoke, default MSVC build, generated C++23 mode check, and CTest 29/29 passing.
