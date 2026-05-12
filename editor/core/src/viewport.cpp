// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/viewport.hpp"

#include <stdexcept>
#include <utility>

namespace mirakana::editor {

std::string_view ViewportState::renderer_name() const noexcept {
    return renderer_name_;
}

ViewportExtent ViewportState::extent() const noexcept {
    return extent_;
}

bool ViewportState::focused() const noexcept {
    return focused_;
}

bool ViewportState::hovered() const noexcept {
    return hovered_;
}

bool ViewportState::ready() const noexcept {
    return !renderer_name_.empty() && extent_.width > 0 && extent_.height > 0;
}

std::uint64_t ViewportState::rendered_frame_count() const noexcept {
    return rendered_frames_;
}

std::uint64_t ViewportState::simulation_frame_count() const noexcept {
    return simulation_frames_;
}

ViewportRunMode ViewportState::run_mode() const noexcept {
    return run_mode_;
}

ViewportTool ViewportState::active_tool() const noexcept {
    return active_tool_;
}

EditorRenderBackend ViewportState::requested_render_backend() const noexcept {
    return requested_render_backend_;
}

EditorRenderBackend ViewportState::active_render_backend() const noexcept {
    return active_render_backend_;
}

std::string_view ViewportState::render_backend_diagnostic() const noexcept {
    return render_backend_diagnostic_;
}

void ViewportState::set_renderer(std::string name) {
    if (name.empty()) {
        throw std::invalid_argument("editor viewport renderer name must not be empty");
    }
    renderer_name_ = std::move(name);
}

void ViewportState::set_render_backend_selection(EditorRenderBackendChoice choice) {
    requested_render_backend_ = choice.requested;
    active_render_backend_ = choice.active;
    render_backend_diagnostic_ = std::string(choice.diagnostic);
    set_renderer(std::string(editor_render_backend_id(choice.active)));
}

void ViewportState::resize(ViewportExtent extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("editor viewport extent must be non-zero");
    }
    extent_ = extent;
}

void ViewportState::set_focused(bool focused) noexcept {
    focused_ = focused;
}

void ViewportState::set_hovered(bool hovered) noexcept {
    hovered_ = hovered;
}

void ViewportState::mark_frame_rendered() noexcept {
    ++rendered_frames_;
}

bool ViewportState::mark_simulation_tick() noexcept {
    if (run_mode_ != ViewportRunMode::play) {
        return false;
    }
    ++simulation_frames_;
    return true;
}

bool ViewportState::play() noexcept {
    if (run_mode_ != ViewportRunMode::edit) {
        return false;
    }
    run_mode_ = ViewportRunMode::play;
    simulation_frames_ = 0;
    return true;
}

bool ViewportState::pause() noexcept {
    if (run_mode_ != ViewportRunMode::play) {
        return false;
    }
    run_mode_ = ViewportRunMode::paused;
    return true;
}

bool ViewportState::resume() noexcept {
    if (run_mode_ != ViewportRunMode::paused) {
        return false;
    }
    run_mode_ = ViewportRunMode::play;
    return true;
}

bool ViewportState::stop() noexcept {
    if (run_mode_ == ViewportRunMode::edit) {
        return false;
    }
    run_mode_ = ViewportRunMode::edit;
    simulation_frames_ = 0;
    return true;
}

void ViewportState::set_active_tool(ViewportTool tool) noexcept {
    active_tool_ = tool;
}

std::string_view viewport_run_mode_label(ViewportRunMode mode) noexcept {
    switch (mode) {
    case ViewportRunMode::edit:
        return "Edit";
    case ViewportRunMode::play:
        return "Play";
    case ViewportRunMode::paused:
        return "Paused";
    }
    return "Unknown";
}

std::string_view viewport_tool_label(ViewportTool tool) noexcept {
    switch (tool) {
    case ViewportTool::select:
        return "Select";
    case ViewportTool::translate:
        return "Move";
    case ViewportTool::rotate:
        return "Rotate";
    case ViewportTool::scale:
        return "Scale";
    }
    return "Unknown";
}

} // namespace mirakana::editor
