// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_material_preview_cache.hpp"

namespace mirakana::editor {
namespace {

[[nodiscard]] EditorMaterialGpuPreviewExecutionSnapshot
make_diagnostic_execution_snapshot(std::string_view diagnostic) {
    return EditorMaterialGpuPreviewExecutionSnapshot{
        .status = EditorMaterialGpuPreviewStatus::rhi_unavailable,
        .diagnostic = std::string(diagnostic),
        .backend_label = "D3D12",
        .display_path_label = "host-private-native",
        .frames_rendered = 0,
        .executes = false,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] EditorMaterialGpuPreviewExecutionSnapshot make_ready_execution_snapshot() {
    return EditorMaterialGpuPreviewExecutionSnapshot{
        .status = EditorMaterialGpuPreviewStatus::ready,
        .diagnostic = "native material preview private D3D12 texture display is ready",
        .backend_label = "D3D12",
        .display_path_label = "host-private-native",
        .frames_rendered = 0,
        .executes = false,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] bool texture_display_attempted(const NativeMaterialPreviewDisplayDesc& desc) noexcept {
    return desc.texture_display_requested || desc.texture_adapter_available || desc.offscreen_target_available ||
           desc.descriptor_lease_available || desc.resource_barriers_recorded || desc.fence_lifecycle_ready;
}

} // namespace

NativeMaterialPreviewDisplayPlan plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc desc) {
    NativeMaterialPreviewDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
        .shader_artifacts_available = desc.shader_artifacts_available,
        .gpu_payload_available = desc.gpu_payload_available,
        .texture_display_requested = desc.texture_display_requested,
        .texture_adapter_available = desc.texture_adapter_available,
        .offscreen_target_available = desc.offscreen_target_available,
        .descriptor_lease_available = desc.descriptor_lease_available,
        .resource_barriers_recorded = desc.resource_barriers_recorded,
        .fence_lifecycle_ready = desc.fence_lifecycle_ready,
        .texture_display_ready = false,
        .native_texture_handles_exposed = false,
        .native_texture_handle_policy = "private",
        .frame_index = desc.frame_index,
        .backend_id = std::string(desc.backend_id),
        .lifecycle_status = "host_unavailable",
        .diagnostic = {},
        .execution_snapshot = make_diagnostic_execution_snapshot("native material preview host unavailable"),
    };

    if (!desc.d3d12_host_available) {
        plan.status_id = "host_unavailable";
        plan.lifecycle_status = "host_unavailable";
        plan.diagnostic = "native material preview display requires an initialized D3D12 host";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.shader_artifacts_available) {
        plan.status_id = "shader_artifacts_missing";
        plan.lifecycle_status = "shader_pending";
        plan.diagnostic = "native material preview display requires ready D3D12 shader artifacts";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.gpu_payload_available) {
        plan.status_id = "gpu_payload_unavailable";
        plan.lifecycle_status = "payload_pending";
        plan.diagnostic = "GPU payload unavailable; showing diagnostic-only native material preview";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    plan.lifecycle_status = "diagnostic_only";
    if (!texture_display_attempted(desc)) {
        plan.diagnostic =
            "native material preview texture display adapter is private and not bound; showing diagnostic-only preview";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.texture_adapter_available) {
        plan.status_id = "texture_adapter_unavailable";
        plan.lifecycle_status = "adapter_pending";
        plan.diagnostic = "native material preview display requires a private D3D12 texture adapter";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.offscreen_target_available) {
        plan.status_id = "offscreen_target_unavailable";
        plan.lifecycle_status = "target_pending";
        plan.diagnostic = "native material preview display requires a private offscreen D3D12 render target";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.descriptor_lease_available) {
        plan.status_id = "descriptor_lease_unavailable";
        plan.lifecycle_status = "descriptor_pending";
        plan.diagnostic = "native material preview display requires a private D3D12 shader-resource descriptor lease";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.resource_barriers_recorded) {
        plan.status_id = "barrier_lifecycle_missing";
        plan.lifecycle_status = "barrier_pending";
        plan.diagnostic = "native material preview display requires render-target to shader-resource barrier evidence";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.fence_lifecycle_ready) {
        plan.status_id = "fence_lifecycle_missing";
        plan.lifecycle_status = "fence_pending";
        plan.diagnostic = "native material preview display requires fence evidence before descriptor reuse";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    plan.status_id = "d3d12_texture_ready";
    plan.lifecycle_status = "ready";
    plan.texture_display_ready = true;
    plan.diagnostic =
        "native material preview private D3D12 texture display is ready without exposing native texture handles";
    plan.execution_snapshot = make_ready_execution_snapshot();
    return plan;
}

} // namespace mirakana::editor
