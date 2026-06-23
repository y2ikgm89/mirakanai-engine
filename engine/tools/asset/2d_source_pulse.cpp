// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/2d_source_pulse.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool contains_control_character(std::string_view path) noexcept {
    return std::ranges::any_of(path, [](const char value) {
        const auto byte = static_cast<unsigned char>(value);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool is_absolute_path(std::string_view path) noexcept {
    if (path.empty()) {
        return false;
    }
    if (path.front() == '/' || path.front() == '\\') {
        return true;
    }
    return path.size() >= 2U && path[1] == ':';
}

[[nodiscard]] bool has_parent_segment(std::string_view path) {
    std::string normalized(path);
    std::ranges::replace(normalized, '\\', '/');
    return normalized == ".." || normalized.starts_with("../") || normalized.ends_with("/..") ||
           normalized.find("/../") != std::string::npos;
}

[[nodiscard]] AssetHotReloadEventKind map_file_watch_event_kind(FileWatchEventKind kind) noexcept {
    switch (kind) {
    case FileWatchEventKind::added:
        return AssetHotReloadEventKind::added;
    case FileWatchEventKind::modified:
        return AssetHotReloadEventKind::modified;
    case FileWatchEventKind::removed:
        return AssetHotReloadEventKind::removed;
    case FileWatchEventKind::unknown:
        break;
    }
    return AssetHotReloadEventKind::unknown;
}

[[nodiscard]] bool valid_revision_transition(const FileWatchEvent& event) noexcept {
    switch (event.kind) {
    case FileWatchEventKind::added:
        return event.previous_revision == 0 && event.current_revision != 0;
    case FileWatchEventKind::modified:
        return event.previous_revision != 0 && event.current_revision != 0;
    case FileWatchEventKind::removed:
        return event.previous_revision != 0 && event.current_revision == 0;
    case FileWatchEventKind::unknown:
        break;
    }
    return false;
}

void validate_path(std::vector<std::string>& diagnostics, std::string_view path) {
    if (path.empty()) {
        diagnostics.push_back("empty_path");
        return;
    }
    if (is_absolute_path(path)) {
        diagnostics.push_back("absolute_path");
    }
    if (has_parent_segment(path)) {
        diagnostics.push_back("parent_segment_path");
    }
    if (contains_control_character(path)) {
        diagnostics.push_back("control_character_path");
    }
}

void validate_forbidden_requests(TwoDSourcePulsePlan& plan, const TwoDSourcePulseDesc& desc) {
    if (desc.native_handle_exposed) {
        plan.diagnostics.push_back("native_handle_exposed");
    }
    if (desc.request_autonomous_background_commit) {
        plan.diagnostics.push_back("autonomous_background_commit_requested");
    }
    if (desc.request_package_script_execution) {
        plan.diagnostics.push_back("package_script_execution_requested");
    }
    if (desc.request_renderer_rhi_handles) {
        plan.diagnostics.push_back("renderer_rhi_handles_requested");
    }
}

} // namespace

TwoDSourcePulsePlan plan_2d_source_pulse_events(const TwoDSourcePulseDesc& desc) {
    TwoDSourcePulsePlan plan;
    plan.native_file_watch_invoked = desc.native_file_watch_invoked && !desc.file_watch_events.empty();
    plan.native_handle_exposed = desc.native_handle_exposed;
    validate_forbidden_requests(plan, desc);

    std::vector<TwoDSourcePulseEventRow> accepted_rows;
    accepted_rows.reserve(desc.file_watch_events.size());

    for (const auto& event : desc.file_watch_events) {
        const auto hot_reload_kind = map_file_watch_event_kind(event.kind);
        const auto previous_diagnostic_count = plan.diagnostics.size();

        if (hot_reload_kind == AssetHotReloadEventKind::unknown) {
            plan.diagnostics.push_back("unknown_file_watch_event_kind");
        }
        validate_path(plan.diagnostics, event.path);
        if (hot_reload_kind != AssetHotReloadEventKind::unknown && !valid_revision_transition(event)) {
            plan.diagnostics.push_back("invalid_revision_transition");
        }

        if (plan.diagnostics.size() != previous_diagnostic_count) {
            continue;
        }

        accepted_rows.push_back(TwoDSourcePulseEventRow{
            .asset = AssetId::from_name(event.path),
            .path = event.path,
            .watch_event_kind = event.kind,
            .hot_reload_event_kind = hot_reload_kind,
            .previous_revision = event.previous_revision,
            .current_revision = event.current_revision,
        });
    }

    if (!plan.diagnostics.empty()) {
        plan.status = TwoDSourcePulseStatus::blocked;
        return plan;
    }

    plan.event_rows = std::move(accepted_rows);
    plan.status =
        plan.event_rows.empty() ? TwoDSourcePulseStatus::no_ready_changes : TwoDSourcePulseStatus::recook_pending;
    return plan;
}

} // namespace mirakana
