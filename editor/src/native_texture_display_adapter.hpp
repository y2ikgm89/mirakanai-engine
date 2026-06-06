// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_material_preview_cache.hpp"
#include "native_viewport_surface.hpp"

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace mirakana::rhi {
class IRhiDevice;
}

namespace mirakana::editor {

struct NativeTextureDisplayAdapterDesc {
    rhi::IRhiDevice* device{nullptr};
    ViewportExtent extent{.width = 1280, .height = 720};
    bool d3d12_host_available{false};
    bool vulkan_host_available{false};
    bool vulkan_validation_layer_ready{false};
    bool vulkan_spirv_artifacts_available{false};
    bool vulkan_synchronization2_ready{false};
    bool renderer_output_available{true};
    bool shader_artifacts_available{true};
    bool gpu_payload_available{true};
    std::string_view backend_id{"d3d12"};
};

struct NativeTextureDisplayAdapterEvidence {
    bool texture_adapter_available{false};
    bool offscreen_target_available{false};
    bool descriptor_lease_available{false};
    bool resource_barriers_recorded{false};
    bool fence_lifecycle_ready{false};
    bool resize_safe_teardown_completed{true};
    bool resize_recreate_required{false};
    bool native_texture_handles_exposed{false};
    ViewportExtent extent{};
    std::uint64_t frame_index{0};
    std::uint64_t frames_rendered{0};
    std::uint64_t descriptor_writes{0};
    std::uint64_t resource_transitions{0};
    std::uint64_t fence_waits{0};
    std::string diagnostic;
};

struct NativeTextureDisplayFrame {
    bool available{false};
    rhi::TextureHandle texture;
    ViewportExtent extent{};
    rhi::Format format{rhi::Format::unknown};
    std::uint64_t frame_index{0};
    std::uint64_t frames_rendered{0};
};

class NativeTextureDisplayAdapter final {
  public:
    explicit NativeTextureDisplayAdapter(NativeTextureDisplayAdapterDesc desc);
    ~NativeTextureDisplayAdapter();

    NativeTextureDisplayAdapter(const NativeTextureDisplayAdapter&) = delete;
    NativeTextureDisplayAdapter& operator=(const NativeTextureDisplayAdapter&) = delete;
    NativeTextureDisplayAdapter(NativeTextureDisplayAdapter&&) noexcept;
    NativeTextureDisplayAdapter& operator=(NativeTextureDisplayAdapter&&) noexcept;

    void resize(ViewportExtent extent);
    [[nodiscard]] NativeViewportDisplayPlan render_viewport_frame(std::uint64_t frame_index);
    [[nodiscard]] NativeMaterialPreviewDisplayPlan render_material_preview_frame(std::uint64_t frame_index);
    [[nodiscard]] const NativeTextureDisplayAdapterEvidence& evidence() const noexcept;
    [[nodiscard]] NativeTextureDisplayFrame display_frame() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
