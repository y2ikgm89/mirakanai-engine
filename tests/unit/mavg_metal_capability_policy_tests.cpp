// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_metal_capability_policy.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using mirakana::MavgMetalCapabilityDiagnosticCode;
using mirakana::MavgMetalCapabilityKind;
using mirakana::MavgMetalCapabilityRow;
using mirakana::MavgMetalCapabilityRowStatus;
using mirakana::MavgMetalCapabilityStatus;

constexpr MavgMetalCapabilityKind kRequiredCapabilities[] = {
    MavgMetalCapabilityKind::streamed_cluster_draw,   MavgMetalCapabilityKind::mesh_shader_execution,
    MavgMetalCapabilityKind::gpu_memory_residency,    MavgMetalCapabilityKind::deformation_tier,
    MavgMetalCapabilityKind::ray_tracing_consistency, MavgMetalCapabilityKind::benchmark_evidence,
};

[[nodiscard]] MavgMetalCapabilityRow make_row(MavgMetalCapabilityKind capability, MavgMetalCapabilityRowStatus status,
                                              std::uint32_t source_index) {
    const auto ready = status == MavgMetalCapabilityRowStatus::ready;
    const auto host_gated = status == MavgMetalCapabilityRowStatus::host_gated;
    return MavgMetalCapabilityRow{
        .capability_id = "mavg.metal.capability",
        .capability = capability,
        .backend = mirakana::rhi::BackendKind::metal,
        .status = status,
        .reviewed = true,
        .backend_local_evidence = true,
        .apple_host_validated = ready,
        .host_gate_required = host_gated,
        .host_validation_recipe_id = "renderer-metal-apple-host-evidence",
        .package_counter_id = ready ? "mavg.metal.counter" : "",
        .request_cross_backend_inference = false,
        .request_native_handle_access = false,
        .request_gpu_execution = false,
        .request_nanite_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::MavgMetalCapabilityRequest make_request(bool include_host_evidence) {
    mirakana::MavgMetalCapabilityRequest request{
        .required_capabilities =
            std::vector<MavgMetalCapabilityKind>{kRequiredCapabilities,
                                                 kRequiredCapabilities + std::size(kRequiredCapabilities)},
        .rows = {},
        .row_budget = 32U,
        .seed = 777U,
    };

    std::uint32_t source_index{1U};
    for (const auto capability : kRequiredCapabilities) {
        request.rows.push_back(make_row(capability,
                                        include_host_evidence ? MavgMetalCapabilityRowStatus::ready
                                                              : MavgMetalCapabilityRowStatus::host_gated,
                                        source_index++));
    }
    return request;
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::MavgMetalCapabilityPlan& plan,
                                           MavgMetalCapabilityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("mavg metal capability policy keeps all rows host gated without Apple host evidence") {
    const auto plan = mirakana::plan_mavg_metal_capabilities(make_request(false));

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 6U);
    MK_REQUIRE(plan.ready_row_count == 0U);
    MK_REQUIRE(plan.host_gated_row_count == 6U);
    MK_REQUIRE(plan.host_validated_capability_count == 0U);
    MK_REQUIRE(!plan.metal_mavg_ready);
    MK_REQUIRE(plan.requires_apple_host_evidence);
    MK_REQUIRE(!plan.has_apple_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.executed_gpu_commands);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.claimed_nanite_equivalence);
}

MK_TEST("mavg metal capability policy becomes ready only with reviewed Apple host evidence") {
    const auto plan = mirakana::plan_mavg_metal_capabilities(make_request(true));

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 6U);
    MK_REQUIRE(plan.ready_row_count == 6U);
    MK_REQUIRE(plan.host_gated_row_count == 0U);
    MK_REQUIRE(plan.host_validated_capability_count == 6U);
    MK_REQUIRE(plan.metal_mavg_ready);
    MK_REQUIRE(plan.requires_apple_host_evidence);
    MK_REQUIRE(plan.has_apple_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("mavg metal capability policy rejects cross backend inference and native handles") {
    auto request = make_request(false);
    request.rows[0].backend = mirakana::rhi::BackendKind::d3d12;
    request.rows[0].request_cross_backend_inference = true;
    request.rows[1].request_native_handle_access = true;

    const auto plan = mirakana::plan_mavg_metal_capabilities(request);

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_backend) == 1U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_cross_backend_inference) == 1U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(!plan.metal_mavg_ready);
}

MK_TEST("mavg metal capability policy rejects unsupported execution and Nanite claims") {
    auto request = make_request(false);
    request.rows[2].request_gpu_execution = true;
    request.rows[3].request_nanite_claim = true;

    const auto plan = mirakana::plan_mavg_metal_capabilities(request);

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_gpu_execution_claim) == 1U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_nanite_claim) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(!plan.executed_gpu_commands);
    MK_REQUIRE(!plan.claimed_nanite_equivalence);
}

MK_TEST("mavg metal capability policy keeps readiness false for invalid ready rows") {
    auto request = make_request(true);
    request.row_budget = 1U;

    const auto plan = mirakana::plan_mavg_metal_capabilities(request);

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.ready_row_count == 0U);
    MK_REQUIRE(plan.host_validated_capability_count == 0U);
    MK_REQUIRE(!plan.metal_mavg_ready);
    MK_REQUIRE(!plan.has_apple_host_evidence);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("mavg metal capability policy rejects lowercase native shaped evidence ids") {
    auto request = make_request(true);
    request.rows[0].capability_id = "mavg.mtl.buffer";
    request.rows[1].package_counter_id = "mavg.id3d12.resource";

    const auto plan = mirakana::plan_mavg_metal_capabilities(request);

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim) == 2U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::invalid_capability_row) == 2U);
    MK_REQUIRE(!plan.metal_mavg_ready);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("mavg metal capability policy validates row claims when required capabilities are empty") {
    auto request = mirakana::MavgMetalCapabilityRequest{
        .required_capabilities = {},
        .rows = {make_row(MavgMetalCapabilityKind::streamed_cluster_draw, MavgMetalCapabilityRowStatus::host_gated,
                          1U)},
        .row_budget = 8U,
        .seed = 778U,
    };
    request.rows[0].request_native_handle_access = true;

    const auto plan = mirakana::plan_mavg_metal_capabilities(request);

    MK_REQUIRE(plan.status == MavgMetalCapabilityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::missing_required_capability_row) == 0U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::invalid_capability_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim) == 1U);
    MK_REQUIRE(plan.row_count == 1U);
    MK_REQUIRE(!plan.metal_mavg_ready);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("mavg metal capability policy replay hash is stable under input row order") {
    auto lhs = make_request(false);
    auto rhs = make_request(false);
    std::ranges::reverse(rhs.rows);
    rhs.rows[0].source_index = 99U;
    lhs.rows.back().source_index = 99U;

    const auto left_plan = mirakana::plan_mavg_metal_capabilities(lhs);
    const auto right_plan = mirakana::plan_mavg_metal_capabilities(rhs);

    MK_REQUIRE(left_plan.status == MavgMetalCapabilityStatus::host_evidence_required);
    MK_REQUIRE(right_plan.status == MavgMetalCapabilityStatus::host_evidence_required);
    MK_REQUIRE(left_plan.replay_hash == right_plan.replay_hash);
}

int main() {
    return mirakana::test::run_all();
}
