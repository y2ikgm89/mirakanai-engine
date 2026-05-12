// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/shader_compile.hpp"

#include <algorithm>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string join_relative_path(std::string_view root, std::string_view child) {
    if (root.empty() || root == ".") {
        return std::string(child);
    }
    if (root.back() == '/' || root.back() == '\\') {
        return std::string(root) + std::string(child);
    }
    return std::string(root) + "/" + std::string(child);
}

[[nodiscard]] std::string execution_diagnostic(const ShaderCompileExecutionResult& result) {
    if (result.cache_hit) {
        return result.tool_result.diagnostic.empty() ? std::string("cache hit") : result.tool_result.diagnostic;
    }
    if (result.succeeded()) {
        return result.tool_result.diagnostic.empty() ? std::string("compiled") : result.tool_result.diagnostic;
    }
    if (!result.tool_result.diagnostic.empty() && !result.tool_result.stderr_text.empty()) {
        return result.tool_result.diagnostic + ": " + result.tool_result.stderr_text;
    }
    if (!result.tool_result.diagnostic.empty()) {
        return result.tool_result.diagnostic;
    }
    if (!result.tool_result.stderr_text.empty()) {
        return result.tool_result.stderr_text;
    }
    return "shader compile failed";
}

[[nodiscard]] EditorShaderCompileStatus execution_status(const ShaderCompileExecutionResult& result) noexcept {
    if (result.cache_hit) {
        return EditorShaderCompileStatus::cached;
    }
    return result.succeeded() ? EditorShaderCompileStatus::compiled : EditorShaderCompileStatus::failed;
}

} // namespace

void ShaderCompileState::set_requests(const std::vector<ShaderCompileRequest>& requests) {
    items_.clear();
    items_.reserve(requests.size());
    for (const auto& request : requests) {
        items_.push_back(EditorShaderCompileItem{
            .shader = request.source.id,
            .target = request.target,
            .source_path = request.source.source_path,
            .output_path = request.output_path,
            .profile = request.profile,
            .status = EditorShaderCompileStatus::pending,
            .diagnostic = {},
            .cache_hit = false,
        });
    }

    std::ranges::sort(items_, [](const EditorShaderCompileItem& lhs, const EditorShaderCompileItem& rhs) {
        if (lhs.output_path != rhs.output_path) {
            return lhs.output_path < rhs.output_path;
        }
        return lhs.shader.value < rhs.shader.value;
    });
}

void ShaderCompileState::apply_updates(const std::vector<EditorShaderCompileUpdate>& updates) {
    for (const auto& update : updates) {
        auto* item = find_item(update.shader, update.output_path);
        if (item == nullptr) {
            continue;
        }
        item->status = update.status;
        item->diagnostic = update.diagnostic;
        item->cache_hit = update.cache_hit;
    }
}

void ShaderCompileState::apply_execution_results(const std::vector<EditorShaderCompileExecution>& executions) {
    std::vector<EditorShaderCompileUpdate> updates;
    updates.reserve(executions.size());
    for (const auto& execution : executions) {
        updates.push_back(EditorShaderCompileUpdate{
            .shader = execution.request.source.id,
            .output_path = execution.request.output_path,
            .status = execution_status(execution.result),
            .diagnostic = execution_diagnostic(execution.result),
            .cache_hit = execution.result.cache_hit,
        });
    }
    apply_updates(updates);
}

void ShaderCompileState::clear() noexcept {
    items_.clear();
}

const std::vector<EditorShaderCompileItem>& ShaderCompileState::items() const noexcept {
    return items_;
}

std::size_t ShaderCompileState::item_count() const noexcept {
    return items_.size();
}

std::size_t ShaderCompileState::pending_count() const noexcept {
    return count_status(EditorShaderCompileStatus::pending);
}

std::size_t ShaderCompileState::cached_count() const noexcept {
    return count_status(EditorShaderCompileStatus::cached);
}

std::size_t ShaderCompileState::compiled_count() const noexcept {
    return count_status(EditorShaderCompileStatus::compiled);
}

std::size_t ShaderCompileState::failed_count() const noexcept {
    return count_status(EditorShaderCompileStatus::failed);
}

EditorShaderCompileItem* ShaderCompileState::find_item(AssetId shader, std::string_view output_path) noexcept {
    const auto found = std::ranges::find_if(items_, [shader, output_path](const auto& item) {
        return item.shader == shader && item.output_path == output_path;
    });
    return found == items_.end() ? nullptr : &*found;
}

std::size_t ShaderCompileState::count_status(EditorShaderCompileStatus status) const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(items_, [status](const auto& item) { return item.status == status; }));
}

std::vector<ShaderCompileRequest> make_viewport_shader_compile_requests(std::string_view artifact_output_root) {
    const auto shader_id = AssetId::from_name("editor.default.shader");
    const auto shader_source_path = join_relative_path(artifact_output_root, "default.hlsl");

    return {
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-default.vs.dxil"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-default.ps.dxil"),
            .profile = "ps_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-default.vs.spv"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-default.ps.spv"),
            .profile = "ps_6_7",
        },
    };
}

std::vector<ShaderCompileRequest> make_material_preview_shader_compile_requests(std::string_view artifact_output_root) {
    const auto factor_shader_id = AssetId::from_name("editor.material_preview.factor.shader");
    const auto textured_shader_id = AssetId::from_name("editor.material_preview.textured.shader");
    const auto shader_source_path = join_relative_path(artifact_output_root, "material-preview.hlsl");

    return {
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = factor_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {"MK_MATERIAL_PREVIEW_FACTOR_ONLY=1"},
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-factor.vs.dxil"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = factor_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {"MK_MATERIAL_PREVIEW_FACTOR_ONLY=1"},
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-factor.ps.dxil"),
            .profile = "ps_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = textured_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {"MK_MATERIAL_PREVIEW_TEXTURED=1"},
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-textured.vs.dxil"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = textured_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {"MK_MATERIAL_PREVIEW_TEXTURED=1"},
                },
            .target = ShaderCompileTarget::d3d12_dxil,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-textured.ps.dxil"),
            .profile = "ps_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = factor_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {"MK_MATERIAL_PREVIEW_FACTOR_ONLY=1"},
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-factor.vs.spv"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = factor_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {"MK_MATERIAL_PREVIEW_FACTOR_ONLY=1"},
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-factor.ps.spv"),
            .profile = "ps_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = textured_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {"MK_MATERIAL_PREVIEW_TEXTURED=1"},
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-textured.vs.spv"),
            .profile = "vs_6_7",
        },
        ShaderCompileRequest{
            .source =
                ShaderSourceMetadata{
                    .id = textured_shader_id,
                    .source_path = shader_source_path,
                    .language = ShaderSourceLanguage::hlsl,
                    .stage = ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {"MK_MATERIAL_PREVIEW_TEXTURED=1"},
                },
            .target = ShaderCompileTarget::vulkan_spirv,
            .output_path = join_relative_path(artifact_output_root, "editor-material-preview-textured.ps.spv"),
            .profile = "ps_6_7",
        },
    };
}

std::string_view editor_shader_compile_status_label(EditorShaderCompileStatus status) noexcept {
    switch (status) {
    case EditorShaderCompileStatus::pending:
        return "Pending";
    case EditorShaderCompileStatus::cached:
        return "Cached";
    case EditorShaderCompileStatus::compiled:
        return "Compiled";
    case EditorShaderCompileStatus::failed:
        return "Failed";
    case EditorShaderCompileStatus::unknown:
        break;
    }
    return "Unknown";
}

} // namespace mirakana::editor
