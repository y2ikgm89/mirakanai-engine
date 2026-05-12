// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime_host_sdl3_detail {

struct SceneComputeMorphMeshBinding {
    AssetId mesh;
    AssetId morph_mesh;
    MeshGpuBinding mesh_binding;
};

class SceneGpuBindingInjectingRenderer final : public IRenderer {
  public:
    SceneGpuBindingInjectingRenderer(std::unique_ptr<IRenderer> inner,
                                     runtime_scene_rhi::RuntimeSceneGpuBindingResult bindings,
                                     std::vector<SdlDesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings = {},
                                     std::vector<SceneComputeMorphMeshBinding> compute_morph_mesh_bindings = {},
                                     std::size_t compute_morph_queue_waits = 0,
                                     std::size_t compute_morph_skinned_dispatches = 0,
                                     std::size_t compute_morph_skinned_queue_waits = 0)
        : inner_(std::move(inner)), bindings_(std::move(bindings)),
          morph_mesh_bindings_(std::move(morph_mesh_bindings)),
          compute_morph_mesh_bindings_(std::move(compute_morph_mesh_bindings)) {
        const auto upload_report = runtime_scene_rhi::make_runtime_scene_gpu_upload_execution_report(bindings_);
        stats_.mesh_bindings = bindings_.palette.mesh_count();
        stats_.skinned_mesh_bindings = bindings_.skinned_palette.skinned_mesh_count();
        stats_.morph_mesh_bindings = bindings_.morph_palette.morph_mesh_count();
        stats_.compute_morph_mesh_bindings = compute_morph_mesh_bindings_.size();
        stats_.compute_morph_mesh_dispatches = compute_morph_mesh_bindings_.size();
        stats_.compute_morph_queue_waits = compute_morph_queue_waits;
        stats_.compute_morph_skinned_mesh_bindings = upload_report.compute_morph_skinned_mesh_bindings;
        stats_.compute_morph_skinned_mesh_dispatches = compute_morph_skinned_dispatches;
        stats_.compute_morph_skinned_queue_waits = compute_morph_skinned_queue_waits;
        stats_.material_bindings = bindings_.palette.material_count();
        stats_.mesh_uploads = upload_report.mesh_uploads;
        stats_.skinned_mesh_uploads = upload_report.skinned_mesh_uploads;
        stats_.morph_mesh_uploads = upload_report.morph_mesh_uploads;
        stats_.texture_uploads = upload_report.texture_uploads;
        stats_.material_uploads = upload_report.material_bindings;
        stats_.material_pipeline_layouts = upload_report.material_pipeline_layouts;
        stats_.uploaded_texture_bytes = upload_report.uploaded_texture_bytes;
        stats_.uploaded_mesh_bytes = upload_report.uploaded_mesh_bytes;
        stats_.uploaded_morph_bytes = upload_report.uploaded_morph_bytes;
        stats_.uploaded_material_factor_bytes = upload_report.uploaded_material_factor_bytes;
        stats_.compute_morph_output_position_bytes = upload_report.compute_morph_output_position_bytes;
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

    [[nodiscard]] SdlDesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept {
        auto stats = stats_;
        if (stats.compute_morph_mesh_bindings == 0 && stats.compute_morph_skinned_mesh_bindings == 0) {
            return stats;
        }
        const auto* device = scene_rhi_device();
        if (device == nullptr) {
            return stats;
        }
        const auto rhi_stats = device->stats();
        stats.compute_morph_async_compute_queue_submits = rhi_stats.compute_queue_submits;
        stats.compute_morph_async_graphics_queue_submits = rhi_stats.graphics_queue_submits;
        stats.compute_morph_async_graphics_queue_waits = rhi_stats.queue_waits;
        stats.compute_morph_async_last_compute_submitted_fence_value = rhi_stats.last_compute_submitted_fence_value;
        stats.compute_morph_async_last_graphics_queue_wait_fence_value = rhi_stats.last_graphics_queue_wait_fence_value;
        stats.compute_morph_async_last_graphics_submitted_fence_value = rhi_stats.last_graphics_submitted_fence_value;
        return stats;
    }

    [[nodiscard]] rhi::BufferHandle scene_pbr_frame_uniform_buffer() const noexcept {
        return bindings_.scene_pbr_frame_uniform_buffer;
    }

    [[nodiscard]] const rhi::IRhiDevice* scene_rhi_device() const noexcept {
        if (bindings_.material_bindings.empty()) {
            return nullptr;
        }
        return bindings_.material_bindings.front().binding.owner_device;
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
        if (const auto* skinned = bindings_.skinned_palette.find_skinned_mesh(patched.mesh); skinned != nullptr) {
            patched.gpu_skinning = true;
            patched.skinned_mesh = *skinned;
            patched.mesh_binding = skinned->mesh;
            ++stats_.skinned_mesh_bindings_resolved;
            for (const auto& compute_morph_skinned : bindings_.compute_morph_skinned_mesh_bindings) {
                if (compute_morph_skinned.mesh == patched.mesh) {
                    ++stats_.compute_morph_skinned_mesh_bindings_resolved;
                    ++stats_.compute_morph_skinned_mesh_draws;
                    break;
                }
            }
        } else if (patched.mesh_binding.owner_device == nullptr) {
            if (const auto* binding = bindings_.palette.find_mesh(patched.mesh); binding != nullptr) {
                patched.mesh_binding = *binding;
                ++stats_.mesh_bindings_resolved;
            }
        }
        bool compute_morph_bound = false;
        if (!patched.gpu_skinning) {
            for (const auto& binding : compute_morph_mesh_bindings_) {
                if (binding.mesh != patched.mesh) {
                    continue;
                }
                patched.mesh_binding = binding.mesh_binding;
                compute_morph_bound = true;
                ++stats_.compute_morph_mesh_bindings_resolved;
                ++stats_.compute_morph_mesh_draws;
                break;
            }
        }
        if (!patched.gpu_skinning && !patched.gpu_morphing && !compute_morph_bound) {
            for (const auto& binding : morph_mesh_bindings_) {
                if (binding.mesh != patched.mesh) {
                    continue;
                }
                if (const auto* morph = bindings_.morph_palette.find_morph_mesh(binding.morph_mesh); morph != nullptr) {
                    patched.gpu_morphing = true;
                    patched.morph_mesh = *morph;
                    ++stats_.morph_mesh_bindings_resolved;
                }
                break;
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
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings_;
    std::vector<SceneComputeMorphMeshBinding> compute_morph_mesh_bindings_;
    SdlDesktopPresentationSceneGpuBindingStats stats_;
};

} // namespace mirakana::runtime_host_sdl3_detail
