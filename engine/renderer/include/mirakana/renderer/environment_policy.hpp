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

enum class EnvironmentRenderFeature : std::uint8_t {
    unknown = 0,
    physical_sky,
    sky_lighting,
    height_fog,
    volumetric_fog,
    cloud_layer,
    volumetric_clouds,
    precipitation,
    surface_wetness,
};

enum class EnvironmentPolicyDiagnosticCode : std::uint8_t {
    none = 0,
    no_features,
    invalid_frame_extent,
    invalid_feature,
    duplicate_feature,
    missing_scene_color,
    missing_scene_depth,
    missing_backend_shader_evidence,
    missing_backend_validation_evidence,
    excessive_raymarch_step_budget,
    unsupported_backend,
    unsupported_backend_inheritance,
    unsupported_native_handle_claim,
};

struct EnvironmentRenderFeatureDesc {
    EnvironmentRenderFeature feature{EnvironmentRenderFeature::unknown};
    bool enabled{true};
    bool requires_scene_depth{false};
    bool requires_backend_shader_evidence{false};
    bool requires_backend_validation_evidence{false};
    bool backend_shader_evidence_ready{false};
    bool backend_validation_evidence_ready{false};
    std::uint32_t raymarch_step_budget{0};
    std::uint32_t source_index{0};
};

struct EnvironmentPolicyDesc {
    std::span<const EnvironmentRenderFeatureDesc> features;
    Extent2D frame_extent;
    bool scene_color_available{false};
    bool scene_depth_available{false};
    std::uint32_t max_raymarch_step_budget{128};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool request_backend_inheritance{false};
    bool request_native_handle_access{false};
    bool package_counter_evidence_ready{false};
};

struct EnvironmentRenderFeatureRow {
    EnvironmentRenderFeature feature{EnvironmentRenderFeature::unknown};
    bool uses_scene_depth{false};
    bool requires_backend_shader_evidence{false};
    bool requires_backend_validation_evidence{false};
    bool backend_shader_evidence_ready{false};
    bool backend_validation_evidence_ready{false};
    std::uint32_t raymarch_step_budget{0};
    std::uint32_t pass_index{0};
    std::uint32_t source_index{0};
};

struct EnvironmentPolicyDiagnostic {
    EnvironmentPolicyDiagnosticCode code{EnvironmentPolicyDiagnosticCode::none};
    std::size_t feature_index{0};
    std::uint32_t source_index{0};
    std::string message;
};

struct EnvironmentPolicyPlan {
    std::uint32_t feature_count{0};
    std::uint32_t environment_pass_count{0};
    std::uint32_t frame_graph_pass_count{0};
    std::uint32_t frame_graph_barrier_step_budget{0};
    Extent2D frame_extent;
    bool scene_color_required{false};
    bool scene_depth_required{false};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool backend_shader_evidence_ready{false};
    bool backend_validation_evidence_ready{false};
    bool package_counter_evidence_ready{false};
    std::vector<EnvironmentRenderFeatureRow> feature_rows;
    std::vector<EnvironmentPolicyDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentPolicyPlan plan_environment_render_policy(const EnvironmentPolicyDesc& desc);

[[nodiscard]] bool has_environment_policy_diagnostic(const EnvironmentPolicyPlan& plan,
                                                     EnvironmentPolicyDiagnosticCode code) noexcept;

} // namespace mirakana
