// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeMavgPayloadPageLoadDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    graph_asset_mismatch,
    invalid_graph,
    invalid_payload_format,
    unknown_page,
    duplicate_page_request,
    page_range_overflow,
    page_range_out_of_bounds,
};

struct RuntimeMavgPayloadPageLoadDiagnostic {
    RuntimeMavgPayloadPageLoadDiagnosticCode code{RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPayloadPageLoadDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const std::byte> payload_bytes;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::byte> bytes;
};

struct RuntimeMavgPayloadPageLoadResult {
    std::vector<RuntimeMavgPayloadPageRow> loaded_pages;
    std::vector<RuntimeMavgPayloadPageLoadDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t loaded_page_count{0};
    std::size_t payload_byte_count{0};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool executed_direct_storage{false};
    bool touched_gpu_memory_policy{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

/// Extracts reviewed MAVG payload page byte ranges from caller-owned payload bytes. This is a side-effect-free page
/// loader contract: it does not read files, mutate resident mounts, execute background streaming, invoke DirectStorage,
/// touch GPU memory policy, or access renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPayloadPageLoadResult
load_runtime_mavg_payload_pages(const RuntimeMavgPayloadPageLoadDesc& desc);

} // namespace mirakana::runtime
