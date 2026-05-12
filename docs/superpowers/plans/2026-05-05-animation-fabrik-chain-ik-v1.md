# Animation FABRIK Chain IK v1 (2026-05-05)

**Plan ID:** `animation-fabrik-chain-ik-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-05.

## Goal

Add a deterministic multi-segment XY-plane FABRIK IK helper in `mirakana_animation` so generated gameplay and authored animation code can solve simple chains beyond the existing planar two-bone analytic baseline.

## Context

- `animation-two-bone-ik-xy-plane-v1` provides an analytic two-bone solver with Z-rotation output, but the master plan still keeps broader multi-joint IK as a Phase 6 gap.
- Current animation transforms are translation, positive scale, and Z-rotation-only radians. This slice stays in that planar contract and does not introduce quaternions or full 3D orientation.
- FABRIK is a small deterministic first-party algorithm here; no third-party dependency is added.

## Constraints

- Keep the API in `mirakana::` under `mirakana_animation`; no renderer, RHI, editor, asset, glTF, or platform dependency.
- Inputs and outputs are value types with stable English diagnostics. Invalid inputs return `false` instead of throwing.
- Preserve segment lengths deterministically, clamp targets beyond maximum reach when requested, and report whether the target was actually reached.
- Behavior changes use RED -> GREEN tests before production code.

## Done When

- [x] `mirakana_animation` exposes `solve_animation_fabrik_ik_xy_chain` with segment lengths, root/target, optional initial joint positions, tolerance, max iterations, and deterministic diagnostics.
- [x] Output includes joint positions, per-segment Z rotations, end-effector error, iteration count, `reached`, and `target_was_unreachable`.
- [x] `mirakana_core_tests` covers a reachable 3-segment chain, outer-unreachable clamping, inner-minimum-reach clamping, and invalid input diagnostics.
- [x] Master plan, plan registry, `docs/roadmap.md`, and `engine/agent/manifest.json` record the completed boundary without claiming full 3D orientation, joint limits, animation graph authoring, or generated 3D production readiness.
- [x] Focused build/tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` are recorded below, or concrete local blockers are documented.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| RED `mirakana_core_tests` build after adding tests and before adding production source | PASS | Failed on missing `mirakana/animation/chain_ik.hpp`, proving the new tests exercised the intended public surface first. |
| `cmd /d /c 'set Path=%Path%&& set PATH=&& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" mirakana_core_tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:diagnostic /clp:Summary;ErrorsOnly'` | PASS | Focused Windows build succeeded after adding `engine/animation/include/mirakana/animation/chain_ik.hpp`, `engine/animation/src/chain_ik.cpp`, and the CMake target source. The unsanitized `cmake --build --preset dev --target mirakana_core_tests --verbose` still reproduces this host's duplicate `Path`/`PATH` MSBuild `MSB6001` environment issue, so the focused build used a child-process environment with a single `Path`. |
| `out\build\dev\Debug\mirakana_core_tests.exe` | PASS | New tests passed for reachable 3-segment FABRIK, outer unreachable clamp, inner minimum-reach clamp, and invalid diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Formatting completed after implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check reported `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check reported `public-api-boundary-check: ok` after adding `chain_ik.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration check reported `ai-integration-check: ok` after manifest/check updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final repository validation reported `validate: ok` after docs/manifest synchronization. |

## Non-Goals

- Full 3D joint orientation, quaternion interpolation, twist extraction, pole vectors, angular limits, CCD, skeletal pose write-back, IK assets, glTF IK import, animation graph authoring, GPU morph, renderer/RHI integration, or generated-game production readiness.
