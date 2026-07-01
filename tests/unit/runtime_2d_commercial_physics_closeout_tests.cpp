// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/two_d_commercial_physics_closeout.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::Runtime2DCommercialPhysicsCloseoutDesc;
using mirakana::runtime::Runtime2DCommercialPhysicsDiagnostic;
using mirakana::runtime::Runtime2DCommercialPhysicsDiagnosticCode;
using mirakana::runtime::Runtime2DCommercialPhysicsFeatureKind;
using mirakana::runtime::Runtime2DCommercialPhysicsFeatureRow;
using mirakana::runtime::Runtime2DCommercialPhysicsSourceKind;
using mirakana::runtime::Runtime2DCommercialPhysicsSourceRow;

[[nodiscard]] bool has_diagnostic(const std::vector<Runtime2DCommercialPhysicsDiagnostic>& diagnostics,
                                  Runtime2DCommercialPhysicsDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] Runtime2DCommercialPhysicsFeatureRow feature(Runtime2DCommercialPhysicsFeatureKind kind, std::string id) {
    return Runtime2DCommercialPhysicsFeatureRow{
        .id = std::move(id),
        .kind = kind,
        .validation_recipe_id = "installed-2d-commercial-physics-closeout-smoke",
        .package_counter_prefix = "2d_physics_runtime_extension",
        .ready = true,
        .package_visible = true,
        .deterministic = true,
        .selected_2d_scope = true,
        .simulation_runs = 2U,
        .time_of_impact_rows = kind == Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd ? 5U : 0U,
        .exact_sweep_shape_pair_rows = kind == Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd ? 3U : 0U,
        .kinematic_contact_rows = kind == Runtime2DCommercialPhysicsFeatureKind::kinematic_contact_resolution ? 1U : 0U,
        .joint_rows = kind == Runtime2DCommercialPhysicsFeatureKind::joint_constraints ? 4U : 0U,
        .trigger_event_rows = kind == Runtime2DCommercialPhysicsFeatureKind::trigger_area_events ? 2U : 0U,
        .deterministic_replay_hash =
            kind == Runtime2DCommercialPhysicsFeatureKind::deterministic_replay ? 0x2DCE'2026U : 0U,
    };
}

[[nodiscard]] std::vector<Runtime2DCommercialPhysicsFeatureRow> make_feature_rows() {
    return {
        feature(Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd, "physics.toi-ccd"),
        feature(Runtime2DCommercialPhysicsFeatureKind::joint_constraints, "physics.joint-constraints"),
        feature(Runtime2DCommercialPhysicsFeatureKind::trigger_area_events, "physics.trigger-area-events"),
        feature(Runtime2DCommercialPhysicsFeatureKind::kinematic_contact_resolution,
                "physics.kinematic-contact-resolution"),
        feature(Runtime2DCommercialPhysicsFeatureKind::deterministic_replay, "physics.deterministic-replay"),
        feature(Runtime2DCommercialPhysicsFeatureKind::package_counters, "physics.package-counters"),
    };
}

[[nodiscard]] std::array<Runtime2DCommercialPhysicsSourceRow, 4U> make_source_rows() {
    return {
        Runtime2DCommercialPhysicsSourceRow{
            .id = "repository.physics2d-runtime-api",
            .kind = Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_api,
            .source_path_or_url = "engine/physics/include/mirakana/physics/physics2d.hpp",
            .ready = true,
            .reviewed = true,
            .first_party_or_official = true,
            .public_docs_or_tracked_repo = true,
        },
        Runtime2DCommercialPhysicsSourceRow{
            .id = "repository.physics2d-runtime-tests",
            .kind = Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_tests,
            .source_path_or_url = "tests/unit/physics2d_runtime_extension_tests.cpp",
            .ready = true,
            .reviewed = true,
            .first_party_or_official = true,
            .public_docs_or_tracked_repo = true,
        },
        Runtime2DCommercialPhysicsSourceRow{
            .id = "repository.sample-package-counter-contract",
            .kind = Runtime2DCommercialPhysicsSourceKind::sample_package_counter_contract,
            .source_path_or_url = "games/sample_2d_desktop_runtime_package/game.agent.json",
            .ready = true,
            .reviewed = true,
            .first_party_or_official = true,
            .public_docs_or_tracked_repo = true,
        },
        Runtime2DCommercialPhysicsSourceRow{
            .id = "repository.legal-policy",
            .kind = Runtime2DCommercialPhysicsSourceKind::repository_legal_policy,
            .source_path_or_url = "docs/legal-and-licensing.md",
            .ready = true,
            .reviewed = true,
            .first_party_or_official = true,
            .public_docs_or_tracked_repo = true,
        },
    };
}

[[nodiscard]] Runtime2DCommercialPhysicsCloseoutDesc make_ready_desc() {
    return Runtime2DCommercialPhysicsCloseoutDesc{
        .feature_rows = make_feature_rows(),
        .source_rows = make_source_rows(),
        .selected_package_physics_closeout_claim = true,
    };
}

} // namespace

MK_TEST("runtime 2d commercial physics closeout accepts selected package evidence") {
    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_physics_closeout(make_ready_desc());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.feature_gate_ready);
    MK_REQUIRE(result.source_gate_ready);
    MK_REQUIRE(result.package_counter_gate_ready);
    MK_REQUIRE(result.deterministic_replay_ready);
    MK_REQUIRE(result.feature_rows == 6U);
    MK_REQUIRE(result.source_rows == 4U);
    MK_REQUIRE(result.package_visible_feature_rows == 6U);
    MK_REQUIRE(result.deterministic_feature_rows == 6U);
    MK_REQUIRE(result.simulation_runs == 2U);
    MK_REQUIRE(result.time_of_impact_rows == 5U);
    MK_REQUIRE(result.exact_sweep_shape_pair_rows == 3U);
    MK_REQUIRE(result.kinematic_contact_rows == 1U);
    MK_REQUIRE(result.joint_rows == 4U);
    MK_REQUIRE(result.trigger_event_rows == 2U);
    MK_REQUIRE(result.deterministic_replay_hash != 0U);
    MK_REQUIRE(result.diagnostic_rows == 0U);
    MK_REQUIRE(result.native_handle_access_rows == 0U);
    MK_REQUIRE(result.middleware_dispatch_rows == 0U);
    MK_REQUIRE(result.dynamic_vs_dynamic_ccd_claim_rows == 0U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
    MK_REQUIRE(result.legal_approval_claim_rows == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime 2d commercial physics closeout rejects duplicate feature and source rows") {
    auto desc = make_ready_desc();
    desc.feature_rows[5] = feature(Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd, "duplicate-toi-ccd");
    desc.source_rows[3] = Runtime2DCommercialPhysicsSourceRow{
        .id = "duplicate.repository.physics2d-runtime-api",
        .kind = Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_api,
        .source_path_or_url = "engine/physics/include/mirakana/physics/physics2d.hpp",
        .ready = true,
        .reviewed = true,
        .first_party_or_official = true,
        .public_docs_or_tracked_repo = true,
    };

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_physics_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(!result.feature_gate_ready);
    MK_REQUIRE(!result.source_gate_ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::feature_not_ready));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::source_not_ready));
}

MK_TEST("runtime 2d commercial physics closeout rejects unsafe broad claims") {
    auto desc = make_ready_desc();
    desc.dynamic_vs_dynamic_ccd_broad_claim = true;
    desc.physics_middleware_claim = true;
    desc.broad_physics_parity_claim = true;
    desc.public_native_handles = true;
    desc.external_engine_compatibility_claim = true;
    desc.cross_platform_parity_claim = true;
    desc.legal_approval_claim = true;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_physics_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.dynamic_vs_dynamic_ccd_claim_rows == 1U);
    MK_REQUIRE(result.physics_middleware_claim_rows == 1U);
    MK_REQUIRE(result.broad_physics_parity_claim_rows == 1U);
    MK_REQUIRE(result.native_handle_access_rows == 1U);
    MK_REQUIRE(result.external_engine_claim_rows == 1U);
    MK_REQUIRE(result.cross_platform_parity_claim_rows == 1U);
    MK_REQUIRE(result.legal_approval_claim_rows == 1U);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialPhysicsDiagnosticCode::dynamic_vs_dynamic_ccd_broad_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::physics_middleware_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::broad_physics_parity_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::public_native_handles));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialPhysicsDiagnosticCode::external_engine_compatibility_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::cross_platform_parity_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::legal_approval_claim));
}

int main() {
    return mirakana::test::run_all();
}
