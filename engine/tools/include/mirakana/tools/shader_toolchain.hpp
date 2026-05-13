// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class ShaderToolKind : std::uint8_t { unknown, dxc, spirv_val, metal, metallib };

struct ShaderToolDescriptor {
    ShaderToolKind kind{ShaderToolKind::unknown};
    std::string executable_path;
    std::string version{"unknown"};
    bool supports_spirv_codegen{false};
};

struct ShaderToolDiscoveryRequest {
    std::vector<std::string> search_roots;
    bool include_windows_exe_suffix{true};
};

struct ShaderToolchainReadiness {
    bool dxc_available{false};
    bool dxc_spirv_codegen_available{false};
    bool spirv_validator_available{false};
    bool metal_available{false};
    bool metallib_available{false};
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool ready_for_d3d12_dxil() const noexcept;
    [[nodiscard]] bool ready_for_vulkan_spirv() const noexcept;
    [[nodiscard]] bool ready_for_metal_library() const noexcept;
};

struct ShaderInputFingerprint {
    std::string path;
    std::string digest;
};

struct ShaderArtifactProvenance {
    ShaderGeneratedArtifact artifact;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string source_path;
    std::string profile;
    std::string entry_point;
    ShaderToolDescriptor tool;
    std::vector<ShaderInputFingerprint> inputs;
    std::string command_fingerprint;
};

struct ShaderArtifactCacheIndexEntry {
    std::string artifact_path;
    std::string provenance_path;
    std::string command_fingerprint;
};

struct ShaderArtifactCacheIndex {
    std::vector<ShaderArtifactCacheIndexEntry> entries;
};

struct ShaderArtifactValidationCommand {
    std::string executable;
    std::vector<std::string> arguments;
    ShaderGeneratedArtifact artifact;
};

struct ShaderArtifactValidationResult {
    int exit_code{0};
    std::string diagnostic;
    std::string stdout_text;
    std::string stderr_text;
    ShaderGeneratedArtifact artifact;

    [[nodiscard]] bool succeeded() const noexcept {
        return exit_code == 0;
    }
};

class IShaderArtifactValidatorRunner {
  public:
    virtual ~IShaderArtifactValidatorRunner() = default;
    [[nodiscard]] virtual ShaderArtifactValidationResult run(const ShaderArtifactValidationCommand& command) = 0;
};

class RecordingShaderArtifactValidatorRunner final : public IShaderArtifactValidatorRunner {
  public:
    [[nodiscard]] ShaderArtifactValidationResult run(const ShaderArtifactValidationCommand& command) override;
    [[nodiscard]] const std::vector<ShaderArtifactValidationCommand>& commands() const noexcept;

  private:
    std::vector<ShaderArtifactValidationCommand> commands_;
};

[[nodiscard]] std::string_view shader_tool_kind_name(ShaderToolKind kind) noexcept;
[[nodiscard]] std::vector<ShaderToolDescriptor> discover_shader_tools(const IFileSystem& filesystem,
                                                                      const ShaderToolDiscoveryRequest& request);
[[nodiscard]] ShaderToolchainReadiness
evaluate_shader_toolchain_readiness(const std::vector<ShaderToolDescriptor>& tools);
[[nodiscard]] ShaderArtifactValidationCommand
make_spirv_shader_validation_command(const ShaderGeneratedArtifact& artifact);
[[nodiscard]] bool is_safe_shader_artifact_validation_command(const ShaderArtifactValidationCommand& command) noexcept;
[[nodiscard]] ShaderArtifactValidationResult
run_shader_artifact_validation_command(IShaderArtifactValidatorRunner& runner,
                                       const ShaderArtifactValidationCommand& command);
[[nodiscard]] ShaderArtifactProvenance build_shader_artifact_provenance(const IFileSystem& filesystem,
                                                                        const ShaderCompileRequest& request,
                                                                        const ShaderCompileCommand& command,
                                                                        const ShaderToolDescriptor& tool);
[[nodiscard]] bool is_shader_artifact_cache_valid(const IFileSystem& filesystem,
                                                  const ShaderArtifactProvenance& provenance);

[[nodiscard]] std::string serialize_shader_artifact_provenance(const ShaderArtifactProvenance& provenance);
[[nodiscard]] ShaderArtifactProvenance deserialize_shader_artifact_provenance(std::string_view text);
void upsert_shader_artifact_cache_entry(ShaderArtifactCacheIndex& index, ShaderArtifactCacheIndexEntry entry);
[[nodiscard]] std::string serialize_shader_artifact_cache_index(const ShaderArtifactCacheIndex& index);
[[nodiscard]] ShaderArtifactCacheIndex deserialize_shader_artifact_cache_index(std::string_view text);
void save_shader_artifact_cache_index(IFileSystem& filesystem, std::string_view path,
                                      const ShaderArtifactCacheIndex& index);
[[nodiscard]] ShaderArtifactCacheIndex load_shader_artifact_cache_index(const IFileSystem& filesystem,
                                                                        std::string_view path);

} // namespace mirakana
