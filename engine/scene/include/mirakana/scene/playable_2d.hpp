// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/scene/scene.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mirakana {

enum class Playable2DSceneDiagnosticCode {
    invalid_components,
    missing_primary_orthographic_camera,
    multiple_primary_cameras,
    primary_camera_not_orthographic,
    missing_visible_sprite,
    invalid_sprite_renderer,
    visible_mesh_renderer,
};

struct Playable2DSceneDiagnostic {
    Playable2DSceneDiagnosticCode code{Playable2DSceneDiagnosticCode::invalid_components};
    SceneNodeId node;
    std::string message;
};

struct Playable2DSceneValidationDesc {
    bool require_primary_orthographic_camera{true};
    bool require_visible_sprite{true};
    bool reject_visible_mesh_renderers{true};
};

struct Playable2DSceneValidationResult {
    std::size_t camera_count{0};
    std::size_t primary_camera_count{0};
    std::size_t visible_sprite_count{0};
    std::size_t visible_mesh_count{0};
    bool has_primary_orthographic_camera{false};
    std::vector<Playable2DSceneDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] Playable2DSceneValidationResult validate_playable_2d_scene(const Scene& scene,
                                                                         Playable2DSceneValidationDesc desc = {});
[[nodiscard]] bool has_playable_2d_diagnostic(const Playable2DSceneValidationResult& result,
                                              Playable2DSceneDiagnosticCode code) noexcept;

} // namespace mirakana
