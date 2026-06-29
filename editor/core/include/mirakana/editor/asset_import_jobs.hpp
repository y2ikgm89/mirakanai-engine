// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetImportJobState : std::uint8_t {
    queued,
    copying,
    registering,
    importing,
    refreshing,
    succeeded,
    failed,
    canceled,
};

enum class EditorAssetImportJobCommandKind : std::uint8_t {
    cancel,
    retry,
};

enum class EditorAssetImportJobCommandStatus : std::uint8_t {
    ready,
    blocked,
    rejected_stale_generation,
    unknown_job,
};

struct EditorAssetImportJobRow {
    std::string id;
    std::string parent_job_id;
    std::uint64_t sequence{0};
    EditorAssetImportJobState state{EditorAssetImportJobState::queued};
    std::string state_label;
    std::string progress_label;
    std::string diagnostic;
    std::size_t source_count{0};
    std::size_t total_steps{0};
    std::size_t completed_steps{0};
    std::size_t imported_count{0};
    std::size_t failed_count{0};
    bool can_cancel{false};
    bool can_retry{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool exposes_native_handles{false};
};

struct EditorAssetImportJobSnapshot {
    std::uint64_t generation{1};
    std::vector<EditorAssetImportJobRow> rows;
    std::vector<std::string> diagnostics;
    std::size_t active_count{0};
    std::size_t completed_count{0};
    std::size_t failed_count{0};
    std::size_t canceled_count{0};
    bool exposes_native_handles{false};
};

struct EditorAssetImportJobCommandRequest {
    EditorAssetImportJobCommandKind kind{EditorAssetImportJobCommandKind::cancel};
    std::string job_id;
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool user_confirmed{false};
};

struct EditorAssetImportJobCommandPlan {
    EditorAssetImportJobCommandKind kind{EditorAssetImportJobCommandKind::cancel};
    std::string command_id;
    std::string job_id;
    EditorAssetImportJobCommandStatus status{EditorAssetImportJobCommandStatus::blocked};
    std::vector<std::string> diagnostics;
    bool requires_user_confirmation{true};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool exposes_native_handles{false};
};

struct EditorAssetImportJobCommandResult {
    EditorAssetImportJobCommandPlan plan;
    EditorAssetImportJobSnapshot snapshot;
    bool applied{false};
};

[[nodiscard]] std::string_view editor_asset_import_job_state_label(EditorAssetImportJobState state) noexcept;
[[nodiscard]] std::string_view
editor_asset_import_job_command_kind_label(EditorAssetImportJobCommandKind kind) noexcept;
[[nodiscard]] bool editor_asset_import_job_state_is_active(EditorAssetImportJobState state) noexcept;
[[nodiscard]] bool editor_asset_import_job_state_can_retry(EditorAssetImportJobState state) noexcept;
[[nodiscard]] EditorAssetImportJobSnapshot
make_editor_asset_import_job_model(const EditorAssetImportJobSnapshot& snapshot);
[[nodiscard]] EditorAssetImportJobCommandPlan
plan_editor_asset_import_job_command(const EditorAssetImportJobSnapshot& snapshot,
                                     const EditorAssetImportJobCommandRequest& request);
[[nodiscard]] EditorAssetImportJobCommandResult
apply_editor_asset_import_job_command(const EditorAssetImportJobSnapshot& snapshot,
                                      const EditorAssetImportJobCommandRequest& request);

} // namespace mirakana::editor
