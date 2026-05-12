// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace mirakana {

struct RhiViewportSurfaceDesc {
    rhi::IRhiDevice* device{nullptr};
    Extent2D extent;
    rhi::Format color_format{rhi::Format::rgba8_unorm};
    bool wait_for_completion{true};
    bool allow_native_display_interop{false};
};

struct RhiViewportReadbackFrame {
    Extent2D extent;
    rhi::Format format{rhi::Format::unknown};
    std::uint32_t row_pitch{0};
    std::uint64_t frame_index{0};
    std::vector<std::uint8_t> pixels;
};

struct RhiViewportDisplayFrame {
    Extent2D extent;
    rhi::Format format{rhi::Format::unknown};
    rhi::TextureHandle texture;
    std::uint64_t frame_index{0};
};

struct RhiViewportRenderDesc {
    rhi::GraphicsPipelineHandle graphics_pipeline;
    Color clear_color{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
};

class RhiViewportSurface final {
  public:
    explicit RhiViewportSurface(const RhiViewportSurfaceDesc& desc);

    [[nodiscard]] std::string_view backend_name() const noexcept;
    [[nodiscard]] Extent2D extent() const noexcept;
    [[nodiscard]] rhi::Format color_format() const noexcept;
    [[nodiscard]] rhi::TextureHandle color_texture() const noexcept;
    [[nodiscard]] std::uint64_t frames_rendered() const noexcept;

    void resize(Extent2D extent);
    void render_clear_frame();
    void render_frame(const RhiViewportRenderDesc& desc, const std::function<void(IRenderer&)>& submit);
    [[nodiscard]] RhiViewportDisplayFrame prepare_display_frame();
    [[nodiscard]] RhiViewportReadbackFrame readback_color_frame();

  private:
    [[nodiscard]] rhi::TextureHandle create_color_texture() const;
    [[nodiscard]] rhi::BufferHandle ensure_readback_buffer(std::uint64_t size_bytes);
    void transition_color_state(rhi::ResourceState state);

    rhi::IRhiDevice* device_{nullptr};
    Extent2D extent_;
    rhi::Format color_format_{rhi::Format::rgba8_unorm};
    rhi::TextureHandle color_texture_;
    rhi::BufferHandle readback_buffer_;
    std::uint64_t readback_buffer_size_{0};
    rhi::ResourceState color_state_{rhi::ResourceState::copy_source};
    bool wait_for_completion_{true};
    bool allow_native_display_interop_{false};
    std::uint64_t frames_rendered_{0};
};

} // namespace mirakana
