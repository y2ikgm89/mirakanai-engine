// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp"

#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgPageGpuResourceResidencyExecutionResult& result,
                    RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .mount_id = mount_id,
        .message = std::move(message),
    });
}

void copy_readiness_evidence(RuntimeMavgPageGpuResourceResidencyExecutionResult& result,
                             const RuntimeMavgPageGpuResourceUpdateReadinessResult& readiness) noexcept {
    result.input_ready_resource_count = readiness.resident_page_resources.size();
    result.consumed_gpu_resource_update_readiness = true;
    result.used_directstorage_resource_destination = readiness.used_directstorage_resource_destination;
    result.used_directstorage_caller_owned_rhi_resource_destination =
        readiness.used_directstorage_caller_owned_rhi_resource_destination;
    result.directstorage_status_complete = readiness.directstorage_status_complete;
    result.observed_native_queue_submission = readiness.observed_native_queue_submission;
    result.invoked_file_io = false;
    result.submitted_native_queue = false;
    result.allocated_rhi_resources = false;
    result.enforced_allocator_budget = false;
    result.mutated_mount_set = false;
    result.used_gpu_decompression = false;
    result.exposed_native_handles = false;
}

[[nodiscard]] bool
readiness_has_unsupported_side_effects(const RuntimeMavgPageGpuResourceUpdateReadinessResult& readiness) noexcept {
    return readiness.invoked_file_io || readiness.submitted_native_queue || readiness.allocated_rhi_resources ||
           readiness.invoked_rhi_residency_action || readiness.invoked_native_make_resident ||
           readiness.invoked_native_evict || readiness.enforced_allocator_budget || readiness.mutated_mount_set ||
           readiness.used_gpu_decompression || readiness.exposed_native_handles;
}

[[nodiscard]] bool readiness_has_completed_resource_destination(
    const RuntimeMavgPageGpuResourceUpdateReadinessResult& readiness) noexcept {
    return readiness.used_directstorage_resource_destination &&
           readiness.used_directstorage_caller_owned_rhi_resource_destination &&
           readiness.directstorage_status_complete;
}

void copy_residency_evidence(RuntimeMavgPageGpuResourceResidencyExecutionResult& result) noexcept {
    const auto& residency = result.residency_action_result;
    result.selected_page_resource_count = residency.selected_page_resource_count;
    result.protected_page_resource_count = residency.protected_page_resource_count;
    result.eviction_candidate_resource_count = residency.eviction_candidate_resource_count;
    result.made_resident_count = residency.made_resident_count;
    result.evicted_count = residency.evicted_count;
    result.protected_skip_count = residency.protected_skip_count;
    result.invoked_rhi_residency_action = residency.invoked_rhi_residency_action;
    result.invoked_make_resident_action = residency.invoked_make_resident_action;
    result.invoked_evict_action = residency.invoked_evict_action;
    result.invoked_native_make_resident = residency.invoked_native_make_resident;
    result.invoked_native_evict = residency.invoked_native_evict;
    result.invoked_file_io = result.invoked_file_io || residency.invoked_file_io;
    result.enforced_allocator_budget = result.enforced_allocator_budget || residency.enforced_allocator_budget;
    result.mutated_mount_set = result.mutated_mount_set || residency.mutated_mount_set;
    result.used_gpu_decompression = result.used_gpu_decompression || residency.used_gpu_decompression;
    result.exposed_native_handles = result.exposed_native_handles || residency.exposed_native_handles;
}

} // namespace

RuntimeMavgPageGpuResourceResidencyExecutionResult
execute_runtime_mavg_page_gpu_resource_residency_actions(rhi::IRhiDevice& device,
                                                         const RuntimeMavgPageGpuResourceResidencyExecutionDesc& desc) {
    RuntimeMavgPageGpuResourceResidencyExecutionResult result;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource residency execution graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::missing_graph,
                       desc.graph_asset, 0, {}, "MAVG page GPU resource residency execution requires a graph document");
        invalid_inputs = true;
    } else {
        const auto validation = validate_mavg_cluster_graph(*desc.graph);
        if (desc.graph->asset != desc.graph_asset || !validation.valid()) {
            add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_graph,
                           desc.graph->asset, 0, {},
                           "MAVG page GPU resource residency execution graph must match the requested asset and "
                           "validate successfully");
            invalid_inputs = true;
        }
    }

    const auto* const readiness = desc.readiness_result;
    if (readiness == nullptr) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::missing_readiness_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource residency execution requires resource update readiness evidence");
        invalid_inputs = true;
    } else {
        copy_readiness_evidence(result, *readiness);
        if (!readiness->succeeded()) {
            add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource residency execution requires successful readiness evidence");
            invalid_inputs = true;
        }
        if (readiness_has_unsupported_side_effects(*readiness) ||
            !readiness_has_completed_resource_destination(*readiness)) {
            add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_readiness_result,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource residency execution readiness evidence must stay completed, "
                           "caller-owned resource-destination-only, and side-effect-free");
            invalid_inputs = true;
        }
        if (!readiness->ready_for_residency_actions || readiness->resident_page_resources.empty() ||
            readiness->ready_resource_count != readiness->resident_page_resources.size()) {
            add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::readiness_not_ready,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource residency execution requires ready resident page resources");
            invalid_inputs = true;
        }
    }

    if (invalid_inputs) {
        return result;
    }

    result.residency_action_result = execute_runtime_mavg_page_residency_actions(
        device, RuntimeMavgPageResidencyActionDesc{
                    .graph_asset = desc.graph_asset,
                    .graph = desc.graph,
                    .selected_clusters = desc.selected_clusters,
                    .resident_page_resources = readiness->resident_page_resources,
                    .protected_mount_ids = desc.protected_mount_ids,
                    .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
                    .make_selected_pages_resident = desc.make_selected_pages_resident,
                    .evict_reviewed_candidates = desc.evict_reviewed_candidates,
                });
    copy_residency_evidence(result);

    if (!result.residency_action_result.succeeded()) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::residency_action_failed,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource residency execution delegated residency action failed");
    }

    return result;
}

} // namespace mirakana::runtime_rhi
