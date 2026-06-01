// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_viewport_surface.hpp"

namespace mirakana::editor {
namespace {

[[nodiscard]] bool texture_display_attempted(const NativeViewportDisplayDesc& desc) noexcept {
    return desc.texture_display_requested || desc.texture_adapter_available || desc.offscreen_target_available ||
           desc.descriptor_lease_available || desc.resource_barriers_recorded || desc.fence_lifecycle_ready ||
           desc.resize_recreate_required || !desc.resize_safe_teardown_completed;
}

} // namespace

NativeViewportDisplayPlan plan_native_viewport_display(NativeViewportDisplayDesc desc) {
    NativeViewportDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
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

    if (!desc.d3d12_host_available) {
        plan.status_id = "host_unavailable";
        plan.lifecycle_status = "host_unavailable";
        plan.diagnostic = "native viewport display requires an initialized D3D12 host";
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    plan.lifecycle_status = "diagnostic_only";
    if (!desc.renderer_output_available) {
        plan.diagnostic = "renderer output unavailable; showing diagnostic-only native viewport";
        return plan;
    }

    if (!texture_display_attempted(desc)) {
        plan.diagnostic =
            "native D3D12 texture display adapter is private and not bound; showing diagnostic-only viewport";
        return plan;
    }

    if (!desc.texture_adapter_available) {
        plan.status_id = "texture_adapter_unavailable";
        plan.lifecycle_status = "adapter_pending";
        plan.diagnostic = "native viewport display requires a private D3D12 texture adapter";
        return plan;
    }

    if (!desc.offscreen_target_available) {
        plan.status_id = "offscreen_target_unavailable";
        plan.lifecycle_status = "target_pending";
        plan.diagnostic = "native viewport display requires a private offscreen D3D12 render target";
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
        plan.diagnostic = "native viewport display requires a private D3D12 shader-resource descriptor lease";
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
        plan.diagnostic =
            "native viewport display requires a visible compositor pass sampling the private D3D12 texture";
        return plan;
    }

    plan.status_id = "d3d12_texture_ready";
    plan.lifecycle_status = "ready";
    plan.texture_display_ready = true;
    plan.diagnostic = "native viewport private D3D12 texture display is ready without exposing native texture handles";
    return plan;
}

} // namespace mirakana::editor
