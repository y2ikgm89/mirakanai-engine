// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#if defined(MK_EDITOR_ENABLE_D3D12)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d12.h>
#include <wrl/client.h>
#endif

namespace mirakana::editor {

enum class ViewportDisplayTexturePath : std::uint8_t { cpu_readback = 0, d3d12_shared_texture };

/// SDL3 texture bridge for viewport / material GPU preview display (Dear ImGui shell only).
class SdlViewportTexture {
  public:
    SdlViewportTexture(SDL_Renderer* renderer, mirakana::Extent2D extent);
    ~SdlViewportTexture();

    SdlViewportTexture(const SdlViewportTexture&) = delete;
    SdlViewportTexture& operator=(const SdlViewportTexture&) = delete;
    SdlViewportTexture(SdlViewportTexture&&) noexcept = delete;
    SdlViewportTexture& operator=(SdlViewportTexture&&) noexcept = delete;

    [[nodiscard]] mirakana::Extent2D extent() const noexcept {
        return extent_;
    }

    [[nodiscard]] std::uint64_t version() const noexcept {
        return version_;
    }

    [[nodiscard]] const char* display_path_label() const noexcept;

    /// Stable token for `EditorMaterialGpuPreviewExecutionSnapshot` / retained
    /// `material_asset_preview.gpu.execution` rows (matches editor_core_tests contract vocabulary).
    [[nodiscard]] const char* display_path_contract_id() const noexcept;

    [[nodiscard]] const char* diagnostic() const noexcept {
        return diagnostic_.c_str();
    }

    [[nodiscard]] ImTextureID imgui_texture_id() const noexcept {
        return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(texture_));
    }

    void resize(mirakana::Extent2D extent);

    [[nodiscard]] bool update_from_frame(const mirakana::RhiViewportReadbackFrame& frame,
                                         bool preserve_diagnostic = false);

#if defined(MK_EDITOR_ENABLE_D3D12)
    [[nodiscard]] bool update_from_d3d12_shared_texture(mirakana::rhi::IRhiDevice& device,
                                                        const mirakana::RhiViewportDisplayFrame& frame);
#endif

  private:
    void create_cpu_texture(mirakana::Extent2D extent);
    void destroy() noexcept;

#if defined(MK_EDITOR_ENABLE_D3D12)
    [[nodiscard]] ID3D12Device* renderer_d3d12_device() const noexcept;

    [[nodiscard]] static std::optional<SDL_PixelFormat>
    sdl_pixel_format_for_rhi_format(mirakana::rhi::Format format) noexcept;
#endif

    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
    mirakana::Extent2D extent_{.width = 1, .height = 1};
    std::vector<std::uint8_t> pixels_;
    std::uint64_t version_{0};
    ViewportDisplayTexturePath display_path_{ViewportDisplayTexturePath::cpu_readback};
    std::string diagnostic_;
#if defined(MK_EDITOR_ENABLE_D3D12)
    Microsoft::WRL::ComPtr<ID3D12Resource> native_d3d12_resource_;
    mirakana::rhi::TextureHandle native_source_texture_{};
    std::uint64_t last_native_frame_index_{0};
#endif
};

} // namespace mirakana::editor
