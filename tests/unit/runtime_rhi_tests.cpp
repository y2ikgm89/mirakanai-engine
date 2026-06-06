// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/rhi/upload_staging.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_gpu_memory_pressure.hpp"
#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] float read_float(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    float value = 0.0F;
    const auto source = std::span<const std::uint8_t>{bytes};
    std::memcpy(&value, source.subspan(offset).data(), sizeof(float));
    return value;
}

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_le_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void append_le_f32(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(bytes, bits);
}

void append_vec3(std::vector<std::uint8_t>& bytes, float x, float y, float z) {
    append_le_f32(bytes, x);
    append_le_f32(bytes, y);
    append_le_f32(bytes, z);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_texture_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
        .path = "textures/" + std::to_string(asset.value) + ".geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "texture",
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_mesh_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "meshes/" + std::to_string(asset.value) + ".geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "mesh",
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_skinned_mesh_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::skinned_mesh,
        .path = "meshes/" + std::to_string(asset.value) + ".skinned.geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "skinned_mesh",
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_morph_mesh_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::morph_mesh_cpu,
        .path = "meshes/" + std::to_string(asset.value) + ".morph.geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "morph_mesh_cpu",
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

[[nodiscard]] mirakana::runtime::RuntimeResourceCatalogV2
make_runtime_texture_catalog(std::vector<mirakana::runtime::RuntimeAssetRecord> records) {
    return make_runtime_catalog(std::move(records));
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionResult make_committed_package_streaming_result() {
    mirakana::runtime::RuntimePackageStreamingExecutionResult result;
    result.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed;
    result.target_id = "packaged-scene-streaming";
    result.package_index_path = "runtime/game.geindex";
    result.runtime_scene_validation_target_id = "packaged-scene";
    result.committed = true;
    return result;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeTextureUploadResult
make_runtime_texture_upload(mirakana::rhi::IRhiDevice& device, mirakana::AssetId texture) {
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 10U},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
    return mirakana::runtime_rhi::upload_runtime_texture(device, payload);
}

[[nodiscard]] mirakana::runtime::RuntimeTexturePayload
make_runtime_texture_payload(mirakana::AssetId texture, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeTexturePayload{
        .asset = texture,
        .handle = handle,
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMeshPayload
make_runtime_mesh_payload(mirakana::AssetId mesh, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeMeshPayload{
        .asset = mesh,
        .handle = handle,
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x61}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSkinnedMeshPayload
make_runtime_skinned_mesh_payload(mirakana::AssetId mesh, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeSkinnedMeshPayload{
        .asset = mesh,
        .handle = handle,
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = std::vector<std::uint8_t>(3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes,
                                                  std::uint8_t{0x62}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
        .joint_palette_bytes = std::vector<std::uint8_t>(mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes,
                                                         std::uint8_t{0x00}),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMorphMeshCpuPayload
make_runtime_morph_mesh_payload(mirakana::AssetId morph_asset, mirakana::runtime::RuntimeAssetHandle handle) {
    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    return mirakana::runtime::RuntimeMorphMeshCpuPayload{
        .asset = morph_asset,
        .handle = handle,
        .morph = std::move(morph),
    };
}

[[nodiscard]] mirakana::MavgClusterGraphDocument make_mavg_pressure_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/runtime-rhi-pressure");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/runtime-rhi-pressure-source");
    const auto material = mirakana::AssetId::from_name("materials/runtime-rhi-pressure-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/runtime-rhi-pressure.glb",
        .cluster_payload_uri = "runtime/runtime-rhi-pressure.mavg_payload",
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

void mount_mavg_pressure_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                              mirakana::AssetId asset, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-pressure-page-" + std::to_string(mount_id),
                       .package = mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
                           .handle = mirakana::runtime::RuntimeAssetHandle{.value = mount_id},
                           .asset = asset,
                           .kind = mirakana::AssetKind::mesh,
                           .path = "runtime/mavg/pressure-" + std::to_string(mount_id) + ".geasset",
                           .content_hash = asset.value + mount_id,
                           .source_revision = mount_id,
                           .dependencies = {},
                           .content = std::string(content),
                       }}),
                   })
                   .succeeded());
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceResult& result,
                                  mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime rhi mavg dxgi gpu memory pressure builds rows from d3d12 budget evidence") {
    const auto graph = make_mavg_pressure_graph();
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 256},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}, .estimated_gpu_resident_bytes = 768},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}, .estimated_gpu_resident_bytes = 512},
    };
    const mirakana::rhi::RhiDeviceMemoryDiagnostics memory{
        .os_video_memory_budget_available = true,
        .local_video_memory_budget_bytes = 1000,
        .local_video_memory_usage_bytes = 875,
        .non_local_video_memory_budget_bytes = 2000,
        .non_local_video_memory_usage_bytes = 250,
        .committed_resources_byte_estimate_available = true,
        .committed_resources_byte_estimate = 1536,
    };

    const auto result = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory = memory,
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.used_dxgi_video_memory_budget_evidence);
    MK_REQUIRE(result.produced_gpu_memory_pressure_rows);
    MK_REQUIRE(result.input_resident_page_mount_count == 3U);
    MK_REQUIRE(result.input_estimated_page_count == 3U);
    MK_REQUIRE(result.output_pressure_row_count == 3U);
    MK_REQUIRE(result.local_video_memory_budget_bytes == 1000U);
    MK_REQUIRE(result.local_video_memory_usage_bytes == 875U);
    MK_REQUIRE(result.local_video_memory_pressure_score > 0U);
    MK_REQUIRE(result.estimated_gpu_resident_bytes == 1536U);
    MK_REQUIRE(result.pressure_rows.size() == 3U);
    MK_REQUIRE(result.pressure_rows[0].page_index == 0U);
    MK_REQUIRE(result.pressure_rows[0].eviction_pressure_score == result.local_video_memory_pressure_score);
    MK_REQUIRE(result.pressure_rows[0].estimated_gpu_resident_bytes == 256U);
    MK_REQUIRE(result.pressure_rows[1].page_index == 1U);
    MK_REQUIRE(result.pressure_rows[1].estimated_gpu_resident_bytes == 768U);
    MK_REQUIRE(result.pressure_rows[2].page_index == 2U);
    MK_REQUIRE(result.pressure_rows[2].estimated_gpu_resident_bytes == 512U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(!result.enforced_gpu_residency);
    MK_REQUIRE(!result.reserved_video_memory);
}

MK_TEST("runtime rhi mavg dxgi gpu memory pressure rows feed mavg eviction ordering") {
    const auto graph = make_mavg_pressure_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_mavg_pressure_page(mount_set, 10, mirakana::AssetId::from_name("mavg/pressure/page-0"), "root");
    mount_mavg_pressure_page(mount_set, 11, mirakana::AssetId::from_name("mavg/pressure/page-1"), "large");
    mount_mavg_pressure_page(mount_set, 12, mirakana::AssetId::from_name("mavg/pressure/page-2"), "small");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 0},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 128},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}, .estimated_gpu_resident_bytes = 1024},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}, .estimated_gpu_resident_bytes = 256},
    };
    const auto evidence = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory =
                mirakana::rhi::RhiDeviceMemoryDiagnostics{
                    .os_video_memory_budget_available = true,
                    .local_video_memory_budget_bytes = 1000,
                    .local_video_memory_usage_bytes = 900,
                },
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });
    MK_REQUIRE(evidence.succeeded());

    const auto evictions = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .policy_kind = mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::
                           caller_supplied_gpu_memory_pressure,
                       .gpu_memory_pressure_rows = evidence.pressure_rows,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 4,
                           },
                   });

    MK_REQUIRE(evictions.succeeded());
    MK_REQUIRE(evictions.applied_caller_supplied_gpu_memory_pressure_policy);
    MK_REQUIRE(evictions.gpu_memory_pressure_eviction_candidate_count == 2U);
    MK_REQUIRE(evictions.protected_eviction_candidate_skip_count == 1U);
    MK_REQUIRE(evictions.gpu_memory_pressure_protected_estimated_bytes == 128U);
    MK_REQUIRE(evictions.gpu_memory_pressure_candidate_estimated_bytes == 1280U);
    MK_REQUIRE(evictions.eviction_candidate_unmount_order.size() == 2U);
    MK_REQUIRE(evictions.eviction_candidate_unmount_order[0] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
    MK_REQUIRE(evictions.eviction_candidate_unmount_order[1] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(!evictions.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime rhi mavg dxgi gpu memory pressure requires d3d12 dxgi budget evidence") {
    const auto graph = make_mavg_pressure_graph();
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 256},
    };

    const auto wrong_backend = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::vulkan,
            .memory =
                mirakana::rhi::RhiDeviceMemoryDiagnostics{
                    .os_video_memory_budget_available = true,
                    .local_video_memory_budget_bytes = 1000,
                    .local_video_memory_usage_bytes = 500,
                },
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });
    MK_REQUIRE(!wrong_backend.succeeded());
    MK_REQUIRE(has_diagnostic(
        wrong_backend, mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::unsupported_backend));
    MK_REQUIRE(wrong_backend.pressure_rows.empty());

    const auto missing_budget = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory = mirakana::rhi::RhiDeviceMemoryDiagnostics{},
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });
    MK_REQUIRE(!missing_budget.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_budget,
        mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::missing_dxgi_video_memory_budget));
    MK_REQUIRE(missing_budget.pressure_rows.empty());
}

MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects missing estimate rows") {
    const auto graph = make_mavg_pressure_graph();
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 256},
    };

    const auto result = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory =
                mirakana::rhi::RhiDeviceMemoryDiagnostics{
                    .os_video_memory_budget_available = true,
                    .local_video_memory_budget_bytes = 1000,
                    .local_video_memory_usage_bytes = 800,
                },
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.missing_estimate_row_count == 1U);
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::missing_estimate_row));
    MK_REQUIRE(result.pressure_rows.empty());
}

MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects duplicate estimate rows") {
    const auto graph = make_mavg_pressure_graph();
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 256},
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}, .estimated_gpu_resident_bytes = 512},
    };

    const auto result = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory =
                mirakana::rhi::RhiDeviceMemoryDiagnostics{
                    .os_video_memory_budget_available = true,
                    .local_video_memory_budget_bytes = 1000,
                    .local_video_memory_usage_bytes = 800,
                },
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.duplicate_estimate_row_count == 1U);
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::duplicate_estimate_row));
    MK_REQUIRE(result.pressure_rows.empty());
}

MK_TEST("runtime rhi mavg dxgi gpu memory pressure rejects estimated byte overflow") {
    const auto graph = make_mavg_pressure_graph();
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime_rhi::RuntimeMavgResidentPageGpuMemoryEstimateRow> estimates{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .estimated_gpu_resident_bytes = std::numeric_limits<std::uint64_t>::max()},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}, .estimated_gpu_resident_bytes = 1},
    };

    const auto result = mirakana::runtime_rhi::build_runtime_mavg_dxgi_gpu_memory_pressure_rows(
        mirakana::runtime_rhi::RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc{
            .graph_asset = graph.asset,
            .backend = mirakana::rhi::BackendKind::d3d12,
            .memory =
                mirakana::rhi::RhiDeviceMemoryDiagnostics{
                    .os_video_memory_budget_available = true,
                    .local_video_memory_budget_bytes = 1000,
                    .local_video_memory_usage_bytes = 800,
                },
            .resident_page_mounts = page_mounts,
            .estimated_pages = estimates,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.estimated_gpu_resident_byte_overflow);
    MK_REQUIRE(has_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgGpuMemoryPressureEvidenceDiagnosticCode::estimated_byte_overflow));
    MK_REQUIRE(result.pressure_rows.empty());
}

MK_TEST("runtime rhi upload creates texture resource and records byte upload when payload bytes exist") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = std::vector<std::uint8_t>(32, std::uint8_t{0x7f}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value == 1);
    MK_REQUIRE(result.upload_buffer.value == 1);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.uploaded_bytes == 512);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(result.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(result.frame_graph_barriers_recorded == 2);
    MK_REQUIRE(result.frame_graph_pass_target_state_barriers_recorded == 1);
    MK_REQUIRE(result.frame_graph_final_state_barriers_recorded == 1);
    MK_REQUIRE(result.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(result.texture_desc.extent.width == 4);
    MK_REQUIRE(result.texture_desc.extent.height == 2);
    MK_REQUIRE(result.texture_desc.format == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(result.copy_region.buffer_row_length == 64);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.buffers_created == 1);
    MK_REQUIRE(stats.buffer_writes == 1);
    MK_REQUIRE(stats.bytes_written == 512);
    MK_REQUIRE(stats.buffer_texture_copies == 1);
    MK_REQUIRE(stats.resource_transitions == 2);
    MK_REQUIRE(stats.command_lists_submitted == 1);
}

MK_TEST("runtime rhi texture upload can use caller owned upload ring staging") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    const auto texture = mirakana::AssetId::from_name("textures/ring_albedo");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{31},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = std::vector<std::uint8_t>(32, std::uint8_t{0x51}),
    };
    mirakana::runtime_rhi::RuntimeTextureUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_before_upload = device.stats().buffers_created;

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value != 0);
    MK_REQUIRE(result.upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(result.upload_buffer_caller_owned);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.uploaded_bytes == 512);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(result.frame_graph_barriers_recorded == 2);
    MK_REQUIRE(result.frame_graph_pass_target_state_barriers_recorded == 1);
    MK_REQUIRE(result.frame_graph_final_state_barriers_recorded == 1);
    MK_REQUIRE(result.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(device.stats().buffers_created == buffers_before_upload);
    MK_REQUIRE(device.stats().buffer_writes == 1);
    MK_REQUIRE(device.stats().bytes_written == 512);
    MK_REQUIRE(device.stats().buffer_texture_copies == 1);
    MK_REQUIRE(device.stats().textures_created == 1);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
    MK_REQUIRE(device.stats().fence_waits == 1);
}

MK_TEST("runtime rhi texture upload rejects ring staging exhaustion before side effects") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 32, .min_alignment = 256, .buffer = {}});
    const auto texture = mirakana::AssetId::from_name("textures/ring_too_small");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{32},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = std::vector<std::uint8_t>(32, std::uint8_t{0x52}),
    };
    mirakana::runtime_rhi::RuntimeTextureUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload, options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("ring") != std::string::npos);
    MK_REQUIRE(result.texture.value == 0);
    MK_REQUIRE(result.upload_buffer.value == 0);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation);
    MK_REQUIRE(device.stats().textures_created == 0);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().buffer_texture_copies == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime rhi upload creates texture resource without copy for metadata only payload") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = {},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value == 1);
    MK_REQUIRE(result.upload_buffer.value == 0);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(!result.copy_recorded);
    MK_REQUIRE(result.uploaded_bytes == 0);
    MK_REQUIRE(result.submitted_fence.value == 0);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 0);
    MK_REQUIRE(result.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(result.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(result.frame_graph_pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.frame_graph_final_state_barriers_recorded == 0);
    MK_REQUIRE(result.frame_graph_pass_callbacks_invoked == 0);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.buffers_created == 0);
    MK_REQUIRE(stats.buffer_texture_copies == 0);
}

MK_TEST("runtime rhi upload rejects unsupported texture formats without creating resources") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/greyscale");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::r8_unorm,
        .source_bytes = 8,
        .bytes = std::vector<std::uint8_t>(8, std::uint8_t{0xff}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("format") != std::string::npos);
    MK_REQUIRE(device.stats().textures_created == 0);
}

MK_TEST("runtime rhi upload reports submitted fence without forcing wait") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/no_wait_upload");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{13},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
    mirakana::runtime_rhi::RuntimeTextureUploadOptions options;
    options.wait_for_completion = false;

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(result.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
    MK_REQUIRE(device.stats().fence_waits == 0);
}

MK_TEST("runtime package streaming frame graph handoff builds imported texture binding and executor callback") {
    const auto texture = mirakana::AssetId::from_name("textures/streamed/albedo");
    const auto catalog = make_runtime_texture_catalog(
        {make_runtime_texture_record(texture, mirakana::runtime::RuntimeAssetHandle{.value = 1})});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_runtime_texture_upload(device, texture);
    MK_REQUIRE(upload.succeeded());

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = texture,
            .resource = "package_albedo",
            .upload = &upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.size() == 1);
    MK_REQUIRE(handoff.texture_bindings[0].resource == "package_albedo");
    MK_REQUIRE(handoff.texture_bindings[0].texture.value == upload.texture.value);
    MK_REQUIRE(handoff.texture_bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "package_albedo", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "sample_package_texture",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "package_albedo",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });
    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "sample_package_texture",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto execution =
        mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
            .commands = commands.get(),
            .schedule = schedule,
            .texture_bindings = handoff.texture_bindings,
            .pass_callbacks = callbacks,
            .pass_target_accesses = {},
            .pass_target_states = {},
            .final_states = {},
        });

    MK_REQUIRE(execution.succeeded());
    MK_REQUIRE(execution.pass_callbacks_invoked == 1);
    MK_REQUIRE(callbacks_invoked == 1);
    MK_REQUIRE(execution.barriers_recorded == 0);
}

MK_TEST("runtime package streaming frame graph upload binding transaction uploads resident textures") {
    const auto albedo = mirakana::AssetId::from_name("textures/streamed/transaction_albedo");
    const auto normal = mirakana::AssetId::from_name("textures/streamed/transaction_normal");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto albedo_payload = make_runtime_texture_payload(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1});
    const auto normal_payload = make_runtime_texture_payload(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2});
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
            .asset = albedo,
            .resource = "package_albedo",
            .payload = &albedo_payload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
            .asset = normal,
            .resource = "package_normal",
            .payload = &normal_payload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_frame_graph_texture_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 2);
    MK_REQUIRE(transaction.texture_bindings.size() == 2);
    MK_REQUIRE(transaction.uploaded_bytes == 512);
    MK_REQUIRE(transaction.frame_graph_barriers_recorded == 4);
    MK_REQUIRE(transaction.frame_graph_pass_target_state_barriers_recorded == 2);
    MK_REQUIRE(transaction.frame_graph_final_state_barriers_recorded == 2);
    MK_REQUIRE(transaction.frame_graph_pass_callbacks_invoked == 2);
    MK_REQUIRE(transaction.texture_bindings[0].resource == "package_albedo");
    MK_REQUIRE(transaction.texture_bindings[0].texture.value == transaction.uploads[0].texture.value);
    MK_REQUIRE(transaction.texture_bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(transaction.texture_bindings[1].resource == "package_normal");
    MK_REQUIRE(transaction.texture_bindings[1].texture.value == transaction.uploads[1].texture.value);

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "package_albedo", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "package_normal", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "sample_package_textures",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "package_albedo",
                                                     .access = mirakana::FrameGraphAccess::shader_read},
                  mirakana::FrameGraphResourceAccess{.resource = "package_normal",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
    });
    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "sample_package_textures",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto execution =
        mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
            .commands = commands.get(),
            .schedule = schedule,
            .texture_bindings = transaction.texture_bindings,
            .pass_callbacks = callbacks,
        });

    MK_REQUIRE(execution.succeeded());
    MK_REQUIRE(execution.pass_callbacks_invoked == 1);
    MK_REQUIRE(callbacks_invoked == 1);
    MK_REQUIRE(execution.barriers_recorded == 0);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 2);
    MK_REQUIRE(stats.buffers_created == 2);
    MK_REQUIRE(stats.buffer_texture_copies == 2);
    MK_REQUIRE(stats.resource_transitions == 4);
    MK_REQUIRE(stats.command_lists_submitted == 2);
}

MK_TEST("runtime package streaming texture transaction can reuse caller owned upload ring") {
    const auto albedo = mirakana::AssetId::from_name("textures/streamed/ring_transaction_albedo");
    const auto normal = mirakana::AssetId::from_name("textures/streamed/ring_transaction_normal");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeTextureUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    const auto albedo_payload = make_runtime_texture_payload(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1});
    const auto normal_payload = make_runtime_texture_payload(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2});
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
            .asset = albedo,
            .resource = "package_ring_albedo",
            .payload = &albedo_payload,
            .upload_options = upload_options,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
            .asset = normal,
            .resource = "package_ring_normal",
            .payload = &normal_payload,
            .upload_options = upload_options,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_frame_graph_texture_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 2);
    MK_REQUIRE(transaction.texture_bindings.size() == 2);
    MK_REQUIRE(transaction.uploaded_bytes == 512);
    MK_REQUIRE(transaction.uploads[0].upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[1].upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.texture_bindings[0].resource == "package_ring_albedo");
    MK_REQUIRE(transaction.texture_bindings[0].texture.value == transaction.uploads[0].texture.value);
    MK_REQUIRE(transaction.texture_bindings[1].resource == "package_ring_normal");
    MK_REQUIRE(transaction.texture_bindings[1].texture.value == transaction.uploads[1].texture.value);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation);
    MK_REQUIRE(device.stats().textures_created == 2);
    MK_REQUIRE(device.stats().buffer_writes == 2);
    MK_REQUIRE(device.stats().buffer_texture_copies == 2);
    MK_REQUIRE(device.stats().command_lists_submitted == 2);
}

MK_TEST("runtime package streaming frame graph upload binding transaction rejects missing payload before upload") {
    const auto texture = mirakana::AssetId::from_name("textures/streamed/missing_payload");
    const auto catalog = make_runtime_texture_catalog(
        {make_runtime_texture_record(texture, mirakana::runtime::RuntimeAssetHandle{.value = 1})});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
            .asset = texture,
            .resource = "missing_payload",
        },
    };

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_frame_graph_texture_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(!transaction.succeeded());
    MK_REQUIRE(transaction.uploads.empty());
    MK_REQUIRE(transaction.texture_bindings.empty());
    MK_REQUIRE(transaction.diagnostics.size() == 1);
    MK_REQUIRE(transaction.diagnostics[0].asset == texture);
    MK_REQUIRE(transaction.diagnostics[0].resource == "missing_payload");
    MK_REQUIRE(transaction.diagnostics[0].code == "texture-payload-missing");
    MK_REQUIRE(device.stats().textures_created == 0);
}

MK_TEST("runtime package streaming frame graph handoff rejects non committed streaming result") {
    mirakana::runtime::RuntimePackageStreamingExecutionResult streaming;
    streaming.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required;
    const mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, {});

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].code == "package-streaming-not-committed");
}

MK_TEST("runtime package streaming frame graph handoff rejects texture assets missing from resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/missing/albedo");
    const auto streaming = make_committed_package_streaming_result();
    const mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_runtime_texture_upload(device, texture);

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = texture,
            .resource = "missing_albedo",
            .upload = &upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].asset == texture);
    MK_REQUIRE(handoff.diagnostics[0].code == "runtime-resource-not-live");
}

MK_TEST("runtime package streaming frame graph handoff rejects failed and empty texture uploads") {
    const auto failed_texture = mirakana::AssetId::from_name("textures/failed_upload");
    const auto empty_texture = mirakana::AssetId::from_name("textures/empty_upload");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(failed_texture, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(empty_texture, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::runtime_rhi::RuntimeTextureUploadResult failed_upload;
    failed_upload.diagnostic = "upload failed";
    const mirakana::runtime_rhi::RuntimeTextureUploadResult empty_upload;

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = failed_texture,
            .resource = "failed_upload",
            .upload = &failed_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = empty_texture,
            .resource = "empty_upload",
            .upload = &empty_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 2);
    MK_REQUIRE(handoff.diagnostics[0].code == "texture-upload-failed");
    MK_REQUIRE(handoff.diagnostics[1].code == "texture-upload-empty");
}

MK_TEST("runtime package streaming frame graph handoff rejects texture uploads without owner device provenance") {
    const auto texture = mirakana::AssetId::from_name("textures/missing_owner");
    const auto catalog = make_runtime_texture_catalog(
        {make_runtime_texture_record(texture, mirakana::runtime::RuntimeAssetHandle{.value = 1})});
    const auto streaming = make_committed_package_streaming_result();
    const mirakana::runtime_rhi::RuntimeTextureUploadResult upload{
        .texture = mirakana::rhi::TextureHandle{.value = 42},
    };
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = texture,
            .resource = "missing_owner",
            .upload = &upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].code == "texture-upload-owner-missing");
}

MK_TEST("runtime package streaming frame graph handoff rejects duplicate frame graph resource names") {
    const auto albedo = mirakana::AssetId::from_name("textures/duplicate/albedo");
    const auto normal = mirakana::AssetId::from_name("textures/duplicate/normal");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto albedo_upload = make_runtime_texture_upload(device, albedo);
    const auto normal_upload = make_runtime_texture_upload(device, normal);

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = albedo,
            .resource = "package_texture",
            .upload = &albedo_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = normal,
            .resource = "package_texture",
            .upload = &normal_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].code == "duplicate-frame-graph-resource");
    MK_REQUIRE(handoff.diagnostics[0].resource == "package_texture");
}

MK_TEST("runtime package streaming mesh upload transaction uploads resident static mesh with caller owned ring") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/ring_triangle");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 3};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    const auto payload = make_runtime_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{
            .asset = mesh,
            .payload = &payload,
            .upload_options = upload_options,
        },
    };
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    auto transaction =
        mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 1);
    MK_REQUIRE(transaction.mesh_bindings.size() == 1);
    MK_REQUIRE(transaction.uploaded_bytes == 48);
    MK_REQUIRE(transaction.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(transaction.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(transaction.submitted_fences.size() == 1);
    MK_REQUIRE(transaction.uploads[0].vertex_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].index_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].upload_buffers_caller_owned);
    MK_REQUIRE(transaction.mesh_bindings[0].asset == mesh);
    MK_REQUIRE(transaction.mesh_bindings[0].binding.vertex_buffer.value == transaction.uploads[0].vertex_buffer.value);
    MK_REQUIRE(transaction.mesh_bindings[0].binding.index_buffer.value == transaction.uploads[0].index_buffer.value);
    MK_REQUIRE(transaction.mesh_bindings[0].binding.owner_device == &device);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation + 2);
    MK_REQUIRE(device.stats().buffer_writes == 2);
    MK_REQUIRE(device.stats().buffer_copies == 2);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
}

MK_TEST("runtime package streaming mesh upload transaction reuses staging pool lease backed ring") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/pool_ring_triangle");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 45};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiStagingBufferPool pool(
        device, mirakana::rhi::RhiStagingBufferPoolDesc{.buffer_count = 1, .chunk_size_bytes = 512});
    const auto lease = pool.try_acquire_lease();
    MK_REQUIRE(lease.has_value());
    const auto buffers_after_pool_create = device.stats().buffers_created;
    mirakana::rhi::RhiUploadRing ring(device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = lease->size_bytes,
                                                                               .min_alignment = 256,
                                                                               .buffer = lease->buffer});
    mirakana::runtime_rhi::RuntimeMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    upload_options.queue = mirakana::rhi::QueueKind::copy;
    upload_options.wait_for_completion = false;
    const auto payload = make_runtime_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{
            .asset = mesh,
            .payload = &payload,
            .upload_options = upload_options,
        },
    };

    auto transaction =
        mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 1);
    MK_REQUIRE(transaction.mesh_bindings.size() == 1);
    MK_REQUIRE(transaction.uploaded_bytes == 48);
    MK_REQUIRE(transaction.upload_queue_waits_recorded == 1);
    MK_REQUIRE(transaction.submitted_fences.size() == 1);
    MK_REQUIRE(transaction.submitted_fences.front().queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(transaction.uploads[0].vertex_upload_buffer.value == lease->buffer.value);
    MK_REQUIRE(transaction.uploads[0].index_upload_buffer.value == lease->buffer.value);
    MK_REQUIRE(transaction.uploads[0].upload_buffers_caller_owned);
    MK_REQUIRE(transaction.mesh_bindings[0].asset == mesh);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_pool_create + 2);
    MK_REQUIRE(device.stats().copy_queue_submits == 1);
    MK_REQUIRE(device.stats().graphics_queue_submits == 0);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().queue_waits == 1);
    ring.release_completed(transaction.submitted_fences.front());
    pool.release(*lease);
    const auto reacquired = pool.try_acquire_lease();
    MK_REQUIRE(reacquired.has_value());
    MK_REQUIRE(reacquired->buffer.value == lease->buffer.value);
}

MK_TEST("runtime package upload staging evidence uses pooled async ring for selected package transactions") {
    mirakana::rhi::NullRhiDevice device;

    const auto evidence = mirakana::runtime_rhi::execute_runtime_package_upload_staging_evidence(device);

    MK_REQUIRE(evidence.succeeded());
    MK_REQUIRE(evidence.ready);
    MK_REQUIRE(evidence.package_transactions == 4);
    MK_REQUIRE(evidence.texture_uploads == 1);
    MK_REQUIRE(evidence.mesh_uploads == 1);
    MK_REQUIRE(evidence.skinned_mesh_uploads == 1);
    MK_REQUIRE(evidence.morph_mesh_uploads == 1);
    MK_REQUIRE(evidence.texture_bindings == 1);
    MK_REQUIRE(evidence.mesh_bindings == 1);
    MK_REQUIRE(evidence.skinned_mesh_bindings == 1);
    MK_REQUIRE(evidence.morph_mesh_bindings == 1);
    MK_REQUIRE(evidence.staging_pool_leases == 4);
    MK_REQUIRE(evidence.ring_backed_uploads == 4);
    MK_REQUIRE(evidence.resource_updates_ready);
    MK_REQUIRE(evidence.resource_updates == 4);
    MK_REQUIRE(evidence.resource_update_submitted_fences == 4);
    MK_REQUIRE(evidence.resource_update_graphics_ready_updates == 4);
    MK_REQUIRE(evidence.resource_update_graphics_queue_waits_recorded == 3);
    MK_REQUIRE(evidence.resource_update_same_queue_graphics_updates == 1);
    MK_REQUIRE(evidence.uploaded_bytes > 0);
    MK_REQUIRE(evidence.submitted_fences == 4);
    MK_REQUIRE(evidence.upload_queue_waits_recorded == 3);
    MK_REQUIRE(evidence.copy_queue_submits == 3);
    MK_REQUIRE(evidence.graphics_queue_submits == 1);
    MK_REQUIRE(evidence.queue_waits == 3);
    MK_REQUIRE(evidence.fence_waits == 0);
    MK_REQUIRE(evidence.graphics_waited_for_copy);
}

MK_TEST("runtime package resource update readiness publishes rows after upload fences are graphics ready") {
    const auto texture = mirakana::AssetId::from_name("package/resource_update/texture");
    const auto mesh = mirakana::AssetId::from_name("package/resource_update/static_mesh");
    const auto skinned_mesh = mirakana::AssetId::from_name("package/resource_update/skinned_mesh");
    const auto morph_mesh = mirakana::AssetId::from_name("package/resource_update/morph_mesh");
    const auto texture_handle = mirakana::runtime::RuntimeAssetHandle{.value = 50};
    const auto mesh_handle = mirakana::runtime::RuntimeAssetHandle{.value = 51};
    const auto skinned_mesh_handle = mirakana::runtime::RuntimeAssetHandle{.value = 52};
    const auto morph_mesh_handle = mirakana::runtime::RuntimeAssetHandle{.value = 53};
    const auto catalog = make_runtime_catalog({
        make_runtime_texture_record(texture, texture_handle),
        make_runtime_mesh_record(mesh, mesh_handle),
        make_runtime_skinned_mesh_record(skinned_mesh, skinned_mesh_handle),
        make_runtime_morph_mesh_record(morph_mesh, morph_mesh_handle),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing texture_ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::rhi::RhiUploadRing mesh_ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::rhi::RhiUploadRing skinned_mesh_ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 768, .min_alignment = 256, .buffer = {}});
    mirakana::rhi::RhiUploadRing morph_mesh_ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});

    mirakana::runtime_rhi::RuntimeTextureUploadOptions texture_options;
    texture_options.upload_ring = &texture_ring;
    texture_options.queue = mirakana::rhi::QueueKind::graphics;
    texture_options.wait_for_completion = false;
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.upload_ring = &mesh_ring;
    mesh_options.queue = mirakana::rhi::QueueKind::copy;
    mesh_options.wait_for_completion = false;
    mirakana::runtime_rhi::RuntimeSkinnedMeshUploadOptions skinned_mesh_options;
    skinned_mesh_options.upload_ring = &skinned_mesh_ring;
    skinned_mesh_options.queue = mirakana::rhi::QueueKind::copy;
    skinned_mesh_options.wait_for_completion = false;
    mirakana::runtime_rhi::RuntimeMorphMeshUploadOptions morph_mesh_options;
    morph_mesh_options.upload_ring = &morph_mesh_ring;
    morph_mesh_options.queue = mirakana::rhi::QueueKind::copy;
    morph_mesh_options.wait_for_completion = false;

    const auto texture_payload = make_runtime_texture_payload(texture, texture_handle);
    const auto mesh_payload = make_runtime_mesh_payload(mesh, mesh_handle);
    const auto skinned_mesh_payload = make_runtime_skinned_mesh_payload(skinned_mesh, skinned_mesh_handle);
    const auto morph_mesh_payload = make_runtime_morph_mesh_payload(morph_mesh, morph_mesh_handle);

    const auto texture_upload = mirakana::runtime_rhi::upload_runtime_package_streaming_frame_graph_texture_bindings(
        device, streaming, catalog,
        std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource>{
            mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureUploadSource{
                .asset = texture,
                .resource = "package.resource_update.texture",
                .payload = &texture_payload,
                .upload_options = texture_options,
                .current_state = mirakana::rhi::ResourceState::shader_read,
            },
        });
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource>{
            mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{
                .asset = mesh,
                .payload = &mesh_payload,
                .upload_options = mesh_options,
            },
        });
    const auto skinned_mesh_upload = mirakana::runtime_rhi::upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource>{
            mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource{
                .asset = skinned_mesh,
                .payload = &skinned_mesh_payload,
                .upload_options = skinned_mesh_options,
            },
        });
    const auto morph_mesh_upload = mirakana::runtime_rhi::upload_runtime_package_streaming_morph_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource>{
            mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource{
                .asset = morph_mesh,
                .payload = &morph_mesh_payload,
                .upload_options = morph_mesh_options,
            },
        });

    const auto readiness = mirakana::runtime_rhi::make_runtime_package_resource_update_readiness(
        streaming, catalog,
        mirakana::runtime_rhi::RuntimePackageResourceUpdateReadinessSources{
            .texture_uploads = &texture_upload,
            .mesh_uploads = &mesh_upload,
            .skinned_mesh_uploads = &skinned_mesh_upload,
            .morph_mesh_uploads = &morph_mesh_upload,
        });

    MK_REQUIRE(readiness.succeeded());
    MK_REQUIRE(readiness.ready);
    MK_REQUIRE(readiness.updates.size() == 4);
    MK_REQUIRE(readiness.texture_updates == 1);
    MK_REQUIRE(readiness.mesh_updates == 1);
    MK_REQUIRE(readiness.skinned_mesh_updates == 1);
    MK_REQUIRE(readiness.morph_mesh_updates == 1);
    MK_REQUIRE(readiness.submitted_fences == 4);
    MK_REQUIRE(readiness.graphics_queue_ready_updates == 4);
    MK_REQUIRE(readiness.graphics_queue_waits_recorded == 3);
    MK_REQUIRE(readiness.same_queue_graphics_updates == 1);
    MK_REQUIRE(readiness.updates[0].asset == texture);
    MK_REQUIRE(readiness.updates[0].kind == mirakana::runtime_rhi::RuntimePackageResourceUpdateKind::texture);
    MK_REQUIRE(readiness.updates[0].package_handle == texture_handle);
    MK_REQUIRE(readiness.updates[0].submitted_upload_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(readiness.updates[0].graphics_queue_ready);
    MK_REQUIRE(readiness.updates[0].same_queue_graphics_order);
}

MK_TEST("runtime package resource update readiness rejects async copy upload without graphics wait") {
    const auto mesh = mirakana::AssetId::from_name("package/resource_update/missing_wait_mesh");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 54};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    upload_options.queue = mirakana::rhi::QueueKind::copy;
    upload_options.wait_for_completion = false;
    const auto payload = make_runtime_mesh_payload(mesh, handle);
    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource>{
            mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{
                .asset = mesh,
                .payload = &payload,
                .upload_options = upload_options,
            },
        });
    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.upload_queue_waits_recorded == 1);
    transaction.upload_queue_waits_recorded = 0;

    const auto readiness = mirakana::runtime_rhi::make_runtime_package_resource_update_readiness(
        streaming, catalog,
        mirakana::runtime_rhi::RuntimePackageResourceUpdateReadinessSources{.mesh_uploads = &transaction});

    MK_REQUIRE(!readiness.succeeded());
    MK_REQUIRE(!readiness.ready);
    MK_REQUIRE(readiness.updates.empty());
    MK_REQUIRE(readiness.diagnostics.size() == 1);
    MK_REQUIRE(readiness.diagnostics[0].asset == mesh);
    MK_REQUIRE(readiness.diagnostics[0].code == "resource-update-upload-not-waited");
}

MK_TEST("runtime package streaming mesh upload transaction waits graphics queue for async copy upload") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/async_copy_triangle");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 31};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    upload_options.queue = mirakana::rhi::QueueKind::copy;
    upload_options.wait_for_completion = false;
    const auto payload = make_runtime_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{
            .asset = mesh,
            .payload = &payload,
            .upload_options = upload_options,
        },
    };

    auto transaction =
        mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 1);
    MK_REQUIRE(transaction.mesh_bindings.size() == 1);
    MK_REQUIRE(transaction.submitted_fences.size() == 1);
    MK_REQUIRE(transaction.submitted_fences.front().queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(transaction.upload_queue_waits_recorded == 1);
    MK_REQUIRE(transaction.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(device.stats().copy_queue_submits == 1);
    MK_REQUIRE(device.stats().graphics_queue_submits == 0);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().queue_waits == 1);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_value == transaction.submitted_fences.front().value);
}

MK_TEST("runtime package streaming mesh upload transaction rejects missing mesh payload before upload") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/missing_payload");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 4};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{.asset = mesh},
    };

    auto transaction =
        mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(device, streaming, catalog, sources);

    MK_REQUIRE(!transaction.succeeded());
    MK_REQUIRE(transaction.uploads.empty());
    MK_REQUIRE(transaction.mesh_bindings.empty());
    MK_REQUIRE(transaction.diagnostics.size() == 1);
    MK_REQUIRE(transaction.diagnostics[0].asset == mesh);
    MK_REQUIRE(transaction.diagnostics[0].code == "mesh-payload-missing");
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime package streaming mesh upload transaction rejects non mesh resident rows before upload") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/wrong_kind");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 5};
    const auto catalog = make_runtime_catalog({make_runtime_texture_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto payload = make_runtime_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMeshUploadSource{.asset = mesh, .payload = &payload},
    };

    auto transaction =
        mirakana::runtime_rhi::upload_runtime_package_streaming_mesh_gpu_bindings(device, streaming, catalog, sources);

    MK_REQUIRE(!transaction.succeeded());
    MK_REQUIRE(transaction.uploads.empty());
    MK_REQUIRE(transaction.mesh_bindings.empty());
    MK_REQUIRE(transaction.diagnostics.size() == 1);
    MK_REQUIRE(transaction.diagnostics[0].asset == mesh);
    MK_REQUIRE(transaction.diagnostics[0].code == "runtime-resource-not-mesh");
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST(
    "runtime package streaming skinned mesh upload transaction uploads resident skinned mesh with async copy wait") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/skinned_copy_queue");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 41};
    const auto catalog = make_runtime_catalog({make_runtime_skinned_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 768, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeSkinnedMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    upload_options.queue = mirakana::rhi::QueueKind::copy;
    upload_options.wait_for_completion = false;
    const auto payload = make_runtime_skinned_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource{
            .asset = mesh,
            .payload = &payload,
            .upload_options = upload_options,
        },
    };
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 1);
    MK_REQUIRE(transaction.skinned_mesh_bindings.size() == 1);
    MK_REQUIRE(transaction.uploaded_bytes == 484);
    MK_REQUIRE(transaction.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(transaction.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(transaction.submitted_fences.size() == 1);
    MK_REQUIRE(transaction.submitted_fences.front().queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(transaction.upload_queue_waits_recorded == 1);
    MK_REQUIRE(transaction.uploads[0].vertex_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].index_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].joint_palette_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].upload_buffers_caller_owned);
    MK_REQUIRE(transaction.skinned_mesh_bindings[0].asset == mesh);
    MK_REQUIRE(transaction.skinned_mesh_bindings[0].binding.mesh.vertex_buffer.value ==
               transaction.uploads[0].vertex_buffer.value);
    MK_REQUIRE(transaction.skinned_mesh_bindings[0].binding.joint_palette_buffer.value ==
               transaction.uploads[0].joint_palette_buffer.value);
    MK_REQUIRE(transaction.skinned_mesh_bindings[0].binding.owner_device == &device);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation + 3);
    MK_REQUIRE(device.stats().buffer_writes == 3);
    MK_REQUIRE(device.stats().buffer_copies == 3);
    MK_REQUIRE(device.stats().copy_queue_submits == 1);
    MK_REQUIRE(device.stats().graphics_queue_submits == 0);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().queue_waits == 1);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_value == transaction.submitted_fences.front().value);
}

MK_TEST("runtime package streaming morph mesh upload transaction uploads resident morph mesh with async copy wait") {
    const auto morph_asset = mirakana::AssetId::from_name("morphs/streamed/copy_queue_delta");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 42};
    const auto catalog = make_runtime_catalog({make_runtime_morph_mesh_record(morph_asset, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    mirakana::runtime_rhi::RuntimeMorphMeshUploadOptions upload_options;
    upload_options.upload_ring = &ring;
    upload_options.queue = mirakana::rhi::QueueKind::copy;
    upload_options.wait_for_completion = false;
    const auto payload = make_runtime_morph_mesh_payload(morph_asset, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource{
            .asset = morph_asset,
            .payload = &payload,
            .upload_options = upload_options,
        },
    };
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_morph_mesh_gpu_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(transaction.succeeded());
    MK_REQUIRE(transaction.uploads.size() == 1);
    MK_REQUIRE(transaction.morph_mesh_bindings.size() == 1);
    MK_REQUIRE(transaction.uploaded_bytes == 292);
    MK_REQUIRE(transaction.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(transaction.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(transaction.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(transaction.submitted_fences.size() == 1);
    MK_REQUIRE(transaction.submitted_fences.front().queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(transaction.upload_queue_waits_recorded == 1);
    MK_REQUIRE(transaction.uploads[0].position_delta_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].morph_weight_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(transaction.uploads[0].upload_buffers_caller_owned);
    MK_REQUIRE(transaction.morph_mesh_bindings[0].asset == morph_asset);
    MK_REQUIRE(transaction.morph_mesh_bindings[0].binding.position_delta_buffer.value ==
               transaction.uploads[0].position_delta_buffer.value);
    MK_REQUIRE(transaction.morph_mesh_bindings[0].binding.morph_weight_buffer.value ==
               transaction.uploads[0].morph_weight_buffer.value);
    MK_REQUIRE(transaction.morph_mesh_bindings[0].binding.owner_device == &device);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation + 2);
    MK_REQUIRE(device.stats().buffer_writes == 2);
    MK_REQUIRE(device.stats().buffer_copies == 2);
    MK_REQUIRE(device.stats().copy_queue_submits == 1);
    MK_REQUIRE(device.stats().graphics_queue_submits == 0);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().queue_waits == 1);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_value == transaction.submitted_fences.front().value);
}

MK_TEST("runtime package streaming skinned mesh upload transaction rejects non skinned resident rows before upload") {
    const auto mesh = mirakana::AssetId::from_name("meshes/streamed/skinned_wrong_kind");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 43};
    const auto catalog = make_runtime_catalog({make_runtime_mesh_record(mesh, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto payload = make_runtime_skinned_mesh_payload(mesh, handle);
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingSkinnedMeshUploadSource{.asset = mesh, .payload = &payload},
    };

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(!transaction.succeeded());
    MK_REQUIRE(transaction.uploads.empty());
    MK_REQUIRE(transaction.skinned_mesh_bindings.empty());
    MK_REQUIRE(transaction.diagnostics.size() == 1);
    MK_REQUIRE(transaction.diagnostics[0].asset == mesh);
    MK_REQUIRE(transaction.diagnostics[0].code == "runtime-resource-not-skinned-mesh");
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime package streaming morph mesh upload transaction rejects missing morph payload before upload") {
    const auto morph_asset = mirakana::AssetId::from_name("morphs/streamed/missing_payload");
    const auto handle = mirakana::runtime::RuntimeAssetHandle{.value = 44};
    const auto catalog = make_runtime_catalog({make_runtime_morph_mesh_record(morph_asset, handle)});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingMorphMeshUploadSource{.asset = morph_asset},
    };

    auto transaction = mirakana::runtime_rhi::upload_runtime_package_streaming_morph_mesh_gpu_bindings(
        device, streaming, catalog, sources);

    MK_REQUIRE(!transaction.succeeded());
    MK_REQUIRE(transaction.uploads.empty());
    MK_REQUIRE(transaction.morph_mesh_bindings.empty());
    MK_REQUIRE(transaction.diagnostics.size() == 1);
    MK_REQUIRE(transaction.diagnostics[0].asset == morph_asset);
    MK_REQUIRE(transaction.diagnostics[0].code == "morph-mesh-payload-missing");
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime rhi upload creates mesh buffers and records vertex index byte uploads") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const std::vector<std::uint8_t> vertex_bytes(36, std::uint8_t{0x7a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{2},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);
    const auto uploaded_vertices = device.read_buffer(result.vertex_buffer, 0, result.uploaded_vertex_bytes);
    const auto uploaded_indices = device.read_buffer(result.index_buffer, 0, result.uploaded_index_bytes);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.vertex_buffer.value != 0);
    MK_REQUIRE(result.index_buffer.value != 0);
    MK_REQUIRE(result.vertex_upload_buffer.value != 0);
    MK_REQUIRE(result.index_upload_buffer.value != 0);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(result.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(result.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(result.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(result.vertex_count == 3);
    MK_REQUIRE(result.index_count == 3);
    MK_REQUIRE(result.vertex_stride == 12);
    MK_REQUIRE(result.index_format == mirakana::rhi::IndexFormat::uint32);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.uploaded_vertex_bytes == 36);
    MK_REQUIRE(result.uploaded_index_bytes == 12);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.vertex_copy_region.size_bytes == 36);
    MK_REQUIRE(result.index_copy_region.size_bytes == 12);
    MK_REQUIRE(uploaded_vertices == vertex_bytes);
    MK_REQUIRE(uploaded_indices == index_bytes);
    MK_REQUIRE(mirakana::rhi::has_flag(result.vertex_buffer_desc.usage, mirakana::rhi::BufferUsage::vertex));
    MK_REQUIRE(mirakana::rhi::has_flag(result.vertex_buffer_desc.usage, mirakana::rhi::BufferUsage::copy_destination));
    MK_REQUIRE(mirakana::rhi::has_flag(result.index_buffer_desc.usage, mirakana::rhi::BufferUsage::index));
    MK_REQUIRE(mirakana::rhi::has_flag(result.index_buffer_desc.usage, mirakana::rhi::BufferUsage::copy_destination));

    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == 4);
    MK_REQUIRE(stats.buffer_copies == 2);
    MK_REQUIRE(stats.bytes_copied == 48);
    MK_REQUIRE(stats.buffer_writes == 2);
    MK_REQUIRE(stats.bytes_written == 48);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);

    const auto mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(result);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == result.vertex_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == result.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == result.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == result.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == result.vertex_stride);
    MK_REQUIRE(mesh_binding.index_format == result.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

MK_TEST("runtime rhi mesh upload can use caller owned upload ring staging") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256, .buffer = {}});
    const auto mesh = mirakana::AssetId::from_name("meshes/ring_triangle");
    const std::vector<std::uint8_t> vertex_bytes(36, std::uint8_t{0x44});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{33},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.vertex_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(result.index_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(result.upload_buffers_caller_owned);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(result.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(result.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(result.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(device.read_buffer(result.vertex_buffer, 0, result.uploaded_vertex_bytes) == vertex_bytes);
    MK_REQUIRE(device.read_buffer(result.index_buffer, 0, result.uploaded_index_bytes) == index_bytes);
    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == buffers_after_ring_creation + 2);
    MK_REQUIRE(stats.buffer_writes == 2);
    MK_REQUIRE(stats.bytes_written == 48);
    MK_REQUIRE(stats.buffer_copies == 2);
    MK_REQUIRE(stats.bytes_copied == 48);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);
}

MK_TEST("runtime rhi mesh upload rejects ring staging exhaustion before side effects") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 32, .min_alignment = 256, .buffer = {}});
    const auto mesh = mirakana::AssetId::from_name("meshes/ring_triangle_too_small");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{34},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x45}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x02}),
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload, options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("ring") != std::string::npos);
    MK_REQUIRE(result.vertex_buffer.value == 0);
    MK_REQUIRE(result.index_buffer.value == 0);
    MK_REQUIRE(result.vertex_upload_buffer.value == 0);
    MK_REQUIRE(result.index_upload_buffer.value == 0);
    MK_REQUIRE(device.stats().buffers_created == buffers_after_ring_creation);
    MK_REQUIRE(device.stats().buffer_writes == 0);
    MK_REQUIRE(device.stats().buffer_copies == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime rhi derives tangent-space mesh vertex layout from cooked mesh metadata") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/lit_triangle");
    const std::vector<std::uint8_t> vertex_bytes(144, std::uint8_t{0x5a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{20},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };

    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload);
    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(layout.succeeded());
    MK_REQUIRE(layout.layout == mirakana::runtime_rhi::RuntimeMeshVertexLayout::position_normal_uv_tangent);
    MK_REQUIRE(layout.vertex_stride == mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);
    MK_REQUIRE(layout.vertex_buffers.size() == 1);
    MK_REQUIRE(layout.vertex_buffers[0].stride == 48);
    MK_REQUIRE(layout.vertex_attributes.size() == 4);
    MK_REQUIRE(layout.vertex_attributes[0].semantic == mirakana::rhi::VertexSemantic::position);
    MK_REQUIRE(layout.vertex_attributes[0].offset == 0);
    MK_REQUIRE(layout.vertex_attributes[0].format == mirakana::rhi::VertexFormat::float32x3);
    MK_REQUIRE(layout.vertex_attributes[1].semantic == mirakana::rhi::VertexSemantic::normal);
    MK_REQUIRE(layout.vertex_attributes[1].offset == 12);
    MK_REQUIRE(layout.vertex_attributes[1].format == mirakana::rhi::VertexFormat::float32x3);
    MK_REQUIRE(layout.vertex_attributes[2].semantic == mirakana::rhi::VertexSemantic::texcoord);
    MK_REQUIRE(layout.vertex_attributes[2].offset == 24);
    MK_REQUIRE(layout.vertex_attributes[2].format == mirakana::rhi::VertexFormat::float32x2);
    MK_REQUIRE(layout.vertex_attributes[3].semantic == mirakana::rhi::VertexSemantic::tangent);
    MK_REQUIRE(layout.vertex_attributes[3].offset == 32);
    MK_REQUIRE(layout.vertex_attributes[3].format == mirakana::rhi::VertexFormat::float32x4);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.vertex_stride == 48);
    MK_REQUIRE(result.uploaded_vertex_bytes == 144);
    MK_REQUIRE(device.read_buffer(result.vertex_buffer, 0, result.uploaded_vertex_bytes) == vertex_bytes);
}

MK_TEST("runtime rhi upload defaults to graphics queue for backend neutral upload commands") {
    const mirakana::runtime_rhi::RuntimeTextureUploadOptions texture_options;
    const mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;

    MK_REQUIRE(texture_options.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(mesh_options.queue == mirakana::rhi::QueueKind::graphics);
}

MK_TEST("runtime rhi upload rejects mesh payloads without vertex and index bytes") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/metadata_only");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{3},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = {},
        .index_bytes = {},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("byte") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_copies == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime rhi upload rejects invalid mesh payload metadata before creating buffers") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/broken");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{4},
        .vertex_count = 0,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = {0x00, 0x01, 0x02},
        .index_bytes = {0x03, 0x04, 0x05},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects mesh bytes that do not match draw binding metadata") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/misaligned");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{5},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(35, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(result).vertex_buffer.value == 0);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects partial lit mesh vertex layouts before creating buffers") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/partial_lit_triangle");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{21},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(72, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };

    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload);
    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!layout.succeeded());
    MK_REQUIRE(layout.diagnostic.find("tangent_frame") != std::string::npos);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects partial lit mesh vertex layouts when derivation is disabled") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/manual_partial_lit_triangle");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{22},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions options;
    options.derive_vertex_layout_from_payload = false;
    options.vertex_stride = mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes;

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload, options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime material gpu binding creates descriptor resources for material textures") {
    mirakana::rhi::NullRhiDevice device;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player_albedo");
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 2, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &device}});

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(binding.descriptor_set_layout.value == 1);
    MK_REQUIRE(binding.descriptor_set.value == 1);
    MK_REQUIRE(binding.uniform_buffer.value == 1);
    MK_REQUIRE(binding.owner_device == &device);
    MK_REQUIRE(binding.samplers.size() == 1);
    MK_REQUIRE(binding.writes.size() == 4);
    MK_REQUIRE(binding.writes[0].binding == 0);
    MK_REQUIRE(binding.writes[0].resources[0].type == mirakana::rhi::DescriptorType::uniform_buffer);
    MK_REQUIRE(binding.writes[1].binding == 6);
    MK_REQUIRE(binding.writes[1].resources[0].type == mirakana::rhi::DescriptorType::uniform_buffer);
    MK_REQUIRE(binding.writes[2].binding == 1);
    MK_REQUIRE(binding.writes[2].resources[0].type == mirakana::rhi::DescriptorType::sampled_texture);
    MK_REQUIRE(binding.writes[3].binding == 16);
    MK_REQUIRE(binding.writes[3].resources[0].type == mirakana::rhi::DescriptorType::sampler);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.samplers_created == 1);
    MK_REQUIRE(stats.buffers_created == 3);
    MK_REQUIRE(stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(stats.descriptor_writes == 4);
}

MK_TEST("runtime material gpu binding uploads material factors into uniform buffer") {
    mirakana::rhi::NullRhiDevice device;
    const auto material_id = mirakana::AssetId::from_name("materials/factors");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Factors",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.25F, 0.5F, 0.75F, 1.0F},
                .emissive = {2.0F, 3.0F, 4.0F},
                .metallic = 0.125F,
                .roughness = 0.875F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding =
        mirakana::runtime_rhi::create_runtime_material_gpu_binding(device, metadata, material.factors, {});
    const auto bytes = device.read_buffer(binding.uniform_buffer, 0,
                                          mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    const auto allocation_bytes = device.read_buffer(binding.uniform_buffer, 0, 256);

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(bytes.size() == mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    MK_REQUIRE(allocation_bytes.size() == 256);
    MK_REQUIRE(read_float(bytes, 0) == 0.25F);
    MK_REQUIRE(read_float(bytes, 4) == 0.5F);
    MK_REQUIRE(read_float(bytes, 8) == 0.75F);
    MK_REQUIRE(read_float(bytes, 12) == 1.0F);
    MK_REQUIRE(read_float(bytes, 16) == 2.0F);
    MK_REQUIRE(read_float(bytes, 20) == 3.0F);
    MK_REQUIRE(read_float(bytes, 24) == 4.0F);
    MK_REQUIRE(read_float(bytes, 28) == 0.125F);
    MK_REQUIRE(read_float(bytes, 32) == 0.875F);
    MK_REQUIRE(binding.factor_bytes_uploaded == mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    MK_REQUIRE(binding.submitted_fence.value != 0);
    MK_REQUIRE(binding.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(binding.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(binding.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(binding.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(binding.frame_graph_pass_callbacks_invoked == 1);

    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == 3);
    MK_REQUIRE(stats.buffer_writes == 2);
    MK_REQUIRE(stats.buffer_copies == 1);
}

MK_TEST("runtime material gpu binding can allocate descriptors from a caller owned layout") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture_id = mirakana::AssetId::from_name("textures/external_layout");
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/external_layout"),
        .name = "ExternalLayout",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto layout_desc = mirakana::runtime_rhi::make_runtime_material_descriptor_set_layout_desc(metadata);
    MK_REQUIRE(layout_desc.succeeded());
    const auto layout = device.create_descriptor_set_layout(layout_desc.desc);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &device}},
        mirakana::runtime_rhi::RuntimeMaterialGpuBindingOptions{.descriptor_set_layout = layout,
                                                                .create_descriptor_set_layout = false});

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(binding.descriptor_set_layout.value == layout.value);
    MK_REQUIRE(binding.descriptor_set.value != 0);
    MK_REQUIRE(binding.writes.size() == 4);
    MK_REQUIRE(device.stats().descriptor_set_layouts_created == 1);
    MK_REQUIRE(device.stats().descriptor_sets_allocated == 1);
    MK_REQUIRE(device.stats().descriptor_writes == 4);
}

MK_TEST("runtime material gpu binding rejects texture resources from another rhi device") {
    mirakana::rhi::NullRhiDevice texture_device;
    mirakana::rhi::NullRhiDevice material_device;
    const auto texture = texture_device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/cross_device"),
        .name = "CrossDevice",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{
            .slot = mirakana::MaterialTextureSlot::base_color,
            .texture = mirakana::AssetId::from_name("textures/cross_device")}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        material_device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &texture_device}});

    MK_REQUIRE(!binding.succeeded());
    MK_REQUIRE(binding.diagnostic.find("different rhi device") != std::string::npos);
    MK_REQUIRE(material_device.stats().descriptor_set_layouts_created == 0);
}

MK_TEST("runtime material gpu binding rejects unsupported nonzero material descriptor sets") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::MaterialPipelineBindingMetadata metadata;
    metadata.material = mirakana::AssetId::from_name("materials/nonzero_set");
    metadata.shading_model = mirakana::MaterialShadingModel::lit;
    metadata.surface_mode = mirakana::MaterialSurfaceMode::opaque;
    metadata.bindings.push_back(mirakana::MaterialPipelineBinding{
        .set = 1,
        .binding = 0,
        .resource_kind = mirakana::MaterialBindingResourceKind::uniform_buffer,
        .stages = mirakana::MaterialShaderStageMask::fragment,
        .texture_slot = mirakana::MaterialTextureSlot::unknown,
        .semantic = "material.factors",
    });

    const auto binding =
        mirakana::runtime_rhi::create_runtime_material_gpu_binding(device, metadata, mirakana::MaterialFactors{}, {});

    MK_REQUIRE(!binding.succeeded());
    MK_REQUIRE(binding.diagnostic.find("set 0") != std::string::npos);
    MK_REQUIRE(device.stats().descriptor_set_layouts_created == 0);
}

MK_TEST("runtime rhi skinned mesh upload binds joint palette descriptor set") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/skinned_runtime_upload");
    std::vector<std::uint8_t> vertex_bytes(216, std::uint8_t{0x2a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> palette(64, std::uint8_t{0});
    palette[0] = 0x00;
    palette[1] = 0x00;
    palette[2] = 0x80;
    palette[3] = 0x3f; // float 1.0 LE at m00
    palette[20] = 0x00;
    palette[21] = 0x00;
    palette[22] = 0x80;
    palette[23] = 0x3f; // m11
    palette[40] = 0x00;
    palette[41] = 0x00;
    palette[42] = 0x80;
    palette[43] = 0x3f; // m22
    palette[60] = 0x00;
    palette[61] = 0x00;
    palette[62] = 0x80;
    palette[63] = 0x3f; // m33
    const mirakana::runtime::RuntimeSkinnedMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{9},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = palette,
    };

    const auto upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.submitted_fence.value != 0);
    MK_REQUIRE(upload.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);
    auto binding = mirakana::runtime_rhi::make_runtime_skinned_mesh_gpu_binding(upload);
    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.joint_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi skinned mesh upload can use caller owned upload ring staging") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 768, .min_alignment = 256, .buffer = {}});
    const auto mesh = mirakana::AssetId::from_name("meshes/ring_skinned_runtime_upload");
    const std::vector<std::uint8_t> vertex_bytes(216, std::uint8_t{0x2b});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> palette(64, std::uint8_t{0});
    palette[0] = 0x00;
    palette[1] = 0x00;
    palette[2] = 0x80;
    palette[3] = 0x3f;
    const mirakana::runtime::RuntimeSkinnedMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{35},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = palette,
    };
    mirakana::runtime_rhi::RuntimeSkinnedMeshUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    const auto upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(device, payload, options);

    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.vertex_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.index_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.joint_palette_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.upload_buffers_caller_owned);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(upload.uploaded_vertex_bytes == 216);
    MK_REQUIRE(upload.uploaded_index_bytes == 12);
    MK_REQUIRE(upload.uploaded_joint_palette_bytes == 256);
    MK_REQUIRE(device.read_buffer(upload.vertex_buffer, 0, upload.uploaded_vertex_bytes) == vertex_bytes);
    MK_REQUIRE(device.read_buffer(upload.index_buffer, 0, upload.uploaded_index_bytes) == index_bytes);
    MK_REQUIRE(device.read_buffer(upload.joint_palette_buffer, 0, palette.size()) == palette);
    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == buffers_after_ring_creation + 3);
    MK_REQUIRE(stats.buffer_writes == 3);
    MK_REQUIRE(stats.bytes_written == 484);
    MK_REQUIRE(stats.buffer_copies == 3);
    MK_REQUIRE(stats.bytes_copied == 484);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);
}

MK_TEST("runtime rhi composes compute morph output with skinned mesh attributes") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> skinned_vertex_bytes;
    const auto append_skinned_vertex = [&skinned_vertex_bytes](float x, float y, float z) {
        append_vec3(skinned_vertex_bytes, x, y, z);
        append_vec3(skinned_vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(skinned_vertex_bytes, 0.5F);
        append_le_f32(skinned_vertex_bytes, 0.5F);
        append_vec3(skinned_vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(skinned_vertex_bytes, 1.0F);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_f32(skinned_vertex_bytes, 1.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
    };
    append_skinned_vertex(-1.4F, 0.6F, 0.0F);
    append_skinned_vertex(-0.7F, -0.6F, 0.0F);
    append_skinned_vertex(-1.9F, -0.6F, 0.0F);
    MK_REQUIRE(skinned_vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    std::vector<std::uint8_t> joint_palette_bytes(mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes,
                                                  std::uint8_t{0});
    const mirakana::runtime::RuntimeSkinnedMeshPayload skinned_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_skinned_runtime_upload"),
        .handle = mirakana::runtime::RuntimeAssetHandle{21},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = skinned_vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = joint_palette_bytes,
    };
    const auto skinned_upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(device, skinned_payload);
    MK_REQUIRE(skinned_upload.succeeded());

    std::vector<std::uint8_t> position_bytes;
    append_vec3(position_bytes, -1.4F, 0.6F, 0.0F);
    append_vec3(position_bytes, -0.7F, -0.6F, 0.0F);
    append_vec3(position_bytes, -1.9F, -0.6F, 0.0F);
    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = skinned_payload.asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{22},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = position_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = position_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    }
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_skinned_runtime_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{23},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    auto binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_skinned_mesh_gpu_binding(skinned_upload, compute_binding);

    MK_REQUIRE(binding.mesh.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(binding.mesh.index_buffer.value == skinned_upload.index_buffer.value);
    MK_REQUIRE(binding.mesh.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(binding.skin_attribute_vertex_buffer.value == skinned_upload.vertex_buffer.value);
    MK_REQUIRE(binding.skin_attribute_vertex_stride == mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    MK_REQUIRE(binding.joint_palette_buffer.value == skinned_upload.joint_palette_buffer.value);
    MK_REQUIRE(binding.owner_device == &device);

    mirakana::rhi::DescriptorSetLayoutHandle joint_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(device, skinned_upload, binding, joint_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.joint_descriptor_set.value != 0);
    MK_REQUIRE(joint_layout.value != 0);
}

MK_TEST("runtime rhi morph mesh upload binds position delta storage and weight descriptor set") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_runtime_upload");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    MK_REQUIRE(read_float(morph.target_weight_bytes, 0) == 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{11}, .morph = morph};

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.vertex_count == 3);
    MK_REQUIRE(upload.target_count == 1);
    MK_REQUIRE(upload.uploaded_position_delta_bytes == 36);
    MK_REQUIRE(upload.morph_weight_uniform_allocation_bytes == 256);
    MK_REQUIRE(upload.submitted_fence.value != 0);
    MK_REQUIRE(upload.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(device.read_buffer(upload.position_delta_buffer, 0, upload.uploaded_position_delta_bytes) ==
               morph.targets.front().position_delta_bytes);
    MK_REQUIRE(device.read_buffer(upload.morph_weight_buffer, 0, morph.target_weight_bytes.size()) ==
               morph.target_weight_bytes);

    auto binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload);
    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.morph_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi morph mesh upload binds optional normal and tangent delta streams") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_runtime_upload_tangent_space");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{12}, .morph = morph};

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.uploaded_position_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_normal_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_tangent_delta_bytes == 36);
    MK_REQUIRE(upload.normal_delta_buffer.value != 0);
    MK_REQUIRE(upload.tangent_delta_buffer.value != 0);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(device.read_buffer(upload.normal_delta_buffer, 0, upload.uploaded_normal_delta_bytes) ==
               morph.targets.front().normal_delta_bytes);
    MK_REQUIRE(device.read_buffer(upload.tangent_delta_buffer, 0, upload.uploaded_tangent_delta_bytes) ==
               morph.targets.front().tangent_delta_bytes);

    auto binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload);
    MK_REQUIRE(binding.normal_delta_buffer.value == upload.normal_delta_buffer.value);
    MK_REQUIRE(binding.tangent_delta_buffer.value == upload.tangent_delta_buffer.value);
    MK_REQUIRE(binding.normal_delta_bytes == 36);
    MK_REQUIRE(binding.tangent_delta_bytes == 36);

    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.morph_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi morph mesh upload can use caller owned upload ring staging") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadRing ring(
        device, mirakana::rhi::RhiUploadRingDesc{.size_bytes = 1024, .min_alignment = 256, .buffer = {}});
    const auto mesh = mirakana::AssetId::from_name("meshes/ring_morph_runtime_upload");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const auto expected_position_delta_bytes = morph.targets.front().position_delta_bytes;
    const auto expected_normal_delta_bytes = morph.targets.front().normal_delta_bytes;
    const auto expected_tangent_delta_bytes = morph.targets.front().tangent_delta_bytes;

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{36}, .morph = morph};
    mirakana::runtime_rhi::RuntimeMorphMeshUploadOptions options;
    options.upload_ring = &ring;
    const auto buffers_after_ring_creation = device.stats().buffers_created;

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload, options);

    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.position_delta_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.normal_delta_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.tangent_delta_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.morph_weight_upload_buffer.value == ring.buffer().value);
    MK_REQUIRE(upload.upload_buffers_caller_owned);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);
    MK_REQUIRE(upload.uploaded_position_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_normal_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_tangent_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_weight_bytes == 256);
    MK_REQUIRE(device.read_buffer(upload.position_delta_buffer, 0, upload.uploaded_position_delta_bytes) ==
               expected_position_delta_bytes);
    MK_REQUIRE(device.read_buffer(upload.normal_delta_buffer, 0, upload.uploaded_normal_delta_bytes) ==
               expected_normal_delta_bytes);
    MK_REQUIRE(device.read_buffer(upload.tangent_delta_buffer, 0, upload.uploaded_tangent_delta_bytes) ==
               expected_tangent_delta_bytes);
    MK_REQUIRE(device.read_buffer(upload.morph_weight_buffer, 0, morph.target_weight_bytes.size()) ==
               morph.target_weight_bytes);
    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == buffers_after_ring_creation + 4);
    MK_REQUIRE(stats.buffer_writes == 4);
    MK_REQUIRE(stats.bytes_written == 364);
    MK_REQUIRE(stats.buffer_copies == 4);
    MK_REQUIRE(stats.bytes_copied == 364);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);
}

MK_TEST("runtime rhi morph mesh upload honors selected queue without forcing wait") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_runtime_upload_compute_queue");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.25F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.0F, 0.25F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.0F, 0.0F, 0.25F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 0.5F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{17}, .morph = morph};
    mirakana::runtime_rhi::RuntimeMorphMeshUploadOptions options;
    options.queue = mirakana::rhi::QueueKind::compute;
    options.wait_for_completion = false;

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload, options);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.submitted_fence.value != 0);
    MK_REQUIRE(upload.submitted_fence.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(upload.frame_graph_command_lists_submitted == 1);
    MK_REQUIRE(upload.frame_graph_queue_waits_recorded == 0);
    MK_REQUIRE(upload.frame_graph_barriers_recorded == 0);
    MK_REQUIRE(upload.frame_graph_pass_callbacks_invoked == 1);

    const auto stats = device.stats();
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.compute_queue_submits == 1);
    MK_REQUIRE(stats.fence_waits == 0);
    MK_REQUIRE(device.read_buffer(upload.position_delta_buffer, 0, upload.uploaded_position_delta_bytes) ==
               morph.targets.front().position_delta_bytes);
}

MK_TEST("runtime rhi creates compute morph binding for position output") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, 1.0F, 2.0F, 0.0F);
    append_vec3(vertex_bytes, -2.0F, 4.0F, 0.5F);
    append_vec3(vertex_bytes, 0.0F, -1.0F, 2.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_runtime_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{13},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_runtime_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{14},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    const auto compute_binding =
        mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(device, mesh_upload, morph_upload);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.descriptor_set_layout.value != 0);
    MK_REQUIRE(compute_binding.descriptor_set.value != 0);
    MK_REQUIRE(compute_binding.output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_position_bytes == 36);
    MK_REQUIRE(compute_binding.vertex_count == 3);
    MK_REQUIRE(compute_binding.target_count == 1);
    MK_REQUIRE(compute_binding.owner_device == &device);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
    MK_REQUIRE(device.stats().descriptor_writes >= 4);
}

MK_TEST("runtime rhi creates compute morph output ring with distinct position slots") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(vertex_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(vertex_bytes, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_output_ring_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{121},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_output_ring_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{122},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    compute_options.output_slot_count = 2;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_slots.size() == 2);
    MK_REQUIRE(compute_binding.output_slots[0].output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_slots[1].output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_slots[0].output_position_buffer.value !=
               compute_binding.output_slots[1].output_position_buffer.value);
    MK_REQUIRE(compute_binding.output_position_buffer.value ==
               compute_binding.output_slots[0].output_position_buffer.value);
    MK_REQUIRE(compute_binding.output_slots[1].output_position_bytes == 36);

    const auto slot_one_mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding, 1);
    MK_REQUIRE(slot_one_mesh_binding.vertex_buffer.value ==
               compute_binding.output_slots[1].output_position_buffer.value);
    MK_REQUIRE(slot_one_mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(slot_one_mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(slot_one_mesh_binding.owner_device == &device);

    const auto invalid_slot_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding, 2);
    MK_REQUIRE(invalid_slot_binding.vertex_buffer.value == 0);
}

MK_TEST("runtime rhi creates compute morph binding for normal and tangent outputs") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float px, float py, float pz) {
        append_vec3(vertex_bytes, px, py, pz);
        append_vec3(vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_vec3(vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);

    std::vector<std::uint8_t> bind_positions;
    append_vec3(bind_positions, -0.6F, 0.75F, 0.0F);
    append_vec3(bind_positions, 0.15F, -0.75F, 0.0F);
    append_vec3(bind_positions, -1.35F, -0.75F, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_tangent_frame_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{19},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
        append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    }
    mirakana::MorphMeshCpuTargetSourceDocument target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(target.position_delta_bytes, 0.0F, 0.0F, 0.0F);
        append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
        append_vec3(target.tangent_delta_bytes, -1.0F, 0.0F, 1.0F);
    }
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_tangent_frame_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{20},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.normal_delta_buffer.value != 0);
    MK_REQUIRE(morph_upload.tangent_delta_buffer.value != 0);

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    compute_options.output_normal_usage = mirakana::rhi::BufferUsage::storage |
                                          mirakana::rhi::BufferUsage::copy_source | mirakana::rhi::BufferUsage::vertex;
    compute_options.output_tangent_usage = mirakana::rhi::BufferUsage::storage |
                                           mirakana::rhi::BufferUsage::copy_source | mirakana::rhi::BufferUsage::vertex;

    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_normal_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_tangent_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_position_bytes == 36);
    MK_REQUIRE(compute_binding.output_normal_bytes == 36);
    MK_REQUIRE(compute_binding.output_tangent_bytes == 36);
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_normal_buffer_desc.usage, mirakana::rhi::BufferUsage::storage));
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_tangent_buffer_desc.usage, mirakana::rhi::BufferUsage::storage));

    const auto mesh_binding = mirakana::runtime_rhi::make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(
        mesh_upload, compute_binding);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.normal_vertex_buffer.value == compute_binding.output_normal_buffer.value);
    MK_REQUIRE(mesh_binding.tangent_vertex_buffer.value == compute_binding.output_tangent_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.normal_vertex_stride == mirakana::runtime_rhi::runtime_morph_normal_delta_stride_bytes);
    MK_REQUIRE(mesh_binding.tangent_vertex_stride == mirakana::runtime_rhi::runtime_morph_tangent_delta_stride_bytes);
    MK_REQUIRE(mesh_binding.owner_device == &device);
    MK_REQUIRE(device.stats().descriptor_writes >= 8);
}

MK_TEST("runtime rhi exposes compute morph output as mesh binding") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(vertex_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(vertex_bytes, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_renderer_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{15},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_renderer_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{16},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_position_buffer_desc.usage, mirakana::rhi::BufferUsage::vertex));

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);

    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == mesh_upload.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.index_format == mesh_upload.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

MK_TEST("runtime rhi exposes interleaved compute morph output as position mesh binding") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float px, float py, float pz) {
        append_vec3(vertex_bytes, px, py, pz);
        append_vec3(vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_vec3(vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> bind_positions;
    append_vec3(bind_positions, -0.6F, 0.75F, 0.0F);
    append_vec3(bind_positions, 0.15F, -0.75F, 0.0F);
    append_vec3(bind_positions, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_interleaved_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{17},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());
    MK_REQUIRE(mesh_upload.vertex_stride == mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_interleaved_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{18},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);

    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == mesh_upload.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.index_format == mesh_upload.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

int main() {
    return mirakana::test::run_all();
}
