// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgGpuCullingProducer : std::uint8_t {
    cpu_reference,
    compute_shader,
};

enum class MavgGpuCullingSyncApi : std::uint8_t {
    d3d12,
    vulkan,
};

enum class MavgGpuCullingDiagnosticCode : std::uint8_t {
    invalid_selection,
    no_selected_clusters,
    missing_culling_bounds,
    duplicate_culling_bounds,
    invalid_culling_bounds,
    invalid_instance_count,
    invalid_max_command_count,
    invalid_cluster_draw_range,
    max_command_count_exceeded,
    command_buffer_size_overflow,
};

struct MavgGpuCullingClusterBoundsRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    Vec3 center;
    float radius{0.0F};
    bool visible{true};
};

struct MavgGpuCullingIndirectCommand {
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t start_index_location{0};
    std::int32_t base_vertex_location{0};
    std::uint32_t start_instance_location{0};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    bool fallback_substitution{false};
};

struct MavgGpuCullingIndirectCommandLayout {
    std::uint32_t record_stride_bytes{20};
    std::uint32_t indexed_argument_u32_count{5};
    std::uint32_t argument_buffer_offset_alignment_bytes{4};
    std::uint32_t count_buffer_offset_alignment_bytes{4};
};

struct MavgGpuCullingSyncRequirement {
    MavgGpuCullingSyncApi api{MavgGpuCullingSyncApi::d3d12};
    std::string producer_stage;
    std::string producer_access;
    std::string consumer_stage;
    std::string consumer_access;
    std::string argument_buffer_required_state_or_usage;
    std::string count_buffer_required_state_or_usage;
    std::uint32_t argument_buffer_offset_alignment_bytes{4};
    std::uint32_t count_buffer_offset_alignment_bytes{4};
};

struct MavgGpuCullingDiagnostic {
    MavgGpuCullingDiagnosticCode code{MavgGpuCullingDiagnosticCode::invalid_selection};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::string message;
};

struct MavgGpuCullingDispatchClusterRow {
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t start_index_location{0};
    std::int32_t base_vertex_location{0};
    std::uint32_t start_instance_location{0};
    std::uint32_t visible{0};
    std::uint32_t padding0{0};
    std::uint32_t padding1{0};
};

struct MavgGpuCullingIndirectDesc {
    const MavgLodSelectionResult* selection{nullptr};
    std::span<const MavgGpuCullingClusterBoundsRow> cluster_bounds;
    MavgGpuCullingProducer producer{MavgGpuCullingProducer::cpu_reference};
    std::uint32_t max_command_count{0};
    std::uint32_t instance_count{1};
    std::uint32_t first_instance{0};
    bool require_culling_bounds_for_selected_clusters{true};
};

struct MavgGpuCullingIndirectPlan {
    std::vector<MavgGpuCullingIndirectCommand> commands;
    std::vector<MavgGpuCullingSyncRequirement> sync_requirements;
    std::vector<MavgGpuCullingDiagnostic> diagnostics;
    MavgGpuCullingIndirectCommandLayout command_layout;
    std::uint64_t argument_buffer_size_bytes{0};
    std::uint32_t count_buffer_size_bytes{4};
    std::uint32_t count_buffer_value{0};
    std::uint32_t selected_cluster_count{0};
    std::uint32_t visible_cluster_count{0};
    std::uint32_t culled_cluster_count{0};
    bool prepared_gpu_culling{false};
    bool executed_gpu_culling{false};
    bool executed_indirect_draw{false};
    bool executed_mesh_shader{false};
    bool touched_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgGpuCullingIndirectPlan
plan_mavg_gpu_culling_indirect_commands(const MavgGpuCullingIndirectDesc& desc);

[[nodiscard]] bool has_mavg_gpu_culling_diagnostic(const MavgGpuCullingIndirectPlan& plan,
                                                   MavgGpuCullingDiagnosticCode code) noexcept;

[[nodiscard]] std::vector<MavgGpuCullingDispatchClusterRow>
build_mavg_gpu_culling_dispatch_cluster_rows(const MavgGpuCullingIndirectDesc& desc);

[[nodiscard]] std::vector<std::uint8_t>
encode_mavg_gpu_culling_indirect_argument_buffer_bytes(const MavgGpuCullingIndirectPlan& plan);

[[nodiscard]] std::array<std::uint8_t, 4>
encode_mavg_gpu_culling_indirect_count_buffer_bytes(const MavgGpuCullingIndirectPlan& plan);

} // namespace mirakana
