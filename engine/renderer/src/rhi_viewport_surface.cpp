// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/rhi_viewport_surface.hpp"

#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"

#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace mirakana {

namespace {

void validate_extent(Extent2D extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("rhi viewport surface extent must be non-zero");
    }
}

[[nodiscard]] std::uint32_t align_up_u32(std::uint32_t value, std::uint32_t alignment) {
    if (alignment == 0) {
        throw std::invalid_argument("alignment must be non-zero");
    }
    const auto remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }
    const auto add = alignment - remainder;
    if (value > (std::numeric_limits<std::uint32_t>::max)() - add) {
        throw std::invalid_argument("aligned value overflowed");
    }
    return value + add;
}

[[nodiscard]] std::uint32_t viewport_readback_row_pitch(rhi::Format format, Extent2D extent) {
    const auto texel_bytes = rhi::bytes_per_texel(format);
    if (extent.width > (std::numeric_limits<std::uint32_t>::max)() / texel_bytes) {
        throw std::invalid_argument("viewport readback row pitch overflowed");
    }
    return align_up_u32(extent.width * texel_bytes, 256U);
}

[[nodiscard]] std::uint64_t checked_readback_size(std::uint32_t row_pitch, std::uint32_t height) {
    if (height != 0 && row_pitch > (std::numeric_limits<std::uint64_t>::max)() / height) {
        throw std::invalid_argument("viewport readback size overflowed");
    }
    return static_cast<std::uint64_t>(row_pitch) * height;
}

[[nodiscard]] rhi::ResourceState execute_viewport_color_state_transition(rhi::IRhiCommandList& commands,
                                                                         rhi::TextureHandle color_texture,
                                                                         rhi::ResourceState current_state,
                                                                         rhi::ResourceState target_state) {
    std::array<FrameGraphTextureBinding, 1> texture_bindings{FrameGraphTextureBinding{
        .resource = "viewport_color",
        .texture = color_texture,
        .current_state = current_state,
    }};
    const std::array<FrameGraphTextureFinalState, 1> final_states{FrameGraphTextureFinalState{
        .resource = "viewport_color",
        .state = target_state,
    }};

    const auto result = execute_frame_graph_rhi_texture_schedule(FrameGraphRhiTextureExecutionDesc{
        .commands = &commands,
        .schedule = {},
        .texture_bindings = std::span<FrameGraphTextureBinding>{texture_bindings},
        .pass_callbacks = {},
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = std::span<const FrameGraphTextureFinalState>{final_states},
    });
    if (!result.succeeded()) {
        throw std::runtime_error("rhi viewport surface frame graph color state execution failed");
    }
    return texture_bindings.front().current_state;
}

} // namespace

RhiViewportSurface::RhiViewportSurface(const RhiViewportSurfaceDesc& desc)
    : device_(desc.device), extent_(desc.extent), color_format_(desc.color_format),
      wait_for_completion_(desc.wait_for_completion), allow_native_display_interop_(desc.allow_native_display_interop) {
    if (device_ == nullptr) {
        throw std::invalid_argument("rhi viewport surface requires an rhi device");
    }
    validate_extent(extent_);
    if (color_format_ == rhi::Format::unknown || color_format_ == rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("rhi viewport surface requires a color format");
    }

    color_texture_ = create_color_texture();
}

std::string_view RhiViewportSurface::backend_name() const noexcept {
    return device_->backend_name();
}

Extent2D RhiViewportSurface::extent() const noexcept {
    return extent_;
}

rhi::Format RhiViewportSurface::color_format() const noexcept {
    return color_format_;
}

rhi::TextureHandle RhiViewportSurface::color_texture() const noexcept {
    return color_texture_;
}

std::uint64_t RhiViewportSurface::frames_rendered() const noexcept {
    return frames_rendered_;
}

void RhiViewportSurface::resize(Extent2D extent) {
    validate_extent(extent);
    if (extent.width == extent_.width && extent.height == extent_.height) {
        return;
    }

    extent_ = extent;
    color_texture_ = create_color_texture();
    color_state_ = rhi::ResourceState::copy_source;
}

void RhiViewportSurface::render_clear_frame() {
    auto commands = device_->begin_command_list(rhi::QueueKind::graphics);
    std::array<FrameGraphTextureBinding, 1> texture_bindings{FrameGraphTextureBinding{
        .resource = "viewport_color",
        .texture = color_texture_,
        .current_state = color_state_,
    }};
    const std::array<FrameGraphExecutionStep, 1> schedule{
        FrameGraphExecutionStep::make_pass_invoke("viewport.clear"),
    };
    const std::array<FrameGraphPassExecutionBinding, 1> pass_callbacks{FrameGraphPassExecutionBinding{
        .pass_name = "viewport.clear",
        .callback = [](std::string_view) { return FrameGraphExecutionCallbackResult{}; },
    }};
    const std::array<FrameGraphTexturePassTargetAccess, 1> pass_target_accesses{FrameGraphTexturePassTargetAccess{
        .pass_name = "viewport.clear",
        .resource = "viewport_color",
        .access = FrameGraphAccess::color_attachment_write,
    }};
    const std::array<FrameGraphTexturePassTargetState, 1> pass_target_states{FrameGraphTexturePassTargetState{
        .pass_name = "viewport.clear",
        .resource = "viewport_color",
        .state = rhi::ResourceState::render_target,
    }};
    const std::array<FrameGraphRhiRenderPassDesc, 1> render_passes{FrameGraphRhiRenderPassDesc{
        .pass_name = "viewport.clear",
        .color =
            FrameGraphRhiRenderPassColorAttachment{
                .resource = "viewport_color",
                .texture = {},
                .swapchain_frame = {},
                .load_action = rhi::LoadAction::clear,
                .store_action = rhi::StoreAction::store,
            },
    }};
    const std::array<FrameGraphTextureFinalState, 1> final_states{FrameGraphTextureFinalState{
        .resource = "viewport_color",
        .state = rhi::ResourceState::copy_source,
    }};
    const auto result = execute_frame_graph_rhi_texture_schedule(FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = std::span<const FrameGraphExecutionStep>{schedule},
        .texture_bindings = std::span<FrameGraphTextureBinding>{texture_bindings},
        .pass_callbacks = std::span<const FrameGraphPassExecutionBinding>{pass_callbacks},
        .pass_target_accesses = std::span<const FrameGraphTexturePassTargetAccess>{pass_target_accesses},
        .pass_target_states = std::span<const FrameGraphTexturePassTargetState>{pass_target_states},
        .render_passes = std::span<const FrameGraphRhiRenderPassDesc>{render_passes},
        .final_states = std::span<const FrameGraphTextureFinalState>{final_states},
    });
    color_state_ = texture_bindings.front().current_state;
    if (!result.succeeded()) {
        throw std::runtime_error("rhi viewport surface frame graph color state execution failed");
    }
    commands->close();

    const auto fence = device_->submit(*commands);
    if (wait_for_completion_) {
        device_->wait(fence);
    }
    ++frames_rendered_;
}

void RhiViewportSurface::render_frame(const RhiViewportRenderDesc& desc,
                                      const std::function<void(IRenderer&)>& submit) {
    if (desc.graphics_pipeline.value == 0) {
        throw std::invalid_argument("rhi viewport frame requires a graphics pipeline");
    }
    if (!submit) {
        throw std::invalid_argument("rhi viewport frame requires a submit callback");
    }

    transition_color_state(rhi::ResourceState::render_target);

    RhiFrameRenderer renderer(RhiFrameRendererDesc{
        .device = device_,
        .extent = extent_,
        .color_texture = color_texture_,
        .graphics_pipeline = desc.graphics_pipeline,
        .wait_for_completion = wait_for_completion_,
    });
    renderer.set_clear_color(desc.clear_color);
    renderer.begin_frame();
    submit(renderer);
    renderer.end_frame();

    color_state_ = rhi::ResourceState::render_target;
    transition_color_state(rhi::ResourceState::copy_source);
    ++frames_rendered_;
}

RhiViewportDisplayFrame RhiViewportSurface::prepare_display_frame() {
    transition_color_state(rhi::ResourceState::shader_read);
    return RhiViewportDisplayFrame{
        .extent = extent_,
        .format = color_format_,
        .texture = color_texture_,
        .frame_index = frames_rendered_,
    };
}

RhiViewportReadbackFrame RhiViewportSurface::readback_color_frame() {
    transition_color_state(rhi::ResourceState::copy_source);

    const auto row_pitch = viewport_readback_row_pitch(color_format_, extent_);
    const auto required_bytes = checked_readback_size(row_pitch, extent_.height);
    const auto texel_bytes = rhi::bytes_per_texel(color_format_);
    const auto readback = ensure_readback_buffer(required_bytes);

    auto commands = device_->begin_command_list(rhi::QueueKind::copy);
    commands->copy_texture_to_buffer(
        color_texture_, readback,
        rhi::BufferTextureCopyRegion{
            .buffer_offset = 0,
            .buffer_row_length = row_pitch / texel_bytes,
            .buffer_image_height = extent_.height,
            .texture_offset = rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        });
    commands->close();

    const auto fence = device_->submit(*commands);
    if (wait_for_completion_) {
        device_->wait(fence);
    }

    return RhiViewportReadbackFrame{
        .extent = extent_,
        .format = color_format_,
        .row_pitch = row_pitch,
        .frame_index = frames_rendered_,
        .pixels = device_->read_buffer(readback, 0, required_bytes),
    };
}

rhi::TextureHandle RhiViewportSurface::create_color_texture() const {
    auto usage = rhi::TextureUsage::render_target | rhi::TextureUsage::shader_resource | rhi::TextureUsage::copy_source;
    if (allow_native_display_interop_) {
        usage = usage | rhi::TextureUsage::shared;
    }

    return device_->create_texture(rhi::TextureDesc{
        .extent = rhi::Extent3D{.width = extent_.width, .height = extent_.height, .depth = 1},
        .format = color_format_,
        .usage = usage,
    });
}

rhi::BufferHandle RhiViewportSurface::ensure_readback_buffer(std::uint64_t size_bytes) {
    if (readback_buffer_.value != 0 && readback_buffer_size_ == size_bytes) {
        return readback_buffer_;
    }

    readback_buffer_ = device_->create_buffer(rhi::BufferDesc{
        .size_bytes = size_bytes,
        .usage = rhi::BufferUsage::copy_destination,
    });
    readback_buffer_size_ = size_bytes;
    return readback_buffer_;
}

void RhiViewportSurface::transition_color_state(rhi::ResourceState state) {
    if (color_state_ == state) {
        return;
    }

    auto commands = device_->begin_command_list(rhi::QueueKind::graphics);
    const auto recorded_color_state =
        execute_viewport_color_state_transition(*commands, color_texture_, color_state_, state);
    color_state_ = recorded_color_state;
    commands->close();

    const auto fence = device_->submit(*commands);
    if (wait_for_completion_) {
        device_->wait(fence);
    }
}

} // namespace mirakana
