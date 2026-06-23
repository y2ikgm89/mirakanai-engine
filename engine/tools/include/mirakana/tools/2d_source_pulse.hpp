// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/platform/file_watcher.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class TwoDSourcePulseStatus : std::uint8_t {
    blocked,
    primed,
    no_ready_changes,
    recook_pending,
    committed,
    failed,
};

struct TwoDSourcePulseEventRow {
    AssetId asset;
    std::string path;
    FileWatchEventKind watch_event_kind{FileWatchEventKind::unknown};
    AssetHotReloadEventKind hot_reload_event_kind{AssetHotReloadEventKind::unknown};
    std::uint64_t previous_revision{0};
    std::uint64_t current_revision{0};
};

struct TwoDSourcePulseDesc {
    std::vector<FileWatchEvent> file_watch_events;
    bool native_file_watch_invoked{false};
    bool native_handle_exposed{false};
    bool request_autonomous_background_commit{false};
    bool request_package_script_execution{false};
    bool request_renderer_rhi_handles{false};
};

struct TwoDSourcePulsePlan {
    TwoDSourcePulseStatus status{TwoDSourcePulseStatus::blocked};
    std::vector<TwoDSourcePulseEventRow> event_rows;
    std::vector<std::string> diagnostics;
    bool native_file_watch_invoked{false};
    bool native_handle_exposed{false};
    bool safe_point_required{true};
};

[[nodiscard]] TwoDSourcePulsePlan plan_2d_source_pulse_events(const TwoDSourcePulseDesc& desc);

} // namespace mirakana
