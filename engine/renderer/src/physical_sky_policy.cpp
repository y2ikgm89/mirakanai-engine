// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/physical_sky_policy.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(PhysicalSkyPolicyPlan& plan, PhysicalSkyDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(PhysicalSkyDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool positive_sample_count(std::uint32_t value) noexcept {
    return value > 0U && value <= 1024U;
}

[[nodiscard]] bool supported_aerial_perspective_mode(PhysicalSkyAerialPerspectiveMode mode) noexcept {
    switch (mode) {
    case PhysicalSkyAerialPerspectiveMode::none:
    case PhysicalSkyAerialPerspectiveMode::froxel_volume:
    case PhysicalSkyAerialPerspectiveMode::screen_space:
        return true;
    case PhysicalSkyAerialPerspectiveMode::unknown:
        return false;
    }
    return false;
}

void append_constant_layout_rows(PhysicalSkyPolicyPlan& plan) {
    constexpr std::uint32_t scalar_size = 4U;
    const char* names[] = {
        "planet_radius_km", "atmosphere_height_km",    "rayleigh_density_height_km", "mie_density_height_km",
        "mie_anisotropy",   "ozone_density_height_km", "sun_angular_radius_radians", "solar_illuminance_lux",
    };

    std::uint32_t offset = 0U;
    for (const auto* name : names) {
        plan.constant_layout_rows.push_back(PhysicalSkyConstantLayoutRow{
            .name = name,
            .offset_bytes = offset,
            .size_bytes = scalar_size,
        });
        offset += scalar_size;
    }
}

void append_lut_intent_rows(PhysicalSkyPolicyPlan& plan) {
    plan.lut_intent_rows.push_back(PhysicalSkyLutIntentRow{
        .kind = PhysicalSkyLutKind::transmittance,
        .width = 256,
        .height = 64,
        .depth = 1,
        .sample_count = plan.sample_budget.transmittance_sample_count,
    });
    plan.lut_intent_rows.push_back(PhysicalSkyLutIntentRow{
        .kind = PhysicalSkyLutKind::sky_view,
        .width = 192,
        .height = 108,
        .depth = 1,
        .sample_count = plan.sample_budget.sky_view_sample_count,
    });
    if (plan.aerial_perspective_mode != PhysicalSkyAerialPerspectiveMode::none) {
        plan.lut_intent_rows.push_back(PhysicalSkyLutIntentRow{
            .kind = PhysicalSkyLutKind::aerial_perspective,
            .width = 32,
            .height = 32,
            .depth = 32,
            .sample_count = plan.sample_budget.aerial_perspective_sample_count,
        });
    }
    plan.lut_intent_rows.push_back(PhysicalSkyLutIntentRow{
        .kind = PhysicalSkyLutKind::multiple_scattering,
        .width = 32,
        .height = 32,
        .depth = 1,
        .sample_count = plan.sample_budget.multiple_scattering_sample_count,
    });
}

void validate_atmosphere(PhysicalSkyPolicyPlan& plan, const PhysicalSkyAtmosphereDesc& atmosphere) {
    if (!finite_in_range(atmosphere.rayleigh_density_height_km, 0.001F, 256.0F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_rayleigh_density, "rayleigh_density_height_km",
                       "physical sky requires a positive finite Rayleigh density height");
    }
    if (!finite_in_range(atmosphere.mie_density_height_km, 0.001F, 256.0F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_mie_density, "mie_density_height_km",
                       "physical sky requires a positive finite Mie density height");
    }
    if (!finite_in_range(atmosphere.mie_anisotropy, -0.99F, 0.99F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_mie_anisotropy, "mie_anisotropy",
                       "physical sky Mie anisotropy must be finite in [-0.99, 0.99]");
    }
    if (!finite_in_range(atmosphere.ozone_density_height_km, 0.001F, 256.0F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_ozone_density, "ozone_density_height_km",
                       "physical sky requires a positive finite ozone density height");
    }
    if (!finite_in_range(atmosphere.planet_radius_km, 1.0F, 100000.0F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_planet_radius, "planet_radius_km",
                       "physical sky requires a positive finite planet radius");
    }
    if (!finite_in_range(atmosphere.atmosphere_height_km, 0.001F, 10000.0F) ||
        atmosphere.atmosphere_height_km >= atmosphere.planet_radius_km) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_atmosphere_height, "atmosphere_height_km",
                       "physical sky requires a positive finite atmosphere height below the planet radius");
    }
    if (!finite_in_range(atmosphere.sun_angular_radius_radians, 0.0001F, 0.25F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_sun_disk_radius, "sun_angular_radius_radians",
                       "physical sky requires a positive finite sun disk angular radius");
    }
    if (!finite_in_range(atmosphere.solar_illuminance_lux, 0.001F, 200000.0F)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_sun_disk_radius, "solar_illuminance_lux",
                       "physical sky requires finite positive solar illuminance");
    }
}

void validate_sample_budget(PhysicalSkyPolicyPlan& plan, const PhysicalSkySampleBudgetDesc& sample_budget) {
    if (!positive_sample_count(sample_budget.transmittance_sample_count) ||
        !positive_sample_count(sample_budget.sky_view_sample_count) ||
        !positive_sample_count(sample_budget.aerial_perspective_sample_count) ||
        !positive_sample_count(sample_budget.multiple_scattering_sample_count)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::invalid_sample_budget, "sample_budget",
                       "physical sky sample counts must be in [1, 1024]");
    }
}

} // namespace

bool PhysicalSkyPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

PhysicalSkyPolicyPlan plan_physical_sky_policy(const PhysicalSkyPolicyDesc& desc) {
    PhysicalSkyPolicyPlan plan{
        .atmosphere = desc.atmosphere,
        .sample_budget = desc.sample_budget,
        .aerial_perspective_mode = desc.aerial_perspective_mode,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
    };

    validate_atmosphere(plan, desc.atmosphere);
    validate_sample_budget(plan, desc.sample_budget);
    if (!supported_aerial_perspective_mode(desc.aerial_perspective_mode)) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::unsupported_aerial_perspective_mode, "aerial_perspective_mode",
                       "physical sky requires a supported aerial perspective mode");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready",
                       "physical sky requires reviewed shader contract evidence before readiness");
    }
    if (desc.request_lut_texture_allocation) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::unsupported_lut_texture_allocation,
                       "request_lut_texture_allocation",
                       "physical sky policy planning must not allocate LUT textures in this slice");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::unsupported_backend_execution, "request_backend_execution",
                       "physical sky policy planning must not invoke renderer or RHI backends in this slice");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, PhysicalSkyDiagnosticCode::unsupported_native_handle_claim, "request_native_handle_access",
                       "physical sky policy planning must not expose native renderer or RHI handles");
    }

    append_constant_layout_rows(plan);
    if (supported_aerial_perspective_mode(desc.aerial_perspective_mode)) {
        append_lut_intent_rows(plan);
    }

    return plan;
}

bool has_physical_sky_lut_intent(const PhysicalSkyPolicyPlan& plan, PhysicalSkyLutKind kind) noexcept {
    return std::ranges::any_of(plan.lut_intent_rows, [kind](const auto& row) { return row.kind == kind; });
}

bool has_physical_sky_diagnostic(const PhysicalSkyPolicyPlan& plan, PhysicalSkyDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
