// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_import_jobs.hpp"

#include <algorithm>
#include <string>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string make_progress_label(std::size_t completed, std::size_t total) {
    return std::to_string(completed) + "/" + std::to_string(total);
}

[[nodiscard]] EditorAssetImportJobRow* find_job(EditorAssetImportJobSnapshot& snapshot, std::string_view job_id) {
    const auto it =
        std::ranges::find_if(snapshot.rows, [job_id](const EditorAssetImportJobRow& row) { return row.id == job_id; });
    return it == snapshot.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const EditorAssetImportJobRow* find_job(const EditorAssetImportJobSnapshot& snapshot,
                                                      std::string_view job_id) {
    const auto it =
        std::ranges::find_if(snapshot.rows, [job_id](const EditorAssetImportJobRow& row) { return row.id == job_id; });
    return it == snapshot.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] std::uint64_t next_job_sequence(const EditorAssetImportJobSnapshot& snapshot) noexcept {
    std::uint64_t sequence = 0;
    for (const auto& row : snapshot.rows) {
        sequence = std::max(sequence, row.sequence);
    }
    return sequence + 1U;
}

[[nodiscard]] std::string make_job_id(std::uint64_t sequence) {
    return "asset_import_job." + std::to_string(sequence);
}

} // namespace

std::string_view editor_asset_import_job_state_label(EditorAssetImportJobState state) noexcept {
    switch (state) {
    case EditorAssetImportJobState::queued:
        return "queued";
    case EditorAssetImportJobState::copying:
        return "copying";
    case EditorAssetImportJobState::registering:
        return "registering";
    case EditorAssetImportJobState::importing:
        return "importing";
    case EditorAssetImportJobState::refreshing:
        return "refreshing";
    case EditorAssetImportJobState::succeeded:
        return "succeeded";
    case EditorAssetImportJobState::failed:
        return "failed";
    case EditorAssetImportJobState::canceled:
        return "canceled";
    }
    return "queued";
}

std::string_view editor_asset_import_job_command_kind_label(EditorAssetImportJobCommandKind kind) noexcept {
    switch (kind) {
    case EditorAssetImportJobCommandKind::cancel:
        return "cancel";
    case EditorAssetImportJobCommandKind::retry:
        return "retry";
    }
    return "cancel";
}

bool editor_asset_import_job_state_is_active(EditorAssetImportJobState state) noexcept {
    switch (state) {
    case EditorAssetImportJobState::queued:
    case EditorAssetImportJobState::copying:
    case EditorAssetImportJobState::registering:
    case EditorAssetImportJobState::importing:
    case EditorAssetImportJobState::refreshing:
        return true;
    case EditorAssetImportJobState::succeeded:
    case EditorAssetImportJobState::failed:
    case EditorAssetImportJobState::canceled:
        return false;
    }
    return false;
}

bool editor_asset_import_job_state_can_retry(EditorAssetImportJobState state) noexcept {
    return state == EditorAssetImportJobState::failed || state == EditorAssetImportJobState::canceled;
}

EditorAssetImportJobSnapshot make_editor_asset_import_job_model(const EditorAssetImportJobSnapshot& snapshot) {
    auto model = snapshot;
    std::ranges::sort(model.rows, [](const EditorAssetImportJobRow& lhs, const EditorAssetImportJobRow& rhs) {
        if (lhs.sequence == rhs.sequence) {
            return lhs.id < rhs.id;
        }
        return lhs.sequence < rhs.sequence;
    });

    model.active_count = 0;
    model.completed_count = 0;
    model.failed_count = 0;
    model.canceled_count = 0;
    model.exposes_native_handles = false;

    for (auto& row : model.rows) {
        row.state_label = std::string{editor_asset_import_job_state_label(row.state)};
        row.progress_label = make_progress_label(row.completed_steps, row.total_steps);
        row.can_cancel = editor_asset_import_job_state_is_active(row.state);
        row.can_retry = editor_asset_import_job_state_can_retry(row.state);
        row.exposes_native_handles = false;
        model.exposes_native_handles = model.exposes_native_handles || row.exposes_native_handles;

        if (editor_asset_import_job_state_is_active(row.state)) {
            ++model.active_count;
        } else {
            ++model.completed_count;
        }
        if (row.state == EditorAssetImportJobState::failed) {
            ++model.failed_count;
        }
        if (row.state == EditorAssetImportJobState::canceled) {
            ++model.canceled_count;
        }
    }

    return model;
}

EditorAssetImportJobCommandPlan
plan_editor_asset_import_job_command(const EditorAssetImportJobSnapshot& snapshot,
                                     const EditorAssetImportJobCommandRequest& request) {
    EditorAssetImportJobCommandPlan plan{
        .kind = request.kind,
        .command_id = "asset_import_jobs." + std::string{editor_asset_import_job_command_kind_label(request.kind)},
        .job_id = request.job_id,
    };

    if (request.expected_generation != request.current_generation ||
        request.current_generation != snapshot.generation) {
        plan.status = EditorAssetImportJobCommandStatus::rejected_stale_generation;
        plan.diagnostics.push_back("asset_import_job_rejected_stale_generation");
        return plan;
    }
    if (!request.user_confirmed) {
        plan.status = EditorAssetImportJobCommandStatus::blocked;
        plan.diagnostics.push_back("asset_import_job_command_requires_confirmation");
        return plan;
    }

    const auto* row = find_job(snapshot, request.job_id);
    if (row == nullptr) {
        plan.status = EditorAssetImportJobCommandStatus::unknown_job;
        plan.diagnostics.push_back("asset_import_job_unknown");
        return plan;
    }

    switch (request.kind) {
    case EditorAssetImportJobCommandKind::cancel:
        if (!editor_asset_import_job_state_is_active(row->state)) {
            plan.status = EditorAssetImportJobCommandStatus::blocked;
            plan.diagnostics.push_back("asset_import_job_cancel_requires_active_job");
            return plan;
        }
        break;
    case EditorAssetImportJobCommandKind::retry:
        if (!editor_asset_import_job_state_can_retry(row->state)) {
            plan.status = EditorAssetImportJobCommandStatus::blocked;
            plan.diagnostics.push_back("asset_import_job_retry_requires_failed_or_canceled_job");
            return plan;
        }
        break;
    }

    plan.status = EditorAssetImportJobCommandStatus::ready;
    return plan;
}

EditorAssetImportJobCommandResult
apply_editor_asset_import_job_command(const EditorAssetImportJobSnapshot& snapshot,
                                      const EditorAssetImportJobCommandRequest& request) {
    EditorAssetImportJobCommandResult result;
    result.snapshot = make_editor_asset_import_job_model(snapshot);
    result.plan = plan_editor_asset_import_job_command(result.snapshot, request);
    if (result.plan.status != EditorAssetImportJobCommandStatus::ready) {
        return result;
    }

    auto updated = result.snapshot;
    switch (request.kind) {
    case EditorAssetImportJobCommandKind::cancel:
        if (auto* row = find_job(updated, request.job_id)) {
            row->state = EditorAssetImportJobState::canceled;
            row->diagnostic = "asset import job canceled before next mutation boundary";
            row->imported_count = 0;
            row->failed_count = 0;
        }
        break;
    case EditorAssetImportJobCommandKind::retry:
        if (const auto* row = find_job(updated, request.job_id)) {
            const auto sequence = next_job_sequence(updated);
            updated.rows.push_back(EditorAssetImportJobRow{
                .id = make_job_id(sequence),
                .parent_job_id = row->id,
                .sequence = sequence,
                .state = EditorAssetImportJobState::queued,
                .source_count = row->source_count,
                .total_steps = row->total_steps,
                .mutates_project_files = row->mutates_project_files,
                .executes_import_tools = row->executes_import_tools,
            });
        }
        break;
    }

    ++updated.generation;
    result.snapshot = make_editor_asset_import_job_model(updated);
    result.applied = true;
    return result;
}

} // namespace mirakana::editor
