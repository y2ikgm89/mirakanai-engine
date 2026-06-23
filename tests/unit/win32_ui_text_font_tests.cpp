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

#endif

int main() {
    return mirakana::test::run_all();
}
