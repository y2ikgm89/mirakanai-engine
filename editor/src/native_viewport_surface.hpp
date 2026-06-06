// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/viewport.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana::editor {

struct NativeViewportDisplayDesc {
    bool d3d12_host_available{false};
    bool vulkan_host_available{false};
    bool vulkan_validation_layer_ready{false};
    bool vulkan_spirv_artifacts_available{false};
    bool vulkan_synchronization2_ready{false};
    bool metal_host_available{false};
    bool metal_command_queue_ready{false};
    bool metal_feature_set_ready{false};
    bool metal_shader_library_ready{false};
    bool metal_render_pipeline_ready{false};
    bool metal_texture_render_target_ready{false};
    bool metal_texture_shader_read_ready{false};
    bool metal_sampler_state_ready{false};
    bool metal_render_pass_ready{false};
    bool metal_drawable_present_ready{false};
    bool metal_command_buffer_completed{false};
    bool renderer_output_available{false};
    bool texture_display_requested{false};
    bool texture_adapter_available{false};
    bool offscreen_target_available{false};
    bool descriptor_lease_available{false};
    bool resource_barriers_recorded{false};
    bool fence_lifecycle_ready{false};
    bool resize_safe_teardown_completed{true};
    bool resize_recreate_required{false};
    bool visible_panel_available{true};
    bool visible_texture_composite_recorded{false};
    std::uint64_t visible_texture_composites{0};
    ViewportExtent extent{.width = 1280, .height = 720};
    std::uint64_t frame_index{0};
    std::string_view backend_id{"d3d12"};
};

struct NativeViewportDisplayPlan {
    bool accepted{false};
    std::string status_id{"host_unavailable"};
    bool d3d12_host_available{false};
    bool vulkan_host_available{false};
    bool vulkan_validation_layer_ready{false};
    bool vulkan_spirv_artifacts_available{false};
    bool vulkan_synchronization2_ready{false};
    bool metal_host_available{false};
    bool metal_command_queue_ready{false};
    bool metal_feature_set_ready{false};
    bool metal_shader_library_ready{false};
    bool metal_render_pipeline_ready{false};
    bool metal_texture_render_target_ready{false};
    bool metal_texture_shader_read_ready{false};
    bool metal_sampler_state_ready{false};
    bool metal_render_pass_ready{false};
    bool metal_drawable_present_ready{false};
    bool metal_command_buffer_completed{false};
    bool renderer_output_available{false};
    bool texture_display_requested{false};
    bool texture_adapter_available{false};
    bool offscreen_target_available{false};
    bool descriptor_lease_available{false};
    bool resource_barriers_recorded{false};
    bool fence_lifecycle_ready{false};
    bool resize_safe_teardown_completed{true};
    bool resize_recreate_required{false};
    bool visible_panel_available{true};
    bool visible_texture_composite_recorded{false};
    std::uint64_t visible_texture_composites{0};
    bool texture_display_ready{false};
    bool native_texture_handles_exposed{false};
    std::string native_texture_handle_policy{"private"};
    ViewportExtent extent{};
    std::uint64_t frame_index{0};
    std::string backend_id{"d3d12"};
    std::string lifecycle_status{"host_unavailable"};
    std::string diagnostic;
};

[[nodiscard]] NativeViewportDisplayPlan plan_native_viewport_display(NativeViewportDisplayDesc desc);

} // namespace mirakana::editor
