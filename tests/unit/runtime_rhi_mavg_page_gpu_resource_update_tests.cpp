// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp"

#include <cstdint>

namespace {

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanResult make_ready_destination_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/page-gpu-resource-update-readiness");
    const mirakana::rhi::BufferHandle payload_buffer{.value = 7};

    return mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanResult{
        .destination_rows =
            {
                mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanRow{
                    .graph_asset = graph_asset,
                    .request_index = 0,
                    .page_index = 2,
                    .mount_id = {.value = 22},
                    .buffer = payload_buffer,
                    .source_file_offset = 128,
                    .source_size = 64,
                    .source_file_path = "runtime/page-gpu-resource-update.pages",
                    .destination_offset = 1024,
                    .destination_size = 64,
                    .destination_range_offset = 1024,
                    .destination_range_size = 64,
                    .estimated_gpu_resident_bytes = 128,
                    .fence_wait_point =
                        mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_gpu_work,
                    .synchronized_with_fence = true,
                },
                mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanRow{
                    .graph_asset = graph_asset,
                    .request_index = 1,
                    .page_index = 0,
                    .mount_id = {.value = 20},
                    .buffer = payload_buffer,
                    .source_file_offset = 0,
                    .source_size = 64,
                    .source_file_path = "runtime/page-gpu-resource-update.pages",
                    .destination_offset = 1088,
                    .destination_size = 64,
                    .destination_range_offset = 1088,
                    .destination_range_size = 64,
                    .estimated_gpu_resident_bytes = 128,
                    .fence_wait_point =
                        mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_gpu_work,
                    .synchronized_with_fence = true,
                },
            },
        .diagnostics = {},
        .requested_page_count = 2,
        .planned_destination_count = 2,
        .input_destination_row_count = 2,
        .duplicate_destination_row_count = 0,
        .missing_destination_row_count = 0,
        .total_destination_bytes = 128,
        .total_estimated_gpu_resident_bytes = 256,
        .invoked_file_io = false,
        .used_native_directstorage = false,
        .submitted_native_queue = false,
        .used_directstorage_resource_destination = false,
        .used_gpu_decompression = false,
        .allocated_rhi_resources = false,
        .enforced_allocator_budget = false,
        .mutated_mount_set = false,
        .exposed_native_handles = false,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchResult
make_completed_dispatch_result(const mirakana::runtime_rhi::RuntimeMavgPageBufferDestinationPlanResult& plan) {
    return mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchResult{
        .diagnostics = {},
        .ticket = 44,
        .backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::directstorage,
        .request_count = plan.planned_destination_count,
        .total_source_bytes = 128,
        .total_destination_bytes = plan.total_destination_bytes,
        .submitted_io_queue = true,
        .enqueued_native_requests = true,
        .submitted_native_queue = true,
        .enqueued_status_write = true,
        .signaled_native_fence = true,
        .native_fence_signal_value = 9,
        .native_fence_completed_value = 9,
        .used_directstorage_resource_destination = true,
        .used_directstorage_caller_owned_rhi_resource_destination = true,
        .directstorage_resource_destination_request_count = plan.planned_destination_count,
        .directstorage_resource_destination_bytes = plan.total_destination_bytes,
        .used_native_directstorage = true,
        .used_win32_async_io = false,
        .mutated_mount_set = false,
        .executed_background_worker = false,
        .touched_renderer_or_rhi_handles = true,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult
make_complete_status_result(const mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchResult& dispatch) {
    return mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult{
        .diagnostics = {},
        .ticket = dispatch.ticket,
        .status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete,
        .hresult = 0,
        .complete = true,
        .failed = false,
        .used_native_directstorage = true,
        .used_win32_async_io = false,
        .signaled_native_fence = true,
        .native_fence_signal_value = dispatch.native_fence_signal_value,
        .native_fence_completed_value = dispatch.native_fence_completed_value,
        .used_directstorage_resource_destination = true,
        .used_directstorage_caller_owned_rhi_resource_destination = true,
        .directstorage_resource_destination_request_count = dispatch.directstorage_resource_destination_request_count,
        .directstorage_resource_destination_bytes = dispatch.directstorage_resource_destination_bytes,
        .mutated_mount_set = false,
        .executed_background_worker = false,
        .touched_renderer_or_rhi_handles = true,
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg page gpu resource update readiness emits resident resource rows after completed rhi "
        "destination") {
    const auto plan = make_ready_destination_plan();
    const auto dispatch = make_completed_dispatch_result(plan);
    const auto status = make_complete_status_result(dispatch);

    const auto result = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &plan,
            .dispatch_result = &dispatch,
            .status_result = &status,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.input_destination_row_count == 2U);
    MK_REQUIRE(result.ready_resource_count == 2U);
    MK_REQUIRE(result.ready_destination_bytes == 128U);
    MK_REQUIRE(result.ready_estimated_gpu_resident_bytes == 256U);
    MK_REQUIRE(result.resident_page_resources.size() == 2U);
    MK_REQUIRE(result.update_rows.size() == 2U);
    MK_REQUIRE(result.resident_page_resources[0].graph_asset == plan.destination_rows[0].graph_asset);
    MK_REQUIRE(result.resident_page_resources[0].page_index == 2U);
    MK_REQUIRE(result.resident_page_resources[0].mount_id.value == 22U);
    MK_REQUIRE(result.resident_page_resources[0].resource.kind == mirakana::rhi::RhiResidencyResourceKind::buffer);
    MK_REQUIRE(result.resident_page_resources[0].resource.buffer.value == 7U);
    MK_REQUIRE(result.resident_page_resources[0].estimated_gpu_resident_bytes == 128U);
    MK_REQUIRE(result.update_rows[0].request_index == 0U);
    MK_REQUIRE(result.update_rows[0].page_index == 2U);
    MK_REQUIRE(result.update_rows[0].destination_offset == 1024U);
    MK_REQUIRE(result.update_rows[0].destination_size == 64U);
    MK_REQUIRE(result.update_rows[0].directstorage_resource_destination_complete);
    MK_REQUIRE(result.update_rows[0].ready_for_residency_action);
    MK_REQUIRE(result.resident_page_resources[1].page_index == 0U);
    MK_REQUIRE(result.update_rows[1].request_index == 1U);
    MK_REQUIRE(result.used_directstorage_resource_destination);
    MK_REQUIRE(result.used_directstorage_caller_owned_rhi_resource_destination);
    MK_REQUIRE(result.directstorage_status_complete);
    MK_REQUIRE(result.observed_native_queue_submission);
    MK_REQUIRE(result.ready_for_residency_actions);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.allocated_rhi_resources);
    MK_REQUIRE(!result.invoked_rhi_residency_action);
    MK_REQUIRE(!result.invoked_native_make_resident);
    MK_REQUIRE(!result.invoked_native_evict);
    MK_REQUIRE(!result.enforced_allocator_budget);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.used_gpu_decompression);
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("runtime rhi mavg page gpu resource update readiness rejects incomplete and failed status evidence") {
    const auto plan = make_ready_destination_plan();
    const auto dispatch = make_completed_dispatch_result(plan);
    auto incomplete_status = make_complete_status_result(dispatch);
    incomplete_status.status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::submitted;
    incomplete_status.complete = false;

    const auto incomplete = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &plan,
            .dispatch_result = &dispatch,
            .status_result = &incomplete_status,
        });

    MK_REQUIRE(!incomplete.succeeded());
    MK_REQUIRE(incomplete.resident_page_resources.empty());
    MK_REQUIRE(has_diagnostic(
        incomplete,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::incomplete_status_result));
    MK_REQUIRE(!incomplete.ready_for_residency_actions);
    MK_REQUIRE(!incomplete.invoked_rhi_residency_action);

    auto failed_status = make_complete_status_result(dispatch);
    failed_status.status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::failed;
    failed_status.failed = true;
    failed_status.hresult = -1;

    const auto failed = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &plan,
            .dispatch_result = &dispatch,
            .status_result = &failed_status,
        });

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failed.resident_page_resources.empty());
    MK_REQUIRE(has_diagnostic(
        failed, mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::failed_status_result));
    MK_REQUIRE(!failed.ready_for_residency_actions);
    MK_REQUIRE(!failed.mutated_mount_set);
}

MK_TEST("runtime rhi mavg page gpu resource update readiness rejects missing rhi destination evidence") {
    const auto plan = make_ready_destination_plan();
    auto dispatch = make_completed_dispatch_result(plan);
    dispatch.used_directstorage_caller_owned_rhi_resource_destination = false;
    auto status = make_complete_status_result(dispatch);
    status.used_directstorage_caller_owned_rhi_resource_destination = false;

    const auto result = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &plan,
            .dispatch_result = &dispatch,
            .status_result = &status,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.update_rows.empty());
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::resource_destination_not_used));
    MK_REQUIRE(!result.ready_for_residency_actions);
    MK_REQUIRE(!result.allocated_rhi_resources);
}

MK_TEST("runtime rhi mavg page gpu resource update readiness rejects byte mismatch and duplicate destinations") {
    const auto plan = make_ready_destination_plan();
    auto dispatch = make_completed_dispatch_result(plan);
    dispatch.directstorage_resource_destination_bytes = 64;
    const auto status = make_complete_status_result(dispatch);

    const auto byte_mismatch = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &plan,
            .dispatch_result = &dispatch,
            .status_result = &status,
        });

    MK_REQUIRE(!byte_mismatch.succeeded());
    MK_REQUIRE(byte_mismatch.resident_page_resources.empty());
    MK_REQUIRE(has_diagnostic(byte_mismatch,
                              mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::
                                  submitted_destination_bytes_mismatch));

    auto duplicate_plan = make_ready_destination_plan();
    duplicate_plan.destination_rows[1].page_index = duplicate_plan.destination_rows[0].page_index;
    const auto duplicate_dispatch = make_completed_dispatch_result(duplicate_plan);
    const auto duplicate_status = make_complete_status_result(duplicate_dispatch);

    const auto duplicate = mirakana::runtime_rhi::make_runtime_mavg_page_gpu_resource_update_readiness(
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDesc{
            .graph_asset = duplicate_plan.destination_rows[0].graph_asset,
            .buffer_destination_plan = &duplicate_plan,
            .dispatch_result = &duplicate_dispatch,
            .status_result = &duplicate_status,
        });

    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.update_rows.empty());
    MK_REQUIRE(has_diagnostic(
        duplicate,
        mirakana::runtime_rhi::RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::duplicate_destination_row));
    MK_REQUIRE(duplicate.duplicate_destination_row_count == 1U);
}

int main() {
    return mirakana::test::run_all();
}
