// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/render_backend.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana::editor {

struct ViewportExtent {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

enum class ViewportRunMode : std::uint8_t {
    edit,
    play,
    paused,
};

enum class ViewportTool : std::uint8_t {
    select,
    translate,
    rotate,
    scale,
};

class ViewportState {
  public:
    [[nodiscard]] std::string_view renderer_name() const noexcept;
    [[nodiscard]] ViewportExtent extent() const noexcept;
    [[nodiscard]] bool focused() const noexcept;
    [[nodiscard]] bool hovered() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::uint64_t rendered_frame_count() const noexcept;
    [[nodiscard]] std::uint64_t simulation_frame_count() const noexcept;
    [[nodiscard]] ViewportRunMode run_mode() const noexcept;
    [[nodiscard]] ViewportTool active_tool() const noexcept;
    [[nodiscard]] EditorRenderBackend requested_render_backend() const noexcept;
    [[nodiscard]] EditorRenderBackend active_render_backend() const noexcept;
    [[nodiscard]] std::string_view render_backend_diagnostic() const noexcept;

    void set_renderer(std::string name);
    void set_render_backend_selection(EditorRenderBackendChoice choice);
    void resize(ViewportExtent extent);
    void set_focused(bool focused) noexcept;
    void set_hovered(bool hovered) noexcept;
    void mark_frame_rendered() noexcept;
    [[nodiscard]] bool mark_simulation_tick() noexcept;
    [[nodiscard]] bool play() noexcept;
    [[nodiscard]] bool pause() noexcept;
    [[nodiscard]] bool resume() noexcept;
    [[nodiscard]] bool stop() noexcept;
    void set_active_tool(ViewportTool tool) noexcept;

  private:
    std::string renderer_name_;
    EditorRenderBackend requested_render_backend_{EditorRenderBackend::automatic};
    EditorRenderBackend active_render_backend_{EditorRenderBackend::null};
    std::string render_backend_diagnostic_;
    ViewportExtent extent_;
    bool focused_{false};
    bool hovered_{false};
    std::uint64_t rendered_frames_{0};
    std::uint64_t simulation_frames_{0};
    ViewportRunMode run_mode_{ViewportRunMode::edit};
    ViewportTool active_tool_{ViewportTool::select};
};

[[nodiscard]] std::string_view viewport_run_mode_label(ViewportRunMode mode) noexcept;
[[nodiscard]] std::string_view viewport_tool_label(ViewportTool tool) noexcept;

} // namespace mirakana::editor
