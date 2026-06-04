// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/physical_sky_policy.hpp"

#include <array>
#include <cstring>

namespace {

[[nodiscard]] mirakana::PhysicalSkyPolicyDesc make_valid_physical_sky_desc() {
    return mirakana::PhysicalSkyPolicyDesc{
        .atmosphere =
            mirakana::PhysicalSkyAtmosphereDesc{
                .rayleigh_density_height_km = 8.0F,
                .mie_density_height_km = 1.2F,
                .mie_anisotropy = 0.8F,
                .ozone_density_height_km = 25.0F,
                .planet_radius_km = 6360.0F,
                .atmosphere_height_km = 100.0F,
                .sun_angular_radius_radians = 0.00465F,
            },
        .sample_budget =
            mirakana::PhysicalSkySampleBudgetDesc{
                .transmittance_sample_count = 40,
                .sky_view_sample_count = 32,
                .aerial_perspective_sample_count = 16,
                .multiple_scattering_sample_count = 20,
            },
        .aerial_perspective_mode = mirakana::PhysicalSkyAerialPerspectiveMode::froxel_volume,
        .shader_contract_evidence_ready = true,
    };
}

[[nodiscard]] float read_physical_sky_constant(const std::array<std::uint8_t, 256>& bytes, std::size_t offset) {
    float value = 0.0F;
    std::memcpy(&value, bytes.data() + offset, sizeof(float));
    return value;
}

} // namespace

MK_TEST("renderer physical sky policy plans constant layout and LUT intent rows") {
    const auto plan = mirakana::plan_physical_sky_policy(make_valid_physical_sky_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.allocates_lut_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(plan.constant_layout_rows.size() == 8);
    MK_REQUIRE(plan.constant_layout_rows[0].name == "planet_radius_km");
    MK_REQUIRE(plan.constant_layout_rows[0].offset_bytes == 0);
    MK_REQUIRE(plan.constant_layout_rows[4].name == "mie_anisotropy");
    MK_REQUIRE(plan.constant_layout_rows[4].offset_bytes == 16);
    MK_REQUIRE(plan.constant_layout_rows[7].name == "solar_illuminance_lux");
    MK_REQUIRE(plan.constant_layout_rows[7].offset_bytes == 28);
    MK_REQUIRE(mirakana::has_physical_sky_lut_intent(plan, mirakana::PhysicalSkyLutKind::transmittance));
    MK_REQUIRE(mirakana::has_physical_sky_lut_intent(plan, mirakana::PhysicalSkyLutKind::sky_view));
    MK_REQUIRE(mirakana::has_physical_sky_lut_intent(plan, mirakana::PhysicalSkyLutKind::aerial_perspective));
    MK_REQUIRE(mirakana::has_physical_sky_lut_intent(plan, mirakana::PhysicalSkyLutKind::multiple_scattering));
    MK_REQUIRE(plan.lut_intent_rows.size() == 4);
    MK_REQUIRE(plan.lut_intent_rows[0].sample_count == 40);
    MK_REQUIRE(!plan.lut_intent_rows[0].allocates_texture);
    MK_REQUIRE(!plan.lut_intent_rows[2].allocates_texture);
}

MK_TEST("renderer physical sky policy packs D3D12 constant buffer rows") {
    const auto desc = make_valid_physical_sky_desc();
    std::array<std::uint8_t, mirakana::physical_sky_constants_byte_size()> bytes{};

    mirakana::pack_physical_sky_constants(bytes, desc);

    MK_REQUIRE(mirakana::physical_sky_constants_binding() == 0);
    MK_REQUIRE(mirakana::physical_sky_constants_byte_size() == 256);
    MK_REQUIRE(read_physical_sky_constant(bytes, 0) == desc.atmosphere.planet_radius_km);
    MK_REQUIRE(read_physical_sky_constant(bytes, 4) == desc.atmosphere.atmosphere_height_km);
    MK_REQUIRE(read_physical_sky_constant(bytes, 8) == desc.atmosphere.rayleigh_density_height_km);
    MK_REQUIRE(read_physical_sky_constant(bytes, 12) == desc.atmosphere.mie_density_height_km);
    MK_REQUIRE(read_physical_sky_constant(bytes, 16) == desc.atmosphere.mie_anisotropy);
    MK_REQUIRE(read_physical_sky_constant(bytes, 20) == desc.atmosphere.ozone_density_height_km);
    MK_REQUIRE(read_physical_sky_constant(bytes, 24) == desc.atmosphere.sun_angular_radius_radians);
    MK_REQUIRE(read_physical_sky_constant(bytes, 28) == desc.atmosphere.solar_illuminance_lux);
    MK_REQUIRE(bytes[32] == std::uint8_t{0});
    MK_REQUIRE(bytes.back() == std::uint8_t{0});
}

MK_TEST("renderer physical sky policy fails closed for invalid atmosphere and unsafe claims") {
    auto desc = make_valid_physical_sky_desc();
    desc.atmosphere.rayleigh_density_height_km = 0.0F;
    desc.atmosphere.mie_density_height_km = -1.0F;
    desc.atmosphere.mie_anisotropy = 1.2F;
    desc.atmosphere.ozone_density_height_km = 0.0F;
    desc.atmosphere.planet_radius_km = 0.0F;
    desc.atmosphere.atmosphere_height_km = 0.0F;
    desc.atmosphere.sun_angular_radius_radians = 0.0F;
    desc.sample_budget.transmittance_sample_count = 0;
    desc.sample_budget.sky_view_sample_count = 2048;
    desc.aerial_perspective_mode = mirakana::PhysicalSkyAerialPerspectiveMode::unknown;
    desc.shader_contract_evidence_ready = false;
    desc.request_lut_texture_allocation = true;
    desc.request_native_handle_access = true;

    const auto plan = mirakana::plan_physical_sky_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(!plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.allocates_lut_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(
        mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_rayleigh_density));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_mie_density));
    MK_REQUIRE(
        mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_mie_anisotropy));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_ozone_density));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_planet_radius));
    MK_REQUIRE(
        mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_atmosphere_height));
    MK_REQUIRE(
        mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_sun_disk_radius));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(plan, mirakana::PhysicalSkyDiagnosticCode::invalid_sample_budget));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(
        plan, mirakana::PhysicalSkyDiagnosticCode::unsupported_aerial_perspective_mode));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(
        plan, mirakana::PhysicalSkyDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(
        plan, mirakana::PhysicalSkyDiagnosticCode::unsupported_lut_texture_allocation));
    MK_REQUIRE(mirakana::has_physical_sky_diagnostic(
        plan, mirakana::PhysicalSkyDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
