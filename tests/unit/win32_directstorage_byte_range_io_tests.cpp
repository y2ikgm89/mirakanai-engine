// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/byte_range_io.hpp"
#include "mirakana/platform/win32/win32_directstorage_byte_range_io.hpp"

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

static_assert(std::derived_from<mirakana::win32::Win32DirectStorageByteRangeExecutor, mirakana::IByteRangeIoExecutor>);
static_assert(!std::copy_constructible<mirakana::win32::Win32DirectStorageByteRangeExecutor>);
static_assert(!std::is_copy_assignable_v<mirakana::win32::Win32DirectStorageByteRangeExecutor>);
static_assert(
    std::is_same_v<decltype(std::declval<mirakana::win32::Win32DirectStorageByteRangeExecutor&>().backend_kind()),
                   mirakana::ByteRangeIoBackendKind>);
static_assert(
    std::is_same_v<decltype(std::declval<mirakana::win32::Win32DirectStorageByteRangeExecutor&>().available()), bool>);
static_assert(
    std::is_same_v<decltype(std::declval<mirakana::win32::Win32DirectStorageByteRangeExecutor&>().diagnostics()),
                   const mirakana::win32::Win32DirectStorageReadDiagnostics&>);
static_assert(std::is_same_v<decltype(std::declval<mirakana::win32::Win32DirectStorageByteRangeExecutor&>().read_ranges(
                                 std::declval<std::span<const mirakana::ByteRangeIoReadRequest>>())),
                             std::vector<mirakana::ByteRangeIoReadRow>>);

namespace {

[[nodiscard]] std::string write_directstorage_test_payload() {
    const auto path = std::filesystem::temp_directory_path() / "mirakana_directstorage_byte_range_io_test.bin";
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    return path.string();
}

[[nodiscard]] std::string bytes_to_string(const std::vector<std::byte>& bytes) {
    std::string result;
    result.reserve(bytes.size());
    for (const auto byte : bytes) {
        result.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    }
    return result;
}

void print_directstorage_diagnostics(const mirakana::win32::Win32DirectStorageReadDiagnostics& diagnostics) {
    std::cerr << "directstorage diagnostics: factory_ready=" << diagnostics.factory_ready
              << " queue_ready=" << diagnostics.queue_ready << " status_array_ready=" << diagnostics.status_array_ready
              << " submitted=" << diagnostics.submitted << " request_count=" << diagnostics.request_count
              << " last_hresult=" << diagnostics.last_hresult << '\n';
}

} // namespace

MK_TEST("win32 directstorage byte range executor exposes only first-party runtime contract") {
    mirakana::win32::Win32DirectStorageByteRangeExecutor executor;

    MK_REQUIRE(executor.backend_kind() == mirakana::ByteRangeIoBackendKind::direct_storage);
    const auto& diagnostics = executor.diagnostics();
    MK_REQUIRE(!diagnostics.submitted);
    MK_REQUIRE(diagnostics.request_count == 0U);
}

MK_TEST("win32 directstorage byte range executor returns empty spans without submitting") {
    mirakana::win32::Win32DirectStorageByteRangeExecutor executor;

    const auto rows = executor.read_ranges({});

    MK_REQUIRE(rows.empty());
    MK_REQUIRE(!executor.diagnostics().submitted);
    MK_REQUIRE(executor.diagnostics().request_count == 0U);
}

#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE_TESTS)

MK_TEST("win32 directstorage byte range executor reads requested file ranges in order") {
    const auto payload_path = write_directstorage_test_payload();
    mirakana::win32::Win32DirectStorageByteRangeExecutor executor(
        mirakana::win32::Win32DirectStorageByteRangeExecutorOptions{
            .queue_capacity = 8,
            .completion_timeout = std::chrono::seconds{10},
        });
    if (!executor.available()) {
        print_directstorage_diagnostics(executor.diagnostics());
    }
    MK_REQUIRE(executor.available());

    const std::vector<mirakana::ByteRangeIoReadRequest> requests{
        mirakana::ByteRangeIoReadRequest{
            .path = payload_path,
            .byte_offset = 0,
            .byte_size = 40,
        },
        mirakana::ByteRangeIoReadRequest{
            .path = payload_path,
            .byte_offset = 40,
            .byte_size = 20,
        },
    };

    const auto rows = executor.read_ranges(requests);

    MK_REQUIRE(rows.size() == requests.size());
    MK_REQUIRE(rows[0].path == payload_path);
    MK_REQUIRE(rows[0].byte_offset == requests[0].byte_offset);
    MK_REQUIRE(rows[0].byte_size == requests[0].byte_size);
    MK_REQUIRE(bytes_to_string(rows[0].bytes) == "format=GameEngine.MavgClusterPayload.v1\n");
    MK_REQUIRE(rows[1].path == payload_path);
    MK_REQUIRE(rows[1].byte_offset == requests[1].byte_offset);
    MK_REQUIRE(rows[1].byte_size == requests[1].byte_size);
    MK_REQUIRE(bytes_to_string(rows[1].bytes) == "page0-cluster-bytes\n");
    MK_REQUIRE(executor.diagnostics().submitted);
    MK_REQUIRE(executor.diagnostics().request_count == requests.size());
    MK_REQUIRE(executor.diagnostics().last_hresult == 0);
}

MK_TEST("win32 directstorage byte range executor fails closed for missing files") {
    const auto payload_path =
        (std::filesystem::temp_directory_path() / "mirakana_missing_directstorage_byte_range_io_test.bin").string();
    mirakana::win32::Win32DirectStorageByteRangeExecutor executor(
        mirakana::win32::Win32DirectStorageByteRangeExecutorOptions{
            .queue_capacity = 8,
            .completion_timeout = std::chrono::seconds{10},
        });
    if (!executor.available()) {
        print_directstorage_diagnostics(executor.diagnostics());
    }
    MK_REQUIRE(executor.available());

    const std::vector<mirakana::ByteRangeIoReadRequest> requests{
        mirakana::ByteRangeIoReadRequest{
            .path = payload_path,
            .byte_offset = 0,
            .byte_size = 4,
        },
    };

    const auto rows = executor.read_ranges(requests);

    MK_REQUIRE(rows.empty());
    MK_REQUIRE(!executor.diagnostics().submitted);
    MK_REQUIRE(executor.diagnostics().request_count == 0U);
    MK_REQUIRE(executor.diagnostics().last_hresult != 0);
}

#endif

int main() {
    return mirakana::test::run_all();
}
