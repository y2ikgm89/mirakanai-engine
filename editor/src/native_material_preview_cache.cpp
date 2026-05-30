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

} // namespace

NativeMaterialPreviewDisplayPlan plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc desc) {
    NativeMaterialPreviewDisplayPlan plan{
        .accepted = false,
        .status_id = "host_unavailable",
        .d3d12_host_available = desc.d3d12_host_available,
        .shader_artifacts_available = desc.shader_artifacts_available,
        .gpu_payload_available = desc.gpu_payload_available,
        .texture_display_ready = false,
        .native_texture_handles_exposed = false,
        .native_texture_handle_policy = "private",
        .frame_index = desc.frame_index,
        .backend_id = std::string(desc.backend_id),
        .diagnostic = {},
        .execution_snapshot = make_diagnostic_execution_snapshot("native material preview host unavailable"),
    };

    if (!desc.d3d12_host_available) {
        plan.status_id = "host_unavailable";
        plan.diagnostic = "native material preview display requires an initialized D3D12 host";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.shader_artifacts_available) {
        plan.status_id = "shader_artifacts_missing";
        plan.diagnostic = "native material preview display requires ready D3D12 shader artifacts";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    if (!desc.gpu_payload_available) {
        plan.status_id = "gpu_payload_unavailable";
        plan.diagnostic = "GPU payload unavailable; showing diagnostic-only native material preview";
        plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
        return plan;
    }

    plan.accepted = true;
    plan.status_id = "diagnostic_only";
    plan.diagnostic =
        "native material preview texture display adapter is private and not bound; showing diagnostic-only preview";
    plan.execution_snapshot = make_diagnostic_execution_snapshot(plan.diagnostic);
    return plan;
}

} // namespace mirakana::editor
