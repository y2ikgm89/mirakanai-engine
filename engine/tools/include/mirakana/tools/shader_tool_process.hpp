// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/process.hpp"
#include "mirakana/tools/shader_toolchain.hpp"

#include <string>

namespace mirakana {

struct ShaderToolExecutionPolicy {
    std::string artifact_output_root;
    std::string working_directory;
    std::string executable_override;
};

[[nodiscard]] bool is_shader_artifact_allowed_by_policy(const ShaderGeneratedArtifact& artifact,
                                                        const ShaderToolExecutionPolicy& policy) noexcept;

[[nodiscard]] ProcessCommand make_shader_tool_process_command(const ShaderCompileCommand& command,
                                                              const ShaderToolExecutionPolicy& policy);
[[nodiscard]] ProcessCommand
make_shader_artifact_validation_process_command(const ShaderArtifactValidationCommand& command,
                                                const ShaderToolExecutionPolicy& policy);

class ShaderToolProcessRunner final : public IShaderToolRunner {
  public:
    ShaderToolProcessRunner(IProcessRunner& process_runner, ShaderToolExecutionPolicy policy);

    [[nodiscard]] ShaderToolRunResult run(const ShaderCompileCommand& command) override;

  private:
    IProcessRunner& process_runner_;
    ShaderToolExecutionPolicy policy_;
};

class ShaderArtifactValidationProcessRunner final : public IShaderArtifactValidatorRunner {
  public:
    ShaderArtifactValidationProcessRunner(IProcessRunner& process_runner, ShaderToolExecutionPolicy policy);

    [[nodiscard]] ShaderArtifactValidationResult run(const ShaderArtifactValidationCommand& command) override;

  private:
    IProcessRunner& process_runner_;
    ShaderToolExecutionPolicy policy_;
};

} // namespace mirakana
