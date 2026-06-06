// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/cloud_layer_policy.hpp"

#include <limits>

namespace {

[[nodiscard]] mirakana::EnvironmentCloudLayerDesc make_valid_environment_layer() {
    return mirakana::EnvironmentCloudLayerDesc{
        .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
        .coverage = 0.58F,
        .opacity = 0.75F,
        .altitude_m = 3200.0F,
        .wind_velocity_mps = mirakana::Vec2{.x = 4.0F, .y = 1.5F},
        .cloud_map_asset_ref = "environment/clouds/high_soft_latlong",
        .flow_map_asset_ref = "environment/clouds/high_soft_flow",
        .sky_tint_response = mirakana::Vec3{.x = 0.80F, .y = 0.88F, .z = 1.0F},
        .time_of_day_response = 0.85F,
        .ibl_contribution_mode = mirakana::EnvironmentCloudIblContributionMode::sky_tint_only,
        .ibl_contribution = 0.3F,
    };
}

[[nodiscard]] mirakana::CloudLayerPolicyDesc make_valid_cloud_policy_desc() {
    return mirakana::CloudLayerPolicyDesc{
        .layer = make_valid_environment_layer(),
        .quality_tier = mirakana::CloudLayerQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
    };
}

} // namespace

MK_TEST("renderer cloud layer policy plans cheap 2d cloud layer rows") {
    const auto plan = mirakana::plan_cloud_layer_policy(make_valid_cloud_policy_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::CloudLayerPolicyStatus::planned);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.uses_2d_cloud_layer);
    MK_REQUIRE(!plan.uses_volumetric_clouds);
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(plan.package_evidence_ready);
    MK_REQUIRE(!plan.uploads_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);

    MK_REQUIRE(plan.texture_rows.size() == 1);
    MK_REQUIRE(plan.texture_rows[0].cloud_map_asset_ref == "environment/clouds/high_soft_latlong");
    MK_REQUIRE(plan.texture_rows[0].flow_map_asset_ref == "environment/clouds/high_soft_flow");
    MK_REQUIRE(plan.texture_rows[0].cloud_map_binding_slot == mirakana::cloud_layer_cloud_map_binding());
    MK_REQUIRE(plan.texture_rows[0].flow_map_binding_slot == mirakana::cloud_layer_flow_map_binding());
    MK_REQUIRE(plan.texture_rows[0].sampler_binding_slot == mirakana::cloud_layer_sampler_binding());
    MK_REQUIRE(plan.texture_rows[0].package_evidence_ready);

    MK_REQUIRE(plan.visual_rows.size() == 1);
    MK_REQUIRE(plan.visual_rows[0].coverage == 0.58F);
    MK_REQUIRE(plan.visual_rows[0].opacity == 0.75F);
    MK_REQUIRE(plan.visual_rows[0].altitude_m == 3200.0F);
    MK_REQUIRE(plan.visual_rows[0].wind_velocity_mps.x == 4.0F);
    MK_REQUIRE(plan.visual_rows[0].time_of_day_response == 0.85F);

    MK_REQUIRE(plan.ibl_rows.size() == 1);
    MK_REQUIRE(plan.ibl_rows[0].mode == mirakana::EnvironmentCloudIblContributionMode::sky_tint_only);
    MK_REQUIRE(plan.ibl_rows[0].contribution == 0.3F);

    MK_REQUIRE(plan.shader_contract_rows.size() == 1);
    MK_REQUIRE(plan.shader_contract_rows[0].constants_binding_slot == mirakana::cloud_layer_constants_binding());
    MK_REQUIRE(plan.shader_contract_rows[0].uses_latlong_projection);
    MK_REQUIRE(plan.shader_contract_rows[0].uses_flow_map);
}

MK_TEST("renderer cloud layer policy requires execution evidence before ready promotion") {
    auto desc = make_valid_cloud_policy_desc();
    desc.request_ready_promotion = true;

    const auto blocked_plan = mirakana::plan_cloud_layer_policy(desc);

    MK_REQUIRE(!blocked_plan.succeeded());
    MK_REQUIRE(blocked_plan.status == mirakana::CloudLayerPolicyStatus::blocked);
    MK_REQUIRE(!blocked_plan.ready());
    MK_REQUIRE(mirakana::has_cloud_layer_diagnostic(blocked_plan,
                                                    mirakana::CloudLayerDiagnosticCode::missing_execution_evidence));

    desc.execution_evidence_ready = true;
    const auto ready_plan = mirakana::plan_cloud_layer_policy(desc);

    MK_REQUIRE(ready_plan.succeeded());
    MK_REQUIRE(ready_plan.status == mirakana::CloudLayerPolicyStatus::ready);
    MK_REQUIRE(ready_plan.ready());
}

MK_TEST("renderer cloud layer policy promotes texture upload backend and draw execution evidence") {
    auto desc = make_valid_cloud_policy_desc();
    desc.execution_evidence_ready = true;
    desc.request_ready_promotion = true;
    desc.request_texture_upload = true;
    desc.request_backend_execution = true;

    const auto plan = mirakana::plan_cloud_layer_policy(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.uploads_textures);
    MK_REQUIRE(plan.invokes_backend);
    MK_REQUIRE(plan.renderer_draws == 1);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.uses_volumetric_clouds);
    MK_REQUIRE(plan.texture_rows.size() == 1);
    MK_REQUIRE(plan.texture_rows[0].uploads_texture);
}

MK_TEST("renderer cloud layer policy fails closed for invalid values and unsupported claims") {
    auto desc = make_valid_cloud_policy_desc();
    desc.layer.coverage = std::numeric_limits<float>::quiet_NaN();
    desc.layer.flow_map_asset_ref = "renderer/native/cloud_flow";
    desc.quality_tier = mirakana::CloudLayerQualityTier::unknown;
    desc.shader_contract_evidence_ready = false;
    desc.package_evidence_ready = false;
    desc.request_native_handle_access = true;
    desc.request_volumetric_clouds = true;

    const auto plan = mirakana::plan_cloud_layer_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::CloudLayerPolicyStatus::blocked);
    MK_REQUIRE(!plan.uploads_textures);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(
        mirakana::has_cloud_layer_diagnostic(plan, mirakana::CloudLayerDiagnosticCode::invalid_environment_layer));
    MK_REQUIRE(
        mirakana::has_cloud_layer_diagnostic(plan, mirakana::CloudLayerDiagnosticCode::unsupported_quality_tier));
    MK_REQUIRE(mirakana::has_cloud_layer_diagnostic(
        plan, mirakana::CloudLayerDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(
        mirakana::has_cloud_layer_diagnostic(plan, mirakana::CloudLayerDiagnosticCode::missing_package_evidence));
    MK_REQUIRE(mirakana::has_cloud_layer_diagnostic(
        plan, mirakana::CloudLayerDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(
        mirakana::has_cloud_layer_diagnostic(plan, mirakana::CloudLayerDiagnosticCode::unsupported_volumetric_clouds));
}

int main() {
    return mirakana::test::run_all();
}
