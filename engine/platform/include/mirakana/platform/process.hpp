// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct ProcessCommand {
    std::string executable;
    std::vector<std::string> arguments;
    std::string working_directory;
};

struct ProcessResult {
    bool launched{false};
    int exit_code{-1};
    std::string diagnostic;
    std::string stdout_text;
    std::string stderr_text;

    [[nodiscard]] bool succeeded() const noexcept {
        return launched && exit_code == 0;
    }
};

class IProcessRunner {
  public:
    virtual ~IProcessRunner() = default;
    [[nodiscard]] virtual ProcessResult run(const ProcessCommand& command) = 0;
};

class RecordingProcessRunner final : public IProcessRunner {
  public:
    [[nodiscard]] ProcessResult run(const ProcessCommand& command) override;
    [[nodiscard]] const std::vector<ProcessCommand>& commands() const noexcept;

  private:
    std::vector<ProcessCommand> commands_;
};

[[nodiscard]] bool is_safe_process_command(const ProcessCommand& command) noexcept;
/// True when `command` is a reviewed `pwsh` invocation of `tools/run-validation-recipe.ps1` with the canonical
/// `-NoProfile -ExecutionPolicy Bypass -File` prefix (editor handoff / operator execution contract).
[[nodiscard]] bool is_safe_reviewed_validation_recipe_invocation(const ProcessCommand& command) noexcept;
/// True when `command` is a reviewed `pwsh` invocation of `tools/launch-pix-host-helper.ps1` with the canonical
/// `-NoProfile -ExecutionPolicy Bypass -File` prefix and optional `-SkipLaunch` only (Windows host-helper contract).
[[nodiscard]] bool is_safe_reviewed_pix_host_helper_invocation(const ProcessCommand& command) noexcept;
/// True for direct non-shell executables and for reviewed allowlisted `pwsh` editor handoffs.
[[nodiscard]] bool is_allowed_process_command(const ProcessCommand& command) noexcept;
[[nodiscard]] ProcessResult run_process_command(IProcessRunner& runner, const ProcessCommand& command);

} // namespace mirakana
