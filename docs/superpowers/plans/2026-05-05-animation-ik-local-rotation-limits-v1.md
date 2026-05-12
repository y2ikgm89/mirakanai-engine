# Animation IK Local Rotation Limits v1 (2026-05-05)

**Plan ID:** `animation-ik-local-rotation-limits-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-05.

## Goal

Add deterministic local Z-rotation clamp rows to the FABRIK pose-application helper so gameplay code can keep planar IK output inside authored per-joint rotation ranges before building model poses or skinning palettes.

## Context

- `animation-ik-pose-application-v1` converts FABRIK world segment rotations into local `AnimationPose` Z rotations for ordered parent-child chains.
- The remaining broad IK boundary includes joint limits, but full solver-integrated constraints, pole vectors, and 3D orientation are still larger work.
- This slice intentionally clamps local pose output after unconstrained FABRIK solving; it does not change the solver iteration itself or claim constrained-solver reachability.

## Constraints

- Keep the API in `mirakana::` under `mirakana_animation`; no renderer, RHI, editor, asset, glTF, or platform dependency.
- Limit rows are explicit value types with finite min/max radians and a target joint index that must belong to the segment-controlling part of the provided chain.
- Invalid limit rows return `false` with deterministic diagnostics and leave the pose unchanged.
- Behavior changes use RED -> GREEN tests before production code.

## Done When

- [x] `mirakana_animation` exposes a local Z-rotation limit row type and a limit-aware FABRIK pose-application overload.
- [x] Applying limits clamps only segment-controlling joint local Z rotations and preserves translations, scales, and end-effector rotation.
- [x] Invalid limits are rejected without mutating the pose.
- [x] `mirakana_core_tests` covers clamped application and invalid limit diagnostics.
- [x] Master plan, plan registry, `docs/roadmap.md`, and `engine/agent/manifest.json` record this as local pose-output clamping without claiming solver-integrated joint constraints, pole vectors, full 3D orientation, animation graph authoring, or generated 3D production readiness.
- [x] Focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` are recorded below, or concrete local blockers are documented.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| RED `mirakana_core_tests` build after adding tests and before adding production source | PASS | Failed on missing `mirakana::AnimationIkLocalRotationLimit` and missing limit-aware overload, proving the tests exercised the new public surface first. |
| `cmd /d /c 'set Path=%Path%&& set PATH=&& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" mirakana_core_tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:diagnostic /clp:Summary;ErrorsOnly'` | PASS | Focused Windows build succeeded after adding `AnimationIkLocalRotationLimit` and the overload. |
| `out\build\dev\Debug\mirakana_core_tests.exe` | PASS | New tests passed for clamped local pose application and invalid limit no-mutation diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Formatting completed after implementation, tests, docs, and manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check reported `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check reported `public-api-boundary-check: ok` after extending `chain_ik.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration check reported `ai-integration-check: ok` after manifest/check updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final repository validation reported `validate: ok` after docs/manifest synchronization. |

## Non-Goals

- Solver-integrated joint constraints, pole vectors, CCD, full 3D joint orientation, quaternion interpolation, twist extraction, skeletal retargeting, IK assets, glTF IK import, animation graph authoring, GPU morph, renderer/RHI integration, package cook/runtime changes, or generated-game production readiness.
