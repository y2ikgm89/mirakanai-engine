// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/process.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_optional_token(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] char lower_ascii(char value) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] std::string executable_leaf(std::string_view executable) {
    const auto separator = executable.find_last_of("/\\");
    auto leaf = executable.substr(separator == std::string_view::npos ? 0U : separator + 1U);
    std::string result(leaf);
    std::ranges::transform(result, result.begin(), lower_ascii);
    return result;
}

[[nodiscard]] bool is_shell_executable(std::string_view executable) {
    const auto leaf = executable_leaf(executable);
    return leaf == "cmd" || leaf == "cmd.exe" || leaf == "powershell" || leaf == "powershell.exe" || leaf == "pwsh" ||
           leaf == "pwsh.exe" || leaf == "bash" || leaf == "bash.exe" || leaf == "sh" || leaf == "sh.exe" ||
           leaf == "wscript" || leaf == "wscript.exe" || leaf == "cscript" || leaf == "cscript.exe";
}

} // namespace

ProcessResult RecordingProcessRunner::run(const ProcessCommand& command) {
    commands_.push_back(command);
    return ProcessResult{
        .launched = true,
        .exit_code = 0,
        .diagnostic = {},
        .stdout_text = {},
        .stderr_text = {},
    };
}

const std::vector<ProcessCommand>& RecordingProcessRunner::commands() const noexcept {
    return commands_;
}

bool is_safe_process_command(const ProcessCommand& command) noexcept {
    if (!valid_token(command.executable) || is_shell_executable(command.executable)) {
        return false;
    }
    if (!command.working_directory.empty() && !valid_optional_token(command.working_directory)) {
        return false;
    }
    return std::ranges::all_of(command.arguments, [](const auto& argument) { return valid_optional_token(argument); });
}

bool is_safe_reviewed_validation_recipe_invocation(const ProcessCommand& command) noexcept {
    const auto leaf = executable_leaf(command.executable);
    if (leaf != "pwsh" && leaf != "pwsh.exe") {
        return false;
    }
    if (!valid_token(command.executable)) {
        return false;
    }
    if (!command.working_directory.empty() && !valid_optional_token(command.working_directory)) {
        return false;
    }
    if (command.arguments.size() < 9U) {
        return false;
    }
    const auto& args = command.arguments;
    if (args[0] != "-NoProfile" || args[1] != "-ExecutionPolicy" || args[2] != "Bypass" || args[3] != "-File" ||
        args[4] != "tools/run-validation-recipe.ps1") {
        return false;
    }
    return std::ranges::all_of(command.arguments, [](const auto& argument) { return valid_optional_token(argument); });
}

bool is_safe_reviewed_pix_host_helper_invocation(const ProcessCommand& command) noexcept {
    const auto leaf = executable_leaf(command.executable);
    if (leaf != "pwsh" && leaf != "pwsh.exe") {
        return false;
    }
    if (!valid_token(command.executable)) {
        return false;
    }
    if (command.working_directory.empty() || !valid_optional_token(command.working_directory)) {
        return false;
    }
    const auto& args = command.arguments;
    if (args.size() != 5U && args.size() != 6U) {
        return false;
    }
    if (args[0] != "-NoProfile" || args[1] != "-ExecutionPolicy" || args[2] != "Bypass" || args[3] != "-File" ||
        args[4] != "tools/launch-pix-host-helper.ps1") {
        return false;
    }
    if (args.size() == 6U && args[5] != "-SkipLaunch") {
        return false;
    }
    return std::ranges::all_of(command.arguments, [](const auto& argument) { return valid_optional_token(argument); });
}

bool is_allowed_process_command(const ProcessCommand& command) noexcept {
    return is_safe_process_command(command) || is_safe_reviewed_validation_recipe_invocation(command) ||
           is_safe_reviewed_pix_host_helper_invocation(command);
}

ProcessResult run_process_command(IProcessRunner& runner, const ProcessCommand& command) {
    if (!is_allowed_process_command(command)) {
        throw std::invalid_argument("process command is unsafe");
    }
    return runner.run(command);
}

} // namespace mirakana
