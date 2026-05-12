# glTF Morph Mesh and Weights Animation Import v1 (2026-05-06)

**Plan ID:** `gltf-morph-mesh-and-weights-animation-import-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6  
**Status:** Completed on 2026-05-05 (validation recorded after implementation).

## Goal

Import Khronos glTF 2.0 **non-skinned** triangle primitives with **morph targets** into `AnimationMorphMeshCpuDesc`, bridge them into first-party `morph_mesh_cpu` source/cook/runtime package rows, and import **LINEAR** animation sampler channels with `path: "weights"` into a validated `AnimationMorphWeightsTrackDesc` suitable for `sample_animation_morph_weights_at_time` feeding `AnimationMorphMeshCpuDesc::target_weights`.

## Context

- CPU morph evaluation already exists in `MK_animation` (`morph.hpp` / `morph.cpp`).
- Skin/ joint animation import exists in `gltf_skin_animation_import.`*; morph geometry and `weights` channels were out of scope there.
- Static mesh cook still rejects skinning attributes; this slice keeps morph CPU data in the dedicated `morph_mesh_cpu` asset kind and does not make static mesh payloads carry morph rows.
- `GameEngine.MorphMeshCpuSource.v1` / `GameEngine.CookedMorphMeshCpu.v1` rows, `AssetKind::morph_mesh_cpu`, and `runtime_morph_mesh_cpu_payload` are part of the validated first-party package path for imported morph CPU data.

## Constraints

- **glTF 2.0** semantics only; Draco-compressed primitives, sparse accessors, and non-`LINEAR` sampler interpolation are **rejected** with stable diagnostics (no silent shim).
- Triangle primitives only; **reject** primitives that declare `JOINTS_0` / `WEIGHTS_0` (skinning) to keep the slice host-independent and unambiguous vs morph target weights.
- Weight samples must be **finite** and within **[0, 1]** to match `validate_animation_morph_mesh_cpu_desc`.
- `mesh.weights` length must match morph target count when present; default weights are **zero** when absent.
- `animations[].channels` for `weights`: **exactly one** channel per animated node for this import entry point (duplicate channels are rejected).
- Public API in `mirakana::`; run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` when touching public headers.

## Done When

- [x] `AnimationMorphWeightsTrackDesc` + validation + `sample_animation_morph_weights_at_time` in `MK_animation` with `MK_core_tests` coverage.
- [x] `import_gltf_morph_mesh_cpu_primitive` and `import_gltf_animation_morph_weights_for_mesh_primitive` in `MK_tools` (behind `MK_HAS_ASSET_IMPORTERS`) with `MK_tools_tests` glTF buffer + JSON cases.
- [x] glTF morph mesh CPU import can flow through `AssetImportActionKind::morph_mesh_cpu`, `GameEngine.CookedMorphMeshCpu.v1`, `AssetKind::morph_mesh_cpu`, package index assembly, and `runtime_morph_mesh_cpu_payload`.
- [x] `docs/superpowers/plans/README.md`, master plan Phase 6 child list, `engine/agent/manifest.json` pointers updated after completion.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (or documented local-tool blocker) recorded in the validation table below.

## Validation Evidence


| Command   | Result | Evidence |
| --------- | ------ | -------- |
| `out\build\dev\Debug\MK_core_tests.exe` | Passed | Covers CPU morph POSITION/NORMAL/TANGENT validation/application and `AnimationMorphWeightsTrackDesc` sampling. |
| `out\build\dev\Debug\MK_tools_tests.exe` | Passed | Includes `gltf morph mesh and weights animation import succeeds when importers are enabled`, which also cooks `morph_mesh_cpu`, packages it, and loads `runtime_morph_mesh_cpu_payload`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`; all 29 CTest tests passed. Metal and Apple checks remained diagnostic-only host gates. |


## Non-Goals

- GPU morph execution, renderer/RHI scene binding for morph deformation, sparse accessor support, Draco decode, STEP/CUBICSPLINE samplers, glTF materials, and generated-game morph authoring UX.
