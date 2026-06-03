// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_fog_policy.hpp"

namespace {

[[nodiscard]] mirakana::EnvironmentFogPolicyDesc make_valid_height_fog_desc() {
    return mirakana::EnvironmentFogPolicyDesc{
        .mode = mirakana::EnvironmentFogMode::exponential_height_with_aerial_perspective,
        .density = 0.015F,
        .height_falloff = 0.20F,
        .height_offset_m = 25.0F,
        .start_distance_m = 50.0F,
        .cutoff_distance_m = 5000.0F,
        .max_opacity = 0.80F,
        .sky_affect = 0.35F,
        .directional_inscattering_anisotropy = 0.15F,
        .inscattering_color = mirakana::Vec3{.x = 0.55F, .y = 0.64F, .z = 0.72F},
        .directional_inscattering_color = mirakana::Vec3{.x = 1.0F, .y = 0.86F, .z = 0.62F},
        .sample_step_budget = 12,
        .scene_depth_available = true,
        .shader_contract_evidence_ready = true,
    };
}

} // namespace

MK_TEST("renderer environment fog policy plans depth-aware height fog rows") {
    const auto plan = mirakana::plan_environment_fog_policy(make_valid_height_fog_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.mode == mirakana::EnvironmentFogMode::exponential_height_with_aerial_perspective);
    MK_REQUIRE(plan.requires_scene_depth);
    MK_REQUIRE(plan.scene_depth_available);
    MK_REQUIRE(plan.requires_shader_contract_evidence);
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.allocates_froxel_volume);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);

    MK_REQUIRE(plan.constant_layout_rows.size() == 14);
    MK_REQUIRE(plan.constant_layout_rows[0].name == "density");
    MK_REQUIRE(plan.constant_layout_rows[0].offset_bytes == 0);
    MK_REQUIRE(plan.constant_layout_rows[13].name == "directional_inscattering_b");
    MK_REQUIRE(plan.constant_layout_rows[13].offset_bytes == 52);

    MK_REQUIRE(plan.depth_input_rows.size() == 1);
    MK_REQUIRE(plan.depth_input_rows[0].texture_binding_slot == 0);
    MK_REQUIRE(plan.depth_input_rows[0].sampler_binding_slot == 0);
    MK_REQUIRE(plan.depth_input_rows[0].required);
    MK_REQUIRE(plan.depth_input_rows[0].available);

    MK_REQUIRE(plan.pass_budget_rows.size() == 1);
    MK_REQUIRE(plan.pass_budget_rows[0].postprocess_pass_count == 1);
    MK_REQUIRE(plan.pass_budget_rows[0].frame_graph_pass_count == 2);
    MK_REQUIRE(plan.pass_budget_rows[0].frame_graph_barrier_step_budget == 2);
    MK_REQUIRE(plan.pass_budget_rows[0].fullscreen_triangle_count == 1);
    MK_REQUIRE(plan.pass_budget_rows[0].sample_step_budget == 12);

    MK_REQUIRE(plan.postprocess_rows.size() == 1);
    MK_REQUIRE(plan.postprocess_rows[0].effect == mirakana::PostprocessEffectKind::fog);
    MK_REQUIRE(plan.postprocess_rows[0].uses_scene_depth);
}

MK_TEST("renderer environment fog policy fails closed for invalid values and execution claims") {
    auto desc = make_valid_height_fog_desc();
    desc.mode = mirakana::EnvironmentFogMode::unknown;
    desc.density = -1.0F;
    desc.height_falloff = 0.0F;
    desc.start_distance_m = -10.0F;
    desc.cutoff_distance_m = 5.0F;
    desc.max_opacity = 2.0F;
    desc.sky_affect = 1.5F;
    desc.directional_inscattering_anisotropy = 2.0F;
    desc.inscattering_color.x = -0.1F;
    desc.sample_step_budget = 0;
    desc.scene_depth_available = false;
    desc.shader_contract_evidence_ready = false;
    desc.request_volumetric_fog = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;

    const auto plan = mirakana::plan_environment_fog_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.requires_scene_depth);
    MK_REQUIRE(!plan.scene_depth_available);
    MK_REQUIRE(!plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.allocates_froxel_volume);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::unsupported_mode));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_density));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_height_falloff));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_start_distance));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(
        plan, mirakana::EnvironmentFogDiagnosticCode::invalid_cutoff_distance));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_max_opacity));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_sky_affect));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_anisotropy));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_color));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::invalid_sample_budget));
    MK_REQUIRE(
        mirakana::has_environment_fog_diagnostic(plan, mirakana::EnvironmentFogDiagnosticCode::missing_scene_depth));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(
        plan, mirakana::EnvironmentFogDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(
        plan, mirakana::EnvironmentFogDiagnosticCode::unsupported_volumetric_fog));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(
        plan, mirakana::EnvironmentFogDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_fog_diagnostic(
        plan, mirakana::EnvironmentFogDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
