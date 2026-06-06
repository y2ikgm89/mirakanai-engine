// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgDxgiGpuMemoryPressureEvidenceResult& result,
                    RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgGpuMemoryPressureEvidenceDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_page_index(const std::vector<std::uint32_t>& page_indices,
                                       std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

[[nodiscard]] bool contains_mount_id(const std::vector<runtime::RuntimeResidentPackageMountIdV2>& mount_ids,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] bool matches_page_mount(const runtime::RuntimeMavgResidentPageMountRow& page_mount,
                                      const RuntimeMavgResidentPageGpuMemoryEstimateRow& estimate) noexcept {
    return page_mount.graph_asset == estimate.graph_asset && page_mount.page_index == estimate.page_index &&
           page_mount.mount_id == estimate.mount_id;
}

[[nodiscard]] const RuntimeMavgResidentPageGpuMemoryEstimateRow*
find_estimate_row(std::span<const RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates,
                  const runtime::RuntimeMavgResidentPageMountRow& page_mount) noexcept {
    const auto found =
        std::ranges::find_if(estimates, [&page_mount](const RuntimeMavgResidentPageGpuMemoryEstimateRow& estimate) {
            return matches_page_mount(page_mount, estimate);
        });
    if (found == estimates.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] std::uint64_t pressure_score_from_budget(std::uint64_t usage, std::uint64_t budget) noexcept {
    if (usage == 0 || budget == 0) {
        return 0;
    }

    const auto denominator = std::max<std::uint64_t>(1, budget / 10'000U);
    const auto score = usage / denominator;
    return score == 0 ? 1 : score;
}

} // namespace

RuntimeMavgDxgiGpuMemoryPressureEvidenceResult
build_runtime_mavg_dxgi_gpu_memory_pressure_rows(const RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc& desc) {
    RuntimeMavgDxgiGpuMemoryPressureEvidenceResult result;
    result.input_resident_page_mount_count = desc.resident_page_mounts.size();
    result.input_estimated_page_count = desc.estimated_pages.size();
    result.local_video_memory_budget_bytes = desc.memory.local_video_memory_budget_bytes;
    result.local_video_memory_usage_bytes = desc.memory.local_video_memory_usage_bytes;
    result.non_local_video_memory_budget_bytes = desc.memory.non_local_video_memory_budget_bytes;
    result.non_local_video_memory_usage_bytes = desc.memory.non_local_video_memory_usage_bytes;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0, "MAVG DXGI GPU memory pressure graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.backend != rhi::BackendKind::d3d12) {
        add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::unsupported_backend,
                       desc.graph_asset, 0,
                       "MAVG DXGI GPU memory pressure evidence requires selected D3D12 backend diagnostics");
        invalid_inputs = true;
    }
    if (!desc.memory.os_video_memory_budget_available) {
        add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::missing_dxgi_video_memory_budget,
                       desc.graph_asset, 0,
                       "MAVG DXGI GPU memory pressure evidence requires OS video memory budget diagnostics");
        invalid_inputs = true;
    } else if (desc.memory.local_video_memory_budget_bytes == 0) {
        add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_dxgi_video_memory_budget,
                       desc.graph_asset, 0,
                       "MAVG DXGI GPU memory pressure evidence requires a non-zero local video memory budget");
        invalid_inputs = true;
    }

    std::vector<std::uint32_t> resident_page_indices;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> resident_mount_ids;
    for (const auto& mount : desc.resident_page_mounts) {
        if (mount.graph_asset != desc.graph_asset || mount.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_resident_page_mount,
                           mount.graph_asset, mount.page_index,
                           "MAVG DXGI GPU memory pressure resident page mount rows must match the graph and use "
                           "non-zero mount ids");
            invalid_inputs = true;
            continue;
        }
        if (contains_page_index(resident_page_indices, mount.page_index) ||
            contains_mount_id(resident_mount_ids, mount.mount_id)) {
            ++result.duplicate_resident_page_mount_count;
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::duplicate_resident_page_mount,
                           desc.graph_asset, mount.page_index,
                           "MAVG DXGI GPU memory pressure resident page mount rows must be unique by page and mount");
            invalid_inputs = true;
            continue;
        }
        resident_page_indices.push_back(mount.page_index);
        resident_mount_ids.push_back(mount.mount_id);
    }

    std::vector<std::uint32_t> estimated_page_indices;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> estimated_mount_ids;
    for (const auto& estimate : desc.estimated_pages) {
        if (estimate.graph_asset != desc.graph_asset || estimate.mount_id.value == 0 ||
            estimate.estimated_gpu_resident_bytes == 0) {
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_estimate_row,
                           estimate.graph_asset, estimate.page_index,
                           "MAVG DXGI GPU memory pressure estimate rows must match the graph, use non-zero mount ids, "
                           "and report positive estimated bytes");
            invalid_inputs = true;
            continue;
        }
        if (contains_page_index(estimated_page_indices, estimate.page_index) ||
            contains_mount_id(estimated_mount_ids, estimate.mount_id)) {
            ++result.duplicate_estimate_row_count;
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::duplicate_estimate_row,
                           desc.graph_asset, estimate.page_index,
                           "MAVG DXGI GPU memory pressure estimate rows must be unique by page and mount");
            invalid_inputs = true;
            continue;
        }
        const auto has_matching_mount = std::ranges::any_of(
            desc.resident_page_mounts, [&estimate](const runtime::RuntimeMavgResidentPageMountRow& mount) {
                return matches_page_mount(mount, estimate);
            });
        if (!has_matching_mount) {
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::invalid_estimate_row,
                           desc.graph_asset, estimate.page_index,
                           "MAVG DXGI GPU memory pressure estimate rows must reference resident page mounts");
            invalid_inputs = true;
            continue;
        }
        estimated_page_indices.push_back(estimate.page_index);
        estimated_mount_ids.push_back(estimate.mount_id);
    }

    if (invalid_inputs) {
        return result;
    }

    result.local_video_memory_pressure_score = pressure_score_from_budget(desc.memory.local_video_memory_usage_bytes,
                                                                          desc.memory.local_video_memory_budget_bytes);
    result.used_dxgi_video_memory_budget_evidence = true;

    std::vector<runtime::RuntimeMavgResidentPageMountRow> ordered_mounts(desc.resident_page_mounts.begin(),
                                                                         desc.resident_page_mounts.end());
    std::ranges::sort(ordered_mounts, [](const runtime::RuntimeMavgResidentPageMountRow& lhs,
                                         const runtime::RuntimeMavgResidentPageMountRow& rhs) {
        if (lhs.page_index != rhs.page_index) {
            return lhs.page_index < rhs.page_index;
        }
        return lhs.mount_id.value < rhs.mount_id.value;
    });

    for (const auto& mount : ordered_mounts) {
        const auto* const estimate = find_estimate_row(desc.estimated_pages, mount);
        if (estimate == nullptr) {
            ++result.missing_estimate_row_count;
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::missing_estimate_row,
                           desc.graph_asset, mount.page_index,
                           "MAVG DXGI GPU memory pressure requires one estimate row per resident page mount");
            continue;
        }
        if (estimate->estimated_gpu_resident_bytes >
            std::numeric_limits<std::uint64_t>::max() - result.estimated_gpu_resident_bytes) {
            result.estimated_gpu_resident_byte_overflow = true;
            add_diagnostic(result, RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::estimated_byte_overflow,
                           desc.graph_asset, mount.page_index,
                           "MAVG DXGI GPU memory pressure estimated resident byte total overflowed");
            continue;
        }
        result.estimated_gpu_resident_bytes += estimate->estimated_gpu_resident_bytes;
        result.pressure_rows.push_back(runtime::RuntimeMavgPageStreamingGpuMemoryPressureRow{
            .graph_asset = mount.graph_asset,
            .page_index = mount.page_index,
            .mount_id = mount.mount_id,
            .eviction_pressure_score = result.local_video_memory_pressure_score,
            .estimated_gpu_resident_bytes = estimate->estimated_gpu_resident_bytes,
        });
    }

    if (!result.diagnostics.empty()) {
        result.pressure_rows.clear();
        return result;
    }

    result.output_pressure_row_count = result.pressure_rows.size();
    result.produced_gpu_memory_pressure_rows = !result.pressure_rows.empty();
    return result;
}

} // namespace mirakana::runtime_rhi
