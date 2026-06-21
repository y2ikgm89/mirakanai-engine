// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_advanced_backend_evidence.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

struct ParsedDate {
    int year{0};
    unsigned month{0};
    unsigned day{0};
};

struct TaskRow {
    std::string_view row_id;
    bool ready{false};
};

void add_diagnostic(MavgAdvancedBackendEvidenceResult& result, MavgAdvancedBackendEvidenceDiagnosticCode code,
                    std::string_view row_id, std::string message) {
    result.diagnostics.push_back(MavgAdvancedBackendEvidenceDiagnostic{
        .code = code,
        .row_id = std::string(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool parse_unsigned(std::string_view text, unsigned& value) noexcept {
    const auto* const begin = text.data();
    const auto* const end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} && ptr == end;
}

[[nodiscard]] bool parse_int(std::string_view text, int& value) noexcept {
    const auto* const begin = text.data();
    const auto* const end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} && ptr == end;
}

[[nodiscard]] bool is_leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] unsigned days_in_month(int year, unsigned month) noexcept {
    constexpr std::array common_year_days{31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    if (month == 2U && is_leap_year(year)) {
        return 29U;
    }
    return common_year_days[month - 1U];
}

[[nodiscard]] bool parse_yyyy_mm_dd(std::string_view text, ParsedDate& date) noexcept {
    if (text.size() != 10U || text[4] != '-' || text[7] != '-') {
        return false;
    }
    unsigned month = 0;
    unsigned day = 0;
    int year = 0;
    if (!parse_int(text.substr(0U, 4U), year) || !parse_unsigned(text.substr(5U, 2U), month) ||
        !parse_unsigned(text.substr(8U, 2U), day)) {
        return false;
    }
    if (year < 1970 || month == 0U || month > 12U || day == 0U || day > days_in_month(year, month)) {
        return false;
    }
    date = ParsedDate{.year = year, .month = month, .day = day};
    return true;
}

// Days since 1970-01-01, following the civil calendar conversion used for deterministic age checks.
[[nodiscard]] int days_from_civil(ParsedDate date) noexcept {
    auto year = date.year;
    auto month = static_cast<int>(date.month);
    const auto day = static_cast<int>(date.day);
    year -= month <= 2 ? 1 : 0;
    const auto era = (year >= 0 ? year : year - 399) / 400;
    const auto year_of_era = static_cast<unsigned>(year - era * 400);
    const auto month_prime = static_cast<unsigned>(month + (month > 2 ? -3 : 9));
    const auto day_of_year = (153U * month_prime + 2U) / 5U + static_cast<unsigned>(day) - 1U;
    const auto day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U + day_of_year;
    return era * 146097 + static_cast<int>(day_of_era) - 719468;
}

void require_context7_row(MavgAdvancedBackendEvidenceResult& result, std::string_view row_id, bool ready) {
    if (!ready) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_context7_source, row_id,
                       "MAVG advanced backend evidence requires the Context7 source row to be ready");
    }
}

void require_official_row(MavgAdvancedBackendEvidenceResult& result, std::string_view row_id, bool ready) {
    if (!ready) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_official_source, row_id,
                       "MAVG advanced backend evidence requires the official source row to be ready");
    }
}

void require_official_doc_date(MavgAdvancedBackendEvidenceResult& result, std::string_view row_id,
                               std::string_view date) {
    ParsedDate parsed;
    if (date.empty() || !parse_yyyy_mm_dd(date, parsed)) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_official_source_doc_date, row_id,
                       "MAVG advanced backend evidence requires a valid official source document date");
    }
}

void evaluate_source_gate(MavgAdvancedBackendEvidenceResult& result, const MavgAdvancedBackendEvidenceDesc& desc) {
    ParsedDate source_date;
    ParsedDate current_date;
    if (desc.source_gate_date_yyyy_mm_dd.empty()) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_source_gate_date,
                       "source_gate_date_yyyy_mm_dd", "MAVG advanced backend evidence requires a source gate date");
    } else if (!parse_yyyy_mm_dd(desc.source_gate_date_yyyy_mm_dd, source_date)) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::invalid_source_gate_date,
                       "source_gate_date_yyyy_mm_dd",
                       "MAVG advanced backend evidence source gate date must be YYYY-MM-DD");
    }
    if (desc.source_gate_current_date_yyyy_mm_dd.empty()) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_source_gate_date,
                       "source_gate_current_date_yyyy_mm_dd",
                       "MAVG advanced backend evidence requires the current source validation date");
    } else if (!parse_yyyy_mm_dd(desc.source_gate_current_date_yyyy_mm_dd, current_date)) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::invalid_source_gate_date,
                       "source_gate_current_date_yyyy_mm_dd",
                       "MAVG advanced backend evidence current source validation date must be YYYY-MM-DD");
    }
    if (!desc.source_gate_date_yyyy_mm_dd.empty() && !desc.source_gate_current_date_yyyy_mm_dd.empty() &&
        parse_yyyy_mm_dd(desc.source_gate_date_yyyy_mm_dd, source_date) &&
        parse_yyyy_mm_dd(desc.source_gate_current_date_yyyy_mm_dd, current_date) &&
        days_from_civil(current_date) - days_from_civil(source_date) >
            static_cast<int>(desc.source_gate_max_age_days)) {
        ++result.missing_source_gate_count;
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::stale_source_gate,
                       "source_gate_date_yyyy_mm_dd",
                       "MAVG advanced backend evidence source gate is older than the allowed freshness window");
    }

    require_context7_row(result, "context7_vulkan_docs_ready", desc.context7_vulkan_docs_ready);
    require_context7_row(result, "context7_cmake_ready", desc.context7_cmake_ready);
    require_context7_row(result, "context7_vcpkg_ready", desc.context7_vcpkg_ready);

    require_official_row(result, "official_d3d12_mesh_shader_ready", desc.official_d3d12_mesh_shader_ready);
    require_official_row(result, "official_vulkan_mesh_shader_ready", desc.official_vulkan_mesh_shader_ready);
    require_official_row(result, "official_apple_metal_ready", desc.official_apple_metal_ready);
    require_official_row(result, "official_directstorage_ready", desc.official_directstorage_ready);
    require_official_row(result, "official_nanite_docs_ready", desc.official_nanite_docs_ready);
    require_official_row(result, "official_profiler_docs_ready", desc.official_profiler_docs_ready);

    require_official_doc_date(result, "official_d3d12_mesh_tier_doc_date", desc.official_d3d12_mesh_tier_doc_date);
    require_official_doc_date(result, "official_vulkan_mesh_ext_doc_date", desc.official_vulkan_mesh_ext_doc_date);
    require_official_doc_date(result, "official_metal_feature_table_date", desc.official_metal_feature_table_date);
    require_official_doc_date(result, "official_pix_doc_date", desc.official_pix_doc_date);
    require_official_doc_date(result, "official_nsight_doc_date", desc.official_nsight_doc_date);
    require_official_doc_date(result, "official_rgp_doc_date", desc.official_rgp_doc_date);
    require_official_doc_date(result, "official_intel_gpa_doc_date", desc.official_intel_gpa_doc_date);

    result.source_gate_ready = result.missing_source_gate_count == 0U;
}

void publish_task_flags(MavgAdvancedBackendEvidenceResult& result, const MavgAdvancedBackendEvidenceDesc& desc) {
    if (!result.source_gate_ready) {
        return;
    }
    result.mavg_mesh_shader_lod_d3d12_ready = desc.mavg_mesh_shader_lod_d3d12_ready;
    result.mavg_mesh_shader_lod_vulkan_ready = desc.mavg_mesh_shader_lod_vulkan_ready;
    result.mavg_metal_mesh_lod_ready = desc.mavg_metal_mesh_lod_ready;
    result.mavg_mesh_shader_lod_ready = desc.mavg_mesh_shader_lod_d3d12_ready &&
                                        desc.mavg_mesh_shader_lod_vulkan_ready && desc.mavg_metal_mesh_lod_ready;
    result.mavg_package_visible_backend_readiness_ready = desc.mavg_package_visible_backend_readiness_ready;
    result.mavg_autonomous_streaming_scheduler_ready = desc.mavg_autonomous_streaming_scheduler_ready;
    result.mavg_async_overlap_measured_performance_ready = desc.mavg_async_overlap_measured_performance_ready;
    result.mavg_deformation_integration_ready = desc.mavg_deformation_integration_ready;
    result.mavg_ray_tracing_integration_ready = desc.mavg_ray_tracing_integration_ready;
    result.mavg_broad_cpu_gpu_memory_optimization_ready = desc.mavg_broad_cpu_gpu_memory_optimization_ready;
    result.mavg_nanite_comparison_report_ready = desc.mavg_nanite_comparison_report_ready;
}

void add_missing_task_row(MavgAdvancedBackendEvidenceResult& result, std::string_view row_id) {
    ++result.missing_task_row_count;
    add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_task_row, row_id,
                   "MAVG advanced backend evidence task row is not ready");
}

void evaluate_task_rows(MavgAdvancedBackendEvidenceResult& result, const MavgAdvancedBackendEvidenceDesc& desc) {
    const std::array rows{
        TaskRow{.row_id = "mavg_package_visible_backend_readiness_ready",
                .ready = desc.mavg_package_visible_backend_readiness_ready},
        TaskRow{.row_id = "mavg_mesh_shader_lod_d3d12_ready", .ready = desc.mavg_mesh_shader_lod_d3d12_ready},
        TaskRow{.row_id = "mavg_mesh_shader_lod_vulkan_ready", .ready = desc.mavg_mesh_shader_lod_vulkan_ready},
        TaskRow{.row_id = "mavg_metal_mesh_lod_ready", .ready = desc.mavg_metal_mesh_lod_ready},
        TaskRow{.row_id = "mavg_autonomous_streaming_scheduler_ready",
                .ready = desc.mavg_autonomous_streaming_scheduler_ready},
        TaskRow{.row_id = "mavg_async_overlap_measured_performance_ready",
                .ready = desc.mavg_async_overlap_measured_performance_ready},
        TaskRow{.row_id = "mavg_deformation_integration_ready", .ready = desc.mavg_deformation_integration_ready},
        TaskRow{.row_id = "mavg_ray_tracing_integration_ready", .ready = desc.mavg_ray_tracing_integration_ready},
        TaskRow{.row_id = "mavg_broad_cpu_gpu_memory_optimization_ready",
                .ready = desc.mavg_broad_cpu_gpu_memory_optimization_ready},
        TaskRow{.row_id = "mavg_nanite_comparison_report_ready", .ready = desc.mavg_nanite_comparison_report_ready},
    };
    for (const auto& row : rows) {
        if (!row.ready) {
            add_missing_task_row(result, row.row_id);
        }
    }
    if (!desc.mavg_mesh_shader_lod_d3d12_ready || !desc.mavg_mesh_shader_lod_vulkan_ready ||
        !desc.mavg_metal_mesh_lod_ready) {
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::missing_task_row,
                       "mavg_mesh_shader_lod_ready",
                       "MAVG advanced backend evidence requires every selected mesh shader LOD backend row");
    }
}

void evaluate_forbidden_claims(MavgAdvancedBackendEvidenceResult& result, const MavgAdvancedBackendEvidenceDesc& desc) {
    if (desc.request_mavg_nanite_compatible || desc.request_mavg_nanite_equivalent ||
        desc.request_mavg_nanite_superior) {
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::unsupported_nanite_product_claim,
                       "mavg_nanite_product_claim",
                       "MAVG advanced backend evidence cannot claim Nanite compatibility, equivalence, or superiority");
    }
    if (desc.request_current_active_plan_mutation) {
        add_diagnostic(result, MavgAdvancedBackendEvidenceDiagnosticCode::current_active_plan_mutation_requested,
                       "currentActivePlan",
                       "MAVG advanced backend evidence candidate cannot mutate currentActivePlan during Task 1");
    }
}

} // namespace

MavgAdvancedBackendEvidenceResult evaluate_mavg_advanced_backend_evidence(const MavgAdvancedBackendEvidenceDesc& desc) {
    MavgAdvancedBackendEvidenceResult result;
    evaluate_source_gate(result, desc);
    publish_task_flags(result, desc);
    evaluate_task_rows(result, desc);
    evaluate_forbidden_claims(result, desc);

    result.mavg_advanced_backend_evidence_ready =
        result.source_gate_ready && result.mavg_package_visible_backend_readiness_ready &&
        result.mavg_mesh_shader_lod_ready && result.mavg_autonomous_streaming_scheduler_ready &&
        result.mavg_async_overlap_measured_performance_ready && result.mavg_deformation_integration_ready &&
        result.mavg_ray_tracing_integration_ready && result.mavg_broad_cpu_gpu_memory_optimization_ready &&
        result.mavg_nanite_comparison_report_ready && result.diagnostics.empty();
    return result;
}

bool has_mavg_advanced_backend_evidence_diagnostic(const MavgAdvancedBackendEvidenceResult& result,
                                                   MavgAdvancedBackendEvidenceDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const MavgAdvancedBackendEvidenceDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

bool has_mavg_advanced_backend_evidence_row_diagnostic(const MavgAdvancedBackendEvidenceResult& result,
                                                       std::string_view row_id) noexcept {
    return std::ranges::any_of(result.diagnostics, [row_id](const MavgAdvancedBackendEvidenceDiagnostic& diagnostic) {
        return diagnostic.row_id == row_id;
    });
}

} // namespace mirakana::runtime_rhi
