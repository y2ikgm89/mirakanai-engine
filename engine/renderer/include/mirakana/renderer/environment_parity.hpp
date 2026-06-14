// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentBackendParityStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class EnvironmentBackendParityFeature : std::uint8_t {
    profile_v2 = 0,
    physical_sky,
    height_fog,
    volumetric_fog,
    volumetric_cloud,
    rain_precipitation,
    ibl,
};

enum class EnvironmentBackendParityRowStatus : std::uint8_t {
    ready = 0,
    host_gated,
    unsupported,
};

enum class EnvironmentBackendParityCounterSemantics : std::uint8_t {
    exact_zero = 0,
    exact_one,
    min_positive,
};

enum class EnvironmentBackendParityDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_backend,
    duplicate_required_backend,
    missing_required_backend,
    unsupported_backend,
    missing_required_feature_row,
    duplicate_feature_row,
    invalid_row_taxonomy,
    invalid_row_id,
    invalid_validation_recipe,
    feature_id_mismatch,
    stale_profile_revision,
    stale_preset_pack_revision,
    stale_package_revision,
    quality_tier_mismatch,
    quality_budget_class_mismatch,
    resource_class_mismatch,
    output_tolerance_mismatch,
    missing_feature_presence,
    missing_backend_aggregate_evidence,
    missing_quality_budget_evidence,
    missing_resource_class_evidence,
    missing_output_tolerance_evidence,
    missing_package_counter_evidence,
    package_counter_semantics_mismatch,
    unsupported_row_mismatch,
    diagnostics_nonzero,
    unsupported_fallback,
    unsupported_native_handle_access,
    unsupported_inferred_backend,
    row_budget_exceeded,
};

struct EnvironmentBackendParityCounterExpectation {
    std::string counter_id;
    EnvironmentBackendParityCounterSemantics semantics{EnvironmentBackendParityCounterSemantics::exact_one};
};

struct EnvironmentBackendParityRow {
    std::string feature_id;
    EnvironmentBackendParityFeature feature{EnvironmentBackendParityFeature::profile_v2};
    rhi::BackendKind backend{rhi::BackendKind::null};
    EnvironmentBackendParityRowStatus status{EnvironmentBackendParityRowStatus::ready};
    std::string aggregate_recipe_id;
    std::string host_validation_recipe_id;
    std::string profile_revision;
    std::string preset_pack_revision;
    std::string package_revision;
    std::string quality_tier;
    std::string quality_budget_class;
    std::string resource_class;
    std::string output_tolerance_class;
    std::vector<EnvironmentBackendParityCounterExpectation> counter_expectations;
    std::vector<std::string> unsupported_row_ids;
    bool feature_present{false};
    bool backend_aggregate_ready{false};
    bool quality_budget_ready{false};
    bool resource_class_ready{false};
    bool output_tolerance_ready{false};
    bool package_counters_ready{false};
    bool unsupported_rows_declared{false};
    bool host_validated{false};
    bool host_gate_required{false};
    std::uint32_t diagnostic_count{0U};
    bool fallback_used{false};
    bool native_handle_access{false};
    bool inferred_from_other_backend{false};
    std::uint32_t source_index{0U};
};

struct EnvironmentBackendParityRequest {
    std::vector<rhi::BackendKind> required_backends;
    std::vector<EnvironmentBackendParityRow> rows;
    std::string expected_profile_revision;
    std::string expected_preset_pack_revision;
    std::string expected_package_revision;
    std::string expected_quality_tier;
    std::string expected_quality_budget_class;
    std::string expected_resource_class;
    std::string expected_output_tolerance_class;
    std::vector<std::string> expected_unsupported_row_ids;
    std::size_t row_budget{256U};
    std::uint64_t seed{0U};
};

struct EnvironmentBackendParityDiagnostic {
    EnvironmentBackendParityDiagnosticCode code{EnvironmentBackendParityDiagnosticCode::none};
    rhi::BackendKind backend{rhi::BackendKind::null};
    EnvironmentBackendParityFeature feature{EnvironmentBackendParityFeature::profile_v2};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct EnvironmentBackendParityPlan {
    EnvironmentBackendParityStatus status{EnvironmentBackendParityStatus::invalid_request};
    std::vector<EnvironmentBackendParityDiagnostic> diagnostics;
    std::vector<rhi::BackendKind> required_backends;
    std::vector<EnvironmentBackendParityRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t required_feature_count{0U};
    std::size_t host_validated_backend_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_primary_ready{false};
    bool vulkan_strict_ready{false};
    bool metal_host_ready{false};
    bool requires_metal_host_evidence{false};
    bool environment_backend_parity_ready{false};
    bool invoked_gpu_commands{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews selected environment backend parity evidence rows without running GPU work or transferring proof between
/// backends. A ready plan requires D3D12, strict Vulkan, and Apple-host Metal rows for the same profile and preset
/// pack.
[[nodiscard]] EnvironmentBackendParityPlan
plan_environment_backend_parity(const EnvironmentBackendParityRequest& request);

} // namespace mirakana
