// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/win32/win32_ui_text_font.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#if defined(_WIN32)

namespace {

using mirakana::win32::Win32UiFontDiagnostic;
using mirakana::win32::Win32UiFontDiagnosticCode;
using mirakana::win32::Win32UiFontLicenseStatus;
using mirakana::win32::Win32UiFontLoadRequest;
using mirakana::win32::Win32UiFontLoadResult;
using mirakana::win32::Win32UiFontSourceKind;
using mirakana::win32::Win32UiGlyphRasterRequest;
using mirakana::win32::Win32UiGlyphRasterResult;
using mirakana::win32::Win32UiTextBoundaryKind;
using mirakana::win32::Win32UiTextBoundaryRow;
using mirakana::win32::Win32UiTextFallbackFamilyRow;
using mirakana::win32::Win32UiTextGlyphRow;
using mirakana::win32::Win32UiTextShapeDiagnostic;
using mirakana::win32::Win32UiTextShapeDiagnosticCode;
using mirakana::win32::Win32UiTextShapeRequest;
using mirakana::win32::Win32UiTextShapeResult;

[[nodiscard]] Win32UiTextShapeRequest make_request(std::string text) {
    return Win32UiTextShapeRequest{
        .text = std::move(text),
        .font_family = "Segoe UI",
        .pixel_size = 16.0F,
        .direction = mirakana::ui::TextDirection::left_to_right,
        .language_tag = "en-US",
        .script_tag = "Latn",
        .max_width = 512.0F,
        .row_budget = 128U,
    };
}

[[nodiscard]] bool has_diagnostic(const std::vector<Win32UiTextShapeDiagnostic>& diagnostics,
                                  Win32UiTextShapeDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] bool has_font_diagnostic(const std::vector<Win32UiFontDiagnostic>& diagnostics,
                                       Win32UiFontDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] bool has_boundary(const std::vector<Win32UiTextBoundaryRow>& rows, Win32UiTextBoundaryKind kind) {
    return std::ranges::any_of(rows, [kind](const auto& row) { return row.kind == kind; });
}

[[nodiscard]] bool has_direction(const std::vector<Win32UiTextBoundaryRow>& rows,
                                 mirakana::ui::TextDirection direction) {
    return std::ranges::any_of(rows, [direction](const auto& row) {
        return row.kind == Win32UiTextBoundaryKind::bidi_run && row.direction == direction;
    });
}

[[nodiscard]] bool glyph_clusters_are_unique(const std::vector<Win32UiTextGlyphRow>& glyph_rows) {
    std::vector<std::size_t> clusters;
    clusters.reserve(glyph_rows.size());
    for (const auto& glyph : glyph_rows) {
        if (std::ranges::find(clusters, glyph.cluster_byte_offset) != clusters.end()) {
            return false;
        }
        clusters.push_back(glyph.cluster_byte_offset);
    }
    return true;
}

} // namespace

MK_TEST("win32 DirectWrite text shaping emits glyph fallback boundary and production evidence rows") {
    const Win32UiTextShapeResult result =
        mirakana::win32::shape_win32_ui_text_with_directwrite(make_request("Runtime UI"));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.directwrite_factory_created);
    MK_REQUIRE(result.directwrite_text_layout_created);
    MK_REQUIRE(result.directwrite_text_analyzer_invoked);
    MK_REQUIRE(!result.public_native_handles_exposed);
    MK_REQUIRE(!result.glyph_rows.empty());
    MK_REQUIRE(!result.fallback_family_rows.empty());
    const Win32UiTextFallbackFamilyRow& fallback = result.fallback_family_rows.front();
    MK_REQUIRE(fallback.requested_font_family == "Segoe UI");
    MK_REQUIRE(!fallback.fallback_used);
    MK_REQUIRE(has_boundary(result.boundary_rows, Win32UiTextBoundaryKind::script_run));
    MK_REQUIRE(has_boundary(result.boundary_rows, Win32UiTextBoundaryKind::line_break));
    MK_REQUIRE(has_boundary(result.boundary_rows, Win32UiTextBoundaryKind::bidi_run));
    MK_REQUIRE(!has_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::duplicate_cluster_row));
    MK_REQUIRE(result.layout_runs.size() == 1U);
    MK_REQUIRE(result.layout_runs[0].segments[0].language_tag == "en-US");
    MK_REQUIRE(result.layout_runs[0].segments[0].script_tag == "Latn");

    const auto evidence = mirakana::win32::make_win32_directwrite_text_shaping_production_evidence(result);
    MK_REQUIRE(evidence.id == "runtime-ui-platform.text-shaping.win32.directwrite");
    MK_REQUIRE(evidence.feature == mirakana::ui::RuntimeUiPlatformProductionFeature::production_text_shaping);
    MK_REQUIRE(evidence.proof == mirakana::ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter);
    MK_REQUIRE(evidence.selected);
    MK_REQUIRE(evidence.ready);
    MK_REQUIRE(evidence.dependency_recorded);
    MK_REQUIRE(evidence.host_evidence_available);
    MK_REQUIRE(!evidence.public_native_handles);
}

MK_TEST("win32 DirectWrite text shaping accepts Japanese UTF-8 scalar clusters") {
    auto request = make_request("\xE3\x81\x8B\xE3\x81\xAA");
    request.language_tag = "ja-JP";
    request.script_tag = "Jpan";

    const auto result = mirakana::win32::shape_win32_ui_text_with_directwrite(request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.glyph_rows.size() >= 2U);
    MK_REQUIRE(result.glyph_rows[0].cluster_byte_offset == 0U);
    MK_REQUIRE(result.glyph_rows[1].cluster_byte_offset == 3U);
    MK_REQUIRE(result.boundary_rows[0].language_tag == "ja-JP");
    MK_REQUIRE(result.layout_runs[0].segments[0].script_tag == "Jpan");
}

MK_TEST("win32 DirectWrite text shaping records mixed LTR and RTL bidi evidence") {
    auto request = make_request("abc \xD7\x90\xD7\x91");
    request.language_tag = "he-IL";
    request.script_tag = "Hebr";

    const auto result = mirakana::win32::shape_win32_ui_text_with_directwrite(request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(has_direction(result.boundary_rows, mirakana::ui::TextDirection::left_to_right));
    MK_REQUIRE(has_direction(result.boundary_rows, mirakana::ui::TextDirection::right_to_left));
}

MK_TEST("win32 DirectWrite text shaping rejects missing font family invalid UTF-8 and bad budgets") {
    auto missing_font = make_request("Text");
    missing_font.font_family.clear();
    const auto missing_font_result = mirakana::win32::shape_win32_ui_text_with_directwrite(missing_font);
    MK_REQUIRE(!missing_font_result.succeeded());
    MK_REQUIRE(has_diagnostic(missing_font_result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_font_family));

    auto invalid_utf8 = make_request(std::string{"\xC3\x28", 2U});
    const auto invalid_utf8_result = mirakana::win32::shape_win32_ui_text_with_directwrite(invalid_utf8);
    MK_REQUIRE(!invalid_utf8_result.succeeded());
    MK_REQUIRE(has_diagnostic(invalid_utf8_result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_utf8));

    auto row_budget = make_request("Budget");
    row_budget.row_budget = 1U;
    const auto row_budget_result = mirakana::win32::shape_win32_ui_text_with_directwrite(row_budget);
    MK_REQUIRE(!row_budget_result.succeeded());
    MK_REQUIRE(has_diagnostic(row_budget_result.diagnostics, Win32UiTextShapeDiagnosticCode::row_budget_exceeded));
}

MK_TEST("win32 DirectWrite text shaping validation rejects duplicate cluster rows") {
    auto result = mirakana::win32::shape_win32_ui_text_with_directwrite(make_request("Cluster"));
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(glyph_clusters_are_unique(result.glyph_rows));

    result.glyph_rows.push_back(result.glyph_rows.front());
    const auto validated = mirakana::win32::validate_win32_ui_text_shape_result_rows(std::move(result), 128U);

    MK_REQUIRE(!validated.succeeded());
    MK_REQUIRE(has_diagnostic(validated.diagnostics, Win32UiTextShapeDiagnosticCode::duplicate_cluster_row));
}

MK_TEST("win32 DirectWrite font loading and glyph rasterization emit selected bitmap metrics evidence") {
    const auto font_result = mirakana::win32::load_win32_ui_font_face(Win32UiFontLoadRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .row_budget = 8U,
    });

    MK_REQUIRE(font_result.succeeded());
    MK_REQUIRE(font_result.directwrite_factory_created);
    MK_REQUIRE(font_result.system_font_collection_loaded);
    MK_REQUIRE(!font_result.public_native_handles_exposed);
    MK_REQUIRE(font_result.font_face_rows.size() == 1U);
    MK_REQUIRE(font_result.font_face_rows.front().font_family == "Segoe UI");
    MK_REQUIRE(!font_result.font_face_rows.front().resolved_face_id.empty());
    MK_REQUIRE(font_result.font_face_rows.front().source_kind == Win32UiFontSourceKind::system_font_collection);
    MK_REQUIRE(font_result.font_face_rows.front().license_status ==
               Win32UiFontLicenseStatus::system_font_runtime_reference);

    const auto raster_result = mirakana::win32::rasterize_win32_ui_glyph(Win32UiGlyphRasterRequest{
        .font_family = font_result.font_face_rows.front().font_family,
        .resolved_face_id = font_result.font_face_rows.front().resolved_face_id,
        .source_kind = font_result.font_face_rows.front().source_kind,
        .provenance_id = font_result.font_face_rows.front().provenance_id,
        .license_status = font_result.font_face_rows.front().license_status,
        .glyph_id = 65U,
        .pixel_size = 24.0F,
        .dpi_scale = 1.0F,
        .pixel_format = mirakana::ui::FontRasterizationPixelFormat::alpha8,
        .atlas_padding = 2U,
        .row_budget = 16U,
    });

    MK_REQUIRE(raster_result.succeeded());
    MK_REQUIRE(raster_result.directwrite_factory_created);
    MK_REQUIRE(raster_result.font_face_loaded);
    MK_REQUIRE(raster_result.glyph_metrics_queried);
    MK_REQUIRE(raster_result.glyph_run_analysis_created);
    MK_REQUIRE(raster_result.alpha_texture_created);
    MK_REQUIRE(!raster_result.public_native_handles_exposed);
    MK_REQUIRE(raster_result.allocation.has_value());
    MK_REQUIRE(raster_result.allocation->glyph == 65U);
    MK_REQUIRE(raster_result.allocation->bitmap.pixel_format == mirakana::ui::FontRasterizationPixelFormat::alpha8);
    MK_REQUIRE(!raster_result.allocation->bitmap.pixels.empty());
    MK_REQUIRE(raster_result.allocation->metrics.advance_x > 0.0F);
    MK_REQUIRE(raster_result.font_face_rows.size() == 1U);

    const auto font_evidence = mirakana::win32::make_win32_directwrite_font_loading_production_evidence(font_result);
    MK_REQUIRE(font_evidence.id == "runtime-ui-platform.font-loading.win32.directwrite");
    MK_REQUIRE(font_evidence.feature == mirakana::ui::RuntimeUiPlatformProductionFeature::real_font_loading);
    MK_REQUIRE(font_evidence.selected);
    MK_REQUIRE(font_evidence.ready);
    MK_REQUIRE(font_evidence.dependency_recorded);
    MK_REQUIRE(font_evidence.host_evidence_available);
    MK_REQUIRE(!font_evidence.public_native_handles);

    const auto raster_evidence =
        mirakana::win32::make_win32_directwrite_font_rasterization_production_evidence(raster_result);
    MK_REQUIRE(raster_evidence.id == "runtime-ui-platform.font-rasterization.win32.directwrite");
    MK_REQUIRE(raster_evidence.feature == mirakana::ui::RuntimeUiPlatformProductionFeature::font_rasterization);
    MK_REQUIRE(raster_evidence.selected);
    MK_REQUIRE(raster_evidence.ready);
    MK_REQUIRE(raster_evidence.dependency_recorded);
    MK_REQUIRE(raster_evidence.host_evidence_available);
    MK_REQUIRE(!raster_evidence.public_native_handles);
}

MK_TEST("win32 DirectWrite font loading rejects missing provenance and unsupported project font assets") {
    auto missing_provenance = Win32UiFontLoadRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .row_budget = 8U,
    };
    const auto missing_provenance_result = mirakana::win32::load_win32_ui_font_face(missing_provenance);
    MK_REQUIRE(!missing_provenance_result.succeeded());
    MK_REQUIRE(
        has_font_diagnostic(missing_provenance_result.diagnostics, Win32UiFontDiagnosticCode::missing_provenance));

    const auto missing_license_status_result = mirakana::win32::load_win32_ui_font_face(Win32UiFontLoadRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::unknown,
        .row_budget = 8U,
    });
    MK_REQUIRE(!missing_license_status_result.succeeded());
    MK_REQUIRE(has_font_diagnostic(missing_license_status_result.diagnostics,
                                   Win32UiFontDiagnosticCode::missing_license_status));

    const auto project_asset_result = mirakana::win32::load_win32_ui_font_face(Win32UiFontLoadRequest{
        .font_family = "ProjectFont",
        .source_kind = Win32UiFontSourceKind::project_font_asset,
        .provenance_id = "project.font.asset",
        .license_status = Win32UiFontLicenseStatus::project_font_asset_license_recorded,
        .row_budget = 8U,
    });
    MK_REQUIRE(!project_asset_result.succeeded());
    MK_REQUIRE(has_font_diagnostic(project_asset_result.diagnostics,
                                   Win32UiFontDiagnosticCode::unsupported_project_font_asset));
}

MK_TEST("win32 DirectWrite glyph rasterization rejects missing face id zero pixel size and atlas overflow") {
    const auto missing_face_result = mirakana::win32::rasterize_win32_ui_glyph(Win32UiGlyphRasterRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .glyph_id = 65U,
        .pixel_size = 16.0F,
        .dpi_scale = 1.0F,
        .pixel_format = mirakana::ui::FontRasterizationPixelFormat::alpha8,
        .row_budget = 16U,
    });
    MK_REQUIRE(!missing_face_result.succeeded());
    MK_REQUIRE(has_font_diagnostic(missing_face_result.diagnostics, Win32UiFontDiagnosticCode::missing_face_id));

    const auto font_result = mirakana::win32::load_win32_ui_font_face(Win32UiFontLoadRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .row_budget = 8U,
    });
    MK_REQUIRE(font_result.succeeded());

    const auto zero_size_result = mirakana::win32::rasterize_win32_ui_glyph(Win32UiGlyphRasterRequest{
        .font_family = "Segoe UI",
        .resolved_face_id = font_result.font_face_rows.front().resolved_face_id,
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .glyph_id = 65U,
        .pixel_size = 0.0F,
        .dpi_scale = 1.0F,
        .pixel_format = mirakana::ui::FontRasterizationPixelFormat::alpha8,
        .row_budget = 16U,
    });
    MK_REQUIRE(!zero_size_result.succeeded());
    MK_REQUIRE(has_font_diagnostic(zero_size_result.diagnostics, Win32UiFontDiagnosticCode::invalid_pixel_size));

    const auto atlas_overflow_result = mirakana::win32::rasterize_win32_ui_glyph(Win32UiGlyphRasterRequest{
        .font_family = "Segoe UI",
        .resolved_face_id = font_result.font_face_rows.front().resolved_face_id,
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .glyph_id = 65U,
        .pixel_size = 16.0F,
        .dpi_scale = 1.0F,
        .pixel_format = mirakana::ui::FontRasterizationPixelFormat::alpha8,
        .atlas_padding = 4097U,
        .row_budget = 16U,
    });
    MK_REQUIRE(!atlas_overflow_result.succeeded());
    MK_REQUIRE(
        has_font_diagnostic(atlas_overflow_result.diagnostics, Win32UiFontDiagnosticCode::invalid_atlas_padding));
}

MK_TEST("win32 DirectWrite glyph rasterization validation rejects missing bitmap metrics and native handles") {
    const auto font_result = mirakana::win32::load_win32_ui_font_face(Win32UiFontLoadRequest{
        .font_family = "Segoe UI",
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .row_budget = 8U,
    });
    MK_REQUIRE(font_result.succeeded());

    auto raster_result = mirakana::win32::rasterize_win32_ui_glyph(Win32UiGlyphRasterRequest{
        .font_family = "Segoe UI",
        .resolved_face_id = font_result.font_face_rows.front().resolved_face_id,
        .source_kind = Win32UiFontSourceKind::system_font_collection,
        .provenance_id = "windows.system-font.segoe-ui",
        .license_status = Win32UiFontLicenseStatus::system_font_runtime_reference,
        .glyph_id = 65U,
        .pixel_size = 16.0F,
        .dpi_scale = 1.0F,
        .pixel_format = mirakana::ui::FontRasterizationPixelFormat::alpha8,
        .row_budget = 16U,
    });
    MK_REQUIRE(raster_result.succeeded());
    MK_REQUIRE(raster_result.color_rows.size() == 1U);
    MK_REQUIRE(raster_result.color_rows.front().color_glyph_checked);

    Win32UiGlyphRasterResult missing_bitmap = raster_result;
    missing_bitmap.alpha_texture_created = false;
    missing_bitmap.allocation->bitmap.pixels.clear();
    missing_bitmap.bitmap_rows.clear();
    missing_bitmap = mirakana::win32::validate_win32_ui_glyph_raster_result_rows(std::move(missing_bitmap), 16U);
    MK_REQUIRE(!missing_bitmap.succeeded());
    MK_REQUIRE(has_font_diagnostic(missing_bitmap.diagnostics, Win32UiFontDiagnosticCode::missing_bitmap_pixels));

    Win32UiGlyphRasterResult invalid_metrics = raster_result;
    invalid_metrics.allocation->metrics.advance_x = -1.0F;
    invalid_metrics.metrics_rows.front().advance_x = -1.0F;
    invalid_metrics = mirakana::win32::validate_win32_ui_glyph_raster_result_rows(std::move(invalid_metrics), 16U);
    MK_REQUIRE(!invalid_metrics.succeeded());
    MK_REQUIRE(has_font_diagnostic(invalid_metrics.diagnostics, Win32UiFontDiagnosticCode::invalid_metrics));

    Win32UiFontLoadResult native_font_handles = font_result;
    native_font_handles.public_native_handles_exposed = true;
    native_font_handles = mirakana::win32::validate_win32_ui_font_load_result_rows(std::move(native_font_handles), 8U);
    MK_REQUIRE(!native_font_handles.succeeded());
    MK_REQUIRE(
        has_font_diagnostic(native_font_handles.diagnostics, Win32UiFontDiagnosticCode::public_native_handles_exposed));
}

#endif

int main() {
    return mirakana::test::run_all();
}
