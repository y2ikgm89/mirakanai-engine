// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/shader_metadata.hpp"
#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/sdl_viewport_texture.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <SDL3/SDL.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::editor {

struct MaterialPreviewGpuRebuildDeps {
    mirakana::rhi::IRhiDevice* device{};
    SDL_Renderer* sdl_renderer{};
    mirakana::IFileSystem* tool_filesystem{};
    const mirakana::AssetRegistry* assets{};
    std::function<std::optional<std::string>(mirakana::AssetId shader, mirakana::ShaderSourceStage stage,
                                             mirakana::ShaderCompileTarget target)>
        read_material_preview_shader_bytecode;
};

struct MaterialPreviewGpuRenderDeps {
    mirakana::rhi::IRhiDevice* device{};
    std::function<void(mirakana::RhiViewportSurface&, SdlViewportTexture&)> sync_display_texture;
};

/// GPU-resident material preview cache for the Dear ImGui editor shell (`MK_editor`).
class MaterialPreviewGpuCache {
  public:
    [[nodiscard]] bool ready() const noexcept;

    [[nodiscard]] const SdlViewportTexture* display_texture() const noexcept {
        return display_texture_.get();
    }

    void reset() noexcept;

    MaterialPreviewGpuCache& rebuild(const MaterialPreviewGpuRebuildDeps& deps, mirakana::AssetId material);

    MaterialPreviewGpuCache& ensure(const MaterialPreviewGpuRebuildDeps& deps, mirakana::AssetId material);

    void render(const MaterialPreviewGpuRenderDeps& deps);

    [[nodiscard]] EditorMaterialGpuPreviewExecutionSnapshot execution_snapshot() const;

  private:
    mirakana::AssetId material_{};
    std::string artifact_path_;
    EditorMaterialGpuPreviewStatus status_{EditorMaterialGpuPreviewStatus::unknown};
    std::string diagnostic_;
    std::unique_ptr<mirakana::RhiViewportSurface> surface_;
    std::unique_ptr<SdlViewportTexture> display_texture_;
    std::vector<mirakana::runtime_rhi::RuntimeTextureUploadResult> texture_uploads_;
    /// Caller-owned scene PBR uniform passed into `create_runtime_material_gpu_binding` as
    /// `shared_scene_pbr_frame_uniform`.
    mirakana::rhi::BufferHandle scene_pbr_frame_uniform_{};
    mirakana::runtime_rhi::RuntimeMaterialGpuBinding material_binding_{};
    mirakana::rhi::PipelineLayoutHandle pipeline_layout_{};
    mirakana::rhi::GraphicsPipelineHandle pipeline_{};
    std::uint64_t rendered_version_{0};
};

[[nodiscard]] inline EditorMaterialGpuPreviewExecutionSnapshot
make_material_gpu_preview_execution_snapshot(const MaterialPreviewGpuCache& cache) {
    return cache.execution_snapshot();
}

} // namespace mirakana::editor
