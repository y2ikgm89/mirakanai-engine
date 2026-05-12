// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/skin.hpp"

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_matrix(const Mat4& value) noexcept {
    const auto& values = value.values();
    return std::ranges::all_of(values, [](float element) { return finite(element); });
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool valid_handedness_sign(float value) noexcept {
    return finite(value) && std::abs(std::abs(value) - 1.0F) <= 0.0001F;
}

[[nodiscard]] Vec3 normalize_or_zero(Vec3 value) noexcept {
    const auto magnitude = length(value);
    if (!finite(magnitude) || magnitude <= 0.000001F) {
        return Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }
    return value * (1.0F / magnitude);
}

[[nodiscard]] bool nearly_equal(float lhs, float rhs) noexcept {
    return std::abs(lhs - rhs) <= 0.0001F;
}

[[nodiscard]] bool nearly_identity(const Mat4& value) noexcept {
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            const auto expected = row == column ? 1.0F : 0.0F;
            if (!nearly_equal(value.at(row, column), expected)) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool has_duplicate_skin_joint(const AnimationSkinPayloadDesc& skin, std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (skin.joints[other].skeleton_joint_index == skin.joints[index].skeleton_joint_index) {
            return true;
        }
    }
    return false;
}

void push_diagnostic(std::vector<AnimationSkinDiagnostic>& diagnostics, std::size_t skin_joint_index,
                     std::size_t vertex_index, std::size_t influence_index, AnimationSkinDiagnosticCode code,
                     std::string message) {
    diagnostics.push_back(AnimationSkinDiagnostic{.skin_joint_index = skin_joint_index,
                                                  .vertex_index = vertex_index,
                                                  .influence_index = influence_index,
                                                  .code = code,
                                                  .message = std::move(message)});
}

void push_cpu_diagnostic(std::vector<AnimationCpuSkinningDiagnostic>& diagnostics, std::size_t vertex_index,
                         std::size_t influence_index, std::size_t skin_joint_index,
                         AnimationCpuSkinningDiagnosticCode code, std::string message) {
    diagnostics.push_back(AnimationCpuSkinningDiagnostic{.vertex_index = vertex_index,
                                                         .influence_index = influence_index,
                                                         .skin_joint_index = skin_joint_index,
                                                         .code = code,
                                                         .message = std::move(message)});
}

[[nodiscard]] Mat4 local_transform_matrix(JointLocalTransform transform) noexcept {
    return Mat4::translation(transform.translation) * Mat4::rotation_z(transform.rotation_z_radians) *
           Mat4::scale(transform.scale);
}

// Rest bind matrices walk parent chains; depth is bounded by `skeleton.joints.size()` and invalid
// parent links are rejected by `is_valid_animation_skeleton_desc`.
// NOLINTBEGIN(misc-no-recursion)
[[nodiscard]] Mat4 rest_model_matrix_noalloc(const AnimationSkeletonDesc& skeleton, std::size_t joint_index) noexcept {
    const auto local = local_transform_matrix(skeleton.joints[joint_index].rest);
    const auto parent_index = skeleton.joints[joint_index].parent_index;
    if (parent_index == animation_no_parent) {
        return local;
    }
    return rest_model_matrix_noalloc(skeleton, parent_index) * local;
}
// NOLINTEND(misc-no-recursion)

[[nodiscard]] std::vector<Mat4> build_model_matrices_noexcept(const AnimationSkeletonDesc& skeleton,
                                                              const AnimationPose& pose) {
    std::vector<Mat4> matrices;
    matrices.reserve(pose.joints.size());
    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        auto matrix = local_transform_matrix(pose.joints[index]);
        const auto parent_index = skeleton.joints[index].parent_index;
        if (parent_index != animation_no_parent) {
            matrix = matrices[parent_index] * matrix;
        }
        matrices.push_back(matrix);
    }
    return matrices;
}

[[nodiscard]] bool is_valid_skin_noalloc(const AnimationSkeletonDesc& skeleton,
                                         const AnimationSkinPayloadDesc& skin) noexcept {
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        return false;
    }
    if (skin.vertex_count == 0U || skin.joints.empty() || skin.vertices.empty()) {
        return false;
    }
    if (skin.vertices.size() != static_cast<std::size_t>(skin.vertex_count)) {
        return false;
    }

    for (std::size_t index = 0; index < skin.joints.size(); ++index) {
        const auto& joint = skin.joints[index];
        if (joint.skeleton_joint_index >= skeleton.joints.size() || has_duplicate_skin_joint(skin, index) ||
            !finite_matrix(joint.inverse_bind_matrix)) {
            return false;
        }
    }

    for (const auto& joint : skin.joints) {
        if (!nearly_identity(rest_model_matrix_noalloc(skeleton, joint.skeleton_joint_index) *
                             joint.inverse_bind_matrix)) {
            return false;
        }
    }

    for (const auto& vertex : skin.vertices) {
        if (vertex.influences.empty() || vertex.influences.size() > animation_skin_max_influences) {
            return false;
        }

        float weight_sum = 0.0F;
        for (const auto& influence : vertex.influences) {
            if (influence.skin_joint_index >= skin.joints.size() || !finite(influence.weight) ||
                influence.weight < 0.0F) {
                return false;
            }
            weight_sum += influence.weight;
        }
        if (!finite(weight_sum) || weight_sum <= 0.0F) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool has_valid_cpu_skinning_shape(const AnimationCpuSkinningDesc& desc) noexcept {
    if (desc.bind_positions.empty()) {
        return false;
    }
    if (desc.skin.vertex_count != desc.bind_positions.size() ||
        desc.skin.vertices.size() != desc.bind_positions.size()) {
        return false;
    }
    if (!desc.bind_normals.empty() && desc.bind_normals.size() != desc.bind_positions.size()) {
        return false;
    }
    if (!desc.bind_tangents.empty() && desc.bind_tangents.size() != desc.bind_positions.size()) {
        return false;
    }
    if (!desc.bind_tangent_handedness.empty() && desc.bind_tangent_handedness.size() != desc.bind_positions.size()) {
        return false;
    }
    if (!desc.bind_tangent_handedness.empty() && (desc.bind_normals.empty() || desc.bind_tangents.empty())) {
        return false;
    }
    return desc.palette.matrices.size() == desc.skin.joints.size();
}

[[nodiscard]] bool is_valid_cpu_skinning_noalloc(const AnimationCpuSkinningDesc& desc) noexcept {
    if (!has_valid_cpu_skinning_shape(desc)) {
        return false;
    }
    if (!std::ranges::all_of(desc.bind_positions, finite_vec) || !std::ranges::all_of(desc.bind_normals, finite_vec) ||
        !std::ranges::all_of(desc.bind_tangents, finite_vec) ||
        !std::ranges::all_of(desc.bind_tangent_handedness, valid_handedness_sign) ||
        !std::ranges::all_of(desc.palette.matrices, finite_matrix)) {
        return false;
    }

    for (const auto& vertex : desc.skin.vertices) {
        if (vertex.influences.empty() || vertex.influences.size() > animation_skin_max_influences) {
            return false;
        }

        float weight_sum = 0.0F;
        for (const auto& influence : vertex.influences) {
            if (influence.skin_joint_index >= desc.palette.matrices.size() || !finite(influence.weight) ||
                influence.weight < 0.0F) {
                return false;
            }
            weight_sum += influence.weight;
        }
        if (!finite(weight_sum) || weight_sum <= 0.0F) {
            return false;
        }
    }

    return true;
}

} // namespace

std::vector<AnimationSkinDiagnostic> validate_animation_skin_payload(const AnimationSkeletonDesc& skeleton,
                                                                     const AnimationSkinPayloadDesc& skin) {
    std::vector<AnimationSkinDiagnostic> diagnostics;
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        push_diagnostic(diagnostics, 0, 0, 0, AnimationSkinDiagnosticCode::invalid_skeleton,
                        "animation skin skeleton is invalid");
        return diagnostics;
    }
    if (skin.vertex_count == 0U || skin.joints.empty() || skin.vertices.empty()) {
        push_diagnostic(diagnostics, 0, 0, 0, AnimationSkinDiagnosticCode::empty_skin,
                        "animation skin payload must contain joints and vertices");
        return diagnostics;
    }

    for (std::size_t index = 0; index < skin.joints.size(); ++index) {
        const auto& joint = skin.joints[index];
        if (joint.skeleton_joint_index >= skeleton.joints.size()) {
            push_diagnostic(diagnostics, index, 0, 0, AnimationSkinDiagnosticCode::invalid_skin_joint,
                            "animation skin joint targets an out-of-range skeleton joint");
        } else if (has_duplicate_skin_joint(skin, index)) {
            push_diagnostic(diagnostics, index, 0, 0, AnimationSkinDiagnosticCode::duplicate_skin_joint,
                            "animation skin joint targets a skeleton joint more than once");
        }
        if (!finite_matrix(joint.inverse_bind_matrix)) {
            push_diagnostic(diagnostics, index, 0, 0, AnimationSkinDiagnosticCode::invalid_inverse_bind_matrix,
                            "animation skin inverse bind matrix is invalid");
        } else if (joint.skeleton_joint_index < skeleton.joints.size() &&
                   !nearly_identity(rest_model_matrix_noalloc(skeleton, joint.skeleton_joint_index) *
                                    joint.inverse_bind_matrix)) {
            push_diagnostic(diagnostics, index, 0, 0, AnimationSkinDiagnosticCode::bind_pose_mismatch,
                            "animation skin inverse bind matrix does not match skeleton rest bind pose");
        }
    }

    if (skin.vertices.size() != static_cast<std::size_t>(skin.vertex_count)) {
        push_diagnostic(diagnostics, 0, skin.vertices.size(), 0, AnimationSkinDiagnosticCode::vertex_count_mismatch,
                        "animation skin vertex count does not match vertex influence rows");
    }

    for (std::size_t vertex_index = 0; vertex_index < skin.vertices.size(); ++vertex_index) {
        const auto& vertex = skin.vertices[vertex_index];
        if (vertex.influences.empty() || vertex.influences.size() > animation_skin_max_influences) {
            push_diagnostic(diagnostics, 0, vertex_index, 0,
                            AnimationSkinDiagnosticCode::invalid_vertex_influence_count,
                            "animation skin vertex influence count is invalid");
            continue;
        }

        float weight_sum = 0.0F;
        bool invalid_weight = false;
        for (std::size_t influence_index = 0; influence_index < vertex.influences.size(); ++influence_index) {
            const auto& influence = vertex.influences[influence_index];
            if (influence.skin_joint_index >= skin.joints.size()) {
                push_diagnostic(diagnostics, influence.skin_joint_index, vertex_index, influence_index,
                                AnimationSkinDiagnosticCode::invalid_vertex_joint_index,
                                "animation skin vertex references an out-of-range skin joint");
            }
            if (!finite(influence.weight) || influence.weight < 0.0F) {
                invalid_weight = true;
                push_diagnostic(diagnostics, influence.skin_joint_index, vertex_index, influence_index,
                                AnimationSkinDiagnosticCode::invalid_vertex_weight,
                                "animation skin vertex weight is invalid");
            } else {
                weight_sum += influence.weight;
            }
        }
        if (!invalid_weight && !finite(weight_sum)) {
            push_diagnostic(diagnostics, 0, vertex_index, 0, AnimationSkinDiagnosticCode::invalid_vertex_weight,
                            "animation skin vertex weight sum is invalid");
        } else if (!invalid_weight && weight_sum <= 0.0F) {
            push_diagnostic(diagnostics, 0, vertex_index, 0, AnimationSkinDiagnosticCode::zero_vertex_weight,
                            "animation skin vertex weight sum is zero");
        }
    }

    return diagnostics;
}

bool is_valid_animation_skin_payload(const AnimationSkeletonDesc& skeleton,
                                     const AnimationSkinPayloadDesc& skin) noexcept {
    return is_valid_skin_noalloc(skeleton, skin);
}

AnimationSkinPayloadDesc normalize_animation_skin_payload(const AnimationSkeletonDesc& skeleton,
                                                          const AnimationSkinPayloadDesc& skin) {
    if (!is_valid_animation_skin_payload(skeleton, skin)) {
        throw std::invalid_argument("animation skin payload is invalid");
    }

    auto normalized = skin;
    for (auto& vertex : normalized.vertices) {
        float weight_sum = 0.0F;
        for (const auto& influence : vertex.influences) {
            weight_sum += influence.weight;
        }
        for (auto& influence : vertex.influences) {
            influence.weight /= weight_sum;
        }
    }
    return normalized;
}

AnimationModelPose build_animation_model_pose(const AnimationSkeletonDesc& skeleton, const AnimationPose& pose) {
    if (!validate_animation_pose(skeleton, pose).empty()) {
        throw std::invalid_argument("animation pose is invalid");
    }

    AnimationModelPose model_pose;
    model_pose.joint_matrices = build_model_matrices_noexcept(skeleton, pose);
    return model_pose;
}

AnimationSkinningPalette build_animation_skinning_palette(const AnimationSkeletonDesc& skeleton,
                                                          const AnimationSkinPayloadDesc& skin,
                                                          const AnimationPose& pose) {
    if (!is_valid_animation_skin_payload(skeleton, skin)) {
        throw std::invalid_argument("animation skin payload is invalid");
    }

    const auto model_pose = build_animation_model_pose(skeleton, pose);
    AnimationSkinningPalette palette;
    palette.matrices.reserve(skin.joints.size());
    for (const auto& joint : skin.joints) {
        palette.matrices.push_back(model_pose.joint_matrices[joint.skeleton_joint_index] * joint.inverse_bind_matrix);
    }
    return palette;
}

std::vector<AnimationCpuSkinningDiagnostic> validate_animation_cpu_skinning_desc(const AnimationCpuSkinningDesc& desc) {
    std::vector<AnimationCpuSkinningDiagnostic> diagnostics;
    if (desc.bind_positions.empty()) {
        push_cpu_diagnostic(diagnostics, 0, 0, 0, AnimationCpuSkinningDiagnosticCode::empty_bind_positions,
                            "animation CPU skinning bind position stream must not be empty");
        return diagnostics;
    }
    if (desc.skin.vertex_count != desc.bind_positions.size() ||
        desc.skin.vertices.size() != desc.bind_positions.size()) {
        push_cpu_diagnostic(diagnostics, desc.bind_positions.size(), 0, 0,
                            AnimationCpuSkinningDiagnosticCode::vertex_count_mismatch,
                            "animation CPU skinning vertex count does not match skin influence rows");
        return diagnostics;
    }
    if (!desc.bind_normals.empty() && desc.bind_normals.size() != desc.bind_positions.size()) {
        push_cpu_diagnostic(diagnostics, desc.bind_normals.size(), 0, 0,
                            AnimationCpuSkinningDiagnosticCode::normal_count_mismatch,
                            "animation CPU skinning normal count does not match bind positions");
        return diagnostics;
    }
    if (!desc.bind_tangents.empty() && desc.bind_tangents.size() != desc.bind_positions.size()) {
        push_cpu_diagnostic(diagnostics, desc.bind_tangents.size(), 0, 0,
                            AnimationCpuSkinningDiagnosticCode::tangent_count_mismatch,
                            "animation CPU skinning tangent count does not match bind positions");
        return diagnostics;
    }
    if (!desc.bind_tangent_handedness.empty() && desc.bind_tangent_handedness.size() != desc.bind_positions.size()) {
        push_cpu_diagnostic(diagnostics, desc.bind_tangent_handedness.size(), 0, 0,
                            AnimationCpuSkinningDiagnosticCode::tangent_handedness_count_mismatch,
                            "animation CPU skinning tangent handedness count does not match bind positions");
        return diagnostics;
    }
    if (!desc.bind_tangent_handedness.empty() && (desc.bind_normals.empty() || desc.bind_tangents.empty())) {
        push_cpu_diagnostic(diagnostics, 0, 0, 0,
                            AnimationCpuSkinningDiagnosticCode::tangent_handedness_missing_streams,
                            "animation CPU skinning tangent handedness requires normal and tangent streams");
        return diagnostics;
    }
    if (desc.palette.matrices.size() != desc.skin.joints.size()) {
        push_cpu_diagnostic(diagnostics, 0, 0, desc.palette.matrices.size(),
                            AnimationCpuSkinningDiagnosticCode::palette_count_mismatch,
                            "animation CPU skinning palette count does not match skin joints");
        return diagnostics;
    }

    for (std::size_t vertex_index = 0; vertex_index < desc.bind_positions.size(); ++vertex_index) {
        if (!finite_vec(desc.bind_positions[vertex_index])) {
            push_cpu_diagnostic(diagnostics, vertex_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_bind_position,
                                "animation CPU skinning bind position is invalid");
        }
    }

    for (std::size_t normal_index = 0; normal_index < desc.bind_normals.size(); ++normal_index) {
        if (!finite_vec(desc.bind_normals[normal_index])) {
            push_cpu_diagnostic(diagnostics, normal_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_bind_normal,
                                "animation CPU skinning bind normal is invalid");
        }
    }

    for (std::size_t tangent_index = 0; tangent_index < desc.bind_tangents.size(); ++tangent_index) {
        if (!finite_vec(desc.bind_tangents[tangent_index])) {
            push_cpu_diagnostic(diagnostics, tangent_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_bind_tangent,
                                "animation CPU skinning bind tangent is invalid");
        }
    }

    for (std::size_t handedness_index = 0; handedness_index < desc.bind_tangent_handedness.size(); ++handedness_index) {
        if (!valid_handedness_sign(desc.bind_tangent_handedness[handedness_index])) {
            push_cpu_diagnostic(diagnostics, handedness_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_tangent_handedness,
                                "animation CPU skinning tangent handedness must be finite -1 or +1");
        }
    }

    for (std::size_t matrix_index = 0; matrix_index < desc.palette.matrices.size(); ++matrix_index) {
        if (!finite_matrix(desc.palette.matrices[matrix_index])) {
            push_cpu_diagnostic(diagnostics, 0, 0, matrix_index,
                                AnimationCpuSkinningDiagnosticCode::invalid_palette_matrix,
                                "animation CPU skinning palette matrix is invalid");
        }
    }

    for (std::size_t vertex_index = 0; vertex_index < desc.skin.vertices.size(); ++vertex_index) {
        const auto& vertex = desc.skin.vertices[vertex_index];
        if (vertex.influences.empty() || vertex.influences.size() > animation_skin_max_influences) {
            push_cpu_diagnostic(diagnostics, vertex_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_vertex_influence_count,
                                "animation CPU skinning vertex influence count is invalid");
            continue;
        }

        float weight_sum = 0.0F;
        bool invalid_weight = false;
        for (std::size_t influence_index = 0; influence_index < vertex.influences.size(); ++influence_index) {
            const auto& influence = vertex.influences[influence_index];
            if (influence.skin_joint_index >= desc.palette.matrices.size()) {
                push_cpu_diagnostic(diagnostics, vertex_index, influence_index, influence.skin_joint_index,
                                    AnimationCpuSkinningDiagnosticCode::invalid_vertex_joint_index,
                                    "animation CPU skinning vertex references an out-of-range palette joint");
            }
            if (!finite(influence.weight) || influence.weight < 0.0F) {
                invalid_weight = true;
                push_cpu_diagnostic(diagnostics, vertex_index, influence_index, influence.skin_joint_index,
                                    AnimationCpuSkinningDiagnosticCode::invalid_vertex_weight,
                                    "animation CPU skinning vertex weight is invalid");
            } else {
                weight_sum += influence.weight;
            }
        }
        if (!invalid_weight && !finite(weight_sum)) {
            push_cpu_diagnostic(diagnostics, vertex_index, 0, 0,
                                AnimationCpuSkinningDiagnosticCode::invalid_vertex_weight,
                                "animation CPU skinning vertex weight sum is invalid");
        } else if (!invalid_weight && weight_sum <= 0.0F) {
            push_cpu_diagnostic(diagnostics, vertex_index, 0, 0, AnimationCpuSkinningDiagnosticCode::zero_vertex_weight,
                                "animation CPU skinning vertex weight sum is zero");
        }
    }

    return diagnostics;
}

bool is_valid_animation_cpu_skinning_desc(const AnimationCpuSkinningDesc& desc) noexcept {
    return is_valid_cpu_skinning_noalloc(desc);
}

AnimationCpuSkinnedVertexStream skin_animation_vertices_cpu(const AnimationCpuSkinningDesc& desc) {
    if (!is_valid_animation_cpu_skinning_desc(desc)) {
        throw std::invalid_argument("animation CPU skinning description is invalid");
    }

    AnimationCpuSkinnedVertexStream stream;
    stream.positions.reserve(desc.bind_positions.size());
    const auto skin_normals = !desc.bind_normals.empty();
    const auto skin_tangents = !desc.bind_tangents.empty();
    const auto skin_bitangents = !desc.bind_tangent_handedness.empty();
    if (skin_normals) {
        stream.normals.reserve(desc.bind_normals.size());
    }
    if (skin_tangents) {
        stream.tangents.reserve(desc.bind_tangents.size());
    }
    if (skin_bitangents) {
        stream.bitangents.reserve(desc.bind_tangent_handedness.size());
    }
    for (std::size_t vertex_index = 0; vertex_index < desc.bind_positions.size(); ++vertex_index) {
        const auto& vertex = desc.skin.vertices[vertex_index];
        float weight_sum = 0.0F;
        for (const auto& influence : vertex.influences) {
            weight_sum += influence.weight;
        }

        Vec3 skinned{};
        Vec3 skinned_normal{};
        Vec3 skinned_tangent{};
        for (const auto& influence : vertex.influences) {
            const auto normalized_weight = influence.weight / weight_sum;
            const auto& palette_matrix = desc.palette.matrices[influence.skin_joint_index];
            const auto transformed = transform_point(palette_matrix, desc.bind_positions[vertex_index]);
            skinned = skinned + (transformed * normalized_weight);
            if (skin_normals) {
                const auto transformed_normal = transform_direction(palette_matrix, desc.bind_normals[vertex_index]);
                skinned_normal = skinned_normal + (transformed_normal * normalized_weight);
            }
            if (skin_tangents) {
                const auto transformed_tangent = transform_direction(palette_matrix, desc.bind_tangents[vertex_index]);
                skinned_tangent = skinned_tangent + (transformed_tangent * normalized_weight);
            }
        }
        stream.positions.push_back(skinned);
        Vec3 normalized_normal{};
        Vec3 normalized_tangent{};
        if (skin_normals) {
            normalized_normal = normalize_or_zero(skinned_normal);
            stream.normals.push_back(normalized_normal);
        }
        if (skin_tangents) {
            normalized_tangent = normalize_or_zero(skinned_tangent);
            stream.tangents.push_back(normalized_tangent);
        }
        if (skin_bitangents) {
            const auto sign = desc.bind_tangent_handedness[vertex_index] < 0.0F ? -1.0F : 1.0F;
            stream.bitangents.push_back(normalize_or_zero(cross(normalized_normal, normalized_tangent)) * sign);
        }
    }
    return stream;
}

} // namespace mirakana
