// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/filesystem.hpp"
#include "mirakana/runtime/mavg_payload_page_loader.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::span<const std::byte> bytes_view(const std::string& text) noexcept {
    return std::as_bytes(std::span<const char>(text.data(), text.size()));
}

[[nodiscard]] std::vector<std::byte> byte_copy(std::string_view text) {
    const auto bytes = std::as_bytes(std::span<const char>(text.data(), text.size()));
    return std::vector<std::byte>(bytes.begin(), bytes.end());
}

struct RangeReadCall {
    std::string path;
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
};

struct DirectStorageReadCall {
    std::string path;
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
};

class RecordingRangeFileSystem final : public mirakana::IFileSystem {
  public:
    RecordingRangeFileSystem(std::string payload_path, std::string payload)
        : payload_path_(std::move(payload_path)), payload_(std::move(payload)) {}

    [[nodiscard]] bool exists(std::string_view path) const override {
        return path == payload_path_;
    }

    [[nodiscard]] bool is_directory(std::string_view) const override {
        return false;
    }

    [[nodiscard]] std::string read_text(std::string_view) const override {
        ++read_text_call_count;
        throw std::runtime_error("text reads are not allowed for mavg payload byte ranges");
    }

    [[nodiscard]] std::vector<std::byte> read_binary_range(std::string_view path, std::uint64_t byte_offset,
                                                           std::uint64_t byte_size) const override {
        range_reads.push_back(RangeReadCall{
            .path = std::string(path),
            .byte_offset = byte_offset,
            .byte_size = byte_size,
        });
        if (path != payload_path_) {
            throw std::out_of_range("unexpected payload path");
        }
        if (byte_offset > payload_.size() || byte_size > payload_.size() - static_cast<std::size_t>(byte_offset)) {
            throw std::out_of_range("range outside payload");
        }
        const auto begin = payload_.begin() + static_cast<std::ptrdiff_t>(byte_offset);
        const auto end = begin + static_cast<std::ptrdiff_t>(byte_size);
        return byte_copy(std::string_view(&*begin, static_cast<std::size_t>(end - begin)));
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view) const override {
        return {};
    }

    void write_text(std::string_view, std::string_view) override {
        throw std::runtime_error("writes are not allowed");
    }

    void remove(std::string_view) override {
        throw std::runtime_error("removes are not allowed");
    }

    void remove_empty_directory(std::string_view) override {
        throw std::runtime_error("directory removes are not allowed");
    }

    mutable std::vector<RangeReadCall> range_reads;
    mutable int read_text_call_count{0};

  private:
    std::string payload_path_;
    std::string payload_;
};

class RecordingDirectStorageExecutor final : public mirakana::IByteRangeIoExecutor {
  public:
    RecordingDirectStorageExecutor(std::string payload_path, std::string payload, bool available = true)
        : payload_path_(std::move(payload_path)), payload_(std::move(payload)), available_(available) {}

    [[nodiscard]] mirakana::ByteRangeIoBackendKind backend_kind() const noexcept override {
        return mirakana::ByteRangeIoBackendKind::direct_storage;
    }

    [[nodiscard]] bool available() const noexcept override {
        return available_;
    }

    [[nodiscard]] std::vector<mirakana::ByteRangeIoReadRow>
    read_ranges(std::span<const mirakana::ByteRangeIoReadRequest> requests) override {
        const bool after_prefix_call = !calls.empty();
        if (fail_after_prefix && after_prefix_call) {
            throw std::runtime_error("DirectStorage page read failed");
        }
        std::vector<mirakana::ByteRangeIoReadRow> rows;
        rows.reserve(requests.size());
        for (const auto& request : requests) {
            calls.push_back(DirectStorageReadCall{
                .path = std::string(request.path),
                .byte_offset = request.byte_offset,
                .byte_size = request.byte_size,
            });
            if (request.path != payload_path_) {
                throw std::runtime_error("unexpected DirectStorage payload path");
            }
            if (request.byte_offset > payload_.size() ||
                request.byte_size > payload_.size() - static_cast<std::size_t>(request.byte_offset)) {
                throw std::out_of_range("DirectStorage range outside payload");
            }
            const auto begin = payload_.begin() + static_cast<std::ptrdiff_t>(request.byte_offset);
            const auto end = begin + static_cast<std::ptrdiff_t>(request.byte_size);
            const auto row_byte_size = mismatch_page_row_bytes && after_prefix_call && request.byte_size > 0U
                                           ? static_cast<std::size_t>(request.byte_size - 1U)
                                           : static_cast<std::size_t>(end - begin);
            rows.push_back(mirakana::ByteRangeIoReadRow{
                .path = std::string(request.path),
                .byte_offset = request.byte_offset,
                .byte_size = request.byte_size,
                .bytes = byte_copy(std::string_view(&*begin, row_byte_size)),
            });
        }
        return rows;
    }

    std::vector<DirectStorageReadCall> calls;
    bool fail_after_prefix{false};
    bool mismatch_page_row_bytes{false};

  private:
    std::string payload_path_;
    std::string payload_;
    bool available_{true};
};

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph(const std::string& payload) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/cathedral_cluster_graph");
    const auto source_mesh = mirakana::AssetId::from_name("source/cathedral_high");
    const auto material = mirakana::AssetId::from_name("materials/stone");
    const auto page0 = payload.find("page0-cluster-bytes");
    const auto page1 = payload.find("page1-cluster-bytes");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/meshes/cathedral_high.mesh",
        .cluster_payload_uri = "runtime/mavg/cathedral_cluster_pages.mavgpayload",
        .target_cluster_triangles = 64,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0,
                    .byte_offset = static_cast<std::uint64_t>(page0),
                    .byte_size = 19,
                    .first_cluster = 0,
                    .cluster_count = 1,
                },
                mirakana::MavgClusterGraphPage{
                    .page_index = 1,
                    .byte_offset = static_cast<std::uint64_t>(page1),
                    .byte_size = 19,
                    .first_cluster = 1,
                    .cluster_count = 1,
                },
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material,
                    .first_cluster = 0,
                    .cluster_count = 2,
                },
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 64,
                    .vertex_count = 96,
                    .bounds = {.min = {.x = -1.0F, .y = -1.0F, .z = -1.0F}, .max = {.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 4.0F,
                    .first_index = 0,
                    .index_count = 192,
                    .vertex_base = 0,
                    .children = {1},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 32,
                    .vertex_count = 48,
                    .bounds = {.min = {.x = 0.0F, .y = 0.0F, .z = 0.0F}, .max = {.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 192,
                    .index_count = 96,
                    .vertex_base = 96,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] bool
has_diagnostic_code(const std::vector<mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnostic>& diagnostics,
                    mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime mavg payload page loader copies requested byte ranges without side effects") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    const std::array<std::uint32_t, 2> requested_pages{1, 0};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.loaded_pages.size() == 2);
    MK_REQUIRE(result.loaded_pages[0].page_index == 1);
    MK_REQUIRE(result.loaded_pages[0].bytes == byte_copy("page1-cluster-bytes"));
    MK_REQUIRE(result.loaded_pages[1].page_index == 0);
    MK_REQUIRE(result.loaded_pages[1].bytes == byte_copy("page0-cluster-bytes"));
    MK_REQUIRE(result.requested_page_count == 2);
    MK_REQUIRE(result.loaded_page_count == 2);
    MK_REQUIRE(result.payload_byte_count == payload.size());
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.executed_direct_storage);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page loader rejects duplicate missing and out of bounds pages") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    auto graph = make_graph(payload);
    graph.pages[1].byte_offset = static_cast<std::uint64_t>(payload.size() + 1U);
    const std::array<std::uint32_t, 3> requested_pages{0, 0, 1};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::duplicate_page_request));
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg payload page loader rejects invalid payload format") {
    const std::string payload = "format=GameEngine.UnknownPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result =
        mirakana::runtime::load_runtime_mavg_payload_pages(mirakana::runtime::RuntimeMavgPayloadPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .payload_bytes = bytes_view(payload),
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format));
}

MK_TEST("runtime mavg filesystem payload page loader reads only format prefix and requested byte ranges") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingRangeFileSystem fs(graph.cluster_payload_uri, payload);
    const std::array<std::uint32_t, 2> requested_pages{1, 0};
    const auto expected_prefix_byte_count = std::string("format=GameEngine.MavgClusterPayload.v1\n").size();

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_filesystem(
        mirakana::runtime::RuntimeMavgPayloadFilesystemPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .filesystem = &fs,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.executed_direct_storage);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(result.loaded_pages.size() == 2);
    MK_REQUIRE(result.loaded_pages[0].page_index == 1);
    MK_REQUIRE(result.loaded_pages[0].bytes == byte_copy("page1-cluster-bytes"));
    MK_REQUIRE(result.loaded_pages[1].page_index == 0);
    MK_REQUIRE(result.loaded_pages[1].bytes == byte_copy("page0-cluster-bytes"));
    MK_REQUIRE(result.filesystem_read_byte_count == expected_prefix_byte_count + 19U + 19U);
    MK_REQUIRE(fs.read_text_call_count == 0);
    MK_REQUIRE(fs.range_reads.size() == 3);
    MK_REQUIRE(fs.range_reads[0].byte_offset == 0);
    MK_REQUIRE(fs.range_reads[0].byte_size == expected_prefix_byte_count);
    MK_REQUIRE(fs.range_reads[1].byte_offset == graph.pages[1].byte_offset);
    MK_REQUIRE(fs.range_reads[1].byte_size == graph.pages[1].byte_size);
    MK_REQUIRE(fs.range_reads[2].byte_offset == graph.pages[0].byte_offset);
    MK_REQUIRE(fs.range_reads[2].byte_size == graph.pages[0].byte_size);
}

MK_TEST("runtime mavg filesystem payload page loader rejects range failures without partial rows") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\n";
    auto graph = make_graph(payload + "page1-cluster-bytes\n");
    graph.pages[1].byte_offset = static_cast<std::uint64_t>(payload.size() + 64U);
    RecordingRangeFileSystem fs(graph.cluster_payload_uri, payload);
    const std::array<std::uint32_t, 1> requested_pages{1};

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_filesystem(
        mirakana::runtime::RuntimeMavgPayloadFilesystemPageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .filesystem = &fs,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds));
    MK_REQUIRE(result.invoked_file_io);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg directstorage payload page loader executes reviewed byte ranges without public native handles") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingDirectStorageExecutor direct_storage(graph.cluster_payload_uri, payload);
    const std::array<std::uint32_t, 2> requested_pages{1, 0};
    const auto expected_prefix_byte_count = std::string("format=GameEngine.MavgClusterPayload.v1\n").size();

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_direct_storage(
        mirakana::runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .direct_storage = &direct_storage,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.loaded_pages.size() == 2);
    MK_REQUIRE(result.loaded_pages[0].page_index == 1);
    MK_REQUIRE(result.loaded_pages[0].bytes == byte_copy("page1-cluster-bytes"));
    MK_REQUIRE(result.loaded_pages[1].page_index == 0);
    MK_REQUIRE(result.loaded_pages[1].bytes == byte_copy("page0-cluster-bytes"));
    MK_REQUIRE(result.requested_page_count == 2);
    MK_REQUIRE(result.loaded_page_count == 2);
    MK_REQUIRE(result.payload_byte_count == expected_prefix_byte_count + 19U + 19U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(result.executed_direct_storage);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(direct_storage.calls.size() == 3);
    MK_REQUIRE(direct_storage.calls[0].byte_offset == 0);
    MK_REQUIRE(direct_storage.calls[0].byte_size == expected_prefix_byte_count);
    MK_REQUIRE(direct_storage.calls[1].byte_offset == graph.pages[1].byte_offset);
    MK_REQUIRE(direct_storage.calls[1].byte_size == graph.pages[1].byte_size);
    MK_REQUIRE(direct_storage.calls[2].byte_offset == graph.pages[0].byte_offset);
    MK_REQUIRE(direct_storage.calls[2].byte_size == graph.pages[0].byte_size);
}

MK_TEST("runtime mavg directstorage payload page loader fails closed when unavailable") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingDirectStorageExecutor direct_storage(graph.cluster_payload_uri, payload, false);
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_direct_storage(
        mirakana::runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .direct_storage = &direct_storage,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_unavailable));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.executed_direct_storage);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(direct_storage.calls.empty());
}

MK_TEST("runtime mavg directstorage payload page loader rejects invalid payload format before page reads") {
    const std::string payload = "format=GameEngine.UnknownPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingDirectStorageExecutor direct_storage(graph.cluster_payload_uri, payload);
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_direct_storage(
        mirakana::runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .direct_storage = &direct_storage,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics, mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format));
    MK_REQUIRE(result.executed_direct_storage);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(direct_storage.calls.size() == 1);
    MK_REQUIRE(direct_storage.calls[0].byte_offset == 0);
}

MK_TEST("runtime mavg directstorage payload page loader rejects page read failures without partial rows") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingDirectStorageExecutor direct_storage(graph.cluster_payload_uri, payload);
    direct_storage.fail_after_prefix = true;
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_direct_storage(
        mirakana::runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .direct_storage = &direct_storage,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics,
        mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_page_read_failed));
    MK_REQUIRE(result.executed_direct_storage);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(direct_storage.calls.size() == 1);
}

MK_TEST("runtime mavg directstorage payload page loader rejects mismatched result rows without partial rows") {
    const std::string payload = "format=GameEngine.MavgClusterPayload.v1\npage0-cluster-bytes\npage1-cluster-bytes\n";
    const auto graph = make_graph(payload);
    RecordingDirectStorageExecutor direct_storage(graph.cluster_payload_uri, payload);
    direct_storage.mismatch_page_row_bytes = true;
    const std::array<std::uint32_t, 1> requested_pages{0};

    const auto result = mirakana::runtime::load_runtime_mavg_payload_pages_from_direct_storage(
        mirakana::runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .direct_storage = &direct_storage,
            .payload_path = graph.cluster_payload_uri,
            .page_indices = requested_pages,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.loaded_pages.empty());
    MK_REQUIRE(has_diagnostic_code(
        result.diagnostics,
        mirakana::runtime::RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_result_mismatch));
    MK_REQUIRE(result.executed_direct_storage);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.touched_gpu_memory_policy);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(direct_storage.calls.size() == 2);
}

int main() {
    return mirakana::test::run_all();
}
