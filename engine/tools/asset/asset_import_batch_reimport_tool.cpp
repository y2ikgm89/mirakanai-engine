// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_import_batch_reimport_tool.hpp"

#include "mirakana/assets/asset_package.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

struct PendingApply {
    AssetImportedArtifact artifact;
    std::string project_output_path;
    std::string staged_content;
};

[[nodiscard]] std::string hex_u64(std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << value;
    return output.str();
}

[[nodiscard]] const AssetImportAction* find_action(const AssetImportPlan& plan, AssetId asset) noexcept {
    const auto it =
        std::ranges::find_if(plan.actions, [asset](const AssetImportAction& action) { return action.id == asset; });
    return it == plan.actions.end() ? nullptr : &*it;
}

[[nodiscard]] std::vector<AssetImportBatchReimportPlanRow> ready_rows(const AssetImportBatchReimportPlan& plan) {
    std::vector<AssetImportBatchReimportPlanRow> rows;
    for (const auto& row : plan.rows) {
        if (row.selected && row.status == AssetImportBatchReimportRowStatus::ready) {
            rows.push_back(row);
        }
    }
    return rows;
}

void append_plan_diagnostics(AssetImportBatchReimportExecutionResult& result) {
    for (const auto& diagnostic : result.plan.diagnostics) {
        result.diagnostics.push_back(diagnostic);
    }
}

void append_import_failures(AssetImportBatchReimportExecutionResult& result) {
    for (const auto& failure : result.staged_import.failures) {
        result.diagnostics.push_back("staged import failed: " + failure.diagnostic);
    }
}

[[nodiscard]] AssetImportPlan make_staging_plan(const AssetImportPlan& import_plan,
                                                const std::vector<AssetImportBatchReimportPlanRow>& rows,
                                                AssetImportBatchReimportExecutionResult& result) {
    AssetImportPlan staging_plan;
    staging_plan.actions.reserve(rows.size());
    for (const auto& row : rows) {
        const auto* action = find_action(import_plan, row.asset);
        if (action == nullptr) {
            result.diagnostics.push_back("ready row has no matching import action: " + row.asset_id);
            continue;
        }
        auto staging_action = *action;
        staging_action.output_path = row.staging_path;
        staging_plan.actions.push_back(std::move(staging_action));
    }
    return staging_plan;
}

[[nodiscard]] std::string output_hash(std::string_view content) {
    return "fnv64:" + hex_u64(hash_asset_cooked_content(content));
}

[[nodiscard]] std::vector<PendingApply>
validate_staged_outputs(IFileSystem& filesystem, const std::vector<AssetImportBatchReimportPlanRow>& rows,
                        AssetImportBatchReimportExecutionResult& result) {
    std::vector<PendingApply> pending;
    pending.reserve(rows.size());
    for (const auto& row : rows) {
        if (!filesystem.exists(row.staging_path)) {
            result.diagnostics.push_back("missing staged output: " + row.staging_path);
            continue;
        }
        auto staged_content = filesystem.read_text(row.staging_path);
        if (row.output_validation_required && output_hash(staged_content) != row.expected_output_hash) {
            result.diagnostics.push_back("staged output hash mismatch: " + row.asset_id);
            continue;
        }
        pending.push_back(PendingApply{
            .artifact =
                AssetImportedArtifact{
                    .asset = row.asset,
                    .kind = row.kind,
                    .output_path = row.staging_path,
                },
            .project_output_path = row.output_path,
            .staged_content = std::move(staged_content),
        });
    }
    return pending;
}

} // namespace

AssetImportBatchReimportExecutionResult execute_asset_import_batch_reimport(IFileSystem& filesystem,
                                                                            const AssetImportBatchReimportDesc& desc) {
    return execute_asset_import_batch_reimport(filesystem, desc, AssetImportExecutionOptions{});
}

AssetImportBatchReimportExecutionResult
execute_asset_import_batch_reimport(IFileSystem& filesystem, const AssetImportBatchReimportDesc& desc,
                                    const AssetImportExecutionOptions& options) {
    AssetImportBatchReimportExecutionResult result;
    result.plan = plan_asset_import_batch_reimport(desc);
    append_plan_diagnostics(result);

    if (!desc.apply_requested) {
        return result;
    }
    if (!result.plan.apply_allowed) {
        result.diagnostics.push_back("batch reimport apply is not allowed");
        return result;
    }

    const auto rows = ready_rows(result.plan);
    auto staging_plan = make_staging_plan(desc.import_plan, rows, result);
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.staged_import = execute_asset_import_plan(filesystem, staging_plan, options);
    if (!result.staged_import.succeeded()) {
        append_import_failures(result);
        return result;
    }
    result.staged = true;
    result.staged_artifacts = result.staged_import.imported;

    auto pending = validate_staged_outputs(filesystem, rows, result);
    if (!result.diagnostics.empty() || pending.size() != rows.size()) {
        return result;
    }

    for (const auto& item : pending) {
        filesystem.write_text(item.project_output_path, item.staged_content);
        result.applied_artifacts.push_back(AssetImportedArtifact{
            .asset = item.artifact.asset,
            .kind = item.artifact.kind,
            .output_path = item.project_output_path,
        });
    }
    result.applied = true;
    result.project_outputs_mutated = !pending.empty();
    return result;
}

} // namespace mirakana
