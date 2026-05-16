// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace mirakana {

class RhiNativeUiOverlay;

[[nodiscard]] constexpr std::uint32_t postprocess_scene_color_texture_binding() noexcept {
    return 0;
}

[[nodiscard]] constexpr std::uint32_t postprocess_scene_color_sampler_binding() noexcept {
    return 1;
}

[[nodiscard]] constexpr std::uint32_t postprocess_scene_depth_texture_binding() noexcept {
    return 2;
}

[[nodiscard]] constexpr std::uint32_t postprocess_scene_depth_sampler_binding() noexcept {
    return 3;
}

struct RhiPostprocessFrameRendererDesc {
    rhi::IRhiDevice* device{nullptr};
    Extent2D extent;
    rhi::SwapchainHandle swapchain;
    rhi::Format color_format{rhi::Format::unknown};
    rhi::GraphicsPipelineHandle scene_graphics_pipeline;
    rhi::GraphicsPipelineHandle scene_skinned_graphics_pipeline;
    rhi::GraphicsPipelineHandle scene_morph_graphics_pipeline;
    rhi::ShaderHandle postprocess_vertex_shader;
    /// Fullscreen postprocess chain: stage 0 samples `scene_color` (and optional scene depth); when `size() > 1`,
    /// stage 0 writes an internal color target and the last stage writes the swapchain. Enforced to 1..2 stages in
    /// `RhiPostprocessFrameRenderer`.
    std::vector<rhi::ShaderHandle> postprocess_fragment_stages;
    bool wait_for_completion{true};
    bool enable_depth_input{false};
    rhi::Format depth_format{rhi::Format::depth24_stencil8};
    rhi::ShaderHandle native_ui_overlay_vertex_shader;
    rhi::ShaderHandle native_ui_overlay_fragment_shader;
    NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
    bool enable_native_ui_overlay{false};
    bool enable_native_ui_overlay_textures{false};
};

class RhiPostprocessFrameRenderer final : public IRenderer {
  public:
    explicit RhiPostprocessFrameRenderer(const RhiPostprocessFrameRendererDesc& desc);
    ~RhiPostprocessFrameRenderer() override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] Extent2D backbuffer_extent() const noexcept override;
    [[nodiscard]] RendererStats stats() const noexcept override;
    [[nodiscard]] Color clear_color() const noexcept;
    [[nodiscard]] bool frame_active() const noexcept;
    [[nodiscard]] bool postprocess_ready() const noexcept;
    [[nodiscard]] bool native_ui_overlay_ready() const noexcept;
    [[nodiscard]] bool native_ui_overlay_atlas_ready() const noexcept;
    [[nodiscard]] std::uint32_t frame_graph_pass_count() const noexcept {
        return frame_graph_pass_count_;
    }

    void resize(Extent2D extent) override;
    void set_clear_color(Color color) noexcept override;
    void begin_frame() override;
    void draw_sprite(const SpriteCommand& command) override;
    void draw_mesh(const MeshCommand& command) override;
    void end_frame() override;

  private:
    void require_active_frame() const;
    void release_acquired_swapchain_frame() noexcept;
    void recreate_scene_color_texture();
    void recreate_scene_depth_texture();
    void recreate_post_chain_work_texture();
    void update_postprocess_descriptors();
    void validate_scene_mesh_command(const MeshCommand& command) const;
    void record_scene_mesh_command(const MeshCommand& command);
    void record_scene_pass();

    rhi::IRhiDevice* device_{nullptr};
    Extent2D extent_;
    rhi::SwapchainHandle swapchain_;
    rhi::Format color_format_{rhi::Format::unknown};
    rhi::Format depth_format_{rhi::Format::depth24_stencil8};
    rhi::GraphicsPipelineHandle scene_graphics_pipeline_;
    rhi::GraphicsPipelineHandle scene_skinned_graphics_pipeline_;
    rhi::GraphicsPipelineHandle scene_morph_graphics_pipeline_;
    rhi::ShaderHandle postprocess_vertex_shader_;
    std::vector<rhi::ShaderHandle> postprocess_fragment_stages_;
    std::uint32_t postprocess_stage_count_{1};
    rhi::TextureHandle scene_color_texture_;
    rhi::ResourceState scene_color_state_{rhi::ResourceState::undefined};
    rhi::TextureHandle scene_depth_texture_;
    rhi::ResourceState scene_depth_state_{rhi::ResourceState::undefined};
    rhi::SamplerHandle scene_color_sampler_;
    rhi::SamplerHandle scene_depth_sampler_;
    /// First post stage: sampled scene color (+ optional depth).
    rhi::DescriptorSetLayoutHandle postprocess_first_descriptor_set_layout_{};
    rhi::DescriptorSetHandle postprocess_first_descriptor_set_{};
    rhi::PipelineLayoutHandle postprocess_first_pipeline_layout_{};
    rhi::GraphicsPipelineHandle postprocess_first_pipeline_{};
    /// Optional second stage: samples intermediate `post_chain_work_texture_` (same format as swapchain).
    rhi::DescriptorSetLayoutHandle postprocess_chain_descriptor_set_layout_{};
    rhi::DescriptorSetHandle postprocess_chain_descriptor_set_{};
    rhi::PipelineLayoutHandle postprocess_chain_pipeline_layout_{};
    rhi::GraphicsPipelineHandle postprocess_chain_pipeline_{};
    rhi::TextureHandle post_chain_work_texture_{};
    rhi::ResourceState post_chain_work_state_{rhi::ResourceState::undefined};
    rhi::SwapchainFrameHandle swapchain_frame_;
    bool swapchain_frame_presented_{false};
    bool wait_for_completion_{true};
    bool depth_input_enabled_{false};
    bool native_ui_overlay_enabled_{false};
    bool native_ui_overlay_textures_enabled_{false};
    Color clear_color_{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
    RendererStats stats_{};
    std::unique_ptr<rhi::IRhiCommandList> commands_;
    std::unique_ptr<RhiNativeUiOverlay> native_ui_overlay_;
    std::vector<MeshCommand> pending_meshes_;
    std::vector<SpriteCommand> pending_overlay_sprites_;
    bool frame_active_{false};
    bool skinned_scene_pipeline_bound_{false};
    bool morph_scene_pipeline_bound_{false};
    FrameGraphV1BuildResult postprocess_frame_graph_plan_{};
    std::vector<FrameGraphExecutionStep> postprocess_frame_graph_execution_;
    std::vector<FrameGraphTexturePassTargetAccess> postprocess_frame_graph_target_accesses_;
    std::uint32_t frame_graph_pass_count_{0};
};

} // namespace mirakana
