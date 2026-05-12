# Animation IK Pose Application v1 (2026-05-05)

**Plan ID:** `animation-ik-pose-application-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-05.

## Goal

Add a deterministic `mirakana_animation` helper that writes planar FABRIK chain segment rotations into an existing `AnimationPose` so gameplay code can turn solver output into the current Z-rotation-only skeleton pose contract.

## Context

- `animation-fabrik-chain-ik-v1` exposes deterministic XY-plane joint positions and world-space segment Z rotations.
- `AnimationPose` already stores local `JointLocalTransform` rows, and `build_animation_model_pose` composes parent transforms through translation, Z rotation, and positive scale.
- This slice bridges those two contracts without adding joint limits, full 3D orientation, quaternion math, animation graph assets, or renderer/runtime package behavior.

## Constraints

- Keep the API in `mirakana::` under `mirakana_animation`; no renderer, RHI, editor, asset, glTF, or platform dependency.
- Require an explicit ordered joint chain including the end-effector joint. The helper updates rotations on segment-controlling joints and preserves existing translations, scales, and the end-effector joint rotation.
- Validate skeleton, pose, solution shape, finite rotations, unique in-range joints, and strict parent-child chain order before mutating the pose.
- Behavior changes use RED -> GREEN tests before production code.

## Done When

- [x] `mirakana_animation` exposes `apply_animation_fabrik_ik_xy_solution_to_pose`.
- [x] The helper converts world-space segment rotations into local Z rotations using the current parent pose rotations.
- [x] Invalid descriptions return `false` with deterministic diagnostics and leave the pose unchanged.
- [x] `mirakana_core_tests` covers successful pose application and invalid chain diagnostics.
- [x] Master plan, plan registry, `docs/roadmap.md`, and `engine/agent/manifest.json` record the completed boundary without claiming joint limits, full 3D orientation, animation graph authoring, or generated 3D production readiness.
- [x] Focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` are recorded below, or concrete local blockers are documented.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| RED `mirakana_core_tests` build after adding tests and before adding production source | PASS | Failed on missing `mirakana::apply_animation_fabrik_ik_xy_solution_to_pose`, proving the tests exercised the new public surface first. |
| `cmd /d /c 'set Path=%Path%&& set PATH=&& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" mirakana_core_tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:diagnostic /clp:Summary;ErrorsOnly'` | PASS | Focused Windows build succeeded after adding the public API and implementation. |
| `out\build\dev\Debug\mirakana_core_tests.exe` | PASS | New tests passed for successful FABRIK solution pose application and invalid chain no-mutation diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Formatting completed after implementation, tests, docs, and manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check reported `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check reported `public-api-boundary-check: ok` after extending `chain_ik.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration check reported `ai-integration-check: ok` after manifest/check updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final repository validation reported `validate: ok` after docs/manifest synchronization. |

## Non-Goals

- Joint limits, pole vectors, CCD, full 3D joint orientation, quaternion interpolation, twist extraction, skeletal retargeting, IK assets, glTF IK import, animation graph authoring, GPU morph, renderer/RHI integration, package cook/runtime changes, or generated-game production readiness.
