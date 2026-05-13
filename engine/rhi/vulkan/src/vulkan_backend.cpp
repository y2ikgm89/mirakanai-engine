// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/vulkan/vulkan_backend.hpp"

#include "mirakana/rhi/gpu_debug.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::rhi::vulkan {
namespace {

inline constexpr std::string_view debug_utils_extension_name = "VK_EXT_debug_utils";

void record_queue_submit(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_submitted_fence_queue = fence.queue;
    switch (queue) {
    case QueueKind::graphics:
        ++stats.graphics_queue_submits;
        stats.last_graphics_submitted_fence_value = fence.value;
        stats.last_graphics_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        ++stats.compute_queue_submits;
        stats.last_compute_submitted_fence_value = fence.value;
        stats.last_compute_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        ++stats.copy_queue_submits;
        stats.last_copy_submitted_fence_value = fence.value;
        stats.last_copy_submit_sequence = stats.queue_event_sequence;
        return;
    }
}

void record_queue_wait(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_queue_wait_fence_value = fence.value;
    stats.last_queue_wait_fence_queue = fence.queue;
    stats.last_queue_wait_sequence = stats.queue_event_sequence;
    switch (queue) {
    case QueueKind::graphics:
        stats.last_graphics_queue_wait_fence_value = fence.value;
        stats.last_graphics_queue_wait_fence_queue = fence.queue;
        stats.last_graphics_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        stats.last_compute_queue_wait_fence_value = fence.value;
        stats.last_compute_queue_wait_fence_queue = fence.queue;
        stats.last_compute_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        stats.last_copy_queue_wait_fence_value = fence.value;
        stats.last_copy_queue_wait_fence_queue = fence.queue;
        stats.last_copy_queue_wait_sequence = stats.queue_event_sequence;
        return;
    }
}

#if defined(_WIN32)
#define MK_VULKAN_CALL __stdcall
#else
#define MK_VULKAN_CALL
#endif

using VulkanVoidFunction = void(MK_VULKAN_CALL*)();
using VulkanGetInstanceProcAddr = VulkanVoidFunction(MK_VULKAN_CALL*)(void*, const char*);
using VulkanResult = std::int32_t;

inline constexpr VulkanResult vulkan_success = 0;
inline constexpr VulkanResult vulkan_not_ready = 1;
inline constexpr VulkanResult vulkan_timeout = 2;
inline constexpr VulkanResult vulkan_incomplete = 5;
inline constexpr VulkanResult vulkan_suboptimal = 1000001003;
inline constexpr VulkanResult vulkan_error_out_of_date = -1000001004;
inline constexpr std::size_t vulkan_max_extension_name_size = 256;
inline constexpr std::uint32_t vulkan_queue_graphics_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_queue_compute_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_queue_transfer_bit = 0x00000004U;
inline constexpr std::uint32_t vulkan_structure_type_application_info = 0;
inline constexpr std::uint32_t vulkan_structure_type_instance_create_info = 1;
inline constexpr std::uint32_t vulkan_structure_type_device_queue_create_info = 2;
inline constexpr std::uint32_t vulkan_structure_type_device_create_info = 3;
inline constexpr std::uint32_t vulkan_structure_type_memory_allocate_info = 5;
inline constexpr std::uint32_t vulkan_structure_type_fence_create_info = 8;
inline constexpr std::uint32_t vulkan_structure_type_semaphore_create_info = 9;
inline constexpr std::uint32_t vulkan_structure_type_buffer_create_info = 12;
inline constexpr std::uint32_t vulkan_structure_type_image_view_create_info = 15;
inline constexpr std::uint32_t vulkan_structure_type_sampler_create_info = 31;
inline constexpr std::uint32_t vulkan_structure_type_shader_module_create_info = 16;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_shader_stage_create_info = 18;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_vertex_input_state_create_info = 19;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_input_assembly_state_create_info = 20;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_viewport_state_create_info = 22;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_rasterization_state_create_info = 23;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_multisample_state_create_info = 24;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_depth_stencil_state_create_info = 25;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_color_blend_state_create_info = 26;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_dynamic_state_create_info = 27;
inline constexpr std::uint32_t vulkan_structure_type_graphics_pipeline_create_info = 28;
inline constexpr std::uint32_t vulkan_structure_type_compute_pipeline_create_info = 29;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_layout_create_info = 30;
inline constexpr std::uint32_t vulkan_structure_type_descriptor_set_layout_create_info = 32;
inline constexpr std::uint32_t vulkan_structure_type_descriptor_pool_create_info = 33;
inline constexpr std::uint32_t vulkan_structure_type_descriptor_set_allocate_info = 34;
inline constexpr std::uint32_t vulkan_structure_type_write_descriptor_set = 35;
inline constexpr std::uint32_t vulkan_structure_type_command_pool_create_info = 39;
inline constexpr std::uint32_t vulkan_structure_type_command_buffer_allocate_info = 40;
inline constexpr std::uint32_t vulkan_structure_type_command_buffer_begin_info = 42;
inline constexpr std::uint32_t vulkan_structure_type_rendering_info = 1000044000;
inline constexpr std::uint32_t vulkan_structure_type_rendering_attachment_info = 1000044001;
inline constexpr std::uint32_t vulkan_structure_type_pipeline_rendering_create_info = 1000044002;
inline constexpr std::uint32_t vulkan_structure_type_memory_barrier2 = 1000314000;
inline constexpr std::uint32_t vulkan_structure_type_image_memory_barrier2 = 1000314002;
inline constexpr std::uint32_t vulkan_structure_type_dependency_info = 1000314003;
inline constexpr std::uint32_t vulkan_structure_type_submit_info2 = 1000314004;
inline constexpr std::uint32_t vulkan_structure_type_semaphore_submit_info = 1000314005;
inline constexpr std::uint32_t vulkan_structure_type_command_buffer_submit_info = 1000314006;
inline constexpr std::uint32_t vulkan_structure_type_physical_device_features_2 = 1000059000;
inline constexpr std::uint32_t vulkan_structure_type_physical_device_properties_2 = 1000059001;
inline constexpr std::uint32_t vulkan_structure_type_physical_device_dynamic_rendering_features = 1000044003;
inline constexpr std::uint32_t vulkan_structure_type_physical_device_vulkan_1_3_features = 53;
inline constexpr std::uint32_t vulkan_structure_type_image_create_info = 14;
inline constexpr std::uint32_t vulkan_structure_type_swapchain_create_info = 1000001000;
inline constexpr std::uint32_t vulkan_structure_type_present_info = 1000001001;
inline constexpr std::uint32_t vulkan_structure_type_win32_surface_create_info = 1000009000;
inline constexpr std::size_t vulkan_physical_device_feature_bool_count = 55;
inline constexpr std::size_t vulkan_max_physical_device_name_size = 256;
inline constexpr std::size_t vulkan_uuid_size = 16;
inline constexpr std::size_t vulkan_physical_device_properties_tail_storage_size = 4096;
inline constexpr std::uint32_t vulkan_physical_device_type_other = 0;
inline constexpr std::uint32_t vulkan_physical_device_type_integrated_gpu = 1;
inline constexpr std::uint32_t vulkan_physical_device_type_discrete_gpu = 2;
inline constexpr std::uint32_t vulkan_physical_device_type_virtual_gpu = 3;
inline constexpr std::uint32_t vulkan_physical_device_type_cpu = 4;
inline constexpr std::uint32_t vulkan_command_pool_create_transient_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_command_pool_create_reset_command_buffer_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_command_buffer_level_primary = 0;
inline constexpr std::uint32_t vulkan_shader_stage_vertex_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_shader_stage_fragment_bit = 0x00000010U;
inline constexpr std::uint32_t vulkan_shader_stage_compute_bit = 0x00000020U;
inline constexpr std::uint32_t vulkan_descriptor_type_sampler = 0;
inline constexpr std::uint32_t vulkan_descriptor_type_sampled_image = 2;
inline constexpr std::uint32_t vulkan_descriptor_type_storage_image = 3;
inline constexpr std::uint32_t vulkan_descriptor_type_uniform_buffer = 6;
inline constexpr std::uint32_t vulkan_descriptor_type_storage_buffer = 7;
inline constexpr std::uint32_t vulkan_filter_nearest = 0;
inline constexpr std::uint32_t vulkan_filter_linear = 1;
inline constexpr std::uint32_t vulkan_sampler_mipmap_mode_nearest = 0;
inline constexpr std::uint32_t vulkan_sampler_address_mode_repeat = 0;
inline constexpr std::uint32_t vulkan_sampler_address_mode_clamp_to_edge = 2;
inline constexpr std::uint32_t vulkan_compare_op_never = 0;
inline constexpr std::uint32_t vulkan_compare_op_less = 1;
inline constexpr std::uint32_t vulkan_compare_op_equal = 2;
inline constexpr std::uint32_t vulkan_compare_op_less_or_equal = 3;
inline constexpr std::uint32_t vulkan_compare_op_greater = 4;
inline constexpr std::uint32_t vulkan_compare_op_not_equal = 5;
inline constexpr std::uint32_t vulkan_compare_op_greater_or_equal = 6;
inline constexpr std::uint32_t vulkan_compare_op_always = 7;
inline constexpr std::uint32_t vulkan_border_color_float_transparent_black = 0;
inline constexpr std::uint32_t vulkan_format_r32g32_sfloat = 103;
inline constexpr std::uint32_t vulkan_format_r32g32b32_sfloat = 106;
inline constexpr std::uint32_t vulkan_format_r32g32b32a32_sfloat = 109;
/// `VK_FORMAT_R16G16B16A16_UINT` - four 16-bit unsigned integer components per vertex attribute (skin joint indices).
inline constexpr std::uint32_t vulkan_format_r16g16b16a16_uint = 95;
inline constexpr std::uint32_t vulkan_format_r8g8b8a8_unorm = 37;
inline constexpr std::uint32_t vulkan_format_b8g8r8a8_unorm = 44;
inline constexpr std::uint32_t vulkan_format_d24_unorm_s8_uint = 129;
inline constexpr std::uint32_t vulkan_color_space_srgb_nonlinear = 0;
inline constexpr std::uint32_t vulkan_image_usage_transfer_src_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_image_usage_transfer_dst_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_image_usage_sampled_bit = 0x00000004U;
inline constexpr std::uint32_t vulkan_image_usage_storage_bit = 0x00000008U;
inline constexpr std::uint32_t vulkan_image_usage_color_attachment_bit = 0x00000010U;
inline constexpr std::uint32_t vulkan_image_usage_depth_stencil_attachment_bit = 0x00000020U;
inline constexpr std::uint32_t vulkan_buffer_usage_transfer_src_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_buffer_usage_transfer_dst_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_buffer_usage_uniform_buffer_bit = 0x00000010U;
inline constexpr std::uint32_t vulkan_buffer_usage_storage_buffer_bit = 0x00000020U;
inline constexpr std::uint32_t vulkan_buffer_usage_index_buffer_bit = 0x00000040U;
inline constexpr std::uint32_t vulkan_buffer_usage_vertex_buffer_bit = 0x00000080U;
inline constexpr std::uint32_t vulkan_image_view_type_2d = 1;
inline constexpr std::uint32_t vulkan_component_swizzle_identity = 0;
inline constexpr std::uint32_t vulkan_image_aspect_color_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_image_aspect_depth_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_image_aspect_stencil_bit = 0x00000004U;
inline constexpr std::uint32_t vulkan_memory_property_device_local_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_memory_property_host_visible_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_memory_property_host_coherent_bit = 0x00000004U;
inline constexpr std::uint32_t vulkan_image_type_2d = 1;
inline constexpr std::uint32_t vulkan_image_tiling_optimal = 0;
inline constexpr std::uint32_t vulkan_sharing_mode_exclusive = 0;
inline constexpr std::uint32_t vulkan_sharing_mode_concurrent = 1;
inline constexpr std::uint32_t vulkan_surface_transform_identity_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_composite_alpha_opaque_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_present_mode_immediate = 0;
inline constexpr std::uint32_t vulkan_present_mode_mailbox = 1;
inline constexpr std::uint32_t vulkan_present_mode_fifo = 2;
inline constexpr std::uint32_t vulkan_present_mode_fifo_relaxed = 3;
inline constexpr std::uint32_t vulkan_primitive_topology_line_list = 1;
inline constexpr std::uint32_t vulkan_primitive_topology_triangle_list = 3;
inline constexpr std::uint32_t vulkan_polygon_mode_fill = 0;
inline constexpr std::uint32_t vulkan_cull_mode_none = 0;
inline constexpr std::uint32_t vulkan_front_face_counter_clockwise = 1;
inline constexpr std::uint32_t vulkan_sample_count_1_bit = 1;
inline constexpr std::uint32_t vulkan_color_component_r_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_color_component_g_bit = 0x00000002U;
inline constexpr std::uint32_t vulkan_color_component_b_bit = 0x00000004U;
inline constexpr std::uint32_t vulkan_color_component_a_bit = 0x00000008U;
inline constexpr std::uint32_t vulkan_dynamic_state_viewport = 0;
inline constexpr std::uint32_t vulkan_dynamic_state_scissor = 1;
inline constexpr std::uint32_t vulkan_fence_create_signaled_bit = 0x00000001U;
inline constexpr std::uint32_t vulkan_attachment_load_op_load = 0;
inline constexpr std::uint32_t vulkan_attachment_load_op_clear = 1;
inline constexpr std::uint32_t vulkan_attachment_load_op_dont_care = 2;
inline constexpr std::uint32_t vulkan_attachment_store_op_store = 0;
inline constexpr std::uint32_t vulkan_attachment_store_op_dont_care = 1;
inline constexpr std::uint32_t vulkan_image_layout_color_attachment_optimal = 2;
inline constexpr std::uint32_t vulkan_image_layout_depth_stencil_attachment_optimal = 3;
inline constexpr std::uint32_t vulkan_image_layout_general = 1;
inline constexpr std::uint32_t vulkan_image_layout_shader_read_only_optimal = 5;
inline constexpr std::uint32_t vulkan_resolve_mode_none = 0;
inline constexpr std::uint32_t vulkan_pipeline_bind_point_graphics = 0;
inline constexpr std::uint32_t vulkan_pipeline_bind_point_compute = 1;
inline constexpr std::uint32_t vulkan_index_type_uint16 = 0;
inline constexpr std::uint32_t vulkan_index_type_uint32 = 1;
inline constexpr std::uint32_t vulkan_vertex_input_rate_vertex = 0;
inline constexpr std::uint32_t vulkan_vertex_input_rate_instance = 1;
inline constexpr std::uint32_t vulkan_queue_family_ignored = 0xffffffffU;
inline constexpr std::uint32_t vulkan_image_layout_undefined = 0;
inline constexpr std::uint32_t vulkan_image_layout_transfer_src_optimal = 6;
inline constexpr std::uint32_t vulkan_image_layout_transfer_dst_optimal = 7;
inline constexpr std::uint32_t vulkan_image_layout_present_src = 1000001002;
inline constexpr std::uint64_t vulkan_pipeline_stage2_none = 0;
inline constexpr std::uint64_t vulkan_pipeline_stage2_early_fragment_tests_bit = 0x0000000000000100ULL;
inline constexpr std::uint64_t vulkan_pipeline_stage2_late_fragment_tests_bit = 0x0000000000000200ULL;
inline constexpr std::uint64_t vulkan_pipeline_stage2_fragment_shader_bit = 0x0000000000000080ULL;
inline constexpr std::uint64_t vulkan_pipeline_stage2_compute_shader_bit = 0x0000000000000800ULL;
inline constexpr std::uint64_t vulkan_pipeline_stage2_color_attachment_output_bit = 0x0000000000000400ULL;
inline constexpr std::uint64_t vulkan_pipeline_stage2_transfer_bit = 0x0000000000001000ULL;
inline constexpr std::uint64_t vulkan_access2_none = 0;
inline constexpr std::uint64_t vulkan_access2_shader_read_bit = 0x0000000000000020ULL;
inline constexpr std::uint64_t vulkan_access2_shader_write_bit = 0x0000000000000040ULL;
inline constexpr std::uint64_t vulkan_access2_color_attachment_write_bit = 0x0000000000000100ULL;
inline constexpr std::uint64_t vulkan_access2_depth_stencil_attachment_read_bit = 0x0000000000000200ULL;
inline constexpr std::uint64_t vulkan_access2_depth_stencil_attachment_write_bit = 0x0000000000000400ULL;
inline constexpr std::uint64_t vulkan_access2_transfer_read_bit = 0x0000000000000800ULL;
inline constexpr std::uint64_t vulkan_access2_transfer_write_bit = 0x0000000000001000ULL;
inline constexpr std::uint32_t spirv_magic_word = 0x07230203U;
inline constexpr std::uint32_t spirv_header_word_count = 5;
inline constexpr std::size_t vulkan_max_memory_types = 32;
inline constexpr std::size_t vulkan_max_memory_heaps = 16;

using NativeVulkanInstance = void*;
using NativeVulkanPhysicalDevice = void*;
using NativeVulkanDevice = void*;
using NativeVulkanQueue = void*;
using NativeVulkanSurface = std::uint64_t;
using NativeVulkanSwapchain = std::uint64_t;
using NativeVulkanImage = std::uint64_t;
using NativeVulkanImageView = std::uint64_t;
using NativeVulkanSemaphore = std::uint64_t;
using NativeVulkanFence = std::uint64_t;
using NativeVulkanCommandPool = std::uint64_t;
using NativeVulkanShaderModule = std::uint64_t;
using NativeVulkanPipelineLayout = std::uint64_t;
using NativeVulkanPipeline = std::uint64_t;
using NativeVulkanPipelineCache = std::uint64_t;
using NativeVulkanRenderPass = std::uint64_t;
using NativeVulkanBuffer = std::uint64_t;
using NativeVulkanDeviceMemory = std::uint64_t;
using NativeVulkanDescriptorSetLayout = std::uint64_t;
using NativeVulkanDescriptorPool = std::uint64_t;
using NativeVulkanDescriptorSet = std::uint64_t;
using NativeVulkanSampler = std::uint64_t;

struct NativeVulkanExtensionProperties {
    std::array<char, vulkan_max_extension_name_size> extension_name{};
    std::uint32_t spec_version;
};

struct NativeVulkanExtent3D {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
};

struct NativeVulkanExtent2D {
    std::uint32_t width;
    std::uint32_t height;
};

struct NativeVulkanQueueFamilyProperties {
    std::uint32_t queue_flags;
    std::uint32_t queue_count;
    std::uint32_t timestamp_valid_bits;
    NativeVulkanExtent3D min_image_transfer_granularity;
};

struct NativeVulkanPhysicalDeviceFeatures {
    std::array<std::uint32_t, vulkan_physical_device_feature_bool_count> bools{};
};

struct NativeVulkanPhysicalDeviceFeatures2 {
    std::uint32_t s_type;
    void* next;
    NativeVulkanPhysicalDeviceFeatures features;
};

struct NativeVulkanPhysicalDeviceDynamicRenderingFeatures {
    std::uint32_t s_type;
    void* next;
    std::uint32_t dynamic_rendering;
};

struct NativeVulkanPhysicalDeviceVulkan13Features {
    std::uint32_t s_type;
    void* next;
    std::uint32_t robust_image_access;
    std::uint32_t inline_uniform_block;
    std::uint32_t descriptor_binding_inline_uniform_block_update_after_bind;
    std::uint32_t pipeline_creation_cache_control;
    std::uint32_t private_data;
    std::uint32_t shader_demote_to_helper_invocation;
    std::uint32_t shader_terminate_invocation;
    std::uint32_t subgroup_size_control;
    std::uint32_t compute_full_subgroups;
    std::uint32_t synchronization2;
    std::uint32_t texture_compression_astc_hdr;
    std::uint32_t shader_zero_initialize_workgroup_memory;
    std::uint32_t dynamic_rendering;
    std::uint32_t shader_integer_dot_product;
    std::uint32_t maintenance4;
};

struct NativeVulkanPhysicalDeviceProperties {
    std::uint32_t api_version;
    std::uint32_t driver_version;
    std::uint32_t vendor_id;
    std::uint32_t device_id;
    std::uint32_t device_type;
    std::array<char, vulkan_max_physical_device_name_size> device_name{};
    std::array<std::uint8_t, vulkan_uuid_size> pipeline_cache_uuid{};
    std::array<std::uint8_t, vulkan_physical_device_properties_tail_storage_size> limits_and_sparse_properties{};
};

struct NativeVulkanPhysicalDeviceProperties2 {
    std::uint32_t s_type;
    void* next;
    NativeVulkanPhysicalDeviceProperties properties;
};

struct NativeVulkanWin32SurfaceCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    void* instance;
    void* window;
};

struct NativeVulkanSurfaceCapabilities {
    std::uint32_t min_image_count;
    std::uint32_t max_image_count;
    NativeVulkanExtent2D current_extent;
    NativeVulkanExtent2D min_image_extent;
    NativeVulkanExtent2D max_image_extent;
    std::uint32_t max_image_array_layers;
    std::uint32_t supported_transforms;
    std::uint32_t current_transform;
    std::uint32_t supported_composite_alpha;
    std::uint32_t supported_usage_flags;
};

struct NativeVulkanSurfaceFormat {
    std::uint32_t format;
    std::uint32_t color_space;
};

struct NativeVulkanApplicationInfo {
    std::uint32_t s_type;
    const void* next;
    const char* application_name;
    std::uint32_t application_version;
    const char* engine_name;
    std::uint32_t engine_version;
    std::uint32_t api_version;
};

struct NativeVulkanInstanceCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    const NativeVulkanApplicationInfo* application_info;
    std::uint32_t enabled_layer_count;
    const char* const* enabled_layer_names;
    std::uint32_t enabled_extension_count;
    const char* const* enabled_extension_names;
};

struct NativeVulkanDeviceQueueCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t queue_family_index;
    std::uint32_t queue_count;
    const float* queue_priorities;
};

struct NativeVulkanDeviceCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t queue_create_info_count;
    const NativeVulkanDeviceQueueCreateInfo* queue_create_infos;
    std::uint32_t enabled_layer_count;
    const char* const* enabled_layer_names;
    std::uint32_t enabled_extension_count;
    const char* const* enabled_extension_names;
    const NativeVulkanPhysicalDeviceFeatures* enabled_features;
};

struct NativeVulkanSwapchainCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    NativeVulkanSurface surface;
    std::uint32_t min_image_count;
    std::uint32_t image_format;
    std::uint32_t image_color_space;
    NativeVulkanExtent2D image_extent;
    std::uint32_t image_array_layers;
    std::uint32_t image_usage;
    std::uint32_t image_sharing_mode;
    std::uint32_t queue_family_index_count;
    const std::uint32_t* queue_family_indices;
    std::uint32_t pre_transform;
    std::uint32_t composite_alpha;
    std::uint32_t present_mode;
    std::uint32_t clipped;
    NativeVulkanSwapchain old_swapchain;
};

struct NativeVulkanComponentMapping {
    std::uint32_t r;
    std::uint32_t g;
    std::uint32_t b;
    std::uint32_t a;
};

struct NativeVulkanImageSubresourceRange {
    std::uint32_t aspect_mask;
    std::uint32_t base_mip_level;
    std::uint32_t level_count;
    std::uint32_t base_array_layer;
    std::uint32_t layer_count;
};

struct NativeVulkanImageViewCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    NativeVulkanImage image;
    std::uint32_t view_type;
    std::uint32_t format;
    NativeVulkanComponentMapping components;
    NativeVulkanImageSubresourceRange subresource_range;
};

struct NativeVulkanPresentInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t wait_semaphore_count;
    const NativeVulkanSemaphore* wait_semaphores;
    std::uint32_t swapchain_count;
    const NativeVulkanSwapchain* swapchains;
    const std::uint32_t* image_indices;
    VulkanResult* results;
};

struct NativeVulkanSemaphoreCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
};

struct NativeVulkanFenceCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
};

struct NativeVulkanCommandPoolCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t queue_family_index;
};

struct NativeVulkanCommandBufferAllocateInfo {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanCommandPool command_pool;
    std::uint32_t level;
    std::uint32_t command_buffer_count;
};

struct NativeVulkanCommandBufferBeginInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    const void* inheritance_info;
};

struct NativeVulkanBufferCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint64_t size;
    std::uint32_t usage;
    std::uint32_t sharing_mode;
    std::uint32_t queue_family_index_count;
    const std::uint32_t* queue_family_indices;
};

struct NativeVulkanImageCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t image_type;
    std::uint32_t format;
    NativeVulkanExtent3D extent;
    std::uint32_t mip_levels;
    std::uint32_t array_layers;
    std::uint32_t samples;
    std::uint32_t tiling;
    std::uint32_t usage;
    std::uint32_t sharing_mode;
    std::uint32_t queue_family_index_count;
    const std::uint32_t* queue_family_indices;
    std::uint32_t initial_layout;
};

struct NativeVulkanMemoryRequirements {
    std::uint64_t size;
    std::uint64_t alignment;
    std::uint32_t memory_type_bits;
};

struct NativeVulkanMemoryAllocateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint64_t allocation_size;
    std::uint32_t memory_type_index;
};

struct NativeVulkanMemoryType {
    std::uint32_t property_flags;
    std::uint32_t heap_index;
};

struct NativeVulkanMemoryHeap {
    std::uint64_t size;
    std::uint32_t flags;
};

struct NativeVulkanPhysicalDeviceMemoryProperties {
    std::uint32_t memory_type_count;
    std::array<NativeVulkanMemoryType, vulkan_max_memory_types> memory_types{};
    std::uint32_t memory_heap_count;
    std::array<NativeVulkanMemoryHeap, vulkan_max_memory_heaps> memory_heaps{};
};

struct NativeVulkanShaderModuleCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::size_t code_size;
    const std::uint32_t* code;
};

struct NativeVulkanDescriptorSetLayoutBinding {
    std::uint32_t binding;
    std::uint32_t descriptor_type;
    std::uint32_t descriptor_count;
    std::uint32_t stage_flags;
    const std::uint64_t* immutable_samplers;
};

struct NativeVulkanDescriptorSetLayoutCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t binding_count;
    const NativeVulkanDescriptorSetLayoutBinding* bindings;
};

struct NativeVulkanDescriptorPoolSize {
    std::uint32_t type;
    std::uint32_t descriptor_count;
};

struct NativeVulkanDescriptorPoolCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t max_sets;
    std::uint32_t pool_size_count;
    const NativeVulkanDescriptorPoolSize* pool_sizes;
};

struct NativeVulkanDescriptorSetAllocateInfo {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanDescriptorPool descriptor_pool;
    std::uint32_t descriptor_set_count;
    const NativeVulkanDescriptorSetLayout* set_layouts;
};

struct NativeVulkanDescriptorBufferInfo {
    NativeVulkanBuffer buffer;
    std::uint64_t offset;
    std::uint64_t range;
};

struct NativeVulkanDescriptorImageInfo {
    NativeVulkanSampler sampler;
    NativeVulkanImageView image_view;
    std::uint32_t image_layout;
};

struct NativeVulkanWriteDescriptorSet {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanDescriptorSet dst_set;
    std::uint32_t dst_binding;
    std::uint32_t dst_array_element;
    std::uint32_t descriptor_count;
    std::uint32_t descriptor_type;
    const void* image_info;
    const NativeVulkanDescriptorBufferInfo* buffer_info;
    const void* texel_buffer_view;
};

struct NativeVulkanSamplerCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t mag_filter;
    std::uint32_t min_filter;
    std::uint32_t mipmap_mode;
    std::uint32_t address_mode_u;
    std::uint32_t address_mode_v;
    std::uint32_t address_mode_w;
    float mip_lod_bias;
    std::uint32_t anisotropy_enable;
    float max_anisotropy;
    std::uint32_t compare_enable;
    std::uint32_t compare_op;
    float min_lod;
    float max_lod;
    std::uint32_t border_color;
    std::uint32_t unnormalized_coordinates;
};

struct NativeVulkanPipelineLayoutCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t set_layout_count;
    const std::uint64_t* set_layouts;
    std::uint32_t push_constant_range_count;
    const void* push_constant_ranges;
};

struct NativeVulkanPipelineRenderingCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t view_mask;
    std::uint32_t color_attachment_count;
    const std::uint32_t* color_attachment_formats;
    std::uint32_t depth_attachment_format;
    std::uint32_t stencil_attachment_format;
};

struct NativeVulkanPipelineShaderStageCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t stage;
    NativeVulkanShaderModule module;
    const char* name;
    const void* specialization_info;
};

struct NativeVulkanVertexInputBindingDescription {
    std::uint32_t binding;
    std::uint32_t stride;
    std::uint32_t input_rate;
};

struct NativeVulkanVertexInputAttributeDescription {
    std::uint32_t location;
    std::uint32_t binding;
    std::uint32_t format;
    std::uint32_t offset;
};

struct NativeVulkanPipelineVertexInputStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t vertex_binding_description_count;
    const void* vertex_binding_descriptions;
    std::uint32_t vertex_attribute_description_count;
    const void* vertex_attribute_descriptions;
};

struct NativeVulkanPipelineInputAssemblyStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t topology;
    std::uint32_t primitive_restart_enable;
};

struct NativeVulkanViewport {
    float x;
    float y;
    float width;
    float height;
    float min_depth;
    float max_depth;
};

struct NativeVulkanOffset2D {
    std::int32_t x;
    std::int32_t y;
};

struct NativeVulkanRect2D {
    NativeVulkanOffset2D offset;
    NativeVulkanExtent2D extent;
};

struct NativeVulkanOffset3D {
    std::int32_t x;
    std::int32_t y;
    std::int32_t z;
};

struct NativeVulkanImageSubresourceLayers {
    std::uint32_t aspect_mask;
    std::uint32_t mip_level;
    std::uint32_t base_array_layer;
    std::uint32_t layer_count;
};

struct NativeVulkanBufferImageCopy {
    std::uint64_t buffer_offset;
    std::uint32_t buffer_row_length;
    std::uint32_t buffer_image_height;
    NativeVulkanImageSubresourceLayers image_subresource;
    NativeVulkanOffset3D image_offset;
    NativeVulkanExtent3D image_extent;
};

struct NativeVulkanBufferCopy {
    std::uint64_t src_offset;
    std::uint64_t dst_offset;
    std::uint64_t size;
};

struct NativeVulkanClearColorValue {
    std::array<float, 4> float32{};
};

struct NativeVulkanClearDepthStencilValue {
    float depth;
    std::uint32_t stencil;
};

union NativeVulkanClearValue {
    NativeVulkanClearColorValue color;
    NativeVulkanClearDepthStencilValue depth_stencil;
};

struct NativeVulkanRenderingAttachmentInfo {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanImageView image_view;
    std::uint32_t image_layout;
    std::uint32_t resolve_mode;
    NativeVulkanImageView resolve_image_view;
    std::uint32_t resolve_image_layout;
    std::uint32_t load_op;
    std::uint32_t store_op;
    NativeVulkanClearValue clear_value;
};

struct NativeVulkanRenderingInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    NativeVulkanRect2D render_area;
    std::uint32_t layer_count;
    std::uint32_t view_mask;
    std::uint32_t color_attachment_count;
    const NativeVulkanRenderingAttachmentInfo* color_attachments;
    const NativeVulkanRenderingAttachmentInfo* depth_attachment;
    const NativeVulkanRenderingAttachmentInfo* stencil_attachment;
};

struct NativeVulkanImageMemoryBarrier2 {
    std::uint32_t s_type;
    const void* next;
    std::uint64_t src_stage_mask;
    std::uint64_t src_access_mask;
    std::uint64_t dst_stage_mask;
    std::uint64_t dst_access_mask;
    std::uint32_t old_layout;
    std::uint32_t new_layout;
    std::uint32_t src_queue_family_index;
    std::uint32_t dst_queue_family_index;
    NativeVulkanImage image;
    NativeVulkanImageSubresourceRange subresource_range;
};

struct NativeVulkanMemoryBarrier2 {
    std::uint32_t s_type;
    const void* next;
    std::uint64_t src_stage_mask;
    std::uint64_t src_access_mask;
    std::uint64_t dst_stage_mask;
    std::uint64_t dst_access_mask;
};

struct NativeVulkanDependencyInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t dependency_flags;
    std::uint32_t memory_barrier_count;
    const void* memory_barriers;
    std::uint32_t buffer_memory_barrier_count;
    const void* buffer_memory_barriers;
    std::uint32_t image_memory_barrier_count;
    const NativeVulkanImageMemoryBarrier2* image_memory_barriers;
};

struct NativeVulkanSemaphoreSubmitInfo {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanSemaphore semaphore;
    std::uint64_t value;
    std::uint64_t stage_mask;
    std::uint32_t device_index;
};

struct NativeVulkanCommandBufferSubmitInfo {
    std::uint32_t s_type;
    const void* next;
    NativeVulkanCommandBuffer command_buffer;
    std::uint32_t device_mask;
};

struct NativeVulkanSubmitInfo2 {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t wait_semaphore_info_count;
    const NativeVulkanSemaphoreSubmitInfo* wait_semaphore_infos;
    std::uint32_t command_buffer_info_count;
    const NativeVulkanCommandBufferSubmitInfo* command_buffer_infos;
    std::uint32_t signal_semaphore_info_count;
    const NativeVulkanSemaphoreSubmitInfo* signal_semaphore_infos;
};

struct NativeVulkanPipelineViewportStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t viewport_count;
    const NativeVulkanViewport* viewports;
    std::uint32_t scissor_count;
    const NativeVulkanRect2D* scissors;
};

struct NativeVulkanPipelineRasterizationStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t depth_clamp_enable;
    std::uint32_t rasterizer_discard_enable;
    std::uint32_t polygon_mode;
    std::uint32_t cull_mode;
    std::uint32_t front_face;
    std::uint32_t depth_bias_enable;
    float depth_bias_constant_factor;
    float depth_bias_clamp;
    float depth_bias_slope_factor;
    float line_width;
};

struct NativeVulkanPipelineMultisampleStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t rasterization_samples;
    std::uint32_t sample_shading_enable;
    float min_sample_shading;
    const std::uint32_t* sample_mask;
    std::uint32_t alpha_to_coverage_enable;
    std::uint32_t alpha_to_one_enable;
};

struct NativeVulkanStencilOpState {
    std::uint32_t fail_op;
    std::uint32_t pass_op;
    std::uint32_t depth_fail_op;
    std::uint32_t compare_op;
    std::uint32_t compare_mask;
    std::uint32_t write_mask;
    std::uint32_t reference;
};

struct NativeVulkanPipelineDepthStencilStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t depth_test_enable;
    std::uint32_t depth_write_enable;
    std::uint32_t depth_compare_op;
    std::uint32_t depth_bounds_test_enable;
    std::uint32_t stencil_test_enable;
    NativeVulkanStencilOpState front;
    NativeVulkanStencilOpState back;
    float min_depth_bounds;
    float max_depth_bounds;
};

struct NativeVulkanPipelineColorBlendAttachmentState {
    std::uint32_t blend_enable;
    std::uint32_t src_color_blend_factor;
    std::uint32_t dst_color_blend_factor;
    std::uint32_t color_blend_op;
    std::uint32_t src_alpha_blend_factor;
    std::uint32_t dst_alpha_blend_factor;
    std::uint32_t alpha_blend_op;
    std::uint32_t color_write_mask;
};

struct NativeVulkanPipelineColorBlendStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t logic_op_enable;
    std::uint32_t logic_op;
    std::uint32_t attachment_count;
    const NativeVulkanPipelineColorBlendAttachmentState* attachments;
    std::array<float, 4> blend_constants{};
};

struct NativeVulkanPipelineDynamicStateCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t dynamic_state_count;
    const std::uint32_t* dynamic_states;
};

struct NativeVulkanGraphicsPipelineCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    std::uint32_t stage_count;
    const NativeVulkanPipelineShaderStageCreateInfo* stages;
    const NativeVulkanPipelineVertexInputStateCreateInfo* vertex_input_state;
    const NativeVulkanPipelineInputAssemblyStateCreateInfo* input_assembly_state;
    const void* tessellation_state;
    const NativeVulkanPipelineViewportStateCreateInfo* viewport_state;
    const NativeVulkanPipelineRasterizationStateCreateInfo* rasterization_state;
    const NativeVulkanPipelineMultisampleStateCreateInfo* multisample_state;
    const NativeVulkanPipelineDepthStencilStateCreateInfo* depth_stencil_state;
    const NativeVulkanPipelineColorBlendStateCreateInfo* color_blend_state;
    const NativeVulkanPipelineDynamicStateCreateInfo* dynamic_state;
    NativeVulkanPipelineLayout layout;
    NativeVulkanRenderPass render_pass;
    std::uint32_t subpass;
    NativeVulkanPipeline base_pipeline_handle;
    std::int32_t base_pipeline_index;
};

struct NativeVulkanComputePipelineCreateInfo {
    std::uint32_t s_type;
    const void* next;
    std::uint32_t flags;
    NativeVulkanPipelineShaderStageCreateInfo stage;
    NativeVulkanPipelineLayout layout;
    NativeVulkanPipeline base_pipeline_handle;
    std::int32_t base_pipeline_index;
};

using VulkanEnumerateInstanceVersion = VulkanResult(MK_VULKAN_CALL*)(std::uint32_t*);
using VulkanEnumerateInstanceExtensionProperties = VulkanResult(MK_VULKAN_CALL*)(const char*, std::uint32_t*,
                                                                                 NativeVulkanExtensionProperties*);
using VulkanCreateInstance = VulkanResult(MK_VULKAN_CALL*)(const NativeVulkanInstanceCreateInfo*, const void*,
                                                           NativeVulkanInstance*);
using VulkanDestroyInstance = void(MK_VULKAN_CALL*)(NativeVulkanInstance, const void*);
using VulkanEnumeratePhysicalDevices = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanInstance, std::uint32_t*,
                                                                     NativeVulkanPhysicalDevice*);
using VulkanGetPhysicalDeviceQueueFamilyProperties = void(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice, std::uint32_t*,
                                                                           NativeVulkanQueueFamilyProperties*);
using VulkanEnumerateDeviceExtensionProperties = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice, const char*,
                                                                               std::uint32_t*,
                                                                               NativeVulkanExtensionProperties*);
using VulkanGetPhysicalDeviceFeatures2 = void(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                               NativeVulkanPhysicalDeviceFeatures2*);
using VulkanGetPhysicalDeviceProperties2 = void(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                                 NativeVulkanPhysicalDeviceProperties2*);
using VulkanGetPhysicalDeviceMemoryProperties = void(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                                      NativeVulkanPhysicalDeviceMemoryProperties*);
using VulkanCreateWin32Surface = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanInstance,
                                                               const NativeVulkanWin32SurfaceCreateInfo*, const void*,
                                                               NativeVulkanSurface*);
using VulkanDestroySurface = void(MK_VULKAN_CALL*)(NativeVulkanInstance, NativeVulkanSurface, const void*);
using VulkanGetPhysicalDeviceSurfaceSupport = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice, std::uint32_t,
                                                                            NativeVulkanSurface, std::uint32_t*);
using VulkanGetPhysicalDeviceSurfaceCapabilities = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                                                 NativeVulkanSurface,
                                                                                 NativeVulkanSurfaceCapabilities*);
using VulkanGetPhysicalDeviceSurfaceFormats = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                                            NativeVulkanSurface, std::uint32_t*,
                                                                            NativeVulkanSurfaceFormat*);
using VulkanGetPhysicalDeviceSurfacePresentModes = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                                                 NativeVulkanSurface, std::uint32_t*,
                                                                                 std::uint32_t*);
using VulkanCreateDevice = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanPhysicalDevice,
                                                         const NativeVulkanDeviceCreateInfo*, const void*,
                                                         NativeVulkanDevice*);
using VulkanGetDeviceProcAddr = VulkanVoidFunction(MK_VULKAN_CALL*)(NativeVulkanDevice, const char*);
using VulkanDestroyDevice = void(MK_VULKAN_CALL*)(NativeVulkanDevice, const void*);
using VulkanDeviceWaitIdle = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice);
using VulkanGetDeviceQueue = void(MK_VULKAN_CALL*)(NativeVulkanDevice, std::uint32_t, std::uint32_t,
                                                   NativeVulkanQueue*);
using VulkanCreateCommandPool = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                              const NativeVulkanCommandPoolCreateInfo*, const void*,
                                                              NativeVulkanCommandPool*);
using VulkanDestroyCommandPool = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanCommandPool, const void*);
using VulkanAllocateCommandBuffers = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                   const NativeVulkanCommandBufferAllocateInfo*,
                                                                   NativeVulkanCommandBuffer*);
using VulkanFreeCommandBuffers = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanCommandPool, std::uint32_t,
                                                       const NativeVulkanCommandBuffer*);
using VulkanBeginCommandBuffer = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer,
                                                               const NativeVulkanCommandBufferBeginInfo*);
using VulkanEndCommandBuffer = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer);
using VulkanCmdBeginRendering = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, const NativeVulkanRenderingInfo*);
using VulkanCmdEndRendering = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer);
using VulkanCmdBindPipeline = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, NativeVulkanPipeline);
using VulkanCmdSetViewport = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t,
                                                   const NativeVulkanViewport*);
using VulkanCmdSetScissor = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t,
                                                  const NativeVulkanRect2D*);
using VulkanCmdDraw = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t, std::uint32_t,
                                            std::uint32_t);
using VulkanCmdBindVertexBuffers = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t,
                                                         const NativeVulkanBuffer*, const std::uint64_t*);
using VulkanCmdBindIndexBuffer = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, NativeVulkanBuffer, std::uint64_t,
                                                       std::uint32_t);
using VulkanCmdDrawIndexed = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t,
                                                   std::uint32_t, std::int32_t, std::uint32_t);
using VulkanCmdDispatch = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t, std::uint32_t, std::uint32_t);
using VulkanCmdPipelineBarrier2 = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, const NativeVulkanDependencyInfo*);
using VulkanQueueSubmit2 = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanQueue, std::uint32_t,
                                                         const NativeVulkanSubmitInfo2*, NativeVulkanFence);
using VulkanQueueWaitIdle = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanQueue);
using VulkanCreateBuffer = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanBufferCreateInfo*,
                                                         const void*, NativeVulkanBuffer*);
using VulkanDestroyBuffer = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanBuffer, const void*);
using VulkanGetBufferMemoryRequirements = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanBuffer,
                                                                NativeVulkanMemoryRequirements*);
using VulkanCreateImage = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanImageCreateInfo*,
                                                        const void*, NativeVulkanImage*);
using VulkanDestroyImage = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanImage, const void*);
using VulkanGetImageMemoryRequirements = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanImage,
                                                               NativeVulkanMemoryRequirements*);
using VulkanAllocateMemory = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanMemoryAllocateInfo*,
                                                           const void*, NativeVulkanDeviceMemory*);
using VulkanFreeMemory = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanDeviceMemory, const void*);
using VulkanBindBufferMemory = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanBuffer,
                                                             NativeVulkanDeviceMemory, std::uint64_t);
using VulkanBindImageMemory = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanImage,
                                                            NativeVulkanDeviceMemory, std::uint64_t);
using VulkanMapMemory = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanDeviceMemory, std::uint64_t,
                                                      std::uint64_t, std::uint32_t, void**);
using VulkanUnmapMemory = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanDeviceMemory);
using VulkanCmdCopyImageToBuffer = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, NativeVulkanImage, std::uint32_t,
                                                         NativeVulkanBuffer, std::uint32_t,
                                                         const NativeVulkanBufferImageCopy*);
using VulkanCmdCopyBuffer = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, NativeVulkanBuffer, NativeVulkanBuffer,
                                                  std::uint32_t, const NativeVulkanBufferCopy*);
using VulkanCmdCopyBufferToImage = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, NativeVulkanBuffer,
                                                         NativeVulkanImage, std::uint32_t, std::uint32_t,
                                                         const NativeVulkanBufferImageCopy*);
using VulkanCreateShaderModule = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                               const NativeVulkanShaderModuleCreateInfo*, const void*,
                                                               NativeVulkanShaderModule*);
using VulkanDestroyShaderModule = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanShaderModule, const void*);
using VulkanCreateDescriptorSetLayout = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                      const NativeVulkanDescriptorSetLayoutCreateInfo*,
                                                                      const void*, NativeVulkanDescriptorSetLayout*);
using VulkanDestroyDescriptorSetLayout = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanDescriptorSetLayout,
                                                               const void*);
using VulkanCreateDescriptorPool = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                 const NativeVulkanDescriptorPoolCreateInfo*,
                                                                 const void*, NativeVulkanDescriptorPool*);
using VulkanDestroyDescriptorPool = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanDescriptorPool, const void*);
using VulkanAllocateDescriptorSets = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                   const NativeVulkanDescriptorSetAllocateInfo*,
                                                                   NativeVulkanDescriptorSet*);
using VulkanUpdateDescriptorSets = void(MK_VULKAN_CALL*)(NativeVulkanDevice, std::uint32_t,
                                                         const NativeVulkanWriteDescriptorSet*, std::uint32_t,
                                                         const void*);
using VulkanCmdBindDescriptorSets = void(MK_VULKAN_CALL*)(NativeVulkanCommandBuffer, std::uint32_t,
                                                          NativeVulkanPipelineLayout, std::uint32_t, std::uint32_t,
                                                          const NativeVulkanDescriptorSet*, std::uint32_t,
                                                          const std::uint32_t*);
using VulkanCreateSwapchain = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanSwapchainCreateInfo*,
                                                            const void*, NativeVulkanSwapchain*);
using VulkanDestroySwapchain = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanSwapchain, const void*);
using VulkanGetSwapchainImages = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanSwapchain,
                                                               std::uint32_t*, NativeVulkanImage*);
using VulkanAcquireNextImage = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanSwapchain, std::uint64_t,
                                                             NativeVulkanSemaphore, NativeVulkanFence, std::uint32_t*);
using VulkanCreateImageView = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanImageViewCreateInfo*,
                                                            const void*, NativeVulkanImageView*);
using VulkanDestroyImageView = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanImageView, const void*);
using VulkanCreateSampler = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanSamplerCreateInfo*,
                                                          const void*, NativeVulkanSampler*);
using VulkanDestroySampler = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanSampler, const void*);
using VulkanCreateSemaphore = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanSemaphoreCreateInfo*,
                                                            const void*, NativeVulkanSemaphore*);
using VulkanDestroySemaphore = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanSemaphore, const void*);
using VulkanCreateFence = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, const NativeVulkanFenceCreateInfo*,
                                                        const void*, NativeVulkanFence*);
using VulkanDestroyFence = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanFence, const void*);
using VulkanWaitForFences = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, std::uint32_t, const NativeVulkanFence*,
                                                          std::uint32_t, std::uint64_t);
using VulkanQueuePresent = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanQueue, const NativeVulkanPresentInfo*);
using VulkanCreatePipelineLayout = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                 const NativeVulkanPipelineLayoutCreateInfo*,
                                                                 const void*, NativeVulkanPipelineLayout*);
using VulkanDestroyPipelineLayout = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanPipelineLayout, const void*);
using VulkanCreateGraphicsPipelines = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanPipelineCache,
                                                                    std::uint32_t,
                                                                    const NativeVulkanGraphicsPipelineCreateInfo*,
                                                                    const void*, NativeVulkanPipeline*);
using VulkanCreateComputePipelines = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanPipelineCache,
                                                                   std::uint32_t,
                                                                   const NativeVulkanComputePipelineCreateInfo*,
                                                                   const void*, NativeVulkanPipeline*);
using VulkanDestroyPipeline = void(MK_VULKAN_CALL*)(NativeVulkanDevice, NativeVulkanPipeline, const void*);

struct VulkanRuntimeGlobalCommands {
    VulkanGetInstanceProcAddr get_instance_proc_addr{nullptr};
    VulkanEnumerateInstanceVersion enumerate_instance_version{nullptr};
    VulkanEnumerateInstanceExtensionProperties enumerate_instance_extension_properties{nullptr};
    VulkanCreateInstance create_instance{nullptr};
};

struct RuntimePhysicalDeviceProperties {
    std::string name;
    VulkanPhysicalDeviceType type{VulkanPhysicalDeviceType::other};
    VulkanApiVersion api_version{};
    std::uint32_t driver_version{0};
    std::uint32_t vendor_id{0};
    std::uint32_t device_id{0};
};

struct Vulkan13FeatureSupport {
    bool dynamic_rendering{false};
    bool synchronization2{false};
};

struct QueueFamilySelection {
    std::uint32_t graphics{invalid_vulkan_queue_family};
    std::uint32_t present{invalid_vulkan_queue_family};
};

[[nodiscard]] RhiHostPlatform normalize_host(RhiHostPlatform host) noexcept {
    return host == RhiHostPlatform::unknown ? current_rhi_host_platform() : host;
}

[[nodiscard]] std::string resolve_runtime_library_name(const VulkanLoaderProbeDesc& desc, RhiHostPlatform host) {
    if (!desc.runtime_library.empty()) {
        return std::string{desc.runtime_library};
    }
    return std::string{default_runtime_library_name(host)};
}

[[nodiscard]] bool runtime_library_has_symbol(std::string_view library_name, std::string_view symbol_name) noexcept {
    if (library_name.empty() || symbol_name.empty()) {
        return false;
    }

#if defined(_WIN32)
    auto* const library = LoadLibraryA(std::string{library_name}.c_str());
    if (library == nullptr) {
        return false;
    }
    const auto symbol = GetProcAddress(library, std::string{symbol_name}.c_str()) != nullptr;
    FreeLibrary(library);
    return symbol;
#elif defined(__linux__)
    void* library = dlopen(std::string{library_name}.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        return false;
    }
    const auto symbol = dlsym(library, std::string{symbol_name}.c_str()) != nullptr;
    dlclose(library);
    return symbol;
#else
    return false;
#endif
}

[[nodiscard]] std::string_view extension_name_view(std::string_view value) noexcept {
    return value;
}

[[nodiscard]] std::string_view extension_name_view(const std::string& value) noexcept {
    return value;
}

template <typename AvailableExtensions>
[[nodiscard]] bool extension_is_available_in(const AvailableExtensions& available_extensions,
                                             std::string_view extension) noexcept {
    return std::ranges::any_of(available_extensions, [extension](const auto& available) {
        return extension_name_view(available) == extension;
    });
}

[[nodiscard]] bool is_successful_enumeration_result(VulkanResult result) noexcept {
    return result == vulkan_success || result == vulkan_incomplete;
}

[[nodiscard]] bool is_successful_acquire_result(VulkanResult result) noexcept {
    return result == vulkan_success || result == vulkan_suboptimal;
}

[[nodiscard]] VulkanRuntimeGlobalCommands
resolve_runtime_global_commands(VulkanGetInstanceProcAddr get_instance_proc_addr) noexcept {
    VulkanRuntimeGlobalCommands commands;
    commands.get_instance_proc_addr = get_instance_proc_addr;
    if (get_instance_proc_addr == nullptr) {
        return commands;
    }

    commands.enumerate_instance_version =
        reinterpret_cast<VulkanEnumerateInstanceVersion>(get_instance_proc_addr(nullptr, "vkEnumerateInstanceVersion"));
    commands.enumerate_instance_extension_properties = reinterpret_cast<VulkanEnumerateInstanceExtensionProperties>(
        get_instance_proc_addr(nullptr, "vkEnumerateInstanceExtensionProperties"));
    commands.create_instance =
        reinterpret_cast<VulkanCreateInstance>(get_instance_proc_addr(nullptr, "vkCreateInstance"));
    return commands;
}

[[nodiscard]] std::string string_from_fixed_char_array(const char* values, std::size_t capacity) {
    std::size_t size = 0;
    while (size < capacity && values[size] != '\0') {
        ++size;
    }
    return std::string{values, size};
}

[[nodiscard]] std::string extension_name_from_property(const NativeVulkanExtensionProperties& property) {
    return string_from_fixed_char_array(property.extension_name.data(), property.extension_name.size());
}

[[nodiscard]] VulkanPhysicalDeviceType physical_device_type_from_native(std::uint32_t native_type) noexcept {
    switch (native_type) {
    case vulkan_physical_device_type_integrated_gpu:
        return VulkanPhysicalDeviceType::integrated_gpu;
    case vulkan_physical_device_type_discrete_gpu:
        return VulkanPhysicalDeviceType::discrete_gpu;
    case vulkan_physical_device_type_virtual_gpu:
        return VulkanPhysicalDeviceType::virtual_gpu;
    case vulkan_physical_device_type_cpu:
        return VulkanPhysicalDeviceType::cpu;
    case vulkan_physical_device_type_other:
    default:
        return VulkanPhysicalDeviceType::other;
    }
}

[[nodiscard]] std::vector<std::string>
enumerate_instance_extensions(VulkanEnumerateInstanceExtensionProperties enumerate_instance_extension_properties) {
    if (enumerate_instance_extension_properties == nullptr) {
        return {};
    }

    std::uint32_t extension_count = 0;
    auto result = enumerate_instance_extension_properties(nullptr, &extension_count, nullptr);
    if (!is_successful_enumeration_result(result) || extension_count == 0) {
        return {};
    }

    std::vector<NativeVulkanExtensionProperties> properties(extension_count);
    result = enumerate_instance_extension_properties(nullptr, &extension_count, properties.data());
    if (!is_successful_enumeration_result(result)) {
        return {};
    }
    properties.resize(extension_count);

    std::vector<std::string> extensions;
    extensions.reserve(properties.size());
    for (const auto& property : properties) {
        auto extension_name = extension_name_from_property(property);
        if (!extension_name.empty()) {
            extensions.push_back(std::move(extension_name));
        }
    }
    return extensions;
}

[[nodiscard]] VulkanQueueCapability queue_capabilities_from_flags(std::uint32_t queue_flags) noexcept {
    auto capabilities = VulkanQueueCapability::none;
    if ((queue_flags & vulkan_queue_graphics_bit) != 0U) {
        capabilities = capabilities | VulkanQueueCapability::graphics;
    }
    if ((queue_flags & vulkan_queue_compute_bit) != 0U) {
        capabilities = capabilities | VulkanQueueCapability::compute;
    }
    if ((queue_flags & vulkan_queue_transfer_bit) != 0U) {
        capabilities = capabilities | VulkanQueueCapability::transfer;
    }
    return capabilities;
}

[[nodiscard]] std::vector<VulkanQueueFamilyCandidate>
enumerate_queue_families(VulkanGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties,
                         NativeVulkanPhysicalDevice physical_device) {
    if (get_queue_family_properties == nullptr || physical_device == nullptr) {
        return {};
    }

    std::uint32_t queue_family_count = 0;
    get_queue_family_properties(physical_device, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        return {};
    }

    std::vector<NativeVulkanQueueFamilyProperties> properties(queue_family_count);
    get_queue_family_properties(physical_device, &queue_family_count, properties.data());
    properties.resize(queue_family_count);

    std::vector<VulkanQueueFamilyCandidate> queue_families;
    queue_families.reserve(properties.size());
    for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(properties.size()); ++index) {
        const auto& property = properties[index];
        if (property.queue_count == 0) {
            continue;
        }
        queue_families.push_back(VulkanQueueFamilyCandidate{
            .index = index,
            .queue_count = property.queue_count,
            .capabilities = queue_capabilities_from_flags(property.queue_flags),
            .supports_present = false,
        });
    }
    return queue_families;
}

[[nodiscard]] std::vector<std::string>
enumerate_device_extensions(VulkanEnumerateDeviceExtensionProperties enumerate_device_extension_properties,
                            NativeVulkanPhysicalDevice physical_device) {
    if (enumerate_device_extension_properties == nullptr || physical_device == nullptr) {
        return {};
    }

    std::uint32_t extension_count = 0;
    auto result = enumerate_device_extension_properties(physical_device, nullptr, &extension_count, nullptr);
    if (!is_successful_enumeration_result(result) || extension_count == 0) {
        return {};
    }

    std::vector<NativeVulkanExtensionProperties> properties(extension_count);
    result = enumerate_device_extension_properties(physical_device, nullptr, &extension_count, properties.data());
    if (!is_successful_enumeration_result(result)) {
        return {};
    }
    properties.resize(extension_count);

    std::vector<std::string> extensions;
    extensions.reserve(properties.size());
    for (const auto& property : properties) {
        auto extension_name = extension_name_from_property(property);
        if (!extension_name.empty()) {
            extensions.push_back(std::move(extension_name));
        }
    }
    return extensions;
}

[[nodiscard]] Vulkan13FeatureSupport
query_vulkan13_feature_support(VulkanGetPhysicalDeviceFeatures2 get_physical_device_features2,
                               NativeVulkanPhysicalDevice physical_device) {
    Vulkan13FeatureSupport support;
    if (get_physical_device_features2 == nullptr || physical_device == nullptr) {
        return support;
    }

    NativeVulkanPhysicalDeviceVulkan13Features vulkan13_features{
        .s_type = vulkan_structure_type_physical_device_vulkan_1_3_features,
        .next = nullptr,
        .robust_image_access = 0,
        .inline_uniform_block = 0,
        .descriptor_binding_inline_uniform_block_update_after_bind = 0,
        .pipeline_creation_cache_control = 0,
        .private_data = 0,
        .shader_demote_to_helper_invocation = 0,
        .shader_terminate_invocation = 0,
        .subgroup_size_control = 0,
        .compute_full_subgroups = 0,
        .synchronization2 = 0,
        .texture_compression_astc_hdr = 0,
        .shader_zero_initialize_workgroup_memory = 0,
        .dynamic_rendering = 0,
        .shader_integer_dot_product = 0,
        .maintenance4 = 0,
    };
    NativeVulkanPhysicalDeviceFeatures2 features{
        .s_type = vulkan_structure_type_physical_device_features_2,
        .next = &vulkan13_features,
        .features = {},
    };

    get_physical_device_features2(physical_device, &features);
    support.dynamic_rendering = vulkan13_features.dynamic_rendering != 0U;
    support.synchronization2 = vulkan13_features.synchronization2 != 0U;
    return support;
}

[[nodiscard]] RuntimePhysicalDeviceProperties
query_physical_device_properties(VulkanGetPhysicalDeviceProperties2 get_physical_device_properties2,
                                 NativeVulkanPhysicalDevice physical_device) {
    RuntimePhysicalDeviceProperties result;
    if (get_physical_device_properties2 == nullptr || physical_device == nullptr) {
        return result;
    }

    NativeVulkanPhysicalDeviceProperties2 properties{};
    properties.s_type = vulkan_structure_type_physical_device_properties_2;
    get_physical_device_properties2(physical_device, &properties);

    result.name =
        string_from_fixed_char_array(properties.properties.device_name.data(), vulkan_max_physical_device_name_size);
    result.type = physical_device_type_from_native(properties.properties.device_type);
    result.api_version = decode_vulkan_api_version(properties.properties.api_version);
    result.driver_version = properties.properties.driver_version;
    result.vendor_id = properties.properties.vendor_id;
    result.device_id = properties.properties.device_id;
    return result;
}

[[nodiscard]] VulkanRuntimePhysicalDeviceSnapshot
make_runtime_physical_device_snapshot(std::size_t device_index, NativeVulkanPhysicalDevice physical_device,
                                      VulkanGetPhysicalDeviceProperties2 get_physical_device_properties2,
                                      VulkanGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties,
                                      VulkanEnumerateDeviceExtensionProperties enumerate_device_extension_properties,
                                      VulkanGetPhysicalDeviceFeatures2 get_physical_device_features2) {
    auto properties = query_physical_device_properties(get_physical_device_properties2, physical_device);
    auto device_extensions = enumerate_device_extensions(enumerate_device_extension_properties, physical_device);
    const auto vulkan13_features = query_vulkan13_feature_support(get_physical_device_features2, physical_device);
    const auto supports_swapchain_extension =
        extension_is_available_in(device_extensions, std::string_view{"VK_KHR_swapchain"});

    return VulkanRuntimePhysicalDeviceSnapshot{
        .device_index = device_index,
        .name = std::move(properties.name),
        .type = properties.type,
        .api_version = properties.api_version,
        .driver_version = properties.driver_version,
        .vendor_id = properties.vendor_id,
        .device_id = properties.device_id,
        .queue_families = enumerate_queue_families(get_queue_family_properties, physical_device),
        .device_extensions = std::move(device_extensions),
        .supports_swapchain_extension = supports_swapchain_extension,
        .supports_dynamic_rendering = vulkan13_features.dynamic_rendering,
        .supports_synchronization2 = vulkan13_features.synchronization2,
    };
}

void refresh_surface_probe_queue_family_snapshots(
    VulkanRuntimePhysicalDeviceSnapshotProbeResult& snapshots,
    const std::vector<NativeVulkanPhysicalDevice>& physical_devices,
    VulkanGetPhysicalDeviceProperties2 get_physical_device_properties2,
    VulkanGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties,
    VulkanEnumerateDeviceExtensionProperties enumerate_device_extension_properties,
    VulkanGetPhysicalDeviceFeatures2 get_physical_device_features2) {
    snapshots.devices.clear();
    snapshots.devices.reserve(physical_devices.size());
    snapshots.count_probe.physical_device_count = static_cast<std::uint32_t>(physical_devices.size());
    for (std::size_t device_index = 0; device_index < physical_devices.size(); ++device_index) {
        snapshots.devices.push_back(make_runtime_physical_device_snapshot(
            device_index, physical_devices[device_index], get_physical_device_properties2, get_queue_family_properties,
            enumerate_device_extension_properties, get_physical_device_features2));
    }
}

[[nodiscard]] bool extension_is_enabled_in_plan(const VulkanInstanceCreatePlan& plan,
                                                std::string_view extension) noexcept {
    return std::ranges::any_of(plan.enabled_extensions,
                               [extension](const auto& enabled) { return enabled == extension; });
}

[[nodiscard]] std::vector<VulkanCommandRequest>
instance_command_requests_for_plan(const VulkanInstanceCreatePlan& plan) {
    std::vector<VulkanCommandRequest> requests;
    for (const auto& request : vulkan_backend_command_requests()) {
        if (request.scope != VulkanCommandScope::instance) {
            continue;
        }
        if (request.name == "vkGetPhysicalDeviceSurfaceSupportKHR" &&
            !extension_is_enabled_in_plan(plan, "VK_KHR_surface")) {
            continue;
        }
        requests.push_back(request);
    }
    return requests;
}

[[nodiscard]] std::vector<const char*> extension_name_pointers(const std::vector<std::string>& extensions) {
    std::vector<const char*> pointers;
    pointers.reserve(extensions.size());
    for (const auto& extension : extensions) {
        pointers.push_back(extension.c_str());
    }
    return pointers;
}

[[nodiscard]] NativeVulkanApplicationInfo make_native_application_info(const VulkanInstanceCreateDesc& desc) noexcept {
    return NativeVulkanApplicationInfo{
        .s_type = vulkan_structure_type_application_info,
        .next = nullptr,
        .application_name = desc.application_name.c_str(),
        .application_version = 0,
        .engine_name = "GameEngine",
        .engine_version = 0,
        .api_version = encode_vulkan_api_version(desc.api_version),
    };
}

[[nodiscard]] NativeVulkanInstanceCreateInfo
make_native_instance_create_info(const NativeVulkanApplicationInfo& application_info,
                                 const std::vector<const char*>& enabled_extensions) noexcept {
    return NativeVulkanInstanceCreateInfo{
        .s_type = vulkan_structure_type_instance_create_info,
        .next = nullptr,
        .flags = 0,
        .application_info = &application_info,
        .enabled_layer_count = 0,
        .enabled_layer_names = nullptr,
        .enabled_extension_count = static_cast<std::uint32_t>(enabled_extensions.size()),
        .enabled_extension_names = enabled_extensions.empty() ? nullptr : enabled_extensions.data(),
    };
}

[[nodiscard]] std::vector<NativeVulkanDeviceQueueCreateInfo>
make_native_device_queue_create_infos(const std::vector<VulkanDeviceQueueCreatePlan>& queue_plans,
                                      const std::vector<float>& queue_priorities) {
    std::vector<NativeVulkanDeviceQueueCreateInfo> queue_infos;
    queue_infos.reserve(queue_plans.size());
    for (std::size_t index = 0; index < queue_plans.size(); ++index) {
        queue_infos.push_back(NativeVulkanDeviceQueueCreateInfo{
            .s_type = vulkan_structure_type_device_queue_create_info,
            .next = nullptr,
            .flags = 0,
            .queue_family_index = queue_plans[index].queue_family,
            .queue_count = queue_plans[index].queue_count,
            .queue_priorities = &queue_priorities[index],
        });
    }
    return queue_infos;
}

[[nodiscard]] NativeVulkanDeviceCreateInfo
make_native_device_create_info(const std::vector<NativeVulkanDeviceQueueCreateInfo>& queue_infos,
                               const std::vector<const char*>& enabled_extensions, const void* feature_chain) noexcept {
    return NativeVulkanDeviceCreateInfo{
        .s_type = vulkan_structure_type_device_create_info,
        .next = feature_chain,
        .flags = 0,
        .queue_create_info_count = static_cast<std::uint32_t>(queue_infos.size()),
        .queue_create_infos = queue_infos.empty() ? nullptr : queue_infos.data(),
        .enabled_layer_count = 0,
        .enabled_layer_names = nullptr,
        .enabled_extension_count = static_cast<std::uint32_t>(enabled_extensions.size()),
        .enabled_extension_names = enabled_extensions.empty() ? nullptr : enabled_extensions.data(),
        .enabled_features = nullptr,
    };
}

[[nodiscard]] NativeVulkanPhysicalDeviceVulkan13Features
make_native_vulkan13_features(const VulkanLogicalDeviceCreatePlan& plan) noexcept {
    return NativeVulkanPhysicalDeviceVulkan13Features{
        .s_type = vulkan_structure_type_physical_device_vulkan_1_3_features,
        .next = nullptr,
        .robust_image_access = 0,
        .inline_uniform_block = 0,
        .descriptor_binding_inline_uniform_block_update_after_bind = 0,
        .pipeline_creation_cache_control = 0,
        .private_data = 0,
        .shader_demote_to_helper_invocation = 0,
        .shader_terminate_invocation = 0,
        .subgroup_size_control = 0,
        .compute_full_subgroups = 0,
        .synchronization2 = plan.synchronization2_enabled ? 1U : 0U,
        .texture_compression_astc_hdr = 0,
        .shader_zero_initialize_workgroup_memory = 0,
        .dynamic_rendering = plan.dynamic_rendering_enabled ? 1U : 0U,
        .shader_integer_dot_product = 0,
        .maintenance4 = 0,
    };
}

void append_instance_command_availability(std::vector<VulkanCommandAvailability>& availability,
                                          VulkanGetInstanceProcAddr get_instance_proc_addr,
                                          NativeVulkanInstance instance,
                                          const std::vector<VulkanCommandRequest>& requests) {
    for (const auto& request : requests) {
        const auto resolved =
            get_instance_proc_addr != nullptr && get_instance_proc_addr(instance, request.name.c_str()) != nullptr;
        availability.push_back(
            VulkanCommandAvailability{.name = request.name, .scope = request.scope, .available = resolved});
    }
}

void append_device_command_availability(std::vector<VulkanCommandAvailability>& availability,
                                        VulkanGetDeviceProcAddr get_device_proc_addr, NativeVulkanDevice device,
                                        const std::vector<VulkanCommandRequest>& requests) {
    for (const auto& request : requests) {
        const auto resolved =
            get_device_proc_addr != nullptr && get_device_proc_addr(device, request.name.c_str()) != nullptr;
        availability.push_back(
            VulkanCommandAvailability{.name = request.name, .scope = request.scope, .available = resolved});
    }
}

[[nodiscard]] std::string vulkan_result_diagnostic(std::string_view prefix, VulkanResult result) {
    return std::string{prefix} + ": " + std::to_string(result);
}

[[nodiscard]] VulkanInstanceCreatePlan make_runtime_instance_failure(const VulkanInstanceCreateDesc& desc,
                                                                     std::string_view diagnostic) {
    return VulkanInstanceCreatePlan{
        .supported = false,
        .api_version = desc.api_version,
        .enabled_extensions = {},
        .validation_enabled = false,
        .diagnostic = std::string{diagnostic},
    };
}

[[nodiscard]] bool is_valid_queue(const VulkanQueueFamilyCandidate& queue) noexcept {
    return queue.index != invalid_vulkan_queue_family && queue.queue_count > 0;
}

[[nodiscard]] QueueFamilySelection select_queue_families(const VulkanPhysicalDeviceCandidate& device) noexcept {
    QueueFamilySelection selection;

    for (const auto& queue : device.queue_families) {
        if (!is_valid_queue(queue)) {
            continue;
        }
        const bool graphics_compute = has_queue_capability(queue.capabilities, VulkanQueueCapability::graphics) &&
                                      has_queue_capability(queue.capabilities, VulkanQueueCapability::compute);
        if (graphics_compute && queue.supports_present) {
            return QueueFamilySelection{.graphics = queue.index, .present = queue.index};
        }
        if (selection.graphics == invalid_vulkan_queue_family && graphics_compute) {
            selection.graphics = queue.index;
        }
        if (selection.present == invalid_vulkan_queue_family && queue.supports_present) {
            selection.present = queue.index;
        }
    }

    return selection;
}

[[nodiscard]] bool has_required_queues(const QueueFamilySelection& selection) noexcept {
    return selection.graphics != invalid_vulkan_queue_family && selection.present != invalid_vulkan_queue_family;
}

[[nodiscard]] int device_type_score(VulkanPhysicalDeviceType type) noexcept {
    switch (type) {
    case VulkanPhysicalDeviceType::discrete_gpu:
        return 1000;
    case VulkanPhysicalDeviceType::integrated_gpu:
        return 500;
    case VulkanPhysicalDeviceType::virtual_gpu:
        return 250;
    case VulkanPhysicalDeviceType::cpu:
        return 100;
    case VulkanPhysicalDeviceType::other:
        return 10;
    }
    return 0;
}

[[nodiscard]] int score_device(const VulkanPhysicalDeviceCandidate& device) noexcept {
    auto score = device_type_score(device.type);
    score += static_cast<int>((device.api_version.major * 100) + device.api_version.minor);
    score += device.supports_swapchain_extension ? 50 : 0;
    score += device.supports_dynamic_rendering ? 50 : 0;
    score += device.supports_synchronization2 ? 50 : 0;
    return score;
}

[[nodiscard]] bool is_suitable_device(const VulkanPhysicalDeviceCandidate& device,
                                      const QueueFamilySelection& queues) noexcept {
    return is_vulkan_api_at_least(device.api_version, VulkanApiVersion{.major = 1, .minor = 3}) &&
           device.supports_swapchain_extension && device.supports_dynamic_rendering &&
           device.supports_synchronization2 && has_required_queues(queues);
}

template <typename Devices> [[nodiscard]] VulkanDeviceSelection select_physical_device_impl(const Devices& devices) {
    VulkanDeviceSelection best{
        .suitable = false,
        .device_index = invalid_vulkan_device_index,
        .graphics_queue_family = invalid_vulkan_queue_family,
        .present_queue_family = invalid_vulkan_queue_family,
        .score = 0,
        .diagnostic = "no suitable Vulkan physical device",
    };

    std::size_t index = 0;
    for (const auto& device : devices) {
        const auto queues = select_queue_families(device);
        if (!is_suitable_device(device, queues)) {
            ++index;
            continue;
        }

        const auto score = score_device(device);
        if (!best.suitable || score > best.score) {
            best = VulkanDeviceSelection{
                true, index, queues.graphics, queues.present, score, "selected " + device.name,
            };
        }
        ++index;
    }

    return best;
}

[[nodiscard]] bool extension_is_enabled(const std::vector<std::string>& enabled_extensions,
                                        std::string_view extension) noexcept {
    return std::ranges::any_of(enabled_extensions, [extension](const auto& enabled) { return enabled == extension; });
}

void append_extension_once(std::vector<std::string>& enabled_extensions, std::string_view extension) {
    if (extension.empty() || extension_is_enabled(enabled_extensions, extension)) {
        return;
    }
    enabled_extensions.emplace_back(extension);
}

[[nodiscard]] VulkanInstanceCreateDesc make_surface_instance_desc(VulkanInstanceCreateDesc desc, RhiHostPlatform host) {
    for (const auto& extension : vulkan_surface_instance_extensions(host)) {
        append_extension_once(desc.required_extensions, extension);
    }
    return desc;
}

[[nodiscard]] bool queue_family_is_planned(const std::vector<VulkanDeviceQueueCreatePlan>& queue_families,
                                           std::uint32_t queue_family) noexcept {
    return std::ranges::any_of(queue_families,
                               [queue_family](const auto& planned) { return planned.queue_family == queue_family; });
}

void append_queue_family_once(std::vector<VulkanDeviceQueueCreatePlan>& queue_families, std::uint32_t queue_family) {
    if (queue_family == invalid_vulkan_queue_family || queue_family_is_planned(queue_families, queue_family)) {
        return;
    }

    queue_families.push_back(VulkanDeviceQueueCreatePlan{
        .queue_family = queue_family,
        .queue_count = 1,
        .priority = 1.0F,
    });
}

template <typename AvailableDeviceExtensions>
[[nodiscard]] VulkanLogicalDeviceCreatePlan build_logical_device_create_plan_impl(
    const VulkanLogicalDeviceCreateDesc& desc, const VulkanPhysicalDeviceCandidate& device,
    const VulkanDeviceSelection& selection, const AvailableDeviceExtensions& available_device_extensions) {
    VulkanLogicalDeviceCreatePlan plan{
        .supported = false,
        .queue_families = {},
        .enabled_extensions = {},
        .dynamic_rendering_enabled = false,
        .synchronization2_enabled = false,
        .diagnostic = {},
    };

    if (!selection.suitable) {
        plan.diagnostic = "Vulkan device selection is not suitable";
        return plan;
    }

    append_queue_family_once(plan.queue_families, selection.graphics_queue_family);
    append_queue_family_once(plan.queue_families, selection.present_queue_family);

    if (plan.queue_families.empty()) {
        plan.diagnostic = "Vulkan logical device requires graphics and present queues";
        return plan;
    }

    for (const auto& required : desc.required_extensions) {
        if (required.empty()) {
            continue;
        }
        if (required == "VK_KHR_swapchain" && !device.supports_swapchain_extension) {
            plan.diagnostic = "Vulkan swapchain extension support is required";
            return plan;
        }
        if (!extension_is_available_in(available_device_extensions, required)) {
            plan.diagnostic = "missing required Vulkan device extension: " + required;
            return plan;
        }
        append_extension_once(plan.enabled_extensions, required);
    }

    if (desc.require_dynamic_rendering && !device.supports_dynamic_rendering) {
        plan.diagnostic = "Vulkan dynamic rendering feature is required";
        return plan;
    }
    if (desc.require_synchronization2 && !device.supports_synchronization2) {
        plan.diagnostic = "Vulkan synchronization2 feature is required";
        return plan;
    }

    for (const auto& optional : desc.optional_extensions) {
        if (extension_is_available_in(available_device_extensions, optional)) {
            append_extension_once(plan.enabled_extensions, optional);
        }
    }

    plan.supported = true;
    plan.dynamic_rendering_enabled = desc.require_dynamic_rendering;
    plan.synchronization2_enabled = desc.require_synchronization2;
    plan.diagnostic = "Vulkan logical device create plan ready";
    return plan;
}

[[nodiscard]] bool valid_extent(Extent2D extent) noexcept {
    return extent.width > 0 && extent.height > 0;
}

[[nodiscard]] bool valid_sampler_desc(SamplerDesc desc) noexcept {
    const auto valid_filter = [](SamplerFilter filter) noexcept {
        return filter == SamplerFilter::nearest || filter == SamplerFilter::linear;
    };
    const auto valid_address = [](SamplerAddressMode mode) noexcept {
        return mode == SamplerAddressMode::repeat || mode == SamplerAddressMode::clamp_to_edge;
    };
    return valid_filter(desc.min_filter) && valid_filter(desc.mag_filter) && valid_address(desc.address_u) &&
           valid_address(desc.address_v) && valid_address(desc.address_w);
}

[[nodiscard]] std::uint32_t native_vulkan_filter(SamplerFilter filter) noexcept {
    switch (filter) {
    case SamplerFilter::nearest:
        return vulkan_filter_nearest;
    case SamplerFilter::linear:
        return vulkan_filter_linear;
    }
    return vulkan_filter_nearest;
}

[[nodiscard]] std::uint32_t native_vulkan_address_mode(SamplerAddressMode mode) noexcept {
    switch (mode) {
    case SamplerAddressMode::repeat:
        return vulkan_sampler_address_mode_repeat;
    case SamplerAddressMode::clamp_to_edge:
        return vulkan_sampler_address_mode_clamp_to_edge;
    }
    return vulkan_sampler_address_mode_clamp_to_edge;
}

[[nodiscard]] bool calculate_readback_byte_count(Extent2D extent, std::uint32_t bytes_per_pixel,
                                                 std::uint64_t& byte_count) noexcept {
    if (!valid_extent(extent) || bytes_per_pixel == 0) {
        byte_count = 0;
        return false;
    }

    constexpr auto max_size = std::numeric_limits<std::uint64_t>::max();
    const auto width = static_cast<std::uint64_t>(extent.width);
    const auto height = static_cast<std::uint64_t>(extent.height);
    if (height > max_size / width) {
        byte_count = 0;
        return false;
    }
    const auto pixel_count = width * height;
    if (static_cast<std::uint64_t>(bytes_per_pixel) > max_size / pixel_count) {
        byte_count = 0;
        return false;
    }
    byte_count = pixel_count * static_cast<std::uint64_t>(bytes_per_pixel);
    return true;
}

[[nodiscard]] std::uint32_t find_memory_type_index(const NativeVulkanPhysicalDeviceMemoryProperties& properties,
                                                   std::uint32_t memory_type_bits,
                                                   std::uint32_t required_flags) noexcept {
    const auto memory_type_count =
        std::min<std::uint32_t>(properties.memory_type_count, static_cast<std::uint32_t>(vulkan_max_memory_types));
    std::uint32_t index = 0;
    for (const auto& memory_type :
         std::span{properties.memory_types.data(), static_cast<std::size_t>(memory_type_count)}) {
        const auto type_mask = 1U << index;
        const auto allowed = (memory_type_bits & type_mask) != 0;
        const auto flags = memory_type.property_flags;
        if (allowed && (flags & required_flags) == required_flags) {
            return index;
        }
        ++index;
    }
    return invalid_vulkan_queue_family;
}

[[nodiscard]] std::uint32_t native_vulkan_buffer_usage_flags(const VulkanBufferUsagePlan& usage) noexcept {
    std::uint32_t flags = 0;
    if (usage.transfer_source) {
        flags |= vulkan_buffer_usage_transfer_src_bit;
    }
    if (usage.transfer_destination) {
        flags |= vulkan_buffer_usage_transfer_dst_bit;
    }
    if (usage.vertex) {
        flags |= vulkan_buffer_usage_vertex_buffer_bit;
    }
    if (usage.index) {
        flags |= vulkan_buffer_usage_index_buffer_bit;
    }
    if (usage.uniform) {
        flags |= vulkan_buffer_usage_uniform_buffer_bit;
    }
    if (usage.storage) {
        flags |= vulkan_buffer_usage_storage_buffer_bit;
    }
    return flags;
}

[[nodiscard]] std::uint32_t native_vulkan_index_type(IndexFormat format) {
    switch (format) {
    case IndexFormat::uint16:
        return vulkan_index_type_uint16;
    case IndexFormat::uint32:
        return vulkan_index_type_uint32;
    case IndexFormat::unknown:
        break;
    }
    throw std::invalid_argument("Vulkan index format is unsupported");
}

[[nodiscard]] std::uint32_t native_vulkan_texture_usage_flags(const VulkanTextureUsagePlan& usage) noexcept {
    std::uint32_t flags = 0;
    if (usage.transfer_source) {
        flags |= vulkan_image_usage_transfer_src_bit;
    }
    if (usage.transfer_destination) {
        flags |= vulkan_image_usage_transfer_dst_bit;
    }
    if (usage.sampled) {
        flags |= vulkan_image_usage_sampled_bit;
    }
    if (usage.storage) {
        flags |= vulkan_image_usage_storage_bit;
    }
    if (usage.color_attachment) {
        flags |= vulkan_image_usage_color_attachment_bit;
    }
    if (usage.depth_stencil_attachment) {
        flags |= vulkan_image_usage_depth_stencil_attachment_bit;
    }
    return flags;
}

[[nodiscard]] std::uint32_t native_vulkan_memory_property_flags(const VulkanBufferMemoryPlan& memory) noexcept {
    std::uint32_t flags = 0;
    if (memory.device_local) {
        flags |= vulkan_memory_property_device_local_bit;
    }
    if (memory.host_visible) {
        flags |= vulkan_memory_property_host_visible_bit;
    }
    if (memory.host_coherent) {
        flags |= vulkan_memory_property_host_coherent_bit;
    }
    return flags;
}

[[nodiscard]] Extent2D clamp_extent(Extent2D requested, Extent2D minimum, Extent2D maximum) noexcept {
    if (!valid_extent(minimum) || !valid_extent(maximum)) {
        return requested;
    }

    return Extent2D{
        .width = std::clamp(requested.width, minimum.width, maximum.width),
        .height = std::clamp(requested.height, minimum.height, maximum.height),
    };
}

[[nodiscard]] bool surface_format_supported(const VulkanSurfaceFormatCandidate& candidate) noexcept {
    return candidate.format == Format::rgba8_unorm || candidate.format == Format::bgra8_unorm;
}

[[nodiscard]] Format format_from_native_vulkan_format(std::uint32_t format) noexcept {
    switch (format) {
    case vulkan_format_r8g8b8a8_unorm:
        return Format::rgba8_unorm;
    case vulkan_format_b8g8r8a8_unorm:
        return Format::bgra8_unorm;
    default:
        return Format::unknown;
    }
}

[[nodiscard]] bool dynamic_rendering_color_format_supported(Format format) noexcept {
    return format == Format::rgba8_unorm || format == Format::bgra8_unorm;
}

[[nodiscard]] bool dynamic_rendering_depth_format_supported(Format format) noexcept {
    return format == Format::depth24_stencil8;
}

[[nodiscard]] bool valid_rhi_compare_op(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::never:
    case CompareOp::less:
    case CompareOp::equal:
    case CompareOp::less_equal:
    case CompareOp::greater:
    case CompareOp::not_equal:
    case CompareOp::greater_equal:
    case CompareOp::always:
        return true;
    }
    return false;
}

[[nodiscard]] bool depth_format_supported(Format format) noexcept {
    return format == Format::depth24_stencil8;
}

[[nodiscard]] bool valid_depth_state_for_format(Format depth_format, DepthStencilStateDesc depth_state) noexcept {
    if (!valid_rhi_compare_op(depth_state.depth_compare)) {
        return false;
    }
    if (depth_state.depth_write_enabled && !depth_state.depth_test_enabled) {
        return false;
    }
    if (depth_format == Format::unknown) {
        return !depth_state.depth_test_enabled && !depth_state.depth_write_enabled;
    }
    if (!depth_format_supported(depth_format)) {
        return false;
    }
    return true;
}

[[nodiscard]] std::uint32_t native_vulkan_format(Format format) noexcept {
    switch (format) {
    case Format::rgba8_unorm:
        return vulkan_format_r8g8b8a8_unorm;
    case Format::bgra8_unorm:
        return vulkan_format_b8g8r8a8_unorm;
    case Format::depth24_stencil8:
        return vulkan_format_d24_unorm_s8_uint;
    case Format::unknown:
        return 0;
    }
    return 0;
}

[[nodiscard]] std::uint32_t native_vulkan_image_aspect_flags(Format format) noexcept {
    return depth_format_supported(format) ? vulkan_image_aspect_depth_bit : vulkan_image_aspect_color_bit;
}

[[nodiscard]] std::uint32_t native_vulkan_image_barrier_aspect_flags(Format format) noexcept {
    if (format == Format::depth24_stencil8) {
        return vulkan_image_aspect_depth_bit | vulkan_image_aspect_stencil_bit;
    }
    return native_vulkan_image_aspect_flags(format);
}

[[nodiscard]] std::uint32_t native_vulkan_compare_op(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::never:
        return vulkan_compare_op_never;
    case CompareOp::less:
        return vulkan_compare_op_less;
    case CompareOp::equal:
        return vulkan_compare_op_equal;
    case CompareOp::less_equal:
        return vulkan_compare_op_less_or_equal;
    case CompareOp::greater:
        return vulkan_compare_op_greater;
    case CompareOp::not_equal:
        return vulkan_compare_op_not_equal;
    case CompareOp::greater_equal:
        return vulkan_compare_op_greater_or_equal;
    case CompareOp::always:
        return vulkan_compare_op_always;
    }
    return vulkan_compare_op_less_or_equal;
}

[[nodiscard]] std::uint32_t native_vulkan_vertex_format(VertexFormat format) noexcept {
    switch (format) {
    case VertexFormat::float32x2:
        return vulkan_format_r32g32_sfloat;
    case VertexFormat::float32x3:
        return vulkan_format_r32g32b32_sfloat;
    case VertexFormat::float32x4:
        return vulkan_format_r32g32b32a32_sfloat;
    case VertexFormat::uint16x4:
        return vulkan_format_r16g16b16a16_uint;
    case VertexFormat::unknown:
        return 0;
    }
    return 0;
}

[[nodiscard]] std::uint32_t vertex_format_size(VertexFormat format) noexcept {
    switch (format) {
    case VertexFormat::float32x2:
        return 8;
    case VertexFormat::float32x3:
        return 12;
    case VertexFormat::float32x4:
        return 16;
    case VertexFormat::uint16x4:
        return 8;
    case VertexFormat::unknown:
        return 0;
    }
    return 0;
}

[[nodiscard]] std::uint32_t native_vulkan_input_rate(VertexInputRate rate) noexcept {
    return rate == VertexInputRate::instance ? vulkan_vertex_input_rate_instance : vulkan_vertex_input_rate_vertex;
}

[[nodiscard]] bool valid_vertex_input_desc(const std::vector<VertexBufferLayoutDesc>& vertex_buffers,
                                           const std::vector<VertexAttributeDesc>& vertex_attributes) noexcept {
    for (const auto& layout : vertex_buffers) {
        if (layout.stride == 0) {
            return false;
        }
        const auto duplicates =
            std::ranges::count_if(vertex_buffers, [&layout](const VertexBufferLayoutDesc& candidate) {
                return candidate.binding == layout.binding;
            });
        if (duplicates > 1) {
            return false;
        }
    }

    for (const auto& attribute : vertex_attributes) {
        if (native_vulkan_vertex_format(attribute.format) == 0) {
            return false;
        }
        const auto layout = std::ranges::find_if(vertex_buffers, [&attribute](const VertexBufferLayoutDesc& candidate) {
            return candidate.binding == attribute.binding;
        });
        if (layout == vertex_buffers.end()) {
            return false;
        }
        const auto duplicates =
            std::ranges::count_if(vertex_attributes, [&attribute](const VertexAttributeDesc& candidate) {
                return candidate.location == attribute.location;
            });
        if (duplicates > 1) {
            return false;
        }
        const auto size = vertex_format_size(attribute.format);
        if (size == 0 || attribute.offset > layout->stride || size > layout->stride - attribute.offset) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::uint32_t native_vulkan_topology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::triangle_list:
        return vulkan_primitive_topology_triangle_list;
    case PrimitiveTopology::line_list:
        return vulkan_primitive_topology_line_list;
    }
    return vulkan_primitive_topology_triangle_list;
}

[[nodiscard]] std::uint32_t native_vulkan_descriptor_type(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::uniform_buffer:
        return vulkan_descriptor_type_uniform_buffer;
    case DescriptorType::storage_buffer:
        return vulkan_descriptor_type_storage_buffer;
    case DescriptorType::sampled_texture:
        return vulkan_descriptor_type_sampled_image;
    case DescriptorType::storage_texture:
        return vulkan_descriptor_type_storage_image;
    case DescriptorType::sampler:
        return vulkan_descriptor_type_sampler;
    }
    return vulkan_descriptor_type_uniform_buffer;
}

[[nodiscard]] std::uint32_t native_vulkan_shader_stage_flags(ShaderStageVisibility visibility) noexcept {
    std::uint32_t flags = 0;
    if ((static_cast<std::uint32_t>(visibility) & static_cast<std::uint32_t>(ShaderStageVisibility::vertex)) != 0U) {
        flags |= vulkan_shader_stage_vertex_bit;
    }
    if ((static_cast<std::uint32_t>(visibility) & static_cast<std::uint32_t>(ShaderStageVisibility::fragment)) != 0U) {
        flags |= vulkan_shader_stage_fragment_bit;
    }
    if ((static_cast<std::uint32_t>(visibility) & static_cast<std::uint32_t>(ShaderStageVisibility::compute)) != 0U) {
        flags |= vulkan_shader_stage_compute_bit;
    }
    return flags;
}

[[nodiscard]] bool descriptor_bindings_have_duplicate_numbers(const DescriptorSetLayoutDesc& desc) {
    std::vector<std::uint32_t> bindings;
    bindings.reserve(desc.bindings.size());
    for (const auto& binding : desc.bindings) {
        if (std::ranges::find(bindings, binding.binding) != bindings.end()) {
            return true;
        }
        bindings.push_back(binding.binding);
    }
    return false;
}

[[nodiscard]] const DescriptorBindingDesc* find_descriptor_binding(const DescriptorSetLayoutDesc& desc,
                                                                   std::uint32_t binding) noexcept {
    const auto found = std::ranges::find_if(
        desc.bindings, [binding](const DescriptorBindingDesc& candidate) { return candidate.binding == binding; });
    return found == desc.bindings.end() ? nullptr : &*found;
}

[[nodiscard]] std::uint32_t native_vulkan_load_op(LoadAction action) noexcept {
    switch (action) {
    case LoadAction::load:
        return vulkan_attachment_load_op_load;
    case LoadAction::clear:
        return vulkan_attachment_load_op_clear;
    case LoadAction::dont_care:
        return vulkan_attachment_load_op_dont_care;
    }
    return vulkan_attachment_load_op_clear;
}

[[nodiscard]] std::uint32_t native_vulkan_store_op(StoreAction action) noexcept {
    switch (action) {
    case StoreAction::store:
        return vulkan_attachment_store_op_store;
    case StoreAction::dont_care:
        return vulkan_attachment_store_op_dont_care;
    }
    return vulkan_attachment_store_op_store;
}

[[nodiscard]] NativeVulkanClearValue native_vulkan_clear_color(ClearColorValue color) noexcept {
    NativeVulkanClearValue clear{};
    clear.color = NativeVulkanClearColorValue{
        color.red,
        color.green,
        color.blue,
        color.alpha,
    };
    return clear;
}

[[nodiscard]] NativeVulkanClearValue native_vulkan_clear_depth(ClearDepthValue depth) noexcept {
    NativeVulkanClearValue clear{};
    clear.depth_stencil = NativeVulkanClearDepthStencilValue{.depth = depth.depth, .stencil = 0};
    return clear;
}

[[nodiscard]] bool valid_clear_depth(ClearDepthValue depth) noexcept {
    return std::isfinite(depth.depth) && depth.depth >= 0.0F && depth.depth <= 1.0F;
}

[[nodiscard]] std::uint64_t native_vulkan_stage(VulkanSynchronizationStage stage) noexcept {
    switch (stage) {
    case VulkanSynchronizationStage::none:
        return vulkan_pipeline_stage2_none;
    case VulkanSynchronizationStage::color_attachment_output:
        return vulkan_pipeline_stage2_color_attachment_output_bit;
    case VulkanSynchronizationStage::depth_attachment:
        return vulkan_pipeline_stage2_early_fragment_tests_bit | vulkan_pipeline_stage2_late_fragment_tests_bit;
    case VulkanSynchronizationStage::transfer:
        return vulkan_pipeline_stage2_transfer_bit;
    case VulkanSynchronizationStage::shader:
        return vulkan_pipeline_stage2_fragment_shader_bit;
    }
    return vulkan_pipeline_stage2_none;
}

[[nodiscard]] std::uint64_t native_vulkan_access(VulkanSynchronizationAccess access) noexcept {
    switch (access) {
    case VulkanSynchronizationAccess::none:
        return vulkan_access2_none;
    case VulkanSynchronizationAccess::color_attachment_write:
        return vulkan_access2_color_attachment_write_bit;
    case VulkanSynchronizationAccess::depth_attachment_read_write:
        return vulkan_access2_depth_stencil_attachment_read_bit | vulkan_access2_depth_stencil_attachment_write_bit;
    case VulkanSynchronizationAccess::transfer_read:
        return vulkan_access2_transfer_read_bit;
    case VulkanSynchronizationAccess::transfer_write:
        return vulkan_access2_transfer_write_bit;
    case VulkanSynchronizationAccess::shader_read:
        return vulkan_access2_shader_read_bit;
    }
    return vulkan_access2_none;
}

[[nodiscard]] std::uint32_t native_vulkan_image_layout(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::undefined:
        return vulkan_image_layout_undefined;
    case ResourceState::render_target:
        return vulkan_image_layout_color_attachment_optimal;
    case ResourceState::copy_source:
        return vulkan_image_layout_transfer_src_optimal;
    case ResourceState::copy_destination:
        return vulkan_image_layout_transfer_dst_optimal;
    case ResourceState::shader_read:
        return vulkan_image_layout_shader_read_only_optimal;
    case ResourceState::present:
        return vulkan_image_layout_present_src;
    case ResourceState::depth_write:
        return vulkan_image_layout_depth_stencil_attachment_optimal;
    }
    return vulkan_image_layout_undefined;
}

[[nodiscard]] bool texture_barrier_state_supported(ResourceState state) noexcept {
    return state == ResourceState::undefined || state == ResourceState::render_target ||
           state == ResourceState::copy_source || state == ResourceState::copy_destination ||
           state == ResourceState::shader_read || state == ResourceState::depth_write;
}

[[nodiscard]] bool texture_state_supported_for_desc(const TextureDesc& desc, ResourceState state) noexcept {
    switch (state) {
    case ResourceState::undefined:
        return true;
    case ResourceState::render_target:
        return has_flag(desc.usage, TextureUsage::render_target) &&
               dynamic_rendering_color_format_supported(desc.format);
    case ResourceState::depth_write:
        return has_flag(desc.usage, TextureUsage::depth_stencil) && depth_format_supported(desc.format);
    case ResourceState::copy_source:
        return has_flag(desc.usage, TextureUsage::copy_source);
    case ResourceState::copy_destination:
        return has_flag(desc.usage, TextureUsage::copy_destination);
    case ResourceState::shader_read:
        return has_flag(desc.usage, TextureUsage::shader_resource);
    case ResourceState::present:
        return false;
    }
    return false;
}

[[nodiscard]] VulkanSynchronizationStage vulkan_texture_barrier_stage(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::render_target:
        return VulkanSynchronizationStage::color_attachment_output;
    case ResourceState::copy_source:
    case ResourceState::copy_destination:
        return VulkanSynchronizationStage::transfer;
    case ResourceState::shader_read:
        return VulkanSynchronizationStage::shader;
    case ResourceState::depth_write:
        return VulkanSynchronizationStage::depth_attachment;
    case ResourceState::undefined:
    case ResourceState::present:
        return VulkanSynchronizationStage::none;
    }
    return VulkanSynchronizationStage::none;
}

[[nodiscard]] VulkanSynchronizationAccess vulkan_texture_barrier_access(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::render_target:
        return VulkanSynchronizationAccess::color_attachment_write;
    case ResourceState::copy_source:
        return VulkanSynchronizationAccess::transfer_read;
    case ResourceState::copy_destination:
        return VulkanSynchronizationAccess::transfer_write;
    case ResourceState::shader_read:
        return VulkanSynchronizationAccess::shader_read;
    case ResourceState::depth_write:
        return VulkanSynchronizationAccess::depth_attachment_read_write;
    case ResourceState::undefined:
    case ResourceState::present:
        return VulkanSynchronizationAccess::none;
    }
    return VulkanSynchronizationAccess::none;
}

[[nodiscard]] std::uint64_t native_vulkan_texture_barrier_stage(ResourceState state) noexcept {
    return native_vulkan_stage(vulkan_texture_barrier_stage(state));
}

[[nodiscard]] std::uint64_t native_vulkan_texture_barrier_access(ResourceState state) noexcept {
    return native_vulkan_access(vulkan_texture_barrier_access(state));
}

[[nodiscard]] bool swapchain_barrier_state_supported(ResourceState state) noexcept {
    return state == ResourceState::undefined || state == ResourceState::present ||
           state == ResourceState::render_target || state == ResourceState::copy_source;
}

[[nodiscard]] std::uint32_t native_vulkan_present_mode(VulkanPresentMode mode) noexcept {
    switch (mode) {
    case VulkanPresentMode::immediate:
        return vulkan_present_mode_immediate;
    case VulkanPresentMode::mailbox:
        return vulkan_present_mode_mailbox;
    case VulkanPresentMode::fifo:
        return vulkan_present_mode_fifo;
    case VulkanPresentMode::fifo_relaxed:
        return vulkan_present_mode_fifo_relaxed;
    }
    return vulkan_present_mode_fifo;
}

[[nodiscard]] bool present_mode_from_native_vulkan_present_mode(std::uint32_t native_mode,
                                                                VulkanPresentMode& mode) noexcept {
    switch (native_mode) {
    case vulkan_present_mode_immediate:
        mode = VulkanPresentMode::immediate;
        return true;
    case vulkan_present_mode_mailbox:
        mode = VulkanPresentMode::mailbox;
        return true;
    case vulkan_present_mode_fifo:
        mode = VulkanPresentMode::fifo;
        return true;
    case vulkan_present_mode_fifo_relaxed:
        mode = VulkanPresentMode::fifo_relaxed;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] bool contains_present_mode(const std::vector<VulkanPresentMode>& modes,
                                         VulkanPresentMode expected) noexcept {
    return std::ranges::find(modes, expected) != modes.end();
}

[[nodiscard]] Format choose_surface_format(const VulkanSwapchainCreateDesc& desc,
                                           const std::vector<VulkanSurfaceFormatCandidate>& formats) noexcept {
    const auto preferred = std::ranges::find_if(formats, [&](const auto& candidate) {
        return candidate.format == desc.preferred_format && surface_format_supported(candidate);
    });
    if (preferred != formats.end()) {
        return preferred->format;
    }

    for (const auto fallback : {Format::bgra8_unorm, Format::rgba8_unorm}) {
        const auto match = std::ranges::find_if(formats, [&](const auto& candidate) {
            return candidate.format == fallback && surface_format_supported(candidate);
        });
        if (match != formats.end()) {
            return match->format;
        }
    }

    return Format::unknown;
}

[[nodiscard]] VulkanPresentMode choose_present_mode(const VulkanSwapchainCreateDesc& desc,
                                                    const std::vector<VulkanPresentMode>& modes) noexcept {
    if (desc.vsync) {
        if (contains_present_mode(modes, VulkanPresentMode::fifo)) {
            return VulkanPresentMode::fifo;
        }
        if (contains_present_mode(modes, VulkanPresentMode::fifo_relaxed)) {
            return VulkanPresentMode::fifo_relaxed;
        }
        return modes.front();
    }

    if (contains_present_mode(modes, VulkanPresentMode::mailbox)) {
        return VulkanPresentMode::mailbox;
    }
    if (contains_present_mode(modes, VulkanPresentMode::immediate)) {
        return VulkanPresentMode::immediate;
    }
    if (contains_present_mode(modes, VulkanPresentMode::fifo)) {
        return VulkanPresentMode::fifo;
    }
    return modes.front();
}

[[nodiscard]] std::uint32_t choose_swapchain_image_count(const VulkanSwapchainCreateDesc& desc,
                                                         const VulkanSurfaceCapabilities& capabilities) noexcept {
    auto count = std::max(desc.requested_image_count, capabilities.min_image_count);
    if (capabilities.max_image_count > 0) {
        count = std::min(count, capabilities.max_image_count);
    }
    return count;
}

[[nodiscard]] bool command_is_available(const std::vector<VulkanCommandAvailability>& available_commands,
                                        const VulkanCommandRequest& request) noexcept {
    for (const auto& available : available_commands) {
        if (available.scope == request.scope && available.name == request.name) {
            return available.available;
        }
    }
    return false;
}

[[nodiscard]] bool command_is_resolved(const VulkanCommandResolutionPlan& plan, VulkanCommandScope scope,
                                       std::string_view name) noexcept {
    for (const auto& resolution : plan.resolutions) {
        if (resolution.request.scope == scope && resolution.request.name == name) {
            return resolution.resolved;
        }
    }
    return false;
}

[[nodiscard]] VulkanFrameSynchronizationBarrier make_frame_barrier(ResourceState before, ResourceState after,
                                                                   VulkanSynchronizationStage src_stage,
                                                                   VulkanSynchronizationAccess src_access,
                                                                   VulkanSynchronizationStage dst_stage,
                                                                   VulkanSynchronizationAccess dst_access) noexcept {
    return VulkanFrameSynchronizationBarrier{
        .before = before,
        .after = after,
        .src_stage = src_stage,
        .src_access = src_access,
        .dst_stage = dst_stage,
        .dst_access = dst_access,
    };
}

[[nodiscard]] std::vector<VulkanCommandRequest> global_command_requests() {
    std::vector<VulkanCommandRequest> requests;
    for (const auto& request : vulkan_backend_command_requests()) {
        if (request.scope == VulkanCommandScope::loader || request.scope == VulkanCommandScope::global) {
            requests.push_back(request);
        }
    }
    return requests;
}

void append_global_command_availability(std::vector<VulkanCommandAvailability>& availability,
                                        VulkanGetInstanceProcAddr get_instance_proc_addr,
                                        const std::vector<VulkanCommandRequest>& requests) {
    for (const auto& request : requests) {
        if (request.scope == VulkanCommandScope::loader) {
            availability.push_back(VulkanCommandAvailability{
                .name = request.name, .scope = request.scope, .available = get_instance_proc_addr != nullptr});
            continue;
        }

        const auto resolved =
            get_instance_proc_addr != nullptr && get_instance_proc_addr(nullptr, request.name.c_str()) != nullptr;
        availability.push_back(
            VulkanCommandAvailability{.name = request.name, .scope = request.scope, .available = resolved});
    }
}

template <typename AvailableExtensions>
[[nodiscard]] VulkanInstanceCreatePlan
build_instance_create_plan_impl(const VulkanInstanceCreateDesc& desc, const AvailableExtensions& available_extensions) {
    VulkanInstanceCreatePlan plan{
        .supported = false,
        .api_version = desc.api_version,
        .enabled_extensions = {},
        .validation_enabled = false,
        .diagnostic = {},
    };

    if (desc.application_name.empty()) {
        plan.diagnostic = "Vulkan application name is required";
        return plan;
    }

    if (!is_vulkan_api_at_least(desc.api_version, VulkanApiVersion{.major = 1, .minor = 3})) {
        plan.diagnostic = "Vulkan 1.3 or newer is required";
        return plan;
    }

    for (const auto& required : desc.required_extensions) {
        if (required.empty()) {
            continue;
        }
        if (!extension_is_available_in(available_extensions, required)) {
            plan.diagnostic = "missing required Vulkan instance extension: " + required;
            return plan;
        }
        append_extension_once(plan.enabled_extensions, required);
    }

    for (const auto& optional : desc.optional_extensions) {
        if (extension_is_available_in(available_extensions, optional)) {
            append_extension_once(plan.enabled_extensions, optional);
        }
    }

    plan.supported = true;
    plan.validation_enabled =
        desc.enable_validation && extension_is_enabled(plan.enabled_extensions, debug_utils_extension_name);
    plan.diagnostic = "Vulkan instance create plan ready";
    return plan;
}

} // namespace

inline constexpr std::uint32_t vulkan_structure_type_debug_utils_object_name_info_ext = 1000128000U;

inline constexpr std::int32_t vulkan_object_type_device = 3;
inline constexpr std::int32_t vulkan_object_type_queue = 4;
inline constexpr std::int32_t vulkan_object_type_command_buffer = 6;
inline constexpr std::int32_t vulkan_object_type_buffer = 9;
inline constexpr std::int32_t vulkan_object_type_image = 10;
inline constexpr std::int32_t vulkan_object_type_image_view = 14;
inline constexpr std::int32_t vulkan_object_type_shader_module = 15;
inline constexpr std::int32_t vulkan_object_type_pipeline_layout = 17;
inline constexpr std::int32_t vulkan_object_type_pipeline = 19;
inline constexpr std::int32_t vulkan_object_type_sampler = 21;
inline constexpr std::int32_t vulkan_object_type_swapchain_khr = 1000001000;
inline constexpr std::int32_t vulkan_object_type_command_pool = 25;

// Binary layout matches VkDebugUtilsObjectNameInfoEXT on typical 64-bit Vulkan ABIs (padding before objectHandle).
struct alignas(8) NativeVulkanDebugUtilsObjectNameInfoExt {
    std::uint32_t s_type;
    const void* p_next;
    std::int32_t object_type;
    std::uint32_t padding_reserved;
    std::uint64_t object_handle;
    const char* object_name;
};

using VulkanSetDebugUtilsObjectName = VulkanResult(MK_VULKAN_CALL*)(NativeVulkanDevice,
                                                                    const NativeVulkanDebugUtilsObjectNameInfoExt*);

struct VulkanRuntimeInstance::Impl {
#if defined(_WIN32)
    HMODULE library{nullptr};
#elif defined(__linux__)
    void* library{nullptr};
#endif
    NativeVulkanInstance instance{nullptr};
    VulkanDestroyInstance destroy_instance{nullptr};
    VulkanCommandResolutionPlan command_plan;
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (instance != nullptr && destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
            destroyed = true;
        }
        instance = nullptr;

#if defined(_WIN32)
        if (library != nullptr) {
            FreeLibrary(library);
            library = nullptr;
        }
#elif defined(__linux__)
        if (library != nullptr) {
            dlclose(library);
            library = nullptr;
        }
#endif
    }
};

VulkanRuntimeInstance::VulkanRuntimeInstance() noexcept = default;

VulkanRuntimeInstance::~VulkanRuntimeInstance() = default;

VulkanRuntimeInstance::VulkanRuntimeInstance(VulkanRuntimeInstance&& other) noexcept = default;

VulkanRuntimeInstance& VulkanRuntimeInstance::operator=(VulkanRuntimeInstance&& other) noexcept = default;

VulkanRuntimeInstance::VulkanRuntimeInstance(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeInstance::owns_instance() const noexcept {
    return impl_ != nullptr && impl_->instance != nullptr;
}

bool VulkanRuntimeInstance::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

const VulkanCommandResolutionPlan& VulkanRuntimeInstance::command_plan() const noexcept {
    static const VulkanCommandResolutionPlan empty_plan{};
    if (impl_ == nullptr) {
        return empty_plan;
    }
    return impl_->command_plan;
}

void VulkanRuntimeInstance::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeDevice::Impl {
#if defined(_WIN32)
    HMODULE library{nullptr};
#elif defined(__linux__)
    void* library{nullptr};
#endif
    NativeVulkanInstance instance{nullptr};
    NativeVulkanPhysicalDevice physical_device{nullptr};
    NativeVulkanDevice device{nullptr};
    NativeVulkanQueue graphics_queue{nullptr};
    NativeVulkanQueue present_queue{nullptr};
    VulkanDestroyInstance destroy_instance{nullptr};
    VulkanGetPhysicalDeviceMemoryProperties get_physical_device_memory_properties{nullptr};
    VulkanCreateWin32Surface create_win32_surface{nullptr};
    VulkanDestroySurface destroy_surface{nullptr};
    VulkanGetPhysicalDeviceSurfaceCapabilities get_surface_capabilities{nullptr};
    VulkanGetPhysicalDeviceSurfaceFormats get_surface_formats{nullptr};
    VulkanGetPhysicalDeviceSurfacePresentModes get_surface_present_modes{nullptr};
    VulkanDestroyDevice destroy_device{nullptr};
    VulkanDeviceWaitIdle device_wait_idle{nullptr};
    VulkanCreateCommandPool create_command_pool{nullptr};
    VulkanDestroyCommandPool destroy_command_pool{nullptr};
    VulkanAllocateCommandBuffers allocate_command_buffers{nullptr};
    VulkanFreeCommandBuffers free_command_buffers{nullptr};
    VulkanBeginCommandBuffer begin_command_buffer{nullptr};
    VulkanEndCommandBuffer end_command_buffer{nullptr};
    VulkanCmdBeginRendering cmd_begin_rendering{nullptr};
    VulkanCmdEndRendering cmd_end_rendering{nullptr};
    VulkanCmdBindPipeline cmd_bind_pipeline{nullptr};
    VulkanCmdSetViewport cmd_set_viewport{nullptr};
    VulkanCmdSetScissor cmd_set_scissor{nullptr};
    VulkanCmdDraw cmd_draw{nullptr};
    VulkanCmdBindVertexBuffers cmd_bind_vertex_buffers{nullptr};
    VulkanCmdBindIndexBuffer cmd_bind_index_buffer{nullptr};
    VulkanCmdDrawIndexed cmd_draw_indexed{nullptr};
    VulkanCmdPipelineBarrier2 cmd_pipeline_barrier2{nullptr};
    VulkanQueueSubmit2 queue_submit2{nullptr};
    VulkanQueueWaitIdle queue_wait_idle{nullptr};
    VulkanCreateBuffer create_buffer{nullptr};
    VulkanDestroyBuffer destroy_buffer{nullptr};
    VulkanGetBufferMemoryRequirements get_buffer_memory_requirements{nullptr};
    VulkanCreateImage create_image{nullptr};
    VulkanDestroyImage destroy_image{nullptr};
    VulkanGetImageMemoryRequirements get_image_memory_requirements{nullptr};
    VulkanAllocateMemory allocate_memory{nullptr};
    VulkanFreeMemory free_memory{nullptr};
    VulkanBindBufferMemory bind_buffer_memory{nullptr};
    VulkanBindImageMemory bind_image_memory{nullptr};
    VulkanMapMemory map_memory{nullptr};
    VulkanUnmapMemory unmap_memory{nullptr};
    VulkanCmdCopyImageToBuffer cmd_copy_image_to_buffer{nullptr};
    VulkanCmdCopyBuffer cmd_copy_buffer{nullptr};
    VulkanCmdCopyBufferToImage cmd_copy_buffer_to_image{nullptr};
    VulkanCreateShaderModule create_shader_module{nullptr};
    VulkanDestroyShaderModule destroy_shader_module{nullptr};
    VulkanCreateDescriptorSetLayout create_descriptor_set_layout{nullptr};
    VulkanDestroyDescriptorSetLayout destroy_descriptor_set_layout{nullptr};
    VulkanCreateDescriptorPool create_descriptor_pool{nullptr};
    VulkanDestroyDescriptorPool destroy_descriptor_pool{nullptr};
    VulkanAllocateDescriptorSets allocate_descriptor_sets{nullptr};
    VulkanUpdateDescriptorSets update_descriptor_sets{nullptr};
    VulkanCmdBindDescriptorSets cmd_bind_descriptor_sets{nullptr};
    VulkanCreateSwapchain create_swapchain{nullptr};
    VulkanDestroySwapchain destroy_swapchain{nullptr};
    VulkanGetSwapchainImages get_swapchain_images{nullptr};
    VulkanAcquireNextImage acquire_next_image{nullptr};
    VulkanCreateImageView create_image_view{nullptr};
    VulkanDestroyImageView destroy_image_view{nullptr};
    VulkanCreateSampler create_sampler{nullptr};
    VulkanDestroySampler destroy_sampler{nullptr};
    VulkanCreateSemaphore create_semaphore{nullptr};
    VulkanDestroySemaphore destroy_semaphore{nullptr};
    VulkanCreateFence create_fence{nullptr};
    VulkanDestroyFence destroy_fence{nullptr};
    VulkanWaitForFences wait_for_fences{nullptr};
    VulkanQueuePresent queue_present{nullptr};
    VulkanCreatePipelineLayout create_pipeline_layout{nullptr};
    VulkanDestroyPipelineLayout destroy_pipeline_layout{nullptr};
    VulkanCreateGraphicsPipelines create_graphics_pipelines{nullptr};
    VulkanCreateComputePipelines create_compute_pipelines{nullptr};
    VulkanDestroyPipeline destroy_pipeline{nullptr};
    VulkanCmdDispatch cmd_dispatch{nullptr};
    std::uint32_t graphics_queue_family{invalid_vulkan_queue_family};
    std::uint32_t present_queue_family{invalid_vulkan_queue_family};
    RhiHostPlatform host{RhiHostPlatform::unknown};
    VulkanLogicalDeviceCreatePlan logical_device_plan;
    VulkanCommandResolutionPlan command_plan;
    VulkanSetDebugUtilsObjectName set_debug_utils_object_name{nullptr};
    std::uint64_t debug_utils_name_serial{0};
    bool destroyed{false};

    void apply_debug_utils_object_name(std::int32_t object_type, std::uint64_t object_handle,
                                       const char* object_name) const noexcept {
        if (set_debug_utils_object_name == nullptr || device == nullptr || object_name == nullptr ||
            object_handle == 0U) {
            return;
        }
        const NativeVulkanDebugUtilsObjectNameInfoExt info{
            .s_type = vulkan_structure_type_debug_utils_object_name_info_ext,
            .p_next = nullptr,
            .object_type = object_type,
            .padding_reserved = 0U,
            .object_handle = object_handle,
            .object_name = object_name,
        };
        (void)set_debug_utils_object_name(device, &info);
    }

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (device != nullptr) {
            if (device_wait_idle != nullptr) {
                static_cast<void>(device_wait_idle(device));
            }
            if (destroy_device != nullptr) {
                destroy_device(device, nullptr);
                destroyed = true;
            }
        }
        device = nullptr;
        physical_device = nullptr;
        graphics_queue = nullptr;
        present_queue = nullptr;

        if (instance != nullptr && destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        instance = nullptr;

#if defined(_WIN32)
        if (library != nullptr) {
            FreeLibrary(library);
            library = nullptr;
        }
#elif defined(__linux__)
        if (library != nullptr) {
            dlclose(library);
            library = nullptr;
        }
#endif
    }
};

void vulkan_label_runtime_object(void* impl_opaque, std::int32_t object_type, std::uint64_t handle,
                                 const char* stable_prefix) noexcept {
    // Friend of VulkanRuntimeDevice: casts the opaque device Impl* stored on runtime owners.
    auto* impl = static_cast<VulkanRuntimeDevice::Impl*>(impl_opaque);
    if (impl == nullptr || handle == 0U || stable_prefix == nullptr) {
        return;
    }
    std::array<char, 128> label{};
    const auto serial = ++impl->debug_utils_name_serial;
    (void)std::snprintf(label.data(), label.size(), "%s.%llu", stable_prefix, static_cast<unsigned long long>(serial));
    impl->apply_debug_utils_object_name(object_type, handle, label.data());
}

void vulkan_apply_debug_utils_names_for_device_and_queues(void* impl_opaque) noexcept {
    // Friend of VulkanRuntimeDevice: applies stable queue/device labels once after vkGetDeviceProcAddr wiring.
    auto* impl = static_cast<VulkanRuntimeDevice::Impl*>(impl_opaque);
    if (impl == nullptr || impl->device == nullptr) {
        return;
    }
    const auto dispatch_handle = [](void* ptr) -> std::uint64_t {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
    };
    impl->apply_debug_utils_object_name(vulkan_object_type_device, dispatch_handle(impl->device),
                                        "GameEngine.RHI.Vulkan.Device");
    if (impl->graphics_queue != nullptr) {
        impl->apply_debug_utils_object_name(vulkan_object_type_queue, dispatch_handle(impl->graphics_queue),
                                            "GameEngine.RHI.Vulkan.GraphicsQueue");
    }
    if (impl->present_queue != nullptr && impl->present_queue != impl->graphics_queue) {
        impl->apply_debug_utils_object_name(vulkan_object_type_queue, dispatch_handle(impl->present_queue),
                                            "GameEngine.RHI.Vulkan.PresentQueue");
    }
}

VulkanRuntimeDevice::VulkanRuntimeDevice() noexcept = default;

VulkanRuntimeDevice::~VulkanRuntimeDevice() = default;

VulkanRuntimeDevice::VulkanRuntimeDevice(VulkanRuntimeDevice&& other) noexcept = default;

VulkanRuntimeDevice& VulkanRuntimeDevice::operator=(VulkanRuntimeDevice&& other) noexcept = default;

VulkanRuntimeDevice::VulkanRuntimeDevice(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeDevice::owns_device() const noexcept {
    return impl_ != nullptr && impl_->device != nullptr;
}

bool VulkanRuntimeDevice::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

bool VulkanRuntimeDevice::has_graphics_queue() const noexcept {
    return impl_ != nullptr && impl_->graphics_queue != nullptr;
}

bool VulkanRuntimeDevice::has_present_queue() const noexcept {
    return impl_ != nullptr && impl_->present_queue != nullptr;
}

const VulkanLogicalDeviceCreatePlan& VulkanRuntimeDevice::logical_device_plan() const noexcept {
    static const VulkanLogicalDeviceCreatePlan empty_plan{};
    if (impl_ == nullptr) {
        return empty_plan;
    }
    return impl_->logical_device_plan;
}

const VulkanCommandResolutionPlan& VulkanRuntimeDevice::command_plan() const noexcept {
    static const VulkanCommandResolutionPlan empty_plan{};
    if (impl_ == nullptr) {
        return empty_plan;
    }
    return impl_->command_plan;
}

bool VulkanRuntimeDevice::wait_for_fence_signaled(std::uint64_t fence, std::uint64_t timeout_ns) noexcept {
    if (impl_ == nullptr || impl_->device == nullptr || fence == 0 || impl_->wait_for_fences == nullptr) {
        return false;
    }
    constexpr std::uint32_t wait_all = 1U;
    const auto vk_fence = static_cast<NativeVulkanFence>(fence);
    return impl_->wait_for_fences(impl_->device, 1U, &vk_fence, wait_all, timeout_ns) == vulkan_success;
}

bool VulkanRuntimeDevice::record_primary_viewport(VulkanRuntimeCommandPool& pool,
                                                  const ::mirakana::rhi::ViewportDesc& viewport) const {
    if (!owns_device() || impl_->cmd_set_viewport == nullptr) {
        return false;
    }
    auto* const cmd = pool.native_primary_command_buffer();
    if (cmd == nullptr || !pool.recording()) {
        return false;
    }
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || !std::isfinite(viewport.width) ||
        !std::isfinite(viewport.height) || !std::isfinite(viewport.min_depth) || !std::isfinite(viewport.max_depth)) {
        return false;
    }
    if (viewport.width <= 0.0F || viewport.height <= 0.0F) {
        return false;
    }
    const NativeVulkanViewport vp{.x = viewport.x,
                                  .y = viewport.y,
                                  .width = viewport.width,
                                  .height = viewport.height,
                                  .min_depth = viewport.min_depth,
                                  .max_depth = viewport.max_depth};
    impl_->cmd_set_viewport(cmd, 0, 1, &vp);
    return true;
}

bool VulkanRuntimeDevice::record_primary_scissor(VulkanRuntimeCommandPool& pool,
                                                 const ::mirakana::rhi::ScissorRectDesc& scissor) const {
    if (!owns_device() || impl_->cmd_set_scissor == nullptr) {
        return false;
    }
    auto* const cmd = pool.native_primary_command_buffer();
    if (cmd == nullptr || !pool.recording()) {
        return false;
    }
    if (scissor.width == 0U || scissor.height == 0U) {
        return false;
    }
    const NativeVulkanRect2D rect{
        .offset =
            NativeVulkanOffset2D{.x = static_cast<std::int32_t>(scissor.x), .y = static_cast<std::int32_t>(scissor.y)},
        .extent = NativeVulkanExtent2D{.width = scissor.width, .height = scissor.height},
    };
    impl_->cmd_set_scissor(cmd, 0, 1, &rect);
    return true;
}

void VulkanRuntimeDevice::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeCommandPool::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanCommandPool pool{0};
    NativeVulkanCommandBuffer primary_command_buffer{nullptr};
    bool destroyed{false};
    bool recording{false};
    bool ended{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (pool != 0 || primary_command_buffer != nullptr) {
            destroyed = true;
        }

        if (device_owner != nullptr && device_owner->device != nullptr && pool != 0) {
            if (primary_command_buffer != nullptr && device_owner->free_command_buffers != nullptr) {
                auto* const buffer = primary_command_buffer;
                device_owner->free_command_buffers(device_owner->device, pool, 1, &buffer);
            }
            primary_command_buffer = nullptr;

            if (device_owner->destroy_command_pool != nullptr) {
                device_owner->destroy_command_pool(device_owner->device, pool, nullptr);
            }
        }

        pool = 0;
        primary_command_buffer = nullptr;
        recording = false;
    }
};

VulkanRuntimeCommandPool::VulkanRuntimeCommandPool() noexcept = default;

VulkanRuntimeCommandPool::~VulkanRuntimeCommandPool() = default;

VulkanRuntimeCommandPool::VulkanRuntimeCommandPool(VulkanRuntimeCommandPool&& other) noexcept = default;

VulkanRuntimeCommandPool& VulkanRuntimeCommandPool::operator=(VulkanRuntimeCommandPool&& other) noexcept = default;

VulkanRuntimeCommandPool::VulkanRuntimeCommandPool(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeCommandPool::owns_pool() const noexcept {
    return impl_ != nullptr && impl_->pool != 0;
}

bool VulkanRuntimeCommandPool::owns_primary_command_buffer() const noexcept {
    return impl_ != nullptr && impl_->primary_command_buffer != nullptr;
}

bool VulkanRuntimeCommandPool::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

bool VulkanRuntimeCommandPool::recording() const noexcept {
    return impl_ != nullptr && impl_->recording;
}

bool VulkanRuntimeCommandPool::ended() const noexcept {
    return impl_ != nullptr && impl_->ended;
}

bool VulkanRuntimeCommandPool::begin_primary_command_buffer() {
    if (!owns_primary_command_buffer() || impl_->recording || impl_->device_owner == nullptr ||
        impl_->device_owner->begin_command_buffer == nullptr) {
        return false;
    }

    const NativeVulkanCommandBufferBeginInfo begin_info{
        .s_type = vulkan_structure_type_command_buffer_begin_info,
        .next = nullptr,
        .flags = 0,
        .inheritance_info = nullptr,
    };
    const auto result = impl_->device_owner->begin_command_buffer(impl_->primary_command_buffer, &begin_info);
    if (result != vulkan_success) {
        return false;
    }

    impl_->recording = true;
    impl_->ended = false;
    return true;
}

bool VulkanRuntimeCommandPool::end_primary_command_buffer() {
    if (!owns_primary_command_buffer() || !impl_->recording || impl_->device_owner == nullptr ||
        impl_->device_owner->end_command_buffer == nullptr) {
        return false;
    }

    const auto result = impl_->device_owner->end_command_buffer(impl_->primary_command_buffer);
    if (result != vulkan_success) {
        return false;
    }

    impl_->recording = false;
    impl_->ended = true;
    return true;
}

NativeVulkanCommandBuffer VulkanRuntimeCommandPool::native_primary_command_buffer() const noexcept {
    return impl_ != nullptr ? impl_->primary_command_buffer : nullptr;
}

void VulkanRuntimeCommandPool::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeBuffer::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanBuffer buffer{0};
    NativeVulkanDeviceMemory memory{0};
    BufferDesc desc;
    VulkanBufferMemoryDomain memory_domain{VulkanBufferMemoryDomain::device_local};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (buffer != 0 || memory != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr) {
            if (buffer != 0 && device_owner->destroy_buffer != nullptr) {
                device_owner->destroy_buffer(device_owner->device, buffer, nullptr);
            }
            if (memory != 0 && device_owner->free_memory != nullptr) {
                device_owner->free_memory(device_owner->device, memory, nullptr);
            }
        }
        buffer = 0;
        memory = 0;
    }
};

VulkanRuntimeBuffer::VulkanRuntimeBuffer() noexcept = default;

VulkanRuntimeBuffer::~VulkanRuntimeBuffer() = default;

VulkanRuntimeBuffer::VulkanRuntimeBuffer(VulkanRuntimeBuffer&& other) noexcept = default;

VulkanRuntimeBuffer& VulkanRuntimeBuffer::operator=(VulkanRuntimeBuffer&& other) noexcept = default;

VulkanRuntimeBuffer::VulkanRuntimeBuffer(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeBuffer::owns_buffer() const noexcept {
    return impl_ != nullptr && impl_->buffer != 0;
}

bool VulkanRuntimeBuffer::owns_memory() const noexcept {
    return impl_ != nullptr && impl_->memory != 0;
}

bool VulkanRuntimeBuffer::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

std::uint64_t VulkanRuntimeBuffer::byte_size() const noexcept {
    return impl_ != nullptr ? impl_->desc.size_bytes : 0;
}

BufferUsage VulkanRuntimeBuffer::usage() const noexcept {
    return impl_ != nullptr ? impl_->desc.usage : BufferUsage::none;
}

VulkanBufferMemoryDomain VulkanRuntimeBuffer::memory_domain() const noexcept {
    return impl_ != nullptr ? impl_->memory_domain : VulkanBufferMemoryDomain::device_local;
}

void VulkanRuntimeBuffer::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeTexture::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanImage image{0};
    NativeVulkanImageView image_view{0};
    NativeVulkanDeviceMemory memory{0};
    TextureDesc desc;
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (image != 0 || memory != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr) {
            if (image_view != 0 && device_owner->destroy_image_view != nullptr) {
                device_owner->destroy_image_view(device_owner->device, image_view, nullptr);
            }
            if (image != 0 && device_owner->destroy_image != nullptr) {
                device_owner->destroy_image(device_owner->device, image, nullptr);
            }
            if (memory != 0 && device_owner->free_memory != nullptr) {
                device_owner->free_memory(device_owner->device, memory, nullptr);
            }
        }
        image_view = 0;
        image = 0;
        memory = 0;
    }
};

VulkanRuntimeTexture::VulkanRuntimeTexture() noexcept = default;

VulkanRuntimeTexture::~VulkanRuntimeTexture() = default;

VulkanRuntimeTexture::VulkanRuntimeTexture(VulkanRuntimeTexture&& other) noexcept = default;

VulkanRuntimeTexture& VulkanRuntimeTexture::operator=(VulkanRuntimeTexture&& other) noexcept = default;

VulkanRuntimeTexture::VulkanRuntimeTexture(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeTexture::owns_image() const noexcept {
    return impl_ != nullptr && impl_->image != 0;
}

bool VulkanRuntimeTexture::owns_memory() const noexcept {
    return impl_ != nullptr && impl_->memory != 0;
}

bool VulkanRuntimeTexture::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

Extent3D VulkanRuntimeTexture::extent() const noexcept {
    return impl_ != nullptr ? impl_->desc.extent : Extent3D{};
}

Format VulkanRuntimeTexture::format() const noexcept {
    return impl_ != nullptr ? impl_->desc.format : Format::unknown;
}

TextureUsage VulkanRuntimeTexture::usage() const noexcept {
    return impl_ != nullptr ? impl_->desc.usage : TextureUsage::none;
}

void VulkanRuntimeTexture::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeSampler::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanSampler sampler{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (sampler != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr && device_owner->destroy_sampler != nullptr &&
            sampler != 0) {
            device_owner->destroy_sampler(device_owner->device, sampler, nullptr);
        }
        sampler = 0;
    }
};

VulkanRuntimeSampler::VulkanRuntimeSampler() noexcept = default;

VulkanRuntimeSampler::~VulkanRuntimeSampler() = default;

VulkanRuntimeSampler::VulkanRuntimeSampler(VulkanRuntimeSampler&& other) noexcept = default;

VulkanRuntimeSampler& VulkanRuntimeSampler::operator=(VulkanRuntimeSampler&& other) noexcept = default;

VulkanRuntimeSampler::VulkanRuntimeSampler(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeSampler::owns_sampler() const noexcept {
    return impl_ != nullptr && impl_->sampler != 0;
}

bool VulkanRuntimeSampler::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void VulkanRuntimeSampler::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

[[nodiscard]] VulkanBufferMemoryDomain rhi_buffer_memory_domain(BufferUsage usage) noexcept {
    if (has_flag(usage, BufferUsage::copy_source)) {
        return VulkanBufferMemoryDomain::upload;
    }
    if (usage == BufferUsage::copy_destination) {
        return VulkanBufferMemoryDomain::readback;
    }
    return VulkanBufferMemoryDomain::device_local;
}

namespace {

[[nodiscard]] std::string vulkan_rhi_resource_debug_name(std::string_view prefix, std::uint32_t id) {
    std::string out;
    out.reserve(prefix.size() + 16U);
    out.append(prefix);
    out.push_back('-');
    out.append(std::to_string(id));
    return out;
}

/// Vulkan deferred-destroy order: graphics pipelines and pipeline layouts before shader modules and descriptor
/// parents, descriptor pools before set layouts, then samplers. Buffers and textures follow pipeline-class objects.
[[nodiscard]] constexpr int deferred_vulkan_destroy_rank(RhiResourceKind kind) noexcept {
    switch (kind) {
    case RhiResourceKind::graphics_pipeline:
    case RhiResourceKind::compute_pipeline:
        return 0;
    case RhiResourceKind::pipeline_layout:
        return 1;
    case RhiResourceKind::descriptor_set:
        return 2;
    case RhiResourceKind::descriptor_set_layout:
        return 3;
    case RhiResourceKind::shader:
        return 4;
    case RhiResourceKind::sampler:
        return 5;
    case RhiResourceKind::buffer:
        return 6;
    case RhiResourceKind::texture:
        return 7;
    default:
        return 99;
    }
}

} // namespace

struct VulkanPendingGpuTimelineEntry {
    std::uint64_t timeline_value{0};
    bool owns_submission_sync{false};
    VulkanRuntimeFrameSync submission_sync;
    std::uint32_t swapchain_index{0};
};

class VulkanRhiCommandList;

class VulkanRhiDevice final : public IRhiDevice {
  public:
    explicit VulkanRhiDevice(VulkanRuntimeDevice device) noexcept : device_(std::move(device)) {}
    ~VulkanRhiDevice() override;

    [[nodiscard]] BackendKind backend_kind() const noexcept override {
        return BackendKind::vulkan;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "vulkan";
    }

    [[nodiscard]] RhiStats stats() const noexcept override {
        return stats_;
    }

    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override {
        return 0;
    }

    [[nodiscard]] RhiDeviceMemoryDiagnostics memory_diagnostics() const override {
        RhiDeviceMemoryDiagnostics diagnostics{};
        std::uint64_t total = 0;
        for (std::size_t i = 0; i < buffer_descs_.size(); ++i) {
            if (i < buffer_active_.size() && buffer_active_.at(i)) {
                total += buffer_descs_.at(i).size_bytes;
            }
        }
        for (std::size_t i = 0; i < texture_descs_.size(); ++i) {
            if (i >= texture_active_.size() || !texture_active_.at(i)) {
                continue;
            }
            const auto& desc = texture_descs_.at(i);
            if (desc.format == Format::unknown) {
                continue;
            }
            const auto texels = static_cast<std::uint64_t>(desc.extent.width) *
                                static_cast<std::uint64_t>(desc.extent.height) *
                                static_cast<std::uint64_t>(desc.extent.depth);
            total += texels * static_cast<std::uint64_t>(mirakana::rhi::bytes_per_texel(desc.format));
        }
        diagnostics.committed_resources_byte_estimate = total;
        diagnostics.committed_resources_byte_estimate_available = true;
        return diagnostics;
    }

    [[nodiscard]] const VulkanRuntimeDevice& vulkan_runtime_device() const noexcept {
        return device_;
    }

    [[nodiscard]] const RhiResourceLifetimeRegistry* resource_lifetime_registry() const noexcept override {
        return &resource_lifetime_;
    }

    [[nodiscard]] BufferHandle create_buffer(const BufferDesc& desc) override {
        auto result = create_runtime_buffer(
            device_, VulkanRuntimeBufferDesc{.buffer = desc, .memory_domain = rhi_buffer_memory_domain(desc.usage)});
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi buffer description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        VulkanRuntimeBuffer buffer = std::move(result.buffer);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::buffer,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("buffer", static_cast<std::uint32_t>(buffers_.size() + 1U)),
        });
        if (!lifetime_registration.succeeded()) {
            buffer.reset();
            throw std::logic_error("vulkan rhi buffer lifetime registration failed");
        }

        buffers_.push_back(std::move(buffer));
        buffer_descs_.push_back(desc);
        buffer_active_.push_back(true);
        buffer_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.buffers_created;
        return BufferHandle{static_cast<std::uint32_t>(buffers_.size())};
    }

    [[nodiscard]] TextureHandle create_texture(const TextureDesc& desc) override {
        auto result = create_runtime_texture(device_, VulkanRuntimeTextureDesc{desc});
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi texture description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        VulkanRuntimeTexture texture = std::move(result.texture);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::texture,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("texture", static_cast<std::uint32_t>(textures_.size() + 1U)),
        });
        if (!lifetime_registration.succeeded()) {
            texture.reset();
            throw std::logic_error("vulkan rhi texture lifetime registration failed");
        }

        textures_.push_back(std::move(texture));
        texture_descs_.push_back(desc);
        texture_active_.push_back(true);
        texture_lifetime_.push_back(lifetime_registration.handle);
        texture_states_.push_back(ResourceState::undefined);
        ++stats_.textures_created;
        return TextureHandle{static_cast<std::uint32_t>(textures_.size())};
    }

    [[nodiscard]] SamplerHandle create_sampler(const SamplerDesc& desc) override {
        if (!valid_sampler_desc(desc)) {
            throw std::invalid_argument("vulkan rhi sampler description is invalid or unsupported");
        }

        auto result = create_runtime_sampler(device_, VulkanRuntimeSamplerDesc{desc});
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi sampler creation failed: " + result.diagnostic);
        }

        samplers_.push_back(std::move(result.sampler));
        sampler_descs_.push_back(desc);
        sampler_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::sampler,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("sampler", static_cast<std::uint32_t>(samplers_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            samplers_.pop_back();
            sampler_descs_.pop_back();
            sampler_active_.pop_back();
            throw std::logic_error("vulkan rhi sampler lifetime registration failed");
        }
        sampler_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.samplers_created;
        return SamplerHandle{static_cast<std::uint32_t>(samplers_.size())};
    }

    [[nodiscard]] SwapchainHandle create_swapchain(const SwapchainDesc& desc) override {
        if (desc.surface.value == 0) {
            throw std::invalid_argument("vulkan rhi swapchain creation requires a surface handle");
        }
        if (!valid_extent(desc.extent)) {
            throw std::invalid_argument("vulkan rhi swapchain extent must be non-zero");
        }
        if (desc.buffer_count == 0) {
            throw std::invalid_argument("vulkan rhi swapchain buffer count must be non-zero");
        }

        const auto plan = make_swapchain_create_plan(desc);
        if (!plan.supported) {
            throw std::invalid_argument("vulkan rhi swapchain description is invalid or unsupported: " +
                                        plan.diagnostic);
        }

        auto swapchain_result =
            create_runtime_swapchain(device_, VulkanRuntimeSwapchainDesc{.surface = desc.surface, .plan = plan});
        if (!swapchain_result.created) {
            throw std::invalid_argument("vulkan rhi swapchain description is invalid or unsupported: " +
                                        swapchain_result.diagnostic);
        }

        auto sync_result = create_runtime_frame_sync(device_, {});
        if (!sync_result.created) {
            throw std::invalid_argument("vulkan rhi swapchain frame synchronization failed: " + sync_result.diagnostic);
        }

        swapchains_.push_back(std::move(swapchain_result.swapchain));
        swapchain_syncs_.push_back(std::move(sync_result.sync));
        swapchain_descs_.push_back(desc);
        swapchain_plans_.push_back(plan);
        swapchain_states_.push_back(ResourceState::present);
        swapchain_frame_reserved_.push_back(false);
        ++stats_.swapchains_created;
        return SwapchainHandle{static_cast<std::uint32_t>(swapchains_.size())};
    }

    void resize_swapchain(SwapchainHandle swapchain, Extent2D extent) override {
        if (!owns_swapchain(swapchain)) {
            throw std::invalid_argument("vulkan rhi swapchain handle is unknown");
        }
        if (!valid_extent(extent)) {
            throw std::invalid_argument("vulkan rhi swapchain extent must be non-zero");
        }

        const auto index = swapchain.value - 1U;
        if (swapchain_frame_reserved_.at(index)) {
            throw std::invalid_argument("vulkan rhi swapchain cannot be resized while a frame is pending");
        }

        auto desc = swapchain_descs_.at(index);
        desc.extent = extent;
        const auto plan = make_swapchain_create_plan(desc);
        if (!plan.supported) {
            throw std::invalid_argument("vulkan rhi swapchain description is invalid or unsupported: " +
                                        plan.diagnostic);
        }

        auto swapchain_result =
            create_runtime_swapchain(device_, VulkanRuntimeSwapchainDesc{.surface = desc.surface, .plan = plan});
        if (!swapchain_result.created) {
            throw std::invalid_argument("vulkan rhi swapchain resize failed: " + swapchain_result.diagnostic);
        }

        auto sync_result = create_runtime_frame_sync(device_, {});
        if (!sync_result.created) {
            throw std::invalid_argument("vulkan rhi swapchain frame synchronization failed: " + sync_result.diagnostic);
        }

        swapchains_.at(index) = std::move(swapchain_result.swapchain);
        swapchain_syncs_.at(index) = std::move(sync_result.sync);
        swapchain_descs_.at(index) = desc;
        swapchain_plans_.at(index) = plan;
        swapchain_states_.at(index) = ResourceState::present;
        ++stats_.swapchain_resizes;
    }

    [[nodiscard]] SwapchainFrameHandle acquire_swapchain_frame(SwapchainHandle swapchain) override {
        if (!owns_swapchain(swapchain)) {
            throw std::invalid_argument("vulkan rhi swapchain handle is unknown");
        }

        const auto index = swapchain.value - 1U;
        if (swapchain_frame_reserved_.at(index)) {
            throw std::invalid_argument("vulkan rhi swapchain already has a pending frame");
        }
        if (swapchain_states_.at(index) != ResourceState::present) {
            throw std::invalid_argument("vulkan rhi swapchain is not ready for image acquisition");
        }

        const auto acquire_result =
            acquire_next_runtime_swapchain_image(device_, swapchains_.at(index), swapchain_syncs_.at(index), {});
        if (!acquire_result.acquired) {
            throw std::logic_error("vulkan rhi swapchain frame acquisition failed: " + acquire_result.diagnostic);
        }

        swapchain_frame_reserved_.at(index) = true;
        swapchain_frame_swapchains_.push_back(swapchain);
        swapchain_frame_image_indices_.push_back(acquire_result.image_index);
        swapchain_frame_active_.push_back(true);
        swapchain_frame_presented_.push_back(false);
        ++stats_.swapchain_frames_acquired;
        return SwapchainFrameHandle{static_cast<std::uint32_t>(swapchain_frame_swapchains_.size())};
    }

    void release_swapchain_frame(SwapchainFrameHandle frame) override {
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("vulkan rhi swapchain frame handle is unknown");
        }
        if (swapchain_frame_presented_.at(frame.value - 1U)) {
            throw std::invalid_argument(
                "vulkan rhi swapchain frame cannot be manually released after present recording");
        }
        complete_swapchain_frame(frame);
    }

    [[nodiscard]] TransientBuffer acquire_transient_buffer(const BufferDesc& desc) override {
        const auto buffer = create_buffer(desc);
        const auto lease = TransientResourceHandle{next_transient_resource_++};
        transient_leases_.push_back(TransientLeaseRecord{
            .kind = TransientResourceKind::buffer,
            .buffer = buffer,
            .texture = TextureHandle{},
            .active = true,
        });
        ++stats_.transient_resources_acquired;
        ++stats_.transient_resources_active;
        return TransientBuffer{.lease = lease, .buffer = buffer};
    }

    [[nodiscard]] TransientTexture acquire_transient_texture(const TextureDesc& desc) override {
        const auto texture = create_texture(desc);
        const auto lease = TransientResourceHandle{next_transient_resource_++};
        transient_leases_.push_back(TransientLeaseRecord{
            .kind = TransientResourceKind::texture,
            .buffer = BufferHandle{},
            .texture = texture,
            .active = true,
        });
        ++stats_.transient_resources_acquired;
        ++stats_.transient_resources_active;
        return TransientTexture{.lease = lease, .texture = texture};
    }

    void release_transient(TransientResourceHandle lease) override {
        if (lease.value == 0 || lease.value >= next_transient_resource_) {
            throw std::invalid_argument("vulkan rhi transient resource lease is unknown");
        }

        auto& record = transient_leases_.at(lease.value - 1U);
        if (!record.active) {
            throw std::invalid_argument("vulkan rhi transient resource lease is already released");
        }

        record.active = false;
        if (record.kind == TransientResourceKind::buffer) {
            const auto index = record.buffer.value - 1U;
            if (buffer_active_.at(index)) {
                const auto release_fence = stats_.last_submitted_fence_value;
                (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_.at(index), release_fence);
                buffer_active_.at(index) = false;
            }
        } else {
            const auto index = record.texture.value - 1U;
            if (texture_active_.at(index)) {
                const auto release_fence = stats_.last_submitted_fence_value;
                (void)resource_lifetime_.release_resource_deferred(texture_lifetime_.at(index), release_fence);
                texture_active_.at(index) = false;
                texture_states_.at(index) = ResourceState::undefined;
            }
        }
        retire_deferred_native_resources(stats_.last_completed_fence_value);
        ++stats_.transient_resources_released;
        --stats_.transient_resources_active;
    }

    [[nodiscard]] ShaderHandle create_shader(const ShaderDesc& desc) override {
        if (desc.entry_point.empty() || desc.bytecode == nullptr || desc.bytecode_size == 0) {
            throw std::invalid_argument("vulkan rhi shader requires entry point and bytecode");
        }

        auto result = create_runtime_shader_module(device_, VulkanRuntimeShaderModuleDesc{
                                                                .stage = desc.stage,
                                                                .bytecode = desc.bytecode,
                                                                .bytecode_size = desc.bytecode_size,
                                                            });
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi shader bytecode is invalid or unsupported: " + result.diagnostic);
        }

        shaders_.push_back(std::move(result.module));
        shader_stages_.push_back(desc.stage);
        shader_entry_points_.emplace_back(desc.entry_point);
        shader_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::shader,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("shader", static_cast<std::uint32_t>(shaders_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            shaders_.pop_back();
            shader_stages_.pop_back();
            shader_entry_points_.pop_back();
            shader_active_.pop_back();
            throw std::logic_error("vulkan rhi shader lifetime registration failed");
        }
        shader_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.shader_modules_created;
        return ShaderHandle{static_cast<std::uint32_t>(shaders_.size())};
    }

    [[nodiscard]] DescriptorSetLayoutHandle create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) override {
        auto result = create_runtime_descriptor_set_layout(device_, VulkanRuntimeDescriptorSetLayoutDesc{desc});
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi descriptor set layout description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        descriptor_set_layouts_.push_back(std::move(result.layout));
        descriptor_set_layout_descs_.push_back(desc);
        descriptor_set_layout_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::descriptor_set_layout,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("descriptor-set-layout",
                                                         static_cast<std::uint32_t>(descriptor_set_layouts_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            descriptor_set_layouts_.pop_back();
            descriptor_set_layout_descs_.pop_back();
            descriptor_set_layout_active_.pop_back();
            throw std::logic_error("vulkan rhi descriptor set layout lifetime registration failed");
        }
        descriptor_set_layout_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.descriptor_set_layouts_created;
        return DescriptorSetLayoutHandle{static_cast<std::uint32_t>(descriptor_set_layouts_.size())};
    }

    [[nodiscard]] DescriptorSetHandle allocate_descriptor_set(DescriptorSetLayoutHandle layout) override {
        if (!owns_descriptor_set_layout(layout)) {
            throw std::invalid_argument("vulkan rhi descriptor set layout handle is unknown");
        }

        auto result = create_runtime_descriptor_set(device_, descriptor_set_layouts_.at(layout.value - 1U), {});
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi descriptor set allocation failed: " + result.diagnostic);
        }

        descriptor_sets_.push_back(std::move(result.set));
        descriptor_set_layout_for_sets_.push_back(layout);
        descriptor_set_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::descriptor_set,
            .owner = "vulkan",
            .debug_name =
                vulkan_rhi_resource_debug_name("descriptor-set", static_cast<std::uint32_t>(descriptor_sets_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            descriptor_sets_.pop_back();
            descriptor_set_layout_for_sets_.pop_back();
            descriptor_set_active_.pop_back();
            throw std::logic_error("vulkan rhi descriptor set lifetime registration failed");
        }
        descriptor_set_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.descriptor_sets_allocated;
        return DescriptorSetHandle{static_cast<std::uint32_t>(descriptor_sets_.size())};
    }

    void update_descriptor_set(const DescriptorWrite& write) override {
        if (!owns_descriptor_set(write.set)) {
            throw std::invalid_argument("vulkan rhi descriptor set handle is unknown");
        }
        if (write.resources.empty()) {
            throw std::invalid_argument("vulkan rhi descriptor write must contain at least one resource");
        }
        if (write.resources.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("vulkan rhi descriptor write resource count is too large");
        }

        const auto layout_handle = descriptor_set_layout_for_sets_.at(write.set.value - 1U);
        const auto& layout = descriptor_set_layout_descs_.at(layout_handle.value - 1U);
        const auto* binding = find_descriptor_binding(layout, write.binding);
        if (binding == nullptr) {
            throw std::invalid_argument("vulkan rhi descriptor binding is not declared by the set layout");
        }

        const auto resource_count = static_cast<std::uint32_t>(write.resources.size());
        if (write.array_element >= binding->count || resource_count > binding->count - write.array_element) {
            throw std::invalid_argument("vulkan rhi descriptor write exceeds the declared binding range");
        }

        VulkanRuntimeDescriptorWriteDesc runtime_desc;
        runtime_desc.binding = write.binding;
        runtime_desc.array_element = write.array_element;
        runtime_desc.buffers.reserve(write.resources.size());
        runtime_desc.textures.reserve(write.resources.size());
        runtime_desc.samplers.reserve(write.resources.size());
        for (const auto& resource : write.resources) {
            if (resource.type != binding->type) {
                throw std::invalid_argument("vulkan rhi descriptor resource type must match the binding type");
            }
            if (binding->type == DescriptorType::uniform_buffer || binding->type == DescriptorType::storage_buffer) {
                if (!owns_buffer(resource.buffer_handle)) {
                    throw std::invalid_argument("vulkan rhi descriptor buffer handle is unknown");
                }
                const auto usage = buffer_descs_.at(resource.buffer_handle.value - 1U).usage;
                if (binding->type == DescriptorType::uniform_buffer && !has_flag(usage, BufferUsage::uniform)) {
                    throw std::invalid_argument("vulkan rhi uniform buffer descriptor requires uniform buffer usage");
                }
                if (binding->type == DescriptorType::storage_buffer && !has_flag(usage, BufferUsage::storage)) {
                    throw std::invalid_argument("vulkan rhi storage buffer descriptor requires storage buffer usage");
                }
                runtime_desc.buffers.push_back(VulkanRuntimeDescriptorBufferResource{
                    .type = resource.type,
                    .buffer = &buffers_.at(resource.buffer_handle.value - 1U),
                });
            } else if (binding->type == DescriptorType::sampled_texture ||
                       binding->type == DescriptorType::storage_texture) {
                if (!owns_texture(resource.texture_handle)) {
                    throw std::invalid_argument("vulkan rhi descriptor texture handle is unknown");
                }
                const auto usage = texture_descs_.at(resource.texture_handle.value - 1U).usage;
                if (binding->type == DescriptorType::sampled_texture &&
                    !has_flag(usage, TextureUsage::shader_resource)) {
                    throw std::invalid_argument(
                        "vulkan rhi sampled texture descriptor requires shader_resource texture usage");
                }
                if (binding->type == DescriptorType::storage_texture && !has_flag(usage, TextureUsage::storage)) {
                    throw std::invalid_argument("vulkan rhi storage texture descriptor requires storage texture usage");
                }
                runtime_desc.textures.push_back(VulkanRuntimeDescriptorTextureResource{
                    .type = resource.type,
                    .texture = &textures_.at(resource.texture_handle.value - 1U),
                });
            } else if (binding->type == DescriptorType::sampler) {
                if (!owns_sampler(resource.sampler_handle)) {
                    throw std::invalid_argument("vulkan rhi descriptor sampler handle is unknown");
                }
                runtime_desc.samplers.push_back(VulkanRuntimeDescriptorSamplerResource{
                    &samplers_.at(resource.sampler_handle.value - 1U),
                });
            }
        }

        auto result = update_runtime_descriptor_set(device_, descriptor_sets_.at(write.set.value - 1U), runtime_desc);
        if (!result.updated) {
            throw std::invalid_argument("vulkan rhi descriptor set update failed: " + result.diagnostic);
        }
        ++stats_.descriptor_writes;
    }

    [[nodiscard]] PipelineLayoutHandle create_pipeline_layout(const PipelineLayoutDesc& desc) override {
        VulkanRuntimePipelineLayoutDesc runtime_desc;
        runtime_desc.push_constant_bytes = desc.push_constant_bytes;
        runtime_desc.descriptor_set_layouts.reserve(desc.descriptor_sets.size());
        for (const auto layout : desc.descriptor_sets) {
            if (!owns_descriptor_set_layout(layout)) {
                throw std::invalid_argument("vulkan rhi descriptor set layout handle is unknown");
            }
            runtime_desc.descriptor_set_layouts.push_back(&descriptor_set_layouts_.at(layout.value - 1U));
        }

        auto result = create_runtime_pipeline_layout(device_, runtime_desc);
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi pipeline layout description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        pipeline_layouts_.push_back(std::move(result.layout));
        pipeline_layout_descs_.push_back(desc);
        pipeline_layout_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::pipeline_layout,
            .owner = "vulkan",
            .debug_name =
                vulkan_rhi_resource_debug_name("pipeline-layout", static_cast<std::uint32_t>(pipeline_layouts_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            pipeline_layouts_.pop_back();
            pipeline_layout_descs_.pop_back();
            pipeline_layout_active_.pop_back();
            throw std::logic_error("vulkan rhi pipeline layout lifetime registration failed");
        }
        pipeline_layout_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.pipeline_layouts_created;
        return PipelineLayoutHandle{static_cast<std::uint32_t>(pipeline_layouts_.size())};
    }

    [[nodiscard]] GraphicsPipelineHandle create_graphics_pipeline(const GraphicsPipelineDesc& desc) override {
        if (!owns_pipeline_layout(desc.layout)) {
            throw std::invalid_argument("vulkan rhi pipeline layout handle is unknown");
        }
        if (!owns_shader(desc.vertex_shader) ||
            shader_stages_.at(desc.vertex_shader.value - 1U) != ShaderStage::vertex) {
            throw std::invalid_argument("vulkan rhi vertex shader handle is unknown or has the wrong stage");
        }
        if (!owns_shader(desc.fragment_shader) ||
            shader_stages_.at(desc.fragment_shader.value - 1U) != ShaderStage::fragment) {
            throw std::invalid_argument("vulkan rhi fragment shader handle is unknown or has the wrong stage");
        }
        if (!valid_depth_state_for_format(desc.depth_format, desc.depth_state)) {
            throw std::invalid_argument("vulkan rhi graphics pipeline depth state is invalid or unsupported");
        }

        VulkanDynamicRenderingDesc rendering_desc;
        rendering_desc.extent = Extent2D{.width = 1, .height = 1};
        rendering_desc.color_attachments.push_back(VulkanDynamicRenderingColorAttachmentDesc{
            .format = desc.color_format,
            .load_action = LoadAction::clear,
            .store_action = StoreAction::store,
        });
        if (desc.depth_format != Format::unknown) {
            rendering_desc.has_depth_attachment = true;
            rendering_desc.depth_format = desc.depth_format;
        }

        const auto dynamic_rendering = build_dynamic_rendering_plan(rendering_desc, device_.command_plan());
        if (!dynamic_rendering.supported) {
            throw std::invalid_argument("vulkan rhi graphics pipeline description is invalid or unsupported: " +
                                        dynamic_rendering.diagnostic);
        }

        const auto vertex_index = desc.vertex_shader.value - 1U;
        const auto fragment_index = desc.fragment_shader.value - 1U;
        auto result =
            create_runtime_graphics_pipeline(device_, pipeline_layouts_.at(desc.layout.value - 1U),
                                             shaders_.at(vertex_index), shaders_.at(fragment_index),
                                             VulkanRuntimeGraphicsPipelineDesc{
                                                 .dynamic_rendering = dynamic_rendering,
                                                 .color_format = desc.color_format,
                                                 .depth_format = desc.depth_format,
                                                 .topology = desc.topology,
                                                 .vertex_entry_point = shader_entry_points_.at(vertex_index),
                                                 .fragment_entry_point = shader_entry_points_.at(fragment_index),
                                                 .vertex_buffers = desc.vertex_buffers,
                                                 .vertex_attributes = desc.vertex_attributes,
                                                 .depth_state = desc.depth_state,
                                             });
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi graphics pipeline description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        graphics_pipelines_.push_back(std::move(result.pipeline));
        graphics_pipeline_layouts_.push_back(desc.layout);
        graphics_pipeline_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::graphics_pipeline,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("graphics-pipeline",
                                                         static_cast<std::uint32_t>(graphics_pipelines_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            graphics_pipelines_.pop_back();
            graphics_pipeline_layouts_.pop_back();
            graphics_pipeline_active_.pop_back();
            throw std::logic_error("vulkan rhi graphics pipeline lifetime registration failed");
        }
        graphics_pipeline_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.graphics_pipelines_created;
        return GraphicsPipelineHandle{static_cast<std::uint32_t>(graphics_pipelines_.size())};
    }

    [[nodiscard]] ComputePipelineHandle create_compute_pipeline(const ComputePipelineDesc& desc) override {
        if (!owns_pipeline_layout(desc.layout)) {
            throw std::invalid_argument("vulkan rhi compute pipeline layout handle is unknown");
        }
        if (!owns_shader(desc.compute_shader) ||
            shader_stages_.at(desc.compute_shader.value - 1U) != ShaderStage::compute) {
            throw std::invalid_argument("vulkan rhi compute shader handle is unknown or has the wrong stage");
        }

        const auto shader_index = desc.compute_shader.value - 1U;
        auto result = create_runtime_compute_pipeline(device_, pipeline_layouts_.at(desc.layout.value - 1U),
                                                      shaders_.at(shader_index),
                                                      VulkanRuntimeComputePipelineDesc{
                                                          shader_entry_points_.at(shader_index),
                                                      });
        if (!result.created) {
            throw std::invalid_argument("vulkan rhi compute pipeline description is invalid or unsupported: " +
                                        result.diagnostic);
        }

        compute_pipelines_.push_back(std::move(result.pipeline));
        compute_pipeline_layouts_.push_back(desc.layout);
        compute_pipeline_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::compute_pipeline,
            .owner = "vulkan",
            .debug_name = vulkan_rhi_resource_debug_name("compute-pipeline",
                                                         static_cast<std::uint32_t>(compute_pipelines_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            compute_pipelines_.pop_back();
            compute_pipeline_layouts_.pop_back();
            compute_pipeline_active_.pop_back();
            throw std::logic_error("vulkan rhi compute pipeline lifetime registration failed");
        }
        compute_pipeline_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.compute_pipelines_created;
        return ComputePipelineHandle{static_cast<std::uint32_t>(compute_pipelines_.size())};
    }

    [[nodiscard]] std::unique_ptr<IRhiCommandList> begin_command_list(QueueKind queue) override;

    [[nodiscard]] FenceValue submit(IRhiCommandList& commands) override;

    void wait(FenceValue fence) override;

    void wait_for_queue(QueueKind queue, FenceValue fence) override;

    void write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) override;

    [[nodiscard]] std::vector<std::uint8_t> read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) override;

  private:
    friend class VulkanRhiCommandList;

    struct TransientLeaseRecord {
        TransientResourceKind kind{TransientResourceKind::buffer};
        BufferHandle buffer;
        TextureHandle texture;
        bool active{false};
    };

    [[noreturn]] static void throw_not_implemented(std::string_view feature) {
        throw std::logic_error("vulkan IRhiDevice " + std::string(feature) + " is not implemented yet");
    }

    struct SurfaceSupportQuery {
        bool queried{false};
        VulkanSwapchainSupport support;
        std::string diagnostic;
    };

    [[nodiscard]] SurfaceSupportQuery query_surface_swapchain_support(SurfaceHandle surface) const {
        SurfaceSupportQuery query;
        if (surface.value == 0) {
            query.diagnostic = "Vulkan swapchain support query requires a surface handle";
            return query;
        }
        if (device_.impl_ == nullptr || device_.impl_->host != RhiHostPlatform::windows) {
            query.diagnostic = "Vulkan swapchain support query is unsupported on this host";
            return query;
        }
        if (device_.impl_->create_win32_surface == nullptr || device_.impl_->destroy_surface == nullptr ||
            device_.impl_->get_surface_capabilities == nullptr || device_.impl_->get_surface_formats == nullptr ||
            device_.impl_->get_surface_present_modes == nullptr) {
            query.diagnostic = "Vulkan surface support query commands are unavailable";
            return query;
        }

#if defined(_WIN32)
        NativeVulkanSurface native_surface = 0;
        const auto surface_create_info = NativeVulkanWin32SurfaceCreateInfo{
            .s_type = vulkan_structure_type_win32_surface_create_info,
            .next = nullptr,
            .flags = 0,
            .instance = GetModuleHandleW(nullptr),
            .window = reinterpret_cast<HWND>(surface.value),
        };
        const auto surface_result = device_.impl_->create_win32_surface(device_.impl_->instance, &surface_create_info,
                                                                        nullptr, &native_surface);
        if (surface_result != vulkan_success || native_surface == 0) {
            query.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateWin32SurfaceKHR failed", surface_result);
            return query;
        }

        NativeVulkanSurfaceCapabilities capabilities{};
        const auto capabilities_result =
            device_.impl_->get_surface_capabilities(device_.impl_->physical_device, native_surface, &capabilities);
        if (capabilities_result != vulkan_success) {
            device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
            query.diagnostic = vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed",
                                                        capabilities_result);
            return query;
        }

        query.support.capabilities.min_image_count = capabilities.min_image_count;
        query.support.capabilities.max_image_count = capabilities.max_image_count;
        query.support.capabilities.current_extent =
            Extent2D{.width = capabilities.current_extent.width, .height = capabilities.current_extent.height};
        query.support.capabilities.min_image_extent =
            Extent2D{.width = capabilities.min_image_extent.width, .height = capabilities.min_image_extent.height};
        query.support.capabilities.max_image_extent =
            Extent2D{.width = capabilities.max_image_extent.width, .height = capabilities.max_image_extent.height};
        query.support.capabilities.current_extent_defined =
            capabilities.current_extent.width != std::numeric_limits<std::uint32_t>::max() &&
            capabilities.current_extent.height != std::numeric_limits<std::uint32_t>::max();

        std::uint32_t format_count = 0;
        auto formats_result =
            device_.impl_->get_surface_formats(device_.impl_->physical_device, native_surface, &format_count, nullptr);
        if (!is_successful_enumeration_result(formats_result)) {
            device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
            query.diagnostic =
                vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceFormatsKHR count failed", formats_result);
            return query;
        }
        std::vector<NativeVulkanSurfaceFormat> native_formats(format_count);
        if (format_count > 0) {
            formats_result = device_.impl_->get_surface_formats(device_.impl_->physical_device, native_surface,
                                                                &format_count, native_formats.data());
            if (!is_successful_enumeration_result(formats_result)) {
                device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
                query.diagnostic =
                    vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceFormatsKHR failed", formats_result);
                return query;
            }
            native_formats.resize(format_count);
        }
        for (const auto& native_format : native_formats) {
            if (native_format.format == 0) {
                query.support.formats.push_back(
                    VulkanSurfaceFormatCandidate{.format = Format::bgra8_unorm, .srgb = false});
                query.support.formats.push_back(
                    VulkanSurfaceFormatCandidate{.format = Format::rgba8_unorm, .srgb = false});
                continue;
            }
            const auto format = format_from_native_vulkan_format(native_format.format);
            if (format != Format::unknown) {
                query.support.formats.push_back(VulkanSurfaceFormatCandidate{
                    .format = format, .srgb = native_format.color_space == vulkan_color_space_srgb_nonlinear});
            }
        }

        std::uint32_t present_mode_count = 0;
        auto present_modes_result = device_.impl_->get_surface_present_modes(
            device_.impl_->physical_device, native_surface, &present_mode_count, nullptr);
        if (!is_successful_enumeration_result(present_modes_result)) {
            device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
            query.diagnostic = vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfacePresentModesKHR count failed",
                                                        present_modes_result);
            return query;
        }
        std::vector<std::uint32_t> native_present_modes(present_mode_count);
        if (present_mode_count > 0) {
            present_modes_result = device_.impl_->get_surface_present_modes(
                device_.impl_->physical_device, native_surface, &present_mode_count, native_present_modes.data());
            if (!is_successful_enumeration_result(present_modes_result)) {
                device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
                query.diagnostic = vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfacePresentModesKHR failed",
                                                            present_modes_result);
                return query;
            }
            native_present_modes.resize(present_mode_count);
        }
        for (const auto native_mode : native_present_modes) {
            VulkanPresentMode mode{};
            if (present_mode_from_native_vulkan_present_mode(native_mode, mode) &&
                !contains_present_mode(query.support.present_modes, mode)) {
                query.support.present_modes.push_back(mode);
            }
        }

        device_.impl_->destroy_surface(device_.impl_->instance, native_surface, nullptr);
        query.queried = true;
        query.diagnostic = "Vulkan runtime surface swapchain support query ready";
        return query;
#else
        query.diagnostic = "Vulkan swapchain support query is unsupported on this host";
        return query;
#endif
    }

    [[nodiscard]] VulkanSwapchainCreatePlan make_swapchain_create_plan(const SwapchainDesc& desc) const {
        const auto query = query_surface_swapchain_support(desc.surface);
        if (!query.queried) {
            VulkanSwapchainCreatePlan plan;
            plan.diagnostic = query.diagnostic;
            return plan;
        }

        return build_swapchain_create_plan(VulkanSwapchainCreateDesc{.requested_extent = desc.extent,
                                                                     .preferred_format = desc.format,
                                                                     .requested_image_count = desc.buffer_count,
                                                                     .vsync = desc.vsync},
                                           query.support);
    }

    [[nodiscard]] bool owns_swapchain(SwapchainHandle swapchain) const noexcept {
        return swapchain.value != 0 && swapchain.value <= swapchains_.size() &&
               swapchains_.at(swapchain.value - 1U).owns_swapchain();
    }

    [[nodiscard]] bool owns_swapchain_frame(SwapchainFrameHandle frame) const noexcept {
        return frame.value != 0 && frame.value <= swapchain_frame_active_.size() &&
               swapchain_frame_active_.at(frame.value - 1U);
    }

    [[nodiscard]] SwapchainHandle swapchain_for_frame(SwapchainFrameHandle frame) const {
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("vulkan rhi swapchain frame handle is unknown");
        }
        return swapchain_frame_swapchains_.at(frame.value - 1U);
    }

    [[nodiscard]] std::uint32_t swapchain_image_index_for_frame(SwapchainFrameHandle frame) const {
        if (!owns_swapchain_frame(frame) || frame.value > swapchain_frame_image_indices_.size()) {
            throw std::invalid_argument("vulkan rhi swapchain frame handle is unknown");
        }
        return swapchain_frame_image_indices_.at(frame.value - 1U);
    }

    void complete_swapchain_frame(SwapchainFrameHandle frame) {
        const auto frame_index = frame.value - 1U;
        const auto swapchain = swapchain_frame_swapchains_.at(frame_index);
        swapchain_frame_active_.at(frame_index) = false;
        swapchain_frame_reserved_.at(swapchain.value - 1U) = false;
        ++stats_.swapchain_frames_released;
    }

    /// Destroys Vulkan buffer/image objects for lifetime records deferred through `release_frame`, then retires
    /// those records from `resource_lifetime_`. `completed_frame` matches `RhiStats::last_completed_fence_value`
    /// semantics on this backend (advances on `wait` and on submits that drain the graphics queue).
    void retire_deferred_native_resources(std::uint64_t completed_frame) noexcept;

    /// Blocks on `vkWaitForFences` until all GPU timeline entries through `target_timeline` have completed.
    void advance_gpu_timeline_completion(std::uint64_t target_timeline) noexcept;

    [[nodiscard]] bool owns_shader(ShaderHandle shader) const noexcept {
        return shader.value != 0 && shader.value <= shaders_.size() && shader.value <= shader_active_.size() &&
               shader_active_.at(shader.value - 1U) && shaders_.at(shader.value - 1U).owns_module();
    }

    [[nodiscard]] bool owns_buffer(BufferHandle buffer) const noexcept {
        return buffer.value != 0 && buffer.value <= buffers_.size() && buffer.value <= buffer_active_.size() &&
               buffer_active_.at(buffer.value - 1U) && buffers_.at(buffer.value - 1U).owns_buffer();
    }

    [[nodiscard]] bool owns_texture(TextureHandle texture) const noexcept {
        return texture.value != 0 && texture.value <= textures_.size() && texture.value <= texture_active_.size() &&
               texture_active_.at(texture.value - 1U) && textures_.at(texture.value - 1U).owns_image();
    }

    [[nodiscard]] bool owns_sampler(SamplerHandle sampler) const noexcept {
        return sampler.value != 0 && sampler.value <= samplers_.size() && sampler.value <= sampler_active_.size() &&
               sampler_active_.at(sampler.value - 1U) && samplers_.at(sampler.value - 1U).owns_sampler();
    }

    [[nodiscard]] bool owns_descriptor_set_layout(DescriptorSetLayoutHandle layout) const noexcept {
        return layout.value != 0 && layout.value <= descriptor_set_layouts_.size() &&
               layout.value <= descriptor_set_layout_active_.size() &&
               descriptor_set_layout_active_.at(layout.value - 1U) &&
               descriptor_set_layouts_.at(layout.value - 1U).owns_layout();
    }

    [[nodiscard]] bool owns_descriptor_set(DescriptorSetHandle set) const noexcept {
        return set.value != 0 && set.value <= descriptor_sets_.size() && set.value <= descriptor_set_active_.size() &&
               descriptor_set_active_.at(set.value - 1U) && descriptor_sets_.at(set.value - 1U).owns_set();
    }

    [[nodiscard]] bool owns_pipeline_layout(PipelineLayoutHandle layout) const noexcept {
        return layout.value != 0 && layout.value <= pipeline_layouts_.size() &&
               layout.value <= pipeline_layout_active_.size() && pipeline_layout_active_.at(layout.value - 1U) &&
               pipeline_layouts_.at(layout.value - 1U).owns_layout();
    }

    [[nodiscard]] bool owns_graphics_pipeline(GraphicsPipelineHandle pipeline) const noexcept {
        return pipeline.value != 0 && pipeline.value <= graphics_pipelines_.size() &&
               pipeline.value <= graphics_pipeline_active_.size() &&
               graphics_pipeline_active_.at(pipeline.value - 1U) &&
               graphics_pipelines_.at(pipeline.value - 1U).owns_pipeline();
    }

    [[nodiscard]] bool owns_compute_pipeline(ComputePipelineHandle pipeline) const noexcept {
        return pipeline.value != 0 && pipeline.value <= compute_pipelines_.size() &&
               pipeline.value <= compute_pipeline_active_.size() && compute_pipeline_active_.at(pipeline.value - 1U) &&
               compute_pipelines_.at(pipeline.value - 1U).owns_pipeline();
    }

    VulkanRuntimeDevice device_;
    RhiStats stats_{};
    std::uint32_t next_transient_resource_{1};
    std::uint64_t next_fence_value_{1};
    std::deque<VulkanPendingGpuTimelineEntry> pending_gpu_timeline_;
    std::vector<VulkanRuntimeBuffer> buffers_;
    std::vector<BufferDesc> buffer_descs_;
    std::vector<bool> buffer_active_;
    std::vector<RhiResourceHandle> buffer_lifetime_;
    std::vector<VulkanRuntimeTexture> textures_;
    std::vector<TextureDesc> texture_descs_;
    std::vector<bool> texture_active_;
    std::vector<RhiResourceHandle> texture_lifetime_;
    std::vector<bool> sampler_active_;
    std::vector<RhiResourceHandle> sampler_lifetime_;
    std::vector<bool> shader_active_;
    std::vector<RhiResourceHandle> shader_lifetime_;
    std::vector<bool> descriptor_set_layout_active_;
    std::vector<RhiResourceHandle> descriptor_set_layout_lifetime_;
    std::vector<bool> descriptor_set_active_;
    std::vector<RhiResourceHandle> descriptor_set_lifetime_;
    std::vector<bool> pipeline_layout_active_;
    std::vector<RhiResourceHandle> pipeline_layout_lifetime_;
    std::vector<bool> graphics_pipeline_active_;
    std::vector<RhiResourceHandle> graphics_pipeline_lifetime_;
    std::vector<bool> compute_pipeline_active_;
    std::vector<RhiResourceHandle> compute_pipeline_lifetime_;
    std::vector<ResourceState> texture_states_;
    std::vector<VulkanRuntimeSampler> samplers_;
    std::vector<SamplerDesc> sampler_descs_;
    std::vector<VulkanRuntimeSwapchain> swapchains_;
    std::vector<VulkanRuntimeFrameSync> swapchain_syncs_;
    std::vector<SwapchainDesc> swapchain_descs_;
    std::vector<VulkanSwapchainCreatePlan> swapchain_plans_;
    std::vector<ResourceState> swapchain_states_;
    std::vector<bool> swapchain_frame_reserved_;
    std::vector<SwapchainHandle> swapchain_frame_swapchains_;
    std::vector<std::uint32_t> swapchain_frame_image_indices_;
    std::vector<bool> swapchain_frame_active_;
    std::vector<bool> swapchain_frame_presented_;
    std::vector<VulkanRuntimeShaderModule> shaders_;
    std::vector<ShaderStage> shader_stages_;
    std::vector<std::string> shader_entry_points_;
    std::vector<VulkanRuntimeDescriptorSetLayout> descriptor_set_layouts_;
    std::vector<DescriptorSetLayoutDesc> descriptor_set_layout_descs_;
    std::vector<VulkanRuntimeDescriptorSet> descriptor_sets_;
    std::vector<DescriptorSetLayoutHandle> descriptor_set_layout_for_sets_;
    std::vector<VulkanRuntimePipelineLayout> pipeline_layouts_;
    std::vector<PipelineLayoutDesc> pipeline_layout_descs_;
    std::vector<VulkanRuntimeGraphicsPipeline> graphics_pipelines_;
    std::vector<PipelineLayoutHandle> graphics_pipeline_layouts_;
    std::vector<VulkanRuntimeComputePipeline> compute_pipelines_;
    std::vector<PipelineLayoutHandle> compute_pipeline_layouts_;
    std::vector<TransientLeaseRecord> transient_leases_;
    RhiResourceLifetimeRegistry resource_lifetime_{};
};

class VulkanRhiCommandList final : public IRhiCommandList {
  public:
    VulkanRhiCommandList(VulkanRhiDevice& device, VulkanRuntimeCommandPool pool, QueueKind queue) noexcept
        : device_(&device), pool_(std::move(pool)), queue_(queue), texture_states_(device.texture_states_),
          initial_texture_states_(device.texture_states_) {}

    ~VulkanRhiCommandList() override {
        release_unsubmitted_swapchain_frames();
    }

    [[nodiscard]] QueueKind queue_kind() const noexcept override {
        return queue_;
    }

    [[nodiscard]] bool closed() const noexcept override {
        return closed_;
    }

    void transition_texture(TextureHandle texture, ResourceState before, ResourceState after) override {
        require_open();
        require_no_render_pass();
        if (!device_->owns_texture(texture)) {
            throw std::invalid_argument("vulkan rhi texture handle is unknown");
        }
        const auto texture_index = texture.value - 1U;
        if (texture_state(texture) != before) {
            throw std::invalid_argument("vulkan rhi texture transition before state does not match current state");
        }
        if (before == after) {
            throw std::invalid_argument("vulkan rhi texture transition requires different states");
        }
        const auto& desc = device_->texture_descs_.at(texture_index);
        if (!texture_state_supported_for_desc(desc, before) || !texture_state_supported_for_desc(desc, after)) {
            throw std::invalid_argument(
                "vulkan rhi texture transition state is incompatible with texture usage or format");
        }

        const auto result =
            record_runtime_texture_barrier(device_->device_, pool_, device_->textures_.at(texture_index),
                                           VulkanRuntimeTextureBarrierDesc{.before = before, .after = after});
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi texture transition failed: " + result.diagnostic);
        }

        observe_texture(texture);
        set_texture_state(texture, after);
        recorded_work_ = true;
        ++device_->stats_.resource_transitions;
    }

    void copy_buffer(BufferHandle source, BufferHandle destination, const BufferCopyRegion& region) override {
        require_open();
        require_no_render_pass();
        if (!device_->owns_buffer(source) || !device_->owns_buffer(destination)) {
            throw std::invalid_argument("vulkan rhi buffer copy handles are unknown");
        }
        const auto& source_desc = device_->buffer_descs_.at(source.value - 1U);
        const auto& destination_desc = device_->buffer_descs_.at(destination.value - 1U);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("vulkan rhi buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("vulkan rhi buffer copy destination must use copy_destination");
        }
        if (region.size_bytes == 0 || region.source_offset > source_desc.size_bytes ||
            region.size_bytes > source_desc.size_bytes - region.source_offset ||
            region.destination_offset > destination_desc.size_bytes ||
            region.size_bytes > destination_desc.size_bytes - region.destination_offset) {
            throw std::invalid_argument("vulkan rhi buffer copy range is outside the buffers");
        }

        const auto result = record_runtime_buffer_copy(device_->device_, pool_, device_->buffers_.at(source.value - 1U),
                                                       device_->buffers_.at(destination.value - 1U),
                                                       VulkanRuntimeBufferCopyDesc{region});
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi buffer copy failed: " + result.diagnostic);
        }

        recorded_work_ = true;
        ++device_->stats_.buffer_copies;
        device_->stats_.bytes_copied += region.size_bytes;
    }

    void copy_buffer_to_texture(BufferHandle source, TextureHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_open();
        require_no_render_pass();
        if (!device_->owns_buffer(source) || !device_->owns_texture(destination)) {
            throw std::invalid_argument("vulkan rhi buffer texture copy handles are unknown");
        }
        const auto& source_desc = device_->buffer_descs_.at(source.value - 1U);
        const auto& destination_desc = device_->texture_descs_.at(destination.value - 1U);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("vulkan rhi buffer texture copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, TextureUsage::copy_destination)) {
            throw std::invalid_argument("vulkan rhi buffer texture copy destination must use copy_destination");
        }
        if (texture_state(destination) != ResourceState::copy_destination) {
            throw std::invalid_argument("vulkan rhi buffer texture copy destination must be in copy_destination state");
        }
        observe_texture(destination);
        const auto required_bytes = buffer_texture_copy_required_bytes(destination_desc.format, region);
        if (required_bytes > source_desc.size_bytes) {
            throw std::invalid_argument("vulkan rhi buffer texture copy source range is outside the source buffer");
        }

        const auto result = record_runtime_buffer_texture_copy(
            device_->device_, pool_, device_->buffers_.at(source.value - 1U),
            device_->textures_.at(destination.value - 1U), VulkanRuntimeBufferTextureCopyDesc{region});
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi buffer texture copy failed: " + result.diagnostic);
        }

        recorded_work_ = true;
        ++device_->stats_.buffer_texture_copies;
    }

    void copy_texture_to_buffer(TextureHandle source, BufferHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_open();
        require_no_render_pass();
        if (!device_->owns_texture(source) || !device_->owns_buffer(destination)) {
            throw std::invalid_argument("vulkan rhi texture buffer copy handles are unknown");
        }
        const auto& source_desc = device_->texture_descs_.at(source.value - 1U);
        const auto& destination_desc = device_->buffer_descs_.at(destination.value - 1U);
        if (!has_flag(source_desc.usage, TextureUsage::copy_source)) {
            throw std::invalid_argument("vulkan rhi texture buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("vulkan rhi texture buffer copy destination must use copy_destination");
        }
        if (texture_state(source) != ResourceState::copy_source) {
            throw std::invalid_argument("vulkan rhi texture buffer copy source must be in copy_source state");
        }
        observe_texture(source);
        const auto required_bytes = buffer_texture_copy_required_bytes(source_desc.format, region);
        if (required_bytes > destination_desc.size_bytes) {
            throw std::invalid_argument(
                "vulkan rhi texture buffer copy destination range is outside the destination buffer");
        }

        const auto result = record_runtime_texture_buffer_copy(
            device_->device_, pool_, device_->textures_.at(source.value - 1U),
            device_->buffers_.at(destination.value - 1U), VulkanRuntimeTextureBufferCopyDesc{region});
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi texture buffer copy failed: " + result.diagnostic);
        }

        recorded_work_ = true;
        ++device_->stats_.texture_buffer_copies;
    }

    void present(SwapchainFrameHandle frame) override {
        require_open();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("vulkan rhi swapchain presents require a graphics command list");
        }
        if (render_pass_active_) {
            throw std::logic_error("vulkan rhi swapchain presents must be recorded outside a render pass");
        }
        if (!device_->owns_swapchain_frame(frame)) {
            throw std::invalid_argument("vulkan rhi swapchain frame handle is unknown");
        }
        if (!contains_frame(presentable_swapchain_frames_, frame)) {
            throw std::invalid_argument("vulkan rhi swapchain must complete a render pass before present command");
        }
        if (!contains_frame(reserved_swapchain_frames_, frame)) {
            throw std::invalid_argument(
                "vulkan rhi swapchain present must match a frame recorded by this command list");
        }
        if (device_->swapchain_frame_presented_.at(frame.value - 1U)) {
            throw std::invalid_argument("vulkan rhi swapchain frame is already presented");
        }
        if (!pending_present_frames_.empty()) {
            throw std::invalid_argument("vulkan rhi command list supports one pending swapchain present");
        }

        device_->swapchain_frame_presented_.at(frame.value - 1U) = true;
        pending_present_frames_.push_back(frame);
    }

    void begin_render_pass(const RenderPassDesc& desc) override {
        require_open();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("vulkan rhi render passes require a graphics command list");
        }
        if (render_pass_active_) {
            throw std::logic_error("vulkan rhi render pass is already active");
        }

        const bool uses_swapchain = desc.color.swapchain_frame.value != 0;
        const bool uses_texture = desc.color.texture.value != 0;
        if (uses_swapchain == uses_texture) {
            throw std::invalid_argument("vulkan rhi render pass requires exactly one color attachment");
        }
        if (uses_texture) {
            if (!device_->owns_texture(desc.color.texture)) {
                throw std::invalid_argument("vulkan rhi texture render pass attachment is unknown");
            }
            if (!has_flag(device_->texture_descs_.at(desc.color.texture.value - 1U).usage,
                          TextureUsage::render_target)) {
                throw std::invalid_argument("vulkan rhi texture render pass attachment must use render_target");
            }
            if (texture_state(desc.color.texture) != ResourceState::render_target) {
                throw std::invalid_argument("vulkan rhi texture render pass attachment must be in render_target state");
            }
            if (desc.color.load_action != LoadAction::clear) {
                throw std::invalid_argument("vulkan rhi texture render pass currently requires clear load action");
            }
            const auto color_extent = device_->textures_.at(desc.color.texture.value - 1U).extent();
            validate_depth_attachment(desc.depth, Extent2D{.width = color_extent.width, .height = color_extent.height});

            observe_texture(desc.color.texture);
            if (desc.depth.texture.value != 0) {
                observe_texture(desc.depth.texture);
            }
            active_render_pass_ = desc;
            active_texture_ = desc.color.texture;
            render_pass_active_ = true;
            rendering_recorded_ = false;
            bound_graphics_pipeline_ = GraphicsPipelineHandle{};
            vertex_buffer_bound_ = false;
            index_buffer_bound_ = false;
            bound_vertex_buffer_ = VertexBufferBinding{};
            bound_index_buffer_ = IndexBufferBinding{};
            recorded_work_ = true;
            ++device_->stats_.render_passes_begun;
            return;
        }
        if (!device_->owns_swapchain_frame(desc.color.swapchain_frame)) {
            throw std::invalid_argument("vulkan rhi swapchain frame handle is unknown");
        }
        if (device_->swapchain_frame_presented_.at(desc.color.swapchain_frame.value - 1U)) {
            throw std::invalid_argument("vulkan rhi swapchain frame is already presented");
        }

        const auto swapchain = device_->swapchain_for_frame(desc.color.swapchain_frame);
        const auto swapchain_index = swapchain.value - 1U;
        if (!device_->swapchain_frame_reserved_.at(swapchain_index)) {
            throw std::invalid_argument("vulkan rhi swapchain already has a pending frame");
        }
        if (device_->swapchain_states_.at(swapchain_index) != ResourceState::present) {
            throw std::invalid_argument("vulkan rhi swapchain render pass attachment must be in present state");
        }
        validate_depth_attachment(desc.depth, device_->swapchains_.at(swapchain_index).extent());

        const auto sync_plan = make_frame_sync_plan(false);
        VulkanRuntimeSwapchainFrameBarrierDesc barrier_desc;
        barrier_desc.image_index = device_->swapchain_image_index_for_frame(desc.color.swapchain_frame);
        barrier_desc.barrier = sync_plan.barriers.at(0);
        const auto barrier = record_runtime_swapchain_frame_barrier(
            device_->device_, pool_, device_->swapchains_.at(swapchain_index), barrier_desc);
        if (!barrier.recorded) {
            throw std::logic_error("vulkan rhi swapchain render pass transition failed: " + barrier.diagnostic);
        }

        active_render_pass_ = desc;
        active_swapchain_ = swapchain;
        active_swapchain_frame_ = desc.color.swapchain_frame;
        render_pass_active_ = true;
        rendering_recorded_ = false;
        bound_graphics_pipeline_ = GraphicsPipelineHandle{};
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
        bound_vertex_buffer_ = VertexBufferBinding{};
        bound_index_buffer_ = IndexBufferBinding{};
        track_swapchain_frame(desc.color.swapchain_frame);
        if (desc.depth.texture.value != 0) {
            observe_texture(desc.depth.texture);
        }
        device_->swapchain_states_.at(swapchain_index) = ResourceState::render_target;
        recorded_work_ = true;
        ++device_->stats_.resource_transitions;
        ++device_->stats_.render_passes_begun;
    }

    void end_render_pass() override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi render pass is not active");
        }

        if (!rendering_recorded_ && active_render_pass_.color.load_action == LoadAction::clear) {
            record_clear_rendering();
        }
        if (active_texture_.value != 0) {
            active_render_pass_ = RenderPassDesc{};
            active_texture_ = TextureHandle{};
            render_pass_active_ = false;
            rendering_recorded_ = false;
            bound_graphics_pipeline_ = GraphicsPipelineHandle{};
            vertex_buffer_bound_ = false;
            index_buffer_bound_ = false;
            bound_vertex_buffer_ = VertexBufferBinding{};
            bound_index_buffer_ = IndexBufferBinding{};
            return;
        }

        const auto swapchain_index = active_swapchain_.value - 1U;
        const auto sync_plan = make_frame_sync_plan(false);
        VulkanRuntimeSwapchainFrameBarrierDesc barrier_desc;
        barrier_desc.image_index = device_->swapchain_image_index_for_frame(active_swapchain_frame_);
        barrier_desc.barrier = sync_plan.barriers.at(1);
        const auto barrier = record_runtime_swapchain_frame_barrier(
            device_->device_, pool_, device_->swapchains_.at(swapchain_index), barrier_desc);
        if (!barrier.recorded) {
            throw std::logic_error("vulkan rhi swapchain render pass end transition failed: " + barrier.diagnostic);
        }

        device_->swapchain_states_.at(swapchain_index) = ResourceState::present;
        presentable_swapchain_frames_.push_back(active_swapchain_frame_);
        active_render_pass_ = RenderPassDesc{};
        active_swapchain_ = SwapchainHandle{};
        active_swapchain_frame_ = SwapchainFrameHandle{};
        render_pass_active_ = false;
        rendering_recorded_ = false;
        bound_graphics_pipeline_ = GraphicsPipelineHandle{};
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
        bound_vertex_buffer_ = VertexBufferBinding{};
        bound_index_buffer_ = IndexBufferBinding{};
        ++device_->stats_.resource_transitions;
    }

    void bind_graphics_pipeline(GraphicsPipelineHandle pipeline) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi graphics pipelines must be bound inside a render pass");
        }
        if (!device_->owns_graphics_pipeline(pipeline)) {
            throw std::invalid_argument("vulkan rhi graphics pipeline handle is unknown");
        }
        const auto& runtime_pipeline = device_->graphics_pipelines_.at(pipeline.value - 1U);
        if (runtime_pipeline.color_format() != active_render_pass_color_format() ||
            runtime_pipeline.depth_format() != active_render_pass_depth_format()) {
            throw std::invalid_argument("vulkan rhi graphics pipeline format must match the active render pass");
        }

        bound_graphics_pipeline_ = pipeline;
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
        bound_vertex_buffer_ = VertexBufferBinding{};
        bound_index_buffer_ = IndexBufferBinding{};
        ++device_->stats_.graphics_pipelines_bound;
    }

    void bind_compute_pipeline(ComputePipelineHandle pipeline) override {
        require_open();
        require_no_render_pass();
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("vulkan rhi compute pipelines require a compute command list");
        }
        if (!device_->owns_compute_pipeline(pipeline)) {
            throw std::invalid_argument("vulkan rhi compute pipeline handle is unknown");
        }

        const auto result = record_runtime_compute_pipeline_binding(
            device_->device_, pool_, device_->compute_pipelines_.at(pipeline.value - 1U));
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi compute pipeline bind failed: " + result.diagnostic);
        }

        bound_compute_pipeline_ = pipeline;
        recorded_work_ = true;
        ++device_->stats_.compute_pipelines_bound;
    }

    void bind_descriptor_set(PipelineLayoutHandle layout, std::uint32_t set_index, DescriptorSetHandle set) override {
        require_open();
        const bool compute_binding = !render_pass_active_ && queue_ == QueueKind::compute;
        if (!render_pass_active_ && !compute_binding) {
            throw std::logic_error("vulkan rhi descriptor sets require a graphics or compute pipeline");
        }
        if (!device_->owns_pipeline_layout(layout)) {
            throw std::invalid_argument("vulkan rhi pipeline layout handle is unknown");
        }
        if (!device_->owns_descriptor_set(set)) {
            throw std::invalid_argument("vulkan rhi descriptor set handle is unknown");
        }
        if (render_pass_active_ && !device_->owns_graphics_pipeline(bound_graphics_pipeline_)) {
            throw std::logic_error("vulkan rhi descriptor set binding requires a graphics pipeline");
        }
        if (compute_binding && !device_->owns_compute_pipeline(bound_compute_pipeline_)) {
            throw std::logic_error("vulkan rhi descriptor set binding requires a compute pipeline");
        }
        const auto bound_layout = render_pass_active_
                                      ? device_->graphics_pipeline_layouts_.at(bound_graphics_pipeline_.value - 1U)
                                      : device_->compute_pipeline_layouts_.at(bound_compute_pipeline_.value - 1U);
        if (bound_layout.value != layout.value) {
            throw std::invalid_argument("vulkan rhi descriptor set pipeline layout must match the bound pipeline");
        }

        const auto& layout_desc = device_->pipeline_layout_descs_.at(layout.value - 1U);
        if (set_index >= layout_desc.descriptor_sets.size()) {
            throw std::invalid_argument("vulkan rhi descriptor set index is outside the pipeline layout");
        }
        const auto expected_layout = layout_desc.descriptor_sets.at(set_index);
        const auto actual_layout = device_->descriptor_set_layout_for_sets_.at(set.value - 1U);
        if (expected_layout.value != actual_layout.value) {
            throw std::invalid_argument("vulkan rhi descriptor set layout is not compatible with the pipeline layout");
        }

        const auto result = record_runtime_descriptor_set_binding(
            device_->device_, pool_, device_->pipeline_layouts_.at(layout.value - 1U),
            device_->descriptor_sets_.at(set.value - 1U),
            VulkanRuntimeDescriptorSetBindDesc{
                .first_set = set_index,
                .pipeline_bind_point =
                    compute_binding ? vulkan_pipeline_bind_point_compute : vulkan_pipeline_bind_point_graphics,
            });
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi descriptor set bind failed: " + result.diagnostic);
        }

        recorded_work_ = true;
        ++device_->stats_.descriptor_sets_bound;
    }

    void bind_vertex_buffer(const VertexBufferBinding& binding) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi vertex buffers must be bound inside a render pass");
        }
        if (!device_->owns_graphics_pipeline(bound_graphics_pipeline_)) {
            throw std::logic_error("vulkan rhi vertex buffer binding requires a graphics pipeline");
        }
        if (!device_->owns_buffer(binding.buffer)) {
            throw std::invalid_argument("vulkan rhi vertex buffer handle is unknown");
        }
        if (binding.stride == 0) {
            throw std::invalid_argument("vulkan rhi vertex buffer stride must be non-zero");
        }
        const auto& desc = device_->buffer_descs_.at(binding.buffer.value - 1U);
        if (!has_flag(desc.usage, BufferUsage::vertex)) {
            throw std::invalid_argument("vulkan rhi vertex buffer binding requires vertex usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("vulkan rhi vertex buffer offset is outside the buffer");
        }
        vertex_buffer_bound_ = true;
        bound_vertex_buffer_ = binding;
        ++device_->stats_.vertex_buffer_bindings;
    }

    void bind_index_buffer(const IndexBufferBinding& binding) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi index buffers must be bound inside a render pass");
        }
        if (!device_->owns_graphics_pipeline(bound_graphics_pipeline_)) {
            throw std::logic_error("vulkan rhi index buffer binding requires a graphics pipeline");
        }
        if (!device_->owns_buffer(binding.buffer)) {
            throw std::invalid_argument("vulkan rhi index buffer handle is unknown");
        }
        if (binding.format == IndexFormat::unknown) {
            throw std::invalid_argument("vulkan rhi index buffer format must be known");
        }
        const auto& desc = device_->buffer_descs_.at(binding.buffer.value - 1U);
        if (!has_flag(desc.usage, BufferUsage::index)) {
            throw std::invalid_argument("vulkan rhi index buffer binding requires index usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("vulkan rhi index buffer offset is outside the buffer");
        }
        index_buffer_bound_ = true;
        bound_index_buffer_ = binding;
        ++device_->stats_.index_buffer_bindings;
    }

    void draw(std::uint32_t vertex_count, std::uint32_t instance_count) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi draw requires an active render pass");
        }
        if (!device_->owns_graphics_pipeline(bound_graphics_pipeline_)) {
            throw std::logic_error("vulkan rhi draw requires a bound graphics pipeline");
        }
        if (vertex_count == 0 || instance_count == 0) {
            throw std::invalid_argument("vulkan rhi draw counts must be non-zero");
        }
        const auto dynamic_plan = make_dynamic_rendering_plan();
        const auto color_load_action = color_load_action_for_next_draw();
        const auto depth_load_action = depth_load_action_for_next_draw();
        auto* vertex_buffer =
            vertex_buffer_bound_ ? &device_->buffers_.at(bound_vertex_buffer_.buffer.value - 1U) : nullptr;
        VulkanRuntimeDynamicRenderingDrawResult result;
        if (active_texture_.value != 0) {
            result = record_runtime_texture_rendering_draw(
                device_->device_, pool_, device_->textures_.at(active_texture_.value - 1U),
                device_->graphics_pipelines_.at(bound_graphics_pipeline_.value - 1U),
                VulkanRuntimeTextureRenderingDrawDesc{
                    .dynamic_rendering = dynamic_plan,
                    .vertex_count = vertex_count,
                    .instance_count = instance_count,
                    .first_vertex = 0,
                    .first_instance = 0,
                    .vertex_buffer = vertex_buffer,
                    .vertex_buffer_offset = vertex_buffer_bound_ ? bound_vertex_buffer_.offset : 0,
                    .vertex_buffer_binding = vertex_buffer_bound_ ? bound_vertex_buffer_.binding : 0,
                    .index_buffer = nullptr,
                    .index_buffer_offset = 0,
                    .index_format = IndexFormat::unknown,
                    .index_count = 0,
                    .color_load_action = color_load_action,
                    .color_store_action = active_render_pass_.color.store_action,
                    .clear_color = active_render_pass_.color.clear_color,
                    .depth_texture = active_depth_texture(),
                    .depth_load_action = depth_load_action,
                    .depth_store_action = active_render_pass_.depth.store_action,
                    .clear_depth = active_render_pass_.depth.clear_depth,
                });
        } else {
            const auto swapchain_index = active_swapchain_.value - 1U;
            result = record_runtime_dynamic_rendering_draw(
                device_->device_, pool_, device_->swapchains_.at(swapchain_index),
                device_->graphics_pipelines_.at(bound_graphics_pipeline_.value - 1U),
                VulkanRuntimeDynamicRenderingDrawDesc{
                    .dynamic_rendering = dynamic_plan,
                    .image_index = device_->swapchain_image_index_for_frame(active_swapchain_frame_),
                    .vertex_count = vertex_count,
                    .instance_count = instance_count,
                    .first_vertex = 0,
                    .first_instance = 0,
                    .vertex_buffer = vertex_buffer,
                    .vertex_buffer_offset = vertex_buffer_bound_ ? bound_vertex_buffer_.offset : 0,
                    .vertex_buffer_binding = vertex_buffer_bound_ ? bound_vertex_buffer_.binding : 0,
                    .index_buffer = nullptr,
                    .index_buffer_offset = 0,
                    .index_format = IndexFormat::unknown,
                    .index_count = 0,
                    .color_load_action = color_load_action,
                    .color_store_action = active_render_pass_.color.store_action,
                    .clear_color = active_render_pass_.color.clear_color,
                    .depth_texture = active_depth_texture(),
                    .depth_load_action = depth_load_action,
                    .depth_store_action = active_render_pass_.depth.store_action,
                    .clear_depth = active_render_pass_.depth.clear_depth,
                });
        }
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi draw recording failed: " + result.diagnostic);
        }

        rendering_recorded_ = true;
        ++device_->stats_.draw_calls;
        device_->stats_.vertices_submitted += static_cast<std::uint64_t>(vertex_count) * instance_count;
    }

    void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi indexed draw requires an active render pass");
        }
        if (!device_->owns_graphics_pipeline(bound_graphics_pipeline_)) {
            throw std::logic_error("vulkan rhi indexed draw requires a bound graphics pipeline");
        }
        if (!vertex_buffer_bound_) {
            throw std::logic_error("vulkan rhi indexed draw requires a vertex buffer");
        }
        if (!index_buffer_bound_) {
            throw std::logic_error("vulkan rhi indexed draw requires an index buffer");
        }
        if (index_count == 0 || instance_count == 0) {
            throw std::invalid_argument("vulkan rhi indexed draw counts must be non-zero");
        }
        const auto dynamic_plan = make_dynamic_rendering_plan();
        const auto color_load_action = color_load_action_for_next_draw();
        const auto depth_load_action = depth_load_action_for_next_draw();
        VulkanRuntimeDynamicRenderingDrawResult result;
        if (active_texture_.value != 0) {
            result = record_runtime_texture_rendering_draw(
                device_->device_, pool_, device_->textures_.at(active_texture_.value - 1U),
                device_->graphics_pipelines_.at(bound_graphics_pipeline_.value - 1U),
                VulkanRuntimeTextureRenderingDrawDesc{
                    .dynamic_rendering = dynamic_plan,
                    .vertex_count = 1,
                    .instance_count = instance_count,
                    .first_vertex = 0,
                    .first_instance = 0,
                    .vertex_buffer = &device_->buffers_.at(bound_vertex_buffer_.buffer.value - 1U),
                    .vertex_buffer_offset = bound_vertex_buffer_.offset,
                    .vertex_buffer_binding = bound_vertex_buffer_.binding,
                    .index_buffer = &device_->buffers_.at(bound_index_buffer_.buffer.value - 1U),
                    .index_buffer_offset = bound_index_buffer_.offset,
                    .index_format = bound_index_buffer_.format,
                    .index_count = index_count,
                    .color_load_action = color_load_action,
                    .color_store_action = active_render_pass_.color.store_action,
                    .clear_color = active_render_pass_.color.clear_color,
                    .depth_texture = active_depth_texture(),
                    .depth_load_action = depth_load_action,
                    .depth_store_action = active_render_pass_.depth.store_action,
                    .clear_depth = active_render_pass_.depth.clear_depth,
                });
        } else {
            const auto swapchain_index = active_swapchain_.value - 1U;
            result = record_runtime_dynamic_rendering_draw(
                device_->device_, pool_, device_->swapchains_.at(swapchain_index),
                device_->graphics_pipelines_.at(bound_graphics_pipeline_.value - 1U),
                VulkanRuntimeDynamicRenderingDrawDesc{
                    .dynamic_rendering = dynamic_plan,
                    .image_index = device_->swapchain_image_index_for_frame(active_swapchain_frame_),
                    .vertex_count = 1,
                    .instance_count = instance_count,
                    .first_vertex = 0,
                    .first_instance = 0,
                    .vertex_buffer = &device_->buffers_.at(bound_vertex_buffer_.buffer.value - 1U),
                    .vertex_buffer_offset = bound_vertex_buffer_.offset,
                    .vertex_buffer_binding = bound_vertex_buffer_.binding,
                    .index_buffer = &device_->buffers_.at(bound_index_buffer_.buffer.value - 1U),
                    .index_buffer_offset = bound_index_buffer_.offset,
                    .index_format = bound_index_buffer_.format,
                    .index_count = index_count,
                    .color_load_action = color_load_action,
                    .color_store_action = active_render_pass_.color.store_action,
                    .clear_color = active_render_pass_.color.clear_color,
                    .depth_texture = active_depth_texture(),
                    .depth_load_action = depth_load_action,
                    .depth_store_action = active_render_pass_.depth.store_action,
                    .clear_depth = active_render_pass_.depth.clear_depth,
                });
        }
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi indexed draw recording failed: " + result.diagnostic);
        }

        rendering_recorded_ = true;
        ++device_->stats_.draw_calls;
        ++device_->stats_.indexed_draw_calls;
        device_->stats_.indices_submitted += static_cast<std::uint64_t>(index_count) * instance_count;
    }

    void dispatch(std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z) override {
        require_open();
        require_no_render_pass();
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("vulkan rhi dispatch requires a compute command list");
        }
        if (!device_->owns_compute_pipeline(bound_compute_pipeline_)) {
            throw std::logic_error("vulkan rhi dispatch requires a bound compute pipeline");
        }
        if (group_count_x == 0 || group_count_y == 0 || group_count_z == 0) {
            throw std::invalid_argument("vulkan rhi dispatch workgroup counts must be non-zero");
        }

        const auto result = record_runtime_compute_dispatch(device_->device_, pool_,
                                                            VulkanRuntimeComputeDispatchDesc{
                                                                .group_count_x = group_count_x,
                                                                .group_count_y = group_count_y,
                                                                .group_count_z = group_count_z,
                                                            });
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi compute dispatch failed: " + result.diagnostic);
        }

        recorded_work_ = true;
        ++device_->stats_.compute_dispatches;
        device_->stats_.compute_workgroups_x += group_count_x;
        device_->stats_.compute_workgroups_y += group_count_y;
        device_->stats_.compute_workgroups_z += group_count_z;
    }

    void set_viewport(const ViewportDesc& viewport) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi set_viewport requires an active render pass");
        }
        if (!device_->vulkan_runtime_device().record_primary_viewport(pool_, viewport)) {
            throw std::logic_error("vulkan rhi set_viewport failed");
        }
        recorded_work_ = true;
    }

    void set_scissor(const ScissorRectDesc& scissor) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("vulkan rhi set_scissor requires an active render pass");
        }
        if (!device_->vulkan_runtime_device().record_primary_scissor(pool_, scissor)) {
            throw std::logic_error("vulkan rhi set_scissor failed");
        }
        recorded_work_ = true;
    }

    void begin_gpu_debug_scope(std::string_view name) override {
        require_open();
        validate_rhi_debug_label(name);
        ++gpu_debug_scope_depth_;
        ++device_->stats_.gpu_debug_scopes_begun;
    }

    void end_gpu_debug_scope() override {
        require_open();
        if (gpu_debug_scope_depth_ == 0) {
            throw std::logic_error("vulkan rhi gpu debug scope end without matching begin");
        }
        --gpu_debug_scope_depth_;
        ++device_->stats_.gpu_debug_scopes_ended;
    }

    void insert_gpu_debug_marker(std::string_view name) override {
        require_open();
        validate_rhi_debug_label(name);
        ++device_->stats_.gpu_debug_markers_inserted;
    }

    void close() override {
        if (closed_) {
            throw std::logic_error("vulkan rhi command list is already closed");
        }
        if (gpu_debug_scope_depth_ != 0) {
            throw std::logic_error("vulkan rhi command list cannot close with unbalanced gpu debug scopes");
        }
        if (render_pass_active_) {
            throw std::logic_error("vulkan rhi command list cannot close with an active render pass");
        }
        if (has_unpresented_swapchain_frame()) {
            throw std::logic_error("vulkan rhi command list cannot close with an unpresented swapchain frame");
        }
        if (!pool_.end_primary_command_buffer()) {
            throw std::logic_error("vulkan rhi command list close failed");
        }
        closed_ = true;
    }

    [[nodiscard]] const std::vector<SwapchainFrameHandle>& pending_present_frames() const noexcept {
        return pending_present_frames_;
    }

    [[nodiscard]] bool has_recorded_work() const noexcept {
        return recorded_work_;
    }

    [[nodiscard]] LoadAction color_load_action_for_next_draw() const noexcept {
        return rendering_recorded_ ? LoadAction::load : active_render_pass_.color.load_action;
    }

    [[nodiscard]] LoadAction depth_load_action_for_next_draw() const noexcept {
        return rendering_recorded_ ? LoadAction::load : active_render_pass_.depth.load_action;
    }

    void release_submitted_swapchain_frames() {
        submitted_ = true;
        for (const auto frame : reserved_swapchain_frames_) {
            if (device_ != nullptr && device_->owns_swapchain_frame(frame)) {
                device_->complete_swapchain_frame(frame);
            }
        }
        reserved_swapchain_frames_.clear();
        presentable_swapchain_frames_.clear();
        pending_present_frames_.clear();
    }

    void commit_texture_states(std::vector<ResourceState>& destination) const {
        const auto count = std::min(destination.size(), texture_states_.size());
        for (std::size_t index = 0; index < count; ++index) {
            destination.at(index) = texture_states_.at(index);
        }
    }

    [[nodiscard]] bool observed_texture_states_match(const std::vector<ResourceState>& current) const {
        return std::ranges::all_of(observed_textures_, [&](const auto texture) {
            if (texture.value == 0 || texture.value > current.size() ||
                texture.value > initial_texture_states_.size()) {
                return false;
            }
            const auto index = texture.value - 1U;
            return current.at(index) == initial_texture_states_.at(index);
        });
    }

  private:
    friend class VulkanRhiDevice;

    void require_open() const {
        if (closed_) {
            throw std::logic_error("vulkan rhi command list is closed");
        }
        if (device_ == nullptr || !pool_.recording()) {
            throw std::logic_error("vulkan rhi command list is not recording");
        }
    }

    void require_no_render_pass() const {
        if (render_pass_active_) {
            throw std::logic_error("vulkan rhi copy and transition commands must be recorded outside a render pass");
        }
    }

    [[nodiscard]] static bool contains_frame(const std::vector<SwapchainFrameHandle>& frames,
                                             SwapchainFrameHandle frame) noexcept {
        return std::ranges::any_of(frames,
                                   [frame](SwapchainFrameHandle candidate) { return candidate.value == frame.value; });
    }

    void track_swapchain_frame(SwapchainFrameHandle frame) {
        if (contains_frame(reserved_swapchain_frames_, frame)) {
            throw std::invalid_argument("vulkan rhi swapchain frame is already tracked by this command list");
        }
        reserved_swapchain_frames_.push_back(frame);
    }

    [[nodiscard]] ResourceState texture_state(TextureHandle handle) const {
        if (device_ == nullptr || !device_->owns_texture(handle)) {
            throw std::invalid_argument("vulkan rhi texture handle is unknown");
        }
        if (handle.value > texture_states_.size()) {
            throw std::invalid_argument("vulkan rhi texture state is unknown");
        }
        return texture_states_.at(handle.value - 1U);
    }

    void set_texture_state(TextureHandle handle, ResourceState state) {
        if (device_ == nullptr || !device_->owns_texture(handle)) {
            throw std::invalid_argument("vulkan rhi texture handle is unknown");
        }
        if (handle.value > texture_states_.size()) {
            throw std::invalid_argument("vulkan rhi texture state is unknown");
        }
        texture_states_.at(handle.value - 1U) = state;
    }

    void observe_texture(TextureHandle handle) {
        if (device_ == nullptr || !device_->owns_texture(handle)) {
            throw std::invalid_argument("vulkan rhi texture handle is unknown");
        }
        const auto found = std::ranges::find_if(
            observed_textures_, [handle](TextureHandle observed) { return observed.value == handle.value; });
        if (found == observed_textures_.end()) {
            observed_textures_.push_back(handle);
        }
    }

    [[nodiscard]] bool has_unpresented_swapchain_frame() const {
        return std::ranges::any_of(reserved_swapchain_frames_, [this](const auto frame) {
            return device_ != nullptr && device_->owns_swapchain_frame(frame) &&
                   !device_->swapchain_frame_presented_.at(frame.value - 1U);
        });
    }

    [[nodiscard]] Format active_render_pass_color_format() const noexcept {
        if (device_ == nullptr) {
            return Format::unknown;
        }
        if (active_texture_.value != 0 && active_texture_.value <= device_->textures_.size()) {
            return device_->textures_.at(active_texture_.value - 1U).format();
        }
        if (active_swapchain_.value != 0 && active_swapchain_.value <= device_->swapchains_.size()) {
            return device_->swapchains_.at(active_swapchain_.value - 1U).format();
        }
        return Format::unknown;
    }

    [[nodiscard]] Format active_render_pass_depth_format() const noexcept {
        if (device_ == nullptr || active_render_pass_.depth.texture.value == 0 ||
            active_render_pass_.depth.texture.value > device_->texture_descs_.size()) {
            return Format::unknown;
        }
        return device_->texture_descs_.at(active_render_pass_.depth.texture.value - 1U).format;
    }

    void validate_depth_attachment(const RenderPassDepthAttachment& depth, Extent2D color_extent) const {
        if (depth.texture.value == 0) {
            return;
        }
        if (device_ == nullptr || !device_->owns_texture(depth.texture)) {
            throw std::invalid_argument("vulkan rhi render pass depth attachment is unknown");
        }
        const auto& depth_desc = device_->texture_descs_.at(depth.texture.value - 1U);
        if (!has_flag(depth_desc.usage, TextureUsage::depth_stencil)) {
            throw std::invalid_argument("vulkan rhi render pass depth attachment must use depth_stencil");
        }
        if (!depth_format_supported(depth_desc.format)) {
            throw std::invalid_argument("vulkan rhi render pass depth attachment must use a depth format");
        }
        if (depth_desc.extent.width != color_extent.width || depth_desc.extent.height != color_extent.height ||
            depth_desc.extent.depth != 1) {
            throw std::invalid_argument("vulkan rhi render pass depth attachment extent must match the color target");
        }
        if (texture_state(depth.texture) != ResourceState::depth_write) {
            throw std::invalid_argument("vulkan rhi render pass depth attachment must be in depth_write state");
        }
        if (depth.load_action == LoadAction::clear && !valid_clear_depth(depth.clear_depth)) {
            throw std::invalid_argument("vulkan rhi render pass depth clear value must be finite and in [0, 1]");
        }
    }

    [[nodiscard]] VulkanRuntimeTexture* active_depth_texture() const noexcept {
        if (device_ == nullptr || active_render_pass_.depth.texture.value == 0 ||
            active_render_pass_.depth.texture.value > device_->textures_.size()) {
            return nullptr;
        }
        return &device_->textures_.at(active_render_pass_.depth.texture.value - 1U);
    }

    void release_unsubmitted_swapchain_frames() noexcept {
        if (submitted_ || device_ == nullptr) {
            return;
        }
        for (const auto frame : reserved_swapchain_frames_) {
            if (!device_->owns_swapchain_frame(frame)) {
                continue;
            }
            device_->swapchain_frame_presented_.at(frame.value - 1U) = false;
            device_->complete_swapchain_frame(frame);
        }
        reserved_swapchain_frames_.clear();
        presentable_swapchain_frames_.clear();
        pending_present_frames_.clear();
    }

    [[nodiscard]] VulkanFrameSynchronizationPlan make_frame_sync_plan(bool readback_required) const {
        VulkanFrameSynchronizationDesc desc;
        desc.readback_required = readback_required;
        desc.present_required = true;
        const auto plan = build_frame_synchronization_plan(desc, device_->device_.command_plan());
        if (!plan.supported) {
            throw std::invalid_argument("vulkan rhi frame synchronization is unavailable: " + plan.diagnostic);
        }
        if (plan.barriers.size() < 2) {
            throw std::invalid_argument("vulkan rhi frame synchronization plan is incomplete");
        }
        return plan;
    }

    [[nodiscard]] VulkanDynamicRenderingPlan make_dynamic_rendering_plan() const {
        VulkanDynamicRenderingDesc desc;
        if (active_texture_.value != 0) {
            const auto texture_index = active_texture_.value - 1U;
            const auto extent = device_->textures_.at(texture_index).extent();
            desc.extent = Extent2D{.width = extent.width, .height = extent.height};
            desc.color_attachments.push_back(VulkanDynamicRenderingColorAttachmentDesc{
                .format = device_->textures_.at(texture_index).format(),
                .load_action = active_render_pass_.color.load_action,
                .store_action = active_render_pass_.color.store_action,
            });
        } else {
            const auto swapchain_index = active_swapchain_.value - 1U;
            desc.extent = device_->swapchains_.at(swapchain_index).extent();
            desc.color_attachments.push_back(VulkanDynamicRenderingColorAttachmentDesc{
                .format = device_->swapchains_.at(swapchain_index).format(),
                .load_action = active_render_pass_.color.load_action,
                .store_action = active_render_pass_.color.store_action,
            });
        }
        if (active_render_pass_.depth.texture.value != 0) {
            desc.has_depth_attachment = true;
            desc.depth_format = active_render_pass_depth_format();
            desc.depth_load_action = active_render_pass_.depth.load_action;
            desc.depth_store_action = active_render_pass_.depth.store_action;
        }

        const auto plan = build_dynamic_rendering_plan(desc, device_->device_.command_plan());
        if (!plan.supported) {
            throw std::invalid_argument("vulkan rhi dynamic rendering is unavailable: " + plan.diagnostic);
        }
        return plan;
    }

    void record_clear_rendering() {
        const auto dynamic_plan = make_dynamic_rendering_plan();
        if (active_texture_.value != 0) {
            auto result = record_runtime_texture_rendering_clear(
                device_->device_, pool_, device_->textures_.at(active_texture_.value - 1U),
                VulkanRuntimeTextureRenderingClearDesc{
                    .dynamic_rendering = dynamic_plan,
                    .color_store_action = active_render_pass_.color.store_action,
                    .clear_color = active_render_pass_.color.clear_color,
                    .depth_texture = active_depth_texture(),
                    .depth_load_action = active_render_pass_.depth.load_action,
                    .depth_store_action = active_render_pass_.depth.store_action,
                    .clear_depth = active_render_pass_.depth.clear_depth,
                });
            if (!result.recorded) {
                throw std::logic_error("vulkan rhi texture render pass clear failed: " + result.diagnostic);
            }
            rendering_recorded_ = true;
            return;
        }

        const auto swapchain_index = active_swapchain_.value - 1U;
        auto result = record_runtime_dynamic_rendering_clear(
            device_->device_, pool_, device_->swapchains_.at(swapchain_index),
            VulkanRuntimeDynamicRenderingClearDesc{
                .dynamic_rendering = dynamic_plan,
                .image_index = device_->swapchain_image_index_for_frame(active_swapchain_frame_),
                .color_store_action = active_render_pass_.color.store_action,
                .clear_color = active_render_pass_.color.clear_color,
                .depth_texture = active_depth_texture(),
                .depth_load_action = active_render_pass_.depth.load_action,
                .depth_store_action = active_render_pass_.depth.store_action,
                .clear_depth = active_render_pass_.depth.clear_depth,
            });
        if (!result.recorded) {
            throw std::logic_error("vulkan rhi swapchain render pass clear failed: " + result.diagnostic);
        }
        rendering_recorded_ = true;
    }

    VulkanRhiDevice* device_{nullptr};
    VulkanRuntimeCommandPool pool_;
    QueueKind queue_{QueueKind::graphics};
    bool closed_{false};
    bool submitted_{false};
    bool render_pass_active_{false};
    bool rendering_recorded_{false};
    bool recorded_work_{false};
    bool vertex_buffer_bound_{false};
    bool index_buffer_bound_{false};
    RenderPassDesc active_render_pass_;
    SwapchainHandle active_swapchain_;
    SwapchainFrameHandle active_swapchain_frame_;
    TextureHandle active_texture_;
    GraphicsPipelineHandle bound_graphics_pipeline_;
    ComputePipelineHandle bound_compute_pipeline_;
    VertexBufferBinding bound_vertex_buffer_;
    IndexBufferBinding bound_index_buffer_;
    std::vector<SwapchainFrameHandle> reserved_swapchain_frames_;
    std::vector<SwapchainFrameHandle> presentable_swapchain_frames_;
    std::vector<SwapchainFrameHandle> pending_present_frames_;
    std::vector<ResourceState> texture_states_;
    std::vector<ResourceState> initial_texture_states_;
    std::vector<TextureHandle> observed_textures_;
    std::uint32_t gpu_debug_scope_depth_{0};
};

void VulkanRhiDevice::retire_deferred_native_resources(std::uint64_t completed_frame) noexcept {
    struct PendingDestroy {
        RhiResourceHandle handle{};
        RhiResourceKind kind{RhiResourceKind::unknown};
        int rank{99};
    };
    std::vector<PendingDestroy> pending;
    pending.reserve(32);
    for (const auto& rec : resource_lifetime_.records()) {
        if (rec.state != RhiResourceLifetimeState::deferred_release || rec.release_frame > completed_frame) {
            continue;
        }
        pending.push_back(
            PendingDestroy{.handle = rec.handle, .kind = rec.kind, .rank = deferred_vulkan_destroy_rank(rec.kind)});
    }
    if (pending.empty()) {
        (void)resource_lifetime_.retire_released_resources(completed_frame);
        return;
    }
    std::ranges::stable_sort(pending, [](const PendingDestroy& lhs, const PendingDestroy& rhs) {
        if (lhs.rank != rhs.rank) {
            return lhs.rank < rhs.rank;
        }
        return lhs.handle.id.value < rhs.handle.id.value;
    });

    for (const auto& item : pending) {
        switch (item.kind) {
        case RhiResourceKind::buffer: {
            for (std::size_t i = 0; i < buffer_lifetime_.size(); ++i) {
                if (buffer_lifetime_[i] != item.handle) {
                    continue;
                }
                if (buffers_.at(i).owns_buffer()) {
                    buffers_.at(i).reset();
                }
                break;
            }
        } break;
        case RhiResourceKind::texture: {
            for (std::size_t i = 0; i < texture_lifetime_.size(); ++i) {
                if (texture_lifetime_[i] != item.handle) {
                    continue;
                }
                if (textures_.at(i).owns_image()) {
                    textures_.at(i).reset();
                }
                break;
            }
        } break;
        case RhiResourceKind::sampler: {
            for (std::size_t i = 0; i < sampler_lifetime_.size(); ++i) {
                if (sampler_lifetime_[i] != item.handle) {
                    continue;
                }
                if (samplers_.at(i).owns_sampler()) {
                    samplers_.at(i).reset();
                }
                sampler_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::shader: {
            for (std::size_t i = 0; i < shader_lifetime_.size(); ++i) {
                if (shader_lifetime_[i] != item.handle) {
                    continue;
                }
                if (shaders_.at(i).owns_module()) {
                    shaders_.at(i).reset();
                }
                shader_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::descriptor_set_layout: {
            for (std::size_t i = 0; i < descriptor_set_layout_lifetime_.size(); ++i) {
                if (descriptor_set_layout_lifetime_[i] != item.handle) {
                    continue;
                }
                if (descriptor_set_layouts_.at(i).owns_layout()) {
                    descriptor_set_layouts_.at(i).reset();
                }
                descriptor_set_layout_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::descriptor_set: {
            for (std::size_t i = 0; i < descriptor_set_lifetime_.size(); ++i) {
                if (descriptor_set_lifetime_[i] != item.handle) {
                    continue;
                }
                if (descriptor_sets_.at(i).owns_set() || descriptor_sets_.at(i).owns_pool()) {
                    descriptor_sets_.at(i).reset();
                }
                descriptor_set_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::pipeline_layout: {
            for (std::size_t i = 0; i < pipeline_layout_lifetime_.size(); ++i) {
                if (pipeline_layout_lifetime_[i] != item.handle) {
                    continue;
                }
                if (pipeline_layouts_.at(i).owns_layout()) {
                    pipeline_layouts_.at(i).reset();
                }
                pipeline_layout_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::graphics_pipeline: {
            for (std::size_t i = 0; i < graphics_pipeline_lifetime_.size(); ++i) {
                if (graphics_pipeline_lifetime_[i] != item.handle) {
                    continue;
                }
                if (graphics_pipelines_.at(i).owns_pipeline()) {
                    graphics_pipelines_.at(i).reset();
                }
                graphics_pipeline_active_.at(i) = false;
                break;
            }
        } break;
        case RhiResourceKind::compute_pipeline: {
            for (std::size_t i = 0; i < compute_pipeline_lifetime_.size(); ++i) {
                if (compute_pipeline_lifetime_[i] != item.handle) {
                    continue;
                }
                if (compute_pipelines_.at(i).owns_pipeline()) {
                    compute_pipelines_.at(i).reset();
                }
                compute_pipeline_active_.at(i) = false;
                break;
            }
        } break;
        default:
            break;
        }
    }

    (void)resource_lifetime_.retire_released_resources(completed_frame);
}

void VulkanRhiDevice::advance_gpu_timeline_completion(std::uint64_t target_timeline) noexcept {
    if (!device_.owns_device()) {
        return;
    }
    while (!pending_gpu_timeline_.empty()) {
        auto& front = pending_gpu_timeline_.front();
        if (front.timeline_value > target_timeline) {
            break;
        }
        VulkanPendingGpuTimelineEntry entry = std::move(front);
        pending_gpu_timeline_.pop_front();

        NativeVulkanFence vk_fence = 0;
        if (entry.owns_submission_sync) {
            vk_fence = entry.submission_sync.native_in_flight_fence_handle();
        } else if (entry.swapchain_index < swapchain_syncs_.size()) {
            vk_fence = swapchain_syncs_.at(entry.swapchain_index).native_in_flight_fence_handle();
        }

        if (vk_fence != 0) {
            constexpr std::uint64_t no_timeout = std::numeric_limits<std::uint64_t>::max();
            (void)device_.wait_for_fence_signaled(vk_fence, no_timeout);
        }

        stats_.last_completed_fence_value = std::max(stats_.last_completed_fence_value, entry.timeline_value);
    }
}

VulkanRhiDevice::~VulkanRhiDevice() {
    advance_gpu_timeline_completion(std::numeric_limits<std::uint64_t>::max());
    const auto flush_fence = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t i = 0; i < graphics_pipeline_active_.size(); ++i) {
        if (graphics_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(graphics_pipeline_lifetime_[i], 0);
            graphics_pipeline_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < compute_pipeline_active_.size(); ++i) {
        if (compute_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(compute_pipeline_lifetime_[i], 0);
            compute_pipeline_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < pipeline_layout_active_.size(); ++i) {
        if (pipeline_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(pipeline_layout_lifetime_[i], 0);
            pipeline_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_active_.size(); ++i) {
        if (descriptor_set_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_lifetime_[i], 0);
            descriptor_set_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_layout_active_.size(); ++i) {
        if (descriptor_set_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_layout_lifetime_[i], 0);
            descriptor_set_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < shader_active_.size(); ++i) {
        if (shader_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(shader_lifetime_[i], 0);
            shader_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < sampler_active_.size(); ++i) {
        if (sampler_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(sampler_lifetime_[i], 0);
            sampler_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < buffer_active_.size(); ++i) {
        if (buffer_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_[i], 0);
            buffer_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < texture_active_.size(); ++i) {
        if (texture_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(texture_lifetime_[i], 0);
            texture_active_[i] = false;
        }
    }
    retire_deferred_native_resources(flush_fence);
}

std::unique_ptr<IRhiCommandList> VulkanRhiDevice::begin_command_list(QueueKind queue) {
    if (queue != QueueKind::graphics && queue != QueueKind::compute) {
        throw std::invalid_argument("vulkan rhi command queue kind is invalid or unsupported");
    }

    auto result = create_runtime_command_pool(device_, {});
    if (!result.created) {
        throw std::invalid_argument("vulkan rhi command list creation failed: " + result.diagnostic);
    }
    if (!result.pool.begin_primary_command_buffer()) {
        throw std::logic_error("vulkan rhi command list begin failed");
    }

    ++stats_.command_lists_begun;
    return std::unique_ptr<IRhiCommandList>(new VulkanRhiCommandList(*this, std::move(result.pool), queue));
}

FenceValue VulkanRhiDevice::submit(IRhiCommandList& commands) {
    auto* vulkan_commands = dynamic_cast<VulkanRhiCommandList*>(&commands);
    if (vulkan_commands == nullptr || vulkan_commands->device_ != this) {
        throw std::invalid_argument("vulkan rhi command list must belong to this backend");
    }
    if (!vulkan_commands->closed()) {
        throw std::logic_error("vulkan rhi command list must be closed before submit");
    }
    if (!vulkan_commands->observed_texture_states_match(texture_states_)) {
        throw std::logic_error("vulkan rhi command list was recorded against stale texture state");
    }

    const auto& pending_frames = vulkan_commands->pending_present_frames();
    if (pending_frames.empty()) {
        if (!vulkan_commands->has_recorded_work()) {
            throw std::logic_error("vulkan rhi command list submission requires recorded work");
        }

        auto sync_result =
            create_runtime_frame_sync(device_, VulkanRuntimeFrameSyncDesc{.create_image_available_semaphore = false,
                                                                          .create_render_finished_semaphore = false,
                                                                          .create_in_flight_fence = true,
                                                                          .start_in_flight_fence_signaled = false});
        if (!sync_result.created) {
            throw std::logic_error("vulkan rhi command list submission sync creation failed: " +
                                   sync_result.diagnostic);
        }
        const auto submit_result = submit_runtime_command_buffer(
            device_, vulkan_commands->pool_, sync_result.sync,
            VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = false,
                                                 .signal_render_finished_semaphore = false,
                                                 .signal_in_flight_fence = true,
                                                 .wait_for_graphics_queue_idle = true});
        if (!submit_result.submitted) {
            throw std::logic_error("vulkan rhi command list submission failed: " + submit_result.diagnostic);
        }

        const auto timeline = next_fence_value_++;
        const FenceValue fence{.value = timeline, .queue = vulkan_commands->queue_kind()};
        pending_gpu_timeline_.push_back(VulkanPendingGpuTimelineEntry{.timeline_value = timeline,
                                                                      .owns_submission_sync = true,
                                                                      .submission_sync = std::move(sync_result.sync),
                                                                      .swapchain_index = 0});
        vulkan_commands->commit_texture_states(texture_states_);
        ++stats_.command_lists_submitted;
        ++stats_.fences_signaled;
        stats_.last_submitted_fence_value = timeline;
        record_queue_submit(stats_, vulkan_commands->queue_kind(), fence);
        retire_deferred_native_resources(stats_.last_completed_fence_value);
        return fence;
    }
    if (pending_frames.size() > 1) {
        throw std::logic_error("vulkan rhi command list submission supports one queued present");
    }

    const auto frame = pending_frames.front();
    const auto swapchain = swapchain_for_frame(frame);
    const auto swapchain_index = swapchain.value - 1U;
    const auto image_index = swapchain_image_index_for_frame(frame);

    const auto submit_result =
        submit_runtime_command_buffer(device_, vulkan_commands->pool_, swapchain_syncs_.at(swapchain_index),
                                      VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = true,
                                                                           .signal_render_finished_semaphore = true,
                                                                           .signal_in_flight_fence = false,
                                                                           .wait_for_graphics_queue_idle = true});
    if (!submit_result.submitted) {
        throw std::logic_error("vulkan rhi command list submission failed: " + submit_result.diagnostic);
    }

    vulkan_commands->commit_texture_states(texture_states_);
    const auto present_result = present_runtime_swapchain_image(
        device_, swapchains_.at(swapchain_index), swapchain_syncs_.at(swapchain_index),
        VulkanRuntimeSwapchainPresentDesc{.image_index = image_index, .wait_render_finished_semaphore = true});
    if (!present_result.presented && !present_result.resize_required) {
        throw std::logic_error("vulkan rhi swapchain present failed: " + present_result.diagnostic);
    }

    const auto timeline = next_fence_value_++;
    const FenceValue fence{.value = timeline, .queue = vulkan_commands->queue_kind()};
    pending_gpu_timeline_.push_back(VulkanPendingGpuTimelineEntry{.timeline_value = timeline,
                                                                  .owns_submission_sync = false,
                                                                  .submission_sync = {},
                                                                  .swapchain_index = swapchain_index});
    vulkan_commands->release_submitted_swapchain_frames();
    ++stats_.command_lists_submitted;
    ++stats_.fences_signaled;
    ++stats_.present_calls;
    stats_.last_submitted_fence_value = timeline;
    record_queue_submit(stats_, vulkan_commands->queue_kind(), fence);
    retire_deferred_native_resources(stats_.last_completed_fence_value);
    return fence;
}

void VulkanRhiDevice::wait(FenceValue fence) {
    ++stats_.fence_waits;
    if (fence.value > stats_.last_submitted_fence_value) {
        ++stats_.fence_wait_failures;
        throw std::logic_error("vulkan rhi fence wait failed");
    }
    advance_gpu_timeline_completion(fence.value);
    retire_deferred_native_resources(stats_.last_completed_fence_value);
}

void VulkanRhiDevice::wait_for_queue(QueueKind queue, FenceValue fence) {
    ++stats_.queue_waits;
    if (queue != QueueKind::graphics || fence.value > stats_.last_submitted_fence_value) {
        ++stats_.queue_wait_failures;
        throw std::logic_error("vulkan rhi queue wait failed");
    }
    record_queue_wait(stats_, queue, fence);
}

void VulkanRhiDevice::write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) {
    if (!owns_buffer(buffer)) {
        throw std::invalid_argument("vulkan rhi write buffer handle is unknown");
    }
    if (bytes.empty()) {
        throw std::invalid_argument("vulkan rhi write buffer byte span must be non-empty");
    }
    const auto& desc = buffer_descs_.at(buffer.value - 1U);
    if (!has_flag(desc.usage, BufferUsage::copy_source)) {
        throw std::invalid_argument("vulkan rhi write buffer requires a copy_source upload buffer");
    }
    if (offset > desc.size_bytes || bytes.size() > desc.size_bytes - offset) {
        throw std::invalid_argument("vulkan rhi write buffer range is outside the buffer");
    }

    const auto result = write_runtime_buffer(device_, buffers_.at(buffer.value - 1U),
                                             VulkanRuntimeBufferWriteDesc{.byte_offset = offset, .bytes = bytes});
    if (!result.written) {
        throw std::logic_error("vulkan rhi write buffer mapping failed: " + result.diagnostic);
    }

    ++stats_.buffer_writes;
    stats_.bytes_written += static_cast<std::uint64_t>(bytes.size());
}

std::vector<std::uint8_t> VulkanRhiDevice::read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                       std::uint64_t size_bytes) {
    if (!owns_buffer(buffer)) {
        throw std::invalid_argument("vulkan rhi read buffer handle is unknown");
    }
    if (size_bytes == 0) {
        throw std::invalid_argument("vulkan rhi read buffer size must be non-zero");
    }
    const auto& desc = buffer_descs_.at(buffer.value - 1U);
    if (!has_flag(desc.usage, BufferUsage::copy_destination)) {
        throw std::invalid_argument("vulkan rhi read buffer requires copy_destination usage");
    }
    if (offset > desc.size_bytes || size_bytes > desc.size_bytes - offset) {
        throw std::invalid_argument("vulkan rhi read buffer range is outside the buffer");
    }

    const auto result =
        read_runtime_buffer(device_, buffers_.at(buffer.value - 1U),
                            VulkanRuntimeBufferReadDesc{.byte_offset = offset, .byte_count = size_bytes});
    if (!result.read) {
        throw std::logic_error("vulkan rhi read buffer mapping failed: " + result.diagnostic);
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(result.bytes.size());
    for (const auto byte : result.bytes) {
        bytes.push_back(std::to_integer<std::uint8_t>(byte));
    }

    ++stats_.buffer_reads;
    stats_.bytes_read += size_bytes;
    return bytes;
}

struct VulkanRuntimeShaderModule::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanShaderModule module{0};
    ShaderStage stage{ShaderStage::vertex};
    std::uint64_t bytecode_size{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (module != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr &&
            device_owner->destroy_shader_module != nullptr && module != 0) {
            device_owner->destroy_shader_module(device_owner->device, module, nullptr);
        }
        module = 0;
    }
};

VulkanRuntimeShaderModule::VulkanRuntimeShaderModule() noexcept = default;

VulkanRuntimeShaderModule::~VulkanRuntimeShaderModule() = default;

VulkanRuntimeShaderModule::VulkanRuntimeShaderModule(VulkanRuntimeShaderModule&& other) noexcept = default;

VulkanRuntimeShaderModule& VulkanRuntimeShaderModule::operator=(VulkanRuntimeShaderModule&& other) noexcept = default;

VulkanRuntimeShaderModule::VulkanRuntimeShaderModule(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeShaderModule::owns_module() const noexcept {
    return impl_ != nullptr && impl_->module != 0;
}

bool VulkanRuntimeShaderModule::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

ShaderStage VulkanRuntimeShaderModule::stage() const noexcept {
    return impl_ != nullptr ? impl_->stage : ShaderStage::vertex;
}

std::uint64_t VulkanRuntimeShaderModule::bytecode_size() const noexcept {
    return impl_ != nullptr ? impl_->bytecode_size : 0;
}

void VulkanRuntimeShaderModule::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeDescriptorSetLayout::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanDescriptorSetLayout layout{0};
    DescriptorSetLayoutDesc desc;
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (layout != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr &&
            device_owner->destroy_descriptor_set_layout != nullptr && layout != 0) {
            device_owner->destroy_descriptor_set_layout(device_owner->device, layout, nullptr);
        }
        layout = 0;
    }
};

VulkanRuntimeDescriptorSetLayout::VulkanRuntimeDescriptorSetLayout() noexcept = default;

VulkanRuntimeDescriptorSetLayout::~VulkanRuntimeDescriptorSetLayout() = default;

VulkanRuntimeDescriptorSetLayout::VulkanRuntimeDescriptorSetLayout(VulkanRuntimeDescriptorSetLayout&& other) noexcept =
    default;

VulkanRuntimeDescriptorSetLayout&
VulkanRuntimeDescriptorSetLayout::operator=(VulkanRuntimeDescriptorSetLayout&& other) noexcept = default;

VulkanRuntimeDescriptorSetLayout::VulkanRuntimeDescriptorSetLayout(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool VulkanRuntimeDescriptorSetLayout::owns_layout() const noexcept {
    return impl_ != nullptr && impl_->layout != 0;
}

bool VulkanRuntimeDescriptorSetLayout::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

std::uint32_t VulkanRuntimeDescriptorSetLayout::binding_count() const noexcept {
    return impl_ != nullptr ? static_cast<std::uint32_t>(impl_->desc.bindings.size()) : 0;
}

void VulkanRuntimeDescriptorSetLayout::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeDescriptorSet::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanDescriptorPool pool{0};
    NativeVulkanDescriptorSet set{0};
    NativeVulkanDescriptorSetLayout layout{0};
    DescriptorSetLayoutDesc layout_desc;
    std::uint32_t max_sets{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (pool != 0 || set != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr &&
            device_owner->destroy_descriptor_pool != nullptr && pool != 0) {
            device_owner->destroy_descriptor_pool(device_owner->device, pool, nullptr);
        }
        pool = 0;
        set = 0;
    }
};

VulkanRuntimeDescriptorSet::VulkanRuntimeDescriptorSet() noexcept = default;

VulkanRuntimeDescriptorSet::~VulkanRuntimeDescriptorSet() = default;

VulkanRuntimeDescriptorSet::VulkanRuntimeDescriptorSet(VulkanRuntimeDescriptorSet&& other) noexcept = default;

VulkanRuntimeDescriptorSet&
VulkanRuntimeDescriptorSet::operator=(VulkanRuntimeDescriptorSet&& other) noexcept = default;

VulkanRuntimeDescriptorSet::VulkanRuntimeDescriptorSet(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeDescriptorSet::owns_pool() const noexcept {
    return impl_ != nullptr && impl_->pool != 0;
}

bool VulkanRuntimeDescriptorSet::owns_set() const noexcept {
    return impl_ != nullptr && impl_->set != 0;
}

bool VulkanRuntimeDescriptorSet::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

std::uint32_t VulkanRuntimeDescriptorSet::binding_count() const noexcept {
    return impl_ != nullptr ? static_cast<std::uint32_t>(impl_->layout_desc.bindings.size()) : 0;
}

void VulkanRuntimeDescriptorSet::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimePipelineLayout::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanPipelineLayout layout{0};
    std::vector<NativeVulkanDescriptorSetLayout> descriptor_set_layouts;
    std::uint32_t descriptor_set_layout_count{0};
    std::uint32_t push_constant_bytes{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (layout != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr &&
            device_owner->destroy_pipeline_layout != nullptr && layout != 0) {
            device_owner->destroy_pipeline_layout(device_owner->device, layout, nullptr);
        }
        layout = 0;
    }
};

VulkanRuntimePipelineLayout::VulkanRuntimePipelineLayout() noexcept = default;

VulkanRuntimePipelineLayout::~VulkanRuntimePipelineLayout() = default;

VulkanRuntimePipelineLayout::VulkanRuntimePipelineLayout(VulkanRuntimePipelineLayout&& other) noexcept = default;

VulkanRuntimePipelineLayout&
VulkanRuntimePipelineLayout::operator=(VulkanRuntimePipelineLayout&& other) noexcept = default;

VulkanRuntimePipelineLayout::VulkanRuntimePipelineLayout(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool VulkanRuntimePipelineLayout::owns_layout() const noexcept {
    return impl_ != nullptr && impl_->layout != 0;
}

bool VulkanRuntimePipelineLayout::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

std::uint32_t VulkanRuntimePipelineLayout::descriptor_set_layout_count() const noexcept {
    return impl_ != nullptr ? impl_->descriptor_set_layout_count : 0;
}

std::uint32_t VulkanRuntimePipelineLayout::push_constant_bytes() const noexcept {
    return impl_ != nullptr ? impl_->push_constant_bytes : 0;
}

void VulkanRuntimePipelineLayout::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeGraphicsPipeline::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanPipeline pipeline{0};
    Format color_format{Format::unknown};
    Format depth_format{Format::unknown};
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (pipeline != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr && device_owner->destroy_pipeline != nullptr &&
            pipeline != 0) {
            device_owner->destroy_pipeline(device_owner->device, pipeline, nullptr);
        }
        pipeline = 0;
    }
};

VulkanRuntimeGraphicsPipeline::VulkanRuntimeGraphicsPipeline() noexcept = default;

VulkanRuntimeGraphicsPipeline::~VulkanRuntimeGraphicsPipeline() = default;

VulkanRuntimeGraphicsPipeline::VulkanRuntimeGraphicsPipeline(VulkanRuntimeGraphicsPipeline&& other) noexcept = default;

VulkanRuntimeGraphicsPipeline&
VulkanRuntimeGraphicsPipeline::operator=(VulkanRuntimeGraphicsPipeline&& other) noexcept = default;

VulkanRuntimeGraphicsPipeline::VulkanRuntimeGraphicsPipeline(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool VulkanRuntimeGraphicsPipeline::owns_pipeline() const noexcept {
    return impl_ != nullptr && impl_->pipeline != 0;
}

bool VulkanRuntimeGraphicsPipeline::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

Format VulkanRuntimeGraphicsPipeline::color_format() const noexcept {
    return impl_ != nullptr ? impl_->color_format : Format::unknown;
}

Format VulkanRuntimeGraphicsPipeline::depth_format() const noexcept {
    return impl_ != nullptr ? impl_->depth_format : Format::unknown;
}

PrimitiveTopology VulkanRuntimeGraphicsPipeline::topology() const noexcept {
    return impl_ != nullptr ? impl_->topology : PrimitiveTopology::triangle_list;
}

void VulkanRuntimeGraphicsPipeline::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeComputePipeline::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanPipeline pipeline{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (pipeline != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr && device_owner->destroy_pipeline != nullptr &&
            pipeline != 0) {
            device_owner->destroy_pipeline(device_owner->device, pipeline, nullptr);
        }
        pipeline = 0;
    }
};

VulkanRuntimeComputePipeline::VulkanRuntimeComputePipeline() noexcept = default;

VulkanRuntimeComputePipeline::~VulkanRuntimeComputePipeline() = default;

VulkanRuntimeComputePipeline::VulkanRuntimeComputePipeline(VulkanRuntimeComputePipeline&& other) noexcept = default;

VulkanRuntimeComputePipeline&
VulkanRuntimeComputePipeline::operator=(VulkanRuntimeComputePipeline&& other) noexcept = default;

VulkanRuntimeComputePipeline::VulkanRuntimeComputePipeline(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool VulkanRuntimeComputePipeline::owns_pipeline() const noexcept {
    return impl_ != nullptr && impl_->pipeline != 0;
}

bool VulkanRuntimeComputePipeline::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void VulkanRuntimeComputePipeline::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeSwapchain::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanSurface surface{0};
    NativeVulkanSwapchain swapchain{0};
    std::vector<NativeVulkanImage> images;
    std::vector<NativeVulkanImageView> image_views;
    Extent2D extent;
    Format format{Format::unknown};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (surface != 0 || swapchain != 0 || !image_views.empty()) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr) {
            if (device_owner->destroy_image_view != nullptr) {
                for (const auto view : image_views) {
                    if (view != 0) {
                        device_owner->destroy_image_view(device_owner->device, view, nullptr);
                    }
                }
            }
            if (device_owner->destroy_swapchain != nullptr && swapchain != 0) {
                device_owner->destroy_swapchain(device_owner->device, swapchain, nullptr);
            }
        }
        image_views.clear();
        images.clear();
        swapchain = 0;

        if (device_owner != nullptr && device_owner->instance != nullptr && device_owner->destroy_surface != nullptr &&
            surface != 0) {
            device_owner->destroy_surface(device_owner->instance, surface, nullptr);
        }
        surface = 0;
    }
};

VulkanRuntimeSwapchain::VulkanRuntimeSwapchain() noexcept = default;

VulkanRuntimeSwapchain::~VulkanRuntimeSwapchain() = default;

VulkanRuntimeSwapchain::VulkanRuntimeSwapchain(VulkanRuntimeSwapchain&& other) noexcept = default;

VulkanRuntimeSwapchain& VulkanRuntimeSwapchain::operator=(VulkanRuntimeSwapchain&& other) noexcept = default;

VulkanRuntimeSwapchain::VulkanRuntimeSwapchain(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeSwapchain::owns_surface() const noexcept {
    return impl_ != nullptr && impl_->surface != 0;
}

bool VulkanRuntimeSwapchain::owns_swapchain() const noexcept {
    return impl_ != nullptr && impl_->swapchain != 0;
}

bool VulkanRuntimeSwapchain::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

Extent2D VulkanRuntimeSwapchain::extent() const noexcept {
    return impl_ != nullptr ? impl_->extent : Extent2D{};
}

Format VulkanRuntimeSwapchain::format() const noexcept {
    return impl_ != nullptr ? impl_->format : Format::unknown;
}

std::uint32_t VulkanRuntimeSwapchain::image_count() const noexcept {
    return impl_ != nullptr ? static_cast<std::uint32_t>(impl_->images.size()) : 0;
}

std::uint32_t VulkanRuntimeSwapchain::image_view_count() const noexcept {
    return impl_ != nullptr ? static_cast<std::uint32_t>(impl_->image_views.size()) : 0;
}

void VulkanRuntimeSwapchain::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeFrameSync::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanSemaphore image_available_semaphore{0};
    NativeVulkanSemaphore render_finished_semaphore{0};
    NativeVulkanFence in_flight_fence{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (image_available_semaphore != 0 || render_finished_semaphore != 0 || in_flight_fence != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr) {
            if (device_owner->destroy_fence != nullptr && in_flight_fence != 0) {
                device_owner->destroy_fence(device_owner->device, in_flight_fence, nullptr);
            }
            if (device_owner->destroy_semaphore != nullptr) {
                if (render_finished_semaphore != 0) {
                    device_owner->destroy_semaphore(device_owner->device, render_finished_semaphore, nullptr);
                }
                if (image_available_semaphore != 0) {
                    device_owner->destroy_semaphore(device_owner->device, image_available_semaphore, nullptr);
                }
            }
        }
        image_available_semaphore = 0;
        render_finished_semaphore = 0;
        in_flight_fence = 0;
    }
};

VulkanRuntimeFrameSync::VulkanRuntimeFrameSync() noexcept = default;

VulkanRuntimeFrameSync::~VulkanRuntimeFrameSync() = default;

VulkanRuntimeFrameSync::VulkanRuntimeFrameSync(VulkanRuntimeFrameSync&& other) noexcept = default;

VulkanRuntimeFrameSync& VulkanRuntimeFrameSync::operator=(VulkanRuntimeFrameSync&& other) noexcept = default;

VulkanRuntimeFrameSync::VulkanRuntimeFrameSync(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool VulkanRuntimeFrameSync::owns_image_available_semaphore() const noexcept {
    return impl_ != nullptr && impl_->image_available_semaphore != 0;
}

bool VulkanRuntimeFrameSync::owns_render_finished_semaphore() const noexcept {
    return impl_ != nullptr && impl_->render_finished_semaphore != 0;
}

bool VulkanRuntimeFrameSync::owns_in_flight_fence() const noexcept {
    return impl_ != nullptr && impl_->in_flight_fence != 0;
}

std::uint64_t VulkanRuntimeFrameSync::native_in_flight_fence_handle() const noexcept {
    return impl_ != nullptr ? static_cast<std::uint64_t>(impl_->in_flight_fence) : 0ULL;
}

bool VulkanRuntimeFrameSync::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void VulkanRuntimeFrameSync::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

struct VulkanRuntimeReadbackBuffer::Impl {
    std::shared_ptr<VulkanRuntimeDevice::Impl> device_owner;
    NativeVulkanBuffer buffer{0};
    NativeVulkanDeviceMemory memory{0};
    std::uint64_t byte_size{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (buffer != 0 || memory != 0) {
            destroyed = true;
        }
        if (device_owner != nullptr && device_owner->device != nullptr) {
            if (device_owner->destroy_buffer != nullptr && buffer != 0) {
                device_owner->destroy_buffer(device_owner->device, buffer, nullptr);
            }
            if (device_owner->free_memory != nullptr && memory != 0) {
                device_owner->free_memory(device_owner->device, memory, nullptr);
            }
        }
        buffer = 0;
        memory = 0;
        byte_size = 0;
    }
};

VulkanRuntimeReadbackBuffer::VulkanRuntimeReadbackBuffer() noexcept = default;

VulkanRuntimeReadbackBuffer::~VulkanRuntimeReadbackBuffer() = default;

VulkanRuntimeReadbackBuffer::VulkanRuntimeReadbackBuffer(VulkanRuntimeReadbackBuffer&& other) noexcept = default;

VulkanRuntimeReadbackBuffer&
VulkanRuntimeReadbackBuffer::operator=(VulkanRuntimeReadbackBuffer&& other) noexcept = default;

VulkanRuntimeReadbackBuffer::VulkanRuntimeReadbackBuffer(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool VulkanRuntimeReadbackBuffer::owns_buffer() const noexcept {
    return impl_ != nullptr && impl_->buffer != 0;
}

bool VulkanRuntimeReadbackBuffer::owns_memory() const noexcept {
    return impl_ != nullptr && impl_->memory != 0;
}

bool VulkanRuntimeReadbackBuffer::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

std::uint64_t VulkanRuntimeReadbackBuffer::byte_size() const noexcept {
    return impl_ != nullptr ? impl_->byte_size : 0;
}

void VulkanRuntimeReadbackBuffer::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
}

BackendKind backend_kind() noexcept {
    return BackendKind::vulkan;
}

std::string_view backend_name() noexcept {
    return "vulkan";
}

BackendCapabilityProfile capabilities() noexcept {
    return default_backend_capabilities(BackendKind::vulkan);
}

BackendProbePlan probe_plan() noexcept {
    return backend_probe_plan(BackendKind::vulkan);
}

bool supports_host(RhiHostPlatform host) noexcept {
    return is_backend_supported_on_host(BackendKind::vulkan, host);
}

BackendProbeResult make_probe_result(RhiHostPlatform host, BackendProbeStatus status, std::string_view diagnostic) {
    return make_backend_probe_result(BackendKind::vulkan, host, status, diagnostic);
}

VulkanApiVersion make_vulkan_api_version(std::uint32_t major, std::uint32_t minor) noexcept {
    return VulkanApiVersion{.major = major, .minor = minor};
}

std::uint32_t encode_vulkan_api_version(VulkanApiVersion version, std::uint32_t patch) noexcept {
    return ((version.major & 0x7FU) << 22U) | ((version.minor & 0x3FFU) << 12U) | (patch & 0xFFFU);
}

VulkanApiVersion decode_vulkan_api_version(std::uint32_t encoded) noexcept {
    return VulkanApiVersion{
        .major = (encoded >> 22U) & 0x7FU,
        .minor = (encoded >> 12U) & 0x3FFU,
    };
}

bool is_vulkan_api_at_least(VulkanApiVersion actual, VulkanApiVersion required) noexcept {
    if (actual.major != required.major) {
        return actual.major > required.major;
    }
    return actual.minor >= required.minor;
}

VulkanDeviceSelection select_physical_device(std::initializer_list<VulkanPhysicalDeviceCandidate> devices) {
    return select_physical_device_impl(devices);
}

VulkanDeviceSelection select_physical_device(const std::vector<VulkanPhysicalDeviceCandidate>& devices) {
    return select_physical_device_impl(devices);
}

VulkanInstanceCreatePlan build_instance_create_plan(const VulkanInstanceCreateDesc& desc,
                                                    std::initializer_list<std::string_view> available_extensions) {
    return build_instance_create_plan_impl(desc, available_extensions);
}

VulkanInstanceCreatePlan build_instance_create_plan(const VulkanInstanceCreateDesc& desc,
                                                    const std::vector<std::string>& available_extensions) {
    return build_instance_create_plan_impl(desc, available_extensions);
}

VulkanLogicalDeviceCreatePlan
build_logical_device_create_plan(const VulkanLogicalDeviceCreateDesc& desc, const VulkanPhysicalDeviceCandidate& device,
                                 const VulkanDeviceSelection& selection,
                                 std::initializer_list<std::string_view> available_device_extensions) {
    return build_logical_device_create_plan_impl(desc, device, selection, available_device_extensions);
}

VulkanLogicalDeviceCreatePlan
build_logical_device_create_plan(const VulkanLogicalDeviceCreateDesc& desc, const VulkanPhysicalDeviceCandidate& device,
                                 const VulkanDeviceSelection& selection,
                                 const std::vector<std::string>& available_device_extensions) {
    return build_logical_device_create_plan_impl(desc, device, selection, available_device_extensions);
}

VulkanSwapchainCreatePlan build_swapchain_create_plan(const VulkanSwapchainCreateDesc& desc,
                                                      const VulkanSwapchainSupport& support) {
    VulkanSwapchainCreatePlan plan;
    if (support.formats.empty()) {
        plan.diagnostic = "Vulkan surface exposes no color formats";
        return plan;
    }
    if (support.present_modes.empty()) {
        plan.diagnostic = "Vulkan surface exposes no present modes";
        return plan;
    }
    if (!valid_extent(desc.requested_extent) && !support.capabilities.current_extent_defined) {
        plan.diagnostic = "Vulkan swapchain extent is required";
        return plan;
    }

    const auto format = choose_surface_format(desc, support.formats);
    if (format == Format::unknown) {
        plan.diagnostic = "Vulkan surface exposes no supported color formats";
        return plan;
    }

    plan.extent = support.capabilities.current_extent_defined
                      ? support.capabilities.current_extent
                      : clamp_extent(desc.requested_extent, support.capabilities.min_image_extent,
                                     support.capabilities.max_image_extent);
    if (!valid_extent(plan.extent)) {
        plan.diagnostic = "Vulkan swapchain extent is required";
        return plan;
    }

    plan.supported = true;
    plan.format = format;
    plan.image_count = choose_swapchain_image_count(desc, support.capabilities);
    plan.image_view_count = plan.image_count;
    plan.present_mode = choose_present_mode(desc, support.present_modes);
    plan.acquire_before_render = true;
    plan.diagnostic = "Vulkan swapchain create plan ready";
    return plan;
}

VulkanSwapchainResizePlan build_swapchain_resize_plan(const VulkanSwapchainCreatePlan& current_plan,
                                                      Extent2D requested_extent) {
    VulkanSwapchainResizePlan plan;
    if (!current_plan.supported) {
        plan.diagnostic = "Vulkan swapchain create plan is not supported";
        return plan;
    }
    if (!valid_extent(requested_extent)) {
        plan.diagnostic = "Vulkan swapchain resize extent is required";
        return plan;
    }

    plan.extent = requested_extent;
    plan.resize_required =
        requested_extent.width != current_plan.extent.width || requested_extent.height != current_plan.extent.height;
    plan.diagnostic =
        plan.resize_required ? "Vulkan swapchain resize required" : "Vulkan swapchain resize not required";
    return plan;
}

VulkanDynamicRenderingPlan build_dynamic_rendering_plan(const VulkanDynamicRenderingDesc& desc,
                                                        const VulkanCommandResolutionPlan& command_plan) {
    VulkanDynamicRenderingPlan plan;
    plan.extent = desc.extent;
    plan.begin_rendering_command_resolved =
        command_is_resolved(command_plan, VulkanCommandScope::device, "vkCmdBeginRendering");
    plan.end_rendering_command_resolved =
        command_is_resolved(command_plan, VulkanCommandScope::device, "vkCmdEndRendering");

    if (!valid_extent(desc.extent)) {
        plan.diagnostic = "Vulkan dynamic rendering extent is required";
        return plan;
    }
    if (desc.color_attachments.empty()) {
        plan.diagnostic = "Vulkan dynamic rendering requires at least one color attachment";
        return plan;
    }
    if (!plan.begin_rendering_command_resolved || !plan.end_rendering_command_resolved) {
        plan.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return plan;
    }

    plan.color_formats.reserve(desc.color_attachments.size());
    for (const auto& attachment : desc.color_attachments) {
        if (!dynamic_rendering_color_format_supported(attachment.format)) {
            plan.diagnostic = "Vulkan dynamic rendering color attachment format is unsupported";
            return plan;
        }
        plan.color_formats.push_back(attachment.format);
    }

    if (desc.has_depth_attachment) {
        if (!dynamic_rendering_depth_format_supported(desc.depth_format)) {
            plan.diagnostic = "Vulkan dynamic rendering depth attachment format is unsupported";
            return plan;
        }
        plan.depth_attachment_enabled = true;
        plan.depth_format = desc.depth_format;
    }

    plan.supported = true;
    plan.color_attachment_count = static_cast<std::uint32_t>(plan.color_formats.size());
    plan.diagnostic = "Vulkan dynamic rendering plan ready";
    return plan;
}

VulkanFrameSynchronizationPlan build_frame_synchronization_plan(const VulkanFrameSynchronizationDesc& desc,
                                                                const VulkanCommandResolutionPlan& command_plan) {
    VulkanFrameSynchronizationPlan plan;
    plan.pipeline_barrier2_command_resolved =
        command_is_resolved(command_plan, VulkanCommandScope::device, "vkCmdPipelineBarrier2");
    plan.queue_submit2_command_resolved =
        command_is_resolved(command_plan, VulkanCommandScope::device, "vkQueueSubmit2");

    if (!plan.pipeline_barrier2_command_resolved || !plan.queue_submit2_command_resolved) {
        plan.diagnostic = "Vulkan synchronization2 commands are unavailable";
        return plan;
    }

    plan.order.push_back(VulkanFrameSynchronizationStep::acquire);
    plan.order.push_back(VulkanFrameSynchronizationStep::render);
    plan.barriers.push_back(make_frame_barrier(ResourceState::present, ResourceState::render_target,
                                               VulkanSynchronizationStage::none, VulkanSynchronizationAccess::none,
                                               VulkanSynchronizationStage::color_attachment_output,
                                               VulkanSynchronizationAccess::color_attachment_write));

    if (desc.readback_required) {
        plan.order.push_back(VulkanFrameSynchronizationStep::readback);
        plan.barriers.push_back(make_frame_barrier(
            ResourceState::render_target, ResourceState::copy_source,
            VulkanSynchronizationStage::color_attachment_output, VulkanSynchronizationAccess::color_attachment_write,
            VulkanSynchronizationStage::transfer, VulkanSynchronizationAccess::transfer_read));
    }

    if (desc.present_required) {
        plan.order.push_back(VulkanFrameSynchronizationStep::present);
        if (desc.readback_required) {
            plan.barriers.push_back(
                make_frame_barrier(ResourceState::copy_source, ResourceState::present,
                                   VulkanSynchronizationStage::transfer, VulkanSynchronizationAccess::transfer_read,
                                   VulkanSynchronizationStage::none, VulkanSynchronizationAccess::none));
        } else {
            plan.barriers.push_back(make_frame_barrier(ResourceState::render_target, ResourceState::present,
                                                       VulkanSynchronizationStage::color_attachment_output,
                                                       VulkanSynchronizationAccess::color_attachment_write,
                                                       VulkanSynchronizationStage::none,
                                                       VulkanSynchronizationAccess::none));
        }
    }

    plan.supported = true;
    plan.diagnostic = "Vulkan frame synchronization2 plan ready";
    return plan;
}

VulkanTextureTransitionBarrierPlan build_texture_transition_barrier(ResourceState before, ResourceState after) {
    VulkanTextureTransitionBarrierPlan plan;
    plan.barrier.before = before;
    plan.barrier.after = after;

    if (before == after) {
        plan.diagnostic = "Vulkan texture barrier state transition is required";
        return plan;
    }
    if (!texture_barrier_state_supported(before) || !texture_barrier_state_supported(after)) {
        plan.diagnostic = "Vulkan texture barrier state is unsupported";
        return plan;
    }

    plan.barrier.src_stage = vulkan_texture_barrier_stage(before);
    plan.barrier.src_access = vulkan_texture_barrier_access(before);
    plan.barrier.dst_stage = vulkan_texture_barrier_stage(after);
    plan.barrier.dst_access = vulkan_texture_barrier_access(after);
    plan.supported = true;
    plan.diagnostic = "Vulkan texture transition barrier ready";
    return plan;
}

VulkanSpirvShaderArtifactValidation validate_spirv_shader_artifact(const VulkanSpirvShaderArtifactDesc& desc) {
    VulkanSpirvShaderArtifactValidation validation;
    if (desc.bytecode == nullptr || desc.bytecode_size == 0) {
        validation.diagnostic = "SPIR-V bytecode is required";
        return validation;
    }
    if ((desc.bytecode_size % sizeof(std::uint32_t)) != 0) {
        validation.diagnostic = "SPIR-V bytecode size must be a multiple of 4 bytes";
        return validation;
    }

    validation.word_count = static_cast<std::uint32_t>(desc.bytecode_size / sizeof(std::uint32_t));
    if (validation.word_count < spirv_header_word_count) {
        validation.diagnostic = "SPIR-V bytecode header is incomplete";
        return validation;
    }

    std::uint32_t magic = 0;
    std::memcpy(&magic, desc.bytecode, sizeof(magic));
    if (magic != spirv_magic_word) {
        validation.diagnostic = "SPIR-V magic word is invalid";
        return validation;
    }

    validation.valid = true;
    validation.diagnostic = "SPIR-V shader artifact ready";
    return validation;
}

VulkanRhiDeviceMappingPlan build_rhi_device_mapping_plan(const VulkanRhiDeviceMappingDesc& desc) {
    VulkanRhiDeviceMappingPlan plan;
    if (!desc.command_pool_ready) {
        plan.diagnostic = "Vulkan command pool is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.swapchain.supported) {
        plan.diagnostic = "Vulkan swapchain plan is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.dynamic_rendering.supported) {
        plan.diagnostic = "Vulkan dynamic rendering plan is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.frame_synchronization.supported) {
        plan.diagnostic = "Vulkan synchronization2 plan is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.vertex_shader.valid || !desc.fragment_shader.valid) {
        plan.diagnostic = "Vulkan valid SPIR-V vertex and fragment shaders are required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.compute_shader.valid) {
        plan.diagnostic = "Vulkan valid SPIR-V compute shader is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.descriptor_binding_ready) {
        plan.diagnostic = "Vulkan descriptor binding is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.compute_dispatch_ready) {
        plan.diagnostic = "Vulkan compute pipeline dispatch proof is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.visible_clear_readback_ready) {
        plan.diagnostic = "Vulkan visible clear/readback proof is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.visible_draw_readback_ready) {
        plan.diagnostic = "Vulkan visible draw/readback proof is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.visible_texture_sampling_readback_ready) {
        plan.diagnostic = "Vulkan visible texture sampling/readback proof is required before IRhiDevice mapping";
        return plan;
    }
    if (!desc.visible_depth_readback_ready) {
        plan.diagnostic = "Vulkan visible depth/readback proof is required before IRhiDevice mapping";
        return plan;
    }

    plan.supported = true;
    plan.resources_mapped = true;
    plan.swapchains_mapped = true;
    plan.render_passes_mapped = true;
    plan.pipelines_mapped = true;
    plan.command_lists_mapped = true;
    plan.fences_mapped = true;
    plan.readbacks_mapped = true;
    plan.descriptor_sets_mapped = true;
    plan.compute_dispatch_mapped = true;
    plan.visible_clear_readbacks_mapped = true;
    plan.visible_draw_readbacks_mapped = true;
    plan.visible_texture_sampling_readbacks_mapped = true;
    plan.visible_depth_readbacks_mapped = true;
    plan.diagnostic = "Vulkan IRhiDevice mapping plan ready";
    return plan;
}

VulkanRhiDeviceMappingPlan minimal_irhi_device_mapping_plan() {
    VulkanRhiDeviceMappingDesc desc{};
    desc.command_pool_ready = true;
    desc.swapchain.supported = true;
    desc.dynamic_rendering.supported = true;
    desc.frame_synchronization.supported = true;
    desc.vertex_shader.valid = true;
    desc.fragment_shader.valid = true;
    desc.compute_shader.valid = true;
    desc.descriptor_binding_ready = true;
    desc.compute_dispatch_ready = true;
    desc.visible_clear_readback_ready = true;
    desc.visible_draw_readback_ready = true;
    desc.visible_texture_sampling_readback_ready = true;
    desc.visible_depth_readback_ready = true;
    return build_rhi_device_mapping_plan(desc);
}

[[nodiscard]] bool complete_rhi_device_mapping_plan(const VulkanRhiDeviceMappingPlan& plan) noexcept {
    return plan.supported && plan.resources_mapped && plan.swapchains_mapped && plan.render_passes_mapped &&
           plan.pipelines_mapped && plan.command_lists_mapped && plan.fences_mapped && plan.readbacks_mapped &&
           plan.descriptor_sets_mapped && plan.compute_dispatch_mapped && plan.visible_clear_readbacks_mapped &&
           plan.visible_draw_readbacks_mapped && plan.visible_texture_sampling_readbacks_mapped &&
           plan.visible_depth_readbacks_mapped;
}

VulkanRuntimeBufferCreatePlan build_runtime_buffer_create_plan(const VulkanRuntimeBufferDesc& desc) {
    VulkanRuntimeBufferCreatePlan plan;
    if (desc.buffer.size_bytes == 0) {
        plan.diagnostic = "Vulkan runtime buffer size is required";
        return plan;
    }
    if (desc.buffer.usage == BufferUsage::none) {
        plan.diagnostic = "Vulkan runtime buffer usage is required";
        return plan;
    }

    plan.size_bytes = desc.buffer.size_bytes;
    plan.usage.transfer_source = has_flag(desc.buffer.usage, BufferUsage::copy_source);
    plan.usage.transfer_destination = has_flag(desc.buffer.usage, BufferUsage::copy_destination);
    plan.usage.vertex = has_flag(desc.buffer.usage, BufferUsage::vertex);
    plan.usage.index = has_flag(desc.buffer.usage, BufferUsage::index);
    plan.usage.uniform = has_flag(desc.buffer.usage, BufferUsage::uniform);
    plan.usage.storage = has_flag(desc.buffer.usage, BufferUsage::storage);

    switch (desc.memory_domain) {
    case VulkanBufferMemoryDomain::device_local:
        plan.memory.device_local = true;
        break;
    case VulkanBufferMemoryDomain::upload:
        if (!plan.usage.transfer_source) {
            plan.diagnostic = "Vulkan upload buffer requires copy_source usage";
            return plan;
        }
        plan.memory.host_visible = true;
        plan.memory.host_coherent = true;
        break;
    case VulkanBufferMemoryDomain::readback:
        if (!plan.usage.transfer_destination) {
            plan.diagnostic = "Vulkan readback buffer requires copy_destination usage";
            return plan;
        }
        plan.memory.host_visible = true;
        plan.memory.host_coherent = true;
        break;
    }

    plan.supported = true;
    plan.diagnostic = "Vulkan runtime buffer create plan ready";
    return plan;
}

VulkanRuntimeTextureCreatePlan build_runtime_texture_create_plan(const VulkanRuntimeTextureDesc& desc) {
    VulkanRuntimeTextureCreatePlan plan;
    if (desc.texture.extent.width == 0 || desc.texture.extent.height == 0 || desc.texture.extent.depth == 0) {
        plan.diagnostic = "Vulkan runtime texture extent is required";
        return plan;
    }
    if (!dynamic_rendering_color_format_supported(desc.texture.format) &&
        !dynamic_rendering_depth_format_supported(desc.texture.format)) {
        plan.diagnostic = "Vulkan runtime texture format is unsupported";
        return plan;
    }
    if (desc.texture.usage == TextureUsage::none) {
        plan.diagnostic = "Vulkan runtime texture usage is required";
        return plan;
    }
    if (has_flag(desc.texture.usage, TextureUsage::present)) {
        plan.diagnostic = "Vulkan runtime texture present usage requires swapchain ownership";
        return plan;
    }
    if (has_flag(desc.texture.usage, TextureUsage::shared)) {
        plan.diagnostic = "Vulkan runtime texture shared usage is not supported yet";
        return plan;
    }
    if (desc.texture.format == Format::depth24_stencil8) {
        if (desc.texture.extent.depth != 1) {
            plan.diagnostic = "Vulkan runtime depth texture extent must be 2D";
            return plan;
        }
        constexpr auto sampled_depth_usage = TextureUsage::depth_stencil | TextureUsage::shader_resource;
        if (desc.texture.usage != TextureUsage::depth_stencil && desc.texture.usage != sampled_depth_usage) {
            plan.diagnostic = "Vulkan runtime depth texture usage is not implemented beyond depth_stencil";
            return plan;
        }
    } else if (has_flag(desc.texture.usage, TextureUsage::depth_stencil)) {
        plan.diagnostic = "Vulkan runtime depth texture format is unsupported";
        return plan;
    }

    plan.extent = desc.texture.extent;
    plan.format = desc.texture.format;
    plan.usage.transfer_source = has_flag(desc.texture.usage, TextureUsage::copy_source);
    plan.usage.transfer_destination = has_flag(desc.texture.usage, TextureUsage::copy_destination);
    plan.usage.sampled = has_flag(desc.texture.usage, TextureUsage::shader_resource);
    plan.usage.storage = has_flag(desc.texture.usage, TextureUsage::storage);
    plan.usage.color_attachment = has_flag(desc.texture.usage, TextureUsage::render_target);
    plan.usage.depth_stencil_attachment = has_flag(desc.texture.usage, TextureUsage::depth_stencil);
    plan.memory.device_local = true;
    plan.supported = true;
    plan.diagnostic = "Vulkan runtime texture create plan ready";
    return plan;
}

VulkanPhysicalDeviceCandidate make_physical_device_candidate(const VulkanRuntimePhysicalDeviceSnapshot& snapshot) {
    return VulkanPhysicalDeviceCandidate{
        .name = snapshot.name,
        .type = snapshot.type,
        .api_version = snapshot.api_version,
        .supports_swapchain_extension = snapshot.supports_swapchain_extension,
        .supports_dynamic_rendering = snapshot.supports_dynamic_rendering,
        .supports_synchronization2 = snapshot.supports_synchronization2,
        .queue_families = snapshot.queue_families,
    };
}

std::vector<VulkanCommandRequest> vulkan_backend_command_requests() {
    return {
        {.name = "vkGetInstanceProcAddr", .scope = VulkanCommandScope::loader, .required = true},
        {.name = "vkEnumerateInstanceVersion", .scope = VulkanCommandScope::global, .required = true},
        {.name = "vkEnumerateInstanceExtensionProperties", .scope = VulkanCommandScope::global, .required = true},
        {.name = "vkCreateInstance", .scope = VulkanCommandScope::global, .required = true},
        {.name = "vkDestroyInstance", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkEnumeratePhysicalDevices", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetPhysicalDeviceProperties2", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetPhysicalDeviceMemoryProperties", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetPhysicalDeviceQueueFamilyProperties", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkEnumerateDeviceExtensionProperties", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetPhysicalDeviceFeatures2", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetPhysicalDeviceSurfaceSupportKHR", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkCreateDevice", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkGetDeviceProcAddr", .scope = VulkanCommandScope::instance, .required = true},
        {.name = "vkDestroyDevice", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkGetDeviceQueue", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDeviceWaitIdle", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateCommandPool", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyCommandPool", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkAllocateCommandBuffers", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkFreeCommandBuffers", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkBeginCommandBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkEndCommandBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdPipelineBarrier2", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkQueueSubmit", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkQueueSubmit2", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkQueueWaitIdle", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkGetBufferMemoryRequirements", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateImage", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyImage", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkGetImageMemoryRequirements", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkAllocateMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkFreeMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkBindBufferMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkBindImageMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkMapMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkUnmapMemory", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdCopyBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdCopyBufferToImage", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdCopyImageToBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdBeginRendering", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdEndRendering", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdBindPipeline", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdSetViewport", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdSetScissor", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdDraw", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdBindVertexBuffers", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdBindIndexBuffer", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdDrawIndexed", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateShaderModule", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyShaderModule", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateDescriptorSetLayout", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyDescriptorSetLayout", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateDescriptorPool", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyDescriptorPool", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkAllocateDescriptorSets", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkUpdateDescriptorSets", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdBindDescriptorSets", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreatePipelineLayout", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyPipelineLayout", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateGraphicsPipelines", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateComputePipelines", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyPipeline", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCmdDispatch", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateImageView", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyImageView", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateSampler", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroySampler", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateSemaphore", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroySemaphore", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateFence", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroyFence", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkWaitForFences", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkCreateSwapchainKHR", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkDestroySwapchainKHR", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkGetSwapchainImagesKHR", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkAcquireNextImageKHR", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkQueuePresentKHR", .scope = VulkanCommandScope::device, .required = true},
        {.name = "vkSetDebugUtilsObjectNameEXT", .scope = VulkanCommandScope::device, .required = false},
        {.name = "vkCreateDebugUtilsMessengerEXT", .scope = VulkanCommandScope::instance, .required = false},
        {.name = "vkDestroyDebugUtilsMessengerEXT", .scope = VulkanCommandScope::instance, .required = false},
    };
}

VulkanCommandResolutionPlan
build_command_resolution_plan(const std::vector<VulkanCommandRequest>& requests,
                              const std::vector<VulkanCommandAvailability>& available_commands) {
    VulkanCommandResolutionPlan plan{
        .supported = true,
        .resolutions = {},
        .missing_required_commands = {},
        .diagnostic = "Vulkan command resolution plan ready",
    };

    for (const auto& request : requests) {
        const auto resolved = command_is_available(available_commands, request);
        plan.resolutions.push_back(VulkanCommandResolution{.request = request, .resolved = resolved});
        if (request.required && !resolved) {
            plan.missing_required_commands.push_back(request);
        }
    }

    if (!plan.missing_required_commands.empty()) {
        plan.supported = false;
        plan.diagnostic = "missing required Vulkan command: " + plan.missing_required_commands.front().name;
    }

    return plan;
}

std::vector<VulkanCommandRequest> vulkan_device_command_requests(const VulkanLogicalDeviceCreatePlan& plan) {
    std::vector<VulkanCommandRequest> requests;
    for (const auto& request : vulkan_backend_command_requests()) {
        if (request.scope != VulkanCommandScope::device) {
            continue;
        }
        const auto is_swapchain_command = request.name.find("Swapchain") != std::string::npos ||
                                          request.name == "vkAcquireNextImageKHR" ||
                                          request.name == "vkQueuePresentKHR";
        if (is_swapchain_command && !extension_is_enabled(plan.enabled_extensions, "VK_KHR_swapchain")) {
            continue;
        }
        const auto is_dynamic_rendering_command =
            request.name == "vkCmdBeginRendering" || request.name == "vkCmdEndRendering";
        if (is_dynamic_rendering_command && !plan.dynamic_rendering_enabled) {
            continue;
        }
        const auto is_synchronization2_command =
            request.name == "vkCmdPipelineBarrier2" || request.name == "vkQueueSubmit2";
        if (is_synchronization2_command && !plan.synchronization2_enabled) {
            continue;
        }
        requests.push_back(request);
    }
    return requests;
}

std::string_view default_runtime_library_name(RhiHostPlatform host) noexcept {
    switch (host) {
    case RhiHostPlatform::windows:
        return "vulkan-1.dll";
    case RhiHostPlatform::linux:
        return "libvulkan.so.1";
    case RhiHostPlatform::android:
        return "libvulkan.so";
    case RhiHostPlatform::macos:
    case RhiHostPlatform::ios:
    case RhiHostPlatform::unknown:
        return "";
    }
    return "";
}

VulkanLoaderProbeResult make_loader_probe_result(const VulkanLoaderProbeDesc& desc, bool runtime_loaded,
                                                 bool get_instance_proc_addr_found) {
    const auto host = normalize_host(desc.host);
    auto runtime_library = resolve_runtime_library_name(desc, host);

    if (!supports_host(host)) {
        return VulkanLoaderProbeResult{
            .probe = make_probe_result(host, BackendProbeStatus::unsupported_host,
                                       "Vulkan backend is not supported on this host"),
            .runtime_loaded = false,
            .get_instance_proc_addr_found = false,
            .runtime_library = std::move(runtime_library),
        };
    }

    if (!runtime_loaded) {
        return VulkanLoaderProbeResult{
            .probe = make_probe_result(host, BackendProbeStatus::missing_runtime,
                                       "Vulkan runtime library could not be loaded"),
            .runtime_loaded = false,
            .get_instance_proc_addr_found = false,
            .runtime_library = std::move(runtime_library),
        };
    }

    if (!get_instance_proc_addr_found) {
        return VulkanLoaderProbeResult{
            .probe = make_probe_result(host, BackendProbeStatus::missing_runtime,
                                       "Vulkan loader missing vkGetInstanceProcAddr"),
            .runtime_loaded = true,
            .get_instance_proc_addr_found = false,
            .runtime_library = std::move(runtime_library),
        };
    }

    return VulkanLoaderProbeResult{
        .probe = make_probe_result(host, BackendProbeStatus::available),
        .runtime_loaded = true,
        .get_instance_proc_addr_found = true,
        .runtime_library = std::move(runtime_library),
    };
}

VulkanLoaderProbeResult probe_runtime_loader(const VulkanLoaderProbeDesc& desc) {
    const auto host = normalize_host(desc.host);
    const auto runtime_library = resolve_runtime_library_name(desc, host);
    const auto runtime_loaded =
        supports_host(host) && runtime_library_has_symbol(runtime_library, desc.get_instance_proc_addr_symbol);
    return make_loader_probe_result(
        VulkanLoaderProbeDesc{
            .host = host,
            .runtime_library = runtime_library,
            .get_instance_proc_addr_symbol = desc.get_instance_proc_addr_symbol,
        },
        runtime_loaded, runtime_loaded);
}

VulkanRuntimeGlobalCommandProbeResult probe_runtime_global_commands(const VulkanLoaderProbeDesc& desc) {
    const auto host = normalize_host(desc.host);
    const auto runtime_library = resolve_runtime_library_name(desc, host);
    const auto requests = global_command_requests();
    std::vector<VulkanCommandAvailability> availability;

    if (!supports_host(host)) {
        return VulkanRuntimeGlobalCommandProbeResult{
            .loader = make_loader_probe_result(
                VulkanLoaderProbeDesc{
                    .host = host,
                    .runtime_library = runtime_library,
                    .get_instance_proc_addr_symbol = desc.get_instance_proc_addr_symbol,
                },
                false, false),
            .command_plan = build_command_resolution_plan(requests, availability),
        };
    }

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        return VulkanRuntimeGlobalCommandProbeResult{
            .loader = make_loader_probe_result(
                VulkanLoaderProbeDesc{
                    .host = host,
                    .runtime_library = runtime_library,
                    .get_instance_proc_addr_symbol = desc.get_instance_proc_addr_symbol,
                },
                false, false),
            .command_plan = build_command_resolution_plan(requests, availability),
        };
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{desc.get_instance_proc_addr_symbol}.c_str()));
    append_global_command_availability(availability, get_instance_proc_addr, requests);
    FreeLibrary(library);

    return VulkanRuntimeGlobalCommandProbeResult{
        .loader = make_loader_probe_result(
            VulkanLoaderProbeDesc{
                .host = host,
                .runtime_library = runtime_library,
                .get_instance_proc_addr_symbol = desc.get_instance_proc_addr_symbol,
            },
            true, get_instance_proc_addr != nullptr),
        .command_plan = build_command_resolution_plan(requests, availability),
    };
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        return VulkanRuntimeGlobalCommandProbeResult{
            make_loader_probe_result(
                VulkanLoaderProbeDesc{
                    host,
                    runtime_library,
                    desc.get_instance_proc_addr_symbol,
                },
                false, false),
            build_command_resolution_plan(requests, availability),
        };
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{desc.get_instance_proc_addr_symbol}.c_str()));
    append_global_command_availability(availability, get_instance_proc_addr, requests);
    dlclose(library);

    return VulkanRuntimeGlobalCommandProbeResult{
        make_loader_probe_result(
            VulkanLoaderProbeDesc{
                host,
                runtime_library,
                desc.get_instance_proc_addr_symbol,
            },
            true, get_instance_proc_addr != nullptr),
        build_command_resolution_plan(requests, availability),
    };
#else
    return VulkanRuntimeGlobalCommandProbeResult{
        make_loader_probe_result(
            VulkanLoaderProbeDesc{
                host,
                runtime_library,
                desc.get_instance_proc_addr_symbol,
            },
            false, false),
        build_command_resolution_plan(requests, availability),
    };
#endif
}

VulkanRuntimeInstanceCapabilityProbeResult
probe_runtime_instance_capabilities(const VulkanLoaderProbeDesc& loader_desc,
                                    const VulkanInstanceCreateDesc& instance_desc) {
    auto global = probe_runtime_global_commands(loader_desc);
    VulkanRuntimeInstanceCapabilityProbeResult result{
        .global = std::move(global),
        .api_version = {},
        .instance_extensions = {},
        .instance_plan = {},
    };

    if (!result.global.command_plan.supported) {
        result.instance_plan = make_runtime_instance_failure(instance_desc, result.global.command_plan.diagnostic);
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime library could not be loaded");
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.enumerate_instance_version == nullptr || commands.enumerate_instance_extension_properties == nullptr) {
        FreeLibrary(library);
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime global commands are incomplete");
        return result;
    }

    std::uint32_t encoded_version = 0;
    const auto version_result = commands.enumerate_instance_version(&encoded_version);
    if (version_result != vulkan_success) {
        FreeLibrary(library);
        result.instance_plan = make_runtime_instance_failure(instance_desc, "Vulkan instance API version query failed");
        return result;
    }

    result.api_version = decode_vulkan_api_version(encoded_version);
    if (!is_vulkan_api_at_least(result.api_version, instance_desc.api_version)) {
        FreeLibrary(library);
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime instance API version is lower than requested");
        return result;
    }
    result.instance_extensions = enumerate_instance_extensions(commands.enumerate_instance_extension_properties);
    result.instance_plan = build_instance_create_plan(instance_desc, result.instance_extensions);
    FreeLibrary(library);
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime library could not be loaded");
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.enumerate_instance_version == nullptr || commands.enumerate_instance_extension_properties == nullptr) {
        dlclose(library);
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime global commands are incomplete");
        return result;
    }

    std::uint32_t encoded_version = 0;
    const auto version_result = commands.enumerate_instance_version(&encoded_version);
    if (version_result != vulkan_success) {
        dlclose(library);
        result.instance_plan = make_runtime_instance_failure(instance_desc, "Vulkan instance API version query failed");
        return result;
    }

    result.api_version = decode_vulkan_api_version(encoded_version);
    if (!is_vulkan_api_at_least(result.api_version, instance_desc.api_version)) {
        dlclose(library);
        result.instance_plan =
            make_runtime_instance_failure(instance_desc, "Vulkan runtime instance API version is lower than requested");
        return result;
    }
    result.instance_extensions = enumerate_instance_extensions(commands.enumerate_instance_extension_properties);
    result.instance_plan = build_instance_create_plan(instance_desc, result.instance_extensions);
    dlclose(library);
    return result;
#else
    result.instance_plan =
        make_runtime_instance_failure(instance_desc, "Vulkan runtime probing is unsupported on this host");
    return result;
#endif
}

VulkanRuntimeInstanceCommandProbeResult probe_runtime_instance_commands(const VulkanLoaderProbeDesc& loader_desc,
                                                                        const VulkanInstanceCreateDesc& instance_desc) {
    auto capabilities = probe_runtime_instance_capabilities(loader_desc, instance_desc);
    const auto requests = instance_command_requests_for_plan(capabilities.instance_plan);
    VulkanRuntimeInstanceCommandProbeResult result{
        .capabilities = std::move(capabilities),
        .command_plan = build_command_resolution_plan(requests, {}),
        .instance_created = false,
        .instance_destroyed = false,
        .diagnostic = {},
    };

    if (!result.capabilities.instance_plan.supported) {
        result.diagnostic = result.capabilities.instance_plan.diagnostic;
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(result.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    result.instance_created = true;
    std::vector<VulkanCommandAvailability> availability;
    append_instance_command_availability(availability, get_instance_proc_addr, instance, requests);
    result.command_plan = build_command_resolution_plan(requests, availability);

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
        result.instance_destroyed = true;
    }
    FreeLibrary(library);

    result.diagnostic =
        result.instance_destroyed ? result.command_plan.diagnostic : "Vulkan vkDestroyInstance command is unavailable";
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(result.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    result.instance_created = true;
    std::vector<VulkanCommandAvailability> availability;
    append_instance_command_availability(availability, get_instance_proc_addr, instance, requests);
    result.command_plan = build_command_resolution_plan(requests, availability);

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
        result.instance_destroyed = true;
    }
    dlclose(library);

    result.diagnostic =
        result.instance_destroyed ? result.command_plan.diagnostic : "Vulkan vkDestroyInstance command is unavailable";
    return result;
#else
    result.diagnostic = "Vulkan runtime instance command probing is unsupported on this host";
    return result;
#endif
}

VulkanRuntimeInstanceCreateResult create_runtime_instance(const VulkanLoaderProbeDesc& loader_desc,
                                                          const VulkanInstanceCreateDesc& instance_desc) {
    VulkanRuntimeInstanceCreateResult result;
    result.probe = probe_runtime_instance_commands(loader_desc, instance_desc);
    if (!result.probe.instance_created || !result.probe.instance_destroyed || !result.probe.command_plan.supported) {
        result.diagnostic = result.probe.diagnostic;
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(result.probe.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (destroy_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkDestroyInstance command is unavailable";
        return result;
    }

    auto impl = std::make_unique<VulkanRuntimeInstance::Impl>();
    impl->library = library;
    impl->instance = instance;
    impl->destroy_instance = destroy_instance;
    impl->command_plan = result.probe.command_plan;
    result.instance = VulkanRuntimeInstance{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime instance owner ready";
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(result.probe.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (destroy_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkDestroyInstance command is unavailable";
        return result;
    }

    auto impl = std::make_unique<VulkanRuntimeInstance::Impl>();
    impl->library = library;
    impl->instance = instance;
    impl->destroy_instance = destroy_instance;
    impl->command_plan = result.probe.command_plan;
    result.instance = VulkanRuntimeInstance{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime instance owner ready";
    return result;
#else
    result.diagnostic = "Vulkan runtime instance ownership is unsupported on this host";
    return result;
#endif
}

VulkanRuntimePhysicalDeviceCountProbeResult
probe_runtime_physical_device_count(const VulkanLoaderProbeDesc& loader_desc,
                                    const VulkanInstanceCreateDesc& instance_desc) {
    auto instance_probe = probe_runtime_instance_commands(loader_desc, instance_desc);
    VulkanRuntimePhysicalDeviceCountProbeResult result{
        .instance = std::move(instance_probe),
        .enumerated = false,
        .physical_device_count = 0,
        .diagnostic = {},
    };

    if (!result.instance.instance_created || !result.instance.instance_destroyed ||
        !result.instance.command_plan.supported) {
        result.diagnostic = result.instance.diagnostic;
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers =
        extension_name_pointers(result.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (enumerate_physical_devices == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkEnumeratePhysicalDevices command is unavailable";
        return result;
    }

    std::uint32_t count = 0;
    const auto enumerate_result = enumerate_physical_devices(instance, &count, nullptr);
    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
    }
    FreeLibrary(library);

    if (!is_successful_enumeration_result(enumerate_result)) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan physical device count query failed", enumerate_result);
        return result;
    }

    result.enumerated = true;
    result.physical_device_count = count;
    result.diagnostic = "Vulkan physical device count probe ready";
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers =
        extension_name_pointers(result.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (enumerate_physical_devices == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        dlclose(library);
        result.diagnostic = "Vulkan vkEnumeratePhysicalDevices command is unavailable";
        return result;
    }

    std::uint32_t count = 0;
    const auto enumerate_result = enumerate_physical_devices(instance, &count, nullptr);
    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
    }
    dlclose(library);

    if (!is_successful_enumeration_result(enumerate_result)) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan physical device count query failed", enumerate_result);
        return result;
    }

    result.enumerated = true;
    result.physical_device_count = count;
    result.diagnostic = "Vulkan physical device count probe ready";
    return result;
#else
    result.diagnostic = "Vulkan physical device count probing is unsupported on this host";
    return result;
#endif
}

VulkanRuntimePhysicalDeviceSnapshotProbeResult
probe_runtime_physical_device_snapshots(const VulkanLoaderProbeDesc& loader_desc,
                                        const VulkanInstanceCreateDesc& instance_desc) {
    auto count_probe = probe_runtime_physical_device_count(loader_desc, instance_desc);
    VulkanRuntimePhysicalDeviceSnapshotProbeResult result{
        .count_probe = std::move(count_probe),
        .devices = {},
        .enumerated = false,
        .diagnostic = {},
    };

    if (!result.count_probe.enumerated) {
        result.diagnostic = result.count_probe.diagnostic;
        return result;
    }

    if (result.count_probe.physical_device_count == 0) {
        result.enumerated = true;
        result.diagnostic = "Vulkan physical device snapshot probe ready";
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers =
        extension_name_pointers(result.count_probe.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto get_queue_family_properties = reinterpret_cast<VulkanGetPhysicalDeviceQueueFamilyProperties>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    const auto enumerate_device_extension_properties = reinterpret_cast<VulkanEnumerateDeviceExtensionProperties>(
        get_instance_proc_addr(instance, "vkEnumerateDeviceExtensionProperties"));
    const auto get_physical_device_features2 = reinterpret_cast<VulkanGetPhysicalDeviceFeatures2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceFeatures2"));
    const auto get_physical_device_properties2 = reinterpret_cast<VulkanGetPhysicalDeviceProperties2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties2"));
    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (enumerate_physical_devices == nullptr || get_queue_family_properties == nullptr ||
        enumerate_device_extension_properties == nullptr || get_physical_device_features2 == nullptr ||
        get_physical_device_properties2 == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic = "Vulkan physical device snapshot commands are unavailable";
        return result;
    }

    std::uint32_t physical_device_count = result.count_probe.physical_device_count;
    std::vector<NativeVulkanPhysicalDevice> physical_devices(physical_device_count);
    const auto enumerate_result = enumerate_physical_devices(instance, &physical_device_count, physical_devices.data());
    if (!is_successful_enumeration_result(enumerate_result)) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan physical device handle enumeration failed", enumerate_result);
        return result;
    }
    physical_devices.resize(physical_device_count);
    result.count_probe.physical_device_count = physical_device_count;

    result.devices.reserve(physical_devices.size());
    for (std::size_t device_index = 0; device_index < physical_devices.size(); ++device_index) {
        result.devices.push_back(make_runtime_physical_device_snapshot(
            device_index, physical_devices[device_index], get_physical_device_properties2, get_queue_family_properties,
            enumerate_device_extension_properties, get_physical_device_features2));
    }

    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
    }
    FreeLibrary(library);

    result.enumerated = true;
    result.diagnostic = "Vulkan physical device snapshot probe ready";
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers =
        extension_name_pointers(result.count_probe.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto get_queue_family_properties = reinterpret_cast<VulkanGetPhysicalDeviceQueueFamilyProperties>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    const auto enumerate_device_extension_properties = reinterpret_cast<VulkanEnumerateDeviceExtensionProperties>(
        get_instance_proc_addr(instance, "vkEnumerateDeviceExtensionProperties"));
    const auto get_physical_device_features2 = reinterpret_cast<VulkanGetPhysicalDeviceFeatures2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceFeatures2"));
    const auto get_physical_device_properties2 = reinterpret_cast<VulkanGetPhysicalDeviceProperties2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties2"));
    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (enumerate_physical_devices == nullptr || get_queue_family_properties == nullptr ||
        enumerate_device_extension_properties == nullptr || get_physical_device_features2 == nullptr ||
        get_physical_device_properties2 == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        dlclose(library);
        result.diagnostic = "Vulkan physical device snapshot commands are unavailable";
        return result;
    }

    std::uint32_t physical_device_count = result.count_probe.physical_device_count;
    std::vector<NativeVulkanPhysicalDevice> physical_devices(physical_device_count);
    const auto enumerate_result = enumerate_physical_devices(instance, &physical_device_count, physical_devices.data());
    if (!is_successful_enumeration_result(enumerate_result)) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        dlclose(library);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan physical device handle enumeration failed", enumerate_result);
        return result;
    }
    physical_devices.resize(physical_device_count);
    result.count_probe.physical_device_count = physical_device_count;

    result.devices.reserve(physical_devices.size());
    for (std::size_t device_index = 0; device_index < physical_devices.size(); ++device_index) {
        result.devices.push_back(make_runtime_physical_device_snapshot(
            device_index, physical_devices[device_index], get_physical_device_properties2, get_queue_family_properties,
            enumerate_device_extension_properties, get_physical_device_features2));
    }

    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
    }
    dlclose(library);

    result.enumerated = true;
    result.diagnostic = "Vulkan physical device snapshot probe ready";
    return result;
#else
    result.diagnostic = "Vulkan physical device snapshot probing is unsupported on this host";
    return result;
#endif
}

VulkanRuntimePhysicalDeviceSelectionProbeResult
probe_runtime_physical_device_selection(const VulkanLoaderProbeDesc& loader_desc,
                                        const VulkanInstanceCreateDesc& instance_desc) {
    VulkanRuntimePhysicalDeviceSelectionProbeResult result;
    result.snapshots = probe_runtime_physical_device_snapshots(loader_desc, instance_desc);
    if (!result.snapshots.enumerated) {
        result.diagnostic = result.snapshots.diagnostic;
        return result;
    }

    result.candidates.reserve(result.snapshots.devices.size());
    for (const auto& snapshot : result.snapshots.devices) {
        result.candidates.push_back(make_physical_device_candidate(snapshot));
    }

    result.selection = select_physical_device(result.candidates);
    result.selected = result.selection.suitable;
    result.diagnostic = result.selection.diagnostic;
    return result;
}

std::vector<std::string> vulkan_surface_instance_extensions(RhiHostPlatform host) {
    switch (normalize_host(host)) {
    case RhiHostPlatform::windows:
        return {"VK_KHR_surface", "VK_KHR_win32_surface"};
    case RhiHostPlatform::android:
        return {"VK_KHR_surface", "VK_KHR_android_surface"};
    case RhiHostPlatform::linux:
    case RhiHostPlatform::macos:
    case RhiHostPlatform::ios:
    case RhiHostPlatform::unknown:
        return {};
    }
    return {};
}

VulkanRuntimeSurfaceSupportProbeResult probe_runtime_surface_support(const VulkanLoaderProbeDesc& loader_desc,
                                                                     const VulkanInstanceCreateDesc& instance_desc,
                                                                     SurfaceHandle surface) {
    VulkanRuntimeSurfaceSupportProbeResult result;
    if (surface.value == 0) {
        result.diagnostic = "Vulkan surface support probing requires a surface handle";
        return result;
    }

    const auto host = normalize_host(loader_desc.host);
    const auto surface_extensions = vulkan_surface_instance_extensions(host);
    if (surface_extensions.empty()) {
        result.diagnostic = "Vulkan surface support probing is unsupported on this host";
        return result;
    }

#if defined(_WIN32)
    auto* const window = reinterpret_cast<HWND>(surface.value);
    if (window == nullptr || IsWindow(window) == 0) {
        result.diagnostic = "Vulkan Win32 HWND is invalid";
        return result;
    }

    const auto surface_instance_desc = make_surface_instance_desc(instance_desc, host);
    result.snapshots = probe_runtime_physical_device_snapshots(loader_desc, surface_instance_desc);
    if (!result.snapshots.enumerated) {
        result.diagnostic = result.snapshots.diagnostic;
        return result;
    }

    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers =
        extension_name_pointers(result.snapshots.count_probe.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(surface_instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto create_win32_surface =
        reinterpret_cast<VulkanCreateWin32Surface>(get_instance_proc_addr(instance, "vkCreateWin32SurfaceKHR"));
    const auto destroy_surface =
        reinterpret_cast<VulkanDestroySurface>(get_instance_proc_addr(instance, "vkDestroySurfaceKHR"));
    const auto get_surface_support = reinterpret_cast<VulkanGetPhysicalDeviceSurfaceSupport>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto get_queue_family_properties = reinterpret_cast<VulkanGetPhysicalDeviceQueueFamilyProperties>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    const auto enumerate_device_extension_properties = reinterpret_cast<VulkanEnumerateDeviceExtensionProperties>(
        get_instance_proc_addr(instance, "vkEnumerateDeviceExtensionProperties"));
    const auto get_physical_device_features2 = reinterpret_cast<VulkanGetPhysicalDeviceFeatures2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceFeatures2"));
    const auto get_physical_device_properties2 = reinterpret_cast<VulkanGetPhysicalDeviceProperties2>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties2"));
    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    if (create_win32_surface == nullptr || destroy_surface == nullptr || get_surface_support == nullptr ||
        enumerate_physical_devices == nullptr || get_queue_family_properties == nullptr ||
        enumerate_device_extension_properties == nullptr || get_physical_device_features2 == nullptr ||
        get_physical_device_properties2 == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic = "Vulkan surface support commands are unavailable";
        return result;
    }

    NativeVulkanSurface native_surface = 0;
    const auto surface_create_info = NativeVulkanWin32SurfaceCreateInfo{
        .s_type = vulkan_structure_type_win32_surface_create_info,
        .next = nullptr,
        .flags = 0,
        .instance = GetModuleHandleW(nullptr),
        .window = window,
    };
    const auto surface_create_result = create_win32_surface(instance, &surface_create_info, nullptr, &native_surface);
    if (surface_create_result != vulkan_success || native_surface == 0) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateWin32SurfaceKHR failed", surface_create_result);
        return result;
    }
    result.surface_created = true;

    std::uint32_t physical_device_count = result.snapshots.count_probe.physical_device_count;
    std::vector<NativeVulkanPhysicalDevice> physical_devices(physical_device_count);
    const auto enumerate_result = enumerate_physical_devices(instance, &physical_device_count, physical_devices.data());
    if (!is_successful_enumeration_result(enumerate_result)) {
        destroy_surface(instance, native_surface, nullptr);
        result.surface_destroyed = true;
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan physical device handle enumeration failed", enumerate_result);
        return result;
    }
    physical_devices.resize(physical_device_count);
    refresh_surface_probe_queue_family_snapshots(result.snapshots, physical_devices, get_physical_device_properties2,
                                                 get_queue_family_properties, enumerate_device_extension_properties,
                                                 get_physical_device_features2);

    const auto device_count = std::min(result.snapshots.devices.size(), physical_devices.size());
    for (std::size_t device_index = 0; device_index < device_count; ++device_index) {
        // Surface support must be queried with queue family indices from the same native instance handles.
        for (auto& queue_family : result.snapshots.devices[device_index].queue_families) {
            std::uint32_t supported = 0;
            const auto support_result =
                get_surface_support(physical_devices[device_index], queue_family.index, native_surface, &supported);
            queue_family.supports_present = support_result == vulkan_success && supported != 0U;
        }
    }

    destroy_surface(instance, native_surface, nullptr);
    result.surface_destroyed = true;
    if (destroy_instance != nullptr) {
        destroy_instance(instance, nullptr);
    }
    FreeLibrary(library);

    result.probed = true;
    result.diagnostic = "Vulkan surface support probe ready";
    return result;
#else
    result.diagnostic = "Vulkan surface support probing is unsupported on this host";
    return result;
#endif
}

VulkanRuntimeDeviceCreateResult create_runtime_device(const VulkanLoaderProbeDesc& loader_desc,
                                                      const VulkanInstanceCreateDesc& instance_desc,
                                                      const VulkanLogicalDeviceCreateDesc& device_desc,
                                                      SurfaceHandle surface) {
    VulkanRuntimeDeviceCreateResult result;
    const auto host = normalize_host(loader_desc.host);
    const auto runtime_instance_desc =
        surface.value != 0 ? make_surface_instance_desc(instance_desc, host) : instance_desc;

    if (surface.value != 0) {
        auto surface_probe = probe_runtime_surface_support(loader_desc, instance_desc, surface);
        result.selection_probe.snapshots = std::move(surface_probe.snapshots);
        if (!surface_probe.probed) {
            result.diagnostic = surface_probe.diagnostic;
            return result;
        }

        result.selection_probe.candidates.reserve(result.selection_probe.snapshots.devices.size());
        for (const auto& snapshot : result.selection_probe.snapshots.devices) {
            result.selection_probe.candidates.push_back(make_physical_device_candidate(snapshot));
        }
        result.selection_probe.selection = select_physical_device(result.selection_probe.candidates);
        result.selection_probe.selected = result.selection_probe.selection.suitable;
        result.selection_probe.diagnostic = result.selection_probe.selection.diagnostic;
    } else {
        result.selection_probe = probe_runtime_physical_device_selection(loader_desc, runtime_instance_desc);
    }

    if (!result.selection_probe.selected) {
        result.diagnostic = result.selection_probe.diagnostic;
        return result;
    }

    const auto selected_index = result.selection_probe.selection.device_index;
    if (selected_index >= result.selection_probe.snapshots.devices.size() ||
        selected_index >= result.selection_probe.candidates.size()) {
        result.diagnostic = "Vulkan selected physical device index is out of range";
        return result;
    }

    const auto& selected_snapshot = result.selection_probe.snapshots.devices[selected_index];
    const auto& selected_candidate = result.selection_probe.candidates[selected_index];
    result.logical_device_plan = build_logical_device_create_plan(
        device_desc, selected_candidate, result.selection_probe.selection, selected_snapshot.device_extensions);
    if (!result.logical_device_plan.supported) {
        result.diagnostic = result.logical_device_plan.diagnostic;
        return result;
    }

    const auto runtime_library = resolve_runtime_library_name(loader_desc, host);

#if defined(_WIN32)
    auto* const library = LoadLibraryA(runtime_library.c_str());
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        GetProcAddress(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(
        result.selection_probe.snapshots.count_probe.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(runtime_instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    const auto create_win32_surface =
        reinterpret_cast<VulkanCreateWin32Surface>(get_instance_proc_addr(instance, "vkCreateWin32SurfaceKHR"));
    const auto destroy_surface =
        reinterpret_cast<VulkanDestroySurface>(get_instance_proc_addr(instance, "vkDestroySurfaceKHR"));
    const auto get_surface_capabilities = reinterpret_cast<VulkanGetPhysicalDeviceSurfaceCapabilities>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    const auto get_surface_formats = reinterpret_cast<VulkanGetPhysicalDeviceSurfaceFormats>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    const auto get_surface_present_modes = reinterpret_cast<VulkanGetPhysicalDeviceSurfacePresentModes>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    const auto get_physical_device_memory_properties = reinterpret_cast<VulkanGetPhysicalDeviceMemoryProperties>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceMemoryProperties"));
    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto create_device = reinterpret_cast<VulkanCreateDevice>(get_instance_proc_addr(instance, "vkCreateDevice"));
    const auto get_device_proc_addr =
        reinterpret_cast<VulkanGetDeviceProcAddr>(get_instance_proc_addr(instance, "vkGetDeviceProcAddr"));
    if (destroy_instance == nullptr || get_physical_device_memory_properties == nullptr ||
        enumerate_physical_devices == nullptr || create_device == nullptr || get_device_proc_addr == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        FreeLibrary(library);
        result.diagnostic = "Vulkan logical device instance commands are unavailable";
        return result;
    }
    if (surface.value != 0 &&
        (create_win32_surface == nullptr || destroy_surface == nullptr || get_surface_capabilities == nullptr ||
         get_surface_formats == nullptr || get_surface_present_modes == nullptr)) {
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic = "Vulkan surface query instance commands are unavailable";
        return result;
    }

    std::uint32_t physical_device_count = result.selection_probe.snapshots.count_probe.physical_device_count;
    std::vector<NativeVulkanPhysicalDevice> physical_devices(physical_device_count);
    const auto enumerate_result = enumerate_physical_devices(instance, &physical_device_count, physical_devices.data());
    if (!is_successful_enumeration_result(enumerate_result)) {
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan physical device handle enumeration failed", enumerate_result);
        return result;
    }
    physical_devices.resize(physical_device_count);
    if (selected_index >= physical_devices.size()) {
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic = "Vulkan selected physical device handle is unavailable";
        return result;
    }

    std::vector<float> queue_priorities;
    queue_priorities.reserve(result.logical_device_plan.queue_families.size());
    for (const auto& queue_plan : result.logical_device_plan.queue_families) {
        queue_priorities.push_back(queue_plan.priority);
    }
    const auto queue_infos =
        make_native_device_queue_create_infos(result.logical_device_plan.queue_families, queue_priorities);
    const auto device_extension_pointers = extension_name_pointers(result.logical_device_plan.enabled_extensions);
    auto vulkan13_features = make_native_vulkan13_features(result.logical_device_plan);
    const auto enable_vulkan13_features =
        result.logical_device_plan.dynamic_rendering_enabled || result.logical_device_plan.synchronization2_enabled;
    const auto device_create_info = make_native_device_create_info(
        queue_infos, device_extension_pointers, enable_vulkan13_features ? &vulkan13_features : nullptr);

    NativeVulkanDevice device = nullptr;
    const auto device_create_result =
        create_device(physical_devices[selected_index], &device_create_info, nullptr, &device);
    if (device_create_result != vulkan_success || device == nullptr) {
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateDevice failed", device_create_result);
        return result;
    }

    auto destroy_device = reinterpret_cast<VulkanDestroyDevice>(get_device_proc_addr(device, "vkDestroyDevice"));
    if (destroy_device == nullptr) {
        destroy_device = reinterpret_cast<VulkanDestroyDevice>(get_instance_proc_addr(instance, "vkDestroyDevice"));
    }
    const auto device_wait_idle =
        reinterpret_cast<VulkanDeviceWaitIdle>(get_device_proc_addr(device, "vkDeviceWaitIdle"));
    const auto get_device_queue =
        reinterpret_cast<VulkanGetDeviceQueue>(get_device_proc_addr(device, "vkGetDeviceQueue"));
    const auto create_command_pool =
        reinterpret_cast<VulkanCreateCommandPool>(get_device_proc_addr(device, "vkCreateCommandPool"));
    const auto destroy_command_pool =
        reinterpret_cast<VulkanDestroyCommandPool>(get_device_proc_addr(device, "vkDestroyCommandPool"));
    const auto allocate_command_buffers =
        reinterpret_cast<VulkanAllocateCommandBuffers>(get_device_proc_addr(device, "vkAllocateCommandBuffers"));
    const auto free_command_buffers =
        reinterpret_cast<VulkanFreeCommandBuffers>(get_device_proc_addr(device, "vkFreeCommandBuffers"));
    const auto begin_command_buffer =
        reinterpret_cast<VulkanBeginCommandBuffer>(get_device_proc_addr(device, "vkBeginCommandBuffer"));
    const auto end_command_buffer =
        reinterpret_cast<VulkanEndCommandBuffer>(get_device_proc_addr(device, "vkEndCommandBuffer"));
    const auto cmd_begin_rendering =
        reinterpret_cast<VulkanCmdBeginRendering>(get_device_proc_addr(device, "vkCmdBeginRendering"));
    const auto cmd_end_rendering =
        reinterpret_cast<VulkanCmdEndRendering>(get_device_proc_addr(device, "vkCmdEndRendering"));
    const auto cmd_bind_pipeline =
        reinterpret_cast<VulkanCmdBindPipeline>(get_device_proc_addr(device, "vkCmdBindPipeline"));
    const auto cmd_set_viewport =
        reinterpret_cast<VulkanCmdSetViewport>(get_device_proc_addr(device, "vkCmdSetViewport"));
    const auto cmd_set_scissor = reinterpret_cast<VulkanCmdSetScissor>(get_device_proc_addr(device, "vkCmdSetScissor"));
    const auto cmd_draw = reinterpret_cast<VulkanCmdDraw>(get_device_proc_addr(device, "vkCmdDraw"));
    const auto cmd_bind_vertex_buffers =
        reinterpret_cast<VulkanCmdBindVertexBuffers>(get_device_proc_addr(device, "vkCmdBindVertexBuffers"));
    const auto cmd_bind_index_buffer =
        reinterpret_cast<VulkanCmdBindIndexBuffer>(get_device_proc_addr(device, "vkCmdBindIndexBuffer"));
    const auto cmd_draw_indexed =
        reinterpret_cast<VulkanCmdDrawIndexed>(get_device_proc_addr(device, "vkCmdDrawIndexed"));
    const auto cmd_pipeline_barrier2 =
        reinterpret_cast<VulkanCmdPipelineBarrier2>(get_device_proc_addr(device, "vkCmdPipelineBarrier2"));
    const auto queue_submit2 = reinterpret_cast<VulkanQueueSubmit2>(get_device_proc_addr(device, "vkQueueSubmit2"));
    const auto queue_wait_idle = reinterpret_cast<VulkanQueueWaitIdle>(get_device_proc_addr(device, "vkQueueWaitIdle"));
    const auto create_buffer = reinterpret_cast<VulkanCreateBuffer>(get_device_proc_addr(device, "vkCreateBuffer"));
    const auto destroy_buffer = reinterpret_cast<VulkanDestroyBuffer>(get_device_proc_addr(device, "vkDestroyBuffer"));
    const auto get_buffer_memory_requirements = reinterpret_cast<VulkanGetBufferMemoryRequirements>(
        get_device_proc_addr(device, "vkGetBufferMemoryRequirements"));
    const auto create_image = reinterpret_cast<VulkanCreateImage>(get_device_proc_addr(device, "vkCreateImage"));
    const auto destroy_image = reinterpret_cast<VulkanDestroyImage>(get_device_proc_addr(device, "vkDestroyImage"));
    const auto get_image_memory_requirements = reinterpret_cast<VulkanGetImageMemoryRequirements>(
        get_device_proc_addr(device, "vkGetImageMemoryRequirements"));
    const auto allocate_memory =
        reinterpret_cast<VulkanAllocateMemory>(get_device_proc_addr(device, "vkAllocateMemory"));
    const auto free_memory = reinterpret_cast<VulkanFreeMemory>(get_device_proc_addr(device, "vkFreeMemory"));
    const auto bind_buffer_memory =
        reinterpret_cast<VulkanBindBufferMemory>(get_device_proc_addr(device, "vkBindBufferMemory"));
    const auto bind_image_memory =
        reinterpret_cast<VulkanBindImageMemory>(get_device_proc_addr(device, "vkBindImageMemory"));
    const auto map_memory = reinterpret_cast<VulkanMapMemory>(get_device_proc_addr(device, "vkMapMemory"));
    const auto unmap_memory = reinterpret_cast<VulkanUnmapMemory>(get_device_proc_addr(device, "vkUnmapMemory"));
    const auto cmd_copy_image_to_buffer =
        reinterpret_cast<VulkanCmdCopyImageToBuffer>(get_device_proc_addr(device, "vkCmdCopyImageToBuffer"));
    const auto cmd_copy_buffer = reinterpret_cast<VulkanCmdCopyBuffer>(get_device_proc_addr(device, "vkCmdCopyBuffer"));
    const auto cmd_copy_buffer_to_image =
        reinterpret_cast<VulkanCmdCopyBufferToImage>(get_device_proc_addr(device, "vkCmdCopyBufferToImage"));
    const auto create_shader_module =
        reinterpret_cast<VulkanCreateShaderModule>(get_device_proc_addr(device, "vkCreateShaderModule"));
    const auto destroy_shader_module =
        reinterpret_cast<VulkanDestroyShaderModule>(get_device_proc_addr(device, "vkDestroyShaderModule"));
    const auto create_descriptor_set_layout =
        reinterpret_cast<VulkanCreateDescriptorSetLayout>(get_device_proc_addr(device, "vkCreateDescriptorSetLayout"));
    const auto destroy_descriptor_set_layout = reinterpret_cast<VulkanDestroyDescriptorSetLayout>(
        get_device_proc_addr(device, "vkDestroyDescriptorSetLayout"));
    const auto create_descriptor_pool =
        reinterpret_cast<VulkanCreateDescriptorPool>(get_device_proc_addr(device, "vkCreateDescriptorPool"));
    const auto destroy_descriptor_pool =
        reinterpret_cast<VulkanDestroyDescriptorPool>(get_device_proc_addr(device, "vkDestroyDescriptorPool"));
    const auto allocate_descriptor_sets =
        reinterpret_cast<VulkanAllocateDescriptorSets>(get_device_proc_addr(device, "vkAllocateDescriptorSets"));
    const auto update_descriptor_sets =
        reinterpret_cast<VulkanUpdateDescriptorSets>(get_device_proc_addr(device, "vkUpdateDescriptorSets"));
    const auto cmd_bind_descriptor_sets =
        reinterpret_cast<VulkanCmdBindDescriptorSets>(get_device_proc_addr(device, "vkCmdBindDescriptorSets"));
    const auto create_swapchain =
        reinterpret_cast<VulkanCreateSwapchain>(get_device_proc_addr(device, "vkCreateSwapchainKHR"));
    const auto destroy_swapchain =
        reinterpret_cast<VulkanDestroySwapchain>(get_device_proc_addr(device, "vkDestroySwapchainKHR"));
    const auto get_swapchain_images =
        reinterpret_cast<VulkanGetSwapchainImages>(get_device_proc_addr(device, "vkGetSwapchainImagesKHR"));
    const auto acquire_next_image =
        reinterpret_cast<VulkanAcquireNextImage>(get_device_proc_addr(device, "vkAcquireNextImageKHR"));
    const auto create_image_view =
        reinterpret_cast<VulkanCreateImageView>(get_device_proc_addr(device, "vkCreateImageView"));
    const auto destroy_image_view =
        reinterpret_cast<VulkanDestroyImageView>(get_device_proc_addr(device, "vkDestroyImageView"));
    const auto create_sampler = reinterpret_cast<VulkanCreateSampler>(get_device_proc_addr(device, "vkCreateSampler"));
    const auto destroy_sampler =
        reinterpret_cast<VulkanDestroySampler>(get_device_proc_addr(device, "vkDestroySampler"));
    const auto create_semaphore =
        reinterpret_cast<VulkanCreateSemaphore>(get_device_proc_addr(device, "vkCreateSemaphore"));
    const auto destroy_semaphore =
        reinterpret_cast<VulkanDestroySemaphore>(get_device_proc_addr(device, "vkDestroySemaphore"));
    const auto create_fence = reinterpret_cast<VulkanCreateFence>(get_device_proc_addr(device, "vkCreateFence"));
    const auto destroy_fence = reinterpret_cast<VulkanDestroyFence>(get_device_proc_addr(device, "vkDestroyFence"));
    const auto wait_for_fences = reinterpret_cast<VulkanWaitForFences>(get_device_proc_addr(device, "vkWaitForFences"));
    const auto queue_present = reinterpret_cast<VulkanQueuePresent>(get_device_proc_addr(device, "vkQueuePresentKHR"));
    const auto create_pipeline_layout =
        reinterpret_cast<VulkanCreatePipelineLayout>(get_device_proc_addr(device, "vkCreatePipelineLayout"));
    const auto destroy_pipeline_layout =
        reinterpret_cast<VulkanDestroyPipelineLayout>(get_device_proc_addr(device, "vkDestroyPipelineLayout"));
    const auto create_graphics_pipelines =
        reinterpret_cast<VulkanCreateGraphicsPipelines>(get_device_proc_addr(device, "vkCreateGraphicsPipelines"));
    const auto create_compute_pipelines =
        reinterpret_cast<VulkanCreateComputePipelines>(get_device_proc_addr(device, "vkCreateComputePipelines"));
    const auto destroy_pipeline =
        reinterpret_cast<VulkanDestroyPipeline>(get_device_proc_addr(device, "vkDestroyPipeline"));
    const auto cmd_dispatch = reinterpret_cast<VulkanCmdDispatch>(get_device_proc_addr(device, "vkCmdDispatch"));
    const auto set_debug_utils_object_name =
        reinterpret_cast<VulkanSetDebugUtilsObjectName>(get_device_proc_addr(device, "vkSetDebugUtilsObjectNameEXT"));

    std::vector<VulkanCommandAvailability> device_availability;
    const auto device_requests = vulkan_device_command_requests(result.logical_device_plan);
    append_device_command_availability(device_availability, get_device_proc_addr, device, device_requests);
    result.device.reset();
    const auto device_command_plan = build_command_resolution_plan(device_requests, device_availability);
    const auto synchronization2_enabled = result.logical_device_plan.synchronization2_enabled;
    if (!device_command_plan.supported || destroy_device == nullptr || get_device_queue == nullptr ||
        create_command_pool == nullptr || destroy_command_pool == nullptr || allocate_command_buffers == nullptr ||
        free_command_buffers == nullptr || begin_command_buffer == nullptr || end_command_buffer == nullptr ||
        cmd_begin_rendering == nullptr || cmd_end_rendering == nullptr || cmd_bind_pipeline == nullptr ||
        cmd_set_viewport == nullptr || cmd_set_scissor == nullptr || cmd_draw == nullptr ||
        cmd_bind_vertex_buffers == nullptr || cmd_bind_index_buffer == nullptr || cmd_draw_indexed == nullptr ||
        (synchronization2_enabled && (cmd_pipeline_barrier2 == nullptr || queue_submit2 == nullptr)) ||
        queue_wait_idle == nullptr || create_buffer == nullptr || destroy_buffer == nullptr ||
        get_buffer_memory_requirements == nullptr || create_image == nullptr || destroy_image == nullptr ||
        get_image_memory_requirements == nullptr || allocate_memory == nullptr || free_memory == nullptr ||
        bind_buffer_memory == nullptr || bind_image_memory == nullptr || map_memory == nullptr ||
        unmap_memory == nullptr || cmd_copy_image_to_buffer == nullptr || cmd_copy_buffer == nullptr ||
        cmd_copy_buffer_to_image == nullptr || create_shader_module == nullptr || destroy_shader_module == nullptr ||
        create_descriptor_set_layout == nullptr || destroy_descriptor_set_layout == nullptr ||
        create_descriptor_pool == nullptr || destroy_descriptor_pool == nullptr ||
        allocate_descriptor_sets == nullptr || update_descriptor_sets == nullptr ||
        cmd_bind_descriptor_sets == nullptr || create_pipeline_layout == nullptr ||
        destroy_pipeline_layout == nullptr || create_graphics_pipelines == nullptr ||
        create_compute_pipelines == nullptr || destroy_pipeline == nullptr || cmd_dispatch == nullptr ||
        create_swapchain == nullptr || destroy_swapchain == nullptr || get_swapchain_images == nullptr ||
        acquire_next_image == nullptr || create_image_view == nullptr || destroy_image_view == nullptr ||
        create_semaphore == nullptr || destroy_semaphore == nullptr || create_fence == nullptr ||
        destroy_fence == nullptr || wait_for_fences == nullptr || queue_present == nullptr) {
        if (device_wait_idle != nullptr) {
            static_cast<void>(device_wait_idle(device));
        }
        if (destroy_device != nullptr) {
            destroy_device(device, nullptr);
        }
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic = device_command_plan.supported ? "Vulkan required device commands are unavailable"
                                                          : device_command_plan.diagnostic;
        return result;
    }

    NativeVulkanQueue graphics_queue = nullptr;
    NativeVulkanQueue present_queue = nullptr;
    get_device_queue(device, result.selection_probe.selection.graphics_queue_family, 0, &graphics_queue);
    get_device_queue(device, result.selection_probe.selection.present_queue_family, 0, &present_queue);
    if (graphics_queue == nullptr || present_queue == nullptr) {
        if (device_wait_idle != nullptr) {
            static_cast<void>(device_wait_idle(device));
        }
        destroy_device(device, nullptr);
        destroy_instance(instance, nullptr);
        FreeLibrary(library);
        result.diagnostic = "Vulkan logical device queues are unavailable";
        return result;
    }

    auto impl = std::make_shared<VulkanRuntimeDevice::Impl>();
    impl->library = library;
    impl->instance = instance;
    impl->physical_device = physical_devices[selected_index];
    impl->device = device;
    impl->graphics_queue = graphics_queue;
    impl->present_queue = present_queue;
    impl->destroy_instance = destroy_instance;
    impl->get_physical_device_memory_properties = get_physical_device_memory_properties;
    impl->create_win32_surface = create_win32_surface;
    impl->destroy_surface = destroy_surface;
    impl->get_surface_capabilities = get_surface_capabilities;
    impl->get_surface_formats = get_surface_formats;
    impl->get_surface_present_modes = get_surface_present_modes;
    impl->destroy_device = destroy_device;
    impl->device_wait_idle = device_wait_idle;
    impl->create_command_pool = create_command_pool;
    impl->destroy_command_pool = destroy_command_pool;
    impl->allocate_command_buffers = allocate_command_buffers;
    impl->free_command_buffers = free_command_buffers;
    impl->begin_command_buffer = begin_command_buffer;
    impl->end_command_buffer = end_command_buffer;
    impl->cmd_begin_rendering = cmd_begin_rendering;
    impl->cmd_end_rendering = cmd_end_rendering;
    impl->cmd_bind_pipeline = cmd_bind_pipeline;
    impl->cmd_set_viewport = cmd_set_viewport;
    impl->cmd_set_scissor = cmd_set_scissor;
    impl->cmd_draw = cmd_draw;
    impl->cmd_bind_vertex_buffers = cmd_bind_vertex_buffers;
    impl->cmd_bind_index_buffer = cmd_bind_index_buffer;
    impl->cmd_draw_indexed = cmd_draw_indexed;
    impl->cmd_pipeline_barrier2 = synchronization2_enabled ? cmd_pipeline_barrier2 : nullptr;
    impl->queue_submit2 = synchronization2_enabled ? queue_submit2 : nullptr;
    impl->queue_wait_idle = queue_wait_idle;
    impl->create_buffer = create_buffer;
    impl->destroy_buffer = destroy_buffer;
    impl->get_buffer_memory_requirements = get_buffer_memory_requirements;
    impl->create_image = create_image;
    impl->destroy_image = destroy_image;
    impl->get_image_memory_requirements = get_image_memory_requirements;
    impl->allocate_memory = allocate_memory;
    impl->free_memory = free_memory;
    impl->bind_buffer_memory = bind_buffer_memory;
    impl->bind_image_memory = bind_image_memory;
    impl->map_memory = map_memory;
    impl->unmap_memory = unmap_memory;
    impl->cmd_copy_image_to_buffer = cmd_copy_image_to_buffer;
    impl->cmd_copy_buffer = cmd_copy_buffer;
    impl->cmd_copy_buffer_to_image = cmd_copy_buffer_to_image;
    impl->create_shader_module = create_shader_module;
    impl->destroy_shader_module = destroy_shader_module;
    impl->create_descriptor_set_layout = create_descriptor_set_layout;
    impl->destroy_descriptor_set_layout = destroy_descriptor_set_layout;
    impl->create_descriptor_pool = create_descriptor_pool;
    impl->destroy_descriptor_pool = destroy_descriptor_pool;
    impl->allocate_descriptor_sets = allocate_descriptor_sets;
    impl->update_descriptor_sets = update_descriptor_sets;
    impl->cmd_bind_descriptor_sets = cmd_bind_descriptor_sets;
    impl->create_swapchain = create_swapchain;
    impl->destroy_swapchain = destroy_swapchain;
    impl->get_swapchain_images = get_swapchain_images;
    impl->acquire_next_image = acquire_next_image;
    impl->create_image_view = create_image_view;
    impl->destroy_image_view = destroy_image_view;
    impl->create_sampler = create_sampler;
    impl->destroy_sampler = destroy_sampler;
    impl->create_semaphore = create_semaphore;
    impl->destroy_semaphore = destroy_semaphore;
    impl->create_fence = create_fence;
    impl->destroy_fence = destroy_fence;
    impl->wait_for_fences = wait_for_fences;
    impl->queue_present = queue_present;
    impl->create_pipeline_layout = create_pipeline_layout;
    impl->destroy_pipeline_layout = destroy_pipeline_layout;
    impl->create_graphics_pipelines = create_graphics_pipelines;
    impl->create_compute_pipelines = create_compute_pipelines;
    impl->destroy_pipeline = destroy_pipeline;
    impl->cmd_dispatch = cmd_dispatch;
    impl->set_debug_utils_object_name = set_debug_utils_object_name;
    impl->graphics_queue_family = result.selection_probe.selection.graphics_queue_family;
    impl->present_queue_family = result.selection_probe.selection.present_queue_family;
    impl->host = host;
    impl->logical_device_plan = result.logical_device_plan;
    impl->command_plan = device_command_plan;
    vulkan_apply_debug_utils_names_for_device_and_queues(static_cast<void*>(impl.get()));
    result.device = VulkanRuntimeDevice{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime device owner ready";
    return result;
#elif defined(__linux__)
    void* library = dlopen(runtime_library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        result.diagnostic = "Vulkan runtime library could not be loaded";
        return result;
    }

    const auto get_instance_proc_addr = reinterpret_cast<VulkanGetInstanceProcAddr>(
        dlsym(library, std::string{loader_desc.get_instance_proc_addr_symbol}.c_str()));
    const auto commands = resolve_runtime_global_commands(get_instance_proc_addr);
    if (commands.create_instance == nullptr) {
        dlclose(library);
        result.diagnostic = "Vulkan vkCreateInstance command is unavailable";
        return result;
    }

    const auto extension_pointers = extension_name_pointers(
        result.selection_probe.snapshots.count_probe.instance.capabilities.instance_plan.enabled_extensions);
    const auto application_info = make_native_application_info(runtime_instance_desc);
    const auto create_info = make_native_instance_create_info(application_info, extension_pointers);
    NativeVulkanInstance instance = nullptr;
    const auto create_result = commands.create_instance(&create_info, nullptr, &instance);
    if (create_result != vulkan_success || instance == nullptr) {
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateInstance failed", create_result);
        return result;
    }

    const auto destroy_instance =
        reinterpret_cast<VulkanDestroyInstance>(get_instance_proc_addr(instance, "vkDestroyInstance"));
    const auto create_win32_surface =
        reinterpret_cast<VulkanCreateWin32Surface>(get_instance_proc_addr(instance, "vkCreateWin32SurfaceKHR"));
    const auto destroy_surface =
        reinterpret_cast<VulkanDestroySurface>(get_instance_proc_addr(instance, "vkDestroySurfaceKHR"));
    const auto get_surface_capabilities = reinterpret_cast<VulkanGetPhysicalDeviceSurfaceCapabilities>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    const auto get_physical_device_memory_properties = reinterpret_cast<VulkanGetPhysicalDeviceMemoryProperties>(
        get_instance_proc_addr(instance, "vkGetPhysicalDeviceMemoryProperties"));
    const auto enumerate_physical_devices = reinterpret_cast<VulkanEnumeratePhysicalDevices>(
        get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices"));
    const auto create_device = reinterpret_cast<VulkanCreateDevice>(get_instance_proc_addr(instance, "vkCreateDevice"));
    const auto get_device_proc_addr =
        reinterpret_cast<VulkanGetDeviceProcAddr>(get_instance_proc_addr(instance, "vkGetDeviceProcAddr"));
    if (destroy_instance == nullptr || get_physical_device_memory_properties == nullptr ||
        enumerate_physical_devices == nullptr || create_device == nullptr || get_device_proc_addr == nullptr) {
        if (destroy_instance != nullptr) {
            destroy_instance(instance, nullptr);
        }
        dlclose(library);
        result.diagnostic = "Vulkan logical device instance commands are unavailable";
        return result;
    }

    std::uint32_t physical_device_count = result.selection_probe.snapshots.count_probe.physical_device_count;
    std::vector<NativeVulkanPhysicalDevice> physical_devices(physical_device_count);
    const auto enumerate_result = enumerate_physical_devices(instance, &physical_device_count, physical_devices.data());
    if (!is_successful_enumeration_result(enumerate_result)) {
        destroy_instance(instance, nullptr);
        dlclose(library);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan physical device handle enumeration failed", enumerate_result);
        return result;
    }
    physical_devices.resize(physical_device_count);
    if (selected_index >= physical_devices.size()) {
        destroy_instance(instance, nullptr);
        dlclose(library);
        result.diagnostic = "Vulkan selected physical device handle is unavailable";
        return result;
    }

    std::vector<float> queue_priorities;
    queue_priorities.reserve(result.logical_device_plan.queue_families.size());
    for (const auto& queue_plan : result.logical_device_plan.queue_families) {
        queue_priorities.push_back(queue_plan.priority);
    }
    const auto queue_infos =
        make_native_device_queue_create_infos(result.logical_device_plan.queue_families, queue_priorities);
    const auto device_extension_pointers = extension_name_pointers(result.logical_device_plan.enabled_extensions);
    auto vulkan13_features = make_native_vulkan13_features(result.logical_device_plan);
    const auto enable_vulkan13_features =
        result.logical_device_plan.dynamic_rendering_enabled || result.logical_device_plan.synchronization2_enabled;
    const auto device_create_info = make_native_device_create_info(
        queue_infos, device_extension_pointers, enable_vulkan13_features ? &vulkan13_features : nullptr);

    NativeVulkanDevice device = nullptr;
    const auto device_create_result =
        create_device(physical_devices[selected_index], &device_create_info, nullptr, &device);
    if (device_create_result != vulkan_success || device == nullptr) {
        destroy_instance(instance, nullptr);
        dlclose(library);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateDevice failed", device_create_result);
        return result;
    }

    auto destroy_device = reinterpret_cast<VulkanDestroyDevice>(get_device_proc_addr(device, "vkDestroyDevice"));
    if (destroy_device == nullptr) {
        destroy_device = reinterpret_cast<VulkanDestroyDevice>(get_instance_proc_addr(instance, "vkDestroyDevice"));
    }
    const auto device_wait_idle =
        reinterpret_cast<VulkanDeviceWaitIdle>(get_device_proc_addr(device, "vkDeviceWaitIdle"));
    const auto get_device_queue =
        reinterpret_cast<VulkanGetDeviceQueue>(get_device_proc_addr(device, "vkGetDeviceQueue"));
    const auto create_command_pool =
        reinterpret_cast<VulkanCreateCommandPool>(get_device_proc_addr(device, "vkCreateCommandPool"));
    const auto destroy_command_pool =
        reinterpret_cast<VulkanDestroyCommandPool>(get_device_proc_addr(device, "vkDestroyCommandPool"));
    const auto allocate_command_buffers =
        reinterpret_cast<VulkanAllocateCommandBuffers>(get_device_proc_addr(device, "vkAllocateCommandBuffers"));
    const auto free_command_buffers =
        reinterpret_cast<VulkanFreeCommandBuffers>(get_device_proc_addr(device, "vkFreeCommandBuffers"));
    const auto begin_command_buffer =
        reinterpret_cast<VulkanBeginCommandBuffer>(get_device_proc_addr(device, "vkBeginCommandBuffer"));
    const auto end_command_buffer =
        reinterpret_cast<VulkanEndCommandBuffer>(get_device_proc_addr(device, "vkEndCommandBuffer"));
    const auto cmd_begin_rendering =
        reinterpret_cast<VulkanCmdBeginRendering>(get_device_proc_addr(device, "vkCmdBeginRendering"));
    const auto cmd_end_rendering =
        reinterpret_cast<VulkanCmdEndRendering>(get_device_proc_addr(device, "vkCmdEndRendering"));
    const auto cmd_bind_pipeline =
        reinterpret_cast<VulkanCmdBindPipeline>(get_device_proc_addr(device, "vkCmdBindPipeline"));
    const auto cmd_set_viewport =
        reinterpret_cast<VulkanCmdSetViewport>(get_device_proc_addr(device, "vkCmdSetViewport"));
    const auto cmd_set_scissor = reinterpret_cast<VulkanCmdSetScissor>(get_device_proc_addr(device, "vkCmdSetScissor"));
    const auto cmd_draw = reinterpret_cast<VulkanCmdDraw>(get_device_proc_addr(device, "vkCmdDraw"));
    const auto cmd_bind_vertex_buffers =
        reinterpret_cast<VulkanCmdBindVertexBuffers>(get_device_proc_addr(device, "vkCmdBindVertexBuffers"));
    const auto cmd_bind_index_buffer =
        reinterpret_cast<VulkanCmdBindIndexBuffer>(get_device_proc_addr(device, "vkCmdBindIndexBuffer"));
    const auto cmd_draw_indexed =
        reinterpret_cast<VulkanCmdDrawIndexed>(get_device_proc_addr(device, "vkCmdDrawIndexed"));
    const auto cmd_pipeline_barrier2 =
        reinterpret_cast<VulkanCmdPipelineBarrier2>(get_device_proc_addr(device, "vkCmdPipelineBarrier2"));
    const auto queue_submit2 = reinterpret_cast<VulkanQueueSubmit2>(get_device_proc_addr(device, "vkQueueSubmit2"));
    const auto queue_wait_idle = reinterpret_cast<VulkanQueueWaitIdle>(get_device_proc_addr(device, "vkQueueWaitIdle"));
    const auto create_buffer = reinterpret_cast<VulkanCreateBuffer>(get_device_proc_addr(device, "vkCreateBuffer"));
    const auto destroy_buffer = reinterpret_cast<VulkanDestroyBuffer>(get_device_proc_addr(device, "vkDestroyBuffer"));
    const auto get_buffer_memory_requirements = reinterpret_cast<VulkanGetBufferMemoryRequirements>(
        get_device_proc_addr(device, "vkGetBufferMemoryRequirements"));
    const auto create_image = reinterpret_cast<VulkanCreateImage>(get_device_proc_addr(device, "vkCreateImage"));
    const auto destroy_image = reinterpret_cast<VulkanDestroyImage>(get_device_proc_addr(device, "vkDestroyImage"));
    const auto get_image_memory_requirements = reinterpret_cast<VulkanGetImageMemoryRequirements>(
        get_device_proc_addr(device, "vkGetImageMemoryRequirements"));
    const auto allocate_memory =
        reinterpret_cast<VulkanAllocateMemory>(get_device_proc_addr(device, "vkAllocateMemory"));
    const auto free_memory = reinterpret_cast<VulkanFreeMemory>(get_device_proc_addr(device, "vkFreeMemory"));
    const auto bind_buffer_memory =
        reinterpret_cast<VulkanBindBufferMemory>(get_device_proc_addr(device, "vkBindBufferMemory"));
    const auto bind_image_memory =
        reinterpret_cast<VulkanBindImageMemory>(get_device_proc_addr(device, "vkBindImageMemory"));
    const auto map_memory = reinterpret_cast<VulkanMapMemory>(get_device_proc_addr(device, "vkMapMemory"));
    const auto unmap_memory = reinterpret_cast<VulkanUnmapMemory>(get_device_proc_addr(device, "vkUnmapMemory"));
    const auto cmd_copy_image_to_buffer =
        reinterpret_cast<VulkanCmdCopyImageToBuffer>(get_device_proc_addr(device, "vkCmdCopyImageToBuffer"));
    const auto cmd_copy_buffer = reinterpret_cast<VulkanCmdCopyBuffer>(get_device_proc_addr(device, "vkCmdCopyBuffer"));
    const auto cmd_copy_buffer_to_image =
        reinterpret_cast<VulkanCmdCopyBufferToImage>(get_device_proc_addr(device, "vkCmdCopyBufferToImage"));
    const auto create_shader_module =
        reinterpret_cast<VulkanCreateShaderModule>(get_device_proc_addr(device, "vkCreateShaderModule"));
    const auto destroy_shader_module =
        reinterpret_cast<VulkanDestroyShaderModule>(get_device_proc_addr(device, "vkDestroyShaderModule"));
    const auto create_descriptor_set_layout =
        reinterpret_cast<VulkanCreateDescriptorSetLayout>(get_device_proc_addr(device, "vkCreateDescriptorSetLayout"));
    const auto destroy_descriptor_set_layout = reinterpret_cast<VulkanDestroyDescriptorSetLayout>(
        get_device_proc_addr(device, "vkDestroyDescriptorSetLayout"));
    const auto create_descriptor_pool =
        reinterpret_cast<VulkanCreateDescriptorPool>(get_device_proc_addr(device, "vkCreateDescriptorPool"));
    const auto destroy_descriptor_pool =
        reinterpret_cast<VulkanDestroyDescriptorPool>(get_device_proc_addr(device, "vkDestroyDescriptorPool"));
    const auto allocate_descriptor_sets =
        reinterpret_cast<VulkanAllocateDescriptorSets>(get_device_proc_addr(device, "vkAllocateDescriptorSets"));
    const auto update_descriptor_sets =
        reinterpret_cast<VulkanUpdateDescriptorSets>(get_device_proc_addr(device, "vkUpdateDescriptorSets"));
    const auto cmd_bind_descriptor_sets =
        reinterpret_cast<VulkanCmdBindDescriptorSets>(get_device_proc_addr(device, "vkCmdBindDescriptorSets"));
    const auto create_swapchain =
        reinterpret_cast<VulkanCreateSwapchain>(get_device_proc_addr(device, "vkCreateSwapchainKHR"));
    const auto destroy_swapchain =
        reinterpret_cast<VulkanDestroySwapchain>(get_device_proc_addr(device, "vkDestroySwapchainKHR"));
    const auto get_swapchain_images =
        reinterpret_cast<VulkanGetSwapchainImages>(get_device_proc_addr(device, "vkGetSwapchainImagesKHR"));
    const auto acquire_next_image =
        reinterpret_cast<VulkanAcquireNextImage>(get_device_proc_addr(device, "vkAcquireNextImageKHR"));
    const auto create_image_view =
        reinterpret_cast<VulkanCreateImageView>(get_device_proc_addr(device, "vkCreateImageView"));
    const auto destroy_image_view =
        reinterpret_cast<VulkanDestroyImageView>(get_device_proc_addr(device, "vkDestroyImageView"));
    const auto create_sampler = reinterpret_cast<VulkanCreateSampler>(get_device_proc_addr(device, "vkCreateSampler"));
    const auto destroy_sampler =
        reinterpret_cast<VulkanDestroySampler>(get_device_proc_addr(device, "vkDestroySampler"));
    const auto create_semaphore =
        reinterpret_cast<VulkanCreateSemaphore>(get_device_proc_addr(device, "vkCreateSemaphore"));
    const auto destroy_semaphore =
        reinterpret_cast<VulkanDestroySemaphore>(get_device_proc_addr(device, "vkDestroySemaphore"));
    const auto create_fence = reinterpret_cast<VulkanCreateFence>(get_device_proc_addr(device, "vkCreateFence"));
    const auto destroy_fence = reinterpret_cast<VulkanDestroyFence>(get_device_proc_addr(device, "vkDestroyFence"));
    const auto wait_for_fences = reinterpret_cast<VulkanWaitForFences>(get_device_proc_addr(device, "vkWaitForFences"));
    const auto queue_present = reinterpret_cast<VulkanQueuePresent>(get_device_proc_addr(device, "vkQueuePresentKHR"));
    const auto create_pipeline_layout =
        reinterpret_cast<VulkanCreatePipelineLayout>(get_device_proc_addr(device, "vkCreatePipelineLayout"));
    const auto destroy_pipeline_layout =
        reinterpret_cast<VulkanDestroyPipelineLayout>(get_device_proc_addr(device, "vkDestroyPipelineLayout"));
    const auto create_graphics_pipelines =
        reinterpret_cast<VulkanCreateGraphicsPipelines>(get_device_proc_addr(device, "vkCreateGraphicsPipelines"));
    const auto create_compute_pipelines =
        reinterpret_cast<VulkanCreateComputePipelines>(get_device_proc_addr(device, "vkCreateComputePipelines"));
    const auto destroy_pipeline =
        reinterpret_cast<VulkanDestroyPipeline>(get_device_proc_addr(device, "vkDestroyPipeline"));
    const auto cmd_dispatch = reinterpret_cast<VulkanCmdDispatch>(get_device_proc_addr(device, "vkCmdDispatch"));
    const auto set_debug_utils_object_name =
        reinterpret_cast<VulkanSetDebugUtilsObjectName>(get_device_proc_addr(device, "vkSetDebugUtilsObjectNameEXT"));

    std::vector<VulkanCommandAvailability> device_availability;
    const auto device_requests = vulkan_device_command_requests(result.logical_device_plan);
    append_device_command_availability(device_availability, get_device_proc_addr, device, device_requests);
    const auto device_command_plan = build_command_resolution_plan(device_requests, device_availability);
    const auto synchronization2_enabled = result.logical_device_plan.synchronization2_enabled;
    if (!device_command_plan.supported || destroy_device == nullptr || get_device_queue == nullptr ||
        create_command_pool == nullptr || destroy_command_pool == nullptr || allocate_command_buffers == nullptr ||
        free_command_buffers == nullptr || begin_command_buffer == nullptr || end_command_buffer == nullptr ||
        cmd_begin_rendering == nullptr || cmd_end_rendering == nullptr || cmd_bind_pipeline == nullptr ||
        cmd_set_viewport == nullptr || cmd_set_scissor == nullptr || cmd_draw == nullptr ||
        cmd_bind_vertex_buffers == nullptr || cmd_bind_index_buffer == nullptr || cmd_draw_indexed == nullptr ||
        (synchronization2_enabled && (cmd_pipeline_barrier2 == nullptr || queue_submit2 == nullptr)) ||
        queue_wait_idle == nullptr || create_buffer == nullptr || destroy_buffer == nullptr ||
        get_buffer_memory_requirements == nullptr || create_image == nullptr || destroy_image == nullptr ||
        get_image_memory_requirements == nullptr || allocate_memory == nullptr || free_memory == nullptr ||
        bind_buffer_memory == nullptr || bind_image_memory == nullptr || map_memory == nullptr ||
        unmap_memory == nullptr || cmd_copy_image_to_buffer == nullptr || cmd_copy_buffer == nullptr ||
        cmd_copy_buffer_to_image == nullptr || create_shader_module == nullptr || destroy_shader_module == nullptr ||
        create_descriptor_set_layout == nullptr || destroy_descriptor_set_layout == nullptr ||
        create_descriptor_pool == nullptr || destroy_descriptor_pool == nullptr ||
        allocate_descriptor_sets == nullptr || update_descriptor_sets == nullptr ||
        cmd_bind_descriptor_sets == nullptr || create_pipeline_layout == nullptr ||
        destroy_pipeline_layout == nullptr || create_graphics_pipelines == nullptr ||
        create_compute_pipelines == nullptr || destroy_pipeline == nullptr || cmd_dispatch == nullptr ||
        create_swapchain == nullptr || destroy_swapchain == nullptr || get_swapchain_images == nullptr ||
        acquire_next_image == nullptr || create_image_view == nullptr || destroy_image_view == nullptr ||
        create_semaphore == nullptr || destroy_semaphore == nullptr || create_fence == nullptr ||
        destroy_fence == nullptr || wait_for_fences == nullptr || queue_present == nullptr) {
        if (device_wait_idle != nullptr) {
            static_cast<void>(device_wait_idle(device));
        }
        if (destroy_device != nullptr) {
            destroy_device(device, nullptr);
        }
        destroy_instance(instance, nullptr);
        dlclose(library);
        result.diagnostic = device_command_plan.supported ? "Vulkan required device commands are unavailable"
                                                          : device_command_plan.diagnostic;
        return result;
    }

    NativeVulkanQueue graphics_queue = nullptr;
    NativeVulkanQueue present_queue = nullptr;
    get_device_queue(device, result.selection_probe.selection.graphics_queue_family, 0, &graphics_queue);
    get_device_queue(device, result.selection_probe.selection.present_queue_family, 0, &present_queue);
    if (graphics_queue == nullptr || present_queue == nullptr) {
        if (device_wait_idle != nullptr) {
            static_cast<void>(device_wait_idle(device));
        }
        destroy_device(device, nullptr);
        destroy_instance(instance, nullptr);
        dlclose(library);
        result.diagnostic = "Vulkan logical device queues are unavailable";
        return result;
    }

    auto impl = std::make_shared<VulkanRuntimeDevice::Impl>();
    impl->library = library;
    impl->instance = instance;
    impl->physical_device = physical_devices[selected_index];
    impl->device = device;
    impl->graphics_queue = graphics_queue;
    impl->present_queue = present_queue;
    impl->destroy_instance = destroy_instance;
    impl->get_physical_device_memory_properties = get_physical_device_memory_properties;
    impl->create_win32_surface = create_win32_surface;
    impl->destroy_surface = destroy_surface;
    impl->get_surface_capabilities = get_surface_capabilities;
    impl->destroy_device = destroy_device;
    impl->device_wait_idle = device_wait_idle;
    impl->create_command_pool = create_command_pool;
    impl->destroy_command_pool = destroy_command_pool;
    impl->allocate_command_buffers = allocate_command_buffers;
    impl->free_command_buffers = free_command_buffers;
    impl->begin_command_buffer = begin_command_buffer;
    impl->end_command_buffer = end_command_buffer;
    impl->cmd_begin_rendering = cmd_begin_rendering;
    impl->cmd_end_rendering = cmd_end_rendering;
    impl->cmd_bind_pipeline = cmd_bind_pipeline;
    impl->cmd_set_viewport = cmd_set_viewport;
    impl->cmd_set_scissor = cmd_set_scissor;
    impl->cmd_draw = cmd_draw;
    impl->cmd_bind_vertex_buffers = cmd_bind_vertex_buffers;
    impl->cmd_bind_index_buffer = cmd_bind_index_buffer;
    impl->cmd_draw_indexed = cmd_draw_indexed;
    impl->cmd_pipeline_barrier2 = synchronization2_enabled ? cmd_pipeline_barrier2 : nullptr;
    impl->queue_submit2 = synchronization2_enabled ? queue_submit2 : nullptr;
    impl->queue_wait_idle = queue_wait_idle;
    impl->create_buffer = create_buffer;
    impl->destroy_buffer = destroy_buffer;
    impl->get_buffer_memory_requirements = get_buffer_memory_requirements;
    impl->create_image = create_image;
    impl->destroy_image = destroy_image;
    impl->get_image_memory_requirements = get_image_memory_requirements;
    impl->allocate_memory = allocate_memory;
    impl->free_memory = free_memory;
    impl->bind_buffer_memory = bind_buffer_memory;
    impl->bind_image_memory = bind_image_memory;
    impl->map_memory = map_memory;
    impl->unmap_memory = unmap_memory;
    impl->cmd_copy_image_to_buffer = cmd_copy_image_to_buffer;
    impl->cmd_copy_buffer = cmd_copy_buffer;
    impl->cmd_copy_buffer_to_image = cmd_copy_buffer_to_image;
    impl->create_shader_module = create_shader_module;
    impl->destroy_shader_module = destroy_shader_module;
    impl->create_descriptor_set_layout = create_descriptor_set_layout;
    impl->destroy_descriptor_set_layout = destroy_descriptor_set_layout;
    impl->create_descriptor_pool = create_descriptor_pool;
    impl->destroy_descriptor_pool = destroy_descriptor_pool;
    impl->allocate_descriptor_sets = allocate_descriptor_sets;
    impl->update_descriptor_sets = update_descriptor_sets;
    impl->cmd_bind_descriptor_sets = cmd_bind_descriptor_sets;
    impl->create_swapchain = create_swapchain;
    impl->destroy_swapchain = destroy_swapchain;
    impl->get_swapchain_images = get_swapchain_images;
    impl->acquire_next_image = acquire_next_image;
    impl->create_image_view = create_image_view;
    impl->destroy_image_view = destroy_image_view;
    impl->create_sampler = create_sampler;
    impl->destroy_sampler = destroy_sampler;
    impl->create_semaphore = create_semaphore;
    impl->destroy_semaphore = destroy_semaphore;
    impl->create_fence = create_fence;
    impl->destroy_fence = destroy_fence;
    impl->wait_for_fences = wait_for_fences;
    impl->queue_present = queue_present;
    impl->create_pipeline_layout = create_pipeline_layout;
    impl->destroy_pipeline_layout = destroy_pipeline_layout;
    impl->create_graphics_pipelines = create_graphics_pipelines;
    impl->create_compute_pipelines = create_compute_pipelines;
    impl->destroy_pipeline = destroy_pipeline;
    impl->cmd_dispatch = cmd_dispatch;
    impl->set_debug_utils_object_name = set_debug_utils_object_name;
    impl->graphics_queue_family = result.selection_probe.selection.graphics_queue_family;
    impl->present_queue_family = result.selection_probe.selection.present_queue_family;
    impl->host = host;
    impl->logical_device_plan = result.logical_device_plan;
    impl->command_plan = device_command_plan;
    vulkan_apply_debug_utils_names_for_device_and_queues(static_cast<void*>(impl.get()));
    result.device = VulkanRuntimeDevice{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime device owner ready";
    return result;
#else
    result.diagnostic = "Vulkan runtime device ownership is unsupported on this host";
    return result;
#endif
}

std::unique_ptr<IRhiDevice> create_rhi_device(VulkanRuntimeDevice device,
                                              const VulkanRhiDeviceMappingPlan& mapping_plan) {
    if (!device.owns_device() || !complete_rhi_device_mapping_plan(mapping_plan)) {
        return nullptr;
    }
    return std::make_unique<VulkanRhiDevice>(std::move(device));
}

VulkanRuntimeCommandPoolCreateResult create_runtime_command_pool(VulkanRuntimeDevice& device,
                                                                 const VulkanRuntimeCommandPoolDesc& desc) {
    VulkanRuntimeCommandPoolCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }

    const auto queue_family =
        desc.queue_family == invalid_vulkan_queue_family ? device.impl_->graphics_queue_family : desc.queue_family;
    if (queue_family == invalid_vulkan_queue_family) {
        result.diagnostic = "Vulkan command pool queue family is invalid";
        return result;
    }
    if (device.impl_->create_command_pool == nullptr || device.impl_->destroy_command_pool == nullptr ||
        device.impl_->allocate_command_buffers == nullptr || device.impl_->free_command_buffers == nullptr ||
        device.impl_->begin_command_buffer == nullptr || device.impl_->end_command_buffer == nullptr) {
        result.diagnostic = "Vulkan device command pool commands are unavailable";
        return result;
    }

    std::uint32_t flags = 0;
    if (desc.transient) {
        flags |= vulkan_command_pool_create_transient_bit;
    }
    if (desc.reset_command_buffer) {
        flags |= vulkan_command_pool_create_reset_command_buffer_bit;
    }

    const NativeVulkanCommandPoolCreateInfo pool_create_info{
        .s_type = vulkan_structure_type_command_pool_create_info,
        .next = nullptr,
        .flags = flags,
        .queue_family_index = queue_family,
    };
    NativeVulkanCommandPool pool = 0;
    const auto pool_result = device.impl_->create_command_pool(device.impl_->device, &pool_create_info, nullptr, &pool);
    if (pool_result != vulkan_success || pool == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateCommandPool failed", pool_result);
        return result;
    }

    const NativeVulkanCommandBufferAllocateInfo allocate_info{
        .s_type = vulkan_structure_type_command_buffer_allocate_info,
        .next = nullptr,
        .command_pool = pool,
        .level = vulkan_command_buffer_level_primary,
        .command_buffer_count = 1,
    };
    NativeVulkanCommandBuffer command_buffer = nullptr;
    const auto allocate_result =
        device.impl_->allocate_command_buffers(device.impl_->device, &allocate_info, &command_buffer);
    if (allocate_result != vulkan_success || command_buffer == nullptr) {
        device.impl_->destroy_command_pool(device.impl_->device, pool, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkAllocateCommandBuffers failed", allocate_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_command_pool, pool,
                                "GameEngine.RHI.Vulkan.CommandPool");
    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_command_buffer,
                                static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(command_buffer)),
                                "GameEngine.RHI.Vulkan.CommandBuffer");

    auto impl = std::make_unique<VulkanRuntimeCommandPool::Impl>();
    impl->device_owner = device.impl_;
    impl->pool = pool;
    impl->primary_command_buffer = command_buffer;
    result.pool = VulkanRuntimeCommandPool{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime command pool owner ready";
    return result;
}

VulkanRuntimeBufferCreateResult create_runtime_buffer(VulkanRuntimeDevice& device,
                                                      const VulkanRuntimeBufferDesc& desc) {
    VulkanRuntimeBufferCreateResult result;
    result.plan = build_runtime_buffer_create_plan(desc);
    if (!result.plan.supported) {
        result.diagnostic = result.plan.diagnostic;
        return result;
    }
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (device.impl_->create_buffer == nullptr || device.impl_->destroy_buffer == nullptr ||
        device.impl_->get_buffer_memory_requirements == nullptr || device.impl_->allocate_memory == nullptr ||
        device.impl_->free_memory == nullptr || device.impl_->bind_buffer_memory == nullptr ||
        device.impl_->get_physical_device_memory_properties == nullptr) {
        result.diagnostic = "Vulkan runtime buffer commands are unavailable";
        return result;
    }

    const NativeVulkanBufferCreateInfo buffer_create_info{
        .s_type = vulkan_structure_type_buffer_create_info,
        .next = nullptr,
        .flags = 0,
        .size = result.plan.size_bytes,
        .usage = native_vulkan_buffer_usage_flags(result.plan.usage),
        .sharing_mode = vulkan_sharing_mode_exclusive,
        .queue_family_index_count = 0,
        .queue_family_indices = nullptr,
    };
    NativeVulkanBuffer buffer = 0;
    const auto buffer_result = device.impl_->create_buffer(device.impl_->device, &buffer_create_info, nullptr, &buffer);
    if (buffer_result != vulkan_success || buffer == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateBuffer failed", buffer_result);
        return result;
    }

    NativeVulkanMemoryRequirements requirements{};
    device.impl_->get_buffer_memory_requirements(device.impl_->device, buffer, &requirements);
    if (requirements.size == 0 || requirements.memory_type_bits == 0) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = "Vulkan buffer memory requirements are unavailable";
        return result;
    }

    NativeVulkanPhysicalDeviceMemoryProperties memory_properties{};
    device.impl_->get_physical_device_memory_properties(device.impl_->physical_device, &memory_properties);
    const auto memory_type_index = find_memory_type_index(memory_properties, requirements.memory_type_bits,
                                                          native_vulkan_memory_property_flags(result.plan.memory));
    if (memory_type_index == invalid_vulkan_queue_family) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = "Vulkan buffer memory type is unavailable";
        return result;
    }

    const NativeVulkanMemoryAllocateInfo allocate_info{
        .s_type = vulkan_structure_type_memory_allocate_info,
        .next = nullptr,
        .allocation_size = requirements.size,
        .memory_type_index = memory_type_index,
    };
    NativeVulkanDeviceMemory memory = 0;
    const auto allocate_result = device.impl_->allocate_memory(device.impl_->device, &allocate_info, nullptr, &memory);
    if (allocate_result != vulkan_success || memory == 0) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkAllocateMemory failed", allocate_result);
        return result;
    }

    const auto bind_result = device.impl_->bind_buffer_memory(device.impl_->device, buffer, memory, 0);
    if (bind_result != vulkan_success) {
        device.impl_->free_memory(device.impl_->device, memory, nullptr);
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkBindBufferMemory failed", bind_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_buffer, buffer,
                                "GameEngine.RHI.Vulkan.Buffer");

    auto impl = std::make_unique<VulkanRuntimeBuffer::Impl>();
    impl->device_owner = device.impl_;
    impl->buffer = buffer;
    impl->memory = memory;
    impl->desc = desc.buffer;
    impl->memory_domain = desc.memory_domain;
    result.buffer = VulkanRuntimeBuffer{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime buffer owner ready";
    return result;
}

VulkanRuntimeTextureCreateResult create_runtime_texture(VulkanRuntimeDevice& device,
                                                        const VulkanRuntimeTextureDesc& desc) {
    VulkanRuntimeTextureCreateResult result;
    result.plan = build_runtime_texture_create_plan(desc);
    if (!result.plan.supported) {
        result.diagnostic = result.plan.diagnostic;
        return result;
    }
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (device.impl_->create_image == nullptr || device.impl_->destroy_image == nullptr ||
        device.impl_->get_image_memory_requirements == nullptr || device.impl_->allocate_memory == nullptr ||
        device.impl_->free_memory == nullptr || device.impl_->bind_image_memory == nullptr ||
        device.impl_->get_physical_device_memory_properties == nullptr) {
        result.diagnostic = "Vulkan runtime texture commands are unavailable";
        return result;
    }
    const bool needs_image_view = has_flag(desc.texture.usage, TextureUsage::render_target) ||
                                  has_flag(desc.texture.usage, TextureUsage::depth_stencil) ||
                                  has_flag(desc.texture.usage, TextureUsage::shader_resource) ||
                                  has_flag(desc.texture.usage, TextureUsage::storage);
    if (needs_image_view &&
        (device.impl_->create_image_view == nullptr || device.impl_->destroy_image_view == nullptr)) {
        result.diagnostic = "Vulkan runtime texture image-view commands are unavailable";
        return result;
    }

    const NativeVulkanImageCreateInfo image_create_info{
        .s_type = vulkan_structure_type_image_create_info,
        .next = nullptr,
        .flags = 0,
        .image_type = vulkan_image_type_2d,
        .format = native_vulkan_format(result.plan.format),
        .extent = NativeVulkanExtent3D{.width = result.plan.extent.width,
                                       .height = result.plan.extent.height,
                                       .depth = result.plan.extent.depth},
        .mip_levels = 1,
        .array_layers = 1,
        .samples = vulkan_sample_count_1_bit,
        .tiling = vulkan_image_tiling_optimal,
        .usage = native_vulkan_texture_usage_flags(result.plan.usage),
        .sharing_mode = vulkan_sharing_mode_exclusive,
        .queue_family_index_count = 0,
        .queue_family_indices = nullptr,
        .initial_layout = vulkan_image_layout_undefined,
    };
    NativeVulkanImage image = 0;
    const auto image_result = device.impl_->create_image(device.impl_->device, &image_create_info, nullptr, &image);
    if (image_result != vulkan_success || image == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateImage failed", image_result);
        return result;
    }

    NativeVulkanMemoryRequirements requirements{};
    device.impl_->get_image_memory_requirements(device.impl_->device, image, &requirements);
    if (requirements.size == 0 || requirements.memory_type_bits == 0) {
        device.impl_->destroy_image(device.impl_->device, image, nullptr);
        result.diagnostic = "Vulkan image memory requirements are unavailable";
        return result;
    }

    NativeVulkanPhysicalDeviceMemoryProperties memory_properties{};
    device.impl_->get_physical_device_memory_properties(device.impl_->physical_device, &memory_properties);
    const auto memory_type_index = find_memory_type_index(memory_properties, requirements.memory_type_bits,
                                                          native_vulkan_memory_property_flags(result.plan.memory));
    if (memory_type_index == invalid_vulkan_queue_family) {
        device.impl_->destroy_image(device.impl_->device, image, nullptr);
        result.diagnostic = "Vulkan image memory type is unavailable";
        return result;
    }

    const NativeVulkanMemoryAllocateInfo allocate_info{
        .s_type = vulkan_structure_type_memory_allocate_info,
        .next = nullptr,
        .allocation_size = requirements.size,
        .memory_type_index = memory_type_index,
    };
    NativeVulkanDeviceMemory memory = 0;
    const auto allocate_result = device.impl_->allocate_memory(device.impl_->device, &allocate_info, nullptr, &memory);
    if (allocate_result != vulkan_success || memory == 0) {
        device.impl_->destroy_image(device.impl_->device, image, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkAllocateMemory failed", allocate_result);
        return result;
    }

    const auto bind_result = device.impl_->bind_image_memory(device.impl_->device, image, memory, 0);
    if (bind_result != vulkan_success) {
        device.impl_->free_memory(device.impl_->device, memory, nullptr);
        device.impl_->destroy_image(device.impl_->device, image, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkBindImageMemory failed", bind_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_image, image,
                                "GameEngine.RHI.Vulkan.TextureImage");

    NativeVulkanImageView image_view = 0;
    if (needs_image_view) {
        const NativeVulkanImageViewCreateInfo view_info{
            .s_type = vulkan_structure_type_image_view_create_info,
            .next = nullptr,
            .flags = 0,
            .image = image,
            .view_type = vulkan_image_view_type_2d,
            .format = native_vulkan_format(result.plan.format),
            .components =
                NativeVulkanComponentMapping{
                    .r = vulkan_component_swizzle_identity,
                    .g = vulkan_component_swizzle_identity,
                    .b = vulkan_component_swizzle_identity,
                    .a = vulkan_component_swizzle_identity,
                },
            .subresource_range =
                NativeVulkanImageSubresourceRange{.aspect_mask = native_vulkan_image_aspect_flags(result.plan.format),
                                                  .base_mip_level = 0,
                                                  .level_count = 1,
                                                  .base_array_layer = 0,
                                                  .layer_count = 1},
        };
        const auto view_result =
            device.impl_->create_image_view(device.impl_->device, &view_info, nullptr, &image_view);
        if (view_result != vulkan_success || image_view == 0) {
            device.impl_->free_memory(device.impl_->device, memory, nullptr);
            device.impl_->destroy_image(device.impl_->device, image, nullptr);
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateImageView failed", view_result);
            return result;
        }
        vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_image_view, image_view,
                                    "GameEngine.RHI.Vulkan.TextureImageView");
    }

    auto impl = std::make_unique<VulkanRuntimeTexture::Impl>();
    impl->device_owner = device.impl_;
    impl->image = image;
    impl->image_view = image_view;
    impl->memory = memory;
    impl->desc = desc.texture;
    result.texture = VulkanRuntimeTexture{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime texture owner ready";
    return result;
}

VulkanRuntimeSamplerCreateResult create_runtime_sampler(VulkanRuntimeDevice& device,
                                                        const VulkanRuntimeSamplerDesc& desc) {
    VulkanRuntimeSamplerCreateResult result;
    if (!valid_sampler_desc(desc.sampler)) {
        result.diagnostic = "Vulkan sampler description is invalid";
        return result;
    }
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (device.impl_->create_sampler == nullptr || device.impl_->destroy_sampler == nullptr) {
        result.diagnostic = "Vulkan sampler commands are unavailable";
        return result;
    }

    const NativeVulkanSamplerCreateInfo create_info{
        .s_type = vulkan_structure_type_sampler_create_info,
        .next = nullptr,
        .flags = 0,
        .mag_filter = native_vulkan_filter(desc.sampler.mag_filter),
        .min_filter = native_vulkan_filter(desc.sampler.min_filter),
        .mipmap_mode = vulkan_sampler_mipmap_mode_nearest,
        .address_mode_u = native_vulkan_address_mode(desc.sampler.address_u),
        .address_mode_v = native_vulkan_address_mode(desc.sampler.address_v),
        .address_mode_w = native_vulkan_address_mode(desc.sampler.address_w),
        .mip_lod_bias = 0.0F,
        .anisotropy_enable = 0,
        .max_anisotropy = 1.0F,
        .compare_enable = 0,
        .compare_op = vulkan_compare_op_never,
        .min_lod = 0.0F,
        .max_lod = 0.0F,
        .border_color = vulkan_border_color_float_transparent_black,
        .unnormalized_coordinates = 0,
    };

    NativeVulkanSampler sampler = 0;
    const auto create_result = device.impl_->create_sampler(device.impl_->device, &create_info, nullptr, &sampler);
    if (create_result != vulkan_success || sampler == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateSampler failed", create_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_sampler, sampler,
                                "GameEngine.RHI.Vulkan.Sampler");

    auto impl = std::make_unique<VulkanRuntimeSampler::Impl>();
    impl->device_owner = device.impl_;
    impl->sampler = sampler;
    result.sampler = VulkanRuntimeSampler{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime sampler owner ready";
    return result;
}

VulkanRuntimeReadbackBufferCreateResult create_runtime_readback_buffer(VulkanRuntimeDevice& device,
                                                                       const VulkanRuntimeReadbackBufferDesc& desc) {
    VulkanRuntimeReadbackBufferCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (desc.byte_size == 0) {
        result.diagnostic = "Vulkan readback buffer size is required";
        return result;
    }
    if (device.impl_->create_buffer == nullptr || device.impl_->destroy_buffer == nullptr ||
        device.impl_->get_buffer_memory_requirements == nullptr || device.impl_->allocate_memory == nullptr ||
        device.impl_->free_memory == nullptr || device.impl_->bind_buffer_memory == nullptr ||
        device.impl_->get_physical_device_memory_properties == nullptr) {
        result.diagnostic = "Vulkan readback buffer commands are unavailable";
        return result;
    }

    const NativeVulkanBufferCreateInfo buffer_create_info{
        .s_type = vulkan_structure_type_buffer_create_info,
        .next = nullptr,
        .flags = 0,
        .size = desc.byte_size,
        .usage = vulkan_buffer_usage_transfer_dst_bit,
        .sharing_mode = vulkan_sharing_mode_exclusive,
        .queue_family_index_count = 0,
        .queue_family_indices = nullptr,
    };
    NativeVulkanBuffer buffer = 0;
    const auto buffer_result = device.impl_->create_buffer(device.impl_->device, &buffer_create_info, nullptr, &buffer);
    if (buffer_result != vulkan_success || buffer == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateBuffer failed", buffer_result);
        return result;
    }

    NativeVulkanMemoryRequirements requirements{};
    device.impl_->get_buffer_memory_requirements(device.impl_->device, buffer, &requirements);
    if (requirements.size == 0 || requirements.memory_type_bits == 0) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = "Vulkan readback buffer memory requirements are unavailable";
        return result;
    }

    NativeVulkanPhysicalDeviceMemoryProperties memory_properties{};
    device.impl_->get_physical_device_memory_properties(device.impl_->physical_device, &memory_properties);
    const auto required_memory_flags =
        vulkan_memory_property_host_visible_bit | vulkan_memory_property_host_coherent_bit;
    const auto memory_type_index =
        find_memory_type_index(memory_properties, requirements.memory_type_bits, required_memory_flags);
    if (memory_type_index == invalid_vulkan_queue_family) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = "Vulkan host-visible coherent readback memory type is unavailable";
        return result;
    }

    const NativeVulkanMemoryAllocateInfo allocate_info{
        .s_type = vulkan_structure_type_memory_allocate_info,
        .next = nullptr,
        .allocation_size = requirements.size,
        .memory_type_index = memory_type_index,
    };
    NativeVulkanDeviceMemory memory = 0;
    const auto allocate_result = device.impl_->allocate_memory(device.impl_->device, &allocate_info, nullptr, &memory);
    if (allocate_result != vulkan_success || memory == 0) {
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkAllocateMemory failed", allocate_result);
        return result;
    }

    const auto bind_result = device.impl_->bind_buffer_memory(device.impl_->device, buffer, memory, 0);
    if (bind_result != vulkan_success) {
        device.impl_->free_memory(device.impl_->device, memory, nullptr);
        device.impl_->destroy_buffer(device.impl_->device, buffer, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkBindBufferMemory failed", bind_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_buffer, buffer,
                                "GameEngine.RHI.Vulkan.ReadbackBuffer");

    auto impl = std::make_unique<VulkanRuntimeReadbackBuffer::Impl>();
    impl->device_owner = device.impl_;
    impl->buffer = buffer;
    impl->memory = memory;
    impl->byte_size = desc.byte_size;
    result.buffer = VulkanRuntimeReadbackBuffer{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime readback buffer owner ready";
    return result;
}

VulkanRuntimeShaderModuleCreateResult create_runtime_shader_module(VulkanRuntimeDevice& device,
                                                                   const VulkanRuntimeShaderModuleDesc& desc) {
    VulkanRuntimeShaderModuleCreateResult result;
    result.validation = validate_spirv_shader_artifact(VulkanSpirvShaderArtifactDesc{
        .stage = desc.stage,
        .bytecode = desc.bytecode,
        .bytecode_size = desc.bytecode_size,
    });
    if (!result.validation.valid) {
        result.diagnostic = result.validation.diagnostic;
        return result;
    }

    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (device.impl_->create_shader_module == nullptr || device.impl_->destroy_shader_module == nullptr) {
        result.diagnostic = "Vulkan shader module commands are unavailable";
        return result;
    }

    const NativeVulkanShaderModuleCreateInfo create_info{
        .s_type = vulkan_structure_type_shader_module_create_info,
        .next = nullptr,
        .flags = 0,
        .code_size = static_cast<std::size_t>(desc.bytecode_size),
        .code = static_cast<const std::uint32_t*>(desc.bytecode),
    };

    NativeVulkanShaderModule module = 0;
    const auto create_result = device.impl_->create_shader_module(device.impl_->device, &create_info, nullptr, &module);
    if (create_result != vulkan_success || module == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateShaderModule failed", create_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_shader_module, module,
                                "GameEngine.RHI.Vulkan.ShaderModule");

    auto impl = std::make_unique<VulkanRuntimeShaderModule::Impl>();
    impl->device_owner = device.impl_;
    impl->module = module;
    impl->stage = desc.stage;
    impl->bytecode_size = desc.bytecode_size;
    result.module = VulkanRuntimeShaderModule{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime shader module owner ready";
    return result;
}

VulkanRuntimeDescriptorSetLayoutCreateResult
create_runtime_descriptor_set_layout(VulkanRuntimeDevice& device, const VulkanRuntimeDescriptorSetLayoutDesc& desc) {
    VulkanRuntimeDescriptorSetLayoutCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    for (const auto& binding : desc.layout.bindings) {
        if (binding.count == 0) {
            result.diagnostic = "Vulkan descriptor binding count is required";
            return result;
        }
        if (!has_stage_visibility(binding.stages)) {
            result.diagnostic = "Vulkan descriptor binding shader stages are required";
            return result;
        }
    }
    if (descriptor_bindings_have_duplicate_numbers(desc.layout)) {
        result.diagnostic = "Vulkan descriptor binding indices must be unique";
        return result;
    }
    if (device.impl_->create_descriptor_set_layout == nullptr ||
        device.impl_->destroy_descriptor_set_layout == nullptr) {
        result.diagnostic = "Vulkan descriptor set layout commands are unavailable";
        return result;
    }

    std::vector<NativeVulkanDescriptorSetLayoutBinding> native_bindings;
    native_bindings.reserve(desc.layout.bindings.size());
    for (const auto& binding : desc.layout.bindings) {
        native_bindings.push_back(NativeVulkanDescriptorSetLayoutBinding{
            .binding = binding.binding,
            .descriptor_type = native_vulkan_descriptor_type(binding.type),
            .descriptor_count = binding.count,
            .stage_flags = native_vulkan_shader_stage_flags(binding.stages),
            .immutable_samplers = nullptr,
        });
    }

    const NativeVulkanDescriptorSetLayoutCreateInfo create_info{
        .s_type = vulkan_structure_type_descriptor_set_layout_create_info,
        .next = nullptr,
        .flags = 0,
        .binding_count = static_cast<std::uint32_t>(native_bindings.size()),
        .bindings = native_bindings.empty() ? nullptr : native_bindings.data(),
    };
    NativeVulkanDescriptorSetLayout layout = 0;
    const auto create_result =
        device.impl_->create_descriptor_set_layout(device.impl_->device, &create_info, nullptr, &layout);
    if (create_result != vulkan_success || layout == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateDescriptorSetLayout failed", create_result);
        return result;
    }

    auto impl = std::make_unique<VulkanRuntimeDescriptorSetLayout::Impl>();
    impl->device_owner = device.impl_;
    impl->layout = layout;
    impl->desc = desc.layout;
    result.layout = VulkanRuntimeDescriptorSetLayout{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime descriptor set layout owner ready";
    return result;
}

VulkanRuntimeDescriptorSetCreateResult create_runtime_descriptor_set(VulkanRuntimeDevice& device,
                                                                     VulkanRuntimeDescriptorSetLayout& layout,
                                                                     const VulkanRuntimeDescriptorSetDesc& desc) {
    VulkanRuntimeDescriptorSetCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!layout.owns_layout()) {
        result.diagnostic = "Vulkan runtime descriptor set layout is required";
        return result;
    }
    if (layout.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan descriptor objects must share one runtime device";
        return result;
    }
    if (desc.max_sets == 0) {
        result.diagnostic = "Vulkan descriptor set pool max sets is required";
        return result;
    }
    if (device.impl_->create_descriptor_pool == nullptr || device.impl_->destroy_descriptor_pool == nullptr ||
        device.impl_->allocate_descriptor_sets == nullptr) {
        result.diagnostic = "Vulkan descriptor set allocation commands are unavailable";
        return result;
    }

    std::vector<NativeVulkanDescriptorPoolSize> pool_sizes;
    for (const auto& binding : layout.impl_->desc.bindings) {
        if (binding.count > std::numeric_limits<std::uint32_t>::max() / desc.max_sets) {
            result.diagnostic = "Vulkan descriptor pool size is too large";
            return result;
        }

        const auto native_type = native_vulkan_descriptor_type(binding.type);
        const auto descriptor_count = binding.count * desc.max_sets;
        auto existing =
            std::ranges::find_if(pool_sizes, [native_type](const auto& size) { return size.type == native_type; });
        if (existing == pool_sizes.end()) {
            pool_sizes.push_back(
                NativeVulkanDescriptorPoolSize{.type = native_type, .descriptor_count = descriptor_count});
        } else {
            if (existing->descriptor_count > std::numeric_limits<std::uint32_t>::max() - descriptor_count) {
                result.diagnostic = "Vulkan descriptor pool size is too large";
                return result;
            }
            existing->descriptor_count += descriptor_count;
        }
    }

    const NativeVulkanDescriptorPoolCreateInfo pool_create_info{
        .s_type = vulkan_structure_type_descriptor_pool_create_info,
        .next = nullptr,
        .flags = 0,
        .max_sets = desc.max_sets,
        .pool_size_count = static_cast<std::uint32_t>(pool_sizes.size()),
        .pool_sizes = pool_sizes.empty() ? nullptr : pool_sizes.data(),
    };
    NativeVulkanDescriptorPool pool = 0;
    const auto pool_create_result =
        device.impl_->create_descriptor_pool(device.impl_->device, &pool_create_info, nullptr, &pool);
    if (pool_create_result != vulkan_success || pool == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateDescriptorPool failed", pool_create_result);
        return result;
    }

    const NativeVulkanDescriptorSetLayout native_layout = layout.impl_->layout;
    const NativeVulkanDescriptorSetAllocateInfo allocate_info{
        .s_type = vulkan_structure_type_descriptor_set_allocate_info,
        .next = nullptr,
        .descriptor_pool = pool,
        .descriptor_set_count = 1,
        .set_layouts = &native_layout,
    };
    NativeVulkanDescriptorSet set = 0;
    const auto allocate_result = device.impl_->allocate_descriptor_sets(device.impl_->device, &allocate_info, &set);
    if (allocate_result != vulkan_success || set == 0) {
        device.impl_->destroy_descriptor_pool(device.impl_->device, pool, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkAllocateDescriptorSets failed", allocate_result);
        return result;
    }

    auto impl = std::make_unique<VulkanRuntimeDescriptorSet::Impl>();
    impl->device_owner = device.impl_;
    impl->pool = pool;
    impl->set = set;
    impl->layout = layout.impl_->layout;
    impl->layout_desc = layout.impl_->desc;
    impl->max_sets = desc.max_sets;
    result.set = VulkanRuntimeDescriptorSet{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime descriptor set owner ready";
    return result;
}

VulkanRuntimeDescriptorWriteResult update_runtime_descriptor_set(VulkanRuntimeDevice& device,
                                                                 VulkanRuntimeDescriptorSet& descriptor_set,
                                                                 const VulkanRuntimeDescriptorWriteDesc& desc) {
    VulkanRuntimeDescriptorWriteResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!descriptor_set.owns_set()) {
        result.diagnostic = "Vulkan runtime descriptor set is required";
        return result;
    }
    if (descriptor_set.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan descriptor objects must share one runtime device";
        return result;
    }
    const auto requested_resource_vectors =
        (desc.buffers.empty() ? 0U : 1U) + (desc.textures.empty() ? 0U : 1U) + (desc.samplers.empty() ? 0U : 1U);
    if (requested_resource_vectors != 1U) {
        result.diagnostic = "Vulkan descriptor write requires exactly one resource kind";
        return result;
    }

    const auto resource_count_size = !desc.buffers.empty()    ? desc.buffers.size()
                                     : !desc.textures.empty() ? desc.textures.size()
                                                              : desc.samplers.size();
    if (resource_count_size > std::numeric_limits<std::uint32_t>::max()) {
        result.diagnostic = "Vulkan descriptor write resource count is too large";
        return result;
    }

    const auto* binding = find_descriptor_binding(descriptor_set.impl_->layout_desc, desc.binding);
    if (binding == nullptr) {
        result.diagnostic = "Vulkan descriptor binding is not declared by the set layout";
        return result;
    }

    const auto descriptor_count = static_cast<std::uint32_t>(resource_count_size);
    if (desc.array_element >= binding->count || descriptor_count > binding->count - desc.array_element) {
        result.diagnostic = "Vulkan descriptor write exceeds the declared binding range";
        return result;
    }
    if (device.impl_->update_descriptor_sets == nullptr) {
        result.diagnostic = "Vulkan descriptor set update command is unavailable";
        return result;
    }

    std::vector<NativeVulkanDescriptorBufferInfo> buffer_infos;
    buffer_infos.reserve(desc.buffers.size());
    std::vector<NativeVulkanDescriptorImageInfo> image_infos;
    image_infos.reserve(std::max(desc.textures.size(), desc.samplers.size()));

    if (binding->type == DescriptorType::uniform_buffer || binding->type == DescriptorType::storage_buffer) {
        if (desc.buffers.empty()) {
            result.diagnostic = "Vulkan descriptor buffer resources are required";
            return result;
        }
        for (const auto& resource : desc.buffers) {
            if (resource.type != binding->type) {
                result.diagnostic = "Vulkan descriptor resource type must match the binding type";
                return result;
            }
            if (resource.buffer == nullptr || !resource.buffer->owns_buffer() || !resource.buffer->owns_memory()) {
                result.diagnostic = "Vulkan descriptor buffer resource is required";
                return result;
            }
            if (resource.buffer->impl_->device_owner != device.impl_) {
                result.diagnostic = "Vulkan descriptor buffer must share the runtime device";
                return result;
            }
            const auto usage = resource.buffer->usage();
            if (binding->type == DescriptorType::uniform_buffer && !has_flag(usage, BufferUsage::uniform)) {
                result.diagnostic = "Vulkan uniform buffer descriptor requires uniform buffer usage";
                return result;
            }
            if (binding->type == DescriptorType::storage_buffer && !has_flag(usage, BufferUsage::storage)) {
                result.diagnostic = "Vulkan storage buffer descriptor requires storage buffer usage";
                return result;
            }
            buffer_infos.push_back(NativeVulkanDescriptorBufferInfo{
                .buffer = resource.buffer->impl_->buffer,
                .offset = 0,
                .range = resource.buffer->byte_size(),
            });
        }
    } else if (binding->type == DescriptorType::sampled_texture || binding->type == DescriptorType::storage_texture) {
        if (desc.textures.empty()) {
            result.diagnostic = "Vulkan descriptor texture resources are required";
            return result;
        }
        for (const auto& resource : desc.textures) {
            if (resource.type != binding->type) {
                result.diagnostic = "Vulkan descriptor resource type must match the binding type";
                return result;
            }
            if (resource.texture == nullptr || !resource.texture->owns_image() ||
                resource.texture->impl_->image_view == 0) {
                result.diagnostic = "Vulkan descriptor texture image view is required";
                return result;
            }
            if (resource.texture->impl_->device_owner != device.impl_) {
                result.diagnostic = "Vulkan descriptor texture must share the runtime device";
                return result;
            }
            const auto usage = resource.texture->usage();
            if (binding->type == DescriptorType::sampled_texture && !has_flag(usage, TextureUsage::shader_resource)) {
                result.diagnostic = "Vulkan sampled image descriptor requires shader_resource texture usage";
                return result;
            }
            if (binding->type == DescriptorType::storage_texture && !has_flag(usage, TextureUsage::storage)) {
                result.diagnostic = "Vulkan storage image descriptor requires storage texture usage";
                return result;
            }
            image_infos.push_back(NativeVulkanDescriptorImageInfo{
                .sampler = 0,
                .image_view = resource.texture->impl_->image_view,
                .image_layout = binding->type == DescriptorType::sampled_texture
                                    ? vulkan_image_layout_shader_read_only_optimal
                                    : vulkan_image_layout_general,
            });
        }
    } else if (binding->type == DescriptorType::sampler) {
        if (desc.samplers.empty()) {
            result.diagnostic = "Vulkan descriptor sampler resources are required";
            return result;
        }
        for (const auto& resource : desc.samplers) {
            if (resource.sampler == nullptr || !resource.sampler->owns_sampler()) {
                result.diagnostic = "Vulkan descriptor sampler resource is required";
                return result;
            }
            if (resource.sampler->impl_->device_owner != device.impl_) {
                result.diagnostic = "Vulkan descriptor sampler must share the runtime device";
                return result;
            }
            image_infos.push_back(NativeVulkanDescriptorImageInfo{
                .sampler = resource.sampler->impl_->sampler,
                .image_view = 0,
                .image_layout = vulkan_image_layout_undefined,
            });
        }
    } else {
        result.diagnostic = "Vulkan descriptor type is unsupported";
        return result;
    }

    const NativeVulkanWriteDescriptorSet write{
        .s_type = vulkan_structure_type_write_descriptor_set,
        .next = nullptr,
        .dst_set = descriptor_set.impl_->set,
        .dst_binding = desc.binding,
        .dst_array_element = desc.array_element,
        .descriptor_count = descriptor_count,
        .descriptor_type = native_vulkan_descriptor_type(binding->type),
        .image_info = image_infos.empty() ? nullptr : image_infos.data(),
        .buffer_info = buffer_infos.empty() ? nullptr : buffer_infos.data(),
        .texel_buffer_view = nullptr,
    };
    device.impl_->update_descriptor_sets(device.impl_->device, 1, &write, 0, nullptr);
    result.updated = true;
    result.descriptor_count = descriptor_count;
    result.diagnostic = "Vulkan runtime descriptor set updated";
    return result;
}

VulkanRuntimePipelineLayoutCreateResult create_runtime_pipeline_layout(VulkanRuntimeDevice& device,
                                                                       const VulkanRuntimePipelineLayoutDesc& desc) {
    VulkanRuntimePipelineLayoutCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (desc.descriptor_set_layouts.size() > std::numeric_limits<std::uint32_t>::max()) {
        result.diagnostic = "Vulkan pipeline layout descriptor set count is too large";
        return result;
    }
    const auto descriptor_set_layout_count = static_cast<std::uint32_t>(desc.descriptor_set_layouts.size());
    if (desc.push_constant_bytes != 0) {
        result.diagnostic = "Vulkan runtime pipeline layout push constants are not implemented";
        return result;
    }
    if (device.impl_->create_pipeline_layout == nullptr || device.impl_->destroy_pipeline_layout == nullptr) {
        result.diagnostic = "Vulkan pipeline layout commands are unavailable";
        return result;
    }

    std::vector<NativeVulkanDescriptorSetLayout> native_set_layouts;
    native_set_layouts.reserve(desc.descriptor_set_layouts.size());
    for (const auto* set_layout : desc.descriptor_set_layouts) {
        if (set_layout == nullptr || !set_layout->owns_layout()) {
            result.diagnostic = "Vulkan runtime descriptor set layout is required";
            return result;
        }
        if (set_layout->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan pipeline layout objects must share one runtime device";
            return result;
        }
        native_set_layouts.push_back(set_layout->impl_->layout);
    }

    const NativeVulkanPipelineLayoutCreateInfo create_info{
        .s_type = vulkan_structure_type_pipeline_layout_create_info,
        .next = nullptr,
        .flags = 0,
        .set_layout_count = descriptor_set_layout_count,
        .set_layouts = native_set_layouts.empty() ? nullptr : native_set_layouts.data(),
        .push_constant_range_count = 0,
        .push_constant_ranges = nullptr,
    };
    NativeVulkanPipelineLayout layout = 0;
    const auto create_result =
        device.impl_->create_pipeline_layout(device.impl_->device, &create_info, nullptr, &layout);
    if (create_result != vulkan_success || layout == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreatePipelineLayout failed", create_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_pipeline_layout, layout,
                                "GameEngine.RHI.Vulkan.PipelineLayout");

    auto impl = std::make_unique<VulkanRuntimePipelineLayout::Impl>();
    impl->device_owner = device.impl_;
    impl->layout = layout;
    impl->descriptor_set_layouts = std::move(native_set_layouts);
    impl->descriptor_set_layout_count = static_cast<std::uint32_t>(impl->descriptor_set_layouts.size());
    impl->push_constant_bytes = desc.push_constant_bytes;
    result.layout = VulkanRuntimePipelineLayout{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime pipeline layout owner ready";
    return result;
}

VulkanRuntimeGraphicsPipelineCreateResult
create_runtime_graphics_pipeline(VulkanRuntimeDevice& device, VulkanRuntimePipelineLayout& layout,
                                 VulkanRuntimeShaderModule& vertex_shader, VulkanRuntimeShaderModule& fragment_shader,
                                 const VulkanRuntimeGraphicsPipelineDesc& desc) {
    VulkanRuntimeGraphicsPipelineCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!layout.owns_layout()) {
        result.diagnostic = "Vulkan runtime pipeline layout is required";
        return result;
    }
    if (!vertex_shader.owns_module() || vertex_shader.stage() != ShaderStage::vertex) {
        result.diagnostic = "Vulkan vertex shader module is required";
        return result;
    }
    if (!fragment_shader.owns_module() || fragment_shader.stage() != ShaderStage::fragment) {
        result.diagnostic = "Vulkan fragment shader module is required";
        return result;
    }
    if (layout.impl_->device_owner != device.impl_ || vertex_shader.impl_->device_owner != device.impl_ ||
        fragment_shader.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan graphics pipeline objects must share one runtime device";
        return result;
    }
    if (!desc.dynamic_rendering.supported) {
        result.diagnostic = "Vulkan dynamic rendering plan is required";
        return result;
    }
    if (!dynamic_rendering_color_format_supported(desc.color_format)) {
        result.diagnostic = "Vulkan graphics pipeline color format is unsupported";
        return result;
    }
    if (std::ranges::find(desc.dynamic_rendering.color_formats, desc.color_format) ==
        desc.dynamic_rendering.color_formats.end()) {
        result.diagnostic = "Vulkan graphics pipeline color format must match dynamic rendering plan";
        return result;
    }
    if (!valid_depth_state_for_format(desc.depth_format, desc.depth_state)) {
        result.diagnostic = "Vulkan graphics pipeline depth state is invalid or unsupported";
        return result;
    }
    if (desc.depth_format != Format::unknown) {
        if (!dynamic_rendering_depth_format_supported(desc.depth_format)) {
            result.diagnostic = "Vulkan graphics pipeline depth format is unsupported";
            return result;
        }
        if (!desc.dynamic_rendering.depth_attachment_enabled ||
            desc.dynamic_rendering.depth_format != desc.depth_format) {
            result.diagnostic = "Vulkan graphics pipeline depth format must match dynamic rendering plan";
            return result;
        }
    }
    if (desc.vertex_entry_point.empty() || desc.fragment_entry_point.empty()) {
        result.diagnostic = "Vulkan graphics pipeline shader entry points are required";
        return result;
    }
    if (!valid_vertex_input_desc(desc.vertex_buffers, desc.vertex_attributes)) {
        result.diagnostic = "Vulkan graphics pipeline vertex input description is invalid";
        return result;
    }
    if (device.impl_->create_graphics_pipelines == nullptr || device.impl_->destroy_pipeline == nullptr) {
        result.diagnostic = "Vulkan graphics pipeline commands are unavailable";
        return result;
    }

    const std::string vertex_entry{desc.vertex_entry_point};
    const std::string fragment_entry{desc.fragment_entry_point};
    const std::array<NativeVulkanPipelineShaderStageCreateInfo, 2> shader_stages{{
        {
            .s_type = vulkan_structure_type_pipeline_shader_stage_create_info,
            .next = nullptr,
            .flags = 0,
            .stage = vulkan_shader_stage_vertex_bit,
            .module = vertex_shader.impl_->module,
            .name = vertex_entry.c_str(),
            .specialization_info = nullptr,
        },
        {
            .s_type = vulkan_structure_type_pipeline_shader_stage_create_info,
            .next = nullptr,
            .flags = 0,
            .stage = vulkan_shader_stage_fragment_bit,
            .module = fragment_shader.impl_->module,
            .name = fragment_entry.c_str(),
            .specialization_info = nullptr,
        },
    }};
    std::vector<NativeVulkanVertexInputBindingDescription> vertex_bindings;
    vertex_bindings.reserve(desc.vertex_buffers.size());
    for (const auto& buffer_layout : desc.vertex_buffers) {
        vertex_bindings.push_back(NativeVulkanVertexInputBindingDescription{
            .binding = buffer_layout.binding,
            .stride = buffer_layout.stride,
            .input_rate = native_vulkan_input_rate(buffer_layout.input_rate),
        });
    }

    std::vector<NativeVulkanVertexInputAttributeDescription> vertex_attributes;
    vertex_attributes.reserve(desc.vertex_attributes.size());
    for (const auto& attribute : desc.vertex_attributes) {
        vertex_attributes.push_back(NativeVulkanVertexInputAttributeDescription{
            .location = attribute.location,
            .binding = attribute.binding,
            .format = native_vulkan_vertex_format(attribute.format),
            .offset = attribute.offset,
        });
    }

    const NativeVulkanPipelineVertexInputStateCreateInfo vertex_input{
        .s_type = vulkan_structure_type_pipeline_vertex_input_state_create_info,
        .next = nullptr,
        .flags = 0,
        .vertex_binding_description_count = static_cast<std::uint32_t>(vertex_bindings.size()),
        .vertex_binding_descriptions = vertex_bindings.empty() ? nullptr : vertex_bindings.data(),
        .vertex_attribute_description_count = static_cast<std::uint32_t>(vertex_attributes.size()),
        .vertex_attribute_descriptions = vertex_attributes.empty() ? nullptr : vertex_attributes.data(),
    };
    const NativeVulkanPipelineInputAssemblyStateCreateInfo input_assembly{
        .s_type = vulkan_structure_type_pipeline_input_assembly_state_create_info,
        .next = nullptr,
        .flags = 0,
        .topology = native_vulkan_topology(desc.topology),
        .primitive_restart_enable = 0,
    };
    const NativeVulkanViewport viewport{
        .x = 0.0F,
        .y = 0.0F,
        .width = 1.0F,
        .height = 1.0F,
        .min_depth = 0.0F,
        .max_depth = 1.0F,
    };
    const NativeVulkanRect2D scissor{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = 1, .height = 1},
    };
    const NativeVulkanPipelineViewportStateCreateInfo viewport_state{
        .s_type = vulkan_structure_type_pipeline_viewport_state_create_info,
        .next = nullptr,
        .flags = 0,
        .viewport_count = 1,
        .viewports = &viewport,
        .scissor_count = 1,
        .scissors = &scissor,
    };
    const NativeVulkanPipelineRasterizationStateCreateInfo rasterization{
        .s_type = vulkan_structure_type_pipeline_rasterization_state_create_info,
        .next = nullptr,
        .flags = 0,
        .depth_clamp_enable = 0,
        .rasterizer_discard_enable = 0,
        .polygon_mode = vulkan_polygon_mode_fill,
        .cull_mode = vulkan_cull_mode_none,
        .front_face = vulkan_front_face_counter_clockwise,
        .depth_bias_enable = 0,
        .depth_bias_constant_factor = 0.0F,
        .depth_bias_clamp = 0.0F,
        .depth_bias_slope_factor = 0.0F,
        .line_width = 1.0F,
    };
    const NativeVulkanPipelineMultisampleStateCreateInfo multisample{
        .s_type = vulkan_structure_type_pipeline_multisample_state_create_info,
        .next = nullptr,
        .flags = 0,
        .rasterization_samples = vulkan_sample_count_1_bit,
        .sample_shading_enable = 0,
        .min_sample_shading = 1.0F,
        .sample_mask = nullptr,
        .alpha_to_coverage_enable = 0,
        .alpha_to_one_enable = 0,
    };
    const NativeVulkanStencilOpState stencil{};
    const NativeVulkanPipelineDepthStencilStateCreateInfo depth_stencil{
        .s_type = vulkan_structure_type_pipeline_depth_stencil_state_create_info,
        .next = nullptr,
        .flags = 0,
        .depth_test_enable = desc.depth_state.depth_test_enabled ? 1U : 0U,
        .depth_write_enable = desc.depth_state.depth_write_enabled ? 1U : 0U,
        .depth_compare_op = native_vulkan_compare_op(desc.depth_state.depth_compare),
        .depth_bounds_test_enable = 0,
        .stencil_test_enable = 0,
        .front = stencil,
        .back = stencil,
        .min_depth_bounds = 0.0F,
        .max_depth_bounds = 1.0F,
    };
    const NativeVulkanPipelineColorBlendAttachmentState color_blend_attachment{
        .blend_enable = 0,
        .src_color_blend_factor = 0,
        .dst_color_blend_factor = 0,
        .color_blend_op = 0,
        .src_alpha_blend_factor = 0,
        .dst_alpha_blend_factor = 0,
        .alpha_blend_op = 0,
        .color_write_mask = vulkan_color_component_r_bit | vulkan_color_component_g_bit | vulkan_color_component_b_bit |
                            vulkan_color_component_a_bit,
    };
    const NativeVulkanPipelineColorBlendStateCreateInfo color_blend{
        .s_type = vulkan_structure_type_pipeline_color_blend_state_create_info,
        .next = nullptr,
        .flags = 0,
        .logic_op_enable = 0,
        .logic_op = 0,
        .attachment_count = 1,
        .attachments = &color_blend_attachment,
        .blend_constants = {0.0F, 0.0F, 0.0F, 0.0F},
    };
    const std::array<std::uint32_t, 2> dynamic_states{
        vulkan_dynamic_state_viewport,
        vulkan_dynamic_state_scissor,
    };
    const NativeVulkanPipelineDynamicStateCreateInfo dynamic_state{
        .s_type = vulkan_structure_type_pipeline_dynamic_state_create_info,
        .next = nullptr,
        .flags = 0,
        .dynamic_state_count = 2,
        .dynamic_states = dynamic_states.data(),
    };
    const std::uint32_t color_format = native_vulkan_format(desc.color_format);
    const NativeVulkanPipelineRenderingCreateInfo rendering{
        .s_type = vulkan_structure_type_pipeline_rendering_create_info,
        .next = nullptr,
        .view_mask = 0,
        .color_attachment_count = 1,
        .color_attachment_formats = &color_format,
        .depth_attachment_format = native_vulkan_format(desc.depth_format),
        .stencil_attachment_format = 0,
    };
    const NativeVulkanGraphicsPipelineCreateInfo create_info{
        .s_type = vulkan_structure_type_graphics_pipeline_create_info,
        .next = &rendering,
        .flags = 0,
        .stage_count = 2,
        .stages = shader_stages.data(),
        .vertex_input_state = &vertex_input,
        .input_assembly_state = &input_assembly,
        .tessellation_state = nullptr,
        .viewport_state = &viewport_state,
        .rasterization_state = &rasterization,
        .multisample_state = &multisample,
        .depth_stencil_state = desc.depth_format != Format::unknown ? &depth_stencil : nullptr,
        .color_blend_state = &color_blend,
        .dynamic_state = &dynamic_state,
        .layout = layout.impl_->layout,
        .render_pass = 0,
        .subpass = 0,
        .base_pipeline_handle = 0,
        .base_pipeline_index = -1,
    };

    NativeVulkanPipeline pipeline = 0;
    const auto create_result =
        device.impl_->create_graphics_pipelines(device.impl_->device, 0, 1, &create_info, nullptr, &pipeline);
    if (create_result != vulkan_success || pipeline == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateGraphicsPipelines failed", create_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_pipeline, pipeline,
                                "GameEngine.RHI.Vulkan.GraphicsPipeline");

    auto impl = std::make_unique<VulkanRuntimeGraphicsPipeline::Impl>();
    impl->device_owner = device.impl_;
    impl->pipeline = pipeline;
    impl->color_format = desc.color_format;
    impl->depth_format = desc.depth_format;
    impl->topology = desc.topology;
    result.pipeline = VulkanRuntimeGraphicsPipeline{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime graphics pipeline owner ready";
    return result;
}

VulkanRuntimeComputePipelineCreateResult create_runtime_compute_pipeline(VulkanRuntimeDevice& device,
                                                                         VulkanRuntimePipelineLayout& layout,
                                                                         VulkanRuntimeShaderModule& compute_shader,
                                                                         const VulkanRuntimeComputePipelineDesc& desc) {
    VulkanRuntimeComputePipelineCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!layout.owns_layout()) {
        result.diagnostic = "Vulkan runtime pipeline layout is required";
        return result;
    }
    if (!compute_shader.owns_module() || compute_shader.stage() != ShaderStage::compute) {
        result.diagnostic = "Vulkan compute shader module is required";
        return result;
    }
    if (layout.impl_->device_owner != device.impl_ || compute_shader.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan compute pipeline objects must share one runtime device";
        return result;
    }
    if (desc.entry_point.empty()) {
        result.diagnostic = "Vulkan compute pipeline shader entry point is required";
        return result;
    }
    if (device.impl_->create_compute_pipelines == nullptr || device.impl_->destroy_pipeline == nullptr) {
        result.diagnostic = "Vulkan compute pipeline commands are unavailable";
        return result;
    }

    const std::string entry_point{desc.entry_point};
    const NativeVulkanPipelineShaderStageCreateInfo stage{
        .s_type = vulkan_structure_type_pipeline_shader_stage_create_info,
        .next = nullptr,
        .flags = 0,
        .stage = vulkan_shader_stage_compute_bit,
        .module = compute_shader.impl_->module,
        .name = entry_point.c_str(),
        .specialization_info = nullptr,
    };
    const NativeVulkanComputePipelineCreateInfo create_info{
        .s_type = vulkan_structure_type_compute_pipeline_create_info,
        .next = nullptr,
        .flags = 0,
        .stage = stage,
        .layout = layout.impl_->layout,
        .base_pipeline_handle = 0,
        .base_pipeline_index = -1,
    };

    NativeVulkanPipeline pipeline = 0;
    const auto create_result =
        device.impl_->create_compute_pipelines(device.impl_->device, 0, 1, &create_info, nullptr, &pipeline);
    if (create_result != vulkan_success || pipeline == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateComputePipelines failed", create_result);
        return result;
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_pipeline, pipeline,
                                "GameEngine.RHI.Vulkan.ComputePipeline");

    auto impl = std::make_unique<VulkanRuntimeComputePipeline::Impl>();
    impl->device_owner = device.impl_;
    impl->pipeline = pipeline;
    result.pipeline = VulkanRuntimeComputePipeline{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime compute pipeline owner ready";
    return result;
}

VulkanRuntimeSwapchainCreateResult create_runtime_swapchain(VulkanRuntimeDevice& device,
                                                            const VulkanRuntimeSwapchainDesc& desc) {
    VulkanRuntimeSwapchainCreateResult result;
    if (desc.surface.value == 0) {
        result.diagnostic = "Vulkan runtime swapchain surface handle is required";
        return result;
    }
    if (!desc.plan.supported) {
        result.diagnostic = "Vulkan swapchain create plan is required";
        return result;
    }
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (desc.plan.image_count == 0 || desc.plan.image_view_count == 0 || !valid_extent(desc.plan.extent)) {
        result.diagnostic = "Vulkan swapchain create plan is incomplete";
        return result;
    }
    if (!surface_format_supported(VulkanSurfaceFormatCandidate{.format = desc.plan.format})) {
        result.diagnostic = "Vulkan swapchain color format is unsupported";
        return result;
    }
    if (device.impl_->host != RhiHostPlatform::windows) {
        result.diagnostic = "Vulkan runtime swapchain ownership is unsupported on this host";
        return result;
    }
    if (device.impl_->create_win32_surface == nullptr || device.impl_->destroy_surface == nullptr ||
        device.impl_->get_surface_capabilities == nullptr || device.impl_->get_surface_formats == nullptr ||
        device.impl_->get_surface_present_modes == nullptr) {
        result.diagnostic = "Vulkan Win32 surface commands are unavailable";
        return result;
    }
    if (device.impl_->create_swapchain == nullptr || device.impl_->destroy_swapchain == nullptr ||
        device.impl_->get_swapchain_images == nullptr || device.impl_->create_image_view == nullptr ||
        device.impl_->destroy_image_view == nullptr) {
        result.diagnostic = "Vulkan swapchain commands are unavailable";
        return result;
    }

#if defined(_WIN32)
    NativeVulkanSurface surface = 0;
    const auto surface_create_info = NativeVulkanWin32SurfaceCreateInfo{
        .s_type = vulkan_structure_type_win32_surface_create_info,
        .next = nullptr,
        .flags = 0,
        .instance = GetModuleHandleW(nullptr),
        .window = reinterpret_cast<HWND>(desc.surface.value),
    };
    const auto surface_result =
        device.impl_->create_win32_surface(device.impl_->instance, &surface_create_info, nullptr, &surface);
    if (surface_result != vulkan_success || surface == 0) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateWin32SurfaceKHR failed", surface_result);
        return result;
    }

    NativeVulkanSurfaceCapabilities capabilities{};
    const auto capabilities_result =
        device.impl_->get_surface_capabilities(device.impl_->physical_device, surface, &capabilities);
    if (capabilities_result != vulkan_success) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed", capabilities_result);
        return result;
    }
    const bool current_extent_defined =
        capabilities.current_extent.width != std::numeric_limits<std::uint32_t>::max() &&
        capabilities.current_extent.height != std::numeric_limits<std::uint32_t>::max();
    if (current_extent_defined && (capabilities.current_extent.width != desc.plan.extent.width ||
                                   capabilities.current_extent.height != desc.plan.extent.height)) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = "Vulkan swapchain create plan extent does not match the runtime surface extent";
        return result;
    }

    std::uint32_t format_count = 0;
    auto formats_result =
        device.impl_->get_surface_formats(device.impl_->physical_device, surface, &format_count, nullptr);
    if (!is_successful_enumeration_result(formats_result) || format_count == 0) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceFormatsKHR count failed", formats_result);
        return result;
    }
    std::vector<NativeVulkanSurfaceFormat> native_formats(format_count);
    formats_result =
        device.impl_->get_surface_formats(device.impl_->physical_device, surface, &format_count, native_formats.data());
    if (!is_successful_enumeration_result(formats_result)) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfaceFormatsKHR failed", formats_result);
        return result;
    }
    native_formats.resize(format_count);
    bool format_supported = false;
    for (const auto& native_format : native_formats) {
        if (native_format.format == 0 || format_from_native_vulkan_format(native_format.format) == desc.plan.format) {
            format_supported = true;
            break;
        }
    }
    if (!format_supported) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = "Vulkan swapchain create plan color format is not supported by the runtime surface";
        return result;
    }

    std::uint32_t present_mode_count = 0;
    auto present_modes_result =
        device.impl_->get_surface_present_modes(device.impl_->physical_device, surface, &present_mode_count, nullptr);
    if (!is_successful_enumeration_result(present_modes_result) || present_mode_count == 0) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfacePresentModesKHR count failed",
                                                     present_modes_result);
        return result;
    }
    std::vector<std::uint32_t> native_present_modes(present_mode_count);
    present_modes_result = device.impl_->get_surface_present_modes(device.impl_->physical_device, surface,
                                                                   &present_mode_count, native_present_modes.data());
    if (!is_successful_enumeration_result(present_modes_result)) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic =
            vulkan_result_diagnostic("Vulkan vkGetPhysicalDeviceSurfacePresentModesKHR failed", present_modes_result);
        return result;
    }
    native_present_modes.resize(present_mode_count);
    bool present_mode_supported = false;
    for (const auto native_mode : native_present_modes) {
        VulkanPresentMode mode{};
        if (present_mode_from_native_vulkan_present_mode(native_mode, mode) && mode == desc.plan.present_mode) {
            present_mode_supported = true;
            break;
        }
    }
    if (!present_mode_supported) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = "Vulkan swapchain create plan present mode is not supported by the runtime surface";
        return result;
    }

    const std::array<std::uint32_t, 2> queue_family_indices{
        device.impl_->graphics_queue_family,
        device.impl_->present_queue_family,
    };
    const bool separate_queues = device.impl_->graphics_queue_family != device.impl_->present_queue_family;
    const auto pre_transform =
        capabilities.current_transform != 0 ? capabilities.current_transform : vulkan_surface_transform_identity_bit;
    const auto composite_alpha = (capabilities.supported_composite_alpha & vulkan_composite_alpha_opaque_bit) != 0U
                                     ? vulkan_composite_alpha_opaque_bit
                                     : capabilities.supported_composite_alpha;
    const NativeVulkanSwapchainCreateInfo create_info{
        .s_type = vulkan_structure_type_swapchain_create_info,
        .next = nullptr,
        .flags = 0,
        .surface = surface,
        .min_image_count = desc.plan.image_count,
        .image_format = native_vulkan_format(desc.plan.format),
        .image_color_space = vulkan_color_space_srgb_nonlinear,
        .image_extent = NativeVulkanExtent2D{.width = desc.plan.extent.width, .height = desc.plan.extent.height},
        .image_array_layers = 1,
        .image_usage = vulkan_image_usage_color_attachment_bit | vulkan_image_usage_transfer_src_bit,
        .image_sharing_mode = separate_queues ? vulkan_sharing_mode_concurrent : vulkan_sharing_mode_exclusive,
        .queue_family_index_count = separate_queues ? 2U : 0U,
        .queue_family_indices = separate_queues ? queue_family_indices.data() : nullptr,
        .pre_transform = pre_transform,
        .composite_alpha = composite_alpha,
        .present_mode = native_vulkan_present_mode(desc.plan.present_mode),
        .clipped = 1,
        .old_swapchain = 0,
    };

    NativeVulkanSwapchain swapchain = 0;
    const auto create_result = device.impl_->create_swapchain(device.impl_->device, &create_info, nullptr, &swapchain);
    if (create_result != vulkan_success || swapchain == 0) {
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateSwapchainKHR failed", create_result);
        return result;
    }

    std::uint32_t image_count = 0;
    auto images_result = device.impl_->get_swapchain_images(device.impl_->device, swapchain, &image_count, nullptr);
    if (!is_successful_enumeration_result(images_result) || image_count == 0) {
        device.impl_->destroy_swapchain(device.impl_->device, swapchain, nullptr);
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkGetSwapchainImagesKHR count failed", images_result);
        return result;
    }

    std::vector<NativeVulkanImage> images(image_count);
    images_result = device.impl_->get_swapchain_images(device.impl_->device, swapchain, &image_count, images.data());
    if (!is_successful_enumeration_result(images_result)) {
        device.impl_->destroy_swapchain(device.impl_->device, swapchain, nullptr);
        device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkGetSwapchainImagesKHR failed", images_result);
        return result;
    }
    images.resize(image_count);

    std::vector<NativeVulkanImageView> image_views;
    image_views.reserve(images.size());
    for (const auto image : images) {
        const NativeVulkanImageViewCreateInfo view_info{
            .s_type = vulkan_structure_type_image_view_create_info,
            .next = nullptr,
            .flags = 0,
            .image = image,
            .view_type = vulkan_image_view_type_2d,
            .format = native_vulkan_format(desc.plan.format),
            .components =
                NativeVulkanComponentMapping{
                    .r = vulkan_component_swizzle_identity,
                    .g = vulkan_component_swizzle_identity,
                    .b = vulkan_component_swizzle_identity,
                    .a = vulkan_component_swizzle_identity,
                },
            .subresource_range =
                NativeVulkanImageSubresourceRange{
                    .aspect_mask = vulkan_image_aspect_color_bit,
                    .base_mip_level = 0,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1,
                },
        };
        NativeVulkanImageView image_view = 0;
        const auto view_result =
            device.impl_->create_image_view(device.impl_->device, &view_info, nullptr, &image_view);
        if (view_result != vulkan_success || image_view == 0) {
            for (const auto created_view : image_views) {
                device.impl_->destroy_image_view(device.impl_->device, created_view, nullptr);
            }
            device.impl_->destroy_swapchain(device.impl_->device, swapchain, nullptr);
            device.impl_->destroy_surface(device.impl_->instance, surface, nullptr);
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateImageView failed", view_result);
            return result;
        }
        image_views.push_back(image_view);
    }

    vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_swapchain_khr, swapchain,
                                "GameEngine.RHI.Vulkan.Swapchain");
    for (std::size_t i = 0; i < images.size(); ++i) {
        vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_image, images[i],
                                    "GameEngine.RHI.Vulkan.SwapchainImage");
        vulkan_label_runtime_object(static_cast<void*>(device.impl_.get()), vulkan_object_type_image_view,
                                    image_views[i], "GameEngine.RHI.Vulkan.SwapchainImageView");
    }

    auto impl = std::make_unique<VulkanRuntimeSwapchain::Impl>();
    impl->device_owner = device.impl_;
    impl->surface = surface;
    impl->swapchain = swapchain;
    impl->images = std::move(images);
    impl->image_views = std::move(image_views);
    impl->extent = desc.plan.extent;
    impl->format = desc.plan.format;
    result.swapchain = VulkanRuntimeSwapchain{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime swapchain owner ready";
    return result;
#else
    result.diagnostic = "Vulkan runtime swapchain ownership is unsupported on this host";
    return result;
#endif
}

VulkanRuntimeFrameSyncCreateResult create_runtime_frame_sync(VulkanRuntimeDevice& device,
                                                             const VulkanRuntimeFrameSyncDesc& desc) {
    VulkanRuntimeFrameSyncCreateResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!desc.create_image_available_semaphore && !desc.create_render_finished_semaphore &&
        !desc.create_in_flight_fence) {
        result.diagnostic = "Vulkan runtime frame sync requires at least one primitive";
        return result;
    }
    if (device.impl_->create_semaphore == nullptr || device.impl_->destroy_semaphore == nullptr ||
        device.impl_->create_fence == nullptr || device.impl_->destroy_fence == nullptr) {
        result.diagnostic = "Vulkan frame sync commands are unavailable";
        return result;
    }

    const NativeVulkanSemaphoreCreateInfo semaphore_info{
        .s_type = vulkan_structure_type_semaphore_create_info,
        .next = nullptr,
        .flags = 0,
    };
    NativeVulkanSemaphore image_available = 0;
    NativeVulkanSemaphore render_finished = 0;
    if (desc.create_image_available_semaphore) {
        const auto create_result =
            device.impl_->create_semaphore(device.impl_->device, &semaphore_info, nullptr, &image_available);
        if (create_result != vulkan_success || image_available == 0) {
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateSemaphore failed", create_result);
            return result;
        }
    }
    if (desc.create_render_finished_semaphore) {
        const auto create_result =
            device.impl_->create_semaphore(device.impl_->device, &semaphore_info, nullptr, &render_finished);
        if (create_result != vulkan_success || render_finished == 0) {
            if (image_available != 0) {
                device.impl_->destroy_semaphore(device.impl_->device, image_available, nullptr);
            }
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateSemaphore failed", create_result);
            return result;
        }
    }

    const NativeVulkanFenceCreateInfo fence_info{
        .s_type = vulkan_structure_type_fence_create_info,
        .next = nullptr,
        .flags = desc.start_in_flight_fence_signaled ? vulkan_fence_create_signaled_bit : 0U,
    };
    NativeVulkanFence in_flight_fence = 0;
    if (desc.create_in_flight_fence) {
        const auto create_result =
            device.impl_->create_fence(device.impl_->device, &fence_info, nullptr, &in_flight_fence);
        if (create_result != vulkan_success || in_flight_fence == 0) {
            if (render_finished != 0) {
                device.impl_->destroy_semaphore(device.impl_->device, render_finished, nullptr);
            }
            if (image_available != 0) {
                device.impl_->destroy_semaphore(device.impl_->device, image_available, nullptr);
            }
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkCreateFence failed", create_result);
            return result;
        }
    }

    auto impl = std::make_unique<VulkanRuntimeFrameSync::Impl>();
    impl->device_owner = device.impl_;
    impl->image_available_semaphore = image_available;
    impl->render_finished_semaphore = render_finished;
    impl->in_flight_fence = in_flight_fence;
    result.sync = VulkanRuntimeFrameSync{std::move(impl)};
    result.created = true;
    result.diagnostic = "Vulkan runtime frame sync owner ready";
    return result;
}

VulkanRuntimeSwapchainAcquireResult
acquire_next_runtime_swapchain_image(VulkanRuntimeDevice& device, VulkanRuntimeSwapchain& swapchain,
                                     VulkanRuntimeFrameSync& sync, const VulkanRuntimeSwapchainAcquireDesc& desc) {
    VulkanRuntimeSwapchainAcquireResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (sync.impl_ == nullptr) {
        result.diagnostic = "Vulkan runtime frame sync is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_ || sync.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan acquire objects must share one runtime device";
        return result;
    }
    if (desc.signal_image_available_semaphore && sync.impl_->image_available_semaphore == 0) {
        result.diagnostic = "Vulkan image-available semaphore is required";
        return result;
    }
    if (desc.signal_in_flight_fence && sync.impl_->in_flight_fence == 0) {
        result.diagnostic = "Vulkan acquire fence is required";
        return result;
    }
    if (device.impl_->acquire_next_image == nullptr) {
        result.diagnostic = "Vulkan swapchain acquire command is unavailable";
        return result;
    }

    std::uint32_t image_index = 0;
    const auto acquire_result = device.impl_->acquire_next_image(
        device.impl_->device, swapchain.impl_->swapchain, desc.timeout_ns,
        desc.signal_image_available_semaphore ? sync.impl_->image_available_semaphore : 0,
        desc.signal_in_flight_fence ? sync.impl_->in_flight_fence : 0, &image_index);
    if (is_successful_acquire_result(acquire_result)) {
        result.acquired = true;
        result.suboptimal = acquire_result == vulkan_suboptimal;
        result.resize_required = result.suboptimal;
        result.image_index = image_index;
        result.diagnostic =
            result.suboptimal ? "Vulkan swapchain image acquired but suboptimal" : "Vulkan swapchain image acquired";
        return result;
    }
    if (acquire_result == vulkan_timeout) {
        result.timeout = true;
        result.diagnostic = "Vulkan swapchain acquire timed out";
        return result;
    }
    if (acquire_result == vulkan_not_ready) {
        result.not_ready = true;
        result.diagnostic = "Vulkan swapchain image is not ready";
        return result;
    }
    if (acquire_result == vulkan_error_out_of_date) {
        result.resize_required = true;
        result.diagnostic = "Vulkan swapchain is out of date";
        return result;
    }

    result.diagnostic = vulkan_result_diagnostic("Vulkan vkAcquireNextImageKHR failed", acquire_result);
    return result;
}

VulkanRuntimeSwapchainPresentResult present_runtime_swapchain_image(VulkanRuntimeDevice& device,
                                                                    VulkanRuntimeSwapchain& swapchain,
                                                                    VulkanRuntimeFrameSync& sync,
                                                                    const VulkanRuntimeSwapchainPresentDesc& desc) {
    VulkanRuntimeSwapchainPresentResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (sync.impl_ == nullptr) {
        result.diagnostic = "Vulkan runtime frame sync is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_ || sync.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan present objects must share one runtime device";
        return result;
    }
    if (desc.image_index >= swapchain.image_count()) {
        result.diagnostic = "Vulkan present image index is out of range";
        return result;
    }
    if (desc.wait_render_finished_semaphore && sync.impl_->render_finished_semaphore == 0) {
        result.diagnostic = "Vulkan render-finished semaphore is required";
        return result;
    }
    if (device.impl_->queue_present == nullptr || device.impl_->present_queue == nullptr) {
        result.diagnostic = "Vulkan queue present command is unavailable";
        return result;
    }

    const NativeVulkanSemaphore wait_semaphore = sync.impl_->render_finished_semaphore;
    const NativeVulkanSwapchain swapchain_handle = swapchain.impl_->swapchain;
    const std::uint32_t image_index = desc.image_index;
    const NativeVulkanPresentInfo present_info{
        .s_type = vulkan_structure_type_present_info,
        .next = nullptr,
        .wait_semaphore_count = desc.wait_render_finished_semaphore ? 1U : 0U,
        .wait_semaphores = desc.wait_render_finished_semaphore ? &wait_semaphore : nullptr,
        .swapchain_count = 1,
        .swapchains = &swapchain_handle,
        .image_indices = &image_index,
        .results = nullptr,
    };

    const auto present_result = device.impl_->queue_present(device.impl_->present_queue, &present_info);
    if (present_result == vulkan_success || present_result == vulkan_suboptimal) {
        result.presented = true;
        result.suboptimal = present_result == vulkan_suboptimal;
        result.resize_required = result.suboptimal;
        result.diagnostic =
            result.suboptimal ? "Vulkan swapchain image presented but suboptimal" : "Vulkan swapchain image presented";
        return result;
    }
    if (present_result == vulkan_error_out_of_date) {
        result.resize_required = true;
        result.diagnostic = "Vulkan swapchain is out of date";
        return result;
    }

    result.diagnostic = vulkan_result_diagnostic("Vulkan vkQueuePresentKHR failed", present_result);
    return result;
}

VulkanRuntimeDescriptorSetBindResult record_runtime_descriptor_set_binding(
    VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool, VulkanRuntimePipelineLayout& pipeline_layout,
    VulkanRuntimeDescriptorSet& descriptor_set, const VulkanRuntimeDescriptorSetBindDesc& desc) {
    VulkanRuntimeDescriptorSetBindResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan descriptor objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!pipeline_layout.owns_layout()) {
        result.diagnostic = "Vulkan runtime pipeline layout is required";
        return result;
    }
    if (!descriptor_set.owns_set()) {
        result.diagnostic = "Vulkan runtime descriptor set is required";
        return result;
    }
    if (pipeline_layout.impl_->device_owner != device.impl_ || descriptor_set.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan descriptor objects must share one runtime device";
        return result;
    }
    if (desc.first_set >= pipeline_layout.descriptor_set_layout_count()) {
        result.diagnostic = "Vulkan descriptor set index is out of range";
        return result;
    }
    if (pipeline_layout.impl_->descriptor_set_layouts[desc.first_set] != descriptor_set.impl_->layout) {
        result.diagnostic = "Vulkan descriptor set layout must match pipeline layout";
        return result;
    }
    if (desc.pipeline_bind_point != vulkan_pipeline_bind_point_graphics &&
        desc.pipeline_bind_point != vulkan_pipeline_bind_point_compute) {
        result.diagnostic = "Vulkan descriptor set pipeline bind point is unsupported";
        return result;
    }
    if (device.impl_->cmd_bind_descriptor_sets == nullptr) {
        result.diagnostic = "Vulkan descriptor set bind command is unavailable";
        return result;
    }

    const auto set = descriptor_set.impl_->set;
    device.impl_->cmd_bind_descriptor_sets(command_pool.impl_->primary_command_buffer, desc.pipeline_bind_point,
                                           pipeline_layout.impl_->layout, desc.first_set, 1, &set, 0, nullptr);
    result.recorded = true;
    result.diagnostic = "Vulkan descriptor set binding recorded";
    return result;
}

VulkanRuntimeComputePipelineBindResult record_runtime_compute_pipeline_binding(VulkanRuntimeDevice& device,
                                                                               VulkanRuntimeCommandPool& command_pool,
                                                                               VulkanRuntimeComputePipeline& pipeline) {
    VulkanRuntimeComputePipelineBindResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan compute pipeline objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!pipeline.owns_pipeline()) {
        result.diagnostic = "Vulkan runtime compute pipeline is required";
        return result;
    }
    if (pipeline.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan compute pipeline objects must share one runtime device";
        return result;
    }
    if (device.impl_->cmd_bind_pipeline == nullptr) {
        result.diagnostic = "Vulkan compute pipeline bind command is unavailable";
        return result;
    }

    device.impl_->cmd_bind_pipeline(command_pool.impl_->primary_command_buffer, vulkan_pipeline_bind_point_compute,
                                    pipeline.impl_->pipeline);
    result.recorded = true;
    result.diagnostic = "Vulkan compute pipeline binding recorded";
    return result;
}

VulkanRuntimeComputeDispatchResult record_runtime_compute_dispatch(VulkanRuntimeDevice& device,
                                                                   VulkanRuntimeCommandPool& command_pool,
                                                                   const VulkanRuntimeComputeDispatchDesc& desc) {
    VulkanRuntimeComputeDispatchResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan compute dispatch objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (desc.group_count_x == 0 || desc.group_count_y == 0 || desc.group_count_z == 0) {
        result.diagnostic = "Vulkan compute dispatch workgroup counts must be non-zero";
        return result;
    }
    if (device.impl_->cmd_dispatch == nullptr) {
        result.diagnostic = "Vulkan compute dispatch command is unavailable";
        return result;
    }

    device.impl_->cmd_dispatch(command_pool.impl_->primary_command_buffer, desc.group_count_x, desc.group_count_y,
                               desc.group_count_z);
    if (device.impl_->cmd_pipeline_barrier2 != nullptr) {
        const NativeVulkanMemoryBarrier2 memory_barrier{
            .s_type = vulkan_structure_type_memory_barrier2,
            .next = nullptr,
            .src_stage_mask = vulkan_pipeline_stage2_compute_shader_bit,
            .src_access_mask = vulkan_access2_shader_write_bit,
            .dst_stage_mask = vulkan_pipeline_stage2_transfer_bit,
            .dst_access_mask = vulkan_access2_transfer_read_bit,
        };
        const NativeVulkanDependencyInfo dependency_info{
            .s_type = vulkan_structure_type_dependency_info,
            .next = nullptr,
            .dependency_flags = 0,
            .memory_barrier_count = 1,
            .memory_barriers = &memory_barrier,
            .buffer_memory_barrier_count = 0,
            .buffer_memory_barriers = nullptr,
            .image_memory_barrier_count = 0,
            .image_memory_barriers = nullptr,
        };
        device.impl_->cmd_pipeline_barrier2(command_pool.impl_->primary_command_buffer, &dependency_info);
    }
    result.recorded = true;
    result.diagnostic = "Vulkan compute dispatch recorded";
    return result;
}

VulkanRuntimeDynamicRenderingClearResult
record_runtime_dynamic_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeSwapchain& swapchain,
                                       const VulkanRuntimeDynamicRenderingClearDesc& desc) {
    VulkanRuntimeDynamicRenderingClearResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!desc.dynamic_rendering.supported) {
        result.diagnostic = "Vulkan dynamic rendering plan is required";
        return result;
    }
    if (!desc.dynamic_rendering.begin_rendering_command_resolved ||
        !desc.dynamic_rendering.end_rendering_command_resolved) {
        result.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return result;
    }
    if (!valid_extent(desc.dynamic_rendering.extent)) {
        result.diagnostic = "Vulkan dynamic rendering extent is required";
        return result;
    }
    if (desc.dynamic_rendering.color_attachment_count != 1 || desc.dynamic_rendering.color_formats.size() != 1) {
        result.diagnostic = "Vulkan dynamic rendering requires exactly one swapchain color attachment";
        return result;
    }
    if (desc.image_index >= swapchain.image_view_count()) {
        result.diagnostic = "Vulkan dynamic rendering image index is out of range";
        return result;
    }
    if (desc.dynamic_rendering.color_formats.front() != swapchain.format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match swapchain";
        return result;
    }
    if (desc.dynamic_rendering.extent.width != swapchain.extent().width ||
        desc.dynamic_rendering.extent.height != swapchain.extent().height) {
        result.diagnostic = "Vulkan dynamic rendering extent must match swapchain";
        return result;
    }
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        if (desc.depth_texture == nullptr || !desc.depth_texture->owns_image() ||
            desc.depth_texture->impl_->image_view == 0) {
            result.diagnostic = "Vulkan dynamic rendering depth texture is required";
            return result;
        }
        if (desc.depth_texture->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must share one runtime device";
            return result;
        }
        if (!has_flag(desc.depth_texture->usage(), TextureUsage::depth_stencil) ||
            desc.depth_texture->format() != desc.dynamic_rendering.depth_format) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must match the plan";
            return result;
        }
        const auto depth_extent = desc.depth_texture->extent();
        if (depth_extent.width != desc.dynamic_rendering.extent.width ||
            depth_extent.height != desc.dynamic_rendering.extent.height || depth_extent.depth != 1) {
            result.diagnostic = "Vulkan dynamic rendering depth extent must match color extent";
            return result;
        }
        if (desc.depth_load_action == LoadAction::clear && !valid_clear_depth(desc.clear_depth)) {
            result.diagnostic = "Vulkan dynamic rendering depth clear value must be finite and in [0, 1]";
            return result;
        }
    } else if (desc.depth_texture != nullptr) {
        result.diagnostic = "Vulkan dynamic rendering depth texture requires a depth plan";
        return result;
    }
    if (device.impl_->cmd_begin_rendering == nullptr || device.impl_->cmd_end_rendering == nullptr) {
        result.diagnostic = "Vulkan dynamic rendering clear commands are unavailable";
        return result;
    }

    const auto extent = desc.dynamic_rendering.extent;
    const NativeVulkanRenderingAttachmentInfo color_attachment{
        .s_type = vulkan_structure_type_rendering_attachment_info,
        .next = nullptr,
        .image_view = swapchain.impl_->image_views[desc.image_index],
        .image_layout = vulkan_image_layout_color_attachment_optimal,
        .resolve_mode = vulkan_resolve_mode_none,
        .resolve_image_view = 0,
        .resolve_image_layout = vulkan_image_layout_color_attachment_optimal,
        .load_op = vulkan_attachment_load_op_clear,
        .store_op = native_vulkan_store_op(desc.color_store_action),
        .clear_value = native_vulkan_clear_color(desc.clear_color),
    };
    NativeVulkanRenderingAttachmentInfo depth_attachment{};
    const NativeVulkanRenderingAttachmentInfo* depth_attachment_ptr = nullptr;
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        depth_attachment = NativeVulkanRenderingAttachmentInfo{
            .s_type = vulkan_structure_type_rendering_attachment_info,
            .next = nullptr,
            .image_view = desc.depth_texture->impl_->image_view,
            .image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .resolve_mode = vulkan_resolve_mode_none,
            .resolve_image_view = 0,
            .resolve_image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .load_op = native_vulkan_load_op(desc.depth_load_action),
            .store_op = native_vulkan_store_op(desc.depth_store_action),
            .clear_value = native_vulkan_clear_depth(desc.clear_depth),
        };
        depth_attachment_ptr = &depth_attachment;
    }
    const NativeVulkanRect2D render_area{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };
    const NativeVulkanRenderingInfo rendering_info{
        .s_type = vulkan_structure_type_rendering_info,
        .next = nullptr,
        .flags = 0,
        .render_area = render_area,
        .layer_count = 1,
        .view_mask = 0,
        .color_attachment_count = 1,
        .color_attachments = &color_attachment,
        .depth_attachment = depth_attachment_ptr,
        .stencil_attachment = nullptr,
    };

    auto* const command_buffer = command_pool.impl_->primary_command_buffer;
    device.impl_->cmd_begin_rendering(command_buffer, &rendering_info);
    result.began_rendering = true;
    device.impl_->cmd_end_rendering(command_buffer);
    result.ended_rendering = true;
    result.recorded = true;
    result.diagnostic = "Vulkan dynamic rendering clear recorded";
    return result;
}

VulkanRuntimeDynamicRenderingClearResult
record_runtime_texture_rendering_clear(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeTexture& texture,
                                       const VulkanRuntimeTextureRenderingClearDesc& desc) {
    VulkanRuntimeDynamicRenderingClearResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!texture.owns_image() || texture.impl_->image_view == 0) {
        result.diagnostic = "Vulkan runtime render target texture is required";
        return result;
    }
    if (texture.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!desc.dynamic_rendering.supported) {
        result.diagnostic = "Vulkan dynamic rendering plan is required";
        return result;
    }
    if (!desc.dynamic_rendering.begin_rendering_command_resolved ||
        !desc.dynamic_rendering.end_rendering_command_resolved) {
        result.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return result;
    }
    if (!valid_extent(desc.dynamic_rendering.extent)) {
        result.diagnostic = "Vulkan dynamic rendering extent is required";
        return result;
    }
    if (desc.dynamic_rendering.color_attachment_count != 1 || desc.dynamic_rendering.color_formats.size() != 1) {
        result.diagnostic = "Vulkan dynamic rendering requires exactly one texture color attachment";
        return result;
    }
    if (desc.dynamic_rendering.color_formats.front() != texture.format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match texture";
        return result;
    }
    if (desc.dynamic_rendering.extent.width != texture.extent().width ||
        desc.dynamic_rendering.extent.height != texture.extent().height) {
        result.diagnostic = "Vulkan dynamic rendering extent must match texture";
        return result;
    }
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        if (desc.depth_texture == nullptr || !desc.depth_texture->owns_image() ||
            desc.depth_texture->impl_->image_view == 0) {
            result.diagnostic = "Vulkan dynamic rendering depth texture is required";
            return result;
        }
        if (desc.depth_texture->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must share one runtime device";
            return result;
        }
        if (!has_flag(desc.depth_texture->usage(), TextureUsage::depth_stencil) ||
            desc.depth_texture->format() != desc.dynamic_rendering.depth_format) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must match the plan";
            return result;
        }
        const auto depth_extent = desc.depth_texture->extent();
        if (depth_extent.width != desc.dynamic_rendering.extent.width ||
            depth_extent.height != desc.dynamic_rendering.extent.height || depth_extent.depth != 1) {
            result.diagnostic = "Vulkan dynamic rendering depth extent must match color extent";
            return result;
        }
        if (desc.depth_load_action == LoadAction::clear && !valid_clear_depth(desc.clear_depth)) {
            result.diagnostic = "Vulkan dynamic rendering depth clear value must be finite and in [0, 1]";
            return result;
        }
    } else if (desc.depth_texture != nullptr) {
        result.diagnostic = "Vulkan dynamic rendering depth texture requires a depth plan";
        return result;
    }
    if (device.impl_->cmd_begin_rendering == nullptr || device.impl_->cmd_end_rendering == nullptr) {
        result.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return result;
    }

    const auto extent = desc.dynamic_rendering.extent;
    const NativeVulkanRenderingAttachmentInfo color_attachment{
        .s_type = vulkan_structure_type_rendering_attachment_info,
        .next = nullptr,
        .image_view = texture.impl_->image_view,
        .image_layout = vulkan_image_layout_color_attachment_optimal,
        .resolve_mode = vulkan_resolve_mode_none,
        .resolve_image_view = 0,
        .resolve_image_layout = vulkan_image_layout_color_attachment_optimal,
        .load_op = vulkan_attachment_load_op_clear,
        .store_op = native_vulkan_store_op(desc.color_store_action),
        .clear_value = native_vulkan_clear_color(desc.clear_color),
    };
    NativeVulkanRenderingAttachmentInfo depth_attachment{};
    const NativeVulkanRenderingAttachmentInfo* depth_attachment_ptr = nullptr;
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        depth_attachment = NativeVulkanRenderingAttachmentInfo{
            .s_type = vulkan_structure_type_rendering_attachment_info,
            .next = nullptr,
            .image_view = desc.depth_texture->impl_->image_view,
            .image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .resolve_mode = vulkan_resolve_mode_none,
            .resolve_image_view = 0,
            .resolve_image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .load_op = native_vulkan_load_op(desc.depth_load_action),
            .store_op = native_vulkan_store_op(desc.depth_store_action),
            .clear_value = native_vulkan_clear_depth(desc.clear_depth),
        };
        depth_attachment_ptr = &depth_attachment;
    }
    const NativeVulkanRect2D render_area{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };
    const NativeVulkanRenderingInfo rendering_info{
        .s_type = vulkan_structure_type_rendering_info,
        .next = nullptr,
        .flags = 0,
        .render_area = render_area,
        .layer_count = 1,
        .view_mask = 0,
        .color_attachment_count = 1,
        .color_attachments = &color_attachment,
        .depth_attachment = depth_attachment_ptr,
        .stencil_attachment = nullptr,
    };

    auto* const command_buffer = command_pool.impl_->primary_command_buffer;
    device.impl_->cmd_begin_rendering(command_buffer, &rendering_info);
    result.began_rendering = true;
    device.impl_->cmd_end_rendering(command_buffer);
    result.ended_rendering = true;
    result.recorded = true;
    result.diagnostic = "Vulkan texture dynamic rendering clear recorded";
    return result;
}

VulkanRuntimeDynamicRenderingDrawResult
record_runtime_texture_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                      VulkanRuntimeTexture& texture, VulkanRuntimeGraphicsPipeline& pipeline,
                                      const VulkanRuntimeTextureRenderingDrawDesc& desc) {
    VulkanRuntimeDynamicRenderingDrawResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!texture.owns_image() || texture.impl_->image_view == 0 ||
        !has_flag(texture.usage(), TextureUsage::render_target)) {
        result.diagnostic = "Vulkan runtime render target texture is required";
        return result;
    }
    if (!pipeline.owns_pipeline()) {
        result.diagnostic = "Vulkan runtime graphics pipeline is required";
        return result;
    }
    if (texture.impl_->device_owner != device.impl_ || pipeline.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!desc.dynamic_rendering.supported) {
        result.diagnostic = "Vulkan dynamic rendering plan is required";
        return result;
    }
    if (!desc.dynamic_rendering.begin_rendering_command_resolved ||
        !desc.dynamic_rendering.end_rendering_command_resolved) {
        result.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return result;
    }
    if (!valid_extent(desc.dynamic_rendering.extent)) {
        result.diagnostic = "Vulkan dynamic rendering extent is required";
        return result;
    }
    if (desc.dynamic_rendering.color_attachment_count != 1 || desc.dynamic_rendering.color_formats.size() != 1) {
        result.diagnostic = "Vulkan dynamic rendering requires exactly one texture color attachment";
        return result;
    }
    const auto expected_depth_format =
        desc.dynamic_rendering.depth_attachment_enabled ? desc.dynamic_rendering.depth_format : Format::unknown;
    if (pipeline.depth_format() != expected_depth_format) {
        result.diagnostic = "Vulkan dynamic rendering depth format must match plan";
        return result;
    }
    if (desc.vertex_count == 0 || desc.instance_count == 0) {
        result.diagnostic = "Vulkan draw counts are required";
        return result;
    }
    const auto uses_vertex_buffer = desc.vertex_buffer != nullptr;
    const auto indexed_draw = desc.index_count > 0;
    if (uses_vertex_buffer) {
        if (!desc.vertex_buffer->owns_buffer()) {
            result.diagnostic = "Vulkan vertex buffer draw requires a vertex buffer";
            return result;
        }
        if (desc.vertex_buffer->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan vertex buffer draw buffers must share one runtime device";
            return result;
        }
        if (!has_flag(desc.vertex_buffer->usage(), BufferUsage::vertex)) {
            result.diagnostic = "Vulkan vertex buffer draw buffers must use vertex usage";
            return result;
        }
        if (desc.vertex_buffer_offset >= desc.vertex_buffer->byte_size()) {
            result.diagnostic = "Vulkan vertex buffer draw buffer offset is out of range";
            return result;
        }
    }
    if (indexed_draw) {
        if (!uses_vertex_buffer || desc.index_buffer == nullptr || !desc.index_buffer->owns_buffer()) {
            result.diagnostic = "Vulkan indexed draw requires vertex and index buffers";
            return result;
        }
        if (desc.index_buffer->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan indexed draw buffers must share one runtime device";
            return result;
        }
        if (!has_flag(desc.index_buffer->usage(), BufferUsage::index)) {
            result.diagnostic = "Vulkan indexed draw buffers must use index usage";
            return result;
        }
        if (desc.index_buffer_offset >= desc.index_buffer->byte_size()) {
            result.diagnostic = "Vulkan indexed draw buffer offset is out of range";
            return result;
        }
        if (desc.index_format == IndexFormat::unknown) {
            result.diagnostic = "Vulkan indexed draw index format is required";
            return result;
        }
    }
    if (pipeline.color_format() != texture.format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match texture";
        return result;
    }
    if (desc.dynamic_rendering.color_formats.front() != pipeline.color_format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match plan";
        return result;
    }
    if (desc.dynamic_rendering.extent.width != texture.extent().width ||
        desc.dynamic_rendering.extent.height != texture.extent().height) {
        result.diagnostic = "Vulkan dynamic rendering extent must match texture";
        return result;
    }
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        if (desc.depth_texture == nullptr || !desc.depth_texture->owns_image() ||
            desc.depth_texture->impl_->image_view == 0) {
            result.diagnostic = "Vulkan dynamic rendering depth texture is required";
            return result;
        }
        if (desc.depth_texture->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must share one runtime device";
            return result;
        }
        if (!has_flag(desc.depth_texture->usage(), TextureUsage::depth_stencil) ||
            desc.depth_texture->format() != desc.dynamic_rendering.depth_format) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must match the plan";
            return result;
        }
        const auto depth_extent = desc.depth_texture->extent();
        if (depth_extent.width != desc.dynamic_rendering.extent.width ||
            depth_extent.height != desc.dynamic_rendering.extent.height || depth_extent.depth != 1) {
            result.diagnostic = "Vulkan dynamic rendering depth extent must match color extent";
            return result;
        }
        if (desc.depth_load_action == LoadAction::clear && !valid_clear_depth(desc.clear_depth)) {
            result.diagnostic = "Vulkan dynamic rendering depth clear value must be finite and in [0, 1]";
            return result;
        }
    } else if (desc.depth_texture != nullptr) {
        result.diagnostic = "Vulkan dynamic rendering depth texture requires a depth plan";
        return result;
    }
    if (device.impl_->cmd_begin_rendering == nullptr || device.impl_->cmd_end_rendering == nullptr ||
        device.impl_->cmd_bind_pipeline == nullptr || device.impl_->cmd_set_viewport == nullptr ||
        device.impl_->cmd_set_scissor == nullptr || device.impl_->cmd_draw == nullptr) {
        result.diagnostic = "Vulkan dynamic rendering draw commands are unavailable";
        return result;
    }
    if (uses_vertex_buffer && device.impl_->cmd_bind_vertex_buffers == nullptr) {
        result.diagnostic = "Vulkan vertex buffer draw commands are unavailable";
        return result;
    }
    if (indexed_draw && (device.impl_->cmd_bind_index_buffer == nullptr || device.impl_->cmd_draw_indexed == nullptr)) {
        result.diagnostic = "Vulkan indexed draw commands are unavailable";
        return result;
    }

    const auto extent = desc.dynamic_rendering.extent;
    const NativeVulkanRenderingAttachmentInfo color_attachment{
        .s_type = vulkan_structure_type_rendering_attachment_info,
        .next = nullptr,
        .image_view = texture.impl_->image_view,
        .image_layout = vulkan_image_layout_color_attachment_optimal,
        .resolve_mode = vulkan_resolve_mode_none,
        .resolve_image_view = 0,
        .resolve_image_layout = vulkan_image_layout_color_attachment_optimal,
        .load_op = native_vulkan_load_op(desc.color_load_action),
        .store_op = native_vulkan_store_op(desc.color_store_action),
        .clear_value = native_vulkan_clear_color(desc.clear_color),
    };
    NativeVulkanRenderingAttachmentInfo depth_attachment{};
    const NativeVulkanRenderingAttachmentInfo* depth_attachment_ptr = nullptr;
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        depth_attachment = NativeVulkanRenderingAttachmentInfo{
            .s_type = vulkan_structure_type_rendering_attachment_info,
            .next = nullptr,
            .image_view = desc.depth_texture->impl_->image_view,
            .image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .resolve_mode = vulkan_resolve_mode_none,
            .resolve_image_view = 0,
            .resolve_image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .load_op = native_vulkan_load_op(desc.depth_load_action),
            .store_op = native_vulkan_store_op(desc.depth_store_action),
            .clear_value = native_vulkan_clear_depth(desc.clear_depth),
        };
        depth_attachment_ptr = &depth_attachment;
    }
    const NativeVulkanRect2D render_area{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };
    const NativeVulkanRenderingInfo rendering_info{
        .s_type = vulkan_structure_type_rendering_info,
        .next = nullptr,
        .flags = 0,
        .render_area = render_area,
        .layer_count = 1,
        .view_mask = 0,
        .color_attachment_count = 1,
        .color_attachments = &color_attachment,
        .depth_attachment = depth_attachment_ptr,
        .stencil_attachment = nullptr,
    };
    const NativeVulkanViewport viewport{
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .min_depth = 0.0F,
        .max_depth = 1.0F,
    };
    const NativeVulkanRect2D scissor{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };

    auto* const command_buffer = command_pool.impl_->primary_command_buffer;
    device.impl_->cmd_begin_rendering(command_buffer, &rendering_info);
    result.began_rendering = true;
    device.impl_->cmd_set_viewport(command_buffer, 0, 1, &viewport);
    device.impl_->cmd_set_scissor(command_buffer, 0, 1, &scissor);
    device.impl_->cmd_bind_pipeline(command_buffer, vulkan_pipeline_bind_point_graphics, pipeline.impl_->pipeline);
    result.bound_pipeline = true;
    if (uses_vertex_buffer) {
        const NativeVulkanBuffer vertex_buffer = desc.vertex_buffer->impl_->buffer;
        const std::uint64_t vertex_offset = desc.vertex_buffer_offset;
        device.impl_->cmd_bind_vertex_buffers(command_buffer, desc.vertex_buffer_binding, 1, &vertex_buffer,
                                              &vertex_offset);
    }
    if (indexed_draw) {
        device.impl_->cmd_bind_index_buffer(command_buffer, desc.index_buffer->impl_->buffer, desc.index_buffer_offset,
                                            native_vulkan_index_type(desc.index_format));
        device.impl_->cmd_draw_indexed(command_buffer, desc.index_count, desc.instance_count, 0, 0,
                                       desc.first_instance);
    } else {
        device.impl_->cmd_draw(command_buffer, desc.vertex_count, desc.instance_count, desc.first_vertex,
                               desc.first_instance);
    }
    result.drew = true;
    device.impl_->cmd_end_rendering(command_buffer);
    result.ended_rendering = true;
    result.recorded = true;
    result.diagnostic = "Vulkan texture dynamic rendering draw recorded";
    return result;
}

VulkanRuntimeDynamicRenderingDrawResult
record_runtime_dynamic_rendering_draw(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                      VulkanRuntimeSwapchain& swapchain, VulkanRuntimeGraphicsPipeline& pipeline,
                                      const VulkanRuntimeDynamicRenderingDrawDesc& desc) {
    VulkanRuntimeDynamicRenderingDrawResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (!pipeline.owns_pipeline()) {
        result.diagnostic = "Vulkan runtime graphics pipeline is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_ || pipeline.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan dynamic rendering objects must share one runtime device";
        return result;
    }
    if (!desc.dynamic_rendering.supported) {
        result.diagnostic = "Vulkan dynamic rendering plan is required";
        return result;
    }
    if (!desc.dynamic_rendering.begin_rendering_command_resolved ||
        !desc.dynamic_rendering.end_rendering_command_resolved) {
        result.diagnostic = "Vulkan dynamic rendering commands are unavailable";
        return result;
    }
    if (!valid_extent(desc.dynamic_rendering.extent)) {
        result.diagnostic = "Vulkan dynamic rendering extent is required";
        return result;
    }
    if (desc.dynamic_rendering.color_attachment_count != 1 || desc.dynamic_rendering.color_formats.size() != 1) {
        result.diagnostic = "Vulkan dynamic rendering requires exactly one swapchain color attachment";
        return result;
    }
    const auto expected_depth_format =
        desc.dynamic_rendering.depth_attachment_enabled ? desc.dynamic_rendering.depth_format : Format::unknown;
    if (pipeline.depth_format() != expected_depth_format) {
        result.diagnostic = "Vulkan dynamic rendering depth format must match plan";
        return result;
    }
    if (desc.image_index >= swapchain.image_view_count()) {
        result.diagnostic = "Vulkan dynamic rendering image index is out of range";
        return result;
    }
    if (desc.vertex_count == 0 || desc.instance_count == 0) {
        result.diagnostic = "Vulkan draw counts are required";
        return result;
    }
    const auto uses_vertex_buffer = desc.vertex_buffer != nullptr;
    const auto indexed_draw = desc.index_count > 0;
    if (uses_vertex_buffer) {
        if (!desc.vertex_buffer->owns_buffer()) {
            result.diagnostic = "Vulkan vertex buffer draw requires a vertex buffer";
            return result;
        }
        if (desc.vertex_buffer->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan vertex buffer draw buffers must share one runtime device";
            return result;
        }
        if (!has_flag(desc.vertex_buffer->usage(), BufferUsage::vertex)) {
            result.diagnostic = "Vulkan vertex buffer draw buffers must use vertex usage";
            return result;
        }
        if (desc.vertex_buffer_offset >= desc.vertex_buffer->byte_size()) {
            result.diagnostic = "Vulkan vertex buffer draw buffer offset is out of range";
            return result;
        }
    }
    if (indexed_draw) {
        if (!uses_vertex_buffer || desc.index_buffer == nullptr || !desc.index_buffer->owns_buffer()) {
            result.diagnostic = "Vulkan indexed draw requires vertex and index buffers";
            return result;
        }
        if (desc.index_buffer->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan indexed draw buffers must share one runtime device";
            return result;
        }
        if (!has_flag(desc.index_buffer->usage(), BufferUsage::index)) {
            result.diagnostic = "Vulkan indexed draw buffers must use index usage";
            return result;
        }
        if (desc.index_buffer_offset >= desc.index_buffer->byte_size()) {
            result.diagnostic = "Vulkan indexed draw buffer offset is out of range";
            return result;
        }
        if (desc.index_format == IndexFormat::unknown) {
            result.diagnostic = "Vulkan indexed draw index format is required";
            return result;
        }
    }
    if (pipeline.color_format() != swapchain.format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match swapchain";
        return result;
    }
    if (desc.dynamic_rendering.color_formats.front() != pipeline.color_format()) {
        result.diagnostic = "Vulkan dynamic rendering color format must match plan";
        return result;
    }
    if (desc.dynamic_rendering.extent.width != swapchain.extent().width ||
        desc.dynamic_rendering.extent.height != swapchain.extent().height) {
        result.diagnostic = "Vulkan dynamic rendering extent must match swapchain";
        return result;
    }
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        if (desc.depth_texture == nullptr || !desc.depth_texture->owns_image() ||
            desc.depth_texture->impl_->image_view == 0) {
            result.diagnostic = "Vulkan dynamic rendering depth texture is required";
            return result;
        }
        if (desc.depth_texture->impl_->device_owner != device.impl_) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must share one runtime device";
            return result;
        }
        if (!has_flag(desc.depth_texture->usage(), TextureUsage::depth_stencil) ||
            desc.depth_texture->format() != desc.dynamic_rendering.depth_format) {
            result.diagnostic = "Vulkan dynamic rendering depth texture must match the plan";
            return result;
        }
        const auto depth_extent = desc.depth_texture->extent();
        if (depth_extent.width != desc.dynamic_rendering.extent.width ||
            depth_extent.height != desc.dynamic_rendering.extent.height || depth_extent.depth != 1) {
            result.diagnostic = "Vulkan dynamic rendering depth extent must match color extent";
            return result;
        }
        if (desc.depth_load_action == LoadAction::clear && !valid_clear_depth(desc.clear_depth)) {
            result.diagnostic = "Vulkan dynamic rendering depth clear value must be finite and in [0, 1]";
            return result;
        }
    } else if (desc.depth_texture != nullptr) {
        result.diagnostic = "Vulkan dynamic rendering depth texture requires a depth plan";
        return result;
    }
    if (device.impl_->cmd_begin_rendering == nullptr || device.impl_->cmd_end_rendering == nullptr ||
        device.impl_->cmd_bind_pipeline == nullptr || device.impl_->cmd_set_viewport == nullptr ||
        device.impl_->cmd_set_scissor == nullptr || device.impl_->cmd_draw == nullptr) {
        result.diagnostic = "Vulkan dynamic rendering draw commands are unavailable";
        return result;
    }
    if (uses_vertex_buffer && device.impl_->cmd_bind_vertex_buffers == nullptr) {
        result.diagnostic = "Vulkan vertex buffer draw commands are unavailable";
        return result;
    }
    if (indexed_draw && (device.impl_->cmd_bind_index_buffer == nullptr || device.impl_->cmd_draw_indexed == nullptr)) {
        result.diagnostic = "Vulkan indexed draw commands are unavailable";
        return result;
    }

    const auto extent = desc.dynamic_rendering.extent;
    const NativeVulkanRenderingAttachmentInfo color_attachment{
        .s_type = vulkan_structure_type_rendering_attachment_info,
        .next = nullptr,
        .image_view = swapchain.impl_->image_views[desc.image_index],
        .image_layout = vulkan_image_layout_color_attachment_optimal,
        .resolve_mode = vulkan_resolve_mode_none,
        .resolve_image_view = 0,
        .resolve_image_layout = vulkan_image_layout_color_attachment_optimal,
        .load_op = native_vulkan_load_op(desc.color_load_action),
        .store_op = native_vulkan_store_op(desc.color_store_action),
        .clear_value = native_vulkan_clear_color(desc.clear_color),
    };
    NativeVulkanRenderingAttachmentInfo depth_attachment{};
    const NativeVulkanRenderingAttachmentInfo* depth_attachment_ptr = nullptr;
    if (desc.dynamic_rendering.depth_attachment_enabled) {
        depth_attachment = NativeVulkanRenderingAttachmentInfo{
            .s_type = vulkan_structure_type_rendering_attachment_info,
            .next = nullptr,
            .image_view = desc.depth_texture->impl_->image_view,
            .image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .resolve_mode = vulkan_resolve_mode_none,
            .resolve_image_view = 0,
            .resolve_image_layout = vulkan_image_layout_depth_stencil_attachment_optimal,
            .load_op = native_vulkan_load_op(desc.depth_load_action),
            .store_op = native_vulkan_store_op(desc.depth_store_action),
            .clear_value = native_vulkan_clear_depth(desc.clear_depth),
        };
        depth_attachment_ptr = &depth_attachment;
    }
    const NativeVulkanRect2D render_area{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };
    const NativeVulkanRenderingInfo rendering_info{
        .s_type = vulkan_structure_type_rendering_info,
        .next = nullptr,
        .flags = 0,
        .render_area = render_area,
        .layer_count = 1,
        .view_mask = 0,
        .color_attachment_count = 1,
        .color_attachments = &color_attachment,
        .depth_attachment = depth_attachment_ptr,
        .stencil_attachment = nullptr,
    };
    const NativeVulkanViewport viewport{
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .min_depth = 0.0F,
        .max_depth = 1.0F,
    };
    const NativeVulkanRect2D scissor{
        .offset = NativeVulkanOffset2D{.x = 0, .y = 0},
        .extent = NativeVulkanExtent2D{.width = extent.width, .height = extent.height},
    };

    auto* const command_buffer = command_pool.impl_->primary_command_buffer;
    device.impl_->cmd_begin_rendering(command_buffer, &rendering_info);
    result.began_rendering = true;
    device.impl_->cmd_set_viewport(command_buffer, 0, 1, &viewport);
    device.impl_->cmd_set_scissor(command_buffer, 0, 1, &scissor);
    device.impl_->cmd_bind_pipeline(command_buffer, vulkan_pipeline_bind_point_graphics, pipeline.impl_->pipeline);
    result.bound_pipeline = true;
    if (uses_vertex_buffer) {
        const NativeVulkanBuffer vertex_buffer = desc.vertex_buffer->impl_->buffer;
        const std::uint64_t vertex_offset = desc.vertex_buffer_offset;
        device.impl_->cmd_bind_vertex_buffers(command_buffer, desc.vertex_buffer_binding, 1, &vertex_buffer,
                                              &vertex_offset);
    }
    if (indexed_draw) {
        device.impl_->cmd_bind_index_buffer(command_buffer, desc.index_buffer->impl_->buffer, desc.index_buffer_offset,
                                            native_vulkan_index_type(desc.index_format));
        device.impl_->cmd_draw_indexed(command_buffer, desc.index_count, desc.instance_count, 0, 0,
                                       desc.first_instance);
    } else {
        device.impl_->cmd_draw(command_buffer, desc.vertex_count, desc.instance_count, desc.first_vertex,
                               desc.first_instance);
    }
    result.drew = true;
    device.impl_->cmd_end_rendering(command_buffer);
    result.ended_rendering = true;
    result.recorded = true;
    result.diagnostic = "Vulkan dynamic rendering draw recorded";
    return result;
}

VulkanRuntimeSwapchainFrameBarrierResult
record_runtime_swapchain_frame_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                       VulkanRuntimeSwapchain& swapchain,
                                       const VulkanRuntimeSwapchainFrameBarrierDesc& desc) {
    VulkanRuntimeSwapchainFrameBarrierResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan synchronization objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan synchronization objects must share one runtime device";
        return result;
    }
    if (desc.image_index >= swapchain.image_count()) {
        result.diagnostic = "Vulkan frame barrier image index is out of range";
        return result;
    }
    if (desc.barrier.before == desc.barrier.after) {
        result.diagnostic = "Vulkan frame barrier state transition is required";
        return result;
    }
    if (!swapchain_barrier_state_supported(desc.barrier.before) ||
        !swapchain_barrier_state_supported(desc.barrier.after)) {
        result.diagnostic = "Vulkan frame barrier state is unsupported for swapchain images";
        return result;
    }
    if (device.impl_->cmd_pipeline_barrier2 == nullptr) {
        result.diagnostic = "Vulkan synchronization2 barrier command is unavailable";
        return result;
    }

    const NativeVulkanImageMemoryBarrier2 image_barrier{
        .s_type = vulkan_structure_type_image_memory_barrier2,
        .next = nullptr,
        .src_stage_mask = native_vulkan_stage(desc.barrier.src_stage),
        .src_access_mask = native_vulkan_access(desc.barrier.src_access),
        .dst_stage_mask = native_vulkan_stage(desc.barrier.dst_stage),
        .dst_access_mask = native_vulkan_access(desc.barrier.dst_access),
        .old_layout = native_vulkan_image_layout(desc.barrier.before),
        .new_layout = native_vulkan_image_layout(desc.barrier.after),
        .src_queue_family_index = vulkan_queue_family_ignored,
        .dst_queue_family_index = vulkan_queue_family_ignored,
        .image = swapchain.impl_->images[desc.image_index],
        .subresource_range =
            NativeVulkanImageSubresourceRange{
                .aspect_mask = vulkan_image_aspect_color_bit,
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
    };
    const NativeVulkanDependencyInfo dependency_info{
        .s_type = vulkan_structure_type_dependency_info,
        .next = nullptr,
        .dependency_flags = 0,
        .memory_barrier_count = 0,
        .memory_barriers = nullptr,
        .buffer_memory_barrier_count = 0,
        .buffer_memory_barriers = nullptr,
        .image_memory_barrier_count = 1,
        .image_memory_barriers = &image_barrier,
    };

    device.impl_->cmd_pipeline_barrier2(command_pool.impl_->primary_command_buffer, &dependency_info);
    result.recorded = true;
    result.barrier_count = 1;
    result.diagnostic = "Vulkan swapchain frame barrier recorded";
    return result;
}

VulkanRuntimeCommandBufferSubmitResult submit_runtime_command_buffer(VulkanRuntimeDevice& device,
                                                                     VulkanRuntimeCommandPool& command_pool,
                                                                     VulkanRuntimeFrameSync& sync,
                                                                     const VulkanRuntimeCommandBufferSubmitDesc& desc) {
    VulkanRuntimeCommandBufferSubmitResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan synchronization objects must share one runtime device";
        return result;
    }
    if (command_pool.recording() || !command_pool.ended()) {
        result.diagnostic = "Vulkan command buffer must be ended before submit";
        return result;
    }
    if (sync.impl_ == nullptr) {
        result.diagnostic = "Vulkan runtime frame sync is required";
        return result;
    }
    if (sync.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan synchronization objects must share one runtime device";
        return result;
    }
    if (desc.wait_image_available_semaphore && sync.impl_->image_available_semaphore == 0) {
        result.diagnostic = "Vulkan image-available semaphore is required";
        return result;
    }
    if (desc.signal_render_finished_semaphore && sync.impl_->render_finished_semaphore == 0) {
        result.diagnostic = "Vulkan render-finished semaphore is required";
        return result;
    }
    if (desc.signal_in_flight_fence && sync.impl_->in_flight_fence == 0) {
        result.diagnostic = "Vulkan submit fence is required";
        return result;
    }
    if (device.impl_->queue_submit2 == nullptr || device.impl_->graphics_queue == nullptr) {
        result.diagnostic = "Vulkan synchronization2 submit command is unavailable";
        return result;
    }

    const NativeVulkanSemaphoreSubmitInfo wait_semaphore{
        .s_type = vulkan_structure_type_semaphore_submit_info,
        .next = nullptr,
        .semaphore = sync.impl_->image_available_semaphore,
        .value = 0,
        .stage_mask = vulkan_pipeline_stage2_color_attachment_output_bit,
        .device_index = 0,
    };
    const NativeVulkanSemaphoreSubmitInfo signal_semaphore{
        .s_type = vulkan_structure_type_semaphore_submit_info,
        .next = nullptr,
        .semaphore = sync.impl_->render_finished_semaphore,
        .value = 0,
        .stage_mask = vulkan_pipeline_stage2_color_attachment_output_bit,
        .device_index = 0,
    };
    const NativeVulkanCommandBufferSubmitInfo command_buffer{
        .s_type = vulkan_structure_type_command_buffer_submit_info,
        .next = nullptr,
        .command_buffer = command_pool.impl_->primary_command_buffer,
        .device_mask = 0,
    };
    const NativeVulkanSubmitInfo2 submit_info{
        .s_type = vulkan_structure_type_submit_info2,
        .next = nullptr,
        .flags = 0,
        .wait_semaphore_info_count = desc.wait_image_available_semaphore ? 1U : 0U,
        .wait_semaphore_infos = desc.wait_image_available_semaphore ? &wait_semaphore : nullptr,
        .command_buffer_info_count = 1,
        .command_buffer_infos = &command_buffer,
        .signal_semaphore_info_count = desc.signal_render_finished_semaphore ? 1U : 0U,
        .signal_semaphore_infos = desc.signal_render_finished_semaphore ? &signal_semaphore : nullptr,
    };

    const auto submit_result = device.impl_->queue_submit2(
        device.impl_->graphics_queue, 1, &submit_info, desc.signal_in_flight_fence ? sync.impl_->in_flight_fence : 0);
    if (submit_result != vulkan_success) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkQueueSubmit2 failed", submit_result);
        return result;
    }

    if (desc.wait_for_graphics_queue_idle) {
        if (device.impl_->queue_wait_idle == nullptr) {
            result.diagnostic = "Vulkan queue wait idle command is unavailable";
            return result;
        }
        const auto wait_result = device.impl_->queue_wait_idle(device.impl_->graphics_queue);
        if (wait_result != vulkan_success) {
            result.diagnostic = vulkan_result_diagnostic("Vulkan vkQueueWaitIdle failed", wait_result);
            return result;
        }
        result.graphics_queue_idle_waited = true;
    }
    result.submitted = true;
    result.diagnostic = "Vulkan command buffer submitted";
    return result;
}

VulkanRuntimeTextureBarrierResult record_runtime_texture_barrier(VulkanRuntimeDevice& device,
                                                                 VulkanRuntimeCommandPool& command_pool,
                                                                 VulkanRuntimeTexture& texture,
                                                                 const VulkanRuntimeTextureBarrierDesc& desc) {
    VulkanRuntimeTextureBarrierResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (!texture.owns_image() || !texture.owns_memory()) {
        result.diagnostic = "Vulkan runtime texture is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_ || texture.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan texture barrier objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (desc.before == desc.after) {
        result.diagnostic = "Vulkan texture barrier state transition is required";
        return result;
    }
    if (!texture_barrier_state_supported(desc.before) || !texture_barrier_state_supported(desc.after)) {
        result.diagnostic = "Vulkan texture barrier state is unsupported";
        return result;
    }
    if (!texture_state_supported_for_desc(texture.impl_->desc, desc.before) ||
        !texture_state_supported_for_desc(texture.impl_->desc, desc.after)) {
        result.diagnostic = "Vulkan texture barrier state is incompatible with texture usage or format";
        return result;
    }
    if (device.impl_->cmd_pipeline_barrier2 == nullptr) {
        result.diagnostic = "Vulkan synchronization2 barrier command is unavailable";
        return result;
    }

    const NativeVulkanImageMemoryBarrier2 image_barrier{
        .s_type = vulkan_structure_type_image_memory_barrier2,
        .next = nullptr,
        .src_stage_mask = native_vulkan_texture_barrier_stage(desc.before),
        .src_access_mask = native_vulkan_texture_barrier_access(desc.before),
        .dst_stage_mask = native_vulkan_texture_barrier_stage(desc.after),
        .dst_access_mask = native_vulkan_texture_barrier_access(desc.after),
        .old_layout = native_vulkan_image_layout(desc.before),
        .new_layout = native_vulkan_image_layout(desc.after),
        .src_queue_family_index = vulkan_queue_family_ignored,
        .dst_queue_family_index = vulkan_queue_family_ignored,
        .image = texture.impl_->image,
        .subresource_range =
            NativeVulkanImageSubresourceRange{
                .aspect_mask = native_vulkan_image_barrier_aspect_flags(texture.format()),
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
    };
    const NativeVulkanDependencyInfo dependency_info{
        .s_type = vulkan_structure_type_dependency_info,
        .next = nullptr,
        .dependency_flags = 0,
        .memory_barrier_count = 0,
        .memory_barriers = nullptr,
        .buffer_memory_barrier_count = 0,
        .buffer_memory_barriers = nullptr,
        .image_memory_barrier_count = 1,
        .image_memory_barriers = &image_barrier,
    };

    device.impl_->cmd_pipeline_barrier2(command_pool.impl_->primary_command_buffer, &dependency_info);
    result.recorded = true;
    result.barrier_count = 1;
    result.diagnostic = "Vulkan texture barrier recorded";
    return result;
}

VulkanRuntimeBufferCopyResult record_runtime_buffer_copy(VulkanRuntimeDevice& device,
                                                         VulkanRuntimeCommandPool& command_pool,
                                                         VulkanRuntimeBuffer& source, VulkanRuntimeBuffer& destination,
                                                         const VulkanRuntimeBufferCopyDesc& desc) {
    VulkanRuntimeBufferCopyResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (!source.owns_buffer() || !destination.owns_buffer()) {
        result.diagnostic = "Vulkan runtime buffer copy resources are required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_ || source.impl_->device_owner != device.impl_ ||
        destination.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan buffer copy objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!has_flag(source.usage(), BufferUsage::copy_source) ||
        !has_flag(destination.usage(), BufferUsage::copy_destination)) {
        result.diagnostic = "Vulkan buffer copy requires copy_source and copy_destination usage";
        return result;
    }
    if (desc.region.size_bytes == 0 || desc.region.source_offset > source.byte_size() ||
        desc.region.size_bytes > source.byte_size() - desc.region.source_offset ||
        desc.region.destination_offset > destination.byte_size() ||
        desc.region.size_bytes > destination.byte_size() - desc.region.destination_offset) {
        result.diagnostic = "Vulkan buffer copy range is outside the resources";
        return result;
    }
    if (device.impl_->cmd_copy_buffer == nullptr) {
        result.diagnostic = "Vulkan buffer copy command is unavailable";
        return result;
    }

    const NativeVulkanBufferCopy copy_region{
        .src_offset = desc.region.source_offset,
        .dst_offset = desc.region.destination_offset,
        .size = desc.region.size_bytes,
    };
    device.impl_->cmd_copy_buffer(command_pool.impl_->primary_command_buffer, source.impl_->buffer,
                                  destination.impl_->buffer, 1, &copy_region);
    result.recorded = true;
    result.bytes_copied = desc.region.size_bytes;
    result.diagnostic = "Vulkan buffer copy recorded";
    return result;
}

VulkanRuntimeBufferTextureCopyResult
record_runtime_buffer_texture_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeBuffer& source, VulkanRuntimeTexture& destination,
                                   const VulkanRuntimeBufferTextureCopyDesc& desc) {
    VulkanRuntimeBufferTextureCopyResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (!source.owns_buffer() || !destination.owns_image()) {
        result.diagnostic = "Vulkan buffer texture copy resources are required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_ || source.impl_->device_owner != device.impl_ ||
        destination.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan buffer texture copy objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!has_flag(source.usage(), BufferUsage::copy_source) ||
        !has_flag(destination.usage(), TextureUsage::copy_destination)) {
        result.diagnostic = "Vulkan buffer texture copy requires copy_source buffer and copy_destination texture usage";
        return result;
    }
    try {
        result.required_bytes = buffer_texture_copy_required_bytes(destination.format(), desc.region);
    } catch (const std::invalid_argument& error) {
        result.diagnostic = error.what();
        return result;
    }
    const auto extent = destination.extent();
    if (desc.region.texture_offset.x > extent.width ||
        desc.region.texture_extent.width > extent.width - desc.region.texture_offset.x ||
        desc.region.texture_offset.y > extent.height ||
        desc.region.texture_extent.height > extent.height - desc.region.texture_offset.y ||
        desc.region.texture_offset.z > extent.depth ||
        desc.region.texture_extent.depth > extent.depth - desc.region.texture_offset.z) {
        result.diagnostic = "Vulkan buffer texture copy region is outside the texture";
        return result;
    }
    if (result.required_bytes > source.byte_size()) {
        result.diagnostic = "Vulkan buffer texture copy source range is outside the source buffer";
        return result;
    }
    if (device.impl_->cmd_copy_buffer_to_image == nullptr) {
        result.diagnostic = "Vulkan buffer texture copy command is unavailable";
        return result;
    }

    const NativeVulkanBufferImageCopy copy_region{
        .buffer_offset = desc.region.buffer_offset,
        .buffer_row_length = desc.region.buffer_row_length,
        .buffer_image_height = desc.region.buffer_image_height,
        .image_subresource =
            NativeVulkanImageSubresourceLayers{
                .aspect_mask = vulkan_image_aspect_color_bit, .mip_level = 0, .base_array_layer = 0, .layer_count = 1},
        .image_offset = NativeVulkanOffset3D{.x = static_cast<std::int32_t>(desc.region.texture_offset.x),
                                             .y = static_cast<std::int32_t>(desc.region.texture_offset.y),
                                             .z = static_cast<std::int32_t>(desc.region.texture_offset.z)},
        .image_extent = NativeVulkanExtent3D{.width = desc.region.texture_extent.width,
                                             .height = desc.region.texture_extent.height,
                                             .depth = desc.region.texture_extent.depth},
    };
    device.impl_->cmd_copy_buffer_to_image(command_pool.impl_->primary_command_buffer, source.impl_->buffer,
                                           destination.impl_->image, vulkan_image_layout_transfer_dst_optimal, 1,
                                           &copy_region);
    result.recorded = true;
    result.diagnostic = "Vulkan buffer texture copy recorded";
    return result;
}

VulkanRuntimeTextureBufferCopyResult
record_runtime_texture_buffer_copy(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                   VulkanRuntimeTexture& source, VulkanRuntimeBuffer& destination,
                                   const VulkanRuntimeTextureBufferCopyDesc& desc) {
    VulkanRuntimeTextureBufferCopyResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (!source.owns_image() || !destination.owns_buffer()) {
        result.diagnostic = "Vulkan texture buffer copy resources are required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_ || source.impl_->device_owner != device.impl_ ||
        destination.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan texture buffer copy objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!has_flag(source.usage(), TextureUsage::copy_source) ||
        !has_flag(destination.usage(), BufferUsage::copy_destination)) {
        result.diagnostic = "Vulkan texture buffer copy requires copy_source texture and copy_destination buffer usage";
        return result;
    }
    try {
        result.required_bytes = buffer_texture_copy_required_bytes(source.format(), desc.region);
    } catch (const std::invalid_argument& error) {
        result.diagnostic = error.what();
        return result;
    }
    const auto extent = source.extent();
    if (desc.region.texture_offset.x > extent.width ||
        desc.region.texture_extent.width > extent.width - desc.region.texture_offset.x ||
        desc.region.texture_offset.y > extent.height ||
        desc.region.texture_extent.height > extent.height - desc.region.texture_offset.y ||
        desc.region.texture_offset.z > extent.depth ||
        desc.region.texture_extent.depth > extent.depth - desc.region.texture_offset.z) {
        result.diagnostic = "Vulkan texture buffer copy region is outside the texture";
        return result;
    }
    if (result.required_bytes > destination.byte_size()) {
        result.diagnostic = "Vulkan texture buffer copy destination range is outside the destination buffer";
        return result;
    }
    if (device.impl_->cmd_copy_image_to_buffer == nullptr) {
        result.diagnostic = "Vulkan texture buffer copy command is unavailable";
        return result;
    }

    const NativeVulkanBufferImageCopy copy_region{
        .buffer_offset = desc.region.buffer_offset,
        .buffer_row_length = desc.region.buffer_row_length,
        .buffer_image_height = desc.region.buffer_image_height,
        .image_subresource =
            NativeVulkanImageSubresourceLayers{
                .aspect_mask = vulkan_image_aspect_color_bit, .mip_level = 0, .base_array_layer = 0, .layer_count = 1},
        .image_offset = NativeVulkanOffset3D{.x = static_cast<std::int32_t>(desc.region.texture_offset.x),
                                             .y = static_cast<std::int32_t>(desc.region.texture_offset.y),
                                             .z = static_cast<std::int32_t>(desc.region.texture_offset.z)},
        .image_extent = NativeVulkanExtent3D{.width = desc.region.texture_extent.width,
                                             .height = desc.region.texture_extent.height,
                                             .depth = desc.region.texture_extent.depth},
    };
    device.impl_->cmd_copy_image_to_buffer(command_pool.impl_->primary_command_buffer, source.impl_->image,
                                           vulkan_image_layout_transfer_src_optimal, destination.impl_->buffer, 1,
                                           &copy_region);
    result.recorded = true;
    result.diagnostic = "Vulkan texture buffer copy recorded";
    return result;
}

VulkanRuntimeSwapchainReadbackResult
record_runtime_swapchain_image_readback(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                        VulkanRuntimeSwapchain& swapchain, VulkanRuntimeReadbackBuffer& readback_buffer,
                                        const VulkanRuntimeSwapchainReadbackDesc& desc) {
    VulkanRuntimeSwapchainReadbackResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!command_pool.owns_primary_command_buffer()) {
        result.diagnostic = "Vulkan runtime command pool is required";
        return result;
    }
    if (command_pool.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan readback objects must share one runtime device";
        return result;
    }
    if (!command_pool.recording()) {
        result.diagnostic = "Vulkan command buffer must be recording";
        return result;
    }
    if (!swapchain.owns_swapchain()) {
        result.diagnostic = "Vulkan runtime swapchain is required";
        return result;
    }
    if (swapchain.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan readback objects must share one runtime device";
        return result;
    }
    if (!readback_buffer.owns_buffer() || !readback_buffer.owns_memory()) {
        result.diagnostic = "Vulkan runtime readback buffer is required";
        return result;
    }
    if (readback_buffer.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan readback objects must share one runtime device";
        return result;
    }
    if (desc.image_index >= swapchain.image_count()) {
        result.diagnostic = "Vulkan readback image index is out of range";
        return result;
    }
    if (!valid_extent(desc.extent)) {
        result.diagnostic = "Vulkan readback extent is required";
        return result;
    }
    if (desc.bytes_per_pixel == 0) {
        result.diagnostic = "Vulkan readback bytes per pixel is required";
        return result;
    }
    if (desc.extent.width > swapchain.extent().width || desc.extent.height > swapchain.extent().height) {
        result.diagnostic = "Vulkan readback extent must fit swapchain";
        return result;
    }

    std::uint64_t required_bytes = 0;
    if (!calculate_readback_byte_count(desc.extent, desc.bytes_per_pixel, required_bytes)) {
        result.diagnostic = "Vulkan readback byte count overflow";
        return result;
    }
    result.required_bytes = required_bytes;
    if (required_bytes > readback_buffer.byte_size()) {
        result.diagnostic = "Vulkan readback buffer is too small";
        return result;
    }
    if (device.impl_->cmd_copy_image_to_buffer == nullptr) {
        result.diagnostic = "Vulkan image readback copy command is unavailable";
        return result;
    }

    const NativeVulkanBufferImageCopy copy_region{
        .buffer_offset = 0,
        .buffer_row_length = 0,
        .buffer_image_height = 0,
        .image_subresource =
            NativeVulkanImageSubresourceLayers{
                .aspect_mask = vulkan_image_aspect_color_bit,
                .mip_level = 0,
                .base_array_layer = 0,
                .layer_count = 1,
            },
        .image_offset = NativeVulkanOffset3D{.x = 0, .y = 0, .z = 0},
        .image_extent = NativeVulkanExtent3D{.width = desc.extent.width, .height = desc.extent.height, .depth = 1},
    };

    device.impl_->cmd_copy_image_to_buffer(
        command_pool.impl_->primary_command_buffer, swapchain.impl_->images[desc.image_index],
        vulkan_image_layout_transfer_src_optimal, readback_buffer.impl_->buffer, 1, &copy_region);
    result.recorded = true;
    result.diagnostic = "Vulkan swapchain image readback recorded";
    return result;
}

VulkanRuntimeBufferWriteResult write_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                    const VulkanRuntimeBufferWriteDesc& desc) {
    VulkanRuntimeBufferWriteResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!buffer.owns_buffer() || !buffer.owns_memory()) {
        result.diagnostic = "Vulkan runtime buffer is required";
        return result;
    }
    if (buffer.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan buffer write objects must share one runtime device";
        return result;
    }
    if (buffer.memory_domain() != VulkanBufferMemoryDomain::upload) {
        result.diagnostic = "Vulkan runtime buffer write requires upload memory";
        return result;
    }
    if (device.impl_->map_memory == nullptr || device.impl_->unmap_memory == nullptr) {
        result.diagnostic = "Vulkan buffer write map commands are unavailable";
        return result;
    }
    if (desc.bytes.empty()) {
        result.diagnostic = "Vulkan buffer write bytes are required";
        return result;
    }
    if (desc.byte_offset >= buffer.byte_size()) {
        result.diagnostic = "Vulkan buffer write byte offset is out of range";
        return result;
    }

    const auto available_byte_count = buffer.byte_size() - desc.byte_offset;
    if (desc.bytes.size() > available_byte_count) {
        result.diagnostic = "Vulkan buffer write byte range is out of range";
        return result;
    }

    if (buffer.byte_size() > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        result.diagnostic = "Vulkan buffer write byte count exceeds host addressable size";
        return result;
    }

    void* mapped_memory = nullptr;
    const auto map_result =
        device.impl_->map_memory(device.impl_->device, buffer.impl_->memory, 0, buffer.byte_size(), 0, &mapped_memory);
    if (map_result != vulkan_success || mapped_memory == nullptr) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkMapMemory failed", map_result);
        return result;
    }

    auto* mapped_bytes = static_cast<std::uint8_t*>(mapped_memory);
    std::memcpy(&mapped_bytes[desc.byte_offset], desc.bytes.data(), desc.bytes.size());
    device.impl_->unmap_memory(device.impl_->device, buffer.impl_->memory);
    result.written = true;
    result.diagnostic = "Vulkan runtime buffer written";
    return result;
}

VulkanRuntimeBufferReadResult read_runtime_buffer(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& buffer,
                                                  const VulkanRuntimeBufferReadDesc& desc) {
    VulkanRuntimeBufferReadResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!buffer.owns_buffer() || !buffer.owns_memory()) {
        result.diagnostic = "Vulkan runtime buffer is required";
        return result;
    }
    if (buffer.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan buffer read objects must share one runtime device";
        return result;
    }
    if (buffer.memory_domain() != VulkanBufferMemoryDomain::readback) {
        result.diagnostic = "Vulkan runtime buffer read requires readback memory";
        return result;
    }
    if (device.impl_->map_memory == nullptr || device.impl_->unmap_memory == nullptr) {
        result.diagnostic = "Vulkan buffer read map commands are unavailable";
        return result;
    }
    if (desc.byte_offset >= buffer.byte_size()) {
        result.diagnostic = "Vulkan buffer read byte offset is out of range";
        return result;
    }

    const auto available_byte_count = buffer.byte_size() - desc.byte_offset;
    const auto requested_byte_count = desc.byte_count == 0 ? available_byte_count : desc.byte_count;
    if (requested_byte_count == 0 || requested_byte_count > available_byte_count) {
        result.diagnostic = "Vulkan buffer read byte range is out of range";
        return result;
    }
    if (requested_byte_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        result.diagnostic = "Vulkan buffer read byte count exceeds host addressable size";
        return result;
    }

    void* mapped_memory = nullptr;
    const auto map_result = device.impl_->map_memory(device.impl_->device, buffer.impl_->memory, desc.byte_offset,
                                                     requested_byte_count, 0, &mapped_memory);
    if (map_result != vulkan_success || mapped_memory == nullptr) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkMapMemory failed", map_result);
        return result;
    }

    result.bytes.resize(static_cast<std::size_t>(requested_byte_count));
    std::memcpy(result.bytes.data(), mapped_memory, result.bytes.size());
    device.impl_->unmap_memory(device.impl_->device, buffer.impl_->memory);
    result.read = true;
    result.diagnostic = "Vulkan runtime buffer read";
    return result;
}

VulkanRuntimeReadbackBufferReadResult read_runtime_readback_buffer(VulkanRuntimeDevice& device,
                                                                   VulkanRuntimeReadbackBuffer& readback_buffer,
                                                                   const VulkanRuntimeReadbackBufferReadDesc& desc) {
    VulkanRuntimeReadbackBufferReadResult result;
    if (device.impl_ == nullptr || device.impl_->device == nullptr) {
        result.diagnostic = "Vulkan runtime device is not available";
        return result;
    }
    if (!readback_buffer.owns_buffer() || !readback_buffer.owns_memory()) {
        result.diagnostic = "Vulkan runtime readback buffer is required";
        return result;
    }
    if (readback_buffer.impl_->device_owner != device.impl_) {
        result.diagnostic = "Vulkan readback objects must share one runtime device";
        return result;
    }
    if (device.impl_->map_memory == nullptr || device.impl_->unmap_memory == nullptr) {
        result.diagnostic = "Vulkan readback map commands are unavailable";
        return result;
    }
    if (desc.byte_offset >= readback_buffer.byte_size()) {
        result.diagnostic = "Vulkan readback byte offset is out of range";
        return result;
    }

    const auto available_byte_count = readback_buffer.byte_size() - desc.byte_offset;
    const auto requested_byte_count = desc.byte_count == 0 ? available_byte_count : desc.byte_count;
    if (requested_byte_count == 0 || requested_byte_count > available_byte_count) {
        result.diagnostic = "Vulkan readback byte range is out of range";
        return result;
    }
    if (requested_byte_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        result.diagnostic = "Vulkan readback byte count exceeds host addressable size";
        return result;
    }

    void* mapped_memory = nullptr;
    const auto map_result = device.impl_->map_memory(device.impl_->device, readback_buffer.impl_->memory,
                                                     desc.byte_offset, requested_byte_count, 0, &mapped_memory);
    if (map_result != vulkan_success || mapped_memory == nullptr) {
        result.diagnostic = vulkan_result_diagnostic("Vulkan vkMapMemory failed", map_result);
        return result;
    }

    result.bytes.resize(static_cast<std::size_t>(requested_byte_count));
    std::memcpy(result.bytes.data(), mapped_memory, result.bytes.size());
    device.impl_->unmap_memory(device.impl_->device, readback_buffer.impl_->memory);
    result.read = true;
    result.diagnostic = "Vulkan readback buffer read";
    return result;
}

} // namespace mirakana::rhi::vulkan
