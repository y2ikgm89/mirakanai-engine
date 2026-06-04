// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct MavgClusterCookTriangle {
    MavgBounds3f bounds;
};

struct MavgClusterCookMaterialPartition {
    AssetId material;
    std::uint32_t first_triangle{0};
    std::uint32_t triangle_count{0};
};

struct MavgClusterCookPackageRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::string content;
    std::uint64_t source_revision{1};
    std::vector<AssetId> dependencies;
};

struct MavgClusterCookRequest {
    AssetId graph_asset;
    AssetId source_mesh;
    std::string source_mesh_uri;
    std::string graph_output_path;
    std::string payload_output_path;
    std::string package_index_path;
    std::uint64_t source_revision{1};
    std::uint32_t target_cluster_triangles{128};
    std::uint64_t page_size_bytes{65536};
    std::vector<MavgClusterCookTriangle> triangles;
    std::vector<MavgClusterCookMaterialPartition> material_partitions;
    std::vector<MavgClusterCookPackageRow> package_rows;
};

struct MavgClusterCookChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct MavgClusterCookFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

struct MavgClusterCookResult {
    MavgClusterGraphDocument graph;
    std::string graph_content;
    std::string payload_content;
    AssetCookedPackageIndex package_index;
    std::string package_index_content;
    std::vector<MavgClusterCookChangedFile> changed_files;
    std::vector<MavgClusterCookFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] MavgClusterCookResult plan_mavg_cluster_graph_cook_package(const MavgClusterCookRequest& request);
[[nodiscard]] MavgClusterCookResult apply_mavg_cluster_graph_cook_package(IFileSystem& filesystem,
                                                                          const MavgClusterCookRequest& request);

} // namespace mirakana
