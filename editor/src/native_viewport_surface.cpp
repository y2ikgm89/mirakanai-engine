// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_viewport_surface.hpp"

namespace mirakana::editor {

NativeViewportDisplayPlan plan_native_viewport_display(NativeViewportDisplayDesc desc) {
    NativeViewportDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
        .renderer_output_available = desc.renderer_output_available,
        .texture_display_ready = false,
        .native_texture_handles_exposed = false,
        .native_texture_handle_policy = "private",
        .extent = desc.extent,
        .frame_index = desc.frame_index,
        .backend_id = std::string(desc.backend_id),
        .diagnostic = {},
    };

    if (desc.extent.width == 0 || desc.extent.height == 0) {
        plan.status_id = "invalid_extent";
        plan.diagnostic = "native viewport display requires a non-zero extent";
        return plan;
    }

    if (!desc.d3d12_host_available) {
        plan.status_id = "host_unavailable";
        plan.diagnostic = "native viewport display requires an initialized D3D12 host";
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    if (!desc.renderer_output_available) {
        plan.diagnostic = "renderer output unavailable; showing diagnostic-only native viewport";
        return plan;
    }

    plan.diagnostic = "native D3D12 texture display adapter is private and not bound; showing diagnostic-only viewport";
    return plan;
}

} // namespace mirakana::editor
