// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/morph.hpp"

#include "mirakana/math/vec.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

void push_diagnostic(std::vector<AnimationMorphMeshDiagnostic>& diagnostics, std::size_t morph_target_index,
                     std::size_t vertex_index, AnimationMorphMeshDiagnosticCode code, std::string message) {
    diagnostics.push_back(AnimationMorphMeshDiagnostic{.morph_target_index = morph_target_index,
                                                       .vertex_index = vertex_index,
                                                       .code = code,
                                                       .message = std::move(message)});
}

[[nodiscard]] bool weight_in_unit_interval(float weight) noexcept {
    return finite(weight) && weight >= 0.0F && weight <= 1.0F;
}

[[nodiscard]] Vec3 normalize_or_zero(Vec3 value) noexcept {
    const auto magnitude = length(value);
    if (!finite(magnitude) || magnitude <= 0.000001F) {
        return Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }
    return value * (1.0F / magnitude);
}

} // namespace

std::vector<AnimationMorphMeshDiagnostic>
validate_animation_morph_mesh_cpu_desc(const AnimationMorphMeshCpuDesc& desc) {
    std::vector<AnimationMorphMeshDiagnostic> diagnostics;
    const auto vertex_count = desc.bind_positions.size();
    if (vertex_count == 0) {
        push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::empty_bind_positions,
                        "bind_positions must be non-empty");
        return diagnostics;
    }

    if (desc.targets.size() != desc.target_weights.size()) {
        push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::target_weight_count_mismatch,
                        "targets and target_weights must have the same size");
        return diagnostics;
    }

    for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
        if (!finite_vec(desc.bind_positions[vertex_index])) {
            push_diagnostic(diagnostics, 0, vertex_index, AnimationMorphMeshDiagnosticCode::invalid_bind_position,
                            "bind position must be finite");
            return diagnostics;
        }
    }

    bool any_normal_morph = false;
    bool any_tangent_morph = false;
    for (std::size_t target_index = 0; target_index < desc.targets.size(); ++target_index) {
        const auto& target = desc.targets[target_index];
        const auto has_pos = !target.position_deltas.empty();
        const auto has_nrm = !target.normal_deltas.empty();
        const auto has_tan = !target.tangent_deltas.empty();
        if (!has_pos && !has_nrm && !has_tan) {
            push_diagnostic(
                diagnostics, target_index, 0, AnimationMorphMeshDiagnosticCode::morph_target_empty_streams,
                "morph target must include a non-empty position_deltas, normal_deltas, and/or tangent_deltas stream");
            return diagnostics;
        }
        if (has_pos && target.position_deltas.size() != vertex_count) {
            push_diagnostic(diagnostics, target_index, 0, AnimationMorphMeshDiagnosticCode::morph_vertex_count_mismatch,
                            "morph target position_deltas size must match bind_positions size when present");
            return diagnostics;
        }
        if (has_nrm && target.normal_deltas.size() != vertex_count) {
            push_diagnostic(diagnostics, target_index, 0, AnimationMorphMeshDiagnosticCode::morph_vertex_count_mismatch,
                            "morph target normal_deltas size must match bind_positions size when present");
            return diagnostics;
        }
        if (has_tan && target.tangent_deltas.size() != vertex_count) {
            push_diagnostic(diagnostics, target_index, 0, AnimationMorphMeshDiagnosticCode::morph_vertex_count_mismatch,
                            "morph target tangent_deltas size must match bind_positions size when present");
            return diagnostics;
        }
        if (has_pos) {
            for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                if (!finite_vec(target.position_deltas[vertex_index])) {
                    push_diagnostic(diagnostics, target_index, vertex_index,
                                    AnimationMorphMeshDiagnosticCode::invalid_position_delta,
                                    "morph position delta must be finite");
                    return diagnostics;
                }
            }
        }
        if (has_nrm) {
            any_normal_morph = true;
            for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                if (!finite_vec(target.normal_deltas[vertex_index])) {
                    push_diagnostic(diagnostics, target_index, vertex_index,
                                    AnimationMorphMeshDiagnosticCode::invalid_normal_delta,
                                    "morph normal delta must be finite");
                    return diagnostics;
                }
            }
        }
        if (has_tan) {
            any_tangent_morph = true;
            for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                if (!finite_vec(target.tangent_deltas[vertex_index])) {
                    push_diagnostic(diagnostics, target_index, vertex_index,
                                    AnimationMorphMeshDiagnosticCode::invalid_tangent_delta,
                                    "morph tangent delta must be finite");
                    return diagnostics;
                }
            }
        }
    }

    if (any_normal_morph) {
        if (desc.bind_normals.size() != vertex_count) {
            push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::missing_bind_normals_for_normal_morph,
                            "bind_normals must match bind_positions size when any morph target carries normal_deltas");
            return diagnostics;
        }
        for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            if (!finite_vec(desc.bind_normals[vertex_index])) {
                push_diagnostic(diagnostics, 0, vertex_index, AnimationMorphMeshDiagnosticCode::invalid_bind_normal,
                                "bind normal must be finite");
                return diagnostics;
            }
        }
    } else if (!desc.bind_normals.empty()) {
        if (desc.bind_normals.size() != vertex_count) {
            push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::invalid_bind_normal,
                            "bind_normals must be empty or match bind_positions size");
            return diagnostics;
        }
        push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::bind_normals_without_normal_morph,
                        "bind_normals must be empty when no morph target carries normal_deltas");
        return diagnostics;
    }

    if (any_tangent_morph) {
        if (desc.bind_tangents.size() != vertex_count) {
            push_diagnostic(
                diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::missing_bind_tangents_for_tangent_morph,
                "bind_tangents must match bind_positions size when any morph target carries tangent_deltas");
            return diagnostics;
        }
        for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            if (!finite_vec(desc.bind_tangents[vertex_index])) {
                push_diagnostic(diagnostics, 0, vertex_index, AnimationMorphMeshDiagnosticCode::invalid_bind_tangent,
                                "bind tangent must be finite");
                return diagnostics;
            }
        }
    } else if (!desc.bind_tangents.empty()) {
        if (desc.bind_tangents.size() != vertex_count) {
            push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::invalid_bind_tangent,
                            "bind_tangents must be empty or match bind_positions size");
            return diagnostics;
        }
        push_diagnostic(diagnostics, 0, 0, AnimationMorphMeshDiagnosticCode::bind_tangents_without_tangent_morph,
                        "bind_tangents must be empty when no morph target carries tangent_deltas");
        return diagnostics;
    }

    for (std::size_t target_index = 0; target_index < desc.target_weights.size(); ++target_index) {
        const auto weight = desc.target_weights[target_index];
        if (!weight_in_unit_interval(weight)) {
            push_diagnostic(diagnostics, target_index, 0, AnimationMorphMeshDiagnosticCode::invalid_target_weight,
                            "target weight must be finite and in [0,1]");
            return diagnostics;
        }
    }

    return diagnostics;
}

bool is_valid_animation_morph_mesh_cpu_desc(const AnimationMorphMeshCpuDesc& desc) noexcept {
    return validate_animation_morph_mesh_cpu_desc(desc).empty();
}

std::vector<Vec3> apply_animation_morph_targets_positions_cpu(const AnimationMorphMeshCpuDesc& desc) {
    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("animation morph mesh CPU description is invalid");
    }

    const auto vertex_count = desc.bind_positions.size();
    std::vector<Vec3> out;
    out.reserve(vertex_count);
    for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
        auto position = desc.bind_positions[vertex_index];
        for (std::size_t target_index = 0; target_index < desc.targets.size(); ++target_index) {
            const auto& deltas = desc.targets[target_index].position_deltas;
            if (deltas.empty()) {
                continue;
            }
            const auto weight = desc.target_weights[target_index];
            position = position + deltas[vertex_index] * weight;
        }
        out.push_back(position);
    }
    return out;
}

std::vector<Vec3> apply_animation_morph_targets_normals_cpu(const AnimationMorphMeshCpuDesc& desc) {
    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("animation morph mesh CPU description is invalid");
    }

    const auto vertex_count = desc.bind_positions.size();
    bool any_normal_morph = false;
    for (const auto& target : desc.targets) {
        if (!target.normal_deltas.empty()) {
            any_normal_morph = true;
            break;
        }
    }
    if (!any_normal_morph) {
        throw std::invalid_argument("animation morph mesh CPU description has no normal morph deltas");
    }

    std::vector<Vec3> out;
    out.reserve(vertex_count);
    for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
        auto normal = desc.bind_normals[vertex_index];
        for (std::size_t target_index = 0; target_index < desc.targets.size(); ++target_index) {
            const auto& deltas = desc.targets[target_index].normal_deltas;
            if (deltas.empty()) {
                continue;
            }
            const auto weight = desc.target_weights[target_index];
            normal = normal + deltas[vertex_index] * weight;
        }
        auto morphed = normalize_or_zero(normal);
        if (morphed.x == 0.0F && morphed.y == 0.0F && morphed.z == 0.0F) {
            morphed = normalize_or_zero(desc.bind_normals[vertex_index]);
        }
        out.push_back(morphed);
    }
    return out;
}

std::vector<Vec3> apply_animation_morph_targets_tangents_cpu(const AnimationMorphMeshCpuDesc& desc) {
    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("animation morph mesh CPU description is invalid");
    }

    const auto vertex_count = desc.bind_positions.size();
    bool any_tangent_morph = false;
    for (const auto& target : desc.targets) {
        if (!target.tangent_deltas.empty()) {
            any_tangent_morph = true;
            break;
        }
    }
    if (!any_tangent_morph) {
        throw std::invalid_argument("animation morph mesh CPU description has no tangent morph deltas");
    }

    std::vector<Vec3> out;
    out.reserve(vertex_count);
    for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
        auto tangent = desc.bind_tangents[vertex_index];
        for (std::size_t target_index = 0; target_index < desc.targets.size(); ++target_index) {
            const auto& deltas = desc.targets[target_index].tangent_deltas;
            if (deltas.empty()) {
                continue;
            }
            const auto weight = desc.target_weights[target_index];
            tangent = tangent + deltas[vertex_index] * weight;
        }
        auto morphed = normalize_or_zero(tangent);
        if (morphed.x == 0.0F && morphed.y == 0.0F && morphed.z == 0.0F) {
            morphed = normalize_or_zero(desc.bind_tangents[vertex_index]);
        }
        out.push_back(morphed);
    }
    return out;
}

void push_weights_diagnostic(std::vector<AnimationMorphWeightsTrackDiagnostic>& diagnostics, std::size_t keyframe_index,
                             std::size_t weight_index, AnimationMorphWeightsTrackDiagnosticCode code,
                             std::string message) {
    diagnostics.push_back(AnimationMorphWeightsTrackDiagnostic{
        .keyframe_index = keyframe_index, .weight_index = weight_index, .code = code, .message = std::move(message)});
}

std::vector<AnimationMorphWeightsTrackDiagnostic>
validate_animation_morph_weights_track_desc(const AnimationMorphWeightsTrackDesc& desc) {
    std::vector<AnimationMorphWeightsTrackDiagnostic> diagnostics;
    if (desc.morph_target_count == 0) {
        push_weights_diagnostic(diagnostics, 0, 0, AnimationMorphWeightsTrackDiagnosticCode::morph_target_count_zero,
                                "morph_target_count must be non-zero");
        return diagnostics;
    }
    if (desc.keyframes.empty()) {
        push_weights_diagnostic(diagnostics, 0, 0, AnimationMorphWeightsTrackDiagnosticCode::empty_keyframes,
                                "weights track keyframes must be non-empty");
        return diagnostics;
    }

    float previous_time = -1.0F;
    for (std::size_t keyframe_index = 0; keyframe_index < desc.keyframes.size(); ++keyframe_index) {
        const auto& keyframe = desc.keyframes[keyframe_index];
        if (!finite(keyframe.time_seconds) || keyframe.time_seconds < 0.0F) {
            push_weights_diagnostic(diagnostics, keyframe_index, 0,
                                    AnimationMorphWeightsTrackDiagnosticCode::invalid_time,
                                    "keyframe time must be finite and non-negative");
            return diagnostics;
        }
        if (!(keyframe.time_seconds > previous_time)) {
            push_weights_diagnostic(diagnostics, keyframe_index, 0,
                                    AnimationMorphWeightsTrackDiagnosticCode::keyframe_time_order,
                                    "keyframe times must be strictly increasing");
            return diagnostics;
        }
        previous_time = keyframe.time_seconds;

        if (keyframe.weights.size() != desc.morph_target_count) {
            push_weights_diagnostic(diagnostics, keyframe_index, 0,
                                    AnimationMorphWeightsTrackDiagnosticCode::keyframe_weight_count_mismatch,
                                    "each keyframe weights length must match morph_target_count");
            return diagnostics;
        }
        for (std::size_t weight_index = 0; weight_index < keyframe.weights.size(); ++weight_index) {
            if (!weight_in_unit_interval(keyframe.weights[weight_index])) {
                push_weights_diagnostic(diagnostics, keyframe_index, weight_index,
                                        AnimationMorphWeightsTrackDiagnosticCode::invalid_keyframe_weight,
                                        "morph weight keyframe samples must be finite and in [0,1]");
                return diagnostics;
            }
        }
    }

    return diagnostics;
}

bool is_valid_animation_morph_weights_track_desc(const AnimationMorphWeightsTrackDesc& desc) noexcept {
    return validate_animation_morph_weights_track_desc(desc).empty();
}

std::vector<float> sample_animation_morph_weights_at_time(const AnimationMorphWeightsTrackDesc& desc,
                                                          float time_seconds) {
    if (!is_valid_animation_morph_weights_track_desc(desc) || !finite(time_seconds) || time_seconds < 0.0F) {
        throw std::invalid_argument("animation morph weights track description or time is invalid");
    }
    const auto n = desc.morph_target_count;
    const auto& keyframes = desc.keyframes;
    if (time_seconds <= keyframes.front().time_seconds) {
        return keyframes.front().weights;
    }
    if (time_seconds >= keyframes.back().time_seconds) {
        return keyframes.back().weights;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        if (time_seconds <= keyframes[index].time_seconds) {
            const auto& previous = keyframes[index - 1U];
            const auto& next = keyframes[index];
            const auto duration = next.time_seconds - previous.time_seconds;
            const float t = (time_seconds - previous.time_seconds) / duration;
            std::vector<float> out;
            out.reserve(n);
            for (std::size_t weight_index = 0; weight_index < n; ++weight_index) {
                const float lhs = previous.weights[weight_index];
                const float rhs = next.weights[weight_index];
                out.push_back(lhs + ((rhs - lhs) * t));
            }
            return out;
        }
    }
    return keyframes.back().weights;
}

} // namespace mirakana
