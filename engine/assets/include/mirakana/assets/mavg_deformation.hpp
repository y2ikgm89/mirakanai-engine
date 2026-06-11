// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgDeformationTier : std::uint8_t {
    static_cluster,
    rigid_instance,
    skinned_cluster,
    morph_cluster,
    dynamic_displacement,
};

enum class MavgDeformationDiagnosticCode : std::uint8_t {
    missing_cluster_graph,
    duplicate_tier_row,
    unknown_cluster,
    missing_skinned_bone_bounds,
    invalid_skinned_bone_bounds,
    missing_morph_delta_bounds,
    invalid_morph_delta_bounds,
    unsupported_dynamic_tier,
};

struct MavgDeformationClusterRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgDeformationTier tier{MavgDeformationTier::static_cluster};
};

struct MavgSkinnedClusterBoundsRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t influencing_bone_count{0};
    MavgBounds3f conservative_bounds;
    float max_bone_displacement_radius{0.0F};
};

struct MavgMorphClusterDeltaBoundsRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgBounds3f delta_bounds;
    float max_delta_radius{0.0F};
};

struct MavgDeformationTierDiagnosticsDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const MavgDeformationClusterRow> tier_rows;
    std::span<const MavgSkinnedClusterBoundsRow> skinned_bounds_rows;
    std::span<const MavgMorphClusterDeltaBoundsRow> morph_delta_bounds_rows;
};

struct MavgDeformationDiagnostic {
    MavgDeformationDiagnosticCode code{MavgDeformationDiagnosticCode::missing_cluster_graph};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgDeformationTier tier{MavgDeformationTier::static_cluster};
    std::string message;
};

struct MavgDeformationTierPlanRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgDeformationTier tier{MavgDeformationTier::static_cluster};
    bool supported_by_mavg_cluster_graph{false};
    bool requires_conventional_fallback{true};
    bool requires_runtime_refit{false};
    MavgBounds3f conservative_bounds;
    float bounds_expansion_radius{0.0F};
};

struct MavgDeformationTierDiagnosticsResult {
    std::vector<MavgDeformationTierPlanRow> rows;
    std::vector<MavgDeformationDiagnostic> diagnostics;
    std::uint32_t supported_row_count{0};
    std::uint32_t fallback_row_count{0};
    std::uint32_t skinned_row_count{0};
    std::uint32_t morph_row_count{0};
    bool touched_renderer_rhi_handles{false};
    bool executed_runtime_upload{false};
    bool executed_mesh_shader{false};
    bool executed_directstorage{false};
    bool executed_background_streaming{false};
    bool claimed_broad_optimization{false};
};

[[nodiscard]] MavgDeformationTierDiagnosticsResult
plan_mavg_deformation_tier_diagnostics(const MavgDeformationTierDiagnosticsDesc& desc);

[[nodiscard]] bool has_mavg_deformation_diagnostic(const MavgDeformationTierDiagnosticsResult& result,
                                                   MavgDeformationDiagnosticCode code) noexcept;

} // namespace mirakana
