// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/content_browser.hpp"
#include "mirakana/editor/two_d_commercial_authoring_review.hpp"
#include "mirakana/editor/two_d_commercial_package_update_review.hpp"
#include "mirakana/tools/production_authoring_workflows.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class Editor2DCommercialAuthoringWorkflowProofStatus : std::uint8_t { ready, blocked, host_gated };

enum class Editor2DCommercialAuthoringWorkflowProofSourceOrigin : std::uint8_t {
    first_party_sample,
    generated_fixture,
    external_asset,
    external_engine_project,
    unknown,
};

struct Editor2DCommercialAuthoringWorkflowProofSourceRowInput {
    std::string id;
    Editor2DCommercialAuthoringWorkflowProofSourceOrigin origin{
        Editor2DCommercialAuthoringWorkflowProofSourceOrigin::unknown};
    std::string path;
    std::string evidence_id;
    std::string diagnostic;
    bool copied_external_material{false};
    bool uses_external_engine_schema{false};
    bool claims_legal_approval{false};
};

struct Editor2DCommercialAuthoringWorkflowProofSourceRow {
    std::string id;
    Editor2DCommercialAuthoringWorkflowProofSourceOrigin origin{
        Editor2DCommercialAuthoringWorkflowProofSourceOrigin::unknown};
    Editor2DCommercialAuthoringWorkflowProofStatus status{Editor2DCommercialAuthoringWorkflowProofStatus::blocked};
    std::string status_label;
    std::string origin_label;
    std::string path;
    std::string evidence_id;
    std::string diagnostic;
};

struct Editor2DCommercialAuthoringWorkflowProofDesc {
    Editor2DCommercialAuthoringReviewModel authoring_review;
    Editor2DCommercialPackageUpdateReviewModel package_update_review;
    ContentBrowserNavigationModel asset_navigation;
    ProductionAuthoringWorkflowReviewResult production_workflow_review;
    std::vector<Editor2DCommercialAuthoringWorkflowProofSourceRowInput> source_rows;
    bool request_file_mutation{false};
    bool request_package_io{false};
    bool request_command_execution{false};
    bool request_runtime_source_parsing{false};
    bool request_native_handle_exposure{false};
    bool request_external_engine_project_import{false};
    bool request_external_engine_api_parity_claim{false};
    bool request_legal_approval_claim{false};
};

struct Editor2DCommercialAuthoringWorkflowProofModel {
    std::vector<Editor2DCommercialAuthoringWorkflowProofSourceRow> source_rows;
    Editor2DCommercialAuthoringWorkflowProofStatus status{Editor2DCommercialAuthoringWorkflowProofStatus::blocked};
    std::string status_label;
    bool ready_for_first_party_authoring_workflow{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
    std::size_t first_party_sample_source_count{0U};
    std::size_t generated_fixture_source_count{0U};
    std::size_t accepted_workflow_count{0U};
    std::size_t mutation_ledger_count{0U};
    std::size_t validation_repair_workflow_count{0U};
    std::size_t navigation_visible_item_count{0U};
    std::size_t navigation_page_count{0U};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view editor_2d_commercial_authoring_workflow_proof_status_label(
    Editor2DCommercialAuthoringWorkflowProofStatus status) noexcept;

[[nodiscard]] std::string_view editor_2d_commercial_authoring_workflow_proof_source_origin_label(
    Editor2DCommercialAuthoringWorkflowProofSourceOrigin origin) noexcept;

[[nodiscard]] Editor2DCommercialAuthoringWorkflowProofModel
make_editor_2d_commercial_authoring_workflow_proof_model(const Editor2DCommercialAuthoringWorkflowProofDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_2d_commercial_authoring_workflow_proof_ui_model(const Editor2DCommercialAuthoringWorkflowProofModel& model);

} // namespace mirakana::editor
