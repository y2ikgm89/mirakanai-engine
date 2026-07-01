// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/two_d_commercial_physics_closeout.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace mirakana::runtime {
namespace {

constexpr std::array<Runtime2DCommercialPhysicsFeatureKind, 6U> kRequiredFeatureKinds{
    Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd,
    Runtime2DCommercialPhysicsFeatureKind::joint_constraints,
    Runtime2DCommercialPhysicsFeatureKind::trigger_area_events,
    Runtime2DCommercialPhysicsFeatureKind::kinematic_contact_resolution,
    Runtime2DCommercialPhysicsFeatureKind::deterministic_replay,
    Runtime2DCommercialPhysicsFeatureKind::package_counters,
};

constexpr std::array<Runtime2DCommercialPhysicsSourceKind, 4U> kRequiredSourceKinds{
    Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_api,
    Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_tests,
    Runtime2DCommercialPhysicsSourceKind::sample_package_counter_contract,
    Runtime2DCommercialPhysicsSourceKind::repository_legal_policy,
};

void append_diagnostic(std::vector<Runtime2DCommercialPhysicsDiagnostic>& diagnostics,
                       Runtime2DCommercialPhysicsDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(Runtime2DCommercialPhysicsDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool empty_required(const std::string& value) noexcept {
    return value.empty();
}

[[nodiscard]] bool feature_has_kind_evidence(const Runtime2DCommercialPhysicsFeatureRow& row) noexcept {
    switch (row.kind) {
    case Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd:
        return row.simulation_runs > 0U && row.time_of_impact_rows > 0U && row.exact_sweep_shape_pair_rows > 0U;
    case Runtime2DCommercialPhysicsFeatureKind::joint_constraints:
        return row.joint_rows >= 4U;
    case Runtime2DCommercialPhysicsFeatureKind::trigger_area_events:
        return row.trigger_event_rows >= 2U;
    case Runtime2DCommercialPhysicsFeatureKind::kinematic_contact_resolution:
        return row.kinematic_contact_rows > 0U;
    case Runtime2DCommercialPhysicsFeatureKind::deterministic_replay:
        return row.deterministic_replay_hash != 0U;
    case Runtime2DCommercialPhysicsFeatureKind::package_counters:
        return row.simulation_runs > 0U && !empty_required(row.package_counter_prefix);
    }
    return false;
}

[[nodiscard]] bool feature_row_ready(const Runtime2DCommercialPhysicsFeatureRow& row) noexcept {
    return row.ready && row.package_visible && row.deterministic && row.selected_2d_scope && !empty_required(row.id) &&
           !empty_required(row.validation_recipe_id) && !empty_required(row.package_counter_prefix) &&
           row.diagnostics == 0U && row.native_handle_access_rows == 0U && row.middleware_dispatch_rows == 0U &&
           row.dynamic_vs_dynamic_ccd_claim_rows == 0U && feature_has_kind_evidence(row);
}

[[nodiscard]] bool source_row_ready(const Runtime2DCommercialPhysicsSourceRow& row) noexcept {
    return row.ready && row.reviewed && row.first_party_or_official && row.public_docs_or_tracked_repo &&
           !empty_required(row.id) && !empty_required(row.source_path_or_url);
}

void evaluate_feature_rows(const Runtime2DCommercialPhysicsCloseoutDesc& desc,
                           Runtime2DCommercialPhysicsCloseoutResult& result) {
    bool all_rows_ready = true;
    for (const auto& row : desc.feature_rows) {
        result.simulation_runs = std::max(result.simulation_runs, row.simulation_runs);
        result.time_of_impact_rows += row.time_of_impact_rows;
        result.exact_sweep_shape_pair_rows += row.exact_sweep_shape_pair_rows;
        result.kinematic_contact_rows += row.kinematic_contact_rows;
        result.joint_rows += row.joint_rows;
        result.trigger_event_rows += row.trigger_event_rows;
        result.deterministic_replay_hash ^= row.deterministic_replay_hash;
        result.diagnostic_rows += static_cast<std::size_t>(row.diagnostics);
        result.native_handle_access_rows += static_cast<std::size_t>(row.native_handle_access_rows);
        result.middleware_dispatch_rows += static_cast<std::size_t>(row.middleware_dispatch_rows);
        result.dynamic_vs_dynamic_ccd_claim_rows += static_cast<std::size_t>(row.dynamic_vs_dynamic_ccd_claim_rows);

        if (row.package_visible) {
            ++result.package_visible_feature_rows;
        }
        if (row.deterministic) {
            ++result.deterministic_feature_rows;
        }

        if (feature_row_ready(row)) {
            ++result.feature_rows;
        } else {
            all_rows_ready = false;
            append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::feature_not_ready, row.id,
                              "2D commercial physics closeout feature rows must be ready, deterministic, selected, "
                              "package-visible, scoped to a validation recipe, and free of unsafe counters");
        }
    }

    std::size_t required_feature_rows = 0U;
    for (const auto kind : kRequiredFeatureKinds) {
        std::size_t matching_rows = 0U;
        for (const auto& row : desc.feature_rows) {
            if (row.kind == kind && feature_row_ready(row)) {
                ++matching_rows;
            }
        }
        if (matching_rows != 1U) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::feature_not_ready,
                              std::string{runtime_2d_commercial_physics_feature_kind_name(kind)},
                              "2D commercial physics closeout requires exactly one ready row for each selected "
                              "physics feature kind");
        } else {
            ++required_feature_rows;
        }
    }

    result.feature_gate_ready = all_rows_ready && required_feature_rows == kRequiredFeatureKinds.size() &&
                                result.feature_rows == kRequiredFeatureKinds.size();
    result.package_counter_gate_ready =
        result.feature_gate_ready && result.package_visible_feature_rows == kRequiredFeatureKinds.size();
    result.deterministic_replay_ready = result.feature_gate_ready &&
                                        result.deterministic_feature_rows == kRequiredFeatureKinds.size() &&
                                        result.deterministic_replay_hash != 0U;
}

void evaluate_source_rows(const Runtime2DCommercialPhysicsCloseoutDesc& desc,
                          Runtime2DCommercialPhysicsCloseoutResult& result) {
    bool all_rows_ready = true;
    for (const auto& row : desc.source_rows) {
        if (source_row_ready(row)) {
            ++result.source_rows;
        } else {
            all_rows_ready = false;
            append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::source_not_ready, row.id,
                              "2D commercial physics closeout source rows must be reviewed first-party or official "
                              "tracked public source references");
        }
    }

    std::size_t required_source_rows = 0U;
    for (const auto kind : kRequiredSourceKinds) {
        std::size_t matching_rows = 0U;
        for (const auto& row : desc.source_rows) {
            if (row.kind == kind && source_row_ready(row)) {
                ++matching_rows;
            }
        }
        if (matching_rows != 1U) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::source_not_ready,
                              std::string{runtime_2d_commercial_physics_source_kind_name(kind)},
                              "2D commercial physics closeout requires exactly one reviewed source row for each "
                              "required source kind");
        } else {
            ++required_source_rows;
        }
    }

    result.source_gate_ready = all_rows_ready && required_source_rows == kRequiredSourceKinds.size() &&
                               result.source_rows == kRequiredSourceKinds.size();
}

void evaluate_claims(const Runtime2DCommercialPhysicsCloseoutDesc& desc,
                     Runtime2DCommercialPhysicsCloseoutResult& result) {
    if (!desc.selected_package_physics_closeout_claim) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::selected_package_claim_missing,
                          {}, "2D commercial physics closeout must be scoped to selected package evidence");
    }
    if (desc.dynamic_vs_dynamic_ccd_broad_claim) {
        ++result.dynamic_vs_dynamic_ccd_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPhysicsDiagnosticCode::dynamic_vs_dynamic_ccd_broad_claim, {},
                          "2D commercial physics closeout must not claim broad dynamic-vs-dynamic CCD readiness");
    }
    if (desc.physics_middleware_claim) {
        ++result.physics_middleware_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::physics_middleware_claim, {},
                          "2D commercial physics closeout must not claim physics middleware integration");
    }
    if (desc.broad_physics_parity_claim) {
        ++result.broad_physics_parity_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::broad_physics_parity_claim, {},
                          "2D commercial physics closeout must not claim broad physics-engine parity");
    }
    if (desc.public_native_handles) {
        ++result.native_handle_access_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::public_native_handles, {},
                          "2D commercial physics closeout must not expose native physics handles");
    }
    if (desc.external_engine_compatibility_claim) {
        ++result.external_engine_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPhysicsDiagnosticCode::external_engine_compatibility_claim, {},
                          "2D commercial physics closeout must not claim external commercial engine compatibility");
    }
    if (desc.cross_platform_parity_claim) {
        ++result.cross_platform_parity_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::cross_platform_parity_claim, {},
                          "2D commercial physics closeout must not infer cross-platform physics parity");
    }
    if (desc.legal_approval_claim) {
        ++result.legal_approval_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPhysicsDiagnosticCode::legal_approval_claim, {},
                          "2D commercial physics closeout can provide engineering evidence but not legal approval");
    }
}

} // namespace

bool Runtime2DCommercialPhysicsCloseoutResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view runtime_2d_commercial_physics_feature_kind_name(Runtime2DCommercialPhysicsFeatureKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd:
        return "time_of_impact_ccd";
    case Runtime2DCommercialPhysicsFeatureKind::joint_constraints:
        return "joint_constraints";
    case Runtime2DCommercialPhysicsFeatureKind::trigger_area_events:
        return "trigger_area_events";
    case Runtime2DCommercialPhysicsFeatureKind::kinematic_contact_resolution:
        return "kinematic_contact_resolution";
    case Runtime2DCommercialPhysicsFeatureKind::deterministic_replay:
        return "deterministic_replay";
    case Runtime2DCommercialPhysicsFeatureKind::package_counters:
        return "package_counters";
    }
    return "unknown";
}

std::string_view runtime_2d_commercial_physics_source_kind_name(Runtime2DCommercialPhysicsSourceKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_api:
        return "physics2d_runtime_extension_api";
    case Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_tests:
        return "physics2d_runtime_extension_tests";
    case Runtime2DCommercialPhysicsSourceKind::sample_package_counter_contract:
        return "sample_package_counter_contract";
    case Runtime2DCommercialPhysicsSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

Runtime2DCommercialPhysicsCloseoutResult
evaluate_runtime_2d_commercial_physics_closeout(const Runtime2DCommercialPhysicsCloseoutDesc& desc) {
    Runtime2DCommercialPhysicsCloseoutResult result;

    evaluate_feature_rows(desc, result);
    evaluate_source_rows(desc, result);
    evaluate_claims(desc, result);

    result.ready = desc.selected_package_physics_closeout_claim && result.feature_gate_ready &&
                   result.source_gate_ready && result.package_counter_gate_ready && result.deterministic_replay_ready &&
                   result.diagnostic_rows == 0U && result.native_handle_access_rows == 0U &&
                   result.middleware_dispatch_rows == 0U && result.dynamic_vs_dynamic_ccd_claim_rows == 0U &&
                   result.physics_middleware_claim_rows == 0U && result.broad_physics_parity_claim_rows == 0U &&
                   result.external_engine_claim_rows == 0U && result.cross_platform_parity_claim_rows == 0U &&
                   result.legal_approval_claim_rows == 0U && result.diagnostics.empty();
    return result;
}

} // namespace mirakana::runtime
