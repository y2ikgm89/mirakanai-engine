// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_production_breadth.hpp"

#include <array>
#include <limits>
#include <string_view>

namespace mirakana {
namespace {

constexpr std::array<NavigationProductionBreadthFeature, 10> required_features{
    NavigationProductionBreadthFeature::navmesh_source_import,
    NavigationProductionBreadthFeature::navmesh_bake_config,
    NavigationProductionBreadthFeature::agent_dimension_gate,
    NavigationProductionBreadthFeature::polygon_corridor,
    NavigationProductionBreadthFeature::string_pulling_path,
    NavigationProductionBreadthFeature::dynamic_obstacle_tile_update,
    NavigationProductionBreadthFeature::tiled_navmesh_reference,
    NavigationProductionBreadthFeature::hierarchical_region_streaming,
    NavigationProductionBreadthFeature::crowd_local_avoidance_budget,
    NavigationProductionBreadthFeature::nav_data_streaming_readiness,
};

[[nodiscard]] constexpr std::size_t feature_index(NavigationProductionBreadthFeature feature) noexcept {
    return static_cast<std::size_t>(feature);
}

[[nodiscard]] constexpr bool valid_feature(NavigationProductionBreadthFeature feature) noexcept {
    return feature_index(feature) < required_features.size();
}

void append_diagnostic(NavigationProductionBreadthReview& review, NavigationProductionBreadthDiagnostic diagnostic) {
    if (review.diagnostics.empty()) {
        review.diagnostic = diagnostic;
    }
    review.diagnostics.push_back(diagnostic);
}

void hash_byte(std::uint64_t& hash, std::uint8_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_uint64(std::uint64_t& hash, std::uint64_t value) noexcept {
    for (auto shift = 0U; shift < 64U; shift += 8U) {
        hash_byte(hash, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto character : value) {
        hash_byte(hash, static_cast<std::uint8_t>(character));
    }
    hash_byte(hash, 0U);
}

void add_budget(std::uint64_t& target, std::uint64_t value) noexcept {
    if (target > std::numeric_limits<std::uint64_t>::max() - value) {
        target = std::numeric_limits<std::uint64_t>::max();
        return;
    }
    target += value;
}

} // namespace

NavigationProductionBreadthReview
review_navigation_production_breadth(const NavigationProductionBreadthReviewRequest& request) {
    NavigationProductionBreadthReview review;
    review.status = NavigationProductionBreadthStatus::ready;
    review.review_hash = 1469598103934665603ULL;

    if (request.rows.empty()) {
        review.status = NavigationProductionBreadthStatus::no_rows;
        review.diagnostic = NavigationProductionBreadthDiagnostic::no_rows;
        return review;
    }

    std::array<bool, required_features.size()> seen{};
    std::array<bool, required_features.size()> ready_features{};

    for (const auto& row : request.rows) {
        auto row_ready = true;
        hash_uint64(review.review_hash, static_cast<std::uint64_t>(row.feature));
        hash_uint64(review.review_hash, static_cast<std::uint64_t>(row.proof));
        hash_string(review.review_hash, row.feature_id);
        hash_string(review.review_hash, row.official_source_url);
        hash_string(review.review_hash, row.package_counter_id);
        hash_string(review.review_hash, row.adapter_boundary_id);
        hash_string(review.review_hash, row.host_validation_recipe_id);
        hash_byte(review.review_hash, row.adapter_lifecycle_reviewed ? 1U : 0U);
        hash_byte(review.review_hash, row.reviewed ? 1U : 0U);
        hash_byte(review.review_hash, row.host_validated ? 1U : 0U);
        hash_byte(review.review_hash, row.host_gated ? 1U : 0U);
        hash_byte(review.review_hash, row.dependency_legal_recorded ? 1U : 0U);
        hash_byte(review.review_hash, row.package_visible ? 1U : 0U);
        hash_byte(review.review_hash, row.deterministic_path ? 1U : 0U);
        hash_byte(review.review_hash, row.exposes_native_recast_detour_handles ? 1U : 0U);
        hash_byte(review.review_hash, row.mutates_source_geometry ? 1U : 0U);
        hash_byte(review.review_hash, row.claims_arbitrary_runtime_bake ? 1U : 0U);
        hash_byte(review.review_hash, row.claims_broad_middleware_parity ? 1U : 0U);
        hash_uint64(review.review_hash, row.agent_budget);
        hash_uint64(review.review_hash, row.row_budget);

        if (!valid_feature(row.feature)) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::invalid_feature);
            row_ready = false;
        } else {
            const auto index = feature_index(row.feature);
            if (seen[index]) {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::duplicate_feature);
                row_ready = false;
            }
            seen[index] = true;
        }

        if (row.mutates_source_geometry) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::source_mutation_claimed);
            row_ready = false;
        }
        if (!row.reviewed) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_review);
            row_ready = false;
        } else {
            ++review.reviewed_rows;
        }
        if (row.official_source_url.empty()) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_official_source);
            row_ready = false;
        }
        if (row.agent_budget == 0U || row.row_budget == 0U) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_budget);
            row_ready = false;
        }
        if (row.exposes_native_recast_detour_handles) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::native_handles_exposed);
            row_ready = false;
        }
        if (row.claims_arbitrary_runtime_bake || row.claims_broad_middleware_parity) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::unsupported_broad_claim);
            row_ready = false;
        }
        if (request.require_broad_readiness && !row.package_visible) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_package_counter);
            row_ready = false;
        }
        if (request.require_broad_readiness && !row.deterministic_path) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_deterministic_path);
            row_ready = false;
        }
        if (row.proof == NavigationProductionBreadthProof::optional_recast_detour_adapter) {
            if (row.adapter_boundary_id.empty()) {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_adapter_boundary);
                row_ready = false;
            }
            if (row.host_validation_recipe_id.empty()) {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_host_validation_recipe);
                row_ready = false;
            }
            if (!row.adapter_lifecycle_reviewed) {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_adapter_lifecycle_review);
                row_ready = false;
            }
            if (!row.dependency_legal_recorded) {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_dependency_legal_record);
                row_ready = false;
            }
        }
        if (!row.host_validated) {
            if (row.host_gated) {
                ++review.host_gated_rows;
            } else {
                append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_host_evidence);
                row_ready = false;
            }
        } else {
            ++review.host_validated_rows;
        }

        if (row.package_visible) {
            ++review.package_visible_rows;
        }
        if (row.deterministic_path) {
            ++review.deterministic_path_rows;
        }
        add_budget(review.total_agent_budget, row.agent_budget);
        add_budget(review.total_row_budget, row.row_budget);

        if (row_ready && valid_feature(row.feature)) {
            ready_features[feature_index(row.feature)] = true;
        }
    }

    if (request.require_broad_readiness) {
        auto missing_required_feature = false;
        for (std::size_t index = 0; index < required_features.size(); ++index) {
            if (!seen[index] || !ready_features[index]) {
                missing_required_feature = true;
            }
        }
        if (missing_required_feature) {
            append_diagnostic(review, NavigationProductionBreadthDiagnostic::missing_required_feature);
        }
    }

    for (const auto ready : ready_features) {
        if (ready) {
            ++review.required_features_ready;
        }
    }

    if (!review.diagnostics.empty()) {
        review.status = NavigationProductionBreadthStatus::diagnostics;
    } else if (review.host_gated_rows > 0U) {
        review.status = NavigationProductionBreadthStatus::host_evidence_required;
    }

    return review;
}

} // namespace mirakana
