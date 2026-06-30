// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/two_d_commercial_package_update_review.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

constexpr std::string_view kRootId = "2d_commercial_package_update_review";

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

[[nodiscard]] std::string sanitize_id(std::string_view value, std::string_view fallback) {
    std::string id;
    id.reserve(value.empty() ? fallback.size() : value.size());
    const auto source = value.empty() ? fallback : value;
    for (const auto character : source) {
        const auto ch = static_cast<unsigned char>(character);
        if (std::isalnum(ch) != 0 || character == '_' || character == '-' || character == '.') {
            id.push_back(static_cast<char>(character));
        } else {
            id.push_back('_');
        }
    }
    return id.empty() ? std::string(fallback) : id;
}

[[nodiscard]] bool is_absolute_like(std::string_view path) noexcept {
    return path.starts_with('/') || path.starts_with('\\') ||
           (path.size() >= 2U && path[1] == ':' &&
            ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')));
}

[[nodiscard]] bool has_parent_segment(std::string_view path) noexcept {
    std::size_t begin = 0U;
    while (begin <= path.size()) {
        const auto separator = path.find_first_of("/\\", begin);
        const auto end = separator == std::string_view::npos ? path.size() : separator;
        if (path.substr(begin, end - begin) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return false;
}

[[nodiscard]] bool is_safe_relative_path(std::string_view path) noexcept {
    return !path.empty() && path.find_first_of("\r\n=") == std::string_view::npos && !is_absolute_like(path) &&
           !has_parent_segment(path);
}

void append_unique(std::vector<std::string>& values, std::string value) {
    if (value.empty()) {
        return;
    }
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void append_many_unique(std::vector<std::string>& destination, const std::vector<std::string>& source) {
    for (const auto& value : source) {
        append_unique(destination, sanitize_text(value));
    }
}

void push_diagnostic(Editor2DCommercialPackageUpdateReviewModel& model, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_claim(Editor2DCommercialPackageUpdateReviewModel& model, std::string claim, std::string diagnostic) {
    append_unique(model.unsupported_claims, std::move(claim));
    push_diagnostic(model, std::move(diagnostic));
}

[[nodiscard]] Editor2DCommercialPackageUpdateReviewStatus
aggregate_status(const Editor2DCommercialPackageUpdateReviewModel& model) noexcept {
    if (model.has_blocking_diagnostics ||
        std::ranges::any_of(
            model.preview_rows,
            [](const auto& row) { return row.status == Editor2DCommercialPackageUpdateReviewStatus::blocked; }) ||
        std::ranges::any_of(model.smoke_reviews,
                            [](const auto& row) {
                                return row.status == Editor2DCommercialPackageSmokeReviewStatus::failed ||
                                       row.status == Editor2DCommercialPackageSmokeReviewStatus::blocked ||
                                       row.status == Editor2DCommercialPackageSmokeReviewStatus::missing;
                            }) ||
        std::ranges::any_of(model.rejection_diagnostics, [](const auto& row) { return row.blocks_apply; })) {
        return Editor2DCommercialPackageUpdateReviewStatus::blocked;
    }
    if (model.has_host_gates ||
        std::ranges::any_of(
            model.preview_rows,
            [](const auto& row) { return row.status == Editor2DCommercialPackageUpdateReviewStatus::host_gated; }) ||
        std::ranges::any_of(model.smoke_reviews, [](const auto& row) {
            return row.status == Editor2DCommercialPackageSmokeReviewStatus::host_gated;
        })) {
        return Editor2DCommercialPackageUpdateReviewStatus::host_gated;
    }
    return Editor2DCommercialPackageUpdateReviewStatus::ready;
}

void validate_unsupported_claims(Editor2DCommercialPackageUpdateReviewModel& model,
                                 const Editor2DCommercialPackageUpdateReviewDesc& desc) {
    if (desc.request_package_script_execution) {
        reject_claim(model, "package script execution",
                     "2D commercial package update review must not execute package scripts");
    }
    if (desc.request_validation_execution) {
        reject_claim(model, "validation execution",
                     "2D commercial package update review imports host evidence and does not execute validation");
    }
    if (desc.request_runtime_source_parsing) {
        reject_claim(model, "runtime source parsing",
                     "2D commercial package update review keeps runtime source parsing unsupported");
    }
    if (desc.request_native_handle_exposure) {
        reject_claim(model, "native handle exposure",
                     "2D commercial package update review must not expose native handles");
    }
    if (desc.request_external_engine_project_import) {
        reject_claim(model, "external engine project import",
                     "2D commercial package update review does not import external engine projects");
    }
    if (desc.request_external_engine_api_parity_claim) {
        reject_claim(model, "external engine API parity",
                     "2D commercial package update review does not claim external engine API parity");
    }
}

void add_preview_rows(Editor2DCommercialPackageUpdateReviewModel& model,
                      const ScenePackageRegistrationApplyPlan& plan) {
    if (!is_safe_relative_path(plan.game_manifest_path)) {
        push_diagnostic(model, "unsafe game manifest path for package update preview");
    }
    if (!plan.can_apply) {
        push_diagnostic(model, plan.diagnostic.empty() ? "package update apply plan is not ready" : plan.diagnostic);
    }
    if (plan.runtime_package_files.empty()) {
        push_diagnostic(model, "package update preview requires runtimePackageFiles entries");
    }

    model.preview_rows.reserve(plan.runtime_package_files.size());
    for (std::size_t index = 0U; index < plan.runtime_package_files.size(); ++index) {
        const auto& path = plan.runtime_package_files[index];
        const bool safe = is_safe_relative_path(path);
        if (!safe) {
            push_diagnostic(model, "unsafe runtimePackageFiles preview path: " + sanitize_text(path));
        }
        const auto status = safe ? Editor2DCommercialPackageUpdateReviewStatus::ready
                                 : Editor2DCommercialPackageUpdateReviewStatus::blocked;
        model.preview_rows.push_back(Editor2DCommercialPackageUpdatePreviewRow{
            .id = std::string(kRootId) + ".preview." + std::to_string(index),
            .runtime_package_path = sanitize_text(path),
            .status = status,
            .status_label = std::string(editor_2d_commercial_package_update_review_status_label(status)),
            .diagnostic =
                safe ? "safe runtimePackageFiles mutation preview" : "unsafe runtimePackageFiles mutation preview path",
            .will_add = safe,
        });
    }

    model.safe_package_mutation_preview_available =
        plan.can_apply && !model.preview_rows.empty() && std::ranges::all_of(model.preview_rows, [](const auto& row) {
            return row.status == Editor2DCommercialPackageUpdateReviewStatus::ready && row.will_add;
        });
}

void validate_revision(Editor2DCommercialPackageUpdateReviewModel& model,
                       const Editor2DCommercialPackageUpdateReviewDesc& desc) {
    if (desc.expected_manifest_revision == 0U) {
        push_diagnostic(model, "manifest revision is required before package update apply");
        return;
    }
    if (desc.expected_manifest_revision != desc.active_manifest_revision) {
        push_diagnostic(model, "stale manifest revision: expected " + std::to_string(desc.expected_manifest_revision) +
                                   " active " + std::to_string(desc.active_manifest_revision));
        return;
    }
    model.revision_checked = true;
    model.undo_redo_safe = true;
}

void add_smoke_reviews(Editor2DCommercialPackageUpdateReviewModel& model,
                       const Editor2DCommercialPackageUpdateReviewDesc& desc) {
    bool selected_found = false;
    bool selected_passed = false;
    model.smoke_reviews.reserve(desc.smoke_reviews.size());

    for (std::size_t index = 0U; index < desc.smoke_reviews.size(); ++index) {
        const auto& input = desc.smoke_reviews[index];
        const std::string input_id = sanitize_id(input.id, "smoke_" + std::to_string(index));
        auto status = input.status;
        std::vector<std::string> host_gates;
        append_many_unique(host_gates, input.host_gates);

        if (input.recipe_id.empty()) {
            status = Editor2DCommercialPackageSmokeReviewStatus::blocked;
            push_diagnostic(model, "selected package smoke review recipe id is required");
        }
        if (input.mutates) {
            status = Editor2DCommercialPackageSmokeReviewStatus::blocked;
            push_diagnostic(model, "selected package smoke review must not mutate packages");
        }
        if (input.executes) {
            status = Editor2DCommercialPackageSmokeReviewStatus::blocked;
            push_diagnostic(model, "selected package smoke review must import evidence and not execute validation");
        }
        if (input.exposes_native_handles) {
            status = Editor2DCommercialPackageSmokeReviewStatus::blocked;
            push_diagnostic(model, "selected package smoke review must not expose native handles");
        }
        if (status == Editor2DCommercialPackageSmokeReviewStatus::host_gated || !host_gates.empty()) {
            model.has_host_gates = true;
        }

        const bool selected = input.id == desc.selected_smoke_review_id;
        if (selected) {
            selected_found = true;
            selected_passed = status == Editor2DCommercialPackageSmokeReviewStatus::passed;
            if (!selected_passed) {
                push_diagnostic(model, "selected package smoke review must pass before package update apply");
            }
        }

        model.smoke_reviews.push_back(Editor2DCommercialPackageUpdateSmokeReviewRow{
            .id = std::string(kRootId) + ".smoke." + input_id,
            .recipe_id = sanitize_text(input.recipe_id),
            .status = status,
            .status_label = std::string(editor_2d_commercial_package_smoke_review_status_label(status)),
            .host_gates = std::move(host_gates),
            .diagnostic = sanitize_text(input.diagnostic),
        });
    }

    if (desc.selected_smoke_review_id.empty()) {
        push_diagnostic(model, "selected package smoke review id is required");
    } else if (!selected_found) {
        push_diagnostic(model, "selected package smoke review was not found");
    }
    model.selected_package_smoke_reviewed = selected_found && selected_passed;
}

void add_rejection_diagnostics(Editor2DCommercialPackageUpdateReviewModel& model,
                               const Editor2DCommercialPackageUpdateReviewDesc& desc) {
    bool ready = true;
    model.rejection_diagnostics.reserve(desc.rejection_diagnostics.size());
    for (std::size_t index = 0U; index < desc.rejection_diagnostics.size(); ++index) {
        const auto& input = desc.rejection_diagnostics[index];
        const std::string input_id = sanitize_id(input.id, "rejection_" + std::to_string(index));
        bool blocks_apply = input.blocks_apply;
        if (input.diagnostic.empty()) {
            ready = false;
            blocks_apply = true;
            push_diagnostic(model, "rejection diagnostic is required for " + sanitize_text(input.candidate_path));
        }
        if (input.status_label.empty()) {
            ready = false;
            blocks_apply = true;
            push_diagnostic(model, "rejection status label is required for " + sanitize_text(input.candidate_path));
        }
        if (input.blocks_apply) {
            ready = false;
            push_diagnostic(model, "package rejection blocks apply for " + sanitize_text(input.candidate_path));
        }

        model.rejection_diagnostics.push_back(Editor2DCommercialPackageUpdateRejectionDiagnosticRow{
            .id = std::string(kRootId) + ".rejection." + input_id,
            .candidate_path = sanitize_text(input.candidate_path),
            .status_label = sanitize_text(input.status_label),
            .diagnostic = sanitize_text(input.diagnostic),
            .blocks_apply = blocks_apply,
        });
    }
    model.rejection_diagnostics_ready = ready;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("2D commercial package update review ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
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
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
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

[[nodiscard]] bool review_matches_plan(const Editor2DCommercialPackageUpdateReviewModel& review,
                                       const ScenePackageRegistrationApplyPlan& plan) {
    if (review.game_manifest_path != sanitize_text(plan.game_manifest_path) ||
        review.preview_rows.size() != plan.runtime_package_files.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < plan.runtime_package_files.size(); ++index) {
        const auto& row = review.preview_rows[index];
        if (row.runtime_package_path != sanitize_text(plan.runtime_package_files[index]) || !row.will_add ||
            row.status != Editor2DCommercialPackageUpdateReviewStatus::ready) {
            return false;
        }
    }
    return true;
}

} // namespace

std::string_view
editor_2d_commercial_package_update_review_status_label(Editor2DCommercialPackageUpdateReviewStatus status) noexcept {
    switch (status) {
    case Editor2DCommercialPackageUpdateReviewStatus::ready:
        return "ready";
    case Editor2DCommercialPackageUpdateReviewStatus::blocked:
        return "blocked";
    case Editor2DCommercialPackageUpdateReviewStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

std::string_view
editor_2d_commercial_package_smoke_review_status_label(Editor2DCommercialPackageSmokeReviewStatus status) noexcept {
    switch (status) {
    case Editor2DCommercialPackageSmokeReviewStatus::passed:
        return "passed";
    case Editor2DCommercialPackageSmokeReviewStatus::failed:
        return "failed";
    case Editor2DCommercialPackageSmokeReviewStatus::blocked:
        return "blocked";
    case Editor2DCommercialPackageSmokeReviewStatus::host_gated:
        return "host_gated";
    case Editor2DCommercialPackageSmokeReviewStatus::missing:
        return "missing";
    }
    return "unknown";
}

Editor2DCommercialPackageUpdateReviewModel
make_editor_2d_commercial_package_update_review_model(const Editor2DCommercialPackageUpdateReviewDesc& desc) {
    Editor2DCommercialPackageUpdateReviewModel model;
    model.game_manifest_path = sanitize_text(desc.apply_plan.game_manifest_path);
    model.expected_manifest_revision = desc.expected_manifest_revision;
    model.active_manifest_revision = desc.active_manifest_revision;
    model.undo_stack_count = desc.undo_stack_count;
    model.redo_stack_count = desc.redo_stack_count;
    model.mutates = false;
    model.executes = false;
    model.exposes_native_handles = false;

    validate_unsupported_claims(model, desc);
    add_preview_rows(model, desc.apply_plan);
    validate_revision(model, desc);
    add_smoke_reviews(model, desc);
    add_rejection_diagnostics(model, desc);

    model.status = aggregate_status(model);
    model.status_label = std::string(editor_2d_commercial_package_update_review_status_label(model.status));
    model.ready_for_reviewed_apply = model.status == Editor2DCommercialPackageUpdateReviewStatus::ready &&
                                     model.revision_checked && model.undo_redo_safe &&
                                     model.safe_package_mutation_preview_available &&
                                     model.selected_package_smoke_reviewed && model.rejection_diagnostics_ready;
    return model;
}

mirakana::ui::UiDocument
make_editor_2d_commercial_package_update_review_ui_model(const Editor2DCommercialPackageUpdateReviewModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root(std::string(kRootId), mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{std::string(kRootId)};
    append_label(document, root, std::string(kRootId) + ".status", model.status_label);
    append_label(document, root, std::string(kRootId) + ".ready",
                 model.ready_for_reviewed_apply ? "ready" : "not_ready");
    append_label(document, root, std::string(kRootId) + ".manifest", model.game_manifest_path);
    append_label(document, root, std::string(kRootId) + ".revision.status",
                 model.revision_checked ? "ready" : "blocked");
    append_label(document, root, std::string(kRootId) + ".undo.count", std::to_string(model.undo_stack_count));
    append_label(document, root, std::string(kRootId) + ".redo.count", std::to_string(model.redo_stack_count));
    append_label(document, root, std::string(kRootId) + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, root, std::string(kRootId) + ".diagnostics", join_values(model.diagnostics));

    auto preview_root = make_child(std::string(kRootId) + ".preview", root, mirakana::ui::SemanticRole::list);
    add_or_throw(document, std::move(preview_root));
    const mirakana::ui::ElementId preview_root_id{std::string(kRootId) + ".preview"};
    for (const auto& row : model.preview_rows) {
        mirakana::ui::ElementDesc item = make_child(row.id, preview_root_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.runtime_package_path);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId row_id{row.id};
        append_label(document, row_id, row.id + ".status", row.status_label);
        append_label(document, row_id, row.id + ".diagnostic", row.diagnostic);
    }

    auto smoke_root = make_child(std::string(kRootId) + ".smoke", root, mirakana::ui::SemanticRole::list);
    add_or_throw(document, std::move(smoke_root));
    const mirakana::ui::ElementId smoke_root_id{std::string(kRootId) + ".smoke"};
    for (const auto& row : model.smoke_reviews) {
        mirakana::ui::ElementDesc item = make_child(row.id, smoke_root_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.recipe_id);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId row_id{row.id};
        append_label(document, row_id, row.id + ".status", row.status_label);
        append_label(document, row_id, row.id + ".host_gates", join_values(row.host_gates));
        append_label(document, row_id, row.id + ".diagnostic", row.diagnostic);
    }

    auto rejection_root = make_child(std::string(kRootId) + ".rejection", root, mirakana::ui::SemanticRole::list);
    add_or_throw(document, std::move(rejection_root));
    const mirakana::ui::ElementId rejection_root_id{std::string(kRootId) + ".rejection"};
    for (const auto& row : model.rejection_diagnostics) {
        mirakana::ui::ElementDesc item = make_child(row.id, rejection_root_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.candidate_path);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId row_id{row.id};
        append_label(document, row_id, row.id + ".status", row.status_label);
        append_label(document, row_id, row.id + ".diagnostic", row.diagnostic);
        append_label(document, row_id, row.id + ".blocks_apply", row.blocks_apply ? "true" : "false");
    }

    return document;
}

UndoableAction
make_editor_2d_commercial_package_update_apply_action(ITextStore& store, const ScenePackageRegistrationApplyPlan& plan,
                                                      const Editor2DCommercialPackageUpdateReviewModel& review) {
    if (!review.ready_for_reviewed_apply || !plan.can_apply || !is_safe_relative_path(plan.game_manifest_path) ||
        !review_matches_plan(review, plan)) {
        return {};
    }

    std::string before;
    try {
        before = store.read_text(plan.game_manifest_path);
    } catch (const std::exception&) {
        return {};
    }

    MemoryTextStore preview_store;
    try {
        preview_store.write_text(plan.game_manifest_path, before);
    } catch (const std::exception&) {
        return {};
    }
    const auto result = apply_scene_package_registration_to_manifest(preview_store, plan);
    if (!result.applied) {
        return {};
    }

    std::string after;
    try {
        after = preview_store.read_text(plan.game_manifest_path);
    } catch (const std::exception&) {
        return {};
    }
    if (after == before) {
        return {};
    }

    return UndoableAction{
        .label = "Apply reviewed 2D package update",
        .redo = [&store, path = plan.game_manifest_path, after]() { store.write_text(path, after); },
        .undo = [&store, path = plan.game_manifest_path, before]() { store.write_text(path, before); },
    };
}

} // namespace mirakana::editor
