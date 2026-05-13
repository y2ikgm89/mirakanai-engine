// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/backend_capabilities.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::rhi::vulkan {

using NativeVulkanCommandBuffer = void*;

inline constexpr std::size_t invalid_vulkan_device_index = static_cast<std::size_t>(-1);
inline constexpr std::uint32_t invalid_vulkan_queue_family = static_cast<std::uint32_t>(-1);

struct VulkanApiVersion {
    std::uint32_t major{0};
    std::uint32_t minor{0};
};

enum class VulkanPhysicalDeviceType : std::uint8_t {
    other,
    integrated_gpu,
    discrete_gpu,
    virtual_gpu,
    cpu,
};

enum class VulkanQueueCapability : std::uint8_t {
    none = 0,
    graphics = 1U << 0U,
    compute = 1U << 1U,
    transfer = 1U << 2U,
};

enum class VulkanCommandScope : std::uint8_t {
    loader,
    global,
    instance,
    device,
};

[[nodiscard]] constexpr VulkanQueueCapability operator|(VulkanQueueCapability lhs, VulkanQueueCapability rhs) noexcept {
    return static_cast<VulkanQueueCapability>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr bool has_queue_capability(VulkanQueueCapability value, VulkanQueueCapability flag) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0U;
}

struct VulkanQueueFamilyCandidate {
    std::uint32_t index{invalid_vulkan_queue_family};
    std::uint32_t queue_count{0};
    VulkanQueueCapability capabilities{VulkanQueueCapability::none};
    bool supports_present{false};
};

struct VulkanPhysicalDeviceCandidate {
    std::string name;
    VulkanPhysicalDeviceType type{VulkanPhysicalDeviceType::other};
    VulkanApiVersion api_version{};
    bool supports_swapchain_extension{false};
    bool supports_dynamic_rendering{false};
    bool supports_synchronization2{false};
    std::vector<VulkanQueueFamilyCandidate> queue_families;
};

struct VulkanDeviceSelection {
    bool suitable{false};
    std::size_t device_index{invalid_vulkan_device_index};
    std::uint32_t graphics_queue_family{invalid_vulkan_queue_family};
    std::uint32_t present_queue_family{invalid_vulkan_queue_family};
    int score{0};
    std::string diagnostic;
};

struct VulkanInstanceCreateDesc {
    std::string application_name{"GameEngine"};
    VulkanApiVersion api_version{.major = 1, .minor = 3};
    std::vector<std::string> required_extensions;
    std::vector<std::string> optional_extensions;
    bool enable_validation{false};
};

struct VulkanInstanceCreatePlan {
    bool supported{false};
    VulkanApiVersion api_version{};
    std::vector<std::string> enabled_extensions;
    bool validation_enabled{false};
    std::string diagnostic;
};

struct VulkanDeviceQueueCreatePlan {
    std::uint32_t queue_family{invalid_vulkan_queue_family};
    std::uint32_t queue_count{0};
    float priority{1.0F};
};

struct VulkanLogicalDeviceCreateDesc {
    std::vector<std::string> required_extensions{"VK_KHR_swapchain"};
    std::vector<std::string> optional_extensions;
    bool require_dynamic_rendering{true};
    bool require_synchronization2{true};
};

struct VulkanLogicalDeviceCreatePlan {
    bool supported{false};
    std::vector<VulkanDeviceQueueCreatePlan> queue_families;
    std::vector<std::string> enabled_extensions;
    bool dynamic_rendering_enabled{false};
    bool synchronization2_enabled{false};
    std::string diagnostic;
};

enum class VulkanPresentMode : std::uint8_t {
    immediate,
    mailbox,
    fifo,
    fifo_relaxed,
};

struct VulkanSurfaceFormatCandidate {
    Format format{Format::unknown};
    bool srgb{false};
};

struct VulkanSurfaceCapabilities {
    std::uint32_t min_image_count{0};
    std::uint32_t max_image_count{0};
    Extent2D current_extent;
    Extent2D min_image_extent;
    Extent2D max_image_extent;
    bool current_extent_defined{false};
};

struct VulkanSwapchainSupport {
    VulkanSurfaceCapabilities capabilities;
    std::vector<VulkanSurfaceFormatCandidate> formats;
    std::vector<VulkanPresentMode> present_modes;
};

struct VulkanSwapchainCreateDesc {
    Extent2D requested_extent;
    Format preferred_format{Format::bgra8_unorm};
    std::uint32_t requested_image_count{2};
    bool vsync{true};
};

struct VulkanSwapchainCreatePlan {
    bool supported{false};
    Extent2D extent;
    Format format{Format::unknown};
    std::uint32_t image_count{0};
    std::uint32_t image_view_count{0};
    VulkanPresentMode present_mode{VulkanPresentMode::fifo};
    bool acquire_before_render{false};
    std::string diagnostic;
};

struct VulkanSwapchainResizePlan {
    bool resize_required{false};
    Extent2D extent;
    std::string diagnostic;
};

struct VulkanDynamicRenderingColorAttachmentDesc {
    Format format{Format::unknown};
    LoadAction load_action{LoadAction::clear};
    StoreAction store_action{StoreAction::store};
};

struct VulkanDynamicRenderingDesc {
    Extent2D extent;
    std::vector<VulkanDynamicRenderingColorAttachmentDesc> color_attachments;
    bool has_depth_attachment{false};
    Format depth_format{Format::unknown};
    LoadAction depth_load_action{LoadAction::clear};
    StoreAction depth_store_action{StoreAction::store};
};

struct VulkanDynamicRenderingPlan {
    bool supported{false};
    Extent2D extent;
    std::uint32_t color_attachment_count{0};
    std::vector<Format> color_formats;
    bool depth_attachment_enabled{false};
    Format depth_format{Format::unknown};
    bool begin_rendering_command_resolved{false};
    bool end_rendering_command_resolved{false};
    std::string diagnostic;
};

enum class VulkanSynchronizationStage : std::uint8_t {
    none,
    color_attachment_output,
    depth_attachment,
    transfer,
    shader,
};

enum class VulkanSynchronizationAccess : std::uint8_t {
    none,
    color_attachment_write,
    depth_attachment_read_write,
    transfer_read,
    transfer_write,
    shader_read,
};

enum class VulkanFrameSynchronizationStep : std::uint8_t {
    acquire,
    render,
    readback,
    present,
};

struct VulkanFrameSynchronizationBarrier {
    ResourceState before{ResourceState::undefined};
    ResourceState after{ResourceState::undefined};
    VulkanSynchronizationStage src_stage{VulkanSynchronizationStage::none};
    VulkanSynchronizationAccess src_access{VulkanSynchronizationAccess::none};
    VulkanSynchronizationStage dst_stage{VulkanSynchronizationStage::none};
    VulkanSynchronizationAccess dst_access{VulkanSynchronizationAccess::none};
};

struct VulkanFrameSynchronizationDesc {
    bool readback_required{false};
    bool present_required{true};
};

struct VulkanFrameSynchronizationPlan {
    bool supported{false};
    bool pipeline_barrier2_command_resolved{false};
    bool queue_submit2_command_resolved{false};
    std::vector<VulkanFrameSynchronizationStep> order;
    std::vector<VulkanFrameSynchronizationBarrier> barriers;
    std::string diagnostic;
};

struct VulkanTextureTransitionBarrierPlan {
    bool supported{false};
    VulkanFrameSynchronizationBarrier barrier;
    std::string diagnostic;
};

struct VulkanSpirvShaderArtifactDesc {
    ShaderStage stage{ShaderStage::vertex};
    const void* bytecode{nullptr};
    std::uint64_t bytecode_size{0};
};

struct VulkanSpirvShaderArtifactValidation {
    bool valid{false};
    std::uint32_t word_count{0};
    std::string diagnostic;
};

struct VulkanRhiDeviceMappingDesc {
    bool command_pool_ready{false};
    VulkanSwapchainCreatePlan swapchain;
    VulkanDynamicRenderingPlan dynamic_rendering;
    VulkanFrameSynchronizationPlan frame_synchronization;
    VulkanSpirvShaderArtifactValidation vertex_shader;
    VulkanSpirvShaderArtifactValidation fragment_shader;
    VulkanSpirvShaderArtifactValidation compute_shader;
    bool descriptor_binding_ready{false};
    bool compute_dispatch_ready{false};
    bool visible_clear_readback_ready{false};
    bool visible_draw_readback_ready{false};
    bool visible_texture_sampling_readback_ready{false};
    bool visible_depth_readback_ready{false};
};

struct VulkanRhiDeviceMappingPlan {
    bool supported{false};
    bool resources_mapped{false};
    bool swapchains_mapped{false};
    bool render_passes_mapped{false};
    bool pipelines_mapped{false};
    bool command_lists_mapped{false};
    bool fences_mapped{false};
    bool readbacks_mapped{false};
    bool descriptor_sets_mapped{false};
    bool compute_dispatch_mapped{false};
    bool visible_clear_readbacks_mapped{false};
    bool visible_draw_readbacks_mapped{false};
    bool visible_texture_sampling_readbacks_mapped{false};
    bool visible_depth_readbacks_mapped{false};
    std::string diagnostic;
};

enum class VulkanBufferMemoryDomain : std::uint8_t {
    device_local,
    upload,
    readback,
};

struct VulkanBufferUsagePlan {
    bool transfer_source{false};
    bool transfer_destination{false};
    bool vertex{false};
    bool index{false};
    bool uniform{false};
    bool storage{false};
};

struct VulkanBufferMemoryPlan {
    bool device_local{false};
    bool host_visible{false};
    bool host_coherent{false};
};

struct VulkanRuntimeBufferDesc {
    BufferDesc buffer;
    VulkanBufferMemoryDomain memory_domain{VulkanBufferMemoryDomain::device_local};
};

struct VulkanRuntimeBufferCreatePlan {
    bool supported{false};
    std::uint64_t size_bytes{0};
    VulkanBufferUsagePlan usage;
    VulkanBufferMemoryPlan memory;
    std::string diagnostic;
};

struct VulkanTextureUsagePlan {
    bool transfer_source{false};
    bool transfer_destination{false};
    bool sampled{false};
    bool storage{false};
    bool color_attachment{false};
    bool depth_stencil_attachment{false};
};

struct VulkanRuntimeTextureDesc {
    TextureDesc texture;
};

struct VulkanRuntimeTextureCreatePlan {
    bool supported{false};
    Extent3D extent;
    Format format{Format::unknown};
    VulkanTextureUsagePlan usage;
    VulkanBufferMemoryPlan memory;
    std::string diagnostic;
};

struct VulkanCommandRequest {
    std::string name;
    VulkanCommandScope scope{VulkanCommandScope::global};
    bool required{true};
};

struct VulkanCommandAvailability {
    std::string name;
    VulkanCommandScope scope{VulkanCommandScope::global};
    bool available{false};
};

struct VulkanCommandResolution {
    VulkanCommandRequest request;
    bool resolved{false};
};

struct VulkanCommandResolutionPlan {
    bool supported{false};
    std::vector<VulkanCommandResolution> resolutions;
    std::vector<VulkanCommandRequest> missing_required_commands;
    std::string diagnostic;
};

struct VulkanLoaderProbeDesc {
    RhiHostPlatform host{RhiHostPlatform::unknown};
    std::string_view runtime_library;
    std::string_view get_instance_proc_addr_symbol{"vkGetInstanceProcAddr"};
};

struct VulkanLoaderProbeResult {
    BackendProbeResult probe;
    bool runtime_loaded{false};
    bool get_instance_proc_addr_found{false};
    std::string runtime_library;
};

struct VulkanRuntimeGlobalCommandProbeResult {
    VulkanLoaderProbeResult loader;
    VulkanCommandResolutionPlan command_plan;
};

struct VulkanRuntimeInstanceCapabilityProbeResult {
    VulkanRuntimeGlobalCommandProbeResult global;
    VulkanApiVersion api_version{};
    std::vector<std::string> instance_extensions;
    VulkanInstanceCreatePlan instance_plan;
};

struct VulkanRuntimeInstanceCommandProbeResult {
    VulkanRuntimeInstanceCapabilityProbeResult capabilities;
    VulkanCommandResolutionPlan command_plan;
    bool instance_created{false};
    bool instance_destroyed{false};
    std::string diagnostic;
};

struct VulkanRuntimeInstanceCreateResult;
struct VulkanRuntimeDeviceCreateResult;
struct VulkanRuntimeCommandPoolDesc;
struct VulkanRuntimeCommandPoolCreateResult;
struct VulkanRuntimeBufferCreateResult;
struct VulkanRuntimeTextureCreateResult;
struct VulkanRuntimeSamplerDesc;
struct VulkanRuntimeSamplerCreateResult;
struct VulkanRuntimeShaderModuleDesc;
struct VulkanRuntimeShaderModuleCreateResult;
struct VulkanRuntimeDescriptorSetLayoutDesc;
struct VulkanRuntimeDescriptorSetLayoutCreateResult;
struct VulkanRuntimeDescriptorSetDesc;
struct VulkanRuntimeDescriptorSetCreateResult;
struct VulkanRuntimeDescriptorBufferResource;
struct VulkanRuntimeDescriptorTextureResource;
struct VulkanRuntimeDescriptorSamplerResource;
struct VulkanRuntimeDescriptorWriteDesc;
struct VulkanRuntimeDescriptorWriteResult;
struct VulkanRuntimeDescriptorSetBindDesc;
struct VulkanRuntimeDescriptorSetBindResult;
struct VulkanRuntimePipelineLayoutDesc;
struct VulkanRuntimePipelineLayoutCreateResult;
struct VulkanRuntimeGraphicsPipelineDesc;
struct VulkanRuntimeGraphicsPipelineCreateResult;
struct VulkanRuntimeComputePipelineDesc;
struct VulkanRuntimeComputePipelineCreateResult;
struct VulkanRuntimeComputePipelineBindResult;
struct VulkanRuntimeComputeDispatchDesc;
struct VulkanRuntimeComputeDispatchResult;
struct VulkanRuntimeSwapchainDesc;
struct VulkanRuntimeSwapchainCreateResult;
struct VulkanRuntimeFrameSyncDesc;
struct VulkanRuntimeFrameSyncCreateResult;
struct VulkanRuntimeSwapchainAcquireDesc;
struct VulkanRuntimeSwapchainAcquireResult;
struct VulkanRuntimeSwapchainPresentDesc;
struct VulkanRuntimeSwapchainPresentResult;
struct VulkanRuntimeDynamicRenderingClearDesc;
struct VulkanRuntimeDynamicRenderingClearResult;
struct VulkanRuntimeTextureRenderingClearDesc;
struct VulkanRuntimeDynamicRenderingDrawDesc;
struct VulkanRuntimeTextureRenderingDrawDesc;
struct VulkanRuntimeDynamicRenderingDrawResult;
struct VulkanRuntimeSwapchainFrameBarrierDesc;
struct VulkanRuntimeSwapchainFrameBarrierResult;
struct VulkanRuntimeCommandBufferSubmitDesc;
struct VulkanRuntimeCommandBufferSubmitResult;
struct VulkanRuntimeReadbackBufferDesc;
struct VulkanRuntimeReadbackBufferCreateResult;
struct VulkanRuntimeSwapchainReadbackDesc;
struct VulkanRuntimeSwapchainReadbackResult;
struct VulkanRuntimeBufferWriteDesc;
struct VulkanRuntimeBufferWriteResult;
struct VulkanRuntimeBufferReadDesc;
struct VulkanRuntimeBufferReadResult;
struct VulkanRuntimeReadbackBufferReadDesc;
struct VulkanRuntimeReadbackBufferReadResult;
struct VulkanRuntimeTextureBarrierDesc;
struct VulkanRuntimeTextureBarrierResult;
struct VulkanRuntimeBufferCopyDesc;
struct VulkanRuntimeBufferCopyResult;
struct VulkanRuntimeBufferTextureCopyDesc;
struct VulkanRuntimeBufferTextureCopyResult;
struct VulkanRuntimeTextureBufferCopyDesc;
struct VulkanRuntimeTextureBufferCopyResult;
class VulkanRuntimeCommandPool;
class VulkanRuntimeBuffer;
class VulkanRuntimeTexture;
class VulkanRuntimeSampler;
class VulkanRuntimeShaderModule;
class VulkanRuntimeDescriptorSetLayout;
class VulkanRuntimeDescriptorSet;
class VulkanRuntimePipelineLayout;
class VulkanRuntimeGraphicsPipeline;
class VulkanRuntimeComputePipeline;
class VulkanRuntimeSwapchain;
class VulkanRuntimeFrameSync;
class VulkanRuntimeReadbackBuffer;

class VulkanRuntimeInstance {
  public:
    VulkanRuntimeInstance() noexcept;
    ~VulkanRuntimeInstance();

    VulkanRuntimeInstance(VulkanRuntimeInstance&& other) noexcept;
    VulkanRuntimeInstance& operator=(VulkanRuntimeInstance&& other) noexcept;

    VulkanRuntimeInstance(const VulkanRuntimeInstance&) = delete;
    VulkanRuntimeInstance& operator=(const VulkanRuntimeInstance&) = delete;

    [[nodiscard]] bool owns_instance() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] const VulkanCommandResolutionPlan& command_plan() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeInstance(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeInstanceCreateResult create_runtime_instance(const VulkanLoaderProbeDesc& loader_desc,
                                                                     const VulkanInstanceCreateDesc& instance_desc);
};

struct VulkanRuntimeInstanceCreateResult {
    VulkanRuntimeInstance instance;
    VulkanRuntimeInstanceCommandProbeResult probe;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimePhysicalDeviceCountProbeResult {
    VulkanRuntimeInstanceCommandProbeResult instance;
    bool enumerated{false};
    std::uint32_t physical_device_count{0};
    std::string diagnostic;
};

struct VulkanRuntimePhysicalDeviceSnapshot {
    std::size_t device_index{0};
    std::string name;
    VulkanPhysicalDeviceType type{VulkanPhysicalDeviceType::other};
    VulkanApiVersion api_version{};
    std::uint32_t driver_version{0};
    std::uint32_t vendor_id{0};
    std::uint32_t device_id{0};
    std::vector<VulkanQueueFamilyCandidate> queue_families;
    std::vector<std::string> device_extensions;
    bool supports_swapchain_extension{false};
    bool supports_dynamic_rendering{false};
    bool supports_synchronization2{false};
};

struct VulkanRuntimePhysicalDeviceSnapshotProbeResult {
    VulkanRuntimePhysicalDeviceCountProbeResult count_probe;
    std::vector<VulkanRuntimePhysicalDeviceSnapshot> devices;
    bool enumerated{false};
    std::string diagnostic;
};

struct VulkanRuntimePhysicalDeviceSelectionProbeResult {
    VulkanRuntimePhysicalDeviceSnapshotProbeResult snapshots;
    std::vector<VulkanPhysicalDeviceCandidate> candidates;
    VulkanDeviceSelection selection;
    bool selected{false};
    std::string diagnostic;
};

class VulkanRuntimeDevice {
  public:
    VulkanRuntimeDevice() noexcept;
    ~VulkanRuntimeDevice();

    VulkanRuntimeDevice(VulkanRuntimeDevice&& other) noexcept;
    VulkanRuntimeDevice& operator=(VulkanRuntimeDevice&& other) noexcept;

    VulkanRuntimeDevice(const VulkanRuntimeDevice&) = delete;
    VulkanRuntimeDevice& operator=(const VulkanRuntimeDevice&) = delete;

    [[nodiscard]] bool owns_device() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] bool has_graphics_queue() const noexcept;
    [[nodiscard]] bool has_present_queue() const noexcept;
    [[nodiscard]] const VulkanLogicalDeviceCreatePlan& logical_device_plan() const noexcept;
    [[nodiscard]] const VulkanCommandResolutionPlan& command_plan() const noexcept;
    /// Blocks until the opaque device fence handle (`uint64_t`) is signaled, or the timeout elapses.
    [[nodiscard]] bool wait_for_fence_signaled(std::uint64_t fence, std::uint64_t timeout_ns) noexcept;
    void reset() noexcept;

    /// Records `vkCmdSetViewport` on the pool's primary command buffer while recording.
    [[nodiscard]] bool record_primary_viewport(VulkanRuntimeCommandPool& pool,
                                               const ::mirakana::rhi::ViewportDesc& viewport) const;
    /// Records `vkCmdSetScissor` on the pool's primary command buffer while recording.
    [[nodiscard]] bool record_primary_scissor(VulkanRuntimeCommandPool& pool,
                                              const ::mirakana::rhi::ScissorRectDesc& scissor) const;

  private:
    struct Impl;

    explicit VulkanRuntimeDevice(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend void vulkan_label_runtime_object(void* impl_opaque, std::int32_t object_type, std::uint64_t handle,
                                            const char* stable_prefix) noexcept;
    friend void vulkan_apply_debug_utils_names_for_device_and_queues(void* impl_opaque) noexcept;

    friend VulkanRuntimeDeviceCreateResult create_runtime_device(const VulkanLoaderProbeDesc& loader_desc,
                                                                 const VulkanInstanceCreateDesc& instance_desc,
                                                                 const VulkanLogicalDeviceCreateDesc& device_desc,
                                                                 SurfaceHandle surface);
    friend VulkanRuntimeCommandPoolCreateResult create_runtime_command_pool(VulkanRuntimeDevice& device,
                                                                            const VulkanRuntimeCommandPoolDesc& desc);
    friend VulkanRuntimeBufferCreateResult create_runtime_buffer(VulkanRuntimeDevice& device,
                                                                 const VulkanRuntimeBufferDesc& desc);
    friend VulkanRuntimeBufferWriteResult write_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                               const VulkanRuntimeBufferWriteDesc& desc);
    friend VulkanRuntimeBufferReadResult read_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                             const VulkanRuntimeBufferReadDesc& desc);
    friend VulkanRuntimeTextureCreateResult create_runtime_texture(VulkanRuntimeDevice& device,
                                                                   const VulkanRuntimeTextureDesc& desc);
    friend VulkanRuntimeSamplerCreateResult create_runtime_sampler(VulkanRuntimeDevice& device,
                                                                   const VulkanRuntimeSamplerDesc& desc);
    friend VulkanRuntimeShaderModuleCreateResult
    create_runtime_shader_module(VulkanRuntimeDevice& device, const VulkanRuntimeShaderModuleDesc& desc);
    friend VulkanRuntimeDescriptorSetLayoutCreateResult
    create_runtime_descriptor_set_layout(VulkanRuntimeDevice& device, const VulkanRuntimeDescriptorSetLayoutDesc& desc);
    friend VulkanRuntimeDescriptorSetCreateResult
    create_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSetLayout& layout,
                                  const VulkanRuntimeDescriptorSetDesc& desc);
    friend VulkanRuntimeDescriptorWriteResult
    update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                  const VulkanRuntimeDescriptorWriteDesc& desc);
    friend VulkanRuntimePipelineLayoutCreateResult
    create_runtime_pipeline_layout(VulkanRuntimeDevice& device, const VulkanRuntimePipelineLayoutDesc& desc);
    friend VulkanRuntimeGraphicsPipelineCreateResult create_runtime_graphics_pipeline(
        VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout, VulkanRuntimeShaderModule& vertex_shader,
        VulkanRuntimeShaderModule& fragment_shader, const VulkanRuntimeGraphicsPipelineDesc& desc);
    friend VulkanRuntimeComputePipelineCreateResult
    create_runtime_compute_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                    VulkanRuntimeShaderModule& compute_shader,
                                    const VulkanRuntimeComputePipelineDesc& desc);
    friend VulkanRuntimeComputePipelineBindResult
    record_runtime_compute_pipeline_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                            VulkanRuntimeComputePipeline& pipeline);
    friend VulkanRuntimeComputeDispatchResult
    record_runtime_compute_dispatch(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                    const VulkanRuntimeComputeDispatchDesc& desc);
    friend VulkanRuntimeSwapchainCreateResult create_runtime_swapchain(VulkanRuntimeDevice& device,
                                                                       const VulkanRuntimeSwapchainDesc& desc);
    friend VulkanRuntimeFrameSyncCreateResult create_runtime_frame_sync(VulkanRuntimeDevice& device,
                                                                        const VulkanRuntimeFrameSyncDesc& desc);
    friend VulkanRuntimeSwapchainAcquireResult
    acquire_next_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                         VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainAcquireDesc& desc);
    friend VulkanRuntimeSwapchainPresentResult
    present_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                    VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainPresentDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeTextureRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeDynamicRenderingClearDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_texture_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeTexture& texture,
                                           const VulkanRuntimeTextureRenderingClearDesc& desc);
    friend VulkanRuntimeSwapchainFrameBarrierResult
    record_runtime_swapchain_frame_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeSwapchainFrameBarrierDesc& desc);
    friend VulkanRuntimeCommandBufferSubmitResult
    submit_runtime_command_buffer(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                  VulkanRuntimeFrameSync& sync, const VulkanRuntimeCommandBufferSubmitDesc& desc);
    friend VulkanRuntimeReadbackBufferCreateResult
    create_runtime_readback_buffer(VulkanRuntimeDevice& device, const VulkanRuntimeReadbackBufferDesc& desc);
    friend VulkanRuntimeSwapchainReadbackResult record_runtime_swapchain_image_readback(
        VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimeSwapchain& swapchain,
        VulkanRuntimeReadbackBuffer& readback_buffer, const VulkanRuntimeSwapchainReadbackDesc& desc);
    friend VulkanRuntimeTextureBarrierResult
    record_runtime_texture_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeTexture& texture, const VulkanRuntimeTextureBarrierDesc& desc);
    friend VulkanRuntimeBufferCopyResult record_runtime_buffer_copy(VulkanRuntimeDevice& device,
                                                                    VulkanRuntimeCommandPool& command_pool,
                                                                    VulkanRuntimeBuffer& source,
                                                                    VulkanRuntimeBuffer& destination,
                                                                    const VulkanRuntimeBufferCopyDesc& desc);
    friend VulkanRuntimeBufferTextureCopyResult
    record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                       const VulkanRuntimeBufferTextureCopyDesc& desc);
    friend VulkanRuntimeTextureBufferCopyResult
    record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                       const VulkanRuntimeTextureBufferCopyDesc& desc);
    friend VulkanRuntimeReadbackBufferReadResult
    read_runtime_readback_buffer(VulkanRuntimeDevice& device, VulkanRuntimeReadbackBuffer& readback_buffer,
                                 const VulkanRuntimeReadbackBufferReadDesc& desc);
    friend VulkanRuntimeDescriptorSetBindResult
    record_runtime_descriptor_set_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimePipelineLayout& pipeline_layout,
                                          VulkanRuntimeDescriptorSet& descriptor_set,
                                          const VulkanRuntimeDescriptorSetBindDesc& desc);
    friend class VulkanRuntimeCommandPool;
    friend class VulkanRuntimeShaderModule;
    friend class VulkanRuntimeDescriptorSetLayout;
    friend class VulkanRuntimeDescriptorSet;
    friend class VulkanRuntimePipelineLayout;
    friend class VulkanRuntimeGraphicsPipeline;
    friend class VulkanRuntimeComputePipeline;
    friend class VulkanRuntimeSwapchain;
    friend class VulkanRuntimeFrameSync;
    friend class VulkanRuntimeBuffer;
    friend class VulkanRuntimeTexture;
    friend class VulkanRuntimeSampler;
    friend class VulkanRuntimeReadbackBuffer;
    friend class VulkanRhiDevice;
};

struct VulkanRuntimeDeviceCreateResult {
    VulkanRuntimeDevice device;
    VulkanRuntimePhysicalDeviceSelectionProbeResult selection_probe;
    VulkanLogicalDeviceCreatePlan logical_device_plan;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeCommandPoolDesc {
    std::uint32_t queue_family{invalid_vulkan_queue_family};
    bool reset_command_buffer{true};
    bool transient{false};
};

class VulkanRuntimeCommandPool {
  public:
    VulkanRuntimeCommandPool() noexcept;
    ~VulkanRuntimeCommandPool();

    VulkanRuntimeCommandPool(VulkanRuntimeCommandPool&& other) noexcept;
    VulkanRuntimeCommandPool& operator=(VulkanRuntimeCommandPool&& other) noexcept;

    VulkanRuntimeCommandPool(const VulkanRuntimeCommandPool&) = delete;
    VulkanRuntimeCommandPool& operator=(const VulkanRuntimeCommandPool&) = delete;

    [[nodiscard]] bool owns_pool() const noexcept;
    [[nodiscard]] bool owns_primary_command_buffer() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] bool recording() const noexcept;
    [[nodiscard]] bool ended() const noexcept;
    [[nodiscard]] bool begin_primary_command_buffer();
    [[nodiscard]] bool end_primary_command_buffer();
    void reset() noexcept;

    /// Primary command buffer handle while `recording()` is true; otherwise `nullptr`.
    [[nodiscard]] NativeVulkanCommandBuffer native_primary_command_buffer() const noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeCommandPool(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeCommandPoolCreateResult create_runtime_command_pool(VulkanRuntimeDevice& device,
                                                                            const VulkanRuntimeCommandPoolDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeTextureRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeDynamicRenderingClearDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_texture_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeTexture& texture,
                                           const VulkanRuntimeTextureRenderingClearDesc& desc);
    friend VulkanRuntimeSwapchainFrameBarrierResult
    record_runtime_swapchain_frame_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeSwapchainFrameBarrierDesc& desc);
    friend VulkanRuntimeCommandBufferSubmitResult
    submit_runtime_command_buffer(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                  VulkanRuntimeFrameSync& sync, const VulkanRuntimeCommandBufferSubmitDesc& desc);
    friend VulkanRuntimeSwapchainReadbackResult record_runtime_swapchain_image_readback(
        VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimeSwapchain& swapchain,
        VulkanRuntimeReadbackBuffer& readback_buffer, const VulkanRuntimeSwapchainReadbackDesc& desc);
    friend VulkanRuntimeTextureBarrierResult
    record_runtime_texture_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeTexture& texture, const VulkanRuntimeTextureBarrierDesc& desc);
    friend VulkanRuntimeBufferCopyResult record_runtime_buffer_copy(VulkanRuntimeDevice& device,
                                                                    VulkanRuntimeCommandPool& command_pool,
                                                                    VulkanRuntimeBuffer& source,
                                                                    VulkanRuntimeBuffer& destination,
                                                                    const VulkanRuntimeBufferCopyDesc& desc);
    friend VulkanRuntimeBufferTextureCopyResult
    record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                       const VulkanRuntimeBufferTextureCopyDesc& desc);
    friend VulkanRuntimeTextureBufferCopyResult
    record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                       const VulkanRuntimeTextureBufferCopyDesc& desc);
    friend VulkanRuntimeDescriptorSetBindResult
    record_runtime_descriptor_set_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimePipelineLayout& pipeline_layout,
                                          VulkanRuntimeDescriptorSet& descriptor_set,
                                          const VulkanRuntimeDescriptorSetBindDesc& desc);
    friend VulkanRuntimeComputePipelineBindResult
    record_runtime_compute_pipeline_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                            VulkanRuntimeComputePipeline& pipeline);
    friend VulkanRuntimeComputeDispatchResult
    record_runtime_compute_dispatch(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                    const VulkanRuntimeComputeDispatchDesc& desc);
};

struct VulkanRuntimeCommandPoolCreateResult {
    VulkanRuntimeCommandPool pool;
    bool created{false};
    std::string diagnostic;
};

class VulkanRuntimeBuffer {
  public:
    VulkanRuntimeBuffer() noexcept;
    ~VulkanRuntimeBuffer();

    VulkanRuntimeBuffer(VulkanRuntimeBuffer&& other) noexcept;
    VulkanRuntimeBuffer& operator=(VulkanRuntimeBuffer&& other) noexcept;

    VulkanRuntimeBuffer(const VulkanRuntimeBuffer&) = delete;
    VulkanRuntimeBuffer& operator=(const VulkanRuntimeBuffer&) = delete;

    [[nodiscard]] bool owns_buffer() const noexcept;
    [[nodiscard]] bool owns_memory() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] std::uint64_t byte_size() const noexcept;
    [[nodiscard]] BufferUsage usage() const noexcept;
    [[nodiscard]] VulkanBufferMemoryDomain memory_domain() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeBuffer(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeBufferCreateResult create_runtime_buffer(VulkanRuntimeDevice& device,
                                                                 const VulkanRuntimeBufferDesc& desc);
    friend VulkanRuntimeBufferWriteResult write_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                               const VulkanRuntimeBufferWriteDesc& desc);
    friend VulkanRuntimeBufferReadResult read_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                             const VulkanRuntimeBufferReadDesc& desc);
    friend VulkanRuntimeDescriptorWriteResult
    update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                  const VulkanRuntimeDescriptorWriteDesc& desc);
    friend VulkanRuntimeBufferCopyResult record_runtime_buffer_copy(VulkanRuntimeDevice& device,
                                                                    VulkanRuntimeCommandPool& command_pool,
                                                                    VulkanRuntimeBuffer& source,
                                                                    VulkanRuntimeBuffer& destination,
                                                                    const VulkanRuntimeBufferCopyDesc& desc);
    friend VulkanRuntimeBufferTextureCopyResult
    record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                       const VulkanRuntimeBufferTextureCopyDesc& desc);
    friend VulkanRuntimeTextureBufferCopyResult
    record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                       const VulkanRuntimeTextureBufferCopyDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeTextureRenderingDrawDesc& desc);
};

struct VulkanRuntimeBufferCreateResult {
    VulkanRuntimeBuffer buffer;
    VulkanRuntimeBufferCreatePlan plan;
    bool created{false};
    std::string diagnostic;
};

class VulkanRuntimeTexture {
  public:
    VulkanRuntimeTexture() noexcept;
    ~VulkanRuntimeTexture();

    VulkanRuntimeTexture(VulkanRuntimeTexture&& other) noexcept;
    VulkanRuntimeTexture& operator=(VulkanRuntimeTexture&& other) noexcept;

    VulkanRuntimeTexture(const VulkanRuntimeTexture&) = delete;
    VulkanRuntimeTexture& operator=(const VulkanRuntimeTexture&) = delete;

    [[nodiscard]] bool owns_image() const noexcept;
    [[nodiscard]] bool owns_memory() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] Extent3D extent() const noexcept;
    [[nodiscard]] Format format() const noexcept;
    [[nodiscard]] TextureUsage usage() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeTexture(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeTextureCreateResult create_runtime_texture(VulkanRuntimeDevice& device,
                                                                   const VulkanRuntimeTextureDesc& desc);
    friend VulkanRuntimeDescriptorWriteResult
    update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                  const VulkanRuntimeDescriptorWriteDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeDynamicRenderingClearDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_texture_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeTexture& texture,
                                           const VulkanRuntimeTextureRenderingClearDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeTextureRenderingDrawDesc& desc);
    friend VulkanRuntimeTextureBarrierResult
    record_runtime_texture_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeTexture& texture, const VulkanRuntimeTextureBarrierDesc& desc);
    friend VulkanRuntimeBufferTextureCopyResult
    record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                       const VulkanRuntimeBufferTextureCopyDesc& desc);
    friend VulkanRuntimeTextureBufferCopyResult
    record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                       const VulkanRuntimeTextureBufferCopyDesc& desc);
};

struct VulkanRuntimeTextureCreateResult {
    VulkanRuntimeTexture texture;
    VulkanRuntimeTextureCreatePlan plan;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeSamplerDesc {
    SamplerDesc sampler;
};

class VulkanRuntimeSampler {
  public:
    VulkanRuntimeSampler() noexcept;
    ~VulkanRuntimeSampler();

    VulkanRuntimeSampler(VulkanRuntimeSampler&& other) noexcept;
    VulkanRuntimeSampler& operator=(VulkanRuntimeSampler&& other) noexcept;

    VulkanRuntimeSampler(const VulkanRuntimeSampler&) = delete;
    VulkanRuntimeSampler& operator=(const VulkanRuntimeSampler&) = delete;

    [[nodiscard]] bool owns_sampler() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeSampler(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeSamplerCreateResult create_runtime_sampler(VulkanRuntimeDevice& device,
                                                                   const VulkanRuntimeSamplerDesc& desc);
    friend VulkanRuntimeDescriptorWriteResult
    update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                  const VulkanRuntimeDescriptorWriteDesc& desc);
};

struct VulkanRuntimeSamplerCreateResult {
    VulkanRuntimeSampler sampler;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeShaderModuleDesc {
    ShaderStage stage{ShaderStage::vertex};
    const void* bytecode{nullptr};
    std::uint64_t bytecode_size{0};
};

class VulkanRuntimeShaderModule {
  public:
    VulkanRuntimeShaderModule() noexcept;
    ~VulkanRuntimeShaderModule();

    VulkanRuntimeShaderModule(VulkanRuntimeShaderModule&& other) noexcept;
    VulkanRuntimeShaderModule& operator=(VulkanRuntimeShaderModule&& other) noexcept;

    VulkanRuntimeShaderModule(const VulkanRuntimeShaderModule&) = delete;
    VulkanRuntimeShaderModule& operator=(const VulkanRuntimeShaderModule&) = delete;

    [[nodiscard]] bool owns_module() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] ShaderStage stage() const noexcept;
    [[nodiscard]] std::uint64_t bytecode_size() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeShaderModule(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeShaderModuleCreateResult
    create_runtime_shader_module(VulkanRuntimeDevice& device, const VulkanRuntimeShaderModuleDesc& desc);
    friend VulkanRuntimeGraphicsPipelineCreateResult create_runtime_graphics_pipeline(
        VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout, VulkanRuntimeShaderModule& vertex_shader,
        VulkanRuntimeShaderModule& fragment_shader, const VulkanRuntimeGraphicsPipelineDesc& desc);
    friend VulkanRuntimeComputePipelineCreateResult
    create_runtime_compute_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                    VulkanRuntimeShaderModule& compute_shader,
                                    const VulkanRuntimeComputePipelineDesc& desc);
};

struct VulkanRuntimeShaderModuleCreateResult {
    VulkanRuntimeShaderModule module;
    VulkanSpirvShaderArtifactValidation validation;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeDescriptorSetLayoutDesc {
    DescriptorSetLayoutDesc layout;
};

class VulkanRuntimeDescriptorSetLayout {
  public:
    VulkanRuntimeDescriptorSetLayout() noexcept;
    ~VulkanRuntimeDescriptorSetLayout();

    VulkanRuntimeDescriptorSetLayout(VulkanRuntimeDescriptorSetLayout&& other) noexcept;
    VulkanRuntimeDescriptorSetLayout& operator=(VulkanRuntimeDescriptorSetLayout&& other) noexcept;

    VulkanRuntimeDescriptorSetLayout(const VulkanRuntimeDescriptorSetLayout&) = delete;
    VulkanRuntimeDescriptorSetLayout& operator=(const VulkanRuntimeDescriptorSetLayout&) = delete;

    [[nodiscard]] bool owns_layout() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] std::uint32_t binding_count() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeDescriptorSetLayout(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeDescriptorSetLayoutCreateResult
    create_runtime_descriptor_set_layout(VulkanRuntimeDevice& device, const VulkanRuntimeDescriptorSetLayoutDesc& desc);
    friend VulkanRuntimeDescriptorSetCreateResult
    create_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSetLayout& layout,
                                  const VulkanRuntimeDescriptorSetDesc& desc);
    friend VulkanRuntimePipelineLayoutCreateResult
    create_runtime_pipeline_layout(VulkanRuntimeDevice& device, const VulkanRuntimePipelineLayoutDesc& desc);
};

struct VulkanRuntimeDescriptorSetLayoutCreateResult {
    VulkanRuntimeDescriptorSetLayout layout;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeDescriptorSetDesc {
    std::uint32_t max_sets{1};
};

class VulkanRuntimeDescriptorSet {
  public:
    VulkanRuntimeDescriptorSet() noexcept;
    ~VulkanRuntimeDescriptorSet();

    VulkanRuntimeDescriptorSet(VulkanRuntimeDescriptorSet&& other) noexcept;
    VulkanRuntimeDescriptorSet& operator=(VulkanRuntimeDescriptorSet&& other) noexcept;

    VulkanRuntimeDescriptorSet(const VulkanRuntimeDescriptorSet&) = delete;
    VulkanRuntimeDescriptorSet& operator=(const VulkanRuntimeDescriptorSet&) = delete;

    [[nodiscard]] bool owns_pool() const noexcept;
    [[nodiscard]] bool owns_set() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] std::uint32_t binding_count() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeDescriptorSet(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeDescriptorSetCreateResult
    create_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSetLayout& layout,
                                  const VulkanRuntimeDescriptorSetDesc& desc);
    friend VulkanRuntimeDescriptorWriteResult
    update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                  const VulkanRuntimeDescriptorWriteDesc& desc);
    friend VulkanRuntimeDescriptorSetBindResult
    record_runtime_descriptor_set_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimePipelineLayout& pipeline_layout,
                                          VulkanRuntimeDescriptorSet& descriptor_set,
                                          const VulkanRuntimeDescriptorSetBindDesc& desc);
};

struct VulkanRuntimeDescriptorSetCreateResult {
    VulkanRuntimeDescriptorSet set;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimePipelineLayoutDesc {
    std::uint32_t push_constant_bytes{0};
    std::vector<VulkanRuntimeDescriptorSetLayout*> descriptor_set_layouts;
};

class VulkanRuntimePipelineLayout {
  public:
    VulkanRuntimePipelineLayout() noexcept;
    ~VulkanRuntimePipelineLayout();

    VulkanRuntimePipelineLayout(VulkanRuntimePipelineLayout&& other) noexcept;
    VulkanRuntimePipelineLayout& operator=(VulkanRuntimePipelineLayout&& other) noexcept;

    VulkanRuntimePipelineLayout(const VulkanRuntimePipelineLayout&) = delete;
    VulkanRuntimePipelineLayout& operator=(const VulkanRuntimePipelineLayout&) = delete;

    [[nodiscard]] bool owns_layout() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] std::uint32_t descriptor_set_layout_count() const noexcept;
    [[nodiscard]] std::uint32_t push_constant_bytes() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimePipelineLayout(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimePipelineLayoutCreateResult
    create_runtime_pipeline_layout(VulkanRuntimeDevice& device, const VulkanRuntimePipelineLayoutDesc& desc);
    friend VulkanRuntimeGraphicsPipelineCreateResult create_runtime_graphics_pipeline(
        VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout, VulkanRuntimeShaderModule& vertex_shader,
        VulkanRuntimeShaderModule& fragment_shader, const VulkanRuntimeGraphicsPipelineDesc& desc);
    friend VulkanRuntimeComputePipelineCreateResult
    create_runtime_compute_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                    VulkanRuntimeShaderModule& compute_shader,
                                    const VulkanRuntimeComputePipelineDesc& desc);
    friend VulkanRuntimeDescriptorSetBindResult
    record_runtime_descriptor_set_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimePipelineLayout& pipeline_layout,
                                          VulkanRuntimeDescriptorSet& descriptor_set,
                                          const VulkanRuntimeDescriptorSetBindDesc& desc);
};

struct VulkanRuntimePipelineLayoutCreateResult {
    VulkanRuntimePipelineLayout layout;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeGraphicsPipelineDesc {
    VulkanDynamicRenderingPlan dynamic_rendering;
    Format color_format{Format::unknown};
    Format depth_format{Format::unknown};
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    std::string_view vertex_entry_point{"main"};
    std::string_view fragment_entry_point{"main"};
    std::vector<VertexBufferLayoutDesc> vertex_buffers;
    std::vector<VertexAttributeDesc> vertex_attributes;
    DepthStencilStateDesc depth_state;
};

class VulkanRuntimeGraphicsPipeline {
  public:
    VulkanRuntimeGraphicsPipeline() noexcept;
    ~VulkanRuntimeGraphicsPipeline();

    VulkanRuntimeGraphicsPipeline(VulkanRuntimeGraphicsPipeline&& other) noexcept;
    VulkanRuntimeGraphicsPipeline& operator=(VulkanRuntimeGraphicsPipeline&& other) noexcept;

    VulkanRuntimeGraphicsPipeline(const VulkanRuntimeGraphicsPipeline&) = delete;
    VulkanRuntimeGraphicsPipeline& operator=(const VulkanRuntimeGraphicsPipeline&) = delete;

    [[nodiscard]] bool owns_pipeline() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] Format color_format() const noexcept;
    [[nodiscard]] Format depth_format() const noexcept;
    [[nodiscard]] PrimitiveTopology topology() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeGraphicsPipeline(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeGraphicsPipelineCreateResult create_runtime_graphics_pipeline(
        VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout, VulkanRuntimeShaderModule& vertex_shader,
        VulkanRuntimeShaderModule& fragment_shader, const VulkanRuntimeGraphicsPipelineDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeTextureRenderingDrawDesc& desc);
};

struct VulkanRuntimeGraphicsPipelineCreateResult {
    VulkanRuntimeGraphicsPipeline pipeline;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeComputePipelineDesc {
    std::string_view entry_point{"main"};
};

class VulkanRuntimeComputePipeline {
  public:
    VulkanRuntimeComputePipeline() noexcept;
    ~VulkanRuntimeComputePipeline();

    VulkanRuntimeComputePipeline(VulkanRuntimeComputePipeline&& other) noexcept;
    VulkanRuntimeComputePipeline& operator=(VulkanRuntimeComputePipeline&& other) noexcept;

    VulkanRuntimeComputePipeline(const VulkanRuntimeComputePipeline&) = delete;
    VulkanRuntimeComputePipeline& operator=(const VulkanRuntimeComputePipeline&) = delete;

    [[nodiscard]] bool owns_pipeline() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeComputePipeline(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeComputePipelineCreateResult
    create_runtime_compute_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                    VulkanRuntimeShaderModule& compute_shader,
                                    const VulkanRuntimeComputePipelineDesc& desc);
    friend VulkanRuntimeComputePipelineBindResult
    record_runtime_compute_pipeline_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                            VulkanRuntimeComputePipeline& pipeline);
};

struct VulkanRuntimeComputePipelineCreateResult {
    VulkanRuntimeComputePipeline pipeline;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeSwapchainDesc {
    SurfaceHandle surface;
    VulkanSwapchainCreatePlan plan;
};

class VulkanRuntimeSwapchain {
  public:
    VulkanRuntimeSwapchain() noexcept;
    ~VulkanRuntimeSwapchain();

    VulkanRuntimeSwapchain(VulkanRuntimeSwapchain&& other) noexcept;
    VulkanRuntimeSwapchain& operator=(VulkanRuntimeSwapchain&& other) noexcept;

    VulkanRuntimeSwapchain(const VulkanRuntimeSwapchain&) = delete;
    VulkanRuntimeSwapchain& operator=(const VulkanRuntimeSwapchain&) = delete;

    [[nodiscard]] bool owns_surface() const noexcept;
    [[nodiscard]] bool owns_swapchain() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] Extent2D extent() const noexcept;
    [[nodiscard]] Format format() const noexcept;
    [[nodiscard]] std::uint32_t image_count() const noexcept;
    [[nodiscard]] std::uint32_t image_view_count() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeSwapchain(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeSwapchainCreateResult create_runtime_swapchain(VulkanRuntimeDevice& device,
                                                                       const VulkanRuntimeSwapchainDesc& desc);
    friend VulkanRuntimeSwapchainAcquireResult
    acquire_next_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                         VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainAcquireDesc& desc);
    friend VulkanRuntimeSwapchainPresentResult
    present_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                    VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainPresentDesc& desc);
    friend VulkanRuntimeDynamicRenderingDrawResult
    record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                          VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                          const VulkanRuntimeDynamicRenderingDrawDesc& desc);
    friend VulkanRuntimeDynamicRenderingClearResult
    record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeDynamicRenderingClearDesc& desc);
    friend VulkanRuntimeSwapchainFrameBarrierResult
    record_runtime_swapchain_frame_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                           VulkanRuntimeSwapchain& swapchain,
                                           const VulkanRuntimeSwapchainFrameBarrierDesc& desc);
    friend VulkanRuntimeSwapchainReadbackResult record_runtime_swapchain_image_readback(
        VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimeSwapchain& swapchain,
        VulkanRuntimeReadbackBuffer& readback_buffer, const VulkanRuntimeSwapchainReadbackDesc& desc);
};

struct VulkanRuntimeSwapchainCreateResult {
    VulkanRuntimeSwapchain swapchain;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeFrameSyncDesc {
    bool create_image_available_semaphore{true};
    bool create_render_finished_semaphore{true};
    bool create_in_flight_fence{true};
    bool start_in_flight_fence_signaled{true};
};

class VulkanRuntimeFrameSync {
  public:
    VulkanRuntimeFrameSync() noexcept;
    ~VulkanRuntimeFrameSync();

    VulkanRuntimeFrameSync(VulkanRuntimeFrameSync&& other) noexcept;
    VulkanRuntimeFrameSync& operator=(VulkanRuntimeFrameSync&& other) noexcept;

    VulkanRuntimeFrameSync(const VulkanRuntimeFrameSync&) = delete;
    VulkanRuntimeFrameSync& operator=(const VulkanRuntimeFrameSync&) = delete;

    [[nodiscard]] bool owns_image_available_semaphore() const noexcept;
    [[nodiscard]] bool owns_render_finished_semaphore() const noexcept;
    [[nodiscard]] bool owns_in_flight_fence() const noexcept;
    /// Returns the opaque in-flight fence handle for this sync object, or `0` when none exists.
    [[nodiscard]] std::uint64_t native_in_flight_fence_handle() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeFrameSync(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeFrameSyncCreateResult create_runtime_frame_sync(VulkanRuntimeDevice& device,
                                                                        const VulkanRuntimeFrameSyncDesc& desc);
    friend VulkanRuntimeSwapchainAcquireResult
    acquire_next_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                         VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainAcquireDesc& desc);
    friend VulkanRuntimeSwapchainPresentResult
    present_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                    VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainPresentDesc& desc);
    friend VulkanRuntimeCommandBufferSubmitResult
    submit_runtime_command_buffer(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                  VulkanRuntimeFrameSync& sync, const VulkanRuntimeCommandBufferSubmitDesc& desc);
};

struct VulkanRuntimeReadbackBufferDesc {
    std::uint64_t byte_size{0};
};

class VulkanRuntimeReadbackBuffer {
  public:
    VulkanRuntimeReadbackBuffer() noexcept;
    ~VulkanRuntimeReadbackBuffer();

    VulkanRuntimeReadbackBuffer(VulkanRuntimeReadbackBuffer&& other) noexcept;
    VulkanRuntimeReadbackBuffer& operator=(VulkanRuntimeReadbackBuffer&& other) noexcept;

    VulkanRuntimeReadbackBuffer(const VulkanRuntimeReadbackBuffer&) = delete;
    VulkanRuntimeReadbackBuffer& operator=(const VulkanRuntimeReadbackBuffer&) = delete;

    [[nodiscard]] bool owns_buffer() const noexcept;
    [[nodiscard]] bool owns_memory() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    [[nodiscard]] std::uint64_t byte_size() const noexcept;
    void reset() noexcept;

  private:
    struct Impl;

    explicit VulkanRuntimeReadbackBuffer(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;

    friend VulkanRuntimeReadbackBufferCreateResult
    create_runtime_readback_buffer(VulkanRuntimeDevice& device, const VulkanRuntimeReadbackBufferDesc& desc);
    friend VulkanRuntimeSwapchainReadbackResult record_runtime_swapchain_image_readback(
        VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimeSwapchain& swapchain,
        VulkanRuntimeReadbackBuffer& readback_buffer, const VulkanRuntimeSwapchainReadbackDesc& desc);
    friend VulkanRuntimeReadbackBufferReadResult
    read_runtime_readback_buffer(VulkanRuntimeDevice& device, VulkanRuntimeReadbackBuffer& readback_buffer,
                                 const VulkanRuntimeReadbackBufferReadDesc& desc);
};

struct VulkanRuntimeReadbackBufferCreateResult {
    VulkanRuntimeReadbackBuffer buffer;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeFrameSyncCreateResult {
    VulkanRuntimeFrameSync sync;
    bool created{false};
    std::string diagnostic;
};

struct VulkanRuntimeSwapchainAcquireDesc {
    std::uint64_t timeout_ns{0xffffffffffffffffULL};
    bool signal_image_available_semaphore{true};
    bool signal_in_flight_fence{false};
};

struct VulkanRuntimeSwapchainAcquireResult {
    bool acquired{false};
    bool suboptimal{false};
    bool timeout{false};
    bool not_ready{false};
    bool resize_required{false};
    std::uint32_t image_index{0};
    std::string diagnostic;
};

struct VulkanRuntimeSwapchainPresentDesc {
    std::uint32_t image_index{0};
    bool wait_render_finished_semaphore{true};
};

struct VulkanRuntimeSwapchainPresentResult {
    bool presented{false};
    bool suboptimal{false};
    bool resize_required{false};
    std::string diagnostic;
};

struct VulkanRuntimeDynamicRenderingDrawDesc {
    VulkanDynamicRenderingPlan dynamic_rendering;
    std::uint32_t image_index{0};
    std::uint32_t vertex_count{3};
    std::uint32_t instance_count{1};
    std::uint32_t first_vertex{0};
    std::uint32_t first_instance{0};
    VulkanRuntimeBuffer* vertex_buffer{nullptr};
    std::uint64_t vertex_buffer_offset{0};
    std::uint32_t vertex_buffer_binding{0};
    VulkanRuntimeBuffer* index_buffer{nullptr};
    std::uint64_t index_buffer_offset{0};
    IndexFormat index_format{IndexFormat::unknown};
    std::uint32_t index_count{0};
    LoadAction color_load_action{LoadAction::clear};
    StoreAction color_store_action{StoreAction::store};
    ClearColorValue clear_color;
    VulkanRuntimeTexture* depth_texture{nullptr};
    LoadAction depth_load_action{LoadAction::clear};
    StoreAction depth_store_action{StoreAction::store};
    ClearDepthValue clear_depth;
};

struct VulkanRuntimeDynamicRenderingDrawResult {
    bool recorded{false};
    bool began_rendering{false};
    bool bound_pipeline{false};
    bool drew{false};
    bool ended_rendering{false};
    std::string diagnostic;
};

struct VulkanRuntimeDynamicRenderingClearDesc {
    VulkanDynamicRenderingPlan dynamic_rendering;
    std::uint32_t image_index{0};
    StoreAction color_store_action{StoreAction::store};
    ClearColorValue clear_color;
    VulkanRuntimeTexture* depth_texture{nullptr};
    LoadAction depth_load_action{LoadAction::clear};
    StoreAction depth_store_action{StoreAction::store};
    ClearDepthValue clear_depth;
};

struct VulkanRuntimeDynamicRenderingClearResult {
    bool recorded{false};
    bool began_rendering{false};
    bool ended_rendering{false};
    std::string diagnostic;
};

struct VulkanRuntimeTextureRenderingClearDesc {
    VulkanDynamicRenderingPlan dynamic_rendering;
    StoreAction color_store_action{StoreAction::store};
    ClearColorValue clear_color;
    VulkanRuntimeTexture* depth_texture{nullptr};
    LoadAction depth_load_action{LoadAction::clear};
    StoreAction depth_store_action{StoreAction::store};
    ClearDepthValue clear_depth;
};

struct VulkanRuntimeTextureRenderingDrawDesc {
    VulkanDynamicRenderingPlan dynamic_rendering;
    std::uint32_t vertex_count{3};
    std::uint32_t instance_count{1};
    std::uint32_t first_vertex{0};
    std::uint32_t first_instance{0};
    VulkanRuntimeBuffer* vertex_buffer{nullptr};
    std::uint64_t vertex_buffer_offset{0};
    std::uint32_t vertex_buffer_binding{0};
    VulkanRuntimeBuffer* index_buffer{nullptr};
    std::uint64_t index_buffer_offset{0};
    IndexFormat index_format{IndexFormat::unknown};
    std::uint32_t index_count{0};
    LoadAction color_load_action{LoadAction::clear};
    StoreAction color_store_action{StoreAction::store};
    ClearColorValue clear_color;
    VulkanRuntimeTexture* depth_texture{nullptr};
    LoadAction depth_load_action{LoadAction::clear};
    StoreAction depth_store_action{StoreAction::store};
    ClearDepthValue clear_depth;
};

struct VulkanRuntimeSwapchainFrameBarrierDesc {
    VulkanFrameSynchronizationBarrier barrier;
    std::uint32_t image_index{0};
};

struct VulkanRuntimeSwapchainFrameBarrierResult {
    bool recorded{false};
    std::uint32_t barrier_count{0};
    std::string diagnostic;
};

struct VulkanRuntimeCommandBufferSubmitDesc {
    bool wait_image_available_semaphore{true};
    bool signal_render_finished_semaphore{true};
    bool signal_in_flight_fence{false};
    bool wait_for_graphics_queue_idle{false};
};

struct VulkanRuntimeCommandBufferSubmitResult {
    bool submitted{false};
    bool graphics_queue_idle_waited{false};
    std::string diagnostic;
};

struct VulkanRuntimeSwapchainReadbackDesc {
    std::uint32_t image_index{0};
    Extent2D extent;
    std::uint32_t bytes_per_pixel{4};
};

struct VulkanRuntimeSwapchainReadbackResult {
    bool recorded{false};
    std::uint64_t required_bytes{0};
    std::string diagnostic;
};

struct VulkanRuntimeBufferWriteDesc {
    std::uint64_t byte_offset{0};
    std::span<const std::uint8_t> bytes;
};

struct VulkanRuntimeBufferWriteResult {
    bool written{false};
    std::string diagnostic;
};

struct VulkanRuntimeBufferReadDesc {
    std::uint64_t byte_offset{0};
    std::uint64_t byte_count{0};
};

struct VulkanRuntimeBufferReadResult {
    bool read{false};
    std::vector<std::byte> bytes;
    std::string diagnostic;
};

struct VulkanRuntimeReadbackBufferReadDesc {
    std::uint64_t byte_offset{0};
    std::uint64_t byte_count{0};
};

struct VulkanRuntimeReadbackBufferReadResult {
    bool read{false};
    std::vector<std::byte> bytes;
    std::string diagnostic;
};

struct VulkanRuntimeTextureBarrierDesc {
    ResourceState before{ResourceState::undefined};
    ResourceState after{ResourceState::undefined};
};

struct VulkanRuntimeTextureBarrierResult {
    bool recorded{false};
    std::uint32_t barrier_count{0};
    std::string diagnostic;
};

struct VulkanRuntimeBufferCopyDesc {
    BufferCopyRegion region;
};

struct VulkanRuntimeBufferCopyResult {
    bool recorded{false};
    std::uint64_t bytes_copied{0};
    std::string diagnostic;
};

struct VulkanRuntimeBufferTextureCopyDesc {
    BufferTextureCopyRegion region;
};

struct VulkanRuntimeBufferTextureCopyResult {
    bool recorded{false};
    std::uint64_t required_bytes{0};
    std::string diagnostic;
};

struct VulkanRuntimeTextureBufferCopyDesc {
    BufferTextureCopyRegion region;
};

struct VulkanRuntimeTextureBufferCopyResult {
    bool recorded{false};
    std::uint64_t required_bytes{0};
    std::string diagnostic;
};

struct VulkanRuntimeDescriptorBufferResource {
    DescriptorType type{DescriptorType::uniform_buffer};
    VulkanRuntimeBuffer* buffer{nullptr};
};

struct VulkanRuntimeDescriptorTextureResource {
    DescriptorType type{DescriptorType::sampled_texture};
    VulkanRuntimeTexture* texture{nullptr};
};

struct VulkanRuntimeDescriptorSamplerResource {
    VulkanRuntimeSampler* sampler{nullptr};
};

struct VulkanRuntimeDescriptorWriteDesc {
    std::uint32_t binding{0};
    std::uint32_t array_element{0};
    std::vector<VulkanRuntimeDescriptorBufferResource> buffers;
    std::vector<VulkanRuntimeDescriptorTextureResource> textures;
    std::vector<VulkanRuntimeDescriptorSamplerResource> samplers;
};

struct VulkanRuntimeDescriptorWriteResult {
    bool updated{false};
    std::uint32_t descriptor_count{0};
    std::string diagnostic;
};

struct VulkanRuntimeDescriptorSetBindDesc {
    std::uint32_t first_set{0};
    std::uint32_t pipeline_bind_point{0};
};

struct VulkanRuntimeDescriptorSetBindResult {
    bool recorded{false};
    std::string diagnostic;
};

struct VulkanRuntimeComputePipelineBindResult {
    bool recorded{false};
    std::string diagnostic;
};

struct VulkanRuntimeComputeDispatchDesc {
    std::uint32_t group_count_x{1};
    std::uint32_t group_count_y{1};
    std::uint32_t group_count_z{1};
};

struct VulkanRuntimeComputeDispatchResult {
    bool recorded{false};
    std::string diagnostic;
};

struct VulkanRuntimeSurfaceSupportProbeResult {
    VulkanRuntimePhysicalDeviceSnapshotProbeResult snapshots;
    bool surface_created{false};
    bool surface_destroyed{false};
    bool probed{false};
    std::string diagnostic;
};

[[nodiscard]] BackendKind backend_kind() noexcept;
[[nodiscard]] std::string_view backend_name() noexcept;
[[nodiscard]] BackendCapabilityProfile capabilities() noexcept;
[[nodiscard]] BackendProbePlan probe_plan() noexcept;
[[nodiscard]] bool supports_host(RhiHostPlatform host) noexcept;
[[nodiscard]] BackendProbeResult make_probe_result(RhiHostPlatform host, BackendProbeStatus status,
                                                   std::string_view diagnostic = {});
[[nodiscard]] VulkanApiVersion make_vulkan_api_version(std::uint32_t major, std::uint32_t minor) noexcept;
[[nodiscard]] std::uint32_t encode_vulkan_api_version(VulkanApiVersion version, std::uint32_t patch = 0) noexcept;
[[nodiscard]] VulkanApiVersion decode_vulkan_api_version(std::uint32_t encoded) noexcept;
[[nodiscard]] bool is_vulkan_api_at_least(VulkanApiVersion actual, VulkanApiVersion required) noexcept;
[[nodiscard]] VulkanDeviceSelection
select_physical_device(std::initializer_list<VulkanPhysicalDeviceCandidate> devices);
[[nodiscard]] VulkanDeviceSelection select_physical_device(const std::vector<VulkanPhysicalDeviceCandidate>& devices);
[[nodiscard]] VulkanInstanceCreatePlan
build_instance_create_plan(const VulkanInstanceCreateDesc& desc,
                           std::initializer_list<std::string_view> available_extensions);
[[nodiscard]] VulkanInstanceCreatePlan build_instance_create_plan(const VulkanInstanceCreateDesc& desc,
                                                                  const std::vector<std::string>& available_extensions);
[[nodiscard]] VulkanLogicalDeviceCreatePlan
build_logical_device_create_plan(const VulkanLogicalDeviceCreateDesc& desc, const VulkanPhysicalDeviceCandidate& device,
                                 const VulkanDeviceSelection& selection,
                                 std::initializer_list<std::string_view> available_device_extensions);
[[nodiscard]] VulkanLogicalDeviceCreatePlan
build_logical_device_create_plan(const VulkanLogicalDeviceCreateDesc& desc, const VulkanPhysicalDeviceCandidate& device,
                                 const VulkanDeviceSelection& selection,
                                 const std::vector<std::string>& available_device_extensions);
[[nodiscard]] VulkanSwapchainCreatePlan build_swapchain_create_plan(const VulkanSwapchainCreateDesc& desc,
                                                                    const VulkanSwapchainSupport& support);
[[nodiscard]] VulkanSwapchainResizePlan build_swapchain_resize_plan(const VulkanSwapchainCreatePlan& current_plan,
                                                                    Extent2D requested_extent);
[[nodiscard]] VulkanDynamicRenderingPlan build_dynamic_rendering_plan(const VulkanDynamicRenderingDesc& desc,
                                                                      const VulkanCommandResolutionPlan& command_plan);
[[nodiscard]] VulkanFrameSynchronizationPlan
build_frame_synchronization_plan(const VulkanFrameSynchronizationDesc& desc,
                                 const VulkanCommandResolutionPlan& command_plan);
[[nodiscard]] VulkanTextureTransitionBarrierPlan build_texture_transition_barrier(ResourceState before,
                                                                                  ResourceState after);
[[nodiscard]] VulkanRhiDeviceMappingPlan build_rhi_device_mapping_plan(const VulkanRhiDeviceMappingDesc& desc);
/// Flags required for `create_rhi_device` wrapping a headless `VulkanRuntimeDevice` without swapchain presentation.
/// Matches the contract exercised by `tests/unit/backend_scaffold_tests.cpp` (`ready_vulkan_rhi_mapping_plan`).
[[nodiscard]] VulkanRhiDeviceMappingPlan minimal_irhi_device_mapping_plan();
[[nodiscard]] VulkanRuntimeBufferCreatePlan build_runtime_buffer_create_plan(const VulkanRuntimeBufferDesc& desc);
[[nodiscard]] VulkanRuntimeTextureCreatePlan build_runtime_texture_create_plan(const VulkanRuntimeTextureDesc& desc);
[[nodiscard]] VulkanSpirvShaderArtifactValidation
validate_spirv_shader_artifact(const VulkanSpirvShaderArtifactDesc& desc);
[[nodiscard]] VulkanPhysicalDeviceCandidate
make_physical_device_candidate(const VulkanRuntimePhysicalDeviceSnapshot& snapshot);
[[nodiscard]] std::vector<VulkanCommandRequest> vulkan_backend_command_requests();
[[nodiscard]] VulkanCommandResolutionPlan
build_command_resolution_plan(const std::vector<VulkanCommandRequest>& requests,
                              const std::vector<VulkanCommandAvailability>& available_commands);
[[nodiscard]] std::vector<VulkanCommandRequest>
vulkan_device_command_requests(const VulkanLogicalDeviceCreatePlan& plan);
[[nodiscard]] std::string_view default_runtime_library_name(RhiHostPlatform host) noexcept;
[[nodiscard]] VulkanLoaderProbeResult make_loader_probe_result(const VulkanLoaderProbeDesc& desc, bool runtime_loaded,
                                                               bool get_instance_proc_addr_found);
[[nodiscard]] VulkanLoaderProbeResult probe_runtime_loader(const VulkanLoaderProbeDesc& desc = {});
[[nodiscard]] VulkanRuntimeGlobalCommandProbeResult
probe_runtime_global_commands(const VulkanLoaderProbeDesc& desc = {});
[[nodiscard]] VulkanRuntimeInstanceCapabilityProbeResult
probe_runtime_instance_capabilities(const VulkanLoaderProbeDesc& loader_desc = {},
                                    const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimeInstanceCommandProbeResult
probe_runtime_instance_commands(const VulkanLoaderProbeDesc& loader_desc = {},
                                const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimeInstanceCreateResult
create_runtime_instance(const VulkanLoaderProbeDesc& loader_desc = {},
                        const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimePhysicalDeviceCountProbeResult
probe_runtime_physical_device_count(const VulkanLoaderProbeDesc& loader_desc = {},
                                    const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimePhysicalDeviceSnapshotProbeResult
probe_runtime_physical_device_snapshots(const VulkanLoaderProbeDesc& loader_desc = {},
                                        const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimePhysicalDeviceSelectionProbeResult
probe_runtime_physical_device_selection(const VulkanLoaderProbeDesc& loader_desc = {},
                                        const VulkanInstanceCreateDesc& instance_desc = {});
[[nodiscard]] VulkanRuntimeDeviceCreateResult
create_runtime_device(const VulkanLoaderProbeDesc& loader_desc = {}, const VulkanInstanceCreateDesc& instance_desc = {},
                      const VulkanLogicalDeviceCreateDesc& device_desc = {}, SurfaceHandle surface = {});
[[nodiscard]] std::unique_ptr<IRhiDevice> create_rhi_device(VulkanRuntimeDevice device,
                                                            const VulkanRhiDeviceMappingPlan& mapping_plan);
[[nodiscard]] VulkanRuntimeCommandPoolCreateResult
create_runtime_command_pool(VulkanRuntimeDevice& device, const VulkanRuntimeCommandPoolDesc& desc = {});
[[nodiscard]] VulkanRuntimeBufferCreateResult create_runtime_buffer(VulkanRuntimeDevice& device,
                                                                    const VulkanRuntimeBufferDesc& desc);
[[nodiscard]] VulkanRuntimeTextureCreateResult create_runtime_texture(VulkanRuntimeDevice& device,
                                                                      const VulkanRuntimeTextureDesc& desc);
[[nodiscard]] VulkanRuntimeSamplerCreateResult create_runtime_sampler(VulkanRuntimeDevice& device,
                                                                      const VulkanRuntimeSamplerDesc& desc);
[[nodiscard]] VulkanRuntimeShaderModuleCreateResult
create_runtime_shader_module(VulkanRuntimeDevice& device, const VulkanRuntimeShaderModuleDesc& desc);
[[nodiscard]] VulkanRuntimeDescriptorSetLayoutCreateResult
create_runtime_descriptor_set_layout(VulkanRuntimeDevice& device, const VulkanRuntimeDescriptorSetLayoutDesc& desc);
[[nodiscard]] VulkanRuntimeDescriptorSetCreateResult
create_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSetLayout& layout,
                              const VulkanRuntimeDescriptorSetDesc& desc = {});
[[nodiscard]] VulkanRuntimePipelineLayoutCreateResult
create_runtime_pipeline_layout(VulkanRuntimeDevice& device, const VulkanRuntimePipelineLayoutDesc& desc = {});
[[nodiscard]] VulkanRuntimeGraphicsPipelineCreateResult
create_runtime_graphics_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                 VulkanRuntimeShaderModule& vertex_shader, VulkanRuntimeShaderModule& fragment_shader,
                                 const VulkanRuntimeGraphicsPipelineDesc& desc);
[[nodiscard]] VulkanRuntimeComputePipelineCreateResult
create_runtime_compute_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                VulkanRuntimeShaderModule& compute_shader,
                                const VulkanRuntimeComputePipelineDesc& desc = {});
[[nodiscard]] VulkanRuntimeSwapchainCreateResult create_runtime_swapchain(VulkanRuntimeDevice& device,
                                                                          const VulkanRuntimeSwapchainDesc& desc);
[[nodiscard]] VulkanRuntimeFrameSyncCreateResult create_runtime_frame_sync(VulkanRuntimeDevice& device,
                                                                           const VulkanRuntimeFrameSyncDesc& desc = {});
[[nodiscard]] VulkanRuntimeSwapchainAcquireResult
acquire_next_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                     VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainAcquireDesc& desc = {});
[[nodiscard]] VulkanRuntimeSwapchainPresentResult
present_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainPresentDesc& desc = {});
[[nodiscard]] VulkanRuntimeDynamicRenderingDrawResult
record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                      VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                      const VulkanRuntimeDynamicRenderingDrawDesc& desc);
[[nodiscard]] VulkanRuntimeDynamicRenderingDrawResult
record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                      VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                      const VulkanRuntimeTextureRenderingDrawDesc& desc);
[[nodiscard]] VulkanRuntimeDynamicRenderingClearResult
record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeSwapchain& swapchain,
                                       const VulkanRuntimeDynamicRenderingClearDesc& desc);
[[nodiscard]] VulkanRuntimeDynamicRenderingClearResult
record_runtime_texture_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& texture,
                                       const VulkanRuntimeTextureRenderingClearDesc& desc);
[[nodiscard]] VulkanRuntimeSwapchainFrameBarrierResult
record_runtime_swapchain_frame_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeSwapchain& swapchain,
                                       const VulkanRuntimeSwapchainFrameBarrierDesc& desc);
[[nodiscard]] VulkanRuntimeCommandBufferSubmitResult
submit_runtime_command_buffer(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                              VulkanRuntimeFrameSync& sync, const VulkanRuntimeCommandBufferSubmitDesc& desc = {});
[[nodiscard]] VulkanRuntimeReadbackBufferCreateResult
create_runtime_readback_buffer(VulkanRuntimeDevice& device, const VulkanRuntimeReadbackBufferDesc& desc);
[[nodiscard]] VulkanRuntimeSwapchainReadbackResult
record_runtime_swapchain_image_readback(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                        VulkanRuntimeSwapchain& swapchain, VulkanRuntimeReadbackBuffer& readback_buffer,
                                        const VulkanRuntimeSwapchainReadbackDesc& desc);
[[nodiscard]] VulkanRuntimeBufferWriteResult write_runtime_buffer(VulkanRuntimeDevice& device,
                                                                  VulkanRuntimeBuffer& buffer,
                                                                  const VulkanRuntimeBufferWriteDesc& desc);
[[nodiscard]] VulkanRuntimeBufferReadResult read_runtime_buffer(VulkanRuntimeDevice& device,
                                                                VulkanRuntimeBuffer& buffer,
                                                                const VulkanRuntimeBufferReadDesc& desc = {});
[[nodiscard]] VulkanRuntimeReadbackBufferReadResult
read_runtime_readback_buffer(VulkanRuntimeDevice& device, VulkanRuntimeReadbackBuffer& readback_buffer,
                             const VulkanRuntimeReadbackBufferReadDesc& desc = {});
[[nodiscard]] VulkanRuntimeTextureBarrierResult
record_runtime_texture_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                               VulkanRuntimeTexture& texture, const VulkanRuntimeTextureBarrierDesc& desc);
[[nodiscard]] VulkanRuntimeBufferCopyResult record_runtime_buffer_copy(VulkanRuntimeDevice& device,
                                                                       VulkanRuntimeCommandPool& command_pool,
                                                                       VulkanRuntimeBuffer& source,
                                                                       VulkanRuntimeBuffer& destination,
                                                                       const VulkanRuntimeBufferCopyDesc& desc);
[[nodiscard]] VulkanRuntimeBufferTextureCopyResult
record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                   const VulkanRuntimeBufferTextureCopyDesc& desc);
[[nodiscard]] VulkanRuntimeTextureBufferCopyResult
record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                   const VulkanRuntimeTextureBufferCopyDesc& desc);
[[nodiscard]] VulkanRuntimeDescriptorWriteResult
update_runtime_descriptor_set(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                              const VulkanRuntimeDescriptorWriteDesc& desc);
[[nodiscard]] VulkanRuntimeDescriptorSetBindResult record_runtime_descriptor_set_binding(
    VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimePipelineLayout& pipeline_layout,
    VulkanRuntimeDescriptorSet& descriptor_set, const VulkanRuntimeDescriptorSetBindDesc& desc = {});
[[nodiscard]] VulkanRuntimeComputePipelineBindResult
record_runtime_compute_pipeline_binding(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                        VulkanRuntimeComputePipeline& pipeline);
[[nodiscard]] VulkanRuntimeComputeDispatchResult
record_runtime_compute_dispatch(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                const VulkanRuntimeComputeDispatchDesc& desc);
[[nodiscard]] std::vector<std::string> vulkan_surface_instance_extensions(RhiHostPlatform host);
[[nodiscard]] VulkanRuntimeSurfaceSupportProbeResult
probe_runtime_surface_support(const VulkanLoaderProbeDesc& loader_desc = {},
                              const VulkanInstanceCreateDesc& instance_desc = {}, SurfaceHandle surface = {});

} // namespace mirakana::rhi::vulkan
