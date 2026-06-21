// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace mirakana::rhi::d3d12 {

struct D3d12MavgMeshShaderLodCapabilityResult {
    bool windows_sdk_available{false};
    bool dxgi_factory_created{false};
    bool adapter_selected{false};
    bool device_created{false};
    bool feature_query_executed{false};
    bool mesh_shader_supported{false};
    std::uint32_t mesh_shader_tier{0};
    std::string adapter_name;
    std::string diagnostic_text;
    std::uint32_t diagnostic_count{0};
    bool exposed_native_handles{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_metal_readiness{false};
};

struct D3d12MavgMeshShaderLodTaskRow {
    std::uint32_t cluster_index{0};
    std::uint32_t meshlet_index{0};
    std::uint32_t output_vertex_count{0};
    std::uint32_t output_primitive_count{0};
    std::uint32_t group_thread_count{0};
    std::uint32_t fallback_index_count{0};
};

struct D3d12MavgMeshShaderLodDispatchDesc {
    DeviceBootstrapDesc device{};
    std::span<const D3d12MavgMeshShaderLodTaskRow> task_rows;
    std::uint32_t render_width{64};
    std::uint32_t render_height{64};
};

struct D3d12MavgMeshShaderLodDispatchResult {
    bool succeeded{false};
    bool host_gated{false};
    bool feature_query_executed{false};
    bool d3d12_mesh_shader_supported{false};
    std::uint32_t mesh_shader_tier{0};
    std::string adapter_name;
    std::string amplification_shader_model{"as_6_5"};
    std::string mesh_shader_model{"ms_6_5"};
    std::string pixel_shader_model{"ps_6_0"};
    std::string diagnostic_text;
    bool dxcompiler_available{false};
    bool created_mesh_pipeline_state{false};
    bool used_mesh_shader_bind_point{false};
    bool used_amplification_shader_bind_point{false};
    bool used_input_layout{false};
    bool used_index_buffer{false};
    bool executed_mesh_shader{false};
    bool readback_nonzero{false};
    bool mavg_mesh_shader_lod_d3d12_ready{false};
    bool pipeline_statistics_available{false};
    bool pipeline_statistics_host_gated{true};
    std::uint64_t amplification_shader_invocations{0};
    std::uint64_t mesh_shader_invocations{0};
    std::uint64_t mesh_shader_primitives{0};
    bool claimed_nanite_equivalence{false};
    bool claimed_metal_readiness{false};
    std::uint32_t dispatch_mesh_direct_calls{0};
    std::uint32_t execute_indirect_mesh_dispatch_calls{0};
    std::uint32_t resource_barriers_recorded{0};
    std::uint32_t diagnostic_count{0};
    std::uint32_t failure_stage{0};
    std::uint64_t readback_hash{0};
};

[[nodiscard]] D3d12MavgMeshShaderLodCapabilityResult
probe_d3d12_mavg_mesh_shader_lod_capability(const DeviceBootstrapDesc& desc) noexcept;

[[nodiscard]] D3d12MavgMeshShaderLodDispatchResult
execute_d3d12_mavg_mesh_shader_lod(const D3d12MavgMeshShaderLodDispatchDesc& desc) noexcept;

} // namespace mirakana::rhi::d3d12
