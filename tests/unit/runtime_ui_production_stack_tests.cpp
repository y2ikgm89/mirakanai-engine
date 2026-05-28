// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_production_stack.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace {

using mirakana::ui::RuntimeUiProductionDiagnosticCode;
using mirakana::ui::RuntimeUiProductionEvidenceRow;
using mirakana::ui::RuntimeUiProductionFeatureKind;
using mirakana::ui::RuntimeUiProductionProofKind;
using mirakana::ui::RuntimeUiProductionStackRequest;

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::ui::RuntimeUiProductionDiagnostic>& diagnostics,
                                  RuntimeUiProductionDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_text_shaping_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.text-shaping",
        .feature = RuntimeUiProductionFeatureKind::text_shaping,
        .proof = RuntimeUiProductionProofKind::first_party_contract,
        .host_evidence_required = false,
        .host_evidence_available = false,
        .request_validation = true,
        .shaping_segments = true,
        .shaping_direction_script_language = true,
        .glyph_clusters = true,
        .glyph_advances_offsets = true,
        .fallback_font_rows = true,
        .bidi_boundaries = true,
        .line_break_boundaries = true,
    };
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_font_rasterization_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.font-rasterization",
        .feature = RuntimeUiProductionFeatureKind::font_rasterization,
        .proof = RuntimeUiProductionProofKind::adapter_handoff,
        .host_evidence_required = false,
        .host_evidence_available = false,
        .request_validation = true,
        .glyph_bitmap_rows = true,
        .glyph_pixel_format_rows = true,
        .glyph_metric_rows = true,
    };
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_glyph_atlas_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.glyph-atlas",
        .feature = RuntimeUiProductionFeatureKind::glyph_atlas,
        .proof = RuntimeUiProductionProofKind::adapter_handoff,
        .host_evidence_required = false,
        .host_evidence_available = false,
        .atlas_placement_rows = true,
        .atlas_budget_rows = true,
        .atlas_eviction_diagnostics = true,
        .renderer_texture_upload_handoff = true,
    };
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_renderer_submission_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.renderer-submission",
        .feature = RuntimeUiProductionFeatureKind::renderer_submission,
        .proof = RuntimeUiProductionProofKind::selected_package,
        .host_evidence_required = false,
        .host_evidence_available = false,
        .renderer_texture_upload_handoff = true,
        .selected_package_counter_evidence = true,
    };
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_ime_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.ime",
        .feature = RuntimeUiProductionFeatureKind::ime,
        .proof = RuntimeUiProductionProofKind::host_gate,
        .host_evidence_required = true,
        .host_evidence_available = false,
        .ime_session_begin_end_rows = true,
        .ime_composition_update_rows = true,
        .ime_candidate_rows = true,
        .ime_text_area_cursor_rows = true,
        .ime_committed_text_rows = true,
        .ime_clipboard_rows = true,
        .ime_win32_adapter_proof_rows = true,
        .ime_platform_host_gate_rows = true,
        .platform_adapter_dispatch_boundary = true,
    };
}

[[nodiscard]] RuntimeUiProductionEvidenceRow make_accessibility_row() {
    return RuntimeUiProductionEvidenceRow{
        .id = "runtime-ui.accessibility",
        .feature = RuntimeUiProductionFeatureKind::accessibility,
        .proof = RuntimeUiProductionProofKind::host_gate,
        .host_evidence_required = true,
        .host_evidence_available = false,
        .accessibility_role_rows = true,
        .accessibility_name_rows = true,
        .accessibility_description_rows = true,
        .accessibility_state_rows = true,
        .accessibility_focus_rows = true,
        .accessibility_action_rows = true,
        .accessibility_relationship_rows = true,
        .accessibility_live_region_rows = true,
        .accessibility_keyboard_pattern_rows = true,
        .accessibility_publication_status_rows = true,
        .accessibility_uia_host_gate_rows = true,
        .accessibility_platform_host_gate_rows = true,
    };
}

[[nodiscard]] RuntimeUiProductionStackRequest make_package_request() {
    RuntimeUiProductionStackRequest request;
    request.id = "sample-2d-runtime-ui-production-stack";
    request.rows = {
        make_text_shaping_row(), make_font_rasterization_row(), make_glyph_atlas_row(), make_renderer_submission_row(),
        make_ime_row(),          make_accessibility_row(),
    };
    return request;
}

} // namespace

MK_TEST("runtime ui production stack reports host gated selected package evidence") {
    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(make_package_request());

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::host_evidence_required);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.reviewed);
    MK_REQUIRE(plan.rows.size() == 6U);
    MK_REQUIRE(plan.ready_rows == 4U);
    MK_REQUIRE(plan.host_gated_rows == 2U);
    MK_REQUIRE(plan.text_stack_contract_ready);
    MK_REQUIRE(plan.selected_package_counter_evidence_ready);
    MK_REQUIRE(!plan.production_runtime_ui_ready);
    MK_REQUIRE(plan.requires_ime_host_evidence);
    MK_REQUIRE(!plan.ime_host_evidence_available);
    MK_REQUIRE(plan.requires_accessibility_host_evidence);
    MK_REQUIRE(!plan.accessibility_host_evidence_available);
    MK_REQUIRE(!plan.invoked_text_shaping);
    MK_REQUIRE(!plan.invoked_font_rasterization);
    MK_REQUIRE(!plan.invoked_renderer_upload);
    MK_REQUIRE(!plan.invoked_ime_adapter);
    MK_REQUIRE(!plan.invoked_accessibility_bridge);
    MK_REQUIRE(!plan.invoked_native_platform);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui production stack becomes ready only when host evidence is present") {
    auto request = make_package_request();
    for (auto& row : request.rows) {
        if (row.host_evidence_required) {
            row.host_evidence_available = true;
        }
    }

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::ready);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.ready_rows == 6U);
    MK_REQUIRE(plan.host_gated_rows == 0U);
    MK_REQUIRE(plan.production_runtime_ui_ready);
    MK_REQUIRE(plan.ime_host_evidence_available);
    MK_REQUIRE(plan.accessibility_host_evidence_available);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui production stack distinguishes dependency gated and skipped rows") {
    auto request = make_package_request();
    for (auto& row : request.rows) {
        if (row.host_evidence_required) {
            row.host_evidence_available = true;
        }
    }
    request.rows[0].requires_optional_dependency_adapter = true;
    request.rows[0].dependency_adapter_reviewed = true;
    request.rows[0].glyph_clusters = false;
    request.rows[0].glyph_advances_offsets = false;
    request.rows[1].proof = RuntimeUiProductionProofKind::skipped;
    request.rows[1].glyph_bitmap_rows = false;
    request.rows[1].glyph_metric_rows = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::dependency_evidence_required);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.reviewed);
    MK_REQUIRE(plan.ready_rows == 4U);
    MK_REQUIRE(plan.host_gated_rows == 0U);
    MK_REQUIRE(plan.dependency_gated_rows == 1U);
    MK_REQUIRE(plan.skipped_rows == 1U);
    MK_REQUIRE(plan.adapter_invoked_rows == 0U);
    MK_REQUIRE(plan.unsupported_rows == 0U);
    MK_REQUIRE(!plan.production_runtime_ui_ready);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui production stack rejects incomplete shaping evidence") {
    auto request = make_package_request();
    request.rows[0].glyph_clusters = false;
    request.rows[0].shaping_direction_script_language = false;
    request.rows[0].fallback_font_rows = false;
    request.rows[0].line_break_boundaries = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_direction_script_language));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_glyph_clusters));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_fallback_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_line_breaks));
}

MK_TEST("runtime ui production stack rejects incomplete raster and atlas evidence") {
    auto request = make_package_request();
    request.rows[1].glyph_bitmap_rows = false;
    request.rows[1].glyph_pixel_format_rows = false;
    request.rows[2].atlas_budget_rows = false;
    request.rows[2].renderer_texture_upload_handoff = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_raster_glyph_bitmaps));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_raster_pixel_format_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_atlas_budget_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_renderer_upload_handoff));
}

MK_TEST("runtime ui production stack rejects incomplete ime and accessibility evidence") {
    auto request = make_package_request();
    request.rows[4].ime_session_begin_end_rows = false;
    request.rows[4].ime_composition_update_rows = false;
    request.rows[4].ime_candidate_rows = false;
    request.rows[4].ime_text_area_cursor_rows = false;
    request.rows[4].ime_clipboard_rows = false;
    request.rows[4].ime_win32_adapter_proof_rows = false;
    request.rows[4].ime_platform_host_gate_rows = false;
    request.rows[4].platform_adapter_dispatch_boundary = false;
    request.rows[5].accessibility_name_rows = false;
    request.rows[5].accessibility_description_rows = false;
    request.rows[5].accessibility_relationship_rows = false;
    request.rows[5].accessibility_live_region_rows = false;
    request.rows[5].accessibility_keyboard_pattern_rows = false;
    request.rows[5].accessibility_publication_status_rows = false;
    request.rows[5].accessibility_uia_host_gate_rows = false;
    request.rows[5].accessibility_platform_host_gate_rows = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_session_rows));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_composition_update_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_candidate_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_text_area_cursor_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_clipboard_rows));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_win32_adapter_proof_rows));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_platform_host_gate_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_platform_dispatch_boundary));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_relationships));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_live_regions));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_names));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_descriptions));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_keyboard_patterns));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_publication_status));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_uia_host_gate));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_platform_host_gate));
}

MK_TEST("runtime ui production stack keeps selected accessibility proof host gated") {
    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(make_package_request());

    const auto accessibility_row = std::ranges::find_if(plan.rows, [](const RuntimeUiProductionEvidenceRow& row) {
        return row.feature == RuntimeUiProductionFeatureKind::accessibility;
    });

    MK_REQUIRE(accessibility_row != plan.rows.end());
    MK_REQUIRE(accessibility_row->accessibility_role_rows);
    MK_REQUIRE(accessibility_row->accessibility_name_rows);
    MK_REQUIRE(accessibility_row->accessibility_description_rows);
    MK_REQUIRE(accessibility_row->accessibility_state_rows);
    MK_REQUIRE(accessibility_row->accessibility_focus_rows);
    MK_REQUIRE(accessibility_row->accessibility_action_rows);
    MK_REQUIRE(accessibility_row->accessibility_relationship_rows);
    MK_REQUIRE(accessibility_row->accessibility_live_region_rows);
    MK_REQUIRE(accessibility_row->accessibility_keyboard_pattern_rows);
    MK_REQUIRE(accessibility_row->accessibility_publication_status_rows);
    MK_REQUIRE(accessibility_row->accessibility_uia_host_gate_rows);
    MK_REQUIRE(accessibility_row->accessibility_platform_host_gate_rows);
    MK_REQUIRE(accessibility_row->host_evidence_required);
    MK_REQUIRE(!accessibility_row->host_evidence_available);
    MK_REQUIRE(!accessibility_row->claims_broad_platform_ui_parity);
    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::host_evidence_required);
    MK_REQUIRE(plan.requires_accessibility_host_evidence);
    MK_REQUIRE(!plan.accessibility_host_evidence_available);
    MK_REQUIRE(!plan.production_runtime_ui_ready);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui production stack keeps selected win32 ime proof host gated") {
    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(make_package_request());

    const auto ime_row = std::ranges::find_if(plan.rows, [](const RuntimeUiProductionEvidenceRow& row) {
        return row.feature == RuntimeUiProductionFeatureKind::ime;
    });

    MK_REQUIRE(ime_row != plan.rows.end());
    MK_REQUIRE(ime_row->ime_session_begin_end_rows);
    MK_REQUIRE(ime_row->ime_composition_update_rows);
    MK_REQUIRE(ime_row->ime_candidate_rows);
    MK_REQUIRE(ime_row->ime_text_area_cursor_rows);
    MK_REQUIRE(ime_row->ime_committed_text_rows);
    MK_REQUIRE(ime_row->ime_clipboard_rows);
    MK_REQUIRE(ime_row->ime_win32_adapter_proof_rows);
    MK_REQUIRE(ime_row->ime_platform_host_gate_rows);
    MK_REQUIRE(ime_row->platform_adapter_dispatch_boundary);
    MK_REQUIRE(ime_row->host_evidence_required);
    MK_REQUIRE(!ime_row->host_evidence_available);
    MK_REQUIRE(!ime_row->claims_broad_platform_ui_parity);
    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::host_evidence_required);
    MK_REQUIRE(plan.requires_ime_host_evidence);
    MK_REQUIRE(!plan.ime_host_evidence_available);
    MK_REQUIRE(!plan.production_runtime_ui_ready);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui production stack rejects duplicate missing and unsafe claim rows") {
    auto request = make_package_request();
    request.rows.push_back(request.rows.front());
    request.rows[0].id = "runtime-ui.native.ID3D12Resource";
    request.rows[1].claims_general_production_text_stack = true;
    request.rows[2].uses_ui_middleware_api = true;
    request.rows[3].uses_public_native_handle = true;
    request.rows[4].invokes_adapter = true;
    request.rows[5].dependency_adapter_reviewed = false;
    request.rows[5].requires_optional_dependency_adapter = true;
    request.rows.back().id = request.rows[1].id;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(plan.dependency_gated_rows == 1U);
    MK_REQUIRE(plan.adapter_invoked_rows == 1U);
    MK_REQUIRE(plan.unsupported_rows == 4U);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::duplicate_row_id));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_native_handle));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_broad_production_claim));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unsupported_ui_middleware_api));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::side_effect_invocation));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::unreviewed_dependency_adapter));
}

MK_TEST("runtime ui production stack fails closed for missing feature rows") {
    RuntimeUiProductionStackRequest request;
    request.id = "empty-production-stack";

    const auto empty_plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(empty_plan.status == mirakana::ui::RuntimeUiProductionStackStatus::no_rows);
    MK_REQUIRE(has_diagnostic(empty_plan.diagnostics, RuntimeUiProductionDiagnosticCode::no_rows));

    request = make_package_request();
    request.rows.erase(request.rows.begin() + 4);

    const auto missing_plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(missing_plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(missing_plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_row));
}

int main() {
    return mirakana::test::run_all();
}
