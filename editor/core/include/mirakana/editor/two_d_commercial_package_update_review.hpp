// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class Editor2DCommercialPackageUpdateReviewStatus : std::uint8_t { ready, blocked, host_gated };

enum class Editor2DCommercialPackageSmokeReviewStatus : std::uint8_t {
    passed,
    failed,
    blocked,
    host_gated,
    missing,
};

struct Editor2DCommercialPackageUpdateSmokeReviewInput {
    std::string id;
    std::string recipe_id;
    Editor2DCommercialPackageSmokeReviewStatus status{Editor2DCommercialPackageSmokeReviewStatus::missing};
    std::vector<std::string> host_gates;
    std::string diagnostic;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
};

struct Editor2DCommercialPackageUpdateRejectionDiagnosticInput {
    std::string id;
    std::string candidate_path;
    std::string status_label;
    std::string diagnostic;
    bool blocks_apply{false};
};

struct Editor2DCommercialPackageUpdateReviewDesc {
    ScenePackageRegistrationApplyPlan apply_plan;
    std::uint64_t expected_manifest_revision{0U};
    std::uint64_t active_manifest_revision{0U};
    std::size_t undo_stack_count{0U};
    std::size_t redo_stack_count{0U};
    std::vector<Editor2DCommercialPackageUpdateSmokeReviewInput> smoke_reviews;
    std::string selected_smoke_review_id;
    std::vector<Editor2DCommercialPackageUpdateRejectionDiagnosticInput> rejection_diagnostics;
    bool request_package_script_execution{false};
    bool request_validation_execution{false};
    bool request_runtime_source_parsing{false};
    bool request_native_handle_exposure{false};
    bool request_external_engine_project_import{false};
    bool request_external_engine_api_parity_claim{false};
};

struct Editor2DCommercialPackageUpdatePreviewRow {
    std::string id;
    std::string runtime_package_path;
    Editor2DCommercialPackageUpdateReviewStatus status{Editor2DCommercialPackageUpdateReviewStatus::blocked};
    std::string status_label;
    std::string diagnostic;
    bool will_add{false};
};

struct Editor2DCommercialPackageUpdateSmokeReviewRow {
    std::string id;
    std::string recipe_id;
    Editor2DCommercialPackageSmokeReviewStatus status{Editor2DCommercialPackageSmokeReviewStatus::missing};
    std::string status_label;
    std::vector<std::string> host_gates;
    std::string diagnostic;
};

struct Editor2DCommercialPackageUpdateRejectionDiagnosticRow {
    std::string id;
    std::string candidate_path;
    std::string status_label;
    std::string diagnostic;
    bool blocks_apply{false};
};

struct Editor2DCommercialPackageUpdateReviewModel {
    std::vector<Editor2DCommercialPackageUpdatePreviewRow> preview_rows;
    std::vector<Editor2DCommercialPackageUpdateSmokeReviewRow> smoke_reviews;
    std::vector<Editor2DCommercialPackageUpdateRejectionDiagnosticRow> rejection_diagnostics;
    Editor2DCommercialPackageUpdateReviewStatus status{Editor2DCommercialPackageUpdateReviewStatus::blocked};
    std::string status_label;
    std::string game_manifest_path;
    std::uint64_t expected_manifest_revision{0U};
    std::uint64_t active_manifest_revision{0U};
    std::size_t undo_stack_count{0U};
    std::size_t redo_stack_count{0U};
    bool ready_for_reviewed_apply{false};
    bool revision_checked{false};
    bool undo_redo_safe{false};
    bool safe_package_mutation_preview_available{false};
    bool selected_package_smoke_reviewed{false};
    bool rejection_diagnostics_ready{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view
editor_2d_commercial_package_update_review_status_label(Editor2DCommercialPackageUpdateReviewStatus status) noexcept;

[[nodiscard]] std::string_view
editor_2d_commercial_package_smoke_review_status_label(Editor2DCommercialPackageSmokeReviewStatus status) noexcept;

[[nodiscard]] Editor2DCommercialPackageUpdateReviewModel
make_editor_2d_commercial_package_update_review_model(const Editor2DCommercialPackageUpdateReviewDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_2d_commercial_package_update_review_ui_model(const Editor2DCommercialPackageUpdateReviewModel& model);

[[nodiscard]] UndoableAction
make_editor_2d_commercial_package_update_apply_action(ITextStore& store, const ScenePackageRegistrationApplyPlan& plan,
                                                      const Editor2DCommercialPackageUpdateReviewModel& review);

} // namespace mirakana::editor
