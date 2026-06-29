# Asset Import Coordinate Normalization v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Consume `GameEngine.AssetImportPresets.v1` `mesh.up_axis` and `mesh.unit_scale` in the `MK_tools` mesh import/cook path so imported mesh positions, normals, tangents, morph streams, and transform animation data are written in one project coordinate convention before runtime packaging.

**Architecture:** Keep `MK_assets` as the preset authority, pass the effective mesh preset through first-party typed import actions, and apply one shared `MK_tools` coordinate-normalization helper at every glTF data boundary. Runtime and game code continue to consume cooked first-party artifacts only. Parser types, source file parsing, external-engine project formats, and native handles stay outside editor/runtime public contracts.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_animation`, `MK_editor_core`, optional `fastgltf` behind the existing `asset-importers` vcpkg feature, repository PowerShell validation wrappers.

---

## Locked Decisions

- Project coordinate convention for this plan is right-handed, `+Y` up, meter units, and positive scale. This matches glTF 2.0's default coordinate convention, so default `AssetImportMeshPresetV1{ .up_axis = y, .unit_scale = 1.0F }` is an identity transform.
- `mesh.up_axis` means the source asset's up axis. `mesh.unit_scale` means the positive multiplier from source linear distance units into project meters. Examples: meters use `1.0`, centimeters use `0.01`.
- `AssetImportMeshUpAxis::z` maps source `+Z` up into project `+Y` up using an orientation-preserving rotation around `+X` by `-pi/2`: source `(x, y, z)` becomes project `(x, z, -y)`.
- Positions, translations, bind positions, and morph POSITION deltas are rotated and multiplied by `unit_scale`.
- Normals, tangent xyz directions, morph NORMAL deltas, and morph TANGENT deltas are rotated only, then normalized when the destination contract requires unit vectors. Tangent `w` is preserved because the supported transforms have positive determinant and do not mirror handedness.
- Scale tracks stay dimensionless. For `z` source up, scale components are permuted from `(x, y, z)` to `(x, z, y)`.
- Quaternions are normalized with basis conjugation: `project_q = normalize(axis_q * source_q * conjugate(axis_q))`, where `axis_q = Quat::from_axis_angle(Vec3{1,0,0}, -pi/2)` for `z` source up.
- Inverse bind matrices, when a supported skin path consumes this preset, must be converted as `project_from_source * source_inverse_bind * source_from_project`. This keeps `project_joint * project_inverse_bind * project_vertex` equivalent to the source skinning result after coordinate conversion.
- Triangle winding is not reversed for this plan because `unit_scale` is positive and the Y/Z basis conversion is a rotation with determinant `+1`.
- UVs, joint indices, skin weights, morph weights, material references, texture presets, and audio presets are not coordinate-normalized.
- No backward-compatibility shim is required. This repository is greenfield; public tool functions that import glTF transform-bearing data should take an explicit `AssetImportMeshPresetV1` parameter and all call sites/tests should be updated in the same slice.

## Current Project Facts

- `AssetImportMeshPresetV1` exists in `engine/assets/include/mirakana/assets/asset_import_presets.hpp` with `float unit_scale{1.0F}` and `AssetImportMeshUpAxis up_axis{AssetImportMeshUpAxis::y}`.
- `AssetImportAction` now carries typed `AssetImportMeshPresetV1 mesh_preset` in addition to metadata strings, so tool consumption does not parse UI/review text.
- `editor/core/src/asset_import_review.cpp` now copies the effective reviewed mesh preset into `EditorAssetImportCandidateRow::mesh_preset` and `AssetImportAction::mesh_preset`.
- `engine/tools/asset/asset_import_adapters.cpp` now applies mesh preset coordinate normalization to selected glTF mesh `POSITION`, `NORMAL`, and `TANGENT` streams before writing `GameEngine.MeshSource.v2`.
- `engine/tools/gltf/gltf_morph_animation_import.cpp` now applies mesh preset coordinate normalization to bind positions/normals/tangents and POSITION/NORMAL/TANGENT morph deltas while leaving morph weights unchanged.
- `engine/tools/gltf/gltf_node_animation_import.cpp` now normalizes selected 3D quaternion transform animation rows, keeps y-up scalar `rotation_z` behavior with unit-scaled translations, and fails closed for z-up scalar rotation channels.
- `engine/tools/gltf/gltf_skin_animation_import.cpp` now applies y-up `unit_scale` to scalar skin rest/track translations and fails closed for z-up until a 3D quaternion skin path is intentionally introduced.
- `engine/agent/manifest.fragments/007-importerCapabilities.json` now records selected mesh preset consumption while keeping texture/audio presets and unsupported transform paths value-only or fail-closed.

## Official Source Ledger

| Source | Use in this plan |
| --- | --- |
| Khronos glTF 2.0 specification, `https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html` | Confirms the official glTF basis used for source interpretation: right-handed, `+Y` up, meters, TRS/matrix node transforms. |
| Context7 `/spnda/fastgltf` and fastgltf tools docs, `https://github.com/spnda/fastgltf/blob/main/docs/tools.rst` | Use `fastgltf::iterateAccessor`, `iterateAccessorWithIndex`, and `getAccessorElement` instead of hand-decoding buffers. These helpers account for accessor conversion, normalization, sparse data, and strides. Use `getLocalTransformMatrix` / `getTransformMatrix` when a matrix boundary must be reasoned about. |
| Context7 `/kitware/cmake` and CMake `target_sources`, `https://cmake.org/cmake/help/latest/command/target_sources.html` | Keep source additions and dependency exposure target-scoped. New helper sources go through target-scoped `MK_tools_*` CMake entries; no global include/link settings. |
| Context7 `/microsoft/vcpkg`, `https://github.com/microsoft/vcpkg/blob/master/scripts/buildsystems/vcpkg.cmake` | Keep `fastgltf` behind the existing optional `asset-importers` feature and repository bootstrap. Do not move dependency installation into CMake configure. |
| Unity trademark page, `https://unity.com/legal/trademarks`; Unreal Engine content EULA, `https://www.unrealengine.com/en-US/eula/content`; Godot license compliance docs, `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html` | Legal/originality guardrails only. Do not copy engine UI, code, samples, schemas, assets, or compatibility language. Do not claim Unity/Unreal/Godot import compatibility, parity, equivalence, replacement, or endorsement. This is not legal advice. |

## High Priority

- [x] Add first-party typed preset flow from editor review to import actions.
  - File: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
  - Add `#include "mirakana/assets/asset_import_presets.hpp"` and `AssetImportMeshPresetV1 mesh_preset;` to `AssetImportAction`.
  - File: `engine/assets/include/mirakana/assets/asset_import_presets.hpp`
  - Add `AssetImportMeshPresetV1 mesh;` to `AssetImportPresetReviewV1` so editor review returns the effective mesh preset without requiring string metadata parsing.
  - Add `[[nodiscard]] bool is_valid_asset_import_mesh_preset_v1(const AssetImportMeshPresetV1& preset) noexcept;` so action validation and tool helpers do not duplicate preset rules.
  - File: `engine/assets/src/asset_import_pipeline.cpp`
  - Require `is_valid_asset_import_mesh_preset_v1(action.mesh_preset)` inside `is_valid_asset_import_action`.
  - File: `editor/core/include/mirakana/editor/asset_import_review.hpp`
  - Add `AssetImportMeshPresetV1 mesh_preset;` to `EditorAssetImportCandidateRow`.
  - File: `editor/core/src/asset_import_review.cpp`
  - Set `row.mesh_preset` from the effective `review_asset_import_preset_for_asset` result and copy it in `make_import_action`.
  - Required test first: update `tests/unit/editor_core_tests.cpp` to assert the reviewed mesh row and `ready.import_plan.actions[...]` carry `unit_scale == 0.01F` and `up_axis == AssetImportMeshUpAxis::z` when a per-asset override is supplied.

- [x] Introduce the shared coordinate-normalization helper.
  - File: `engine/tools/include/mirakana/tools/asset_coordinate_normalization.hpp`
  - File: `engine/tools/asset/asset_coordinate_normalization.cpp`
  - File: `engine/tools/asset/CMakeLists.txt`
  - Add the implementation source to `MK_tools_asset` with the existing target-scoped source list. Do not add parser dependencies to this helper.
  - Add an API shaped like:

```cpp
struct AssetCoordinateNormalizationPlan {
    AssetImportMeshPresetV1 preset;
    Quat project_from_source_rotation{Quat::identity()};
    Mat4 project_from_source{Mat4::identity()};
    Mat4 source_from_project{Mat4::identity()};
    bool changes_coordinates{false};
};

[[nodiscard]] AssetCoordinateNormalizationPlan
make_asset_coordinate_normalization_plan(const AssetImportMeshPresetV1& preset);
[[nodiscard]] Vec3 normalize_asset_position(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept;
[[nodiscard]] Vec3 normalize_asset_direction(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept;
[[nodiscard]] Vec3 normalize_asset_scale(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept;
[[nodiscard]] Quat normalize_asset_rotation(const AssetCoordinateNormalizationPlan& plan, Quat value) noexcept;
[[nodiscard]] Mat4 normalize_asset_inverse_bind_matrix(const AssetCoordinateNormalizationPlan& plan, Mat4 value) noexcept;
```

  - Required test first: add `tests/unit/tools_tests.cpp` coverage proving:
    - identity preset leaves position, direction, scale, and quaternion unchanged;
    - `up_axis=z`, `unit_scale=0.01F` maps position `(1,2,3)` to `(0.01,0.03,-0.02)`;
    - source `+Z` normal maps to project `+Y`;
    - source `Z` rotation maps to project `Y` rotation by quaternion basis conjugation;
    - scale `(1,2,3)` maps to `(1,3,2)`;
    - inverse bind matrix conversion composes as `project_from_source * source_inverse_bind * source_from_project`.

- [x] Apply the helper to glTF mesh source import.
  - File: `engine/tools/asset/asset_import_adapters.cpp`
  - Change `mesh_document_from_gltf` to take `const AssetImportMeshPresetV1& mesh_preset`.
  - In `GltfMeshExternalAssetImporter::import_source_document`, pass `action.mesh_preset`.
  - In `append_gltf_positions`, normalize each `POSITION` with position rules before writing `mesh.vertex_data_hex`.
  - In `append_gltf_tangent_space_vertices`, normalize `POSITION`, `NORMAL`, and tangent xyz; keep `TEXCOORD_0` and tangent `w` unchanged.
  - Continue using `fastgltf::iterateAccessor` / `iterateAccessorWithIndex` for official accessor handling, including sparse accessors, custom strides, zero-initialized accessors, and normalized/component-converted data where glTF and the engine document contract allow them.
  - Required test first: add a glTF triangle import test with `mesh_preset.up_axis = z` and `mesh_preset.unit_scale = 0.01F`; deserialize `GameEngine.MeshSource.v2` and assert the written float32 vertex payload contains normalized positions/normals/tangents and unchanged indices/UV/tangent `w`.

- [x] Apply the helper to morph mesh CPU import.
  - File: `engine/tools/include/mirakana/tools/gltf_morph_animation_import.hpp`
  - Change `import_gltf_morph_mesh_cpu_primitive` to take `const AssetImportMeshPresetV1& mesh_preset`.
  - File: `engine/tools/gltf/gltf_morph_animation_import.cpp`
  - Remove local sparse-accessor rejection for VEC3 streams that `fastgltf::iterateAccessor` can legally materialize. Keep explicit type/count/range validation for the first-party destination contract.
  - Normalize bind positions and POSITION deltas with position rules.
  - Normalize bind normals, bind tangents, NORMAL deltas, and TANGENT deltas with direction rules.
  - Leave morph weights and weight animation unchanged.
  - File: `engine/tools/asset/asset_import_adapters.cpp`
  - Pass `action.mesh_preset` from `GltfMorphMeshCpuExternalAssetImporter`.
  - Required test first: add z-up morph import coverage where bind position, bind normal, bind tangent, and one POSITION/NORMAL/TANGENT target delta are converted, while `mesh.weights` stays byte-identical.

- [x] Apply the helper to 3D node transform animation import.
  - File: `engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp`
  - Change `import_gltf_node_transform_animation_tracks_3d` and `import_gltf_node_transform_animation_quaternion_clip` to take `const AssetImportMeshPresetV1& mesh_preset`.
  - File: `engine/tools/gltf/gltf_node_animation_import.cpp`
  - Remove local sparse-accessor rejection where `fastgltf::iterateAccessor` / `getAccessorElement` can legally materialize output samples. Keep explicit time ordering, count, interpolation, finite, positive-scale, and quaternion validation.
  - Normalize translation keyframes with position rules.
  - Normalize quaternion rotation keyframes with quaternion basis conjugation.
  - Normalize scale keyframes with scale permutation rules.
  - Keep interpolation validation, duplicate-channel rejection, and quaternion validation fail-closed.
  - Required test first: add a z-up quaternion animation import test proving translation `(1,2,3)` becomes `(1,3,-2)`, scale `(1,2,3)` becomes `(1,3,2)`, and a source Z-axis rotation samples as a project Y-axis rotation.

- [x] Keep scalar `rotation_z` transform paths honest.
  - File: `engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp`
  - Change `import_gltf_node_transform_animation_tracks`, `import_gltf_node_transform_animation_float_clip`, and `import_gltf_node_transform_animation_binding_source` to take `const AssetImportMeshPresetV1& mesh_preset`.
  - File: `engine/tools/gltf/gltf_node_animation_import.cpp`
  - For `up_axis=y`, keep current scalar `rotation_z` behavior and still apply `unit_scale` to translation.
  - For `up_axis=z`, support translation/scale-only clips by writing project-space component targets, but reject any rotation channel with a diagnostic that says z-up rotation conversion requires the quaternion clip path.
  - Required test first: add one z-up translation/scale-only float clip test and one z-up rotation float clip rejection test.

- [x] Fail closed on current z-axis-only skin import instead of silently producing wrong animation transforms.
  - File: `engine/tools/include/mirakana/tools/gltf_skin_animation_import.hpp`
  - Change `import_gltf_skin_skeleton_and_skin_payload` and `import_gltf_animation_joint_tracks_for_skin` to take `const AssetImportMeshPresetV1& mesh_preset`.
  - File: `engine/tools/gltf/gltf_skin_animation_import.cpp`
  - For `up_axis=y`, apply `unit_scale` to joint rest translations and joint translation tracks.
  - For `up_axis=z`, return a stable diagnostic before import because `AnimationSkeletonDesc` only stores `rotation_z_radians` and cannot represent converted source Z rotations as project Y rotations. Keep inverse bind conversion out of this path until a 3D skin contract exists.
  - Required test first: add y-up `unit_scale=0.01F` skin translation coverage and z-up rejection coverage.

- [x] Add source/cooked evidence rows without creating a new parser/runtime surface.
  - File: `engine/assets/include/mirakana/assets/asset_source_format.hpp`
  - File: `engine/assets/src/asset_source_format.cpp`
  - Decision: no source/cooked schema field was added in this slice. The cooked/source payloads are already project-space values, and adding optional metadata to `GameEngine.MeshSource.v2` / `GameEngine.MorphMeshCpuSource.v1` would create a second truth that runtime code does not need. Package-visible coordinate review evidence remains a medium-priority value-row follow-up.
  - If a future source document format adds normalized-coordinate metadata, keep the clean-break format name honest by updating the schema contract and parser validation in the same slice.

- [x] Update docs, manifest fragments, and static contracts after behavior is green.
  - Files:
    - `docs/current-capabilities.md`
    - `docs/editor.md`
    - `engine/agent/manifest.fragments/004-modules.json`
    - `engine/agent/manifest.fragments/007-importerCapabilities.json`
  - Replace the current "presets are metadata only" statement with the narrower truth: mesh `up_axis` and `unit_scale` are consumed by selected `MK_tools` glTF mesh/morph/3D animation import paths; texture/audio and unsupported transform paths remain value-only or fail-closed.
  - Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after manifest fragment edits.

## Medium Priority

- [ ] Add package-visible review evidence for normalized coordinates.
  - Extend existing asset import production review rows so a package or import review can report `mesh.coordinates_normalized`, `mesh.source_up_axis`, `mesh.unit_scale`, and `mesh.target_up_axis`.
  - Add tests that prove these rows are value-only and do not execute codecs, mutate files, or parse runtime source assets.

- [ ] Add editor Source Pulse row display and filtering for normalized coordinate state.
  - Surface reviewed coordinate metadata in `EditorAssetImportCandidateRow` and the asset browser model without adding visible external-engine terminology.
  - Add editor-core tests for metadata projection and unsupported z-up scalar rotation diagnostics.

- [ ] Design the follow-up 3D skin import contract.
  - Use `AnimationSkeleton3dDesc`, quaternion rest transforms, 3D joint tracks, and converted inverse bind matrices.
  - Keep this out of the high-priority implementation unless the current `AnimationSkinPayloadDesc` can be proven to support 3D skeleton validation without shims.

- [ ] Add a retained validation recipe for coordinate-normalized sample imports.
  - The recipe should generate synthetic glTF inputs in an ignored temp/output location, run the importer/cook path, and assert project-space payload values.
  - Do not add third-party assets to the repository.

## Deferred Or Explicitly Out Of Scope

- [ ] Arbitrary source axes beyond `y` and `z`.
- [ ] Left-handed source coordinate conversion, negative scale, mirroring, or automatic winding reversal.
- [ ] Assimp or broad model-format import.
- [ ] Runtime parsing of `.gltf`, `.glb`, or any source asset file.
- [ ] Unity `.unity` / `.prefab`, Unreal `.uasset` / `.uproject`, Godot `.tscn` / `.godot`, or marketplace package import.
- [ ] Compatibility, parity, replacement, or equivalence claims against Unity, Unreal Engine, Godot, or their asset pipelines.
- [ ] Copying external-engine UI expression, code, sample content, asset schemas, naming conventions, or documentation prose.

## Validation Plan

Run focused loops during implementation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_tools_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_tools_tests|MK_editor_core_tests"
```

Run targeted static checks after code is green:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/assets/include/mirakana/assets/asset_import_pipeline.hpp,engine/assets/include/mirakana/assets/asset_import_presets.hpp,engine/assets/src/asset_import_pipeline.cpp,engine/assets/src/asset_import_presets.cpp,editor/core/include/mirakana/editor/asset_import_review.hpp,editor/core/src/asset_import_review.cpp,engine/tools/include/mirakana/tools/asset_coordinate_normalization.hpp,engine/tools/asset/asset_coordinate_normalization.cpp,engine/tools/asset/asset_import_adapters.cpp,engine/tools/gltf/gltf_morph_animation_import.cpp,engine/tools/gltf/gltf_node_animation_import.cpp,engine/tools/gltf/gltf_skin_animation_import.cpp,tests/unit/core_tests.cpp,tests/unit/tools_tests.cpp,tests/unit/editor_core_tests.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Run full slice closeout once behavior, docs, and manifest are settled:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected closeout:

- `MK_tools_tests` proves z-up/unit-scale mesh, morph, and 3D transform animation import values are baked into project convention.
- `MK_editor_core_tests` proves reviewed presets flow into typed import actions.
- `MK_core_tests` proves preset validation and source document serialization remain deterministic.
- `check-json-contracts`, `check-ai-integration`, and `check-agents` pass after manifest/docs changes.
- No new dependency, third-party asset, external-engine import surface, compatibility claim, or legal-advice claim is added.

## Validation Evidence

2026-06-30 worktree evidence:

- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_editor_core_tests MK_tools_tests`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests|MK_tools_tests"`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/assets/src/asset_import_pipeline.cpp,engine/assets/src/asset_import_presets.cpp,editor/core/src/asset_import_review.cpp,engine/tools/asset/asset_coordinate_normalization.cpp,engine/tools/asset/asset_import_adapters.cpp,engine/tools/gltf/gltf_morph_animation_import.cpp,engine/tools/gltf/gltf_node_animation_import.cpp,engine/tools/gltf/gltf_skin_animation_import.cpp,tests/unit/core_tests.cpp,tests/unit/tools_tests.cpp,tests/unit/editor_core_tests.cpp"`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (`159/159` tests passed).
- Blocked optional lane: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset asset-importers` fails before compile because `SPNGConfig.cmake` / `spng-config.cmake` is missing from `vcpkg_installed`. The required bootstrap command, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers`, was rejected in this session because command policy requires approval and the active approval policy is `never`. CI or an approval-capable local session should run `tools/bootstrap-deps.ps1 -Feature asset-importers` followed by `tools/build-asset-importers.ps1`.

## Done When

- `AssetImportPresets.v1` mesh `up_axis` / `unit_scale` are consumed by selected `MK_tools` glTF mesh, morph, and 3D transform animation import paths.
- Unsupported transform paths fail closed with stable diagnostics rather than silently writing mixed coordinate data.
- Runtime/package artifacts contain only first-party project-coordinate data.
- Docs and manifest fragments state the exact supported and unsupported preset-consumption scope.
- Legal/originality review remains clean: official glTF/fastgltf/CMake/vcpkg docs only, synthetic tests only, and no Unity/Unreal/Godot code/assets/UI/schema/compatibility claim.
