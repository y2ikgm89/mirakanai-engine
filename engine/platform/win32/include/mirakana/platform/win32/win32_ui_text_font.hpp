// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/runtime_ui_platform_production.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
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

enum class Win32UiFontSourceKind : std::uint8_t {
    unknown,
    system_font_collection,
    project_font_asset,
};

enum class Win32UiFontLicenseStatus : std::uint8_t {
    unknown,
    system_font_runtime_reference,
    project_font_asset_license_recorded,
};

enum class Win32UiFontDiagnosticCode : std::uint8_t {
    invalid_font_family,
    invalid_source_kind,
    missing_provenance,
    missing_license_status,
    unsupported_project_font_asset,
    missing_face_id,
    font_face_mismatch,
    invalid_glyph_id,
    invalid_pixel_size,
    invalid_dpi_scale,
    invalid_pixel_format,
    invalid_atlas_padding,
    row_budget_exceeded,
    directwrite_factory_unavailable,
    directwrite_font_collection_failed,
    directwrite_font_family_not_found,
    directwrite_font_face_failed,
    directwrite_glyph_metrics_failed,
    directwrite_rendering_mode_failed,
    directwrite_glyph_run_analysis_failed,
    missing_bitmap_pixels,
    invalid_metrics,
    atlas_overflow,
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

struct Win32UiFontLoadRequest {
    std::string font_family;
    Win32UiFontSourceKind source_kind{Win32UiFontSourceKind::system_font_collection};
    std::string provenance_id;
    Win32UiFontLicenseStatus license_status{Win32UiFontLicenseStatus::unknown};
    std::size_t row_budget{16U};
};

struct Win32UiFontFaceRow {
    std::string font_family;
    std::string resolved_face_id;
    Win32UiFontSourceKind source_kind{Win32UiFontSourceKind::unknown};
    std::string provenance_id;
    Win32UiFontLicenseStatus license_status{Win32UiFontLicenseStatus::unknown};
    std::uint32_t glyph_count{0};
    std::uint32_t design_units_per_em{0};
};

struct Win32UiGlyphRasterRequest {
    std::string font_family;
    std::string resolved_face_id;
    Win32UiFontSourceKind source_kind{Win32UiFontSourceKind::system_font_collection};
    std::string provenance_id;
    Win32UiFontLicenseStatus license_status{Win32UiFontLicenseStatus::unknown};
    std::uint32_t glyph_id{0};
    float pixel_size{16.0F};
    float dpi_scale{1.0F};
    ui::FontRasterizationPixelFormat pixel_format{ui::FontRasterizationPixelFormat::alpha8};
    std::uint32_t atlas_padding{0};
    std::size_t row_budget{16U};
};

struct Win32UiGlyphBitmapRow {
    std::uint32_t glyph_id{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    ui::FontRasterizationPixelFormat pixel_format{ui::FontRasterizationPixelFormat::unknown};
    std::size_t pixel_bytes{0};
    std::uint32_t atlas_padding{0};
};

struct Win32UiGlyphMetricsRow {
    std::uint32_t glyph_id{0};
    float width{0.0F};
    float height{0.0F};
    float bearing_x{0.0F};
    float bearing_y{0.0F};
    float advance_x{0.0F};
    float advance_y{0.0F};
};

struct Win32UiGlyphColorRow {
    std::uint32_t glyph_id{0};
    bool color_glyph_checked{false};
    bool color_glyph_used{false};
};

struct Win32UiFontDiagnostic {
    Win32UiFontDiagnosticCode code{Win32UiFontDiagnosticCode::invalid_font_family};
    std::string message;
};

struct Win32UiFontLoadResult {
    bool ready{false};
    bool directwrite_factory_created{false};
    bool system_font_collection_loaded{false};
    bool public_native_handles_exposed{false};
    std::vector<Win32UiFontFaceRow> font_face_rows;
    std::vector<Win32UiFontDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct Win32UiGlyphRasterResult {
    bool ready{false};
    bool directwrite_factory_created{false};
    bool system_font_collection_loaded{false};
    bool font_face_loaded{false};
    bool glyph_metrics_queried{false};
    bool recommended_rendering_mode_queried{false};
    bool glyph_run_analysis_created{false};
    bool alpha_texture_created{false};
    bool public_native_handles_exposed{false};
    std::vector<Win32UiFontFaceRow> font_face_rows;
    std::vector<Win32UiGlyphBitmapRow> bitmap_rows;
    std::vector<Win32UiGlyphMetricsRow> metrics_rows;
    std::vector<Win32UiGlyphColorRow> color_rows;
    std::optional<ui::GlyphAtlasAllocation> allocation;
    std::vector<Win32UiFontDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] Win32UiTextShapeResult shape_win32_ui_text_with_directwrite(const Win32UiTextShapeRequest& request);
[[nodiscard]] Win32UiTextShapeResult validate_win32_ui_text_shape_result_rows(Win32UiTextShapeResult result,
                                                                              std::size_t row_budget);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_text_shaping_production_evidence(const Win32UiTextShapeResult& result);
[[nodiscard]] Win32UiFontLoadResult load_win32_ui_font_face(const Win32UiFontLoadRequest& request);
[[nodiscard]] Win32UiGlyphRasterResult rasterize_win32_ui_glyph(const Win32UiGlyphRasterRequest& request);
[[nodiscard]] Win32UiFontLoadResult validate_win32_ui_font_load_result_rows(Win32UiFontLoadResult result,
                                                                            std::size_t row_budget);
[[nodiscard]] Win32UiGlyphRasterResult validate_win32_ui_glyph_raster_result_rows(Win32UiGlyphRasterResult result,
                                                                                  std::size_t row_budget);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_font_loading_production_evidence(const Win32UiFontLoadResult& result);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_font_rasterization_production_evidence(const Win32UiGlyphRasterResult& result);

} // namespace mirakana::win32
