// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/dynamic_library.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/platform/win32_process.hpp"
#include "mirakana/platform/windows_file_watcher.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef MK_PROCESS_PROBE_PATH
#error MK_PROCESS_PROBE_PATH must point at the test process probe executable
#endif

#ifndef MK_DYNAMIC_LIBRARY_PROBE_PATH
#error MK_DYNAMIC_LIBRARY_PROBE_PATH must point at the test dynamic library probe
#endif

namespace {

[[nodiscard]] std::vector<mirakana::FileWatchEvent> wait_for_native_file_event(mirakana::WindowsFileWatcher& watcher,
                                                                               const std::string& path) {
    std::vector<mirakana::FileWatchEvent> collected;
    for (int attempt = 0; attempt < 100; ++attempt) {
        auto result = watcher.poll();
        if (!result.active || !result.diagnostic.empty()) {
            throw std::runtime_error("watcher inactive: " + result.diagnostic);
        }
        collected.insert(collected.end(), result.events.begin(), result.events.end());
        if (std::ranges::any_of(collected, [&](const mirakana::FileWatchEvent& event) { return event.path == path; })) {
            return collected;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return collected;
}

[[nodiscard]] std::string describe_events(const std::vector<mirakana::FileWatchEvent>& events) {
    std::ostringstream output;
    output << "events=" << events.size();
    for (const auto& event : events) {
        output << " [" << static_cast<int>(event.kind) << ':' << event.path << ']';
    }
    return output.str();
}

} // namespace

MK_TEST("win32 process runner captures stdout stderr and exit code") {
    mirakana::Win32ProcessRunner runner;
    const mirakana::ProcessCommand command{
        .executable = MK_PROCESS_PROBE_PATH,
        .arguments = {"--stdout", "hello from stdout", "--stderr", "warning from stderr", "--exit", "7"},
        .working_directory = {},
    };

    const auto result = mirakana::run_process_command(runner, command);

    MK_REQUIRE(result.launched);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.exit_code == 7);
    MK_REQUIRE(result.stdout_text == "hello from stdout\r\n");
    MK_REQUIRE(result.stderr_text == "warning from stderr\r\n");
    MK_REQUIRE(result.diagnostic.empty());
}

MK_TEST("win32 process runner reports launch failures") {
    mirakana::Win32ProcessRunner runner;
    const auto result = runner.run(mirakana::ProcessCommand{
        .executable = "Z:/definitely/not/a/real/tool.exe",
        .arguments = {},
        .working_directory = {},
    });

    MK_REQUIRE(!result.launched);
    MK_REQUIRE(result.exit_code != 0);
    MK_REQUIRE(!result.diagnostic.empty());
}

MK_TEST("windows file watcher reports native file changes without exposing handles") {
    const auto availability = mirakana::default_file_watch_backend_availability();
    MK_REQUIRE(availability.polling);
    MK_REQUIRE(availability.native);
    MK_REQUIRE(availability.native_backend == mirakana::FileWatchNativeBackendKind::windows_read_directory_changes);
    const auto default_choice = mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::automatic);
    MK_REQUIRE(default_choice.available);
    MK_REQUIRE(default_choice.selected == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(default_choice.native_backend == mirakana::FileWatchNativeBackendKind::windows_read_directory_changes);

    const auto root = std::filesystem::current_path() / "ge-windows-file-watcher-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    {
        mirakana::WindowsFileWatcher watcher(mirakana::WindowsFileWatcherDesc{
            .directory = root,
            .path_prefix = "assets",
            .recursive = true,
        });
        MK_REQUIRE(watcher.active());
        MK_REQUIRE(watcher.backend_kind() == mirakana::FileWatchBackendKind::native);
        MK_REQUIRE(watcher.native_backend_kind() ==
                   mirakana::FileWatchNativeBackendKind::windows_read_directory_changes);

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

MK_TEST("dynamic library loads absolute module and resolves symbol") {
    auto result = mirakana::load_dynamic_library(std::filesystem::path{MK_DYNAMIC_LIBRARY_PROBE_PATH});

    MK_REQUIRE(result.status == mirakana::DynamicLibraryLoadStatus::loaded);
    MK_REQUIRE(result.library.loaded());
    MK_REQUIRE(result.diagnostic.empty());

    auto symbol = mirakana::resolve_dynamic_library_symbol(result.library, "MK_dynamic_library_probe_add");

    MK_REQUIRE(symbol.status == mirakana::DynamicLibrarySymbolStatus::resolved);
    MK_REQUIRE(symbol.address != nullptr);

    using AddFn = int (*)(int, int);
    const auto add = reinterpret_cast<AddFn>(symbol.address);
    MK_REQUIRE(add(2, 5) == 7);
}

MK_TEST("dynamic library rejects relative module paths") {
    auto result = mirakana::load_dynamic_library(std::filesystem::path{"relative_probe.dll"});

    MK_REQUIRE(result.status == mirakana::DynamicLibraryLoadStatus::blocked);
    MK_REQUIRE(!result.library.loaded());
    MK_REQUIRE(result.diagnostic.find("absolute") != std::string::npos);
}

MK_TEST("dynamic library reports missing absolute modules") {
    auto result = mirakana::load_dynamic_library(std::filesystem::current_path() / "definitely_missing_probe.dll");

    MK_REQUIRE(result.status == mirakana::DynamicLibraryLoadStatus::failed);
    MK_REQUIRE(!result.library.loaded());
    MK_REQUIRE(!result.diagnostic.empty());
}

MK_TEST("dynamic library rejects unsafe symbol names") {
    auto result = mirakana::load_dynamic_library(std::filesystem::path{MK_DYNAMIC_LIBRARY_PROBE_PATH});
    MK_REQUIRE(result.status == mirakana::DynamicLibraryLoadStatus::loaded);

    auto symbol = mirakana::resolve_dynamic_library_symbol(result.library, "bad/symbol");

    MK_REQUIRE(symbol.status == mirakana::DynamicLibrarySymbolStatus::blocked);
    MK_REQUIRE(symbol.address == nullptr);
    MK_REQUIRE(!symbol.diagnostic.empty());
}

MK_TEST("dynamic library reports missing symbols") {
    auto result = mirakana::load_dynamic_library(std::filesystem::path{MK_DYNAMIC_LIBRARY_PROBE_PATH});
    MK_REQUIRE(result.status == mirakana::DynamicLibraryLoadStatus::loaded);

    auto symbol = mirakana::resolve_dynamic_library_symbol(result.library, "MK_dynamic_library_probe_missing");

    MK_REQUIRE(symbol.status == mirakana::DynamicLibrarySymbolStatus::missing);
    MK_REQUIRE(symbol.address == nullptr);
    MK_REQUIRE(!symbol.diagnostic.empty());
}

int main() {
    return mirakana::test::run_all();
}
