// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/tools/shader_toolchain.hpp"

#include <cstdint>
#include <vector>

namespace mirakana {

struct ShaderCompileExecutionRequest {
    ShaderCompileRequest compile_request;
    ShaderToolDescriptor tool;
    std::string cache_index_path;
    bool allow_cache{true};
    bool write_artifact_marker{true};
};

struct ShaderCompileExecutionResult {
    ShaderToolRunResult tool_result;
    ShaderArtifactProvenance provenance;
    std::string provenance_path;
    bool cache_hit{false};
    bool artifact_written{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return cache_hit || tool_result.succeeded();
    }
};

struct ShaderArtifactValidationExecutionRequest {
    ShaderGeneratedArtifact artifact;
    ShaderToolDescriptor validator;
};

struct ShaderArtifactValidationExecutionResult {
    ShaderArtifactValidationResult validation_result;
    bool artifact_checked{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return artifact_checked && validation_result.succeeded();
    }
};

struct ShaderHotReloadPlan {
    ShaderGeneratedArtifact artifact;
    std::string provenance_path;
    bool artifact_exists{false};
    bool provenance_exists{false};
    bool inputs_current{false};
    bool compile_required{false};
    bool pipeline_recreation_required{false};
    std::string diagnostic;
};

struct ShaderPipelineRecreationPlan {
    ShaderGeneratedArtifact artifact;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    bool recreate_pipeline{false};
    std::string diagnostic;
};

struct ShaderPipelineCachePlanEntry {
    ShaderGeneratedArtifact artifact;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string cache_index_path;
    ShaderHotReloadPlan hot_reload;
    bool cache_index_exists{false};
    bool cache_index_valid{false};
    bool cache_entry_exists{false};
    bool cache_entry_current{false};
    bool cache_index_update_required{false};
    std::string cache_diagnostic;
};

struct ShaderPipelineCachePlan {
    bool ok{false};
    std::vector<ShaderPipelineCachePlanEntry> entries;
    std::uint32_t compile_required_count{0};
    std::uint32_t pipeline_recreation_required_count{0};
    std::uint32_t cache_index_update_required_count{0};
    std::vector<std::string> diagnostics;
};

struct ShaderPipelineCacheReconcileResult {
    ShaderPipelineCachePlan plan;
    std::vector<std::string> written_cache_index_paths;
};

[[nodiscard]] std::string shader_artifact_provenance_path(const ShaderGeneratedArtifact& artifact);
[[nodiscard]] ShaderHotReloadPlan build_shader_hot_reload_plan(const IFileSystem& filesystem,
                                                               const ShaderCompileExecutionRequest& request);
[[nodiscard]] ShaderPipelineRecreationPlan
build_shader_pipeline_recreation_plan(const ShaderCompileExecutionResult& result);
[[nodiscard]] ShaderPipelineCachePlan
build_shader_pipeline_cache_plan(const IFileSystem& filesystem,
                                 const std::vector<ShaderCompileExecutionRequest>& requests);
[[nodiscard]] ShaderPipelineCacheReconcileResult
reconcile_shader_pipeline_cache_index(IFileSystem& filesystem,
                                      const std::vector<ShaderCompileExecutionRequest>& requests);
[[nodiscard]] ShaderCompileExecutionResult execute_shader_compile_action(IFileSystem& filesystem,
                                                                         IShaderToolRunner& runner,
                                                                         const ShaderCompileExecutionRequest& request);
[[nodiscard]] ShaderArtifactValidationExecutionResult
execute_shader_artifact_validation_action(const IFileSystem& filesystem, IShaderArtifactValidatorRunner& runner,
                                          const ShaderArtifactValidationExecutionRequest& request);

} // namespace mirakana
