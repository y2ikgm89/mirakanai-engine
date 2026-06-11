// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/platform/byte_range_io.hpp"
#include "mirakana/platform/filesystem.hpp"

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
    missing_filesystem,
    missing_payload_path,
    payload_path_mismatch,
    missing_direct_storage_executor,
    direct_storage_unavailable,
    unknown_page,
    duplicate_page_request,
    page_range_overflow,
    page_range_out_of_bounds,
    page_range_read_failed,
    direct_storage_page_read_failed,
    direct_storage_result_mismatch,
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

struct RuntimeMavgPayloadFilesystemPageLoadDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    IFileSystem* filesystem{nullptr};
    std::string_view payload_path;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::byte> bytes;
};

struct RuntimeMavgPayloadDirectStoragePageLoadDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    IByteRangeIoExecutor* direct_storage{nullptr};
    std::string_view payload_path;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageLoadResult {
    std::vector<RuntimeMavgPayloadPageRow> loaded_pages;
    std::vector<RuntimeMavgPayloadPageLoadDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t loaded_page_count{0};
    std::size_t payload_byte_count{0};
    std::size_t filesystem_read_byte_count{0};
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

/// Reads reviewed MAVG payload page byte ranges from the first-party filesystem byte-range contract. This remains a
/// narrow synchronous loader: it does not mutate resident mounts, execute background streaming, invoke DirectStorage,
/// touch GPU memory policy, or access renderer/RHI/native handles.
[[nodiscard]] RuntimeMavgPayloadPageLoadResult
load_runtime_mavg_payload_pages_from_filesystem(const RuntimeMavgPayloadFilesystemPageLoadDesc& desc);

/// Executes reviewed MAVG payload page byte ranges through a caller-owned DirectStorage page read executor. The runtime
/// boundary is host-gated and native-handle-free: DirectStorage SDK objects stay behind the executor, unavailable hosts
/// fail closed, and this API does not mutate residency, run background workers, touch GPU memory policy, or access
/// renderer/RHI handles.
[[nodiscard]] RuntimeMavgPayloadPageLoadResult
load_runtime_mavg_payload_pages_from_direct_storage(const RuntimeMavgPayloadDirectStoragePageLoadDesc& desc);

} // namespace mirakana::runtime
