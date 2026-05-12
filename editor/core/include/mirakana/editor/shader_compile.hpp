// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/tools/shader_compile_action.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorShaderCompileStatus { unknown, pending, cached, compiled, failed };

struct EditorShaderCompileItem {
    AssetId shader;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string source_path;
    std::string output_path;
    std::string profile;
    EditorShaderCompileStatus status{EditorShaderCompileStatus::unknown};
    std::string diagnostic;
    bool cache_hit{false};
};

struct EditorShaderCompileUpdate {
    AssetId shader;
    std::string output_path;
    EditorShaderCompileStatus status{EditorShaderCompileStatus::unknown};
    std::string diagnostic;
    bool cache_hit{false};
};

struct EditorShaderCompileExecution {
    ShaderCompileRequest request;
    ShaderCompileExecutionResult result;
};

class ShaderCompileState {
  public:
    void set_requests(const std::vector<ShaderCompileRequest>& requests);
    void apply_updates(const std::vector<EditorShaderCompileUpdate>& updates);
    void apply_execution_results(const std::vector<EditorShaderCompileExecution>& executions);
    void clear() noexcept;

    [[nodiscard]] const std::vector<EditorShaderCompileItem>& items() const noexcept;
    [[nodiscard]] std::size_t item_count() const noexcept;
    [[nodiscard]] std::size_t pending_count() const noexcept;
    [[nodiscard]] std::size_t cached_count() const noexcept;
    [[nodiscard]] std::size_t compiled_count() const noexcept;
    [[nodiscard]] std::size_t failed_count() const noexcept;

  private:
    [[nodiscard]] EditorShaderCompileItem* find_item(AssetId shader, std::string_view output_path) noexcept;
    [[nodiscard]] std::size_t count_status(EditorShaderCompileStatus status) const noexcept;

    std::vector<EditorShaderCompileItem> items_;
};

[[nodiscard]] std::vector<ShaderCompileRequest>
make_viewport_shader_compile_requests(std::string_view artifact_output_root);
[[nodiscard]] std::vector<ShaderCompileRequest>
make_material_preview_shader_compile_requests(std::string_view artifact_output_root);
[[nodiscard]] std::string_view editor_shader_compile_status_label(EditorShaderCompileStatus status) noexcept;

} // namespace mirakana::editor
