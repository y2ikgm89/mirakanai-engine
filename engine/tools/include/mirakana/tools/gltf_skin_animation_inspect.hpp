// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/mat4.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/// One glTF skin entry (Khronos glTF 2.0 `skins[]`; read-only tooling summary).
struct GltfSkinInspectSummary {
    std::size_t skin_index{0};
    std::string name;
    std::size_t joint_count{0};
    bool has_inverse_bind_matrices_accessor{false};
};

/// One glTF animation entry (`animations[]`; channel count only for v1 inspect).
struct GltfAnimationInspectSummary {
    std::size_t animation_index{0};
    std::string name;
    std::size_t channel_count{0};
};

/// Read-only summary of glTF skins, animations, and skinned mesh attributes (no cooked writes).
struct GltfSkinAnimationInspectReport {
    bool parse_succeeded{false};
    std::string diagnostic;
    std::vector<std::string> warnings;
    std::vector<GltfSkinInspectSummary> skins;
    std::vector<GltfAnimationInspectSummary> animations;
    /// Triangle primitives that declare both `JOINTS_0` and `WEIGHTS_0` (import is rejected until skin cook exists).
    std::size_t skinned_triangle_primitive_count{0};
};

/// Result of reading one glTF `skins[skin_index]` joint list and inverse bind matrices (IBM), if present.
/// Per Khronos glTF 2.0, when IBM is omitted each joint uses the identity matrix. This does not map glTF node
/// indices to `AnimationSkeletonDesc` joint indices (that mapping is Phase B+ cook contract work).
struct GltfSkinBindExtractReport {
    bool succeeded{false};
    std::string diagnostic;
    /// glTF `nodes[]` indices in the same order as `skins[].joints`.
    std::vector<std::uint32_t> joint_node_indices;
    /// One row per skin joint; identity-filled when IBM accessor is absent.
    std::vector<Mat4> inverse_bind_matrices;
};

/// Parses UTF-8 `.gltf` JSON (or GLB bytes) using the same external-buffer policy as mesh import.
/// When `MK_ENABLE_ASSET_IMPORTERS` is off, returns `parse_succeeded == false` with a stable diagnostic.
[[nodiscard]] GltfSkinAnimationInspectReport
inspect_gltf_skin_animation(std::string_view document_bytes_utf8, std::string_view source_path_for_external_buffers);

/// Loads joint node indices and IBM values for `gltf.skins[skin_index]`.
/// On validation failure sets `succeeded == false` and a stable `diagnostic` (no exceptions).
[[nodiscard]] GltfSkinBindExtractReport extract_gltf_skin_bind_data(std::string_view document_bytes_utf8,
                                                                    std::string_view source_path_for_external_buffers,
                                                                    std::size_t skin_index);

} // namespace mirakana
