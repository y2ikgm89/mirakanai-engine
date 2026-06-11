// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene_renderer/mavg_scene_lod.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streamed-backend-draw/graph");
    const auto material_body = mirakana::AssetId::from_name("materials/streamed-body");
    const auto material_trim = mirakana::AssetId::from_name("materials/streamed-trim");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = mirakana::AssetId::from_name("meshes/streamed-source"),
        .source_mesh_uri = "source/streamed.gltf",
        .cluster_payload_uri = "runtime/streamed.mavgpayload",
        .target_cluster_triangles = 1,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 0, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 256, .byte_size = 256, .first_cluster = 2, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_body, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material_trim, .first_cluster = 2, .cluster_count = 1},
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 0.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 1,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 1,
                    .geometric_error = 1.0F,
                    .first_index = 0,
                    .index_count = 3,
                    .vertex_base = 0,
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 1,
                    .parent_cluster_index = 2,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 2,
                    .geometric_error = 1.0F,
                    .first_index = 3,
                    .index_count = 3,
                    .vertex_base = 4,
                },
            },
    };
}

[[nodiscard]] mirakana::MavgLodSelectionResult make_selection(mirakana::AssetId graph_asset) {
    return mirakana::MavgLodSelectionResult{
        .selected_clusters =
            {
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 1,
                    .page_index = 1,
                    .lod_level = 0,
                    .material_partition = 0,
                    .first_index = 0,
                    .index_count = 3,
                    .vertex_base = 0,
                },
                mirakana::MavgLodSelectedCluster{
                    .graph_asset = graph_asset,
                    .cluster_index = 2,
                    .page_index = 2,
                    .lod_level = 0,
                    .material_partition = 1,
                    .first_index = 3,
                    .index_count = 3,
                    .vertex_base = 4,
                },
            },
    };
}

[[nodiscard]] mirakana::MeshGpuBinding make_mesh_binding(const mirakana::rhi::IRhiDevice& device,
                                                         std::uint32_t base) noexcept {
    return mirakana::MeshGpuBinding{
        .vertex_buffer = mirakana::rhi::BufferHandle{base},
        .index_buffer = mirakana::rhi::BufferHandle{base + 1U},
        .vertex_count = 16,
        .index_count = 6,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = 12,
        .index_format = mirakana::rhi::IndexFormat::uint32,
        .owner_device = &device,
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

[[nodiscard]] mirakana::SceneGpuBindingPalette make_material_bindings(const mirakana::MavgClusterGraphDocument& graph,
                                                                      mirakana::rhi::IRhiDevice& device) {
    mirakana::SceneGpuBindingPalette bindings;
    MK_REQUIRE(bindings.try_add_material(graph.material_partitions[0].material, make_material_binding(device, 201)));
    MK_REQUIRE(bindings.try_add_material(graph.material_partitions[1].material, make_material_binding(device, 301)));
    return bindings;
}

[[nodiscard]] mirakana::SceneGpuBindingPalette
make_rhi_material_bindings(const mirakana::MavgClusterGraphDocument& graph, mirakana::rhi::IRhiDevice& device,
                           mirakana::rhi::PipelineLayoutHandle pipeline_layout,
                           mirakana::rhi::DescriptorSetHandle descriptor_set) {
    mirakana::SceneGpuBindingPalette bindings;
    const auto binding = mirakana::MaterialGpuBinding{
        .pipeline_layout = pipeline_layout,
        .descriptor_set = descriptor_set,
        .descriptor_set_index = 0,
        .owner_device = &device,
    };
    MK_REQUIRE(bindings.try_add_material(graph.material_partitions[0].material, binding));
    MK_REQUIRE(bindings.try_add_material(graph.material_partitions[1].material, binding));
    return bindings;
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset,
                                                                mirakana::runtime::RuntimeAssetHandle handle,
                                                                std::string content = "streamed-page") {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/streamed-backend-draw/" + std::to_string(handle.value) + ".geasset",
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
                       .label = "mavg-streamed-backend-draw-page-" + std::to_string(mount_id),
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

[[nodiscard]] mirakana::runtime::RuntimeMeshPayload make_payload(mirakana::AssetId page_asset,
                                                                 mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeMeshPayload{
        .asset = page_asset,
        .handle = handle,
        .vertex_count = 16,
        .index_count = 6,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(16U * 12U, std::uint8_t{0x61}),
        .index_bytes = std::vector<std::uint8_t>(6U * 4U, std::uint8_t{0x00}),
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult
make_two_page_adoption(mirakana::AssetId graph_asset) {
    mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult adoption;
    adoption.adopted_rows = {
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionRow{
            .graph_asset = graph_asset,
            .page_index = 1,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11},
            .resident_mount =
                mirakana::runtime::RuntimeResidentPackageMountResultV2{
                    .status = mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted,
                },
        },
        mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionRow{
            .graph_asset = graph_asset,
            .page_index = 2,
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
            .resident_mount =
                mirakana::runtime::RuntimeResidentPackageMountResultV2{
                    .status = mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted,
                },
        },
    };
    adoption.adopted_page_count = adoption.adopted_rows.size();
    adoption.mounted_package_count = adoption.adopted_rows.size();
    adoption.committed = true;
    adoption.mutated_mount_set = true;
    return adoption;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult
make_real_streamed_upload(const mirakana::MavgClusterGraphDocument& graph, mirakana::rhi::IRhiDevice& device) {
    const auto page_asset_1 = mirakana::AssetId::from_name("mavg/streamed-backend-draw/uploaded-page-1");
    const auto page_asset_2 = mirakana::AssetId::from_name("mavg/streamed-backend-draw/uploaded-page-2");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_page(mount_set, 11, page_asset_1, mirakana::runtime::RuntimeAssetHandle{.value = 21});
    mount_page(mount_set, 12, page_asset_2, mirakana::runtime::RuntimeAssetHandle{.value = 22});
    const auto cache = make_cache(mount_set);
    const auto adoption = make_two_page_adoption(graph.asset);
    const std::vector<mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow> payloads{
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 1,
            .payload = make_payload(page_asset_1, resident_package_handle(cache.catalog(), page_asset_1)),
        },
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPagePayloadRow{
            .graph_asset = graph.asset,
            .page_index = 2,
            .payload = make_payload(page_asset_2, resident_package_handle(cache.catalog(), page_asset_2)),
        },
    };
    return mirakana::runtime_rhi::upload_runtime_mavg_streamed_cluster_pages(
        device, mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadDesc{
                    .graph_asset = graph.asset,
                    .graph = &graph,
                    .safe_point_adoption = &adoption,
                    .resident_catalog = &cache.catalog(),
                    .page_payloads = payloads,
                });
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult
make_streamed_upload(const mirakana::MavgClusterGraphDocument& graph, mirakana::rhi::IRhiDevice& device) {
    mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult upload;
    upload.package_visible = true;
    upload.streamed_cluster_pages_ready = true;
    upload.invoked_gpu_upload = true;
    upload.uploaded_page_count = 2;
    upload.uploaded_cluster_count = 2;
    upload.page_bindings = {
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPageBindingRow{
            .graph_asset = graph.asset,
            .page_asset = mirakana::AssetId::from_name("mavg/streamed-backend-draw/page-1"),
            .page_index = 1,
            .uploaded_cluster_count = 1,
            .uploaded_bytes = 128,
            .binding = make_mesh_binding(device, 401),
        },
        mirakana::runtime_rhi::RuntimeMavgStreamedClusterPageBindingRow{
            .graph_asset = graph.asset,
            .page_asset = mirakana::AssetId::from_name("mavg/streamed-backend-draw/page-2"),
            .page_index = 2,
            .uploaded_cluster_count = 1,
            .uploaded_bytes = 128,
            .binding = make_mesh_binding(device, 501),
        },
    };
    return upload;
}

[[nodiscard]] mirakana::MeshGpuBinding make_owned_mesh_binding(mirakana::rhi::IRhiDevice& device,
                                                               std::uint32_t vertex_count, std::uint32_t index_count) {
    return mirakana::MeshGpuBinding{
        .vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
            .size_bytes = static_cast<std::uint64_t>(vertex_count) * 12U,
            .usage = mirakana::rhi::BufferUsage::vertex,
        }),
        .index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
            .size_bytes = static_cast<std::uint64_t>(index_count) * 4U,
            .usage = mirakana::rhi::BufferUsage::index,
        }),
        .vertex_count = vertex_count,
        .index_count = index_count,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = 12,
        .index_format = mirakana::rhi::IndexFormat::uint32,
        .owner_device = &device,
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult
make_owned_streamed_upload(const mirakana::MavgClusterGraphDocument& graph, mirakana::rhi::IRhiDevice& device) {
    auto upload = make_streamed_upload(graph, device);
    upload.page_bindings[0].binding = make_owned_mesh_binding(device, 16, 6);
    upload.page_bindings[1].binding = make_owned_mesh_binding(device, 16, 6);
    return upload;
}

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::MavgSceneLodDiagnostic>& diagnostics,
                                  mirakana::MavgSceneLodDiagnosticCode code) noexcept {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
create_pipeline(mirakana::rhi::IRhiDevice& device, mirakana::rhi::Format color_format,
                mirakana::rhi::PipelineLayoutHandle pipeline_layout = {}) {
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = pipeline_layout.value == 0 ? device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{})
                                                   : pipeline_layout;
    return device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = color_format,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = 12, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });
}

} // namespace

MK_TEST("streamed mavg scene lod planning chooses page gpu bindings") {
    const auto graph = make_graph();
    MK_REQUIRE(mirakana::validate_mavg_cluster_graph(graph).valid());
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    auto material_bindings = make_material_bindings(graph, device);
    const auto upload = make_streamed_upload(graph, device);

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F}},
            .material_bindings = &material_bindings,
            .streamed_upload = &upload,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.submitted_cluster_count == 2);
    MK_REQUIRE(result.mesh_commands.size() == 2);
    MK_REQUIRE(result.mesh_commands[0].mesh == upload.page_bindings[0].page_asset);
    MK_REQUIRE(result.mesh_commands[0].mesh_binding.vertex_buffer.value == 401);
    MK_REQUIRE(result.mesh_commands[0].material_binding.pipeline_layout.value == 201);
    MK_REQUIRE(result.mesh_commands[0].indexed_range.enabled);
    MK_REQUIRE(result.mesh_commands[0].indexed_range.first_index == 0);
    MK_REQUIRE(result.mesh_commands[0].indexed_range.index_count == 3);
    MK_REQUIRE(result.mesh_commands[0].indexed_range.vertex_base == 0);
    MK_REQUIRE(result.mesh_commands[1].mesh == upload.page_bindings[1].page_asset);
    MK_REQUIRE(result.mesh_commands[1].mesh_binding.vertex_buffer.value == 501);
    MK_REQUIRE(result.mesh_commands[1].material_binding.pipeline_layout.value == 301);
    MK_REQUIRE(result.mesh_commands[1].indexed_range.enabled);
    MK_REQUIRE(result.mesh_commands[1].indexed_range.first_index == 3);
    MK_REQUIRE(result.mesh_commands[1].indexed_range.index_count == 3);
    MK_REQUIRE(result.mesh_commands[1].indexed_range.vertex_base == 4);
}

MK_TEST("streamed mavg scene lod planning rejects missing page gpu binding") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    auto material_bindings = make_material_bindings(graph, device);
    auto upload = make_streamed_upload(graph, device);
    upload.page_bindings.pop_back();

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .material_bindings = &material_bindings,
            .streamed_upload = &upload,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mesh_commands.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, mirakana::MavgSceneLodDiagnosticCode::missing_streamed_page_binding));
}

MK_TEST("streamed mavg scene lod planning rejects missing streamed upload") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph, mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mesh_commands.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, mirakana::MavgSceneLodDiagnosticCode::missing_streamed_upload));
}

MK_TEST("streamed mavg scene lod planning rejects not ready streamed upload") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    auto upload = make_streamed_upload(graph, device);
    upload.streamed_cluster_pages_ready = false;

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .streamed_upload = &upload,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mesh_commands.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, mirakana::MavgSceneLodDiagnosticCode::streamed_upload_not_ready));
}

MK_TEST("streamed mavg scene lod planning rejects page binding whose draw range is outside upload") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    auto upload = make_streamed_upload(graph, device);
    upload.page_bindings[1].binding.index_count = 3;

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .streamed_upload = &upload,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.mesh_commands.empty());
    MK_REQUIRE(has_diagnostic(result.diagnostics, mirakana::MavgSceneLodDiagnosticCode::missing_streamed_page_binding));
}

MK_TEST("streamed mavg scene lod planning consumes actual streamed upload page bindings") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_real_streamed_upload(graph, device);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.page_bindings.size() == 2);

    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .streamed_upload = &upload,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mesh_commands.size() == 2);
    MK_REQUIRE(result.mesh_commands[0].mesh == upload.page_bindings[0].page_asset);
    MK_REQUIRE(result.mesh_commands[0].mesh_binding.owner_device == &device);
    MK_REQUIRE(result.mesh_commands[1].mesh == upload.page_bindings[1].page_asset);
    MK_REQUIRE(result.mesh_commands[1].mesh_binding.owner_device == &device);
}

MK_TEST("streamed mavg scene lod planned commands submit through null renderer") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_owned_streamed_upload(graph, device);
    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .streamed_upload = &upload,
        });
    MK_REQUIRE(result.succeeded());

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    renderer.begin_frame();
    for (const auto& command : result.mesh_commands) {
        renderer.draw_mesh(command);
    }
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(stats.meshes_submitted == result.mesh_commands.size());
    MK_REQUIRE(stats.frames_finished == 1);
}

MK_TEST("streamed mavg scene lod planned commands record range aware rhi draws") {
    const auto graph = make_graph();
    const auto selection = make_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_owned_streamed_upload(graph, device);
    const auto material_set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto material_set = device.allocate_descriptor_set(material_set_layout);
    const auto pipeline_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {material_set_layout}, .push_constant_bytes = 0});
    auto material_bindings = make_rhi_material_bindings(graph, device, pipeline_layout, material_set);
    const auto result = mirakana::runtime_scene_rhi::plan_runtime_mavg_streamed_scene_lod_mesh_commands(
        selection, graph,
        mirakana::runtime_scene_rhi::RuntimeMavgStreamedSceneLodSubmitDesc{
            .material_bindings = &material_bindings,
            .streamed_upload = &upload,
        });
    MK_REQUIRE(result.succeeded());

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto pipeline = create_pipeline(device, mirakana::rhi::Format::rgba8_unorm, pipeline_layout);
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    for (const auto& command : result.mesh_commands) {
        renderer.draw_mesh(command);
    }
    renderer.end_frame();

    const auto stats = device.stats();
    MK_REQUIRE(stats.indexed_draw_calls == 2);
    MK_REQUIRE(stats.descriptor_sets_bound == 2);
    MK_REQUIRE(stats.indices_submitted == 6);
    MK_REQUIRE(stats.last_indexed_draw_index_count == 3);
    MK_REQUIRE(stats.last_indexed_draw_first_index == 3);
    MK_REQUIRE(stats.last_indexed_draw_vertex_offset == 4);
}

int main() {
    return mirakana::test::run_all();
}
