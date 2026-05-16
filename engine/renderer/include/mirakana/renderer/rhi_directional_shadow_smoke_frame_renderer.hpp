// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace mirakana {

class RhiNativeUiOverlay;

struct RhiDirectionalShadowSmokeFrameRendererDesc {
    rhi::IRhiDevice* device{nullptr};
    /// Swapchain / scene color resolution.
    Extent2D extent;
    rhi::SwapchainHandle swapchain;
    rhi::Format color_format{rhi::Format::unknown};
    rhi::GraphicsPipelineHandle scene_graphics_pipeline;
    rhi::GraphicsPipelineHandle scene_skinned_graphics_pipeline;
    /// Optional scene pipeline for graphics morph draws in the shadow receiver scene pass.
    rhi::GraphicsPipelineHandle scene_morph_graphics_pipeline;
    rhi::PipelineLayoutHandle scene_pipeline_layout;
    rhi::GraphicsPipelineHandle shadow_graphics_pipeline;
    rhi::DescriptorSetLayoutHandle shadow_receiver_descriptor_set_layout;
    rhi::ShaderHandle postprocess_vertex_shader;
    /// Single post-resolve stage for the swapchain path (exactly one non-null fragment shader).
    std::vector<rhi::ShaderHandle> postprocess_fragment_stages;
    bool wait_for_completion{true};
    rhi::Format scene_depth_format{rhi::Format::depth24_stencil8};
    rhi::Format shadow_depth_format{rhi::Format::depth24_stencil8};
    ShadowReceiverFilterMode shadow_filter_mode{ShadowReceiverFilterMode::fixed_pcf_3x3};
    float shadow_filter_radius_texels{1.0F};
    std::uint32_t shadow_filter_tap_count{9};
    rhi::ShaderHandle native_ui_overlay_vertex_shader;
    rhi::ShaderHandle native_ui_overlay_fragment_shader;
    NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
    bool enable_native_ui_overlay{false};
    bool enable_native_ui_overlay_textures{false};
    /// Horizontal directional shadow atlas (`width = per-cascade tile width * directional_shadow_cascade_count`).
    /// When zero in either dimension, the renderer derives a square tile from `extent` (clamped) and uses
    /// `directional_shadow_cascade_count` for atlas width.
    Extent2D shadow_depth_atlas_extent{};
    std::uint32_t directional_shadow_cascade_count{1};
    /// Initial packed bytes for `register(b2)` / binding 2; populated by `pack_shadow_receiver_constants`.
    std::array<std::uint8_t, shadow_receiver_constants_byte_size()> shadow_receiver_constants_initial{};
    /// Descriptor set index for `shadow_receiver_descriptor_set_layout` when binding shadow receiver resources on the
    /// scene pass (`RegisterSpace == index` in the D3D12 backend). Defaults to `1` for `[material, shadow]` layouts;
    /// use `2` when the scene root signature inserts a joint palette set at index `1` (`[material, joint, shadow]`).
    std::uint32_t shadow_receiver_descriptor_set_index{1};
};

class RhiDirectionalShadowSmokeFrameRenderer final : public IRenderer {
  public:
    explicit RhiDirectionalShadowSmokeFrameRenderer(const RhiDirectionalShadowSmokeFrameRendererDesc& desc);
    ~RhiDirectionalShadowSmokeFrameRenderer() override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] Extent2D backbuffer_extent() const noexcept override;
    [[nodiscard]] RendererStats stats() const noexcept override;
    [[nodiscard]] Color clear_color() const noexcept;
    [[nodiscard]] bool frame_active() const noexcept;
    [[nodiscard]] bool directional_shadow_ready() const noexcept;
    [[nodiscard]] bool native_ui_overlay_ready() const noexcept;
    [[nodiscard]] bool native_ui_overlay_atlas_ready() const noexcept;
    [[nodiscard]] ShadowReceiverFilterMode shadow_filter_mode() const noexcept;
    [[nodiscard]] float shadow_filter_radius_texels() const noexcept;
    [[nodiscard]] std::uint32_t shadow_filter_tap_count() const noexcept;
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
    void recreate_scene_textures();
    void recreate_shadow_textures();
    void recompute_shadow_atlas_extent();
    [[nodiscard]] std::uint32_t shadow_cascade_tile_width() const noexcept;
    void update_descriptors();
    void record_shadow_mesh_draw(const MeshCommand& command);
    void record_scene_mesh_draw(const MeshCommand& command);

    rhi::IRhiDevice* device_{nullptr};
    Extent2D extent_;
    rhi::SwapchainHandle swapchain_;
    rhi::Format color_format_{rhi::Format::unknown};
    rhi::Format scene_depth_format_{rhi::Format::depth24_stencil8};
    rhi::Format shadow_depth_format_{rhi::Format::depth24_stencil8};
    rhi::GraphicsPipelineHandle scene_graphics_pipeline_;
    rhi::GraphicsPipelineHandle scene_skinned_graphics_pipeline_;
    rhi::GraphicsPipelineHandle scene_morph_graphics_pipeline_;
    rhi::PipelineLayoutHandle scene_pipeline_layout_;
    rhi::GraphicsPipelineHandle shadow_graphics_pipeline_;
    rhi::DescriptorSetLayoutHandle shadow_receiver_descriptor_set_layout_;
    rhi::ShaderHandle postprocess_vertex_shader_;
    rhi::ShaderHandle postprocess_fragment_shader_;
    rhi::TextureHandle scene_color_texture_;
    rhi::ResourceState scene_color_state_{rhi::ResourceState::undefined};
    rhi::TextureHandle scene_depth_texture_;
    rhi::ResourceState scene_depth_state_{rhi::ResourceState::undefined};
    rhi::TextureHandle shadow_color_texture_;
    rhi::ResourceState shadow_color_state_{rhi::ResourceState::undefined};
    rhi::TextureHandle shadow_depth_texture_;
    rhi::ResourceState shadow_depth_state_{rhi::ResourceState::undefined};
    rhi::SamplerHandle scene_color_sampler_;
    rhi::SamplerHandle scene_depth_sampler_;
    rhi::SamplerHandle shadow_sampler_;
    rhi::BufferHandle shadow_receiver_constants_buffer_;
    rhi::DescriptorSetHandle shadow_receiver_descriptor_set_;
    std::uint32_t shadow_receiver_descriptor_set_index_{1};
    rhi::DescriptorSetLayoutHandle postprocess_descriptor_set_layout_;
    rhi::DescriptorSetHandle postprocess_descriptor_set_;
    rhi::PipelineLayoutHandle postprocess_pipeline_layout_;
    rhi::GraphicsPipelineHandle postprocess_pipeline_;
    rhi::SwapchainFrameHandle swapchain_frame_;
    bool swapchain_frame_presented_{false};
    bool wait_for_completion_{true};
    ShadowReceiverFilterMode shadow_filter_mode_{ShadowReceiverFilterMode::fixed_pcf_3x3};
    float shadow_filter_radius_texels_{1.0F};
    std::uint32_t shadow_filter_tap_count_{9};
    std::uint32_t directional_shadow_cascade_count_{1};
    Extent2D shadow_atlas_extent_{};
    bool shadow_atlas_explicit_{false};
    bool native_ui_overlay_enabled_{false};
    bool native_ui_overlay_textures_enabled_{false};
    Color clear_color_{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
    RendererStats stats_{};
    std::unique_ptr<rhi::IRhiCommandList> commands_;
    std::unique_ptr<RhiNativeUiOverlay> native_ui_overlay_;
    std::vector<MeshCommand> pending_meshes_;
    std::vector<SpriteCommand> pending_overlay_sprites_;
    bool frame_active_{false};
    bool internal_textures_need_recreate_{false};
    FrameGraphV1BuildResult shadow_smoke_frame_graph_plan_{};
    std::vector<FrameGraphExecutionStep> shadow_smoke_frame_graph_execution_;
    std::vector<FrameGraphTexturePassTargetAccess> shadow_smoke_frame_graph_target_accesses_;
    std::uint32_t frame_graph_pass_count_{0};
};

} // namespace mirakana
