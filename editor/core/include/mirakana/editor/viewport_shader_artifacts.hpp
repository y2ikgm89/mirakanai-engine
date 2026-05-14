// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class ViewportShaderArtifactStatus : std::uint8_t {
    missing,
    empty,
    ready,
    unsupported_stage,
    unsupported_target,
};

struct ViewportShaderArtifactItem {
    AssetId shader;
    ShaderSourceStage stage{ShaderSourceStage::unknown};
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string output_path;
    std::string profile;
    std::string entry_point;
    ViewportShaderArtifactStatus status{ViewportShaderArtifactStatus::missing};
    std::size_t byte_size{0};
    std::string diagnostic;
};

class ViewportShaderArtifactState {
  public:
    void refresh_from(const IFileSystem& filesystem, const std::vector<ShaderCompileRequest>& requests);
    void clear() noexcept;

    [[nodiscard]] const std::vector<ViewportShaderArtifactItem>& items() const noexcept;
    [[nodiscard]] std::size_t item_count() const noexcept;
    [[nodiscard]] std::size_t ready_count() const noexcept;
    [[nodiscard]] bool ready_for_d3d12() const noexcept;
    [[nodiscard]] bool ready_for_d3d12(AssetId shader) const noexcept;
    [[nodiscard]] bool ready_for_vulkan() const noexcept;
    [[nodiscard]] bool ready_for_vulkan(AssetId shader) const noexcept;
    [[nodiscard]] const ViewportShaderArtifactItem* find(ShaderSourceStage stage) const noexcept;
    [[nodiscard]] const ViewportShaderArtifactItem* find(ShaderSourceStage stage,
                                                         ShaderCompileTarget target) const noexcept;
    [[nodiscard]] const ViewportShaderArtifactItem* find(AssetId shader, ShaderSourceStage stage,
                                                         ShaderCompileTarget target) const noexcept;

  private:
    std::vector<ViewportShaderArtifactItem> items_;
};

[[nodiscard]] std::string_view viewport_shader_artifact_status_label(ViewportShaderArtifactStatus status) noexcept;

} // namespace mirakana::editor
