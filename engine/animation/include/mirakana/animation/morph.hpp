// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

/// One glTF morph target: additive POSITION, NORMAL, and/or TANGENT (xyz) deltas per vertex.
/// Each non-empty stream must have length `vertex_count` matching `AnimationMorphMeshCpuDesc::bind_positions`.
/// At least one stream must be non-empty per target.
struct AnimationMorphTargetCpuDesc {
    std::vector<Vec3> position_deltas;
    std::vector<Vec3> normal_deltas;
    std::vector<Vec3> tangent_deltas;
};

/// CPU-only morph evaluation aligned with glTF 2.0 linear blend morphing:
/// - positions: `p' = p + sum_i w_i * delta_p_i` (missing per-target position stream counts as zero delta)
/// - normals: `n_raw = n + sum_i w_i * delta_n_i` then unit-length normalize (missing per-target normal stream counts
/// as zero delta)
/// - tangents (xyz): `t_raw = t + sum_i w_i * delta_t_i` then unit-length normalize (missing per-target tangent stream
/// counts as zero delta)
///
/// `bind_normals` must be present iff any target carries non-empty `normal_deltas`.
/// `bind_tangents` must be present iff any target carries non-empty `tangent_deltas`.
/// `target_weights.size()` must equal `targets.size()`. Each weight must be finite and in `[0, 1]`.
struct AnimationMorphMeshCpuDesc {
    std::vector<Vec3> bind_positions;
    std::vector<Vec3> bind_normals;
    std::vector<Vec3> bind_tangents;
    std::vector<AnimationMorphTargetCpuDesc> targets;
    std::vector<float> target_weights;
};

/// glTF `animations[].channels` with `path` `weights`: one row of `morph_target_count` weights per `time_seconds`.
/// Rows must be strictly increasing in time (same contract as sorted scalar keyframes used elsewhere).
struct AnimationMorphWeightsKeyframeDesc {
    float time_seconds{0.0F};
    std::vector<float> weights;
};

struct AnimationMorphWeightsTrackDesc {
    std::size_t morph_target_count{0};
    std::vector<AnimationMorphWeightsKeyframeDesc> keyframes;
};

enum class AnimationMorphWeightsTrackDiagnosticCode : std::uint8_t {
    morph_target_count_zero,
    empty_keyframes,
    invalid_time,
    keyframe_time_order,
    keyframe_weight_count_mismatch,
    invalid_keyframe_weight,
};

struct AnimationMorphWeightsTrackDiagnostic {
    std::size_t keyframe_index{0};
    std::size_t weight_index{0};
    AnimationMorphWeightsTrackDiagnosticCode code{AnimationMorphWeightsTrackDiagnosticCode::empty_keyframes};
    std::string message;
};

enum class AnimationMorphMeshDiagnosticCode : std::uint8_t {
    empty_bind_positions,
    target_weight_count_mismatch,
    morph_vertex_count_mismatch,
    morph_target_empty_streams,
    invalid_bind_position,
    invalid_position_delta,
    invalid_bind_normal,
    invalid_normal_delta,
    missing_bind_normals_for_normal_morph,
    bind_normals_without_normal_morph,
    invalid_bind_tangent,
    invalid_tangent_delta,
    missing_bind_tangents_for_tangent_morph,
    bind_tangents_without_tangent_morph,
    invalid_target_weight,
};

struct AnimationMorphMeshDiagnostic {
    std::size_t morph_target_index{0};
    std::size_t vertex_index{0};
    AnimationMorphMeshDiagnosticCode code{AnimationMorphMeshDiagnosticCode::empty_bind_positions};
    std::string message;
};

[[nodiscard]] std::vector<AnimationMorphMeshDiagnostic>
validate_animation_morph_mesh_cpu_desc(const AnimationMorphMeshCpuDesc& desc);
[[nodiscard]] bool is_valid_animation_morph_mesh_cpu_desc(const AnimationMorphMeshCpuDesc& desc) noexcept;
[[nodiscard]] std::vector<Vec3> apply_animation_morph_targets_positions_cpu(const AnimationMorphMeshCpuDesc& desc);
[[nodiscard]] std::vector<Vec3> apply_animation_morph_targets_normals_cpu(const AnimationMorphMeshCpuDesc& desc);
[[nodiscard]] std::vector<Vec3> apply_animation_morph_targets_tangents_cpu(const AnimationMorphMeshCpuDesc& desc);

[[nodiscard]] std::vector<AnimationMorphWeightsTrackDiagnostic>
validate_animation_morph_weights_track_desc(const AnimationMorphWeightsTrackDesc& desc);
[[nodiscard]] bool is_valid_animation_morph_weights_track_desc(const AnimationMorphWeightsTrackDesc& desc) noexcept;
[[nodiscard]] std::vector<float> sample_animation_morph_weights_at_time(const AnimationMorphWeightsTrackDesc& desc,
                                                                        float time_seconds);

} // namespace mirakana
