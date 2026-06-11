// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_quality_governor.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] std::uint8_t feature_sort_key(MavgQualityGovernorFeatureKind feature) noexcept {
    return static_cast<std::uint8_t>(feature);
}

[[nodiscard]] bool is_supported_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal || backend == rhi::BackendKind::null;
}

[[nodiscard]] bool is_host_gated_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::metal;
}

[[nodiscard]] bool is_host_gated_row(const MavgQualityGovernorCounterRow& row) noexcept {
    return row.host_gate_required || is_host_gated_backend(row.backend);
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9'));
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool forbidden_native_token(std::string_view token) noexcept {
    return token == "native" || token == "handle" || token == "hwnd" || token == "hinstance" ||
           token.starts_with("id3d12") || token.starts_with("vk") || token.starts_with("mtl") ||
           token.starts_with("sdl") || token == "imgui";
}

[[nodiscard]] bool has_native_token(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (forbidden_native_token(token)) {
            return true;
        }
        token.clear();
    }
    return forbidden_native_token(token);
}

[[nodiscard]] bool valid_id(std::string_view id) {
    return !id.empty() && !has_native_token(id) && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool valid_limits(const MavgQualityGovernorLimits& limits) noexcept {
    return limits.max_cpu_frame_time_p95_us > 0U && limits.max_gpu_frame_time_p95_us > 0U &&
           limits.max_mavg_cpu_selection_time_p95_us > 0U && limits.max_mavg_gpu_culling_time_p95_us > 0U &&
           limits.max_screen_error_p99_micropixels > 0U && limits.max_resident_gpu_bytes > 0U &&
           limits.max_upload_bytes > 0U && limits.max_draw_count > 0U && limits.max_dispatch_count > 0U;
}

void add_diagnostic(MavgQualityGovernorResult& result, MavgQualityGovernorDiagnosticCode code,
                    const MavgQualityGovernorCounterRow& row, std::string field, std::uint64_t value,
                    std::uint64_t budget, std::string message) {
    result.diagnostics.push_back(MavgQualityGovernorDiagnostic{
        .code = code,
        .scene_id = row.scene_id,
        .backend = row.backend,
        .feature = row.feature,
        .field = std::move(field),
        .value = value,
        .budget = budget,
        .message = std::move(message),
        .source_index = row.source_index,
    });
}

void add_request_diagnostic(MavgQualityGovernorResult& result, MavgQualityGovernorDiagnosticCode code,
                            std::string field, std::uint64_t value, std::uint64_t budget, std::string message) {
    result.diagnostics.push_back(MavgQualityGovernorDiagnostic{
        .code = code,
        .field = std::move(field),
        .value = value,
        .budget = budget,
        .message = std::move(message),
    });
}

[[nodiscard]] bool requires_rt_consistency(const MavgQualityGovernorRequest& request,
                                           const MavgQualityGovernorCounterRow& row) noexcept {
    return request.require_rt_consistency_evidence || row.feature == MavgQualityGovernorFeatureKind::ray_tracing ||
           row.rt_consistency_diagnostic_count > 0U;
}

[[nodiscard]] bool is_budget_diagnostic(MavgQualityGovernorDiagnosticCode code) noexcept {
    switch (code) {
    case MavgQualityGovernorDiagnosticCode::cpu_frame_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::gpu_frame_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::mavg_cpu_selection_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::mavg_gpu_culling_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::screen_error_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::fallback_rate_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::page_miss_rate_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::temporal_churn_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::missing_required_geometry:
    case MavgQualityGovernorDiagnosticCode::rt_consistency_diagnostic_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::resident_gpu_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::upload_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::draw_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::dispatch_budget_exceeded:
        return true;
    case MavgQualityGovernorDiagnosticCode::none:
    case MavgQualityGovernorDiagnosticCode::invalid_limits:
    case MavgQualityGovernorDiagnosticCode::invalid_row_id:
    case MavgQualityGovernorDiagnosticCode::duplicate_counter_row:
    case MavgQualityGovernorDiagnosticCode::row_budget_exceeded:
    case MavgQualityGovernorDiagnosticCode::missing_host_evidence:
    case MavgQualityGovernorDiagnosticCode::missing_backend_local_evidence:
    case MavgQualityGovernorDiagnosticCode::missing_no_hole_evidence:
    case MavgQualityGovernorDiagnosticCode::missing_fallback_evidence:
    case MavgQualityGovernorDiagnosticCode::missing_rt_consistency_evidence:
    case MavgQualityGovernorDiagnosticCode::unsupported_native_handle_access:
    case MavgQualityGovernorDiagnosticCode::unsupported_nanite_claim:
    case MavgQualityGovernorDiagnosticCode::unsupported_benchmark_superiority_claim:
    case MavgQualityGovernorDiagnosticCode::unsupported_broad_optimization_claim:
    case MavgQualityGovernorDiagnosticCode::unsupported_backend_parity_claim:
        return false;
    }
    return false;
}

void validate_counter_ids(MavgQualityGovernorResult& result, const MavgQualityGovernorCounterRow& row) {
    if (!is_supported_backend(row.backend) || !valid_id(row.scene_id) || !valid_id(row.package_target_id) ||
        !valid_id(row.validation_recipe_id) || !valid_id(row.benchmark_command_id)) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::invalid_row_id, row, "ids", 0U, 0U,
                       "MAVG quality governor rows require backend-neutral scene, package, recipe, and command ids");
    }
}

void validate_duplicate_rows(MavgQualityGovernorResult& result, const MavgQualityGovernorRequest& request) {
    std::vector<std::string> seen;
    seen.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        std::string key;
        key.append(row.scene_id);
        key.push_back('\n');
        key.append(row.package_target_id);
        key.push_back('\n');
        key.append(std::to_string(static_cast<std::uint8_t>(row.backend)));
        key.push_back('\n');
        key.append(std::to_string(static_cast<std::uint8_t>(row.feature)));
        if (std::ranges::find(seen, key) != seen.end()) {
            add_diagnostic(result, MavgQualityGovernorDiagnosticCode::duplicate_counter_row, row, "row", 0U, 0U,
                           "MAVG quality governor allows one row per scene, package, backend, and feature");
            continue;
        }
        seen.push_back(std::move(key));
    }
}

void validate_evidence(MavgQualityGovernorResult& result, const MavgQualityGovernorRequest& request,
                       const MavgQualityGovernorCounterRow& row) {
    if (is_host_gated_row(row) && !row.host_evidence) {
        return;
    }
    if (!row.host_evidence) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::missing_host_evidence, row, "host_evidence", 0U, 1U,
                       "MAVG benchmark readiness requires explicit host evidence or a host gate");
    }
    if (!row.backend_local_evidence) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::missing_backend_local_evidence, row,
                       "backend_local_evidence", 0U, 1U,
                       "MAVG benchmark evidence cannot be inferred from another backend");
    }
    if (!row.no_hole_evidence) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::missing_no_hole_evidence, row, "no_hole_evidence", 0U,
                       1U, "MAVG quality rows require no-hole selected/fallback geometry evidence");
    }
    if (!row.fallback_evidence) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::missing_fallback_evidence, row, "fallback_evidence",
                       0U, 1U, "MAVG quality rows require explicit fallback evidence");
    }
    if (requires_rt_consistency(request, row) && !row.rt_consistency_evidence) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::missing_rt_consistency_evidence, row,
                       "rt_consistency_evidence", 0U, 1U,
                       "MAVG RT benchmark rows require explicit raster/RT consistency evidence");
    }
}

void validate_budgets(MavgQualityGovernorResult& result, const MavgQualityGovernorLimits& limits,
                      const MavgQualityGovernorCounterRow& row) {
    const auto check = [&result, &row](bool exceeded, MavgQualityGovernorDiagnosticCode code, std::string field,
                                       std::uint64_t value, std::uint64_t budget, std::string message) {
        if (exceeded) {
            add_diagnostic(result, code, row, std::move(field), value, budget, std::move(message));
        }
    };
    check(row.cpu_frame_time_p95_us > limits.max_cpu_frame_time_p95_us,
          MavgQualityGovernorDiagnosticCode::cpu_frame_budget_exceeded, "cpu_frame_time_p95_us",
          row.cpu_frame_time_p95_us, limits.max_cpu_frame_time_p95_us, "CPU frame p95 exceeded the MAVG budget");
    check(row.gpu_frame_time_p95_us > limits.max_gpu_frame_time_p95_us,
          MavgQualityGovernorDiagnosticCode::gpu_frame_budget_exceeded, "gpu_frame_time_p95_us",
          row.gpu_frame_time_p95_us, limits.max_gpu_frame_time_p95_us, "GPU frame p95 exceeded the MAVG budget");
    check(row.mavg_cpu_selection_time_p95_us > limits.max_mavg_cpu_selection_time_p95_us,
          MavgQualityGovernorDiagnosticCode::mavg_cpu_selection_budget_exceeded, "mavg_cpu_selection_time_p95_us",
          row.mavg_cpu_selection_time_p95_us, limits.max_mavg_cpu_selection_time_p95_us,
          "MAVG CPU selection p95 exceeded the selected budget");
    check(row.mavg_gpu_culling_time_p95_us > limits.max_mavg_gpu_culling_time_p95_us,
          MavgQualityGovernorDiagnosticCode::mavg_gpu_culling_budget_exceeded, "mavg_gpu_culling_time_p95_us",
          row.mavg_gpu_culling_time_p95_us, limits.max_mavg_gpu_culling_time_p95_us,
          "MAVG GPU culling p95 exceeded the selected budget");
    check(row.screen_error_p99_micropixels > limits.max_screen_error_p99_micropixels,
          MavgQualityGovernorDiagnosticCode::screen_error_budget_exceeded, "screen_error_p99_micropixels",
          row.screen_error_p99_micropixels, limits.max_screen_error_p99_micropixels,
          "screen-space error p99 exceeded the selected budget");
    check(row.fallback_rate_per_million > limits.max_fallback_rate_per_million,
          MavgQualityGovernorDiagnosticCode::fallback_rate_budget_exceeded, "fallback_rate_per_million",
          row.fallback_rate_per_million, limits.max_fallback_rate_per_million,
          "fallback rate exceeded the selected budget");
    check(row.page_miss_rate_per_million > limits.max_page_miss_rate_per_million,
          MavgQualityGovernorDiagnosticCode::page_miss_rate_budget_exceeded, "page_miss_rate_per_million",
          row.page_miss_rate_per_million, limits.max_page_miss_rate_per_million,
          "page miss rate exceeded the selected budget");
    check(row.temporal_churn_per_million > limits.max_temporal_churn_per_million,
          MavgQualityGovernorDiagnosticCode::temporal_churn_budget_exceeded, "temporal_churn_per_million",
          row.temporal_churn_per_million, limits.max_temporal_churn_per_million,
          "temporal churn exceeded the selected budget");
    check(row.missing_required_geometry_count > limits.max_missing_required_geometry_count,
          MavgQualityGovernorDiagnosticCode::missing_required_geometry, "missing_required_geometry_count",
          row.missing_required_geometry_count, limits.max_missing_required_geometry_count,
          "MAVG benchmark rows must not hide missing required geometry");
    check(row.rt_consistency_diagnostic_count > limits.max_rt_consistency_diagnostic_count,
          MavgQualityGovernorDiagnosticCode::rt_consistency_diagnostic_budget_exceeded,
          "rt_consistency_diagnostic_count", row.rt_consistency_diagnostic_count,
          limits.max_rt_consistency_diagnostic_count, "RT consistency diagnostics exceeded the selected budget");
    check(row.resident_gpu_bytes > limits.max_resident_gpu_bytes,
          MavgQualityGovernorDiagnosticCode::resident_gpu_budget_exceeded, "resident_gpu_bytes", row.resident_gpu_bytes,
          limits.max_resident_gpu_bytes, "resident GPU bytes exceeded the selected budget");
    check(row.upload_bytes > limits.max_upload_bytes, MavgQualityGovernorDiagnosticCode::upload_budget_exceeded,
          "upload_bytes", row.upload_bytes, limits.max_upload_bytes, "upload bytes exceeded the selected budget");
    check(row.draw_count > limits.max_draw_count, MavgQualityGovernorDiagnosticCode::draw_budget_exceeded, "draw_count",
          row.draw_count, limits.max_draw_count, "draw count exceeded the selected budget");
    check(row.dispatch_count > limits.max_dispatch_count, MavgQualityGovernorDiagnosticCode::dispatch_budget_exceeded,
          "dispatch_count", row.dispatch_count, limits.max_dispatch_count,
          "dispatch count exceeded the selected budget");
}

void validate_unsupported_claims(MavgQualityGovernorResult& result, const MavgQualityGovernorCounterRow& row) {
    if (row.request_native_handle_access) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::unsupported_native_handle_access, row,
                       "request_native_handle_access", 1U, 0U,
                       "MAVG quality governor evidence must not expose native handles");
    }
    if (row.request_nanite_claim) {
        add_diagnostic(
            result, MavgQualityGovernorDiagnosticCode::unsupported_nanite_claim, row, "request_nanite_claim", 1U, 0U,
            "Nanite compatibility, equivalence, or superiority claims require later legal and benchmark evidence");
    }
    if (row.request_benchmark_superiority_claim) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::unsupported_benchmark_superiority_claim, row,
                       "request_benchmark_superiority_claim", 1U, 0U,
                       "benchmark superiority claims require measured host evidence outside this dry-run governor");
    }
    if (row.request_broad_optimization_claim) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::unsupported_broad_optimization_claim, row,
                       "request_broad_optimization_claim", 1U, 0U,
                       "broad optimization claims require focused evidence for the exact workload");
    }
    if (row.request_backend_parity_claim) {
        add_diagnostic(result, MavgQualityGovernorDiagnosticCode::unsupported_backend_parity_claim, row,
                       "request_backend_parity_claim", 1U, 0U,
                       "backend parity cannot be inferred from one backend row");
    }
}

void sort_rows(std::vector<MavgQualityGovernorCounterRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.scene_id != rhs.scene_id) {
            return lhs.scene_id < rhs.scene_id;
        }
        if (lhs.package_target_id != rhs.package_target_id) {
            return lhs.package_target_id < rhs.package_target_id;
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(MavgQualityGovernorResult& result) {
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.scene_id != rhs.scene_id) {
            return lhs.scene_id < rhs.scene_id;
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        if (lhs.field != rhs.field) {
            return lhs.field < rhs.field;
        }
        return lhs.source_index < rhs.source_index;
    });
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

[[nodiscard]] std::uint64_t compute_replay_hash(const MavgQualityGovernorRequest& request,
                                                const MavgQualityGovernorResult& result) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    hash_mix(hash, request.limits.max_cpu_frame_time_p95_us);
    hash_mix(hash, request.limits.max_gpu_frame_time_p95_us);
    hash_mix(hash, request.limits.max_mavg_cpu_selection_time_p95_us);
    hash_mix(hash, request.limits.max_mavg_gpu_culling_time_p95_us);
    hash_mix(hash, request.limits.max_screen_error_p99_micropixels);
    hash_mix(hash, request.limits.max_fallback_rate_per_million);
    hash_mix(hash, request.limits.max_page_miss_rate_per_million);
    hash_mix(hash, request.limits.max_temporal_churn_per_million);
    hash_mix(hash, request.limits.max_missing_required_geometry_count);
    hash_mix(hash, request.limits.max_rt_consistency_diagnostic_count);
    hash_mix(hash, request.limits.max_resident_gpu_bytes);
    hash_mix(hash, request.limits.max_upload_bytes);
    hash_mix(hash, request.limits.max_draw_count);
    hash_mix(hash, request.limits.max_dispatch_count);
    hash_mix(hash, static_cast<std::uint64_t>(request.row_budget));
    hash_mix(hash, request.require_rt_consistency_evidence ? 1U : 0U);
    for (const auto& row : result.rows) {
        hash_string(hash, row.scene_id);
        hash_string(hash, row.package_target_id);
        hash_string(hash, row.validation_recipe_id);
        hash_string(hash, row.benchmark_command_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.feature));
        hash_mix(hash, row.host_evidence ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix(hash, row.backend_local_evidence ? 1U : 0U);
        hash_mix(hash, row.no_hole_evidence ? 1U : 0U);
        hash_mix(hash, row.fallback_evidence ? 1U : 0U);
        hash_mix(hash, row.rt_consistency_evidence ? 1U : 0U);
        hash_mix(hash, row.cpu_frame_time_p95_us);
        hash_mix(hash, row.gpu_frame_time_p95_us);
        hash_mix(hash, row.mavg_cpu_selection_time_p95_us);
        hash_mix(hash, row.mavg_gpu_culling_time_p95_us);
        hash_mix(hash, row.screen_error_p99_micropixels);
        hash_mix(hash, row.fallback_rate_per_million);
        hash_mix(hash, row.page_miss_rate_per_million);
        hash_mix(hash, row.temporal_churn_per_million);
        hash_mix(hash, row.missing_required_geometry_count);
        hash_mix(hash, row.rt_consistency_diagnostic_count);
        hash_mix(hash, row.resident_gpu_bytes);
        hash_mix(hash, row.upload_bytes);
        hash_mix(hash, row.draw_count);
        hash_mix(hash, row.dispatch_count);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool row_ready(const MavgQualityGovernorCounterRow& row, const MavgQualityGovernorRequest& request) {
    return row.host_evidence && !is_host_gated_row(row) && row.backend_local_evidence && row.no_hole_evidence &&
           row.fallback_evidence && (!requires_rt_consistency(request, row) || row.rt_consistency_evidence) &&
           !row.request_native_handle_access && !row.request_nanite_claim && !row.request_benchmark_superiority_claim &&
           !row.request_broad_optimization_claim && !row.request_backend_parity_claim;
}

void append_rows_and_counts(MavgQualityGovernorResult& result, const MavgQualityGovernorRequest& request,
                            bool include_output_rows) {
    std::vector<MavgQualityGovernorCounterRow> sorted_rows = request.rows;
    sort_rows(sorted_rows);
    result.row_count = sorted_rows.size();
    for (const auto& row : sorted_rows) {
        if (is_host_gated_row(row)) {
            ++result.host_gated_row_count;
        } else if (row_ready(row, request)) {
            ++result.ready_row_count;
        }
        if (row.request_native_handle_access || row.request_nanite_claim || row.request_benchmark_superiority_claim ||
            row.request_broad_optimization_claim || row.request_backend_parity_claim) {
            ++result.unsupported_claim_row_count;
        }
    }
    if (include_output_rows) {
        result.rows = std::move(sorted_rows);
    }
}

} // namespace

MavgQualityGovernorResult evaluate_mavg_quality_governor(const MavgQualityGovernorRequest& request) {
    MavgQualityGovernorResult result;
    if (request.rows.empty()) {
        result.status = MavgQualityGovernorStatus::no_rows;
        return result;
    }
    if (!valid_limits(request.limits)) {
        add_request_diagnostic(
            result, MavgQualityGovernorDiagnosticCode::invalid_limits, "limits", 0U, 1U,
            "MAVG quality governor limits require positive timing, error, memory, draw, and dispatch budgets");
    }
    if (request.rows.size() > request.row_budget) {
        add_request_diagnostic(result, MavgQualityGovernorDiagnosticCode::row_budget_exceeded, "row_budget",
                               request.rows.size(), request.row_budget,
                               "MAVG quality governor request exceeded its row budget");
    }
    validate_duplicate_rows(result, request);
    for (const auto& row : request.rows) {
        validate_counter_ids(result, row);
        validate_evidence(result, request, row);
        validate_budgets(result, request.limits, row);
        validate_unsupported_claims(result, row);
    }
    if (!result.diagnostics.empty()) {
        append_rows_and_counts(result, request, false);
        sort_diagnostics(result);
        const auto only_budget_diagnostics = std::ranges::all_of(
            result.diagnostics, [](const auto& diagnostic) { return is_budget_diagnostic(diagnostic.code); });
        result.status = only_budget_diagnostics ? MavgQualityGovernorStatus::budget_exceeded
                                                : MavgQualityGovernorStatus::invalid_request;
        return result;
    }

    append_rows_and_counts(result, request, true);
    result.replay_hash = compute_replay_hash(request, result);
    result.selected_benchmark_harness_ready = result.ready_row_count > 0U;
    result.broad_mavg_benchmark_ready = false;
    result.ready = result.host_gated_row_count == 0U && result.ready_row_count == result.row_count;
    result.status = result.host_gated_row_count > 0U ? MavgQualityGovernorStatus::host_evidence_required
                                                     : MavgQualityGovernorStatus::ready;
    return result;
}

bool has_mavg_quality_governor_diagnostic(const MavgQualityGovernorResult& result,
                                          MavgQualityGovernorDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
