// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/vulkan/vulkan_backend.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace mirakana::rhi::vulkan {

inline constexpr std::uint32_t vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes = 12U;

struct VulkanMavgMeshShaderLodCapabilityResult {
    bool loader_available{false};
    bool instance_created{false};
    bool physical_device_selected{false};
    bool device_extension_supported{false};
    bool feature_query_executed{false};
    bool mesh_shader_supported{false};
    bool task_shader_supported{false};
    bool mesh_shader_queries_supported{false};
    bool mesh_shader_enabled{false};
    bool task_shader_enabled{false};
    bool draw_indirect_count_supported{false};
    bool draw_indirect_count_enabled{false};
    bool draw_mesh_tasks_direct_command_available{false};
    bool draw_mesh_tasks_indirect_command_available{false};
    bool draw_mesh_tasks_indirect_count_command_available{false};
    std::uint32_t max_task_work_group_count_x{0};
    std::uint32_t max_task_work_group_count_y{0};
    std::uint32_t max_task_work_group_count_z{0};
    std::uint32_t max_task_work_group_total_count{0};
    std::uint32_t max_mesh_work_group_count_x{0};
    std::uint32_t max_mesh_work_group_count_y{0};
    std::uint32_t max_mesh_work_group_count_z{0};
    std::uint32_t max_mesh_work_group_total_count{0};
    std::uint32_t max_mesh_output_vertices{0};
    std::uint32_t max_mesh_output_primitives{0};
    std::uint32_t vendor_id{0};
    std::uint32_t device_id{0};
    std::string adapter_name;
    std::string diagnostic_text;
    std::uint32_t diagnostic_count{0};
    bool exposed_native_handles{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_metal_readiness{false};
};

struct VulkanMavgMeshShaderLodTaskRow {
    std::uint32_t cluster_index{0};
    std::uint32_t meshlet_index{0};
    std::uint32_t output_vertex_count{0};
    std::uint32_t output_primitive_count{0};
    std::uint32_t task_group_count_x{0};
    std::uint32_t task_group_count_y{0};
    std::uint32_t task_group_count_z{0};
    std::uint32_t mesh_group_thread_count{0};
    std::uint32_t fallback_index_count{0};
};

struct VulkanMavgMeshShaderLodDispatchDesc {
    std::span<const std::uint32_t> task_shader_spirv;
    std::span<const std::uint32_t> mesh_shader_spirv;
    std::span<const std::uint32_t> fragment_shader_spirv;
    std::string_view task_shader_entry_point{"main"};
    std::string_view mesh_shader_entry_point{"main"};
    std::string_view fragment_shader_entry_point{"main"};
    std::span<const VulkanMavgMeshShaderLodTaskRow> task_rows;
    std::uint32_t render_width{64};
    std::uint32_t render_height{64};
    bool allow_conventional_indexed_fallback{false};
    bool request_indirect_draw{false};
    bool request_indirect_count_draw{false};
    std::uint64_t indirect_buffer_size_bytes{0};
    std::uint64_t indirect_buffer_offset_bytes{0};
    std::uint32_t indirect_draw_count{0};
    std::uint32_t indirect_stride_bytes{vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes};
    std::uint64_t count_buffer_size_bytes{0};
    std::uint64_t count_buffer_offset_bytes{0};
    std::uint32_t max_indirect_count_draws{0};
};

struct VulkanMavgMeshShaderLodDispatchResult {
    bool succeeded{false};
    bool host_gated{false};
    bool feature_query_executed{false};
    bool vulkan_mesh_shader_supported{false};
    bool vulkan_task_shader_supported{false};
    bool mesh_shader_enabled{false};
    bool task_shader_enabled{false};
    bool created_mesh_pipeline_state{false};
    bool used_mesh_shader_stage{false};
    bool used_task_shader_stage{false};
    bool used_input_layout{false};
    bool used_index_buffer{false};
    bool executed_mesh_shader{false};
    bool readback_nonzero{false};
    bool mavg_mesh_shader_lod_vulkan_ready{false};
    bool draw_mesh_tasks_indirect_eligible{false};
    bool draw_mesh_tasks_indirect_count_eligible{false};
    bool mavg_mesh_shader_lod_vulkan_indirect_count_host_gated{false};
    bool fallback_indexed_draw_preserved{false};
    bool fallback_indexed_draw_promoted_readiness{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_metal_readiness{false};
    std::uint32_t draw_mesh_tasks_direct_calls{0};
    std::uint32_t draw_mesh_tasks_indirect_calls{0};
    std::uint32_t draw_mesh_tasks_indirect_count_calls{0};
    std::uint32_t resource_barriers_recorded{0};
    std::uint32_t diagnostic_count{0};
    std::uint32_t failure_stage{0};
    std::uint64_t readback_hash{0};
    std::string adapter_name;
    std::string diagnostic_text;
};

[[nodiscard]] VulkanMavgMeshShaderLodCapabilityResult probe_vulkan_mavg_mesh_shader_lod_capability() noexcept;

[[nodiscard]] VulkanMavgMeshShaderLodDispatchResult
execute_vulkan_mavg_mesh_shader_lod(const VulkanMavgMeshShaderLodDispatchDesc& desc) noexcept;

} // namespace mirakana::rhi::vulkan
