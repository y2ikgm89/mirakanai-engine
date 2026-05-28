// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"

#include <memory>
#include <string_view>
#include <utility>

namespace mirakana::runtime_host_win32_detail {

class Win32SceneGpuBindingInjectingRenderer final : public IRenderer {
  public:
    Win32SceneGpuBindingInjectingRenderer(std::unique_ptr<IRenderer> inner,
                                          runtime_scene_rhi::RuntimeSceneGpuBindingResult bindings)
        : inner_(std::move(inner)), bindings_(std::move(bindings)) {
        const auto upload_report = runtime_scene_rhi::make_runtime_scene_gpu_upload_execution_report(bindings_);
        stats_.mesh_bindings = bindings_.palette.mesh_count();
        stats_.material_bindings = bindings_.palette.material_count();
        stats_.mesh_uploads = upload_report.mesh_uploads;
        stats_.texture_uploads = upload_report.texture_uploads;
        stats_.material_uploads = upload_report.material_bindings;
        stats_.material_pipeline_layouts = upload_report.material_pipeline_layouts;
        stats_.uploaded_texture_bytes = upload_report.uploaded_texture_bytes;
        stats_.uploaded_mesh_bytes = upload_report.uploaded_mesh_bytes;
        stats_.uploaded_material_factor_bytes = upload_report.uploaded_material_factor_bytes;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return inner_->backend_name();
    }

    [[nodiscard]] Extent2D backbuffer_extent() const noexcept override {
        return inner_->backbuffer_extent();
    }

    [[nodiscard]] RendererStats stats() const noexcept override {
        return inner_->stats();
    }

    [[nodiscard]] Win32DesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept {
        return stats_;
    }

    void resize(Extent2D extent) override {
        inner_->resize(extent);
    }

    void set_clear_color(Color color) noexcept override {
        inner_->set_clear_color(color);
    }

    void begin_frame() override {
        inner_->begin_frame();
    }

    void draw_sprite(const SpriteCommand& command) override {
        inner_->draw_sprite(command);
    }

    void draw_mesh(const MeshCommand& command) override {
        auto patched = command;
        if (patched.mesh_binding.owner_device == nullptr) {
            if (const auto* binding = bindings_.palette.find_mesh(patched.mesh); binding != nullptr) {
                patched.mesh_binding = *binding;
                ++stats_.mesh_bindings_resolved;
            }
        }
        if (patched.material_binding.owner_device == nullptr) {
            if (const auto* binding = bindings_.palette.find_material(patched.material); binding != nullptr) {
                patched.material_binding = *binding;
                ++stats_.material_bindings_resolved;
            }
        }
        inner_->draw_mesh(patched);
    }

    void end_frame() override {
        inner_->end_frame();
    }

  private:
    std::unique_ptr<IRenderer> inner_;
    runtime_scene_rhi::RuntimeSceneGpuBindingResult bindings_;
    Win32DesktopPresentationSceneGpuBindingStats stats_;
};

} // namespace mirakana::runtime_host_win32_detail
