// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiPackageSmokeSceneKind : std::uint8_t {
    multilingual_glyph_fallback,
    long_label_layout,
    controller_only_navigation,
    accessibility_tree_review,
};

enum class RuntimeUiPackageSmokeSceneDiagnosticCode : std::uint8_t {
    no_scene_rows,
    missing_scene_id,
    duplicate_scene_id,
    unsupported_scene_kind,
    missing_multilingual_scene,
    missing_long_label_scene,
    missing_controller_only_scene,
    missing_accessibility_tree_scene,
    selected_scene_not_ready,
    missing_language_rows,
    missing_glyph_fallback,
    missing_long_label_wrap,
    missing_controller_navigation,
    missing_controller_glyph,
    missing_accessibility_tree,
    missing_accessibility_action,
    missing_accessibility_reading_order,
    missing_supporting_evidence,
    public_native_handles,
    ui_middleware_claim,
    external_engine_compatibility_claim,
    row_budget_overflow,
};

struct RuntimeUiPackageSmokeSceneRow {
    std::string id;
    RuntimeUiPackageSmokeSceneKind kind{RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback};
    bool selected{false};
    bool ready{false};
    std::size_t language_rows{0U};
    std::size_t glyph_fallback_rows{0U};
    std::size_t long_label_code_units{0U};
    std::size_t wrapped_line_rows{0U};
    std::size_t controller_navigation_edges{0U};
    std::size_t controller_glyph_rows{0U};
    std::size_t accessibility_nodes{0U};
    std::size_t accessibility_action_rows{0U};
    std::size_t accessibility_reading_order_rows{0U};
    std::size_t supporting_evidence_rows{0U};
    bool public_native_handles{false};
    bool ui_middleware_claim{false};
    bool external_engine_compatibility_claim{false};
};

struct RuntimeUiPackageSmokeSceneDiagnostic {
    RuntimeUiPackageSmokeSceneDiagnosticCode code{RuntimeUiPackageSmokeSceneDiagnosticCode::no_scene_rows};
    std::string row_id;
    std::string message;
};

struct RuntimeUiPackageSmokeSceneReview {
    bool ready{false};
    std::vector<RuntimeUiPackageSmokeSceneRow> rows;
    std::vector<RuntimeUiPackageSmokeSceneDiagnostic> diagnostics;
    std::size_t scene_rows{0U};
    std::size_t selected_rows{0U};
    std::size_t ready_rows{0U};
    std::size_t language_rows{0U};
    std::size_t glyph_fallback_rows{0U};
    std::size_t long_label_rows{0U};
    std::size_t long_label_max_code_units{0U};
    std::size_t controller_only_rows{0U};
    std::size_t controller_navigation_edges{0U};
    std::size_t controller_glyph_rows{0U};
    std::size_t accessibility_tree_rows{0U};
    std::size_t accessibility_nodes{0U};
    std::size_t accessibility_action_rows{0U};
    std::size_t accessibility_reading_order_rows{0U};
    std::size_t supporting_evidence_rows{0U};
    std::size_t native_handle_rows{0U};
    std::size_t ui_middleware_claim_rows{0U};
    std::size_t external_engine_compatibility_claim_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view runtime_ui_package_smoke_scene_kind_name(RuntimeUiPackageSmokeSceneKind kind) noexcept;
[[nodiscard]] RuntimeUiPackageSmokeSceneReview
review_runtime_ui_package_smoke_scenes(std::span<const RuntimeUiPackageSmokeSceneRow> rows,
                                       std::size_t row_budget = 32U);

} // namespace mirakana::ui
