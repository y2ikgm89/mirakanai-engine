// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/mavg_payload_pages.hpp"
#include "mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp"
#include "mirakana/runtime_rhi/mavg_residency.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_buffer_destination_plan,
    invalid_buffer_destination_plan,
    missing_dispatch_result,
    invalid_dispatch_result,
    missing_status_result,
    incomplete_status_result,
    failed_status_result,
    resource_destination_not_used,
    submitted_request_count_mismatch,
    submitted_destination_bytes_mismatch,
    status_destination_bytes_mismatch,
    invalid_destination_row,
    duplicate_destination_row,
    ticket_mismatch,
};

struct RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic {
    RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode code{
        RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgPageGpuResourceUpdateRow {
    AssetId graph_asset;
    std::uint32_t request_index{0};
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::BufferHandle buffer;
    std::uint64_t destination_offset{0};
    std::uint32_t destination_size{0};
    std::uint64_t destination_range_offset{0};
    std::uint64_t destination_range_size{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
    bool directstorage_resource_destination_complete{false};
    bool ready_for_residency_action{false};
};

struct RuntimeMavgPageGpuResourceUpdateReadinessDesc {
    AssetId graph_asset;
    const RuntimeMavgPageBufferDestinationPlanResult* buffer_destination_plan{nullptr};
    const runtime::RuntimeMavgPayloadNativeIoDispatchResult* dispatch_result{nullptr};
    const runtime::RuntimeMavgPayloadNativeIoStatusPollResult* status_result{nullptr};
};

struct RuntimeMavgPageGpuResourceUpdateReadinessResult {
    std::vector<RuntimeMavgResidentPageResourceRow> resident_page_resources;
    std::vector<RuntimeMavgPageGpuResourceUpdateRow> update_rows;
    std::vector<RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic> diagnostics;
    std::size_t input_destination_row_count{0};
    std::size_t ready_resource_count{0};
    std::size_t duplicate_destination_row_count{0};
    std::uint64_t ready_destination_bytes{0};
    std::uint64_t ready_estimated_gpu_resident_bytes{0};
    bool used_directstorage_resource_destination{false};
    bool used_directstorage_caller_owned_rhi_resource_destination{false};
    bool directstorage_status_complete{false};
    bool observed_native_queue_submission{false};
    bool ready_for_residency_actions{false};
    bool invoked_file_io{false};
    bool submitted_native_queue{false};
    bool allocated_rhi_resources{false};
    bool invoked_rhi_residency_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool enforced_allocator_budget{false};
    bool mutated_mount_set{false};
    bool used_gpu_decompression{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgPageGpuResourceUpdateReadinessResult
make_runtime_mavg_page_gpu_resource_update_readiness(const RuntimeMavgPageGpuResourceUpdateReadinessDesc& desc);

} // namespace mirakana::runtime_rhi
