// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/rhi_viewport_surface.hpp"

#include "mirakana/renderer/rhi_frame_renderer.hpp"

#include <limits>
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
    if (color_state_ != rhi::ResourceState::render_target) {
        commands->transition_texture(color_texture_, color_state_, rhi::ResourceState::render_target);
        color_state_ = rhi::ResourceState::render_target;
    }
    commands->begin_render_pass(rhi::RenderPassDesc{
        .color =
            rhi::RenderPassColorAttachment{
                .texture = color_texture_,
                .load_action = rhi::LoadAction::clear,
                .store_action = rhi::StoreAction::store,
                .swapchain_frame = rhi::SwapchainFrameHandle{},
            },
    });
    commands->end_render_pass();
    commands->transition_texture(color_texture_, rhi::ResourceState::render_target, rhi::ResourceState::copy_source);
    color_state_ = rhi::ResourceState::copy_source;
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
    commands->transition_texture(color_texture_, color_state_, state);
    commands->close();

    const auto fence = device_->submit(*commands);
    if (wait_for_completion_) {
        device_->wait(fence);
    }
    color_state_ = state;
}

} // namespace mirakana
