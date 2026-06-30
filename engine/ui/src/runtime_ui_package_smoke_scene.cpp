// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_package_smoke_scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

constexpr std::size_t kRequiredSceneKindCount{4U};

void append_diagnostic(std::vector<RuntimeUiPackageSmokeSceneDiagnostic>& diagnostics,
                       RuntimeUiPackageSmokeSceneDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(RuntimeUiPackageSmokeSceneDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool is_valid_kind(RuntimeUiPackageSmokeSceneKind kind) noexcept {
    switch (kind) {
    case RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback:
    case RuntimeUiPackageSmokeSceneKind::long_label_layout:
    case RuntimeUiPackageSmokeSceneKind::controller_only_navigation:
    case RuntimeUiPackageSmokeSceneKind::accessibility_tree_review:
        return true;
    }
    return false;
}

void update_counters(RuntimeUiPackageSmokeSceneReview& review, const RuntimeUiPackageSmokeSceneRow& row) noexcept {
    ++review.scene_rows;
    if (row.selected) {
        ++review.selected_rows;
        if (row.ready) {
            ++review.ready_rows;
        }
    }

    review.language_rows += row.language_rows;
    review.glyph_fallback_rows += row.glyph_fallback_rows;
    review.long_label_max_code_units = std::max(review.long_label_max_code_units, row.long_label_code_units);
    review.controller_navigation_edges += row.controller_navigation_edges;
    review.controller_glyph_rows += row.controller_glyph_rows;
    review.accessibility_nodes += row.accessibility_nodes;
    review.accessibility_action_rows += row.accessibility_action_rows;
    review.accessibility_reading_order_rows += row.accessibility_reading_order_rows;
    review.supporting_evidence_rows += row.supporting_evidence_rows;

    if (row.public_native_handles) {
        ++review.native_handle_rows;
    }
    if (row.ui_middleware_claim) {
        ++review.ui_middleware_claim_rows;
    }
    if (row.external_engine_compatibility_claim) {
        ++review.external_engine_compatibility_claim_rows;
    }
}

void validate_common_row(RuntimeUiPackageSmokeSceneReview& review, const RuntimeUiPackageSmokeSceneRow& row) {
    if (row.selected && !row.ready) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::selected_scene_not_ready,
                          row.id, "selected runtime UI package smoke scene rows must be ready");
    }
    if (row.selected && row.supporting_evidence_rows == 0U) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_supporting_evidence,
                          row.id, "selected runtime UI package smoke scene rows require supporting evidence rows");
    }
    if (row.public_native_handles) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::public_native_handles, row.id,
                          "runtime UI package smoke scene evidence must not expose native or backend handles");
    }
    if (row.ui_middleware_claim) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::ui_middleware_claim, row.id,
                          "runtime UI package smoke scene evidence must not claim UI middleware API exposure");
    }
    if (row.external_engine_compatibility_claim) {
        append_diagnostic(review.diagnostics,
                          RuntimeUiPackageSmokeSceneDiagnosticCode::external_engine_compatibility_claim, row.id,
                          "runtime UI package smoke scene evidence must not claim Unity, Unreal, Godot, visual, "
                          "workflow, or API compatibility");
    }
}

void validate_kind_row(RuntimeUiPackageSmokeSceneReview& review, const RuntimeUiPackageSmokeSceneRow& row) {
    switch (row.kind) {
    case RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback:
        if (row.language_rows == 0U) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_language_rows,
                              row.id, "multilingual runtime UI smoke scenes require language rows");
        }
        if (row.glyph_fallback_rows == 0U) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_glyph_fallback,
                              row.id, "multilingual runtime UI smoke scenes require glyph fallback rows");
        }
        return;
    case RuntimeUiPackageSmokeSceneKind::long_label_layout:
        ++review.long_label_rows;
        if (row.long_label_code_units == 0U || row.wrapped_line_rows == 0U) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_long_label_wrap,
                              row.id, "long-label runtime UI smoke scenes require wrapped layout rows");
        }
        return;
    case RuntimeUiPackageSmokeSceneKind::controller_only_navigation:
        ++review.controller_only_rows;
        if (row.controller_navigation_edges == 0U) {
            append_diagnostic(review.diagnostics,
                              RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_navigation, row.id,
                              "controller-only runtime UI smoke scenes require navigation edges");
        }
        if (row.controller_glyph_rows == 0U) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_glyph,
                              row.id, "controller-only runtime UI smoke scenes require controller glyph rows");
        }
        return;
    case RuntimeUiPackageSmokeSceneKind::accessibility_tree_review:
        ++review.accessibility_tree_rows;
        if (row.accessibility_nodes == 0U) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_tree,
                              row.id, "accessibility runtime UI smoke scenes require published accessibility nodes");
        }
        if (row.accessibility_action_rows == 0U) {
            append_diagnostic(review.diagnostics,
                              RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_action, row.id,
                              "accessibility runtime UI smoke scenes require accessibility action rows");
        }
        if (row.accessibility_reading_order_rows == 0U) {
            append_diagnostic(review.diagnostics,
                              RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_reading_order, row.id,
                              "accessibility runtime UI smoke scenes require reading-order rows");
        }
        return;
    }
}

void append_missing_scene_diagnostics(RuntimeUiPackageSmokeSceneReview& review,
                                      const std::array<bool, kRequiredSceneKindCount>& selected_ready_kinds) {
    if (!selected_ready_kinds[static_cast<std::size_t>(RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback)]) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_multilingual_scene, {},
                          "runtime UI package smoke scenes require a selected multilingual glyph fallback scene");
    }
    if (!selected_ready_kinds[static_cast<std::size_t>(RuntimeUiPackageSmokeSceneKind::long_label_layout)]) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_long_label_scene, {},
                          "runtime UI package smoke scenes require a selected long-label layout scene");
    }
    if (!selected_ready_kinds[static_cast<std::size_t>(RuntimeUiPackageSmokeSceneKind::controller_only_navigation)]) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_controller_only_scene,
                          {}, "runtime UI package smoke scenes require a selected controller-only navigation scene");
    }
    if (!selected_ready_kinds[static_cast<std::size_t>(RuntimeUiPackageSmokeSceneKind::accessibility_tree_review)]) {
        append_diagnostic(review.diagnostics,
                          RuntimeUiPackageSmokeSceneDiagnosticCode::missing_accessibility_tree_scene, {},
                          "runtime UI package smoke scenes require a selected accessibility tree review scene");
    }
}

} // namespace

bool RuntimeUiPackageSmokeSceneReview::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view runtime_ui_package_smoke_scene_kind_name(RuntimeUiPackageSmokeSceneKind kind) noexcept {
    switch (kind) {
    case RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback:
        return "multilingual_glyph_fallback";
    case RuntimeUiPackageSmokeSceneKind::long_label_layout:
        return "long_label_layout";
    case RuntimeUiPackageSmokeSceneKind::controller_only_navigation:
        return "controller_only_navigation";
    case RuntimeUiPackageSmokeSceneKind::accessibility_tree_review:
        return "accessibility_tree_review";
    }
    return "unknown";
}

RuntimeUiPackageSmokeSceneReview
review_runtime_ui_package_smoke_scenes(std::span<const RuntimeUiPackageSmokeSceneRow> rows, std::size_t row_budget) {
    RuntimeUiPackageSmokeSceneReview review;
    review.rows.assign(rows.begin(), rows.end());

    if (rows.empty()) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::no_scene_rows, {},
                          "runtime UI package smoke scene review requires rows");
    }
    if (row_budget == 0U || rows.size() > row_budget) {
        append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::row_budget_overflow, {},
                          "runtime UI package smoke scene rows exceed the request budget");
    }

    std::vector<std::string> row_ids;
    row_ids.reserve(rows.size());
    std::array<bool, kRequiredSceneKindCount> selected_ready_kinds{};

    for (const auto& row : rows) {
        update_counters(review, row);

        if (row.id.empty()) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::missing_scene_id, {},
                              "runtime UI package smoke scene row id must not be empty");
        } else if (contains_id(row_ids, row.id)) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::duplicate_scene_id, row.id,
                              "runtime UI package smoke scene row ids must be unique");
        } else {
            row_ids.push_back(row.id);
        }

        if (!is_valid_kind(row.kind)) {
            append_diagnostic(review.diagnostics, RuntimeUiPackageSmokeSceneDiagnosticCode::unsupported_scene_kind,
                              row.id, "runtime UI package smoke scene row has an unsupported kind");
            continue;
        }

        validate_common_row(review, row);
        validate_kind_row(review, row);

        if (row.selected && row.ready) {
            selected_ready_kinds[static_cast<std::size_t>(row.kind)] = true;
        }
    }

    append_missing_scene_diagnostics(review, selected_ready_kinds);
    review.ready = review.diagnostics.empty() &&
                   std::ranges::all_of(selected_ready_kinds, [](bool selected_ready) { return selected_ready; });
    return review;
}

} // namespace mirakana::ui
