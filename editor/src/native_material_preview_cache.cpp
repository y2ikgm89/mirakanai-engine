// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_material_preview_cache.hpp"

namespace mirakana::editor {
namespace {

[[nodiscard]] EditorMaterialGpuPreviewExecutionSnapshot
make_diagnostic_execution_snapshot(std::string_view backend_label, std::string_view diagnostic) {
    return EditorMaterialGpuPreviewExecutionSnapshot{
        .status = EditorMaterialGpuPreviewStatus::rhi_unavailable,
        .diagnostic = std::string(diagnostic),
        .backend_label = std::string(backend_label),
        .display_path_label = "host-private-native",
        .frames_rendered = 0,
        .executes = false,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] EditorMaterialGpuPreviewExecutionSnapshot
make_ready_execution_snapshot(std::uint64_t frames_rendered, bool executes, std::string_view backend_label) {
    return EditorMaterialGpuPreviewExecutionSnapshot{
        .status = EditorMaterialGpuPreviewStatus::ready,
        .diagnostic = "native material preview private " + std::string{backend_label} + " texture display is ready",
        .backend_label = std::string{backend_label},
        .display_path_label = "host-private-native",
        .frames_rendered = frames_rendered,
        .executes = executes,
        .exposes_native_handles = false,
    };
}

[[nodiscard]] bool texture_display_attempted(const NativeMaterialPreviewDisplayDesc& desc) noexcept {
    return desc.texture_display_requested || desc.texture_adapter_available || desc.offscreen_target_available ||
           desc.descriptor_lease_available || desc.resource_barriers_recorded || desc.fence_lifecycle_ready;
}

[[nodiscard]] bool vulkan_backend(std::string_view backend_id) noexcept {
    return backend_id == "vulkan";
}

[[nodiscard]] bool backend_host_available(const NativeMaterialPreviewDisplayDesc& desc) noexcept {
    return vulkan_backend(desc.backend_id) ? desc.vulkan_host_available : desc.d3d12_host_available;
}

[[nodiscard]] std::string_view backend_display_name(std::string_view backend_id) noexcept {
    return vulkan_backend(backend_id) ? "Vulkan" : "D3D12";
}

[[nodiscard]] std::string_view ready_status_id(std::string_view backend_id) noexcept {
    return vulkan_backend(backend_id) ? "vulkan_texture_ready" : "d3d12_texture_ready";
}

} // namespace

NativeMaterialPreviewDisplayPlan plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc desc) {
    const bool is_vulkan = vulkan_backend(desc.backend_id);
    const auto backend_name = backend_display_name(desc.backend_id);
    NativeMaterialPreviewDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
        .vulkan_host_available = desc.vulkan_host_available,
        .vulkan_validation_layer_ready = desc.vulkan_validation_layer_ready,
        .vulkan_spirv_artifacts_available = desc.vulkan_spirv_artifacts_available,
        .vulkan_synchronization2_ready = desc.vulkan_synchronization2_ready,
        .shader_artifacts_available = desc.shader_artifacts_available,
        .gpu_payload_available = desc.gpu_payload_available,
        .texture_display_requested = desc.texture_display_requested,
        .texture_adapter_available = desc.texture_adapter_available,
        .offscreen_target_available = desc.offscreen_target_available,
        .descriptor_lease_available = desc.descriptor_lease_available,
        .resource_barriers_recorded = desc.resource_barriers_recorded,
        .fence_lifecycle_ready = desc.fence_lifecycle_ready,
        .visible_panel_available = desc.visible_panel_available,
        .visible_texture_composite_recorded = desc.visible_texture_composite_recorded,
        .visible_texture_composites = desc.visible_texture_composites,
        .texture_display_ready = false,
        .native_texture_handles_exposed = false,
        .native_texture_handle_policy = "private",
        .frame_index = desc.frame_index,
        .backend_id = std::string(desc.backend_id),
        .lifecycle_status = "host_unavailable",
        .diagnostic = {},
        .execution_snapshot =
            make_diagnostic_execution_snapshot(backend_name, "native material preview host unavailable"),
    };

    if (!backend_host_available(desc)) {
        plan.status_id = "host_unavailable";
        plan.lifecycle_status = "host_unavailable";
        plan.diagnostic =
            "native material preview display requires an initialized " + std::string{backend_name} + " host";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (is_vulkan && !desc.vulkan_validation_layer_ready) {
        plan.status_id = "vulkan_validation_layer_unavailable";
        plan.lifecycle_status = "validation_pending";
        plan.diagnostic = "native material preview Vulkan display requires VK_LAYER_KHRONOS_validation evidence";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (is_vulkan && !desc.vulkan_spirv_artifacts_available) {
        plan.status_id = "vulkan_spirv_artifacts_missing";
        plan.lifecycle_status = "shader_pending";
        plan.diagnostic = "native material preview Vulkan display requires validated SPIR-V shader artifacts";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (is_vulkan && !desc.vulkan_synchronization2_ready) {
        plan.status_id = "vulkan_synchronization2_unavailable";
        plan.lifecycle_status = "barrier_pending";
        plan.diagnostic = "native material preview Vulkan display requires synchronization2 barrier evidence";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.shader_artifacts_available) {
        plan.status_id = "shader_artifacts_missing";
        plan.lifecycle_status = "shader_pending";
        plan.diagnostic =
            "native material preview display requires ready " + std::string{backend_name} + " shader artifacts";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.gpu_payload_available) {
        plan.status_id = "gpu_payload_unavailable";
        plan.lifecycle_status = "payload_pending";
        plan.diagnostic = "GPU payload unavailable; showing diagnostic-only native material preview";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    plan.lifecycle_status = "diagnostic_only";
    if (!texture_display_attempted(desc)) {
        plan.diagnostic =
            "native material preview texture display adapter is private and not bound; showing diagnostic-only preview";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.texture_adapter_available) {
        plan.status_id = "texture_adapter_unavailable";
        plan.lifecycle_status = "adapter_pending";
        plan.diagnostic =
            "native material preview display requires a private " + std::string{backend_name} + " texture adapter";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.offscreen_target_available) {
        plan.status_id = "offscreen_target_unavailable";
        plan.lifecycle_status = "target_pending";
        plan.diagnostic = "native material preview display requires a private offscreen " + std::string{backend_name} +
                          " render target";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.descriptor_lease_available) {
        plan.status_id = "descriptor_lease_unavailable";
        plan.lifecycle_status = "descriptor_pending";
        plan.diagnostic = "native material preview display requires a private " + std::string{backend_name} +
                          " shader-resource descriptor lease";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.resource_barriers_recorded) {
        plan.status_id = "barrier_lifecycle_missing";
        plan.lifecycle_status = "barrier_pending";
        plan.diagnostic = "native material preview display requires render-target to shader-resource barrier evidence";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.fence_lifecycle_ready) {
        plan.status_id = "fence_lifecycle_missing";
        plan.lifecycle_status = "fence_pending";
        plan.diagnostic = "native material preview display requires fence evidence before descriptor reuse";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.visible_panel_available) {
        plan.status_id = "visible_panel_unavailable";
        plan.lifecycle_status = "panel_pending";
        plan.diagnostic = "native material preview display requires a visible material preview panel";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    if (!desc.visible_texture_composite_recorded || desc.visible_texture_composites == 0U) {
        plan.status_id = "visible_composite_pending";
        plan.lifecycle_status = "presentation_pending";
        plan.diagnostic = "native material preview display requires a visible compositor pass sampling the private " +
                          std::string{backend_name} + " texture";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(backend_name, plan.diagnostic);
        return plan;
    }

    plan.status_id = std::string{ready_status_id(desc.backend_id)};
    plan.lifecycle_status = "ready";
    plan.texture_display_ready = true;
    plan.diagnostic = "native material preview private " + std::string{backend_name} +
                      " texture display is ready without exposing native texture handles";
    plan.execution_snapshot = make_ready_execution_snapshot(desc.frames_rendered, desc.executes, backend_name);
    return plan;
}

} // namespace mirakana::editor
