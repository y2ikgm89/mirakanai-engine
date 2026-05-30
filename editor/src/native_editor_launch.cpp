// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_launch.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {
namespace {

template <typename T> [[nodiscard]] bool parse_integer(std::string_view text, T& value) {
    if (text.empty()) {
        return false;
    }

    T parsed{};
    const auto* const first = text.data();
    const auto* const last = text.data() + text.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last) {
        return false;
    }

    value = parsed;
    return true;
}

[[nodiscard]] std::string_view argv_token(int argc, char** argv, int index) noexcept {
    if (index < 0 || index >= argc || argv == nullptr || argv[index] == nullptr) {
        return {};
    }
    return std::string_view{argv[index]};
}

[[nodiscard]] bool token_requires_value(std::string_view token) noexcept {
    return token == "--width" || token == "--height" || token == "--smoke-frames";
}

[[nodiscard]] std::string missing_value_diagnostic(std::string_view option_name) {
    std::string diagnostic{"missing value for "};
    diagnostic.append(option_name);
    return diagnostic;
}

[[nodiscard]] std::string invalid_value_diagnostic(std::string_view option_name, std::string_view value) {
    std::string diagnostic{"invalid numeric value for "};
    diagnostic.append(option_name);
    diagnostic.append(": ");
    diagnostic.append(value);
    return diagnostic;
}

[[nodiscard]] std::string unknown_option_diagnostic(std::string_view token) {
    std::string diagnostic{"unknown option: "};
    diagnostic.append(token);
    return diagnostic;
}

} // namespace

NativeEditorLaunchParseResult parse_native_editor_launch(int argc, char** argv) {
    NativeEditorLaunchParseResult result;
    for (int index = 1; index < argc; ++index) {
        const auto token = argv_token(argc, argv, index);
        if (token == "--no-user-config") {
            result.options.no_user_config = true;
        } else if (token == "--smoke-resize") {
            result.options.smoke_resize = true;
        } else if (token_requires_value(token)) {
            if (index + 1 >= argc) {
                result.diagnostics.push_back(missing_value_diagnostic(token));
                continue;
            }

            ++index;
            const auto value = argv_token(argc, argv, index);
            if (token == "--width") {
                std::uint32_t parsed = result.options.width;
                if (parse_integer(value, parsed)) {
                    result.options.width = parsed;
                } else {
                    result.diagnostics.push_back(invalid_value_diagnostic(token, value));
                }
            } else if (token == "--height") {
                std::uint32_t parsed = result.options.height;
                if (parse_integer(value, parsed)) {
                    result.options.height = parsed;
                } else {
                    result.diagnostics.push_back(invalid_value_diagnostic(token, value));
                }
            } else if (token == "--smoke-frames") {
                std::int32_t parsed = result.options.smoke_frames;
                if (parse_integer(value, parsed)) {
                    result.options.smoke_frames = parsed;
                } else {
                    result.diagnostics.push_back(invalid_value_diagnostic(token, value));
                }
            }
        } else if (!token.empty()) {
            result.diagnostics.push_back(unknown_option_diagnostic(token));
        }
    }
    return result;
}

NativeEditorLaunchValidation validate_native_editor_launch_options(const NativeEditorLaunchOptions& options) {
    if (options.width == 0U || options.height == 0U) {
        return NativeEditorLaunchValidation{.valid = false,
                                            .diagnostic = "native editor window extent must be positive"};
    }
    if (options.smoke_frames != -1 && options.smoke_frames <= 0) {
        return NativeEditorLaunchValidation{
            .valid = false,
            .diagnostic = "native editor smoke frames must be -1 for interactive mode or a positive frame count"};
    }
    if (options.smoke_resize && options.smoke_frames < 2) {
        return NativeEditorLaunchValidation{.valid = false,
                                            .diagnostic = "native editor smoke resize requires at least two frames"};
    }
    return NativeEditorLaunchValidation{.valid = true, .diagnostic = {}};
}

NativeEditorLaunchValidation validate_native_editor_launch(const NativeEditorLaunchParseResult& launch) {
    if (!launch.diagnostics.empty()) {
        return NativeEditorLaunchValidation{.valid = false, .diagnostic = launch.diagnostics.front()};
    }
    return validate_native_editor_launch_options(launch.options);
}

std::string native_editor_launch_usage(std::string_view executable_name) {
    if (executable_name.empty()) {
        executable_name = "MK_editor";
    }
    std::string usage;
    usage.reserve(executable_name.size() + 128U);
    usage.append("usage: ");
    usage.append(executable_name);
    usage.append(
        " [--width <pixels>] [--height <pixels>] [--smoke-frames <count>] [--smoke-resize] [--no-user-config]");
    return usage;
}

} // namespace mirakana::editor
