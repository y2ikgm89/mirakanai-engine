// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/playtest_package_review.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class Editor2DCommercialAuthoringReviewStatus : std::uint8_t { ready, blocked, host_gated };

enum class Editor2DCommercialAuthoringReviewSurface : std::uint8_t {
    atlas_inspection,
    tile_set_review,
    tilemap_chunk_editing,
    collision_overlay_review,
    animation_preview,
    package_diff_review,
    validation_diagnostics,
};

struct Editor2DCommercialAuthoringReviewRowInput {
    std::string id;
    Editor2DCommercialAuthoringReviewSurface surface{Editor2DCommercialAuthoringReviewSurface::atlas_inspection};
    Editor2DCommercialAuthoringReviewStatus status{Editor2DCommercialAuthoringReviewStatus::blocked};
    std::string source_model;
    std::size_t source_row_count{0};
    std::vector<std::string> retained_ui_row_ids;
    std::vector<std::string> host_gates;
    std::string diagnostic;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
};

struct Editor2DCommercialAuthoringReviewRow {
    std::string id;
    Editor2DCommercialAuthoringReviewSurface surface{Editor2DCommercialAuthoringReviewSurface::atlas_inspection};
    Editor2DCommercialAuthoringReviewStatus status{Editor2DCommercialAuthoringReviewStatus::blocked};
    std::string status_label;
    std::string source_model;
    std::size_t source_row_count{0};
    std::vector<std::string> retained_ui_row_ids;
    std::vector<std::string> host_gates;
    std::string diagnostic;
};

struct Editor2DCommercialAuthoringReviewDesc {
    EditorTilemapPackageDiagnosticsModel tilemap_diagnostics;
    EditorSpritePreviewDiagnosticsModel sprite_preview_diagnostics;
    EditorAiValidationRecipePreflightModel validation_preflight;
    std::vector<Editor2DCommercialAuthoringReviewRowInput> review_rows;
    bool request_package_mutation{false};
    bool request_validation_execution{false};
    bool request_runtime_source_parsing{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_native_handle_exposure{false};
    bool request_external_engine_project_import{false};
    bool request_external_engine_api_parity_claim{false};
};

struct Editor2DCommercialAuthoringReviewModel {
    std::vector<Editor2DCommercialAuthoringReviewRow> rows;
    Editor2DCommercialAuthoringReviewStatus status{Editor2DCommercialAuthoringReviewStatus::blocked};
    std::string status_label;
    bool ready_for_authoring_review{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view
editor_2d_commercial_authoring_review_status_label(Editor2DCommercialAuthoringReviewStatus status) noexcept;

[[nodiscard]] std::string_view
editor_2d_commercial_authoring_review_surface_id(Editor2DCommercialAuthoringReviewSurface surface) noexcept;

[[nodiscard]] Editor2DCommercialAuthoringReviewModel
make_editor_2d_commercial_authoring_review_model(const Editor2DCommercialAuthoringReviewDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_2d_commercial_authoring_review_ui_model(const Editor2DCommercialAuthoringReviewModel& model);

} // namespace mirakana::editor
