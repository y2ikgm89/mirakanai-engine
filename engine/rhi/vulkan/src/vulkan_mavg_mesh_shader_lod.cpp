// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp"

#include <limits>
#include <string_view>

namespace mirakana::rhi::vulkan {
namespace {

[[nodiscard]] bool extension_available(std::span<const std::string> extensions, std::string_view needle) noexcept {
    for (const auto& extension : extensions) {
        if (extension == needle) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool api_version_at_least(VulkanApiVersion version, std::uint32_t major, std::uint32_t minor) noexcept {
    return version.major > major || (version.major == major && version.minor >= minor);
}

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

void fail(VulkanMavgMeshShaderLodDispatchResult& result, std::uint32_t stage, std::string_view diagnostic) {
    result.failure_stage = stage;
    result.diagnostic_text = std::string{diagnostic};
    ++result.diagnostic_count;
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
    result.draw_indirect_count_supported =
        api_version_at_least(selected->api_version, 1, 2) ||
        extension_available(selected->device_extensions, "VK_KHR_draw_indirect_count") ||
        extension_available(selected->device_extensions, "VK_AMD_draw_indirect_count");

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

        const auto candidate = make_physical_device_candidate(*selected);
        const auto selection = select_physical_device(device_desc, {candidate});
        const auto plan =
            build_logical_device_create_plan(device_desc, candidate, selection, selected->device_extensions);
        result.mesh_shader_enabled = plan.mesh_shader_enabled;
        result.task_shader_enabled = plan.task_shader_enabled;
        result.draw_indirect_count_enabled = false;
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
        result.draw_mesh_tasks_direct_command_available =
            command_resolved(runtime_device.device.command_plan(), "vkCmdDrawMeshTasksEXT");
        result.draw_mesh_tasks_indirect_command_available =
            command_resolved(runtime_device.device.command_plan(), "vkCmdDrawMeshTasksIndirectEXT");
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
    if (desc.request_indirect_draw &&
        !indirect_range_valid(desc.indirect_buffer_size_bytes, desc.indirect_buffer_offset_bytes,
                              desc.indirect_draw_count, desc.indirect_stride_bytes)) {
        fail(result, 3U, "vulkan_mavg_mesh_shader_lod_indirect_range_invalid");
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

    if (desc.task_shader_spirv.empty() || desc.mesh_shader_spirv.empty() || desc.fragment_shader_spirv.empty()) {
        result.host_gated = true;
        fail(result, 6U, "vulkan_mesh_shader_lod_spirv_artifacts_missing");
        return result;
    }

    result.host_gated = true;
    fail(result, 7U, "vulkan_mesh_shader_lod_execution_path_not_promoted");
    return result;
}

} // namespace mirakana::rhi::vulkan
