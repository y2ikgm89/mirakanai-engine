// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp"

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "rhi_native_ui_overlay.hpp"

#include <algorithm>
#include <cmath>
#include <span>
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

[[nodiscard]] bool supported_shadow_filter_mode(ShadowReceiverFilterMode mode) noexcept {
    return mode == ShadowReceiverFilterMode::none || mode == ShadowReceiverFilterMode::fixed_pcf_3x3;
}

[[nodiscard]] std::uint32_t expected_shadow_filter_tap_count(ShadowReceiverFilterMode mode) noexcept {
    switch (mode) {
    case ShadowReceiverFilterMode::none:
        return 1;
    case ShadowReceiverFilterMode::fixed_pcf_3x3:
        return 9;
    }
    return 0;
}

[[nodiscard]] float expected_shadow_filter_radius_texels(ShadowReceiverFilterMode mode) noexcept {
    switch (mode) {
    case ShadowReceiverFilterMode::none:
        return 0.0F;
    case ShadowReceiverFilterMode::fixed_pcf_3x3:
        return 1.0F;
    }
    return -1.0F;
}

void validate_mesh_gpu_binding(const MeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (binding.vertex_buffer.value == 0 || binding.index_buffer.value == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command requires vertex and index buffers");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command uses buffers from a different rhi device");
    }
    if (binding.vertex_stride == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command requires a vertex stride");
    }
    if (binding.vertex_count == 0 || binding.index_count == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command requires vertex and index counts");
    }
    if (binding.index_format == rhi::IndexFormat::unknown) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command requires an index format");
    }
    if ((binding.normal_vertex_buffer.value != 0) != (binding.normal_vertex_stride != 0) ||
        (binding.tangent_vertex_buffer.value != 0) != (binding.tangent_vertex_stride != 0)) {
        throw std::invalid_argument("rhi shadow smoke renderer mesh command has inconsistent optional vertex streams");
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

void validate_skinned_mesh_gpu_binding(const SkinnedMeshGpuBinding& binding, const rhi::IRhiDevice& device) {
    if (!has_skinned_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi shadow smoke renderer skinned mesh command requires skinned gpu binding data");
    }
    validate_mesh_gpu_binding(binding.mesh, device);
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer skinned mesh command uses resources from a different rhi device");
    }
}

void validate_morph_mesh_gpu_binding(const MorphMeshGpuBinding& binding, const rhi::IRhiDevice& device,
                                     std::uint32_t base_vertex_count) {
    if (!has_morph_mesh_gpu_binding(binding)) {
        throw std::invalid_argument("rhi shadow smoke renderer morph mesh command requires morph gpu binding data");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi shadow smoke renderer morph mesh command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer morph mesh command uses resources from a different rhi device");
    }
    if (binding.position_delta_buffer.value == 0 || binding.morph_weight_buffer.value == 0 ||
        binding.morph_descriptor_set.value == 0) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer morph mesh command requires morph buffers and descriptor set");
    }
    if (binding.vertex_count == 0 || binding.target_count == 0 || binding.position_delta_bytes == 0 ||
        binding.morph_weight_uniform_allocation_bytes == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer morph mesh command requires morph metadata");
    }
    if ((binding.normal_delta_buffer.value != 0) != (binding.normal_delta_bytes != 0) ||
        (binding.tangent_delta_buffer.value != 0) != (binding.tangent_delta_bytes != 0)) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer morph mesh command has inconsistent optional morph streams");
    }
    if (binding.vertex_count != base_vertex_count) {
        throw std::invalid_argument("rhi shadow smoke renderer morph mesh command vertex count must match base mesh");
    }
}

void validate_material_gpu_binding(const MaterialGpuBinding& binding, const rhi::IRhiDevice& device,
                                   rhi::PipelineLayoutHandle expected_layout) {
    if (binding.pipeline_layout.value == 0 || binding.descriptor_set.value == 0) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer material command requires pipeline layout and descriptor set");
    }
    if (binding.pipeline_layout.value != expected_layout.value) {
        throw std::invalid_argument("rhi shadow smoke renderer material command uses a different pipeline layout");
    }
    if (binding.owner_device == nullptr) {
        throw std::invalid_argument("rhi shadow smoke renderer material command requires an owner rhi device");
    }
    if (binding.owner_device != &device) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer material command uses descriptors from a different rhi device");
    }
}

[[nodiscard]] FrameGraphV1Desc make_shadow_smoke_frame_graph_v1_desc() {
    FrameGraphV1Desc desc;
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "swapchain", .lifetime = FrameGraphResourceLifetime::imported});
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "shadow_depth", .lifetime = FrameGraphResourceLifetime::transient});
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "scene_color", .lifetime = FrameGraphResourceLifetime::transient});
    desc.resources.push_back(
        FrameGraphResourceV1Desc{.name = "scene_depth", .lifetime = FrameGraphResourceLifetime::transient});

    desc.passes.push_back(FrameGraphPassV1Desc{
        .name = "shadow.directional.depth",
        .reads = {},
        .writes = {FrameGraphResourceAccess{.resource = "shadow_depth",
                                            .access = FrameGraphAccess::depth_attachment_write}},
    });
    desc.passes.push_back(FrameGraphPassV1Desc{
        .name = "scene.shadow_receiver",
        .reads = {FrameGraphResourceAccess{.resource = "shadow_depth", .access = FrameGraphAccess::shader_read}},
        .writes = {FrameGraphResourceAccess{.resource = "scene_color",
                                            .access = FrameGraphAccess::color_attachment_write},
                   FrameGraphResourceAccess{.resource = "scene_depth",
                                            .access = FrameGraphAccess::depth_attachment_write}},
    });
    desc.passes.push_back(FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {FrameGraphResourceAccess{.resource = "scene_color", .access = FrameGraphAccess::shader_read},
                  FrameGraphResourceAccess{.resource = "scene_depth", .access = FrameGraphAccess::shader_read}},
        .writes = {FrameGraphResourceAccess{.resource = "swapchain",
                                            .access = FrameGraphAccess::color_attachment_write}},
    });
    return desc;
}

} // namespace

RhiDirectionalShadowSmokeFrameRenderer::RhiDirectionalShadowSmokeFrameRenderer(
    const RhiDirectionalShadowSmokeFrameRendererDesc& desc)
    : device_(desc.device), extent_(desc.extent), swapchain_(desc.swapchain), color_format_(desc.color_format),
      scene_depth_format_(desc.scene_depth_format), shadow_depth_format_(desc.shadow_depth_format),
      scene_graphics_pipeline_(desc.scene_graphics_pipeline),
      scene_skinned_graphics_pipeline_(desc.scene_skinned_graphics_pipeline),
      scene_morph_graphics_pipeline_(desc.scene_morph_graphics_pipeline),
      scene_pipeline_layout_(desc.scene_pipeline_layout), shadow_graphics_pipeline_(desc.shadow_graphics_pipeline),
      shadow_receiver_descriptor_set_layout_(desc.shadow_receiver_descriptor_set_layout),
      shadow_receiver_descriptor_set_index_(desc.shadow_receiver_descriptor_set_index),
      postprocess_vertex_shader_(desc.postprocess_vertex_shader), postprocess_fragment_shader_{},
      wait_for_completion_(desc.wait_for_completion), shadow_filter_mode_(desc.shadow_filter_mode),
      shadow_filter_radius_texels_(desc.shadow_filter_radius_texels),
      shadow_filter_tap_count_(desc.shadow_filter_tap_count),
      directional_shadow_cascade_count_(std::clamp(desc.directional_shadow_cascade_count, 1U, 8U)),
      native_ui_overlay_enabled_(desc.enable_native_ui_overlay),
      native_ui_overlay_textures_enabled_(desc.enable_native_ui_overlay_textures) {
    if (device_ == nullptr) {
        throw std::invalid_argument("rhi shadow smoke renderer requires an rhi device");
    }
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer extent must be non-zero");
    }
    if (swapchain_.value == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer requires a swapchain");
    }
    if (color_format_ == rhi::Format::unknown || color_format_ == rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("rhi shadow smoke renderer requires a color format");
    }
    if (scene_depth_format_ != rhi::Format::depth24_stencil8 || shadow_depth_format_ != rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("rhi shadow smoke renderer requires depth24_stencil8 depth formats");
    }
    if (scene_graphics_pipeline_.value == 0 || shadow_graphics_pipeline_.value == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer requires scene and shadow pipelines");
    }
    if (scene_pipeline_layout_.value == 0 || shadow_receiver_descriptor_set_layout_.value == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer requires scene and shadow descriptor layouts");
    }
    if (shadow_receiver_descriptor_set_index_ == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer shadow receiver descriptor set index must be non-zero");
    }
    if (desc.postprocess_fragment_stages.size() != 1) {
        throw std::invalid_argument("rhi shadow smoke renderer requires exactly one postprocess fragment stage");
    }
    postprocess_fragment_shader_ = desc.postprocess_fragment_stages.front();
    if (postprocess_vertex_shader_.value == 0 || postprocess_fragment_shader_.value == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer requires postprocess shaders");
    }
    if (!supported_shadow_filter_mode(shadow_filter_mode_) || shadow_filter_tap_count_ == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer requires a supported shadow filter policy");
    }
    if (!std::isfinite(shadow_filter_radius_texels_) || shadow_filter_radius_texels_ < 0.0F ||
        shadow_filter_radius_texels_ > 8.0F) {
        throw std::invalid_argument("rhi shadow smoke renderer requires a finite shadow filter radius");
    }
    if (shadow_filter_tap_count_ != expected_shadow_filter_tap_count(shadow_filter_mode_) ||
        shadow_filter_radius_texels_ != expected_shadow_filter_radius_texels(shadow_filter_mode_)) {
        throw std::invalid_argument("rhi shadow smoke renderer requires coherent shadow filter metadata");
    }
    if (native_ui_overlay_enabled_ &&
        (desc.native_ui_overlay_vertex_shader.value == 0 || desc.native_ui_overlay_fragment_shader.value == 0)) {
        throw std::invalid_argument("rhi shadow smoke renderer native ui overlay requires overlay shaders");
    }
    if (native_ui_overlay_textures_enabled_ && !native_ui_overlay_enabled_) {
        throw std::invalid_argument("rhi shadow smoke renderer textured native ui overlay requires native ui overlay");
    }

    if (desc.shadow_depth_atlas_extent.width > 0 && desc.shadow_depth_atlas_extent.height > 0) {
        shadow_atlas_extent_ = desc.shadow_depth_atlas_extent;
        shadow_atlas_explicit_ = true;
        if (shadow_atlas_extent_.width % directional_shadow_cascade_count_ != 0) {
            throw std::invalid_argument(
                "rhi shadow smoke renderer shadow depth atlas width must be divisible by cascade count");
        }
    } else {
        shadow_atlas_explicit_ = false;
        recompute_shadow_atlas_extent();
    }

    shadow_smoke_frame_graph_plan_ = compile_frame_graph_v1(make_shadow_smoke_frame_graph_v1_desc());
    if (!shadow_smoke_frame_graph_plan_.succeeded() || shadow_smoke_frame_graph_plan_.pass_count != 3) {
        throw std::logic_error("rhi shadow smoke renderer frame graph v1 is invalid");
    }
    shadow_smoke_frame_graph_execution_ = schedule_frame_graph_v1_execution(shadow_smoke_frame_graph_plan_);
    frame_graph_pass_count_ = static_cast<std::uint32_t>(shadow_smoke_frame_graph_plan_.ordered_passes.size());

    recreate_shadow_textures();
    recreate_scene_textures();
    scene_color_sampler_ = device_->create_sampler(rhi::SamplerDesc{});
    scene_depth_sampler_ = device_->create_sampler(rhi::SamplerDesc{
        .min_filter = rhi::SamplerFilter::nearest,
        .mag_filter = rhi::SamplerFilter::nearest,
        .address_u = rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = rhi::SamplerAddressMode::clamp_to_edge,
    });
    shadow_sampler_ = device_->create_sampler(rhi::SamplerDesc{
        .min_filter = rhi::SamplerFilter::nearest,
        .mag_filter = rhi::SamplerFilter::nearest,
        .address_u = rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = rhi::SamplerAddressMode::clamp_to_edge,
    });
    shadow_receiver_constants_buffer_ = device_->create_buffer(rhi::BufferDesc{
        .size_bytes = shadow_receiver_constants_byte_size(),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (shadow_receiver_constants_buffer_.value != 0) {
        device_->write_buffer(shadow_receiver_constants_buffer_, 0,
                              std::span<const std::uint8_t>(desc.shadow_receiver_constants_initial.data(),
                                                            desc.shadow_receiver_constants_initial.size()));
    }
    shadow_receiver_descriptor_set_ = device_->allocate_descriptor_set(shadow_receiver_descriptor_set_layout_);
    postprocess_descriptor_set_layout_ = device_->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
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
        rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_depth_texture_binding(),
            .type = rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
        rhi::DescriptorBindingDesc{
            .binding = postprocess_scene_depth_sampler_binding(),
            .type = rhi::DescriptorType::sampler,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
    }});
    postprocess_descriptor_set_ = device_->allocate_descriptor_set(postprocess_descriptor_set_layout_);
    update_descriptors();
    postprocess_pipeline_layout_ = device_->create_pipeline_layout(
        rhi::PipelineLayoutDesc{.descriptor_sets = {postprocess_descriptor_set_layout_}, .push_constant_bytes = 0});
    postprocess_pipeline_ = device_->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
        .layout = postprocess_pipeline_layout_,
        .vertex_shader = postprocess_vertex_shader_,
        .fragment_shader = postprocess_fragment_shader_,
        .color_format = color_format_,
        .depth_format = rhi::Format::unknown,
        .topology = rhi::PrimitiveTopology::triangle_list,
    });
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

RhiDirectionalShadowSmokeFrameRenderer::~RhiDirectionalShadowSmokeFrameRenderer() {
    release_acquired_swapchain_frame();
}

std::string_view RhiDirectionalShadowSmokeFrameRenderer::backend_name() const noexcept {
    return device_->backend_name();
}

Extent2D RhiDirectionalShadowSmokeFrameRenderer::backbuffer_extent() const noexcept {
    return extent_;
}

RendererStats RhiDirectionalShadowSmokeFrameRenderer::stats() const noexcept {
    return stats_;
}

Color RhiDirectionalShadowSmokeFrameRenderer::clear_color() const noexcept {
    return clear_color_;
}

bool RhiDirectionalShadowSmokeFrameRenderer::frame_active() const noexcept {
    return frame_active_;
}

bool RhiDirectionalShadowSmokeFrameRenderer::directional_shadow_ready() const noexcept {
    return scene_color_texture_.value != 0 && scene_depth_texture_.value != 0 && shadow_color_texture_.value != 0 &&
           shadow_depth_texture_.value != 0 && scene_color_sampler_.value != 0 && scene_depth_sampler_.value != 0 &&
           shadow_sampler_.value != 0 && shadow_receiver_constants_buffer_.value != 0 &&
           shadow_receiver_descriptor_set_.value != 0 && postprocess_descriptor_set_layout_.value != 0 &&
           postprocess_descriptor_set_.value != 0 && postprocess_pipeline_layout_.value != 0 &&
           postprocess_pipeline_.value != 0;
}

bool RhiDirectionalShadowSmokeFrameRenderer::native_ui_overlay_ready() const noexcept {
    return native_ui_overlay_enabled_ && native_ui_overlay_ != nullptr && native_ui_overlay_->ready();
}

bool RhiDirectionalShadowSmokeFrameRenderer::native_ui_overlay_atlas_ready() const noexcept {
    return native_ui_overlay_ready() && native_ui_overlay_->atlas_ready();
}

ShadowReceiverFilterMode RhiDirectionalShadowSmokeFrameRenderer::shadow_filter_mode() const noexcept {
    return shadow_filter_mode_;
}

float RhiDirectionalShadowSmokeFrameRenderer::shadow_filter_radius_texels() const noexcept {
    return shadow_filter_radius_texels_;
}

std::uint32_t RhiDirectionalShadowSmokeFrameRenderer::shadow_filter_tap_count() const noexcept {
    return shadow_filter_tap_count_;
}

void RhiDirectionalShadowSmokeFrameRenderer::resize(Extent2D extent) {
    if (frame_active_) {
        throw std::logic_error("rhi shadow smoke renderer cannot resize during an active frame");
    }
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("rhi shadow smoke renderer extent must be non-zero");
    }
    if (extent.width == extent_.width && extent.height == extent_.height) {
        return;
    }
    device_->resize_swapchain(swapchain_, rhi::Extent2D{.width = extent.width, .height = extent.height});
    extent_ = extent;
    if (!shadow_atlas_explicit_) {
        recompute_shadow_atlas_extent();
    }
    recreate_shadow_textures();
    recreate_scene_textures();
    update_descriptors();
    if (native_ui_overlay_ != nullptr) {
        native_ui_overlay_->resize(extent_);
    }
}

void RhiDirectionalShadowSmokeFrameRenderer::set_clear_color(Color color) noexcept {
    clear_color_ = color;
}

void RhiDirectionalShadowSmokeFrameRenderer::begin_frame() {
    if (frame_active_) {
        throw std::logic_error("rhi shadow smoke renderer frame already active");
    }

    pending_meshes_.clear();
    pending_overlay_sprites_.clear();
    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
    std::unique_ptr<rhi::IRhiCommandList> commands;
    try {
        swapchain_frame_ = device_->acquire_swapchain_frame(swapchain_);
        commands = device_->begin_command_list(rhi::QueueKind::graphics);
        if (shadow_color_state_ != rhi::ResourceState::render_target) {
            commands->transition_texture(shadow_color_texture_, shadow_color_state_, rhi::ResourceState::render_target);
        }
        if (shadow_depth_state_ != rhi::ResourceState::depth_write) {
            commands->transition_texture(shadow_depth_texture_, shadow_depth_state_, rhi::ResourceState::depth_write);
        }
        commands->begin_render_pass(rhi::RenderPassDesc{
            .color =
                rhi::RenderPassColorAttachment{
                    .texture = shadow_color_texture_,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::dont_care,
                    .swapchain_frame = rhi::SwapchainFrameHandle{},
                    .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                },
            .depth =
                rhi::RenderPassDepthAttachment{
                    .texture = shadow_depth_texture_,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .clear_depth = rhi::ClearDepthValue{1.0F},
                },
        });
        commands->bind_graphics_pipeline(shadow_graphics_pipeline_);

        commands_ = std::move(commands);
        frame_active_ = true;
        ++stats_.frames_started;
    } catch (...) {
        release_acquired_swapchain_frame();
        commands_.reset();
        frame_active_ = false;
        throw;
    }
}

void RhiDirectionalShadowSmokeFrameRenderer::draw_sprite(const SpriteCommand& command) {
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

void RhiDirectionalShadowSmokeFrameRenderer::draw_mesh(const MeshCommand& command) {
    require_active_frame();
    if (command.gpu_skinning && command.gpu_morphing) {
        throw std::invalid_argument(
            "rhi shadow smoke renderer mesh command cannot enable gpu skinning and morphing together");
    }
    if (command.gpu_morphing) {
        if (scene_morph_graphics_pipeline_.value == 0) {
            throw std::invalid_argument("rhi shadow smoke renderer morph mesh command requires a morph scene pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_, scene_pipeline_layout_);
        validate_morph_mesh_gpu_binding(command.morph_mesh, *device_, command.mesh_binding.vertex_count);
    }
    if (command.gpu_skinning) {
        validate_material_gpu_binding(command.material_binding, *device_, scene_pipeline_layout_);
        validate_skinned_mesh_gpu_binding(command.skinned_mesh, *device_);
    } else if (has_mesh_gpu_binding(command.mesh_binding)) {
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
    }
    if (has_material_gpu_binding(command.material_binding)) {
        validate_material_gpu_binding(command.material_binding, *device_, scene_pipeline_layout_);
    }
    pending_meshes_.push_back(command);
    ++stats_.meshes_submitted;
}

void RhiDirectionalShadowSmokeFrameRenderer::end_frame() {
    require_active_frame();
    try {
        for (std::uint32_t cascade = 0; cascade < directional_shadow_cascade_count_; ++cascade) {
            const auto tile_w = shadow_cascade_tile_width();
            const auto x = static_cast<float>(cascade * tile_w);
            commands_->set_viewport(rhi::ViewportDesc{
                .x = x,
                .y = 0.0F,
                .width = static_cast<float>(tile_w),
                .height = static_cast<float>(shadow_atlas_extent_.height),
                .min_depth = 0.0F,
                .max_depth = 1.0F,
            });
            commands_->set_scissor(rhi::ScissorRectDesc{
                .x = cascade * tile_w,
                .y = 0U,
                .width = tile_w,
                .height = shadow_atlas_extent_.height,
            });
            for (const auto& command : pending_meshes_) {
                record_shadow_mesh_draw(command);
            }
        }
        commands_->end_render_pass();
        commands_->transition_texture(shadow_depth_texture_, rhi::ResourceState::depth_write,
                                      rhi::ResourceState::shader_read);
        commands_->transition_texture(scene_color_texture_, scene_color_state_, rhi::ResourceState::render_target);
        if (scene_depth_state_ != rhi::ResourceState::depth_write) {
            commands_->transition_texture(scene_depth_texture_, scene_depth_state_, rhi::ResourceState::depth_write);
        }
        commands_->begin_render_pass(rhi::RenderPassDesc{
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
            .depth =
                rhi::RenderPassDepthAttachment{
                    .texture = scene_depth_texture_,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .clear_depth = rhi::ClearDepthValue{1.0F},
                },
        });
        commands_->bind_graphics_pipeline(scene_graphics_pipeline_);
        commands_->bind_descriptor_set(scene_pipeline_layout_, shadow_receiver_descriptor_set_index_,
                                       shadow_receiver_descriptor_set_);
        for (const auto& command : pending_meshes_) {
            record_scene_mesh_draw(command);
        }
        commands_->end_render_pass();
        commands_->transition_texture(scene_color_texture_, rhi::ResourceState::render_target,
                                      rhi::ResourceState::shader_read);
        commands_->transition_texture(scene_depth_texture_, rhi::ResourceState::depth_write,
                                      rhi::ResourceState::shader_read);
        const auto overlay_draw = native_ui_overlay_ready()
                                      ? native_ui_overlay_->prepare(pending_overlay_sprites_, *commands_)
                                      : RhiNativeUiOverlayPreparedDraw{};
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
        commands_->bind_graphics_pipeline(postprocess_pipeline_);
        commands_->bind_descriptor_set(postprocess_pipeline_layout_, 0, postprocess_descriptor_set_);
        commands_->draw(3, 1);
        if (native_ui_overlay_ready()) {
            native_ui_overlay_->record_draw(overlay_draw, *commands_);
        }
        commands_->end_render_pass();
        commands_->transition_texture(scene_depth_texture_, rhi::ResourceState::shader_read,
                                      rhi::ResourceState::depth_write);
        commands_->transition_texture(shadow_depth_texture_, rhi::ResourceState::shader_read,
                                      rhi::ResourceState::depth_write);
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
        shadow_color_state_ = rhi::ResourceState::render_target;
        scene_color_state_ = rhi::ResourceState::shader_read;
        scene_depth_state_ = rhi::ResourceState::depth_write;
        shadow_depth_state_ = rhi::ResourceState::depth_write;
        pending_meshes_.clear();
        pending_overlay_sprites_.clear();
        frame_active_ = false;
        ++stats_.frames_finished;
        for (const auto& step : shadow_smoke_frame_graph_execution_) {
            if (step.kind == FrameGraphExecutionStep::Kind::barrier) {
                ++stats_.framegraph_barrier_steps_executed;
            }
        }
        stats_.framegraph_passes_executed += frame_graph_pass_count();
        ++stats_.postprocess_passes_executed;
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
        pending_meshes_.clear();
        pending_overlay_sprites_.clear();
        frame_active_ = false;
        throw;
    }
}

void RhiDirectionalShadowSmokeFrameRenderer::require_active_frame() const {
    if (!frame_active_ || commands_ == nullptr) {
        throw std::logic_error("rhi shadow smoke renderer frame is not active");
    }
}

void RhiDirectionalShadowSmokeFrameRenderer::release_acquired_swapchain_frame() noexcept {
    if (device_ == nullptr || swapchain_frame_.value == 0 || swapchain_frame_presented_) {
        return;
    }

    (void)try_release_swapchain_frame(*device_, swapchain_frame_);

    swapchain_frame_ = {};
    swapchain_frame_presented_ = false;
}

void RhiDirectionalShadowSmokeFrameRenderer::recreate_scene_textures() {
    scene_color_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = color_format_,
        .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::shader_resource,
    });
    scene_color_state_ = rhi::ResourceState::undefined;
    scene_depth_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = scene_depth_format_,
        .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
    });
    scene_depth_state_ = rhi::ResourceState::undefined;
}

void RhiDirectionalShadowSmokeFrameRenderer::recompute_shadow_atlas_extent() {
    const auto tile = std::min<std::uint32_t>(512U, std::max(32U, std::min(extent_.width, extent_.height)));
    shadow_atlas_extent_.height = tile;
    shadow_atlas_extent_.width = tile * directional_shadow_cascade_count_;
}

std::uint32_t RhiDirectionalShadowSmokeFrameRenderer::shadow_cascade_tile_width() const noexcept {
    if (directional_shadow_cascade_count_ == 0) {
        return shadow_atlas_extent_.width;
    }
    return shadow_atlas_extent_.width / directional_shadow_cascade_count_;
}

void RhiDirectionalShadowSmokeFrameRenderer::recreate_shadow_textures() {
    shadow_color_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = shadow_atlas_extent_.width, .height = shadow_atlas_extent_.height, .depth = 1},
        .format = color_format_,
        .usage = rhi::TextureUsage::render_target,
    });
    shadow_color_state_ = rhi::ResourceState::undefined;
    shadow_depth_texture_ = device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = shadow_atlas_extent_.width, .height = shadow_atlas_extent_.height, .depth = 1},
        .format = shadow_depth_format_,
        .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
    });
    shadow_depth_state_ = rhi::ResourceState::undefined;
}

void RhiDirectionalShadowSmokeFrameRenderer::update_descriptors() {
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = shadow_receiver_descriptor_set_,
        .binding = shadow_receiver_depth_texture_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, shadow_depth_texture_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = shadow_receiver_descriptor_set_,
        .binding = shadow_receiver_sampler_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::sampler(shadow_sampler_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = shadow_receiver_descriptor_set_,
        .binding = shadow_receiver_constants_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                      shadow_receiver_constants_buffer_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_descriptor_set_,
        .binding = postprocess_scene_color_texture_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, scene_color_texture_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_descriptor_set_,
        .binding = postprocess_scene_color_sampler_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::sampler(scene_color_sampler_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_descriptor_set_,
        .binding = postprocess_scene_depth_texture_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, scene_depth_texture_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = postprocess_descriptor_set_,
        .binding = postprocess_scene_depth_sampler_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::sampler(scene_depth_sampler_)},
    });
}

void RhiDirectionalShadowSmokeFrameRenderer::record_shadow_mesh_draw(const MeshCommand& command) {
    if (has_mesh_gpu_binding(command.mesh_binding)) {
        bind_mesh_vertex_buffers(*commands_, command.mesh_binding);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.mesh_binding.index_buffer,
            .offset = command.mesh_binding.index_offset,
            .format = command.mesh_binding.index_format,
        });
        commands_->draw_indexed(command.mesh_binding.index_count, 1);
        return;
    }
    commands_->draw(3, 1);
}

void RhiDirectionalShadowSmokeFrameRenderer::record_scene_mesh_draw(const MeshCommand& command) {
    if (command.gpu_skinning) {
        if (scene_skinned_graphics_pipeline_.value == 0) {
            throw std::invalid_argument(
                "rhi shadow smoke renderer skinned mesh command requires a skinned scene graphics pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_, scene_pipeline_layout_);
        validate_skinned_mesh_gpu_binding(command.skinned_mesh, *device_);
        commands_->bind_graphics_pipeline(scene_skinned_graphics_pipeline_);
        commands_->bind_descriptor_set(scene_pipeline_layout_, command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
        commands_->bind_descriptor_set(scene_pipeline_layout_, 1, command.skinned_mesh.joint_descriptor_set);
        bind_mesh_vertex_buffers(*commands_, command.skinned_mesh.mesh);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.skinned_mesh.mesh.index_buffer,
            .offset = command.skinned_mesh.mesh.index_offset,
            .format = command.skinned_mesh.mesh.index_format,
        });
        commands_->draw_indexed(command.skinned_mesh.mesh.index_count, 1);
        ++stats_.gpu_skinning_draws;
        ++stats_.skinned_palette_descriptor_binds;
        commands_->bind_graphics_pipeline(scene_graphics_pipeline_);
        return;
    }

    if (command.gpu_morphing) {
        if (scene_morph_graphics_pipeline_.value == 0) {
            throw std::invalid_argument(
                "rhi shadow smoke renderer morph mesh command requires a morph scene graphics pipeline");
        }
        validate_material_gpu_binding(command.material_binding, *device_, scene_pipeline_layout_);
        validate_mesh_gpu_binding(command.mesh_binding, *device_);
        validate_morph_mesh_gpu_binding(command.morph_mesh, *device_, command.mesh_binding.vertex_count);
        commands_->bind_graphics_pipeline(scene_morph_graphics_pipeline_);
        commands_->bind_descriptor_set(scene_pipeline_layout_, command.material_binding.descriptor_set_index,
                                       command.material_binding.descriptor_set);
        commands_->bind_descriptor_set(scene_pipeline_layout_, 1, command.morph_mesh.morph_descriptor_set);
        bind_mesh_vertex_buffers(*commands_, command.mesh_binding);
        commands_->bind_index_buffer(rhi::IndexBufferBinding{
            .buffer = command.mesh_binding.index_buffer,
            .offset = command.mesh_binding.index_offset,
            .format = command.mesh_binding.index_format,
        });
        commands_->draw_indexed(command.mesh_binding.index_count, 1);
        ++stats_.gpu_morph_draws;
        ++stats_.morph_descriptor_binds;
        commands_->bind_graphics_pipeline(scene_graphics_pipeline_);
        return;
    }

    if (has_material_gpu_binding(command.material_binding)) {
        commands_->bind_descriptor_set(scene_pipeline_layout_, command.material_binding.descriptor_set_index,
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
        return;
    }
    commands_->draw(3, 1);
}

} // namespace mirakana
