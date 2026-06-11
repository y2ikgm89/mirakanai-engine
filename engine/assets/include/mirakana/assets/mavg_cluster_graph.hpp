// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct MavgVec3f {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct MavgBounds3f {
    MavgVec3f min;
    MavgVec3f max;
};

struct MavgClusterGraphPage {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::uint32_t first_cluster{0};
    std::uint32_t cluster_count{0};
};

struct MavgClusterGraphMaterialPartition {
    AssetId material;
    std::uint32_t first_cluster{0};
    std::uint32_t cluster_count{0};
};

struct MavgClusterGraphCluster {
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t local_cluster_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t triangle_count{0};
    std::uint32_t vertex_count{0};
    MavgBounds3f bounds;
    std::uint32_t material_partition{0};
    std::uint32_t parent_cluster_index{0};
    bool has_parent{false};
    std::uint32_t resident_fallback_cluster_index{0};
    float geometric_error{0.0F};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::vector<std::uint32_t> children;
};

struct MavgClusterGraphDocument {
    AssetId asset;
    AssetId source_mesh;
    std::string source_mesh_uri;
    std::string cluster_payload_uri;
    std::uint32_t target_cluster_triangles{0};
    std::uint64_t page_size_bytes{0};
    std::vector<MavgClusterGraphPage> pages;
    std::vector<MavgClusterGraphMaterialPartition> material_partitions;
    std::vector<MavgClusterGraphCluster> clusters;
};

struct MavgLodResidentPageSet {
    std::vector<std::uint32_t> page_indices;
};

struct MavgLodPageRequest {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    float priority{0.0F};
    std::string reason;
};

enum class MavgClusterGraphDiagnosticCode : std::uint8_t {
    invalid_asset,
    invalid_source_mesh,
    invalid_source_mesh_uri,
    invalid_cluster_payload_uri,
    invalid_target_cluster_triangles,
    invalid_page_size_bytes,
    missing_page,
    duplicate_page_index,
    invalid_page_byte_range,
    invalid_page_cluster_range,
    missing_material_partition,
    invalid_material_asset,
    invalid_material_partition_range,
    missing_cluster,
    duplicate_cluster_index,
    unknown_cluster_page,
    invalid_cluster_page_local_index,
    invalid_cluster_counts,
    invalid_cluster_bounds,
    unknown_material_partition,
    unknown_child_cluster,
    self_child_cluster,
    duplicate_child_cluster,
    missing_root_cluster,
    missing_parent_cluster,
    parent_cycle,
    parent_error_less_than_child,
    missing_resident_fallback,
    fallback_not_ancestor,
    invalid_cluster_draw_range,
    invalid_cluster_geometric_error,
};

struct MavgClusterGraphDiagnostic {
    MavgClusterGraphDiagnosticCode code{MavgClusterGraphDiagnosticCode::invalid_asset};
    AssetId asset;
    std::string field;
    std::string message;
};

struct MavgClusterGraphValidationResult {
    std::vector<MavgClusterGraphDiagnostic> diagnostics;

    [[nodiscard]] bool valid() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] std::string_view mavg_cluster_graph_format_v1() noexcept;
[[nodiscard]] std::string_view mavg_cluster_payload_format_v1() noexcept;
[[nodiscard]] MavgClusterGraphValidationResult validate_mavg_cluster_graph(const MavgClusterGraphDocument& document);
[[nodiscard]] MavgClusterGraphDocument canonicalize_mavg_cluster_graph(MavgClusterGraphDocument document);
[[nodiscard]] std::string serialize_mavg_cluster_graph_document(const MavgClusterGraphDocument& document);
[[nodiscard]] MavgClusterGraphDocument deserialize_mavg_cluster_graph_document(std::string_view text);

} // namespace mirakana
