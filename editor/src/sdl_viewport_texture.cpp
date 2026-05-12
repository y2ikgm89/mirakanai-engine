// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/sdl_viewport_texture.hpp"

#include <SDL3/SDL.h>
#include <cstddef>
#include <cstdint>

#if defined(MK_EDITOR_ENABLE_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include <bit>
#endif
#include <optional>
#include <stdexcept>
#include <string>

namespace mirakana::editor {

namespace {

#if defined(MK_EDITOR_ENABLE_D3D12)
[[nodiscard]] std::string
shared_texture_export_diagnostic(const mirakana::rhi::d3d12::D3d12SharedTextureExportResult& exported) {
    if (exported.device_unavailable) {
        return "Viewport RHI device is not a D3D12 device";
    }
    if (exported.invalid_texture) {
        return "Viewport RHI texture cannot be exported";
    }
    if (exported.texture_not_shareable) {
        return "Viewport RHI texture was not created with shared usage";
    }
    return "D3D12 shared viewport texture export failed";
}
#endif

} // namespace

const char* SdlViewportTexture::display_path_label() const noexcept {
    switch (display_path_) {
    case ViewportDisplayTexturePath::cpu_readback:
        return "CPU readback";
    case ViewportDisplayTexturePath::d3d12_shared_texture:
        return "D3D12 shared texture";
    }
    return "unknown";
}

const char* SdlViewportTexture::display_path_contract_id() const noexcept {
    switch (display_path_) {
    case ViewportDisplayTexturePath::cpu_readback:
        return "cpu-readback";
    case ViewportDisplayTexturePath::d3d12_shared_texture:
        return "d3d12-shared-texture";
    }
    return "unknown";
}

SdlViewportTexture::SdlViewportTexture(SDL_Renderer* renderer, mirakana::Extent2D extent) : renderer_(renderer) {
    if (renderer_ == nullptr) {
        throw std::invalid_argument("viewport display texture requires an SDL renderer");
    }
    resize(extent);
}

SdlViewportTexture::~SdlViewportTexture() {
    destroy();
}

void SdlViewportTexture::resize(mirakana::Extent2D extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("viewport display texture extent must be non-zero");
    }
    if (texture_ != nullptr && extent_.width == extent.width && extent_.height == extent.height) {
        return;
    }

    create_cpu_texture(extent);
}

bool SdlViewportTexture::update_from_frame(const mirakana::RhiViewportReadbackFrame& frame, bool preserve_diagnostic) {
    if (texture_ == nullptr || frame.pixels.empty() || frame.row_pitch == 0) {
        return false;
    }
    if (display_path_ != ViewportDisplayTexturePath::cpu_readback || frame.extent.width != extent_.width ||
        frame.extent.height != extent_.height) {
        create_cpu_texture(frame.extent);
    }

    const bool updated = SDL_UpdateTexture(texture_, nullptr, frame.pixels.data(), static_cast<int>(frame.row_pitch));
    if (updated) {
        display_path_ = ViewportDisplayTexturePath::cpu_readback;
        if (!preserve_diagnostic) {
            diagnostic_.clear();
        }
        ++version_;
    }
    return updated;
}

#if defined(MK_EDITOR_ENABLE_D3D12)
bool SdlViewportTexture::update_from_d3d12_shared_texture(mirakana::rhi::IRhiDevice& device,
                                                          const mirakana::RhiViewportDisplayFrame& frame) {
    const auto pixel_format = sdl_pixel_format_for_rhi_format(frame.format);
    if (!pixel_format.has_value()) {
        diagnostic_ = "D3D12 shared viewport format is unsupported by SDL";
        return false;
    }

    ID3D12Device* sdl_device = renderer_d3d12_device();
    if (sdl_device == nullptr) {
        diagnostic_ = "SDL renderer is not using the D3D12 backend";
        return false;
    }

    if (display_path_ == ViewportDisplayTexturePath::d3d12_shared_texture && texture_ != nullptr &&
        native_source_texture_.value == frame.texture.value && extent_.width == frame.extent.width &&
        extent_.height == frame.extent.height) {
        if (last_native_frame_index_ != frame.frame_index) {
            last_native_frame_index_ = frame.frame_index;
            ++version_;
        }
        diagnostic_.clear();
        return true;
    }

    auto exported = mirakana::rhi::d3d12::export_shared_texture(device, frame.texture);
    if (!exported.succeeded) {
        diagnostic_ = shared_texture_export_diagnostic(exported);
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> opened_resource;
    const HRESULT open_result = sdl_device->OpenSharedHandle(std::bit_cast<HANDLE>(exported.shared_handle.value),
                                                             IID_PPV_ARGS(&opened_resource));
    mirakana::rhi::d3d12::close_shared_texture_handle(exported.shared_handle);
    if (FAILED(open_result) || opened_resource == nullptr) {
        diagnostic_ = "SDL D3D12 device could not open the shared viewport texture";
        return false;
    }

    const SDL_PropertiesID properties = SDL_CreateProperties();
    if (properties == 0) {
        diagnostic_ = std::string("SDL_CreateProperties failed: ") + SDL_GetError();
        return false;
    }

    struct PropertiesGuard {
      private:
        SDL_PropertiesID id_{0};

      public:
        explicit PropertiesGuard(SDL_PropertiesID props) noexcept : id_(props) {}
        PropertiesGuard(const PropertiesGuard&) = delete;
        PropertiesGuard& operator=(const PropertiesGuard&) = delete;
        PropertiesGuard(PropertiesGuard&&) = delete;
        PropertiesGuard& operator=(PropertiesGuard&&) = delete;
        ~PropertiesGuard() {
            if (id_ != 0) {
                SDL_DestroyProperties(id_);
            }
        }
    } properties_guard{properties};

    const bool configured =
        SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, static_cast<Sint64>(*pixel_format)) &&
        SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER,
                              static_cast<Sint64>(SDL_TEXTUREACCESS_STATIC)) &&
        SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER,
                              static_cast<Sint64>(frame.extent.width)) &&
        SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER,
                              static_cast<Sint64>(frame.extent.height)) &&
        SDL_SetPointerProperty(properties, SDL_PROP_TEXTURE_CREATE_D3D12_TEXTURE_POINTER, opened_resource.Get());
    if (!configured) {
        diagnostic_ = std::string("SDL texture property setup failed: ") + SDL_GetError();
        return false;
    }

    SDL_Texture* next_texture = SDL_CreateTextureWithProperties(renderer_, properties);
    if (next_texture == nullptr) {
        diagnostic_ = std::string("SDL_CreateTextureWithProperties failed: ") + SDL_GetError();
        return false;
    }

    destroy();
    texture_ = next_texture;
    native_d3d12_resource_ = opened_resource;
    native_source_texture_ = frame.texture;
    last_native_frame_index_ = frame.frame_index;
    extent_ = frame.extent;
    display_path_ = ViewportDisplayTexturePath::d3d12_shared_texture;
    diagnostic_.clear();
    (void)SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_LINEAR);
    (void)SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
    ++version_;
    return true;
}

ID3D12Device* SdlViewportTexture::renderer_d3d12_device() const noexcept {
    const SDL_PropertiesID properties = SDL_GetRendererProperties(renderer_);
    if (properties == 0) {
        return nullptr;
    }
    return static_cast<ID3D12Device*>(
        SDL_GetPointerProperty(properties, SDL_PROP_RENDERER_D3D12_DEVICE_POINTER, nullptr));
}

std::optional<SDL_PixelFormat>
SdlViewportTexture::sdl_pixel_format_for_rhi_format(mirakana::rhi::Format format) noexcept {
    switch (format) {
    case mirakana::rhi::Format::rgba8_unorm:
        return SDL_PIXELFORMAT_RGBA32;
    case mirakana::rhi::Format::bgra8_unorm:
        return SDL_PIXELFORMAT_BGRA32;
    case mirakana::rhi::Format::unknown:
    case mirakana::rhi::Format::depth24_stencil8:
        return std::nullopt;
    }
    return std::nullopt;
}
#endif

void SdlViewportTexture::create_cpu_texture(mirakana::Extent2D extent) {
    destroy();
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                 static_cast<int>(extent.width), static_cast<int>(extent.height));
    if (texture_ == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
    }

    (void)SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_LINEAR);
    (void)SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
    extent_ = extent;
    pixels_.assign(static_cast<std::size_t>(extent.width) * static_cast<std::size_t>(extent.height) * 4U,
                   std::uint8_t{0});
    display_path_ = ViewportDisplayTexturePath::cpu_readback;
    diagnostic_.clear();
    ++version_;
}

void SdlViewportTexture::destroy() noexcept {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
#if defined(MK_EDITOR_ENABLE_D3D12)
    native_d3d12_resource_.Reset();
    native_source_texture_ = mirakana::rhi::TextureHandle{};
    last_native_frame_index_ = 0;
#endif
}

} // namespace mirakana::editor
