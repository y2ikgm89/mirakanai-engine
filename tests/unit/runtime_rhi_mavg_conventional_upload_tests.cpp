// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_conventional_upload.hpp"
#include "mirakana/scene_renderer/mavg_scene_lod.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_mavg_graph_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle,
                       mirakana::AssetKind kind = mirakana::AssetKind::mavg_cluster_graph) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "mavg/" + std::to_string(asset.value) + ".mavg_graph",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "mavg_cluster_graph",
    };
}

[[nodiscard]] mirakana::runtime::RuntimeResourceCatalogV2
make_runtime_catalog(std::vector<mirakana::runtime::RuntimeAssetRecord> records) {
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2(
        catalog, mirakana::runtime::RuntimeAssetPackage{std::move(records)});
    MK_REQUIRE(build.succeeded());
    return catalog;
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionResult make_committed_streaming_result() {
    mirakana::runtime::RuntimePackageStreamingExecutionResult result;
    result.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed;
    result.target_id = "mavg-runtime-lod";
    result.package_index_path = "runtime/mavg_lod.geindex";
    result.runtime_scene_validation_target_id = "mavg-runtime-lod";
    result.committed = true;
    return result;
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_mavg_upload_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/runtime/hero");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/runtime/hero-source");
    const auto material_body = mirakana::AssetId::from_name("materials/runtime/body");
    const auto material_trim = mirakana::AssetId::from_name("materials/runtime/trim");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/hero.gltf",
        .cluster_payload_uri = "runtime/hero.mavg_payload",
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
                    .material = material_body, .first_cluster = 0, .cluster_count = 2},
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_trim, .first_cluster = 2, .cluster_count = 1},
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
                    .material_partition = 1,
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

[[nodiscard]] mirakana::runtime::RuntimeMeshPayload
make_mavg_runtime_mesh_payload(mirakana::AssetId graph_asset, mirakana::runtime::RuntimeAssetHandle handle,
                               std::uint32_t index_count = 12) {
    return mirakana::runtime::RuntimeMeshPayload{
        .asset = graph_asset,
        .handle = handle,
        .vertex_count = 32,
        .index_count = index_count,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(32U * 12U, std::uint8_t{0x61}),
        .index_bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(index_count) * 4U, std::uint8_t{0x00}),
    };
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_mavg_upload_selection(mirakana::AssetId graph_asset) {
    return mirakana::MavgLodSelectionResult{
        .selected_clusters =
            {
                mirakana::MavgLodSelectedCluster{.graph_asset = graph_asset,
                                                 .cluster_index = 1,
                                                 .page_index = 1,
                                                 .lod_level = 1,
                                                 .material_partition = 0,
                                                 .first_index = 6,
                                                 .index_count = 3,
                                                 .vertex_base = 12,
                                                 .fallback_substitution = false},
                mirakana::MavgLodSelectedCluster{.graph_asset = graph_asset,
                                                 .cluster_index = 2,
                                                 .page_index = 2,
                                                 .lod_level = 1,
                                                 .material_partition = 1,
                                                 .first_index = 9,
                                                 .index_count = 3,
                                                 .vertex_base = 24,
                                                 .fallback_substitution = true},
            },
        .page_requests = {},
        .diagnostics = {},
        .fallback_substitution_count = 1,
        .missing_page_count = 0,
        .budget_degraded = false,
    };
}

[[nodiscard]] mirakana::MaterialGpuBinding make_material_binding(const mirakana::rhi::IRhiDevice& device,
                                                                 std::uint32_t base) noexcept {
    return mirakana::MaterialGpuBinding{
        .pipeline_layout = mirakana::rhi::PipelineLayoutHandle{base},
        .descriptor_set = mirakana::rhi::DescriptorSetHandle{base + 1U},
        .descriptor_set_index = 0,
        .owner_device = &device,
    };
}

} // namespace

MK_TEST("mavg conventional upload publishes package visible mesh binding for scene lod planning") {
    const auto graph = make_mavg_upload_graph();
    const auto validation = mirakana::validate_mavg_cluster_graph(graph);
    MK_REQUIRE(validation.valid());
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 7};
    const auto catalog = make_runtime_catalog({make_mavg_graph_record(graph.asset, handle)});
    const auto streaming = make_committed_streaming_result();
    const auto payload = make_mavg_runtime_mesh_payload(graph.asset, handle);
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_conventional_mesh_binding(
        device, mirakana::runtime_rhi::RuntimeMavgConventionalMeshUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .streaming_result = &streaming,
                    .resident_catalog = &catalog,
                    .payload = &payload,
                });

    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.package_visible);
    MK_REQUIRE(upload.conventional_mesh_ready);
    MK_REQUIRE(!upload.executed_gpu_culling);
    MK_REQUIRE(!upload.executed_indirect_draw);
    MK_REQUIRE(!upload.executed_mesh_shader);
    MK_REQUIRE(!upload.touched_native_handles);
    MK_REQUIRE(upload.graph_page_count == graph.pages.size());
    MK_REQUIRE(upload.graph_cluster_count == graph.clusters.size());
    MK_REQUIRE(upload.payload_vertex_count == payload.vertex_count);
    MK_REQUIRE(upload.payload_index_count == payload.index_count);
    MK_REQUIRE(upload.uploaded_bytes == payload.vertex_bytes.size() + payload.index_bytes.size());
    MK_REQUIRE(upload.submitted_fences.size() == 1);
    MK_REQUIRE(upload.mesh_binding.graph_asset == graph.asset);
    MK_REQUIRE(upload.mesh_binding.binding.owner_device == &device);
    MK_REQUIRE(upload.mesh_binding.binding.vertex_count == payload.vertex_count);
    MK_REQUIRE(upload.mesh_binding.binding.index_count == payload.index_count);

    const auto scene_mesh_binding = mirakana::MavgSceneLodMeshBinding{.graph_asset = upload.mesh_binding.graph_asset,
                                                                      .mesh_binding = upload.mesh_binding.binding};
    const std::vector<mirakana::MavgSceneLodMaterialBinding> materials{
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[0].material,
                                              .material_binding = make_material_binding(device, 201)},
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[1].material,
                                              .material_binding = make_material_binding(device, 301)},
    };

    const auto plan = mirakana::plan_mavg_scene_lod_mesh_commands(
        make_mavg_upload_selection(graph.asset), graph,
        mirakana::MavgSceneLodSubmitDesc{
            .graph_asset = graph.asset,
            .mesh_binding = &scene_mesh_binding,
            .material_bindings = std::span<const mirakana::MavgSceneLodMaterialBinding>{materials},
        });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.mesh_commands.size() == 2);
    MK_REQUIRE(plan.mesh_commands[0].mesh_binding.index_buffer.value == upload.mesh_binding.binding.index_buffer.value);
    MK_REQUIRE(plan.mesh_commands[0].indexed_range.first_index == 6);
    MK_REQUIRE(plan.mesh_commands[1].indexed_range.first_index == 9);
}

MK_TEST("mavg conventional upload rejects non committed streaming before creating gpu buffers") {
    const auto graph = make_mavg_upload_graph();
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 8};
    const auto catalog = make_runtime_catalog({make_mavg_graph_record(graph.asset, handle)});
    const auto payload = make_mavg_runtime_mesh_payload(graph.asset, handle);
    mirakana::runtime::RuntimePackageStreamingExecutionResult streaming;
    streaming.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required;
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_conventional_mesh_binding(
        device, mirakana::runtime_rhi::RuntimeMavgConventionalMeshUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .streaming_result = &streaming,
                    .resident_catalog = &catalog,
                    .payload = &payload,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(upload.diagnostics.size() == 1);
    MK_REQUIRE(upload.diagnostics[0].code == "package-streaming-not-committed");
    MK_REQUIRE(upload.mesh_binding.binding.owner_device == nullptr);
    MK_REQUIRE(upload.upload.owner_device == nullptr);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("mavg conventional upload rejects resident catalog rows that are not mavg cluster graphs") {
    const auto graph = make_mavg_upload_graph();
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 9};
    const auto catalog = make_runtime_catalog({make_mavg_graph_record(graph.asset, handle, mirakana::AssetKind::mesh)});
    const auto streaming = make_committed_streaming_result();
    const auto payload = make_mavg_runtime_mesh_payload(graph.asset, handle);
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_conventional_mesh_binding(
        device, mirakana::runtime_rhi::RuntimeMavgConventionalMeshUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .streaming_result = &streaming,
                    .resident_catalog = &catalog,
                    .payload = &payload,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(upload.diagnostics.size() == 1);
    MK_REQUIRE(upload.diagnostics[0].code == "runtime-resource-not-mavg-cluster-graph");
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("mavg conventional upload rejects payload handle mismatches before upload") {
    const auto graph = make_mavg_upload_graph();
    const auto catalog_handle = mirakana::runtime::RuntimeAssetHandle{.value = 10};
    const auto payload_handle = mirakana::runtime::RuntimeAssetHandle{.value = 11};
    const auto catalog = make_runtime_catalog({make_mavg_graph_record(graph.asset, catalog_handle)});
    const auto streaming = make_committed_streaming_result();
    const auto payload = make_mavg_runtime_mesh_payload(graph.asset, payload_handle);
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_conventional_mesh_binding(
        device, mirakana::runtime_rhi::RuntimeMavgConventionalMeshUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .streaming_result = &streaming,
                    .resident_catalog = &catalog,
                    .payload = &payload,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(upload.diagnostics.size() == 1);
    MK_REQUIRE(upload.diagnostics[0].code == "mavg-payload-handle-mismatch");
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("mavg conventional upload rejects graph draw ranges outside runtime payload") {
    const auto graph = make_mavg_upload_graph();
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 12};
    const auto catalog = make_runtime_catalog({make_mavg_graph_record(graph.asset, handle)});
    const auto streaming = make_committed_streaming_result();
    const auto payload = make_mavg_runtime_mesh_payload(graph.asset, handle, 10);
    mirakana::rhi::NullRhiDevice device;

    const auto upload = mirakana::runtime_rhi::upload_runtime_mavg_conventional_mesh_binding(
        device, mirakana::runtime_rhi::RuntimeMavgConventionalMeshUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .streaming_result = &streaming,
                    .resident_catalog = &catalog,
                    .payload = &payload,
                });

    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(upload.diagnostics.size() == 1);
    MK_REQUIRE(upload.diagnostics[0].code == "mavg-draw-range-outside-payload");
    MK_REQUIRE(upload.diagnostics[0].cluster_index == 2);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

int main() {
    return mirakana::test::run_all();
}
