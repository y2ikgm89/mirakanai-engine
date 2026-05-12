// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/shader_tool_process.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_optional_token(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool is_absolute_or_parent_relative(std::string_view path) noexcept {
    if (path.empty()) {
        return true;
    }
    if (path.size() >= 2U && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':') {
        return true;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto separator = path.find_first_of("/\\", start);
        const auto part =
            path.substr(start, separator == std::string_view::npos ? path.size() - start : separator - start);
        if (part == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return false;
}

[[nodiscard]] std::string normalize_asset_path(std::string_view path) {
    std::string normalized(path);
    for (auto& character : normalized) {
        if (character == '\\') {
            character = '/';
        }
    }
    while (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}

[[nodiscard]] bool is_path_under_root(std::string_view path, std::string_view root) {
    if (root.empty()) {
        return true;
    }
    const auto normalized_path = normalize_asset_path(path);
    const auto normalized_root = normalize_asset_path(root);
    if (normalized_root.empty()) {
        return true;
    }
    return normalized_path == normalized_root ||
           (normalized_path.size() > normalized_root.size() && normalized_path[normalized_root.size()] == '/' &&
            normalized_path.substr(0, normalized_root.size()) == normalized_root);
}

} // namespace

bool is_shader_artifact_allowed_by_policy(const ShaderGeneratedArtifact& artifact,
                                          const ShaderToolExecutionPolicy& policy) noexcept {
    return is_valid_shader_generated_artifact(artifact) && valid_optional_token(policy.artifact_output_root) &&
           valid_optional_token(policy.working_directory) && !is_absolute_or_parent_relative(artifact.path) &&
           (policy.artifact_output_root.empty() || !is_absolute_or_parent_relative(policy.artifact_output_root)) &&
           is_path_under_root(artifact.path, policy.artifact_output_root);
}

ProcessCommand make_shader_tool_process_command(const ShaderCompileCommand& command,
                                                const ShaderToolExecutionPolicy& policy) {
    if (!is_safe_shader_tool_command(command)) {
        throw std::invalid_argument("shader tool command is unsafe");
    }
    if (!is_shader_artifact_allowed_by_policy(command.artifact, policy)) {
        throw std::invalid_argument("shader artifact path is outside the execution policy");
    }

    ProcessCommand process_command{
        .executable = policy.executable_override.empty() ? command.executable : policy.executable_override,
        .arguments = command.arguments,
        .working_directory = policy.working_directory,
    };
    if (!is_safe_process_command(process_command)) {
        throw std::invalid_argument("shader tool process command is unsafe");
    }
    return process_command;
}

ProcessCommand make_shader_artifact_validation_process_command(const ShaderArtifactValidationCommand& command,
                                                               const ShaderToolExecutionPolicy& policy) {
    if (!is_safe_shader_artifact_validation_command(command)) {
        throw std::invalid_argument("shader artifact validation command is unsafe");
    }
    if (!is_shader_artifact_allowed_by_policy(command.artifact, policy)) {
        throw std::invalid_argument("shader artifact validation path is outside the execution policy");
    }

    ProcessCommand process_command{
        .executable = policy.executable_override.empty() ? command.executable : policy.executable_override,
        .arguments = command.arguments,
        .working_directory = policy.working_directory,
    };
    if (!is_safe_process_command(process_command)) {
        throw std::invalid_argument("shader artifact validation process command is unsafe");
    }
    return process_command;
}

ShaderToolProcessRunner::ShaderToolProcessRunner(IProcessRunner& process_runner, ShaderToolExecutionPolicy policy)
    : process_runner_(process_runner), policy_(std::move(policy)) {}

ShaderToolRunResult ShaderToolProcessRunner::run(const ShaderCompileCommand& command) {
    const auto process_command = make_shader_tool_process_command(command, policy_);
    const auto process_result = run_process_command(process_runner_, process_command);
    return ShaderToolRunResult{
        .exit_code = process_result.exit_code,
        .diagnostic = process_result.diagnostic,
        .stdout_text = process_result.stdout_text,
        .stderr_text = process_result.stderr_text,
        .artifact = command.artifact,
    };
}

ShaderArtifactValidationProcessRunner::ShaderArtifactValidationProcessRunner(IProcessRunner& process_runner,
                                                                             ShaderToolExecutionPolicy policy)
    : process_runner_(process_runner), policy_(std::move(policy)) {}

ShaderArtifactValidationResult
ShaderArtifactValidationProcessRunner::run(const ShaderArtifactValidationCommand& command) {
    const auto process_command = make_shader_artifact_validation_process_command(command, policy_);
    const auto process_result = run_process_command(process_runner_, process_command);
    return ShaderArtifactValidationResult{
        .exit_code = process_result.exit_code,
        .diagnostic = process_result.diagnostic,
        .stdout_text = process_result.stdout_text,
        .stderr_text = process_result.stderr_text,
        .artifact = command.artifact,
    };
}

} // namespace mirakana
