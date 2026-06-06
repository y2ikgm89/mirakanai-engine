// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/material_weathering_policy.hpp"

#include <algorithm>
#include <array>

namespace {

[[nodiscard]] mirakana::EnvironmentMaterialWeatheringPlan make_environment_weathering_plan() {
    mirakana::EnvironmentProfileDesc profile{};
    profile.id = "renderer_material_weathering";
    profile.weather = mirakana::EnvironmentWeatherKind::storm;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.7F,
        .particle_radius_mm = 1.0F,
        .fall_speed_mps = 7.0F,
        .wind_speed_mps = 2.0F,
    };
    return mirakana::plan_environment_material_weathering(mirakana::EnvironmentMaterialWeatheringDesc{
        .environment = profile,
        .snow_accumulation = 0.5F,
        .ice_intensity = 0.2F,
    });
}

[[nodiscard]] mirakana::MaterialWeatheringPolicyDesc make_policy_desc() {
    return mirakana::MaterialWeatheringPolicyDesc{
        .environment_plan = make_environment_weathering_plan(),
        .quality_tier = mirakana::MaterialWeatheringQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
        .request_material_parameter_binding = true,
        .request_backend_execution = true,
        .material_parameter_binding_count = 1,
        .material_constant_bytes_uploaded = 64,
        .backend_invocation_count = 1,
    };
}

} // namespace

MK_TEST("renderer material weathering policy promotes wet and snow material parameter binding evidence") {
    const mirakana::MaterialWeatheringPolicyPlan plan = mirakana::plan_material_weathering_policy(make_policy_desc());

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.status == mirakana::MaterialWeatheringPolicyStatus::ready);
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(plan.package_evidence_ready);
    MK_REQUIRE(plan.execution_evidence_ready);
    MK_REQUIRE(plan.material_parameter_bindings == 1);
    MK_REQUIRE(plan.material_constant_bytes_uploaded == 64);
    MK_REQUIRE(plan.invokes_backend);
    MK_REQUIRE(plan.backend_invocations == 1);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_source_materials);

    MK_REQUIRE(plan.constant_rows.size() == 1);
    MK_REQUIRE(plan.constant_rows[0].constants_binding_slot == mirakana::material_weathering_constants_binding());
    MK_REQUIRE(plan.constant_rows[0].constant_bytes == mirakana::material_weathering_constants_byte_size());
    MK_REQUIRE(plan.constant_rows[0].state == mirakana::EnvironmentMaterialWeatheringState::mixed);
    MK_REQUIRE(plan.wet_rows.size() == 1);
    MK_REQUIRE(plan.snow_rows.size() == 1);
    MK_REQUIRE(plan.ice_rows.size() == 1);
    MK_REQUIRE(plan.quality_rows.size() == 1);
    MK_REQUIRE(plan.quality_rows[0].ready);
}

MK_TEST("renderer material weathering constants pack wetness snow and source mutation guard") {
    const auto desc = make_policy_desc();
    std::array<std::uint8_t, mirakana::material_weathering_constants_byte_size()> bytes{};

    mirakana::pack_material_weathering_constants(bytes, desc);

    const bool any_nonzero = std::ranges::any_of(bytes, [](std::uint8_t value) { return value != 0U; });
    MK_REQUIRE(any_nonzero);
}

MK_TEST("renderer material weathering policy fails closed without execution or with side-effect requests") {
    auto desc = make_policy_desc();
    desc.execution_evidence_ready = false;
    desc.material_parameter_binding_count = 0;
    desc.material_constant_bytes_uploaded = 0;
    desc.backend_invocation_count = 0;
    desc.request_native_handle_access = true;
    desc.request_source_material_mutation = true;

    const mirakana::MaterialWeatheringPolicyPlan plan = mirakana::plan_material_weathering_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::MaterialWeatheringPolicyStatus::blocked);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.mutates_source_materials);
    MK_REQUIRE(mirakana::has_material_weathering_policy_diagnostic(
        plan, mirakana::MaterialWeatheringPolicyDiagnosticCode::missing_execution_evidence));
    MK_REQUIRE(mirakana::has_material_weathering_policy_diagnostic(
        plan, mirakana::MaterialWeatheringPolicyDiagnosticCode::missing_material_parameter_binding_evidence));
    MK_REQUIRE(mirakana::has_material_weathering_policy_diagnostic(
        plan, mirakana::MaterialWeatheringPolicyDiagnosticCode::unsupported_native_handle_claim));
    MK_REQUIRE(mirakana::has_material_weathering_policy_diagnostic(
        plan, mirakana::MaterialWeatheringPolicyDiagnosticCode::unsupported_source_material_mutation));
}

int main() {
    return mirakana::test::run_all();
}
