// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"

#include "mirakana/renderer/frame_graph.hpp"
#include "rhi_native_ui_overlay.hpp"

#include <stdexcept>
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

void validate_mesh_gpu_binding(const MeshGpuBinding& binding, const rhi::IRhiDevice& device);

void validate_skinned_mesh_gpu_binding(const SkinnedMeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (!has_skinned_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi postprocess renderer skinned mesh command requires skinned gpu binding data");
    }
    validate_mesh_gpu_binding(binding.mesh, device);
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi postprocess renderer skinned mesh command uses resources from a different rhi device");
    }
}

void validate_morph_mesh_gpu_binding(const MorphMeshGpuBinding& binding, const rhi::IRhiDevice& device,
                                     std::uint32_t base_vertex_count) {
    if (!has_morph_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi postprocess renderer morph mesh command requires morph gpu binding data");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi postprocess renderer morph mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi postprocess renderer morph mesh command uses resources from a different rhi device");
    }
    if (binding.position_delta_buffer.value == 0 || binding.morph_weight_buffer.value == 0 ||
        binding.morph_descriptor_set.value == 0) {
        throw std::invalid_argument(
            "rhi postprocess renderer morph mesh command requires morph buffers and descriptor set");
    }
    if (binding.vertex_count == 0 || binding.target_count == 0 || binding.position_delta_bytes == 0 ||
        binding.morph_weight_uniform_allocation_bytes == 0) {
        throw std::invalid_argument("rhi postprocess renderer morph mesh command requires morph metadata");
    }
    if ((binding.normal_delta_buffer.value != 0) != (binding.normal_delta_bytes != 0) ||
        (binding.tangent_delta_buffer.value != 0) != (binding.tangent_delta_bytes != 0)) {
        throw std::invalid_argument(
            "rhi postprocess renderer morph mesh command has inconsistent optional morph streams");
    }
    if (binding.vertex_count != base_vertex_count) {
        throw std::invalid_argument("rhi postprocess renderer morph mesh command vertex count must match base mesh");
    }
}

void validate_mesh_gpu_binding(const MeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (binding.vertex_buffer.value == 0 || binding.index_buffer.value == 0) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires vertex and index buffers");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument("rhi postprocess renderer mesh command uses buffers from a different rhi device");
    }
    if (binding.vertex_stride == 0) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires a vertex stride");
    }
    if (binding.vertex_count == 0) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires vertex count");
    }
    if (binding.index_count == 0) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires index count");
    }
    if (binding.index_format == rhi::IndexFormat::unknown) {
        throw std::invalid_argument("rhi postprocess renderer mesh command requires index format");
    }
    if ((binding.normal_vertex_buffer.value != 0) != (binding.normal_vertex_stride != 0) ||
        (binding.tangent_vertex_buffer.value != 0) != (binding.tangent_vertex_stride != 0)) {
        throw std::invalid_argument("rhi postprocess renderer mesh command has inconsistent optional vertex streams");
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

void validate_material_gpu_binding(const MaterialGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (binding.pipeline_layout.value == 0 || binding.descriptor_set.value == 0) {
        throw std::invalid_argument(
            "rhi postprocess renderer material command requires pipeline layout and descriptor set");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi postprocess renderer material command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi postprocess renderer material command uses descriptors from a different rhi device");
    }
}

[[nodiscard]] FrameGraphV1Desc make_postprocess_frame_graph_v1_desc(bool depth_input_enabled,
                                                                    std::uint32_t postprocess_stage_count) {
    FrameGraphV1Desc desc;
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "swapchain", .lifetime = FrameGraphResourceLifetime::imported});
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "scene_color", .lifetime = FrameGraphResourceLifetime::transient});
    if (depth_input_enabled) {
        desc.resources.push_back(
            FrameGraphResourceV1Desc{.name = "scene_depth", .lifetime = FrameGraphResourceLifetime::transient});
    }
    if (postprocess_stage_count >= 2) {
        desc.resources.push_back(
            FrameGraphResourceV1Desc{.name = "post_work", .lifetime = FrameGraphResourceLifetime::transient});
    }

    std::vector<FrameGraphResourceAccess> scene_writes;
    scene_writes.push_back(
        FrameGraphResourceAccess{.resource = "scene_color", .access = FrameGraphAccess::color_attachment_write});
    if (depth_input_enabled) {
        scene_writes.push_back(
            FrameGraphResourceAccess{.resource = "scene_depth", .access = FrameGraphAccess::depth_attachment_write});
    }
    desc.passes.push_back(FrameGraphPassV1Desc{.name = "scene_color", .reads = {}, .writes = std::move(scene_writes)});

    if (postprocess_stage_count == 1) {
        std::vector<FrameGraphResourceAccess> postprocess_reads;
        postprocess_reads.push_back(
            FrameGraphResourceAccess{.resource = "scene_color", .access = FrameGraphAccess::shader_read});
        if (depth_input_enabled) {
            postprocess_reads.push_back(
                FrameGraphResourceAccess{.resource = "scene_depth", .access = FrameGraphAccess::shader_read});
        }
        desc.passes.push_back(FrameGraphPassV1Desc{
            .name = "postprocess",
            .reads = std::move(postprocess_reads),
            .writes = {FrameGraphResourceAccess{.resource = "swapchain",
                                                .access = FrameGraphAccess::color_attachment_write}},
        });
    } else {
        std::vector<FrameGraphResourceAccess> first_reads;
        first_reads.push_back(
            FrameGraphResourceAccess{.resource = "scene_color", .access = FrameGraphAccess::shader_read});
        if (depth_input_enabled) {
            first_reads.push_back(
                FrameGraphResourceAccess{.resource = "scene_depth", .access = FrameGraphAccess::shader_read});
        }
        desc.passes.push_back(FrameGraphPassV1Desc{
            .name = "postprocess_chain_0",
            .reads = std::move(first_reads),
            .writes = {FrameGraphResourceAccess{.resource = "post_work",
                                                .access = FrameGraphAccess::color_attachment_write}},
        });
        desc.passes.push_back(FrameGraphPassV1Desc{
            .name = "postprocess_chain_1",
            .reads = {FrameGraphResourceAccess{.resource = "post_work", .access = FrameGraphAccess::shader_read}},
            .writes = {FrameGraphResourceAccess{.resource = "swapchain",
                                                .access = FrameGraphAccess::color_attachment_write}},
        });
    }
    return desc;
}

} // namespace

RhiPostprocessFrameRenderer::RhiPostprocessFrameRenderer(const RhiPostprocessFrameRendererDesc& desc)
    : device_(desc.device), extent_(desc.extent), swapchain_(desc.swapchain), color_format_(desc.color_format),
      depth_format_(desc.depth_format), scene_graphics_pipeline_(desc.scene_graphics_pipeline),
      scene_skinned_graphics_pipeline_(desc.scene_skinned_graphics_pipeline),
      scene_morph_graphics_pipeline_(desc.scene_morph_graphics_pipeline),
      postprocess_vertex_shader_(desc.postprocess_vertex_shader),
      postprocess_fragment_stages_(desc.postprocess_fragment_stages), wait_for_completion_(desc.wait_for_completion),
      depth_input_enabled_(desc.enable_depth_input), native_ui_overlay_enabled_(desc.enable_native_ui_overlay),
      native_ui_overlay_textures_enabled_(desc.enable_native_ui_overlay_textures) {
    if (device_ == nullptr) {
        throw std::invalid_argument("rhi postprocess renderer requires an rhi device");
    }
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument("rhi postprocess renderer extent must be non-zero");
    }
    if (swapchain_.value == 0) {
        throw std::invalid_argument("rhi postprocess renderer requires a swapchain");
    }
    if (color_format_ == rhi::Format::unknown || color_format_ == rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("rhi postprocess renderer requires a color format");
    }
    if (scene_graphics_pipeline_.value == 0) {
        throw std::invalid_argument("rhi postprocess renderer requires a scene graphics pipeline");
    }
    if (postprocess_fragment_stages_.empty() || postprocess_fragment_stages_.size() > 2) {
        throw std::invalid_argument("rhi postprocess renderer requires one or two postprocess fragment stages");
    }
    for (const auto& fs : postprocess_fragment_stages_) {
        if (fs.value == 0) {
            throw std::invalid_argument("rhi postprocess renderer requires non-null postprocess fragment shaders");
        }
    }
    if (postprocess_vertex_shader_.value == 0) {
        throw std::invalid_argument("rhi postprocess renderer requires postprocess vertex shader");
    }
    postprocess_stage_count_ = static_cast<std::uint32_t>(postprocess_fragment_stages_.size());
    if (depth_input_enabled_ && depth_format_ != rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("rhi postprocess renderer depth input requires depth24_stencil8");
    }
    if (native_ui_overlay_enabled_ &&
        (desc.native_ui_overlay_vertex_shader.value == 0 || desc.native_ui_overlay_fragment_shader.value == 0)) {
        throw std::invalid_argument("rhi postprocess renderer native ui overlay requires overlay shaders");
    }
    if (native_ui_overlay_textures_enabled_ && !native_ui_overlay_enabled_) {
        throw std::invalid_argument("rhi postprocess renderer textured native ui overlay requires native ui overlay");
    }

    postprocess_frame_graph_plan_ =
        compile_frame_graph_v1(make_postprocess_frame_graph_v1_desc(depth_input_enabled_, postprocess_stage_count_));
    const auto expected_pass_count = 1U + postprocess_stage_count_;
    if (!postprocess_frame_graph_plan_.succeeded() || postprocess_frame_graph_plan_.pass_count != expected_pass_count) {
        throw std::logic_error("rhi postprocess renderer frame graph v1 is invalid");
    }
    postprocess_frame_graph_execution_ = schedule_frame_graph_v1_execution(postprocess_frame_graph_plan_);
    frame_graph_pass_count_ = static_cast<std::uint32_t>(postprocess_frame_graph_plan_.ordered_passes.size());

    recreate_scene_color_texture();
    recreate_scene_depth_texture();
    recreate_post_chain_work_texture();
    scene_color_sampler_ = device_->create_sampler(rhi::SamplerDesc{});
    if (depth_input_enabled_) {
        scene_depth_sampler_ = device_->create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::nearest,
            .mag_filter = rhi::SamplerFilter::nearest,
            .address_u = rhi::SamplerAddressMode::clamp_to_edge,
            .address_v = rhi::SamplerAddressMode::clamp_to_edge,
            .address_w = rhi::SamplerAddressMode::clamp_to_edge,
        });
    }
    std::vector<rhi::DescriptorBindingDesc> first_bindings{
        rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_color_texture_binding(),
            .type = rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
        rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_color_sampler_binding(),
            .type = rhi::DescriptorType::sampler,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
    };
    if (depth_input_enabled_) {
        first_bindings.push_back(rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_depth_texture_binding(),
            .type = rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        });
        first_bindings.push_back(rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_depth_sampler_binding(),
            .type = rhi::DescriptorType::sampler,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        });
    }
    postprocess_first_descriptor_set_layout_ =
        device_->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{first_bindings});
    postprocess_first_descriptor_set_ = device_->allocate_descriptor_set(postprocess_first_descriptor_set_layout_);
    if (postprocess_stage_count_ >= 2) {
        postprocess_chain_descriptor_set_layout_ = device_->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = postprocess_scene_color_texture_binding(),
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = postprocess_scene_color_sampler_binding(),
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
        }});
        postprocess_chain_descriptor_set_ = device_->allocate_descriptor_set(postprocess_chain_descriptor_set_layout_);
    }
    update_postprocess_descriptors();
    postprocess_first_pipeline_layout_ = device_->create_pipeline_layout(rhi::PipelineLayoutDesc{
        .descriptor_sets = {postprocess_first_descriptor_set_layout_}, .push_constant_bytes = 0});
    postprocess_first_pipeline_ = device_->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
        .layout = postprocess_first_pipeline_layout_,
        .vertex_shader = postprocess_vertex_shader_,
        .fragment_shader = postprocess_fragment_stages_[0],
        .color_format = color_format_,
        .depth_format = rhi::Format::unknown,
        .topology = rhi::PrimitiveTopology::triangle_list,
    });
    if (postprocess_stage_count_ >= 2) {
        postprocess_chain_pipeline_layout_ = device_->create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {postprocess_chain_descriptor_set_layout_}, .push_constant_bytes = 0});
        postprocess_chain_pipeline_ = device_->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = postprocess_chain_pipeline_layout_,
            .vertex_shader = postprocess_vertex_shader_,
            .fragment_shader = postprocess_fragment_stages_[1],
            .color_format = color_format_,
            .depth_format = rhi::Format::unknown,
            .topology = rhi::PrimitiveTopology::triangle_list,
        });
    }
    if (native_ui_overlay_enabled_) {
        native_ui_overlay_ = std::make_unique<RhiNativeUiOverlay>(RhiNativeUiOverlayDesc{
            .device = device_,
            .extent = extent_,
            .color_format = color_format_,
            .vertex_shader = desc.native_ui_overlay_vertex_shader,
            .fragment_shader = desc.native_ui_overlay_fragment_shader,
            .atlas = desc.native_ui_overlay_atlas,
            .enable_textures = native_ui_overlay_textures_enabled_,
        });
    }
}

RhiPostprocessFrameRenderer::~RhiPostprocessFrameRenderer() {
    release_acquired_swapchain_frame();
}

std::string_view RhiPostprocessFrameRenderer::backend_name() const noexcept {
    return device_->backend_name();
}

Extent2D RhiPostprocessFrameRenderer::backbuffer_extent() const noexcept {
    return extent_;
}

RendererStats RhiPostprocessFrameRenderer::stats() const noexcept {
    return stats_;
}

Color RhiPostprocessFrameRenderer::clear_color() const noexcept {
    return clear_color_;
}

bool RhiPostprocessFrameRenderer::frame_active() const noexcept {
    return frame_active_;
}

bool RhiPostprocessFrameRenderer::postprocess_ready() const noexcept {
    const auto first_ready =
        scene_color_texture_.value != 0 && scene_color_sampler_.value != 0 &&
        (!depth_input_enabled_ || (scene_depth_texture_.value != 0 && scene_depth_sampler_.value != 0)) &&
        postprocess_first_descriptor_set_layout_.value != 0 && postprocess_first_descriptor_set_.value != 0 &&
        postprocess_first_pipeline_layout_.value != 0 && postprocess_first_pipeline_.value != 0;
    if (postprocess_stage_count_ < 2) {
        return first_ready;
    }
    return first_ready && post_chain_work_texture_.value != 0 && postprocess_chain_descriptor_set_layout_.value != 0 &&
           postprocess_chain_descriptor_set_.value != 0 && postprocess_chain_pipeline_layout_.value != 0 &&
           postprocess_chain_pipeline_.value != 0;
}

bool RhiPostprocessFrameRenderer::native_ui_overlay_ready() const noexcept {
    return native_ui_overlay_enabled_ && native_ui_overlay_ != nullptr && native_ui_overlay_->ready();
}

bool RhiPostprocessFrameRenderer::native_ui_overlay_atlas_ready() const noexcept {
    return native_ui_overlay_ready() && native_ui_overlay_->atlas_ready();
}

void RhiPostprocessFrameRenderer::resize(Extent2D extent) {
    if (frame_active_) {
        throw std::logic_error("rhi postprocess renderer cannot resize during an active frame");
    }
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("rhi postprocess renderer extent must be non-zero");
    }
    if (extent.width == extent_.width && extent.height == extent_.height) {
        return;
    }
    device_->resize_swapchain(swapchain_, rhi::Extent2D{.width = extent.width, .height = extent.height});
    extent_ = extent;
    recreate_scene_color_texture();
    recreate_scene_depth_texture();
    recreate_post_chain_work_texture();
    update_postprocess_descriptors();
    if (native_ui_overlay_ != nullptr) {
        native_ui_overlay_->resize(extent_);
    }
}

void RhiPostprocessFrameRenderer::set_clear_color(Color color) noexcept {
    clear_color_ = color;
}

void RhiPostprocessFrameRenderer::begin_frame() {
    if (frame_active_) {
        throw std::logic_error("rhi postprocess renderer frame already active");
    }

    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
    pending_overlay_sprites_.clear();
    std::unique_ptr<rhi::IRhiCommandList> commands;
    try {
        swapchain_frame_ = device_->acquire_swapchain_frame(swapchain_);
        commands = device_->begin_command_list(rhi::QueueKind::graphics);
        commands->transition_texture(scene_color_texture_, scene_color_state_, rhi::ResourceState::render_target);
        if (depth_input_enabled_ && scene_depth_state_ != rhi::ResourceState::depth_write) {
            commands->transition_texture(scene_depth_texture_, scene_depth_state_, rhi::ResourceState::depth_write);
        }
        rhi::RenderPassDesc scene_pass{
            .color =
                rhi::RenderPassColorAttachment{
                    .texture = scene_color_texture_,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .swapchain_frame = rhi::SwapchainFrameHandle{},
                    .clear_color = rhi::ClearColorValue{.red = clear_color_.r,
                                                        .green = clear_color_.g,
                                                        .blue = clear_color_.b,
                                                        .alpha = clear_color_.a},
                },
        };
        if (depth_input_enabled_) {
            scene_pass.depth = rhi::RenderPassDepthAttachment{
                .texture = scene_depth_texture_,
                .load_action = rhi::LoadAction::clear,
                .store_action = rhi::StoreAction::store,
                .clear_depth = rhi::ClearDepthValue{1.0F},
            };
        }
        commands->begin_render_pass(scene_pass);
        commands->bind_graphics_pipeline(scene_graphics_pipeline_);

        commands_ = std::move(commands);
        frame_active_ = true;
        skinned_scene_pipeline_bound_ = false;
        morph_scene_pipeline_bound_ = false;
        ++stats_.frames_started;
    } catch (...) {
        release_acquired_swapchain_frame();
        commands_.reset();
        frame_active_ = false;
        throw;
    }
}

void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand& command) {
    require_active_frame();
    if (native_ui_overlay_ready()) {
        pending_overlay_sprites_.push_back(command);
        ++stats_.native_ui_overlay_sprites_submitted;
        if (command.texture.enabled) {
            ++stats_.native_ui_overlay_textured_sprites_submitted;
        }
    }
    ++stats_.sprites_submitted;
}

void RhiPostprocessFrameRenderer::draw_mesh(const MeshCommand& command) {
    require_active_frame();
    if (command.gpu_skinning && command.gpu_morphing) {
        throw std::invalid_argument(
            "rhi postprocess renderer mesh command cannot enable gpu skinning and morphing together");
    }
    if (command.gpu_skinning) {
        if (scene_skinned_graphics_pipeline_.value == 0) {
            throw std::invalid_argument(
                "rhi postprocess renderer skinned mesh command requires a skinned scene pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_);
        validate_skinned_mesh_gpu_binding(command.skinned_mesh, *device_);
        commands_->bind_graphics_pipeline(scene_skinned_graphics_pipeline_);
        skinned_scene_pipeline_bound_ = true;
        morph_scene_pipeline_bound_ = false;
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout,
                                       command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout, 1,
                                       command.skinned_mesh.joint_descriptor_set);
        bind_mesh_vertex_buffers(*commands_, command.skinned_mesh.mesh);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.skinned_mesh.mesh.index_buffer,
            .offset = command.skinned_mesh.mesh.index_offset,
            .format = command.skinned_mesh.mesh.index_format,
        });
        commands_->draw_indexed(command.skinned_mesh.mesh.index_count, 1);
        ++stats_.meshes_submitted;
        ++stats_.gpu_skinning_draws;
        ++stats_.skinned_palette_descriptor_binds;
        return;
    }

    if (command.gpu_morphing) {
        if (scene_morph_graphics_pipeline_.value == 0) {
            throw std::invalid_argument("rhi postprocess renderer morph mesh command requires a morph scene pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_);
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
        validate_morph_mesh_gpu_binding(command.morph_mesh, *device_, command.mesh_binding.vertex_count);

        commands_->bind_graphics_pipeline(scene_morph_graphics_pipeline_);
        skinned_scene_pipeline_bound_ = false;
        morph_scene_pipeline_bound_ = true;
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
        ++stats_.meshes_submitted;
        ++stats_.gpu_morph_draws;
        ++stats_.morph_descriptor_binds;
        return;
    }

    if (skinned_scene_pipeline_bound_ || morph_scene_pipeline_bound_) {
        commands_->bind_graphics_pipeline(scene_graphics_pipeline_);
        skinned_scene_pipeline_bound_ = false;
        morph_scene_pipeline_bound_ = false;
    }
    if (has_material_gpu_binding(command.material_binding)) {
        validate_material_gpu_binding(command.material_binding, *device_);
        commands_->bind_descriptor_set(command.material_binding.pipeline_layout,
                                       command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
    }

    if (has_mesh_gpu_binding(command.mesh_binding)) {
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
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
    ++stats_.meshes_submitted;
}

void RhiPostprocessFrameRenderer::end_frame() {
    require_active_frame();
    try {
        commands_->end_render_pass();
        commands_->transition_texture(scene_color_texture_, rhi::ResourceState::render_target,
                                      rhi::ResourceState::shader_read);
        if (depth_input_enabled_) {
            commands_->transition_texture(scene_depth_texture_, rhi::ResourceState::depth_write,
                                          rhi::ResourceState::shader_read);
        }
        const auto overlay_draw = native_ui_overlay_ready()
                                      ? native_ui_overlay_->prepare(pending_overlay_sprites_, *commands_)
                                      : RhiNativeUiOverlayPreparedDraw{};
        if (postprocess_stage_count_ == 1) {
            commands_->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = rhi::TextureHandle{},
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = swapchain_frame_,
                        .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                    },
            });
            commands_->bind_graphics_pipeline(postprocess_first_pipeline_);
            commands_->bind_descriptor_set(postprocess_first_pipeline_layout_, 0, postprocess_first_descriptor_set_);
            commands_->draw(3, 1);
            if (native_ui_overlay_ready()) {
                native_ui_overlay_->record_draw(overlay_draw, *commands_);
            }
            commands_->end_render_pass();
        } else {
            commands_->transition_texture(post_chain_work_texture_, post_chain_work_state_,
                                          rhi::ResourceState::render_target);
            post_chain_work_state_ = rhi::ResourceState::render_target;
            commands_->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = post_chain_work_texture_,
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = rhi::SwapchainFrameHandle{},
                        .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                    },
            });
            commands_->bind_graphics_pipeline(postprocess_first_pipeline_);
            commands_->bind_descriptor_set(postprocess_first_pipeline_layout_, 0, postprocess_first_descriptor_set_);
            commands_->draw(3, 1);
            commands_->end_render_pass();
            commands_->transition_texture(post_chain_work_texture_, rhi::ResourceState::render_target,
                                          rhi::ResourceState::shader_read);
            post_chain_work_state_ = rhi::ResourceState::shader_read;
            if (depth_input_enabled_) {
                commands_->transition_texture(scene_depth_texture_, rhi::ResourceState::shader_read,
                                              rhi::ResourceState::depth_write);
            }
            commands_->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = rhi::TextureHandle{},
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = swapchain_frame_,
                        .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                    },
            });
            commands_->bind_graphics_pipeline(postprocess_chain_pipeline_);
            commands_->bind_descriptor_set(postprocess_chain_pipeline_layout_, 0, postprocess_chain_descriptor_set_);
            commands_->draw(3, 1);
            if (native_ui_overlay_ready()) {
                native_ui_overlay_->record_draw(overlay_draw, *commands_);
            }
            commands_->end_render_pass();
        }
        if (depth_input_enabled_ && postprocess_stage_count_ == 1) {
            commands_->transition_texture(scene_depth_texture_, rhi::ResourceState::shader_read,
                                          rhi::ResourceState::depth_write);
        }
        commands_->present(swapchain_frame_);
        swapchain_frame_presented_ = true;
        commands_->close();

        const auto fence = device_->submit(*commands_);
        if (wait_for_completion_) {
            device_->wait(fence);
        }

        commands_.reset();
        swapchain_frame_ = {};
        swapchain_frame_presented_ = false;
        scene_color_state_ = rhi::ResourceState::shader_read;
        if (depth_input_enabled_) {
            scene_depth_state_ = rhi::ResourceState::depth_write;
        }
        if (postprocess_stage_count_ >= 2) {
            post_chain_work_state_ = rhi::ResourceState::shader_read;
        }
        pending_overlay_sprites_.clear();
        frame_active_ = false;
        ++stats_.frames_finished;
        for (const auto& step : postprocess_frame_graph_execution_) {
            if (step.kind == FrameGraphExecutionStep::Kind::barrier) {
                ++stats_.framegraph_barrier_steps_executed;
            }
        }
        stats_.framegraph_passes_executed += frame_graph_pass_count();
        stats_.postprocess_passes_executed += postprocess_stage_count_;
        if (overlay_draw.vertex_count != 0) {
            ++stats_.native_ui_overlay_draws;
        }
        stats_.native_ui_overlay_texture_binds += overlay_draw.texture_bind_count;
        if (overlay_draw.textured_sprite_count != 0) {
            ++stats_.native_ui_overlay_textured_draws;
        }
    } catch (...) {
        release_acquired_swapchain_frame();
        commands_.reset();
        pending_overlay_sprites_.clear();
        frame_active_ = false;
        throw;
    }
}

void RhiPostprocessFrameRenderer::require_active_frame() const {
    if (!frame_active_ || commands_ == nullptr) {
        throw std::logic_error("rhi postprocess renderer frame is not active");
    }
}

void RhiPostprocessFrameRenderer::release_acquired_swapchain_frame() noexcept {
    if (device_ == nullptr || swapchain_frame_.value == 0) {
        swapchain_frame_presented_ = false;
        return;
    }

    if (!swapchain_frame_presented_) {
        try {
            device_->release_swapchain_frame(swapchain_frame_);
        } catch (...) {
        }
    }

    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
}

void RhiPostprocessFrameRenderer::recreate_scene_color_texture() {
    scene_color_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = color_format_,
        .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::shader_resource,
    });
    scene_color_state_ = rhi::ResourceState::undefined;
}

void RhiPostprocessFrameRenderer::recreate_scene_depth_texture() {
    if (!depth_input_enabled_) {
        scene_depth_texture_ = {};
        scene_depth_state_ = rhi::ResourceState::undefined;
        return;
    }

    scene_depth_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = depth_format_,
        .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
    });
    scene_depth_state_ = rhi::ResourceState::undefined;
}

void RhiPostprocessFrameRenderer::update_postprocess_descriptors() {
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_first_descriptor_set_,
        .binding = postprocess_scene_color_texture_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, scene_color_texture_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_first_descriptor_set_,
        .binding = postprocess_scene_color_sampler_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::sampler(scene_color_sampler_)},
    });
    if (depth_input_enabled_) {
        device_->update_descriptor_set(rhi::DescriptorWrite{
            .set = postprocess_first_descriptor_set_,
            .binding = postprocess_scene_depth_texture_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, scene_depth_texture_)},
        });
        device_->update_descriptor_set(rhi::DescriptorWrite{
            .set = postprocess_first_descriptor_set_,
            .binding = postprocess_scene_depth_sampler_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::sampler(scene_depth_sampler_)},
        });
    }
    if (postprocess_stage_count_ >= 2 && postprocess_chain_descriptor_set_.value != 0) {
        device_->update_descriptor_set(rhi::DescriptorWrite{
            .set = postprocess_chain_descriptor_set_,
            .binding = postprocess_scene_color_texture_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture,
                                                           post_chain_work_texture_)},
        });
        device_->update_descriptor_set(rhi::DescriptorWrite{
            .set = postprocess_chain_descriptor_set_,
            .binding = postprocess_scene_color_sampler_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::sampler(scene_color_sampler_)},
        });
    }
}

void RhiPostprocessFrameRenderer::recreate_post_chain_work_texture() {
    if (postprocess_stage_count_ < 2) {
        post_chain_work_texture_ = {};
        post_chain_work_state_ = rhi::ResourceState::undefined;
        return;
    }
    post_chain_work_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = color_format_,
        .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::shader_resource,
    });
    post_chain_work_state_ = rhi::ResourceState::undefined;
}

} // namespace mirakana
