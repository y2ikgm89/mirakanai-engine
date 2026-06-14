// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_parity.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::array kRequiredFeatures{
    EnvironmentBackendParityFeature::profile_v2,
    EnvironmentBackendParityFeature::physical_sky,
    EnvironmentBackendParityFeature::height_fog,
    EnvironmentBackendParityFeature::volumetric_fog,
    EnvironmentBackendParityFeature::volumetric_cloud,
    EnvironmentBackendParityFeature::rain_precipitation,
    EnvironmentBackendParityFeature::ibl,
};

[[nodiscard]] bool is_supported_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal;
}

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] std::uint8_t feature_sort_key(EnvironmentBackendParityFeature feature) noexcept {
    return static_cast<std::uint8_t>(feature);
}

[[nodiscard]] bool is_id_char(char ch) noexcept {
    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '-';
}

[[nodiscard]] bool is_valid_id(std::string_view value) {
    return !value.empty() && std::ranges::all_of(value, is_id_char);
}

[[nodiscard]] bool contains_backend(const std::vector<rhi::BackendKind>& backends, rhi::BackendKind backend) {
    return std::ranges::find(backends, backend) != backends.end();
}

[[nodiscard]] std::string_view canonical_feature_id(EnvironmentBackendParityFeature feature) noexcept {
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
    return "";
}

[[nodiscard]] bool is_supported_feature(EnvironmentBackendParityFeature feature) noexcept {
    return !canonical_feature_id(feature).empty();
}

[[nodiscard]] bool row_backend_allowed(const EnvironmentBackendParityRequest& request, rhi::BackendKind backend) {
    return is_supported_backend(backend) && contains_backend(request.required_backends, backend);
}

[[nodiscard]] std::vector<std::string> sorted_strings(std::vector<std::string> values) {
    std::ranges::sort(values);
    return values;
}

[[nodiscard]] bool same_string_set(const std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
    return sorted_strings(lhs) == sorted_strings(rhs);
}

[[nodiscard]] std::vector<EnvironmentBackendParityCounterExpectation>
expected_counter_expectations(const EnvironmentBackendParityRow& row) {
    const auto id = std::string{canonical_feature_id(row.feature)};
    return {
        EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".ready",
            .semantics = EnvironmentBackendParityCounterSemantics::exact_one,
        },
        EnvironmentBackendParityCounterExpectation{
            .counter_id = "environment_backend_parity." + id + ".diagnostics",
            .semantics = EnvironmentBackendParityCounterSemantics::exact_zero,
        },
    };
}

[[nodiscard]] bool counter_expectations_valid(const EnvironmentBackendParityRow& row) {
    if (row.counter_expectations.empty()) {
        return false;
    }
    return std::ranges::all_of(row.counter_expectations,
                               [](const auto& counter) { return is_valid_id(counter.counter_id); });
}

[[nodiscard]] bool counter_expectations_match(const EnvironmentBackendParityRow& row) {
    const auto expected = expected_counter_expectations(row);
    if (row.counter_expectations.size() != expected.size()) {
        return false;
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (row.counter_expectations[index].counter_id != expected[index].counter_id ||
            row.counter_expectations[index].semantics != expected[index].semantics) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool ready_row(const EnvironmentBackendParityRow& row) noexcept {
    return row.status == EnvironmentBackendParityRowStatus::ready && row.feature_present &&
           row.backend_aggregate_ready && row.quality_budget_ready && row.resource_class_ready &&
           row.output_tolerance_ready && row.package_counters_ready && row.unsupported_rows_declared &&
           row.host_validated && !row.host_gate_required && row.diagnostic_count == 0U && !row.fallback_used &&
           !row.native_handle_access && !row.inferred_from_other_backend;
}

[[nodiscard]] bool host_gated_row(const EnvironmentBackendParityRow& row) noexcept {
    return row.status == EnvironmentBackendParityRowStatus::host_gated && row.host_gate_required && !row.host_validated;
}

void add_diagnostic(EnvironmentBackendParityPlan& plan, EnvironmentBackendParityDiagnosticCode code,
                    rhi::BackendKind backend, EnvironmentBackendParityFeature feature, std::string row_id,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(EnvironmentBackendParityDiagnostic{
        .code = code,
        .backend = backend,
        .feature = feature,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_backends(std::vector<rhi::BackendKind>& backends) {
    std::ranges::sort(backends,
                      [](const auto lhs, const auto rhs) { return backend_sort_key(lhs) < backend_sort_key(rhs); });
}

void sort_rows(std::vector<EnvironmentBackendParityRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        if (lhs.feature_id != rhs.feature_id) {
            return lhs.feature_id < rhs.feature_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(EnvironmentBackendParityPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void validate_budget(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request) {
    if (request.required_backends.size() + request.rows.size() > request.row_budget) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::row_budget_exceeded, rhi::BackendKind::null,
                       EnvironmentBackendParityFeature::profile_v2, {}, "environment parity row budget exceeded", 0U);
    }
}

void validate_required_backends(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request) {
    std::vector<rhi::BackendKind> seen;
    seen.reserve(request.required_backends.size());
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::invalid_required_backend, backend,
                           EnvironmentBackendParityFeature::profile_v2, {},
                           "environment parity supports d3d12, vulkan, and metal only", 0U);
            continue;
        }
        if (contains_backend(seen, backend)) {
            add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::duplicate_required_backend, backend,
                           EnvironmentBackendParityFeature::profile_v2, {},
                           "environment parity required backends must be unique", 0U);
            continue;
        }
        seen.push_back(backend);
    }

    for (const auto backend : {rhi::BackendKind::d3d12, rhi::BackendKind::vulkan, rhi::BackendKind::metal}) {
        if (!contains_backend(seen, backend)) {
            add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_required_backend, backend,
                           EnvironmentBackendParityFeature::profile_v2, {},
                           "environment backend parity requires d3d12, strict vulkan, and apple-host metal", 0U);
        }
    }
}

void validate_duplicate_rows(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request) {
    std::vector<std::string> seen;
    seen.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        std::string key;
        key.append(std::to_string(static_cast<std::uint8_t>(row.backend)));
        key.push_back('\n');
        key.append(std::to_string(static_cast<std::uint8_t>(row.feature)));
        if (std::ranges::find(seen, key) != seen.end()) {
            add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::duplicate_feature_row, row.backend,
                           row.feature, row.feature_id,
                           "environment parity allows one row per backend and normalized feature", row.source_index);
            continue;
        }
        seen.push_back(std::move(key));
    }
}

void validate_required_rows(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request) {
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            continue;
        }
        for (const auto feature : kRequiredFeatures) {
            const auto has_row = std::ranges::any_of(request.rows, [backend, feature](const auto& row) {
                return row.backend == backend && row.feature == feature;
            });
            if (!has_row) {
                add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_required_feature_row, backend,
                               feature, {}, "every required backend needs every normalized environment feature row",
                               0U);
            }
        }
    }
}

void validate_row_taxonomy(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRow& row) {
    bool valid{false};
    switch (row.status) {
    case EnvironmentBackendParityRowStatus::ready:
        valid = row.host_validated && !row.host_gate_required;
        break;
    case EnvironmentBackendParityRowStatus::host_gated:
        valid = row.host_gate_required && !row.host_validated;
        break;
    case EnvironmentBackendParityRowStatus::unsupported:
        valid = !row.host_validated && !row.host_gate_required;
        break;
    }
    if (!valid) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::invalid_row_taxonomy, row.backend, row.feature,
                       row.feature_id, "environment parity rows need explicit ready, host-gated, or unsupported state",
                       row.source_index);
    }
}

void validate_row_identity(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request,
                           const EnvironmentBackendParityRow& row) {
    if (!row_backend_allowed(request, row.backend)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::unsupported_backend, row.backend, row.feature,
                       row.feature_id, "environment parity row backend must be required and supported",
                       row.source_index);
    }
    if (!is_supported_feature(row.feature) || !is_valid_id(row.feature_id)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::invalid_row_id, row.backend, row.feature,
                       row.feature_id, "environment parity rows require a backend-neutral normalized feature id",
                       row.source_index);
    } else if (row.feature_id != canonical_feature_id(row.feature)) {
        add_diagnostic(
            plan, EnvironmentBackendParityDiagnosticCode::feature_id_mismatch, row.backend, row.feature, row.feature_id,
            "environment parity cannot compare feature counts without normalized feature ids", row.source_index);
    }
    if (!is_valid_id(row.aggregate_recipe_id) || !is_valid_id(row.host_validation_recipe_id)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::invalid_validation_recipe, row.backend,
                       row.feature, row.feature_id,
                       "environment parity rows require reviewed aggregate and host validation recipe ids",
                       row.source_index);
    }
}

void validate_freshness(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request,
                        const EnvironmentBackendParityRow& row) {
    if (row.profile_revision != request.expected_profile_revision) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::stale_profile_revision, row.backend, row.feature,
                       row.feature_id, "environment parity requires the selected EnvironmentProfile revision",
                       row.source_index);
    }
    if (row.preset_pack_revision != request.expected_preset_pack_revision) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::stale_preset_pack_revision, row.backend,
                       row.feature, row.feature_id,
                       "environment parity requires the selected EnvironmentPresetPack revision", row.source_index);
    }
    if (row.package_revision != request.expected_package_revision) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::stale_package_revision, row.backend, row.feature,
                       row.feature_id, "environment parity requires the selected package revision", row.source_index);
    }
}

void validate_expected_classes(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request,
                               const EnvironmentBackendParityRow& row) {
    if (row.quality_tier != request.expected_quality_tier) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::quality_tier_mismatch, row.backend, row.feature,
                       row.feature_id, "environment parity requires the same quality tier on every backend",
                       row.source_index);
    }
    if (row.quality_budget_class != request.expected_quality_budget_class) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::quality_budget_class_mismatch, row.backend,
                       row.feature, row.feature_id,
                       "environment parity requires the same quality-budget class on every backend", row.source_index);
    }
    if (row.resource_class != request.expected_resource_class) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::resource_class_mismatch, row.backend, row.feature,
                       row.feature_id, "environment parity requires the same resource class on every backend",
                       row.source_index);
    }
    if (row.output_tolerance_class != request.expected_output_tolerance_class) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::output_tolerance_mismatch, row.backend,
                       row.feature, row.feature_id,
                       "environment parity requires the same output tolerance class on every backend",
                       row.source_index);
    }
}

void validate_counter_rows(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRow& row) {
    if (host_gated_row(row)) {
        return;
    }
    if (!row.package_counters_ready || !counter_expectations_valid(row)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_package_counter_evidence, row.backend,
                       row.feature, row.feature_id,
                       "environment parity rows require package-visible counter expectations", row.source_index);
        return;
    }
    if (!counter_expectations_match(row)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::package_counter_semantics_mismatch, row.backend,
                       row.feature, row.feature_id,
                       "environment parity counter ids and zero/one/positive semantics must match", row.source_index);
    }
}

void validate_unsupported_rows(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request,
                               const EnvironmentBackendParityRow& row) {
    if (!row.unsupported_rows_declared ||
        !same_string_set(row.unsupported_row_ids, request.expected_unsupported_row_ids)) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::unsupported_row_mismatch, row.backend, row.feature,
                       row.feature_id, "environment parity unsupported and non-claim rows must match",
                       row.source_index);
    }
}

void validate_ready_evidence(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRow& row) {
    if (host_gated_row(row)) {
        return;
    }
    if (!row.feature_present) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_feature_presence, row.backend, row.feature,
                       row.feature_id, "environment parity rows must prove the selected feature is present",
                       row.source_index);
    }
    if (!row.backend_aggregate_ready) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_backend_aggregate_evidence, row.backend,
                       row.feature, row.feature_id, "environment parity rows require backend-local aggregate readiness",
                       row.source_index);
    }
    if (!row.quality_budget_ready) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_quality_budget_evidence, row.backend,
                       row.feature, row.feature_id, "environment parity rows require quality-budget evidence",
                       row.source_index);
    }
    if (!row.resource_class_ready) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_resource_class_evidence, row.backend,
                       row.feature, row.feature_id, "environment parity rows require resource-class evidence",
                       row.source_index);
    }
    if (!row.output_tolerance_ready) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::missing_output_tolerance_evidence, row.backend,
                       row.feature, row.feature_id, "environment parity rows require output-tolerance evidence",
                       row.source_index);
    }
    if (row.diagnostic_count != 0U) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::diagnostics_nonzero, row.backend, row.feature,
                       row.feature_id, "environment parity ready rows require zero diagnostics", row.source_index);
    }
    if (row.fallback_used) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::unsupported_fallback, row.backend, row.feature,
                       row.feature_id, "environment parity rows must not use fallback backends or disabled features",
                       row.source_index);
    }
    if (row.native_handle_access) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::unsupported_native_handle_access, row.backend,
                       row.feature, row.feature_id,
                       "environment parity evidence must not expose native renderer or RHI handles", row.source_index);
    }
    if (row.inferred_from_other_backend) {
        add_diagnostic(plan, EnvironmentBackendParityDiagnosticCode::unsupported_inferred_backend, row.backend,
                       row.feature, row.feature_id, "environment parity cannot infer evidence from another backend",
                       row.source_index);
    }
}

void validate_rows(EnvironmentBackendParityPlan& plan, const EnvironmentBackendParityRequest& request) {
    for (const auto& row : request.rows) {
        validate_row_taxonomy(plan, row);
        validate_row_identity(plan, request, row);
        validate_freshness(plan, request, row);
        validate_expected_classes(plan, request, row);
        validate_counter_rows(plan, row);
        validate_unsupported_rows(plan, request, row);
        validate_ready_evidence(plan, row);
    }
}

[[nodiscard]] bool backend_all_features_ready(const EnvironmentBackendParityPlan& plan, rhi::BackendKind backend) {
    for (const auto feature : kRequiredFeatures) {
        const auto row_it = std::ranges::find_if(plan.rows, [backend, feature](const auto& row) {
            return row.backend == backend && row.feature == feature;
        });
        if (row_it == plan.rows.end() || !ready_row(*row_it)) {
            return false;
        }
    }
    return true;
}

void summarize(EnvironmentBackendParityPlan& plan) {
    std::vector<rhi::BackendKind> ready_backends;
    for (const auto& row : plan.rows) {
        if (ready_row(row)) {
            ++plan.ready_row_count;
        } else if (host_gated_row(row)) {
            ++plan.host_gated_row_count;
        }
    }

    for (const auto backend : plan.required_backends) {
        const auto backend_ready = backend_all_features_ready(plan, backend);
        if (backend_ready && !contains_backend(ready_backends, backend)) {
            ready_backends.push_back(backend);
        }
        if (backend == rhi::BackendKind::d3d12) {
            plan.d3d12_primary_ready = backend_ready;
        } else if (backend == rhi::BackendKind::vulkan) {
            plan.vulkan_strict_ready = backend_ready;
        } else if (backend == rhi::BackendKind::metal) {
            plan.requires_metal_host_evidence = true;
            plan.metal_host_ready = backend_ready;
        }
    }
    plan.host_validated_backend_count = ready_backends.size();
    plan.environment_backend_parity_ready = std::ranges::all_of(
        plan.required_backends, [&plan](const auto backend) { return backend_all_features_ready(plan, backend); });
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        hash_mix(hash, static_cast<unsigned char>(ch));
    }
    hash_mix(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const EnvironmentBackendParityPlan& plan,
                                                const EnvironmentBackendParityRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    for (const auto backend : plan.required_backends) {
        hash_mix(hash, static_cast<std::uint8_t>(backend));
    }
    for (const auto& row : plan.rows) {
        hash_string(hash, row.feature_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.feature));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_string(hash, row.aggregate_recipe_id);
        hash_string(hash, row.host_validation_recipe_id);
        hash_string(hash, row.profile_revision);
        hash_string(hash, row.preset_pack_revision);
        hash_string(hash, row.package_revision);
        hash_string(hash, row.quality_tier);
        hash_string(hash, row.quality_budget_class);
        hash_string(hash, row.resource_class);
        hash_string(hash, row.output_tolerance_class);
        for (const auto& counter : row.counter_expectations) {
            hash_string(hash, counter.counter_id);
            hash_mix(hash, static_cast<std::uint8_t>(counter.semantics));
        }
        for (const auto& unsupported : row.unsupported_row_ids) {
            hash_string(hash, unsupported);
        }
        hash_mix(hash, row.feature_present ? 1U : 0U);
        hash_mix(hash, row.backend_aggregate_ready ? 1U : 0U);
        hash_mix(hash, row.quality_budget_ready ? 1U : 0U);
        hash_mix(hash, row.resource_class_ready ? 1U : 0U);
        hash_mix(hash, row.output_tolerance_ready ? 1U : 0U);
        hash_mix(hash, row.package_counters_ready ? 1U : 0U);
        hash_mix(hash, row.unsupported_rows_declared ? 1U : 0U);
        hash_mix(hash, row.host_validated ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix(hash, row.diagnostic_count);
        hash_mix(hash, row.fallback_used ? 1U : 0U);
        hash_mix(hash, row.native_handle_access ? 1U : 0U);
        hash_mix(hash, row.inferred_from_other_backend ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

} // namespace

bool EnvironmentBackendParityPlan::succeeded() const noexcept {
    return status == EnvironmentBackendParityStatus::ready && diagnostics.empty() && environment_backend_parity_ready;
}

EnvironmentBackendParityPlan plan_environment_backend_parity(const EnvironmentBackendParityRequest& request) {
    EnvironmentBackendParityPlan plan;
    plan.required_backends = request.required_backends;
    plan.rows = request.rows;
    plan.row_count = request.rows.size();
    plan.required_feature_count = kRequiredFeatures.size();
    sort_backends(plan.required_backends);
    sort_rows(plan.rows);

    if (request.required_backends.empty() || request.rows.empty()) {
        plan.status = EnvironmentBackendParityStatus::no_rows;
        return plan;
    }

    validate_budget(plan, request);
    validate_required_backends(plan, request);
    validate_duplicate_rows(plan, request);
    validate_required_rows(plan, request);
    validate_rows(plan, request);
    sort_diagnostics(plan);

    if (!plan.diagnostics.empty()) {
        plan.status = EnvironmentBackendParityStatus::invalid_request;
        return plan;
    }

    summarize(plan);
    plan.replay_hash = compute_replay_hash(plan, request);
    if (plan.environment_backend_parity_ready) {
        plan.status = EnvironmentBackendParityStatus::ready;
    } else {
        plan.status = EnvironmentBackendParityStatus::host_evidence_required;
    }
    return plan;
}

} // namespace mirakana
