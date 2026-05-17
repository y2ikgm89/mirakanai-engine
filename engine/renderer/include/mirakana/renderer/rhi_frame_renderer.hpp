// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace mirakana {

class RhiNativeUiOverlay;

struct RhiFrameRendererDesc {
    rhi::IRhiDevice* device{nullptr};
    Extent2D extent;
    rhi::TextureHandle color_texture;
    rhi::SwapchainHandle swapchain;
    rhi::GraphicsPipelineHandle graphics_pipeline;
    bool wait_for_completion{true};
    rhi::TextureHandle depth_texture;
    rhi::Format native_sprite_overlay_color_format{rhi::Format::unknown};
    rhi::ShaderHandle native_sprite_overlay_vertex_shader;
    rhi::ShaderHandle native_sprite_overlay_fragment_shader;
    NativeUiOverlayAtlasBinding native_sprite_overlay_atlas;
    bool enable_native_sprite_overlay{false};
    bool enable_native_sprite_overlay_textures{false};
    /// Optional skinned mesh pipeline (root signature includes joint palette descriptor set after material set 0).
    rhi::GraphicsPipelineHandle skinned_graphics_pipeline;
    /// Optional morph mesh pipeline (root signature includes morph descriptor set after material set 0).
    rhi::GraphicsPipelineHandle morph_graphics_pipeline;
    rhi::ResourceState color_texture_state{rhi::ResourceState::render_target};
    rhi::ResourceState depth_texture_state{rhi::ResourceState::depth_write};
};

class RhiFrameRenderer final : public IRenderer {
  public:
    explicit RhiFrameRenderer(const RhiFrameRendererDesc& desc);
    ~RhiFrameRenderer() override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] Extent2D backbuffer_extent() const noexcept override;
    [[nodiscard]] RendererStats stats() const noexcept override;
    [[nodiscard]] Color clear_color() const noexcept;
    [[nodiscard]] bool frame_active() const noexcept;

    void resize(Extent2D extent) override;
    void set_clear_color(Color color) noexcept override;
    void replace_graphics_pipeline(rhi::GraphicsPipelineHandle pipeline);
    void replace_depth_texture(rhi::TextureHandle texture, rhi::ResourceState state);
    void begin_frame() override;
    void draw_sprite(const SpriteCommand& command) override;
    void draw_mesh(const MeshCommand& command) override;
    void end_frame() override;

  private:
    enum class QueuedPrimaryDrawKind : std::uint8_t { sprite, mesh };

    struct QueuedPrimaryDraw {
        QueuedPrimaryDrawKind kind{QueuedPrimaryDrawKind::sprite};
        MeshCommand mesh;
    };

    void require_active_frame() const;
    void release_acquired_swapchain_frame() noexcept;
    void record_queued_mesh_command(const MeshCommand& command, RendererStats& recorded_stats);

    rhi::IRhiDevice* device_{nullptr};
    Extent2D extent_;
    rhi::TextureHandle color_texture_;
    rhi::SwapchainHandle swapchain_;
    rhi::SwapchainFrameHandle swapchain_frame_;
    bool swapchain_frame_presented_{false};
    rhi::GraphicsPipelineHandle graphics_pipeline_;
    rhi::GraphicsPipelineHandle skinned_graphics_pipeline_;
    rhi::GraphicsPipelineHandle morph_graphics_pipeline_;
    rhi::TextureHandle depth_texture_;
    rhi::ResourceState color_texture_state_{rhi::ResourceState::render_target};
    rhi::ResourceState depth_texture_state_{rhi::ResourceState::depth_write};
    bool wait_for_completion_{true};
    Color clear_color_{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
    RendererStats stats_{};
    std::unique_ptr<rhi::IRhiCommandList> commands_;
    std::unique_ptr<RhiNativeUiOverlay> native_sprite_overlay_;
    std::vector<QueuedPrimaryDraw> queued_primary_draws_;
    std::vector<SpriteCommand> pending_native_sprite_overlay_sprites_;
    bool frame_active_{false};
    bool skinned_pipeline_bound_{false};
    bool morph_pipeline_bound_{false};
};

} // namespace mirakana
