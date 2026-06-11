// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset,
                                                                mirakana::runtime::RuntimeAssetHandle handle,
                                                                mirakana::AssetKind kind = mirakana::AssetKind::mesh,
                                                                std::string content = "streamed-page") {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "runtime/mavg/streamed/" + std::to_string(handle.value) + ".geasset",
        .content_hash = asset.value + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

void mount_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-streamed-page-" + std::to_string(mount_id),
                       .package = mirakana::runtime::RuntimeAssetPackage({make_record(asset, handle)}),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 cache;
    MK_REQUIRE(cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                   .succeeded());
    return cache;
}

[[nodiscard]] mirakana::runtime::RuntimeAssetHandle
resident_package_handle(const mirakana::runtime::RuntimeResourceCatalogV2& catalog, mirakana::AssetId asset) {
    const auto handle = mirakana::runtime::find_runtime_resource_v2(catalog, asset);
    MK_REQUIRE(handle.has_value());
    const auto* const record = mirakana::runtime::runtime_resource_record_v2(catalog, *handle);
    MK_REQUIRE(record != nullptr);
    return record->package_handle;
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streamed-upload/graph");
    const auto source_mesh = mirakana::AssetId::from_name("mavg/streamed-upload/source");
    const auto material = mirakana::AssetId::from_name("mavg/streamed-upload/material");
    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/streamed.gltf",
        .cluster_payload_uri = "runtime/streamed.mavgpayload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 256, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 256, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 512, .byte_size = 256, .first_cluster = 2, .cluster_count = 1},
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
                    .vertex_base = 12,
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
                    .vertex_base = 24,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMeshPayload make_payload(mirakana::AssetId page_asset,
                                                                 mirakana::runtime::RuntimeAssetHandle handle,
                                                                 std::uint32_t index_count = 9) {
    return mirakana::runtime::RuntimeMeshPayload{
        .asset = page_asset,
        .handle = handle,
        .vertex_count = 16,
        .index_count = index_count,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(16U * 12U, std::uint8_t{0x61}),
        .index_bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(index_count) * 4U, std::uint8_t{0x00}),
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult
make_adoption(mirakana::AssetId graph_asset, bool committed = true) {
    mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult adoption;
    adoption.adopted_rows.push_back(mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionRow{
        .graph_asset = graph_asset,
        .page_index = 1,
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11},
        .resident_mount =
            mirakana::runtime::RuntimeResidentPackageMountResultV2{
                .status = mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted,
            },
    });
    adoption.adopted_page_count = adoption.adopted_rows.size();
    adoption.mounted_package_count = 1;
    adoption.committed = committed;
    adoption.mutated_mount_set = committed;
    return adoption;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult
make_two_page_adoption(mirakana::AssetId graph_asset) {
    auto adoption = make_adoption(graph_asset);
    adoption.adopted_rows.push_back(mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionRow{
        .graph_asset = graph_asset,
        .page_index = 2,
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
        .resident_mount =
            mirakana::runtime::RuntimeResidentPackageMountResultV2{
                .status = mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted,
            },
    });
    adoption.adopted_page_count = adoption.adopted_rows.size();
    adoption.mounted_package_count = adoption.adopted_rows.size();
    return adoption;
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

void require_succeeded(const mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult& result) {
    if (result.succeeded()) {
        return;
    }
    if (result.diagnostics.empty()) {
        throw std::runtime_error("streamed cluster upload failed without diagnostics");
    }
    throw std::runtime_error(result.diagnostics.front().message);
}

} // namespace

MK_TEST("mavg streamed cluster gpu upload publishes page mesh bindings after committed safe point adoption") {
    const auto graph = make_graph();
    MK_REQUIRE(mirakana::validate_mavg_cluster_graph(graph).valid());
    const auto page_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto page_handle = mirakana::runtime::RuntimeAssetHandle{.value = 21};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset, page_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset);
    const auto payload = make_payload(page_asset, resident_package_handle(cache.catalog(), page_asset));
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    require_succeeded(upload);
    MK_REQUIRE(upload.package_visible);
    MK_REQUIRE(upload.streamed_cluster_pages_ready);
    MK_REQUIRE(upload.uploaded_page_count == 1U);
    MK_REQUIRE(upload.uploaded_cluster_count == 1U);
    MK_REQUIRE(upload.uploaded_bytes == payload.vertex_bytes.size() + payload.index_bytes.size());
    MK_REQUIRE(upload.page_bindings.size() == 1U);
    MK_REQUIRE(upload.page_bindings[0].graph_asset == graph.asset);
    MK_REQUIRE(upload.page_bindings[0].page_index == 1U);
    MK_REQUIRE(upload.page_bindings[0].page_asset == page_asset);
    MK_REQUIRE(upload.page_bindings[0].binding.owner_device == &device);
    MK_REQUIRE(upload.page_bindings[0].binding.vertex_count == payload.vertex_count);
    MK_REQUIRE(upload.page_bindings[0].binding.index_count == payload.index_count);
    MK_REQUIRE(upload.invoked_gpu_upload);
    MK_REQUIRE(!upload.invoked_candidate_load);
    MK_REQUIRE(!upload.mutated_streaming_state);
    MK_REQUIRE(!upload.invoked_direct_storage);
    MK_REQUIRE(!upload.executed_backend);
    MK_REQUIRE(!upload.executed_mesh_shader);
    MK_REQUIRE(!upload.touched_native_handles);
    MK_REQUIRE(!upload.proved_async_overlap_performance);
}

MK_TEST("mavg streamed cluster gpu upload rejects uncommitted safe point adoption before gpu upload") {
    const auto graph = make_graph();
    const auto page_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto page_handle = mirakana::runtime::RuntimeAssetHandle{.value = 22};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset, page_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset, false);
    const auto payload = make_payload(page_asset, resident_package_handle(cache.catalog(), page_asset));
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::adoption_not_committed));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("mavg streamed cluster gpu upload rejects graph asset mismatch before gpu upload") {
    const auto graph = make_graph();
    const auto page_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto page_handle = mirakana::runtime::RuntimeAssetHandle{.value = 25};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset, page_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset);
    const auto payload = make_payload(page_asset, resident_package_handle(cache.catalog(), page_asset));
    const auto mismatched_graph_asset = mirakana::AssetId::from_name("mavg/streamed-upload/other-graph");
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = mismatched_graph_asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::graph_asset_mismatch));
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::page_payload_mismatch));
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::adopted_row_mismatch));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("mavg streamed cluster gpu upload rejects resident handle mismatch before gpu upload") {
    const auto graph = make_graph();
    const auto page_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto catalog_handle = mirakana::runtime::RuntimeAssetHandle{.value = 26};
    const auto stale_payload_handle = mirakana::runtime::RuntimeAssetHandle{.value = 27};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset, catalog_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset);
    const auto payload = make_payload(page_asset, stale_payload_handle);
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::payload_handle_mismatch));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("mavg streamed cluster gpu upload rejects page payloads missing from resident catalog") {
    const auto graph = make_graph();
    const auto resident_asset = mirakana::AssetId::from_name("mavg/streamed-upload/resident-page1");
    const auto payload_asset = mirakana::AssetId::from_name("mavg/streamed-upload/not-resident-page1");
    const auto page_handle = mirakana::runtime::RuntimeAssetHandle{.value = 23};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, resident_asset, page_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset);
    const auto payload = make_payload(payload_asset, page_handle);
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::page_not_resident));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("mavg streamed cluster gpu upload does not publish partial bindings when a later page is invalid") {
    const auto graph = make_graph();
    const auto page1_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto page2_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page2");
    const auto page1_handle = mirakana::runtime::RuntimeAssetHandle{.value = 28};
    const auto page2_handle = mirakana::runtime::RuntimeAssetHandle{.value = 29};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page1_asset, page1_handle);
    mount_page(mount_set, 12, page2_asset, page2_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_two_page_adoption(graph.asset);
    const auto page1_payload = make_payload(page1_asset, resident_package_handle(cache.catalog(), page1_asset));
    auto page2_payload = make_payload(page2_asset, resident_package_handle(cache.catalog(), page2_asset), 12);
    page2_payload.vertex_count = 32;
    page2_payload.vertex_bytes = std::vector<std::uint8_t>(32U * 12U, std::uint8_t{0x62});
    page2_payload.index_bytes.pop_back();
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = page1_payload,
        },
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 2,
            .payload = page2_payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::mesh_payload_invalid));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(upload.page_bindings.empty());
    MK_REQUIRE(upload.uploaded_page_count == 0U);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("mavg streamed cluster gpu upload rejects cluster draw ranges outside page payload") {
    const auto graph = make_graph();
    const auto page_asset = mirakana::AssetId::from_name("mavg/streamed-upload/page1");
    const auto page_handle = mirakana::runtime::RuntimeAssetHandle{.value = 24};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset, page_handle);
    const auto cache = make_cache(mount_set);
    const auto adoption = make_adoption(graph.asset);
    const auto payload = make_payload(page_asset, resident_package_handle(cache.catalog(), page_asset), 8);
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = payload,
        },
    };
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(has_diagnostic(
        upload,
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::cluster_range_outside_payload));
    MK_REQUIRE(!upload.invoked_gpu_upload);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

int main() {
    return mirakana::test::run_all();
}
