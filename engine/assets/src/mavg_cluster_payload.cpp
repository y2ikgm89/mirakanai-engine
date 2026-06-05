// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/mavg_cluster_payload.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view mavg_cluster_payload_format = "GameEngine.MavgClusterPayload.v1";

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_hex(std::string_view value) noexcept {
    if ((value.size() % 2U) != 0U) {
        return false;
    }
    return std::ranges::all_of(value, [](char character) noexcept {
        return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') ||
               (character >= 'A' && character <= 'F');
    });
}

[[nodiscard]] std::uint64_t decoded_hex_size(std::string_view value) noexcept {
    return static_cast<std::uint64_t>(value.size() / 2U);
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

[[nodiscard]] std::int32_t parse_i32(std::string_view value, std::string_view context) {
    std::int32_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
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

[[nodiscard]] std::unordered_map<std::string, std::string> parse_key_values(std::string_view text) {
    std::unordered_map<std::string, std::string> values;
    std::size_t line_begin = 0;
    while (line_begin < text.size()) {
        const auto line_end = text.find('\n', line_begin);
        const auto line = text.substr(line_begin, line_end == std::string_view::npos ? std::string_view::npos
                                                                                     : line_end - line_begin);
        if (!line.empty()) {
            const auto [key, value] = split_once(line, '=', "mavg cluster payload key value");
            if (!valid_token(key)) {
                throw std::invalid_argument("mavg cluster payload key is invalid");
            }
            const auto inserted = values.emplace(std::string{key}, std::string{value});
            if (!inserted.second) {
                throw std::invalid_argument("mavg cluster payload key is duplicated");
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
        throw std::invalid_argument("mavg cluster payload required key is missing");
    }
    return it->second;
}

void add_diagnostic(std::vector<MavgClusterPayloadDiagnostic>& diagnostics, MavgClusterPayloadDiagnosticCode code,
                    AssetId asset, std::uint32_t page_index, std::uint32_t cluster_index, std::string field,
                    std::string message) {
    diagnostics.push_back(MavgClusterPayloadDiagnostic{
        .code = code,
        .asset = asset,
        .page_index = page_index,
        .cluster_index = cluster_index,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] const MavgClusterGraphPage* find_graph_page(const MavgClusterGraphDocument& graph,
                                                          std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
    return found == graph.pages.end() ? nullptr : &*found;
}

[[nodiscard]] const MavgClusterGraphCluster* find_graph_cluster(const MavgClusterGraphDocument& graph,
                                                                std::uint32_t cluster_index) noexcept {
    const auto found = std::ranges::find_if(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
    return found == graph.clusters.end() ? nullptr : &*found;
}

[[nodiscard]] bool page_range_overflows(std::uint64_t offset, std::uint64_t size) noexcept {
    return offset > std::numeric_limits<std::uint64_t>::max() - size;
}

[[nodiscard]] bool page_ranges_overlap(const MavgClusterPayloadPage& lhs, const MavgClusterPayloadPage& rhs) noexcept {
    if (page_range_overflows(lhs.byte_offset, lhs.byte_size) || page_range_overflows(rhs.byte_offset, rhs.byte_size)) {
        return false;
    }
    const auto lhs_end = lhs.byte_offset + lhs.byte_size;
    const auto rhs_end = rhs.byte_offset + rhs.byte_size;
    return lhs.byte_offset < rhs_end && rhs.byte_offset < lhs_end;
}

[[nodiscard]] MavgClusterPayloadDocument canonicalize_payload(MavgClusterPayloadDocument document) {
    std::ranges::sort(document.pages, [](const MavgClusterPayloadPage& lhs, const MavgClusterPayloadPage& rhs) {
        return lhs.page_index < rhs.page_index;
    });
    std::ranges::sort(document.clusters,
                      [](const MavgClusterPayloadCluster& lhs, const MavgClusterPayloadCluster& rhs) {
                          return lhs.cluster_index < rhs.cluster_index;
                      });
    return document;
}

} // namespace

std::string_view mavg_cluster_payload_format_v1() noexcept {
    return mavg_cluster_payload_format;
}

MavgClusterPayloadValidationResult validate_mavg_cluster_payload(const MavgClusterPayloadDocument& payload,
                                                                 const MavgClusterGraphDocument& graph) {
    std::vector<MavgClusterPayloadDiagnostic> diagnostics;
    if (payload.asset.value == 0) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_asset, payload.asset, 0, 0, "asset.id",
                       "mavg cluster payload asset id is invalid");
    }
    if (payload.asset != graph.asset) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::graph_asset_mismatch, payload.asset, 0, 0,
                       "asset.id", "mavg cluster payload asset id must match the graph asset");
    }

    const auto graph_validation = validate_mavg_cluster_graph(graph);
    if (!graph_validation.valid()) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_graph, graph.asset, 0, 0, "graph",
                       "mavg cluster payload graph validation failed");
    }

    if (payload.index_format != "uint32") {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_payload_format, payload.asset, 0, 0,
                       "index.format", "mavg cluster payload index format must be uint32");
    }

    const auto vertex_bytes = decoded_hex_size(payload.vertex_data_hex);
    if (payload.vertex_count == 0U || payload.vertex_stride_bytes == 0U || !valid_hex(payload.vertex_data_hex) ||
        vertex_bytes != static_cast<std::uint64_t>(payload.vertex_count) * payload.vertex_stride_bytes) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_vertex_payload, payload.asset, 0, 0,
                       "vertex.data_hex", "mavg cluster payload vertex bytes do not match metadata");
    }

    const auto index_bytes = decoded_hex_size(payload.index_data_hex);
    if (payload.index_count == 0U || !valid_hex(payload.index_data_hex) ||
        index_bytes != static_cast<std::uint64_t>(payload.index_count) * sizeof(std::uint32_t)) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_index_payload, payload.asset, 0, 0,
                       "index.data_hex", "mavg cluster payload index bytes do not match metadata");
    }

    const auto page_bytes = decoded_hex_size(payload.page_data_hex);
    if (payload.pages.empty() || !valid_hex(payload.page_data_hex)) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::missing_page, payload.asset, 0, 0, "page.count",
                       "mavg cluster payload pages and page bytes are required");
    }
    if (payload.clusters.empty()) {
        add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::missing_cluster, payload.asset, 0, 0,
                       "cluster.count", "mavg cluster payload clusters are required");
    }

    std::unordered_set<std::uint32_t> seen_pages;
    for (std::size_t index = 0; index < payload.pages.size(); ++index) {
        const auto& page = payload.pages[index];
        if (!seen_pages.insert(page.page_index).second) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::duplicate_page_index, payload.asset,
                           page.page_index, 0, "page.index", "mavg cluster payload page index is duplicated");
        }

        const auto* const graph_page = find_graph_page(graph, page.page_index);
        if (graph_page == nullptr) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::unknown_graph_page, payload.asset,
                           page.page_index, 0, "page.index", "mavg cluster payload page is not present in graph");
        } else if (page.byte_offset != graph_page->byte_offset || page.byte_size != graph_page->byte_size ||
                   page.first_cluster != graph_page->first_cluster || page.cluster_count != graph_page->cluster_count) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_page_range, payload.asset,
                           page.page_index, 0, "page.range",
                           "mavg cluster payload page row must match graph page byte and cluster ranges");
        }

        if (page.byte_size == 0U || page_range_overflows(page.byte_offset, page.byte_size) ||
            page.byte_offset + page.byte_size > page_bytes) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::invalid_page_range, payload.asset,
                           page.page_index, 0, "page.byte_range",
                           "mavg cluster payload page byte range is outside page data");
        }

        for (std::size_t other = index + 1U; other < payload.pages.size(); ++other) {
            if (page_ranges_overlap(page, payload.pages[other])) {
                add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::overlapping_page_range, payload.asset,
                               page.page_index, 0, "page.byte_range",
                               "mavg cluster payload page byte ranges must not overlap");
            }
        }
    }

    for (const auto& graph_page : graph.pages) {
        if (seen_pages.find(graph_page.page_index) == seen_pages.end()) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::missing_graph_page, payload.asset,
                           graph_page.page_index, 0, "page.index", "mavg cluster payload is missing a graph page");
        }
    }

    std::unordered_set<std::uint32_t> seen_clusters;
    for (const auto& cluster : payload.clusters) {
        if (!seen_clusters.insert(cluster.cluster_index).second) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::duplicate_cluster_index, payload.asset, 0,
                           cluster.cluster_index, "cluster.index", "mavg cluster payload cluster index is duplicated");
        }

        const auto* const graph_cluster = find_graph_cluster(graph, cluster.cluster_index);
        if (graph_cluster == nullptr) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::unknown_graph_cluster, payload.asset, 0,
                           cluster.cluster_index, "cluster.index",
                           "mavg cluster payload cluster is not present in graph");
            continue;
        }
        if (cluster.page_index != graph_cluster->page_index) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::cluster_page_mismatch, payload.asset,
                           cluster.page_index, cluster.cluster_index, "cluster.page",
                           "mavg cluster payload cluster page must match graph cluster page");
        }
        if (cluster.first_index != graph_cluster->first_index || cluster.index_count != graph_cluster->index_count ||
            cluster.vertex_base != graph_cluster->vertex_base) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::cluster_draw_range_mismatch, payload.asset,
                           cluster.page_index, cluster.cluster_index, "cluster.draw_range",
                           "mavg cluster payload cluster draw range must match graph cluster draw range");
        }
    }

    for (const auto& graph_cluster : graph.clusters) {
        if (seen_clusters.find(graph_cluster.cluster_index) == seen_clusters.end()) {
            add_diagnostic(diagnostics, MavgClusterPayloadDiagnosticCode::missing_graph_cluster, payload.asset, 0,
                           graph_cluster.cluster_index, "cluster.index",
                           "mavg cluster payload is missing a graph cluster");
        }
    }

    std::ranges::sort(diagnostics,
                      [](const MavgClusterPayloadDiagnostic& lhs, const MavgClusterPayloadDiagnostic& rhs) {
                          if (lhs.field != rhs.field) {
                              return lhs.field < rhs.field;
                          }
                          if (lhs.page_index != rhs.page_index) {
                              return lhs.page_index < rhs.page_index;
                          }
                          if (lhs.cluster_index != rhs.cluster_index) {
                              return lhs.cluster_index < rhs.cluster_index;
                          }
                          return static_cast<int>(lhs.code) < static_cast<int>(rhs.code);
                      });
    return MavgClusterPayloadValidationResult{.diagnostics = std::move(diagnostics)};
}

std::string serialize_mavg_cluster_payload_document(const MavgClusterPayloadDocument& document) {
    const auto canonical = canonicalize_payload(document);

    std::ostringstream output;
    output << "format=" << mavg_cluster_payload_format << '\n';
    output << "asset.id=" << canonical.asset.value << '\n';
    output << "vertex.count=" << canonical.vertex_count << '\n';
    output << "vertex.stride_bytes=" << canonical.vertex_stride_bytes << '\n';
    output << "vertex.data_hex=" << canonical.vertex_data_hex << '\n';
    output << "index.count=" << canonical.index_count << '\n';
    output << "index.format=" << canonical.index_format << '\n';
    output << "index.data_hex=" << canonical.index_data_hex << '\n';
    output << "page.data_hex=" << canonical.page_data_hex << '\n';
    output << "page.count=" << canonical.pages.size() << '\n';
    for (std::size_t index = 0; index < canonical.pages.size(); ++index) {
        const auto& page = canonical.pages[index];
        output << "page." << index << ".index=" << page.page_index << '\n';
        output << "page." << index << ".byte_offset=" << page.byte_offset << '\n';
        output << "page." << index << ".byte_size=" << page.byte_size << '\n';
        output << "page." << index << ".first_cluster=" << page.first_cluster << '\n';
        output << "page." << index << ".cluster_count=" << page.cluster_count << '\n';
    }
    output << "cluster.count=" << canonical.clusters.size() << '\n';
    for (std::size_t index = 0; index < canonical.clusters.size(); ++index) {
        const auto& cluster = canonical.clusters[index];
        output << "cluster." << index << ".index=" << cluster.cluster_index << '\n';
        output << "cluster." << index << ".page=" << cluster.page_index << '\n';
        output << "cluster." << index << ".first_index=" << cluster.first_index << '\n';
        output << "cluster." << index << ".index_count=" << cluster.index_count << '\n';
        output << "cluster." << index << ".vertex_base=" << cluster.vertex_base << '\n';
    }
    return output.str();
}

MavgClusterPayloadDocument deserialize_mavg_cluster_payload_document(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != mavg_cluster_payload_format) {
        throw std::invalid_argument("mavg cluster payload format is unsupported");
    }

    MavgClusterPayloadDocument document;
    document.asset = AssetId{parse_u64(required_value(values, "asset.id"), "mavg cluster payload asset id")};
    document.vertex_count = parse_u32(required_value(values, "vertex.count"), "mavg cluster payload vertex count");
    document.vertex_stride_bytes =
        parse_u32(required_value(values, "vertex.stride_bytes"), "mavg cluster payload vertex stride");
    document.vertex_data_hex = required_value(values, "vertex.data_hex");
    document.index_count = parse_u32(required_value(values, "index.count"), "mavg cluster payload index count");
    document.index_format = required_value(values, "index.format");
    document.index_data_hex = required_value(values, "index.data_hex");
    document.page_data_hex = required_value(values, "page.data_hex");

    const auto page_count = parse_u32(required_value(values, "page.count"), "mavg cluster payload page count");
    document.pages.reserve(page_count);
    for (std::uint32_t ordinal = 0; ordinal < page_count; ++ordinal) {
        const auto prefix = "page." + std::to_string(ordinal) + ".";
        document.pages.push_back(MavgClusterPayloadPage{
            .page_index = parse_u32(required_value(values, prefix + "index"), "mavg cluster payload page index"),
            .byte_offset =
                parse_u64(required_value(values, prefix + "byte_offset"), "mavg cluster payload page byte offset"),
            .byte_size = parse_u64(required_value(values, prefix + "byte_size"), "mavg cluster payload page byte size"),
            .first_cluster =
                parse_u32(required_value(values, prefix + "first_cluster"), "mavg cluster payload page first cluster"),
            .cluster_count =
                parse_u32(required_value(values, prefix + "cluster_count"), "mavg cluster payload page cluster count"),
        });
    }

    const auto cluster_count = parse_u32(required_value(values, "cluster.count"), "mavg cluster payload cluster count");
    document.clusters.reserve(cluster_count);
    for (std::uint32_t ordinal = 0; ordinal < cluster_count; ++ordinal) {
        const auto prefix = "cluster." + std::to_string(ordinal) + ".";
        document.clusters.push_back(MavgClusterPayloadCluster{
            .cluster_index = parse_u32(required_value(values, prefix + "index"), "mavg cluster payload cluster index"),
            .page_index = parse_u32(required_value(values, prefix + "page"), "mavg cluster payload cluster page"),
            .first_index =
                parse_u32(required_value(values, prefix + "first_index"), "mavg cluster payload cluster first index"),
            .index_count =
                parse_u32(required_value(values, prefix + "index_count"), "mavg cluster payload cluster index count"),
            .vertex_base =
                parse_i32(required_value(values, prefix + "vertex_base"), "mavg cluster payload cluster vertex base"),
        });
    }

    return canonicalize_payload(std::move(document));
}

} // namespace mirakana
