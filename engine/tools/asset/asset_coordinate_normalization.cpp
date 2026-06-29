// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_coordinate_normalization.hpp"

#include <cmath>
#include <stdexcept>

namespace mirakana {
namespace {

constexpr float half_pi = 1.57079632679489661923F;
constexpr float scale_epsilon = 0.000001F;

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Vec3 normalized_or_original(Vec3 value) noexcept {
    if (!finite_vec3(value)) {
        return value;
    }
    const float magnitude = length(value);
    if (!std::isfinite(magnitude) || magnitude <= scale_epsilon) {
        return value;
    }
    return value * (1.0F / magnitude);
}

[[nodiscard]] Mat4 asset_axis_rotation_matrix(AssetImportMeshUpAxis axis) noexcept {
    if (axis == AssetImportMeshUpAxis::z) {
        return Mat4::rotation_x(-half_pi);
    }
    return Mat4::identity();
}

[[nodiscard]] Mat4 inverse_asset_axis_rotation_matrix(AssetImportMeshUpAxis axis) noexcept {
    if (axis == AssetImportMeshUpAxis::z) {
        return Mat4::rotation_x(half_pi);
    }
    return Mat4::identity();
}

} // namespace

AssetCoordinateNormalizationPlan make_asset_coordinate_normalization_plan(const AssetImportMeshPresetV1& preset) {
    if (!is_valid_asset_import_mesh_preset_v1(preset)) {
        throw std::invalid_argument("asset coordinate normalization preset is invalid");
    }

    AssetCoordinateNormalizationPlan plan;
    plan.preset = preset;
    plan.project_from_source_rotation = preset.up_axis == AssetImportMeshUpAxis::z
                                            ? Quat::from_axis_angle(Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, -half_pi)
                                            : Quat::identity();
    plan.project_from_source =
        asset_axis_rotation_matrix(preset.up_axis) *
        Mat4::scale(Vec3{.x = preset.unit_scale, .y = preset.unit_scale, .z = preset.unit_scale});
    const float inverse_scale = 1.0F / preset.unit_scale;
    plan.source_from_project = Mat4::scale(Vec3{.x = inverse_scale, .y = inverse_scale, .z = inverse_scale}) *
                               inverse_asset_axis_rotation_matrix(preset.up_axis);
    plan.changes_coordinates =
        preset.up_axis != AssetImportMeshUpAxis::y || std::abs(preset.unit_scale - 1.0F) > scale_epsilon;
    return plan;
}

Vec3 normalize_asset_position(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept {
    return transform_point(plan.project_from_source, value);
}

Vec3 normalize_asset_direction(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept {
    const auto rotated = rotate(plan.project_from_source_rotation, value);
    return normalized_or_original(rotated);
}

Vec3 normalize_asset_scale(const AssetCoordinateNormalizationPlan& plan, Vec3 value) noexcept {
    if (plan.preset.up_axis == AssetImportMeshUpAxis::z) {
        return Vec3{.x = value.x, .y = value.z, .z = value.y};
    }
    return value;
}

Quat normalize_asset_rotation(const AssetCoordinateNormalizationPlan& plan, Quat value) noexcept {
    const auto axis = normalize_quat(plan.project_from_source_rotation);
    return normalize_quat(axis * value * conjugate(axis));
}

Mat4 normalize_asset_inverse_bind_matrix(const AssetCoordinateNormalizationPlan& plan, Mat4 value) noexcept {
    return plan.project_from_source * value * plan.source_from_project;
}

} // namespace mirakana
