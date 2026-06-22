// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/platform/byte_range_io.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::vector<std::byte> byte_copy(std::string_view text) {
    const auto bytes = std::as_bytes(std::span<const char>(text.data(), text.size()));
    return std::vector<std::byte>(bytes.begin(), bytes.end());
}

[[nodiscard]] std::string make_payload_text() {
    return "format=" + std::string(mirakana::mavg_cluster_payload_format_v1()) +
           "\nroot-page-bytes----page1-cluster-bytespage2-cluster-bytes";
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph(std::string_view payload) {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/autonomous-streaming/graph");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/autonomous-streaming-source");
    const auto material = mirakana::AssetId::from_name("materials/autonomous-streaming-material");
    const auto page0 = payload.find("root-page-bytes----");
    const auto page1 = payload.find("page1-cluster-bytes");
    const auto page2 = payload.find("page2-cluster-bytes");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/autonomous-streaming.glb",
        .cluster_payload_uri = "runtime/mavg/autonomous-streaming.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 64,
        .pages =
            {
                mirakana::MavgClusterGraphPage{.page_index = 0,
                                               .byte_offset = static_cast<std::uint64_t>(page0),
                                               .byte_size = 19,
                                               .first_cluster = 0,
                                               .cluster_count = 1},
                mirakana::MavgClusterGraphPage{.page_index = 1,
                                               .byte_offset = static_cast<std::uint64_t>(page1),
                                               .byte_size = 19,
                                               .first_cluster = 1,
                                               .cluster_count = 1},
                mirakana::MavgClusterGraphPage{.page_index = 2,
                                               .byte_offset = static_cast<std::uint64_t>(page2),
                                               .byte_size = 19,
                                               .first_cluster = 2,
                                               .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 3},
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
                    .children = {1, 2},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = 0.0F},
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
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 9,
                    .index_count = 3,
                    .vertex_base = 7,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPageStreamingCandidateRow make_candidate(mirakana::AssetId graph_asset,
                                                                                     std::uint32_t page_index) {
    return mirakana::runtime::RuntimeMavgPageStreamingCandidateRow{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = "runtime/mavg/autonomous/page" + std::to_string(page_index) + ".geindex",
                .content_root = "runtime/mavg/autonomous",
                .label = "mavg-autonomous-page-" + std::to_string(page_index),
            },
    };
}

void write_page_package(mirakana::MemoryFileSystem& filesystem, std::uint32_t page_index, mirakana::AssetId asset,
                        std::string_view content) {
    const auto package_path = "page" + std::to_string(page_index) + ".payload";
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = mirakana::AssetKind::mesh,
                                                                      .path = package_path,
                                                                      .content = std::string(content),
                                                                      .source_revision = page_index + 1U,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/mavg/autonomous/page" + std::to_string(page_index) + ".geindex",
                          mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/mavg/autonomous/" + package_path, content);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::AssetId asset, std::uint32_t handle_value,
                                                                  std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = handle_value},
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/autonomous/resident-" + std::to_string(handle_value) + ".geasset",
        .content_hash = asset.value + handle_value,
        .source_revision = handle_value,
        .dependencies = {},
        .content = std::string(content),
    }});
}

void mount_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                mirakana::AssetId asset, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-autonomous-resident-" + std::to_string(mount_id),
                       .package = make_package(asset, mount_id, content),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::GpuMemoryPolicyPlan make_gpu_memory_policy(std::uint64_t target_bytes) {
    const std::vector<mirakana::GpuMemoryRequestDesc> requests{
        mirakana::GpuMemoryRequestDesc{.residency = mirakana::GpuMemoryResidencyClass::placed,
                                       .requested_bytes = target_bytes,
                                       .require_declared_budget_evidence = true,
                                       .require_residency_pressure_evidence = true,
                                       .require_package_counter_evidence = true,
                                       .source_index = 7}};
    return mirakana::plan_gpu_memory_policy(mirakana::GpuMemoryPolicyDesc{
        .requests = requests,
        .declared_local_budget_bytes = 64,
        .os_video_memory_budget_available = true,
        .os_local_budget_bytes = 64,
        .os_local_usage_bytes = 48,
        .residency_pressure_event_count = 1,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .package_counter_evidence_ready = true,
    });
}

[[nodiscard]] mirakana::MavgLodViewDesc near_view(float camera_x = -10.0F) {
    return mirakana::MavgLodViewDesc{
        .camera_world_position = mirakana::Vec3{.x = camera_x, .y = 0.0F, .z = 0.0F},
        .viewport_height_pixels = 1080.0F,
        .target_error_pixels = 1.0F,
    };
}

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
        if (throw_on_read) {
            throw std::runtime_error("DirectStorage scheduler test failure");
        }
        std::vector<mirakana::ByteRangeIoReadRow> rows;
        for (const auto& request : requests) {
            calls.push_back(request.byte_offset);
            if (request.path != payload_path_) {
                throw std::runtime_error("unexpected DirectStorage payload path");
            }
            if (request.byte_offset > payload_.size() || request.byte_size > payload_.size() - request.byte_offset) {
                throw std::out_of_range("DirectStorage range outside payload");
            }
            const auto begin = payload_.begin() + static_cast<std::ptrdiff_t>(request.byte_offset);
            const auto end = begin + static_cast<std::ptrdiff_t>(request.byte_size);
            rows.push_back(mirakana::ByteRangeIoReadRow{
                .path = std::string(request.path),
                .byte_offset = request.byte_offset,
                .byte_size = request.byte_size,
                .bytes = byte_copy(std::string_view(&*begin, static_cast<std::size_t>(end - begin))),
            });
        }
        return rows;
    }

    std::vector<std::uint64_t> calls;
    bool throw_on_read{false};

  private:
    std::string payload_path_;
    std::string payload_;
    bool available_{true};
};

struct SchedulerFixture {
    std::string payload{make_payload_text()};
    mirakana::MavgClusterGraphDocument graph{make_graph(payload)};
    mirakana::MemoryFileSystem filesystem;
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    mirakana::JobExecutionPool execution_pool{mirakana::JobExecutionPoolDesc{
        .name = "runtime_rhi.mavg.autonomous_streaming_scheduler",
        .logical_processor_count = 2,
        .worker_count = 2,
        .queue_capacity_per_worker = 4,
        .scratch_budget_bytes_per_worker = 512,
        .frame_index = 11,
    }};
    std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_candidate(graph.asset, 1),
        make_candidate(graph.asset, 2),
    };
    std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
    };
    mirakana::MavgLodResidentPageSet resident_pages{.page_indices = {0}};
    mirakana::GpuMemoryPolicyPlan gpu_policy{make_gpu_memory_policy(18)};

    SchedulerFixture() {
        filesystem.write_text(graph.cluster_payload_uri, payload);
        write_page_package(filesystem, 1, mirakana::AssetId::from_name("mavg/autonomous/page-1"), "page-one");
        write_page_package(filesystem, 2, mirakana::AssetId::from_name("mavg/autonomous/page-2"), "page-two");
        mount_page(mount_set, 10, mirakana::AssetId::from_name("mavg/autonomous/root"), "root-page");
        mount_page(mount_set, 12, mirakana::AssetId::from_name("mavg/autonomous/stale"), "stale-page-large");
        MK_REQUIRE(catalog_cache
                       .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                                mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                       .succeeded());
    }
};

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerDesc
make_desc(SchedulerFixture& fixture, const mirakana::MavgLodViewDesc& view) {
    return mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerDesc{
        .graph_asset = fixture.graph.asset,
        .graph = &fixture.graph,
        .view =
            mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerViewState{
                .lod_view = view,
            },
        .resident_pages = &fixture.resident_pages,
        .candidates = fixture.candidates,
        .filesystem = &fixture.filesystem,
        .execution_pool = &fixture.execution_pool,
        .resident_page_mounts = fixture.page_mounts,
        .gpu_memory_policy = &fixture.gpu_policy,
        .payload_path = fixture.graph.cluster_payload_uri,
        .max_queued_pages = 2,
        .max_pending_pages = 2,
        .max_dispatch_pages = 1,
        .max_adopt_pages = 1,
        .frame_index = 11,
        .safe_point_policy = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSafePointPolicy::adopt_when_loaded,
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg autonomous streaming scheduler selects dispatches adopts and evicts without page requests") {
    SchedulerFixture fixture;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 20,
    };
    const auto view = near_view();
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, make_desc(fixture, view));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(result.selected_without_caller_precomputed_requests);
    MK_REQUIRE(result.selected_page_request_count > 0U);
    MK_REQUIRE(result.dispatched_page_count == 1U);
    MK_REQUIRE(result.adopted_page_count == 1U);
    MK_REQUIRE(result.eviction_candidate_count >= 1U);
    MK_REQUIRE(result.applied_gpu_memory_pressure_policy);
    MK_REQUIRE(result.executed_filesystem_payload_io);
    MK_REQUIRE(!result.executed_direct_storage_payload_io);
    MK_REQUIRE(result.executed_background_worker);
    MK_REQUIRE(result.mutated_mount_set);
    MK_REQUIRE(result.invoked_catalog_refresh);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(!result.exposed_native_handles);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(fixture.mount_set.generation() > previous_generation);
    MK_REQUIRE(state.next_mount_id == 21U);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler keeps pending rows and coalesces duplicates across frames") {
    SchedulerFixture fixture;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 30,
    };
    const auto view = near_view();
    auto desc = make_desc(fixture, view);
    desc.max_dispatch_pages = 0;
    desc.safe_point_policy = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSafePointPolicy::defer;

    const auto first = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, desc);
    const auto second = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, desc);

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(!first.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(first.pending_page_count_after > 0U);
    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(second.duplicate_pending_page_count > 0U);
    MK_REQUIRE(second.pending_page_count_after == first.pending_page_count_after);
    MK_REQUIRE(second.bounded_per_frame_work);
    MK_REQUIRE(!second.mutated_mount_set);
    MK_REQUIRE(!second.invoked_catalog_refresh);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler responds to camera movement and page heat priority") {
    SchedulerFixture fixture;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 35,
    };
    std::vector<mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerPageHeatRow> heat_rows{
        mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerPageHeatRow{
            .graph_asset = fixture.graph.asset,
            .page_index = 1,
            .heat = 1000.0F,
            .last_used_frame = 12,
        },
    };

    auto far_desc = make_desc(fixture, near_view(-10000.0F));
    far_desc.safe_point_policy = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSafePointPolicy::defer;
    const auto far = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, far_desc);

    auto near_desc = make_desc(fixture, near_view());
    near_desc.safe_point_policy = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSafePointPolicy::defer;
    near_desc.page_heat = heat_rows;
    const auto near = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, near_desc);

    MK_REQUIRE(far.succeeded());
    MK_REQUIRE(far.selected_page_request_count == 0U);
    MK_REQUIRE(near.succeeded());
    MK_REQUIRE(near.selected_page_request_count > 0U);
    MK_REQUIRE(near.dispatched_page_count == 1U);
    MK_REQUIRE(near.background_service.loaded_rows.size() == 1U);
    MK_REQUIRE(near.background_service.loaded_rows[0].row.page_index == 1U);
    MK_REQUIRE(!near.mutated_mount_set);
    MK_REQUIRE(!near.mavg_autonomous_streaming_scheduler_ready);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler routes payload reads through directstorage executor") {
    SchedulerFixture fixture;
    RecordingDirectStorageExecutor direct_storage(fixture.graph.cluster_payload_uri, fixture.payload);
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 40,
    };
    const auto view = near_view();
    auto desc = make_desc(fixture, view);
    desc.io_backend = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingIoBackendKind::direct_storage_byte_range;
    desc.direct_storage = &direct_storage;
    desc.safe_point_policy = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSafePointPolicy::defer;

    const auto result = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.used_direct_storage_backend_row);
    MK_REQUIRE(result.executed_direct_storage_payload_io);
    MK_REQUIRE(!result.executed_filesystem_payload_io);
    MK_REQUIRE(!direct_storage.calls.empty());
    MK_REQUIRE(!result.exposed_native_handles);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler fails closed on directstorage executor failure") {
    SchedulerFixture fixture;
    RecordingDirectStorageExecutor direct_storage(fixture.graph.cluster_payload_uri, fixture.payload);
    direct_storage.throw_on_read = true;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 50,
    };
    const auto view = near_view();
    auto desc = make_desc(fixture, view);
    desc.io_backend = mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingIoBackendKind::direct_storage_byte_range;
    desc.direct_storage = &direct_storage;
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::payload_page_load_failed));
    MK_REQUIRE(result.executed_direct_storage_payload_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(fixture.mount_set.generation() == previous_generation);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler cancels selected pages before io") {
    SchedulerFixture fixture;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 60,
    };
    const auto view = near_view();
    std::vector<std::uint32_t> cancelled{1, 2};
    auto desc = make_desc(fixture, view);
    desc.cancelled_page_indices = cancelled;

    const auto result = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(!result.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(result.cancelled_page_count >= 1U);
    MK_REQUIRE(result.queued_page_count == 0U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.executed_background_worker);
    MK_REQUIRE(!result.mutated_mount_set);
}

MK_TEST("runtime rhi mavg autonomous streaming scheduler preserves safe point atomicity on invalid mount rows") {
    SchedulerFixture fixture;
    mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerState state{
        .graph_asset = fixture.graph.asset,
        .next_mount_id = 0,
    };
    const auto view = near_view();
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::runtime_rhi::tick_runtime_mavg_autonomous_streaming_scheduler(
        fixture.mount_set, fixture.catalog_cache, state, make_desc(fixture, view));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_autonomous_streaming_scheduler_ready);
    MK_REQUIRE(has_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::safe_point_adoption_failed));
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(fixture.mount_set.generation() == previous_generation);
    MK_REQUIRE(state.next_mount_id == 0U);
}

int main() {
    return mirakana::test::run_all();
}
