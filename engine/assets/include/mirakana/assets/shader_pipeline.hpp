// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/shader_metadata.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class ShaderCompileTarget { unknown, d3d12_dxil, vulkan_spirv, metal_ir, metal_library };

struct ShaderCompileRequest {
    ShaderSourceMetadata source;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string output_path;
    std::string profile;
    std::vector<std::string> include_paths;
    bool debug_symbols{true};
    bool optimize{true};
};

struct ShaderCompileCommand {
    std::string executable;
    std::vector<std::string> arguments;
    ShaderGeneratedArtifact artifact;
};

enum class ShaderIncludeKind { quoted, system };

struct ShaderSourceDependency {
    std::string path;
    ShaderIncludeKind kind{ShaderIncludeKind::quoted};
};

struct ShaderToolRunResult {
    int exit_code{0};
    std::string diagnostic;
    std::string stdout_text;
    std::string stderr_text;
    ShaderGeneratedArtifact artifact;

    [[nodiscard]] bool succeeded() const noexcept {
        return exit_code == 0;
    }
};

class IShaderToolRunner {
  public:
    virtual ~IShaderToolRunner() = default;
    [[nodiscard]] virtual ShaderToolRunResult run(const ShaderCompileCommand& command) = 0;
};

class RecordingShaderToolRunner final : public IShaderToolRunner {
  public:
    [[nodiscard]] ShaderToolRunResult run(const ShaderCompileCommand& command) override;
    [[nodiscard]] const std::vector<ShaderCompileCommand>& commands() const noexcept;

  private:
    std::vector<ShaderCompileCommand> commands_;
};

[[nodiscard]] bool is_valid_shader_compile_request(const ShaderCompileRequest& request) noexcept;
[[nodiscard]] ShaderCompileCommand make_shader_compile_command(const ShaderCompileRequest& request);
[[nodiscard]] bool is_safe_shader_tool_command(const ShaderCompileCommand& command) noexcept;
[[nodiscard]] ShaderToolRunResult run_shader_tool_command(IShaderToolRunner& runner,
                                                          const ShaderCompileCommand& command);
[[nodiscard]] std::vector<ShaderSourceDependency> discover_shader_source_dependencies(std::string_view source_text);
[[nodiscard]] std::vector<AssetDependencyEdge> make_shader_include_dependency_edges(const ShaderSourceMetadata& source,
                                                                                    std::string_view source_text);

[[nodiscard]] std::string serialize_shader_artifact_manifest(const std::vector<ShaderSourceMetadata>& shaders);
[[nodiscard]] std::vector<ShaderSourceMetadata> deserialize_shader_artifact_manifest(std::string_view text);

} // namespace mirakana
