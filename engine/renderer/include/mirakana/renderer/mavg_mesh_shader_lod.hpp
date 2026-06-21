// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/mavg_lod_selection.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgMeshShaderLodDiagnosticCode : std::uint8_t {
    invalid_selection,
    no_selected_clusters,
    missing_material_root,
    duplicate_material_root,
    mismatched_material_roots,
    missing_meshlet_rows,
    duplicate_meshlet_rows,
    invalid_meshlet_output,
    invalid_meshlet_group_size,
    invalid_fallback_draw_range,
    max_task_rows_exceeded,
};

struct MavgMeshShaderLodMaterialRootRow {
    AssetId graph_asset;
    std::uint32_t material_partition{0};
    AssetId material_root;
};

struct MavgMeshShaderLodMeshletRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t meshlet_index{0};
    std::uint32_t local_meshlet_index{0};
    std::uint32_t output_vertex_count{0};
    std::uint32_t output_primitive_count{0};
    std::uint32_t group_thread_count{0};
};

struct MavgMeshShaderLodDiagnostic {
    MavgMeshShaderLodDiagnosticCode code{MavgMeshShaderLodDiagnosticCode::invalid_selection};
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::string message;
};

struct MavgMeshShaderLodTaskRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    AssetId material_root;
    std::uint32_t meshlet_index{0};
    std::uint32_t local_meshlet_index{0};
    std::uint32_t output_vertex_count{0};
    std::uint32_t output_primitive_count{0};
    std::uint32_t group_thread_count{0};
    std::uint32_t fallback_first_index{0};
    std::uint32_t fallback_index_count{0};
    std::int32_t fallback_vertex_base{0};
    bool fallback_substitution{false};
};

struct MavgMeshShaderLodFallbackDrawRow {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    bool fallback_substitution{false};
    bool uses_index_buffer{true};
};

struct MavgMeshShaderLodDesc {
    const MavgLodSelectionResult* selection{nullptr};
    std::span<const MavgMeshShaderLodMaterialRootRow> material_roots;
    std::span<const MavgMeshShaderLodMeshletRow> meshlets;
    std::uint32_t max_task_rows{0};
    std::uint32_t expected_group_thread_count{32};
    bool require_material_root_consistency{true};
};

struct MavgMeshShaderLodPlan {
    std::vector<MavgMeshShaderLodTaskRow> task_rows;
    std::vector<MavgMeshShaderLodFallbackDrawRow> fallback_draws;
    std::vector<MavgMeshShaderLodDiagnostic> diagnostics;
    std::uint32_t selected_cluster_count{0};
    std::uint32_t meshlet_task_count{0};
    std::uint32_t fallback_draw_count{0};
    bool uses_mesh_shader_bind_points{false};
    bool uses_amplification_shader_bind_point{false};
    bool requires_input_assembler{false};
    bool requires_index_buffer{false};
    bool fallback_to_conventional_indexed_draws{false};
    bool fallback_promotes_mesh_shader_lod_ready{false};
    bool mavg_mesh_shader_lod_ready{false};
    bool executed_mesh_shader{false};
    bool executed_d3d12{false};
    bool executed_vulkan{false};
    bool claimed_metal_readiness{false};
    bool claimed_nanite_equivalence{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgMeshShaderLodPlan plan_mavg_mesh_shader_lod_tasks(const MavgMeshShaderLodDesc& desc);

[[nodiscard]] bool has_mavg_mesh_shader_lod_diagnostic(const MavgMeshShaderLodPlan& plan,
                                                       MavgMeshShaderLodDiagnosticCode code) noexcept;

} // namespace mirakana
