// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct MavgClusterPayloadPage {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::uint32_t first_cluster{0};
    std::uint32_t cluster_count{0};
};

struct MavgClusterPayloadCluster {
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
};

struct MavgClusterPayloadDocument {
    AssetId asset;
    std::uint32_t vertex_count{0};
    std::uint32_t vertex_stride_bytes{0};
    std::string vertex_data_hex;
    std::uint32_t index_count{0};
    std::string index_format;
    std::string index_data_hex;
    std::string page_data_hex;
    std::vector<MavgClusterPayloadPage> pages;
    std::vector<MavgClusterPayloadCluster> clusters;
};

enum class MavgClusterPayloadDiagnosticCode : std::uint8_t {
    invalid_asset = 0,
    graph_asset_mismatch,
    invalid_graph,
    invalid_payload_format,
    invalid_vertex_payload,
    invalid_index_payload,
    missing_page,
    duplicate_page_index,
    unknown_graph_page,
    missing_graph_page,
    invalid_page_range,
    overlapping_page_range,
    missing_cluster,
    duplicate_cluster_index,
    unknown_graph_cluster,
    missing_graph_cluster,
    cluster_page_mismatch,
    cluster_draw_range_mismatch,
};

struct MavgClusterPayloadDiagnostic {
    MavgClusterPayloadDiagnosticCode code{MavgClusterPayloadDiagnosticCode::invalid_asset};
    AssetId asset;
    std::uint32_t page_index{0};
    std::uint32_t cluster_index{0};
    std::string field;
    std::string message;
};

struct MavgClusterPayloadValidationResult {
    std::vector<MavgClusterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool valid() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] std::string_view mavg_cluster_payload_format_v1() noexcept;
[[nodiscard]] MavgClusterPayloadDocument deserialize_mavg_cluster_payload_document(std::string_view text);
[[nodiscard]] std::string serialize_mavg_cluster_payload_document(const MavgClusterPayloadDocument& document);
[[nodiscard]] MavgClusterPayloadValidationResult
validate_mavg_cluster_payload(const MavgClusterPayloadDocument& payload, const MavgClusterGraphDocument& graph);

} // namespace mirakana
