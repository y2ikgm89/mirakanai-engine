// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_policy.hpp"

#include <array>

MK_TEST("renderer environment policy plans backend-neutral feature rows") {
    const std::array features = {
        mirakana::EnvironmentRenderFeatureDesc{
            .feature = mirakana::EnvironmentRenderFeature::physical_sky,
            .requires_backend_shader_evidence = true,
            .requires_backend_validation_evidence = true,
            .backend_shader_evidence_ready = true,
            .backend_validation_evidence_ready = true,
        },
        mirakana::EnvironmentRenderFeatureDesc{
            .feature = mirakana::EnvironmentRenderFeature::height_fog,
            .requires_scene_depth = true,
            .requires_backend_shader_evidence = true,
            .requires_backend_validation_evidence = true,
            .backend_shader_evidence_ready = true,
            .backend_validation_evidence_ready = true,
            .raymarch_step_budget = 24,
        },
    };

    const auto plan = mirakana::plan_environment_render_policy(mirakana::EnvironmentPolicyDesc{
        .features = features,
        .frame_extent = mirakana::Extent2D{.width = 1920, .height = 1080},
        .scene_color_available = true,
        .scene_depth_available = true,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .package_counter_evidence_ready = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.feature_count == 2);
    MK_REQUIRE(plan.environment_pass_count == 2);
    MK_REQUIRE(plan.frame_graph_pass_count == 2);
    MK_REQUIRE(plan.scene_color_required);
    MK_REQUIRE(plan.scene_depth_required);
    MK_REQUIRE(plan.backend == mirakana::rhi::BackendKind::d3d12);
    MK_REQUIRE(plan.backend_shader_evidence_ready);
    MK_REQUIRE(plan.backend_validation_evidence_ready);
    MK_REQUIRE(plan.package_counter_evidence_ready);
    MK_REQUIRE(plan.feature_rows.size() == 2);
    MK_REQUIRE(plan.feature_rows[1].uses_scene_depth);
    MK_REQUIRE(plan.feature_rows[1].raymarch_step_budget == 24);
}

MK_TEST("renderer environment policy fails closed for missing evidence and unsafe claims") {
    const std::array features = {
        mirakana::EnvironmentRenderFeatureDesc{
            .feature = mirakana::EnvironmentRenderFeature::volumetric_fog,
            .requires_scene_depth = true,
            .requires_backend_shader_evidence = true,
            .requires_backend_validation_evidence = true,
            .backend_shader_evidence_ready = false,
            .backend_validation_evidence_ready = false,
            .raymarch_step_budget = 512,
        },
    };

    const auto plan = mirakana::plan_environment_render_policy(mirakana::EnvironmentPolicyDesc{
        .features = features,
        .frame_extent = mirakana::Extent2D{.width = 1280, .height = 720},
        .scene_color_available = false,
        .scene_depth_available = false,
        .max_raymarch_step_budget = 64,
        .backend = mirakana::rhi::BackendKind::vulkan,
        .request_backend_inheritance = true,
        .request_native_handle_access = true,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(!plan.backend_shader_evidence_ready);
    MK_REQUIRE(!plan.backend_validation_evidence_ready);
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::missing_scene_color));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::missing_scene_depth));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::missing_backend_shader_evidence));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::missing_backend_validation_evidence));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::excessive_raymarch_step_budget));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::unsupported_backend_inheritance));
    MK_REQUIRE(mirakana::has_environment_policy_diagnostic(
        plan, mirakana::EnvironmentPolicyDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
