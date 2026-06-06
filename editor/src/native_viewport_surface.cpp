// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_viewport_surface.hpp"

namespace mirakana::editor {
namespace {

enum class NativeTextureDisplayBackend : std::uint8_t {
    d3d12,
    vulkan,
    metal,
    unsupported,
};

[[nodiscard]] bool texture_display_attempted(const NativeViewportDisplayDesc& desc) noexcept {
    return desc.texture_display_requested || desc.texture_adapter_available || desc.offscreen_target_available ||
           desc.descriptor_lease_available || desc.resource_barriers_recorded || desc.fence_lifecycle_ready ||
           desc.resize_recreate_required || !desc.resize_safe_teardown_completed;
}

[[nodiscard]] NativeTextureDisplayBackend native_texture_display_backend(std::string_view backend_id) noexcept {
    if (backend_id == "d3d12") {
        return NativeTextureDisplayBackend::d3d12;
    }
    if (backend_id == "vulkan") {
        return NativeTextureDisplayBackend::vulkan;
    }
    if (backend_id == "metal") {
        return NativeTextureDisplayBackend::metal;
    }
    return NativeTextureDisplayBackend::unsupported;
}

[[nodiscard]] bool backend_host_available(const NativeViewportDisplayDesc& desc) noexcept {
    switch (native_texture_display_backend(desc.backend_id)) {
    case NativeTextureDisplayBackend::d3d12:
        return desc.d3d12_host_available;
    case NativeTextureDisplayBackend::vulkan:
        return desc.vulkan_host_available;
    case NativeTextureDisplayBackend::metal:
        return desc.metal_host_available;
    case NativeTextureDisplayBackend::unsupported:
        break;
    }
    return false;
}

[[nodiscard]] std::string_view backend_display_name(std::string_view backend_id) noexcept {
    switch (native_texture_display_backend(backend_id)) {
    case NativeTextureDisplayBackend::d3d12:
        return "D3D12";
    case NativeTextureDisplayBackend::vulkan:
        return "Vulkan";
    case NativeTextureDisplayBackend::metal:
        return "Metal";
    case NativeTextureDisplayBackend::unsupported:
        break;
    }
    return "unsupported backend";
}

[[nodiscard]] std::string_view ready_status_id(std::string_view backend_id) noexcept {
    switch (native_texture_display_backend(backend_id)) {
    case NativeTextureDisplayBackend::d3d12:
        return "d3d12_texture_ready";
    case NativeTextureDisplayBackend::vulkan:
        return "vulkan_texture_ready";
    case NativeTextureDisplayBackend::metal:
        return "metal_texture_ready";
    case NativeTextureDisplayBackend::unsupported:
        break;
    }
    return "unsupported_backend";
}

} // namespace

NativeViewportDisplayPlan plan_native_viewport_display(NativeViewportDisplayDesc desc) {
    NativeViewportDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
        .vulkan_host_available = desc.vulkan_host_available,
        .vulkan_validation_layer_ready = desc.vulkan_validation_layer_ready,
        .vulkan_spirv_artifacts_available = desc.vulkan_spirv_artifacts_available,
        .vulkan_synchronization2_ready = desc.vulkan_synchronization2_ready,
        .metal_host_available = desc.metal_host_available,
        .metal_command_queue_ready = desc.metal_command_queue_ready,
        .metal_feature_set_ready = desc.metal_feature_set_ready,
        .metal_shader_library_ready = desc.metal_shader_library_ready,
        .metal_render_pipeline_ready = desc.metal_render_pipeline_ready,
        .metal_texture_render_target_ready = desc.metal_texture_render_target_ready,
        .metal_texture_shader_read_ready = desc.metal_texture_shader_read_ready,
        .metal_sampler_state_ready = desc.metal_sampler_state_ready,
        .metal_render_pass_ready = desc.metal_render_pass_ready,
        .metal_drawable_present_ready = desc.metal_drawable_present_ready,
        .metal_command_buffer_completed = desc.metal_command_buffer_completed,
        .renderer_output_available = desc.renderer_output_available,
        .texture_display_requested = desc.texture_display_requested,
        .texture_adapter_available = desc.texture_adapter_available,
        .offscreen_target_available = desc.offscreen_target_available,
        .descriptor_lease_available = desc.descriptor_lease_available,
        .resource_barriers_recorded = desc.resource_barriers_recorded,
        .fence_lifecycle_ready = desc.fence_lifecycle_ready,
        .resize_safe_teardown_completed = desc.resize_safe_teardown_completed,
        .resize_recreate_required = desc.resize_recreate_required,
        .visible_panel_available = desc.visible_panel_available,
        .visible_texture_composite_recorded = desc.visible_texture_composite_recorded,
        .visible_texture_composites = desc.visible_texture_composites,
        .texture_display_ready = false,
        .native_texture_handles_exposed = false,
        .native_texture_handle_policy = "private",
        .extent = desc.extent,
        .frame_index = desc.frame_index,
        .backend_id = std::string(desc.backend_id),
        .lifecycle_status = "host_unavailable",
        .diagnostic = {},
    };

    if (desc.extent.width == 0 || desc.extent.height == 0) {
        plan.status_id = "invalid_extent";
        plan.lifecycle_status = "invalid_extent";
        plan.diagnostic = "native viewport display requires a non-zero extent";
        return plan;
    }

    const auto backend = native_texture_display_backend(desc.backend_id);
    if (backend == NativeTextureDisplayBackend::unsupported) {
        plan.status_id = "unsupported_backend";
        plan.lifecycle_status = "host_unavailable";
        plan.diagnostic = "native viewport display requires a supported private backend";
        return plan;
    }

    const bool is_vulkan = backend == NativeTextureDisplayBackend::vulkan;
    const bool is_metal = backend == NativeTextureDisplayBackend::metal;
    const auto backend_name = backend_display_name(desc.backend_id);
    if (!backend_host_available(desc)) {
        plan.status_id = "host_unavailable";
        plan.lifecycle_status = "host_unavailable";
        plan.diagnostic = "native viewport display requires an initialized " + std::string{backend_name} + " host";
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    plan.lifecycle_status = "diagnostic_only";

    if (is_vulkan && !desc.vulkan_validation_layer_ready) {
        plan.status_id = "vulkan_validation_layer_unavailable";
        plan.lifecycle_status = "validation_pending";
        plan.diagnostic = "native viewport Vulkan display requires VK_LAYER_KHRONOS_validation evidence";
        return plan;
    }

    if (is_vulkan && !desc.vulkan_spirv_artifacts_available) {
        plan.status_id = "vulkan_spirv_artifacts_missing";
        plan.lifecycle_status = "shader_pending";
        plan.diagnostic = "native viewport Vulkan display requires validated SPIR-V shader artifacts";
        return plan;
    }

    if (is_vulkan && !desc.vulkan_synchronization2_ready) {
        plan.status_id = "vulkan_synchronization2_unavailable";
        plan.lifecycle_status = "barrier_pending";
        plan.diagnostic = "native viewport Vulkan display requires synchronization2 barrier evidence";
        return plan;
    }

    if (is_metal && !desc.metal_command_queue_ready) {
        plan.status_id = "metal_command_queue_unavailable";
        plan.lifecycle_status = "queue_pending";
        plan.diagnostic = "native viewport Metal display requires command queue evidence";
        return plan;
    }

    if (is_metal && !desc.metal_feature_set_ready) {
        plan.status_id = "metal_feature_set_unavailable";
        plan.lifecycle_status = "feature_set_pending";
        plan.diagnostic = "native viewport Metal display requires Apple feature family evidence";
        return plan;
    }

    if (is_metal && !desc.metal_shader_library_ready) {
        plan.status_id = "metal_shader_library_missing";
        plan.lifecycle_status = "shader_pending";
        plan.diagnostic = "native viewport Metal display requires a non-empty metallib shader library";
        return plan;
    }

    if (is_metal && !desc.metal_render_pipeline_ready) {
        plan.status_id = "metal_render_pipeline_unavailable";
        plan.lifecycle_status = "pipeline_pending";
        plan.diagnostic = "native viewport Metal display requires render pipeline evidence";
        return plan;
    }

    if (is_metal && !desc.metal_texture_render_target_ready) {
        plan.status_id = "metal_texture_render_target_unavailable";
        plan.lifecycle_status = "target_pending";
        plan.diagnostic = "native viewport Metal display requires a render-target texture";
        return plan;
    }

    if (is_metal && !desc.metal_texture_shader_read_ready) {
        plan.status_id = "metal_shader_read_sampling_unavailable";
        plan.lifecycle_status = "sampling_pending";
        plan.diagnostic = "native viewport Metal display requires shader-read texture sampling evidence";
        return plan;
    }

    if (is_metal && !desc.metal_sampler_state_ready) {
        plan.status_id = "metal_sampler_state_unavailable";
        plan.lifecycle_status = "sampler_pending";
        plan.diagnostic = "native viewport Metal display requires sampler state evidence";
        return plan;
    }

    if (is_metal && !desc.metal_render_pass_ready) {
        plan.status_id = "metal_render_pass_unavailable";
        plan.lifecycle_status = "render_pass_pending";
        plan.diagnostic = "native viewport Metal display requires render pass evidence";
        return plan;
    }

    if (is_metal && !desc.metal_drawable_present_ready) {
        plan.status_id = "metal_drawable_present_unavailable";
        plan.lifecycle_status = "present_pending";
        plan.diagnostic = "native viewport Metal display requires drawable present evidence";
        return plan;
    }

    if (is_metal && !desc.metal_command_buffer_completed) {
        plan.status_id = "metal_command_buffer_incomplete";
        plan.lifecycle_status = "completion_pending";
        plan.diagnostic = "native viewport Metal display requires completed command-buffer evidence";
        return plan;
    }

    if (!desc.renderer_output_available) {
        plan.diagnostic = "renderer output unavailable; showing diagnostic-only native viewport";
        return plan;
    }

    if (!texture_display_attempted(desc)) {
        plan.diagnostic = "native " + std::string{backend_name} +
                          " texture display adapter is private and not bound; showing diagnostic-only viewport";
        return plan;
    }

    if (!desc.texture_adapter_available) {
        plan.status_id = "texture_adapter_unavailable";
        plan.lifecycle_status = "adapter_pending";
        plan.diagnostic =
            "native viewport display requires a private " + std::string{backend_name} + " texture adapter";
        return plan;
    }

    if (!desc.offscreen_target_available) {
        plan.status_id = "offscreen_target_unavailable";
        plan.lifecycle_status = "target_pending";
        plan.diagnostic =
            "native viewport display requires a private offscreen " + std::string{backend_name} + " render target";
        return plan;
    }

    if (desc.resize_recreate_required && !desc.resize_safe_teardown_completed) {
        plan.status_id = "resize_recreate_required";
        plan.lifecycle_status = "resize_pending";
        plan.diagnostic =
            "native viewport display requires resize-safe teardown before binding the replacement texture";
        return plan;
    }

    if (!desc.descriptor_lease_available) {
        plan.status_id = "descriptor_lease_unavailable";
        plan.lifecycle_status = "descriptor_pending";
        plan.diagnostic = "native viewport display requires a private " + std::string{backend_name} +
                          " shader-resource descriptor lease";
        return plan;
    }

    if (!desc.resource_barriers_recorded) {
        plan.status_id = "barrier_lifecycle_missing";
        plan.lifecycle_status = "barrier_pending";
        plan.diagnostic = "native viewport display requires render-target to shader-resource barrier evidence";
        return plan;
    }

    if (!desc.fence_lifecycle_ready) {
        plan.status_id = "fence_lifecycle_missing";
        plan.lifecycle_status = "fence_pending";
        plan.diagnostic = "native viewport display requires fence evidence before descriptor reuse";
        return plan;
    }

    if (!desc.visible_panel_available) {
        plan.status_id = "visible_panel_unavailable";
        plan.lifecycle_status = "panel_pending";
        plan.diagnostic = "native viewport display requires a visible editor viewport panel";
        return plan;
    }

    if (!desc.visible_texture_composite_recorded || desc.visible_texture_composites == 0U) {
        plan.status_id = "visible_composite_pending";
        plan.lifecycle_status = "presentation_pending";
        plan.diagnostic = "native viewport display requires a visible compositor pass sampling the private " +
                          std::string{backend_name} + " texture";
        return plan;
    }

    plan.status_id = std::string{ready_status_id(desc.backend_id)};
    plan.lifecycle_status = "ready";
    plan.texture_display_ready = true;
    plan.diagnostic = "native viewport private " + std::string{backend_name} +
                      " texture display is ready without exposing native texture handles";
    return plan;
}

} // namespace mirakana::editor
