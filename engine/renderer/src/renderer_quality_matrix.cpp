// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/renderer_quality_matrix.hpp"

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
    RendererQualityFeatureKind::materials,         RendererQualityFeatureKind::lighting_shadows,
    RendererQualityFeatureKind::postprocess,       RendererQualityFeatureKind::sprite_ui,
    RendererQualityFeatureKind::scene_scale,       RendererQualityFeatureKind::gpu_memory_residency,
    RendererQualityFeatureKind::profiling_capture,
};

constexpr std::array kReadyEvidenceCategories{
    RendererQualityEvidenceCategory::synchronization,  RendererQualityEvidenceCategory::shader_tool_validation,
    RendererQualityEvidenceCategory::memory_residency, RendererQualityEvidenceCategory::render_pass_frame_graph,
    RendererQualityEvidenceCategory::profiling,        RendererQualityEvidenceCategory::package_evidence,
};

[[nodiscard]] bool is_supported_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal;
}

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] std::uint8_t feature_sort_key(RendererQualityFeatureKind feature) noexcept {
    return static_cast<std::uint8_t>(feature);
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

[[nodiscard]] bool row_backend_allowed(const RendererQualityMatrixRequest& request, rhi::BackendKind backend) {
    return is_supported_backend(backend) && contains_backend(request.required_backends, backend);
}

[[nodiscard]] bool has_evidence_category(const RendererQualityMatrixRow& row,
                                         RendererQualityEvidenceCategory category) {
    return std::ranges::find(row.evidence_categories, category) != row.evidence_categories.end();
}

[[nodiscard]] bool has_ready_evidence_categories(const RendererQualityMatrixRow& row) {
    return std::ranges::all_of(kReadyEvidenceCategories,
                               [&row](const auto category) { return has_evidence_category(row, category); });
}

[[nodiscard]] bool is_host_gated_row(const RendererQualityMatrixRow& row) noexcept {
    return row.status == RendererQualityRowStatus::host_gated ||
           (row.backend == rhi::BackendKind::metal && row.host_gate_required && !row.host_validated);
}

[[nodiscard]] bool is_dependency_gated_row(const RendererQualityMatrixRow& row) noexcept {
    return row.status == RendererQualityRowStatus::dependency_gated;
}

[[nodiscard]] bool is_unsupported_row(const RendererQualityMatrixRow& row) noexcept {
    return row.status == RendererQualityRowStatus::unsupported;
}

[[nodiscard]] bool resource_synchronization_ready(const RendererQualityMatrixRow& row) noexcept {
    switch (row.backend) {
    case rhi::BackendKind::d3d12:
        return row.d3d12_resource_state_barrier_evidence && row.d3d12_fence_evidence;
    case rhi::BackendKind::vulkan:
        return row.vulkan_synchronization2_evidence && row.vulkan_layout_transition_evidence;
    case rhi::BackendKind::metal:
        return row.metal_resource_synchronization_evidence && row.metal_feature_set_evidence;
    case rhi::BackendKind::null:
        return false;
    }
    return false;
}

[[nodiscard]] bool backend_validation_ready(const RendererQualityMatrixRow& row) noexcept {
    if (row.backend == rhi::BackendKind::vulkan) {
        return row.vulkan_validation_layer_evidence && row.vulkan_spirv_validation_evidence;
    }
    return true;
}

[[nodiscard]] bool package_counter_ids_ready(const RendererQualityMatrixRow& row) {
    if (row.package_counter_ids.empty()) {
        return false;
    }
    return std::ranges::all_of(row.package_counter_ids, [](const auto& counter_id) {
        return is_valid_id(counter_id) && !has_native_token(counter_id);
    });
}

[[nodiscard]] bool row_ready(const RendererQualityMatrixRow& row) {
    return row.status == RendererQualityRowStatus::ready && has_ready_evidence_categories(row) && row.reviewed &&
           row.backend_local_evidence && resource_synchronization_ready(row) && backend_validation_ready(row) &&
           row.shader_tool_validation_evidence && package_counter_ids_ready(row) && row.timing_budget_us > 0U &&
           row.gpu_memory_evidence && row.backend_parity_evidence && row.host_validated &&
           !row.request_native_handle_access && !row.request_capture_execution && !row.request_crash_upload_execution &&
           !row.request_inferred_backend_parity && !row.request_subjective_visual_quality_claim;
}

void add_diagnostic(RendererQualityMatrixPlan& plan, RendererQualityMatrixDiagnosticCode code, rhi::BackendKind backend,
                    RendererQualityFeatureKind feature, std::string row_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(RendererQualityMatrixDiagnostic{
        .code = code,
        .backend = backend,
        .feature = feature,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] std::size_t request_row_count(const RendererQualityMatrixRequest& request) noexcept {
    return request.required_backends.size() + request.rows.size();
}

void sort_required_backends(std::vector<rhi::BackendKind>& backends) {
    std::ranges::sort(backends,
                      [](const auto lhs, const auto rhs) { return backend_sort_key(lhs) < backend_sort_key(rhs); });
}

void sort_rows(std::vector<RendererQualityMatrixRow>& rows) {
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

void sort_diagnostics(RendererQualityMatrixPlan& plan) {
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
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

void validate_required_backends(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    std::vector<rhi::BackendKind> seen;
    seen.reserve(request.required_backends.size());
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::invalid_required_backend, backend,
                           RendererQualityFeatureKind::materials, {},
                           "renderer quality matrix supports d3d12, vulkan, and metal backends only", 0U);
            continue;
        }
        if (contains_backend(seen, backend)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::duplicate_required_backend, backend,
                           RendererQualityFeatureKind::materials, {},
                           "renderer quality matrix required backends must be unique", 0U);
            continue;
        }
        seen.push_back(backend);
    }
}

void validate_duplicate_rows(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    std::vector<std::string> seen;
    seen.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        std::string key;
        key.append(std::to_string(static_cast<std::uint8_t>(row.backend)));
        key.push_back('\n');
        key.append(std::to_string(static_cast<std::uint8_t>(row.feature)));
        if (std::ranges::find(seen, key) != seen.end()) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::duplicate_quality_row, row.backend, row.feature,
                           row.feature_id,
                           "renderer quality matrix allows one row per backend and required feature kind",
                           row.source_index);
            continue;
        }
        seen.push_back(std::move(key));
    }
}

void validate_required_feature_rows(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            continue;
        }
        for (const auto feature : kRequiredFeatures) {
            const auto has_feature_row = std::ranges::any_of(request.rows, [backend, feature](const auto& row) {
                return row.backend == backend && row.feature == feature;
            });
            if (!has_feature_row) {
                add_diagnostic(
                    plan, RendererQualityMatrixDiagnosticCode::missing_required_quality_row, backend, feature, {},
                    "each required backend needs local renderer quality evidence for every required feature", 0U);
            }
        }
    }
}

void validate_row_ids(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    for (const auto& row : request.rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_backend, row.backend, row.feature,
                           row.feature_id, "renderer quality matrix row backend must be required and supported",
                           row.source_index);
        }
        const auto has_valid_gate_id =
            row.status != RendererQualityRowStatus::dependency_gated ||
            (is_valid_id(row.dependency_gate_id) && !has_native_token(row.dependency_gate_id));
        const auto has_valid_unsupported_id =
            row.status != RendererQualityRowStatus::unsupported ||
            (is_valid_id(row.unsupported_claim_id) && !has_native_token(row.unsupported_claim_id));
        const auto has_valid_status_category =
            (row.status == RendererQualityRowStatus::ready && has_ready_evidence_categories(row)) ||
            (row.status == RendererQualityRowStatus::host_gated &&
             has_evidence_category(row, RendererQualityEvidenceCategory::host_gate)) ||
            (row.status == RendererQualityRowStatus::dependency_gated &&
             has_evidence_category(row, RendererQualityEvidenceCategory::dependency_gate)) ||
            (row.status == RendererQualityRowStatus::unsupported &&
             has_evidence_category(row, RendererQualityEvidenceCategory::unsupported_claim));
        const auto requires_package_counters = row.status != RendererQualityRowStatus::host_gated;
        if (!is_valid_id(row.feature_id) || has_native_token(row.feature_id) || !row.reviewed || !has_valid_gate_id ||
            !has_valid_unsupported_id || !has_valid_status_category ||
            (requires_package_counters && !package_counter_ids_ready(row))) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::invalid_quality_row, row.backend, row.feature,
                           row.feature_id,
                           "renderer quality matrix rows require reviewed backend-neutral ids and counter ids",
                           row.source_index);
        }
        if (has_native_token(row.notes)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_native_handle_claim, row.backend,
                           row.feature, row.feature_id,
                           "renderer quality matrix notes must not expose backend-native handles or tokens",
                           row.source_index);
        }
    }
}

void validate_unsupported_claims(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    for (const auto& row : request.rows) {
        if (row.request_native_handle_access) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_native_handle_claim, row.backend,
                           row.feature, row.feature_id,
                           "renderer quality matrix evidence must not expose native renderer or RHI handles",
                           row.source_index);
        }
        if (row.request_capture_execution) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_capture_execution, row.backend,
                           row.feature, row.feature_id,
                           "native GPU capture execution remains operator-owned and host-gated", row.source_index);
        }
        if (row.request_crash_upload_execution) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_crash_upload_execution, row.backend,
                           row.feature, row.feature_id,
                           "automatic crash upload execution remains outside renderer quality matrix planning",
                           row.source_index);
        }
        if (row.request_inferred_backend_parity) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_inferred_backend_parity, row.backend,
                           row.feature, row.feature_id,
                           "renderer backend parity must be proven by backend-local evidence rows", row.source_index);
        }
        if (row.request_subjective_visual_quality_claim) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::unsupported_subjective_visual_quality_claim,
                           row.backend, row.feature, row.feature_id,
                           "subjective visual quality claims require separate measured or reviewed evidence",
                           row.source_index);
        }
    }
}

void validate_evidence_rows(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    for (const auto& row : request.rows) {
        if (is_dependency_gated_row(row) || is_unsupported_row(row)) {
            if (!package_counter_ids_ready(row)) {
                add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_package_counter_evidence, row.backend,
                               row.feature, row.feature_id,
                               "non-ready renderer quality rows require package-visible status counters",
                               row.source_index);
            }
            continue;
        }

        if (!row.backend_local_evidence) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_backend_local_evidence, row.backend,
                           row.feature, row.feature_id,
                           "renderer quality evidence cannot be inferred from another backend", row.source_index);
        }

        if (is_host_gated_row(row)) {
            continue;
        }

        if (row.backend == rhi::BackendKind::metal && !row.host_validated) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_metal_host_evidence, row.backend,
                           row.feature, row.feature_id,
                           "Metal renderer quality rows require Apple host evidence or an explicit host gate",
                           row.source_index);
        }
        if (!resource_synchronization_ready(row)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_resource_synchronization_evidence,
                           row.backend, row.feature, row.feature_id,
                           "renderer quality rows require backend-local resource synchronization evidence",
                           row.source_index);
        }
        if (!backend_validation_ready(row)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_backend_validation_evidence, row.backend,
                           row.feature, row.feature_id,
                           "strict Vulkan rows require validation-layer and SPIR-V validation evidence",
                           row.source_index);
        }
        if (!row.shader_tool_validation_evidence) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_shader_tool_validation_evidence,
                           row.backend, row.feature, row.feature_id,
                           "renderer quality rows require shader or tool validation evidence", row.source_index);
        }
        if (!package_counter_ids_ready(row)) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_package_counter_evidence, row.backend,
                           row.feature, row.feature_id, "renderer quality rows require package-visible counter ids",
                           row.source_index);
        }
        if (row.timing_budget_us == 0U) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_timing_budget_evidence, row.backend,
                           row.feature, row.feature_id, "renderer quality rows require a bounded timing budget",
                           row.source_index);
        }
        if (!row.gpu_memory_evidence) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_gpu_memory_evidence, row.backend,
                           row.feature, row.feature_id, "renderer quality rows require GPU memory/residency evidence",
                           row.source_index);
        }
        if (!row.backend_parity_evidence) {
            add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::missing_backend_parity_evidence, row.backend,
                           row.feature, row.feature_id, "renderer quality rows require backend parity proof rows",
                           row.source_index);
        }
    }
}

void validate_budget(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RendererQualityMatrixDiagnosticCode::row_budget_exceeded, rhi::BackendKind::null,
                       RendererQualityFeatureKind::materials, {},
                       "renderer quality matrix request exceeds its row budget", 0U);
    }
}

void append_output_rows(RendererQualityMatrixPlan& plan, const RendererQualityMatrixRequest& request) {
    plan.required_backends = request.required_backends;
    sort_required_backends(plan.required_backends);
    plan.rows = request.rows;
    sort_rows(plan.rows);
    plan.row_count = plan.rows.size();
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

[[nodiscard]] std::uint64_t compute_replay_hash(const RendererQualityMatrixPlan& plan,
                                                const RendererQualityMatrixRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    for (const auto backend : plan.required_backends) {
        hash_mix(hash, static_cast<std::uint8_t>(backend));
    }
    for (const auto& row : plan.rows) {
        hash_string(hash, row.feature_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.feature));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.proof));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        for (const auto category : row.evidence_categories) {
            hash_mix(hash, static_cast<std::uint8_t>(category));
        }
        hash_string(hash, row.dependency_gate_id);
        hash_string(hash, row.unsupported_claim_id);
        hash_string(hash, row.notes);
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.backend_local_evidence ? 1U : 0U);
        hash_mix(hash, row.d3d12_resource_state_barrier_evidence ? 1U : 0U);
        hash_mix(hash, row.d3d12_fence_evidence ? 1U : 0U);
        hash_mix(hash, row.vulkan_synchronization2_evidence ? 1U : 0U);
        hash_mix(hash, row.vulkan_layout_transition_evidence ? 1U : 0U);
        hash_mix(hash, row.vulkan_validation_layer_evidence ? 1U : 0U);
        hash_mix(hash, row.vulkan_spirv_validation_evidence ? 1U : 0U);
        hash_mix(hash, row.metal_resource_synchronization_evidence ? 1U : 0U);
        hash_mix(hash, row.metal_feature_set_evidence ? 1U : 0U);
        hash_mix(hash, row.shader_tool_validation_evidence ? 1U : 0U);
        for (const auto& counter_id : row.package_counter_ids) {
            hash_string(hash, counter_id);
        }
        hash_mix(hash, row.timing_budget_us);
        hash_mix(hash, row.gpu_memory_evidence ? 1U : 0U);
        hash_mix(hash, row.backend_parity_evidence ? 1U : 0U);
        hash_mix(hash, row.host_validated ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool backend_all_required_rows_ready(const RendererQualityMatrixPlan& plan, rhi::BackendKind backend) {
    for (const auto feature : kRequiredFeatures) {
        const auto row_it = std::ranges::find_if(plan.rows, [backend, feature](const auto& row) {
            return row.backend == backend && row.feature == feature;
        });
        if (row_it == plan.rows.end() || !row_ready(*row_it)) {
            return false;
        }
    }
    return true;
}

void compute_readiness(RendererQualityMatrixPlan& plan) {
    std::vector<rhi::BackendKind> ready_backends;
    for (const auto& row : plan.rows) {
        if (row_ready(row)) {
            ++plan.ready_row_count;
        } else if (is_host_gated_row(row)) {
            ++plan.host_gated_row_count;
        } else if (is_dependency_gated_row(row)) {
            ++plan.dependency_gated_row_count;
        } else if (is_unsupported_row(row)) {
            ++plan.unsupported_row_count;
        }
    }

    for (const auto backend : plan.required_backends) {
        const auto backend_ready = backend_all_required_rows_ready(plan, backend);
        if (backend_ready && !contains_backend(ready_backends, backend)) {
            ready_backends.push_back(backend);
        }
        if (backend == rhi::BackendKind::d3d12) {
            plan.d3d12_quality_matrix_ready = backend_ready;
        } else if (backend == rhi::BackendKind::vulkan) {
            plan.vulkan_strict_quality_matrix_ready = backend_ready;
        } else if (backend == rhi::BackendKind::metal) {
            plan.requires_metal_host_evidence = true;
            plan.metal_quality_matrix_ready = backend_ready;
            plan.has_metal_host_evidence = backend_ready;
        }
    }
    plan.host_validated_backend_count = ready_backends.size();
    plan.selected_package_quality_evidence_ready =
        plan.d3d12_quality_matrix_ready && plan.vulkan_strict_quality_matrix_ready;
    plan.general_renderer_quality_ready = std::ranges::all_of(
        plan.required_backends, [&plan](const auto backend) { return backend_all_required_rows_ready(plan, backend); });
}

} // namespace

bool RendererQualityMatrixPlan::succeeded() const noexcept {
    return status == RendererQualityMatrixStatus::ready || status == RendererQualityMatrixStatus::no_rows;
}

RendererQualityMatrixPlan plan_renderer_quality_matrix(const RendererQualityMatrixRequest& request) {
    RendererQualityMatrixPlan plan;

    if (request_row_count(request) == 0U) {
        plan.status = RendererQualityMatrixStatus::no_rows;
        return plan;
    }

    validate_required_backends(plan, request);
    validate_duplicate_rows(plan, request);
    validate_required_feature_rows(plan, request);
    validate_row_ids(plan, request);
    validate_unsupported_claims(plan, request);
    validate_evidence_rows(plan, request);
    validate_budget(plan, request);

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RendererQualityMatrixStatus::invalid_request;
        return plan;
    }

    append_output_rows(plan, request);
    compute_readiness(plan);
    plan.replay_hash = compute_replay_hash(plan, request);
    if (plan.unsupported_row_count > 0U) {
        plan.status = RendererQualityMatrixStatus::unsupported;
    } else if (plan.dependency_gated_row_count > 0U) {
        plan.status = RendererQualityMatrixStatus::dependency_evidence_required;
    } else if (plan.requires_metal_host_evidence && !plan.has_metal_host_evidence) {
        plan.status = RendererQualityMatrixStatus::host_evidence_required;
    } else {
        plan.status = RendererQualityMatrixStatus::ready;
    }
    return plan;
}

} // namespace mirakana
