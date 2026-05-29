// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/production_vfx_profiling.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal;
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_native_token(std::string_view token) {
    return token == "native" || token == "handle" || token == "hwnd" || token == "hinstance" || token == "id3d12" ||
           token == "vkhandle" || token == "mtl" || token == "sdl" || token == "sdl3" || token == "imgui";
}

[[nodiscard]] bool has_native_token(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_native_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_native_token(token);
}

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_backend(const std::vector<rhi::BackendKind>& backends, rhi::BackendKind backend) {
    return std::ranges::find(backends, backend) != backends.end();
}

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] bool row_backend_allowed(const RendererProductionVfxProfilingRequest& request, rhi::BackendKind backend) {
    return is_supported_backend(backend) && contains_backend(request.required_backends, backend);
}

void add_diagnostic(RendererProductionVfxProfilingPlan& plan, RendererProductionVfxProfilingDiagnosticCode code,
                    rhi::BackendKind backend, std::string row_id, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RendererProductionVfxProfilingDiagnostic{
        .code = code,
        .backend = backend,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] std::size_t request_row_count(const RendererProductionVfxProfilingRequest& request) noexcept {
    return request.required_backends.size() + request.feature_rows.size() + request.gpu_particle_budget_rows.size() +
           request.postprocess_rows.size() + request.backend_timing_rows.size() + request.cpu_profile_rows.size() +
           request.package_counter_rows.size() + request.crash_telemetry_handoff_rows.size();
}

void sort_required_backends(std::vector<rhi::BackendKind>& backends) {
    std::ranges::sort(backends,
                      [](const auto lhs, const auto rhs) { return backend_sort_key(lhs) < backend_sort_key(rhs); });
}

void sort_diagnostics(RendererProductionVfxProfilingPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::string row_key(const RendererProductionVfxProfilingDiagnostic& diagnostic) {
    std::string key;
    key.reserve(diagnostic.row_id.size() + 32U);
    key.append(std::to_string(static_cast<std::uint8_t>(diagnostic.backend)));
    key.push_back('\n');
    key.append(diagnostic.row_id);
    key.push_back('\n');
    key.append(std::to_string(diagnostic.source_index));
    return key;
}

[[nodiscard]] std::size_t count_rejected_unsafe_rows(const RendererProductionVfxProfilingPlan& plan) {
    std::vector<std::string> keys;
    keys.reserve(plan.diagnostics.size());
    for (const auto& diagnostic : plan.diagnostics) {
        auto key = row_key(diagnostic);
        if (std::ranges::find(keys, key) == keys.end()) {
            keys.push_back(std::move(key));
        }
    }
    return keys.size();
}

void validate_required_backends(RendererProductionVfxProfilingPlan& plan,
                                const RendererProductionVfxProfilingRequest& request) {
    std::vector<rhi::BackendKind> seen;
    seen.reserve(request.required_backends.size());
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_required_backend, backend, {},
                           "production renderer VFX profiling supports d3d12, vulkan, and metal backends only", 0U);
            continue;
        }
        if (contains_backend(seen, backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::duplicate_required_backend, backend, {},
                           "required renderer production backends must be unique", 0U);
            continue;
        }
        seen.push_back(backend);
    }
}

void validate_feature_rows(RendererProductionVfxProfilingPlan& plan,
                           const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.feature_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend,
                           row.feature_id, "feature row backend must be one of the required production backends",
                           row.source_index);
        }
        if (!is_valid_id(row.feature_id) || has_native_token(row.feature_id) || !row.reviewed) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_feature_row, row.backend,
                           row.feature_id, "feature rows require reviewed backend-neutral feature ids",
                           row.source_index);
        }
        if (row.request_broad_performance_claim) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::broad_performance_claim, row.backend,
                           row.feature_id, "broad renderer performance claims require separate measured evidence",
                           row.source_index);
        }
        if (row.request_native_handle_access) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_native_handle_claim,
                           row.backend, row.feature_id,
                           "production VFX evidence must not expose native renderer or RHI handles", row.source_index);
        }
    }
}

void validate_particle_budget_rows(RendererProductionVfxProfilingPlan& plan,
                                   const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.gpu_particle_budget_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(
                plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend, row.effect_id,
                "GPU particle budget row backend must be one of the required production backends", row.source_index);
        }
        if (!is_valid_id(row.effect_id) || has_native_token(row.effect_id) || row.max_particles == 0U ||
            row.max_emitters == 0U || row.max_spawn_per_frame == 0U || row.max_spawn_per_frame > row.max_particles ||
            row.simulation_budget_us == 0U || row.submission_budget_us == 0U) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_gpu_particle_budget, row.backend,
                           row.effect_id,
                           "GPU particle budgets require backend-neutral ids and positive bounded work budgets",
                           row.source_index);
        }
    }
}

void validate_postprocess_rows(RendererProductionVfxProfilingPlan& plan,
                               const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.postprocess_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend,
                           row.chain_id, "postprocess row backend must be one of the required production backends",
                           row.source_index);
        }
        if (!is_valid_id(row.chain_id) || has_native_token(row.chain_id) || row.pass_count == 0U ||
            row.pass_count > 8U || !row.scene_color_available || !row.scene_depth_available || !row.hdr_available ||
            !row.history_available || !row.backend_shader_evidence_ready) {
            add_diagnostic(
                plan, RendererProductionVfxProfilingDiagnosticCode::invalid_postprocess_row, row.backend, row.chain_id,
                "postprocess rows require scene color, depth, HDR, history, shader evidence, and pass limits",
                row.source_index);
        }
    }
}

[[nodiscard]] bool is_valid_timing_row(const RendererProductionBackendTimingRow& row) {
    return is_valid_id(row.profile_zone_id) && !has_native_token(row.profile_zone_id) &&
           row.gpu_timestamp_frequency_hz > 0U && row.end_tick > row.begin_tick &&
           row.calibrated_cpu_end_tick >= row.calibrated_cpu_begin_tick && row.max_clock_deviation_ns > 0U &&
           (row.debug_scope_count > 0U || row.debug_marker_count > 0U);
}

[[nodiscard]] bool is_valid_cpu_profile_row(const RendererProductionCpuProfileRow& row) {
    return is_valid_id(row.profile_zone_id) && !has_native_token(row.profile_zone_id) &&
           row.end_tick > row.begin_tick && row.budget_us > 0U && row.sample_count > 0U;
}

[[nodiscard]] bool is_valid_package_counter_row(const RendererProductionPackageCounterRow& row) {
    return is_valid_id(row.counter_id) && !has_native_token(row.counter_id) && row.ready != row.host_gated;
}

[[nodiscard]] bool requires_strict_backend_evidence(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan;
}

[[nodiscard]] bool backend_synchronization_evidence_ready(const RendererProductionBackendTimingRow& row) noexcept {
    return row.resource_barrier_count > 0U && row.layout_transition_count > 0U && row.queue_wait_count > 0U &&
           row.queue_ownership_transfer_reviewed;
}

[[nodiscard]] bool backend_shader_validation_ready(const RendererProductionBackendTimingRow& row) noexcept {
    return row.shader_validation_count > 0U;
}

[[nodiscard]] bool backend_host_recipe_ready(const RendererProductionBackendTimingRow& row) noexcept {
    return row.host_validated && row.strict_host_recipe_ready;
}

[[nodiscard]] bool backend_host_evidence_ready(const RendererProductionBackendTimingRow& row) noexcept {
    return is_valid_timing_row(row) && backend_synchronization_evidence_ready(row) &&
           backend_shader_validation_ready(row) && row.backend_validation_ready && backend_host_recipe_ready(row) &&
           row.capture_handoff_ready;
}

[[nodiscard]] bool should_validate_full_backend_evidence(const RendererProductionBackendTimingRow& row) noexcept {
    return requires_strict_backend_evidence(row.backend) || row.host_validated;
}

[[nodiscard]] RendererProductionBackendEvidenceRow
make_backend_evidence_row(const RendererProductionBackendTimingRow& row) {
    const auto timing_ready = is_valid_timing_row(row);
    const auto synchronization_ready = backend_synchronization_evidence_ready(row);
    const auto shader_ready = backend_shader_validation_ready(row);
    const auto host_recipe_ready = backend_host_recipe_ready(row);
    const auto host_ready = backend_host_evidence_ready(row);
    return RendererProductionBackendEvidenceRow{
        .backend = row.backend,
        .profile_zone_id = row.profile_zone_id,
        .resource_barrier_count = row.resource_barrier_count,
        .layout_transition_count = row.layout_transition_count,
        .queue_wait_count = row.queue_wait_count,
        .queue_ownership_transfer_reviewed = row.queue_ownership_transfer_reviewed,
        .shader_validation_count = row.shader_validation_count,
        .timing_ready = timing_ready,
        .synchronization_ready = synchronization_ready,
        .shader_validation_ready = shader_ready,
        .backend_validation_ready = row.backend_validation_ready,
        .host_recipe_ready = host_recipe_ready,
        .capture_handoff_ready = row.capture_handoff_ready,
        .host_evidence_ready = host_ready,
        .host_gated = row.backend == rhi::BackendKind::metal && !host_ready,
        .source_index = row.source_index,
    };
}

void validate_timing_rows(RendererProductionVfxProfilingPlan& plan,
                          const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.backend_timing_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend,
                           row.profile_zone_id,
                           "backend timing row backend must be one of the required production backends",
                           row.source_index);
        }
        if (!is_valid_timing_row(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_backend_timing, row.backend,
                           row.profile_zone_id,
                           "backend timing rows require timestamp frequency, ordered ticks, calibration, and markers",
                           row.source_index);
            continue;
        }
        if (!should_validate_full_backend_evidence(row)) {
            continue;
        }
        if (!backend_synchronization_evidence_ready(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_synchronization_evidence,
                           row.backend, row.profile_zone_id,
                           "backend evidence requires reviewed barriers, layout transitions, queue waits, and queue "
                           "ownership transfer assumptions",
                           row.source_index);
        }
        if (!backend_shader_validation_ready(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_shader_validation,
                           row.backend, row.profile_zone_id,
                           "backend evidence requires shader artifact or tool validation proof", row.source_index);
        }
        if (!row.backend_validation_ready) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_validation_evidence,
                           row.backend, row.profile_zone_id,
                           "backend evidence requires debug layer or validation layer proof", row.source_index);
        }
        if (!backend_host_recipe_ready(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_host_evidence,
                           row.backend, row.profile_zone_id,
                           "backend evidence requires an explicit host validation recipe result", row.source_index);
        }
        if (!row.capture_handoff_ready) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_capture_handoff,
                           row.backend, row.profile_zone_id,
                           "backend evidence requires reviewed operator capture handoff proof without executing "
                           "external capture",
                           row.source_index);
        }
    }
}

void validate_cpu_profile_rows(RendererProductionVfxProfilingPlan& plan,
                               const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.cpu_profile_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend,
                           row.profile_zone_id,
                           "CPU profile row backend must be one of the required production backends", row.source_index);
        }
        if (!is_valid_cpu_profile_row(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_cpu_profile_row, row.backend,
                           row.profile_zone_id,
                           "CPU profile rows require backend-neutral profile ids, ordered ticks, budget, and samples",
                           row.source_index);
        }
    }
}

void validate_package_counter_rows(RendererProductionVfxProfilingPlan& plan,
                                   const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.package_counter_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(
                plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend, row.counter_id,
                "package counter row backend must be one of the required production backends", row.source_index);
        }
        if (!is_valid_package_counter_row(row)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_package_counter_row, row.backend,
                           row.counter_id,
                           "package counter rows require backend-neutral ids and exactly one ready or host-gated state",
                           row.source_index);
        }
    }
}

void validate_crash_handoff_rows(RendererProductionVfxProfilingPlan& plan,
                                 const RendererProductionVfxProfilingRequest& request) {
    for (const auto& row : request.crash_telemetry_handoff_rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_backend, row.backend,
                           row.handoff_id,
                           "crash telemetry handoff row backend must be one of the required production backends",
                           row.source_index);
        }
        if (!is_valid_id(row.handoff_id) || has_native_token(row.handoff_id) || row.trace_event_count == 0U ||
            !row.crash_dump_reviewed || !row.symbolication_ready || !row.telemetry_schema_reviewed ||
            !row.operator_handoff_ready) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_crash_telemetry_handoff,
                           row.backend, row.handoff_id,
                           "crash telemetry handoff rows require reviewed dumps, symbols, schema, and operator handoff",
                           row.source_index);
        }
        if (row.request_crash_upload) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_crash_upload, row.backend,
                           row.handoff_id, "automatic crash telemetry upload remains outside this value-only planner",
                           row.source_index);
        }
        if (row.request_native_capture) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_native_handle_claim,
                           row.backend, row.handoff_id,
                           "native capture execution remains operator-owned and host-gated", row.source_index);
        }
    }
}

template <class Row, class IdFn>
[[nodiscard]] bool has_row_for_backend(const std::vector<Row>& rows, rhi::BackendKind backend,
                                       const IdFn& is_valid_row) {
    return std::ranges::any_of(
        rows, [backend, &is_valid_row](const auto& row) { return row.backend == backend && is_valid_row(row); });
}

void validate_backend_parity(RendererProductionVfxProfilingPlan& plan,
                             const RendererProductionVfxProfilingRequest& request) {
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            continue;
        }

        const auto has_feature = has_row_for_backend(
            request.feature_rows, backend, [](const auto& row) { return is_valid_id(row.feature_id) && row.reviewed; });
        const auto has_budget = has_row_for_backend(request.gpu_particle_budget_rows, backend, [](const auto& row) {
            return is_valid_id(row.effect_id) && row.max_particles > 0U;
        });
        const auto has_postprocess = has_row_for_backend(request.postprocess_rows, backend, [](const auto& row) {
            return is_valid_id(row.chain_id) && row.pass_count > 0U && row.backend_shader_evidence_ready;
        });
        const auto has_crash = has_row_for_backend(request.crash_telemetry_handoff_rows, backend, [](const auto& row) {
            return is_valid_id(row.handoff_id) && row.crash_dump_reviewed && row.operator_handoff_ready;
        });
        const auto has_timing = has_row_for_backend(request.backend_timing_rows, backend,
                                                    [](const auto& row) { return is_valid_timing_row(row); });
        const auto has_cpu_profile = has_row_for_backend(request.cpu_profile_rows, backend,
                                                         [](const auto& row) { return is_valid_cpu_profile_row(row); });
        const auto has_package_counter = has_row_for_backend(
            request.package_counter_rows, backend, [](const auto& row) { return is_valid_package_counter_row(row); });

        if (!(has_feature && has_budget && has_postprocess && has_crash)) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_parity, backend, {},
                           "each required backend needs its own feature, particle, postprocess, and crash handoff rows",
                           0U);
        }
        if (!has_timing) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_timing, backend, {},
                           "each required backend needs its own timing/profile evidence row", 0U);
        }
        if (!has_cpu_profile) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_cpu_profile_rows, backend, {},
                           "each required backend needs its own CPU profile evidence row", 0U);
        }
        if (!has_package_counter) {
            add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::missing_package_counter_rows, backend,
                           {}, "each required backend needs its own package-visible readiness counter row", 0U);
        }
    }
}

void validate_budgets(RendererProductionVfxProfilingPlan& plan, const RendererProductionVfxProfilingRequest& request) {
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RendererProductionVfxProfilingDiagnosticCode::row_budget_exceeded, rhi::BackendKind::null,
                       {}, "renderer production VFX/profile request exceeds its row budget", 0U);
    }
}

template <class Row, class IdFn> void sort_rows(std::vector<Row>& rows, const IdFn& id_fn) {
    std::ranges::sort(rows, [&id_fn](const auto& lhs, const auto& rhs) {
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        const auto lhs_id = id_fn(lhs);
        const auto rhs_id = id_fn(rhs);
        if (lhs_id != rhs_id) {
            return lhs_id < rhs_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void append_output_rows(RendererProductionVfxProfilingPlan& plan,
                        const RendererProductionVfxProfilingRequest& request) {
    plan.required_backends = request.required_backends;
    sort_required_backends(plan.required_backends);
    plan.feature_rows = request.feature_rows;
    plan.gpu_particle_budget_rows = request.gpu_particle_budget_rows;
    plan.postprocess_rows = request.postprocess_rows;
    plan.backend_timing_rows = request.backend_timing_rows;
    plan.cpu_profile_rows = request.cpu_profile_rows;
    plan.package_counter_rows = request.package_counter_rows;
    plan.crash_telemetry_handoff_rows = request.crash_telemetry_handoff_rows;

    sort_rows(plan.feature_rows, [](const auto& row) -> std::string_view { return row.feature_id; });
    sort_rows(plan.gpu_particle_budget_rows, [](const auto& row) -> std::string_view { return row.effect_id; });
    sort_rows(plan.postprocess_rows, [](const auto& row) -> std::string_view { return row.chain_id; });
    sort_rows(plan.backend_timing_rows, [](const auto& row) -> std::string_view { return row.profile_zone_id; });
    sort_rows(plan.cpu_profile_rows, [](const auto& row) -> std::string_view { return row.profile_zone_id; });
    sort_rows(plan.package_counter_rows, [](const auto& row) -> std::string_view { return row.counter_id; });
    sort_rows(plan.crash_telemetry_handoff_rows, [](const auto& row) -> std::string_view { return row.handoff_id; });
    plan.backend_evidence_rows.reserve(plan.backend_timing_rows.size());
    for (const auto& row : plan.backend_timing_rows) {
        plan.backend_evidence_rows.push_back(make_backend_evidence_row(row));
    }
    sort_rows(plan.backend_evidence_rows, [](const auto& row) -> std::string_view { return row.profile_zone_id; });

    plan.feature_row_count = plan.feature_rows.size();
    plan.gpu_particle_budget_row_count = plan.gpu_particle_budget_rows.size();
    plan.postprocess_row_count = plan.postprocess_rows.size();
    plan.backend_timing_row_count = plan.backend_timing_rows.size();
    plan.backend_evidence_row_count = plan.backend_evidence_rows.size();
    plan.cpu_profile_row_count = plan.cpu_profile_rows.size();
    plan.package_counter_row_count = plan.package_counter_rows.size();
    plan.package_counter_ready_count = static_cast<std::size_t>(
        std::ranges::count_if(plan.package_counter_rows, [](const auto& row) { return row.ready; }));
    plan.package_counter_host_gated_count = static_cast<std::size_t>(
        std::ranges::count_if(plan.package_counter_rows, [](const auto& row) { return row.host_gated; }));
    plan.crash_telemetry_handoff_row_count = plan.crash_telemetry_handoff_rows.size();
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

[[nodiscard]] std::uint64_t compute_replay_hash(const RendererProductionVfxProfilingPlan& plan,
                                                const RendererProductionVfxProfilingRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    for (const auto backend : plan.required_backends) {
        hash_mix(hash, static_cast<std::uint8_t>(backend));
    }
    for (const auto& row : plan.feature_rows) {
        hash_string(hash, row.feature_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.request_broad_performance_claim ? 1U : 0U);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.gpu_particle_budget_rows) {
        hash_string(hash, row.effect_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, row.max_particles);
        hash_mix(hash, row.max_emitters);
        hash_mix(hash, row.max_spawn_per_frame);
        hash_mix(hash, row.simulation_budget_us);
        hash_mix(hash, row.submission_budget_us);
        hash_mix(hash, row.requires_compute_simulation ? 1U : 0U);
        hash_mix(hash, row.requires_gpu_sort ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.postprocess_rows) {
        hash_string(hash, row.chain_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, row.pass_count);
        hash_mix(hash, row.scene_color_available ? 1U : 0U);
        hash_mix(hash, row.scene_depth_available ? 1U : 0U);
        hash_mix(hash, row.hdr_available ? 1U : 0U);
        hash_mix(hash, row.history_available ? 1U : 0U);
        hash_mix(hash, row.backend_shader_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.backend_timing_rows) {
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_string(hash, row.profile_zone_id);
        hash_mix(hash, row.gpu_timestamp_frequency_hz);
        hash_mix(hash, row.begin_tick);
        hash_mix(hash, row.end_tick);
        hash_mix(hash, row.end_tick - row.begin_tick);
        hash_mix(hash, row.calibrated_cpu_begin_tick);
        hash_mix(hash, row.calibrated_cpu_end_tick);
        hash_mix(hash, row.max_clock_deviation_ns);
        hash_mix(hash, row.debug_scope_count);
        hash_mix(hash, row.debug_marker_count);
        hash_mix(hash, row.resource_barrier_count);
        hash_mix(hash, row.layout_transition_count);
        hash_mix(hash, row.queue_wait_count);
        hash_mix(hash, row.queue_ownership_transfer_reviewed ? 1U : 0U);
        hash_mix(hash, row.shader_validation_count);
        hash_mix(hash, row.backend_validation_ready ? 1U : 0U);
        hash_mix(hash, row.strict_host_recipe_ready ? 1U : 0U);
        hash_mix(hash, row.capture_handoff_ready ? 1U : 0U);
        hash_mix(hash, row.host_validated ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.cpu_profile_rows) {
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_string(hash, row.profile_zone_id);
        hash_mix(hash, row.begin_tick);
        hash_mix(hash, row.end_tick);
        hash_mix(hash, row.end_tick - row.begin_tick);
        hash_mix(hash, row.budget_us);
        hash_mix(hash, row.sample_count);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.package_counter_rows) {
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_string(hash, row.counter_id);
        hash_mix(hash, row.ready ? 1U : 0U);
        hash_mix(hash, row.host_gated ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    for (const auto& row : plan.crash_telemetry_handoff_rows) {
        hash_string(hash, row.handoff_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, row.trace_event_count);
        hash_mix(hash, row.crash_dump_reviewed ? 1U : 0U);
        hash_mix(hash, row.symbolication_ready ? 1U : 0U);
        hash_mix(hash, row.telemetry_schema_reviewed ? 1U : 0U);
        hash_mix(hash, row.operator_handoff_ready ? 1U : 0U);
        hash_mix(hash, row.request_crash_upload ? 1U : 0U);
        hash_mix(hash, row.request_native_capture ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

void compute_host_evidence(RendererProductionVfxProfilingPlan& plan) {
    std::vector<rhi::BackendKind> host_validated_backends;
    for (const auto& row : plan.backend_evidence_rows) {
        if (row.host_evidence_ready) {
            ++plan.backend_evidence_ready_count;
        }
        if (row.host_gated) {
            ++plan.backend_evidence_host_gated_count;
        }
    }
    for (const auto backend : plan.required_backends) {
        bool has_backend_row{false};
        bool all_backend_rows_ready{true};
        for (const auto& row : plan.backend_evidence_rows) {
            if (row.backend != backend) {
                continue;
            }
            has_backend_row = true;
            all_backend_rows_ready = all_backend_rows_ready && row.host_evidence_ready;
        }
        const auto backend_ready = has_backend_row && all_backend_rows_ready;
        if (backend_ready && !contains_backend(host_validated_backends, backend)) {
            host_validated_backends.push_back(backend);
        }
        if (backend == rhi::BackendKind::d3d12) {
            plan.d3d12_host_evidence_ready = backend_ready;
        } else if (backend == rhi::BackendKind::vulkan) {
            plan.vulkan_strict_host_evidence_ready = backend_ready;
        } else if (backend == rhi::BackendKind::metal) {
            plan.requires_metal_host_evidence = true;
            plan.metal_host_evidence_ready = backend_ready;
            plan.has_metal_host_evidence = backend_ready;
        }
    }
    plan.host_validated_backend_count = host_validated_backends.size();
}

} // namespace

bool RendererProductionVfxProfilingPlan::succeeded() const noexcept {
    return status == RendererProductionVfxProfilingStatus::ready ||
           status == RendererProductionVfxProfilingStatus::no_rows;
}

RendererProductionVfxProfilingPlan
plan_renderer_production_vfx_profiling(const RendererProductionVfxProfilingRequest& request) {
    RendererProductionVfxProfilingPlan plan;

    if (request_row_count(request) == 0U) {
        plan.status = RendererProductionVfxProfilingStatus::no_rows;
        return plan;
    }

    validate_required_backends(plan, request);
    validate_feature_rows(plan, request);
    validate_particle_budget_rows(plan, request);
    validate_postprocess_rows(plan, request);
    validate_timing_rows(plan, request);
    validate_cpu_profile_rows(plan, request);
    validate_package_counter_rows(plan, request);
    validate_crash_handoff_rows(plan, request);
    validate_backend_parity(plan, request);
    validate_budgets(plan, request);

    if (!plan.diagnostics.empty()) {
        plan.rejected_unsafe_row_count = count_rejected_unsafe_rows(plan);
        sort_diagnostics(plan);
        plan.status = RendererProductionVfxProfilingStatus::invalid_request;
        return plan;
    }

    append_output_rows(plan, request);
    compute_host_evidence(plan);
    plan.replay_hash = compute_replay_hash(plan, request);
    plan.status = plan.requires_metal_host_evidence && !plan.has_metal_host_evidence
                      ? RendererProductionVfxProfilingStatus::host_evidence_required
                      : RendererProductionVfxProfilingStatus::ready;
    return plan;
}

} // namespace mirakana
