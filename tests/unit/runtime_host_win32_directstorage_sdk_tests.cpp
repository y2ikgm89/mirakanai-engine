// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"

#include <dstorage.h>
#include <dstorageerr.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ScopedTempDirectory final {
    ScopedTempDirectory() {
        path = std::filesystem::temp_directory_path() / "mirakanai_mavg_directstorage_native_execution";
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~ScopedTempDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    ScopedTempDirectory(const ScopedTempDirectory&) = delete;
    ScopedTempDirectory& operator=(const ScopedTempDirectory&) = delete;
    ScopedTempDirectory(ScopedTempDirectory&&) = delete;
    ScopedTempDirectory& operator=(ScopedTempDirectory&&) = delete;

    std::filesystem::path path;
};

void require(bool value) {
    if (!value) {
        std::terminate();
    }
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
    require(output.good());
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult
poll_until_complete(mirakana::runtime::IRuntimeMavgPayloadNativeIoDispatcher& dispatcher, std::uint64_t ticket) {
    mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollResult status;
    for (std::size_t attempt = 0; attempt < 500U; ++attempt) {
        status = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
            mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
                .dispatcher = &dispatcher,
                .ticket = ticket,
            });
        if (status.complete || status.failed) {
            return status;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return status;
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult make_directstorage_request_plan() {
    mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult request_plan;
    request_plan.requests.push_back(mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow{
        .request_index = 0,
        .page_index = 7,
        .source_file_offset = 2,
        .source_size = 4,
        .source_file_path = "payload.bin",
        .destination_offset = 3,
        .destination_size = 4,
        .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write,
        .source_is_file = true,
        .destination_is_memory = true,
        .synchronized_with_fence = false,
        .debug_name = "mavg.payload.page.7",
    });
    request_plan.requests.push_back(mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow{
        .request_index = 1,
        .page_index = 8,
        .source_file_offset = 8,
        .source_size = 4,
        .source_file_path = "payload.bin",
        .destination_offset = 10,
        .destination_size = 4,
        .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write,
        .source_is_file = true,
        .destination_is_memory = true,
        .synchronized_with_fence = false,
        .debug_name = "mavg.payload.page.8",
    });
    request_plan.requested_page_count = 2;
    request_plan.planned_request_count = 2;
    request_plan.total_source_bytes = 8;
    request_plan.total_destination_bytes = 8;
    request_plan.requires_native_directstorage_sdk = true;
    return request_plan;
}

void run_directstorage_file_to_memory_queue_status_execution_test() {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U,
                                                 0xeeU, 0xefU, 0xb0U, 0xb1U, 0xb2U, 0xb3U};
    write_binary_file(temp.path / "payload.bin", source_bytes);

    auto request_plan = make_directstorage_request_plan();
    std::vector<std::uint8_t> destination(20U, 0xcdU);
    mirakana::Win32MavgPayloadDirectStorageDispatcher dispatcher{
        mirakana::Win32MavgPayloadDirectStorageDispatcherDesc{.root_path = temp.path}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::directstorage,
            .submission_tag = 101U,
            .destination_memory = destination,
            .require_native_directstorage = true,
            .enqueue_status_after_requests = true,
            .signal_fence_after_requests = false,
        });

    require(dispatch.succeeded());
    require(dispatch.ticket != 0U);
    require(dispatch.backend == mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::directstorage);
    require(dispatch.request_count == 2U);
    require(dispatch.total_source_bytes == 8U);
    require(dispatch.total_destination_bytes == 8U);
    require(dispatch.submitted_io_queue);
    require(dispatch.enqueued_native_requests);
    require(dispatch.submitted_native_queue);
    require(dispatch.enqueued_status_write);
    require(!dispatch.signaled_native_fence);
    require(dispatch.used_native_directstorage);
    require(!dispatch.used_win32_async_io);
    require(!dispatch.executed_background_worker);
    require(!dispatch.touched_renderer_or_rhi_handles);

    const auto status = poll_until_complete(dispatcher, dispatch.ticket);

    require(status.succeeded());
    require(status.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete);
    require(status.complete);
    require(!status.failed);
    require(status.used_native_directstorage);
    require(!status.used_win32_async_io);
    require(!status.signaled_native_fence);
    require(!status.executed_background_worker);
    require(!status.touched_renderer_or_rhi_handles);
    require(destination[0] == 0xcdU);
    require(destination[2] == 0xcdU);
    require(destination[3] == 0xa0U);
    require(destination[4] == 0xa1U);
    require(destination[5] == 0xa2U);
    require(destination[6] == 0xa3U);
    require(destination[7] == 0xcdU);
    require(destination[9] == 0xcdU);
    require(destination[10] == 0xb0U);
    require(destination[11] == 0xb1U);
    require(destination[12] == 0xb2U);
    require(destination[13] == 0xb3U);
    require(destination[14] == 0xcdU);
}

void run_directstorage_fence_request_fails_closed_test() {
    ScopedTempDirectory temp;
    const std::vector<std::uint8_t> source_bytes{0x10U, 0x11U, 0xa0U, 0xa1U, 0xa2U, 0xa3U};
    write_binary_file(temp.path / "payload.bin", source_bytes);

    auto request_plan = make_directstorage_request_plan();
    request_plan.requests.resize(1U);
    request_plan.requested_page_count = 1;
    request_plan.planned_request_count = 1;
    request_plan.total_source_bytes = 4;
    request_plan.total_destination_bytes = 4;
    std::vector<std::uint8_t> destination(16U, 0xcdU);
    mirakana::Win32MavgPayloadDirectStorageDispatcher dispatcher{
        mirakana::Win32MavgPayloadDirectStorageDispatcherDesc{.root_path = temp.path}};

    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::directstorage,
            .destination_memory = destination,
            .require_native_directstorage = true,
            .enqueue_status_after_requests = true,
            .signal_fence_after_requests = true,
        });

    require(!dispatch.succeeded());
    require(dispatch.ticket == 0U);
    require(!dispatch.diagnostics.empty());
    require(!dispatch.submitted_io_queue);
    require(!dispatch.enqueued_native_requests);
    require(!dispatch.submitted_native_queue);
    require(!dispatch.signaled_native_fence);
    require(!dispatch.touched_renderer_or_rhi_handles);
    for (const auto byte : destination) {
        require(byte == 0xcdU);
    }
}

void run_directstorage_rejects_unsafe_source_paths_before_factory_test() {
    const std::vector<std::string> unsafe_paths{
        "C:payload.bin",
        R"(\payload.bin)",
        R"(C:\payload.bin)",
        R"(\\server\share\payload.bin)",
    };
    for (const auto& unsafe_path : unsafe_paths) {
        ScopedTempDirectory temp;
        auto request_plan = make_directstorage_request_plan();
        request_plan.requests.resize(1U);
        request_plan.requests.front().source_file_path = unsafe_path;
        request_plan.requested_page_count = 1;
        request_plan.planned_request_count = 1;
        request_plan.total_source_bytes = 4;
        request_plan.total_destination_bytes = 4;
        std::vector<std::uint8_t> destination(16U, 0xcdU);
        mirakana::Win32MavgPayloadDirectStorageDispatcher dispatcher{
            mirakana::Win32MavgPayloadDirectStorageDispatcherDesc{.root_path = temp.path}};

        const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
            mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
                .dispatcher = &dispatcher,
                .request_plan = &request_plan,
                .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::directstorage,
                .destination_memory = destination,
                .require_native_directstorage = true,
                .enqueue_status_after_requests = true,
                .signal_fence_after_requests = false,
            });

        require(!dispatch.succeeded());
        require(dispatch.ticket == 0U);
        require(!dispatch.diagnostics.empty());
        require(dispatch.diagnostics.front().message.find("relative single-line path") != std::string::npos);
        require(!dispatch.submitted_io_queue);
        require(!dispatch.enqueued_native_requests);
        require(!dispatch.submitted_native_queue);
        require(!dispatch.used_native_directstorage);
        for (const auto byte : destination) {
            require(byte == 0xcdU);
        }
    }
}

} // namespace

int main() {
    static_assert(sizeof(DSTORAGE_REQUEST) > 0);
    static_assert(sizeof(DSTORAGE_CONFIGURATION1) > 0);

    auto* const factory_entry = &DStorageGetFactory;
    require(factory_entry != nullptr);

    run_directstorage_file_to_memory_queue_status_execution_test();
    run_directstorage_fence_request_fails_closed_test();
    run_directstorage_rejects_unsafe_source_paths_before_factory_test();
    return 0;
}
