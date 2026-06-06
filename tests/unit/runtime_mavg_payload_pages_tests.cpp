// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/runtime/mavg_payload_pages.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::string repeated_hex_byte(std::uint8_t value, std::size_t count) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(count * 2U);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(digits[(value >> 4U) & 0x0fU]);
        result.push_back(digits[value & 0x0fU]);
    }
    return result;
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_payload_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/runtime-page-addressable");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/runtime-page-addressable-source");
    const auto material = mirakana::AssetId::from_name("materials/runtime-page-addressable-material");
    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/runtime-page-addressable.glb",
        .cluster_payload_uri = "runtime/mavg/runtime-page-addressable.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 2},
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 2,
                    .vertex_count = 4,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = -1.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 6,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = 0.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 0.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 6,
                    .index_count = 3,
                    .vertex_base = 4,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] std::string make_payload_text(const mirakana::MavgClusterGraphDocument& graph) {
    return mirakana::serialize_mavg_cluster_payload_document(mirakana::MavgClusterPayloadDocument{
        .asset = graph.asset,
        .vertex_count = 4,
        .vertex_stride_bytes = 32,
        .vertex_data_hex = repeated_hex_byte(0x11U, 4U * 32U),
        .index_count = 9,
        .index_format = "uint32",
        .index_data_hex = repeated_hex_byte(0x22U, 9U * 4U),
        .page_data_hex = repeated_hex_byte(0x30U, 64U) + repeated_hex_byte(0x40U, 64U),
        .pages =
            {
                mirakana::MavgClusterPayloadPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 64, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterPayloadPage{
                    .page_index = 1, .byte_offset = 64, .byte_size = 64, .first_cluster = 1, .cluster_count = 1},
            },
        .clusters =
            {
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 0, .page_index = 0, .first_index = 0, .index_count = 6, .vertex_base = 0},
                mirakana::MavgClusterPayloadCluster{
                    .cluster_index = 1, .page_index = 1, .first_index = 6, .index_count = 3, .vertex_base = 4},
            },
    });
}

[[nodiscard]] bool
has_diagnostic(const std::vector<mirakana::runtime::RuntimeMavgPayloadPageSliceDiagnostic>& diagnostics,
               mirakana::runtime::RuntimeMavgPayloadPageSliceDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
has_diagnostic(const std::vector<mirakana::runtime::RuntimeMavgPayloadPageFileLoadDiagnostic>& diagnostics,
               mirakana::runtime::RuntimeMavgPayloadPageFileLoadDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
has_diagnostic(const std::vector<mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDiagnostic>& diagnostics,
               mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
has_diagnostic(const std::vector<mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnostic>& diagnostics,
               mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanResult
make_directstorage_request_plan(const mirakana::MavgClusterGraphDocument& graph, const std::string& payload_text) {
    const std::vector<std::uint32_t> page_indices{1, 0};
    return mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
            .destination_base_offset = 4096U,
            .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_source_access,
            .synchronize_with_fence = true,
        });
}

class RecordingNativeIoDispatcher final : public mirakana::runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit RecordingNativeIoDispatcher(mirakana::runtime::RuntimeMavgPayloadNativeIoBackend backend)
        : backend_(backend) {}

    [[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override {
        return backend_;
    }

    [[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override {
        ++dispatch_call_count;
        last_request_count = requests.size();
        last_submission_tag = desc.submission_tag;
        last_destination_memory_bytes = desc.destination_memory.size();
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult result{
            .ticket = next_ticket++,
            .backend = backend_,
            .enqueued_request_count = requests.size(),
            .submitted_source_bytes = 0,
            .submitted_destination_bytes = 0,
            .submitted_io_queue = true,
            .enqueued_native_requests = false,
            .submitted_native_queue = false,
            .enqueued_status_write = desc.enqueue_status_after_requests,
            .signaled_native_fence = desc.signal_fence_after_requests,
            .native_fence_signal_value = desc.signal_fence_after_requests ? 19U : 0U,
            .native_fence_completed_value = 0U,
            .used_native_directstorage = false,
            .used_win32_async_io = false,
            .executed_background_worker = false,
            .touched_renderer_or_rhi_handles = false,
        };
        if (fail_dispatch) {
            result.submitted_io_queue = false;
            result.diagnostics.push_back(mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnostic{
                .code = mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                .message = "simulated dispatch failure",
            });
            return result;
        }
        for (const auto& request : requests) {
            result.submitted_source_bytes += request.source_size;
            result.submitted_destination_bytes += request.destination_size;
        }
        status_rows.push_back(mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult{
            .ticket = result.ticket,
            .status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::submitted,
            .complete = false,
            .signaled_native_fence = result.signaled_native_fence,
            .native_fence_signal_value = result.native_fence_signal_value,
            .native_fence_completed_value = 0U,
        });
        status_rows.push_back(mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult{
            .ticket = result.ticket,
            .status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete,
            .complete = true,
            .signaled_native_fence = result.signaled_native_fence,
            .native_fence_signal_value = result.native_fence_signal_value,
            .native_fence_completed_value = result.native_fence_signal_value,
        });
        return result;
    }

    [[nodiscard]] mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult
    poll_status(std::uint64_t ticket) override {
        ++status_call_count;
        if (fail_status) {
            return mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult{
                .diagnostics =
                    {
                        mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnostic{
                            .code = mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed,
                            .ticket = ticket,
                            .message = "simulated status failure",
                        },
                    },
                .ticket = ticket,
                .status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::failed,
                .hresult = -1,
                .complete = true,
                .failed = true,
            };
        }
        while (next_status_index < status_rows.size()) {
            const auto status = status_rows[next_status_index++];
            if (status.ticket == ticket) {
                return status;
            }
        }
        return mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult{
            .diagnostics =
                {
                    mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnostic{
                        .code = mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::unknown_ticket,
                        .ticket = ticket,
                        .message = "unknown ticket",
                    },
                },
            .ticket = ticket,
            .status = mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::failed,
            .failed = true,
        };
    }

    mirakana::runtime::RuntimeMavgPayloadNativeIoBackend backend_{
        mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter};
    std::uint64_t next_ticket{700};
    std::size_t dispatch_call_count{0};
    std::size_t status_call_count{0};
    std::size_t last_request_count{0};
    std::size_t last_destination_memory_bytes{0};
    std::uint64_t last_submission_tag{0};
    bool fail_dispatch{false};
    bool fail_status{false};
    std::size_t next_status_index{0};
    std::vector<mirakana::runtime::RuntimeMavgPayloadNativeIoStatusBackendResult> status_rows;
};

} // namespace

MK_TEST("runtime mavg payload page slices extract requested decoded bytes in request order") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> page_indices{1, 0};

    const auto result =
        mirakana::runtime::extract_runtime_mavg_payload_page_slices(mirakana::runtime::RuntimeMavgPayloadPageSliceDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .page_indices = page_indices,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(result.extracted_page_count == 2U);
    MK_REQUIRE(result.pages.size() == 2U);
    MK_REQUIRE(result.pages[0].page_index == 1U);
    MK_REQUIRE(result.pages[0].byte_offset == 64U);
    MK_REQUIRE(result.pages[0].payload_bytes.size() == 64U);
    MK_REQUIRE(result.pages[0].payload_bytes[0] == 0x40U);
    MK_REQUIRE(result.pages[1].page_index == 0U);
    MK_REQUIRE(result.pages[1].byte_offset == 0U);
    MK_REQUIRE(result.pages[1].payload_bytes.size() == 64U);
    MK_REQUIRE(result.pages[1].payload_bytes[0] == 0x30U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page slices reject duplicate and unknown page requests before extraction") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> page_indices{1, 1, 99};

    const auto result =
        mirakana::runtime::extract_runtime_mavg_payload_page_slices(mirakana::runtime::RuntimeMavgPayloadPageSliceDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .page_indices = page_indices,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.pages.empty());
    MK_REQUIRE(result.requested_page_count == 3U);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::runtime::RuntimeMavgPayloadPageSliceDiagnosticCode::duplicate_requested_page));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageSliceDiagnosticCode::unknown_page));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page slices reject invalid payload schema before extraction") {
    const auto graph = make_payload_graph();
    auto payload_text = make_payload_text(graph);
    const auto offset = payload_text.find("page.1.byte_offset=64\n");
    MK_REQUIRE(offset != std::string::npos);
    payload_text.replace(offset, std::string{"page.1.byte_offset=64\n"}.size(), "page.1.byte_offset=32\n");
    const std::vector<std::uint32_t> page_indices{1};

    const auto result =
        mirakana::runtime::extract_runtime_mavg_payload_page_slices(mirakana::runtime::RuntimeMavgPayloadPageSliceDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .page_indices = page_indices,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.pages.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              mirakana::runtime::RuntimeMavgPayloadPageSliceDiagnosticCode::invalid_payload));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload file pages read selected byte ranges from a filesystem blob") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    mirakana::MemoryFileSystem filesystem;
    std::vector<std::uint8_t> blob(128U, 0U);
    std::fill(blob.begin(), blob.begin() + 64, static_cast<std::uint8_t>(0x51U));
    std::fill(blob.begin() + 64, blob.end(), static_cast<std::uint8_t>(0x62U));
    filesystem.write_bytes("runtime/mavg/runtime-page-addressable.pages", blob);
    const std::vector<std::uint32_t> page_indices{1, 0};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_file_pages(mirakana::runtime::RuntimeMavgPayloadPageFileLoadDesc{
            .filesystem = &filesystem,
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(result.loaded_page_count == 2U);
    MK_REQUIRE(result.pages.size() == 2U);
    MK_REQUIRE(result.pages[0].page_index == 1U);
    MK_REQUIRE(result.pages[0].byte_offset == 64U);
    MK_REQUIRE(result.pages[0].payload_bytes.size() == 64U);
    MK_REQUIRE(result.pages[0].payload_bytes[0] == 0x62U);
    MK_REQUIRE(result.pages[1].page_index == 0U);
    MK_REQUIRE(result.pages[1].byte_offset == 0U);
    MK_REQUIRE(result.pages[1].payload_bytes.size() == 64U);
    MK_REQUIRE(result.pages[1].payload_bytes[0] == 0x51U);
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload file pages fail closed when blob byte range is unavailable") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_bytes("runtime/mavg/runtime-page-addressable.pages", std::vector<std::uint8_t>(80U, 0x7aU));
    const std::vector<std::uint32_t> page_indices{1};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_file_pages(mirakana::runtime::RuntimeMavgPayloadPageFileLoadDesc{
            .filesystem = &filesystem,
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.pages.empty());
    MK_REQUIRE(result.requested_page_count == 1U);
    MK_REQUIRE(has_diagnostic(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageFileLoadDiagnosticCode::payload_file_read_failed));
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload directstorage requests plan selected page ranges without executing io") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> page_indices{1, 0};

    const auto result = mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
            .destination_base_offset = 4096U,
            .fence_wait_point = mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_source_access,
            .synchronize_with_fence = true,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(result.planned_request_count == 2U);
    MK_REQUIRE(result.total_source_bytes == 128U);
    MK_REQUIRE(result.total_destination_bytes == 128U);
    MK_REQUIRE(result.requests.size() == 2U);
    MK_REQUIRE(result.requests[0].request_index == 0U);
    MK_REQUIRE(result.requests[0].page_index == 1U);
    MK_REQUIRE(result.requests[0].source_file_offset == 64U);
    MK_REQUIRE(result.requests[0].source_size == 64U);
    MK_REQUIRE(result.requests[0].source_file_path == "runtime/mavg/runtime-page-addressable.pages");
    MK_REQUIRE(result.requests[0].destination_offset == 4096U);
    MK_REQUIRE(result.requests[0].destination_size == 64U);
    MK_REQUIRE(result.requests[0].source_is_file);
    MK_REQUIRE(result.requests[0].destination_is_memory);
    MK_REQUIRE(result.requests[0].synchronized_with_fence);
    MK_REQUIRE(result.requests[0].fence_wait_point ==
               mirakana::runtime::RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_source_access);
    MK_REQUIRE(result.requests[0].debug_name == "mavg.payload.page.1");
    MK_REQUIRE(result.requests[1].request_index == 1U);
    MK_REQUIRE(result.requests[1].page_index == 0U);
    MK_REQUIRE(result.requests[1].source_file_offset == 0U);
    MK_REQUIRE(result.requests[1].source_size == 64U);
    MK_REQUIRE(result.requests[1].source_file_path == "runtime/mavg/runtime-page-addressable.pages");
    MK_REQUIRE(result.requests[1].destination_offset == 4160U);
    MK_REQUIRE(result.requests[1].destination_size == 64U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(result.requires_native_directstorage_sdk);
    MK_REQUIRE(!result.enqueued_native_requests);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.signaled_native_fence);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload directstorage request planning rejects duplicate pages before io") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> page_indices{1, 1};

    const auto result = mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.requests.empty());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(has_diagnostic(
        result.diagnostics,
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::duplicate_requested_page));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.enqueued_native_requests);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.signaled_native_fence);
}

MK_TEST("runtime mavg payload directstorage request planning rejects overflowing destination ranges") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> page_indices{0, 1};

    const auto result = mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = page_indices,
            .destination_base_offset = std::numeric_limits<std::uint64_t>::max() - 32U,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.requests.empty());
    MK_REQUIRE(result.requested_page_count == 2U);
    MK_REQUIRE(has_diagnostic(
        result.diagnostics,
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::destination_range_overflow));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.enqueued_native_requests);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(!result.signaled_native_fence);
}

MK_TEST("runtime mavg payload native io dispatch submits request plan through caller adapter") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const auto request_plan = make_directstorage_request_plan(graph, payload_text);
    std::vector<std::uint8_t> destination_memory(4096U + 128U);
    RecordingNativeIoDispatcher dispatcher{mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter};

    const auto result = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter,
            .submission_tag = 42U,
            .destination_memory = destination_memory,
            .require_native_directstorage = false,
            .enqueue_status_after_requests = true,
            .signal_fence_after_requests = true,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.ticket == 700U);
    MK_REQUIRE(result.backend == mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter);
    MK_REQUIRE(result.request_count == 2U);
    MK_REQUIRE(result.total_source_bytes == 128U);
    MK_REQUIRE(result.total_destination_bytes == 128U);
    MK_REQUIRE(result.submitted_io_queue);
    MK_REQUIRE(!result.enqueued_native_requests);
    MK_REQUIRE(!result.submitted_native_queue);
    MK_REQUIRE(result.enqueued_status_write);
    MK_REQUIRE(result.signaled_native_fence);
    MK_REQUIRE(result.native_fence_signal_value == 19U);
    MK_REQUIRE(result.native_fence_completed_value == 0U);
    MK_REQUIRE(!result.used_native_directstorage);
    MK_REQUIRE(!result.used_win32_async_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(dispatcher.dispatch_call_count == 1U);
    MK_REQUIRE(dispatcher.last_request_count == 2U);
    MK_REQUIRE(dispatcher.last_destination_memory_bytes == destination_memory.size());
    MK_REQUIRE(dispatcher.last_submission_tag == 42U);

    (void)mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
        mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
            .dispatcher = &dispatcher,
            .ticket = result.ticket,
        });
    const auto complete = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
        mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
            .dispatcher = &dispatcher,
            .ticket = result.ticket,
        });
    MK_REQUIRE(complete.succeeded());
    MK_REQUIRE(complete.complete);
    MK_REQUIRE(complete.signaled_native_fence);
    MK_REQUIRE(complete.native_fence_signal_value == result.native_fence_signal_value);
    MK_REQUIRE(complete.native_fence_completed_value == result.native_fence_signal_value);
}

MK_TEST("runtime mavg payload native io dispatch rejects invalid inputs before adapter calls") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const std::vector<std::uint32_t> duplicate_pages{1, 1};
    const auto failed_plan = mirakana::runtime::plan_runtime_mavg_payload_directstorage_requests(
        mirakana::runtime::RuntimeMavgPayloadDirectStorageRequestPlanDesc{
            .graph = &graph,
            .payload_text = payload_text,
            .payload_blob_path = "runtime/mavg/runtime-page-addressable.pages",
            .page_indices = duplicate_pages,
        });
    RecordingNativeIoDispatcher dispatcher{mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter};

    const auto missing_dispatcher = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = nullptr,
            .request_plan = &failed_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter,
            .require_native_directstorage = false,
        });
    const auto invalid_plan = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &failed_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter,
            .require_native_directstorage = false,
        });

    MK_REQUIRE(!missing_dispatcher.succeeded());
    MK_REQUIRE(has_diagnostic(missing_dispatcher.diagnostics,
                              mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::missing_dispatcher));
    MK_REQUIRE(!invalid_plan.succeeded());
    MK_REQUIRE(has_diagnostic(invalid_plan.diagnostics,
                              mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::invalid_request_plan));
    MK_REQUIRE(dispatcher.dispatch_call_count == 0U);
}

MK_TEST("runtime mavg payload native io status polling reports submitted then complete") {
    const auto graph = make_payload_graph();
    const auto payload_text = make_payload_text(graph);
    const auto request_plan = make_directstorage_request_plan(graph, payload_text);
    RecordingNativeIoDispatcher dispatcher{mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter};
    const auto dispatch = mirakana::runtime::dispatch_runtime_mavg_payload_native_io_requests(
        mirakana::runtime::RuntimeMavgPayloadNativeIoDispatchDesc{
            .dispatcher = &dispatcher,
            .request_plan = &request_plan,
            .required_backend = mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter,
            .require_native_directstorage = false,
        });

    const auto submitted = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
        mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
            .dispatcher = &dispatcher,
            .ticket = dispatch.ticket,
        });
    const auto complete = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
        mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
            .dispatcher = &dispatcher,
            .ticket = dispatch.ticket,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(submitted.succeeded());
    MK_REQUIRE(submitted.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::submitted);
    MK_REQUIRE(!submitted.complete);
    MK_REQUIRE(!submitted.failed);
    MK_REQUIRE(!submitted.signaled_native_fence);
    MK_REQUIRE(submitted.native_fence_signal_value == 0U);
    MK_REQUIRE(submitted.native_fence_completed_value == 0U);
    MK_REQUIRE(complete.succeeded());
    MK_REQUIRE(complete.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::complete);
    MK_REQUIRE(complete.complete);
    MK_REQUIRE(!complete.failed);
    MK_REQUIRE(!complete.signaled_native_fence);
    MK_REQUIRE(complete.native_fence_signal_value == 0U);
    MK_REQUIRE(complete.native_fence_completed_value == 0U);
    MK_REQUIRE(dispatcher.status_call_count == 2U);
}

MK_TEST("runtime mavg payload native io status polling reports backend failures") {
    RecordingNativeIoDispatcher dispatcher{mirakana::runtime::RuntimeMavgPayloadNativeIoBackend::test_adapter};
    dispatcher.fail_status = true;

    const auto result = mirakana::runtime::poll_runtime_mavg_payload_native_io_status(
        mirakana::runtime::RuntimeMavgPayloadNativeIoStatusPollDesc{
            .dispatcher = &dispatcher,
            .ticket = 99U,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeMavgPayloadNativeIoStatus::failed);
    MK_REQUIRE(result.failed);
    MK_REQUIRE(result.hresult == -1);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, mirakana::runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed));
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

int main() {
    return mirakana::test::run_all();
}
