// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_parity.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::EnvironmentBackendParityCounterSemantics;
using mirakana::EnvironmentBackendParityDiagnosticCode;
using mirakana::EnvironmentBackendParityFeature;
using mirakana::EnvironmentBackendParityRowStatus;
using mirakana::EnvironmentBackendParityStatus;

constexpr EnvironmentBackendParityFeature kRequiredFeatures[] = {
    EnvironmentBackendParityFeature::profile_v2,
    EnvironmentBackendParityFeature::physical_sky,
    EnvironmentBackendParityFeature::height_fog,
    EnvironmentBackendParityFeature::volumetric_fog,
    EnvironmentBackendParityFeature::volumetric_cloud,
    EnvironmentBackendParityFeature::rain_precipitation,
    EnvironmentBackendParityFeature::ibl,
};

[[nodiscard]] std::string feature_id(EnvironmentBackendParityFeature feature) {
    switch (feature) {
    case EnvironmentBackendParityFeature::profile_v2:
        return "profile_v2";
    case EnvironmentBackendParityFeature::physical_sky:
        return "physical_sky";
    case EnvironmentBackendParityFeature::height_fog:
        return "height_fog";
    case EnvironmentBackendParityFeature::volumetric_fog:
        return "volumetric_fog";
    case EnvironmentBackendParityFeature::volumetric_cloud:
        return "volumetric_cloud";
    case EnvironmentBackendParityFeature::rain_precipitation:
        return "rain_precipitation";
    case EnvironmentBackendParityFeature::ibl:
        return "ibl";
    }
    return "unknown";
}

[[nodiscard]] std::string aggregate_recipe_id(mirakana::rhi::BackendKind backend) {
    switch (backend) {
    case mirakana::rhi::BackendKind::d3d12:
        return "desktop-runtime-sample-game-environment-ready-aggregate";
    case mirakana::rhi::BackendKind::vulkan:
        return "desktop-runtime-sample-game-environment-vulkan-strict-aggregate";
    case mirakana::rhi::BackendKind::metal:
        return "renderer-metal-environment-aggregate-apple-host-evidence";
    case mirakana::rhi::BackendKind::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] std::string host_recipe_id(mirakana::rhi::BackendKind backend) {
    switch (backend) {
    case mirakana::rhi::BackendKind::d3d12:
        return "d3d12-windows-primary";
    case mirakana::rhi::BackendKind::vulkan:
        return "vulkan-strict";
    case mirakana::rhi::BackendKind::metal:
        return "metal-apple";
    case mirakana::rhi::BackendKind::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] std::vector<mirakana::EnvironmentBackendParityCounterExpectation>
counter_expectations(const std::string& id) {
    return {
        mirakana::EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".ready",
            .semantics = EnvironmentBackendParityCounterSemantics::exact_one,
        },
        mirakana::EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".diagnostics",
            .semantics = EnvironmentBackendParityCounterSemantics::exact_zero,
        },
    };
}

[[nodiscard]] mirakana::EnvironmentBackendParityRow make_ready_row(EnvironmentBackendParityFeature feature,
                                                                   mirakana::rhi::BackendKind backend,
                                                                   std::uint32_t source_index) {
    const auto id = feature_id(feature);
    return mirakana::EnvironmentBackendParityRow{
        .feature_id = id,
        .feature = feature,
        .backend = backend,
        .status = EnvironmentBackendParityRowStatus::ready,
        .aggregate_recipe_id = aggregate_recipe_id(backend),
        .host_validation_recipe_id = host_recipe_id(backend),
        .profile_revision = "GameEngine.EnvironmentProfile.v2:commercial-fixture-2026-06-14",
        .preset_pack_revision = "GameEngine.EnvironmentPresetPack.v1:first-party-commercial-v1",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .quality_budget_class = "environment.quality-budget.high.v1",
        .resource_class = "environment.aggregate.selected.resources.v1",
        .output_tolerance_class = "environment.visual.readback.hash-tolerance.v1",
        .counter_expectations = counter_expectations(id),
        .unsupported_row_ids =
            {
                "environment_backend_parity.unselected_platforms_host_gated",
                "environment_backend_parity.no_native_handles",
            },
        .feature_present = true,
        .backend_aggregate_ready = true,
        .quality_budget_ready = true,
        .resource_class_ready = true,
        .output_tolerance_ready = true,
        .package_counters_ready = true,
        .unsupported_rows_declared = true,
        .host_validated = true,
        .host_gate_required = false,
        .diagnostic_count = 0U,
        .fallback_used = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::EnvironmentBackendParityRow make_metal_host_gated_row(EnvironmentBackendParityFeature feature,
                                                                              std::uint32_t source_index) {
    auto row = make_ready_row(feature, mirakana::rhi::BackendKind::metal, source_index);
    row.status = EnvironmentBackendParityRowStatus::host_gated;
    row.backend_aggregate_ready = false;
    row.quality_budget_ready = false;
    row.resource_class_ready = false;
    row.output_tolerance_ready = false;
    row.package_counters_ready = false;
    row.host_validated = false;
    row.host_gate_required = true;
    return row;
}

[[nodiscard]] mirakana::EnvironmentBackendParityRequest make_request(bool include_metal_host_evidence) {
    std::vector<mirakana::EnvironmentBackendParityRow> rows;
    rows.reserve(21U);
    std::uint32_t source_index{1U};
    for (const auto feature : kRequiredFeatures) {
        rows.push_back(make_ready_row(feature, mirakana::rhi::BackendKind::d3d12, source_index++));
        rows.push_back(make_ready_row(feature, mirakana::rhi::BackendKind::vulkan, source_index++));
        rows.push_back(include_metal_host_evidence
                           ? make_ready_row(feature, mirakana::rhi::BackendKind::metal, source_index++)
                           : make_metal_host_gated_row(feature, source_index++));
    }

    return mirakana::EnvironmentBackendParityRequest{
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .rows = std::move(rows),
        .expected_profile_revision = "GameEngine.EnvironmentProfile.v2:commercial-fixture-2026-06-14",
        .expected_preset_pack_revision = "GameEngine.EnvironmentPresetPack.v1:first-party-commercial-v1",
        .expected_package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .expected_quality_tier = "high",
        .expected_quality_budget_class = "environment.quality-budget.high.v1",
        .expected_resource_class = "environment.aggregate.selected.resources.v1",
        .expected_output_tolerance_class = "environment.visual.readback.hash-tolerance.v1",
        .expected_unsupported_row_ids =
            {
                "environment_backend_parity.unselected_platforms_host_gated",
                "environment_backend_parity.no_native_handles",
            },
        .row_budget = 64U,
        .seed = 789U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::EnvironmentBackendParityPlan& plan,
                                           EnvironmentBackendParityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("environment backend parity remains host gated without Apple Metal aggregate evidence") {
    const auto plan = mirakana::plan_environment_backend_parity(make_request(false));

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 21U);
    MK_REQUIRE(plan.ready_row_count == 14U);
    MK_REQUIRE(plan.host_gated_row_count == 7U);
    MK_REQUIRE(plan.required_feature_count == 7U);
    MK_REQUIRE(plan.host_validated_backend_count == 2U);
    MK_REQUIRE(plan.d3d12_primary_ready);
    MK_REQUIRE(plan.vulkan_strict_ready);
    MK_REQUIRE(!plan.metal_host_ready);
    MK_REQUIRE(plan.requires_metal_host_evidence);
    MK_REQUIRE(!plan.environment_backend_parity_ready);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment backend parity is ready only when all selected backends match") {
    const auto plan = mirakana::plan_environment_backend_parity(make_request(true));

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 21U);
    MK_REQUIRE(plan.ready_row_count == 21U);
    MK_REQUIRE(plan.host_gated_row_count == 0U);
    MK_REQUIRE(plan.host_validated_backend_count == 3U);
    MK_REQUIRE(plan.d3d12_primary_ready);
    MK_REQUIRE(plan.vulkan_strict_ready);
    MK_REQUIRE(plan.metal_host_ready);
    MK_REQUIRE(plan.environment_backend_parity_ready);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment backend parity rejects stale profile preset and package revisions") {
    auto request = make_request(true);
    request.rows[0].profile_revision = "GameEngine.EnvironmentProfile.v2:stale";
    request.rows[1].preset_pack_revision = "GameEngine.EnvironmentPresetPack.v1:stale";
    request.rows[2].package_revision = "sample_desktop_runtime_game:stale";

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::stale_profile_revision) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::stale_preset_pack_revision) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::stale_package_revision) == 1U);
    MK_REQUIRE(!plan.environment_backend_parity_ready);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("environment backend parity rejects missing feature rows and duplicates") {
    auto request = make_request(true);
    request.rows.pop_back();
    request.rows.push_back(request.rows.front());
    request.rows.back().source_index = 99U;

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_required_feature_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::duplicate_feature_row) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("environment backend parity requires d3d12 vulkan and metal backends") {
    auto request = make_request(true);
    request.required_backends = {
        mirakana::rhi::BackendKind::d3d12,
        mirakana::rhi::BackendKind::vulkan,
    };

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_required_backend) == 1U);
    MK_REQUIRE(!plan.environment_backend_parity_ready);
}

MK_TEST("environment backend parity rejects feature-count-only equality") {
    auto request = make_request(true);
    request.rows[1].feature_id = "different_feature_with_same_count";

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::feature_id_mismatch) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("environment backend parity rejects silent feature disables and fallback claims") {
    auto request = make_request(true);
    auto& row = request.rows[0];
    row.feature_present = false;
    row.backend_aggregate_ready = false;
    row.quality_budget_ready = false;
    row.resource_class_ready = false;
    row.output_tolerance_ready = false;
    row.package_counters_ready = false;
    row.unsupported_rows_declared = false;
    row.diagnostic_count = 2U;
    row.fallback_used = true;
    row.native_handle_access = true;
    row.inferred_from_other_backend = true;

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_feature_presence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_backend_aggregate_evidence) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_quality_budget_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_resource_class_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_output_tolerance_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::missing_package_counter_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::unsupported_row_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::diagnostics_nonzero) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::unsupported_fallback) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::unsupported_native_handle_access) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::unsupported_inferred_backend) == 1U);
}

MK_TEST("environment backend parity rejects package counter tolerance and quality mismatches") {
    auto request = make_request(true);
    request.rows[0].quality_tier = "ultra";
    request.rows[1].quality_budget_class = "environment.quality-budget.ultra.v1";
    request.rows[2].resource_class = "environment.aggregate.other";
    request.rows[3].output_tolerance_class = "visual.screenshot-only";
    request.rows[4].counter_expectations[0].semantics = EnvironmentBackendParityCounterSemantics::min_positive;
    request.rows[5].unsupported_row_ids = {"environment_backend_parity.no_native_handles"};

    const auto plan = mirakana::plan_environment_backend_parity(request);

    MK_REQUIRE(plan.status == EnvironmentBackendParityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::quality_tier_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::quality_budget_class_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::resource_class_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::output_tolerance_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::package_counter_semantics_mismatch) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentBackendParityDiagnosticCode::unsupported_row_mismatch) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
