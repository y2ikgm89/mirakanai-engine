// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/tools/shader_toolchain.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
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

enum class ShaderGenerationCacheReviewStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class ShaderGenerationCacheReviewDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_compile_request,
    missing_d3d12_toolchain_evidence,
    missing_vulkan_toolchain_evidence,
    missing_spirv_validation_evidence,
    missing_cache_metadata,
    missing_provenance_metadata,
    unsupported_live_shader_generation,
    unsupported_runtime_compiler_execution,
    unsupported_native_cache_handle_access,
    unsupported_renderer_rhi_residency,
    unsupported_metal_library_generation,
    row_budget_exceeded,
};

struct ShaderGenerationCacheReviewRequest {
    std::vector<ShaderCompileExecutionRequest> compile_requests;
    std::vector<ShaderArtifactValidationExecutionRequest> validation_requests;
    ShaderToolchainReadiness toolchain;
    std::size_t row_budget{64U};
    bool request_live_shader_generation{false};
    bool request_runtime_compiler_execution{false};
    bool request_native_cache_handle_access{false};
    bool request_renderer_rhi_residency{false};
    bool request_metal_library_generation{false};
};

struct ShaderGenerationCacheReviewRow {
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    ShaderGeneratedArtifact artifact;
    std::string source_path;
    std::string profile;
    std::string entry_point;
    std::string target_environment;
    std::string executable;
    std::vector<std::string> command_arguments;
    std::vector<std::string> validation_arguments;
    std::string cache_index_path;
    std::string provenance_path;
    std::string command_fingerprint;
    std::size_t input_count{0U};
    bool host_toolchain_ready{false};
    bool cache_entry_current{false};
    bool provenance_current{false};
    bool spirv_validation_ready{false};
    bool ready{false};
};

struct ShaderGenerationCacheReviewDiagnostic {
    ShaderGenerationCacheReviewDiagnosticCode code{ShaderGenerationCacheReviewDiagnosticCode::none};
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string artifact_path;
    std::string message;
};

struct ShaderGenerationCacheReview {
    ShaderGenerationCacheReviewStatus status{ShaderGenerationCacheReviewStatus::invalid_request};
    std::vector<ShaderGenerationCacheReviewDiagnostic> diagnostics;
    std::vector<ShaderGenerationCacheReviewRow> rows;
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t unsupported_claim_rows{0U};
    std::size_t d3d12_compile_rows{0U};
    std::size_t vulkan_compile_rows{0U};
    std::size_t spirv_validation_rows{0U};
    std::size_t cache_key_rows{0U};
    std::size_t provenance_rows{0U};
    bool reviewed{false};
    bool ready{false};
    bool d3d12_offline_compile_ready{false};
    bool vulkan_offline_compile_ready{false};
    bool selected_package_cache_ready{false};
    bool live_shader_generation_ready{false};
    bool runtime_compiler_execution_ready{false};
    bool native_cache_handle_ready{false};
    bool renderer_rhi_residency_ready{false};
    bool metal_library_generation_ready{false};
    bool invoked_live_shader_generation{false};
    bool invoked_runtime_compiler{false};
    bool exposed_native_cache_handle{false};
    bool invoked_renderer_rhi_residency{false};
    bool invoked_metal_library_generation{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == ShaderGenerationCacheReviewStatus::ready;
    }
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
[[nodiscard]] ShaderGenerationCacheReview
review_shader_generation_cache_execution(const IFileSystem& filesystem,
                                         const ShaderGenerationCacheReviewRequest& request);

} // namespace mirakana
