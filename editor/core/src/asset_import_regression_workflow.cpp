// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_import_regression_workflow.hpp"

#include "mirakana/assets/asset_import_presets.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string sanitize_row_token(std::string_view value) {
    std::string token;
    token.reserve(value.size());
    for (const char character : value) {
        if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.') {
            token.push_back(character);
        } else {
            token.push_back('_');
        }
    }
    return token.empty() ? "unknown" : token;
}

[[nodiscard]] std::string workflow_row_id(std::string_view category, std::string_view asset_id) {
    return "asset_browser.import_workflow." + std::string{category} + "." + sanitize_row_token(asset_id);
}

[[nodiscard]] std::string first_or_dash(const std::vector<std::string>& diagnostics) {
    return diagnostics.empty() ? "-" : diagnostics.front();
}

[[nodiscard]] std::string_view batch_status_label(AssetImportBatchReimportRowStatus status) noexcept {
    switch (status) {
    case AssetImportBatchReimportRowStatus::skipped:
        return "skipped";
    case AssetImportBatchReimportRowStatus::ready:
        return "ready";
    case AssetImportBatchReimportRowStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

[[nodiscard]] std::string_view preset_diff_status_label(AssetImportPresetDiffRowStatus status) noexcept {
    switch (status) {
    case AssetImportPresetDiffRowStatus::unchanged:
        return "unchanged";
    case AssetImportPresetDiffRowStatus::changed:
        return "changed";
    case AssetImportPresetDiffRowStatus::review_blocked:
        return "review_blocked";
    }
    return "review_blocked";
}

[[nodiscard]] std::string_view action_kind_label(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
        return "texture";
    case AssetImportActionKind::mesh:
        return "mesh";
    case AssetImportActionKind::morph_mesh_cpu:
        return "morph_mesh_cpu";
    case AssetImportActionKind::animation_float_clip:
        return "animation_float_clip";
    case AssetImportActionKind::animation_quaternion_clip:
        return "animation_quaternion_clip";
    case AssetImportActionKind::material:
        return "material";
    case AssetImportActionKind::scene:
        return "scene";
    case AssetImportActionKind::audio:
        return "audio";
    case AssetImportActionKind::environment_profile:
        return "environment_profile";
    case AssetImportActionKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_corpus_row(const AssetImportRegressionCorpusAssetV1& asset) {
    const bool blocked = asset.license_policy == AssetImportRegressionLicensePolicy::rejected ||
                         asset.provenance.external_engine_material;
    const std::string status = blocked ? "blocked" : "registered";
    std::string diagnostic = "legal provenance accepted for corpus review";
    if (asset.license_policy == AssetImportRegressionLicensePolicy::rejected) {
        diagnostic = "asset import regression license policy rejected";
    }
    if (asset.provenance.external_engine_material) {
        diagnostic = "external_engine_material rejected for clean-room corpus use";
    }
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id("corpus", asset.asset_id),
        .category_label = "corpus_asset",
        .asset_id = asset.asset_id,
        .asset_key_label = asset.asset_key.value,
        .source_path = asset.source_path,
        .status_label = status,
        .detail_label = "kind=" + std::string{asset_import_regression_asset_kind_label(asset.kind)} +
                        " license=" + std::string{asset_import_regression_license_policy_label(asset.license_policy)},
        .diagnostic = std::move(diagnostic),
        .ready = !blocked,
        .blocked = blocked,
    };
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_report_row(const AssetImportRegressionReportRowV1& report) {
    const bool blocked = !report.succeeded || !report.ready_for_commercial_evidence;
    const std::string status = blocked ? "blocked" : "ready";
    std::string detail =
        "phase=" + report.phase + " code=" + std::string{asset_import_regression_diagnostic_code_label(report.code)};
    if (!report.deterministic_output_hash.empty()) {
        detail += " output_hash=" + report.deterministic_output_hash;
    }
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id("report", report.asset_id),
        .category_label = "report",
        .asset_id = report.asset_id,
        .source_path = report.source_path,
        .status_label = status,
        .detail_label = std::move(detail),
        .diagnostic = report.message.empty() ? "latest importer regression report row reviewed" : report.message,
        .ready = !blocked,
        .blocked = blocked,
    };
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_triage_row(const AssetImportRegressionTriageRowV1& row) {
    const bool failed = row.code != AssetImportRegressionDiagnosticCode::none;
    const bool blocked = row.reimport_decision == AssetImportRegressionReimportDecision::blocked;
    std::string detail =
        "phase=" + row.phase + " code=" + std::string{asset_import_regression_diagnostic_code_label(row.code)} +
        " severity=" + std::string{asset_import_regression_triage_severity_label(row.severity)} +
        " recommended_action=" + std::string{asset_import_regression_recommended_action_label(row.recommended_action)} +
        " reimport_decision=" + std::string{asset_import_regression_reimport_decision_label(row.reimport_decision)} +
        " repro=" + row.repro_command_id;
    if (row.preset_diff_required) {
        detail += " preset_diff_required=true";
    }
    if (row.axis_unit_preview_required) {
        detail += " axis_unit_preview_required=true";
    }
    if (row.legal_blocked) {
        detail += " legal_blocked=true";
    }
    if (row.nondeterministic) {
        detail += " nondeterministic=true";
    }
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id(failed ? "failure" : "triage", row.asset_id),
        .category_label = failed ? "triage_failure" : "triage_review",
        .asset_id = row.asset_id,
        .asset_key_label = std::to_string(row.asset.value),
        .source_path = row.source_path,
        .status_label = std::string{asset_import_regression_triage_severity_label(row.severity)},
        .detail_label = std::move(detail),
        .diagnostic = "operator triage row reviewed",
        .ready = !blocked,
        .blocked = blocked,
    };
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_batch_row(const AssetImportBatchReimportPlanRow& batch) {
    const bool blocked = batch.status == AssetImportBatchReimportRowStatus::blocked;
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id("batch_reimport", batch.asset_id),
        .category_label = "batch_reimport",
        .asset_id = batch.asset_id,
        .source_path = batch.source_path,
        .status_label = std::string{batch_status_label(batch.status)},
        .detail_label = "action=" + std::string{action_kind_label(batch.kind)} + " output=" + batch.output_path +
                        " staging=" + batch.staging_path,
        .diagnostic = batch.diagnostic.empty() ? "batch reimport row reviewed" : batch.diagnostic,
        .ready = batch.status == AssetImportBatchReimportRowStatus::ready,
        .blocked = blocked,
        .selected = batch.selected,
    };
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_preset_diff_row(const AssetImportPresetDiffRow& diff) {
    const bool blocked = diff.status == AssetImportPresetDiffRowStatus::review_blocked || diff.review_blocked;
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id("preset_diff", diff.asset_id),
        .category_label = "preset_diff",
        .asset_id = diff.asset_id,
        .asset_key_label = diff.asset_key.value,
        .source_path = diff.source_path,
        .status_label = std::string{preset_diff_status_label(diff.status)},
        .detail_label = "fields=" + std::to_string(diff.field_changes.size()) +
                        " source_review=" + std::to_string(diff.source_review_required ? 1 : 0) +
                        " cooked_output=" + std::to_string(diff.cooked_output_changes ? 1 : 0) +
                        " package_output=" + std::to_string(diff.package_output_changes ? 1 : 0),
        .diagnostic = first_or_dash(diff.diagnostics),
        .ready = !blocked,
        .blocked = blocked,
    };
}

[[nodiscard]] EditorAssetBrowserImportWorkflowRow make_axis_preview_row(const AssetAxisUnitPreview& preview) {
    const bool blocked = !preview.ready();
    return EditorAssetBrowserImportWorkflowRow{
        .id = workflow_row_id("axis_unit_preview", preview.asset_id),
        .category_label = "axis_unit_preview",
        .asset_id = preview.asset_id,
        .source_path = preview.source_path,
        .status_label = blocked ? "blocked" : "ready",
        .detail_label = "unit_scale=" + std::to_string(preview.unit_scale) +
                        " up_axis=" + std::string{asset_import_mesh_up_axis_label(preview.up_axis)} +
                        " samples=" + std::to_string(preview.rows.size()) +
                        " changes_coordinates=" + std::to_string(preview.changes_coordinates ? 1 : 0),
        .diagnostic = first_or_dash(preview.diagnostics),
        .ready = !blocked,
        .blocked = blocked,
    };
}

void append_command(std::vector<EditorAssetBrowserRetainedCommandRow>& rows,
                    EditorAssetImportRegressionWorkflowCommandKind kind, bool enabled, bool requires_confirmation) {
    const std::string command_id{editor_asset_import_regression_workflow_command_id(kind)};
    std::string label;
    switch (kind) {
    case EditorAssetImportRegressionWorkflowCommandKind::run_corpus:
        label = "Run importer corpus";
        break;
    case EditorAssetImportRegressionWorkflowCommandKind::open_report:
        label = "Open importer report";
        break;
    case EditorAssetImportRegressionWorkflowCommandKind::batch_reimport:
        label = "Batch reimport";
        break;
    case EditorAssetImportRegressionWorkflowCommandKind::preset_diff:
        label = "Show preset diff";
        break;
    case EditorAssetImportRegressionWorkflowCommandKind::axis_unit_preview:
        label = "Show axis/unit preview";
        break;
    }
    rows.push_back(EditorAssetBrowserRetainedCommandRow{
        .command_id = command_id,
        .label = std::move(label),
        .status_label = enabled ? "ready" : "blocked",
        .enabled = enabled,
        .requires_user_confirmation = requires_confirmation,
    });
}

void append_diagnostics(std::vector<std::string>& target, const std::vector<std::string>& diagnostics) {
    target.insert(target.end(), diagnostics.begin(), diagnostics.end());
}

void refresh_summary(EditorAssetImportRegressionWorkflowModel& model) {
    model.ready_row_count = static_cast<std::uint64_t>(
        std::ranges::count_if(model.rows, [](const auto& row) { return row.ready && !row.blocked; }));
    model.blocked_row_count =
        static_cast<std::uint64_t>(std::ranges::count_if(model.rows, [](const auto& row) { return row.blocked; }));
    model.mutates_project_files =
        std::ranges::any_of(model.rows, [](const auto& row) { return row.mutates_project_files; });
    model.executes_import_tools =
        std::ranges::any_of(model.rows, [](const auto& row) { return row.executes_import_tools; });
    model.executes_package_scripts =
        std::ranges::any_of(model.rows, [](const auto& row) { return row.executes_package_scripts; });
    model.executes_validation_recipes =
        std::ranges::any_of(model.rows, [](const auto& row) { return row.executes_validation_recipes; });
    model.exposes_native_handles =
        std::ranges::any_of(model.rows, [](const auto& row) { return row.exposes_native_handles; });

    if (model.rows.empty()) {
        model.status = EditorAssetImportRegressionWorkflowStatus::empty;
    } else if (!model.diagnostics.empty() || model.blocked_row_count > 0U) {
        model.status = EditorAssetImportRegressionWorkflowStatus::blocked;
    } else {
        model.status = EditorAssetImportRegressionWorkflowStatus::ready;
    }
    model.status_label = std::string{editor_asset_import_regression_workflow_status_label(model.status)};
}

} // namespace

std::string_view
editor_asset_import_regression_workflow_status_label(EditorAssetImportRegressionWorkflowStatus status) noexcept {
    switch (status) {
    case EditorAssetImportRegressionWorkflowStatus::empty:
        return "Asset import regression workflow empty";
    case EditorAssetImportRegressionWorkflowStatus::ready:
        return "Asset import regression workflow ready";
    case EditorAssetImportRegressionWorkflowStatus::blocked:
        return "Asset import regression workflow blocked";
    }
    return "Asset import regression workflow blocked";
}

std::string_view
editor_asset_import_regression_workflow_command_id(EditorAssetImportRegressionWorkflowCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetImportRegressionWorkflowCommandKind::run_corpus:
        return "asset_browser.importer_corpus.run";
    case EditorAssetImportRegressionWorkflowCommandKind::open_report:
        return "asset_browser.importer_corpus.open_report";
    case EditorAssetImportRegressionWorkflowCommandKind::batch_reimport:
        return "asset_browser.import.batch_reimport";
    case EditorAssetImportRegressionWorkflowCommandKind::preset_diff:
        return "asset_browser.import.preset_diff";
    case EditorAssetImportRegressionWorkflowCommandKind::axis_unit_preview:
        return "asset_browser.import.axis_unit_preview";
    }
    return "asset_browser.importer_corpus.run";
}

EditorAssetImportRegressionWorkflowModel
make_editor_asset_import_regression_workflow_model(const EditorAssetImportRegressionWorkflowDesc& desc) {
    EditorAssetImportRegressionWorkflowModel model;
    model.generation = desc.generation;

    bool corpus_ready = false;
    if (desc.corpus != nullptr) {
        append_diagnostics(model.diagnostics, validate_asset_import_regression_corpus_v1(*desc.corpus));
        model.rows.reserve(model.rows.size() + desc.corpus->assets.size());
        for (const auto& asset : desc.corpus->assets) {
            model.rows.push_back(make_corpus_row(asset));
        }
        corpus_ready =
            model.diagnostics.empty() && std::ranges::none_of(model.rows, [](const auto& row) { return row.blocked; });
    }

    if (desc.latest_report != nullptr) {
        model.rows.reserve(model.rows.size() + desc.latest_report->rows.size());
        for (const auto& row : desc.latest_report->rows) {
            model.rows.push_back(make_report_row(row));
        }
    }

    bool triage_reimport_ready = false;
    bool triage_has_legal_blocker = false;
    bool triage_preset_diff_ready = false;
    bool triage_axis_preview_ready = false;
    if (desc.triage != nullptr) {
        model.rows.reserve(model.rows.size() + desc.triage->rows.size());
        for (const auto& row : desc.triage->rows) {
            model.rows.push_back(make_triage_row(row));
            triage_reimport_ready = triage_reimport_ready ||
                                    row.reimport_decision == AssetImportRegressionReimportDecision::dry_run_allowed;
            triage_has_legal_blocker = triage_has_legal_blocker || row.legal_blocked;
            triage_preset_diff_ready = triage_preset_diff_ready || row.preset_diff_required;
            triage_axis_preview_ready = triage_axis_preview_ready || row.axis_unit_preview_required;
        }
    }

    bool batch_ready = false;
    if (desc.batch_reimport != nullptr) {
        model.rows.reserve(model.rows.size() + desc.batch_reimport->rows.size());
        for (const auto& row : desc.batch_reimport->rows) {
            model.rows.push_back(make_batch_row(row));
        }
        append_diagnostics(model.diagnostics, desc.batch_reimport->diagnostics);
        batch_ready = desc.batch_reimport->ready();
    }

    bool preset_diff_ready = false;
    if (desc.preset_diff != nullptr) {
        model.rows.reserve(model.rows.size() + desc.preset_diff->rows.size());
        for (const auto& row : desc.preset_diff->rows) {
            model.rows.push_back(make_preset_diff_row(row));
        }
        append_diagnostics(model.diagnostics, desc.preset_diff->diagnostics);
        preset_diff_ready = desc.preset_diff->ready();
    }

    bool preview_ready = false;
    if (!desc.axis_unit_previews.empty()) {
        model.rows.reserve(model.rows.size() + desc.axis_unit_previews.size());
        for (const auto& preview : desc.axis_unit_previews) {
            model.rows.push_back(make_axis_preview_row(preview));
            append_diagnostics(model.diagnostics, preview.diagnostics);
        }
        preview_ready =
            std::ranges::all_of(desc.axis_unit_previews, [](const auto& preview) { return preview.ready(); });
    }

    append_command(model.command_rows, EditorAssetImportRegressionWorkflowCommandKind::run_corpus, corpus_ready, true);
    append_command(model.command_rows, EditorAssetImportRegressionWorkflowCommandKind::open_report,
                   desc.latest_report != nullptr, false);
    append_command(model.command_rows, EditorAssetImportRegressionWorkflowCommandKind::batch_reimport,
                   batch_ready || (triage_reimport_ready && !triage_has_legal_blocker), true);
    append_command(model.command_rows, EditorAssetImportRegressionWorkflowCommandKind::preset_diff,
                   preset_diff_ready || triage_preset_diff_ready, false);
    append_command(model.command_rows, EditorAssetImportRegressionWorkflowCommandKind::axis_unit_preview,
                   preview_ready || triage_axis_preview_ready, false);

    refresh_summary(model);
    return model;
}

EditorAssetBrowserRetainedUiDesc
make_editor_asset_import_regression_workflow_retained_ui_desc(const EditorAssetImportRegressionWorkflowModel& model) {
    return EditorAssetBrowserRetainedUiDesc{
        .query_status_label = model.status_label,
        .command_rows = model.command_rows,
        .import_workflow_rows = model.rows,
    };
}

} // namespace mirakana::editor
