// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/material_tool.hpp"

#include "mirakana/assets/material_graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_failure(std::vector<MaterialInstancePackageUpdateFailure>& failures, std::string diagnostic) {
    failures.push_back(MaterialInstancePackageUpdateFailure{std::move(diagnostic)});
}

[[nodiscard]] bool is_safe_package_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos) {
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

void validate_package_relative_path(std::vector<MaterialInstancePackageUpdateFailure>& failures, std::string_view field,
                                    std::string_view path) {
    if (!is_safe_package_path(path)) {
        add_failure(failures, std::string(field) + " must be package-relative and must not escape the package");
    }
}

void append_changed_file(std::vector<MaterialInstancePackageChangedFile>& files, std::string path,
                         std::string content) {
    MaterialInstancePackageChangedFile file;
    file.path = std::move(path);
    file.content = std::move(content);
    file.content_hash = hash_asset_cooked_content(file.content);
    files.push_back(std::move(file));
}

[[nodiscard]] AssetCookedPackageEntry* find_entry(AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] const AssetCookedPackageEntry* find_entry(const AssetCookedPackageIndex& index, AssetId asset) noexcept {
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
    ids.erase(std::ranges::unique(ids).begin(), ids.end());
}

void sort_package_index(AssetCookedPackageIndex& index) {
    std::ranges::sort(index.entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
    std::ranges::sort(index.dependencies, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) {
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

void validate_unsupported_claims(std::vector<MaterialInstancePackageUpdateFailure>& failures,
                                 const MaterialInstancePackageUpdateDesc& desc) {
    if (desc.material_graph != "unsupported") {
        add_failure(failures, "material graph is not supported by material instance package apply tooling");
    }
    if (desc.shader_graph != "unsupported") {
        add_failure(failures, "shader graph is not supported by material instance package apply tooling");
    }
    if (desc.live_shader_generation != "unsupported") {
        add_failure(failures, "live shader generation is not supported by material instance package apply tooling");
    }
}

void validate_unsupported_claims(std::vector<MaterialInstancePackageUpdateFailure>& failures,
                                 const MaterialGraphPackageUpdateDesc& desc) {
    if (desc.shader_graph != "unsupported") {
        add_failure(failures, "shader graph is not supported by material graph package apply tooling");
    }
    if (desc.live_shader_generation != "unsupported") {
        add_failure(failures, "live shader generation is not supported by material graph package apply tooling");
    }
    if (desc.renderer_rhi_residency != "unsupported") {
        add_failure(failures, "renderer/RHI residency is not supported by material graph package apply tooling");
    }
    if (desc.package_streaming != "unsupported") {
        add_failure(failures, "package streaming is not supported by material graph package apply tooling");
    }
}

[[nodiscard]] std::vector<AssetId> texture_dependencies(const MaterialInstanceDefinition& instance) {
    std::vector<AssetId> dependencies;
    dependencies.reserve(instance.texture_overrides.size());
    for (const auto& binding : instance.texture_overrides) {
        dependencies.push_back(binding.texture);
    }
    sort_asset_ids(dependencies);
    return dependencies;
}

[[nodiscard]] std::vector<AssetId> texture_dependencies(const MaterialDefinition& material) {
    std::vector<AssetId> dependencies;
    dependencies.reserve(material.texture_bindings.size());
    for (const auto& binding : material.texture_bindings) {
        dependencies.push_back(binding.texture);
    }
    sort_asset_ids(dependencies);
    return dependencies;
}

void validate_material_package_rows(std::vector<MaterialInstancePackageUpdateFailure>& failures,
                                    const AssetCookedPackageIndex& index, const MaterialInstancePackageUpdateDesc& desc,
                                    const std::vector<AssetId>& textures) {
    const auto* base = find_entry(index, desc.instance.parent);
    if (base == nullptr) {
        add_failure(failures, "base material package entry is missing");
    } else if (base->kind != AssetKind::material) {
        add_failure(failures, "base material package entry is not a material");
    }

    if (has_path_collision(index, desc.instance.id, desc.output_path)) {
        add_failure(failures, "material instance output path is already used by another package entry");
    }

    const auto* existing = find_entry(index, desc.instance.id);
    if (existing != nullptr && existing->kind != AssetKind::material) {
        add_failure(failures, "existing material instance package entry kind must be material");
    }

    for (const auto texture : textures) {
        const auto* texture_entry = find_entry(index, texture);
        if (texture_entry == nullptr) {
            add_failure(failures, "texture override package entry is missing");
            continue;
        }
        if (texture_entry->kind != AssetKind::texture) {
            add_failure(failures, "texture override package entry is not a texture");
        }
    }
}

void validate_material_graph_package_rows(std::vector<MaterialInstancePackageUpdateFailure>& failures,
                                          const AssetCookedPackageIndex& index,
                                          const MaterialGraphPackageUpdateDesc& desc,
                                          const MaterialDefinition& material, const std::vector<AssetId>& textures) {
    if (has_path_collision(index, material.id, desc.output_path)) {
        add_failure(failures, "material graph output path is already used by another package entry");
    }

    const auto* existing = find_entry(index, material.id);
    if (existing != nullptr && existing->kind != AssetKind::material) {
        add_failure(failures, "existing material graph package entry kind must be material");
    }

    for (const auto texture : textures) {
        const auto* texture_entry = find_entry(index, texture);
        if (texture_entry == nullptr) {
            add_failure(failures, "texture package entry is missing");
            continue;
        }
        if (texture_entry->kind != AssetKind::texture) {
            add_failure(failures, "texture package entry is not a texture");
        }
    }
}

void replace_material_entry(AssetCookedPackageIndex& index, AssetId material, std::string path,
                            std::uint64_t source_revision, std::string_view material_content,
                            std::vector<AssetId> dependencies) {
    const auto removed_deps = std::ranges::remove_if(
        index.dependencies, [material](const AssetDependencyEdge& edge) { return edge.asset == material; });
    index.dependencies.erase(removed_deps.begin(), removed_deps.end());

    for (const auto dependency : dependencies) {
        index.dependencies.push_back(AssetDependencyEdge{
            .asset = material, .dependency = dependency, .kind = AssetDependencyKind::material_texture, .path = path});
    }

    if (auto* entry = find_entry(index, material); entry != nullptr) {
        *entry = AssetCookedPackageEntry{
            .asset = material,
            .kind = AssetKind::material,
            .path = std::move(path),
            .content_hash = hash_asset_cooked_content(material_content),
            .source_revision = source_revision,
            .dependencies = std::move(dependencies),
        };
    } else {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = material,
            .kind = AssetKind::material,
            .path = std::move(path),
            .content_hash = hash_asset_cooked_content(material_content),
            .source_revision = source_revision,
            .dependencies = std::move(dependencies),
        });
    }

    sort_package_index(index);
}

} // namespace

MaterialInstancePackageUpdateResult
plan_material_instance_package_update(const MaterialInstancePackageUpdateDesc& desc) {
    MaterialInstancePackageUpdateResult result;

    validate_package_relative_path(result.failures, "package_index_path", desc.package_index_path);
    validate_package_relative_path(result.failures, "output_path", desc.output_path);
    if (desc.source_revision == 0) {
        add_failure(result.failures, "source revision must be non-zero");
    }
    validate_unsupported_claims(result.failures, desc);

    try {
        result.material_content = serialize_material_instance_definition(desc.instance);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("material instance definition is invalid: ") + error.what());
    }

    if (!result.failures.empty()) {
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("package index is invalid: ") + error.what());
        return result;
    }

    auto dependencies = texture_dependencies(desc.instance);
    validate_material_package_rows(result.failures, index, desc, dependencies);
    if (!result.failures.empty()) {
        return result;
    }

    replace_material_entry(index, desc.instance.id, desc.output_path, desc.source_revision, result.material_content,
                           std::move(dependencies));

    try {
        result.package_index_content = serialize_asset_cooked_package_index(index);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("updated package index is invalid: ") + error.what());
        result.package_index_content.clear();
        return result;
    }

    append_changed_file(result.changed_files, desc.output_path, result.material_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

MaterialGraphPackageUpdateResult plan_material_graph_package_update(const MaterialGraphPackageUpdateDesc& desc) {
    MaterialGraphPackageUpdateResult result;

    validate_package_relative_path(result.failures, "package_index_path", desc.package_index_path);
    validate_package_relative_path(result.failures, "output_path", desc.output_path);
    if (desc.source_revision == 0) {
        add_failure(result.failures, "source revision must be non-zero");
    }
    validate_unsupported_claims(result.failures, desc);

    MaterialDefinition material;
    try {
        const auto graph = deserialize_material_graph(desc.material_graph_content);
        material = lower_material_graph_to_definition(graph);
        result.material_content = serialize_material_definition(material);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("material graph is invalid: ") + error.what());
    }

    if (!result.failures.empty()) {
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("package index is invalid: ") + error.what());
        return result;
    }

    auto dependencies = texture_dependencies(material);
    validate_material_graph_package_rows(result.failures, index, desc, material, dependencies);
    if (!result.failures.empty()) {
        return result;
    }

    replace_material_entry(index, material.id, desc.output_path, desc.source_revision, result.material_content,
                           std::move(dependencies));

    try {
        result.package_index_content = serialize_asset_cooked_package_index(index);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("updated package index is invalid: ") + error.what());
        result.package_index_content.clear();
        return result;
    }

    append_changed_file(result.changed_files, desc.output_path, result.material_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

MaterialInstancePackageUpdateResult
apply_material_instance_package_update(IFileSystem& filesystem, const MaterialInstancePackageApplyDesc& desc) {
    MaterialInstancePackageUpdateResult result;
    std::string index_content;
    try {
        index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("package index could not be read: ") + error.what());
        return result;
    }

    result = plan_material_instance_package_update(MaterialInstancePackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(index_content),
        .output_path = desc.output_path,
        .source_revision = desc.source_revision,
        .instance = desc.instance,
        .material_graph = desc.material_graph,
        .shader_graph = desc.shader_graph,
        .live_shader_generation = desc.live_shader_generation,
    });
    if (!result.succeeded()) {
        return result;
    }

    filesystem.write_text(desc.output_path, result.material_content);
    filesystem.write_text(desc.package_index_path, result.package_index_content);
    return result;
}

MaterialGraphPackageUpdateResult apply_material_graph_package_update(IFileSystem& filesystem,
                                                                     const MaterialGraphPackageApplyDesc& desc) {
    MaterialGraphPackageUpdateResult result;
    validate_package_relative_path(result.failures, "package_index_path", desc.package_index_path);
    validate_package_relative_path(result.failures, "material_graph_path", desc.material_graph_path);
    validate_package_relative_path(result.failures, "output_path", desc.output_path);
    if (!result.failures.empty()) {
        return result;
    }

    std::string graph_content;
    try {
        graph_content = filesystem.read_text(desc.material_graph_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("material graph could not be read: ") + error.what());
        return result;
    }

    std::string index_content;
    try {
        index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, std::string("package index could not be read: ") + error.what());
        return result;
    }

    result = plan_material_graph_package_update(MaterialGraphPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(index_content),
        .material_graph_content = std::move(graph_content),
        .output_path = desc.output_path,
        .source_revision = desc.source_revision,
        .shader_graph = desc.shader_graph,
        .live_shader_generation = desc.live_shader_generation,
        .renderer_rhi_residency = desc.renderer_rhi_residency,
        .package_streaming = desc.package_streaming,
    });
    if (!result.succeeded()) {
        return result;
    }

    filesystem.write_text(desc.output_path, result.material_content);
    filesystem.write_text(desc.package_index_path, result.package_index_content);
    return result;
}

} // namespace mirakana
