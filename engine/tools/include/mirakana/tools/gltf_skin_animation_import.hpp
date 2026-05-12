// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/animation/skin.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/// Result of importing one glTF skin plus one skinned triangle primitive into `mirakana_animation` skeleton + skin
/// payload models (Khronos glTF 2.0). Unsupported joint rotations (non z-axis-only quaternions), invalid scales, or
/// validation failures set `succeeded == false` with a stable `diagnostic` (no exceptions).
struct GltfSkinSkeletonAndSkinPayloadImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationSkeletonDesc skeleton;
    AnimationSkinPayloadDesc skin_payload;
};

/// Result of importing one glTF `animations[animation_index]` into `AnimationJointTrackDesc` rows for joints listed
/// in `skins[skin_index].joints`. Only `LINEAR` samplers, `Translation` / `Rotation` / `Scale` paths are supported;
/// `Rotation` must be z-axis-only quaternions (maps to `rotation_z_keyframes`). Other paths or samplers are rejected.
struct GltfAnimationJointTracksImportReport {
    bool succeeded{false};
    std::string diagnostic;
    std::vector<AnimationJointTrackDesc> joint_tracks;
};

/// Builds `AnimationSkeletonDesc` from `skins[skin_index].joints` node local TRS (parent links inferred from glTF
/// `nodes[].children`) and `AnimationSkinPayloadDesc` from the given triangle primitive (`JOINTS_0` + `WEIGHTS_0`).
/// Joint names are deterministic (`j0`, `j1`, …) for stable tooling output.
[[nodiscard]] GltfSkinSkeletonAndSkinPayloadImportReport
import_gltf_skin_skeleton_and_skin_payload(std::string_view document_bytes_utf8,
                                           std::string_view source_path_for_external_buffers, std::size_t skin_index,
                                           std::size_t mesh_index, std::size_t primitive_index);

/// Builds joint tracks for nodes targeted by `animations[animation_index]` that appear in `skins[skin_index].joints`.
/// Channels targeting other nodes are ignored. Fails on duplicate (joint, path) pairs or unsupported sampler/path data.
[[nodiscard]] GltfAnimationJointTracksImportReport
import_gltf_animation_joint_tracks_for_skin(std::string_view document_bytes_utf8,
                                            std::string_view source_path_for_external_buffers, std::size_t skin_index,
                                            std::size_t animation_index);

} // namespace mirakana
