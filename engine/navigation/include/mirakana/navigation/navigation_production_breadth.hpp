// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class NavigationProductionBreadthStatus : std::uint8_t {
    ready,
    host_evidence_required,
    diagnostics,
    no_rows,
    invalid_request,
};

enum class NavigationProductionBreadthFeature : std::uint8_t {
    navmesh_source_import,
    navmesh_bake_config,
    agent_dimension_gate,
    polygon_corridor,
    string_pulling_path,
    dynamic_obstacle_tile_update,
    tiled_navmesh_reference,
    hierarchical_region_streaming,
    crowd_local_avoidance_budget,
    nav_data_streaming_readiness,
};

enum class NavigationProductionBreadthProof : std::uint8_t {
    first_party_value_contract,
    reviewed_nav_asset,
    package_counter,
    host_validation,
    optional_recast_detour_adapter,
    deterministic_path,
    explicit_non_goal,
};

enum class NavigationProductionBreadthDiagnostic : std::uint8_t {
    none,
    no_rows,
    invalid_feature,
    duplicate_feature,
    missing_required_feature,
    missing_review,
    missing_official_source,
    missing_host_evidence,
    missing_package_counter,
    missing_dependency_legal_record,
    missing_deterministic_path,
    missing_budget,
    native_handles_exposed,
    source_mutation_claimed,
    unsupported_broad_claim,
};

struct NavigationProductionBreadthEvidenceRow {
    NavigationProductionBreadthFeature feature{NavigationProductionBreadthFeature::navmesh_source_import};
    NavigationProductionBreadthProof proof{NavigationProductionBreadthProof::first_party_value_contract};
    std::string feature_id;
    std::string official_source_url;
    std::string package_counter_id;
    bool reviewed{false};
    bool host_validated{false};
    bool host_gated{false};
    bool dependency_legal_recorded{false};
    bool package_visible{false};
    bool deterministic_path{false};
    std::uint64_t agent_budget{0};
    std::uint64_t row_budget{0};
    bool exposes_native_recast_detour_handles{false};
    bool mutates_source_geometry{false};
    bool claims_arbitrary_runtime_bake{false};
    bool claims_broad_middleware_parity{false};
};

struct NavigationProductionBreadthReviewRequest {
    std::span<const NavigationProductionBreadthEvidenceRow> rows;
    bool require_broad_readiness{false};
};

struct NavigationProductionBreadthReview {
    NavigationProductionBreadthStatus status{NavigationProductionBreadthStatus::invalid_request};
    NavigationProductionBreadthDiagnostic diagnostic{NavigationProductionBreadthDiagnostic::none};
    std::vector<NavigationProductionBreadthDiagnostic> diagnostics;
    std::size_t required_features_ready{0};
    std::size_t reviewed_rows{0};
    std::size_t host_validated_rows{0};
    std::size_t host_gated_rows{0};
    std::size_t package_visible_rows{0};
    std::size_t deterministic_path_rows{0};
    std::uint64_t total_agent_budget{0};
    std::uint64_t total_row_budget{0};
    std::uint64_t review_hash{0};
};

[[nodiscard]] NavigationProductionBreadthReview
review_navigation_production_breadth(const NavigationProductionBreadthReviewRequest& request);

} // namespace mirakana
