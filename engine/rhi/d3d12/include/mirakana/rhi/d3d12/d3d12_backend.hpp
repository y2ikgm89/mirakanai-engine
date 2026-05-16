// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace mirakana::rhi::d3d12 {

struct RuntimeProbe {
    bool windows_sdk_available{false};
    bool dxgi_factory_created{false};
    bool debug_layer_available{false};
    std::uint32_t adapter_count{0};
    std::uint32_t hardware_adapter_count{0};
    bool hardware_device_supported{false};
    bool warp_adapter_available{false};
    bool warp_device_supported{false};
};

struct DeviceBootstrapDesc {
    bool prefer_warp{false};
    bool enable_debug_layer{false};
};

struct DeviceBootstrapResult {
    bool succeeded{false};
    bool used_warp{false};
    bool debug_layer_enabled{false};
    bool device_created{false};
    bool command_queue_created{false};
    bool fence_created{false};
    std::uint64_t initial_fence_value{0};
};

struct ResourceOwnershipDesc {
    DeviceBootstrapDesc device;
    std::uint64_t upload_buffer_size_bytes{4096};
    std::uint64_t readback_buffer_size_bytes{4096};
    Extent2D texture_extent{.width = 4, .height = 4};
    Format texture_format{Format::rgba8_unorm};
};

struct ResourceOwnershipResult {
    bool succeeded{false};
    bool validation_failed{false};
    bool device_created{false};
    bool upload_buffer_created{false};
    bool readback_buffer_created{false};
    bool default_texture_created{false};
    std::uint64_t texture_allocation_size_bytes{0};
};

struct NativeResourceHandle {
    std::uint32_t value{0};
};

struct NativeCommandListHandle {
    std::uint32_t value{0};
};

struct NativeWindowHandle {
    void* value{nullptr};
};

struct NativeSwapchainHandle {
    std::uint32_t value{0};
};

struct NativeDescriptorHeapHandle {
    std::uint32_t value{0};
};

struct NativeRootSignatureHandle {
    std::uint32_t value{0};
};

struct NativeShaderHandle {
    std::uint32_t value{0};
};

struct NativeGraphicsPipelineHandle {
    std::uint32_t value{0};
};

struct NativeComputePipelineHandle {
    std::uint32_t value{0};
};

struct D3d12SharedTextureHandle {
    std::uintptr_t value{0};
};

struct D3d12SharedTextureExportResult {
    bool succeeded{false};
    bool device_unavailable{false};
    bool invalid_texture{false};
    bool texture_not_shareable{false};
    std::uint32_t native_error_code{0};
    D3d12SharedTextureHandle shared_handle;
    Extent2D extent;
    Format format{Format::unknown};
};

enum class NativeDescriptorHeapKind : std::uint8_t { cbv_srv_uav = 0, sampler };

struct NativeSwapchainDesc {
    NativeWindowHandle window;
    SwapchainDesc swapchain;
};

struct NativeDescriptorHeapDesc {
    NativeDescriptorHeapKind kind{NativeDescriptorHeapKind::cbv_srv_uav};
    std::uint32_t capacity{0};
    bool shader_visible{true};
};

struct NativeDescriptorHeapBinding {
    NativeDescriptorHeapHandle cbv_srv_uav;
    NativeDescriptorHeapHandle sampler;
};

struct NativeDescriptorWriteDesc {
    NativeDescriptorHeapHandle heap;
    std::uint32_t descriptor_index{0};
    NativeResourceHandle resource;
    DescriptorType type{DescriptorType::uniform_buffer};
    SamplerDesc sampler;
};

struct NativeRootSignatureDesc {
    std::vector<DescriptorSetLayoutDesc> descriptor_sets;
    std::uint32_t push_constant_bytes{0};
};

struct NativeShaderBytecodeDesc {
    ShaderStage stage{ShaderStage::vertex};
    const void* bytecode{nullptr};
    std::uint64_t bytecode_size{0};
};

struct NativeGraphicsPipelineDesc {
    NativeRootSignatureHandle root_signature;
    NativeShaderHandle vertex_shader;
    NativeShaderHandle fragment_shader;
    Format color_format{Format::unknown};
    Format depth_format{Format::unknown};
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    std::vector<VertexBufferLayoutDesc> vertex_buffers;
    std::vector<VertexAttributeDesc> vertex_attributes;
    DepthStencilStateDesc depth_state;
};

struct NativeComputePipelineDesc {
    NativeRootSignatureHandle root_signature;
    NativeShaderHandle compute_shader;
};

struct NativeSwapchainInfo {
    bool valid{false};
    Extent2D extent;
    Format format{Format::unknown};
    std::uint32_t buffer_count{0};
    std::uint32_t current_back_buffer{0};
    std::uint32_t render_target_view_count{0};
};

struct NativeDescriptorHeapInfo {
    bool valid{false};
    NativeDescriptorHeapKind kind{NativeDescriptorHeapKind::cbv_srv_uav};
    std::uint32_t capacity{0};
    bool shader_visible{false};
    std::uint32_t descriptor_size{0};
};

struct NativeRootSignatureInfo {
    bool valid{false};
    std::uint32_t descriptor_table_count{0};
    std::uint32_t descriptor_range_count{0};
    std::uint32_t push_constant_bytes{0};
};

struct NativeShaderInfo {
    bool valid{false};
    ShaderStage stage{ShaderStage::vertex};
    std::uint64_t bytecode_size{0};
};

struct NativeGraphicsPipelineInfo {
    bool valid{false};
    NativeRootSignatureHandle root_signature;
    Format color_format{Format::unknown};
    Format depth_format{Format::unknown};
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    std::uint32_t vertex_buffer_layout_count{0};
    std::uint32_t vertex_attribute_count{0};
};

struct NativeComputePipelineInfo {
    bool valid{false};
    NativeRootSignatureHandle root_signature;
};

struct DeviceContextStats {
    std::uint64_t committed_buffers_created{0};
    std::uint64_t committed_textures_created{0};
    std::uint64_t committed_resources_alive{0};
    std::uint64_t swapchains_created{0};
    std::uint64_t swapchains_alive{0};
    std::uint64_t swapchain_back_buffers_created{0};
    std::uint64_t swapchain_presents{0};
    std::uint64_t swapchain_resizes{0};
    std::uint64_t render_target_view_heaps_created{0};
    std::uint64_t render_target_views_created{0};
    std::uint64_t swapchain_back_buffer_transitions{0};
    std::uint64_t texture_transitions{0};
    std::uint64_t texture_aliasing_barriers{0};
    std::uint64_t swapchain_back_buffer_clears{0};
    std::uint64_t texture_render_target_clears{0};
    std::uint64_t descriptor_heaps_created{0};
    std::uint64_t shader_visible_descriptor_heaps_created{0};
    std::uint64_t shader_visible_descriptors_reserved{0};
    std::uint64_t descriptor_views_written{0};
    std::uint64_t root_signatures_created{0};
    std::uint64_t descriptor_tables_created{0};
    std::uint64_t descriptor_ranges_created{0};
    std::uint64_t descriptor_heaps_bound{0};
    std::uint64_t root_signatures_bound{0};
    std::uint64_t descriptor_tables_bound{0};
    std::uint64_t shader_modules_created{0};
    std::uint64_t shader_bytecode_bytes_owned{0};
    std::uint64_t graphics_pipelines_created{0};
    std::uint64_t compute_pipelines_created{0};
    std::uint64_t swapchain_render_targets_set{0};
    std::uint64_t texture_render_targets_set{0};
    std::uint64_t graphics_pipelines_bound{0};
    std::uint64_t compute_pipelines_bound{0};
    std::uint64_t vertex_buffer_bindings{0};
    std::uint64_t index_buffer_bindings{0};
    std::uint64_t draw_calls{0};
    std::uint64_t indexed_draw_calls{0};
    std::uint64_t compute_dispatches{0};
    std::uint64_t vertices_submitted{0};
    std::uint64_t indices_submitted{0};
    std::uint64_t compute_workgroups_x{0};
    std::uint64_t compute_workgroups_y{0};
    std::uint64_t compute_workgroups_z{0};
    std::uint64_t buffer_copies{0};
    std::uint64_t buffer_texture_copies{0};
    std::uint64_t texture_buffer_copies{0};
    std::uint64_t bytes_copied{0};
    std::uint64_t command_allocators_created{0};
    std::uint64_t command_lists_created{0};
    std::uint64_t command_lists_alive{0};
    std::uint64_t command_lists_closed{0};
    std::uint64_t command_lists_reset{0};
    std::uint64_t command_lists_executed{0};
    std::uint64_t fence_signals{0};
    std::uint64_t fence_waits{0};
    std::uint64_t fence_wait_timeouts{0};
    std::uint64_t queue_waits{0};
    std::uint64_t queue_wait_failures{0};
    std::uint64_t last_submitted_fence_value{0};
    QueueKind last_submitted_fence_queue{QueueKind::graphics};
    std::uint64_t graphics_queue_submits{0};
    std::uint64_t compute_queue_submits{0};
    std::uint64_t copy_queue_submits{0};
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
    std::uint64_t shared_texture_handles_created{0};
};

enum class QueueTimestampMeasurementStatus : std::uint8_t {
    unsupported = 0,
    failed,
    ready,
};

struct QueueTimestampMeasurementSupport {
    QueueKind queue{QueueKind::graphics};
    bool supported{false};
    std::uint64_t frequency{0};
    std::string_view diagnostic;
};

struct QueueTimestampInterval {
    QueueKind queue{QueueKind::graphics};
    QueueTimestampMeasurementStatus status{QueueTimestampMeasurementStatus::unsupported};
    std::uint64_t frequency{0};
    std::uint64_t begin_ticks{0};
    std::uint64_t end_ticks{0};
    double elapsed_seconds{0.0};
    std::string_view diagnostic;
};

enum class QueueClockCalibrationStatus : std::uint8_t {
    unsupported = 0,
    failed,
    ready,
};

struct QueueClockCalibration {
    QueueKind queue{QueueKind::graphics};
    QueueClockCalibrationStatus status{QueueClockCalibrationStatus::unsupported};
    std::uint64_t frequency{0};
    std::uint64_t gpu_timestamp{0};
    std::uint64_t cpu_timestamp{0};
    std::string_view diagnostic;
};

enum class QueueCalibratedTimingStatus : std::uint8_t {
    unsupported = 0,
    failed,
    ready,
};

struct QueueCalibratedTiming {
    QueueKind queue{QueueKind::graphics};
    QueueCalibratedTimingStatus status{QueueCalibratedTimingStatus::unsupported};
    std::uint64_t frequency{0};
    std::uint64_t qpc_frequency{0};
    std::uint64_t begin_ticks{0};
    std::uint64_t end_ticks{0};
    double elapsed_seconds{0.0};
    std::uint64_t calibration_before_gpu_timestamp{0};
    std::uint64_t calibration_before_cpu_timestamp{0};
    std::uint64_t calibration_after_gpu_timestamp{0};
    std::uint64_t calibration_after_cpu_timestamp{0};
    std::uint64_t calibrated_begin_cpu_timestamp{0};
    std::uint64_t calibrated_end_cpu_timestamp{0};
    std::string_view diagnostic;
};

enum class SubmittedCommandCalibratedTimingStatus : std::uint8_t {
    unsupported = 0,
    not_submitted,
    not_ready,
    failed,
    ready,
};

struct SubmittedCommandCalibratedTiming {
    QueueKind queue{QueueKind::graphics};
    FenceValue submitted_fence{};
    SubmittedCommandCalibratedTimingStatus status{SubmittedCommandCalibratedTimingStatus::unsupported};
    std::uint64_t frequency{0};
    std::uint64_t qpc_frequency{0};
    std::uint64_t begin_ticks{0};
    std::uint64_t end_ticks{0};
    double elapsed_seconds{0.0};
    std::uint64_t calibration_before_gpu_timestamp{0};
    std::uint64_t calibration_before_cpu_timestamp{0};
    std::uint64_t calibration_after_gpu_timestamp{0};
    std::uint64_t calibration_after_cpu_timestamp{0};
    std::uint64_t calibrated_begin_cpu_timestamp{0};
    std::uint64_t calibrated_end_cpu_timestamp{0};
    std::string_view diagnostic;
};

enum class QueueCalibratedOverlapStatus : std::uint8_t {
    not_requested = 0,
    missing_schedule_evidence,
    missing_timing_evidence,
    measured_non_overlapping,
    measured_overlapping,
};

struct QueueCalibratedOverlapDiagnostics {
    QueueCalibratedOverlapStatus status{QueueCalibratedOverlapStatus::not_requested};
    RhiAsyncOverlapReadinessStatus schedule_status{RhiAsyncOverlapReadinessStatus::not_requested};
    bool schedule_ready{false};
    bool compute_timing_ready{false};
    bool graphics_timing_ready{false};
    bool intervals_overlap{false};
    std::uint64_t compute_begin_cpu_timestamp{0};
    std::uint64_t compute_end_cpu_timestamp{0};
    std::uint64_t graphics_begin_cpu_timestamp{0};
    std::uint64_t graphics_end_cpu_timestamp{0};
    double compute_elapsed_seconds{0.0};
    double graphics_elapsed_seconds{0.0};
    std::string_view diagnostic;
};

class DeviceContext final {
  public:
    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;
    DeviceContext(DeviceContext&&) noexcept;
    DeviceContext& operator=(DeviceContext&&) noexcept;
    ~DeviceContext();

    [[nodiscard]] static std::unique_ptr<DeviceContext> create(const DeviceBootstrapDesc& desc);

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] static BackendKind backend_kind() noexcept;
    [[nodiscard]] bool used_warp() const noexcept;
    [[nodiscard]] bool debug_layer_enabled() const noexcept;
    [[nodiscard]] DeviceContextStats stats() const noexcept;

    [[nodiscard]] NativeResourceHandle create_committed_buffer(const BufferDesc& desc);
    [[nodiscard]] NativeResourceHandle create_committed_texture(const TextureDesc& desc);
    [[nodiscard]] NativeSwapchainHandle create_swapchain_for_window(const NativeSwapchainDesc& desc);
    [[nodiscard]] bool present_swapchain(NativeSwapchainHandle handle);
    [[nodiscard]] bool resize_swapchain(NativeSwapchainHandle handle, Extent2D extent);
    [[nodiscard]] NativeSwapchainInfo swapchain_info(NativeSwapchainHandle handle) const noexcept;
    [[nodiscard]] NativeDescriptorHeapHandle create_descriptor_heap(const NativeDescriptorHeapDesc& desc);
    [[nodiscard]] NativeDescriptorHeapInfo descriptor_heap_info(NativeDescriptorHeapHandle handle) const noexcept;
    [[nodiscard]] bool write_descriptor(const NativeDescriptorWriteDesc& desc);
    [[nodiscard]] NativeRootSignatureHandle create_root_signature(const NativeRootSignatureDesc& desc);
    [[nodiscard]] NativeRootSignatureInfo root_signature_info(NativeRootSignatureHandle handle) const noexcept;
    [[nodiscard]] NativeShaderHandle create_shader_module(const NativeShaderBytecodeDesc& desc);
    [[nodiscard]] NativeShaderInfo shader_info(NativeShaderHandle handle) const noexcept;
    [[nodiscard]] NativeGraphicsPipelineHandle create_graphics_pipeline(const NativeGraphicsPipelineDesc& desc);
    [[nodiscard]] NativeGraphicsPipelineInfo graphics_pipeline_info(NativeGraphicsPipelineHandle handle) const noexcept;
    [[nodiscard]] NativeComputePipelineHandle create_compute_pipeline(const NativeComputePipelineDesc& desc);
    [[nodiscard]] NativeComputePipelineInfo compute_pipeline_info(NativeComputePipelineHandle handle) const noexcept;
    [[nodiscard]] bool transition_swapchain_back_buffer(NativeCommandListHandle commands,
                                                        NativeSwapchainHandle swapchain, ResourceState before,
                                                        ResourceState after);
    [[nodiscard]] bool transition_texture(NativeCommandListHandle commands, NativeResourceHandle texture,
                                          ResourceState before, ResourceState after);
    [[nodiscard]] bool texture_aliasing_barrier(NativeCommandListHandle commands, NativeResourceHandle before,
                                                NativeResourceHandle after);
    [[nodiscard]] bool clear_swapchain_back_buffer(NativeCommandListHandle commands, NativeSwapchainHandle swapchain,
                                                   float red, float green, float blue, float alpha);
    [[nodiscard]] bool clear_texture_render_target(NativeCommandListHandle commands, NativeResourceHandle texture,
                                                   float red, float green, float blue, float alpha);
    [[nodiscard]] bool clear_texture_depth_stencil(NativeCommandListHandle commands, NativeResourceHandle texture,
                                                   float depth);
    [[nodiscard]] bool set_descriptor_heaps(NativeCommandListHandle commands,
                                            const NativeDescriptorHeapBinding& binding);
    [[nodiscard]] bool set_graphics_root_signature(NativeCommandListHandle commands,
                                                   NativeRootSignatureHandle root_signature);
    [[nodiscard]] bool set_graphics_descriptor_table(NativeCommandListHandle commands,
                                                     NativeRootSignatureHandle root_signature,
                                                     std::uint32_t root_parameter_index,
                                                     NativeDescriptorHeapHandle heap, std::uint32_t descriptor_index);
    [[nodiscard]] bool set_compute_root_signature(NativeCommandListHandle commands,
                                                  NativeRootSignatureHandle root_signature);
    [[nodiscard]] bool set_compute_descriptor_table(NativeCommandListHandle commands,
                                                    NativeRootSignatureHandle root_signature,
                                                    std::uint32_t root_parameter_index, NativeDescriptorHeapHandle heap,
                                                    std::uint32_t descriptor_index);
    [[nodiscard]] bool set_swapchain_render_target(NativeCommandListHandle commands, NativeSwapchainHandle swapchain,
                                                   NativeResourceHandle depth = {});
    [[nodiscard]] bool set_texture_render_target(NativeCommandListHandle commands, NativeResourceHandle texture,
                                                 NativeResourceHandle depth = {});
    [[nodiscard]] bool bind_graphics_pipeline(NativeCommandListHandle commands, NativeGraphicsPipelineHandle pipeline);
    [[nodiscard]] bool bind_compute_pipeline(NativeCommandListHandle commands, NativeComputePipelineHandle pipeline);
    [[nodiscard]] bool bind_vertex_buffer(NativeCommandListHandle commands, NativeResourceHandle buffer,
                                          std::uint64_t offset, std::uint32_t stride, std::uint32_t slot = 0);
    [[nodiscard]] bool bind_index_buffer(NativeCommandListHandle commands, NativeResourceHandle buffer,
                                         std::uint64_t offset, IndexFormat format);
    [[nodiscard]] bool draw(NativeCommandListHandle commands, std::uint32_t vertex_count, std::uint32_t instance_count);
    [[nodiscard]] bool draw_indexed(NativeCommandListHandle commands, std::uint32_t index_count,
                                    std::uint32_t instance_count);
    [[nodiscard]] bool dispatch(NativeCommandListHandle commands, std::uint32_t group_count_x,
                                std::uint32_t group_count_y, std::uint32_t group_count_z);
    [[nodiscard]] bool set_viewport(NativeCommandListHandle commands, const mirakana::rhi::ViewportDesc& viewport);
    [[nodiscard]] bool set_scissor(NativeCommandListHandle commands, const mirakana::rhi::ScissorRectDesc& scissor);
    [[nodiscard]] bool copy_buffer(NativeCommandListHandle commands, NativeResourceHandle source,
                                   NativeResourceHandle destination, const BufferCopyRegion& region);
    [[nodiscard]] bool copy_buffer_to_texture(NativeCommandListHandle commands, NativeResourceHandle source,
                                              NativeResourceHandle destination, const BufferTextureCopyRegion& region);
    [[nodiscard]] bool copy_texture_to_buffer(NativeCommandListHandle commands, NativeResourceHandle source,
                                              NativeResourceHandle destination, const BufferTextureCopyRegion& region);
    [[nodiscard]] bool write_buffer(NativeResourceHandle buffer, std::uint64_t offset,
                                    std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::vector<std::uint8_t> read_buffer(NativeResourceHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) const;
    [[nodiscard]] D3d12SharedTextureExportResult export_shared_texture(NativeResourceHandle texture,
                                                                       const TextureDesc& desc);
    [[nodiscard]] NativeCommandListHandle create_command_list(QueueKind queue);
    [[nodiscard]] bool close_command_list(NativeCommandListHandle handle);
    [[nodiscard]] bool reset_command_list(NativeCommandListHandle handle);
    [[nodiscard]] FenceValue execute_command_list(NativeCommandListHandle handle);
    [[nodiscard]] FenceValue completed_fence() const noexcept;
    /// Highest fence value signaled on the graphics queue after `execute_command_list` (0 if none yet).
    [[nodiscard]] FenceValue last_submitted_fence() const noexcept;
    /// Releases the committed resource slot after the GPU has finished using it. Safe to call multiple times.
    void destroy_committed_resource(NativeResourceHandle handle) noexcept;
    void destroy_graphics_pipeline(NativeGraphicsPipelineHandle handle) noexcept;
    void destroy_compute_pipeline(NativeComputePipelineHandle handle) noexcept;
    void destroy_root_signature(NativeRootSignatureHandle handle) noexcept;
    void destroy_shader_module(NativeShaderHandle handle) noexcept;
    [[nodiscard]] bool wait_for_fence(FenceValue fence, std::uint32_t timeout_ms);
    [[nodiscard]] bool queue_wait_for_fence(QueueKind queue, FenceValue fence);
    [[nodiscard]] QueueTimestampMeasurementSupport queue_timestamp_measurement_support(QueueKind queue);
    [[nodiscard]] QueueTimestampInterval measure_queue_timestamp_interval(QueueKind queue);
    [[nodiscard]] QueueClockCalibration calibrate_queue_clock(QueueKind queue);
    [[nodiscard]] QueueCalibratedTiming measure_calibrated_queue_timing(QueueKind queue);
    [[nodiscard]] SubmittedCommandCalibratedTiming read_submitted_command_calibrated_timing(FenceValue fence);

    void unwind_gpu_debug_events(NativeCommandListHandle handle) noexcept;
    [[nodiscard]] bool begin_gpu_debug_event(NativeCommandListHandle handle, std::string_view name);
    [[nodiscard]] bool end_gpu_debug_event(NativeCommandListHandle handle);
    [[nodiscard]] bool insert_gpu_debug_marker(NativeCommandListHandle handle, std::string_view name);
    [[nodiscard]] std::uint32_t gpu_debug_scope_depth(NativeCommandListHandle handle) noexcept;
    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept;
    [[nodiscard]] RhiDeviceMemoryDiagnostics memory_diagnostics() const;

  private:
    struct Impl;

    explicit DeviceContext(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] BackendKind backend_kind() noexcept;
[[nodiscard]] std::string_view backend_name() noexcept;
[[nodiscard]] bool compiled_with_windows_sdk() noexcept;
[[nodiscard]] RuntimeProbe probe_runtime() noexcept;
[[nodiscard]] DeviceBootstrapResult bootstrap_device(const DeviceBootstrapDesc& desc) noexcept;
[[nodiscard]] ResourceOwnershipResult bootstrap_resource_ownership(const ResourceOwnershipDesc& desc) noexcept;
[[nodiscard]] std::unique_ptr<IRhiDevice> create_rhi_device(const DeviceBootstrapDesc& desc);
[[nodiscard]] QueueCalibratedTiming measure_rhi_device_calibrated_queue_timing(IRhiDevice& device, QueueKind queue);
[[nodiscard]] SubmittedCommandCalibratedTiming read_rhi_device_submitted_command_calibrated_timing(IRhiDevice& device,
                                                                                                   FenceValue fence);
[[nodiscard]] QueueCalibratedOverlapDiagnostics
diagnose_calibrated_compute_graphics_overlap(const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                             const QueueCalibratedTiming& compute,
                                             const QueueCalibratedTiming& graphics) noexcept;
[[nodiscard]] QueueCalibratedOverlapDiagnostics
diagnose_calibrated_compute_graphics_overlap(const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                             const SubmittedCommandCalibratedTiming& compute,
                                             const SubmittedCommandCalibratedTiming& graphics) noexcept;
[[nodiscard]] QueueCalibratedOverlapDiagnostics
diagnose_rhi_device_submitted_command_compute_graphics_overlap(IRhiDevice& device,
                                                               const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                                               FenceValue compute_fence, FenceValue graphics_fence);
[[nodiscard]] D3d12SharedTextureExportResult export_shared_texture(IRhiDevice& device, TextureHandle texture) noexcept;
void close_shared_texture_handle(D3d12SharedTextureHandle handle) noexcept;

} // namespace mirakana::rhi::d3d12
