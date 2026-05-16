// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/memory_diagnostics.hpp"
#include "mirakana/rhi/resource_lifetime.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace mirakana::rhi {

enum class BackendKind : std::uint8_t { null = 0, d3d12, vulkan, metal };

enum class QueueKind : std::uint8_t { graphics = 0, compute, copy };

enum class ShaderStage : std::uint8_t { vertex = 0, fragment, compute };

enum class ShaderStageVisibility : std::uint8_t {
    none = 0,
    vertex = 1U << 0U,
    fragment = 1U << 1U,
    compute = 1U << 2U
};

enum class PrimitiveTopology : std::uint8_t { triangle_list = 0, line_list };

enum class IndexFormat : std::uint8_t { unknown = 0, uint16, uint32 };

enum class VertexFormat : std::uint8_t { unknown = 0, float32x2, float32x3, float32x4, uint16x4 };

enum class VertexInputRate : std::uint8_t { vertex = 0, instance };

enum class VertexSemantic : std::uint8_t {
    position = 0,
    normal,
    tangent,
    texcoord,
    color,
    custom,
    joint_indices,
    joint_weights
};

enum class LoadAction : std::uint8_t { load = 0, clear, dont_care };

enum class StoreAction : std::uint8_t { store = 0, dont_care };

enum class CompareOp : std::uint8_t { never = 0, less, equal, less_equal, greater, not_equal, greater_equal, always };

enum class Format : std::uint8_t { unknown = 0, rgba8_unorm, bgra8_unorm, depth24_stencil8 };

enum class ResourceState : std::uint8_t {
    undefined = 0,
    copy_source,
    copy_destination,
    shader_read,
    render_target,
    depth_write,
    present
};

enum class DescriptorType : std::uint8_t {
    uniform_buffer = 0,
    storage_buffer,
    sampled_texture,
    storage_texture,
    sampler
};

enum class SamplerFilter : std::uint8_t { nearest = 0, linear };

enum class SamplerAddressMode : std::uint8_t { repeat = 0, clamp_to_edge };

enum class BufferUsage : std::uint8_t {
    none = 0,
    vertex = 1U << 0U,
    index = 1U << 1U,
    uniform = 1U << 2U,
    storage = 1U << 3U,
    copy_source = 1U << 4U,
    copy_destination = 1U << 5U
};

enum class TextureUsage : std::uint8_t {
    none = 0,
    shader_resource = 1U << 0U,
    render_target = 1U << 1U,
    depth_stencil = 1U << 2U,
    copy_source = 1U << 3U,
    copy_destination = 1U << 4U,
    present = 1U << 5U,
    storage = 1U << 6U,
    shared = 1U << 7U
};

[[nodiscard]] constexpr ShaderStageVisibility operator|(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept {
    return static_cast<ShaderStageVisibility>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) noexcept {
    return static_cast<BufferUsage>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr bool has_flag(BufferUsage value, BufferUsage flag) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0U;
}

[[nodiscard]] constexpr bool has_flag(TextureUsage value, TextureUsage flag) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0U;
}

[[nodiscard]] constexpr bool has_stage_visibility(ShaderStageVisibility value) noexcept {
    return value != ShaderStageVisibility::none;
}

struct Extent3D {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t depth{1};
};

struct Offset3D {
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t z{0};
};

struct Extent2D {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct BufferHandle {
    std::uint32_t value{0};
};

struct TextureHandle {
    std::uint32_t value{0};
};

struct SamplerHandle {
    std::uint32_t value{0};
};

struct SwapchainHandle {
    std::uint32_t value{0};
};

struct SwapchainFrameHandle {
    std::uint32_t value{0};
};

struct SurfaceHandle {
    std::uintptr_t value{0};
};

struct ShaderHandle {
    std::uint32_t value{0};
};

struct PipelineLayoutHandle {
    std::uint32_t value{0};
};

struct DescriptorSetLayoutHandle {
    std::uint32_t value{0};
};

struct DescriptorSetHandle {
    std::uint32_t value{0};
};

struct TransientResourceHandle {
    std::uint32_t value{0};
};

struct GraphicsPipelineHandle {
    std::uint32_t value{0};
};

struct ComputePipelineHandle {
    std::uint32_t value{0};
};

struct FenceValue {
    std::uint64_t value{0};
    QueueKind queue{QueueKind::graphics};
};

struct BufferDesc {
    std::uint64_t size_bytes{0};
    BufferUsage usage{BufferUsage::none};
};

struct VertexBufferBinding {
    BufferHandle buffer;
    std::uint64_t offset{0};
    std::uint32_t stride{0};
    std::uint32_t binding{0};
};

struct IndexBufferBinding {
    BufferHandle buffer;
    std::uint64_t offset{0};
    IndexFormat format{IndexFormat::unknown};
};

struct TextureDesc {
    Extent3D extent;
    Format format{Format::unknown};
    TextureUsage usage{TextureUsage::none};
};

struct SamplerDesc {
    SamplerFilter min_filter{SamplerFilter::linear};
    SamplerFilter mag_filter{SamplerFilter::linear};
    SamplerAddressMode address_u{SamplerAddressMode::repeat};
    SamplerAddressMode address_v{SamplerAddressMode::repeat};
    SamplerAddressMode address_w{SamplerAddressMode::repeat};
};

struct SwapchainDesc {
    Extent2D extent;
    Format format{Format::unknown};
    std::uint32_t buffer_count{0};
    bool vsync{true};
    SurfaceHandle surface;
};

struct BufferCopyRegion {
    std::uint64_t source_offset{0};
    std::uint64_t destination_offset{0};
    std::uint64_t size_bytes{0};
};

struct BufferTextureCopyRegion {
    std::uint64_t buffer_offset{0};
    std::uint32_t buffer_row_length{0};
    std::uint32_t buffer_image_height{0};
    Offset3D texture_offset;
    Extent3D texture_extent;
};

[[nodiscard]] std::uint32_t bytes_per_texel(Format format);
[[nodiscard]] std::uint64_t buffer_texture_copy_required_bytes(Format format, const BufferTextureCopyRegion& region);

struct ShaderDesc {
    ShaderStage stage{ShaderStage::vertex};
    std::string_view entry_point;
    std::uint64_t bytecode_size{0};
    const void* bytecode{nullptr};
};

struct DescriptorBindingDesc {
    std::uint32_t binding{0};
    DescriptorType type{DescriptorType::uniform_buffer};
    std::uint32_t count{1};
    ShaderStageVisibility stages{ShaderStageVisibility::none};
};

struct DescriptorSetLayoutDesc {
    std::vector<DescriptorBindingDesc> bindings;
};

struct DescriptorResource {
    DescriptorType type{DescriptorType::uniform_buffer};
    BufferHandle buffer_handle;
    TextureHandle texture_handle;
    SamplerHandle sampler_handle;

    [[nodiscard]] static DescriptorResource buffer(DescriptorType type, BufferHandle handle) noexcept {
        return DescriptorResource{.type = type,
                                  .buffer_handle = handle,
                                  .texture_handle = TextureHandle{},
                                  .sampler_handle = SamplerHandle{}};
    }

    [[nodiscard]] static DescriptorResource texture(DescriptorType type, TextureHandle handle) noexcept {
        return DescriptorResource{
            .type = type, .buffer_handle = BufferHandle{}, .texture_handle = handle, .sampler_handle = SamplerHandle{}};
    }

    [[nodiscard]] static DescriptorResource sampler(SamplerHandle handle) noexcept {
        return DescriptorResource{.type = DescriptorType::sampler,
                                  .buffer_handle = BufferHandle{},
                                  .texture_handle = TextureHandle{},
                                  .sampler_handle = handle};
    }
};

struct DescriptorWrite {
    DescriptorSetHandle set;
    std::uint32_t binding{0};
    std::uint32_t array_element{0};
    std::vector<DescriptorResource> resources;
};

struct PipelineLayoutDesc {
    std::vector<DescriptorSetLayoutHandle> descriptor_sets;
    std::uint32_t push_constant_bytes{0};
};

struct VertexBufferLayoutDesc {
    std::uint32_t binding{0};
    std::uint32_t stride{0};
    VertexInputRate input_rate{VertexInputRate::vertex};
};

struct VertexAttributeDesc {
    std::uint32_t location{0};
    std::uint32_t binding{0};
    std::uint32_t offset{0};
    VertexFormat format{VertexFormat::unknown};
    VertexSemantic semantic{VertexSemantic::custom};
    std::uint32_t semantic_index{0};
};

struct DepthStencilStateDesc {
    bool depth_test_enabled{false};
    bool depth_write_enabled{false};
    CompareOp depth_compare{CompareOp::less_equal};
};

struct GraphicsPipelineDesc {
    PipelineLayoutHandle layout;
    ShaderHandle vertex_shader;
    ShaderHandle fragment_shader;
    Format color_format{Format::unknown};
    Format depth_format{Format::unknown};
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    std::vector<VertexBufferLayoutDesc> vertex_buffers;
    std::vector<VertexAttributeDesc> vertex_attributes;
    DepthStencilStateDesc depth_state;
};

struct ComputePipelineDesc {
    PipelineLayoutHandle layout;
    ShaderHandle compute_shader;
};

struct ClearColorValue {
    float red{0.0F};
    float green{0.0F};
    float blue{0.0F};
    float alpha{1.0F};
};

struct ClearDepthValue {
    float depth{1.0F};
};

struct RenderPassColorAttachment {
    TextureHandle texture;
    LoadAction load_action{LoadAction::clear};
    StoreAction store_action{StoreAction::store};
    SwapchainFrameHandle swapchain_frame;
    ClearColorValue clear_color;
};

struct RenderPassDepthAttachment {
    TextureHandle texture;
    LoadAction load_action{LoadAction::clear};
    StoreAction store_action{StoreAction::store};
    ClearDepthValue clear_depth;
};

struct RenderPassDesc {
    RenderPassColorAttachment color;
    RenderPassDepthAttachment depth;
};

/// Dynamic viewport in framebuffer coordinates (matches D3D12/Vulkan conventions).
struct ViewportDesc {
    float x{0.0F};
    float y{0.0F};
    float width{0.0F};
    float height{0.0F};
    float min_depth{0.0F};
    float max_depth{1.0F};
};

/// Scissor rectangle in pixel coordinates (upper-left origin).
struct ScissorRectDesc {
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct TransientBuffer {
    TransientResourceHandle lease;
    BufferHandle buffer;
};

struct TransientTexture {
    TransientResourceHandle lease;
    TextureHandle texture;
};

enum class TransientResourceKind : std::uint8_t { buffer = 0, texture };

struct RhiStats {
    std::uint64_t buffers_created{0};
    std::uint64_t textures_created{0};
    std::uint64_t samplers_created{0};
    std::uint64_t swapchains_created{0};
    std::uint64_t swapchain_resizes{0};
    std::uint64_t swapchain_frames_acquired{0};
    std::uint64_t swapchain_frames_released{0};
    std::uint64_t shader_modules_created{0};
    std::uint64_t descriptor_set_layouts_created{0};
    std::uint64_t descriptor_sets_allocated{0};
    std::uint64_t descriptor_writes{0};
    std::uint64_t pipeline_layouts_created{0};
    std::uint64_t graphics_pipelines_created{0};
    std::uint64_t compute_pipelines_created{0};
    std::uint64_t command_lists_begun{0};
    std::uint64_t command_lists_submitted{0};
    std::uint64_t resource_transitions{0};
    std::uint64_t texture_aliasing_barriers{0};
    std::uint64_t render_passes_begun{0};
    std::uint64_t graphics_pipelines_bound{0};
    std::uint64_t compute_pipelines_bound{0};
    std::uint64_t descriptor_sets_bound{0};
    std::uint64_t vertex_buffer_bindings{0};
    std::uint64_t index_buffer_bindings{0};
    std::uint64_t buffer_writes{0};
    std::uint64_t buffer_copies{0};
    std::uint64_t buffer_texture_copies{0};
    std::uint64_t texture_buffer_copies{0};
    std::uint64_t buffer_reads{0};
    std::uint64_t bytes_written{0};
    std::uint64_t bytes_copied{0};
    std::uint64_t bytes_read{0};
    std::uint64_t present_calls{0};
    std::uint64_t transient_resources_acquired{0};
    std::uint64_t transient_resources_released{0};
    std::uint64_t transient_resources_active{0};
    std::uint64_t draw_calls{0};
    std::uint64_t indexed_draw_calls{0};
    std::uint64_t compute_dispatches{0};
    std::uint64_t vertices_submitted{0};
    std::uint64_t indices_submitted{0};
    std::uint64_t compute_workgroups_x{0};
    std::uint64_t compute_workgroups_y{0};
    std::uint64_t compute_workgroups_z{0};
    std::uint64_t fences_signaled{0};
    std::uint64_t fence_waits{0};
    std::uint64_t fence_wait_failures{0};
    std::uint64_t queue_waits{0};
    std::uint64_t queue_wait_failures{0};
    std::uint64_t graphics_queue_submits{0};
    std::uint64_t compute_queue_submits{0};
    std::uint64_t copy_queue_submits{0};
    std::uint64_t last_submitted_fence_value{0};
    QueueKind last_submitted_fence_queue{QueueKind::graphics};
    std::uint64_t last_completed_fence_value{0};
    std::uint64_t last_graphics_submitted_fence_value{0};
    std::uint64_t last_compute_submitted_fence_value{0};
    std::uint64_t last_copy_submitted_fence_value{0};
    std::uint64_t last_queue_wait_fence_value{0};
    QueueKind last_queue_wait_fence_queue{QueueKind::graphics};
    std::uint64_t last_graphics_queue_wait_fence_value{0};
    QueueKind last_graphics_queue_wait_fence_queue{QueueKind::graphics};
    std::uint64_t last_compute_queue_wait_fence_value{0};
    QueueKind last_compute_queue_wait_fence_queue{QueueKind::graphics};
    std::uint64_t last_copy_queue_wait_fence_value{0};
    QueueKind last_copy_queue_wait_fence_queue{QueueKind::graphics};
    std::uint64_t queue_event_sequence{0};
    std::uint64_t last_graphics_submit_sequence{0};
    std::uint64_t last_compute_submit_sequence{0};
    std::uint64_t last_copy_submit_sequence{0};
    std::uint64_t last_queue_wait_sequence{0};
    std::uint64_t last_graphics_queue_wait_sequence{0};
    std::uint64_t last_compute_queue_wait_sequence{0};
    std::uint64_t last_copy_queue_wait_sequence{0};
    std::uint64_t shared_texture_exports{0};
    std::uint64_t shared_texture_export_failures{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
};

enum class RhiAsyncOverlapReadinessStatus : std::uint8_t {
    not_requested = 0,
    missing_queue_evidence,
    missing_pipelined_slot_evidence,
    unsupported_missing_timestamp_support,
    not_proven_serial_dependency,
    ready_for_backend_private_timing
};

struct RhiPipelinedComputeGraphicsScheduleEvidence {
    std::uint32_t output_slot_count{0};
    std::uint32_t current_compute_output_slot_index{0};
    std::uint32_t graphics_consumed_output_slot_index{0};
    FenceValue previous_compute_fence;
    FenceValue current_compute_fence;
};

struct RhiAsyncOverlapReadinessDiagnostics {
    RhiAsyncOverlapReadinessStatus status{RhiAsyncOverlapReadinessStatus::not_requested};
    bool compute_queue_submitted{false};
    bool graphics_queue_waited_for_compute{false};
    bool graphics_queue_submitted_after_wait{false};
    bool same_frame_graphics_wait_serializes_compute{false};
    bool gpu_timestamps_available{false};
    bool output_ring_available{false};
    bool compute_and_graphics_use_distinct_output_slots{false};
    bool graphics_queue_waited_for_previous_compute{false};
    bool graphics_queue_waited_for_current_compute{false};
    std::uint32_t output_slot_count{0};
    std::uint32_t current_compute_output_slot_index{0};
    std::uint32_t graphics_consumed_output_slot_index{0};
    std::uint64_t previous_compute_submitted_fence_value{0};
    std::uint64_t current_compute_fence_value{0};
    std::uint64_t last_compute_submitted_fence_value{0};
    std::uint64_t last_graphics_queue_wait_fence_value{0};
    QueueKind last_graphics_queue_wait_fence_queue{QueueKind::graphics};
    std::uint64_t last_graphics_submitted_fence_value{0};
    std::uint64_t last_graphics_queue_wait_sequence{0};
    std::uint64_t last_graphics_submit_sequence{0};
};

/// Classifies first-party compute/graphics queue sequencing evidence for backend-private overlap measurement.
/// This does not expose native queues, fences, command lists, query heaps, timestamp resources, or GPU timestamp
/// values.
[[nodiscard]] RhiAsyncOverlapReadinessDiagnostics
diagnose_compute_graphics_async_overlap_readiness(const RhiStats& stats,
                                                  std::uint64_t gpu_timestamp_ticks_per_second) noexcept;

/// Classifies an output-ring schedule where graphics consumes a previously completed compute output slot while compute
/// writes a different current slot. The result is readiness evidence only; GPU timing values stay backend-private.
[[nodiscard]] RhiAsyncOverlapReadinessDiagnostics
diagnose_pipelined_compute_graphics_async_overlap_readiness(const RhiStats& stats,
                                                            const RhiPipelinedComputeGraphicsScheduleEvidence& schedule,
                                                            std::uint64_t gpu_timestamp_ticks_per_second) noexcept;

class IRhiCommandList {
  public:
    virtual ~IRhiCommandList() = default;

    [[nodiscard]] virtual QueueKind queue_kind() const noexcept = 0;
    [[nodiscard]] virtual bool closed() const noexcept = 0;

    virtual void transition_texture(TextureHandle texture, ResourceState before, ResourceState after) = 0;
    virtual void texture_aliasing_barrier(TextureHandle before, TextureHandle after) = 0;
    virtual void copy_buffer(BufferHandle source, BufferHandle destination, const BufferCopyRegion& region) = 0;
    virtual void copy_buffer_to_texture(BufferHandle source, TextureHandle destination,
                                        const BufferTextureCopyRegion& region) = 0;
    virtual void copy_texture_to_buffer(TextureHandle source, BufferHandle destination,
                                        const BufferTextureCopyRegion& region) = 0;
    virtual void present(SwapchainFrameHandle frame) = 0;
    virtual void begin_render_pass(const RenderPassDesc& desc) = 0;
    virtual void end_render_pass() = 0;
    virtual void bind_graphics_pipeline(GraphicsPipelineHandle pipeline) = 0;
    virtual void bind_compute_pipeline(ComputePipelineHandle pipeline) = 0;
    virtual void bind_descriptor_set(PipelineLayoutHandle layout, std::uint32_t set_index, DescriptorSetHandle set) = 0;
    virtual void bind_vertex_buffer(const VertexBufferBinding& binding) = 0;
    virtual void bind_index_buffer(const IndexBufferBinding& binding) = 0;
    virtual void draw(std::uint32_t vertex_count, std::uint32_t instance_count) = 0;
    virtual void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count) = 0;
    virtual void dispatch(std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z) = 0;

    /// Sets the dynamic viewport for subsequent draws inside the active render pass.
    virtual void set_viewport(const ViewportDesc& viewport) = 0;
    /// Sets the dynamic scissor for subsequent draws inside the active render pass.
    virtual void set_scissor(const ScissorRectDesc& scissor) = 0;

    /// Records a nested GPU debug region for tools such as PIX (D3D12) when supported. Labels must satisfy
    /// `mirakana::rhi::is_valid_rhi_debug_label` (non-empty printable ASCII, max 256 bytes).
    virtual void begin_gpu_debug_scope(std::string_view name) = 0;
    virtual void end_gpu_debug_scope() = 0;
    virtual void insert_gpu_debug_marker(std::string_view name) = 0;

    virtual void close() = 0;
};

class IRhiDevice {
  public:
    virtual ~IRhiDevice() = default;

    [[nodiscard]] virtual BackendKind backend_kind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
    [[nodiscard]] virtual RhiStats stats() const noexcept = 0;

    /// Timestamp query resolution in ticks per second for the primary graphics queue, or `0` when unsupported
    /// (null backend, Vulkan until timestamp queries are wired, or D3D12 failure).
    [[nodiscard]] virtual std::uint64_t gpu_timestamp_ticks_per_second() const noexcept = 0;

    /// Returns a backend-neutral diagnostic snapshot (OS video memory where available, plus best-effort committed
    /// resource byte estimates). Callers must not treat this as an allocator contract or eviction signal.
    [[nodiscard]] virtual RhiDeviceMemoryDiagnostics memory_diagnostics() const = 0;

    /// Optional per-device lifetime ledger (`NullRhiDevice`, D3D12, Vulkan). Other backends return `nullptr`.
    [[nodiscard]] virtual const RhiResourceLifetimeRegistry* resource_lifetime_registry() const noexcept {
        return nullptr;
    }

    [[nodiscard]] virtual BufferHandle create_buffer(const BufferDesc& desc) = 0;
    [[nodiscard]] virtual TextureHandle create_texture(const TextureDesc& desc) = 0;
    [[nodiscard]] virtual SamplerHandle create_sampler(const SamplerDesc& desc) = 0;
    [[nodiscard]] virtual SwapchainHandle create_swapchain(const SwapchainDesc& desc) = 0;
    virtual void resize_swapchain(SwapchainHandle swapchain, Extent2D extent) = 0;
    [[nodiscard]] virtual SwapchainFrameHandle acquire_swapchain_frame(SwapchainHandle swapchain) = 0;
    virtual void release_swapchain_frame(SwapchainFrameHandle frame) = 0;
    [[nodiscard]] virtual TransientBuffer acquire_transient_buffer(const BufferDesc& desc) = 0;
    [[nodiscard]] virtual TransientTexture acquire_transient_texture(const TextureDesc& desc) = 0;
    virtual void release_transient(TransientResourceHandle lease) = 0;
    [[nodiscard]] virtual ShaderHandle create_shader(const ShaderDesc& desc) = 0;
    [[nodiscard]] virtual DescriptorSetLayoutHandle
    create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) = 0;
    [[nodiscard]] virtual DescriptorSetHandle allocate_descriptor_set(DescriptorSetLayoutHandle layout) = 0;
    virtual void update_descriptor_set(const DescriptorWrite& write) = 0;
    [[nodiscard]] virtual PipelineLayoutHandle create_pipeline_layout(const PipelineLayoutDesc& desc) = 0;
    [[nodiscard]] virtual GraphicsPipelineHandle create_graphics_pipeline(const GraphicsPipelineDesc& desc) = 0;
    [[nodiscard]] virtual ComputePipelineHandle create_compute_pipeline(const ComputePipelineDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<IRhiCommandList> begin_command_list(QueueKind queue) = 0;
    [[nodiscard]] virtual FenceValue submit(IRhiCommandList& commands) = 0;
    virtual void wait(FenceValue fence) = 0;
    virtual void wait_for_queue(QueueKind queue, FenceValue fence) = 0;
    virtual void write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                                std::uint64_t size_bytes) = 0;
};

/// Deterministic null backend for RHI contract tests. `null_mark_*_released` marks handles inactive so safe-point
/// teardown paths can mirror GPU release ordering without native drivers.
class NullRhiDevice final : public IRhiDevice {
  public:
    NullRhiDevice() = default;
    ~NullRhiDevice() override;

    [[nodiscard]] BackendKind backend_kind() const noexcept override;
    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] RhiStats stats() const noexcept override;
    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override;
    [[nodiscard]] RhiDeviceMemoryDiagnostics memory_diagnostics() const override;
    [[nodiscard]] FenceValue completed_fence() const noexcept;

    [[nodiscard]] BufferHandle create_buffer(const BufferDesc& desc) override;
    [[nodiscard]] TextureHandle create_texture(const TextureDesc& desc) override;
    [[nodiscard]] SamplerHandle create_sampler(const SamplerDesc& desc) override;
    [[nodiscard]] SwapchainHandle create_swapchain(const SwapchainDesc& desc) override;
    void resize_swapchain(SwapchainHandle swapchain, Extent2D extent) override;
    [[nodiscard]] SwapchainFrameHandle acquire_swapchain_frame(SwapchainHandle swapchain) override;
    void release_swapchain_frame(SwapchainFrameHandle frame) override;
    [[nodiscard]] TransientBuffer acquire_transient_buffer(const BufferDesc& desc) override;
    [[nodiscard]] TransientTexture acquire_transient_texture(const TextureDesc& desc) override;
    void release_transient(TransientResourceHandle lease) override;
    [[nodiscard]] ShaderHandle create_shader(const ShaderDesc& desc) override;
    [[nodiscard]] DescriptorSetLayoutHandle create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) override;
    [[nodiscard]] DescriptorSetHandle allocate_descriptor_set(DescriptorSetLayoutHandle layout) override;
    void update_descriptor_set(const DescriptorWrite& write) override;
    [[nodiscard]] PipelineLayoutHandle create_pipeline_layout(const PipelineLayoutDesc& desc) override;
    [[nodiscard]] GraphicsPipelineHandle create_graphics_pipeline(const GraphicsPipelineDesc& desc) override;
    [[nodiscard]] ComputePipelineHandle create_compute_pipeline(const ComputePipelineDesc& desc) override;
    [[nodiscard]] std::unique_ptr<IRhiCommandList> begin_command_list(QueueKind queue) override;
    [[nodiscard]] FenceValue submit(IRhiCommandList& commands) override;
    void wait(FenceValue fence) override;
    void wait_for_queue(QueueKind queue, FenceValue fence) override;
    void write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) override;
    [[nodiscard]] std::vector<std::uint8_t> read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) override;

    [[nodiscard]] bool null_mark_buffer_released(BufferHandle buffer) noexcept;
    [[nodiscard]] bool null_mark_texture_released(TextureHandle texture) noexcept;
    [[nodiscard]] bool null_mark_sampler_released(SamplerHandle sampler) noexcept;
    [[nodiscard]] bool null_mark_descriptor_set_released(DescriptorSetHandle set) noexcept;
    [[nodiscard]] bool null_mark_descriptor_set_layout_released(DescriptorSetLayoutHandle layout) noexcept;
    [[nodiscard]] bool null_mark_pipeline_layout_released(PipelineLayoutHandle layout) noexcept;
    [[nodiscard]] bool null_mark_shader_released(ShaderHandle shader) noexcept;
    [[nodiscard]] bool null_mark_graphics_pipeline_released(GraphicsPipelineHandle pipeline) noexcept;
    [[nodiscard]] bool null_mark_compute_pipeline_released(ComputePipelineHandle pipeline) noexcept;

    /// Read-only view of the per-device lifetime ledger used for deterministic tests and diagnostics.
    [[nodiscard]] const RhiResourceLifetimeRegistry* resource_lifetime_registry() const noexcept override {
        return &resource_lifetime_;
    }

  private:
    friend class NullRhiCommandList;

    struct TransientLeaseRecord {
        TransientResourceKind kind{TransientResourceKind::buffer};
        BufferHandle buffer;
        TextureHandle texture;
        bool active{false};
    };

    [[nodiscard]] bool owns_texture(TextureHandle texture) const noexcept;
    [[nodiscard]] bool owns_sampler(SamplerHandle sampler) const noexcept;
    [[nodiscard]] bool owns_buffer(BufferHandle buffer) const noexcept;
    [[nodiscard]] bool owns_swapchain(SwapchainHandle swapchain) const noexcept;
    [[nodiscard]] bool owns_shader(ShaderHandle shader) const noexcept;
    [[nodiscard]] bool owns_descriptor_set_layout(DescriptorSetLayoutHandle layout) const noexcept;
    [[nodiscard]] bool owns_descriptor_set(DescriptorSetHandle set) const noexcept;
    [[nodiscard]] bool owns_pipeline_layout(PipelineLayoutHandle layout) const noexcept;
    [[nodiscard]] bool owns_graphics_pipeline(GraphicsPipelineHandle pipeline) const noexcept;
    [[nodiscard]] bool owns_compute_pipeline(ComputePipelineHandle pipeline) const noexcept;
    [[nodiscard]] const BufferDesc& buffer_desc(BufferHandle buffer) const;
    [[nodiscard]] const TextureDesc& texture_desc(TextureHandle texture) const;
    [[nodiscard]] const SwapchainDesc& swapchain_desc(SwapchainHandle swapchain) const;
    [[nodiscard]] bool owns_swapchain_frame(SwapchainFrameHandle frame) const noexcept;
    [[nodiscard]] SwapchainHandle swapchain_for_frame(SwapchainFrameHandle frame) const;
    [[nodiscard]] bool swapchain_frame_active(SwapchainFrameHandle frame) const;
    [[nodiscard]] bool swapchain_frame_presented(SwapchainFrameHandle frame) const;
    void set_swapchain_frame_presented(SwapchainFrameHandle frame, bool presented);
    void complete_swapchain_frame(SwapchainFrameHandle frame);
    [[nodiscard]] ResourceState swapchain_state(SwapchainHandle swapchain) const;
    void set_swapchain_state(SwapchainHandle swapchain, ResourceState state);
    [[nodiscard]] bool swapchain_presentable(SwapchainHandle swapchain) const;
    void set_swapchain_presentable(SwapchainHandle swapchain, bool presentable);
    [[nodiscard]] bool swapchain_frame_reserved(SwapchainHandle swapchain) const;
    void reserve_swapchain_frame(SwapchainHandle swapchain);
    void release_swapchain_reservation(SwapchainHandle swapchain);
    [[nodiscard]] ShaderStage shader_stage(ShaderHandle shader) const;
    [[nodiscard]] const DescriptorSetLayoutDesc& descriptor_set_layout_desc(DescriptorSetLayoutHandle layout) const;
    [[nodiscard]] DescriptorSetLayoutHandle descriptor_set_layout_for_set(DescriptorSetHandle set) const;
    [[nodiscard]] const PipelineLayoutDesc& pipeline_layout_desc(PipelineLayoutHandle layout) const;
    [[nodiscard]] PipelineLayoutHandle pipeline_layout_for_pipeline(GraphicsPipelineHandle pipeline) const;
    [[nodiscard]] PipelineLayoutHandle pipeline_layout_for_compute_pipeline(ComputePipelineHandle pipeline) const;
    [[nodiscard]] Format graphics_pipeline_color_format(GraphicsPipelineHandle pipeline) const;
    [[nodiscard]] Format graphics_pipeline_depth_format(GraphicsPipelineHandle pipeline) const;
    [[nodiscard]] ResourceState texture_state(TextureHandle texture) const;
    void set_texture_state(TextureHandle texture, ResourceState state);

    void retire_resource_lifetime_to_completed_fence() noexcept;

    RhiResourceLifetimeRegistry resource_lifetime_{};
    std::vector<RhiResourceHandle> buffer_lifetime_;
    std::vector<RhiResourceHandle> texture_lifetime_;
    std::vector<RhiResourceHandle> sampler_lifetime_;
    std::vector<RhiResourceHandle> shader_lifetime_;
    std::vector<RhiResourceHandle> descriptor_set_layout_lifetime_;
    std::vector<RhiResourceHandle> descriptor_set_lifetime_;
    std::vector<RhiResourceHandle> pipeline_layout_lifetime_;
    std::vector<RhiResourceHandle> graphics_pipeline_lifetime_;
    std::vector<RhiResourceHandle> compute_pipeline_lifetime_;
    RhiStats stats_{};
    std::uint32_t next_buffer_{1};
    std::uint32_t next_texture_{1};
    std::uint32_t next_sampler_{1};
    std::uint32_t next_swapchain_{1};
    std::uint32_t next_swapchain_frame_{1};
    std::uint32_t next_shader_{1};
    std::uint32_t next_descriptor_set_layout_{1};
    std::uint32_t next_descriptor_set_{1};
    std::uint32_t next_transient_resource_{1};
    std::uint32_t next_pipeline_layout_{1};
    std::uint32_t next_graphics_pipeline_{1};
    std::uint32_t next_compute_pipeline_{1};
    std::vector<BufferDesc> buffers_;
    std::vector<bool> buffer_active_;
    std::vector<std::vector<std::uint8_t>> buffer_bytes_;
    std::vector<TextureDesc> textures_;
    std::vector<bool> texture_active_;
    std::vector<ResourceState> texture_states_;
    std::vector<SamplerDesc> samplers_;
    std::vector<bool> sampler_active_;
    std::vector<SwapchainDesc> swapchains_;
    std::vector<SwapchainHandle> swapchain_frame_swapchains_;
    std::vector<bool> swapchain_frame_active_;
    std::vector<bool> swapchain_frame_presented_;
    std::vector<ResourceState> swapchain_states_;
    std::vector<bool> swapchain_presentable_;
    std::vector<bool> swapchain_frame_reserved_;
    std::vector<ShaderStage> shader_stages_;
    std::vector<bool> shader_active_;
    std::vector<DescriptorSetLayoutDesc> descriptor_set_layouts_;
    std::vector<bool> descriptor_set_layout_active_;
    std::vector<DescriptorSetLayoutHandle> descriptor_set_layout_for_sets_;
    std::vector<bool> descriptor_set_active_;
    std::vector<TransientLeaseRecord> transient_leases_;
    std::vector<PipelineLayoutDesc> pipeline_layouts_;
    std::vector<bool> pipeline_layout_active_;
    std::vector<PipelineLayoutHandle> graphics_pipeline_layouts_;
    std::vector<Format> graphics_pipeline_color_formats_;
    std::vector<Format> graphics_pipeline_depth_formats_;
    std::vector<bool> graphics_pipeline_active_;
    std::vector<PipelineLayoutHandle> compute_pipeline_layouts_;
    std::vector<bool> compute_pipeline_active_;
    std::array<FenceValue, 3> last_signaled_by_queue_{};
    std::array<FenceValue, 3> completed_by_queue_{};
    FenceValue last_signaled_{};
    FenceValue completed_{};
};

} // namespace mirakana::rhi
