// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/material_preview_gpu_cache.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <span>

namespace mirakana::editor {

namespace {

[[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
create_material_preview_graphics_pipeline(mirakana::rhi::IRhiDevice& device, mirakana::rhi::Format color_format,
                                          mirakana::rhi::PipelineLayoutHandle layout,
                                          const std::string* vertex_bytecode, const std::string* fragment_bytecode) {
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode == nullptr ? 64U : static_cast<std::uint64_t>(vertex_bytecode->size()),
        .bytecode = vertex_bytecode == nullptr ? nullptr : vertex_bytecode->data(),
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = fragment_bytecode == nullptr ? 64U : static_cast<std::uint64_t>(fragment_bytecode->size()),
        .bytecode = fragment_bytecode == nullptr ? nullptr : fragment_bytecode->data(),
    });
    return device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = color_format,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = {},
    });
}

[[nodiscard]] mirakana::AssetId material_preview_factor_shader_id() {
    return mirakana::AssetId::from_name("editor.material_preview.factor");
}

[[nodiscard]] mirakana::AssetId material_preview_textured_shader_id() {
    return mirakana::AssetId::from_name("editor.material_preview.textured");
}

[[nodiscard]] mirakana::AssetId
material_preview_shader_id_for_plan(const mirakana::editor::EditorMaterialGpuPreviewPlan& plan) {
    const auto has_base_color_texture = std::ranges::any_of(plan.textures, [](const auto& item) {
        return item.slot == mirakana::MaterialTextureSlot::base_color && item.payload.source_bytes > 0;
    });
    return has_base_color_texture ? material_preview_textured_shader_id() : material_preview_factor_shader_id();
}

} // namespace

bool MaterialPreviewGpuCache::ready() const noexcept {
    return status_ == EditorMaterialGpuPreviewStatus::ready && surface_ != nullptr && display_texture_ != nullptr &&
           pipeline_.value != 0 && material_binding_.descriptor_set.value != 0 && scene_pbr_frame_uniform_.value != 0;
}

EditorMaterialGpuPreviewExecutionSnapshot MaterialPreviewGpuCache::execution_snapshot() const {
    EditorMaterialGpuPreviewExecutionSnapshot snapshot;
    snapshot.status = status_;
    snapshot.diagnostic = diagnostic_;
    if (surface_ != nullptr) {
        snapshot.backend_label = std::string(surface_->backend_name());
        snapshot.frames_rendered = surface_->frames_rendered();
    }
    if (display_texture_ != nullptr) {
        snapshot.display_path_label = display_texture_->display_path_contract_id();
    }
    return snapshot;
}

void MaterialPreviewGpuCache::reset() noexcept {
    *this = MaterialPreviewGpuCache{};
}

MaterialPreviewGpuCache& MaterialPreviewGpuCache::rebuild(const MaterialPreviewGpuRebuildDeps& deps,
                                                          mirakana::AssetId material) {
    reset();
    material_ = material;

    if (deps.device == nullptr || deps.sdl_renderer == nullptr) {
        status_ = EditorMaterialGpuPreviewStatus::rhi_unavailable;
        diagnostic_ = "material preview RHI device is unavailable";
        return *this;
    }
    if (deps.tool_filesystem == nullptr || deps.assets == nullptr || !deps.read_material_preview_shader_bytecode) {
        status_ = EditorMaterialGpuPreviewStatus::rhi_unavailable;
        diagnostic_ = "material preview rebuild dependencies are incomplete";
        return *this;
    }
    if (deps.device->backend_kind() == mirakana::rhi::BackendKind::null) {
        status_ = EditorMaterialGpuPreviewStatus::rhi_unavailable;
        diagnostic_ = "material preview requires a native RHI backend";
        return *this;
    }

    try {
        const auto plan = make_editor_material_gpu_preview_plan(*deps.tool_filesystem, *deps.assets, material_);
        artifact_path_ = plan.preview.artifact_path;
        if (!plan.ready()) {
            status_ = plan.status;
            diagnostic_ = plan.diagnostic;
            return *this;
        }

        std::vector<mirakana::runtime_rhi::RuntimeMaterialTextureResource> texture_resources;
        texture_resources.reserve(plan.textures.size());
        texture_uploads_.reserve(plan.textures.size());
        for (const auto& texture : plan.textures) {
            auto upload = mirakana::runtime_rhi::upload_runtime_texture(*deps.device, texture.payload);
            if (!upload.succeeded()) {
                status_ = EditorMaterialGpuPreviewStatus::render_failed;
                diagnostic_ = upload.diagnostic;
                return *this;
            }
            texture_resources.push_back(mirakana::runtime_rhi::RuntimeMaterialTextureResource{
                .slot = texture.slot, .texture = upload.texture, .owner_device = upload.owner_device});
            texture_uploads_.push_back(std::move(upload));
        }

        scene_pbr_frame_uniform_ = deps.device->create_buffer(mirakana::rhi::BufferDesc{
            .size_bytes = mirakana::runtime_rhi::runtime_scene_pbr_frame_uniform_size_bytes,
            .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_source,
        });
        std::array<std::uint8_t, mirakana::runtime_rhi::runtime_scene_pbr_frame_uniform_size_bytes> scene_clear{};
        deps.device->write_buffer(scene_pbr_frame_uniform_, 0,
                                  std::span<const std::uint8_t>(scene_clear.data(), scene_clear.size()));

        mirakana::runtime_rhi::RuntimeMaterialGpuBindingOptions material_bind_opts{};
        material_bind_opts.shared_scene_pbr_frame_uniform = scene_pbr_frame_uniform_;
        material_binding_ = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
            *deps.device, plan.material.binding_metadata, plan.material.material.factors, texture_resources,
            material_bind_opts);
        if (!material_binding_.succeeded()) {
            status_ = EditorMaterialGpuPreviewStatus::render_failed;
            diagnostic_ = material_binding_.diagnostic;
            return *this;
        }

        std::optional<std::string> vertex_bytecode;
        std::optional<std::string> fragment_bytecode;
        const auto material_shader = material_preview_shader_id_for_plan(plan);
        if (deps.device->backend_kind() == mirakana::rhi::BackendKind::d3d12) {
            vertex_bytecode = deps.read_material_preview_shader_bytecode(
                material_shader, mirakana::ShaderSourceStage::vertex, mirakana::ShaderCompileTarget::d3d12_dxil);
            fragment_bytecode = deps.read_material_preview_shader_bytecode(
                material_shader, mirakana::ShaderSourceStage::fragment, mirakana::ShaderCompileTarget::d3d12_dxil);
            if (!vertex_bytecode.has_value() || !fragment_bytecode.has_value()) {
                status_ = EditorMaterialGpuPreviewStatus::rhi_unavailable;
                diagnostic_ = "D3D12 material preview shader bytecode is missing";
                return *this;
            }
        } else if (deps.device->backend_kind() == mirakana::rhi::BackendKind::vulkan) {
            vertex_bytecode = deps.read_material_preview_shader_bytecode(
                material_shader, mirakana::ShaderSourceStage::vertex, mirakana::ShaderCompileTarget::vulkan_spirv);
            fragment_bytecode = deps.read_material_preview_shader_bytecode(
                material_shader, mirakana::ShaderSourceStage::fragment, mirakana::ShaderCompileTarget::vulkan_spirv);
            if (!vertex_bytecode.has_value() || !fragment_bytecode.has_value()) {
                status_ = EditorMaterialGpuPreviewStatus::rhi_unavailable;
                diagnostic_ = "Vulkan material preview shader SPIR-V is missing";
                return *this;
            }
        }

        surface_ = std::make_unique<mirakana::RhiViewportSurface>(mirakana::RhiViewportSurfaceDesc{
            .device = deps.device,
            .extent = mirakana::Extent2D{.width = 128, .height = 128},
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .wait_for_completion = true,
            .allow_native_display_interop = deps.device->backend_kind() == mirakana::rhi::BackendKind::d3d12,
        });
        display_texture_ = std::make_unique<SdlViewportTexture>(deps.sdl_renderer, surface_->extent());
        pipeline_layout_ = deps.device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
            .descriptor_sets = {material_binding_.descriptor_set_layout},
            .push_constant_bytes = 0,
        });
        pipeline_ =
            create_material_preview_graphics_pipeline(*deps.device, surface_->color_format(), pipeline_layout_,
                                                      vertex_bytecode.has_value() ? &(*vertex_bytecode) : nullptr,
                                                      fragment_bytecode.has_value() ? &(*fragment_bytecode) : nullptr);
        status_ = EditorMaterialGpuPreviewStatus::ready;
        return *this;
    } catch (const std::exception& error) {
        status_ = EditorMaterialGpuPreviewStatus::render_failed;
        diagnostic_ = error.what();
        return *this;
    }
}

MaterialPreviewGpuCache& MaterialPreviewGpuCache::ensure(const MaterialPreviewGpuRebuildDeps& deps,
                                                         mirakana::AssetId material) {
    if (material_ == material && status_ != EditorMaterialGpuPreviewStatus::unknown) {
        return *this;
    }
    return rebuild(deps, material);
}

void MaterialPreviewGpuCache::render(const MaterialPreviewGpuRenderDeps& deps) {
    if (!ready() || surface_ == nullptr || display_texture_ == nullptr) {
        return;
    }
    if (deps.device == nullptr || !deps.sync_display_texture) {
        status_ = EditorMaterialGpuPreviewStatus::render_failed;
        diagnostic_ = "material preview render dependencies are incomplete";
        return;
    }
    if (material_binding_.owner_device != deps.device) {
        status_ = EditorMaterialGpuPreviewStatus::render_failed;
        diagnostic_ = "material preview cache belongs to a different RHI device";
        return;
    }

    try {
        if (rendered_version_ == 0) {
            surface_->render_frame(
                mirakana::RhiViewportRenderDesc{
                    .graphics_pipeline = pipeline_,
                    .clear_color = mirakana::Color{.r = 0.035F, .g = 0.04F, .b = 0.05F, .a = 1.0F},
                },
                [this, device = deps.device](mirakana::IRenderer& renderer) {
                    const auto extent = surface_->extent();
                    const float aspect =
                        extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0F;
                    std::array<std::uint8_t, mirakana::scene_pbr_frame_uniform_packed_bytes> pbr_bytes{};
                    mirakana::pack_scene_pbr_frame_gpu(
                        std::span<std::uint8_t>(pbr_bytes.data(), pbr_bytes.size()),
                        mirakana::ScenePbrFrameGpuPackInput{
                            .packet = nullptr,
                            .camera = mirakana::SceneCameraMatrices{.view_from_world = mirakana::Mat4::identity(),
                                                                    .clip_from_view = mirakana::Mat4::identity(),
                                                                    .clip_from_world = mirakana::Mat4::identity()},
                            .world_from_node = mirakana::Mat4::identity(),
                            .camera_world_position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 2.5F},
                            .ambient_rgb = {0.06F, 0.07F, 0.08F},
                            .viewport_aspect = aspect,
                        });
                    device->write_buffer(scene_pbr_frame_uniform_, 0,
                                         std::span<const std::uint8_t>(pbr_bytes.data(), pbr_bytes.size()));
                    renderer.draw_mesh(mirakana::MeshCommand{
                        .transform = mirakana::Transform3D{},
                        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                        .mesh = mirakana::AssetId::from_name("editor.material_preview.triangle"),
                        .material = material_,
                        .world_from_node = mirakana::Mat4::identity(),
                        .mesh_binding = mirakana::MeshGpuBinding{},
                        .material_binding =
                            mirakana::MaterialGpuBinding{
                                .pipeline_layout = pipeline_layout_,
                                .descriptor_set = material_binding_.descriptor_set,
                                .descriptor_set_index = 0,
                                .owner_device = material_binding_.owner_device,
                            },
                        .gpu_skinning = false,
                        .skinned_mesh = {},
                        .gpu_morphing = false,
                        .morph_mesh = {},
                    });
                });
            rendered_version_ = surface_->frames_rendered();
        }
        deps.sync_display_texture(*surface_, *display_texture_);
    } catch (const std::exception& error) {
        status_ = EditorMaterialGpuPreviewStatus::render_failed;
        diagnostic_ = error.what();
    }
}

} // namespace mirakana::editor
