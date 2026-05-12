// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

struct RhiNativeUiOverlayDesc {
    rhi::IRhiDevice* device{nullptr};
    Extent2D extent;
    rhi::Format color_format{rhi::Format::unknown};
    rhi::ShaderHandle vertex_shader;
    rhi::ShaderHandle fragment_shader;
    NativeUiOverlayAtlasBinding atlas;
    bool enable_textures{false};
};

struct RhiNativeUiOverlayPreparedDraw {
    std::uint32_t vertex_count{0};
    std::uint64_t sprite_count{0};
    std::uint64_t textured_sprite_count{0};
    std::uint64_t texture_bind_count{0};
    std::uint64_t batch_count{0};
    std::uint64_t textured_batch_count{0};
    std::vector<SpriteBatchRange> batches;
};

class RhiNativeUiOverlay final {
  public:
    explicit RhiNativeUiOverlay(const RhiNativeUiOverlayDesc& desc);

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] bool atlas_ready() const noexcept;

    void resize(Extent2D extent) noexcept;
    [[nodiscard]] RhiNativeUiOverlayPreparedDraw prepare(std::span<const SpriteCommand> sprites,
                                                         rhi::IRhiCommandList& commands);
    void record_draw(const RhiNativeUiOverlayPreparedDraw& prepared, rhi::IRhiCommandList& commands);

  private:
    void ensure_vertex_capacity(std::uint64_t required_bytes);

    rhi::IRhiDevice* device_{nullptr};
    Extent2D extent_;
    rhi::Format color_format_{rhi::Format::unknown};
    AssetId atlas_page_;
    bool texture_sampling_enabled_{false};
    rhi::DescriptorSetLayoutHandle atlas_descriptor_set_layout_;
    rhi::DescriptorSetHandle atlas_descriptor_set_;
    rhi::TextureHandle atlas_texture_;
    rhi::SamplerHandle atlas_sampler_;
    rhi::TextureHandle fallback_texture_;
    rhi::SamplerHandle fallback_sampler_;
    rhi::PipelineLayoutHandle pipeline_layout_;
    rhi::GraphicsPipelineHandle pipeline_;
    rhi::BufferHandle vertex_buffer_;
    std::uint64_t vertex_capacity_bytes_{0};
};

} // namespace mirakana
