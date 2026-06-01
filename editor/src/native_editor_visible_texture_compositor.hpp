// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_texture_display_adapter.hpp"

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace mirakana::editor {

struct NativeEditorVisibleTextureCompositorDesc {
    rhi::IRhiDevice* device{nullptr};
    rhi::SwapchainHandle swapchain;
    ViewportExtent extent{.width = 1280, .height = 720};
    std::string_view vertex_shader_entry_point{"vs_main"};
    std::span<const std::uint8_t> vertex_shader_bytecode;
    std::string_view fragment_shader_entry_point{"ps_main"};
    std::span<const std::uint8_t> fragment_shader_bytecode;
    std::string_view backend_id{"d3d12"};
};

struct NativeEditorVisibleTextureCompositorEvidence {
    bool compositor_available{false};
    bool visible_panel_available{false};
    bool swapchain_frame_acquired{false};
    bool sampled_texture_descriptor_bound{false};
    bool render_pass_recorded{false};
    bool draw_recorded{false};
    bool present_recorded{false};
    bool fence_waited{false};
    bool native_texture_handles_exposed{false};
    std::uint64_t visible_texture_composites{0};
    std::uint64_t descriptor_writes{0};
    std::uint64_t descriptor_sets_bound{0};
    std::uint64_t render_passes{0};
    std::uint64_t draw_calls{0};
    std::uint64_t present_calls{0};
    std::uint64_t fence_waits{0};
    std::string diagnostic;
};

class NativeEditorVisibleTextureCompositor final {
  public:
    explicit NativeEditorVisibleTextureCompositor(NativeEditorVisibleTextureCompositorDesc desc);
    ~NativeEditorVisibleTextureCompositor();

    NativeEditorVisibleTextureCompositor(const NativeEditorVisibleTextureCompositor&) = delete;
    NativeEditorVisibleTextureCompositor& operator=(const NativeEditorVisibleTextureCompositor&) = delete;
    NativeEditorVisibleTextureCompositor(NativeEditorVisibleTextureCompositor&&) noexcept;
    NativeEditorVisibleTextureCompositor& operator=(NativeEditorVisibleTextureCompositor&&) noexcept;

    void resize(ViewportExtent extent);
    [[nodiscard]] NativeViewportDisplayPlan render_viewport_frame(NativeTextureDisplayAdapter& adapter,
                                                                  std::uint64_t frame_index);
    [[nodiscard]] NativeMaterialPreviewDisplayPlan render_material_preview_frame(NativeTextureDisplayAdapter& adapter,
                                                                                 std::uint64_t frame_index);
    [[nodiscard]] const NativeEditorVisibleTextureCompositorEvidence& evidence() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
