// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/material_weathering.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(EnvironmentMaterialWeatheringPlan& plan, EnvironmentMaterialWeatheringDiagnosticCode code,
                    std::string field, std::string message) {
    plan.diagnostics.push_back(EnvironmentMaterialWeatheringDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool precipitation_wets_material(EnvironmentPrecipitationKind kind) noexcept {
    switch (kind) {
    case EnvironmentPrecipitationKind::rain:
    case EnvironmentPrecipitationKind::sleet:
    case EnvironmentPrecipitationKind::hail:
        return true;
    case EnvironmentPrecipitationKind::none:
    case EnvironmentPrecipitationKind::snow:
    case EnvironmentPrecipitationKind::ash:
    case EnvironmentPrecipitationKind::dust:
        return false;
    }
    return false;
}

[[nodiscard]] EnvironmentPrecipitationKind effective_precipitation_kind(const EnvironmentProfileDesc& environment) {
    if (environment.precipitation.kind != EnvironmentPrecipitationKind::none) {
        return environment.precipitation.kind;
    }
    if (environment.weather == EnvironmentWeatherKind::rain || environment.weather == EnvironmentWeatherKind::storm) {
        return EnvironmentPrecipitationKind::rain;
    }
    if (environment.weather == EnvironmentWeatherKind::snow) {
        return EnvironmentPrecipitationKind::snow;
    }
    return EnvironmentPrecipitationKind::none;
}

[[nodiscard]] EnvironmentMaterialWeatheringState select_state(float wetness, float snow, float ice) noexcept {
    if (wetness > 0.0F && snow > 0.0F) {
        return EnvironmentMaterialWeatheringState::mixed;
    }
    if (snow > 0.0F) {
        return EnvironmentMaterialWeatheringState::snow_covered;
    }
    if (wetness > 0.0F) {
        return EnvironmentMaterialWeatheringState::wet;
    }
    if (ice > 0.0F) {
        return EnvironmentMaterialWeatheringState::icy;
    }
    return EnvironmentMaterialWeatheringState::dry;
}

void validate_desc(EnvironmentMaterialWeatheringPlan& plan, const EnvironmentMaterialWeatheringDesc& desc) {
    if (!validate_environment_profile(desc.environment).succeeded()) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::invalid_environment_profile, "environment",
                       "material weathering requires a valid environment profile");
    }
    if (!finite_in_range(desc.wetness_intensity, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::invalid_wetness_intensity,
                       "wetness_intensity", "material wetness intensity must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.snow_accumulation, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::invalid_snow_accumulation,
                       "snow_accumulation", "material snow accumulation must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.ice_intensity, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::invalid_ice_intensity, "ice_intensity",
                       "material ice intensity must be finite and in [0, 1]");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::unsupported_backend_execution,
                       "request_backend_execution",
                       "environment material weathering planning must not invoke renderer backends");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "environment material weathering planning must not expose native handles");
    }
    if (desc.request_source_material_mutation) {
        add_diagnostic(plan, EnvironmentMaterialWeatheringDiagnosticCode::unsupported_source_material_mutation,
                       "request_source_material_mutation",
                       "runtime material weathering may emit parameter rows but must not rewrite source materials");
    }
}

void append_rows(EnvironmentMaterialWeatheringPlan& plan, const EnvironmentMaterialWeatheringDesc& desc) {
    auto wetness = desc.wetness_intensity;
    auto snow = desc.snow_accumulation;
    auto ice = desc.ice_intensity;

    if (desc.derive_from_environment_precipitation && desc.environment.precipitation.intensity > 0.0F) {
        const auto kind = effective_precipitation_kind(desc.environment);
        if (precipitation_wets_material(kind)) {
            wetness = std::max(wetness, desc.environment.precipitation.intensity);
        } else if (kind == EnvironmentPrecipitationKind::snow) {
            snow = std::max(snow, desc.environment.precipitation.intensity);
        }
    }

    plan.rows.push_back(EnvironmentMaterialWeatheringRow{
        .state = select_state(wetness, snow, ice),
        .wetness_intensity = wetness,
        .snow_accumulation = snow,
        .ice_intensity = ice,
    });

    if (wetness > 0.0F) {
        plan.wet_rows.push_back(EnvironmentMaterialWetnessRow{
            .intensity = wetness,
        });
    }
    if (snow > 0.0F) {
        plan.snow_rows.push_back(EnvironmentMaterialSnowAccumulationRow{
            .coverage = snow,
        });
    }
    if (ice > 0.0F) {
        plan.ice_rows.push_back(EnvironmentMaterialIceRow{
            .intensity = ice,
        });
    }
}

} // namespace

bool EnvironmentMaterialWeatheringPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentMaterialWeatheringPlan plan_environment_material_weathering(const EnvironmentMaterialWeatheringDesc& desc) {
    EnvironmentMaterialWeatheringPlan plan{
        .status = EnvironmentMaterialWeatheringPlanStatus::planned,
    };

    validate_desc(plan, desc);
    if (!plan.succeeded()) {
        plan.status = EnvironmentMaterialWeatheringPlanStatus::blocked;
        return plan;
    }

    append_rows(plan, desc);
    return plan;
}

bool has_environment_material_weathering_diagnostic(const EnvironmentMaterialWeatheringPlan& plan,
                                                    EnvironmentMaterialWeatheringDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
