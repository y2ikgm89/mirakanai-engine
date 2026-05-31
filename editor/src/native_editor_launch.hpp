// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct NativeEditorLaunchOptions {
    std::uint32_t width{1280};
    std::uint32_t height{720};
    std::int32_t smoke_frames{-1};
    bool smoke_resize{false};
    bool no_user_config{false};
};

struct NativeEditorLaunchValidation {
    bool valid{false};
    std::string diagnostic;
};

struct NativeEditorUserConfigPolicy {
    bool ini_file_enabled{true};
    bool log_file_enabled{true};
};

struct NativeEditorLaunchParseResult {
    NativeEditorLaunchOptions options;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] NativeEditorLaunchParseResult parse_native_editor_launch(int argc, char** argv);

[[nodiscard]] NativeEditorLaunchValidation
validate_native_editor_launch_options(const NativeEditorLaunchOptions& options);

[[nodiscard]] NativeEditorLaunchValidation validate_native_editor_launch(const NativeEditorLaunchParseResult& launch);

[[nodiscard]] NativeEditorUserConfigPolicy
make_native_editor_user_config_policy(const NativeEditorLaunchOptions& options) noexcept;

[[nodiscard]] std::string native_editor_launch_usage(std::string_view executable_name);

[[nodiscard]] constexpr int native_editor_launch_usage_error_exit_code() noexcept {
    return 2;
}

} // namespace mirakana::editor
