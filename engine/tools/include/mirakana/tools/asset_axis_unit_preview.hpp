// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetAxisUnitPreviewSampleKind : std::uint8_t { vertex, joint };

struct AssetAxisUnitPreviewBounds {
    Vec3 min;
    Vec3 max;
    bool valid{false};
};

struct AssetAxisUnitPreviewBasisAxis {
    std::string label;
    Vec3 source_axis;
    Vec3 project_axis;
};

struct AssetAxisUnitPreviewSample {
    std::string label;
    Vec3 position;
    Vec3 direction;
    Quat rotation{Quat::identity()};
    bool has_direction{false};
    bool has_rotation{false};
};

struct AssetAxisUnitPreviewRow {
    AssetAxisUnitPreviewSampleKind kind{AssetAxisUnitPreviewSampleKind::vertex};
    std::string label;
    Vec3 source_position;
    Vec3 project_position;
    Vec3 source_direction;
    Vec3 project_direction;
    Quat source_rotation{Quat::identity()};
    Quat project_rotation{Quat::identity()};
    bool has_direction{false};
    bool has_rotation{false};
};

struct AssetAxisUnitPreviewDesc {
    std::string asset_id;
    std::string source_path;
    AssetImportActionKind action_kind{AssetImportActionKind::unknown};
    AssetImportMeshPresetV1 mesh_preset;
    std::vector<AssetAxisUnitPreviewSample> vertex_samples;
    std::vector<AssetAxisUnitPreviewSample> joint_samples;
    std::uint64_t max_sample_rows{128U};
};

struct AssetAxisUnitPreview {
    std::string asset_id;
    std::string source_path;
    AssetImportMeshUpAxis up_axis{AssetImportMeshUpAxis::y};
    float unit_scale{1.0F};
    bool changes_coordinates{false};
    AssetAxisUnitPreviewBounds source_bounds;
    AssetAxisUnitPreviewBounds project_bounds;
    std::vector<AssetAxisUnitPreviewBasisAxis> basis;
    std::vector<AssetAxisUnitPreviewRow> rows;
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool ready() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] AssetAxisUnitPreview build_asset_axis_unit_preview(const AssetAxisUnitPreviewDesc& desc);

} // namespace mirakana
