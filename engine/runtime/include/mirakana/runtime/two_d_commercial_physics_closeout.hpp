// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class Runtime2DCommercialPhysicsFeatureKind : std::uint8_t {
    time_of_impact_ccd,
    joint_constraints,
    trigger_area_events,
    kinematic_contact_resolution,
    deterministic_replay,
    package_counters,
};

enum class Runtime2DCommercialPhysicsSourceKind : std::uint8_t {
    physics2d_runtime_extension_api,
    physics2d_runtime_extension_tests,
    sample_package_counter_contract,
    repository_legal_policy,
};

enum class Runtime2DCommercialPhysicsDiagnosticCode : std::uint8_t {
    feature_not_ready,
    source_not_ready,
    selected_package_claim_missing,
    dynamic_vs_dynamic_ccd_broad_claim,
    physics_middleware_claim,
    broad_physics_parity_claim,
    public_native_handles,
    external_engine_compatibility_claim,
    cross_platform_parity_claim,
    legal_approval_claim,
};

struct Runtime2DCommercialPhysicsFeatureRow {
    std::string id;
    Runtime2DCommercialPhysicsFeatureKind kind{Runtime2DCommercialPhysicsFeatureKind::time_of_impact_ccd};
    std::string validation_recipe_id;
    std::string package_counter_prefix;
    bool ready{false};
    bool package_visible{false};
    bool deterministic{false};
    bool selected_2d_scope{false};
    std::uint64_t simulation_runs{0U};
    std::uint64_t time_of_impact_rows{0U};
    std::uint64_t exact_sweep_shape_pair_rows{0U};
    std::uint64_t kinematic_contact_rows{0U};
    std::uint64_t joint_rows{0U};
    std::uint64_t trigger_event_rows{0U};
    std::uint64_t deterministic_replay_hash{0U};
    std::uint64_t diagnostics{0U};
    std::uint64_t native_handle_access_rows{0U};
    std::uint64_t middleware_dispatch_rows{0U};
    std::uint64_t dynamic_vs_dynamic_ccd_claim_rows{0U};
};

struct Runtime2DCommercialPhysicsSourceRow {
    std::string id;
    Runtime2DCommercialPhysicsSourceKind kind{Runtime2DCommercialPhysicsSourceKind::physics2d_runtime_extension_api};
    std::string source_path_or_url;
    bool ready{false};
    bool reviewed{false};
    bool first_party_or_official{false};
    bool public_docs_or_tracked_repo{false};
};

struct Runtime2DCommercialPhysicsDiagnostic {
    Runtime2DCommercialPhysicsDiagnosticCode code{Runtime2DCommercialPhysicsDiagnosticCode::feature_not_ready};
    std::string row_id;
    std::string message;
};

struct Runtime2DCommercialPhysicsCloseoutDesc {
    std::vector<Runtime2DCommercialPhysicsFeatureRow> feature_rows;
    std::array<Runtime2DCommercialPhysicsSourceRow, 4U> source_rows{};
    bool selected_package_physics_closeout_claim{false};
    bool dynamic_vs_dynamic_ccd_broad_claim{false};
    bool physics_middleware_claim{false};
    bool broad_physics_parity_claim{false};
    bool public_native_handles{false};
    bool external_engine_compatibility_claim{false};
    bool cross_platform_parity_claim{false};
    bool legal_approval_claim{false};
};

struct Runtime2DCommercialPhysicsCloseoutResult {
    bool ready{false};
    bool feature_gate_ready{false};
    bool source_gate_ready{false};
    bool package_counter_gate_ready{false};
    bool deterministic_replay_ready{false};
    std::vector<Runtime2DCommercialPhysicsDiagnostic> diagnostics;
    std::size_t feature_rows{0U};
    std::size_t source_rows{0U};
    std::size_t package_visible_feature_rows{0U};
    std::size_t deterministic_feature_rows{0U};
    std::uint64_t simulation_runs{0U};
    std::uint64_t time_of_impact_rows{0U};
    std::uint64_t exact_sweep_shape_pair_rows{0U};
    std::uint64_t kinematic_contact_rows{0U};
    std::uint64_t joint_rows{0U};
    std::uint64_t trigger_event_rows{0U};
    std::uint64_t deterministic_replay_hash{0U};
    std::size_t diagnostic_rows{0U};
    std::size_t native_handle_access_rows{0U};
    std::size_t middleware_dispatch_rows{0U};
    std::size_t dynamic_vs_dynamic_ccd_claim_rows{0U};
    std::size_t physics_middleware_claim_rows{0U};
    std::size_t broad_physics_parity_claim_rows{0U};
    std::size_t external_engine_claim_rows{0U};
    std::size_t cross_platform_parity_claim_rows{0U};
    std::size_t legal_approval_claim_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view
runtime_2d_commercial_physics_feature_kind_name(Runtime2DCommercialPhysicsFeatureKind kind) noexcept;
[[nodiscard]] std::string_view
runtime_2d_commercial_physics_source_kind_name(Runtime2DCommercialPhysicsSourceKind kind) noexcept;
[[nodiscard]] Runtime2DCommercialPhysicsCloseoutResult
evaluate_runtime_2d_commercial_physics_closeout(const Runtime2DCommercialPhysicsCloseoutDesc& desc);

} // namespace mirakana::runtime
