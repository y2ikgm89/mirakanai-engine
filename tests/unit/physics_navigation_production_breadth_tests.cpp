// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/navigation/navigation_production_breadth.hpp"
#include "mirakana/physics/physics_production_breadth.hpp"

#include <vector>

namespace {

[[nodiscard]] mirakana::PhysicsProductionBreadthEvidenceRow
physics_row(mirakana::PhysicsProductionBreadthFeature feature, const char* feature_id) {
    return mirakana::PhysicsProductionBreadthEvidenceRow{
        .feature = feature,
        .proof = mirakana::PhysicsProductionBreadthProof::first_party_value_contract,
        .feature_id = feature_id,
        .official_source_url = "https://jrouwe.github.io/JoltPhysics/",
        .package_counter_id = feature_id,
        .adapter_boundary_id = {},
        .host_validation_recipe_id = {},
        .adapter_lifecycle_reviewed = false,
        .reviewed = true,
        .host_validated = true,
        .package_visible = true,
        .deterministic_replay = true,
        .body_budget = 64U,
        .row_budget = 16U,
    };
}

[[nodiscard]] std::vector<mirakana::PhysicsProductionBreadthEvidenceRow> physics_ready_rows() {
    return {
        physics_row(mirakana::PhysicsProductionBreadthFeature::oriented_box_query, "physics.query.obb"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::convex_query, "physics.query.convex"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::mesh_query, "physics.query.mesh"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::persistent_joint_asset, "physics.joint.asset"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::ragdoll_constraint_group, "physics.ragdoll.group"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::controller_tuning, "physics.controller.tuning"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::vehicle_policy, "physics.vehicle.policy"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::deterministic_replay_signature,
                    "physics.replay.signature"),
        physics_row(mirakana::PhysicsProductionBreadthFeature::native_adapter_capacity, "physics.native.capacity"),
    };
}

[[nodiscard]] mirakana::NavigationProductionBreadthEvidenceRow
navigation_row(mirakana::NavigationProductionBreadthFeature feature, const char* feature_id) {
    return mirakana::NavigationProductionBreadthEvidenceRow{
        .feature = feature,
        .proof = mirakana::NavigationProductionBreadthProof::first_party_value_contract,
        .feature_id = feature_id,
        .official_source_url = "https://recastnav.com/",
        .package_counter_id = feature_id,
        .adapter_boundary_id = {},
        .host_validation_recipe_id = {},
        .adapter_lifecycle_reviewed = false,
        .reviewed = true,
        .host_validated = true,
        .package_visible = true,
        .deterministic_path = true,
        .agent_budget = 8U,
        .row_budget = 16U,
    };
}

[[nodiscard]] std::vector<mirakana::NavigationProductionBreadthEvidenceRow> navigation_ready_rows() {
    return {
        navigation_row(mirakana::NavigationProductionBreadthFeature::navmesh_source_import,
                       "navigation.navmesh.import"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::navmesh_bake_config, "navigation.navmesh.bake"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::agent_dimension_gate,
                       "navigation.agent.dimensions"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::polygon_corridor, "navigation.detour.corridor"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::string_pulling_path,
                       "navigation.detour.string_pulling"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::dynamic_obstacle_tile_update,
                       "navigation.tile_cache.obstacle"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::tiled_navmesh_reference,
                       "navigation.tiled_navmesh"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::hierarchical_region_streaming,
                       "navigation.region.streaming"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::crowd_local_avoidance_budget,
                       "navigation.crowd.avoidance"),
        navigation_row(mirakana::NavigationProductionBreadthFeature::nav_data_streaming_readiness,
                       "navigation.navdata.streaming"),
    };
}

} // namespace

MK_TEST("physics production breadth review accepts complete first party evidence") {
    const auto rows = physics_ready_rows();

    const auto review = mirakana::review_physics_production_breadth(
        mirakana::PhysicsProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::PhysicsProductionBreadthStatus::ready);
    MK_REQUIRE(review.diagnostic == mirakana::PhysicsProductionBreadthDiagnostic::none);
    MK_REQUIRE(review.required_features_ready == rows.size());
    MK_REQUIRE(review.package_visible_rows == rows.size());
    MK_REQUIRE(review.deterministic_replay_rows == rows.size());
    MK_REQUIRE(review.review_hash != 0U);
}

MK_TEST("physics production breadth review fails closed on missing and unsafe evidence") {
    auto rows = physics_ready_rows();
    rows.pop_back();
    rows[0].native_handles_exposed = true;
    rows[1].reviewed = false;
    rows[2].official_source_url.clear();
    rows[3].body_budget = 0U;
    rows[4].claims_broad_middleware_parity = true;

    const auto review = mirakana::review_physics_production_breadth(
        mirakana::PhysicsProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::PhysicsProductionBreadthStatus::diagnostics);
    MK_REQUIRE(review.diagnostic == mirakana::PhysicsProductionBreadthDiagnostic::native_handles_exposed);
    MK_REQUIRE(review.diagnostics.size() == 6U);
    MK_REQUIRE(review.diagnostics[1] == mirakana::PhysicsProductionBreadthDiagnostic::missing_review);
    MK_REQUIRE(review.diagnostics[2] == mirakana::PhysicsProductionBreadthDiagnostic::missing_official_source);
    MK_REQUIRE(review.diagnostics[3] == mirakana::PhysicsProductionBreadthDiagnostic::missing_budget);
    MK_REQUIRE(review.diagnostics[4] == mirakana::PhysicsProductionBreadthDiagnostic::unsupported_broad_claim);
    MK_REQUIRE(review.diagnostics[5] == mirakana::PhysicsProductionBreadthDiagnostic::missing_required_feature);
}

MK_TEST("physics production breadth review separates host gated optional adapter evidence") {
    auto rows = physics_ready_rows();
    rows[8].proof = mirakana::PhysicsProductionBreadthProof::optional_native_adapter;
    rows[8].adapter_boundary_id = "MK_physics_jolt";
    rows[8].host_validation_recipe_id = "validate-physics-jolt";
    rows[8].adapter_lifecycle_reviewed = true;
    rows[8].host_validated = false;
    rows[8].host_gated = true;
    rows[8].dependency_legal_recorded = true;

    const auto review = mirakana::review_physics_production_breadth(
        mirakana::PhysicsProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::PhysicsProductionBreadthStatus::host_evidence_required);
    MK_REQUIRE(review.diagnostic == mirakana::PhysicsProductionBreadthDiagnostic::none);
    MK_REQUIRE(review.host_gated_rows == 1U);
}

MK_TEST("physics production breadth review requires optional adapter boundary evidence") {
    auto rows = physics_ready_rows();
    rows[8].proof = mirakana::PhysicsProductionBreadthProof::optional_native_adapter;
    rows[8].host_validated = false;
    rows[8].host_gated = true;
    rows[8].dependency_legal_recorded = true;

    const auto review = mirakana::review_physics_production_breadth(
        mirakana::PhysicsProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::PhysicsProductionBreadthStatus::diagnostics);
    MK_REQUIRE(review.diagnostic == mirakana::PhysicsProductionBreadthDiagnostic::missing_adapter_boundary);
    MK_REQUIRE(review.diagnostics.size() == 4U);
    MK_REQUIRE(review.diagnostics[1] == mirakana::PhysicsProductionBreadthDiagnostic::missing_host_validation_recipe);
    MK_REQUIRE(review.diagnostics[2] == mirakana::PhysicsProductionBreadthDiagnostic::missing_adapter_lifecycle_review);
    MK_REQUIRE(review.diagnostics[3] == mirakana::PhysicsProductionBreadthDiagnostic::missing_required_feature);
}

MK_TEST("navigation production breadth review accepts complete first party evidence") {
    const auto rows = navigation_ready_rows();

    const auto review = mirakana::review_navigation_production_breadth(
        mirakana::NavigationProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::NavigationProductionBreadthStatus::ready);
    MK_REQUIRE(review.diagnostic == mirakana::NavigationProductionBreadthDiagnostic::none);
    MK_REQUIRE(review.required_features_ready == rows.size());
    MK_REQUIRE(review.package_visible_rows == rows.size());
    MK_REQUIRE(review.deterministic_path_rows == rows.size());
    MK_REQUIRE(review.review_hash != 0U);
}

MK_TEST("navigation production breadth review fails closed on source mutation and native claims") {
    auto rows = navigation_ready_rows();
    rows.pop_back();
    rows[0].mutates_source_geometry = true;
    rows[1].reviewed = false;
    rows[2].official_source_url.clear();
    rows[3].agent_budget = 0U;
    rows[4].exposes_native_recast_detour_handles = true;
    rows[5].claims_arbitrary_runtime_bake = true;

    const auto review = mirakana::review_navigation_production_breadth(
        mirakana::NavigationProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::NavigationProductionBreadthStatus::diagnostics);
    MK_REQUIRE(review.diagnostic == mirakana::NavigationProductionBreadthDiagnostic::source_mutation_claimed);
    MK_REQUIRE(review.diagnostics.size() == 7U);
    MK_REQUIRE(review.diagnostics[1] == mirakana::NavigationProductionBreadthDiagnostic::missing_review);
    MK_REQUIRE(review.diagnostics[2] == mirakana::NavigationProductionBreadthDiagnostic::missing_official_source);
    MK_REQUIRE(review.diagnostics[3] == mirakana::NavigationProductionBreadthDiagnostic::missing_budget);
    MK_REQUIRE(review.diagnostics[4] == mirakana::NavigationProductionBreadthDiagnostic::native_handles_exposed);
    MK_REQUIRE(review.diagnostics[5] == mirakana::NavigationProductionBreadthDiagnostic::unsupported_broad_claim);
    MK_REQUIRE(review.diagnostics[6] == mirakana::NavigationProductionBreadthDiagnostic::missing_required_feature);
}

MK_TEST("navigation production breadth review separates host gated recast detour evidence") {
    auto rows = navigation_ready_rows();
    rows[0].proof = mirakana::NavigationProductionBreadthProof::optional_recast_detour_adapter;
    rows[0].adapter_boundary_id = "RecastDetourPrivateAdapter";
    rows[0].host_validation_recipe_id = "validate-navigation-recast-detour";
    rows[0].adapter_lifecycle_reviewed = true;
    rows[0].host_validated = false;
    rows[0].host_gated = true;
    rows[0].dependency_legal_recorded = true;

    const auto review = mirakana::review_navigation_production_breadth(
        mirakana::NavigationProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::NavigationProductionBreadthStatus::host_evidence_required);
    MK_REQUIRE(review.diagnostic == mirakana::NavigationProductionBreadthDiagnostic::none);
    MK_REQUIRE(review.host_gated_rows == 1U);
}

MK_TEST("navigation production breadth review requires recast detour adapter boundary evidence") {
    auto rows = navigation_ready_rows();
    rows[0].proof = mirakana::NavigationProductionBreadthProof::optional_recast_detour_adapter;
    rows[0].host_validated = false;
    rows[0].host_gated = true;
    rows[0].dependency_legal_recorded = true;

    const auto review = mirakana::review_navigation_production_breadth(
        mirakana::NavigationProductionBreadthReviewRequest{.rows = rows, .require_broad_readiness = true});

    MK_REQUIRE(review.status == mirakana::NavigationProductionBreadthStatus::diagnostics);
    MK_REQUIRE(review.diagnostic == mirakana::NavigationProductionBreadthDiagnostic::missing_adapter_boundary);
    MK_REQUIRE(review.diagnostics.size() == 4U);
    MK_REQUIRE(review.diagnostics[1] ==
               mirakana::NavigationProductionBreadthDiagnostic::missing_host_validation_recipe);
    MK_REQUIRE(review.diagnostics[2] ==
               mirakana::NavigationProductionBreadthDiagnostic::missing_adapter_lifecycle_review);
    MK_REQUIRE(review.diagnostics[3] == mirakana::NavigationProductionBreadthDiagnostic::missing_required_feature);
}

int main() {
    return mirakana::test::run_all();
}
