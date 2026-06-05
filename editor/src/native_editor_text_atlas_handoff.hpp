// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

struct NativeEditorTextAtlasHandoffDesc {
    std::string text{"Mirakanai"};
    std::string font_family{"Segoe UI"};
    float pixel_size{18.0F};
    float max_width{512.0F};
};

struct NativeEditorTextAtlasHandoffEvidenceRow {
    std::string id;
    std::string status;
    bool adapter_invoked{false};
    bool glyphs_ready{false};
    bool fallback_used{false};
    bool host_evidence_required{false};
    bool host_evidence_available{false};
    bool unsupported{false};
    bool direction_script_language_ready{false};
    bool glyph_clusters_ready{false};
    bool glyph_advances_offsets_ready{false};
    bool bidi_boundaries_ready{false};
    bool word_boundaries_ready{false};
    bool line_break_boundaries_ready{false};
    bool font_face_ready{false};
    bool glyph_index_lookup_ready{false};
    bool glyph_metrics_ready{false};
    bool bitmap_format_ready{false};
    bool atlas_allocation_ready{false};
    bool license_provenance_ready{false};
    bool dependency_gated{false};
    bool native_handles_public{false};
};

struct NativeEditorTextAtlasHandoffEvidence {
    std::string status{"not_evaluated"};
    bool text_shaping_adapter_invoked{false};
    bool font_rasterizer_adapter_invoked{false};
    bool glyphs_ready{false};
    bool fallback_used{false};
    bool atlas_handoff_ready{false};
    bool native_handles_exposed{false};
    std::string text_shaping_status{"not_evaluated"};
    std::string font_fallback_status{"not_evaluated"};
    std::string glyph_atlas_status{"not_evaluated"};
    std::string bidi_status{"not_evaluated"};
    std::string line_break_status{"not_evaluated"};
    std::string dependency_license_records_status{"not_evaluated"};
    std::string harf_buzz_dependency_status{"dependency_gated"};
    std::string free_type_dependency_status{"dependency_gated"};
    std::string icu_dependency_status{"dependency_gated"};
    std::uint32_t adapter_invoked_rows{0};
    std::uint32_t glyphs_ready_rows{0};
    std::uint32_t fallback_rows{0};
    std::uint32_t fallback_used_rows{0};
    std::uint32_t fallback_not_used_rows{0};
    std::uint32_t shaping_segment_rows{0};
    std::uint32_t shaping_direction_script_language_rows{0};
    std::uint32_t glyph_cluster_rows{0};
    std::uint32_t glyph_advance_offset_rows{0};
    std::uint32_t bidi_boundary_rows{0};
    std::uint32_t word_boundary_rows{0};
    std::uint32_t line_break_boundary_rows{0};
    std::uint32_t font_face_rows{0};
    std::uint32_t glyph_index_lookup_rows{0};
    std::uint32_t glyph_metric_rows{0};
    std::uint32_t glyph_bitmap_format_rows{0};
    std::uint32_t glyph_atlas_allocation_rows{0};
    std::uint32_t font_license_provenance_rows{0};
    std::uint32_t dependency_gated_rows{0};
    std::uint32_t host_gated_rows{0};
    std::uint32_t unsupported_rows{0};
    std::uint32_t shaped_glyph_count{0};
    std::uint32_t rasterized_glyph_count{0};
    std::uint32_t zero_ink_glyph_count{0};
    std::vector<NativeEditorTextAtlasHandoffEvidenceRow> rows;
};

[[nodiscard]] NativeEditorTextAtlasHandoffEvidence
make_native_editor_directwrite_text_atlas_handoff_evidence(const NativeEditorTextAtlasHandoffDesc& desc);

} // namespace mirakana::editor
