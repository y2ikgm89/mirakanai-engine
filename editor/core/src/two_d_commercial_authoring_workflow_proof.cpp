// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/two_d_commercial_authoring_workflow_proof.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

constexpr std::string_view kRootId = "2d_commercial_authoring_workflow_proof";

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

void append_unique(std::vector<std::string>& values, std::string value) {
    if (value.empty()) {
        return;
    }
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void push_diagnostic(Editor2DCommercialAuthoringWorkflowProofModel& model, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_claim(Editor2DCommercialAuthoringWorkflowProofModel& model, std::string claim, std::string diagnostic) {
    append_unique(model.unsupported_claims, std::move(claim));
    push_diagnostic(model, std::move(diagnostic));
}

[[nodiscard]] bool is_game_owned_path(std::string_view path) {
    constexpr std::string_view prefix = "games/";
    if (!path.starts_with(prefix) || path.ends_with("/") || path.find('\\') != std::string_view::npos ||
        path.find("..") != std::string_view::npos || path.find("//") != std::string_view::npos ||
        path.find(';') != std::string_view::npos) {
        return false;
    }

    const auto remaining = path.substr(prefix.size());
    const auto separator = remaining.find('/');
    if (separator == std::string_view::npos || separator == 0U || separator + 1U == remaining.size()) {
        return false;
    }

    const auto game_name = remaining.substr(0, separator);
    return std::ranges::all_of(game_name, [](const unsigned char character) {
        return std::islower(character) != 0 || std::isdigit(character) != 0 || character == '_';
    });
}

[[nodiscard]] bool is_allowed_source_origin(Editor2DCommercialAuthoringWorkflowProofSourceOrigin origin) noexcept {
    return origin == Editor2DCommercialAuthoringWorkflowProofSourceOrigin::first_party_sample ||
           origin == Editor2DCommercialAuthoringWorkflowProofSourceOrigin::generated_fixture;
}

[[nodiscard]] std::string row_id_for(std::string_view id, std::size_t index) {
    std::string suffix{id};
    if (suffix.empty()) {
        suffix = std::to_string(index);
    }
    return std::string(kRootId) + ".source." + suffix;
}

[[nodiscard]] Editor2DCommercialAuthoringWorkflowProofStatus
aggregate_status(const Editor2DCommercialAuthoringWorkflowProofModel& model) noexcept {
    if (model.has_blocking_diagnostics || std::ranges::any_of(model.source_rows, [](const auto& row) {
            return row.status == Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
        })) {
        return Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
    }
    if (model.has_host_gates || std::ranges::any_of(model.source_rows, [](const auto& row) {
            return row.status == Editor2DCommercialAuthoringWorkflowProofStatus::host_gated;
        })) {
        return Editor2DCommercialAuthoringWorkflowProofStatus::host_gated;
    }
    return Editor2DCommercialAuthoringWorkflowProofStatus::ready;
}

void validate_requested_scope(Editor2DCommercialAuthoringWorkflowProofModel& model,
                              const Editor2DCommercialAuthoringWorkflowProofDesc& desc) {
    if (desc.request_file_mutation) {
        reject_claim(model, "file mutation",
                     "2D commercial authoring workflow proof is review evidence and does not mutate files");
    }
    if (desc.request_package_io) {
        reject_claim(model, "package IO",
                     "2D commercial authoring workflow proof does not read or write cooked package payloads");
    }
    if (desc.request_command_execution) {
        reject_claim(model, "command execution",
                     "2D commercial authoring workflow proof does not execute commands or validation recipes");
    }
    if (desc.request_runtime_source_parsing) {
        reject_claim(model, "runtime source parsing",
                     "2D commercial authoring workflow proof keeps runtime source parsing out of the claim");
    }
    if (desc.request_native_handle_exposure) {
        reject_claim(model, "native handle exposure",
                     "2D commercial authoring workflow proof must not expose native handles");
    }
    if (desc.request_external_engine_project_import) {
        reject_claim(model, "external engine project import",
                     "2D commercial authoring workflow proof does not import external engine projects");
    }
    if (desc.request_external_engine_api_parity_claim) {
        reject_claim(model, "external engine API parity",
                     "2D commercial authoring workflow proof does not claim external engine API parity");
    }
    if (desc.request_legal_approval_claim) {
        reject_claim(model, "legal approval",
                     "2D commercial authoring workflow proof is engineering evidence, not legal approval");
    }
}

void validate_authoring_review(Editor2DCommercialAuthoringWorkflowProofModel& model,
                               const Editor2DCommercialAuthoringReviewModel& authoring_review) {
    if (authoring_review.has_host_gates ||
        authoring_review.status == Editor2DCommercialAuthoringReviewStatus::host_gated) {
        model.has_host_gates = true;
    }
    if (!authoring_review.ready_for_authoring_review) {
        if (authoring_review.status == Editor2DCommercialAuthoringReviewStatus::host_gated) {
            return;
        }
        push_diagnostic(model, "2D commercial authoring review must be ready before workflow proof");
    }
    if (authoring_review.mutates || authoring_review.executes || authoring_review.exposes_native_handles) {
        push_diagnostic(model, "2D commercial authoring review input must stay value-only");
    }
}

void validate_package_update_review(Editor2DCommercialAuthoringWorkflowProofModel& model,
                                    const Editor2DCommercialPackageUpdateReviewModel& package_update_review) {
    if (package_update_review.has_host_gates ||
        package_update_review.status == Editor2DCommercialPackageUpdateReviewStatus::host_gated) {
        model.has_host_gates = true;
    }
    if (!package_update_review.ready_for_reviewed_apply) {
        if (package_update_review.status == Editor2DCommercialPackageUpdateReviewStatus::host_gated) {
            return;
        }
        push_diagnostic(model, "2D commercial package update review must be ready before workflow proof");
    }
    if (!package_update_review.revision_checked || !package_update_review.undo_redo_safe ||
        !package_update_review.safe_package_mutation_preview_available ||
        !package_update_review.selected_package_smoke_reviewed || !package_update_review.rejection_diagnostics_ready) {
        push_diagnostic(model, "2D commercial package update review is missing required safety evidence");
    }
    if (package_update_review.mutates || package_update_review.executes ||
        package_update_review.exposes_native_handles) {
        push_diagnostic(model, "2D commercial package update review input must stay value-only");
    }
}

void validate_asset_navigation(Editor2DCommercialAuthoringWorkflowProofModel& model,
                               const ContentBrowserNavigationModel& navigation) {
    model.navigation_visible_item_count = navigation.visible_item_count;
    model.navigation_page_count = navigation.page_count;
    if (navigation.mutates || navigation.executes) {
        push_diagnostic(model, "asset navigation proof must not mutate state or execute import tools");
    }
    if (navigation.visible_item_count == 0U || navigation.rows.empty() || navigation.page_count == 0U) {
        push_diagnostic(model, "asset navigation proof requires visible rows from selected first-party content");
    }
    if (!navigation.diagnostics.empty()) {
        push_diagnostic(model, "asset navigation proof must use an unclamped diagnostic-free request");
    }
}

void validate_production_workflow(Editor2DCommercialAuthoringWorkflowProofModel& model,
                                  const ProductionAuthoringWorkflowReviewResult& workflow_review) {
    model.accepted_workflow_count = workflow_review.accepted_rows.size();
    model.mutation_ledger_count = workflow_review.mutation_ledger_rows.size();
    model.validation_repair_workflow_count = workflow_review.validation_repair_rows.size();
    if (!workflow_review.diagnostics.empty()) {
        push_diagnostic(model, "production authoring workflow diagnostics must be resolved before proof");
    }
    if (workflow_review.accepted_rows.empty()) {
        push_diagnostic(model, "production authoring workflow proof requires accepted workflow rows");
    }
    if (workflow_review.mutation_ledger_rows.empty()) {
        push_diagnostic(model, "production authoring workflow proof requires mutation-ledger review rows");
    }
    if (workflow_review.validation_repair_rows.empty()) {
        push_diagnostic(model, "production authoring workflow proof requires a validation-repair row");
    }
    if (workflow_review.invoked_file_mutation) {
        reject_claim(model, "file mutation", "production authoring workflow proof must not invoke file mutation");
    }
    if (workflow_review.invoked_package_io) {
        reject_claim(model, "package IO", "production authoring workflow proof must not invoke package IO");
    }
    if (workflow_review.invoked_command_execution) {
        reject_claim(model, "command execution",
                     "production authoring workflow proof must not invoke command execution");
    }
}

void add_source_rows(Editor2DCommercialAuthoringWorkflowProofModel& model,
                     const std::vector<Editor2DCommercialAuthoringWorkflowProofSourceRowInput>& rows) {
    if (rows.empty()) {
        push_diagnostic(model, "authoring workflow proof requires first-party source rows");
        return;
    }

    std::unordered_set<std::string> seen_ids;
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& input = rows[index];
        Editor2DCommercialAuthoringWorkflowProofSourceRow row{
            .id = row_id_for(input.id, index),
            .origin = input.origin,
            .status = Editor2DCommercialAuthoringWorkflowProofStatus::ready,
            .status_label = {},
            .origin_label =
                std::string(editor_2d_commercial_authoring_workflow_proof_source_origin_label(input.origin)),
            .path = sanitize_text(input.path),
            .evidence_id = sanitize_text(input.evidence_id),
            .diagnostic = sanitize_text(input.diagnostic),
        };

        if (!seen_ids.insert(row.id).second) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "duplicate authoring workflow proof source row id");
        }
        if (!is_allowed_source_origin(input.origin)) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "source origin must be first-party sample or generated fixture");
        }
        if (input.origin == Editor2DCommercialAuthoringWorkflowProofSourceOrigin::first_party_sample) {
            ++model.first_party_sample_source_count;
        } else if (input.origin == Editor2DCommercialAuthoringWorkflowProofSourceOrigin::generated_fixture) {
            ++model.generated_fixture_source_count;
        }
        if (!is_game_owned_path(input.path)) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "source path must be game-owned and repo-relative");
        }
        if (input.evidence_id.empty()) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "source evidence id is required");
        }
        if (input.copied_external_material) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "source row must not copy external material");
        }
        if (input.uses_external_engine_schema) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            push_diagnostic(model, "source row must not use external engine schema");
        }
        if (input.claims_legal_approval) {
            row.status = Editor2DCommercialAuthoringWorkflowProofStatus::blocked;
            reject_claim(model, "legal approval", "source rows must not claim legal approval");
        }
        row.status_label = std::string(editor_2d_commercial_authoring_workflow_proof_status_label(row.status));
        model.source_rows.push_back(std::move(row));
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("2D commercial authoring workflow proof ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    auto desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }
    std::string joined;
    for (const auto& value : values) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += value;
    }
    return joined;
}

} // namespace

std::string_view editor_2d_commercial_authoring_workflow_proof_status_label(
    Editor2DCommercialAuthoringWorkflowProofStatus status) noexcept {
    switch (status) {
    case Editor2DCommercialAuthoringWorkflowProofStatus::ready:
        return "ready";
    case Editor2DCommercialAuthoringWorkflowProofStatus::blocked:
        return "blocked";
    case Editor2DCommercialAuthoringWorkflowProofStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

std::string_view editor_2d_commercial_authoring_workflow_proof_source_origin_label(
    Editor2DCommercialAuthoringWorkflowProofSourceOrigin origin) noexcept {
    switch (origin) {
    case Editor2DCommercialAuthoringWorkflowProofSourceOrigin::first_party_sample:
        return "first_party_sample";
    case Editor2DCommercialAuthoringWorkflowProofSourceOrigin::generated_fixture:
        return "generated_fixture";
    case Editor2DCommercialAuthoringWorkflowProofSourceOrigin::external_asset:
        return "external_asset";
    case Editor2DCommercialAuthoringWorkflowProofSourceOrigin::external_engine_project:
        return "external_engine_project";
    case Editor2DCommercialAuthoringWorkflowProofSourceOrigin::unknown:
        return "unknown";
    }
    return "unknown";
}

Editor2DCommercialAuthoringWorkflowProofModel
make_editor_2d_commercial_authoring_workflow_proof_model(const Editor2DCommercialAuthoringWorkflowProofDesc& desc) {
    Editor2DCommercialAuthoringWorkflowProofModel model;
    model.mutates = false;
    model.executes = false;
    model.exposes_native_handles = false;

    validate_requested_scope(model, desc);
    validate_authoring_review(model, desc.authoring_review);
    validate_package_update_review(model, desc.package_update_review);
    validate_asset_navigation(model, desc.asset_navigation);
    validate_production_workflow(model, desc.production_workflow_review);
    add_source_rows(model, desc.source_rows);

    model.status = aggregate_status(model);
    model.status_label = std::string(editor_2d_commercial_authoring_workflow_proof_status_label(model.status));
    model.ready_for_first_party_authoring_workflow =
        model.status == Editor2DCommercialAuthoringWorkflowProofStatus::ready;
    return model;
}

mirakana::ui::UiDocument make_editor_2d_commercial_authoring_workflow_proof_ui_model(
    const Editor2DCommercialAuthoringWorkflowProofModel& model) {
    mirakana::ui::UiDocument document;
    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{std::string(kRootId)};
    root.role = mirakana::ui::SemanticRole::panel;
    root.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    add_or_throw(document, std::move(root));

    const mirakana::ui::ElementId root_id{std::string(kRootId)};
    append_label(document, root_id, std::string(kRootId) + ".status", model.status_label);
    append_label(document, root_id, std::string(kRootId) + ".ready",
                 model.ready_for_first_party_authoring_workflow ? "ready" : "not_ready");
    append_label(document, root_id, std::string(kRootId) + ".accepted_workflows",
                 std::to_string(model.accepted_workflow_count));
    append_label(document, root_id, std::string(kRootId) + ".mutation_ledger",
                 std::to_string(model.mutation_ledger_count));
    append_label(document, root_id, std::string(kRootId) + ".validation_repairs",
                 std::to_string(model.validation_repair_workflow_count));
    append_label(document, root_id, std::string(kRootId) + ".navigation_visible_items",
                 std::to_string(model.navigation_visible_item_count));
    append_label(document, root_id, std::string(kRootId) + ".unsupported_claims",
                 join_values(model.unsupported_claims));
    append_label(document, root_id, std::string(kRootId) + ".diagnostics", join_values(model.diagnostics));

    for (const auto& row : model.source_rows) {
        const mirakana::ui::ElementId row_id{row.id};
        auto item = make_child(row.id, root_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.origin_label);
        add_or_throw(document, std::move(item));
        append_label(document, row_id, row.id + ".status", row.status_label);
        append_label(document, row_id, row.id + ".origin", row.origin_label);
        append_label(document, row_id, row.id + ".path", row.path);
        append_label(document, row_id, row.id + ".evidence", row.evidence_id);
        append_label(document, row_id, row.id + ".diagnostic", row.diagnostic);
    }

    return document;
}

} // namespace mirakana::editor
