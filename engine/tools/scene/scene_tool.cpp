// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/scene_tool.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

struct SceneDependencySet {
    std::vector<AssetId> meshes;
    std::vector<AssetId> materials;
    std::vector<AssetId> sprites;
};

void add_failure(std::vector<ScenePackageUpdateFailure>& failures, AssetId asset, std::string path,
                 std::string diagnostic) {
    failures.push_back(ScenePackageUpdateFailure{
        .asset = asset,
        .path = std::move(path),
        .diagnostic = std::move(diagnostic),
    });
}

void sort_failures(std::vector<ScenePackageUpdateFailure>& failures) {
    std::ranges::sort(failures, [](const ScenePackageUpdateFailure& lhs, const ScenePackageUpdateFailure& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.diagnostic < rhs.diagnostic;
    });
}

[[nodiscard]] bool is_safe_package_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find('\0') != std::string_view::npos ||
        path.find('\n') != std::string_view::npos || path.find('\r') != std::string_view::npos) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto token = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty() || token == "." || token == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

void validate_package_relative_path(std::vector<ScenePackageUpdateFailure>& failures, AssetId asset,
                                    std::string_view path, std::string_view field) {
    if (!is_safe_package_path(path)) {
        add_failure(failures, asset, std::string{path},
                    std::string{field} + " must be package-relative and must not escape the package");
    }
}

void append_changed_file(std::vector<ScenePackageChangedFile>& files, std::string path, std::string content) {
    ScenePackageChangedFile file;
    file.path = std::move(path);
    file.content = std::move(content);
    file.content_hash = hash_asset_cooked_content(file.content);
    files.push_back(std::move(file));
}

[[nodiscard]] const AssetCookedPackageEntry* find_entry(const AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] AssetCookedPackageEntry* find_entry(AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] bool has_path_collision(const AssetCookedPackageIndex& index, AssetId asset, std::string_view path) {
    return std::ranges::any_of(index.entries, [asset, path](const AssetCookedPackageEntry& entry) {
        return entry.asset != asset && entry.path == path;
    });
}

void sort_asset_ids(std::vector<AssetId>& ids) {
    std::ranges::sort(ids, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
}

void sort_unique_asset_ids(std::vector<AssetId>& ids) {
    sort_asset_ids(ids);
    ids.erase(std::ranges::unique(ids, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }).begin(), ids.end());
}

[[nodiscard]] bool has_invalid_or_duplicate_ids(std::vector<AssetId> ids) {
    for (const auto id : ids) {
        if (id.value == 0) {
            return true;
        }
    }
    sort_asset_ids(ids);
    return std::ranges::adjacent_find(ids, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }) != ids.end();
}

void sort_entries(std::vector<AssetCookedPackageEntry>& entries) {
    std::ranges::sort(entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
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
        if (lhs.dependency.value != rhs.dependency.value) {
            return lhs.dependency.value < rhs.dependency.value;
        }
        return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
    });
}

[[nodiscard]] bool same_edge(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

void canonicalize_package_index(AssetCookedPackageIndex& index) {
    for (auto& entry : index.entries) {
        sort_unique_asset_ids(entry.dependencies);
    }
    sort_entries(index.entries);
    sort_edges(index.dependencies);
    index.dependencies.erase(std::ranges::unique(index.dependencies, same_edge).begin(), index.dependencies.end());
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void validate_scene_transforms(std::vector<ScenePackageUpdateFailure>& failures, AssetId scene_asset,
                               const Scene& scene, std::string_view path) {
    for (const auto& node : scene.nodes()) {
        if (!finite_vec3(node.transform.position) || !finite_vec3(node.transform.rotation_radians) ||
            !finite_vec3(node.transform.scale) || node.transform.scale.x == 0.0F || node.transform.scale.y == 0.0F ||
            node.transform.scale.z == 0.0F) {
            add_failure(failures, scene_asset, std::string{path}, "scene transform must be finite with non-zero scale");
        }
    }
}

[[nodiscard]] SceneDependencySet collect_scene_dependencies(const Scene& scene) {
    SceneDependencySet dependencies;
    for (const auto& node : scene.nodes()) {
        if (node.components.mesh_renderer.has_value()) {
            dependencies.meshes.push_back(node.components.mesh_renderer->mesh);
            dependencies.materials.push_back(node.components.mesh_renderer->material);
        }
        if (node.components.sprite_renderer.has_value()) {
            dependencies.sprites.push_back(node.components.sprite_renderer->sprite);
            dependencies.materials.push_back(node.components.sprite_renderer->material);
        }
    }
    sort_unique_asset_ids(dependencies.meshes);
    sort_unique_asset_ids(dependencies.materials);
    sort_unique_asset_ids(dependencies.sprites);
    return dependencies;
}

void validate_declared_dependencies(std::vector<ScenePackageUpdateFailure>& failures,
                                    const ScenePackageUpdateDesc& desc, const SceneDependencySet& actual) {
    if (has_invalid_or_duplicate_ids(desc.mesh_dependencies)) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene mesh dependency declarations must be unique non-zero asset ids");
    }
    if (has_invalid_or_duplicate_ids(desc.material_dependencies)) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene material dependency declarations must be unique non-zero asset ids");
    }
    if (has_invalid_or_duplicate_ids(desc.sprite_dependencies)) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene sprite dependency declarations must be unique non-zero asset ids");
    }

    auto meshes = desc.mesh_dependencies;
    auto materials = desc.material_dependencies;
    auto sprites = desc.sprite_dependencies;
    sort_unique_asset_ids(meshes);
    sort_unique_asset_ids(materials);
    sort_unique_asset_ids(sprites);

    if (meshes != actual.meshes) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene mesh dependency declarations do not match scene payload");
    }
    if (materials != actual.materials) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene material dependency declarations do not match scene payload");
    }
    if (sprites != actual.sprites) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene sprite dependency declarations do not match scene payload");
    }
}

void validate_unsupported_claims(std::vector<ScenePackageUpdateFailure>& failures, const ScenePackageUpdateDesc& desc) {
    if (desc.editor_productization != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "editor productization is not supported by scene package apply tooling");
    }
    if (desc.prefab_mutation != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "prefab mutation is not supported by scene package apply tooling");
    }
    if (desc.runtime_source_import != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "runtime source import is not supported by scene package apply tooling");
    }
    if (desc.renderer_rhi_residency != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "renderer/RHI residency is not supported by scene package apply tooling");
    }
    if (desc.package_streaming != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "package streaming is not supported by scene package apply tooling");
    }
    if (desc.material_graph != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "material graph is not supported by scene package apply tooling");
    }
    if (desc.shader_graph != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "shader graph is not supported by scene package apply tooling");
    }
    if (desc.live_shader_generation != "unsupported") {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "live shader generation is not supported by scene package apply tooling");
    }
}

void validate_dependency_entry(std::vector<ScenePackageUpdateFailure>& failures, const AssetCookedPackageIndex& index,
                               AssetId scene_asset, AssetId dependency, AssetKind expected_kind,
                               std::string_view label) {
    const auto* entry = find_entry(index, dependency);
    if (entry == nullptr) {
        add_failure(failures, dependency, {}, std::string{"scene "} + std::string{label} + " package entry is missing");
        return;
    }

    validate_package_relative_path(failures, dependency, entry->path,
                                   std::string{"scene "} + std::string{label} + " package entry path");
    if (entry->kind != expected_kind) {
        const auto* const kind_name = expected_kind == AssetKind::mesh       ? "mesh"
                                      : expected_kind == AssetKind::material ? "material"
                                      : expected_kind == AssetKind::texture  ? "texture"
                                                                             : "expected kind";
        add_failure(failures, dependency, entry->path,
                    std::string{"scene "} + std::string{label} + " package entry is not a " + kind_name);
    }
    if (entry->asset == scene_asset) {
        add_failure(failures, dependency, entry->path, "scene dependency package entry must not reference itself");
    }
}

void validate_package_rows(std::vector<ScenePackageUpdateFailure>& failures, const AssetCookedPackageIndex& index,
                           const ScenePackageUpdateDesc& desc, const SceneDependencySet& dependencies) {
    if (has_path_collision(index, desc.scene_asset, desc.output_path)) {
        add_failure(failures, desc.scene_asset, desc.output_path,
                    "scene output path collides with another package entry");
    }

    const auto* existing = find_entry(index, desc.scene_asset);
    if (existing != nullptr && existing->kind != AssetKind::scene) {
        add_failure(failures, desc.scene_asset, existing->path, "existing scene package entry kind must be scene");
    }

    for (const auto mesh : dependencies.meshes) {
        validate_dependency_entry(failures, index, desc.scene_asset, mesh, AssetKind::mesh, "mesh");
    }
    for (const auto material : dependencies.materials) {
        validate_dependency_entry(failures, index, desc.scene_asset, material, AssetKind::material, "material");
    }
    for (const auto sprite : dependencies.sprites) {
        validate_dependency_entry(failures, index, desc.scene_asset, sprite, AssetKind::texture, "sprite");
    }
}

void append_dependency_edges(AssetCookedPackageIndex& index, AssetId scene_asset,
                             const std::vector<AssetId>& dependencies, AssetDependencyKind kind) {
    for (const auto dependency : dependencies) {
        const auto* entry = find_entry(index, dependency);
        if (entry == nullptr) {
            continue;
        }
        index.dependencies.push_back(
            AssetDependencyEdge{.asset = scene_asset, .dependency = dependency, .kind = kind, .path = entry->path});
    }
}

[[nodiscard]] std::vector<AssetId> all_scene_dependencies(SceneDependencySet dependencies) {
    std::vector<AssetId> result;
    result.reserve(dependencies.meshes.size() + dependencies.materials.size() + dependencies.sprites.size());
    result.insert(result.end(), dependencies.meshes.begin(), dependencies.meshes.end());
    result.insert(result.end(), dependencies.materials.begin(), dependencies.materials.end());
    result.insert(result.end(), dependencies.sprites.begin(), dependencies.sprites.end());
    sort_unique_asset_ids(result);
    return result;
}

void replace_scene_entry(AssetCookedPackageIndex& index, const ScenePackageUpdateDesc& desc,
                         std::string_view scene_content, const SceneDependencySet& dependencies) {
    const auto dependency_ids = all_scene_dependencies(dependencies);
    const auto removed_scene_deps =
        std::ranges::remove_if(index.dependencies, [scene_asset = desc.scene_asset](const AssetDependencyEdge& edge) {
            return edge.asset == scene_asset;
        });
    index.dependencies.erase(removed_scene_deps.begin(), removed_scene_deps.end());

    append_dependency_edges(index, desc.scene_asset, dependencies.meshes, AssetDependencyKind::scene_mesh);
    append_dependency_edges(index, desc.scene_asset, dependencies.materials, AssetDependencyKind::scene_material);
    append_dependency_edges(index, desc.scene_asset, dependencies.sprites, AssetDependencyKind::scene_sprite);

    if (auto* entry = find_entry(index, desc.scene_asset); entry != nullptr) {
        *entry = AssetCookedPackageEntry{
            .asset = desc.scene_asset,
            .kind = AssetKind::scene,
            .path = desc.output_path,
            .content_hash = hash_asset_cooked_content(scene_content),
            .source_revision = desc.source_revision,
            .dependencies = dependency_ids,
        };
    } else {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = desc.scene_asset,
            .kind = AssetKind::scene,
            .path = desc.output_path,
            .content_hash = hash_asset_cooked_content(scene_content),
            .source_revision = desc.source_revision,
            .dependencies = dependency_ids,
        });
    }

    canonicalize_package_index(index);
}

} // namespace

ScenePackageUpdateResult plan_scene_package_update(const ScenePackageUpdateDesc& desc) {
    ScenePackageUpdateResult result;

    validate_package_relative_path(result.failures, desc.scene_asset, desc.package_index_path,
                                   "scene package index path");
    validate_package_relative_path(result.failures, desc.scene_asset, desc.output_path, "scene output path");
    if (desc.scene_asset.value == 0) {
        add_failure(result.failures, desc.scene_asset, desc.output_path, "scene asset id must be non-zero");
    }
    if (desc.source_revision == 0) {
        add_failure(result.failures, desc.scene_asset, desc.output_path, "scene source revision must be non-zero");
    }
    validate_unsupported_claims(result.failures, desc);
    validate_scene_transforms(result.failures, desc.scene_asset, desc.scene, desc.output_path);

    try {
        result.scene_content = serialize_scene(desc.scene);
        [[maybe_unused]] const Scene parsed_scene = deserialize_scene(result.scene_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.scene_asset, desc.output_path,
                    std::string{"scene payload is invalid: "} + error.what());
    }

    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto dependencies = collect_scene_dependencies(desc.scene);
    validate_declared_dependencies(result.failures, desc, dependencies);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.scene_asset, desc.package_index_path,
                    std::string{"scene package index is invalid: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    validate_package_rows(result.failures, index, desc, dependencies);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    replace_scene_entry(index, desc, result.scene_content, dependencies);

    try {
        result.package_index_content = serialize_asset_cooked_package_index(index);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.scene_asset, desc.package_index_path,
                    std::string{"updated scene package index is invalid: "} + error.what());
        result.package_index_content.clear();
        sort_failures(result.failures);
        return result;
    }

    append_changed_file(result.changed_files, desc.output_path, result.scene_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

ScenePackageUpdateResult apply_scene_package_update(IFileSystem& filesystem, const ScenePackageApplyDesc& desc) {
    ScenePackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.scene_asset, desc.package_index_path,
                    std::string{"failed to read scene package index: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    result = plan_scene_package_update(ScenePackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .output_path = desc.output_path,
        .source_revision = desc.source_revision,
        .scene_asset = desc.scene_asset,
        .scene = desc.scene,
        .mesh_dependencies = desc.mesh_dependencies,
        .material_dependencies = desc.material_dependencies,
        .sprite_dependencies = desc.sprite_dependencies,
        .editor_productization = desc.editor_productization,
        .prefab_mutation = desc.prefab_mutation,
        .runtime_source_import = desc.runtime_source_import,
        .renderer_rhi_residency = desc.renderer_rhi_residency,
        .package_streaming = desc.package_streaming,
        .material_graph = desc.material_graph,
        .shader_graph = desc.shader_graph,
        .live_shader_generation = desc.live_shader_generation,
    });
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.output_path, result.scene_content);
        filesystem.write_text(desc.package_index_path, result.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.scene_asset, desc.package_index_path,
                    std::string{"failed to write scene package update: "} + error.what());
        sort_failures(result.failures);
    }
    return result;
}

} // namespace mirakana
