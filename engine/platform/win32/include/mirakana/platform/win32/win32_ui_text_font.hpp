// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/runtime_ui_platform_production.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::win32 {

enum class Win32UiTextBoundaryKind : std::uint8_t {
    script_run,
    line_break,
    bidi_run,
};

enum class Win32UiTextShapeDiagnosticCode : std::uint8_t {
    invalid_text,
    invalid_utf8,
    missing_font_family,
    invalid_pixel_size,
    invalid_max_width,
    missing_language_tag,
    missing_script_tag,
    row_budget_exceeded,
    directwrite_factory_unavailable,
    directwrite_text_format_failed,
    directwrite_text_layout_failed,
    directwrite_text_analysis_failed,
    missing_glyph_rows,
    missing_boundary_rows,
    missing_fallback_rows,
    invalid_cluster_boundary,
    duplicate_cluster_row,
    public_native_handles_exposed,
};

struct Win32UiTextShapeRequest {
    std::string text;
    std::string font_family;
    float pixel_size{16.0F};
    ui::TextDirection direction{ui::TextDirection::automatic};
    std::string language_tag{"und"};
    std::string script_tag{"Zyyy"};
    float max_width{0.0F};
    std::size_t row_budget{128U};
};

struct Win32UiTextGlyphRow {
    std::uint32_t glyph_id{0};
    std::size_t cluster_byte_offset{0};
    float advance_x{0.0F};
    float advance_y{0.0F};
    float offset_x{0.0F};
    float offset_y{0.0F};
    std::string font_family;
};

struct Win32UiTextBoundaryRow {
    Win32UiTextBoundaryKind kind{Win32UiTextBoundaryKind::script_run};
    std::size_t start_byte{0};
    std::size_t end_byte{0};
    ui::TextDirection direction{ui::TextDirection::left_to_right};
    std::string language_tag;
    std::string script_tag;
    std::uint8_t bidi_level{0};
    bool line_break_allowed{false};
};

struct Win32UiTextFallbackFamilyRow {
    std::size_t cluster_byte_offset{0};
    std::string requested_font_family;
    std::string resolved_font_family;
    bool fallback_used{false};
};

struct Win32UiTextShapeDiagnostic {
    Win32UiTextShapeDiagnosticCode code{Win32UiTextShapeDiagnosticCode::invalid_text};
    std::string message;
    std::size_t byte_offset{0};
};

struct Win32UiTextShapeResult {
    bool ready{false};
    bool directwrite_factory_created{false};
    bool directwrite_text_layout_created{false};
    bool directwrite_text_analyzer_invoked{false};
    bool public_native_handles_exposed{false};
    std::string shaped_text;
    std::vector<Win32UiTextGlyphRow> glyph_rows;
    std::vector<Win32UiTextBoundaryRow> boundary_rows;
    std::vector<Win32UiTextFallbackFamilyRow> fallback_family_rows;
    std::vector<ui::TextLayoutRun> layout_runs;
    std::vector<Win32UiTextShapeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] Win32UiTextShapeResult shape_win32_ui_text_with_directwrite(const Win32UiTextShapeRequest& request);
[[nodiscard]] Win32UiTextShapeResult validate_win32_ui_text_shape_result_rows(Win32UiTextShapeResult result,
                                                                              std::size_t row_budget);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_text_shaping_production_evidence(const Win32UiTextShapeResult& result);

} // namespace mirakana::win32
