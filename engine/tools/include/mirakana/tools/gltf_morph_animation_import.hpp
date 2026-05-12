// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/morph.hpp"
#include "mirakana/assets/asset_source_format.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace mirakana {

/// Result of importing one glTF triangle primitive morph target stack into `AnimationMorphMeshCpuDesc` (Khronos glTF
/// 2.0). Draco, sparse accessors, and skinning attributes are rejected with stable diagnostics (no exceptions on
/// failure).
struct GltfMorphMeshCpuImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationMorphMeshCpuDesc morph_mesh;
};

/// Result of importing one glTF `animations[animation_index]` morph `weights` channel for a node that references
/// `meshes[mesh_index]`, validated against `meshes[mesh_index].primitives[primitive_index].targets.size()`.
struct GltfMorphWeightsAnimationImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationMorphWeightsTrackDesc weights_track;
};

/// Result of importing one glTF `weights` animation channel into a first-party scalar float clip source document.
/// The output contains one track per morph target named `gltf/node/<node>/weights/<target-index>`.
struct GltfAnimationFloatClipImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationFloatClipSourceDocument clip;
};

/// Imports bind positions/normals/tangents plus morph deltas for a single triangle primitive without `JOINTS_0` /
/// `WEIGHTS_0`. `mesh.weights` becomes `morph_mesh.target_weights` when present and consistent.
[[nodiscard]] GltfMorphMeshCpuImportReport
import_gltf_morph_mesh_cpu_primitive(std::string_view document_bytes_utf8,
                                     std::string_view source_path_for_external_buffers, std::size_t mesh_index,
                                     std::size_t primitive_index);

/// Imports the single `weights` animation channel targeting `animated_node_index`, which must reference `mesh_index`.
/// Only `LINEAR` interpolation is supported. Output accessor must store `times.size() * morph_target_count` float
/// scalars ordered by time, then morph target index.
[[nodiscard]] GltfMorphWeightsAnimationImportReport import_gltf_animation_morph_weights_for_mesh_primitive(
    std::string_view document_bytes_utf8, std::string_view source_path_for_external_buffers,
    std::size_t animation_index, std::size_t mesh_index, std::size_t primitive_index, std::size_t animated_node_index);

/// Imports the selected glTF morph `weights` animation as `GameEngine.AnimationFloatClipSource.v1` rows so the same
/// data can be cooked as `AssetKind::animation_float_clip` and sampled through `mirakana_animation` float tracks.
[[nodiscard]] GltfAnimationFloatClipImportReport import_gltf_morph_weights_animation_float_clip(
    std::string_view document_bytes_utf8, std::string_view source_path_for_external_buffers,
    std::size_t animation_index, std::size_t mesh_index, std::size_t primitive_index, std::size_t animated_node_index);

} // namespace mirakana
