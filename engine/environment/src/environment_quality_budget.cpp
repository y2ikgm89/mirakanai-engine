// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_quality_budget.hpp"

#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool quality_preset_valid(EnvironmentQualityPreset preset) noexcept {
    switch (preset) {
    case EnvironmentQualityPreset::low:
    case EnvironmentQualityPreset::medium:
    case EnvironmentQualityPreset::high:
    case EnvironmentQualityPreset::ultra:
    case EnvironmentQualityPreset::custom:
        return true;
    }
    return false;
}

[[nodiscard]] bool custom_limits_valid(const EnvironmentQualityBudgetLimitsDesc& limits) noexcept {
    return limits.physical_sky_sample_budget > 0U && limits.height_fog_sample_step_budget > 0U &&
           limits.volumetric_fog_raymarch_step_budget > 0U && limits.volumetric_cloud_primary_step_budget > 0U &&
           limits.volumetric_cloud_light_step_budget > 0U && limits.precipitation_particle_budget > 0U &&
           limits.transient_gpu_byte_budget > 0U && limits.framegraph_pass_budget > 0U &&
           limits.framegraph_barrier_step_budget > 0U && limits.texture_upload_budget > 0U &&
           limits.renderer_draw_budget > 0U && limits.compute_dispatch_budget > 0U;
}

void add_diagnostic(EnvironmentQualityBudgetPlan& plan, EnvironmentQualityBudgetDiagnosticCode code, std::string field,
                    std::uint64_t value, std::uint64_t budget, std::string message) {
    plan.diagnostics.push_back(EnvironmentQualityBudgetDiagnostic{
        .code = code,
        .field = std::move(field),
        .value = value,
        .budget = budget,
        .message = std::move(message),
    });
}

void add_if_exceeded(EnvironmentQualityBudgetPlan& plan, EnvironmentQualityBudgetDiagnosticCode code, std::string field,
                     std::uint64_t value, std::uint64_t budget) {
    if (value <= budget) {
        return;
    }
    add_diagnostic(plan, code, std::move(field), value, budget, "environment quality budget exceeded");
}

} // namespace

std::string_view environment_quality_budget_status_name(EnvironmentQualityBudgetStatus status) noexcept {
    switch (status) {
    case EnvironmentQualityBudgetStatus::ready:
        return "ready";
    case EnvironmentQualityBudgetStatus::budget_exceeded:
        return "budget_exceeded";
    case EnvironmentQualityBudgetStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

std::string_view environment_quality_budget_diagnostic_code_name(EnvironmentQualityBudgetDiagnosticCode code) noexcept {
    switch (code) {
    case EnvironmentQualityBudgetDiagnosticCode::none:
        return "none";
    case EnvironmentQualityBudgetDiagnosticCode::unsupported_quality_preset:
        return "unsupported_quality_preset";
    case EnvironmentQualityBudgetDiagnosticCode::missing_custom_limits:
        return "missing_custom_limits";
    case EnvironmentQualityBudgetDiagnosticCode::invalid_custom_limits:
        return "invalid_custom_limits";
    case EnvironmentQualityBudgetDiagnosticCode::physical_sky_sample_budget_exceeded:
        return "physical_sky_sample_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::height_fog_sample_step_budget_exceeded:
        return "height_fog_sample_step_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::volumetric_fog_raymarch_step_budget_exceeded:
        return "volumetric_fog_raymarch_step_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::volumetric_cloud_primary_step_budget_exceeded:
        return "volumetric_cloud_primary_step_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::volumetric_cloud_light_step_budget_exceeded:
        return "volumetric_cloud_light_step_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::precipitation_particle_budget_exceeded:
        return "precipitation_particle_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::transient_gpu_byte_budget_exceeded:
        return "transient_gpu_byte_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::framegraph_pass_budget_exceeded:
        return "framegraph_pass_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::framegraph_barrier_step_budget_exceeded:
        return "framegraph_barrier_step_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::texture_upload_budget_exceeded:
        return "texture_upload_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::renderer_draw_budget_exceeded:
        return "renderer_draw_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::compute_dispatch_budget_exceeded:
        return "compute_dispatch_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::diagnostics_budget_exceeded:
        return "diagnostics_budget_exceeded";
    case EnvironmentQualityBudgetDiagnosticCode::native_handle_access:
        return "native_handle_access";
    case EnvironmentQualityBudgetDiagnosticCode::broad_optimization_claimed:
        return "broad_optimization_claimed";
    }
    return "unknown";
}

EnvironmentQualityBudgetLimitsDesc environment_quality_budget_preset_limits(EnvironmentQualityPreset preset) {
    switch (preset) {
    case EnvironmentQualityPreset::low:
        return EnvironmentQualityBudgetLimitsDesc{
            .physical_sky_sample_budget = 48,
            .height_fog_sample_step_budget = 16,
            .volumetric_fog_raymarch_step_budget = 24,
            .volumetric_cloud_primary_step_budget = 32,
            .volumetric_cloud_light_step_budget = 4,
            .precipitation_particle_budget = 1,
            .transient_gpu_byte_budget = 8ULL * 1024ULL * 1024ULL,
            .framegraph_pass_budget = 4,
            .framegraph_barrier_step_budget = 12,
            .texture_upload_budget = 4,
            .renderer_draw_budget = 4,
            .compute_dispatch_budget = 1,
            .diagnostics_budget = 0,
        };
    case EnvironmentQualityPreset::medium:
        return EnvironmentQualityBudgetLimitsDesc{
            .physical_sky_sample_budget = 72,
            .height_fog_sample_step_budget = 32,
            .volumetric_fog_raymarch_step_budget = 40,
            .volumetric_cloud_primary_step_budget = 40,
            .volumetric_cloud_light_step_budget = 6,
            .precipitation_particle_budget = 1,
            .transient_gpu_byte_budget = 16ULL * 1024ULL * 1024ULL,
            .framegraph_pass_budget = 6,
            .framegraph_barrier_step_budget = 18,
            .texture_upload_budget = 4,
            .renderer_draw_budget = 4,
            .compute_dispatch_budget = 1,
            .diagnostics_budget = 0,
        };
    case EnvironmentQualityPreset::high:
        return EnvironmentQualityBudgetLimitsDesc{
            .physical_sky_sample_budget = 108,
            .height_fog_sample_step_budget = 48,
            .volumetric_fog_raymarch_step_budget = 64,
            .volumetric_cloud_primary_step_budget = 64,
            .volumetric_cloud_light_step_budget = 8,
            .precipitation_particle_budget = 1,
            .transient_gpu_byte_budget = 32ULL * 1024ULL * 1024ULL,
            .framegraph_pass_budget = 8,
            .framegraph_barrier_step_budget = 24,
            .texture_upload_budget = 6,
            .renderer_draw_budget = 6,
            .compute_dispatch_budget = 2,
            .diagnostics_budget = 0,
        };
    case EnvironmentQualityPreset::ultra:
        return EnvironmentQualityBudgetLimitsDesc{
            .physical_sky_sample_budget = 160,
            .height_fog_sample_step_budget = 64,
            .volumetric_fog_raymarch_step_budget = 96,
            .volumetric_cloud_primary_step_budget = 96,
            .volumetric_cloud_light_step_budget = 16,
            .precipitation_particle_budget = 2,
            .transient_gpu_byte_budget = 64ULL * 1024ULL * 1024ULL,
            .framegraph_pass_budget = 12,
            .framegraph_barrier_step_budget = 36,
            .texture_upload_budget = 8,
            .renderer_draw_budget = 8,
            .compute_dispatch_budget = 4,
            .diagnostics_budget = 0,
        };
    case EnvironmentQualityPreset::custom:
        break;
    }
    return EnvironmentQualityBudgetLimitsDesc{};
}

EnvironmentQualityBudgetPlan evaluate_environment_quality_budget(const EnvironmentQualityBudgetRequest& request) {
    EnvironmentQualityBudgetPlan plan{
        .preset = request.preset,
        .usage = request.usage,
    };

    if (!quality_preset_valid(request.preset)) {
        add_diagnostic(plan, EnvironmentQualityBudgetDiagnosticCode::unsupported_quality_preset, "quality_preset",
                       static_cast<std::uint8_t>(request.preset), 0U, "environment quality preset is unsupported");
        plan.status = EnvironmentQualityBudgetStatus::invalid_request;
        return plan;
    }

    if (request.preset == EnvironmentQualityPreset::custom) {
        if (!request.custom_limits_provided) {
            add_diagnostic(plan, EnvironmentQualityBudgetDiagnosticCode::missing_custom_limits, "custom_limits", 0U, 0U,
                           "custom environment quality preset requires explicit limits");
            plan.status = EnvironmentQualityBudgetStatus::invalid_request;
            return plan;
        }
        plan.limits = request.custom_limits;
        if (!custom_limits_valid(plan.limits)) {
            add_diagnostic(plan, EnvironmentQualityBudgetDiagnosticCode::invalid_custom_limits, "custom_limits", 0U, 0U,
                           "custom environment quality limits must be positive");
            plan.status = EnvironmentQualityBudgetStatus::invalid_request;
            return plan;
        }
    } else {
        plan.limits = environment_quality_budget_preset_limits(request.preset);
    }

    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::physical_sky_sample_budget_exceeded,
                    "physical_sky_sample_budget", request.usage.physical_sky_sample_budget,
                    plan.limits.physical_sky_sample_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::height_fog_sample_step_budget_exceeded,
                    "height_fog_sample_step_budget", request.usage.height_fog_sample_step_budget,
                    plan.limits.height_fog_sample_step_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::volumetric_fog_raymarch_step_budget_exceeded,
                    "volumetric_fog_raymarch_step_budget", request.usage.volumetric_fog_raymarch_step_budget,
                    plan.limits.volumetric_fog_raymarch_step_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::volumetric_cloud_primary_step_budget_exceeded,
                    "volumetric_cloud_primary_step_budget", request.usage.volumetric_cloud_primary_step_budget,
                    plan.limits.volumetric_cloud_primary_step_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::volumetric_cloud_light_step_budget_exceeded,
                    "volumetric_cloud_light_step_budget", request.usage.volumetric_cloud_light_step_budget,
                    plan.limits.volumetric_cloud_light_step_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::precipitation_particle_budget_exceeded,
                    "precipitation_particle_rows", request.usage.precipitation_particle_rows,
                    plan.limits.precipitation_particle_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::transient_gpu_byte_budget_exceeded,
                    "transient_gpu_byte_estimate", request.usage.transient_gpu_byte_estimate,
                    plan.limits.transient_gpu_byte_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::framegraph_pass_budget_exceeded, "framegraph_passes",
                    request.usage.framegraph_passes, plan.limits.framegraph_pass_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::framegraph_barrier_step_budget_exceeded,
                    "framegraph_barrier_steps", request.usage.framegraph_barrier_steps,
                    plan.limits.framegraph_barrier_step_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::texture_upload_budget_exceeded, "texture_uploads",
                    request.usage.texture_uploads, plan.limits.texture_upload_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::renderer_draw_budget_exceeded, "renderer_draws",
                    request.usage.renderer_draws, plan.limits.renderer_draw_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::compute_dispatch_budget_exceeded,
                    "compute_dispatches", request.usage.compute_dispatches, plan.limits.compute_dispatch_budget);
    add_if_exceeded(plan, EnvironmentQualityBudgetDiagnosticCode::diagnostics_budget_exceeded, "diagnostics",
                    request.usage.diagnostics, plan.limits.diagnostics_budget);
    if (request.usage.native_handle_access) {
        add_diagnostic(plan, EnvironmentQualityBudgetDiagnosticCode::native_handle_access, "native_handle_access", 1U,
                       0U, "environment quality budget evidence must not expose native handles");
    }
    if (request.usage.broad_optimization_claimed) {
        add_diagnostic(plan, EnvironmentQualityBudgetDiagnosticCode::broad_optimization_claimed,
                       "broad_optimization_claimed", 1U, 0U,
                       "environment quality budgets do not claim broad optimization");
    }

    plan.ready = plan.diagnostics.empty();
    plan.status = plan.ready ? EnvironmentQualityBudgetStatus::ready : EnvironmentQualityBudgetStatus::budget_exceeded;
    return plan;
}

bool has_environment_quality_budget_diagnostic(const EnvironmentQualityBudgetPlan& plan,
                                               EnvironmentQualityBudgetDiagnosticCode code) noexcept {
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace mirakana
