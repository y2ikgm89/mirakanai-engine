// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/material_asset_preview_panel.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana::editor {

struct NativeMaterialPreviewDisplayDesc {
    bool d3d12_host_available{false};
    bool shader_artifacts_available{false};
    bool gpu_payload_available{false};
    bool texture_display_requested{false};
    bool texture_adapter_available{false};
    bool offscreen_target_available{false};
    bool descriptor_lease_available{false};
    bool resource_barriers_recorded{false};
    bool fence_lifecycle_ready{false};
    std::uint64_t frame_index{0};
    std::string_view backend_id{"d3d12"};
};

struct NativeMaterialPreviewDisplayPlan {
    bool accepted{false};
    std::string status_id{"host_unavailable"};
    bool d3d12_host_available{false};
    bool shader_artifacts_available{false};
    bool gpu_payload_available{false};
    bool texture_display_requested{false};
    bool texture_adapter_available{false};
    bool offscreen_target_available{false};
    bool descriptor_lease_available{false};
    bool resource_barriers_recorded{false};
    bool fence_lifecycle_ready{false};
    bool texture_display_ready{false};
    bool native_texture_handles_exposed{false};
    std::string native_texture_handle_policy{"private"};
    std::uint64_t frame_index{0};
    std::string backend_id{"d3d12"};
    std::string lifecycle_status{"host_unavailable"};
    std::string diagnostic;
    EditorMaterialGpuPreviewExecutionSnapshot execution_snapshot;
};

[[nodiscard]] NativeMaterialPreviewDisplayPlan
plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc desc);

} // namespace mirakana::editor
