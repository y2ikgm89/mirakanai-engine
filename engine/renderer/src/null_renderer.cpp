// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/renderer.hpp"

#include <stdexcept>

namespace mirakana {

NullRenderer::NullRenderer(Extent2D extent) : extent_(extent) {
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument("renderer extent must be non-zero");
    }
}

std::string_view NullRenderer::backend_name() const noexcept {
    return "null";
}

Extent2D NullRenderer::backbuffer_extent() const noexcept {
    return extent_;
}

RendererStats NullRenderer::stats() const noexcept {
    return stats_;
}

Color NullRenderer::clear_color() const noexcept {
    return clear_color_;
}

bool NullRenderer::frame_active() const noexcept {
    return frame_active_;
}

void NullRenderer::resize(Extent2D extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("renderer extent must be non-zero");
    }
    extent_ = extent;
}

void NullRenderer::set_clear_color(Color color) noexcept {
    clear_color_ = color;
}

void NullRenderer::begin_frame() {
    if (frame_active_) {
        throw std::logic_error("renderer frame already active");
    }
    frame_active_ = true;
    ++stats_.frames_started;
}

void NullRenderer::draw_sprite(const SpriteCommand& /*command*/) {
    if (!frame_active_) {
        throw std::logic_error("renderer frame is not active");
    }
    ++stats_.sprites_submitted;
}

void NullRenderer::draw_mesh(const MeshCommand& command) {
    if (!frame_active_) {
        throw std::logic_error("renderer frame is not active");
    }
    if (command.gpu_skinning) {
        ++stats_.gpu_skinning_draws;
    }
    if (command.gpu_morphing) {
        ++stats_.gpu_morph_draws;
    }
    ++stats_.meshes_submitted;
}

void NullRenderer::end_frame() {
    if (!frame_active_) {
        throw std::logic_error("renderer frame is not active");
    }
    frame_active_ = false;
    ++stats_.frames_finished;
}

} // namespace mirakana
