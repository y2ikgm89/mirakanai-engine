// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/scene_renderer/mavg_scene_lod.hpp"

#include <span>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_scene_lod_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/hero");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/hero-source");
    const auto material_body = mirakana::AssetId::from_name("materials/body");
    const auto material_trim = mirakana::AssetId::from_name("materials/trim");

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

[[nodiscard]] mirakana::MavgLodSelectionResult make_scene_lod_selection(mirakana::AssetId graph_asset) {
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

[[nodiscard]] mirakana::MeshGpuBinding make_mesh_binding(const mirakana::rhi::IRhiDevice& device) noexcept {
    return mirakana::MeshGpuBinding{
        .vertex_buffer = mirakana::rhi::BufferHandle{101},
        .index_buffer = mirakana::rhi::BufferHandle{102},
        .vertex_count = 48,
        .index_count = 96,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = 48,
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

} // namespace

MK_TEST("mavg scene lod planning creates range aware mesh commands") {
    const auto graph = make_scene_lod_graph();
    const auto validation = mirakana::validate_mavg_cluster_graph(graph);
    MK_REQUIRE(validation.valid());
    const auto selection = make_scene_lod_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;

    const auto mesh_binding =
        mirakana::MavgSceneLodMeshBinding{.graph_asset = graph.asset, .mesh_binding = make_mesh_binding(device)};
    const std::vector<mirakana::MavgSceneLodMaterialBinding> materials{
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[0].material,
                                              .material_binding = make_material_binding(device, 201)},
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[1].material,
                                              .material_binding = make_material_binding(device, 301)},
    };

    const auto result = mirakana::plan_mavg_scene_lod_mesh_commands(
        selection, graph,
        mirakana::MavgSceneLodSubmitDesc{
            .graph_asset = graph.asset,
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F}},
            .fallback_color = mirakana::Color{.r = 0.2F, .g = 0.3F, .b = 0.4F, .a = 1.0F},
            .mesh_binding = &mesh_binding,
            .material_bindings = std::span<const mirakana::MavgSceneLodMaterialBinding>{materials},
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.submitted_cluster_count == 2);
    MK_REQUIRE(result.fallback_substitution_count == 1);
    MK_REQUIRE(result.mesh_commands.size() == 2);

    const auto& first = result.mesh_commands[0];
    MK_REQUIRE(first.mesh == graph.asset);
    MK_REQUIRE(first.material == graph.material_partitions[0].material);
    MK_REQUIRE(first.transform.position.x == 2.0F);
    MK_REQUIRE(first.mesh_binding.index_buffer.value == 102);
    MK_REQUIRE(first.material_binding.pipeline_layout.value == 201);
    MK_REQUIRE(first.indexed_range.enabled);
    MK_REQUIRE(first.indexed_range.first_index == 6);
    MK_REQUIRE(first.indexed_range.index_count == 3);
    MK_REQUIRE(first.indexed_range.vertex_base == 12);

    const auto& second = result.mesh_commands[1];
    MK_REQUIRE(second.material == graph.material_partitions[1].material);
    MK_REQUIRE(second.material_binding.pipeline_layout.value == 301);
    MK_REQUIRE(second.indexed_range.enabled);
    MK_REQUIRE(second.indexed_range.first_index == 9);
    MK_REQUIRE(second.indexed_range.index_count == 3);
    MK_REQUIRE(second.indexed_range.vertex_base == 24);
}

MK_TEST("mavg scene lod planning reports missing material binding and uses fallback color") {
    const auto graph = make_scene_lod_graph();
    const auto selection = make_scene_lod_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    const auto mesh_binding =
        mirakana::MavgSceneLodMeshBinding{.graph_asset = graph.asset, .mesh_binding = make_mesh_binding(device)};
    const std::vector<mirakana::MavgSceneLodMaterialBinding> materials{
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[0].material,
                                              .material_binding = make_material_binding(device, 401)},
    };

    const auto result = mirakana::plan_mavg_scene_lod_mesh_commands(
        selection, graph,
        mirakana::MavgSceneLodSubmitDesc{
            .graph_asset = graph.asset,
            .fallback_color = mirakana::Color{.r = 0.7F, .g = 0.6F, .b = 0.5F, .a = 1.0F},
            .mesh_binding = &mesh_binding,
            .material_bindings = std::span<const mirakana::MavgSceneLodMaterialBinding>{materials},
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.missing_material_binding_count == 1);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(result.mesh_commands.size() == 2);
    MK_REQUIRE(result.mesh_commands[1].material == graph.material_partitions[1].material);
    MK_REQUIRE(result.mesh_commands[1].material_binding.pipeline_layout.value == 0);
    MK_REQUIRE(result.mesh_commands[1].color.r == 0.7F);
    MK_REQUIRE(result.mesh_commands[1].color.g == 0.6F);
    MK_REQUIRE(result.mesh_commands[1].color.b == 0.5F);
}

MK_TEST("mavg scene lod planned commands submit through null renderer") {
    const auto graph = make_scene_lod_graph();
    const auto selection = make_scene_lod_selection(graph.asset);
    mirakana::rhi::NullRhiDevice device;
    const auto mesh_binding =
        mirakana::MavgSceneLodMeshBinding{.graph_asset = graph.asset, .mesh_binding = make_mesh_binding(device)};
    const std::vector<mirakana::MavgSceneLodMaterialBinding> materials{
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[0].material,
                                              .material_binding = make_material_binding(device, 501)},
        mirakana::MavgSceneLodMaterialBinding{.material = graph.material_partitions[1].material,
                                              .material_binding = make_material_binding(device, 601)},
    };
    const auto result = mirakana::plan_mavg_scene_lod_mesh_commands(
        selection, graph,
        mirakana::MavgSceneLodSubmitDesc{
            .graph_asset = graph.asset,
            .mesh_binding = &mesh_binding,
            .material_bindings = std::span<const mirakana::MavgSceneLodMaterialBinding>{materials},
        });

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

int main() {
    return mirakana::test::run_all();
}
