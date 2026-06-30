// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_batch_reimport.hpp"

#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool safe_relative_path(std::string_view value) noexcept {
    return clean_text(value) && value.front() != '/' && value.find('\\') == std::string_view::npos &&
           value.find(':') == std::string_view::npos && !contains_parent_segment(value);
}

[[nodiscard]] bool safe_staging_token(std::string_view value) noexcept {
    return clean_text(value) && value.find('/') == std::string_view::npos &&
           value.find('\\') == std::string_view::npos && value.find(':') == std::string_view::npos && value != "." &&
           value != "..";
}

[[nodiscard]] std::string join_path(std::string_view root, std::string_view leaf) {
    std::string path{root};
    if (!path.ends_with('/')) {
        path.push_back('/');
    }
    path += leaf;
    return path;
}

[[nodiscard]] bool legal_blocked_code(AssetImportRegressionDiagnosticCode code) noexcept {
    switch (code) {
    case AssetImportRegressionDiagnosticCode::missing_license_provenance:
    case AssetImportRegressionDiagnosticCode::rejected_license:
    case AssetImportRegressionDiagnosticCode::external_engine_material:
        return true;
    case AssetImportRegressionDiagnosticCode::none:
    case AssetImportRegressionDiagnosticCode::invalid_manifest:
    case AssetImportRegressionDiagnosticCode::duplicate_asset_id:
    case AssetImportRegressionDiagnosticCode::unsafe_source_path:
    case AssetImportRegressionDiagnosticCode::missing_source_file:
    case AssetImportRegressionDiagnosticCode::source_hash_mismatch:
    case AssetImportRegressionDiagnosticCode::unsupported_format:
    case AssetImportRegressionDiagnosticCode::parser_error:
    case AssetImportRegressionDiagnosticCode::validator_error:
    case AssetImportRegressionDiagnosticCode::missing_external_resource:
    case AssetImportRegressionDiagnosticCode::unsafe_external_resource_path:
    case AssetImportRegressionDiagnosticCode::unsupported_extension:
    case AssetImportRegressionDiagnosticCode::unsupported_animation_channel:
    case AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination:
    case AssetImportRegressionDiagnosticCode::coordinate_normalization_failed:
    case AssetImportRegressionDiagnosticCode::material_extraction_failed:
    case AssetImportRegressionDiagnosticCode::texture_decode_failed:
    case AssetImportRegressionDiagnosticCode::texture_transcode_failed:
    case AssetImportRegressionDiagnosticCode::cooked_output_mismatch:
    case AssetImportRegressionDiagnosticCode::nondeterministic_output:
    case AssetImportRegressionDiagnosticCode::row_budget_exceeded:
        break;
    }
    return false;
}

using ActionByAsset = std::unordered_map<AssetId, const AssetImportAction*, AssetIdHash>;

[[nodiscard]] ActionByAsset map_actions(const AssetImportPlan& import_plan, AssetImportBatchReimportPlan& plan) {
    ActionByAsset actions;
    actions.reserve(import_plan.actions.size());
    for (const auto& action : import_plan.actions) {
        if (!is_valid_asset_import_action(action)) {
            plan.diagnostics.push_back("import_plan.invalid_action");
            continue;
        }
        const auto [_, inserted] = actions.emplace(action.id, &action);
        if (!inserted) {
            plan.diagnostics.push_back("import_plan.duplicate_asset_action");
        }
    }
    return actions;
}

[[nodiscard]] std::unordered_set<std::string> selected_ids(const std::vector<std::string>& ids,
                                                           AssetImportBatchReimportPlan& plan) {
    std::unordered_set<std::string> selected;
    selected.reserve(ids.size());
    for (const auto& id : ids) {
        if (!safe_staging_token(id)) {
            plan.diagnostics.push_back("selection.invalid_asset_id");
            continue;
        }
        const auto [_, inserted] = selected.insert(id);
        if (!inserted) {
            plan.diagnostics.push_back("selection.duplicate_asset_id");
        }
    }
    return selected;
}

void block_row(AssetImportBatchReimportPlanRow& row, std::string diagnostic) {
    row.status = AssetImportBatchReimportRowStatus::blocked;
    row.diagnostic = std::move(diagnostic);
}

} // namespace

AssetImportBatchReimportPlan plan_asset_import_batch_reimport(const AssetImportBatchReimportDesc& desc) {
    AssetImportBatchReimportPlan plan;
    plan.apply_requested = desc.apply_requested;
    plan.dry_run_acknowledged = desc.dry_run_acknowledged;
    plan.dry_run_required = desc.apply_requested && !desc.dry_run_acknowledged;

    if (!safe_staging_token(desc.run_id)) {
        plan.diagnostics.push_back("run_id.invalid");
    }
    if (!safe_relative_path(desc.staging_root)) {
        plan.diagnostics.push_back("staging_root.invalid");
    }
    if (desc.max_selected_assets == 0U) {
        plan.diagnostics.push_back("max_selected_assets.invalid");
    }
    if (desc.apply_requested && !desc.dry_run_acknowledged) {
        plan.diagnostics.push_back("dry_run.required_before_apply");
    }

    const auto actions = map_actions(desc.import_plan, plan);
    const auto selected = selected_ids(desc.selected_asset_ids, plan);
    const auto select_all = desc.selected_asset_ids.empty();

    std::unordered_set<std::string> seen_report_rows;
    seen_report_rows.reserve(desc.report.rows.size());
    plan.rows.reserve(desc.report.rows.size());
    for (const auto& report_row : desc.report.rows) {
        AssetImportBatchReimportPlanRow row{
            .asset_id = report_row.asset_id,
            .asset = report_row.asset,
            .source_path = report_row.source_path,
            .source_sha256 = report_row.source_sha256,
            .preset_sha256 = report_row.preset_sha256,
            .importer_id = report_row.importer_id,
            .expected_output_hash = report_row.deterministic_output_hash,
            .selected = select_all || selected.find(report_row.asset_id) != selected.end(),
            .legal_blocked = legal_blocked_code(report_row.code),
            .output_validation_required = !report_row.deterministic_output_hash.empty(),
        };

        if (!safe_staging_token(report_row.asset_id)) {
            block_row(row, "asset id is unsafe for staging");
        }
        const auto [_, inserted] = seen_report_rows.insert(report_row.asset_id);
        if (!inserted) {
            block_row(row, "duplicate report row");
        }
        const auto action = actions.find(report_row.asset);
        if (action == actions.end() || action->second == nullptr) {
            block_row(row, "missing import action");
        } else {
            row.kind = action->second->kind;
            row.output_path = action->second->output_path;
            if (action->second->source_path != report_row.source_path) {
                block_row(row, "source path drift");
            }
        }

        if (!row.selected) {
            row.status = AssetImportBatchReimportRowStatus::skipped;
        } else {
            ++plan.selected_count;
            if (row.status != AssetImportBatchReimportRowStatus::blocked) {
                if (row.legal_blocked) {
                    block_row(row, "legal policy blocks reimport");
                } else if (!report_row.succeeded || !report_row.ready_for_commercial_evidence) {
                    block_row(row, "source report row is not ready");
                } else if (!safe_relative_path(row.source_path) || !safe_relative_path(row.output_path)) {
                    block_row(row, "source or output path is unsafe");
                } else {
                    row.status = AssetImportBatchReimportRowStatus::ready;
                    row.staging_path = join_path(join_path(desc.staging_root, desc.run_id), row.asset_id + ".cooked");
                }
            }
        }
        plan.rows.push_back(std::move(row));
    }

    if (plan.selected_count > desc.max_selected_assets) {
        plan.diagnostics.push_back("selection.max_selected_assets_exceeded");
    }

    for (const auto& id : selected) {
        const auto found = std::ranges::find_if(
            plan.rows, [&id](const AssetImportBatchReimportPlanRow& row) { return row.asset_id == id; });
        if (found == plan.rows.end()) {
            plan.diagnostics.push_back("selection.missing_report_row");
        }
    }

    for (const auto& row : plan.rows) {
        if (!row.selected) {
            continue;
        }
        if (row.status == AssetImportBatchReimportRowStatus::ready) {
            ++plan.ready_row_count;
        } else {
            ++plan.blocked_row_count;
        }
    }

    plan.apply_allowed = desc.apply_requested && desc.dry_run_acknowledged && plan.ready() &&
                         plan.selected_count > 0U && plan.selected_count <= desc.max_selected_assets;
    plan.ready_for_apply = plan.apply_allowed;
    plan.mutates_project_outputs = plan.apply_allowed;
    return plan;
}

} // namespace mirakana
