// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_lighting_policy.hpp"

namespace {

[[nodiscard]] mirakana::EnvironmentLightingPolicyDesc make_valid_lighting_desc() {
    return mirakana::EnvironmentLightingPolicyDesc{
        .dependency_mode = mirakana::EnvironmentLightingDependencyMode::first_party_cooked_texture,
        .visual_sky_source = mirakana::EnvironmentLightingSkySource::physical_sky,
        .lighting_sky_source = mirakana::EnvironmentLightingSkySource::specified_cubemap,
        .reflection_cubemap =
            mirakana::EnvironmentReflectionCubemapDesc{
                .asset = mirakana::AssetId::from_name("environment/day_ibl_cubemap"),
                .edge_size = 256,
                .mip_count = 9,
                .format = mirakana::EnvironmentLightingTextureFormat::rgba16_float,
                .hdr_metadata_ready = true,
                .package_evidence_ready = true,
            },
        .irradiance =
            mirakana::EnvironmentIrradianceDesc{
                .spherical_harmonic_order = 3,
                .coefficient_count = 9,
            },
        .radiance =
            mirakana::EnvironmentRadianceDesc{
                .mip_count = 9,
                .roughness_mip_count = 9,
                .max_roughness = 1.0F,
            },
        .hdr_clamp =
            mirakana::EnvironmentHdrClampPolicyDesc{
                .enabled = true,
                .max_luminance_nits = 20000.0F,
                .exposure_bias = 0.0F,
            },
        .visual_sky_visible_in_main_camera = false,
    };
}

} // namespace

MK_TEST("renderer environment lighting policy separates visual sky from IBL rows") {
    const auto plan = mirakana::plan_environment_lighting_policy(make_valid_lighting_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.dependency_mode == mirakana::EnvironmentLightingDependencyMode::first_party_cooked_texture);
    MK_REQUIRE(plan.visual_sky_source == mirakana::EnvironmentLightingSkySource::physical_sky);
    MK_REQUIRE(plan.lighting_sky_source == mirakana::EnvironmentLightingSkySource::specified_cubemap);
    MK_REQUIRE(!plan.visual_sky_visible_in_main_camera);
    MK_REQUIRE(!plan.requires_visual_sky_visible_in_main_camera);
    MK_REQUIRE(!plan.uses_new_import_dependency);
    MK_REQUIRE(!plan.allocates_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);

    MK_REQUIRE(plan.reflection_cubemap_rows.size() == 1);
    MK_REQUIRE(plan.reflection_cubemap_rows[0].asset == mirakana::AssetId::from_name("environment/day_ibl_cubemap"));
    MK_REQUIRE(plan.reflection_cubemap_rows[0].face_count == 6);
    MK_REQUIRE(plan.reflection_cubemap_rows[0].edge_size == 256);
    MK_REQUIRE(plan.reflection_cubemap_rows[0].mip_count == 9);
    MK_REQUIRE(plan.reflection_cubemap_rows[0].package_evidence_ready);

    MK_REQUIRE(plan.irradiance_rows.size() == 9);
    MK_REQUIRE(plan.irradiance_rows[0].coefficient_index == 0);
    MK_REQUIRE(plan.irradiance_rows[8].coefficient_index == 8);
    MK_REQUIRE(plan.irradiance_rows[8].spherical_harmonic_order == 3);

    MK_REQUIRE(plan.radiance_mip_rows.size() == 9);
    MK_REQUIRE(plan.radiance_mip_rows[0].mip_index == 0);
    MK_REQUIRE(plan.radiance_mip_rows[0].edge_size == 256);
    MK_REQUIRE(plan.radiance_mip_rows[0].roughness == 0.0F);
    MK_REQUIRE(plan.radiance_mip_rows[8].mip_index == 8);
    MK_REQUIRE(plan.radiance_mip_rows[8].edge_size == 1);
    MK_REQUIRE(plan.radiance_mip_rows[8].roughness > 0.99F);

    MK_REQUIRE(plan.package_evidence_rows.size() == 1);
    MK_REQUIRE(plan.package_evidence_rows[0].ready);
    MK_REQUIRE(plan.hdr_clamp_row.enabled);
    MK_REQUIRE(plan.hdr_clamp_row.max_luminance_nits == 20000.0F);
}

MK_TEST("renderer environment lighting policy fails closed for dependency and runtime claims") {
    auto desc = make_valid_lighting_desc();
    desc.dependency_mode = mirakana::EnvironmentLightingDependencyMode::ktx_import;
    desc.visual_sky_source = mirakana::EnvironmentLightingSkySource::unknown;
    desc.lighting_sky_source = mirakana::EnvironmentLightingSkySource::unknown;
    desc.reflection_cubemap.asset = {};
    desc.reflection_cubemap.edge_size = 300;
    desc.reflection_cubemap.mip_count = 0;
    desc.reflection_cubemap.hdr_metadata_ready = false;
    desc.reflection_cubemap.package_evidence_ready = false;
    desc.irradiance.spherical_harmonic_order = 4;
    desc.irradiance.coefficient_count = 6;
    desc.radiance.roughness_mip_count = 12;
    desc.radiance.max_roughness = 2.0F;
    desc.hdr_clamp.max_luminance_nits = 0.0F;
    desc.request_coupled_visual_and_lighting_sky = true;
    desc.request_runtime_capture = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;

    const auto plan = mirakana::plan_environment_lighting_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(!plan.uses_new_import_dependency);
    MK_REQUIRE(!plan.allocates_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::unsupported_dependency_request));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_visual_sky_source));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_lighting_sky_source));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_asset));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_size));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_reflection_cubemap_mips));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::missing_hdr_metadata));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::missing_package_evidence));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_irradiance_order));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_radiance_mips));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_roughness_policy));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::invalid_hdr_clamp));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::unsupported_visual_lighting_coupling));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::unsupported_runtime_capture));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_lighting_diagnostic(
        plan, mirakana::EnvironmentLightingDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
