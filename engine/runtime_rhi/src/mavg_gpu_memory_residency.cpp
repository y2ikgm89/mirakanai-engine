// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgGpuMemoryResidencyResult& result, RuntimeMavgGpuMemoryResidencyDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgGpuMemoryResidencyDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void copy_eviction_diagnostics(RuntimeMavgGpuMemoryResidencyResult& result, AssetId graph_asset) {
    for (const auto& diagnostic : result.eviction_review.diagnostics) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::eviction_plan_failed, graph_asset,
                       diagnostic.page_index,
                       diagnostic.message.empty() ? "MAVG GPU memory pressure eviction planning failed"
                                                  : diagnostic.message);
    }

    if (!result.eviction_review.succeeded() && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::eviction_plan_failed, graph_asset, 0,
                       "MAVG GPU memory pressure eviction planning failed");
    }
}

void add_gpu_policy_failure(RuntimeMavgGpuMemoryResidencyResult& result, const GpuMemoryPolicyPlan& policy,
                            AssetId graph_asset) {
    if (policy.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::gpu_memory_policy_failed, graph_asset, 0,
                       "GPU memory policy failed without diagnostics");
        return;
    }

    for (const auto& diagnostic : policy.diagnostics) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::gpu_memory_policy_failed, graph_asset,
                       static_cast<std::uint32_t>(diagnostic.request_index),
                       diagnostic.message.empty() ? "GPU memory policy failed" : diagnostic.message);
    }
}

} // namespace

RuntimeMavgGpuMemoryResidencyResult
plan_runtime_mavg_gpu_memory_pressure_residency(const runtime::RuntimeResidentPackageMountSetV2& mount_set,
                                                const RuntimeMavgGpuMemoryResidencyDesc& desc) {
    RuntimeMavgGpuMemoryResidencyResult result;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG GPU memory residency graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG GPU memory residency requires a graph document");
        invalid_inputs = true;
    }
    if (desc.gpu_memory_policy == nullptr) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_gpu_memory_policy, desc.graph_asset,
                       0, "MAVG GPU memory residency requires a renderer GPU memory policy plan");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& policy = *desc.gpu_memory_policy;
    if (!policy.succeeded()) {
        add_gpu_policy_failure(result, policy, desc.graph_asset);
    }
    if (!policy.memory_budget_evidence_ready) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_memory_budget_evidence,
                       desc.graph_asset, 0,
                       "MAVG GPU memory residency requires declared local or non-local memory budget evidence");
    }
    if (!policy.residency_pressure_evidence_ready) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_residency_pressure_evidence,
                       desc.graph_asset, 0,
                       "MAVG GPU memory residency requires GPU residency pressure evidence before eviction planning");
    }
    if (!policy.package_counter_evidence_ready) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::missing_package_counter_evidence,
                       desc.graph_asset, 0,
                       "MAVG GPU memory residency requires package-visible memory counter evidence");
    }
    if (policy.total_counted_bytes == 0) {
        add_diagnostic(result, RuntimeMavgGpuMemoryResidencyDiagnosticCode::invalid_target_budget, desc.graph_asset, 0,
                       "MAVG GPU memory residency requires a non-zero counted byte target");
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.target_resident_content_bytes = policy.total_counted_bytes;
    result.target_budget.max_resident_content_bytes = policy.total_counted_bytes;
    result.eviction_review = runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set, runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
                       .graph_asset = desc.graph_asset,
                       .graph = desc.graph,
                       .selected_clusters = desc.selected_clusters,
                       .resident_page_mounts = desc.resident_page_mounts,
                       .policy_kind = desc.policy_kind,
                       .recency_rows = desc.recency_rows,
                       .caller_protected_mount_ids = desc.caller_protected_mount_ids,
                       .target_budget = result.target_budget,
                       .overlay = desc.overlay,
                   });

    result.invoked_eviction_plan = result.eviction_review.invoked_eviction_plan;
    result.protected_mount_count = result.eviction_review.protected_mount_ids.size();
    result.eviction_candidate_count = result.eviction_review.eviction_candidate_unmount_order.size();
    result.evicted_mount_count = result.eviction_review.eviction_plan.steps.size();
    result.invoked_file_io = result.eviction_review.invoked_file_io;
    result.mutated_mount_set = result.eviction_review.mutated_mount_set;
    result.touched_renderer_or_rhi_handles = result.eviction_review.touched_renderer_or_rhi_handles;
    copy_eviction_diagnostics(result, desc.graph_asset);
    result.applied_gpu_memory_pressure_policy = result.diagnostics.empty() && result.eviction_review.succeeded();
    return result;
}

bool has_runtime_mavg_gpu_memory_residency_diagnostic(const RuntimeMavgGpuMemoryResidencyResult& result,
                                                      RuntimeMavgGpuMemoryResidencyDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const RuntimeMavgGpuMemoryResidencyDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana::runtime_rhi
