// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class PhysicsProductionBreadthStatus : std::uint8_t {
    ready,
    host_evidence_required,
    diagnostics,
    no_rows,
    invalid_request,
};

enum class PhysicsProductionBreadthFeature : std::uint8_t {
    oriented_box_query,
    convex_query,
    mesh_query,
    persistent_joint_asset,
    ragdoll_constraint_group,
    controller_tuning,
    vehicle_policy,
    deterministic_replay_signature,
    native_adapter_capacity,
};

enum class PhysicsProductionBreadthProof : std::uint8_t {
    first_party_value_contract,
    reviewed_asset,
    package_counter,
    host_validation,
    optional_native_adapter,
    deterministic_replay,
    explicit_non_goal,
};

enum class PhysicsProductionBreadthDiagnostic : std::uint8_t {
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
    missing_adapter_boundary,
    missing_host_validation_recipe,
    missing_adapter_lifecycle_review,
    missing_deterministic_replay,
    missing_budget,
    native_handles_exposed,
    unsupported_broad_claim,
};

struct PhysicsProductionBreadthEvidenceRow {
    PhysicsProductionBreadthFeature feature{PhysicsProductionBreadthFeature::oriented_box_query};
    PhysicsProductionBreadthProof proof{PhysicsProductionBreadthProof::first_party_value_contract};
    std::string feature_id;
    std::string official_source_url;
    std::string package_counter_id;
    std::string adapter_boundary_id;
    std::string host_validation_recipe_id;
    bool adapter_lifecycle_reviewed{false};
    bool reviewed{false};
    bool host_validated{false};
    bool host_gated{false};
    bool dependency_legal_recorded{false};
    bool package_visible{false};
    bool deterministic_replay{false};
    std::uint64_t body_budget{0};
    std::uint64_t row_budget{0};
    bool native_handles_exposed{false};
    bool claims_broad_middleware_parity{false};
};

struct PhysicsProductionBreadthReviewRequest {
    std::span<const PhysicsProductionBreadthEvidenceRow> rows;
    bool require_broad_readiness{false};
};

struct PhysicsProductionBreadthReview {
    PhysicsProductionBreadthStatus status{PhysicsProductionBreadthStatus::invalid_request};
    PhysicsProductionBreadthDiagnostic diagnostic{PhysicsProductionBreadthDiagnostic::none};
    std::vector<PhysicsProductionBreadthDiagnostic> diagnostics;
    std::size_t required_features_ready{0};
    std::size_t reviewed_rows{0};
    std::size_t host_validated_rows{0};
    std::size_t host_gated_rows{0};
    std::size_t package_visible_rows{0};
    std::size_t deterministic_replay_rows{0};
    std::uint64_t total_body_budget{0};
    std::uint64_t total_row_budget{0};
    std::uint64_t review_hash{0};
};

[[nodiscard]] PhysicsProductionBreadthReview
review_physics_production_breadth(const PhysicsProductionBreadthReviewRequest& request);

} // namespace mirakana
