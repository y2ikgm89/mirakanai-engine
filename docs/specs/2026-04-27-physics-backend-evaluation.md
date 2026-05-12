# Advanced 3D Physics Backend Evaluation

## Goal

Decide whether GameEngine should add a third-party advanced 3D physics backend now, and define the dependency boundary if one is added later.

## Decision

Do not integrate a third-party physics backend in the current wave.

Keep the default `mirakana_physics` module first-party, deterministic, dependency-light, and sufficient for gameplay tests, simple collision, replay coverage, and AI-generated sample games. When advanced 3D requirements exceed the first-party solver, prefer a future optional `mirakana_physics_jolt` adapter behind a manifest feature such as `physics-jolt`.

2026-05-09 Physics 1.0 gate update: Jolt/native middleware is explicitly excluded from the Physics 1.0 ready surface. The accepted 1.0 path is the first-party `mirakana_physics` surface only: package-authored collision rows, exact current-primitive queries, deterministic contact manifolds, opt-in CCD rows, character/dynamic policy rows, explicit distance-joint solving, and host-independent determinism budget gates. A future Jolt adapter remains allowed only as an optional dependency-gated plan, not as a prerequisite for the 1.0 ready claim.

## Rationale

Current first-party physics already covers deterministic 2D/3D body integration, 2D AABB/circle contacts, 3D AABB/sphere/capsule contacts, layers/masks, exact current-primitive sweeps, package-authored collision rows, deterministic contact manifolds, opt-in CCD rows, character/dynamic policy rows, explicit distance-joint solving, iterative solver configuration, stacked-body stability coverage, replay signatures, and count-based budget gates. Adding a full middleware backend now would increase build, packaging, documentation, legal, dependency, and API surface before the engine has accepted ready-surface requirements that require it.

Jolt Physics is the preferred future candidate because its upstream project is game-oriented, cross-platform, MIT licensed, active, available through vcpkg, and documents deterministic behavior support. Context7 review of upstream Jolt documentation on 2026-05-09 also confirmed relevant integration constraints: supported C++ compilers, no external dependencies beyond the standard library, no RTTI or exceptions by default, and explicit floating-point compiler-flag handling such as precise models / disabled contraction. Those requirements are appropriate for a future optional adapter gate, not an implicit 1.0 dependency.

PhysX remains worth tracking for specialized high-end simulation, but it is larger, has stronger SDK/tooling implications, and can introduce GPU/CUDA-oriented complexity that is not appropriate for the baseline engine path yet.

Bullet and ReactPhysics3D remain acceptable fallback candidates under permissive licenses, but they are less aligned with the current target of a modern optional production game backend than Jolt.

## Candidate Summary


| Candidate      | License / source status             | Fit now               | Notes                                                                             |
| -------------- | ----------------------------------- | --------------------- | --------------------------------------------------------------------------------- |
| Jolt Physics   | MIT, upstream GitHub and vcpkg port | Best future candidate | Game/VR focused, active, broad platform support, deterministic option documented. |
| NVIDIA PhysX   | BSD-3-Clause docs                   | Defer                 | Powerful, but heavier SDK/GPU/tooling implications.                               |
| Bullet Physics | Zlib license                        | Defer                 | Mature and broad, but older API shape and larger integration surface.             |
| ReactPhysics3D | Zlib license                        | Defer                 | Small and permissive, but less proven for the engine's future advanced 3D target. |


## Required Integration Boundary

If a third-party backend is added later:

- Add it only behind an optional vcpkg manifest feature.
- Create an adapter module such as `engine/physics/jolt`; do not make `mirakana_physics` depend on third-party headers.
- Expose only first-party `mirakana::` handles, shapes, materials, filters, contacts, and replay inputs.
- Keep native backend objects behind PIMPL or opaque first-party handles.
- Add deterministic conversion tests from first-party body/shape/material data into backend setup data.
- Add fixed-step replay tests, stacked-body tests, collision filtering tests, and a benchmark harness before claiming production readiness.
- Update `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md`, `vcpkg.json`, package metadata, and `engine/agent/manifest.json` in the same change.

For Physics 1.0, the machine-readable decision lives in `engine/agent/manifest.json.aiOperableProductionLoop.physicsBackendAdapterDecisions` with `physics-1-0-jolt-native-adapter` set to `excluded-from-1-0-ready-surface`.

## References

- Jolt Physics upstream: [https://github.com/jrouwe/JoltPhysics](https://github.com/jrouwe/JoltPhysics)
- Jolt deterministic simulation docs: [https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/](https://jrouwe.github.io/JoltPhysicsDocs/5.1.0/)
- Jolt vcpkg port metadata: [https://vcpkg.roundtrip.dev/ports/joltphysics](https://vcpkg.roundtrip.dev/ports/joltphysics)
- NVIDIA PhysX license docs: [https://nvidia-omniverse.github.io/PhysX/physx/5.1.1/docs/License.html](https://nvidia-omniverse.github.io/PhysX/physx/5.1.1/docs/License.html)
- Bullet documentation: [https://pybullet.org/Bullet/BulletFull/](https://pybullet.org/Bullet/BulletFull/)
- ReactPhysics3D documentation: [https://www.reactphysics3d.com/documentation/](https://www.reactphysics3d.com/documentation/)


