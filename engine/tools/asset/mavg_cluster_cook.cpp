// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/mavg_cluster_cook.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint64_t cluster_payload_stride_bytes = 64U;

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_package_path(std::string_view path) noexcept {
    if (!valid_token(path) || path.front() == '/' || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos) {
        return false;
    }
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

void add_failure(std::vector<MavgClusterCookFailure>& failures, AssetId asset, std::string path,
                 std::string diagnostic) {
    failures.push_back(MavgClusterCookFailure{
        .asset = asset,
        .path = std::move(path),
        .diagnostic = std::move(diagnostic),
    });
}

[[nodiscard]] MavgBounds3f merge_bounds(const MavgBounds3f& lhs, const MavgBounds3f& rhs) noexcept {
    return MavgBounds3f{
        .min =
            MavgVec3f{
                .x = std::min(lhs.min.x, rhs.min.x),
                .y = std::min(lhs.min.y, rhs.min.y),
                .z = std::min(lhs.min.z, rhs.min.z),
            },
        .max =
            MavgVec3f{
                .x = std::max(lhs.max.x, rhs.max.x),
                .y = std::max(lhs.max.y, rhs.max.y),
                .z = std::max(lhs.max.z, rhs.max.z),
            },
    };
}

[[nodiscard]] bool valid_bounds(const MavgBounds3f& bounds) noexcept {
    return bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

[[nodiscard]] std::uint32_t find_material_partition_for_triangle(const MavgClusterCookRequest& request,
                                                                 std::uint32_t triangle_index) noexcept {
    for (std::uint32_t index = 0; index < request.material_partitions.size(); ++index) {
        const auto& partition = request.material_partitions[index];
        if (triangle_index >= partition.first_triangle &&
            triangle_index < partition.first_triangle + partition.triangle_count) {
            return index;
        }
    }
    return std::numeric_limits<std::uint32_t>::max();
}

[[nodiscard]] bool package_row_exists(const MavgClusterCookRequest& request, AssetId asset) noexcept {
    return std::ranges::any_of(request.package_rows,
                               [asset](const MavgClusterCookPackageRow& row) { return row.asset == asset; });
}

[[nodiscard]] std::string package_row_path(const MavgClusterCookRequest& request, AssetId asset,
                                           std::string_view fallback) {
    const auto it = std::ranges::find_if(request.package_rows,
                                         [asset](const MavgClusterCookPackageRow& row) { return row.asset == asset; });
    return it == request.package_rows.end() ? std::string{fallback} : it->path;
}

void validate_request(const MavgClusterCookRequest& request, std::vector<MavgClusterCookFailure>& failures) {
    if (request.graph_asset.value == 0) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg graph asset is invalid");
    }
    if (request.source_mesh.value == 0 || request.source_mesh == request.graph_asset) {
        add_failure(failures, request.graph_asset, request.source_mesh_uri, "mavg source mesh asset is invalid");
    }
    if (!valid_token(request.source_mesh_uri)) {
        add_failure(failures, request.graph_asset, request.source_mesh_uri, "mavg source mesh uri is invalid");
    }
    if (!valid_package_path(request.graph_output_path)) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg graph output path is invalid");
    }
    if (!valid_package_path(request.payload_output_path)) {
        add_failure(failures, request.graph_asset, request.payload_output_path, "mavg payload output path is invalid");
    }
    if (!valid_package_path(request.package_index_path)) {
        add_failure(failures, request.graph_asset, request.package_index_path, "mavg package index path is invalid");
    }
    if (request.graph_output_path == request.payload_output_path ||
        request.graph_output_path == request.package_index_path ||
        request.payload_output_path == request.package_index_path) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg output paths must be distinct");
    }
    if (request.source_revision == 0U) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg source revision is invalid");
    }
    if (request.target_cluster_triangles == 0U) {
        add_failure(failures, request.graph_asset, request.graph_output_path,
                    "mavg target cluster triangle count is invalid");
    }
    if (request.page_size_bytes < cluster_payload_stride_bytes) {
        add_failure(failures, request.graph_asset, request.payload_output_path, "mavg page size is invalid");
    }
    if (request.triangles.empty()) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg triangle input is empty");
    }
    for (const auto& triangle : request.triangles) {
        if (!valid_bounds(triangle.bounds)) {
            add_failure(failures, request.graph_asset, request.graph_output_path, "mavg triangle bounds are invalid");
        }
    }
    if (request.material_partitions.empty()) {
        add_failure(failures, request.graph_asset, request.graph_output_path, "mavg material partition input is empty");
    }
    for (const auto& partition : request.material_partitions) {
        const auto end = static_cast<std::uint64_t>(partition.first_triangle) + partition.triangle_count;
        if (partition.material.value == 0 || partition.material == request.graph_asset ||
            partition.triangle_count == 0U || end > request.triangles.size()) {
            add_failure(failures, request.graph_asset, request.graph_output_path, "mavg material partition is invalid");
        }
        if (!package_row_exists(request, partition.material)) {
            add_failure(failures, partition.material, request.graph_output_path,
                        "mavg material dependency package row is missing");
        }
    }
    if (!package_row_exists(request, request.source_mesh)) {
        add_failure(failures, request.source_mesh, request.graph_output_path,
                    "mavg source mesh dependency package row is missing");
    }
}

[[nodiscard]] std::vector<MavgClusterGraphCluster> build_clusters(const MavgClusterCookRequest& request) {
    std::vector<MavgClusterGraphCluster> clusters;
    const auto cluster_count = static_cast<std::uint32_t>(
        (request.triangles.size() + request.target_cluster_triangles - 1U) / request.target_cluster_triangles);
    clusters.reserve(cluster_count);
    const auto clusters_per_page =
        static_cast<std::uint32_t>(std::max<std::uint64_t>(1U, request.page_size_bytes / cluster_payload_stride_bytes));
    for (std::uint32_t cluster_index = 0; cluster_index < cluster_count; ++cluster_index) {
        const auto first_triangle = cluster_index * request.target_cluster_triangles;
        const auto remaining = static_cast<std::uint32_t>(request.triangles.size() - first_triangle);
        const auto triangle_count = std::min(request.target_cluster_triangles, remaining);
        auto bounds = request.triangles[first_triangle].bounds;
        for (std::uint32_t offset = 1; offset < triangle_count; ++offset) {
            bounds = merge_bounds(bounds, request.triangles[first_triangle + offset].bounds);
        }
        clusters.push_back(MavgClusterGraphCluster{
            .cluster_index = cluster_index,
            .page_index = cluster_index / clusters_per_page,
            .local_cluster_index = cluster_index % clusters_per_page,
            .lod_level = 0U,
            .triangle_count = triangle_count,
            .vertex_count = triangle_count * 3U,
            .bounds = bounds,
            .material_partition = find_material_partition_for_triangle(request, first_triangle),
            .children = {},
        });
    }
    return clusters;
}

[[nodiscard]] std::vector<MavgClusterGraphPage> build_pages(std::uint32_t cluster_count,
                                                            std::uint64_t page_size_bytes) {
    std::vector<MavgClusterGraphPage> pages;
    const auto clusters_per_page =
        static_cast<std::uint32_t>(std::max<std::uint64_t>(1U, page_size_bytes / cluster_payload_stride_bytes));
    for (std::uint32_t first_cluster = 0, page_index = 0; first_cluster < cluster_count; ++page_index) {
        const auto remaining = cluster_count - first_cluster;
        const auto page_cluster_count = std::min(clusters_per_page, remaining);
        pages.push_back(MavgClusterGraphPage{
            .page_index = page_index,
            .byte_offset = static_cast<std::uint64_t>(first_cluster) * cluster_payload_stride_bytes,
            .byte_size = static_cast<std::uint64_t>(page_cluster_count) * cluster_payload_stride_bytes,
            .first_cluster = first_cluster,
            .cluster_count = page_cluster_count,
        });
        first_cluster += page_cluster_count;
    }
    return pages;
}

[[nodiscard]] std::vector<MavgClusterGraphMaterialPartition>
build_material_partitions(const MavgClusterCookRequest& request) {
    std::vector<MavgClusterGraphMaterialPartition> partitions;
    partitions.reserve(request.material_partitions.size());
    for (const auto& partition : request.material_partitions) {
        const auto first_cluster = partition.first_triangle / request.target_cluster_triangles;
        const auto last_cluster =
            (partition.first_triangle + partition.triangle_count - 1U) / request.target_cluster_triangles;
        partitions.push_back(MavgClusterGraphMaterialPartition{
            .material = partition.material,
            .first_cluster = first_cluster,
            .cluster_count = last_cluster - first_cluster + 1U,
        });
    }
    return partitions;
}

[[nodiscard]] MavgClusterGraphDocument build_graph_document(const MavgClusterCookRequest& request) {
    auto clusters = build_clusters(request);
    return canonicalize_mavg_cluster_graph(MavgClusterGraphDocument{
        .asset = request.graph_asset,
        .source_mesh = request.source_mesh,
        .source_mesh_uri = request.source_mesh_uri,
        .cluster_payload_uri = request.payload_output_path,
        .target_cluster_triangles = request.target_cluster_triangles,
        .page_size_bytes = request.page_size_bytes,
        .pages = build_pages(static_cast<std::uint32_t>(clusters.size()), request.page_size_bytes),
        .material_partitions = build_material_partitions(request),
        .clusters = std::move(clusters),
    });
}

[[nodiscard]] std::string serialize_payload(AssetId graph_asset, const MavgClusterGraphDocument& graph) {
    std::ostringstream output;
    output << "format=GameEngine.MavgClusterPayload.v1\n";
    output << "asset.id=" << graph_asset.value << '\n';
    output << "cluster.count=" << graph.clusters.size() << '\n';
    for (std::size_t index = 0; index < graph.clusters.size(); ++index) {
        output << "cluster." << index << ".index=" << graph.clusters[index].cluster_index << '\n';
        output << "cluster." << index << ".triangles=" << graph.clusters[index].triangle_count << '\n';
    }
    return output.str();
}

[[nodiscard]] std::vector<AssetId> graph_dependencies(const MavgClusterCookRequest& request) {
    std::vector<AssetId> dependencies;
    dependencies.push_back(request.source_mesh);
    for (const auto& partition : request.material_partitions) {
        dependencies.push_back(partition.material);
    }
    std::ranges::sort(dependencies, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
    dependencies.erase(std::ranges::unique(dependencies, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }).begin(),
                       dependencies.end());
    return dependencies;
}

[[nodiscard]] AssetCookedPackageIndex build_package_index(const MavgClusterCookRequest& request,
                                                          std::string_view graph_content) {
    std::vector<AssetCookedArtifact> artifacts;
    artifacts.reserve(request.package_rows.size() + 1U);
    for (const auto& row : request.package_rows) {
        artifacts.push_back(AssetCookedArtifact{
            .asset = row.asset,
            .kind = row.kind,
            .path = row.path,
            .content = row.content,
            .source_revision = row.source_revision,
            .dependencies = row.dependencies,
        });
    }
    artifacts.push_back(AssetCookedArtifact{
        .asset = request.graph_asset,
        .kind = AssetKind::mavg_cluster_graph,
        .path = request.graph_output_path,
        .content = std::string{graph_content},
        .source_revision = request.source_revision,
        .dependencies = graph_dependencies(request),
    });

    std::vector<AssetDependencyEdge> edges;
    edges.push_back(AssetDependencyEdge{
        .asset = request.graph_asset,
        .dependency = request.source_mesh,
        .kind = AssetDependencyKind::mavg_source_mesh,
        .path = request.source_mesh_uri,
    });
    std::unordered_set<AssetId, AssetIdHash> seen_materials;
    for (const auto& partition : request.material_partitions) {
        if (seen_materials.insert(partition.material).second) {
            edges.push_back(AssetDependencyEdge{
                .asset = request.graph_asset,
                .dependency = partition.material,
                .kind = AssetDependencyKind::mavg_material,
                .path = package_row_path(request, partition.material, "material"),
            });
        }
    }
    return build_asset_cooked_package_index(std::move(artifacts), std::move(edges));
}

[[nodiscard]] MavgClusterCookChangedFile changed_file(std::string path, std::string content) {
    const auto content_hash = hash_asset_cooked_content(content);
    return MavgClusterCookChangedFile{
        .path = std::move(path),
        .content = std::move(content),
        .content_hash = content_hash,
    };
}

void write_changed_files(IFileSystem& filesystem, const std::vector<MavgClusterCookChangedFile>& files) {
    for (const auto& file : files) {
        filesystem.write_text(file.path, file.content);
    }
}

} // namespace

MavgClusterCookResult plan_mavg_cluster_graph_cook_package(const MavgClusterCookRequest& request) {
    MavgClusterCookResult result;
    validate_request(request, result.failures);
    if (!result.succeeded()) {
        return result;
    }

    try {
        result.graph = build_graph_document(request);
        const auto validation = validate_mavg_cluster_graph(result.graph);
        if (!validation.valid()) {
            add_failure(result.failures, request.graph_asset, request.graph_output_path,
                        validation.diagnostics.front().message);
            return result;
        }
        result.graph_content = serialize_mavg_cluster_graph_document(result.graph);
        result.payload_content = serialize_payload(request.graph_asset, result.graph);
        result.package_index = build_package_index(request, result.graph_content);
        result.package_index_content = serialize_asset_cooked_package_index(result.package_index);
        result.changed_files.push_back(changed_file(request.graph_output_path, result.graph_content));
        result.changed_files.push_back(changed_file(request.payload_output_path, result.payload_content));
        result.changed_files.push_back(changed_file(request.package_index_path, result.package_index_content));
    } catch (const std::exception& error) {
        result.changed_files.clear();
        add_failure(result.failures, request.graph_asset, request.graph_output_path, error.what());
    }

    return result;
}

MavgClusterCookResult apply_mavg_cluster_graph_cook_package(IFileSystem& filesystem,
                                                            const MavgClusterCookRequest& request) {
    auto result = plan_mavg_cluster_graph_cook_package(request);
    if (!result.succeeded()) {
        return result;
    }
    try {
        write_changed_files(filesystem, result.changed_files);
    } catch (const std::exception& error) {
        add_failure(result.failures, request.graph_asset, request.graph_output_path, error.what());
    }
    return result;
}

} // namespace mirakana
