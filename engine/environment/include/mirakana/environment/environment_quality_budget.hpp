// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class EnvironmentQualityBudgetStatus : std::uint8_t {
    ready = 0,
    budget_exceeded,
    invalid_request,
};

enum class EnvironmentQualityBudgetDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_quality_preset,
    missing_custom_limits,
    invalid_custom_limits,
    physical_sky_sample_budget_exceeded,
    height_fog_sample_step_budget_exceeded,
    volumetric_fog_raymarch_step_budget_exceeded,
    volumetric_cloud_primary_step_budget_exceeded,
    volumetric_cloud_light_step_budget_exceeded,
    precipitation_particle_budget_exceeded,
    transient_gpu_byte_budget_exceeded,
    framegraph_pass_budget_exceeded,
    framegraph_barrier_step_budget_exceeded,
    texture_upload_budget_exceeded,
    renderer_draw_budget_exceeded,
    compute_dispatch_budget_exceeded,
    diagnostics_budget_exceeded,
    native_handle_access,
    broad_optimization_claimed,
};

struct EnvironmentQualityBudgetLimitsDesc {
    std::uint32_t physical_sky_sample_budget{0};
    std::uint32_t height_fog_sample_step_budget{0};
    std::uint32_t volumetric_fog_raymarch_step_budget{0};
    std::uint32_t volumetric_cloud_primary_step_budget{0};
    std::uint32_t volumetric_cloud_light_step_budget{0};
    std::uint32_t precipitation_particle_budget{0};
    std::uint64_t transient_gpu_byte_budget{0};
    std::uint32_t framegraph_pass_budget{0};
    std::uint32_t framegraph_barrier_step_budget{0};
    std::uint32_t texture_upload_budget{0};
    std::uint32_t renderer_draw_budget{0};
    std::uint32_t compute_dispatch_budget{0};
    std::uint32_t diagnostics_budget{0};
};

struct EnvironmentQualityBudgetUsageDesc {
    std::uint32_t physical_sky_sample_budget{0};
    std::uint32_t height_fog_sample_step_budget{0};
    std::uint32_t volumetric_fog_raymarch_step_budget{0};
    std::uint32_t volumetric_cloud_primary_step_budget{0};
    std::uint32_t volumetric_cloud_light_step_budget{0};
    std::uint32_t precipitation_particle_rows{0};
    std::uint64_t transient_gpu_byte_estimate{0};
    std::uint32_t framegraph_passes{0};
    std::uint32_t framegraph_barrier_steps{0};
    std::uint32_t texture_uploads{0};
    std::uint32_t renderer_draws{0};
    std::uint32_t compute_dispatches{0};
    std::uint32_t diagnostics{0};
    bool native_handle_access{false};
    bool broad_optimization_claimed{false};
};

struct EnvironmentQualityBudgetRequest {
    EnvironmentQualityPreset preset{EnvironmentQualityPreset::medium};
    EnvironmentQualityBudgetUsageDesc usage;
    EnvironmentQualityBudgetLimitsDesc custom_limits;
    bool custom_limits_provided{false};
};

struct EnvironmentQualityBudgetDiagnostic {
    EnvironmentQualityBudgetDiagnosticCode code{EnvironmentQualityBudgetDiagnosticCode::none};
    std::string field;
    std::uint64_t value{0};
    std::uint64_t budget{0};
    std::string message;
};

struct EnvironmentQualityBudgetPlan {
    EnvironmentQualityBudgetStatus status{EnvironmentQualityBudgetStatus::invalid_request};
    EnvironmentQualityPreset preset{EnvironmentQualityPreset::medium};
    EnvironmentQualityBudgetLimitsDesc limits;
    EnvironmentQualityBudgetUsageDesc usage;
    std::vector<EnvironmentQualityBudgetDiagnostic> diagnostics;
    bool ready{false};
};

[[nodiscard]] std::string_view environment_quality_budget_status_name(EnvironmentQualityBudgetStatus status) noexcept;
[[nodiscard]] std::string_view
environment_quality_budget_diagnostic_code_name(EnvironmentQualityBudgetDiagnosticCode code) noexcept;

[[nodiscard]] EnvironmentQualityBudgetLimitsDesc
environment_quality_budget_preset_limits(EnvironmentQualityPreset preset);

[[nodiscard]] EnvironmentQualityBudgetPlan
evaluate_environment_quality_budget(const EnvironmentQualityBudgetRequest& request);

[[nodiscard]] bool has_environment_quality_budget_diagnostic(const EnvironmentQualityBudgetPlan& plan,
                                                             EnvironmentQualityBudgetDiagnosticCode code) noexcept;

} // namespace mirakana
