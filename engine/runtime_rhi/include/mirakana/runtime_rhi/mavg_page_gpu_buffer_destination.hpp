// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime/mavg_payload_pages.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageBufferDestinationDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    invalid_graph,
    missing_request_plan,
    invalid_request_plan,
    invalid_destination_row,
    duplicate_destination_row,
    missing_destination_row,
    destination_range_mismatch,
};

struct RuntimeMavgPageBufferDestinationDiagnostic {
    RuntimeMavgPageBufferDestinationDiagnosticCode code{
        RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgPageBufferDestinationRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::BufferHandle buffer;
    std::uint64_t destination_offset{0};
    std::uint64_t destination_size{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
};

struct RuntimeMavgPageBufferDestinationDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult* request_plan{nullptr};
    std::span<const RuntimeMavgPageBufferDestinationRow> destination_rows;
};

struct RuntimeMavgPageBufferDestinationPlanRow {
    AssetId graph_asset;
    std::uint32_t request_index{0};
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::BufferHandle buffer;
    std::uint64_t source_file_offset{0};
    std::uint32_t source_size{0};
    std::string source_file_path;
    std::uint64_t destination_offset{0};
    std::uint32_t destination_size{0};
    std::uint64_t destination_range_offset{0};
    std::uint64_t destination_range_size{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
    runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint fence_wait_point{
        runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write};
    bool synchronized_with_fence{false};
};

struct RuntimeMavgPageBufferDestinationPlanResult {
    std::vector<RuntimeMavgPageBufferDestinationPlanRow> destination_rows;
    std::vector<RuntimeMavgPageBufferDestinationDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t planned_destination_count{0};
    std::size_t input_destination_row_count{0};
    std::size_t duplicate_destination_row_count{0};
    std::size_t missing_destination_row_count{0};
    std::uint64_t total_destination_bytes{0};
    std::uint64_t total_estimated_gpu_resident_bytes{0};
    bool invoked_file_io{false};
    bool used_native_directstorage{false};
    bool submitted_native_queue{false};
    bool used_directstorage_resource_destination{false};
    bool used_gpu_decompression{false};
    bool allocated_rhi_resources{false};
    bool enforced_allocator_budget{false};
    bool mutated_mount_set{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgPageBufferDestinationPlanResult
plan_runtime_mavg_page_buffer_destinations(const RuntimeMavgPageBufferDestinationDesc& desc);

} // namespace mirakana::runtime_rhi
