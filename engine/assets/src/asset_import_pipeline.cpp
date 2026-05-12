// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_pipeline.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_action_kind(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
    case AssetImportActionKind::mesh:
    case AssetImportActionKind::morph_mesh_cpu:
    case AssetImportActionKind::animation_float_clip:
    case AssetImportActionKind::animation_quaternion_clip:
    case AssetImportActionKind::material:
    case AssetImportActionKind::scene:
    case AssetImportActionKind::audio:
        return true;
    case AssetImportActionKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_dependencies(const std::vector<AssetId>& dependencies) noexcept {
    for (std::size_t index = 0; index < dependencies.size(); ++index) {
        if (dependencies[index].value == 0) {
            return false;
        }
        for (std::size_t other = index + 1U; other < dependencies.size(); ++other) {
            if (dependencies[index] == dependencies[other]) {
                return false;
            }
        }
    }
    return true;
}

void add_unique_dependency(std::vector<AssetId>& dependencies, AssetId dependency) {
    if (std::ranges::find(dependencies, dependency) == dependencies.end()) {
        dependencies.push_back(dependency);
    }
}

void sort_asset_ids(std::vector<AssetId>& dependencies) {
    std::ranges::sort(dependencies, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
}

void sort_actions(std::vector<AssetImportAction>& actions) {
    std::ranges::sort(actions, [](const AssetImportAction& lhs, const AssetImportAction& rhs) {
        if (lhs.output_path != rhs.output_path) {
            return lhs.output_path < rhs.output_path;
        }
        return lhs.id.value < rhs.id.value;
    });
}

void sort_edges(std::vector<AssetDependencyEdge>& edges) {
    std::ranges::sort(edges, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.dependency.value < rhs.dependency.value;
    });
}

void add_scene_dependency_edges(AssetImportPlan& plan, std::vector<AssetId>& action_dependencies, AssetId scene_id,
                                std::span<const AssetId> dependencies, AssetDependencyKind kind,
                                const AssetImportMetadataRegistry& imports) {
    for (const auto dependency : dependencies) {
        std::string dependency_path;
        if (kind == AssetDependencyKind::scene_mesh) {
            const auto* mesh = imports.find_mesh(dependency);
            if (mesh == nullptr) {
                throw std::invalid_argument("scene import references an unknown mesh dependency");
            }
            dependency_path = mesh->imported_path;
        } else if (kind == AssetDependencyKind::scene_material) {
            const auto* material = imports.find_material(dependency);
            if (material == nullptr) {
                throw std::invalid_argument("scene import references an unknown material dependency");
            }
            dependency_path = material->imported_path;
        } else if (kind == AssetDependencyKind::scene_sprite) {
            const auto* texture = imports.find_texture(dependency);
            if (texture == nullptr) {
                throw std::invalid_argument("scene import references an unknown sprite texture dependency");
            }
            dependency_path = texture->imported_path;
        } else {
            throw std::invalid_argument("scene import dependency kind is invalid");
        }

        add_unique_dependency(action_dependencies, dependency);
        AssetDependencyEdge edge{
            .asset = scene_id,
            .dependency = dependency,
            .kind = kind,
            .path = std::move(dependency_path),
        };
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("scene dependency edge is invalid");
        }
        plan.dependencies.push_back(std::move(edge));
    }
}

} // namespace

bool is_valid_asset_import_action(const AssetImportAction& action) noexcept {
    return action.id.value != 0 && valid_action_kind(action.kind) && valid_token(action.source_path) &&
           valid_token(action.output_path) && valid_dependencies(action.dependencies);
}

AssetImportPlan build_asset_import_plan(const AssetImportMetadataRegistry& imports) {
    AssetImportPlan plan;

    const auto textures = imports.texture_records();
    for (const auto& texture : textures) {
        AssetImportAction action{
            .id = texture.id,
            .kind = AssetImportActionKind::texture,
            .source_path = texture.source_path,
            .output_path = texture.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("texture import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto meshes = imports.mesh_records();
    for (const auto& mesh : meshes) {
        AssetImportAction action{
            .id = mesh.id,
            .kind = AssetImportActionKind::mesh,
            .source_path = mesh.source_path,
            .output_path = mesh.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("mesh import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto morph_meshes_cpu = imports.morph_mesh_cpu_records();
    for (const auto& morph : morph_meshes_cpu) {
        AssetImportAction action{
            .id = morph.id,
            .kind = AssetImportActionKind::morph_mesh_cpu,
            .source_path = morph.source_path,
            .output_path = morph.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("morph mesh CPU import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto animation_float_clips = imports.animation_float_clip_records();
    for (const auto& clip : animation_float_clips) {
        AssetImportAction action{
            .id = clip.id,
            .kind = AssetImportActionKind::animation_float_clip,
            .source_path = clip.source_path,
            .output_path = clip.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("animation float clip import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto animation_quaternion_clips = imports.animation_quaternion_clip_records();
    for (const auto& clip : animation_quaternion_clips) {
        AssetImportAction action{
            .id = clip.id,
            .kind = AssetImportActionKind::animation_quaternion_clip,
            .source_path = clip.source_path,
            .output_path = clip.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("animation quaternion clip import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto materials = imports.material_records();
    for (const auto& material : materials) {
        AssetImportAction action{
            .id = material.id,
            .kind = AssetImportActionKind::material,
            .source_path = material.source_path,
            .output_path = material.imported_path,
            .dependencies = material.texture_dependencies,
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("material import action is invalid");
        }

        for (const auto texture_id : material.texture_dependencies) {
            const auto* texture = imports.find_texture(texture_id);
            if (texture == nullptr) {
                throw std::invalid_argument("material import references an unknown texture dependency");
            }
            AssetDependencyEdge edge{
                .asset = material.id,
                .dependency = texture_id,
                .kind = AssetDependencyKind::material_texture,
                .path = texture->imported_path,
            };
            if (!is_valid_asset_dependency_edge(edge)) {
                throw std::invalid_argument("material texture dependency edge is invalid");
            }
            plan.dependencies.push_back(std::move(edge));
        }

        plan.actions.push_back(std::move(action));
    }

    const auto audio = imports.audio_records();
    for (const auto& item : audio) {
        AssetImportAction action{
            .id = item.id,
            .kind = AssetImportActionKind::audio,
            .source_path = item.source_path,
            .output_path = item.imported_path,
            .dependencies = {},
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("audio import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    const auto scenes = imports.scene_records();
    for (const auto& scene : scenes) {
        std::vector<AssetId> dependencies;
        add_scene_dependency_edges(plan, dependencies, scene.id, scene.mesh_dependencies,
                                   AssetDependencyKind::scene_mesh, imports);
        add_scene_dependency_edges(plan, dependencies, scene.id, scene.material_dependencies,
                                   AssetDependencyKind::scene_material, imports);
        add_scene_dependency_edges(plan, dependencies, scene.id, scene.sprite_dependencies,
                                   AssetDependencyKind::scene_sprite, imports);
        sort_asset_ids(dependencies);

        AssetImportAction action{
            .id = scene.id,
            .kind = AssetImportActionKind::scene,
            .source_path = scene.source_path,
            .output_path = scene.imported_path,
            .dependencies = std::move(dependencies),
        };
        if (!is_valid_asset_import_action(action)) {
            throw std::invalid_argument("scene import action is invalid");
        }
        plan.actions.push_back(std::move(action));
    }

    sort_actions(plan.actions);
    sort_edges(plan.dependencies);
    return plan;
}

AssetImportPlan build_asset_recook_plan(const AssetImportPlan& import_plan,
                                        const std::vector<AssetHotReloadRecookRequest>& requests) {
    AssetImportPlan recook_plan;
    std::unordered_set<AssetId, AssetIdHash> requested_assets;

    for (const auto& request : requests) {
        if (request.asset.value == 0) {
            throw std::invalid_argument("asset recook request has an invalid asset id");
        }
        const auto [_, inserted] = requested_assets.insert(request.asset);
        if (!inserted) {
            continue;
        }

        const auto action = std::ranges::find_if(
            import_plan.actions, [asset = request.asset](const AssetImportAction& item) { return item.id == asset; });
        if (action == import_plan.actions.end()) {
            throw std::invalid_argument("asset recook request has no import action");
        }

        recook_plan.actions.push_back(*action);
    }

    for (const auto& edge : import_plan.dependencies) {
        if (requested_assets.find(edge.asset) != requested_assets.end()) {
            recook_plan.dependencies.push_back(edge);
        }
    }

    sort_actions(recook_plan.actions);
    sort_edges(recook_plan.dependencies);
    return recook_plan;
}

} // namespace mirakana
