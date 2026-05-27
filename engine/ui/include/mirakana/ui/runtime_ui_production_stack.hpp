// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiProductionStackStatus : std::uint8_t {
    ready,
    host_evidence_required,
    dependency_evidence_required,
    evidence_skipped,
    no_rows,
    invalid_request,
};

enum class RuntimeUiProductionFeatureKind : std::uint8_t {
    text_shaping,
    font_rasterization,
    glyph_atlas,
    renderer_submission,
    ime,
    accessibility,
};

enum class RuntimeUiProductionProofKind : std::uint8_t {
    first_party_contract,
    adapter_handoff,
    selected_package,
    host_gate,
    skipped,
};

enum class RuntimeUiProductionDiagnosticCode : std::uint8_t {
    no_rows,
    missing_request_id,
    missing_row_id,
    duplicate_row_id,
    unsupported_feature,
    unsupported_proof_kind,
    missing_text_shaping_row,
    missing_font_rasterization_row,
    missing_glyph_atlas_row,
    missing_renderer_submission_row,
    missing_ime_row,
    missing_accessibility_row,
    missing_request_validation,
    missing_shaping_segments,
    missing_shaping_direction_script_language,
    missing_shaping_glyph_clusters,
    missing_shaping_advances_offsets,
    missing_shaping_fallback_rows,
    missing_shaping_bidi_boundaries,
    missing_shaping_line_breaks,
    missing_raster_glyph_bitmaps,
    missing_raster_pixel_format_rows,
    missing_raster_glyph_metrics,
    missing_atlas_placement_rows,
    missing_atlas_budget_rows,
    missing_atlas_eviction_diagnostics,
    missing_renderer_upload_handoff,
    missing_selected_package_counter_evidence,
    missing_ime_begin_update_end,
    missing_ime_candidate_rows,
    missing_ime_text_area_rows,
    missing_ime_committed_text_rows,
    missing_platform_dispatch_boundary,
    missing_accessibility_roles,
    missing_accessibility_labels,
    missing_accessibility_states,
    missing_accessibility_focus,
    missing_accessibility_actions,
    missing_accessibility_relationships,
    missing_accessibility_live_regions,
    missing_accessibility_os_publication_gate,
    unsupported_native_handle,
    unsupported_ui_middleware_api,
    unsupported_broad_production_claim,
    unreviewed_dependency_adapter,
    side_effect_invocation,
    row_budget_exceeded,
};

struct RuntimeUiProductionEvidenceRow {
    std::string id;
    RuntimeUiProductionFeatureKind feature{RuntimeUiProductionFeatureKind::text_shaping};
    RuntimeUiProductionProofKind proof{RuntimeUiProductionProofKind::first_party_contract};
    bool host_evidence_required{false};
    bool host_evidence_available{false};
    bool request_validation{false};
    bool shaping_segments{false};
    bool shaping_direction_script_language{false};
    bool glyph_clusters{false};
    bool glyph_advances_offsets{false};
    bool fallback_font_rows{false};
    bool bidi_boundaries{false};
    bool line_break_boundaries{false};
    bool glyph_bitmap_rows{false};
    bool glyph_pixel_format_rows{false};
    bool glyph_metric_rows{false};
    bool atlas_placement_rows{false};
    bool atlas_budget_rows{false};
    bool atlas_eviction_diagnostics{false};
    bool renderer_texture_upload_handoff{false};
    bool selected_package_counter_evidence{false};
    bool ime_begin_update_end{false};
    bool ime_candidate_rows{false};
    bool ime_text_area_rows{false};
    bool ime_committed_text_rows{false};
    bool platform_adapter_dispatch_boundary{false};
    bool accessibility_role_rows{false};
    bool accessibility_label_rows{false};
    bool accessibility_state_rows{false};
    bool accessibility_focus_rows{false};
    bool accessibility_action_rows{false};
    bool accessibility_relationship_rows{false};
    bool accessibility_live_region_rows{false};
    bool accessibility_os_publication_gate{false};
    bool requires_optional_dependency_adapter{false};
    bool dependency_adapter_reviewed{true};
    bool uses_public_native_handle{false};
    bool uses_ui_middleware_api{false};
    bool claims_general_production_text_stack{false};
    bool claims_broad_platform_ui_parity{false};
    bool invokes_adapter{false};
    bool invokes_native_platform{false};
    bool invokes_renderer_upload{false};
};

struct RuntimeUiProductionStackRequest {
    std::string id;
    std::vector<RuntimeUiProductionEvidenceRow> rows;
    std::size_t row_budget{64U};
};

struct RuntimeUiProductionDiagnostic {
    RuntimeUiProductionDiagnosticCode code{RuntimeUiProductionDiagnosticCode::no_rows};
    std::string row_id;
    std::string message;
};

struct RuntimeUiProductionStackPlan {
    RuntimeUiProductionStackStatus status{RuntimeUiProductionStackStatus::invalid_request};
    std::vector<RuntimeUiProductionEvidenceRow> rows;
    std::vector<RuntimeUiProductionDiagnostic> diagnostics;
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t dependency_gated_rows{0U};
    std::size_t skipped_rows{0U};
    std::size_t adapter_invoked_rows{0U};
    std::size_t unsupported_rows{0U};
    bool reviewed{false};
    bool text_stack_contract_ready{false};
    bool selected_package_counter_evidence_ready{false};
    bool production_runtime_ui_ready{false};
    bool requires_ime_host_evidence{false};
    bool ime_host_evidence_available{false};
    bool requires_accessibility_host_evidence{false};
    bool accessibility_host_evidence_available{false};
    bool invoked_text_shaping{false};
    bool invoked_font_rasterization{false};
    bool invoked_renderer_upload{false};
    bool invoked_ime_adapter{false};
    bool invoked_accessibility_bridge{false};
    bool invoked_native_platform{false};
    std::uint64_t replay_hash{0U};

    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] std::string_view runtime_ui_production_stack_status_name(RuntimeUiProductionStackStatus status) noexcept;
[[nodiscard]] RuntimeUiProductionStackPlan
plan_runtime_ui_production_stack(const RuntimeUiProductionStackRequest& request);

} // namespace mirakana::ui
