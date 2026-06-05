// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/precipitation_policy.hpp"

#include <limits>

namespace {

[[nodiscard]] mirakana::EnvironmentPrecipitationPlan make_environment_plan() {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "renderer_precipitation";
    profile.weather = mirakana::EnvironmentWeatherKind::storm;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.8F,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 8.5F,
        .wind_speed_mps = 6.0F,
    };

    return mirakana::plan_environment_precipitation(mirakana::EnvironmentPrecipitationPlanDesc{
        .environment = profile,
        .scene_geometry_occlusion_required = true,
        .occlusion_policy_available = true,
    });
}

[[nodiscard]] mirakana::EnvironmentPrecipitationPlan make_snow_environment_plan() {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "renderer_precipitation_snow";
    profile.weather = mirakana::EnvironmentWeatherKind::snow;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::snow,
        .intensity = 0.55F,
        .particle_radius_mm = 1.1F,
        .fall_speed_mps = 1.4F,
        .wind_speed_mps = 2.0F,
    };

    return mirakana::plan_environment_precipitation(mirakana::EnvironmentPrecipitationPlanDesc{
        .environment = profile,
        .scene_geometry_occlusion_required = true,
        .occlusion_policy_available = true,
    });
}

[[nodiscard]] mirakana::PrecipitationPolicyDesc make_precipitation_policy_desc() {
    return mirakana::PrecipitationPolicyDesc{
        .environment_plan = make_environment_plan(),
        .quality_tier = mirakana::PrecipitationQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
    };
}

} // namespace

MK_TEST("renderer precipitation policy plans shader wetness occlusion and audio handoff rows") {
    const auto plan = mirakana::plan_precipitation_policy(make_precipitation_policy_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::PrecipitationPolicyStatus::planned);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(plan.package_evidence_ready);
    MK_REQUIRE(!plan.uploads_particle_buffers);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_materials);
    MK_REQUIRE(!plan.plays_audio);

    MK_REQUIRE(plan.shader_rows.size() == 1);
    MK_REQUIRE(plan.shader_rows[0].kind == mirakana::EnvironmentPrecipitationKind::rain);
    MK_REQUIRE(plan.shader_rows[0].constants_binding_slot == mirakana::precipitation_constants_binding());
    MK_REQUIRE(plan.shader_rows[0].particle_texture_binding_slot == mirakana::precipitation_particle_texture_binding());
    MK_REQUIRE(plan.shader_rows[0].scene_depth_texture_binding_slot ==
               mirakana::precipitation_scene_depth_texture_binding());
    MK_REQUIRE(plan.shader_rows[0].sampler_binding_slot == mirakana::precipitation_sampler_binding());
    MK_REQUIRE(plan.shader_rows[0].uses_camera_near_particles);
    MK_REQUIRE(plan.shader_rows[0].uses_scene_depth_occlusion);

    MK_REQUIRE(plan.wetness_rows.size() == 1);
    MK_REQUIRE(plan.wetness_rows[0].enabled);
    MK_REQUIRE(plan.wetness_rows[0].splash_intent);
    MK_REQUIRE(plan.wetness_rows[0].ripple_intent);
    MK_REQUIRE(!plan.wetness_rows[0].mutates_materials);

    MK_REQUIRE(plan.audio_handoff_rows.size() >= 3);
    MK_REQUIRE(plan.quality_rows.size() == 1);
    MK_REQUIRE(plan.quality_rows[0].tier == mirakana::PrecipitationQualityTier::balanced);
}

MK_TEST("renderer precipitation policy keeps snow package evidence value-only without wetness rows") {
    const auto plan = mirakana::plan_precipitation_policy(mirakana::PrecipitationPolicyDesc{
        .environment_plan = make_snow_environment_plan(),
        .quality_tier = mirakana::PrecipitationQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.shader_rows.size() == 1);
    MK_REQUIRE(plan.shader_rows[0].kind == mirakana::EnvironmentPrecipitationKind::snow);
    MK_REQUIRE(plan.shader_rows[0].uses_camera_near_particles);
    MK_REQUIRE(plan.shader_rows[0].uses_scene_depth_occlusion);
    MK_REQUIRE(plan.wetness_rows.empty());
    MK_REQUIRE(plan.audio_handoff_rows.size() >= 1);
    MK_REQUIRE(!plan.uploads_particle_buffers);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_materials);
    MK_REQUIRE(!plan.plays_audio);
}

MK_TEST("renderer precipitation policy requires execution evidence before ready promotion") {
    auto desc = make_precipitation_policy_desc();
    desc.request_ready_promotion = true;

    const auto blocked_plan = mirakana::plan_precipitation_policy(desc);

    MK_REQUIRE(!blocked_plan.succeeded());
    MK_REQUIRE(blocked_plan.status == mirakana::PrecipitationPolicyStatus::blocked);
    MK_REQUIRE(!blocked_plan.ready());
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        blocked_plan, mirakana::PrecipitationPolicyDiagnosticCode::missing_execution_evidence));

    desc.execution_evidence_ready = true;
    const auto ready_plan = mirakana::plan_precipitation_policy(desc);

    MK_REQUIRE(ready_plan.succeeded());
    MK_REQUIRE(ready_plan.status == mirakana::PrecipitationPolicyStatus::ready);
    MK_REQUIRE(ready_plan.ready());
}

MK_TEST("renderer precipitation policy fails closed for invalid plans and unsupported claims") {
    auto desc = make_precipitation_policy_desc();
    desc.environment_plan.diagnostics.push_back(mirakana::EnvironmentPrecipitationDiagnostic{
        .code = mirakana::EnvironmentPrecipitationDiagnosticCode::invalid_intensity,
        .field = "precipitation.intensity",
        .message = "test invalid plan",
    });
    desc.environment_plan.status = mirakana::EnvironmentPrecipitationPlanStatus::blocked;
    desc.quality_tier = mirakana::PrecipitationQualityTier::unknown;
    desc.shader_contract_evidence_ready = false;
    desc.package_evidence_ready = false;
    desc.request_particle_buffer_upload = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;
    desc.request_material_mutation = true;
    desc.request_audio_playback = true;

    const auto plan = mirakana::plan_precipitation_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::PrecipitationPolicyStatus::blocked);
    MK_REQUIRE(!plan.uploads_particle_buffers);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_materials);
    MK_REQUIRE(!plan.plays_audio);
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::invalid_environment_plan));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_quality_tier));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::missing_package_evidence));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_particle_buffer_upload));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_material_mutation));
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        plan, mirakana::PrecipitationPolicyDiagnosticCode::unsupported_audio_playback));

    auto nan_desc = make_precipitation_policy_desc();
    nan_desc.environment_plan.particle_rows[0].intensity = std::numeric_limits<float>::quiet_NaN();
    const auto nan_plan = mirakana::plan_precipitation_policy(nan_desc);

    MK_REQUIRE(!nan_plan.succeeded());
    MK_REQUIRE(mirakana::has_precipitation_policy_diagnostic(
        nan_plan, mirakana::PrecipitationPolicyDiagnosticCode::invalid_environment_plan));
}

int main() {
    return mirakana::test::run_all();
}
