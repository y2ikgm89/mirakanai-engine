// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class SceneScaleDrawGroupKind : std::uint8_t {
    unknown = 0,
    sprite,
    static_mesh,
    skinned_mesh,
    morph_mesh,
};

enum class SceneScaleCullingMode : std::uint8_t {
    none = 0,
    cpu_frustum,
    gpu_indirect,
};

enum class SceneScaleBatchingMode : std::uint8_t {
    none = 0,
    instanced_draw,
    gpu_indirect,
};

enum class SceneScaleLodMode : std::uint8_t {
    none = 0,
    distance_band,
    screen_size,
    gpu_driven,
};

enum class SceneScaleDiagnosticCode : std::uint8_t {
    none = 0,
    no_draw_groups,
    invalid_frame_extent,
    too_many_draw_groups,
    invalid_draw_group,
    zero_instance_count,
    invalid_visible_instance_count,
    missing_scene_resources,
    unsupported_batching_mode,
    unsupported_culling_mode,
    unsupported_lod_mode,
    invalid_lod_count,
    too_many_visible_instances,
    too_many_draw_calls,
    missing_backend_instancing_evidence,
    missing_performance_measurement,
};

struct SceneScaleDrawGroupDesc {
    SceneScaleDrawGroupKind kind{SceneScaleDrawGroupKind::unknown};
    std::uint32_t instance_count{0};
    std::uint32_t visible_instance_count{0};
    SceneScaleCullingMode culling{SceneScaleCullingMode::none};
    SceneScaleBatchingMode batching{SceneScaleBatchingMode::none};
    SceneScaleLodMode lod{SceneScaleLodMode::none};
    std::uint32_t lod_count{1};
    bool scene_resources_available{true};
    std::uint32_t source_index{0};
};

struct SceneScalePolicyDesc {
    std::span<const SceneScaleDrawGroupDesc> draw_groups;
    Extent2D frame_extent;
    std::uint32_t max_draw_group_count{128};
    std::uint32_t max_visible_instance_count{16'384};
    std::uint32_t max_draw_call_count{4'096};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool require_backend_instancing_evidence{false};
    bool backend_instancing_evidence_ready{false};
    bool require_performance_measurement{false};
    bool performance_measurement_ready{false};
};

struct SceneScaleDrawGroupRow {
    SceneScaleDrawGroupKind kind{SceneScaleDrawGroupKind::unknown};
    std::uint32_t requested_instance_count{0};
    std::uint32_t visible_instance_count{0};
    std::uint32_t culled_instance_count{0};
    std::uint32_t draw_call_count{0};
    std::uint32_t instanced_visible_instance_count{0};
    SceneScaleCullingMode culling{SceneScaleCullingMode::none};
    SceneScaleBatchingMode batching{SceneScaleBatchingMode::none};
    SceneScaleLodMode lod{SceneScaleLodMode::none};
    std::uint32_t lod_count{1};
    bool uses_instancing{false};
    std::uint32_t source_index{0};
};

struct SceneScaleDiagnostic {
    SceneScaleDiagnosticCode code{SceneScaleDiagnosticCode::none};
    std::size_t group_index{0};
    std::uint32_t source_index{0};
    std::string message;
};

struct SceneScalePolicyPlan {
    std::uint32_t draw_group_count{0};
    std::uint64_t requested_instance_count{0};
    std::uint64_t visible_instance_count{0};
    std::uint64_t culled_instance_count{0};
    std::uint32_t draw_call_count{0};
    std::uint32_t instanced_draw_call_count{0};
    std::uint64_t instanced_visible_instance_count{0};
    std::uint32_t lod_group_count{0};
    std::uint32_t cpu_culling_group_count{0};
    Extent2D frame_extent;
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool backend_instancing_evidence_ready{false};
    bool performance_measurement_ready{false};
    std::vector<SceneScaleDrawGroupRow> draw_group_rows;
    std::vector<SceneScaleDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] SceneScalePolicyPlan plan_scene_scale_policy(const SceneScalePolicyDesc& desc);

[[nodiscard]] bool has_scene_scale_policy_diagnostic(const SceneScalePolicyPlan& plan,
                                                     SceneScaleDiagnosticCode code) noexcept;

} // namespace mirakana
