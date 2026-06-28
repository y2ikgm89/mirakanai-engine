// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_browser_production.hpp"

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string asset_row_id(AssetId asset) {
    return "asset_browser.source_pulse." + std::to_string(asset.value);
}

[[nodiscard]] std::string preview_row_id(std::string_view kind, AssetId asset) {
    return "asset_browser.preview." + std::string(kind) + "." + std::to_string(asset.value);
}

[[nodiscard]] bool import_plan_contains(const AssetImportPlan* plan, AssetId asset, std::string_view output_path) {
    if (plan == nullptr) {
        return false;
    }
    return std::ranges::any_of(plan->actions, [asset, output_path](const AssetImportAction& action) {
        return action.id == asset && (output_path.empty() || action.output_path == output_path);
    });
}

[[nodiscard]] char ascii_lower(char value) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] bool ascii_is_space(char value) noexcept {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' || value == '\v';
}

[[nodiscard]] std::string lower_ascii(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char character : value) {
        lowered.push_back(ascii_lower(character));
    }
    return lowered;
}

[[nodiscard]] bool equals_case_insensitive(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return std::ranges::equal(lhs, rhs, [](char left, char right) { return ascii_lower(left) == ascii_lower(right); });
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view text, std::string_view prefix) {
    if (prefix.size() > text.size()) {
        return false;
    }
    return equals_case_insensitive(text.substr(0, prefix.size()), prefix);
}

[[nodiscard]] bool contains_case_insensitive(std::string_view text, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > text.size()) {
        return false;
    }
    return !std::ranges::search(text, needle, [](char lhs, char rhs) {
                return ascii_lower(lhs) == ascii_lower(rhs);
            }).empty();
}

[[nodiscard]] std::vector<std::string_view> split_query_tokens(std::string_view query_text) {
    std::vector<std::string_view> tokens;
    std::size_t cursor = 0;
    while (cursor < query_text.size()) {
        while (cursor < query_text.size() && ascii_is_space(query_text[cursor])) {
            ++cursor;
        }
        const std::size_t begin = cursor;
        while (cursor < query_text.size() && !ascii_is_space(query_text[cursor])) {
            ++cursor;
        }
        if (begin != cursor) {
            tokens.push_back(query_text.substr(begin, cursor - begin));
        }
    }
    return tokens;
}

[[nodiscard]] bool is_supported_query_operator(std::string_view key) {
    return key == "kind" || key == "scope" || key == "state" || key == "key" || key == "path";
}

[[nodiscard]] std::string_view command_label(EditorAssetBrowserCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetBrowserCommandKind::reload_source_registry:
        return "Reload source registry";
    case EditorAssetBrowserCommandKind::review_import_sources:
        return "Review import sources";
    case EditorAssetBrowserCommandKind::copy_external_sources:
        return "Copy external sources";
    case EditorAssetBrowserCommandKind::execute_reviewed_import_plan:
        return "Execute reviewed import plan";
    case EditorAssetBrowserCommandKind::preview_cooked_package:
        return "Preview cooked package";
    case EditorAssetBrowserCommandKind::stage_hot_reload_recook:
        return "Stage hot-reload recook";
    case EditorAssetBrowserCommandKind::inspect_selection:
        return "Inspect selection";
    case EditorAssetBrowserCommandKind::apply_package_registration:
        return "Apply package registration";
    }
    return "Reload source registry";
}

[[nodiscard]] std::string_view command_mode_label(EditorAssetBrowserCommandMode mode) noexcept {
    switch (mode) {
    case EditorAssetBrowserCommandMode::dry_run:
        return "dry_run";
    case EditorAssetBrowserCommandMode::apply:
        return "apply";
    }
    return "dry_run";
}

[[nodiscard]] bool command_requires_confirmation(EditorAssetBrowserCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetBrowserCommandKind::copy_external_sources:
    case EditorAssetBrowserCommandKind::execute_reviewed_import_plan:
    case EditorAssetBrowserCommandKind::stage_hot_reload_recook:
    case EditorAssetBrowserCommandKind::apply_package_registration:
        return true;
    case EditorAssetBrowserCommandKind::reload_source_registry:
    case EditorAssetBrowserCommandKind::review_import_sources:
    case EditorAssetBrowserCommandKind::preview_cooked_package:
    case EditorAssetBrowserCommandKind::inspect_selection:
        return false;
    }
    return false;
}

[[nodiscard]] bool command_mutates_project_files(EditorAssetBrowserCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetBrowserCommandKind::copy_external_sources:
    case EditorAssetBrowserCommandKind::execute_reviewed_import_plan:
    case EditorAssetBrowserCommandKind::stage_hot_reload_recook:
    case EditorAssetBrowserCommandKind::apply_package_registration:
        return true;
    case EditorAssetBrowserCommandKind::reload_source_registry:
    case EditorAssetBrowserCommandKind::review_import_sources:
    case EditorAssetBrowserCommandKind::preview_cooked_package:
    case EditorAssetBrowserCommandKind::inspect_selection:
        return false;
    }
    return false;
}

[[nodiscard]] bool command_executes_import_tools(EditorAssetBrowserCommandKind kind) noexcept {
    return kind == EditorAssetBrowserCommandKind::execute_reviewed_import_plan;
}

[[nodiscard]] bool is_external_engine_material(const EditorAssetBrowserLegalProvenanceRow& row) {
    if (row.external_engine_material) {
        return true;
    }
    const std::string review_text =
        lower_ascii(row.source_url + " " + row.modification_status + " " + row.asset_key_label);
    return review_text.find("assetstore.unity.com") != std::string::npos ||
           review_text.find("unity.com/packages") != std::string::npos ||
           review_text.find("unrealengine.com/marketplace") != std::string::npos ||
           review_text.find("fab.com") != std::string::npos ||
           review_text.find("unreal marketplace") != std::string::npos ||
           review_text.find("engine_sample_content") != std::string::npos ||
           review_text.find("engine_logo_or_trademark") != std::string::npos ||
           review_text.find("copied_editor_ui_expression") != std::string::npos ||
           review_text.find("copied_editor_screenshot") != std::string::npos ||
           review_text.find("copied_editor_icon") != std::string::npos ||
           review_text.find("copied_editor_layout") != std::string::npos;
}

[[nodiscard]] bool is_restricted_license(std::string_view license_id) {
    const std::string license = lower_ascii(license_id);
    return license.find("cc-by-nc") != std::string::npos || license.find("cc-nc") != std::string::npos ||
           license.find("-nc") != std::string::npos || license.find("cc-by-nd") != std::string::npos ||
           license.find("cc-nd") != std::string::npos || license.find("-nd") != std::string::npos;
}

[[nodiscard]] bool supported_open_exr_compression(std::string_view compression) {
    return equals_case_insensitive(compression, "none") || equals_case_insensitive(compression, "zip") ||
           equals_case_insensitive(compression, "zips") || equals_case_insensitive(compression, "piz") ||
           equals_case_insensitive(compression, "rle");
}

[[nodiscard]] bool unsupported_open_exr_policy(std::string_view value) {
    return value.empty() || contains_case_insensitive(value, "unsupported") ||
           contains_case_insensitive(value, "multipart") || contains_case_insensitive(value, "deep");
}

[[nodiscard]] bool missing_ktx2_metadata(const EditorAssetBrowserKtx2BasisSourceReviewRow& row) {
    return row.source_path.empty() || row.basis_color_model.empty() || row.dimensions.empty() || row.levels.empty() ||
           row.layers.empty() || row.faces.empty() || row.supercompression.empty() || row.payload_byte_count.empty();
}

[[nodiscard]] std::string_view thumbnail_kind_id(EditorAssetThumbnailKind kind) noexcept {
    switch (kind) {
    case EditorAssetThumbnailKind::texture:
        return "thumbnail_texture";
    case EditorAssetThumbnailKind::mesh:
        return "thumbnail_mesh";
    case EditorAssetThumbnailKind::material:
        return "thumbnail_material";
    case EditorAssetThumbnailKind::scene:
        return "thumbnail_scene";
    case EditorAssetThumbnailKind::audio:
        return "thumbnail_audio";
    case EditorAssetThumbnailKind::unknown:
        break;
    }
    return "thumbnail_unknown";
}

[[nodiscard]] std::string_view scene_reference_diagnostic_kind_label(SceneAuthoringDiagnosticKind kind) noexcept {
    switch (kind) {
    case SceneAuthoringDiagnosticKind::missing_asset:
        return "missing_asset";
    case SceneAuthoringDiagnosticKind::wrong_asset_kind:
        return "wrong_asset_kind";
    }
    return "missing_asset";
}

[[nodiscard]] std::string fallback_asset_label(AssetId asset) {
    return "asset:" + std::to_string(asset.value);
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_material_preview_evidence_row(const EditorMaterialAssetPreviewPanelModel& preview) {
    EditorAssetBrowserPreviewEvidenceRow row{
        .id = preview_row_id("material", preview.material),
        .asset_key_label = preview.material_id.empty() ? fallback_asset_label(preview.material) : preview.material_id,
        .preview_kind = "material",
        .backend_label = preview.gpu_execution_backend_label.empty() ? "-" : preview.gpu_execution_backend_label,
        .display_path_label = preview.gpu_execution_display_path_label.empty()
                                  ? preview.material_path
                                  : preview.gpu_execution_display_path_label,
        .status_label =
            preview.gpu_execution_status_label.empty() ? preview.status_label : preview.gpu_execution_status_label,
        .diagnostic = preview.gpu_execution_diagnostic.empty() ? "material preview execution evidence is host-owned"
                                                               : preview.gpu_execution_diagnostic,
        .frame_or_sample_count = preview.gpu_execution_frames_rendered,
        .host_owned = preview.gpu_execution_host_owned,
        .ready = preview.gpu_execution_ready,
        .exposes_native_handles = preview.exposes_native_handles,
    };
    if (preview.executes || preview.exposes_native_handles) {
        row.status_label = "blocked";
        row.diagnostic = "material preview evidence must not claim editor-core execution or native handle exposure";
        row.ready = false;
        row.host_owned = true;
        row.exposes_native_handles = preview.exposes_native_handles;
    }
    if (row.status_label.empty()) {
        row.status_label = "Host Required";
    }
    if (row.display_path_label.empty()) {
        row.display_path_label = "-";
    }
    return row;
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_thumbnail_preview_evidence_row(const EditorAssetThumbnailRequest& request) {
    const std::string_view kind = thumbnail_kind_id(request.kind);
    return EditorAssetBrowserPreviewEvidenceRow{
        .id = preview_row_id(kind, request.asset),
        .asset_key_label = request.output_path.empty() ? fallback_asset_label(request.asset) : request.output_path,
        .preview_kind = std::string(kind),
        .backend_label = "host-thumbnail-queue",
        .display_path_label = request.output_path.empty() ? request.source_path : request.output_path,
        .status_label = "host_request_queued",
        .diagnostic = "thumbnail request is host-owned; editor core does not decode or upload source files",
        .frame_or_sample_count = 0,
        .host_owned = true,
        .ready = false,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_gltf_inspect_evidence_row(const EditorAssetBrowserGltfInspectEvidenceInput& input) {
    EditorAssetBrowserPreviewEvidenceRow row{
        .id = preview_row_id("gltf_mesh_inspect", input.asset),
        .asset_key_label = input.asset_key_label.empty() ? fallback_asset_label(input.asset) : input.asset_key_label,
        .preview_kind = "gltf_mesh_inspect",
        .backend_label = "gltf2-mesh-inspector",
        .display_path_label = input.source_path,
        .status_label = input.report.parse_succeeded ? "inspect_ready" : "inspect_failed",
        .diagnostic = input.report.parse_succeeded
                          ? "glTF 2.0 mesh primitives inspected as value-only attributes/indices evidence"
                          : input.report.diagnostic,
        .frame_or_sample_count = static_cast<std::uint64_t>(input.report.rows.size()),
        .host_owned = true,
        .ready = input.report.parse_succeeded,
        .exposes_native_handles = false,
    };
    if (row.display_path_label.empty()) {
        row.display_path_label = "-";
    }
    if (row.diagnostic.empty()) {
        row.diagnostic = "glTF mesh inspect report is unavailable";
    }
    return row;
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_audio_summary_evidence_row(const EditorAssetBrowserAudioSummaryInput& input) {
    const bool ready = is_valid_audio_source_document(input.audio);
    return EditorAssetBrowserPreviewEvidenceRow{
        .id = preview_row_id("audio_summary", input.asset),
        .asset_key_label = input.asset_key_label.empty() ? fallback_asset_label(input.asset) : input.asset_key_label,
        .preview_kind = "audio_summary",
        .backend_label = "source-audio-metadata",
        .display_path_label = input.source_path.empty() ? "-" : input.source_path,
        .status_label = ready ? "metadata_ready" : "invalid_audio_summary",
        .diagnostic = "audio summary uses metadata only: " + std::to_string(input.audio.channel_count) + "ch " +
                      std::to_string(input.audio.sample_rate) + "Hz " +
                      std::string(audio_source_sample_format_name(input.audio.sample_format)),
        .frame_or_sample_count = input.audio.frame_count,
        .host_owned = true,
        .ready = ready,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_scene_reference_diagnostic_evidence_row(const SceneAuthoringDiagnostic& diagnostic, std::size_t index) {
    return EditorAssetBrowserPreviewEvidenceRow{
        .id = "asset_browser.preview.scene_reference." + std::to_string(diagnostic.node.value) + "." +
              std::to_string(diagnostic.asset.value) + "." + std::to_string(index),
        .asset_key_label = fallback_asset_label(diagnostic.asset),
        .preview_kind = "scene_reference_diagnostic",
        .backend_label = "scene-authoring-validator",
        .display_path_label = diagnostic.field.empty() ? "-" : diagnostic.field,
        .status_label = "blocked",
        .diagnostic =
            std::string(scene_reference_diagnostic_kind_label(diagnostic.kind)) + ": " + diagnostic.diagnostic,
        .frame_or_sample_count = 0,
        .host_owned = true,
        .ready = false,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] EditorAssetBrowserPreviewEvidenceRow
make_hot_reload_recook_evidence_row(const AssetHotReloadRecookRequest& request) {
    return EditorAssetBrowserPreviewEvidenceRow{
        .id = preview_row_id("hot_reload_recook", request.asset),
        .asset_key_label = fallback_asset_label(request.asset),
        .preview_kind = "hot_reload_recook",
        .backend_label = "asset-hot-reload-scheduler",
        .display_path_label = request.trigger_path.empty() ? "-" : request.trigger_path,
        .status_label = "recook_staged",
        .diagnostic = std::string(editor_asset_hot_reload_recook_reason_label(request.reason)),
        .frame_or_sample_count = request.current_revision,
        .host_owned = true,
        .ready = true,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] bool preview_matches_source_pulse(const EditorAssetBrowserPreviewEvidenceRow& preview,
                                                const EditorAssetBrowserSourcePulseRow& row) {
    return (!preview.asset_key_label.empty() && preview.asset_key_label == row.asset_key_label) ||
           (!preview.display_path_label.empty() &&
            (preview.display_path_label == row.imported_path || preview.display_path_label == row.source_path));
}

void apply_preview_evidence_to_source_pulse_rows(EditorAssetBrowserProductionModel& model) {
    for (auto& source_row : model.rows) {
        for (const auto& preview : model.preview_rows) {
            if (!preview_matches_source_pulse(preview, source_row)) {
                continue;
            }
            if (preview.preview_kind == "hot_reload_recook") {
                source_row.hot_reload_status_label = preview.status_label;
            } else {
                source_row.preview_status_label = preview.status_label;
            }
            source_row.host_gated = source_row.host_gated || (preview.host_owned && !preview.ready);
            source_row.blocked = source_row.blocked || preview.exposes_native_handles;
        }
    }
}

void append_command_report_rows(EditorAssetBrowserCommandPlan& plan, EditorAssetBrowserCommandMode mode) {
    plan.report_rows.push_back("command_id=" + plan.command_id);
    plan.report_rows.push_back("mode=" + std::string(command_mode_label(mode)));
    plan.report_rows.push_back("expected_generation=" + std::to_string(plan.expected_generation));
    plan.report_rows.push_back("current_generation=" + std::to_string(plan.current_generation));
    plan.report_rows.push_back("editor_core_execution=false");
    plan.report_rows.push_back("package_scripts=false");
    plan.report_rows.push_back("validation_recipes=false");
    plan.report_rows.push_back("native_handles=false");
}

[[nodiscard]] EditorAssetBrowserQueryTokenRow make_query_token(std::size_t index, std::string key, std::string value,
                                                               bool blocked) {
    return EditorAssetBrowserQueryTokenRow{
        .id = "asset_browser.query.token." + std::to_string(index),
        .key = std::move(key),
        .value = std::move(value),
        .status_label = blocked ? "blocked" : "active",
        .active = !blocked,
        .blocked = blocked,
    };
}

void append_normalized_token(std::string& normalized_query, std::string_view token) {
    if (!normalized_query.empty()) {
        normalized_query.push_back(' ');
    }
    normalized_query.append(token);
}

[[nodiscard]] bool matches_query_state(const EditorAssetBrowserSourcePulseRow& row, std::string_view value) {
    if (equals_case_insensitive(value, "ready")) {
        return equals_case_insensitive(row.state_label, "ready") && !row.blocked && !row.host_gated;
    }
    if (equals_case_insensitive(value, "blocked")) {
        return row.blocked;
    }
    if (equals_case_insensitive(value, "host_gated")) {
        return row.host_gated;
    }
    if (equals_case_insensitive(value, "missing")) {
        return contains_case_insensitive(row.state_label, "missing");
    }
    return equals_case_insensitive(row.state_label, value);
}

[[nodiscard]] bool matches_query_operator(const EditorAssetBrowserSourcePulseRow& row, std::string_view key,
                                          std::string_view value) {
    if (key == "kind") {
        const std::string_view kind_label = row.kind_label.empty() ? asset_kind_label(row.kind) : row.kind_label;
        return equals_case_insensitive(kind_label, value);
    }
    if (key == "scope") {
        return equals_case_insensitive(row.scope_label, value);
    }
    if (key == "state") {
        return matches_query_state(row, value);
    }
    if (key == "key") {
        return starts_with_case_insensitive(row.asset_key_label, value);
    }
    if (key == "path") {
        return contains_case_insensitive(row.source_path, value) || contains_case_insensitive(row.imported_path, value);
    }
    return false;
}

[[nodiscard]] bool matches_plain_text(const EditorAssetBrowserSourcePulseRow& row, std::string_view value) {
    return contains_case_insensitive(row.display_name, value) || contains_case_insensitive(row.source_path, value) ||
           contains_case_insensitive(row.imported_path, value) || contains_case_insensitive(row.asset_key_label, value);
}

[[nodiscard]] bool matches_query_token(const EditorAssetBrowserSourcePulseRow& row,
                                       const EditorAssetBrowserQueryTokenRow& token) {
    if (token.key == "text") {
        return matches_plain_text(row, token.value);
    }
    return matches_query_operator(row, token.key, token.value);
}

[[nodiscard]] bool matches_all_query_tokens(const EditorAssetBrowserSourcePulseRow& row,
                                            const std::vector<EditorAssetBrowserQueryTokenRow>& tokens) {
    return std::ranges::all_of(tokens, [&row](const EditorAssetBrowserQueryTokenRow& token) {
        return !token.active || matches_query_token(row, token);
    });
}

[[nodiscard]] EditorAssetBrowserSourcePulseRow make_source_pulse_row(const ContentBrowserItem& item,
                                                                     const ContentBrowserItem* selected,
                                                                     const AssetImportPlan* import_plan) {
    const bool import_planned = import_plan_contains(import_plan, item.id, item.path);
    EditorAssetBrowserSourcePulseRow row{
        .asset = item.id,
        .kind = item.kind,
        .row_id = asset_row_id(item.id),
        .kind_label = std::string(asset_kind_label(item.kind)),
        .asset_key_label = item.asset_key_label,
        .source_path = item.identity_source_path,
        .imported_path = item.path,
        .display_name = item.display_name,
        .scope_label = item.identity_source_path.empty() ? "cooked" : "source",
        .state_label = item.identity_backed ? "ready" : "missing_identity",
        .import_status_label = import_planned ? "planned" : "not_planned",
        .package_status_label = "not_reviewed",
        .provenance_status_label = "not_reviewed",
        .preview_status_label = "not_requested",
        .hot_reload_status_label = "not_staged",
        .selected = selected != nullptr && selected->id == item.id,
        .identity_backed = item.identity_backed,
        .source_visible = !item.identity_source_path.empty(),
        .package_visible = false,
        .blocked = !item.identity_backed,
        .host_gated = false,
    };
    return row;
}

void sort_rows(std::vector<EditorAssetBrowserSourcePulseRow>& rows) {
    std::ranges::sort(rows,
                      [](const EditorAssetBrowserSourcePulseRow& lhs, const EditorAssetBrowserSourcePulseRow& rhs) {
                          if (lhs.imported_path != rhs.imported_path) {
                              return lhs.imported_path < rhs.imported_path;
                          }
                          if (lhs.asset_key_label != rhs.asset_key_label) {
                              return lhs.asset_key_label < rhs.asset_key_label;
                          }
                          return lhs.row_id < rhs.row_id;
                      });
}

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

void require_safe_field(std::string_view field, std::string_view value) {
    if (value.empty() || contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("asset browser production ui field is invalid: ") + std::string(field));
    }
}

void require_safe_label(std::string_view field, std::string_view value) {
    if (contains_line_separator(value)) {
        throw std::invalid_argument(std::string("asset browser production ui label is invalid: ") + std::string(field));
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("asset browser production ui element could not be added");
    }
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

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

} // namespace

std::string_view editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept {
    switch (status) {
    case EditorAssetBrowserProductionStatus::empty:
        return "Asset browser empty";
    case EditorAssetBrowserProductionStatus::ready:
        return "Asset browser ready";
    case EditorAssetBrowserProductionStatus::attention:
        return "Asset browser attention";
    }
    return "Asset browser empty";
}

std::string_view editor_asset_browser_command_id(EditorAssetBrowserCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetBrowserCommandKind::reload_source_registry:
        return "asset_browser.source_registry.reload";
    case EditorAssetBrowserCommandKind::review_import_sources:
        return "asset_browser.import.review_sources";
    case EditorAssetBrowserCommandKind::copy_external_sources:
        return "asset_browser.import.copy_external_sources";
    case EditorAssetBrowserCommandKind::execute_reviewed_import_plan:
        return "asset_browser.import.execute_reviewed_plan";
    case EditorAssetBrowserCommandKind::preview_cooked_package:
        return "asset_browser.cook.package_preview";
    case EditorAssetBrowserCommandKind::stage_hot_reload_recook:
        return "asset_browser.hot_reload.stage_recook";
    case EditorAssetBrowserCommandKind::inspect_selection:
        return "asset_browser.selection.inspect";
    case EditorAssetBrowserCommandKind::apply_package_registration:
        return "asset_browser.package.apply_registration";
    }
    return "asset_browser.source_registry.reload";
}

std::vector<EditorAssetBrowserPreviewEvidenceRow>
make_editor_asset_browser_preview_evidence_rows(const EditorAssetBrowserPreviewEvidenceDesc& desc) {
    std::vector<EditorAssetBrowserPreviewEvidenceRow> rows;
    rows.reserve(desc.material_previews.size() + desc.thumbnail_requests.size() + desc.gltf_inspects.size() +
                 desc.audio_summaries.size() + desc.scene_reference_diagnostics.size() +
                 desc.hot_reload_recook_requests.size());

    for (const auto& preview : desc.material_previews) {
        rows.push_back(make_material_preview_evidence_row(preview));
    }
    for (const auto& request : desc.thumbnail_requests) {
        rows.push_back(make_thumbnail_preview_evidence_row(request));
    }
    for (const auto& input : desc.gltf_inspects) {
        rows.push_back(make_gltf_inspect_evidence_row(input));
    }
    for (const auto& input : desc.audio_summaries) {
        rows.push_back(make_audio_summary_evidence_row(input));
    }
    for (std::size_t index = 0; index < desc.scene_reference_diagnostics.size(); ++index) {
        rows.push_back(make_scene_reference_diagnostic_evidence_row(desc.scene_reference_diagnostics[index], index));
    }
    for (const auto& request : desc.hot_reload_recook_requests) {
        rows.push_back(make_hot_reload_recook_evidence_row(request));
    }

    std::ranges::sort(rows, [](const EditorAssetBrowserPreviewEvidenceRow& lhs,
                               const EditorAssetBrowserPreviewEvidenceRow& rhs) { return lhs.id < rhs.id; });
    return rows;
}

EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc) {
    EditorAssetBrowserProductionModel model;
    model.project_root = desc.project_root;
    model.asset_root = desc.asset_root;
    model.source_registry_path = desc.source_registry_path;
    model.generation = desc.generation;

    if (desc.browser == nullptr) {
        model.diagnostics.push_back("content browser state is missing");
        return model;
    }

    const auto visible_items = desc.browser->visible_items();
    const auto* selected = desc.browser->selected_asset();
    model.total_row_count = desc.browser->item_count();
    model.visible_row_count = visible_items.size();
    model.rows.reserve(visible_items.size());
    for (const auto& item : visible_items) {
        model.rows.push_back(make_source_pulse_row(item, selected, desc.import_plan));
    }
    sort_rows(model.rows);
    if (desc.preview_evidence != nullptr) {
        model.preview_rows = make_editor_asset_browser_preview_evidence_rows(*desc.preview_evidence);
        apply_preview_evidence_to_source_pulse_rows(model);
        model.exposes_native_handles =
            std::ranges::any_of(model.preview_rows, [](const auto& row) { return row.exposes_native_handles; });
    }

    model.status =
        model.rows.empty() ? EditorAssetBrowserProductionStatus::empty : EditorAssetBrowserProductionStatus::ready;
    model.status_label = std::string(editor_asset_browser_production_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("asset_browser", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"asset_browser"};

    require_safe_field("status", model.status_label);
    require_safe_label("source_registry_path", model.source_registry_path);
    append_label(document, root, "asset_browser.status", model.status_label);
    append_label(document, root, "asset_browser.source_registry.path",
                 model.source_registry_path.empty() ? "-" : model.source_registry_path);
    append_label(document, root, "asset_browser.total_rows", std::to_string(model.total_row_count));
    append_label(document, root, "asset_browser.visible_rows", std::to_string(model.visible_row_count));

    add_or_throw(document, make_child("asset_browser.source_pulse", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_root{"asset_browser.source_pulse"};

    for (const auto& row : model.rows) {
        require_safe_field("row_id", row.row_id);
        require_safe_label("asset_key_label", row.asset_key_label);
        require_safe_label("source_path", row.source_path);
        require_safe_label("imported_path", row.imported_path);
        require_safe_field("state_label", row.state_label);

        mirakana::ui::ElementDesc item = make_child(row.row_id, list_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.display_name.empty() ? row.asset_key_label : row.display_name);
        item.enabled = !row.blocked;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{row.row_id};
        append_label(document, item_id, row.row_id + ".asset_key",
                     row.asset_key_label.empty() ? "-" : row.asset_key_label);
        append_label(document, item_id, row.row_id + ".scope", row.scope_label);
        append_label(document, item_id, row.row_id + ".source_path", row.source_path.empty() ? "-" : row.source_path);
        append_label(document, item_id, row.row_id + ".imported_path",
                     row.imported_path.empty() ? "-" : row.imported_path);
        append_label(document, item_id, row.row_id + ".state", row.state_label);
        append_label(document, item_id, row.row_id + ".import_status", row.import_status_label);
        append_label(document, item_id, row.row_id + ".package_status", row.package_status_label);
    }

    return document;
}

EditorAssetBrowserQueryResult plan_editor_asset_browser_query(const EditorAssetBrowserQueryDesc& desc) {
    EditorAssetBrowserQueryResult result;
    const std::vector<std::string_view> tokens = split_query_tokens(desc.query_text);
    if (tokens.empty()) {
        result.rows = desc.rows;
        return result;
    }

    result.tokens.reserve(tokens.size());
    bool blocked = false;
    for (const std::string_view token : tokens) {
        append_normalized_token(result.normalized_query, token);
        if (token.find(':') != std::string_view::npos) {
            blocked = true;
            result.diagnostics.push_back("unsupported query syntax: " + std::string(token));
            result.tokens.push_back(make_query_token(result.tokens.size(), "syntax", std::string(token), true));
            continue;
        }

        const auto equals = token.find('=');
        if (equals == std::string_view::npos) {
            result.tokens.push_back(make_query_token(result.tokens.size(), "text", std::string(token), false));
            continue;
        }

        const std::string key = lower_ascii(token.substr(0, equals));
        const std::string value{token.substr(equals + 1U)};
        if (!is_supported_query_operator(key)) {
            blocked = true;
            result.diagnostics.push_back("unsupported query operator: " + key);
            result.tokens.push_back(make_query_token(result.tokens.size(), key, value, true));
            continue;
        }
        if (value.empty()) {
            blocked = true;
            result.diagnostics.push_back("empty query value: " + key);
            result.tokens.push_back(make_query_token(result.tokens.size(), key, value, true));
            continue;
        }
        result.tokens.push_back(make_query_token(result.tokens.size(), key, value, false));
    }

    if (blocked) {
        result.status = EditorAssetBrowserQueryStatus::blocked;
        result.status_label = "Asset browser query blocked";
        return result;
    }

    result.rows.reserve(desc.rows.size());
    std::ranges::copy_if(desc.rows, std::back_inserter(result.rows),
                         [&result](const auto& row) { return matches_all_query_tokens(row, result.tokens); });
    result.status = EditorAssetBrowserQueryStatus::ready;
    result.status_label = "Asset browser query ready";
    return result;
}

EditorAssetBrowserCommandPlan plan_editor_asset_browser_command(const EditorAssetBrowserCommandRequest& request) {
    EditorAssetBrowserCommandPlan plan{
        .command_id = std::string(editor_asset_browser_command_id(request.kind)),
        .label = std::string(command_label(request.kind)),
        .status = EditorAssetBrowserCommandStatus::blocked,
        .status_label = "blocked",
        .expected_generation = request.expected_generation,
        .current_generation = request.current_generation,
        .requires_user_confirmation = command_requires_confirmation(request.kind),
    };
    append_command_report_rows(plan, request.mode);

    if (request.expected_generation != request.current_generation) {
        plan.status = EditorAssetBrowserCommandStatus::rejected_stale_generation;
        plan.status_label = "rejected_stale_generation";
        plan.diagnostics.push_back("rejected_stale_generation: expected " +
                                   std::to_string(request.expected_generation) + " current " +
                                   std::to_string(request.current_generation));
        return plan;
    }

    if (request.mode == EditorAssetBrowserCommandMode::apply && plan.requires_user_confirmation &&
        !request.user_confirmed) {
        plan.diagnostics.push_back("requires_user_confirmation: " + plan.command_id);
        return plan;
    }

    plan.status = EditorAssetBrowserCommandStatus::ready;
    plan.status_label = "ready";
    if (request.mode == EditorAssetBrowserCommandMode::apply && request.user_confirmed) {
        plan.mutates_project_files = command_mutates_project_files(request.kind);
        plan.executes_import_tools = command_executes_import_tools(request.kind);
    }
    return plan;
}

EditorAssetBrowserLegalProvenanceRow
review_editor_asset_browser_legal_provenance(const EditorAssetBrowserLegalProvenanceRow& row) {
    EditorAssetBrowserLegalProvenanceRow reviewed = row;
    reviewed.accepted_for_package = false;
    reviewed.blocked = true;

    if (is_external_engine_material(row)) {
        reviewed.external_engine_material = true;
        reviewed.status_label = "external_engine_material_rejected";
        reviewed.diagnostic = "external engine material is blocked for clean-room asset browser package use";
        return reviewed;
    }
    if (row.license_id.empty()) {
        reviewed.status_label = "missing_license";
        reviewed.diagnostic = "third-party material requires a license id";
        return reviewed;
    }
    if (row.source_url.empty() || row.retrieved_date.empty() || row.copyright_holder.empty() ||
        row.distribution_target.empty()) {
        reviewed.status_label = "missing_provenance";
        reviewed.diagnostic = "third-party material requires source URL, retrieval date, copyright holder, and target";
        return reviewed;
    }
    if (!row.notice_complete) {
        reviewed.status_label = "missing_notice";
        reviewed.diagnostic = "third-party material requires a complete notice row";
        return reviewed;
    }
    if (is_restricted_license(row.license_id)) {
        reviewed.status_label = "license_restricted";
        reviewed.diagnostic = "non-commercial or no-derivatives material is blocked for production package use";
        return reviewed;
    }

    reviewed.blocked = false;
    reviewed.accepted_for_package = true;
    reviewed.status_label = "accepted_for_package";
    reviewed.diagnostic = "legal provenance row is complete";
    return reviewed;
}

EditorAssetBrowserOpenExrSourceReviewRow
review_editor_asset_browser_open_exr_source(const EditorAssetBrowserOpenExrSourceReviewRow& row) {
    EditorAssetBrowserOpenExrSourceReviewRow reviewed = row;
    reviewed.blocked = true;

    if (!row.optional_importer_feature) {
        reviewed.status_label = "optional_importer_unavailable";
        reviewed.diagnostic = "OpenEXR optional importer evidence is missing";
        return reviewed;
    }
    if (row.source_path.empty() || !row.header_required_attributes_present || row.display_window.empty() ||
        row.data_window.empty() || row.pixel_aspect_ratio.empty() || row.channels.empty() || row.compression.empty() ||
        row.line_order.empty() || row.screen_window_width.empty() || row.screen_window_center.empty() ||
        row.tiled_policy.empty()) {
        reviewed.status_label = "missing_exr_required_metadata";
        reviewed.diagnostic = "OpenEXR required header metadata is incomplete";
        return reviewed;
    }
    if (!supported_open_exr_compression(row.compression) ||
        contains_case_insensitive(row.pixel_type_rows, "unsupported") || row.pixel_type_rows.empty() ||
        unsupported_open_exr_policy(row.tiled_policy) || unsupported_open_exr_policy(row.multipart_policy) ||
        unsupported_open_exr_policy(row.deep_image_policy)) {
        reviewed.status_label = "unsupported_exr_metadata";
        reviewed.diagnostic = "OpenEXR metadata is outside the accepted package review subset";
        return reviewed;
    }
    if (row.scene_linear_claimed && row.declared_color_intent.empty()) {
        reviewed.status_label = "missing_exr_color_intent";
        reviewed.diagnostic = "scene-linear OpenEXR claims require explicit color intent metadata";
        return reviewed;
    }

    reviewed.blocked = false;
    reviewed.status_label = "metadata_ready";
    reviewed.diagnostic = "OpenEXR metadata review is ready";
    return reviewed;
}

EditorAssetBrowserKtx2BasisSourceReviewRow
review_editor_asset_browser_ktx2_basis_source(const EditorAssetBrowserKtx2BasisSourceReviewRow& row) {
    EditorAssetBrowserKtx2BasisSourceReviewRow reviewed = row;
    reviewed.blocked = true;
    reviewed.editor_core_upload_executed = false;

    if (!row.optional_importer_feature) {
        reviewed.status_label = "optional_importer_unavailable";
        reviewed.diagnostic = "KTX2/Basis optional importer evidence is missing";
        return reviewed;
    }
    if (row.gpu_upload_requested || row.editor_core_upload_executed) {
        reviewed.status_label = "editor_core_upload_rejected";
        reviewed.diagnostic = "editor core must not upload KTX2/Basis textures";
        return reviewed;
    }
    if (missing_ktx2_metadata(row) || !row.loaded_with_image_data) {
        reviewed.status_label = "missing_ktx2_metadata";
        reviewed.diagnostic = "KTX2/Basis texture metadata is incomplete";
        return reviewed;
    }
    if (row.needs_transcoding && row.selected_transcode_target.empty()) {
        reviewed.status_label = "ktx2_transcode_target_missing";
        reviewed.diagnostic = "KTX2/Basis transcoding requires a selected target";
        return reviewed;
    }
    if (row.needs_transcoding && row.backend_format_support_evidence_id.empty()) {
        reviewed.status_label = "ktx2_backend_evidence_missing";
        reviewed.diagnostic = "KTX2/Basis transcoding requires backend format-support evidence";
        return reviewed;
    }

    reviewed.blocked = false;
    reviewed.status_label = "metadata_ready";
    reviewed.diagnostic = "KTX2/Basis metadata review is ready";
    return reviewed;
}

} // namespace mirakana::editor
