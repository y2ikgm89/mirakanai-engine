// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/two_d_commercial_authoring_review.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

constexpr std::string_view kRootId = "2d_commercial_authoring_review";

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

void append_many_unique(std::vector<std::string>& destination, const std::vector<std::string>& source) {
    for (const auto& value : source) {
        append_unique(destination, sanitize_text(value));
    }
}

void push_diagnostic(Editor2DCommercialAuthoringReviewModel& model, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_claim(Editor2DCommercialAuthoringReviewModel& model, std::string claim, std::string diagnostic) {
    append_unique(model.unsupported_claims, std::move(claim));
    push_diagnostic(model, std::move(diagnostic));
}

[[nodiscard]] bool is_required_input_surface(Editor2DCommercialAuthoringReviewSurface surface) noexcept {
    return surface == Editor2DCommercialAuthoringReviewSurface::tile_set_review ||
           surface == Editor2DCommercialAuthoringReviewSurface::package_diff_review;
}

[[nodiscard]] std::string row_id_for_surface(Editor2DCommercialAuthoringReviewSurface surface) {
    return std::string(kRootId) + "." + std::string(editor_2d_commercial_authoring_review_surface_id(surface));
}

[[nodiscard]] std::string surface_label(Editor2DCommercialAuthoringReviewSurface surface) {
    std::string label{editor_2d_commercial_authoring_review_surface_id(surface)};
    std::ranges::replace(label, '_', ' ');
    return label;
}

[[nodiscard]] Editor2DCommercialAuthoringReviewStatus
aggregate_status(const Editor2DCommercialAuthoringReviewModel& model) noexcept {
    if (model.has_blocking_diagnostics || std::ranges::any_of(model.rows, [](const auto& row) {
            return row.status == Editor2DCommercialAuthoringReviewStatus::blocked;
        })) {
        return Editor2DCommercialAuthoringReviewStatus::blocked;
    }
    if (model.has_host_gates || std::ranges::any_of(model.rows, [](const auto& row) {
            return row.status == Editor2DCommercialAuthoringReviewStatus::host_gated;
        })) {
        return Editor2DCommercialAuthoringReviewStatus::host_gated;
    }
    return Editor2DCommercialAuthoringReviewStatus::ready;
}

void add_row(Editor2DCommercialAuthoringReviewModel& model, Editor2DCommercialAuthoringReviewRow row) {
    row.id = row_id_for_surface(row.surface);
    row.status_label = std::string(editor_2d_commercial_authoring_review_status_label(row.status));
    if (row.status == Editor2DCommercialAuthoringReviewStatus::blocked) {
        model.has_blocking_diagnostics = true;
    }
    if (row.status == Editor2DCommercialAuthoringReviewStatus::host_gated || !row.host_gates.empty()) {
        model.has_host_gates = true;
    }
    model.rows.push_back(std::move(row));
}

[[nodiscard]] Editor2DCommercialAuthoringReviewRow missing_row(Editor2DCommercialAuthoringReviewSurface surface,
                                                               std::string source_model, std::string diagnostic) {
    return Editor2DCommercialAuthoringReviewRow{
        .id = {},
        .surface = surface,
        .status = Editor2DCommercialAuthoringReviewStatus::blocked,
        .status_label = {},
        .source_model = std::move(source_model),
        .source_row_count = 0U,
        .retained_ui_row_ids = {},
        .host_gates = {},
        .diagnostic = std::move(diagnostic),
    };
}

[[nodiscard]] const Editor2DCommercialAuthoringReviewRowInput*
first_input_for_surface(const std::vector<Editor2DCommercialAuthoringReviewRowInput>& rows,
                        Editor2DCommercialAuthoringReviewSurface surface) noexcept {
    const auto it = std::ranges::find_if(rows, [surface](const auto& row) { return row.surface == surface; });
    return it == rows.end() ? nullptr : &(*it);
}

void detect_duplicate_input_surfaces(Editor2DCommercialAuthoringReviewModel& model,
                                     const std::vector<Editor2DCommercialAuthoringReviewRowInput>& rows) {
    std::vector<Editor2DCommercialAuthoringReviewSurface> seen;
    for (const auto& row : rows) {
        if (std::ranges::find(seen, row.surface) != seen.end()) {
            push_diagnostic(model, "duplicate 2D commercial authoring review surface '" +
                                       std::string(editor_2d_commercial_authoring_review_surface_id(row.surface)) +
                                       "'");
            continue;
        }
        seen.push_back(row.surface);
    }
}

[[nodiscard]] Editor2DCommercialAuthoringReviewRow
row_from_input(const Editor2DCommercialAuthoringReviewRowInput& input, Editor2DCommercialAuthoringReviewModel& model) {
    Editor2DCommercialAuthoringReviewRow row{
        .id = {},
        .surface = input.surface,
        .status = input.status,
        .status_label = {},
        .source_model = sanitize_text(input.source_model),
        .source_row_count = input.source_row_count,
        .retained_ui_row_ids = {},
        .host_gates = {},
        .diagnostic = sanitize_text(input.diagnostic),
    };
    append_many_unique(row.retained_ui_row_ids, input.retained_ui_row_ids);
    append_many_unique(row.host_gates, input.host_gates);

    if (input.source_model.empty()) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row source model is required for " + surface_label(input.surface));
    }
    if (input.source_row_count == 0U) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row source rows are required for " + surface_label(input.surface));
    }
    if (input.retained_ui_row_ids.empty()) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row retained ui row ids are required for " + surface_label(input.surface));
    }
    if (input.mutates) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row must not mutate packages or authoring documents");
    }
    if (input.executes) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row must not execute validation recipes, package scripts, or shell commands");
    }
    if (input.exposes_native_handles) {
        row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
        push_diagnostic(model, "input row must not expose native handles");
    }
    return row;
}

void add_atlas_inspection_row(Editor2DCommercialAuthoringReviewModel& model,
                              const EditorSpritePreviewDiagnosticsModel& diagnostics) {
    if (!diagnostics.has_atlas_pages) {
        push_diagnostic(model, "missing atlas inspection retained sprite atlas rows");
        add_row(model, missing_row(Editor2DCommercialAuthoringReviewSurface::atlas_inspection,
                                   "EditorSpritePreviewDiagnosticsModel",
                                   "missing atlas inspection retained sprite atlas rows"));
        return;
    }

    add_row(model, Editor2DCommercialAuthoringReviewRow{
                       .id = {},
                       .surface = Editor2DCommercialAuthoringReviewSurface::atlas_inspection,
                       .status = diagnostics.has_blocking_diagnostics ? Editor2DCommercialAuthoringReviewStatus::blocked
                                                                      : Editor2DCommercialAuthoringReviewStatus::ready,
                       .status_label = {},
                       .source_model = "EditorSpritePreviewDiagnosticsModel",
                       .source_row_count = diagnostics.atlas_pages.size(),
                       .retained_ui_row_ids = {"sprite_preview_diagnostics.atlas"},
                       .host_gates = {},
                       .diagnostic = diagnostics.has_blocking_diagnostics
                                         ? "atlas inspection blocked by sprite preview diagnostics"
                                         : "sprite atlas inspection rows are ready",
                   });
}

void add_tilemap_chunk_row(Editor2DCommercialAuthoringReviewModel& model,
                           const EditorTilemapPackageDiagnosticsModel& diagnostics) {
    if (!diagnostics.has_tilemap_payloads) {
        push_diagnostic(model, "missing tilemap chunk editing retained tilemap rows");
        add_row(model, missing_row(Editor2DCommercialAuthoringReviewSurface::tilemap_chunk_editing,
                                   "EditorTilemapPackageDiagnosticsModel",
                                   "missing tilemap chunk editing retained tilemap rows"));
        return;
    }

    add_row(model, Editor2DCommercialAuthoringReviewRow{
                       .id = {},
                       .surface = Editor2DCommercialAuthoringReviewSurface::tilemap_chunk_editing,
                       .status = diagnostics.has_blocking_diagnostics ? Editor2DCommercialAuthoringReviewStatus::blocked
                                                                      : Editor2DCommercialAuthoringReviewStatus::ready,
                       .status_label = {},
                       .source_model = "EditorTilemapPackageDiagnosticsModel",
                       .source_row_count = diagnostics.rows.size(),
                       .retained_ui_row_ids = {"2d_commercial_authoring_review.tilemap_chunk_editing.chunks"},
                       .host_gates = {},
                       .diagnostic = diagnostics.has_blocking_diagnostics
                                         ? "tilemap chunk editing blocked by tilemap diagnostics"
                                         : "tilemap chunk editing rows are ready",
                   });
}

void add_collision_overlay_row(Editor2DCommercialAuthoringReviewModel& model,
                               const EditorSpritePreviewDiagnosticsModel& diagnostics) {
    const auto collision_row_count = diagnostics.hitbox_count + diagnostics.hurtbox_count;
    if (collision_row_count == 0U) {
        push_diagnostic(model, "missing collision overlay review retained hitbox rows");
        add_row(model, missing_row(Editor2DCommercialAuthoringReviewSurface::collision_overlay_review,
                                   "EditorSpritePreviewDiagnosticsModel",
                                   "missing collision overlay review retained hitbox rows"));
        return;
    }

    add_row(model, Editor2DCommercialAuthoringReviewRow{
                       .id = {},
                       .surface = Editor2DCommercialAuthoringReviewSurface::collision_overlay_review,
                       .status = (diagnostics.has_blocking_diagnostics || diagnostics.hitbox_diagnostic_count > 0U)
                                     ? Editor2DCommercialAuthoringReviewStatus::blocked
                                     : Editor2DCommercialAuthoringReviewStatus::ready,
                       .status_label = {},
                       .source_model = "EditorSpritePreviewDiagnosticsModel",
                       .source_row_count = collision_row_count,
                       .retained_ui_row_ids = {"sprite_preview_diagnostics.selected"},
                       .host_gates = {},
                       .diagnostic = (diagnostics.has_blocking_diagnostics || diagnostics.hitbox_diagnostic_count > 0U)
                                         ? "collision overlay review blocked by sprite collision diagnostics"
                                         : "collision overlay review rows are ready",
                   });
}

void add_animation_preview_row(Editor2DCommercialAuthoringReviewModel& model,
                               const EditorSpritePreviewDiagnosticsModel& diagnostics) {
    if (!diagnostics.has_animation_payloads) {
        push_diagnostic(model, "missing animation preview retained animation rows");
        add_row(model, missing_row(Editor2DCommercialAuthoringReviewSurface::animation_preview,
                                   "EditorSpritePreviewDiagnosticsModel",
                                   "missing animation preview retained animation rows"));
        return;
    }

    add_row(model, Editor2DCommercialAuthoringReviewRow{
                       .id = {},
                       .surface = Editor2DCommercialAuthoringReviewSurface::animation_preview,
                       .status = diagnostics.has_blocking_diagnostics ? Editor2DCommercialAuthoringReviewStatus::blocked
                                                                      : Editor2DCommercialAuthoringReviewStatus::ready,
                       .status_label = {},
                       .source_model = "EditorSpritePreviewDiagnosticsModel",
                       .source_row_count = diagnostics.animations.size(),
                       .retained_ui_row_ids = {"sprite_preview_diagnostics.animations"},
                       .host_gates = {},
                       .diagnostic = diagnostics.has_blocking_diagnostics
                                         ? "animation preview blocked by sprite animation diagnostics"
                                         : "animation preview rows are ready",
                   });
}

void add_validation_diagnostics_row(Editor2DCommercialAuthoringReviewModel& model,
                                    const EditorAiValidationRecipePreflightModel& validation_preflight) {
    if (validation_preflight.rows.empty()) {
        push_diagnostic(model, "missing validation diagnostics retained preflight rows");
        add_row(model, missing_row(Editor2DCommercialAuthoringReviewSurface::validation_diagnostics,
                                   "EditorAiValidationRecipePreflightModel",
                                   "missing validation diagnostics retained preflight rows"));
        return;
    }

    add_row(model, Editor2DCommercialAuthoringReviewRow{
                       .id = {},
                       .surface = Editor2DCommercialAuthoringReviewSurface::validation_diagnostics,
                       .status = validation_preflight.has_blocking_diagnostics
                                     ? Editor2DCommercialAuthoringReviewStatus::blocked
                                     : (validation_preflight.has_host_gated_recipes
                                            ? Editor2DCommercialAuthoringReviewStatus::host_gated
                                            : Editor2DCommercialAuthoringReviewStatus::ready),
                       .status_label = {},
                       .source_model = "EditorAiValidationRecipePreflightModel",
                       .source_row_count = validation_preflight.rows.size(),
                       .retained_ui_row_ids = {"ai_validation_recipe_preflight.rows"},
                       .host_gates = {},
                       .diagnostic = validation_preflight.has_blocking_diagnostics
                                         ? "validation diagnostics blocked by preflight diagnostics"
                                         : "validation diagnostics rows are ready",
                   });
}

void add_required_input_row(Editor2DCommercialAuthoringReviewModel& model,
                            const std::vector<Editor2DCommercialAuthoringReviewRowInput>& rows,
                            Editor2DCommercialAuthoringReviewSurface surface) {
    const auto* input = first_input_for_surface(rows, surface);
    if (input == nullptr) {
        const std::string label = surface_label(surface);
        push_diagnostic(model, "missing " + label + " retained review rows");
        add_row(model, missing_row(surface, "Editor2DCommercialAuthoringReviewRowInput",
                                   "missing " + label + " retained review rows"));
        return;
    }
    add_row(model, row_from_input(*input, model));
}

void validate_unexpected_inputs(Editor2DCommercialAuthoringReviewModel& model,
                                const std::vector<Editor2DCommercialAuthoringReviewRowInput>& rows) {
    for (const auto& row : rows) {
        if (!is_required_input_surface(row.surface)) {
            push_diagnostic(model, "2D commercial authoring review surface '" +
                                       std::string(editor_2d_commercial_authoring_review_surface_id(row.surface)) +
                                       "' is owned by built-in diagnostics and must not be supplied as a free row");
        }
    }
}

void detect_duplicate_retained_rows(Editor2DCommercialAuthoringReviewModel& model) {
    std::unordered_set<std::string> seen;
    for (auto& row : model.rows) {
        for (const auto& id : row.retained_ui_row_ids) {
            if (id.empty()) {
                row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
                push_diagnostic(model, "empty retained ui row id in " + surface_label(row.surface));
                continue;
            }
            if (!seen.insert(id).second) {
                row.status = Editor2DCommercialAuthoringReviewStatus::blocked;
                row.status_label = std::string(editor_2d_commercial_authoring_review_status_label(row.status));
                push_diagnostic(model, "duplicate retained ui row id '" + id + "'");
            }
        }
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("2D commercial authoring review ui element could not be added");
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

void append_review_row(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                       const Editor2DCommercialAuthoringReviewRow& row) {
    const mirakana::ui::ElementId row_id{row.id};
    mirakana::ui::ElementDesc item = make_child(row.id, root, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(surface_label(row.surface));
    add_or_throw(document, std::move(item));
    append_label(document, row_id, row.id + ".status", row.status_label);
    append_label(document, row_id, row.id + ".source_model", row.source_model);
    append_label(document, row_id, row.id + ".source_rows", std::to_string(row.source_row_count));
    append_label(document, row_id, row.id + ".retained_ui_rows", join_values(row.retained_ui_row_ids));
    append_label(document, row_id, row.id + ".host_gates", join_values(row.host_gates));
    append_label(document, row_id, row.id + ".diagnostic", row.diagnostic);
}

} // namespace

std::string_view
editor_2d_commercial_authoring_review_status_label(Editor2DCommercialAuthoringReviewStatus status) noexcept {
    switch (status) {
    case Editor2DCommercialAuthoringReviewStatus::ready:
        return "ready";
    case Editor2DCommercialAuthoringReviewStatus::blocked:
        return "blocked";
    case Editor2DCommercialAuthoringReviewStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

std::string_view
editor_2d_commercial_authoring_review_surface_id(Editor2DCommercialAuthoringReviewSurface surface) noexcept {
    switch (surface) {
    case Editor2DCommercialAuthoringReviewSurface::atlas_inspection:
        return "atlas_inspection";
    case Editor2DCommercialAuthoringReviewSurface::tile_set_review:
        return "tile_set_review";
    case Editor2DCommercialAuthoringReviewSurface::tilemap_chunk_editing:
        return "tilemap_chunk_editing";
    case Editor2DCommercialAuthoringReviewSurface::collision_overlay_review:
        return "collision_overlay_review";
    case Editor2DCommercialAuthoringReviewSurface::animation_preview:
        return "animation_preview";
    case Editor2DCommercialAuthoringReviewSurface::package_diff_review:
        return "package_diff_review";
    case Editor2DCommercialAuthoringReviewSurface::validation_diagnostics:
        return "validation_diagnostics";
    }
    return "unknown";
}

Editor2DCommercialAuthoringReviewModel
make_editor_2d_commercial_authoring_review_model(const Editor2DCommercialAuthoringReviewDesc& desc) {
    Editor2DCommercialAuthoringReviewModel model;
    model.mutates = false;
    model.executes = false;
    model.exposes_native_handles = false;

    if (desc.request_package_mutation) {
        reject_claim(model, "package mutation",
                     "2D commercial authoring review does not mutate packages or authoring documents");
    }
    if (desc.request_validation_execution) {
        reject_claim(model, "validation execution",
                     "2D commercial authoring review does not execute validation recipes");
    }
    if (desc.request_runtime_source_parsing) {
        reject_claim(model, "runtime source parsing",
                     "2D commercial authoring review keeps runtime source parsing out of editor core");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_claim(model, "renderer/RHI handle exposure",
                     "2D commercial authoring review must not expose renderer or RHI handles");
    }
    if (desc.request_native_handle_exposure) {
        reject_claim(model, "native handle exposure", "2D commercial authoring review must not expose native handles");
    }
    if (desc.request_external_engine_project_import) {
        reject_claim(model, "external engine project import",
                     "2D commercial authoring review does not import external engine projects");
    }
    if (desc.request_external_engine_api_parity_claim) {
        reject_claim(model, "external engine API parity",
                     "2D commercial authoring review does not claim external engine API parity");
    }

    detect_duplicate_input_surfaces(model, desc.review_rows);
    validate_unexpected_inputs(model, desc.review_rows);

    add_atlas_inspection_row(model, desc.sprite_preview_diagnostics);
    add_required_input_row(model, desc.review_rows, Editor2DCommercialAuthoringReviewSurface::tile_set_review);
    add_tilemap_chunk_row(model, desc.tilemap_diagnostics);
    add_collision_overlay_row(model, desc.sprite_preview_diagnostics);
    add_animation_preview_row(model, desc.sprite_preview_diagnostics);
    add_required_input_row(model, desc.review_rows, Editor2DCommercialAuthoringReviewSurface::package_diff_review);
    add_validation_diagnostics_row(model, desc.validation_preflight);
    detect_duplicate_retained_rows(model);

    model.status = aggregate_status(model);
    model.status_label = std::string(editor_2d_commercial_authoring_review_status_label(model.status));
    model.ready_for_authoring_review = model.status == Editor2DCommercialAuthoringReviewStatus::ready;
    return model;
}

mirakana::ui::UiDocument
make_editor_2d_commercial_authoring_review_ui_model(const Editor2DCommercialAuthoringReviewModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root(std::string(kRootId), mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{std::string(kRootId)};
    append_label(document, root, std::string(kRootId) + ".status", model.status_label);
    append_label(document, root, std::string(kRootId) + ".ready",
                 model.ready_for_authoring_review ? "ready" : "not_ready");
    append_label(document, root, std::string(kRootId) + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, root, std::string(kRootId) + ".diagnostics", join_values(model.diagnostics));

    for (const auto& row : model.rows) {
        append_review_row(document, root, row);
    }
    return document;
}

} // namespace mirakana::editor
