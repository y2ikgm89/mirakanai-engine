// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/mavg_deformation.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <tuple>
#include <unordered_set>

namespace mirakana {
namespace {

struct TierKey {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    MavgDeformationTier tier{MavgDeformationTier::static_cluster};

    friend bool operator==(const TierKey& lhs, const TierKey& rhs) noexcept {
        return lhs.graph_asset == rhs.graph_asset && lhs.cluster_index == rhs.cluster_index && lhs.tier == rhs.tier;
    }
};

struct TierKeyHash {
    [[nodiscard]] std::size_t operator()(const TierKey& key) const noexcept {
        auto value = std::hash<std::uint64_t>{}(key.graph_asset.value);
        value ^= static_cast<std::size_t>(key.cluster_index) + 0x9E3779B9U + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::size_t>(key.tier) + 0x9E3779B9U + (value << 6U) + (value >> 2U);
        return value;
    }
};

[[nodiscard]] bool valid_bounds(const MavgBounds3f& bounds) noexcept {
    return std::isfinite(bounds.min.x) && std::isfinite(bounds.min.y) && std::isfinite(bounds.min.z) &&
           std::isfinite(bounds.max.x) && std::isfinite(bounds.max.y) && std::isfinite(bounds.max.z) &&
           bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

[[nodiscard]] bool valid_radius(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F;
}

[[nodiscard]] MavgBounds3f expand_bounds_by_delta(const MavgBounds3f& base_bounds,
                                                  const MavgBounds3f& delta_bounds) noexcept {
    return MavgBounds3f{
        .min =
            {
                .x = base_bounds.min.x + delta_bounds.min.x,
                .y = base_bounds.min.y + delta_bounds.min.y,
                .z = base_bounds.min.z + delta_bounds.min.z,
            },
        .max =
            {
                .x = base_bounds.max.x + delta_bounds.max.x,
                .y = base_bounds.max.y + delta_bounds.max.y,
                .z = base_bounds.max.z + delta_bounds.max.z,
            },
    };
}

void add_diagnostic(MavgDeformationTierDiagnosticsResult& result, MavgDeformationDiagnosticCode code,
                    const MavgDeformationClusterRow& row, std::string message) {
    result.diagnostics.push_back(MavgDeformationDiagnostic{
        .code = code,
        .graph_asset = row.graph_asset,
        .cluster_index = row.cluster_index,
        .tier = row.tier,
        .message = std::move(message),
    });
}

[[nodiscard]] bool cluster_exists(const MavgClusterGraphDocument& graph, std::uint32_t cluster_index) noexcept {
    return std::ranges::any_of(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster(const MavgClusterGraphDocument& graph,
                                                          std::uint32_t cluster_index) noexcept {
    const auto iter = std::ranges::find_if(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
    return iter == graph.clusters.end() ? nullptr : &(*iter);
}

[[nodiscard]] const MavgSkinnedClusterBoundsRow* find_skinned_bounds(std::span<const MavgSkinnedClusterBoundsRow> rows,
                                                                     AssetId graph_asset,
                                                                     std::uint32_t cluster_index) noexcept {
    const auto iter = std::ranges::find_if(rows, [graph_asset, cluster_index](const MavgSkinnedClusterBoundsRow& row) {
        return row.graph_asset == graph_asset && row.cluster_index == cluster_index;
    });
    return iter == rows.end() ? nullptr : &(*iter);
}

[[nodiscard]] const MavgMorphClusterDeltaBoundsRow*
find_morph_bounds(std::span<const MavgMorphClusterDeltaBoundsRow> rows, AssetId graph_asset,
                  std::uint32_t cluster_index) noexcept {
    const auto iter =
        std::ranges::find_if(rows, [graph_asset, cluster_index](const MavgMorphClusterDeltaBoundsRow& row) {
            return row.graph_asset == graph_asset && row.cluster_index == cluster_index;
        });
    return iter == rows.end() ? nullptr : &(*iter);
}

[[nodiscard]] bool sort_before(const MavgDeformationTierPlanRow& lhs, const MavgDeformationTierPlanRow& rhs) noexcept {
    return std::tuple{lhs.graph_asset.value, lhs.cluster_index, static_cast<std::uint8_t>(lhs.tier)} <
           std::tuple{rhs.graph_asset.value, rhs.cluster_index, static_cast<std::uint8_t>(rhs.tier)};
}

void count_row(MavgDeformationTierDiagnosticsResult& result, const MavgDeformationTierPlanRow& row) noexcept {
    if (row.supported_by_mavg_cluster_graph) {
        ++result.supported_row_count;
    }
    if (row.requires_conventional_fallback) {
        ++result.fallback_row_count;
    }
    if (row.tier == MavgDeformationTier::skinned_cluster) {
        ++result.skinned_row_count;
    }
    if (row.tier == MavgDeformationTier::morph_cluster) {
        ++result.morph_row_count;
    }
}

} // namespace

MavgDeformationTierDiagnosticsResult
plan_mavg_deformation_tier_diagnostics(const MavgDeformationTierDiagnosticsDesc& desc) {
    MavgDeformationTierDiagnosticsResult result;
    if (desc.graph == nullptr) {
        result.diagnostics.push_back(MavgDeformationDiagnostic{
            .code = MavgDeformationDiagnosticCode::missing_cluster_graph,
            .message = "mavg deformation tier diagnostics require a cluster graph",
        });
        return result;
    }

    std::unordered_set<TierKey, TierKeyHash> seen;
    for (const auto& row : desc.tier_rows) {
        const TierKey key{.graph_asset = row.graph_asset, .cluster_index = row.cluster_index, .tier = row.tier};
        if (!seen.insert(key).second) {
            add_diagnostic(result, MavgDeformationDiagnosticCode::duplicate_tier_row, row,
                           "duplicate mavg deformation tier row");
            continue;
        }

        const bool known_graph = row.graph_asset == desc.graph->asset;
        const bool known_cluster = known_graph && cluster_exists(*desc.graph, row.cluster_index);
        const auto* cluster = known_cluster ? find_cluster(*desc.graph, row.cluster_index) : nullptr;
        MavgDeformationTierPlanRow planned{
            .graph_asset = row.graph_asset,
            .cluster_index = row.cluster_index,
            .tier = row.tier,
            .supported_by_mavg_cluster_graph = false,
            .requires_conventional_fallback = true,
            .requires_runtime_refit = false,
            .conservative_bounds = cluster == nullptr ? MavgBounds3f{} : cluster->bounds,
            .bounds_expansion_radius = 0.0F,
        };

        if (!known_cluster) {
            add_diagnostic(result, MavgDeformationDiagnosticCode::unknown_cluster, row,
                           "mavg deformation tier row targets an unknown cluster");
            result.rows.push_back(planned);
            continue;
        }

        switch (row.tier) {
        case MavgDeformationTier::static_cluster:
        case MavgDeformationTier::rigid_instance:
            planned.supported_by_mavg_cluster_graph = true;
            planned.requires_conventional_fallback = false;
            break;
        case MavgDeformationTier::skinned_cluster: {
            const auto* skinned = find_skinned_bounds(desc.skinned_bounds_rows, row.graph_asset, row.cluster_index);
            if (skinned == nullptr) {
                add_diagnostic(result, MavgDeformationDiagnosticCode::missing_skinned_bone_bounds, row,
                               "skinned mavg deformation rows require conservative bone bounds");
                break;
            }
            if (skinned->influencing_bone_count == 0U || !valid_bounds(skinned->conservative_bounds) ||
                !valid_radius(skinned->max_bone_displacement_radius)) {
                add_diagnostic(result, MavgDeformationDiagnosticCode::invalid_skinned_bone_bounds, row,
                               "skinned mavg deformation bounds must be finite and have bone influence evidence");
                break;
            }
            planned.supported_by_mavg_cluster_graph = true;
            planned.requires_conventional_fallback = false;
            planned.conservative_bounds = skinned->conservative_bounds;
            planned.bounds_expansion_radius = skinned->max_bone_displacement_radius;
            break;
        }
        case MavgDeformationTier::morph_cluster: {
            const auto* morph = find_morph_bounds(desc.morph_delta_bounds_rows, row.graph_asset, row.cluster_index);
            if (morph == nullptr) {
                add_diagnostic(result, MavgDeformationDiagnosticCode::missing_morph_delta_bounds, row,
                               "morph mavg deformation rows require precomputed delta bounds");
                break;
            }
            if (!valid_bounds(morph->delta_bounds) || !valid_radius(morph->max_delta_radius)) {
                add_diagnostic(result, MavgDeformationDiagnosticCode::invalid_morph_delta_bounds, row,
                               "morph mavg deformation delta bounds must be finite and non-negative");
                break;
            }
            planned.supported_by_mavg_cluster_graph = true;
            planned.requires_conventional_fallback = false;
            planned.conservative_bounds = expand_bounds_by_delta(planned.conservative_bounds, morph->delta_bounds);
            planned.bounds_expansion_radius = morph->max_delta_radius;
            break;
        }
        case MavgDeformationTier::dynamic_displacement:
            add_diagnostic(result, MavgDeformationDiagnosticCode::unsupported_dynamic_tier, row,
                           "dynamic displacement mavg deformation is not supported by this asset-level slice");
            break;
        }

        result.rows.push_back(planned);
    }

    std::ranges::sort(result.rows, sort_before);
    for (const auto& row : result.rows) {
        count_row(result, row);
    }
    return result;
}

bool has_mavg_deformation_diagnostic(const MavgDeformationTierDiagnosticsResult& result,
                                     MavgDeformationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const MavgDeformationDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
