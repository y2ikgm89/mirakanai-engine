// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/environment_authoring.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EnvironmentArtistWorkflowProductionRequirementKind : std::uint8_t {
    import_openexr = 0,
    import_ktx2_basis,
    import_gltf_material,
    review_usd_materialx_ocio,
    cook_package,
    live_preview_d3d12,
    live_preview_vulkan,
    live_preview_metal_host,
    weather_timeline_edit,
    preset_batch_apply,
    validation_report,
    profiler_artifact_review,
    undo_redo_revision_safety,
    operator_review,
};

enum class EnvironmentArtistWorkflowProductionRequirementStatus : std::uint8_t {
    ready = 0,
    missing,
    blocked,
};

enum class EnvironmentArtistWorkflowProductionCloseoutStatus : std::uint8_t {
    blocked = 0,
    ready,
};

struct EnvironmentArtistWorkflowProductionRequirementInputRow {
    EnvironmentArtistWorkflowProductionRequirementKind kind{
        EnvironmentArtistWorkflowProductionRequirementKind::import_openexr};
    std::string evidence_id;
    bool ready{false};
    bool reviewed{false};
    bool package_visible{false};
    bool host_validated{false};
    std::string validation_recipe_id;
    std::vector<std::string> retained_ui_row_ids;
};

struct EnvironmentArtistWorkflowProductionCloseoutDesc {
    EnvironmentArtistWorkflowReadyReviewModel selected_package_workflow;
    std::vector<EnvironmentArtistWorkflowProductionRequirementInputRow> requirements;
    bool request_environment_artist_workflow_production_ready{false};
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_validation_recipe_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentArtistWorkflowProductionRequirementRow {
    std::string row_id;
    std::string label;
    EnvironmentArtistWorkflowProductionRequirementKind kind{
        EnvironmentArtistWorkflowProductionRequirementKind::import_openexr};
    EnvironmentArtistWorkflowProductionRequirementStatus status{
        EnvironmentArtistWorkflowProductionRequirementStatus::missing};
    std::string evidence_id;
    bool reviewed{false};
    bool package_visible{false};
    bool host_validated{false};
    std::string validation_recipe_id;
    std::vector<std::string> retained_ui_row_ids;
    std::vector<std::string> blocked_by;
    std::string diagnostic;
};

struct EnvironmentArtistWorkflowProductionCloseoutModel {
    EnvironmentArtistWorkflowProductionCloseoutStatus status{
        EnvironmentArtistWorkflowProductionCloseoutStatus::blocked};
    std::vector<EnvironmentArtistWorkflowProductionRequirementRow> rows;
    std::size_t required_rows{0U};
    std::size_t ready_rows{0U};
    bool selected_package_workflow_ready{false};
    bool environment_artist_workflow_production_ready{false};
    bool workflow_import_openexr_ready{false};
    bool workflow_import_ktx2_basis_ready{false};
    bool workflow_import_gltf_material_ready{false};
    bool workflow_review_usd_materialx_ocio_ready{false};
    bool workflow_cook_package_ready{false};
    bool workflow_live_preview_d3d12_ready{false};
    bool workflow_live_preview_vulkan_ready{false};
    bool workflow_live_preview_metal_host_ready{false};
    bool workflow_weather_timeline_edit_ready{false};
    bool workflow_preset_batch_apply_ready{false};
    bool workflow_validation_report_ready{false};
    bool workflow_profiler_artifact_review_ready{false};
    bool workflow_undo_redo_revision_safety_ready{false};
    bool workflow_operator_review_ready{false};
    bool invokes_backend{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view
environment_artist_workflow_production_requirement_id(EnvironmentArtistWorkflowProductionRequirementKind kind) noexcept;
[[nodiscard]] std::string_view environment_artist_workflow_production_requirement_status_label(
    EnvironmentArtistWorkflowProductionRequirementStatus status) noexcept;
[[nodiscard]] std::string_view environment_artist_workflow_production_closeout_status_label(
    EnvironmentArtistWorkflowProductionCloseoutStatus status) noexcept;

[[nodiscard]] EnvironmentArtistWorkflowProductionCloseoutModel
evaluate_environment_artist_workflow_production_closeout(const EnvironmentArtistWorkflowProductionCloseoutDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument make_environment_artist_workflow_production_closeout_ui_model(
    const EnvironmentArtistWorkflowProductionCloseoutModel& model);

} // namespace mirakana::editor
