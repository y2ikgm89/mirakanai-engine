// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::string hex_bytes(std::size_t count, std::string_view byte) {
    std::string encoded;
    encoded.reserve(count * 2U);
    for (std::size_t index = 0; index < count; ++index) {
        encoded.append(byte);
    }
    return encoded;
}

[[nodiscard]] std::string texture_payload(mirakana::AssetId texture) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(texture.value) +
           "\n"
           "asset.kind=texture\n"
           "texture.width=2\n"
           "texture.height=2\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=16\n"
           "texture.data_hex=" +
           hex_bytes(16, "7f") + "\n";
}

[[nodiscard]] std::string mesh_payload(mirakana::AssetId mesh) {
    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(mesh.value) +
           "\n"
           "asset.kind=mesh\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=false\n"
           "mesh.has_uvs=false\n"
           "mesh.has_tangent_frame=false\n"
           "mesh.vertex_data_hex=" +
           hex_bytes(36, "3f") +
           "\n"
           "mesh.index_data_hex=000000000100000002000000\n";
}

[[nodiscard]] std::string lit_mesh_payload(mirakana::AssetId mesh) {
    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(mesh.value) +
           "\n"
           "asset.kind=mesh\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=true\n"
           "mesh.has_uvs=true\n"
           "mesh.has_tangent_frame=true\n"
           "mesh.vertex_data_hex=" +
           hex_bytes(144, "3f") +
           "\n"
           "mesh.index_data_hex=000000000100000002000000\n";
}

[[nodiscard]] std::string material_payload(mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Textured",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.2F, 0.4F, 0.8F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.65F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
}

[[nodiscard]] std::string untextured_material_payload(mirakana::AssetId material) {
    return mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Untextured",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {},
        .double_sided = false,
    });
}

[[nodiscard]] mirakana::Scene make_scene(mirakana::AssetId mesh, mirakana::AssetId material, bool duplicate = false) {
    mirakana::Scene scene("RuntimeGpuScene");
    const auto first = scene.create_node("MeshA");
    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(first, components);

    if (duplicate) {
        const auto second = scene.create_node("MeshB");
        scene.set_components(second, components);
    }

    return scene;
}

[[nodiscard]] mirakana::Scene make_two_material_scene(mirakana::AssetId mesh, mirakana::AssetId first_material,
                                                      mirakana::AssetId second_material) {
    mirakana::Scene scene("RuntimeGpuScene");

    const auto first = scene.create_node("MeshA");
    mirakana::SceneNodeComponents first_components;
    first_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mesh, .material = first_material, .visible = true};
    scene.set_components(first, first_components);

    const auto second = scene.create_node("MeshB");
    mirakana::SceneNodeComponents second_components;
    second_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mesh, .material = second_material, .visible = true};
    scene.set_components(second, second_components);

    return scene;
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_textured_package(mirakana::AssetId mesh, mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(material, texture),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_lit_textured_package(mirakana::AssetId mesh, mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/lit_triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = lit_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(material, texture),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_two_textured_material_package(mirakana::AssetId mesh, mirakana::AssetId first_material,
                                   mirakana::AssetId second_material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = first_material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured_a.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(first_material, texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{4},
            .asset = second_material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured_b.material",
            .content_hash = 4,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(second_material, texture),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_mixed_material_layout_package(mirakana::AssetId mesh, mirakana::AssetId textured_material,
                                   mirakana::AssetId untextured_material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = textured_material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(textured_material, texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{4},
            .asset = untextured_material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/untextured.material",
            .content_hash = 4,
            .source_revision = 1,
            .dependencies = {},
            .content = untextured_material_payload(untextured_material),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_missing_mesh_package(mirakana::AssetId material) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/untextured.material",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = untextured_material_payload(material),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_untextured_package(mirakana::AssetId mesh,
                                                                             mirakana::AssetId material) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/untextured.material",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = untextured_material_payload(material),
        },
    });
}

[[nodiscard]] std::string make_identity_joint_palette_hex() {
    constexpr std::array<float, 16> k_row_major_identity{
        1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
    };
    std::string out;
    out.reserve(128U);
    for (const float f : k_row_major_identity) {
        std::uint32_t bits{};
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        std::memcpy(&bits, &f, sizeof(float));
        static constexpr auto k_hex = std::string_view{"0123456789abcdef"};
        for (int shift = 28; shift >= 0; shift -= 4) {
            const auto nybble = static_cast<std::uint32_t>((bits >> shift) & 0xFU);
            out.push_back(k_hex[nybble]);
        }
    }
    return out;
}

[[nodiscard]] std::string skinned_mesh_payload(mirakana::AssetId mesh) {
    return std::string("format=GameEngine.CookedSkinnedMesh.v1\n") + "asset.id=" + std::to_string(mesh.value) +
           "\n"
           "asset.kind=skinned_mesh\n"
           "skinned_mesh.vertex_count=3\n"
           "skinned_mesh.index_count=3\n"
           "skinned_mesh.joint_count=1\n"
           "skinned_mesh.vertex_data_hex=" +
           hex_bytes(216, "2a") +
           "\n"
           "skinned_mesh.index_data_hex=000000000100000002000000\n"
           "skinned_mesh.joint_palette_hex=" +
           make_identity_joint_palette_hex() + "\n";
}

[[nodiscard]] std::string morph_mesh_payload(mirakana::AssetId morph) {
    return std::string("format=GameEngine.CookedMorphMeshCpu.v1\n") + "asset.id=" + std::to_string(morph.value) +
           "\n"
           "asset.kind=morph_mesh_cpu\n"
           "morph.vertex_count=3\n"
           "morph.target_count=1\n"
           "morph.bind_positions_hex=" +
           hex_bytes(36, "00") +
           "\n"
           "morph.target_weights_hex=0000803f\n"
           "morph.target.0.position_deltas_hex=" +
           hex_bytes(36, "00") + "\n";
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_skinned_untextured_package(mirakana::AssetId mesh,
                                                                                     mirakana::AssetId material) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::skinned_mesh,
            .path = "assets/meshes/skinned_triangle.skinned_mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = skinned_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/untextured.material",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = untextured_material_payload(material),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_skinned_morph_untextured_package(mirakana::AssetId mesh, mirakana::AssetId morph, mirakana::AssetId material) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::skinned_mesh,
            .path = "assets/meshes/skinned_morph_triangle.skinned_mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = skinned_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = morph,
            .kind = mirakana::AssetKind::morph_mesh_cpu,
            .path = "assets/morphs/skinned_morph_triangle.morph_mesh_cpu",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = morph_mesh_payload(morph),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/skinned_morph_untextured.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {},
            .content = untextured_material_payload(material),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_morph_package(mirakana::AssetId morph) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = morph,
            .kind = mirakana::AssetKind::morph_mesh_cpu,
            .path = "assets/morphs/triangle.morph_mesh_cpu",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = morph_mesh_payload(morph),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_textured_morph_package(mirakana::AssetId mesh,
                                                                                 mirakana::AssetId morph,
                                                                                 mirakana::AssetId material,
                                                                                 mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = morph,
            .kind = mirakana::AssetKind::morph_mesh_cpu,
            .path = "assets/morphs/triangle.morph_mesh_cpu",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {},
            .content = morph_mesh_payload(morph),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{4},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured.material",
            .content_hash = 4,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(material, texture),
        },
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_missing_texture_package(mirakana::AssetId mesh, mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/triangle.mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/textured.material",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {texture},
            .content = material_payload(material, texture),
        },
    });
}

} // namespace

MK_TEST("runtime scene rhi builds gpu palette from cooked scene package references") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material, true);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.palette.mesh_count() == 1);
    MK_REQUIRE(result.palette.material_count() == 1);
    MK_REQUIRE(result.mesh_uploads.size() == 1);
    MK_REQUIRE(result.texture_uploads.size() == 1);
    MK_REQUIRE(result.material_bindings.size() == 1);
    MK_REQUIRE(result.material_pipeline_layouts.size() == 1);

    const auto* mesh_binding = result.palette.find_mesh(mesh);
    const auto* material_binding = result.palette.find_material(material);
    MK_REQUIRE(mesh_binding != nullptr);
    MK_REQUIRE(material_binding != nullptr);
    MK_REQUIRE(mesh_binding->vertex_buffer.value != 0);
    MK_REQUIRE(mesh_binding->index_buffer.value != 0);
    MK_REQUIRE(mesh_binding->owner_device == &device);
    MK_REQUIRE(material_binding->pipeline_layout.value != 0);
    MK_REQUIRE(material_binding->descriptor_set.value != 0);
    MK_REQUIRE(material_binding->owner_device == &device);

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = mesh_binding->vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &result.palette});
    renderer.end_frame();

    const auto stats = device.stats();
    MK_REQUIRE(submitted.meshes_submitted == 2);
    MK_REQUIRE(submitted.mesh_gpu_bindings_resolved == 2);
    MK_REQUIRE(submitted.material_gpu_bindings_resolved == 2);
    MK_REQUIRE(stats.descriptor_sets_bound == 2);
    MK_REQUIRE(stats.indexed_draw_calls == 2);
    MK_REQUIRE(stats.indices_submitted == 6);
}

MK_TEST("runtime scene rhi upload execution reports readiness without exposing native handles") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material, true);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    desc.packet = &packet;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);

    MK_REQUIRE(execution.report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::ready);
    MK_REQUIRE(execution.report.mesh_uploads == 1);
    MK_REQUIRE(execution.report.texture_uploads == 1);
    MK_REQUIRE(execution.report.material_bindings == 1);
    MK_REQUIRE(execution.report.material_pipeline_layouts == 1);
    MK_REQUIRE(execution.report.uploaded_mesh_bytes == 48);
    MK_REQUIRE(execution.report.uploaded_texture_bytes == 512);
    MK_REQUIRE(execution.report.uploaded_material_factor_bytes ==
               mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    MK_REQUIRE(execution.report.submitted_upload_fence_count == 3);
    MK_REQUIRE(execution.bindings.submitted_upload_fences.size() == 3);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.value != 0);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.value ==
               execution.bindings.submitted_upload_fences.back().value);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.queue ==
               execution.bindings.submitted_upload_fences.back().queue);
    MK_REQUIRE(execution.report.diagnostics.empty());
    MK_REQUIRE(execution.bindings.palette.mesh_count() == 1);
    MK_REQUIRE(execution.bindings.palette.material_count() == 1);
}

MK_TEST("runtime scene rhi upload execution preserves submitted fences in submit order across queues") {
    const auto mesh = mirakana::AssetId::from_name("meshes/ordered_fence_triangle");
    const auto material = mirakana::AssetId::from_name("materials/ordered_fence_textured");
    const auto texture = mirakana::AssetId::from_name("textures/ordered_fence_base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material, true);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    desc.packet = &packet;
    desc.binding_options.mesh_upload.queue = mirakana::rhi::QueueKind::compute;
    desc.binding_options.texture_upload.queue = mirakana::rhi::QueueKind::copy;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);

    MK_REQUIRE(execution.report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::ready);
    MK_REQUIRE(execution.report.submitted_upload_fence_count == 3);
    MK_REQUIRE(execution.bindings.submitted_upload_fences.size() == 3);
    MK_REQUIRE(execution.bindings.submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(execution.bindings.submitted_upload_fences[1].queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(execution.bindings.submitted_upload_fences[2].queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.value ==
               execution.bindings.submitted_upload_fences.back().value);
    MK_REQUIRE(execution.report.last_submitted_upload_fence.queue == mirakana::rhi::QueueKind::graphics);
}

MK_TEST("runtime scene rhi upload execution rejects missing package scene or device request") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);

    MK_REQUIRE(execution.report.status ==
               mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::invalid_request);
    MK_REQUIRE(execution.report.diagnostics.size() == 1);
    MK_REQUIRE(execution.report.diagnostics[0].message.find("scene render packet") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().textures_created == 0);
}

MK_TEST("runtime scene rhi upload execution reports per asset failures separately from scene validation") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/missing");
    const auto package = make_missing_texture_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    desc.packet = &packet;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);

    MK_REQUIRE(execution.report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::failed);
    MK_REQUIRE(execution.report.mesh_uploads == 1);
    MK_REQUIRE(execution.report.texture_uploads == 0);
    MK_REQUIRE(execution.report.material_bindings == 0);
    MK_REQUIRE(execution.report.diagnostics.size() == 1);
    MK_REQUIRE(execution.report.diagnostics[0].asset == texture);
    MK_REQUIRE(execution.report.diagnostics[0].message.find("texture") != std::string::npos);
}

MK_TEST("runtime scene rhi derives lit mesh vertex layout from cooked payload metadata") {
    const auto mesh = mirakana::AssetId::from_name("meshes/lit_triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_lit_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mesh_uploads.size() == 1);
    MK_REQUIRE(result.mesh_uploads[0].upload.vertex_stride ==
               mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);
    const auto* mesh_binding = result.palette.find_mesh(mesh);
    MK_REQUIRE(mesh_binding != nullptr);
    MK_REQUIRE(mesh_binding->vertex_stride == mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding->owner_device == &device);
}

MK_TEST("runtime scene rhi shares generated material pipeline layout across compatible materials") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto first_material = mirakana::AssetId::from_name("materials/textured_a");
    const auto second_material = mirakana::AssetId::from_name("materials/textured_b");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_two_textured_material_package(mesh, first_material, second_material, texture);
    const auto scene = make_two_material_scene(mesh, first_material, second_material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.palette.mesh_count() == 1);
    MK_REQUIRE(result.palette.material_count() == 2);
    MK_REQUIRE(result.material_pipeline_layouts.size() == 1);
    const auto* first_binding = result.palette.find_material(first_material);
    const auto* second_binding = result.palette.find_material(second_material);
    MK_REQUIRE(first_binding != nullptr);
    MK_REQUIRE(second_binding != nullptr);
    MK_REQUIRE(first_binding->pipeline_layout.value == second_binding->pipeline_layout.value);

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto* mesh_binding = result.palette.find_mesh(mesh);
    MK_REQUIRE(mesh_binding != nullptr);
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = first_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = mesh_binding->vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 16, .height = 16},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &result.palette});
    renderer.end_frame();

    MK_REQUIRE(submitted.material_gpu_bindings_resolved == 2);
    MK_REQUIRE(device.stats().descriptor_sets_bound == 2);
}

MK_TEST("runtime scene rhi rejects incompatible material descriptor layouts for one scene pipeline") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto textured_material = mirakana::AssetId::from_name("materials/textured");
    const auto untextured_material = mirakana::AssetId::from_name("materials/untextured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_mixed_material_layout_package(mesh, textured_material, untextured_material, texture);
    const auto scene = make_two_material_scene(mesh, textured_material, untextured_material);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(
        device, package, mirakana::build_scene_render_packet(scene));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.material_count() == 1);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == untextured_material);
    MK_REQUIRE(result.failures[0].diagnostic.find("descriptor layout") != std::string::npos);
}

MK_TEST("runtime scene rhi can use a caller owned material descriptor layout") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto material_metadata = mirakana::build_material_pipeline_binding_metadata(mirakana::MaterialDefinition{
        .id = material,
        .name = "Textured",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
    const auto descriptor_layout_desc =
        mirakana::runtime_rhi::make_runtime_material_descriptor_set_layout_desc(material_metadata);
    MK_REQUIRE(descriptor_layout_desc.succeeded());
    const auto descriptor_layout = device.create_descriptor_set_layout(descriptor_layout_desc.desc);

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.shared_material_descriptor_set_layout = descriptor_layout;
    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.material_pipeline_layouts.size() == 1);
    MK_REQUIRE(result.material_bindings.size() == 1);
    MK_REQUIRE(result.material_bindings[0].binding.descriptor_set_layout.value == descriptor_layout.value);
    const auto* material_binding = result.palette.find_material(material);
    MK_REQUIRE(material_binding != nullptr);
    MK_REQUIRE(material_binding->pipeline_layout.value == result.material_pipeline_layouts[0].value);

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto* mesh_binding = result.palette.find_mesh(mesh);
    MK_REQUIRE(mesh_binding != nullptr);
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = mesh_binding->vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 16, .height = 16},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &result.palette});
    renderer.end_frame();

    MK_REQUIRE(submitted.material_gpu_bindings_resolved == 1);
    MK_REQUIRE(device.stats().descriptor_sets_bound == 1);
}

MK_TEST("runtime scene rhi appends caller owned receiver descriptor layouts after material set") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto shadow_receiver_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_depth_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.additional_pipeline_descriptor_set_layouts.push_back(shadow_receiver_layout);
    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.material_pipeline_layouts.size() == 1);
    const auto* material_binding = result.palette.find_material(material);
    MK_REQUIRE(material_binding != nullptr);
    MK_REQUIRE(material_binding->descriptor_set_index == 0);

    const auto shadow_receiver_set = device.allocate_descriptor_set(shadow_receiver_layout);
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_shadow_receiver",
        .bytecode_size = 64,
    });
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
    });
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto frame = device.acquire_swapchain_frame(swapchain);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
                .clear_color = mirakana::rhi::ClearColorValue{},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(material_binding->pipeline_layout, material_binding->descriptor_set_index,
                                  material_binding->descriptor_set);
    commands->bind_descriptor_set(material_binding->pipeline_layout, 1, shadow_receiver_set);
    commands->end_render_pass();
    commands->present(frame);
    commands->close();
    const auto fence = device.submit(*commands);
    device.wait(fence);

    MK_REQUIRE(device.stats().descriptor_sets_bound == 2);
}

MK_TEST("runtime scene rhi rejects incompatible caller owned descriptor layouts") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/base_color");
    const auto package = make_textured_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.shared_material_descriptor_set_layout =
        device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{
            {mirakana::rhi::DescriptorBindingDesc{
                .binding = 7,
                .type = mirakana::rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = mirakana::rhi::ShaderStageVisibility::fragment,
            }},
        });
    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(
        device, package, mirakana::build_scene_render_packet(scene), options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.material_count() == 0);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == material);
    MK_REQUIRE(result.failures[0].diagnostic.find("descriptor binding") != std::string::npos);
}

MK_TEST("runtime scene rhi reports missing mesh payloads without registering mesh binding") {
    const auto mesh = mirakana::AssetId::from_name("meshes/missing");
    const auto material = mirakana::AssetId::from_name("materials/untextured");
    const auto package = make_missing_mesh_package(material);
    const auto scene = make_scene(mesh, material);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(
        device, package, mirakana::build_scene_render_packet(scene));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.mesh_count() == 0);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == mesh);
    MK_REQUIRE(result.failures[0].diagnostic.find("mesh") != std::string::npos);
}

MK_TEST("runtime scene rhi reports missing material textures without registering material binding") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/textured");
    const auto texture = mirakana::AssetId::from_name("textures/missing");
    const auto package = make_missing_texture_package(mesh, material, texture);
    const auto scene = make_scene(mesh, material);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(
        device, package, mirakana::build_scene_render_packet(scene));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.mesh_count() == 1);
    MK_REQUIRE(result.palette.material_count() == 0);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == texture);
    MK_REQUIRE(result.failures[0].diagnostic.find("texture") != std::string::npos);
}

MK_TEST("runtime scene gpu binding safe point teardown invalidates null device mesh buffers") {
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const auto material = mirakana::AssetId::from_name("materials/untextured");
    const auto package = make_untextured_package(mesh, material);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    desc.packet = &packet;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);
    MK_REQUIRE(execution.bindings.succeeded());
    const auto upload_buffer = execution.bindings.mesh_uploads[0].upload.vertex_upload_buffer;

    std::array<std::uint8_t, 4> bytes{1, 2, 3, 4};
    device.write_buffer(upload_buffer, 0, bytes);

    const auto teardown =
        mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_binding_safe_point_teardown(device, execution.bindings);
    MK_REQUIRE(teardown.null_device_teardown_completed());
    MK_REQUIRE(teardown.buffers_released > 0);

    bool write_threw = false;
    try {
        device.write_buffer(upload_buffer, 0, bytes);
    } catch (const std::invalid_argument&) {
        write_threw = true;
    }
    MK_REQUIRE(write_threw);
}

MK_TEST("runtime scene gpu binding safe point teardown skips failed binding palettes") {
    const auto mesh = mirakana::AssetId::from_name("meshes/missing");
    const auto material = mirakana::AssetId::from_name("materials/untextured");
    const auto package = make_missing_mesh_package(material);
    const auto scene = make_scene(mesh, material);
    mirakana::rhi::NullRhiDevice device;

    const auto bindings = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(
        device, package, mirakana::build_scene_render_packet(scene));
    MK_REQUIRE(!bindings.succeeded());

    const auto teardown =
        mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_binding_safe_point_teardown(device, bindings);
    MK_REQUIRE(!teardown.null_device_teardown_completed());
    MK_REQUIRE(teardown.status ==
               mirakana::runtime_scene_rhi::RuntimeSceneGpuSafePointTeardownStatus::skipped_binding_failed);
}

MK_TEST("runtime scene rhi builds skinned gpu palette from cooked skinned mesh package") {
    const auto mesh = mirakana::AssetId::from_name("meshes/skinned_triangle_runtime_scene");
    const auto material = mirakana::AssetId::from_name("materials/skinned_untextured_runtime_scene");
    const auto package = make_skinned_untextured_package(mesh, material);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    const auto result = mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.skinned_joint_descriptor_set_layout.value != 0);
    MK_REQUIRE(result.skinned_palette.skinned_mesh_count() == 1);
    MK_REQUIRE(result.palette.mesh_count() == 0);
    MK_REQUIRE(result.mesh_uploads.empty());
    MK_REQUIRE(result.skinned_mesh_uploads.size() == 1);
    const auto* skinned_binding = result.skinned_palette.find_skinned_mesh(mesh);
    MK_REQUIRE(skinned_binding != nullptr);
    MK_REQUIRE(skinned_binding->joint_descriptor_set.value != 0);
    MK_REQUIRE(skinned_binding->mesh.vertex_stride == mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
}

MK_TEST("runtime scene rhi upload execution counts skinned mesh upload bytes") {
    const auto mesh = mirakana::AssetId::from_name("meshes/skinned_triangle_runtime_scene_upload");
    const auto material = mirakana::AssetId::from_name("materials/skinned_untextured_runtime_scene_upload");
    const auto package = make_skinned_untextured_package(mesh, material);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc desc;
    desc.device = &device;
    desc.package = &package;
    desc.packet = &packet;
    const auto execution = mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload(desc);

    MK_REQUIRE(execution.report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::ready);
    MK_REQUIRE(execution.report.skinned_mesh_uploads == 1);
    MK_REQUIRE(execution.report.mesh_uploads == 0);
    MK_REQUIRE(execution.report.uploaded_mesh_bytes == 216U + 12U + 256U);
}

MK_TEST("runtime scene rhi builds compute morph skinned gpu palette from selected cooked assets") {
    const auto mesh = mirakana::AssetId::from_name("meshes/compute_morph_skinned_runtime_scene");
    const auto morph = mirakana::AssetId::from_name("morphs/compute_morph_skinned_runtime_scene");
    const auto material = mirakana::AssetId::from_name("materials/compute_morph_skinned_runtime_scene");
    const auto package = make_skinned_morph_untextured_package(mesh, morph, material);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.compute_morph_skinned_mesh_bindings.push_back(
        mirakana::runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshBinding{.mesh = mesh, .morph_mesh = morph});

    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.skinned_palette.skinned_mesh_count() == 1);
    MK_REQUIRE(result.morph_palette.morph_mesh_count() == 0);
    MK_REQUIRE(result.skinned_mesh_uploads.size() == 1);
    MK_REQUIRE(result.morph_mesh_uploads.size() == 1);
    MK_REQUIRE(result.compute_morph_skinned_mesh_bindings.size() == 1);

    const auto& compute_resource = result.compute_morph_skinned_mesh_bindings[0];
    MK_REQUIRE(compute_resource.mesh == mesh);
    MK_REQUIRE(compute_resource.morph_mesh == morph);
    MK_REQUIRE(compute_resource.base_position_upload.vertex_stride ==
               mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(compute_resource.compute_binding.output_position_buffer.value != 0);
    MK_REQUIRE(mirakana::rhi::has_flag(compute_resource.compute_binding.output_position_buffer_desc.usage,
                                       mirakana::rhi::BufferUsage::vertex));

    const auto* skinned_binding = result.skinned_palette.find_skinned_mesh(mesh);
    MK_REQUIRE(skinned_binding != nullptr);
    MK_REQUIRE(skinned_binding->mesh.vertex_buffer.value ==
               compute_resource.compute_binding.output_position_buffer.value);
    MK_REQUIRE(skinned_binding->mesh.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(skinned_binding->skin_attribute_vertex_buffer.value ==
               result.skinned_mesh_uploads[0].upload.vertex_buffer.value);
    MK_REQUIRE(skinned_binding->skin_attribute_vertex_stride ==
               mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    MK_REQUIRE(skinned_binding->joint_descriptor_set.value != 0);
    MK_REQUIRE(result.skinned_joint_descriptor_set_layout.value != 0);
    MK_REQUIRE(result.submitted_upload_fences.size() == 4);
    MK_REQUIRE(result.submitted_upload_fences[0].value == result.skinned_mesh_uploads[0].upload.submitted_fence.value);
    MK_REQUIRE(result.submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value);
    MK_REQUIRE(result.submitted_upload_fences[2].value == result.morph_mesh_uploads[0].upload.submitted_fence.value);
    MK_REQUIRE(result.submitted_upload_fences[3].value == result.material_bindings[0].binding.submitted_fence.value);

    const auto report = mirakana::runtime_scene_rhi::make_runtime_scene_gpu_upload_execution_report(result);
    MK_REQUIRE(report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::ready);
    MK_REQUIRE(report.compute_morph_skinned_mesh_bindings == 1);
    MK_REQUIRE(report.compute_morph_output_position_bytes == 36U);
    MK_REQUIRE(report.submitted_upload_fence_count == 4);
    MK_REQUIRE(report.last_submitted_upload_fence.value != 0);
    MK_REQUIRE(report.last_submitted_upload_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(report.last_submitted_upload_fence.value == result.submitted_upload_fences.back().value);
}

MK_TEST("runtime scene rhi builds morph gpu palette from selected cooked morph package") {
    const auto morph = mirakana::AssetId::from_name("morphs/triangle_runtime_scene");
    const auto package = make_morph_package(morph);
    const mirakana::SceneRenderPacket packet;
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.morph_mesh_assets = {morph};
    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.morph_descriptor_set_layout.value != 0);
    MK_REQUIRE(result.morph_palette.morph_mesh_count() == 1);
    MK_REQUIRE(result.morph_mesh_uploads.size() == 1);

    const auto* binding = result.morph_palette.find_morph_mesh(morph);
    MK_REQUIRE(binding != nullptr);
    MK_REQUIRE(binding->morph_descriptor_set.value != 0);
    MK_REQUIRE(binding->vertex_count == 3);
    MK_REQUIRE(binding->target_count == 1);

    const auto report = mirakana::runtime_scene_rhi::make_runtime_scene_gpu_upload_execution_report(result);
    MK_REQUIRE(report.status == mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionStatus::ready);
    MK_REQUIRE(report.morph_mesh_uploads == 1);
    MK_REQUIRE(report.uploaded_morph_bytes == 36U + 256U);
    MK_REQUIRE(report.submitted_upload_fence_count == 1);
    MK_REQUIRE(result.submitted_upload_fences.size() == 1);
    MK_REQUIRE(report.last_submitted_upload_fence.value != 0);
    MK_REQUIRE(report.last_submitted_upload_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(report.last_submitted_upload_fence.value == result.submitted_upload_fences.back().value);

    const auto teardown =
        mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_binding_safe_point_teardown(device, result);
    MK_REQUIRE(teardown.null_device_teardown_completed());
    MK_REQUIRE(teardown.buffers_released >= 4);
    MK_REQUIRE(teardown.descriptor_sets_released >= 1);
    MK_REQUIRE(teardown.descriptor_set_layouts_released >= 1);
}

MK_TEST("runtime scene rhi appends morph descriptor set layout to morph-only material pipelines") {
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_visible_triangle");
    const auto morph = mirakana::AssetId::from_name("morphs/morph_visible_triangle");
    const auto material = mirakana::AssetId::from_name("materials/morph_visible_textured");
    const auto texture = mirakana::AssetId::from_name("textures/morph_visible_base_color");
    const auto package = make_textured_morph_package(mesh, morph, material, texture);
    const auto scene = make_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.morph_mesh_assets = {morph};
    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.morph_descriptor_set_layout.value != 0);
    MK_REQUIRE(result.material_pipeline_layouts.size() == 1);

    const auto* material_binding = result.palette.find_material(material);
    MK_REQUIRE(material_binding != nullptr);
    const auto* morph_binding = result.morph_palette.find_morph_mesh(morph);
    MK_REQUIRE(morph_binding != nullptr);

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_morph_material_layout",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_morph_material_layout",
        .bytecode_size = 64,
    });
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(material_binding->pipeline_layout, material_binding->descriptor_set_index,
                                  material_binding->descriptor_set);
    commands->bind_descriptor_set(material_binding->pipeline_layout, 1, morph_binding->morph_descriptor_set);
    commands->end_render_pass();
    commands->close();
}

MK_TEST("runtime scene rhi reports selected morph package with wrong asset kind") {
    const auto morph = mirakana::AssetId::from_name("morphs/wrong_kind_runtime_scene");
    const auto material = mirakana::AssetId::from_name("materials/untextured_morph_wrong_kind");
    const auto package = make_untextured_package(morph, material);
    const mirakana::SceneRenderPacket packet;
    mirakana::rhi::NullRhiDevice device;

    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingOptions options;
    options.morph_mesh_assets = {morph};
    const auto result =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(device, package, packet, options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.morph_palette.morph_mesh_count() == 0);
    MK_REQUIRE(result.morph_mesh_uploads.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == morph);
    MK_REQUIRE(result.failures[0].diagnostic.find("morph_mesh_cpu") != std::string::npos);
}

int main() {
    return mirakana::test::run_all();
}
