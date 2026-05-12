# Animation Two-Bone IK XY Plane v1 (2026-05-06)

**Plan ID:** `animation-two-bone-ik-xy-plane-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-06.

## Goal

Provide a **minimal analytic two-bone IK** solver in the **parent XY plane** (consistent with engine joint **Z-rotation** DOF usage elsewhere): given positive bone lengths and a 2D target in the parent frame, compute **parent** and **child** joint Z rotations with deterministic elbow branch selection.

## Context

- glTF does not standardize IK; this is first-party engine math only.
- Intended as a foundation for later gameplay / rigging slices without coupling to RHI or glTF import.

## Constraints

- **Breaking changes allowed** in new public types (first introduction).
- Solver is **pure** `mirakana_animation` + `mirakana_math` (`Vec2`); no filesystem, no asset formats.
- Reject non-finite inputs, non-positive bone lengths, degenerate targets, and unreachable targets (distance outside `[|L0-L1|, L0+L1]` with epsilon).

## Done When

- [x] `solve_animation_two_bone_ik_xy_plane` (+ diagnostics) implemented and covered by `mirakana_core_tests`.
- [x] Manifest / master plan / README reference this slice alongside the morph import slice where appropriate.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build out/build/dev --parallel 1 --target mirakana_core_tests` | PASS | MSVC dev preset; `animation two bone IK reaches planar targets within reachable annulus` test PASS. |

## Non-Goals

- Full FABRIK/C CD chains, twist extraction, joint limits beyond reachability, integration with `AnimationSkeletonDesc` sampling, IK in glTF assets.
