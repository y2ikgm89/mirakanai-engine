// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view mavg_cluster_graph_format = "GameEngine.MavgClusterGraph.v1";

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_bounds(const MavgBounds3f& bounds) noexcept {
    return std::isfinite(bounds.min.x) && std::isfinite(bounds.min.y) && std::isfinite(bounds.min.z) &&
           std::isfinite(bounds.max.x) && std::isfinite(bounds.max.y) && std::isfinite(bounds.max.z) &&
           bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

void add_diagnostic(std::vector<MavgClusterGraphDiagnostic>& diagnostics, MavgClusterGraphDiagnosticCode code,
                    AssetId asset, std::string field, std::string message) {
    diagnostics.push_back(MavgClusterGraphDiagnostic{
        .code = code,
        .asset = asset,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] std::unordered_set<std::uint32_t>
cluster_index_set(const std::vector<MavgClusterGraphCluster>& clusters) {
    std::unordered_set<std::uint32_t> result;
    for (const auto& cluster : clusters) {
        result.insert(cluster.cluster_index);
    }
    return result;
}

[[nodiscard]] bool cluster_range_is_known(std::uint32_t first_cluster, std::uint32_t cluster_count,
                                          const std::unordered_set<std::uint32_t>& known_clusters) noexcept {
    if (cluster_count == 0U || first_cluster > std::numeric_limits<std::uint32_t>::max() - cluster_count + 1U) {
        return false;
    }
    for (std::uint32_t offset = 0; offset < cluster_count; ++offset) {
        if (known_clusters.find(first_cluster + offset) == known_clusters.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::unordered_map<std::uint32_t, MavgClusterGraphPage>
page_map(const std::vector<MavgClusterGraphPage>& pages) {
    std::unordered_map<std::uint32_t, MavgClusterGraphPage> result;
    for (const auto& page : pages) {
        result.emplace(page.page_index, page);
    }
    return result;
}

[[nodiscard]] bool local_cluster_index_is_unique(const MavgClusterGraphDocument& document, std::uint32_t page_index,
                                                 std::uint32_t local_cluster_index,
                                                 std::uint32_t cluster_index) noexcept {
    for (const auto& cluster : document.clusters) {
        if (cluster.cluster_index != cluster_index && cluster.page_index == page_index &&
            cluster.local_cluster_index == local_cluster_index) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool has_duplicate_child(std::span<const std::uint32_t> children) {
    std::unordered_set<std::uint32_t> seen;
    for (const auto child : children) {
        if (!seen.insert(child).second) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view context) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(std::string(context) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view context) {
    const auto parsed = parse_u64(value, context);
    if (parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(context) + " is out of range");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view context) {
    const auto valid_character = [](char character) noexcept {
        return (character >= '0' && character <= '9') || character == '+' || character == '-' || character == '.' ||
               character == 'e' || character == 'E';
    };
    if (value.empty() || !std::ranges::all_of(value, valid_character)) {
        throw std::invalid_argument(std::string(context) + " is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument(std::string(context) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] std::pair<std::string_view, std::string_view> split_once(std::string_view value, char separator,
                                                                       std::string_view context) {
    const auto position = value.find(separator);
    if (position == std::string_view::npos) {
        throw std::invalid_argument(std::string(context) + " is malformed");
    }
    return {value.substr(0, position), value.substr(position + 1U)};
}

[[nodiscard]] MavgVec3f parse_vec3(std::string_view value, std::string_view context) {
    const auto [x_text, yz_text] = split_once(value, ',', context);
    const auto [y_text, z_text] = split_once(yz_text, ',', context);
    return MavgVec3f{
        .x = parse_float(x_text, context),
        .y = parse_float(y_text, context),
        .z = parse_float(z_text, context),
    };
}

[[nodiscard]] std::vector<std::uint32_t> parse_child_list(std::string_view value) {
    std::vector<std::uint32_t> result;
    if (value.empty()) {
        return result;
    }
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find(',', begin);
        const auto part = value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        result.push_back(parse_u32(part, "mavg cluster child"));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return result;
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_key_values(std::string_view text) {
    std::unordered_map<std::string, std::string> values;
    std::size_t line_begin = 0;
    while (line_begin < text.size()) {
        const auto line_end = text.find('\n', line_begin);
        const auto line = text.substr(line_begin, line_end == std::string_view::npos ? std::string_view::npos
                                                                                     : line_end - line_begin);
        if (!line.empty()) {
            const auto [key, value] = split_once(line, '=', "mavg cluster graph key value");
            if (!valid_token(key)) {
                throw std::invalid_argument("mavg cluster graph key is invalid");
            }
            const auto inserted = values.emplace(std::string{key}, std::string{value});
            if (!inserted.second) {
                throw std::invalid_argument("mavg cluster graph key is duplicated");
            }
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        line_begin = line_end + 1U;
    }
    return values;
}

[[nodiscard]] std::string required_value(const std::unordered_map<std::string, std::string>& values,
                                         std::string_view key) {
    const auto it = values.find(std::string{key});
    if (it == values.end()) {
        throw std::invalid_argument("mavg cluster graph required key is missing");
    }
    return it->second;
}

void throw_if_invalid(const MavgClusterGraphDocument& document) {
    const auto validation = validate_mavg_cluster_graph(document);
    if (!validation.valid()) {
        throw std::invalid_argument(validation.diagnostics.front().message);
    }
}

} // namespace

std::string_view mavg_cluster_graph_format_v1() noexcept {
    return mavg_cluster_graph_format;
}

MavgClusterGraphValidationResult validate_mavg_cluster_graph(const MavgClusterGraphDocument& document) {
    std::vector<MavgClusterGraphDiagnostic> diagnostics;
    if (document.asset.value == 0) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_asset, document.asset, "asset.id",
                       "mavg cluster graph asset id is invalid");
    }
    if (document.source_mesh.value == 0 || document.source_mesh == document.asset) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_source_mesh, document.asset,
                       "source.mesh.asset", "mavg cluster graph source mesh asset is invalid");
    }
    if (!valid_token(document.source_mesh_uri)) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_source_mesh_uri, document.asset,
                       "source.mesh.uri", "mavg cluster graph source mesh uri is invalid");
    }
    if (!valid_token(document.cluster_payload_uri)) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_cluster_payload_uri, document.asset,
                       "cluster.payload.uri", "mavg cluster graph payload uri is invalid");
    }
    if (document.target_cluster_triangles == 0U) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_target_cluster_triangles, document.asset,
                       "cluster.target_triangles", "mavg cluster graph target cluster triangle count is invalid");
    }
    if (document.page_size_bytes == 0U) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_page_size_bytes, document.asset,
                       "page.size_bytes", "mavg cluster graph page size is invalid");
    }
    if (document.pages.empty()) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::missing_page, document.asset, "page.count",
                       "mavg cluster graph must contain at least one page");
    }
    if (document.material_partitions.empty()) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::missing_material_partition, document.asset,
                       "material_partition.count", "mavg cluster graph must contain at least one material partition");
    }
    if (document.clusters.empty()) {
        add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::missing_cluster, document.asset, "cluster.count",
                       "mavg cluster graph must contain at least one cluster");
    }

    const auto known_clusters = cluster_index_set(document.clusters);
    std::unordered_set<std::uint32_t> seen_pages;
    for (const auto& page : document.pages) {
        if (!seen_pages.insert(page.page_index).second) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::duplicate_page_index, document.asset,
                           "page.index", "mavg cluster graph page index is duplicated");
        }
        if (page.byte_size == 0U || (document.page_size_bytes != 0U && page.byte_size > document.page_size_bytes) ||
            !cluster_range_is_known(page.first_cluster, page.cluster_count, known_clusters)) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_page_cluster_range, document.asset,
                           "page.cluster_range", "mavg cluster graph page cluster range is invalid");
        }
    }

    for (const auto& partition : document.material_partitions) {
        if (partition.material.value == 0 || partition.material == document.asset ||
            partition.material == document.source_mesh) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_material_asset, document.asset,
                           "material_partition.material", "mavg cluster graph material asset is invalid");
        }
        if (!cluster_range_is_known(partition.first_cluster, partition.cluster_count, known_clusters)) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_material_partition_range,
                           document.asset, "material_partition.cluster_range",
                           "mavg cluster graph material partition cluster range is invalid");
        }
    }

    const auto pages_by_index = page_map(document.pages);
    std::unordered_set<std::uint32_t> seen_clusters;
    for (const auto& cluster : document.clusters) {
        if (!seen_clusters.insert(cluster.cluster_index).second) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::duplicate_cluster_index, document.asset,
                           "cluster.index", "mavg cluster graph cluster index is duplicated");
        }
        const auto page = pages_by_index.find(cluster.page_index);
        if (page == pages_by_index.end()) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::unknown_cluster_page, document.asset,
                           "cluster.page", "mavg cluster graph cluster page is unknown");
        } else if (cluster.local_cluster_index >= page->second.cluster_count ||
                   page->second.first_cluster + cluster.local_cluster_index != cluster.cluster_index ||
                   !local_cluster_index_is_unique(document, cluster.page_index, cluster.local_cluster_index,
                                                  cluster.cluster_index)) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_cluster_page_local_index,
                           document.asset, "cluster.local_cluster_index",
                           "mavg cluster graph local cluster index is invalid");
        }
        if (cluster.triangle_count == 0U || cluster.vertex_count == 0U) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_cluster_counts, document.asset,
                           "cluster.counts", "mavg cluster graph cluster counts are invalid");
        }
        if (!valid_bounds(cluster.bounds)) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::invalid_cluster_bounds, document.asset,
                           "cluster.bounds", "mavg cluster graph cluster bounds are invalid");
        }
        if (cluster.material_partition >= document.material_partitions.size()) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::unknown_material_partition, document.asset,
                           "cluster.material_partition", "mavg cluster graph material partition is unknown");
        }
        for (const auto child : cluster.children) {
            if (child == cluster.cluster_index) {
                add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::self_child_cluster, document.asset,
                               "cluster.children", "mavg cluster graph cluster cannot reference itself as a child");
            } else if (known_clusters.find(child) == known_clusters.end()) {
                add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::unknown_child_cluster, document.asset,
                               "cluster.children", "mavg cluster graph child cluster is unknown");
            }
        }
        if (has_duplicate_child(cluster.children)) {
            add_diagnostic(diagnostics, MavgClusterGraphDiagnosticCode::duplicate_child_cluster, document.asset,
                           "cluster.children", "mavg cluster graph child cluster is duplicated");
        }
    }

    std::ranges::sort(diagnostics, [](const MavgClusterGraphDiagnostic& lhs, const MavgClusterGraphDiagnostic& rhs) {
        if (lhs.field != rhs.field) {
            return lhs.field < rhs.field;
        }
        return static_cast<int>(lhs.code) < static_cast<int>(rhs.code);
    });
    return MavgClusterGraphValidationResult{.diagnostics = std::move(diagnostics)};
}

MavgClusterGraphDocument canonicalize_mavg_cluster_graph(MavgClusterGraphDocument document) {
    std::ranges::sort(document.pages, [](const MavgClusterGraphPage& lhs, const MavgClusterGraphPage& rhs) {
        return lhs.page_index < rhs.page_index;
    });

    std::vector<std::pair<std::uint32_t, MavgClusterGraphMaterialPartition>> material_partitions;
    material_partitions.reserve(document.material_partitions.size());
    for (std::uint32_t index = 0; index < document.material_partitions.size(); ++index) {
        material_partitions.emplace_back(index, std::move(document.material_partitions[index]));
    }
    std::ranges::sort(material_partitions, [](const auto& lhs, const auto& rhs) {
        if (lhs.second.first_cluster != rhs.second.first_cluster) {
            return lhs.second.first_cluster < rhs.second.first_cluster;
        }
        if (lhs.second.cluster_count != rhs.second.cluster_count) {
            return lhs.second.cluster_count < rhs.second.cluster_count;
        }
        return lhs.second.material.value < rhs.second.material.value;
    });

    std::vector<std::uint32_t> material_partition_remap(document.material_partitions.size(), 0U);
    document.material_partitions.clear();
    document.material_partitions.reserve(material_partitions.size());
    for (std::uint32_t index = 0; index < material_partitions.size(); ++index) {
        material_partition_remap[material_partitions[index].first] = index;
        document.material_partitions.push_back(std::move(material_partitions[index].second));
    }

    for (auto& cluster : document.clusters) {
        if (cluster.material_partition < material_partition_remap.size()) {
            cluster.material_partition = material_partition_remap[cluster.material_partition];
        }
        std::ranges::sort(cluster.children);
        cluster.children.erase(std::ranges::unique(cluster.children).begin(), cluster.children.end());
    }
    std::ranges::sort(document.clusters, [](const MavgClusterGraphCluster& lhs, const MavgClusterGraphCluster& rhs) {
        return lhs.cluster_index < rhs.cluster_index;
    });

    return document;
}

std::string serialize_mavg_cluster_graph_document(const MavgClusterGraphDocument& document) {
    throw_if_invalid(document);
    const auto canonical = canonicalize_mavg_cluster_graph(document);

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "format=" << mavg_cluster_graph_format << '\n';
    output << "asset.id=" << canonical.asset.value << '\n';
    output << "asset.kind=mavg_cluster_graph\n";
    output << "source.mesh.asset=" << canonical.source_mesh.value << '\n';
    output << "source.mesh.uri=" << canonical.source_mesh_uri << '\n';
    output << "cluster.payload.uri=" << canonical.cluster_payload_uri << '\n';
    output << "cluster.target_triangles=" << canonical.target_cluster_triangles << '\n';
    output << "page.size_bytes=" << canonical.page_size_bytes << '\n';
    output << "page.count=" << canonical.pages.size() << '\n';
    for (std::size_t index = 0; index < canonical.pages.size(); ++index) {
        const auto& page = canonical.pages[index];
        output << "page." << index << ".index=" << page.page_index << '\n';
        output << "page." << index << ".byte_offset=" << page.byte_offset << '\n';
        output << "page." << index << ".byte_size=" << page.byte_size << '\n';
        output << "page." << index << ".first_cluster=" << page.first_cluster << '\n';
        output << "page." << index << ".cluster_count=" << page.cluster_count << '\n';
    }
    output << "material_partition.count=" << canonical.material_partitions.size() << '\n';
    for (std::size_t index = 0; index < canonical.material_partitions.size(); ++index) {
        const auto& partition = canonical.material_partitions[index];
        output << "material_partition." << index << ".material=" << partition.material.value << '\n';
        output << "material_partition." << index << ".first_cluster=" << partition.first_cluster << '\n';
        output << "material_partition." << index << ".cluster_count=" << partition.cluster_count << '\n';
    }
    output << "cluster.count=" << canonical.clusters.size() << '\n';
    for (std::size_t index = 0; index < canonical.clusters.size(); ++index) {
        const auto& cluster = canonical.clusters[index];
        output << "cluster." << index << ".index=" << cluster.cluster_index << '\n';
        output << "cluster." << index << ".page=" << cluster.page_index << '\n';
        output << "cluster." << index << ".local_cluster_index=" << cluster.local_cluster_index << '\n';
        output << "cluster." << index << ".lod=" << cluster.lod_level << '\n';
        output << "cluster." << index << ".triangles=" << cluster.triangle_count << '\n';
        output << "cluster." << index << ".vertices=" << cluster.vertex_count << '\n';
        output << "cluster." << index << ".bounds_min=" << cluster.bounds.min.x << ',' << cluster.bounds.min.y << ','
               << cluster.bounds.min.z << '\n';
        output << "cluster." << index << ".bounds_max=" << cluster.bounds.max.x << ',' << cluster.bounds.max.y << ','
               << cluster.bounds.max.z << '\n';
        output << "cluster." << index << ".material_partition=" << cluster.material_partition << '\n';
        output << "cluster." << index << ".children=";
        for (std::size_t child = 0; child < cluster.children.size(); ++child) {
            if (child != 0U) {
                output << ',';
            }
            output << cluster.children[child];
        }
        output << '\n';
    }
    return output.str();
}

MavgClusterGraphDocument deserialize_mavg_cluster_graph_document(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != mavg_cluster_graph_format) {
        throw std::invalid_argument("mavg cluster graph format is unsupported");
    }
    if (required_value(values, "asset.kind") != "mavg_cluster_graph") {
        throw std::invalid_argument("mavg cluster graph asset kind is unsupported");
    }

    MavgClusterGraphDocument document;
    document.asset = AssetId{parse_u64(required_value(values, "asset.id"), "mavg cluster graph asset id")};
    document.source_mesh =
        AssetId{parse_u64(required_value(values, "source.mesh.asset"), "mavg cluster graph source mesh")};
    document.source_mesh_uri = required_value(values, "source.mesh.uri");
    document.cluster_payload_uri = required_value(values, "cluster.payload.uri");
    document.target_cluster_triangles =
        parse_u32(required_value(values, "cluster.target_triangles"), "mavg cluster target triangles");
    document.page_size_bytes = parse_u64(required_value(values, "page.size_bytes"), "mavg cluster page size");

    const auto page_count = parse_u32(required_value(values, "page.count"), "mavg cluster graph page count");
    document.pages.reserve(page_count);
    for (std::uint32_t ordinal = 0; ordinal < page_count; ++ordinal) {
        const auto prefix = "page." + std::to_string(ordinal) + ".";
        document.pages.push_back(MavgClusterGraphPage{
            .page_index = parse_u32(required_value(values, prefix + "index"), "mavg cluster graph page index"),
            .byte_offset = parse_u64(required_value(values, prefix + "byte_offset"), "mavg cluster graph page offset"),
            .byte_size = parse_u64(required_value(values, prefix + "byte_size"), "mavg cluster graph page size"),
            .first_cluster =
                parse_u32(required_value(values, prefix + "first_cluster"), "mavg cluster graph page first cluster"),
            .cluster_count =
                parse_u32(required_value(values, prefix + "cluster_count"), "mavg cluster graph page cluster count"),
        });
    }

    const auto material_partition_count =
        parse_u32(required_value(values, "material_partition.count"), "mavg material partition count");
    document.material_partitions.reserve(material_partition_count);
    for (std::uint32_t ordinal = 0; ordinal < material_partition_count; ++ordinal) {
        const auto prefix = "material_partition." + std::to_string(ordinal) + ".";
        document.material_partitions.push_back(MavgClusterGraphMaterialPartition{
            .material =
                AssetId{parse_u64(required_value(values, prefix + "material"), "mavg material partition asset")},
            .first_cluster =
                parse_u32(required_value(values, prefix + "first_cluster"), "mavg material partition first cluster"),
            .cluster_count =
                parse_u32(required_value(values, prefix + "cluster_count"), "mavg material partition cluster count"),
        });
    }

    const auto cluster_count = parse_u32(required_value(values, "cluster.count"), "mavg cluster count");
    document.clusters.reserve(cluster_count);
    for (std::uint32_t ordinal = 0; ordinal < cluster_count; ++ordinal) {
        const auto prefix = "cluster." + std::to_string(ordinal) + ".";
        document.clusters.push_back(MavgClusterGraphCluster{
            .cluster_index = parse_u32(required_value(values, prefix + "index"), "mavg cluster index"),
            .page_index = parse_u32(required_value(values, prefix + "page"), "mavg cluster page"),
            .local_cluster_index =
                parse_u32(required_value(values, prefix + "local_cluster_index"), "mavg cluster local index"),
            .lod_level = parse_u32(required_value(values, prefix + "lod"), "mavg cluster lod"),
            .triangle_count = parse_u32(required_value(values, prefix + "triangles"), "mavg cluster triangles"),
            .vertex_count = parse_u32(required_value(values, prefix + "vertices"), "mavg cluster vertices"),
            .bounds =
                MavgBounds3f{
                    .min = parse_vec3(required_value(values, prefix + "bounds_min"), "mavg cluster bounds min"),
                    .max = parse_vec3(required_value(values, prefix + "bounds_max"), "mavg cluster bounds max"),
                },
            .material_partition =
                parse_u32(required_value(values, prefix + "material_partition"), "mavg cluster material partition"),
            .children = parse_child_list(required_value(values, prefix + "children")),
        });
    }

    throw_if_invalid(document);
    return canonicalize_mavg_cluster_graph(std::move(document));
}

} // namespace mirakana
