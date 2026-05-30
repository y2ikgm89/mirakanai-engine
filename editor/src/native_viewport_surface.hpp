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
    bool renderer_output_available{false};
    ViewportExtent extent{.width = 1280, .height = 720};
    std::uint64_t frame_index{0};
    std::string_view backend_id{"d3d12"};
};

struct NativeViewportDisplayPlan {
    bool accepted{false};
    std::string status_id{"host_unavailable"};
    bool d3d12_host_available{false};
    bool renderer_output_available{false};
    bool texture_display_ready{false};
    bool native_texture_handles_exposed{false};
    std::string native_texture_handle_policy{"private"};
    ViewportExtent extent{};
    std::uint64_t frame_index{0};
    std::string backend_id{"d3d12"};
    std::string diagnostic;
};

[[nodiscard]] NativeViewportDisplayPlan plan_native_viewport_display(NativeViewportDisplayDesc desc);

} // namespace mirakana::editor
