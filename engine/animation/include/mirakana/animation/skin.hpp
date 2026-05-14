// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/math/mat4.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

inline constexpr std::size_t animation_skin_max_influences = 4;

struct AnimationSkinJointDesc {
    std::size_t skeleton_joint_index{0};
    Mat4 inverse_bind_matrix{Mat4::identity()};
};

struct AnimationSkinVertexInfluence {
    std::size_t skin_joint_index{0};
    float weight{0.0F};
};

struct AnimationSkinVertexWeights {
    std::vector<AnimationSkinVertexInfluence> influences;
};

struct AnimationSkinPayloadDesc {
    std::uint32_t vertex_count{0};
    std::vector<AnimationSkinJointDesc> joints;
    std::vector<AnimationSkinVertexWeights> vertices;
};

enum class AnimationSkinDiagnosticCode : std::uint8_t {
    invalid_skeleton,
    empty_skin,
    invalid_skin_joint,
    duplicate_skin_joint,
    invalid_inverse_bind_matrix,
    bind_pose_mismatch,
    vertex_count_mismatch,
    invalid_vertex_influence_count,
    invalid_vertex_joint_index,
    invalid_vertex_weight,
    zero_vertex_weight,
};

struct AnimationSkinDiagnostic {
    std::size_t skin_joint_index{0};
    std::size_t vertex_index{0};
    std::size_t influence_index{0};
    AnimationSkinDiagnosticCode code{AnimationSkinDiagnosticCode::empty_skin};
    std::string message;
};

struct AnimationModelPose {
    std::vector<Mat4> joint_matrices;
};

struct AnimationSkinningPalette {
    std::vector<Mat4> matrices;
};

struct AnimationCpuSkinningDesc {
    std::vector<Vec3> bind_positions;
    AnimationSkinPayloadDesc skin;
    AnimationSkinningPalette palette;
    std::vector<Vec3> bind_normals;
    std::vector<Vec3> bind_tangents;
    std::vector<float> bind_tangent_handedness;
};

struct AnimationCpuSkinnedVertexStream {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec3> tangents;
    std::vector<Vec3> bitangents;
};

enum class AnimationCpuSkinningDiagnosticCode : std::uint8_t {
    empty_bind_positions,
    vertex_count_mismatch,
    palette_count_mismatch,
    invalid_bind_position,
    normal_count_mismatch,
    invalid_bind_normal,
    tangent_count_mismatch,
    invalid_bind_tangent,
    tangent_handedness_count_mismatch,
    tangent_handedness_missing_streams,
    invalid_tangent_handedness,
    invalid_palette_matrix,
    invalid_vertex_influence_count,
    invalid_vertex_joint_index,
    invalid_vertex_weight,
    zero_vertex_weight,
};

struct AnimationCpuSkinningDiagnostic {
    std::size_t vertex_index{0};
    std::size_t influence_index{0};
    std::size_t skin_joint_index{0};
    AnimationCpuSkinningDiagnosticCode code{AnimationCpuSkinningDiagnosticCode::empty_bind_positions};
    std::string message;
};

[[nodiscard]] std::vector<AnimationSkinDiagnostic>
validate_animation_skin_payload(const AnimationSkeletonDesc& skeleton, const AnimationSkinPayloadDesc& skin);
[[nodiscard]] bool is_valid_animation_skin_payload(const AnimationSkeletonDesc& skeleton,
                                                   const AnimationSkinPayloadDesc& skin) noexcept;
[[nodiscard]] AnimationSkinPayloadDesc normalize_animation_skin_payload(const AnimationSkeletonDesc& skeleton,
                                                                        const AnimationSkinPayloadDesc& skin);

[[nodiscard]] AnimationModelPose build_animation_model_pose(const AnimationSkeletonDesc& skeleton,
                                                            const AnimationPose& pose);
[[nodiscard]] AnimationSkinningPalette build_animation_skinning_palette(const AnimationSkeletonDesc& skeleton,
                                                                        const AnimationSkinPayloadDesc& skin,
                                                                        const AnimationPose& pose);

[[nodiscard]] std::vector<AnimationCpuSkinningDiagnostic>
validate_animation_cpu_skinning_desc(const AnimationCpuSkinningDesc& desc);
[[nodiscard]] bool is_valid_animation_cpu_skinning_desc(const AnimationCpuSkinningDesc& desc) noexcept;
[[nodiscard]] AnimationCpuSkinnedVertexStream skin_animation_vertices_cpu(const AnimationCpuSkinningDesc& desc);

} // namespace mirakana
