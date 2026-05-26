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
        .ime_begin_update_end = true,
        .ime_candidate_rows = true,
        .ime_text_area_rows = true,
        .ime_committed_text_rows = true,
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
        .accessibility_label_rows = true,
        .accessibility_state_rows = true,
        .accessibility_focus_rows = true,
        .accessibility_action_rows = true,
        .accessibility_relationship_rows = true,
        .accessibility_live_region_rows = true,
        .accessibility_os_publication_gate = true,
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

MK_TEST("runtime ui production stack rejects incomplete shaping evidence") {
    auto request = make_package_request();
    request.rows[0].glyph_clusters = false;
    request.rows[0].fallback_font_rows = false;
    request.rows[0].line_break_boundaries = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_glyph_clusters));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_fallback_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_shaping_line_breaks));
}

MK_TEST("runtime ui production stack rejects incomplete raster and atlas evidence") {
    auto request = make_package_request();
    request.rows[1].glyph_bitmap_rows = false;
    request.rows[2].atlas_budget_rows = false;
    request.rows[2].renderer_texture_upload_handoff = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_raster_glyph_bitmaps));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_atlas_budget_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_renderer_upload_handoff));
}

MK_TEST("runtime ui production stack rejects incomplete ime and accessibility evidence") {
    auto request = make_package_request();
    request.rows[4].ime_candidate_rows = false;
    request.rows[4].platform_adapter_dispatch_boundary = false;
    request.rows[5].accessibility_relationship_rows = false;
    request.rows[5].accessibility_live_region_rows = false;

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiProductionStackStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_ime_candidate_rows));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_platform_dispatch_boundary));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_relationships));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, RuntimeUiProductionDiagnosticCode::missing_accessibility_live_regions));
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
