// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/rhi_frame_renderer.hpp"

#include "rhi_native_ui_overlay.hpp"

#include "mirakana/renderer/frame_graph_rhi.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool has_mesh_gpu_binding(const MeshGpuBinding& binding) noexcept {
    return binding.vertex_buffer.value != 0 || binding.index_buffer.value != 0 || binding.vertex_count != 0 ||
           binding.index_count != 0 || binding.vertex_offset != 0 || binding.index_offset != 0 ||
           binding.vertex_stride != 0 || binding.index_format != rhi::IndexFormat::unknown ||
           binding.normal_vertex_buffer.value != 0 || binding.tangent_vertex_buffer.value != 0 ||
           binding.normal_vertex_stride != 0 || binding.tangent_vertex_stride != 0;
}

[[nodiscard]] bool has_material_gpu_binding(const MaterialGpuBinding& binding) noexcept {
    return binding.pipeline_layout.value != 0 || binding.descriptor_set.value != 0;
}

[[nodiscard]] bool has_skinned_mesh_gpu_binding(const SkinnedMeshGpuBinding& binding) noexcept {
    return has_mesh_gpu_binding(binding.mesh) && binding.joint_palette_buffer.value != 0 &&
           binding.joint_descriptor_set.value != 0 && binding.joint_count != 0 &&
           binding.joint_palette_uniform_allocation_bytes != 0 && binding.owner_device != nullptr;
}

[[nodiscard]] bool has_morph_mesh_gpu_binding(const MorphMeshGpuBinding& binding) noexcept {
    const bool normal_stream_valid = (binding.normal_delta_buffer.value != 0) == (binding.normal_delta_bytes != 0);
    const bool tangent_stream_valid = (binding.tangent_delta_buffer.value != 0) == (binding.tangent_delta_bytes != 0);
    return binding.position_delta_buffer.value != 0 && binding.morph_weight_buffer.value != 0 &&
           binding.morph_descriptor_set.value != 0 && binding.vertex_count != 0 && binding.target_count != 0 &&
           binding.position_delta_bytes != 0 && binding.morph_weight_uniform_allocation_bytes != 0 &&
           binding.owner_device != nullptr && normal_stream_valid && tangent_stream_valid;
}

void validate_mesh_gpu_binding(const MeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (binding.vertex_buffer.value == 0 || binding.index_buffer.value == 0) {
        throw std::invalid_argument("rhi frame renderer mesh command requires vertex and index buffers");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi frame renderer mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument("rhi frame renderer mesh command uses buffers from a different rhi device");
    }
    if (binding.vertex_stride == 0) {
        throw std::invalid_argument("rhi frame renderer mesh command requires a vertex stride");
    }
    if (binding.vertex_count == 0) {
        throw std::invalid_argument("rhi frame renderer mesh command requires vertex count");
    }
    if (binding.index_count == 0) {
        throw std::invalid_argument("rhi frame renderer mesh command requires index count");
    }
    if (binding.index_format == rhi::IndexFormat::unknown) {
        throw std::invalid_argument("rhi frame renderer mesh command requires index format");
    }
    if ((binding.normal_vertex_buffer.value != 0) != (binding.normal_vertex_stride != 0) ||
        (binding.tangent_vertex_buffer.value != 0) != (binding.tangent_vertex_stride != 0)) {
        throw std::invalid_argument("rhi frame renderer mesh command has inconsistent optional vertex streams");
    }
}

void bind_mesh_vertex_buffers(rhi::IRhiCommandList& commands, const MeshGpuBinding& binding) {
    commands.bind_vertex_buffer(rhi::VertexBufferBinding{
        .buffer = binding.vertex_buffer,
        .offset = binding.vertex_offset,
        .stride = binding.vertex_stride,
        .binding = 0,
    });
    if (binding.normal_vertex_buffer.value != 0) {
        commands.bind_vertex_buffer(rhi::VertexBufferBinding{
            .buffer = binding.normal_vertex_buffer,
            .offset = binding.normal_vertex_offset,
            .stride = binding.normal_vertex_stride,
            .binding = 1,
        });
    }
    if (binding.tangent_vertex_buffer.value != 0) {
        commands.bind_vertex_buffer(rhi::VertexBufferBinding{
            .buffer = binding.tangent_vertex_buffer,
            .offset = binding.tangent_vertex_offset,
            .stride = binding.tangent_vertex_stride,
            .binding = 2,
        });
    }
}

[[nodiscard]] bool try_release_swapchain_frame(rhi::IRhiDevice& device, rhi::SwapchainFrameHandle frame) noexcept {
    try {
        device.release_swapchain_frame(frame);
        return true;
    } catch (...) {
        return false;
    }
}

[[nodiscard]] std::vector<FrameGraphExecutionStep> make_primary_color_frame_graph_schedule() {
    FrameGraphV1Desc desc;
    desc.resources.push_back(FrameGraphResourceV1Desc{
        .name = "primary_color",
        .lifetime = FrameGraphResourceLifetime::imported,
    });
    desc.passes.push_back(FrameGraphPassV1Desc{
        .name = "primary_color",
        .reads = {},
        .writes = {FrameGraphResourceAccess{.resource = "primary_color",
                                            .access = FrameGraphAccess::color_attachment_write}},
    });

    const auto plan = compile_frame_graph_v1(desc);
    if (!plan.succeeded()) {
        return {};
    }
    return schedule_frame_graph_v1_execution(plan);
}

void bind_skinned_mesh_vertex_buffers(rhi::IRhiCommandList& commands, const SkinnedMeshGpuBinding& binding) {
    bind_mesh_vertex_buffers(commands, binding.mesh);
    if (binding.skin_attribute_vertex_buffer.value != 0) {
        commands.bind_vertex_buffer(rhi::VertexBufferBinding{
            .buffer = binding.skin_attribute_vertex_buffer,
            .offset = binding.skin_attribute_vertex_offset,
            .stride = binding.skin_attribute_vertex_stride,
            .binding = 3,
        });
    }
}

void validate_skinned_mesh_gpu_binding(const SkinnedMeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (!has_skinned_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi frame renderer skinned mesh command requires skinned gpu binding data");
    }
    validate_mesh_gpu_binding(binding.mesh, device);
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi frame renderer skinned mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi frame renderer skinned mesh command uses resources from a different rhi device");
    }
    if (binding.joint_palette_buffer.value == 0 || binding.joint_descriptor_set.value == 0) {
        throw std::invalid_argument(
            "rhi frame renderer skinned mesh command requires joint palette buffer and descriptor set");
    }
    if (binding.joint_count == 0 || binding.joint_palette_uniform_allocation_bytes == 0) {
        throw std::invalid_argument("rhi frame renderer skinned mesh command requires joint palette metadata");
    }
    if ((binding.skin_attribute_vertex_buffer.value != 0) != (binding.skin_attribute_vertex_stride != 0)) {
        throw std::invalid_argument(
            "rhi frame renderer skinned mesh command has inconsistent skin attribute vertex stream");
    }
}

void validate_morph_mesh_gpu_binding(const MorphMeshGpuBinding& binding, const rhi::IRhiDevice& device,
                                     std::uint32_t base_vertex_count) {
    if (!has_morph_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi frame renderer morph mesh command requires morph gpu binding data");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi frame renderer morph mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument("rhi frame renderer morph mesh command uses resources from a different rhi device");
    }
    if (binding.position_delta_buffer.value == 0 || binding.morph_weight_buffer.value == 0 ||
        binding.morph_descriptor_set.value == 0) {
        throw std::invalid_argument("rhi frame renderer morph mesh command requires morph buffers and descriptor set");
    }
    if (binding.vertex_count == 0 || binding.target_count == 0 || binding.position_delta_bytes == 0 ||
        binding.morph_weight_uniform_allocation_bytes == 0) {
        throw std::invalid_argument("rhi frame renderer morph mesh command requires morph metadata");
    }
    if ((binding.normal_delta_buffer.value != 0) != (binding.normal_delta_bytes != 0) ||
        (binding.tangent_delta_buffer.value != 0) != (binding.tangent_delta_bytes != 0)) {
        throw std::invalid_argument("rhi frame renderer morph mesh command has inconsistent optional morph streams");
    }
    if (binding.vertex_count != base_vertex_count) {
        throw std::invalid_argument("rhi frame renderer morph mesh command vertex count must match base mesh");
    }
}

void validate_material_gpu_binding(const MaterialGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (binding.pipeline_layout.value == 0 || binding.descriptor_set.value == 0) {
        throw std::invalid_argument("rhi frame renderer material command requires pipeline layout and descriptor set");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi frame renderer material command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument("rhi frame renderer material command uses descriptors from a different rhi device");
    }
}

} // namespace

RhiFrameRenderer::RhiFrameRenderer(const RhiFrameRendererDesc& desc)
    : device_(desc.device), extent_(desc.extent), color_texture_(desc.color_texture), swapchain_(desc.swapchain),
      graphics_pipeline_(desc.graphics_pipeline), skinned_graphics_pipeline_(desc.skinned_graphics_pipeline),
      morph_graphics_pipeline_(desc.morph_graphics_pipeline), depth_texture_(desc.depth_texture),
      wait_for_completion_(desc.wait_for_completion) {
    if (device_ == nullptr) {
        throw std::invalid_argument("rhi frame renderer requires an rhi device");
    }
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument("rhi frame renderer extent must be non-zero");
    }
    if ((color_texture_.value == 0) == (swapchain_.value == 0)) {
        throw std::invalid_argument("rhi frame renderer requires exactly one color target");
    }
    if (graphics_pipeline_.value == 0) {
        throw std::invalid_argument("rhi frame renderer requires a graphics pipeline");
    }
    if (desc.enable_native_sprite_overlay && (desc.native_sprite_overlay_vertex_shader.value == 0 ||
                                              desc.native_sprite_overlay_fragment_shader.value == 0)) {
        throw std::invalid_argument("rhi frame renderer native sprite overlay requires overlay shaders");
    }
    if (desc.enable_native_sprite_overlay && desc.native_sprite_overlay_color_format == rhi::Format::unknown) {
        throw std::invalid_argument("rhi frame renderer native sprite overlay requires a color format");
    }
    if (desc.enable_native_sprite_overlay_textures && !desc.enable_native_sprite_overlay) {
        throw std::invalid_argument("rhi frame renderer textured native sprite overlay requires native sprite overlay");
    }
    if (desc.enable_native_sprite_overlay) {
        native_sprite_overlay_ = std::make_unique<RhiNativeUiOverlay>(RhiNativeUiOverlayDesc{
            .device = device_,
            .extent = extent_,
            .color_format = desc.native_sprite_overlay_color_format,
            .vertex_shader = desc.native_sprite_overlay_vertex_shader,
            .fragment_shader = desc.native_sprite_overlay_fragment_shader,
            .atlas = desc.native_sprite_overlay_atlas,
            .enable_textures = desc.enable_native_sprite_overlay_textures,
        });
    }
}

RhiFrameRenderer::~RhiFrameRenderer() {
    release_acquired_swapchain_frame();
}

std::string_view RhiFrameRenderer::backend_name() const noexcept {
    return device_->backend_name();
}

Extent2D RhiFrameRenderer::backbuffer_extent() const noexcept {
    return extent_;
}

RendererStats RhiFrameRenderer::stats() const noexcept {
    return stats_;
}

Color RhiFrameRenderer::clear_color() const noexcept {
    return clear_color_;
}

bool RhiFrameRenderer::frame_active() const noexcept {
    return frame_active_;
}

void RhiFrameRenderer::resize(Extent2D extent) {
    if (frame_active_) {
        throw std::logic_error("rhi frame renderer cannot resize during an active frame");
    }
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("rhi frame renderer extent must be non-zero");
    }
    if (swapchain_.value != 0) {
        device_->resize_swapchain(swapchain_, rhi::Extent2D{.width = extent.width, .height = extent.height});
    }
    extent_ = extent;
    if (native_sprite_overlay_ != nullptr) {
        native_sprite_overlay_->resize(extent_);
    }
}

void RhiFrameRenderer::set_clear_color(Color color) noexcept {
    clear_color_ = color;
}

void RhiFrameRenderer::replace_graphics_pipeline(rhi::GraphicsPipelineHandle pipeline) {
    if (frame_active_) {
        throw std::logic_error("rhi frame renderer cannot replace graphics pipeline during an active frame");
    }
    if (pipeline.value == 0) {
        throw std::invalid_argument("rhi frame renderer requires a graphics pipeline");
    }
    graphics_pipeline_ = pipeline;
}

void RhiFrameRenderer::replace_depth_texture(rhi::TextureHandle texture) {
    if (frame_active_) {
        throw std::logic_error("rhi frame renderer cannot replace depth texture during an active frame");
    }
    depth_texture_ = texture;
}

void RhiFrameRenderer::begin_frame() {
    if (frame_active_) {
        throw std::logic_error("rhi frame renderer frame already active");
    }

    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
    pending_native_sprite_overlay_sprites_.clear();
    std::unique_ptr<rhi::IRhiCommandList> commands;
    try {
        if (swapchain_.value != 0) {
            swapchain_frame_ = device_->acquire_swapchain_frame(swapchain_);
        }

        commands = device_->begin_command_list(rhi::QueueKind::graphics);

        commands_ = std::move(commands);
        queued_primary_draws_.clear();
        frame_active_ = true;
        skinned_pipeline_bound_ = false;
        morph_pipeline_bound_ = false;
        ++stats_.frames_started;
    } catch (...) {
        release_acquired_swapchain_frame();
        commands_.reset();
        frame_active_ = false;
        throw;
    }
}

void RhiFrameRenderer::draw_sprite(const SpriteCommand& command) {
    require_active_frame();
    if (native_sprite_overlay_ != nullptr && native_sprite_overlay_->ready()) {
        pending_native_sprite_overlay_sprites_.push_back(command);
        ++stats_.native_ui_overlay_sprites_submitted;
        if (command.texture.enabled) {
            ++stats_.native_ui_overlay_textured_sprites_submitted;
        }
    } else {
        queued_primary_draws_.push_back(QueuedPrimaryDraw{.kind = QueuedPrimaryDrawKind::sprite});
    }
    ++stats_.sprites_submitted;
}

void RhiFrameRenderer::draw_mesh(const MeshCommand& command) {
    require_active_frame();
    if (command.gpu_skinning && command.gpu_morphing) {
        throw std::invalid_argument("rhi frame renderer mesh command cannot enable gpu skinning and morphing together");
    }
    if (command.gpu_skinning) {
        if (skinned_graphics_pipeline_.value == 0) {
            throw std::invalid_argument("rhi frame renderer skinned mesh command requires a skinned graphics pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_);
        validate_skinned_mesh_gpu_binding(command.skinned_mesh, *device_);

        queued_primary_draws_.push_back(QueuedPrimaryDraw{.kind = QueuedPrimaryDrawKind::mesh, .mesh = command});
        ++stats_.meshes_submitted;
        return;
    }

    if (command.gpu_morphing) {
        if (morph_graphics_pipeline_.value == 0) {
            throw std::invalid_argument("rhi frame renderer morph mesh command requires a morph graphics pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_);
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
        validate_morph_mesh_gpu_binding(command.morph_mesh, *device_, command.mesh_binding.vertex_count);

        queued_primary_draws_.push_back(QueuedPrimaryDraw{.kind = QueuedPrimaryDrawKind::mesh, .mesh = command});
        ++stats_.meshes_submitted;
        return;
    }

    if (has_material_gpu_binding(command.material_binding)) {
        validate_material_gpu_binding(command.material_binding, *device_);
    }

    if (has_mesh_gpu_binding(command.mesh_binding)) {
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
    }
    queued_primary_draws_.push_back(QueuedPrimaryDraw{.kind = QueuedPrimaryDrawKind::mesh, .mesh = command});
    ++stats_.meshes_submitted;
}

void RhiFrameRenderer::end_frame() {
    require_active_frame();
    try {
        const auto overlay_draw =
            native_sprite_overlay_ != nullptr && native_sprite_overlay_->ready()
                ? native_sprite_overlay_->prepare(pending_native_sprite_overlay_sprites_, *commands_)
                : RhiNativeUiOverlayPreparedDraw{};
        const auto schedule = make_primary_color_frame_graph_schedule();
        if (schedule.empty()) {
            throw std::runtime_error("rhi frame renderer primary frame graph schedule failed");
        }
        RendererStats recorded_primary_stats{};
        const std::vector<FrameGraphPassExecutionBinding> pass_callbacks{
            FrameGraphPassExecutionBinding{
                .pass_name = "primary_color",
                .callback =
                    [&overlay_draw, &recorded_primary_stats, this](std::string_view) {
                        commands_->begin_render_pass(primary_render_pass_desc());
                        commands_->bind_graphics_pipeline(graphics_pipeline_);
                        skinned_pipeline_bound_ = false;
                        morph_pipeline_bound_ = false;
                        for (const auto& draw : queued_primary_draws_) {
                            switch (draw.kind) {
                            case QueuedPrimaryDrawKind::sprite:
                                if (skinned_pipeline_bound_ || morph_pipeline_bound_) {
                                    commands_->bind_graphics_pipeline(graphics_pipeline_);
                                    skinned_pipeline_bound_ = false;
                                    morph_pipeline_bound_ = false;
                                }
                                commands_->draw(3, 1);
                                break;
                            case QueuedPrimaryDrawKind::mesh:
                                record_queued_mesh_command(draw.mesh, recorded_primary_stats);
                                break;
                            }
                        }
                        if (overlay_draw.vertex_count != 0) {
                            native_sprite_overlay_->record_draw(overlay_draw, *commands_);
                        }
                        commands_->end_render_pass();
                        return FrameGraphExecutionCallbackResult{};
                    },
            },
        };
        const auto frame_graph_execution = execute_frame_graph_rhi_texture_schedule(FrameGraphRhiTextureExecutionDesc{
            .commands = commands_.get(),
            .schedule = schedule,
            .texture_bindings = {},
            .pass_callbacks = pass_callbacks,
            .pass_target_accesses = {},
            .pass_target_states = {},
            .final_states = {},
        });
        if (!frame_graph_execution.succeeded()) {
            std::string message{"rhi frame renderer frame graph rhi texture execution failed"};
            if (!frame_graph_execution.diagnostics.empty() &&
                !frame_graph_execution.diagnostics.front().message.empty()) {
                message += ": ";
                message += frame_graph_execution.diagnostics.front().message;
            }
            throw std::runtime_error(message);
        }
        if (swapchain_.value != 0) {
            commands_->present(swapchain_frame_);
            swapchain_frame_presented_ = true;
        }
        commands_->close();

        const auto fence = device_->submit(*commands_);
        if (wait_for_completion_) {
            device_->wait(fence);
        }

        commands_.reset();
        swapchain_frame_ = {};
        swapchain_frame_presented_ = false;
        queued_primary_draws_.clear();
        pending_native_sprite_overlay_sprites_.clear();
        frame_active_ = false;
        ++stats_.frames_finished;
        stats_.gpu_skinning_draws += recorded_primary_stats.gpu_skinning_draws;
        stats_.skinned_palette_descriptor_binds += recorded_primary_stats.skinned_palette_descriptor_binds;
        stats_.gpu_morph_draws += recorded_primary_stats.gpu_morph_draws;
        stats_.morph_descriptor_binds += recorded_primary_stats.morph_descriptor_binds;
        stats_.framegraph_passes_executed += frame_graph_execution.pass_callbacks_invoked;
        stats_.framegraph_barrier_steps_executed += frame_graph_execution.barriers_recorded;
        stats_.native_ui_overlay_draws += overlay_draw.batch_count;
        stats_.native_ui_overlay_texture_binds += overlay_draw.texture_bind_count;
        stats_.native_ui_overlay_textured_draws += overlay_draw.textured_batch_count;
        stats_.native_sprite_batches_executed += overlay_draw.batch_count;
        stats_.native_sprite_batch_sprites_executed += overlay_draw.sprite_count;
        stats_.native_sprite_batch_textured_sprites_executed += overlay_draw.textured_sprite_count;
        stats_.native_sprite_batch_texture_binds += overlay_draw.texture_bind_count;
    } catch (...) {
        release_acquired_swapchain_frame();
        commands_.reset();
        swapchain_frame_ = {};
        swapchain_frame_presented_ = false;
        queued_primary_draws_.clear();
        pending_native_sprite_overlay_sprites_.clear();
        frame_active_ = false;
        throw;
    }
}

mirakana::rhi::RenderPassDesc RhiFrameRenderer::primary_render_pass_desc() const {
    rhi::RenderPassDesc render_pass{
        .color =
            rhi::RenderPassColorAttachment{
                .texture = color_texture_,
                .load_action = rhi::LoadAction::clear,
                .store_action = rhi::StoreAction::store,
                .swapchain_frame = swapchain_frame_,
                .clear_color = rhi::ClearColorValue{.red = clear_color_.r,
                                                    .green = clear_color_.g,
                                                    .blue = clear_color_.b,
                                                    .alpha = clear_color_.a},
            },
    };
    if (depth_texture_.value != 0) {
        render_pass.depth.texture = depth_texture_;
    }
    return render_pass;
}

void RhiFrameRenderer::record_queued_mesh_command(const MeshCommand& command, RendererStats& recorded_stats) {
    if (command.gpu_skinning) {
        commands_->bind_graphics_pipeline(skinned_graphics_pipeline_);
        skinned_pipeline_bound_ = true;
        morph_pipeline_bound_ = false;
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout,
                                       command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout, 1,
                                       command.skinned_mesh.joint_descriptor_set);
        bind_skinned_mesh_vertex_buffers(*commands_, command.skinned_mesh);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.skinned_mesh.mesh.index_buffer,
            .offset = command.skinned_mesh.mesh.index_offset,
            .format = command.skinned_mesh.mesh.index_format,
        });
        commands_->draw_indexed(command.skinned_mesh.mesh.index_count, 1);
        ++recorded_stats.gpu_skinning_draws;
        ++recorded_stats.skinned_palette_descriptor_binds;
        return;
    }

    if (command.gpu_morphing) {
        commands_->bind_graphics_pipeline(morph_graphics_pipeline_);
        skinned_pipeline_bound_ = false;
        morph_pipeline_bound_ = true;
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout,
                                       command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout, 1,
                                       command.morph_mesh.morph_descriptor_set);
        bind_mesh_vertex_buffers(*commands_, command.mesh_binding);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.mesh_binding.index_buffer,
            .offset = command.mesh_binding.index_offset,
            .format = command.mesh_binding.index_format,
        });
        commands_->draw_indexed(command.mesh_binding.index_count, 1);
        ++recorded_stats.gpu_morph_draws;
        ++recorded_stats.morph_descriptor_binds;
        return;
    }

    if (skinned_pipeline_bound_ || morph_pipeline_bound_) {
        commands_->bind_graphics_pipeline(graphics_pipeline_);
        skinned_pipeline_bound_ = false;
        morph_pipeline_bound_ = false;
    }

    if (has_material_gpu_binding(command.material_binding)) {
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout,
                                       command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
    }

    if (has_mesh_gpu_binding(command.mesh_binding)) {
        bind_mesh_vertex_buffers(*commands_, command.mesh_binding);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.mesh_binding.index_buffer,
            .offset = command.mesh_binding.index_offset,
            .format = command.mesh_binding.index_format,
        });
        commands_->draw_indexed(command.mesh_binding.index_count, 1);
    } else {
        commands_->draw(3, 1);
    }
}

void RhiFrameRenderer::require_active_frame() const {
    if (!frame_active_ || commands_ == nullptr) {
        throw std::logic_error("rhi frame renderer frame is not active");
    }
}

void RhiFrameRenderer::release_acquired_swapchain_frame() noexcept {
    if (device_ == nullptr || swapchain_frame_.value == 0) {
        swapchain_frame_presented_ = false;
        return;
    }

    if (!swapchain_frame_presented_) {
        (void)try_release_swapchain_frame(*device_, swapchain_frame_);
    }

    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
}

} // namespace mirakana
