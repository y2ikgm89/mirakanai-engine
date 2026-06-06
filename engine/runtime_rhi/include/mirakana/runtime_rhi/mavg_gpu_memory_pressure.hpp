// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/memory_diagnostics.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    unsupported_backend,
    missing_dxgi_video_memory_budget,
    invalid_dxgi_video_memory_budget,
    invalid_resident_page_mount,
    duplicate_resident_page_mount,
    invalid_estimate_row,
    duplicate_estimate_row,
    missing_estimate_row,
    estimated_byte_overflow,
};

struct RuntimeMavgGpuMemoryPressureEvidenceDiagnostic {
    RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode code{
        RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgResidentPageGpuMemoryEstimateRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t estimated_gpu_resident_bytes{0};
};

struct RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc {
    AssetId graph_asset;
    rhi::BackendKind backend{rhi::BackendKind::null};
    rhi::RhiDeviceMemoryDiagnostics memory;
    std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const RuntimeMavgResidentPageGpuMemoryEstimateRow> estimated_pages;
};

struct RuntimeMavgDxgiGpuMemoryPressureEvidenceResult {
    std::vector<runtime::RuntimeMavgPageStreamingGpuMemoryPressureRow> pressure_rows;
    std::vector<RuntimeMavgGpuMemoryPressureEvidenceDiagnostic> diagnostics;
    std::size_t input_resident_page_mount_count{0};
    std::size_t input_estimated_page_count{0};
    std::size_t output_pressure_row_count{0};
    std::size_t missing_estimate_row_count{0};
    std::size_t duplicate_estimate_row_count{0};
    std::size_t duplicate_resident_page_mount_count{0};
    std::uint64_t local_video_memory_budget_bytes{0};
    std::uint64_t local_video_memory_usage_bytes{0};
    std::uint64_t non_local_video_memory_budget_bytes{0};
    std::uint64_t non_local_video_memory_usage_bytes{0};
    std::uint64_t local_video_memory_pressure_score{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
    bool estimated_gpu_resident_byte_overflow{false};
    bool used_dxgi_video_memory_budget_evidence{false};
    bool produced_gpu_memory_pressure_rows{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool touched_renderer_or_rhi_handles{false};
    bool enforced_gpu_residency{false};
    bool reserved_video_memory{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgDxgiGpuMemoryPressureEvidenceResult
build_runtime_mavg_dxgi_gpu_memory_pressure_rows(const RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc& desc);

} // namespace mirakana::runtime_rhi
