// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_package_smoke_scene.hpp"

#include <algorithm>
#include <vector>

namespace {

using mirakana::ui::RuntimeUiPackageSmokeSceneDiagnostic;
using mirakana::ui::RuntimeUiPackageSmokeSceneDiagnosticCode;
using mirakana::ui::RuntimeUiPackageSmokeSceneKind;
using mirakana::ui::RuntimeUiPackageSmokeSceneRow;

[[nodiscard]] bool has_diagnostic(const std::vector<RuntimeUiPackageSmokeSceneDiagnostic>& diagnostics,
                                  RuntimeUiPackageSmokeSceneDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] std::vector<RuntimeUiPackageSmokeSceneRow> make_complete_rows() {
    return {
        RuntimeUiPackageSmokeSceneRow{
            .id = "sample-2d-ui.multilingual-menu",
            .kind = RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback,
            .selected = true,
            .ready = true,
            .language_rows = 3U,
            .glyph_fallback_rows = 2U,
            .supporting_evidence_rows = 1U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "sample-2d-ui.long-label-settings",
            .kind = RuntimeUiPackageSmokeSceneKind::long_label_layout,
            .selected = true,
            .ready = true,
            .long_label_code_units = 96U,
            .wrapped_line_rows = 3U,
            .supporting_evidence_rows = 1U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "sample-2d-ui.controller-only-pause",
            .kind = RuntimeUiPackageSmokeSceneKind::controller_only_navigation,
            .selected = true,
            .ready = true,
            .controller_navigation_edges = 4U,
            .controller_glyph_rows = 2U,
            .supporting_evidence_rows = 1U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "sample-2d-ui.accessibility-tree",
            .kind = RuntimeUiPackageSmokeSceneKind::accessibility_tree_review,
            .selected = true,
            .ready = true,
            .accessibility_nodes = 5U,
            .accessibility_action_rows = 2U,
            .accessibility_reading_order_rows = 5U,
            .supporting_evidence_rows = 1U,
        },
    };
}

} // namespace

MK_TEST("runtime ui package smoke scenes require every selected scene family") {
    auto rows = make_complete_rows();
    rows.pop_back();

    const auto result = mirakana::ui::review_runtime_ui_package_smoke_scenes(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_tree_scene));
}

MK_TEST("runtime ui package smoke scenes require multilingual fallback and long label wrapping evidence") {
    auto rows = make_complete_rows();
    rows[0].glyph_fallback_rows = 0U;
    rows[1].wrapped_line_rows = 0U;

    const auto result = mirakana::ui::review_runtime_ui_package_smoke_scenes(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_glyph_fallback));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_long_label_wrap));
}

MK_TEST("runtime ui package smoke scenes reject missing controller and accessibility evidence") {
    auto rows = make_complete_rows();
    rows[2].controller_navigation_edges = 0U;
    rows[2].controller_glyph_rows = 0U;
    rows[3].accessibility_action_rows = 0U;
    rows[3].accessibility_reading_order_rows = 0U;

    const auto result = mirakana::ui::review_runtime_ui_package_smoke_scenes(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_navigation));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_glyph));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_action));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_reading_order));
}

MK_TEST("runtime ui package smoke scenes reject unsafe API and external engine claims") {
    auto rows = make_complete_rows();
    rows[0].public_native_handles = true;
    rows[1].ui_middleware_claim = true;
    rows[2].external_engine_compatibility_claim = true;

    const auto result = mirakana::ui::review_runtime_ui_package_smoke_scenes(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::public_native_handles));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::ui_middleware_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              RuntimeUiPackageSmokeSceneDiagnosticCode::external_engine_compatibility_claim));
}

MK_TEST("runtime ui package smoke scenes become ready with selected package-visible UI evidence") {
    const auto result = mirakana::ui::review_runtime_ui_package_smoke_scenes(make_complete_rows());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.scene_rows == 4U);
    MK_REQUIRE(result.selected_rows == 4U);
    MK_REQUIRE(result.ready_rows == 4U);
    MK_REQUIRE(result.language_rows == 3U);
    MK_REQUIRE(result.glyph_fallback_rows == 2U);
    MK_REQUIRE(result.long_label_rows == 1U);
    MK_REQUIRE(result.long_label_max_code_units == 96U);
    MK_REQUIRE(result.controller_only_rows == 1U);
    MK_REQUIRE(result.controller_navigation_edges == 4U);
    MK_REQUIRE(result.controller_glyph_rows == 2U);
    MK_REQUIRE(result.accessibility_tree_rows == 1U);
    MK_REQUIRE(result.accessibility_nodes == 5U);
    MK_REQUIRE(result.accessibility_action_rows == 2U);
    MK_REQUIRE(result.accessibility_reading_order_rows == 5U);
    MK_REQUIRE(result.supporting_evidence_rows == 4U);
    MK_REQUIRE(result.native_handle_rows == 0U);
    MK_REQUIRE(result.ui_middleware_claim_rows == 0U);
    MK_REQUIRE(result.external_engine_compatibility_claim_rows == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
