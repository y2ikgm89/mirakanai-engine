// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_production_stack.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

constexpr std::uint64_t kFnvOffsetBasis{14695981039346656037ULL};
constexpr std::uint64_t kFnvPrime{1099511628211ULL};

void append_diagnostic(std::vector<RuntimeUiProductionDiagnostic>& diagnostics, RuntimeUiProductionDiagnosticCode code,
                       std::string row_id, std::string message) {
    diagnostics.push_back(RuntimeUiProductionDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool is_valid_feature(RuntimeUiProductionFeatureKind feature) noexcept {
    switch (feature) {
    case RuntimeUiProductionFeatureKind::text_shaping:
    case RuntimeUiProductionFeatureKind::font_rasterization:
    case RuntimeUiProductionFeatureKind::glyph_atlas:
    case RuntimeUiProductionFeatureKind::renderer_submission:
    case RuntimeUiProductionFeatureKind::ime:
    case RuntimeUiProductionFeatureKind::accessibility:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_proof(RuntimeUiProductionProofKind proof) noexcept {
    switch (proof) {
    case RuntimeUiProductionProofKind::first_party_contract:
    case RuntimeUiProductionProofKind::adapter_handoff:
    case RuntimeUiProductionProofKind::selected_package:
    case RuntimeUiProductionProofKind::host_gate:
    case RuntimeUiProductionProofKind::skipped:
        return true;
    }
    return false;
}

[[nodiscard]] std::string lowercase_ascii(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return result;
}

[[nodiscard]] bool is_token_character(char character) noexcept {
    return std::isalnum(static_cast<unsigned char>(character)) != 0;
}

[[nodiscard]] bool is_unsafe_native_token(std::string_view token) noexcept {
    constexpr std::array<std::string_view, 18> unsafe_tokens{
        "native", "backend", "rhi",  "gpu",  "d3d12",     "id3d12",   "vulkan",      "vk",    "metal",
        "mtl",    "sdl",     "sdl3", "hwnd", "hinstance", "nswindow", "caeagllayer", "imgui", "dearimgui",
    };
    return std::ranges::find(unsafe_tokens, token) != unsafe_tokens.end();
}

[[nodiscard]] bool has_unsafe_native_reference(std::string_view value) {
    const auto lower = lowercase_ascii(value);
    std::size_t token_begin = 0U;
    while (token_begin < lower.size()) {
        while (token_begin < lower.size() && !is_token_character(lower[token_begin])) {
            ++token_begin;
        }
        auto token_end = token_begin;
        while (token_end < lower.size() && is_token_character(lower[token_end])) {
            ++token_end;
        }
        if (token_begin < token_end &&
            is_unsafe_native_token(std::string_view{lower}.substr(token_begin, token_end - token_begin))) {
            return true;
        }
        token_begin = token_end;
    }
    return false;
}

[[nodiscard]] bool is_adapter_invoked(const RuntimeUiProductionEvidenceRow& row) noexcept {
    return row.invokes_adapter || row.invokes_native_platform || row.invokes_renderer_upload;
}

[[nodiscard]] bool is_unsupported_claim(const RuntimeUiProductionEvidenceRow& row) {
    return row.uses_public_native_handle || has_unsafe_native_reference(row.id) || row.uses_ui_middleware_api ||
           row.claims_general_production_text_stack || row.claims_broad_platform_ui_parity;
}

void record_adapter_invocation(RuntimeUiProductionStackPlan& plan, const RuntimeUiProductionEvidenceRow& row) noexcept {
    if (!is_adapter_invoked(row)) {
        return;
    }

    ++plan.adapter_invoked_rows;
    if (row.invokes_native_platform) {
        plan.invoked_native_platform = true;
    }
    if (row.invokes_renderer_upload) {
        plan.invoked_renderer_upload = true;
    }
    if (!row.invokes_adapter) {
        return;
    }

    switch (row.feature) {
    case RuntimeUiProductionFeatureKind::text_shaping:
        plan.invoked_text_shaping = true;
        break;
    case RuntimeUiProductionFeatureKind::font_rasterization:
        plan.invoked_font_rasterization = true;
        break;
    case RuntimeUiProductionFeatureKind::glyph_atlas:
    case RuntimeUiProductionFeatureKind::renderer_submission:
        plan.invoked_renderer_upload = true;
        break;
    case RuntimeUiProductionFeatureKind::ime:
        plan.invoked_ime_adapter = true;
        break;
    case RuntimeUiProductionFeatureKind::accessibility:
        plan.invoked_accessibility_bridge = true;
        break;
    }
}

void require_flag(std::vector<RuntimeUiProductionDiagnostic>& diagnostics, bool value,
                  RuntimeUiProductionDiagnosticCode code, std::string_view row_id, std::string_view message) {
    if (!value) {
        append_diagnostic(diagnostics, code, std::string{row_id}, std::string{message});
    }
}

void validate_text_shaping_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                               const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.request_validation, RuntimeUiProductionDiagnosticCode::missing_request_validation,
                 row.id, "runtime UI text shaping evidence requires strict request validation");
    require_flag(diagnostics, row.shaping_segments, RuntimeUiProductionDiagnosticCode::missing_shaping_segments, row.id,
                 "runtime UI text shaping evidence requires shaping segment rows");
    require_flag(diagnostics, row.shaping_direction_script_language,
                 RuntimeUiProductionDiagnosticCode::missing_shaping_direction_script_language, row.id,
                 "runtime UI text shaping evidence requires resolved direction, script, and language rows");
    require_flag(diagnostics, row.glyph_clusters, RuntimeUiProductionDiagnosticCode::missing_shaping_glyph_clusters,
                 row.id, "runtime UI text shaping evidence requires glyph cluster rows");
    require_flag(diagnostics, row.glyph_advances_offsets,
                 RuntimeUiProductionDiagnosticCode::missing_shaping_advances_offsets, row.id,
                 "runtime UI text shaping evidence requires glyph advance and offset rows");
    require_flag(diagnostics, row.fallback_font_rows, RuntimeUiProductionDiagnosticCode::missing_shaping_fallback_rows,
                 row.id, "runtime UI text shaping evidence requires fallback font rows");
    require_flag(diagnostics, row.bidi_boundaries, RuntimeUiProductionDiagnosticCode::missing_shaping_bidi_boundaries,
                 row.id, "runtime UI text shaping evidence requires bidi boundary rows");
    require_flag(diagnostics, row.line_break_boundaries, RuntimeUiProductionDiagnosticCode::missing_shaping_line_breaks,
                 row.id, "runtime UI text shaping evidence requires line-break boundary rows");
}

void validate_font_rasterization_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                                     const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.request_validation, RuntimeUiProductionDiagnosticCode::missing_request_validation,
                 row.id, "runtime UI font rasterization evidence requires strict request validation");
    require_flag(diagnostics, row.glyph_bitmap_rows, RuntimeUiProductionDiagnosticCode::missing_raster_glyph_bitmaps,
                 row.id, "runtime UI font rasterization evidence requires glyph bitmap rows");
    require_flag(diagnostics, row.glyph_pixel_format_rows,
                 RuntimeUiProductionDiagnosticCode::missing_raster_pixel_format_rows, row.id,
                 "runtime UI font rasterization evidence requires glyph pixel-format rows");
    require_flag(diagnostics, row.glyph_metric_rows, RuntimeUiProductionDiagnosticCode::missing_raster_glyph_metrics,
                 row.id, "runtime UI font rasterization evidence requires glyph metric rows");
}

void validate_glyph_atlas_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                              const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.atlas_placement_rows, RuntimeUiProductionDiagnosticCode::missing_atlas_placement_rows,
                 row.id, "runtime UI glyph atlas evidence requires placement rows");
    require_flag(diagnostics, row.atlas_budget_rows, RuntimeUiProductionDiagnosticCode::missing_atlas_budget_rows,
                 row.id, "runtime UI glyph atlas evidence requires budget rows");
    require_flag(diagnostics, row.atlas_eviction_diagnostics,
                 RuntimeUiProductionDiagnosticCode::missing_atlas_eviction_diagnostics, row.id,
                 "runtime UI glyph atlas evidence requires eviction diagnostics");
    require_flag(diagnostics, row.renderer_texture_upload_handoff,
                 RuntimeUiProductionDiagnosticCode::missing_renderer_upload_handoff, row.id,
                 "runtime UI glyph atlas evidence requires renderer texture upload handoff rows");
}

void validate_renderer_submission_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                                      const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.renderer_texture_upload_handoff,
                 RuntimeUiProductionDiagnosticCode::missing_renderer_upload_handoff, row.id,
                 "runtime UI renderer submission evidence requires renderer texture upload handoff rows");
    require_flag(diagnostics, row.selected_package_counter_evidence,
                 RuntimeUiProductionDiagnosticCode::missing_selected_package_counter_evidence, row.id,
                 "runtime UI renderer submission evidence requires selected package counter evidence");
}

void validate_ime_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                      const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.ime_begin_update_end, RuntimeUiProductionDiagnosticCode::missing_ime_begin_update_end,
                 row.id, "runtime UI IME evidence requires begin, update, and end rows");
    require_flag(diagnostics, row.ime_candidate_rows, RuntimeUiProductionDiagnosticCode::missing_ime_candidate_rows,
                 row.id, "runtime UI IME evidence requires candidate rows");
    require_flag(diagnostics, row.ime_text_area_rows, RuntimeUiProductionDiagnosticCode::missing_ime_text_area_rows,
                 row.id, "runtime UI IME evidence requires text input area and cursor rows");
    require_flag(diagnostics, row.ime_committed_text_rows,
                 RuntimeUiProductionDiagnosticCode::missing_ime_committed_text_rows, row.id,
                 "runtime UI IME evidence requires committed text rows");
    require_flag(diagnostics, row.platform_adapter_dispatch_boundary,
                 RuntimeUiProductionDiagnosticCode::missing_platform_dispatch_boundary, row.id,
                 "runtime UI IME evidence requires platform adapter dispatch boundary rows");
}

void validate_accessibility_row(std::vector<RuntimeUiProductionDiagnostic>& diagnostics,
                                const RuntimeUiProductionEvidenceRow& row) {
    require_flag(diagnostics, row.accessibility_role_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_roles, row.id,
                 "runtime UI accessibility evidence requires role rows");
    require_flag(diagnostics, row.accessibility_label_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_labels, row.id,
                 "runtime UI accessibility evidence requires label rows");
    require_flag(diagnostics, row.accessibility_state_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_states, row.id,
                 "runtime UI accessibility evidence requires state rows");
    require_flag(diagnostics, row.accessibility_focus_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_focus, row.id,
                 "runtime UI accessibility evidence requires focus rows");
    require_flag(diagnostics, row.accessibility_action_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_actions, row.id,
                 "runtime UI accessibility evidence requires action rows");
    require_flag(diagnostics, row.accessibility_relationship_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_relationships, row.id,
                 "runtime UI accessibility evidence requires relationship rows");
    require_flag(diagnostics, row.accessibility_live_region_rows,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_live_regions, row.id,
                 "runtime UI accessibility evidence requires live-region update rows");
    require_flag(diagnostics, row.accessibility_os_publication_gate,
                 RuntimeUiProductionDiagnosticCode::missing_accessibility_os_publication_gate, row.id,
                 "runtime UI accessibility evidence requires an OS publication host gate");
}

void hash_byte(std::uint64_t& hash, std::uint8_t value) noexcept {
    hash ^= value;
    hash *= kFnvPrime;
}

void hash_bool(std::uint64_t& hash, bool value) noexcept {
    hash_byte(hash, value ? 1U : 0U);
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const unsigned char character : value) {
        hash_byte(hash, character);
    }
    hash_byte(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeUiProductionStackRequest& request) noexcept {
    auto hash = kFnvOffsetBasis;
    hash_string(hash, request.id);
    for (const auto& row : request.rows) {
        hash_string(hash, row.id);
        hash_byte(hash, static_cast<std::uint8_t>(row.feature));
        hash_byte(hash, static_cast<std::uint8_t>(row.proof));
        hash_bool(hash, row.host_evidence_required);
        hash_bool(hash, row.host_evidence_available);
        hash_bool(hash, row.request_validation);
        hash_bool(hash, row.shaping_segments);
        hash_bool(hash, row.shaping_direction_script_language);
        hash_bool(hash, row.glyph_clusters);
        hash_bool(hash, row.glyph_advances_offsets);
        hash_bool(hash, row.fallback_font_rows);
        hash_bool(hash, row.bidi_boundaries);
        hash_bool(hash, row.line_break_boundaries);
        hash_bool(hash, row.glyph_bitmap_rows);
        hash_bool(hash, row.glyph_pixel_format_rows);
        hash_bool(hash, row.glyph_metric_rows);
        hash_bool(hash, row.atlas_placement_rows);
        hash_bool(hash, row.atlas_budget_rows);
        hash_bool(hash, row.atlas_eviction_diagnostics);
        hash_bool(hash, row.renderer_texture_upload_handoff);
        hash_bool(hash, row.selected_package_counter_evidence);
        hash_bool(hash, row.ime_begin_update_end);
        hash_bool(hash, row.ime_candidate_rows);
        hash_bool(hash, row.ime_text_area_rows);
        hash_bool(hash, row.ime_committed_text_rows);
        hash_bool(hash, row.platform_adapter_dispatch_boundary);
        hash_bool(hash, row.accessibility_role_rows);
        hash_bool(hash, row.accessibility_label_rows);
        hash_bool(hash, row.accessibility_state_rows);
        hash_bool(hash, row.accessibility_focus_rows);
        hash_bool(hash, row.accessibility_action_rows);
        hash_bool(hash, row.accessibility_relationship_rows);
        hash_bool(hash, row.accessibility_live_region_rows);
        hash_bool(hash, row.accessibility_os_publication_gate);
        hash_bool(hash, row.requires_optional_dependency_adapter);
        hash_bool(hash, row.dependency_adapter_reviewed);
        hash_bool(hash, row.uses_public_native_handle);
        hash_bool(hash, row.uses_ui_middleware_api);
        hash_bool(hash, row.claims_general_production_text_stack);
        hash_bool(hash, row.claims_broad_platform_ui_parity);
        hash_bool(hash, row.invokes_adapter);
        hash_bool(hash, row.invokes_native_platform);
        hash_bool(hash, row.invokes_renderer_upload);
    }
    return hash == 0U ? 1U : hash;
}

void append_missing_feature_diagnostics(RuntimeUiProductionStackPlan& plan, std::array<bool, 6>& seen) {
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::text_shaping)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_text_shaping_row, {},
                          "runtime UI production stack requires a text shaping evidence row");
    }
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::font_rasterization)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_font_rasterization_row, {},
                          "runtime UI production stack requires a font rasterization evidence row");
    }
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::glyph_atlas)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_glyph_atlas_row, {},
                          "runtime UI production stack requires a glyph atlas evidence row");
    }
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::renderer_submission)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_renderer_submission_row, {},
                          "runtime UI production stack requires a renderer submission evidence row");
    }
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::ime)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_row, {},
                          "runtime UI production stack requires an IME evidence row");
    }
    if (!seen[static_cast<std::size_t>(RuntimeUiProductionFeatureKind::accessibility)]) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_row, {},
                          "runtime UI production stack requires an accessibility evidence row");
    }
}

} // namespace

bool RuntimeUiProductionStackPlan::ready() const noexcept {
    return status == RuntimeUiProductionStackStatus::ready && production_runtime_ui_ready && diagnostics.empty();
}

std::string_view runtime_ui_production_stack_status_name(RuntimeUiProductionStackStatus status) noexcept {
    switch (status) {
    case RuntimeUiProductionStackStatus::ready:
        return "ready";
    case RuntimeUiProductionStackStatus::host_evidence_required:
        return "host_evidence_required";
    case RuntimeUiProductionStackStatus::dependency_evidence_required:
        return "dependency_evidence_required";
    case RuntimeUiProductionStackStatus::evidence_skipped:
        return "evidence_skipped";
    case RuntimeUiProductionStackStatus::no_rows:
        return "no_rows";
    case RuntimeUiProductionStackStatus::invalid_request:
        return "invalid_request";
    }
    return "invalid_request";
}

RuntimeUiProductionStackPlan plan_runtime_ui_production_stack(const RuntimeUiProductionStackRequest& request) {
    RuntimeUiProductionStackPlan plan;
    plan.replay_hash = compute_replay_hash(request);

    if (request.id.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_request_id, {},
                          "runtime UI production stack request id must not be empty");
    }
    if (request.rows.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::no_rows, {},
                          "runtime UI production stack requires evidence rows");
        plan.status = RuntimeUiProductionStackStatus::no_rows;
        return plan;
    }
    if (request.row_budget == 0U || request.rows.size() > request.row_budget) {
        append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::row_budget_exceeded, {},
                          "runtime UI production stack rows exceed the request budget");
    }

    std::vector<std::string> row_ids;
    row_ids.reserve(request.rows.size());
    std::array<bool, 6> seen_features{};

    for (const auto& row : request.rows) {
        if (row.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_row_id, {},
                              "runtime UI production evidence row id must not be empty");
        } else if (contains_id(row_ids, row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::duplicate_row_id, row.id,
                              "runtime UI production evidence row ids must be unique");
        } else {
            row_ids.push_back(row.id);
        }

        if (!is_valid_feature(row.feature)) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_feature, row.id,
                              "runtime UI production evidence row has an unsupported feature kind");
            continue;
        }
        if (!is_valid_proof(row.proof)) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_proof_kind, row.id,
                              "runtime UI production evidence row has an unsupported proof kind");
        }

        seen_features[static_cast<std::size_t>(row.feature)] = true;
        const bool skipped = row.proof == RuntimeUiProductionProofKind::skipped;
        const bool dependency_gated = row.requires_optional_dependency_adapter;
        if (skipped) {
            ++plan.skipped_rows;
        }
        if (dependency_gated) {
            ++plan.dependency_gated_rows;
        }
        if (is_unsupported_claim(row)) {
            ++plan.unsupported_rows;
        }
        record_adapter_invocation(plan, row);

        if (row.uses_public_native_handle || has_unsafe_native_reference(row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_native_handle, row.id,
                              "runtime UI production evidence must not expose native, backend, GPU, or RHI handles");
        }
        if (row.uses_ui_middleware_api) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_ui_middleware_api,
                              row.id, "runtime UI production evidence must stay on first-party UI contracts");
        }
        if (row.claims_general_production_text_stack || row.claims_broad_platform_ui_parity) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_broad_production_claim,
                              row.id,
                              "runtime UI production evidence must not claim broad production text or platform parity");
        }
        if (row.requires_optional_dependency_adapter && !row.dependency_adapter_reviewed) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unreviewed_dependency_adapter,
                              row.id, "runtime UI optional dependency adapters require dependency and license review");
        }
        if (row.invokes_adapter || row.invokes_native_platform || row.invokes_renderer_upload) {
            append_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::side_effect_invocation, row.id,
                              "runtime UI production evidence planning must not invoke adapters or host services");
        }

        if (skipped || dependency_gated) {
            continue;
        }

        switch (row.feature) {
        case RuntimeUiProductionFeatureKind::text_shaping:
            validate_text_shaping_row(plan.diagnostics, row);
            break;
        case RuntimeUiProductionFeatureKind::font_rasterization:
            validate_font_rasterization_row(plan.diagnostics, row);
            break;
        case RuntimeUiProductionFeatureKind::glyph_atlas:
            validate_glyph_atlas_row(plan.diagnostics, row);
            break;
        case RuntimeUiProductionFeatureKind::renderer_submission:
            validate_renderer_submission_row(plan.diagnostics, row);
            break;
        case RuntimeUiProductionFeatureKind::ime:
            validate_ime_row(plan.diagnostics, row);
            break;
        case RuntimeUiProductionFeatureKind::accessibility:
            validate_accessibility_row(plan.diagnostics, row);
            break;
        }
    }

    append_missing_feature_diagnostics(plan, seen_features);

    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeUiProductionStackStatus::invalid_request;
        return plan;
    }

    plan.rows = request.rows;
    plan.reviewed = true;
    plan.text_stack_contract_ready = true;
    plan.selected_package_counter_evidence_ready = true;

    for (const auto& row : plan.rows) {
        if (row.proof == RuntimeUiProductionProofKind::skipped || row.requires_optional_dependency_adapter) {
            continue;
        }
        if (row.host_evidence_required && !row.host_evidence_available) {
            ++plan.host_gated_rows;
        } else {
            ++plan.ready_rows;
        }

        if (row.feature == RuntimeUiProductionFeatureKind::renderer_submission) {
            plan.selected_package_counter_evidence_ready =
                row.selected_package_counter_evidence && row.proof == RuntimeUiProductionProofKind::selected_package;
        }
        if (row.feature == RuntimeUiProductionFeatureKind::ime) {
            plan.requires_ime_host_evidence = row.host_evidence_required;
            plan.ime_host_evidence_available = !row.host_evidence_required || row.host_evidence_available;
        }
        if (row.feature == RuntimeUiProductionFeatureKind::accessibility) {
            plan.requires_accessibility_host_evidence = row.host_evidence_required;
            plan.accessibility_host_evidence_available = !row.host_evidence_required || row.host_evidence_available;
        }
    }

    plan.production_runtime_ui_ready = plan.text_stack_contract_ready && plan.selected_package_counter_evidence_ready &&
                                       plan.host_gated_rows == 0U && plan.dependency_gated_rows == 0U &&
                                       plan.skipped_rows == 0U && plan.ready_rows == plan.rows.size();
    if (plan.production_runtime_ui_ready) {
        plan.status = RuntimeUiProductionStackStatus::ready;
    } else if (plan.host_gated_rows > 0U) {
        plan.status = RuntimeUiProductionStackStatus::host_evidence_required;
    } else if (plan.dependency_gated_rows > 0U) {
        plan.status = RuntimeUiProductionStackStatus::dependency_evidence_required;
    } else {
        plan.status = RuntimeUiProductionStackStatus::evidence_skipped;
    }
    return plan;
}

} // namespace mirakana::ui
