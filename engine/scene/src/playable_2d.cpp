// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/playable_2d.hpp"

#include <algorithm>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(Playable2DSceneValidationResult& result, Playable2DSceneDiagnosticCode code, SceneNodeId node,
                    std::string message) {
    result.diagnostics.push_back(Playable2DSceneDiagnostic{.code = code, .node = node, .message = std::move(message)});
}

} // namespace

bool Playable2DSceneValidationResult::succeeded() const noexcept {
    return diagnostics.empty();
}

Playable2DSceneValidationResult validate_playable_2d_scene(const Scene& scene, Playable2DSceneValidationDesc desc) {
    Playable2DSceneValidationResult result;

    for (const auto& node : scene.nodes()) {
        if (!is_valid_scene_node_components(node.components)) {
            add_diagnostic(result, Playable2DSceneDiagnosticCode::invalid_components, node.id,
                           "2D playable scene node has invalid component data");
        }

        if (node.components.camera.has_value()) {
            ++result.camera_count;
            const auto& camera = *node.components.camera;
            if (camera.primary) {
                ++result.primary_camera_count;
                if (is_valid_camera_component(camera) && camera.projection == CameraProjectionMode::orthographic) {
                    result.has_primary_orthographic_camera = true;
                } else if (camera.projection != CameraProjectionMode::orthographic) {
                    add_diagnostic(result, Playable2DSceneDiagnosticCode::primary_camera_not_orthographic, node.id,
                                   "2D playable scene primary camera must be orthographic");
                }
            }
        }

        if (node.components.sprite_renderer.has_value() && node.components.sprite_renderer->visible) {
            if (is_valid_sprite_renderer_component(*node.components.sprite_renderer)) {
                ++result.visible_sprite_count;
            } else {
                add_diagnostic(result, Playable2DSceneDiagnosticCode::invalid_sprite_renderer, node.id,
                               "2D playable scene visible sprite renderer is invalid");
            }
        }

        if (node.components.mesh_renderer.has_value() && node.components.mesh_renderer->visible) {
            ++result.visible_mesh_count;
            if (desc.reject_visible_mesh_renderers) {
                add_diagnostic(result, Playable2DSceneDiagnosticCode::visible_mesh_renderer, node.id,
                               "2D playable source-tree recipe does not accept visible 3D mesh renderers");
            }
        }
    }

    if (desc.require_primary_orthographic_camera && !result.has_primary_orthographic_camera) {
        add_diagnostic(result, Playable2DSceneDiagnosticCode::missing_primary_orthographic_camera, null_scene_node,
                       "2D playable scene requires a primary orthographic camera");
    }
    if (result.primary_camera_count > 1) {
        add_diagnostic(result, Playable2DSceneDiagnosticCode::multiple_primary_cameras, null_scene_node,
                       "2D playable scene requires a single primary camera");
    }
    if (desc.require_visible_sprite && result.visible_sprite_count == 0) {
        add_diagnostic(result, Playable2DSceneDiagnosticCode::missing_visible_sprite, null_scene_node,
                       "2D playable scene requires at least one visible sprite renderer");
    }

    return result;
}

bool has_playable_2d_diagnostic(const Playable2DSceneValidationResult& result,
                                Playable2DSceneDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const Playable2DSceneDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
