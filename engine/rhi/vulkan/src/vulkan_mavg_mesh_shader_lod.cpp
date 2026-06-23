// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

namespace mirakana::rhi::vulkan {
namespace {

inline constexpr std::uint64_t k_vulkan_pipeline_stage2_host_bit = 0x0000000000004000ULL;
inline constexpr std::uint64_t k_vulkan_pipeline_stage2_draw_indirect_bit = 0x0000000000000002ULL;
inline constexpr std::uint64_t k_vulkan_pipeline_stage2_task_shader_bit_ext = 0x0000000000080000ULL;
inline constexpr std::uint64_t k_vulkan_pipeline_stage2_mesh_shader_bit_ext = 0x0000000000100000ULL;
inline constexpr std::uint64_t k_vulkan_access2_host_write_bit = 0x0000000000004000ULL;
inline constexpr std::uint64_t k_vulkan_access2_indirect_command_read_bit = 0x0000000000000001ULL;
inline constexpr std::uint64_t k_vulkan_access2_shader_read_bit = 0x0000000000000020ULL;
inline constexpr std::uint64_t k_shader_payload_buffer_size_bytes = sizeof(std::uint32_t) * 2ULL;

[[nodiscard]] bool command_resolved(const VulkanCommandResolutionPlan& plan, std::string_view command_name) noexcept {
    for (const auto& resolution : plan.resolutions) {
        if (resolution.request.name == command_name) {
            return resolution.resolved;
        }
    }
    return false;
}

[[nodiscard]] bool valid_task_row(const VulkanMavgMeshShaderLodTaskRow& row) noexcept {
    constexpr std::uint32_t max_mesh_shader_group_thread_count = 128U;
    return row.output_vertex_count >= 3U && row.output_primitive_count > 0U && row.task_group_count_x > 0U &&
           row.task_group_count_y > 0U && row.task_group_count_z > 0U && row.mesh_group_thread_count > 0U &&
           row.mesh_group_thread_count <= max_mesh_shader_group_thread_count && row.fallback_index_count > 0U;
}

[[nodiscard]] bool valid_task_rows(std::span<const VulkanMavgMeshShaderLodTaskRow> rows) noexcept {
    if (rows.empty()) {
        return false;
    }
    for (const auto& row : rows) {
        if (!valid_task_row(row)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool add_would_overflow(std::uint64_t lhs, std::uint64_t rhs) noexcept {
    return lhs > std::numeric_limits<std::uint64_t>::max() - rhs;
}

[[nodiscard]] bool mul_would_overflow(std::uint64_t lhs, std::uint64_t rhs) noexcept {
    return lhs != 0U && rhs > std::numeric_limits<std::uint64_t>::max() / lhs;
}

[[nodiscard]] bool indirect_range_valid(std::uint64_t buffer_size_bytes, std::uint64_t offset_bytes,
                                        std::uint32_t draw_count, std::uint32_t stride_bytes) noexcept {
    if (draw_count == 0U || (offset_bytes % 4U) != 0U ||
        stride_bytes < vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes || (stride_bytes % 4U) != 0U) {
        return false;
    }

    const auto last_draw = static_cast<std::uint64_t>(draw_count - 1U);
    const auto stride = static_cast<std::uint64_t>(stride_bytes);
    if (mul_would_overflow(stride, last_draw)) {
        return false;
    }
    const auto last_draw_offset = stride * last_draw;
    if (add_would_overflow(offset_bytes, last_draw_offset)) {
        return false;
    }
    const auto command_end_without_size = offset_bytes + last_draw_offset;
    if (add_would_overflow(command_end_without_size, vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes)) {
        return false;
    }
    return command_end_without_size + vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes <= buffer_size_bytes;
}

[[nodiscard]] bool count_buffer_range_valid(std::uint64_t buffer_size_bytes, std::uint64_t offset_bytes) noexcept {
    constexpr std::uint64_t count_buffer_size_bytes = sizeof(std::uint32_t);
    return (offset_bytes % 4U) == 0U && offset_bytes <= buffer_size_bytes &&
           count_buffer_size_bytes <= buffer_size_bytes - offset_bytes;
}

[[nodiscard]] std::uint64_t indirect_argument_payload_size_bytes(std::uint32_t draw_count,
                                                                 std::uint32_t stride_bytes) noexcept {
    if (draw_count == 0U) {
        return 0;
    }
    return (static_cast<std::uint64_t>(stride_bytes) * static_cast<std::uint64_t>(draw_count - 1U)) +
           vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes;
}

[[nodiscard]] std::vector<std::uint8_t> encode_indirect_command_payload(const VulkanMavgMeshShaderLodTaskRow& row,
                                                                        std::uint32_t draw_count,
                                                                        std::uint32_t stride_bytes) {
    std::vector<std::uint8_t> payload(
        static_cast<std::size_t>(indirect_argument_payload_size_bytes(draw_count, stride_bytes)));
    for (std::uint32_t draw_index = 0; draw_index < draw_count; ++draw_index) {
        const VulkanMavgMeshShaderLodIndirectCommand command{
            .group_count_x = row.task_group_count_x,
            .group_count_y = row.task_group_count_y,
            .group_count_z = row.task_group_count_z,
        };
        std::memcpy(payload.data() + (static_cast<std::size_t>(draw_index) * stride_bytes), &command, sizeof(command));
    }
    return payload;
}

[[nodiscard]] std::array<std::uint8_t, sizeof(std::uint32_t)> encode_u32_bytes(std::uint32_t value) noexcept {
    std::array<std::uint8_t, sizeof(std::uint32_t)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

[[nodiscard]] std::array<std::uint8_t, k_shader_payload_buffer_size_bytes>
encode_shader_payload_bytes(std::uint32_t task_value, std::uint32_t mesh_value) noexcept {
    const std::array<std::uint32_t, 2> values{task_value, mesh_value};
    std::array<std::uint8_t, k_shader_payload_buffer_size_bytes> bytes{};
    std::memcpy(bytes.data(), values.data(), bytes.size());
    return bytes;
}

void populate_indirect_validation(VulkanMavgMeshShaderLodDispatchResult& result,
                                  const VulkanMavgMeshShaderLodDispatchDesc& desc) noexcept {
    result.indirect_argument_buffer_usage_ready = has_flag(desc.indirect_buffer_usage, BufferUsage::indirect);
    result.indirect_argument_offset_aligned = (desc.indirect_buffer_offset_bytes % 4U) == 0U;
    result.indirect_stride_valid =
        desc.indirect_stride_bytes >= vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes &&
        (desc.indirect_stride_bytes % 4U) == 0U;
    const auto draw_count = desc.request_indirect_count_draw ? desc.max_indirect_count_draws : desc.indirect_draw_count;
    result.indirect_argument_range_valid =
        result.indirect_argument_buffer_usage_ready &&
        indirect_range_valid(desc.indirect_buffer_size_bytes, desc.indirect_buffer_offset_bytes, draw_count,
                             desc.indirect_stride_bytes);
    result.draw_mesh_tasks_indirect_eligible = result.indirect_argument_buffer_usage_ready &&
                                               result.indirect_argument_offset_aligned &&
                                               result.indirect_stride_valid && result.indirect_argument_range_valid;

    if (desc.request_indirect_count_draw) {
        result.indirect_count_buffer_usage_ready = has_flag(desc.count_buffer_usage, BufferUsage::indirect);
        result.indirect_count_offset_aligned = (desc.count_buffer_offset_bytes % 4U) == 0U;
        result.indirect_count_range_valid =
            result.indirect_count_buffer_usage_ready &&
            count_buffer_range_valid(desc.count_buffer_size_bytes, desc.count_buffer_offset_bytes);
        result.draw_mesh_tasks_indirect_count_eligible =
            result.draw_mesh_tasks_indirect_eligible && result.indirect_count_buffer_usage_ready &&
            result.indirect_count_offset_aligned && result.indirect_count_range_valid;
    }
}

[[nodiscard]] std::string_view
indirect_validation_failure_diagnostic(const VulkanMavgMeshShaderLodDispatchResult& result,
                                       const VulkanMavgMeshShaderLodDispatchDesc& desc) noexcept {
    if (!result.indirect_argument_buffer_usage_ready) {
        return "vulkan_mavg_mesh_shader_lod_indirect_argument_usage_invalid";
    }
    if (!result.indirect_argument_offset_aligned) {
        return "vulkan_mavg_mesh_shader_lod_indirect_argument_offset_unaligned";
    }
    if (!result.indirect_stride_valid) {
        return "vulkan_mavg_mesh_shader_lod_indirect_stride_invalid";
    }
    if (desc.request_indirect_count_draw && desc.max_indirect_count_draws == 0U) {
        return "vulkan_mavg_mesh_shader_lod_indirect_count_max_draws_invalid";
    }
    if (!result.indirect_argument_range_valid) {
        return "vulkan_mavg_mesh_shader_lod_indirect_argument_range_invalid";
    }
    return "vulkan_mavg_mesh_shader_lod_indirect_range_invalid";
}

[[nodiscard]] std::string_view
indirect_count_validation_failure_diagnostic(const VulkanMavgMeshShaderLodDispatchResult& result) noexcept {
    if (!result.indirect_count_buffer_usage_ready) {
        return "vulkan_mavg_mesh_shader_lod_indirect_count_usage_invalid";
    }
    if (!result.indirect_count_offset_aligned) {
        return "vulkan_mavg_mesh_shader_lod_indirect_count_offset_unaligned";
    }
    if (!result.indirect_count_range_valid) {
        return "vulkan_mavg_mesh_shader_lod_indirect_count_range_invalid";
    }
    return "vulkan_mavg_mesh_shader_lod_indirect_count_validation_invalid";
}

void fail(VulkanMavgMeshShaderLodDispatchResult& result, std::uint32_t stage, std::string_view diagnostic) {
    result.failure_stage = stage;
    result.diagnostic_text = std::string{diagnostic};
    ++result.diagnostic_count;
}

[[nodiscard]] std::uint64_t hash_readback_bytes(std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto byte : bytes) {
        hash ^= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(byte));
        hash *= 1099511628211ULL;
    }
    return hash;
}

[[nodiscard]] bool any_readback_byte_nonzero(std::span<const std::byte> bytes) noexcept {
    for (const auto byte : bytes) {
        if (std::to_integer<std::uint8_t>(byte) != 0U) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool workgroup_product_exceeds_limit(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                                   std::uint32_t limit) noexcept {
    if (limit == 0U) {
        return x != 0U || y != 0U || z != 0U;
    }
    if (x == 0U || y == 0U || z == 0U) {
        return false;
    }
    const auto limit64 = static_cast<std::uint64_t>(limit);
    const auto x64 = static_cast<std::uint64_t>(x);
    const auto y64 = static_cast<std::uint64_t>(y);
    const auto z64 = static_cast<std::uint64_t>(z);
    if (x64 > limit64 || y64 > limit64 / x64) {
        return true;
    }
    const auto xy = x64 * y64;
    return z64 > limit64 / xy;
}

} // namespace

VulkanMavgMeshShaderLodCapabilityResult probe_vulkan_mavg_mesh_shader_lod_capability() noexcept {
    VulkanMavgMeshShaderLodCapabilityResult result;

    const VulkanLoaderProbeDesc loader_desc{.host = current_rhi_host_platform()};
    const auto loader = probe_runtime_loader(loader_desc);
    result.loader_available = loader.runtime_loaded && loader.get_instance_proc_addr_found;
    if (!result.loader_available) {
        result.diagnostic_text = "vulkan_loader_unavailable";
        ++result.diagnostic_count;
        return result;
    }

    VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanMavgMeshShaderLodProbe";
    instance_desc.api_version = make_vulkan_api_version(1, 3);

    const auto snapshots = probe_runtime_physical_device_snapshots(loader_desc, instance_desc);
    result.instance_created = snapshots.count_probe.instance.instance_created;
    if (!result.instance_created || snapshots.devices.empty()) {
        result.diagnostic_text =
            snapshots.diagnostic.empty() ? "vulkan_physical_device_snapshot_unavailable" : snapshots.diagnostic;
        ++result.diagnostic_count;
        return result;
    }

    const VulkanRuntimePhysicalDeviceSnapshot* selected = nullptr;
    for (const auto& device : snapshots.devices) {
        if (device.supports_mesh_shader_extension && device.mesh_shader_supported && device.task_shader_supported) {
            selected = &device;
            break;
        }
    }
    if (selected == nullptr) {
        for (const auto& device : snapshots.devices) {
            if (device.supports_mesh_shader_extension) {
                selected = &device;
                break;
            }
        }
    }
    if (selected == nullptr) {
        selected = &snapshots.devices.front();
    }

    result.physical_device_selected = true;
    result.adapter_name = selected->name;
    result.vendor_id = selected->vendor_id;
    result.device_id = selected->device_id;
    result.device_extension_supported = selected->supports_mesh_shader_extension;
    result.feature_query_executed = selected->mesh_shader_feature_queried;
    result.mesh_shader_supported = selected->mesh_shader_supported;
    result.task_shader_supported = selected->task_shader_supported;
    result.mesh_shader_queries_supported = selected->mesh_shader_queries_supported;
    result.max_task_work_group_count_x = selected->max_task_work_group_count_x;
    result.max_task_work_group_count_y = selected->max_task_work_group_count_y;
    result.max_task_work_group_count_z = selected->max_task_work_group_count_z;
    result.max_task_work_group_total_count = selected->max_task_work_group_total_count;
    result.max_mesh_work_group_count_x = selected->max_mesh_work_group_count_x;
    result.max_mesh_work_group_count_y = selected->max_mesh_work_group_count_y;
    result.max_mesh_work_group_count_z = selected->max_mesh_work_group_count_z;
    result.max_mesh_work_group_total_count = selected->max_mesh_work_group_total_count;
    result.max_mesh_output_vertices = selected->max_mesh_output_vertices;
    result.max_mesh_output_primitives = selected->max_mesh_output_primitives;
    result.draw_indirect_count_supported = selected->draw_indirect_count_supported;

    if (!result.device_extension_supported) {
        result.diagnostic_text = "vulkan_ext_mesh_shader_unavailable";
        ++result.diagnostic_count;
    } else if (!result.mesh_shader_supported) {
        result.diagnostic_text = "vulkan_mesh_shader_feature_unavailable";
        ++result.diagnostic_count;
    } else if (!result.task_shader_supported) {
        result.diagnostic_text = "vulkan_task_shader_feature_unavailable";
        ++result.diagnostic_count;
    } else {
        result.diagnostic_text.clear();
    }

    if (result.device_extension_supported && result.mesh_shader_supported && result.task_shader_supported) {
        VulkanLogicalDeviceCreateDesc device_desc;
        device_desc.required_extensions.clear();
        device_desc.require_present_queue = false;
        device_desc.require_mesh_shader = true;
        device_desc.require_task_shader = true;
        device_desc.require_draw_indirect_count = result.draw_indirect_count_supported;

        const auto candidate = make_physical_device_candidate(*selected);
        const auto selection = select_physical_device(device_desc, {candidate});
        const auto plan =
            build_logical_device_create_plan(device_desc, candidate, selection, selected->device_extensions);
        result.mesh_shader_enabled = plan.mesh_shader_enabled;
        result.task_shader_enabled = plan.task_shader_enabled;
        result.draw_indirect_count_enabled = plan.draw_indirect_count_enabled;
        if (!plan.supported) {
            result.diagnostic_text = plan.diagnostic;
            ++result.diagnostic_count;
            return result;
        }

        const auto runtime_device = create_runtime_device(loader_desc, instance_desc, device_desc);
        if (!runtime_device.created) {
            result.diagnostic_text = runtime_device.diagnostic.empty() ? "vulkan_mesh_shader_device_create_failed"
                                                                       : runtime_device.diagnostic;
            ++result.diagnostic_count;
            return result;
        }

        result.mesh_shader_enabled = runtime_device.device.logical_device_plan().mesh_shader_enabled;
        result.task_shader_enabled = runtime_device.device.logical_device_plan().task_shader_enabled;
        result.draw_indirect_count_enabled = runtime_device.device.logical_device_plan().draw_indirect_count_enabled;
        result.draw_mesh_tasks_direct_command_available =
            command_resolved(runtime_device.device.command_plan(), "vkCmdDrawMeshTasksEXT");
        result.draw_mesh_tasks_indirect_command_available =
            command_resolved(runtime_device.device.command_plan(), "vkCmdDrawMeshTasksIndirectEXT");
        result.draw_mesh_tasks_indirect_count_command_available =
            command_resolved(runtime_device.device.command_plan(), "vkCmdDrawMeshTasksIndirectCountEXT");
    }

    return result;
}

VulkanMavgMeshShaderLodDispatchResult
execute_vulkan_mavg_mesh_shader_lod(const VulkanMavgMeshShaderLodDispatchDesc& desc) noexcept {
    VulkanMavgMeshShaderLodDispatchResult result;
    result.mavg_mesh_shader_lod_vulkan_ready = false;
    result.fallback_indexed_draw_preserved = desc.allow_conventional_indexed_fallback;
    result.fallback_indexed_draw_promoted_readiness = false;

    if (desc.task_rows.empty()) {
        fail(result, 1U, "vulkan_mavg_mesh_shader_lod_empty_task_rows");
        return result;
    }
    if (!valid_task_rows(desc.task_rows)) {
        fail(result, 2U, "vulkan_mavg_mesh_shader_lod_invalid_task_row");
        return result;
    }
    if (desc.request_indirect_draw || desc.request_indirect_count_draw) {
        populate_indirect_validation(result, desc);
    }
    if (desc.request_indirect_draw && !result.draw_mesh_tasks_indirect_eligible) {
        fail(result, 3U, indirect_validation_failure_diagnostic(result, desc));
        return result;
    }
    if (desc.request_indirect_count_draw && !result.draw_mesh_tasks_indirect_eligible) {
        fail(result, 3U, indirect_validation_failure_diagnostic(result, desc));
        return result;
    }
    if (desc.request_indirect_count_draw && !result.draw_mesh_tasks_indirect_count_eligible) {
        fail(result, 3U, indirect_count_validation_failure_diagnostic(result));
        return result;
    }

    const auto capability = probe_vulkan_mavg_mesh_shader_lod_capability();
    result.feature_query_executed = capability.feature_query_executed;
    result.vulkan_mesh_shader_supported = capability.mesh_shader_supported;
    result.vulkan_task_shader_supported = capability.task_shader_supported;
    result.mesh_shader_enabled = capability.mesh_shader_enabled;
    result.task_shader_enabled = capability.task_shader_enabled;
    result.adapter_name = capability.adapter_name;

    if (!capability.feature_query_executed || !capability.device_extension_supported ||
        !capability.mesh_shader_supported) {
        result.host_gated = true;
        fail(result, 4U,
             capability.diagnostic_text.empty() ? "vulkan_mesh_shader_host_gated" : capability.diagnostic_text);
        return result;
    }

    if (!capability.mesh_shader_enabled || !capability.task_shader_enabled) {
        result.host_gated = true;
        fail(result, 5U, "vulkan_mesh_shader_feature_enable_path_missing");
        return result;
    }

    if (!capability.draw_mesh_tasks_direct_command_available ||
        !capability.draw_mesh_tasks_indirect_command_available) {
        result.host_gated = true;
        fail(result, 5U, "vulkan_mesh_shader_draw_command_unavailable");
        return result;
    }
    if (desc.request_indirect_count_draw &&
        (!capability.draw_indirect_count_supported || !capability.draw_indirect_count_enabled ||
         !capability.draw_mesh_tasks_indirect_count_command_available)) {
        result.host_gated = true;
        result.mavg_mesh_shader_lod_vulkan_indirect_count_host_gated = true;
        fail(result, 5U, "vulkan_mesh_shader_draw_indirect_count_host_gated");
        return result;
    }

    if (desc.task_shader_spirv.empty() || desc.mesh_shader_spirv.empty() || desc.fragment_shader_spirv.empty()) {
        result.host_gated = true;
        fail(result, 6U, "vulkan_mesh_shader_lod_spirv_artifacts_missing");
        return result;
    }

    if (desc.render_width == 0U || desc.render_height == 0U) {
        fail(result, 7U, "vulkan_mesh_shader_lod_render_extent_invalid");
        return result;
    }
    const auto& first_row = desc.task_rows.front();
    if (first_row.task_group_count_x > capability.max_task_work_group_count_x ||
        first_row.task_group_count_y > capability.max_task_work_group_count_y ||
        first_row.task_group_count_z > capability.max_task_work_group_count_z ||
        workgroup_product_exceeds_limit(first_row.task_group_count_x, first_row.task_group_count_y,
                                        first_row.task_group_count_z, capability.max_task_work_group_total_count)) {
        result.host_gated = true;
        fail(result, 7U, "vulkan_mesh_shader_lod_task_workgroup_limits_exceeded");
        return result;
    }
    if (first_row.output_vertex_count > capability.max_mesh_output_vertices ||
        first_row.output_primitive_count > capability.max_mesh_output_primitives) {
        result.host_gated = true;
        fail(result, 7U, "vulkan_mesh_shader_lod_mesh_output_limits_exceeded");
        return result;
    }

    const VulkanLoaderProbeDesc loader_desc{.host = current_rhi_host_platform()};
    VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanMavgMeshShaderLodExecution";
    instance_desc.api_version = make_vulkan_api_version(1, 3);

    VulkanLogicalDeviceCreateDesc device_desc;
    device_desc.require_present_queue = false;
    device_desc.require_mesh_shader = true;
    device_desc.require_task_shader = true;
    device_desc.require_draw_indirect_count = desc.request_indirect_count_draw;

    auto runtime_device = create_runtime_device(loader_desc, instance_desc, device_desc);
    if (!runtime_device.created) {
        result.host_gated = true;
        fail(result, 7U,
             runtime_device.diagnostic.empty() ? "vulkan_mesh_shader_lod_runtime_device_unavailable"
                                               : runtime_device.diagnostic);
        return result;
    }
    result.mesh_shader_enabled = runtime_device.device.logical_device_plan().mesh_shader_enabled;
    result.task_shader_enabled = runtime_device.device.logical_device_plan().task_shader_enabled;
    if (!result.mesh_shader_enabled || !result.task_shader_enabled) {
        result.host_gated = true;
        fail(result, 7U, "vulkan_mesh_shader_lod_runtime_device_features_not_enabled");
        return result;
    }

    const auto color_format = Format::rgba8_unorm;
    VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = Extent2D{.width = desc.render_width, .height = desc.render_height};
    rendering_desc.color_attachments.push_back(VulkanDynamicRenderingColorAttachmentDesc{
        .format = color_format,
        .load_action = LoadAction::clear,
        .store_action = StoreAction::store,
    });
    const auto dynamic_rendering = build_dynamic_rendering_plan(rendering_desc, runtime_device.device.command_plan());
    if (!dynamic_rendering.supported) {
        result.host_gated = true;
        fail(result, 7U,
             dynamic_rendering.diagnostic.empty() ? "vulkan_mesh_shader_lod_dynamic_rendering_unavailable"
                                                  : dynamic_rendering.diagnostic);
        return result;
    }

    DescriptorSetLayoutDesc payload_descriptor_layout_desc;
    payload_descriptor_layout_desc.bindings = {
        DescriptorBindingDesc{
            .binding = 0,
            .type = DescriptorType::storage_buffer,
            .count = 1,
            .stages = ShaderStageVisibility::task | ShaderStageVisibility::mesh,
        },
    };
    auto payload_descriptor_layout = create_runtime_descriptor_set_layout(
        runtime_device.device, VulkanRuntimeDescriptorSetLayoutDesc{.layout = payload_descriptor_layout_desc});
    if (!payload_descriptor_layout.created) {
        fail(result, 7U,
             payload_descriptor_layout.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_descriptor_layout_failed"
                                                          : payload_descriptor_layout.diagnostic);
        return result;
    }
    auto payload_descriptor_set =
        create_runtime_descriptor_set(runtime_device.device, payload_descriptor_layout.layout);
    if (!payload_descriptor_set.created) {
        fail(result, 7U,
             payload_descriptor_set.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_descriptor_set_failed"
                                                       : payload_descriptor_set.diagnostic);
        return result;
    }

    auto pipeline_layout = create_runtime_pipeline_layout(
        runtime_device.device,
        VulkanRuntimePipelineLayoutDesc{.descriptor_set_layouts = {&payload_descriptor_layout.layout}});
    if (!pipeline_layout.created) {
        fail(result, 7U,
             pipeline_layout.diagnostic.empty() ? "vulkan_mesh_shader_lod_pipeline_layout_failed"
                                                : pipeline_layout.diagnostic);
        return result;
    }

    auto mesh_pipeline =
        create_runtime_mesh_graphics_pipeline(runtime_device.device, pipeline_layout.layout,
                                              VulkanRuntimeMeshGraphicsPipelineDesc{
                                                  .dynamic_rendering = dynamic_rendering,
                                                  .color_format = color_format,
                                                  .depth_format = Format::unknown,
                                                  .task_shader_spirv = desc.task_shader_spirv,
                                                  .mesh_shader_spirv = desc.mesh_shader_spirv,
                                                  .fragment_shader_spirv = desc.fragment_shader_spirv,
                                                  .task_entry_point = desc.task_shader_entry_point,
                                                  .mesh_entry_point = desc.mesh_shader_entry_point,
                                                  .fragment_entry_point = desc.fragment_shader_entry_point,
                                              });
    if (!mesh_pipeline.created) {
        fail(result, 7U,
             mesh_pipeline.diagnostic.empty() ? "vulkan_mesh_shader_lod_pipeline_create_failed"
                                              : mesh_pipeline.diagnostic);
        return result;
    }
    result.created_mesh_pipeline_state = true;
    result.used_mesh_shader_stage = mesh_pipeline.used_mesh_shader_stage;
    result.used_task_shader_stage = mesh_pipeline.used_task_shader_stage;
    result.used_input_layout = mesh_pipeline.used_vertex_input_state;

    auto render_target = create_runtime_texture(
        runtime_device.device,
        VulkanRuntimeTextureDesc{
            .texture =
                TextureDesc{
                    .extent = Extent3D{.width = desc.render_width, .height = desc.render_height, .depth = 1},
                    .format = color_format,
                    .usage = TextureUsage::render_target | TextureUsage::copy_source,
                },
        });
    if (!render_target.created) {
        fail(result, 7U,
             render_target.diagnostic.empty() ? "vulkan_mesh_shader_lod_render_target_create_failed"
                                              : render_target.diagnostic);
        return result;
    }

    const auto readback_bytes = format_copy_compact_bytes(
        color_format, Extent3D{.width = desc.render_width, .height = desc.render_height, .depth = 1});
    auto readback_buffer = create_runtime_buffer(
        runtime_device.device,
        VulkanRuntimeBufferDesc{
            .buffer = BufferDesc{.size_bytes = readback_bytes, .usage = BufferUsage::copy_destination},
            .memory_domain = VulkanBufferMemoryDomain::readback,
        });
    if (!readback_buffer.created) {
        fail(result, 7U,
             readback_buffer.diagnostic.empty() ? "vulkan_mesh_shader_lod_readback_buffer_create_failed"
                                                : readback_buffer.diagnostic);
        return result;
    }

    auto shader_payload_buffer = create_runtime_buffer(
        runtime_device.device, VulkanRuntimeBufferDesc{
                                   .buffer = BufferDesc{.size_bytes = k_shader_payload_buffer_size_bytes,
                                                        .usage = BufferUsage::storage | BufferUsage::copy_source},
                                   .memory_domain = VulkanBufferMemoryDomain::upload,
                               });
    if (!shader_payload_buffer.created) {
        fail(result, 7U,
             shader_payload_buffer.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_buffer_create_failed"
                                                      : shader_payload_buffer.diagnostic);
        return result;
    }
    const auto shader_payload_bytes = encode_shader_payload_bytes(1U, 1U);
    const auto payload_write = write_runtime_buffer(
        runtime_device.device, shader_payload_buffer.buffer,
        VulkanRuntimeBufferWriteDesc{
            .byte_offset = 0,
            .bytes = std::span<const std::uint8_t>{shader_payload_bytes.data(), shader_payload_bytes.size()},
        });
    if (!payload_write.written) {
        fail(result, 7U,
             payload_write.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_buffer_write_failed"
                                              : payload_write.diagnostic);
        return result;
    }
    const auto payload_descriptor_update = update_runtime_descriptor_set(
        runtime_device.device, payload_descriptor_set.set,
        VulkanRuntimeDescriptorWriteDesc{
            .binding = 0,
            .buffers = {VulkanRuntimeDescriptorBufferResource{.type = DescriptorType::storage_buffer,
                                                              .buffer = &shader_payload_buffer.buffer}},
        });
    if (!payload_descriptor_update.updated) {
        fail(result, 7U,
             payload_descriptor_update.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_descriptor_update_failed"
                                                          : payload_descriptor_update.diagnostic);
        return result;
    }

    VulkanRuntimeBufferCreateResult indirect_argument_buffer;
    VulkanRuntimeBufferCreateResult indirect_count_buffer;
    std::uint32_t indirect_count_value = 0U;
    if (desc.request_indirect_draw || desc.request_indirect_count_draw) {
        indirect_argument_buffer =
            create_runtime_buffer(runtime_device.device, VulkanRuntimeBufferDesc{
                                                             .buffer =
                                                                 BufferDesc{
                                                                     .size_bytes = desc.indirect_buffer_size_bytes,
                                                                     .usage = desc.indirect_buffer_usage,
                                                                 },
                                                             .memory_domain = VulkanBufferMemoryDomain::upload,
                                                         });
        if (!indirect_argument_buffer.created) {
            fail(result, 7U,
                 indirect_argument_buffer.diagnostic.empty()
                     ? "vulkan_mesh_shader_lod_indirect_argument_buffer_create_failed"
                     : indirect_argument_buffer.diagnostic);
            return result;
        }

        const auto indirect_record_count =
            desc.request_indirect_count_draw ? desc.max_indirect_count_draws : desc.indirect_draw_count;
        auto indirect_payload =
            encode_indirect_command_payload(first_row, indirect_record_count, desc.indirect_stride_bytes);
        const auto argument_write = write_runtime_buffer(
            runtime_device.device, indirect_argument_buffer.buffer,
            VulkanRuntimeBufferWriteDesc{
                .byte_offset = desc.indirect_buffer_offset_bytes,
                .bytes = std::span<const std::uint8_t>{indirect_payload.data(), indirect_payload.size()},
            });
        if (!argument_write.written) {
            fail(result, 7U,
                 argument_write.diagnostic.empty() ? "vulkan_mesh_shader_lod_indirect_argument_write_failed"
                                                   : argument_write.diagnostic);
            return result;
        }
    }

    if (desc.request_indirect_count_draw) {
        indirect_count_buffer =
            create_runtime_buffer(runtime_device.device, VulkanRuntimeBufferDesc{
                                                             .buffer =
                                                                 BufferDesc{
                                                                     .size_bytes = desc.count_buffer_size_bytes,
                                                                     .usage = desc.count_buffer_usage,
                                                                 },
                                                             .memory_domain = VulkanBufferMemoryDomain::upload,
                                                         });
        if (!indirect_count_buffer.created) {
            fail(result, 7U,
                 indirect_count_buffer.diagnostic.empty() ? "vulkan_mesh_shader_lod_indirect_count_buffer_create_failed"
                                                          : indirect_count_buffer.diagnostic);
            return result;
        }

        indirect_count_value = std::min(desc.indirect_draw_count, desc.max_indirect_count_draws);
        const auto count_bytes = encode_u32_bytes(indirect_count_value);
        const auto count_write =
            write_runtime_buffer(runtime_device.device, indirect_count_buffer.buffer,
                                 VulkanRuntimeBufferWriteDesc{
                                     .byte_offset = desc.count_buffer_offset_bytes,
                                     .bytes = std::span<const std::uint8_t>{count_bytes.data(), count_bytes.size()},
                                 });
        if (!count_write.written) {
            fail(result, 7U,
                 count_write.diagnostic.empty() ? "vulkan_mesh_shader_lod_indirect_count_write_failed"
                                                : count_write.diagnostic);
            return result;
        }
        result.last_indirect_count_buffer_value = indirect_count_value;
        result.last_indirect_executed_draw_count = indirect_count_value;
    }

    auto command_pool = create_runtime_command_pool(runtime_device.device, VulkanRuntimeCommandPoolDesc{});
    if (!command_pool.created || !command_pool.pool.begin_primary_command_buffer()) {
        fail(result, 7U,
             command_pool.diagnostic.empty() ? "vulkan_mesh_shader_lod_command_buffer_begin_failed"
                                             : command_pool.diagnostic);
        return result;
    }
    const auto barrier_to_render = record_runtime_texture_barrier(
        runtime_device.device, command_pool.pool, render_target.texture,
        VulkanRuntimeTextureBarrierDesc{.before = ResourceState::undefined, .after = ResourceState::render_target});
    if (!barrier_to_render.recorded) {
        fail(result, 7U,
             barrier_to_render.diagnostic.empty() ? "vulkan_mesh_shader_lod_render_barrier_failed"
                                                  : barrier_to_render.diagnostic);
        return result;
    }
    result.resource_barriers_recorded += barrier_to_render.barrier_count;

    if (desc.request_indirect_draw || desc.request_indirect_count_draw) {
        const auto indirect_record_count =
            desc.request_indirect_count_draw ? desc.max_indirect_count_draws : desc.indirect_draw_count;
        const auto argument_barrier = record_runtime_buffer_memory_barrier2(
            runtime_device.device, command_pool.pool,
            VulkanRuntimeBufferMemoryBarrierDesc{
                .buffer = &indirect_argument_buffer.buffer,
                .src_stage_mask = k_vulkan_pipeline_stage2_host_bit,
                .src_access_mask = k_vulkan_access2_host_write_bit,
                .dst_stage_mask = k_vulkan_pipeline_stage2_draw_indirect_bit,
                .dst_access_mask = k_vulkan_access2_indirect_command_read_bit,
                .offset = desc.indirect_buffer_offset_bytes,
                .size = indirect_argument_payload_size_bytes(indirect_record_count, desc.indirect_stride_bytes),
            });
        if (!argument_barrier.recorded) {
            fail(result, 7U,
                 argument_barrier.diagnostic.empty() ? "vulkan_mesh_shader_lod_indirect_argument_barrier_failed"
                                                     : argument_barrier.diagnostic);
            return result;
        }
        result.resource_barriers_recorded += argument_barrier.barrier_count;
        result.draw_indirect_stage_barriers_recorded += argument_barrier.barrier_count;
    }

    if (desc.request_indirect_count_draw) {
        const auto count_barrier =
            record_runtime_buffer_memory_barrier2(runtime_device.device, command_pool.pool,
                                                  VulkanRuntimeBufferMemoryBarrierDesc{
                                                      .buffer = &indirect_count_buffer.buffer,
                                                      .src_stage_mask = k_vulkan_pipeline_stage2_host_bit,
                                                      .src_access_mask = k_vulkan_access2_host_write_bit,
                                                      .dst_stage_mask = k_vulkan_pipeline_stage2_draw_indirect_bit,
                                                      .dst_access_mask = k_vulkan_access2_indirect_command_read_bit,
                                                      .offset = desc.count_buffer_offset_bytes,
                                                      .size = sizeof(std::uint32_t),
                                                  });
        if (!count_barrier.recorded) {
            fail(result, 7U,
                 count_barrier.diagnostic.empty() ? "vulkan_mesh_shader_lod_indirect_count_barrier_failed"
                                                  : count_barrier.diagnostic);
            return result;
        }
        result.resource_barriers_recorded += count_barrier.barrier_count;
        result.draw_indirect_stage_barriers_recorded += count_barrier.barrier_count;
    }

    const auto payload_shader_barrier =
        record_runtime_buffer_memory_barrier2(runtime_device.device, command_pool.pool,
                                              VulkanRuntimeBufferMemoryBarrierDesc{
                                                  .buffer = &shader_payload_buffer.buffer,
                                                  .src_stage_mask = k_vulkan_pipeline_stage2_host_bit,
                                                  .src_access_mask = k_vulkan_access2_host_write_bit,
                                                  .dst_stage_mask = k_vulkan_pipeline_stage2_task_shader_bit_ext |
                                                                    k_vulkan_pipeline_stage2_mesh_shader_bit_ext,
                                                  .dst_access_mask = k_vulkan_access2_shader_read_bit,
                                                  .offset = 0,
                                                  .size = k_shader_payload_buffer_size_bytes,
                                              });
    if (!payload_shader_barrier.recorded) {
        fail(result, 7U,
             payload_shader_barrier.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_barrier_failed"
                                                       : payload_shader_barrier.diagnostic);
        return result;
    }
    result.resource_barriers_recorded += payload_shader_barrier.barrier_count;
    result.task_shader_stage_barriers_recorded += payload_shader_barrier.barrier_count;
    result.mesh_shader_stage_barriers_recorded += payload_shader_barrier.barrier_count;

    const auto payload_descriptor_bind = record_runtime_descriptor_set_binding(
        runtime_device.device, command_pool.pool, pipeline_layout.layout, payload_descriptor_set.set,
        VulkanRuntimeDescriptorSetBindDesc{.first_set = 0U, .pipeline_bind_point = 0U});
    if (!payload_descriptor_bind.recorded) {
        fail(result, 7U,
             payload_descriptor_bind.diagnostic.empty() ? "vulkan_mesh_shader_lod_payload_descriptor_bind_failed"
                                                        : payload_descriptor_bind.diagnostic);
        return result;
    }

    const auto draw_result = record_runtime_texture_rendering_mesh_tasks_draw(
        runtime_device.device, command_pool.pool, render_target.texture, mesh_pipeline.pipeline,
        VulkanRuntimeTextureRenderingMeshTasksDrawDesc{
            .dynamic_rendering = dynamic_rendering,
            .group_count_x = first_row.task_group_count_x,
            .group_count_y = first_row.task_group_count_y,
            .group_count_z = first_row.task_group_count_z,
            .mesh_tasks_indirect_draw = desc.request_indirect_draw,
            .mesh_tasks_indirect_count_draw = desc.request_indirect_count_draw,
            .indirect_argument_buffer = (desc.request_indirect_draw || desc.request_indirect_count_draw)
                                            ? &indirect_argument_buffer.buffer
                                            : nullptr,
            .indirect_argument_buffer_offset = desc.indirect_buffer_offset_bytes,
            .indirect_count_buffer = desc.request_indirect_count_draw ? &indirect_count_buffer.buffer : nullptr,
            .indirect_count_buffer_offset = desc.count_buffer_offset_bytes,
            .indirect_draw_count =
                desc.request_indirect_count_draw ? desc.max_indirect_count_draws : desc.indirect_draw_count,
            .indirect_command_stride_bytes = desc.indirect_stride_bytes,
            .color_load_action = LoadAction::clear,
            .color_store_action = StoreAction::store,
            .clear_color = ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
        });
    if (!draw_result.recorded) {
        fail(result, 7U,
             draw_result.diagnostic.empty() ? "vulkan_mesh_shader_lod_draw_record_failed" : draw_result.diagnostic);
        return result;
    }
    result.draw_mesh_tasks_direct_calls = draw_result.direct_draw_calls;
    result.draw_mesh_tasks_indirect_calls = draw_result.indirect_draw_calls;
    result.draw_mesh_tasks_indirect_count_calls = draw_result.indirect_count_draw_calls;

    const auto barrier_to_copy = record_runtime_texture_barrier(
        runtime_device.device, command_pool.pool, render_target.texture,
        VulkanRuntimeTextureBarrierDesc{.before = ResourceState::render_target, .after = ResourceState::copy_source});
    if (!barrier_to_copy.recorded) {
        fail(result, 7U,
             barrier_to_copy.diagnostic.empty() ? "vulkan_mesh_shader_lod_copy_barrier_failed"
                                                : barrier_to_copy.diagnostic);
        return result;
    }
    result.resource_barriers_recorded += barrier_to_copy.barrier_count;

    const auto copy_result = record_runtime_texture_buffer_copy(
        runtime_device.device, command_pool.pool, render_target.texture, readback_buffer.buffer,
        VulkanRuntimeTextureBufferCopyDesc{
            .region =
                BufferTextureCopyRegion{
                    .buffer_offset = 0,
                    .buffer_row_length = 0,
                    .buffer_image_height = 0,
                    .texture_offset = Offset3D{},
                    .texture_extent = Extent3D{.width = desc.render_width, .height = desc.render_height, .depth = 1},
                },
        });
    if (!copy_result.recorded) {
        fail(result, 7U,
             copy_result.diagnostic.empty() ? "vulkan_mesh_shader_lod_readback_copy_failed" : copy_result.diagnostic);
        return result;
    }

    if (!command_pool.pool.end_primary_command_buffer()) {
        fail(result, 7U, "vulkan_mesh_shader_lod_command_buffer_end_failed");
        return result;
    }
    auto frame_sync = create_runtime_frame_sync(runtime_device.device,
                                                VulkanRuntimeFrameSyncDesc{.create_image_available_semaphore = false,
                                                                           .create_render_finished_semaphore = false,
                                                                           .create_in_flight_fence = true,
                                                                           .start_in_flight_fence_signaled = false});
    if (!frame_sync.created) {
        fail(result, 7U,
             frame_sync.diagnostic.empty() ? "vulkan_mesh_shader_lod_frame_sync_create_failed" : frame_sync.diagnostic);
        return result;
    }
    const auto submit =
        submit_runtime_command_buffer(runtime_device.device, command_pool.pool, frame_sync.sync,
                                      VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = false,
                                                                           .signal_render_finished_semaphore = false,
                                                                           .signal_in_flight_fence = true,
                                                                           .wait_for_graphics_queue_idle = true});
    if (!submit.submitted) {
        fail(result, 7U, submit.diagnostic.empty() ? "vulkan_mesh_shader_lod_submit_failed" : submit.diagnostic);
        return result;
    }

    const auto readback = read_runtime_buffer(runtime_device.device, readback_buffer.buffer,
                                              VulkanRuntimeBufferReadDesc{.byte_count = readback_bytes});
    if (!readback.read) {
        fail(result, 7U, readback.diagnostic.empty() ? "vulkan_mesh_shader_lod_readback_failed" : readback.diagnostic);
        return result;
    }

    result.executed_mesh_shader = draw_result.drew;
    result.readback_nonzero = any_readback_byte_nonzero(readback.bytes);
    result.readback_hash = hash_readback_bytes(readback.bytes);
    result.shader_payload_consumed_by_task_or_mesh =
        result.task_shader_stage_barriers_recorded > 0U && result.mesh_shader_stage_barriers_recorded > 0U;
    const auto common_execution_ready =
        result.executed_mesh_shader && result.created_mesh_pipeline_state && result.used_mesh_shader_stage &&
        result.used_task_shader_stage && !result.used_input_layout && !result.used_index_buffer &&
        result.readback_nonzero && result.readback_hash != 0U && result.shader_payload_consumed_by_task_or_mesh;
    result.mavg_mesh_shader_lod_vulkan_ready = common_execution_ready && !desc.request_indirect_draw &&
                                               !desc.request_indirect_count_draw &&
                                               result.draw_mesh_tasks_direct_calls == 1U;
    result.mavg_mesh_shader_lod_vulkan_indirect_ready =
        common_execution_ready && desc.request_indirect_draw && !desc.request_indirect_count_draw &&
        result.draw_mesh_tasks_direct_calls == 0U && result.draw_mesh_tasks_indirect_calls == 1U &&
        result.draw_indirect_stage_barriers_recorded > 0U;
    result.mavg_mesh_shader_lod_vulkan_indirect_count_ready =
        common_execution_ready && desc.request_indirect_count_draw && result.draw_mesh_tasks_direct_calls == 0U &&
        result.draw_mesh_tasks_indirect_count_calls == 1U &&
        result.last_indirect_count_buffer_value <= desc.max_indirect_count_draws &&
        result.last_indirect_executed_draw_count ==
            std::min(result.last_indirect_count_buffer_value, desc.max_indirect_count_draws) &&
        result.draw_indirect_stage_barriers_recorded > 1U;
    result.succeeded = result.mavg_mesh_shader_lod_vulkan_ready || result.mavg_mesh_shader_lod_vulkan_indirect_ready ||
                       result.mavg_mesh_shader_lod_vulkan_indirect_count_ready;
    if (!result.succeeded) {
        fail(result, 7U, "vulkan_mesh_shader_lod_readback_not_ready");
        return result;
    }
    if (result.mavg_mesh_shader_lod_vulkan_indirect_count_ready) {
        result.diagnostic_text = "vulkan_mesh_shader_lod_indirect_count_execution_ready";
    } else if (result.mavg_mesh_shader_lod_vulkan_indirect_ready) {
        result.diagnostic_text = "vulkan_mesh_shader_lod_indirect_execution_ready";
    } else {
        result.diagnostic_text = "vulkan_mesh_shader_lod_execution_ready";
    }
    return result;
}

} // namespace mirakana::rhi::vulkan
