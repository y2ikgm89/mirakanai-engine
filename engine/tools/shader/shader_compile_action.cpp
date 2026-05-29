// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/shader_compile_action.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos && value.find('=') == std::string_view::npos;
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

[[nodiscard]] std::string marker_for_successful_compile(const ShaderCompileExecutionRequest& request,
                                                        const ShaderCompileCommand& command,
                                                        const ShaderToolRunResult& result) {
    std::ostringstream output;
    output << "format=GameEngine.ShaderArtifact.v1\n";
    output << "source=" << request.compile_request.source.source_path << '\n';
    output << "artifact=" << command.artifact.path << '\n';
    output << "profile=" << command.artifact.profile << '\n';
    output << "entry=" << command.artifact.entry_point << '\n';
    output << "tool.kind=" << shader_tool_kind_name(request.tool.kind) << '\n';
    output << "tool.version=" << request.tool.version << '\n';
    output << "diagnostic.bytes=" << result.diagnostic.size() << '\n';
    output << "stdout.bytes=" << result.stdout_text.size() << '\n';
    return output.str();
}

[[nodiscard]] const ShaderArtifactCacheIndexEntry* find_cache_entry(const ShaderArtifactCacheIndex& index,
                                                                    std::string_view artifact_path) noexcept {
    const auto found = std::ranges::find_if(index.entries, [artifact_path](const ShaderArtifactCacheIndexEntry& entry) {
        return entry.artifact_path == artifact_path;
    });
    return found == index.entries.end() ? nullptr : &*found;
}

struct CacheIndexLoadResult {
    bool exists{false};
    bool valid{false};
    ShaderArtifactCacheIndex index;
    std::string diagnostic;
};

[[nodiscard]] CacheIndexLoadResult load_cache_index_safely(const IFileSystem& filesystem, std::string_view path) {
    CacheIndexLoadResult result;
    result.exists = filesystem.exists(path);
    if (!result.exists) {
        result.diagnostic = "shader cache index is missing; repair from current provenance";
        return result;
    }

    try {
        result.index = load_shader_artifact_cache_index(filesystem, path);
        result.valid = true;
        result.diagnostic = "shader cache index is readable";
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"shader cache index is invalid: "} + error.what();
    }
    return result;
}

[[nodiscard]] ShaderArtifactProvenance load_provenance_for_plan(const IFileSystem& filesystem, std::string_view path) {
    return deserialize_shader_artifact_provenance(filesystem.read_text(path));
}

[[nodiscard]] bool cache_entry_matches_provenance(const ShaderArtifactCacheIndexEntry& entry,
                                                  const ShaderArtifactProvenance& provenance,
                                                  std::string_view provenance_path) noexcept {
    return entry.artifact_path == provenance.artifact.path && entry.provenance_path == provenance_path &&
           entry.command_fingerprint == provenance.command_fingerprint;
}

void append_unique(std::vector<std::string>& paths, std::string value) {
    if (std::ranges::find(paths, value) == paths.end()) {
        paths.push_back(std::move(value));
    }
}

[[nodiscard]] std::string target_environment_name(ShaderCompileTarget target) {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return "d3d12-dxil";
    case ShaderCompileTarget::vulkan_spirv:
        return "vulkan1.3";
    case ShaderCompileTarget::metal_ir:
        return "metal-ir";
    case ShaderCompileTarget::metal_library:
        return "metal-library";
    case ShaderCompileTarget::unknown:
        break;
    }
    return "unknown";
}

void append_review_diagnostic(std::vector<ShaderGenerationCacheReviewDiagnostic>& diagnostics,
                              ShaderGenerationCacheReviewDiagnosticCode code, ShaderCompileTarget target,
                              std::string artifact_path, std::string message) {
    diagnostics.push_back(ShaderGenerationCacheReviewDiagnostic{
        .code = code,
        .target = target,
        .artifact_path = std::move(artifact_path),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_invalid_shader_generation_diagnostic(ShaderGenerationCacheReviewDiagnosticCode code) noexcept {
    switch (code) {
    case ShaderGenerationCacheReviewDiagnosticCode::invalid_compile_request:
    case ShaderGenerationCacheReviewDiagnosticCode::missing_cache_metadata:
    case ShaderGenerationCacheReviewDiagnosticCode::missing_provenance_metadata:
    case ShaderGenerationCacheReviewDiagnosticCode::unsupported_live_shader_generation:
    case ShaderGenerationCacheReviewDiagnosticCode::unsupported_runtime_compiler_execution:
    case ShaderGenerationCacheReviewDiagnosticCode::unsupported_native_cache_handle_access:
    case ShaderGenerationCacheReviewDiagnosticCode::unsupported_renderer_rhi_residency:
    case ShaderGenerationCacheReviewDiagnosticCode::unsupported_metal_library_generation:
    case ShaderGenerationCacheReviewDiagnosticCode::row_budget_exceeded:
        return true;
    case ShaderGenerationCacheReviewDiagnosticCode::none:
    case ShaderGenerationCacheReviewDiagnosticCode::missing_d3d12_toolchain_evidence:
    case ShaderGenerationCacheReviewDiagnosticCode::missing_vulkan_toolchain_evidence:
    case ShaderGenerationCacheReviewDiagnosticCode::missing_spirv_validation_evidence:
        return false;
    }
    return true;
}

[[nodiscard]] const ShaderArtifactValidationExecutionRequest*
find_validation_request_for_artifact(const std::vector<ShaderArtifactValidationExecutionRequest>& requests,
                                     const ShaderGeneratedArtifact& artifact) noexcept {
    const auto found = std::ranges::find_if(requests, [&artifact](const ShaderArtifactValidationExecutionRequest& row) {
        return row.artifact.path == artifact.path && row.artifact.format == artifact.format &&
               row.artifact.profile == artifact.profile && row.artifact.entry_point == artifact.entry_point;
    });
    return found == requests.end() ? nullptr : &*found;
}

} // namespace

std::string shader_artifact_provenance_path(const ShaderGeneratedArtifact& artifact) {
    if (!is_valid_shader_generated_artifact(artifact) || is_absolute_or_parent_relative(artifact.path)) {
        throw std::invalid_argument("shader artifact path is invalid");
    }
    return artifact.path + ".geprovenance";
}

ShaderHotReloadPlan build_shader_hot_reload_plan(const IFileSystem& filesystem,
                                                 const ShaderCompileExecutionRequest& request) {
    if (!is_valid_shader_compile_request(request.compile_request)) {
        throw std::invalid_argument("shader hot reload compile request is invalid");
    }
    if (!valid_token(request.cache_index_path) || is_absolute_or_parent_relative(request.cache_index_path)) {
        throw std::invalid_argument("shader hot reload cache index path is unsafe");
    }

    const auto command = make_shader_compile_command(request.compile_request);
    ShaderHotReloadPlan plan;
    plan.artifact = command.artifact;
    plan.provenance_path = shader_artifact_provenance_path(command.artifact);
    plan.artifact_exists = filesystem.exists(command.artifact.path);
    plan.provenance_exists = filesystem.exists(plan.provenance_path);

    if (!plan.artifact_exists) {
        plan.compile_required = true;
        plan.pipeline_recreation_required = true;
        plan.diagnostic = "shader artifact is missing; compile before pipeline recreation";
        return plan;
    }
    if (!plan.provenance_exists) {
        plan.compile_required = true;
        plan.pipeline_recreation_required = true;
        plan.diagnostic = "shader artifact provenance is missing; compile before pipeline recreation";
        return plan;
    }

    try {
        const auto existing_provenance =
            deserialize_shader_artifact_provenance(filesystem.read_text(plan.provenance_path));
        const auto current_provenance =
            build_shader_artifact_provenance(filesystem, request.compile_request, command, request.tool);
        plan.inputs_current = existing_provenance.command_fingerprint == current_provenance.command_fingerprint &&
                              is_shader_artifact_cache_valid(filesystem, existing_provenance);
    } catch (const std::exception& error) {
        plan.compile_required = true;
        plan.pipeline_recreation_required = true;
        plan.diagnostic = std::string{"shader artifact provenance is invalid: "} + error.what();
        return plan;
    }

    if (!plan.inputs_current) {
        plan.compile_required = true;
        plan.pipeline_recreation_required = true;
        plan.diagnostic = "shader artifact provenance is stale; compile before pipeline recreation";
        return plan;
    }

    plan.diagnostic = "shader artifact is current; pipeline recreation is not required";
    return plan;
}

ShaderPipelineRecreationPlan build_shader_pipeline_recreation_plan(const ShaderCompileExecutionResult& result) {
    ShaderPipelineRecreationPlan plan;
    plan.artifact = result.tool_result.artifact;
    plan.target = result.provenance.target;

    if (!result.succeeded()) {
        plan.diagnostic = "shader compile failed; pipeline recreation skipped";
        return plan;
    }
    if (result.cache_hit) {
        plan.diagnostic = "shader compile cache hit; pipeline recreation is not required";
        return plan;
    }
    if (!is_valid_shader_generated_artifact(plan.artifact) || plan.target == ShaderCompileTarget::unknown) {
        plan.diagnostic = "shader compile result is missing pipeline recreation metadata";
        return plan;
    }

    plan.recreate_pipeline = true;
    plan.diagnostic = "shader pipeline recreation required";
    return plan;
}

ShaderPipelineCachePlan build_shader_pipeline_cache_plan(const IFileSystem& filesystem,
                                                         const std::vector<ShaderCompileExecutionRequest>& requests) {
    ShaderPipelineCachePlan plan;
    if (requests.empty()) {
        plan.diagnostics.emplace_back("shader pipeline cache plan requires at least one request");
        return plan;
    }

    plan.ok = true;
    plan.entries.reserve(requests.size());
    for (const auto& request : requests) {
        auto hot_reload = build_shader_hot_reload_plan(filesystem, request);
        ShaderPipelineCachePlanEntry entry;
        entry.artifact = hot_reload.artifact;
        entry.target = request.compile_request.target;
        entry.cache_index_path = request.cache_index_path;
        entry.hot_reload = std::move(hot_reload);

        const auto cache_index = load_cache_index_safely(filesystem, entry.cache_index_path);
        entry.cache_index_exists = cache_index.exists;
        entry.cache_index_valid = cache_index.valid;
        entry.cache_diagnostic = cache_index.diagnostic;

        if (entry.hot_reload.compile_required) {
            entry.cache_diagnostic = "shader compile required; cache index repair deferred";
        } else if (!entry.hot_reload.inputs_current || !entry.hot_reload.provenance_exists) {
            entry.cache_diagnostic = "shader provenance is not current; cache index repair deferred";
        } else {
            const auto provenance = load_provenance_for_plan(filesystem, entry.hot_reload.provenance_path);
            if (cache_index.valid) {
                const auto* cache_entry = find_cache_entry(cache_index.index, entry.artifact.path);
                entry.cache_entry_exists = cache_entry != nullptr;
                entry.cache_entry_current =
                    cache_entry != nullptr &&
                    cache_entry_matches_provenance(*cache_entry, provenance, entry.hot_reload.provenance_path);
            }

            if (!entry.cache_index_valid) {
                entry.cache_index_update_required = true;
            } else if (!entry.cache_entry_exists) {
                entry.cache_index_update_required = true;
                entry.cache_diagnostic = "shader cache entry is missing; repair from current provenance";
            } else if (!entry.cache_entry_current) {
                entry.cache_index_update_required = true;
                entry.cache_diagnostic = "shader cache entry is stale; repair from current provenance";
            } else {
                entry.cache_diagnostic = "shader cache entry is current";
            }
        }

        if (entry.hot_reload.compile_required) {
            ++plan.compile_required_count;
        }
        if (entry.hot_reload.pipeline_recreation_required) {
            ++plan.pipeline_recreation_required_count;
        }
        if (entry.cache_index_update_required) {
            ++plan.cache_index_update_required_count;
        }
        plan.entries.push_back(std::move(entry));
    }

    std::ranges::sort(plan.entries,
                      [](const ShaderPipelineCachePlanEntry& lhs, const ShaderPipelineCachePlanEntry& rhs) {
                          if (lhs.artifact.path != rhs.artifact.path) {
                              return lhs.artifact.path < rhs.artifact.path;
                          }
                          return lhs.cache_index_path < rhs.cache_index_path;
                      });
    return plan;
}

ShaderPipelineCacheReconcileResult
reconcile_shader_pipeline_cache_index(IFileSystem& filesystem,
                                      const std::vector<ShaderCompileExecutionRequest>& requests) {
    ShaderPipelineCacheReconcileResult result;
    result.plan = build_shader_pipeline_cache_plan(filesystem, requests);
    if (!result.plan.ok) {
        return result;
    }

    for (const auto& entry : result.plan.entries) {
        if (!entry.cache_index_update_required || !entry.hot_reload.inputs_current ||
            !entry.hot_reload.provenance_exists) {
            continue;
        }

        auto cache_index = load_cache_index_safely(filesystem, entry.cache_index_path);
        auto index = cache_index.valid ? std::move(cache_index.index) : ShaderArtifactCacheIndex{};
        const auto provenance = load_provenance_for_plan(filesystem, entry.hot_reload.provenance_path);
        upsert_shader_artifact_cache_entry(index, ShaderArtifactCacheIndexEntry{
                                                      .artifact_path = provenance.artifact.path,
                                                      .provenance_path = entry.hot_reload.provenance_path,
                                                      .command_fingerprint = provenance.command_fingerprint,
                                                  });
        save_shader_artifact_cache_index(filesystem, entry.cache_index_path, index);
        append_unique(result.written_cache_index_paths, entry.cache_index_path);
    }

    std::ranges::sort(result.written_cache_index_paths);
    result.written_cache_index_paths.erase(std::ranges::unique(result.written_cache_index_paths).begin(),
                                           result.written_cache_index_paths.end());
    return result;
}

ShaderCompileExecutionResult execute_shader_compile_action(IFileSystem& filesystem, IShaderToolRunner& runner,
                                                           const ShaderCompileExecutionRequest& request) {
    if (!is_valid_shader_compile_request(request.compile_request)) {
        throw std::invalid_argument("shader compile execution request is invalid");
    }
    if (!valid_token(request.cache_index_path) || is_absolute_or_parent_relative(request.cache_index_path)) {
        throw std::invalid_argument("shader cache index path is unsafe");
    }

    const auto command = make_shader_compile_command(request.compile_request);
    const auto provenance_path = shader_artifact_provenance_path(command.artifact);
    auto index = filesystem.exists(request.cache_index_path)
                     ? load_shader_artifact_cache_index(filesystem, request.cache_index_path)
                     : ShaderArtifactCacheIndex{};

    if (request.allow_cache) {
        const auto* entry = find_cache_entry(index, command.artifact.path);
        if (entry != nullptr && filesystem.exists(entry->provenance_path) && filesystem.exists(command.artifact.path)) {
            const auto cached_provenance =
                deserialize_shader_artifact_provenance(filesystem.read_text(entry->provenance_path));
            const auto current_provenance =
                build_shader_artifact_provenance(filesystem, request.compile_request, command, request.tool);
            if (entry->command_fingerprint == current_provenance.command_fingerprint &&
                cached_provenance.command_fingerprint == current_provenance.command_fingerprint &&
                is_shader_artifact_cache_valid(filesystem, cached_provenance)) {
                return ShaderCompileExecutionResult{
                    .tool_result = ShaderToolRunResult{.exit_code = 0,
                                                       .diagnostic = "cache hit",
                                                       .stdout_text = {},
                                                       .stderr_text = {},
                                                       .artifact = command.artifact},
                    .provenance = cached_provenance,
                    .provenance_path = entry->provenance_path,
                    .cache_hit = true,
                    .artifact_written = false,
                };
            }
        }
    }

    auto tool_result = run_shader_tool_command(runner, command);
    if (!tool_result.succeeded()) {
        return ShaderCompileExecutionResult{
            .tool_result = tool_result,
            .provenance = {},
            .provenance_path = provenance_path,
            .cache_hit = false,
            .artifact_written = false,
        };
    }

    bool artifact_written = false;
    if (request.write_artifact_marker && !filesystem.exists(command.artifact.path)) {
        filesystem.write_text(command.artifact.path, marker_for_successful_compile(request, command, tool_result));
        artifact_written = true;
    }
    if (!filesystem.exists(command.artifact.path)) {
        return ShaderCompileExecutionResult{
            .tool_result =
                ShaderToolRunResult{
                    .exit_code = -1,
                    .diagnostic = "shader artifact was not produced: " + command.artifact.path,
                    .stdout_text = tool_result.stdout_text,
                    .stderr_text = tool_result.stderr_text,
                    .artifact = command.artifact,
                },
            .provenance = {},
            .provenance_path = provenance_path,
            .cache_hit = false,
            .artifact_written = false,
        };
    }

    const auto provenance =
        build_shader_artifact_provenance(filesystem, request.compile_request, command, request.tool);
    filesystem.write_text(provenance_path, serialize_shader_artifact_provenance(provenance));
    upsert_shader_artifact_cache_entry(index, ShaderArtifactCacheIndexEntry{
                                                  .artifact_path = command.artifact.path,
                                                  .provenance_path = provenance_path,
                                                  .command_fingerprint = provenance.command_fingerprint,
                                              });
    save_shader_artifact_cache_index(filesystem, request.cache_index_path, index);

    return ShaderCompileExecutionResult{
        .tool_result = tool_result,
        .provenance = provenance,
        .provenance_path = provenance_path,
        .cache_hit = false,
        .artifact_written = artifact_written,
    };
}

ShaderArtifactValidationExecutionResult
execute_shader_artifact_validation_action(const IFileSystem& filesystem, IShaderArtifactValidatorRunner& runner,
                                          const ShaderArtifactValidationExecutionRequest& request) {
    if (!is_valid_shader_generated_artifact(request.artifact) ||
        request.artifact.format != ShaderArtifactFormat::spirv || request.validator.kind != ShaderToolKind::spirv_val ||
        !valid_token(request.validator.executable_path)) {
        throw std::invalid_argument("shader artifact validation request is invalid");
    }

    const auto command = make_spirv_shader_validation_command(request.artifact);
    if (!filesystem.exists(request.artifact.path)) {
        return ShaderArtifactValidationExecutionResult{
            .validation_result =
                ShaderArtifactValidationResult{
                    .exit_code = -1,
                    .diagnostic = "shader artifact is missing: " + request.artifact.path,
                    .stdout_text = {},
                    .stderr_text = {},
                    .artifact = command.artifact,
                },
            .artifact_checked = false,
        };
    }

    return ShaderArtifactValidationExecutionResult{
        .validation_result = run_shader_artifact_validation_command(runner, command),
        .artifact_checked = true,
    };
}

ShaderGenerationCacheReview
review_shader_generation_cache_execution(const IFileSystem& filesystem,
                                         const ShaderGenerationCacheReviewRequest& request) {
    ShaderGenerationCacheReview review;

    const auto append_unsupported = [&](ShaderGenerationCacheReviewDiagnosticCode code, std::string message) {
        ++review.unsupported_claim_rows;
        append_review_diagnostic(review.diagnostics, code, ShaderCompileTarget::unknown, {}, std::move(message));
    };

    if (request.request_live_shader_generation) {
        append_unsupported(ShaderGenerationCacheReviewDiagnosticCode::unsupported_live_shader_generation,
                           "live shader generation is unsupported by reviewed shader cache execution");
    }
    if (request.request_runtime_compiler_execution) {
        append_unsupported(ShaderGenerationCacheReviewDiagnosticCode::unsupported_runtime_compiler_execution,
                           "runtime shader compiler execution is unsupported");
    }
    if (request.request_native_cache_handle_access) {
        append_unsupported(ShaderGenerationCacheReviewDiagnosticCode::unsupported_native_cache_handle_access,
                           "native PSO/Vulkan/Metal cache handles are not exposed");
    }
    if (request.request_renderer_rhi_residency) {
        append_unsupported(ShaderGenerationCacheReviewDiagnosticCode::unsupported_renderer_rhi_residency,
                           "renderer/RHI residency is outside shader cache execution review");
    }
    if (request.request_metal_library_generation) {
        append_unsupported(ShaderGenerationCacheReviewDiagnosticCode::unsupported_metal_library_generation,
                           "Metal library generation remains Apple host/toolchain gated");
    }
    if (request.compile_requests.size() > request.row_budget) {
        append_review_diagnostic(review.diagnostics, ShaderGenerationCacheReviewDiagnosticCode::row_budget_exceeded,
                                 ShaderCompileTarget::unknown, {},
                                 "shader generation cache review row budget exceeded");
    }
    if (request.compile_requests.empty()) {
        review.status = review.diagnostics.empty() ? ShaderGenerationCacheReviewStatus::no_rows
                                                   : ShaderGenerationCacheReviewStatus::invalid_request;
        return review;
    }

    for (const auto& compile_request : request.compile_requests) {
        ShaderGenerationCacheReviewRow row;
        row.target = compile_request.compile_request.target;
        row.source_path = compile_request.compile_request.source.source_path;
        row.profile = compile_request.compile_request.profile;
        row.entry_point = compile_request.compile_request.source.entry_point;
        row.target_environment = target_environment_name(row.target);
        row.cache_index_path = compile_request.cache_index_path;

        try {
            const auto command = make_shader_compile_command(compile_request.compile_request);
            row.artifact = command.artifact;
            row.executable = command.executable;
            row.command_arguments = command.arguments;
            row.provenance_path = shader_artifact_provenance_path(command.artifact);

            switch (row.target) {
            case ShaderCompileTarget::d3d12_dxil:
                ++review.d3d12_compile_rows;
                row.host_toolchain_ready = request.toolchain.ready_for_d3d12_dxil();
                if (!row.host_toolchain_ready) {
                    ++review.host_gated_rows;
                    append_review_diagnostic(
                        review.diagnostics, ShaderGenerationCacheReviewDiagnosticCode::missing_d3d12_toolchain_evidence,
                        row.target, row.artifact.path,
                        "DXC toolchain evidence is missing for D3D12 DXIL shader execution");
                }
                break;
            case ShaderCompileTarget::vulkan_spirv:
                ++review.vulkan_compile_rows;
                row.host_toolchain_ready = request.toolchain.ready_for_vulkan_spirv();
                if (!row.host_toolchain_ready) {
                    append_review_diagnostic(
                        review.diagnostics,
                        ShaderGenerationCacheReviewDiagnosticCode::missing_vulkan_toolchain_evidence, row.target,
                        row.artifact.path,
                        "DXC SPIR-V CodeGen and spirv-val evidence is missing for Vulkan SPIR-V shader execution");
                }
                if (const auto* validation_request =
                        find_validation_request_for_artifact(request.validation_requests, command.artifact);
                    validation_request != nullptr && validation_request->validator.kind == ShaderToolKind::spirv_val &&
                    valid_token(validation_request->validator.executable_path)) {
                    const auto validation_command = make_spirv_shader_validation_command(validation_request->artifact);
                    row.validation_arguments = validation_command.arguments;
                    row.spirv_validation_ready = is_safe_shader_artifact_validation_command(validation_command);
                }
                if (row.spirv_validation_ready) {
                    ++review.spirv_validation_rows;
                } else {
                    append_review_diagnostic(
                        review.diagnostics,
                        ShaderGenerationCacheReviewDiagnosticCode::missing_spirv_validation_evidence, row.target,
                        row.artifact.path, "SPIR-V validation command evidence is missing for Vulkan shader artifacts");
                }
                if (!row.host_toolchain_ready || !row.spirv_validation_ready) {
                    ++review.host_gated_rows;
                }
                break;
            case ShaderCompileTarget::metal_ir:
            case ShaderCompileTarget::metal_library:
                ++review.host_gated_rows;
                append_review_diagnostic(
                    review.diagnostics, ShaderGenerationCacheReviewDiagnosticCode::unsupported_metal_library_generation,
                    row.target, row.artifact.path, "Metal shader generation remains Apple host/toolchain gated");
                break;
            case ShaderCompileTarget::unknown:
                append_review_diagnostic(review.diagnostics,
                                         ShaderGenerationCacheReviewDiagnosticCode::invalid_compile_request, row.target,
                                         row.artifact.path, "shader compile target is unknown");
                break;
            }

            const auto cache_plan = build_shader_pipeline_cache_plan(filesystem, {compile_request});
            if (!cache_plan.entries.empty()) {
                const auto& cache_entry = cache_plan.entries.front();
                row.cache_entry_current = cache_entry.cache_index_valid && cache_entry.cache_entry_current;
                row.provenance_current =
                    cache_entry.hot_reload.provenance_exists && cache_entry.hot_reload.inputs_current;
            }
            if (row.cache_entry_current) {
                ++review.cache_key_rows;
            } else {
                append_review_diagnostic(review.diagnostics,
                                         ShaderGenerationCacheReviewDiagnosticCode::missing_cache_metadata, row.target,
                                         row.artifact.path, "shader cache index entry is missing or stale");
            }
            if (row.provenance_current) {
                ++review.provenance_rows;
                const auto provenance =
                    deserialize_shader_artifact_provenance(filesystem.read_text(row.provenance_path));
                row.command_fingerprint = provenance.command_fingerprint;
                row.input_count = provenance.inputs.size();
            } else {
                append_review_diagnostic(
                    review.diagnostics, ShaderGenerationCacheReviewDiagnosticCode::missing_provenance_metadata,
                    row.target, row.artifact.path, "shader provenance metadata is missing or stale");
            }
            row.ready = row.host_toolchain_ready && row.cache_entry_current && row.provenance_current &&
                        (row.target != ShaderCompileTarget::vulkan_spirv || row.spirv_validation_ready);
            if (row.ready) {
                ++review.ready_rows;
            }
        } catch (const std::exception& error) {
            append_review_diagnostic(
                review.diagnostics, ShaderGenerationCacheReviewDiagnosticCode::invalid_compile_request, row.target,
                row.artifact.path, std::string{"shader generation cache review row is invalid: "} + error.what());
        }

        review.rows.push_back(std::move(row));
    }

    review.d3d12_offline_compile_ready =
        review.d3d12_compile_rows > 0U && std::ranges::any_of(review.rows, [](const auto& row) {
            return row.target == ShaderCompileTarget::d3d12_dxil && row.ready;
        });
    review.vulkan_offline_compile_ready =
        review.vulkan_compile_rows > 0U && std::ranges::any_of(review.rows, [](const auto& row) {
            return row.target == ShaderCompileTarget::vulkan_spirv && row.ready;
        });
    review.selected_package_cache_ready = !review.rows.empty() && review.ready_rows == review.rows.size() &&
                                          review.cache_key_rows == review.rows.size() &&
                                          review.provenance_rows == review.rows.size();
    review.reviewed = !review.rows.empty();

    const bool has_invalid_diagnostic = std::ranges::any_of(review.diagnostics, [](const auto& diagnostic) {
        return is_invalid_shader_generation_diagnostic(diagnostic.code);
    });
    if (has_invalid_diagnostic) {
        review.status = ShaderGenerationCacheReviewStatus::invalid_request;
    } else if (review.host_gated_rows > 0U) {
        review.status = ShaderGenerationCacheReviewStatus::host_evidence_required;
    } else {
        review.status = ShaderGenerationCacheReviewStatus::ready;
    }
    review.ready = review.status == ShaderGenerationCacheReviewStatus::ready && review.selected_package_cache_ready;
    return review;
}

} // namespace mirakana
