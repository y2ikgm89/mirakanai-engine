// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <string_view>

namespace mirakana {

struct Extent2D {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct Color {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

struct RendererStats {
    std::uint64_t frames_started{0};
    std::uint64_t frames_finished{0};
    std::uint64_t sprites_submitted{0};
    std::uint64_t meshes_submitted{0};
    /// Incremented when a `MeshCommand` with `gpu_skinning == true` completes a skinned indexed draw path.
    std::uint64_t gpu_skinning_draws{0};
    /// Incremented when a skinned draw binds `SkinnedMeshGpuBinding::joint_descriptor_set` (set index 1).
    std::uint64_t skinned_palette_descriptor_binds{0};
    /// Incremented when a `MeshCommand` with `gpu_morphing == true` completes a morphed indexed draw path.
    std::uint64_t gpu_morph_draws{0};
    /// Incremented when a morph draw binds `MorphMeshGpuBinding::morph_descriptor_set` (set index 1).
    std::uint64_t morph_descriptor_binds{0};
    std::uint64_t framegraph_passes_executed{0};
    /// Count of `FrameGraphExecutionStep::Kind::barrier` steps recorded for completed frames (Frame Graph v1
    /// execution).
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t postprocess_passes_executed{0};
    std::uint64_t native_ui_overlay_sprites_submitted{0};
    std::uint64_t native_ui_overlay_textured_sprites_submitted{0};
    std::uint64_t native_ui_overlay_texture_binds{0};
    std::uint64_t native_ui_overlay_draws{0};
    std::uint64_t native_ui_overlay_textured_draws{0};
    /// Native 2D sprite overlay batches actually recorded as RHI draw calls from order-preserving adjacent runs.
    std::uint64_t native_sprite_batches_executed{0};
    std::uint64_t native_sprite_batch_sprites_executed{0};
    std::uint64_t native_sprite_batch_textured_sprites_executed{0};
    std::uint64_t native_sprite_batch_texture_binds{0};
};

struct SpriteUvRect {
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
};

struct SpriteTextureRegion {
    bool enabled{false};
    AssetId atlas_page;
    SpriteUvRect uv_rect;
};

struct SpriteCommand {
    Transform2D transform;
    Color color;
    SpriteTextureRegion texture;
};

struct NativeUiOverlayAtlasBinding {
    AssetId atlas_page;
    rhi::TextureHandle texture;
    rhi::SamplerHandle sampler;
    const rhi::IRhiDevice* owner_device{nullptr};
};

struct MeshGpuBinding {
    rhi::BufferHandle vertex_buffer;
    rhi::BufferHandle index_buffer;
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    std::uint64_t vertex_offset{0};
    std::uint64_t index_offset{0};
    std::uint32_t vertex_stride{0};
    rhi::IndexFormat index_format{rhi::IndexFormat::unknown};
    const rhi::IRhiDevice* owner_device{nullptr};
    rhi::BufferHandle normal_vertex_buffer;
    rhi::BufferHandle tangent_vertex_buffer;
    std::uint64_t normal_vertex_offset{0};
    std::uint64_t tangent_vertex_offset{0};
    std::uint32_t normal_vertex_stride{0};
    std::uint32_t tangent_vertex_stride{0};
};

/// GPU resources for linear blend skinning: interleaved skinned vertices + indices in `mesh`, joint palette
/// matrices in `joint_palette_buffer` (256-byte aligned constant buffer allocation).
struct SkinnedMeshGpuBinding {
    MeshGpuBinding mesh;
    rhi::BufferHandle joint_palette_buffer;
    rhi::BufferHandle joint_palette_upload_buffer;
    rhi::DescriptorSetHandle joint_descriptor_set{};
    std::uint32_t joint_count{0};
    std::uint64_t joint_palette_uniform_allocation_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    rhi::BufferHandle skin_attribute_vertex_buffer;
    std::uint64_t skin_attribute_vertex_offset{0};
    std::uint32_t skin_attribute_vertex_stride{0};
};

/// GPU resources for additive morph target POSITION deltas plus optional NORMAL/TANGENT deltas. Delta buffers store
/// tightly packed float3 rows ordered by target then vertex; `morph_weight_buffer` is a 256-byte aligned uniform
/// allocation with one float32 weight per target.
struct MorphMeshGpuBinding {
    rhi::BufferHandle position_delta_buffer;
    rhi::BufferHandle normal_delta_buffer;
    rhi::BufferHandle tangent_delta_buffer;
    rhi::BufferHandle position_delta_upload_buffer;
    rhi::BufferHandle normal_delta_upload_buffer;
    rhi::BufferHandle tangent_delta_upload_buffer;
    rhi::BufferHandle morph_weight_buffer;
    rhi::BufferHandle morph_weight_upload_buffer;
    rhi::DescriptorSetHandle morph_descriptor_set{};
    std::uint32_t vertex_count{0};
    std::uint32_t target_count{0};
    std::uint64_t position_delta_bytes{0};
    std::uint64_t normal_delta_bytes{0};
    std::uint64_t tangent_delta_bytes{0};
    std::uint64_t morph_weight_uniform_allocation_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
};

struct MaterialGpuBinding {
    rhi::PipelineLayoutHandle pipeline_layout;
    rhi::DescriptorSetHandle descriptor_set;
    std::uint32_t descriptor_set_index{0};
    const rhi::IRhiDevice* owner_device{nullptr};
};

struct MeshCommand {
    Transform3D transform;
    Color color;
    AssetId mesh;
    AssetId material;
    Mat4 world_from_node{Mat4::identity()};
    MeshGpuBinding mesh_binding;
    MaterialGpuBinding material_binding;
    /// When true, `skinned_mesh` carries vertex/index buffers plus joint palette; static path uses `mesh_binding`
    /// only.
    bool gpu_skinning{false};
    SkinnedMeshGpuBinding skinned_mesh;
    /// When true, `mesh_binding` carries the base mesh and `morph_mesh` carries morph delta/weight descriptors.
    bool gpu_morphing{false};
    MorphMeshGpuBinding morph_mesh;
};

class IRenderer {
  public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
    [[nodiscard]] virtual Extent2D backbuffer_extent() const noexcept = 0;
    [[nodiscard]] virtual RendererStats stats() const noexcept = 0;

    virtual void resize(Extent2D extent) = 0;
    virtual void set_clear_color(Color color) noexcept = 0;
    virtual void begin_frame() = 0;
    virtual void draw_sprite(const SpriteCommand& command) = 0;
    virtual void draw_mesh(const MeshCommand& command) = 0;
    virtual void end_frame() = 0;
};

class NullRenderer final : public IRenderer {
  public:
    explicit NullRenderer(Extent2D extent);

    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] Extent2D backbuffer_extent() const noexcept override;
    [[nodiscard]] RendererStats stats() const noexcept override;
    [[nodiscard]] Color clear_color() const noexcept;
    [[nodiscard]] bool frame_active() const noexcept;

    void resize(Extent2D extent) override;
    void set_clear_color(Color color) noexcept override;
    void begin_frame() override;
    void draw_sprite(const SpriteCommand& command) override;
    void draw_mesh(const MeshCommand& command) override;
    void end_frame() override;

  private:
    Extent2D extent_;
    Color clear_color_{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
    RendererStats stats_{};
    bool frame_active_{false};
};

} // namespace mirakana
