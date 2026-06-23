// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/2d_source_pulse.hpp"

#include <algorithm>
#include <string_view>

namespace {

[[nodiscard]] bool has_diagnostic(const std::vector<std::string>& diagnostics, std::string_view code) {
    return std::ranges::find(diagnostics, code) != diagnostics.end();
}

[[nodiscard]] mirakana::FileWatchEvent make_file_watch_event(mirakana::FileWatchEventKind kind, std::string_view path,
                                                             std::uint64_t previous_revision,
                                                             std::uint64_t current_revision) {
    return mirakana::FileWatchEvent{
        .kind = kind,
        .path = std::string(path),
        .previous_revision = previous_revision,
        .current_revision = current_revision,
        .previous_size_bytes = previous_revision == 0 ? 0ULL : 32ULL,
        .current_size_bytes = current_revision == 0 ? 0ULL : 48ULL,
    };
}

} // namespace

MK_TEST("2d source pulse maps native or polling file events to source pulse rows") {
    const auto added = make_file_watch_event(mirakana::FileWatchEventKind::added, "source/textures/player.png", 0, 1);
    const auto modified =
        make_file_watch_event(mirakana::FileWatchEventKind::modified, "source/tilemaps/level_01.tilemap", 2, 3);
    const auto removed =
        make_file_watch_event(mirakana::FileWatchEventKind::removed, "source/audio/old_theme.wav", 4, 0);

    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events = {added, modified, removed},
        .native_file_watch_invoked = true,
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::recook_pending);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.safe_point_required);
    MK_REQUIRE(plan.native_file_watch_invoked);
    MK_REQUIRE(!plan.native_handle_exposed);
    MK_REQUIRE(plan.event_rows.size() == 3);
    MK_REQUIRE(plan.event_rows[0].asset == mirakana::AssetId::from_name("source/textures/player.png"));
    MK_REQUIRE(plan.event_rows[0].watch_event_kind == mirakana::FileWatchEventKind::added);
    MK_REQUIRE(plan.event_rows[0].hot_reload_event_kind == mirakana::AssetHotReloadEventKind::added);
    MK_REQUIRE(plan.event_rows[0].previous_revision == 0);
    MK_REQUIRE(plan.event_rows[0].current_revision == 1);
    MK_REQUIRE(plan.event_rows[1].asset == mirakana::AssetId::from_name("source/tilemaps/level_01.tilemap"));
    MK_REQUIRE(plan.event_rows[1].hot_reload_event_kind == mirakana::AssetHotReloadEventKind::modified);
    MK_REQUIRE(plan.event_rows[2].asset == mirakana::AssetId::from_name("source/audio/old_theme.wav"));
    MK_REQUIRE(plan.event_rows[2].hot_reload_event_kind == mirakana::AssetHotReloadEventKind::removed);
}

MK_TEST("2d source pulse rejects invalid paths and unknown event kinds") {
    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events =
            {
                make_file_watch_event(mirakana::FileWatchEventKind::unknown, "source/textures/player.png", 1, 2),
                make_file_watch_event(mirakana::FileWatchEventKind::modified, "", 1, 2),
                make_file_watch_event(mirakana::FileWatchEventKind::modified, "C:/external/file.png", 1, 2),
                make_file_watch_event(mirakana::FileWatchEventKind::modified, "../external/file.png", 1, 2),
                make_file_watch_event(mirakana::FileWatchEventKind::modified, "source/bad\nfile.png", 1, 2),
            },
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::blocked);
    MK_REQUIRE(plan.event_rows.empty());
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "unknown_file_watch_event_kind"));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "empty_path"));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "absolute_path"));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "parent_segment_path"));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "control_character_path"));
}

MK_TEST("2d source pulse rejects native handle exposure") {
    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events = {make_file_watch_event(mirakana::FileWatchEventKind::modified,
                                                    "source/textures/player.png", 1, 2)},
        .native_handle_exposed = true,
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::blocked);
    MK_REQUIRE(plan.native_handle_exposed);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "native_handle_exposed"));
}

MK_TEST("2d source pulse rejects autonomous background commit") {
    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events = {make_file_watch_event(mirakana::FileWatchEventKind::modified,
                                                    "source/textures/player.png", 1, 2)},
        .request_autonomous_background_commit = true,
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::blocked);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "autonomous_background_commit_requested"));
}

MK_TEST("2d source pulse rejects package script and renderer rhi requests") {
    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events = {make_file_watch_event(mirakana::FileWatchEventKind::modified,
                                                    "source/textures/player.png", 1, 2)},
        .request_package_script_execution = true,
        .request_renderer_rhi_handles = true,
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::blocked);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "package_script_execution_requested"));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, "renderer_rhi_handles_requested"));
}

MK_TEST("2d source pulse does not construct or own platform watchers") {
    const auto plan = mirakana::plan_2d_source_pulse_events(mirakana::TwoDSourcePulseDesc{
        .file_watch_events = {make_file_watch_event(mirakana::FileWatchEventKind::modified,
                                                    "source/textures/player.png", 1, 2)},
        .native_file_watch_invoked = false,
    });

    MK_REQUIRE(plan.status == mirakana::TwoDSourcePulseStatus::recook_pending);
    MK_REQUIRE(!plan.native_file_watch_invoked);
    MK_REQUIRE(!plan.native_handle_exposed);
    MK_REQUIRE(plan.safe_point_required);
    MK_REQUIRE(plan.event_rows.size() == 1);
}

int main() {
    return mirakana::test::run_all();
}
