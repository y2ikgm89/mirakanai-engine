// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/assets/asset_source_format.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/// glTF node transform animation rows imported into first-party 2D/Z-rotation keyframe tracks. Rotation is
/// intentionally limited to z-axis-only quaternions because these rows feed `rotation_z_radians` scalar workflows.
struct GltfNodeTransformAnimationTrack {
    std::size_t node_index{0};
    std::string node_name;
    std::vector<Vec3Keyframe> translation_keyframes;
    std::vector<FloatKeyframe> rotation_z_keyframes;
    std::vector<Vec3Keyframe> scale_keyframes;
};

/// glTF node transform animation rows imported into first-party 3D keyframe tracks. Rotation keeps the source XYZW
/// unit quaternion for `AnimationPose3d` workflows.
struct GltfNodeTransformAnimationTrack3d {
    std::size_t node_index{0};
    std::string node_name;
    std::vector<Vec3Keyframe> translation_keyframes;
    std::vector<QuatKeyframe> rotation_keyframes;
    std::vector<Vec3Keyframe> scale_keyframes;
};

/// Result of importing one glTF `animations[animation_index]` into ordinary node TRS keyframes. Only `LINEAR`
/// interpolation and `translation` / z-axis-only `rotation` / positive `scale` channels are supported.
struct GltfNodeTransformAnimationImportReport {
    bool succeeded{false};
    std::string diagnostic;
    std::vector<GltfNodeTransformAnimationTrack> node_tracks;
};

/// Result of importing one glTF `animations[animation_index]` into 3D node TRS keyframes. Only `LINEAR` interpolation
/// and `translation` / full unit-quaternion `rotation` / positive `scale` channels are supported.
struct GltfNodeTransformAnimationImport3dReport {
    bool succeeded{false};
    std::string diagnostic;
    std::vector<GltfNodeTransformAnimationTrack3d> node_tracks;
};

/// Result of importing one glTF node transform animation into a first-party scalar float clip source document. The
/// output contains stable scalar targets named `gltf/node/<node>/translation/<x|y|z>`,
/// `gltf/node/<node>/rotation_z`, and `gltf/node/<node>/scale/<x|y|z>`.
struct GltfNodeTransformAnimationFloatClipImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationFloatClipSourceDocument clip;
};

/// Result of importing one glTF node transform animation into a first-party 3D TRS quaternion clip source document.
/// The output contains one track per selected glTF node target named `gltf/node/<node>`.
struct GltfNodeTransformAnimationQuaternionClipImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationQuaternionClipSourceDocument clip;
};

/// Result of importing glTF node transform animation targets into authored transform binding source rows. The target
/// names match `import_gltf_node_transform_animation_float_clip`; `node_name` is the glTF node name, or
/// `gltf_node_<index>` for unnamed nodes.
struct GltfNodeTransformAnimationBindingSourceImportReport {
    bool succeeded{false};
    std::string diagnostic;
    AnimationTransformBindingSourceDocument binding_source;
};

/// Imports all node transform channels in the selected glTF animation. Channels are grouped by target node and sorted
/// by `node_index`. glTF `weights` channels stay on the morph-weight importer path and are rejected here.
[[nodiscard]] GltfNodeTransformAnimationImportReport
import_gltf_node_transform_animation_tracks(std::string_view document_bytes_utf8,
                                            std::string_view source_path_for_external_buffers,
                                            std::size_t animation_index);

/// Imports all node transform channels in the selected glTF animation into 3D local-pose-friendly keyframes. Channels
/// are grouped by target node and sorted by `node_index`. glTF `weights` channels stay on the morph-weight importer
/// path and are rejected here.
[[nodiscard]] GltfNodeTransformAnimationImport3dReport
import_gltf_node_transform_animation_tracks_3d(std::string_view document_bytes_utf8,
                                               std::string_view source_path_for_external_buffers,
                                               std::size_t animation_index);

/// Imports the selected glTF node transform animation as `GameEngine.AnimationFloatClipSource.v1` rows so ordinary TRS
/// animation can use the existing `AssetKind::animation_float_clip` cook/package/runtime path.
[[nodiscard]] GltfNodeTransformAnimationFloatClipImportReport
import_gltf_node_transform_animation_float_clip(std::string_view document_bytes_utf8,
                                                std::string_view source_path_for_external_buffers,
                                                std::size_t animation_index);

/// Imports the selected glTF node transform animation as `GameEngine.AnimationQuaternionClipSource.v1` rows so 3D
/// local-pose workflows can use the `AssetKind::animation_quaternion_clip` cook/package/runtime path.
[[nodiscard]] GltfNodeTransformAnimationQuaternionClipImportReport
import_gltf_node_transform_animation_quaternion_clip(std::string_view document_bytes_utf8,
                                                     std::string_view source_path_for_external_buffers,
                                                     std::size_t animation_index);

/// Imports the selected glTF node transform animation as `GameEngine.AnimationTransformBindingSource.v1` rows that can
/// be resolved by `mirakana_runtime_scene` against loaded scene node names.
[[nodiscard]] GltfNodeTransformAnimationBindingSourceImportReport
import_gltf_node_transform_animation_binding_source(std::string_view document_bytes_utf8,
                                                    std::string_view source_path_for_external_buffers,
                                                    std::size_t animation_index);

} // namespace mirakana
