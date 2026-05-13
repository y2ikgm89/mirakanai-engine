// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#if defined(__linux__)
#include "mirakana/platform/linux_file_watcher.hpp"
#elif defined(__APPLE__)
#include "mirakana/platform/macos_file_watcher.hpp"
#else
#error Native file watcher tests are only supported on Linux and macOS.
#endif

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

[[nodiscard]] std::string describe_events(const std::vector<mirakana::FileWatchEvent>& events) {
    std::ostringstream output;
    output << "events=" << events.size();
    for (const auto& event : events) {
        output << " [" << static_cast<int>(event.kind) << ':' << event.path << ']';
    }
    return output.str();
}

#if defined(__linux__)
[[nodiscard]] std::vector<mirakana::FileWatchEvent> wait_for_native_file_event(mirakana::LinuxFileWatcher& watcher,
                                                                               const std::string& path) {
#elif defined(__APPLE__)
[[nodiscard]] std::vector<mirakana::FileWatchEvent> wait_for_native_file_event(mirakana::MacOSFileWatcher& watcher,
                                                                               const std::string& path) {
#endif
    std::vector<mirakana::FileWatchEvent> collected;
    for (int attempt = 0; attempt < 300; ++attempt) {
        auto result = watcher.poll();
        if (!result.active || !result.diagnostic.empty()) {
            throw std::runtime_error("watcher inactive: " + result.diagnostic);
        }
        collected.insert(collected.end(), result.events.begin(), result.events.end());
        if (std::ranges::any_of(collected, [&](const mirakana::FileWatchEvent& event) { return event.path == path; })) {
            return collected;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return collected;
}

} // namespace

MK_TEST("native file watcher reports file changes without exposing handles") {
    const auto availability = mirakana::default_file_watch_backend_availability();
    MK_REQUIRE(availability.polling);
    MK_REQUIRE(availability.native);
#if defined(__linux__)
    MK_REQUIRE(availability.native_backend == mirakana::FileWatchNativeBackendKind::linux_inotify);
#elif defined(__APPLE__)
    MK_REQUIRE(availability.native_backend == mirakana::FileWatchNativeBackendKind::macos_fsevents);
#endif

    const auto default_choice = mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::automatic);
    MK_REQUIRE(default_choice.available);
    MK_REQUIRE(default_choice.selected == mirakana::FileWatchBackendKind::native);

    const auto root = std::filesystem::temp_directory_path() / "ge-native-file-watcher-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    {
#if defined(__linux__)
        mirakana::LinuxFileWatcher watcher(mirakana::LinuxFileWatcherDesc{
            root,
            "assets",
            true,
        });
        MK_REQUIRE(watcher.native_backend_kind() == mirakana::FileWatchNativeBackendKind::linux_inotify);
#elif defined(__APPLE__)
        mirakana::MacOSFileWatcher watcher(mirakana::MacOSFileWatcherDesc{
            root,
            "assets",
            true,
        });
        MK_REQUIRE(watcher.native_backend_kind() == mirakana::FileWatchNativeBackendKind::macos_fsevents);
#endif
        MK_REQUIRE(watcher.active());
        MK_REQUIRE(watcher.backend_kind() == mirakana::FileWatchBackendKind::native);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        {
            std::ofstream output(root / "player.texture", std::ios::binary | std::ios::trunc);
            output << "albedo=blue\n";
        }

        const auto events = wait_for_native_file_event(watcher, "assets/player.texture");
        const auto found = std::ranges::find_if(events, [](const mirakana::FileWatchEvent& event) {
            return event.path == "assets/player.texture" && (event.kind == mirakana::FileWatchEventKind::added ||
                                                             event.kind == mirakana::FileWatchEventKind::modified);
        });
        if (found == events.end()) {
            throw std::runtime_error(describe_events(events));
        }
    }

    std::filesystem::remove_all(root);
}

int main() {
    return mirakana::test::run_all();
}
