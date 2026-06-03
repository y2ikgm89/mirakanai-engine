// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class PhysicalSkyAerialPerspectiveMode : std::uint8_t {
    unknown = 0,
    none,
    froxel_volume,
    screen_space,
};

enum class PhysicalSkyLutKind : std::uint8_t {
    unknown = 0,
    transmittance,
    sky_view,
    aerial_perspective,
    multiple_scattering,
};

enum class PhysicalSkyDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_rayleigh_density,
    invalid_mie_density,
    invalid_mie_anisotropy,
    invalid_ozone_density,
    invalid_planet_radius,
    invalid_atmosphere_height,
    invalid_sun_disk_radius,
    invalid_sample_budget,
    unsupported_aerial_perspective_mode,
    missing_shader_contract_evidence,
    unsupported_lut_texture_allocation,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
};

struct PhysicalSkyAtmosphereDesc {
    float rayleigh_density_height_km{8.0F};
    float mie_density_height_km{1.2F};
    float mie_anisotropy{0.8F};
    float ozone_density_height_km{25.0F};
    float planet_radius_km{6360.0F};
    float atmosphere_height_km{100.0F};
    float sun_angular_radius_radians{0.00465F};
    float solar_illuminance_lux{100000.0F};
};

struct PhysicalSkySampleBudgetDesc {
    std::uint32_t transmittance_sample_count{40};
    std::uint32_t sky_view_sample_count{32};
    std::uint32_t aerial_perspective_sample_count{16};
    std::uint32_t multiple_scattering_sample_count{20};
};

struct PhysicalSkyPolicyDesc {
    PhysicalSkyAtmosphereDesc atmosphere;
    PhysicalSkySampleBudgetDesc sample_budget;
    PhysicalSkyAerialPerspectiveMode aerial_perspective_mode{PhysicalSkyAerialPerspectiveMode::froxel_volume};
    bool shader_contract_evidence_ready{false};
    bool request_lut_texture_allocation{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
};

struct PhysicalSkyConstantLayoutRow {
    std::string name;
    std::uint32_t offset_bytes{0};
    std::uint32_t size_bytes{4};
};

struct PhysicalSkyLutIntentRow {
    PhysicalSkyLutKind kind{PhysicalSkyLutKind::unknown};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t depth{1};
    std::uint32_t sample_count{0};
    bool requires_shader_contract_evidence{true};
    bool allocates_texture{false};
};

struct PhysicalSkyDiagnostic {
    PhysicalSkyDiagnosticCode code{PhysicalSkyDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct PhysicalSkyPolicyPlan {
    PhysicalSkyAtmosphereDesc atmosphere;
    PhysicalSkySampleBudgetDesc sample_budget;
    PhysicalSkyAerialPerspectiveMode aerial_perspective_mode{PhysicalSkyAerialPerspectiveMode::unknown};
    bool shader_contract_evidence_ready{false};
    bool allocates_lut_textures{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<PhysicalSkyConstantLayoutRow> constant_layout_rows;
    std::vector<PhysicalSkyLutIntentRow> lut_intent_rows;
    std::vector<PhysicalSkyDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] PhysicalSkyPolicyPlan plan_physical_sky_policy(const PhysicalSkyPolicyDesc& desc);

[[nodiscard]] bool has_physical_sky_lut_intent(const PhysicalSkyPolicyPlan& plan, PhysicalSkyLutKind kind) noexcept;

[[nodiscard]] bool has_physical_sky_diagnostic(const PhysicalSkyPolicyPlan& plan,
                                               PhysicalSkyDiagnosticCode code) noexcept;

} // namespace mirakana
