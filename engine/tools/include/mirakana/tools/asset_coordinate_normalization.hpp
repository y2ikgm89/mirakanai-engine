// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_presets.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

namespace mirakana {

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
[[nodiscard]] Mat4 normalize_asset_inverse_bind_matrix(const AssetCoordinateNormalizationPlan& plan,
                                                       Mat4 value) noexcept;

} // namespace mirakana
