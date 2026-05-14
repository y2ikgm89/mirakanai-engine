// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/tilemap_tool.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_failure(std::vector<CookedTilemapAuthoringFailure>& failures, AssetId asset, std::string path,
                 std::string diagnostic) {
    failures.push_back(CookedTilemapAuthoringFailure{
        .asset = asset,
        .path = std::move(path),
        .diagnostic = std::move(diagnostic),
    });
}

void sort_failures(std::vector<CookedTilemapAuthoringFailure>& failures) {
    std::ranges::sort(failures, [](const CookedTilemapAuthoringFailure& lhs, const CookedTilemapAuthoringFailure& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.diagnostic < rhs.diagnostic;
    });
}

[[nodiscard]] bool valid_output_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\0') == std::string_view::npos && path.find('\n') == std::string_view::npos &&
           path.find('\r') == std::string_view::npos && path.find('\\') == std::string_view::npos &&
           path.find(':') == std::string_view::npos && path.front() != '/' && path.find("..") == std::string_view::npos;
}

void sort_asset_ids(std::vector<AssetId>& ids) {
    std::ranges::sort(ids, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
}

void sort_dependency_edges(std::vector<AssetDependencyEdge>& edges) {
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

void sort_package_entries(std::vector<AssetCookedPackageEntry>& entries) {
    std::ranges::sort(entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

[[nodiscard]] TilemapMetadataDocument make_document(const CookedTilemapAuthoringDesc& desc) {
    TilemapMetadataDocument document;
    document.asset = desc.tilemap_asset;
    document.source_decoding = desc.source_decoding;
    document.atlas_packing = desc.atlas_packing;
    document.native_gpu_sprite_batching = desc.native_gpu_sprite_batching;
    document.atlas_page = desc.atlas_page;
    document.atlas_page_uri = desc.atlas_page_uri;
    document.tile_width = desc.tile_width;
    document.tile_height = desc.tile_height;
    document.tiles.reserve(desc.tiles.size());
    for (const auto& tile : desc.tiles) {
        document.tiles.push_back(TilemapAtlasTile{
            .id = tile.id,
            .page = tile.page,
            .uv = TilemapUvRect{.u0 = tile.u0, .v0 = tile.v0, .u1 = tile.u1, .v1 = tile.v1},
            .color = tile.color,
        });
    }
    document.layers.reserve(desc.layers.size());
    for (const auto& layer : desc.layers) {
        document.layers.push_back(TilemapLayer{
            .name = layer.name,
            .width = layer.width,
            .height = layer.height,
            .visible = layer.visible,
            .cells = layer.cells,
        });
    }
    return document;
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

[[nodiscard]] bool has_dependency(const AssetCookedPackageEntry& entry, AssetId dependency) noexcept {
    return std::ranges::find(entry.dependencies, dependency) != entry.dependencies.end();
}

[[nodiscard]] bool has_tilemap_texture_edge(const AssetCookedPackageIndex& index, AssetId tilemap_asset,
                                            AssetId texture_asset, std::string_view tilemap_path) noexcept {
    return std::ranges::any_of(
        index.dependencies, [tilemap_asset, texture_asset, tilemap_path](const AssetDependencyEdge& edge) {
            return edge.asset == tilemap_asset && edge.dependency == texture_asset &&
                   edge.kind == AssetDependencyKind::tilemap_texture && edge.path == tilemap_path;
        });
}

void append_changed_file(std::vector<CookedTilemapPackageChangedFile>& files, std::string path, std::string content) {
    files.push_back(CookedTilemapPackageChangedFile{
        .path = std::move(path),
        .content = std::move(content),
        .content_hash = 0,
    });
    files.back().content_hash = hash_asset_cooked_content(files.back().content);
}

void validate_package_relative_path(std::vector<CookedTilemapAuthoringFailure>& failures, AssetId asset,
                                    std::string_view path, std::string_view field) {
    if (!valid_output_path(path)) {
        add_failure(failures, asset, std::string{path}, std::string{field} + " must be a package-relative safe path");
    }
}

[[nodiscard]] bool has_path_collision(const AssetCookedPackageIndex& index, AssetId asset, std::string_view path) {
    return std::ranges::any_of(index.entries, [asset, path](const AssetCookedPackageEntry& entry) {
        return entry.asset != asset && entry.path == path;
    });
}

[[nodiscard]] bool edge_same_identity(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

void canonicalize_package_index(AssetCookedPackageIndex& index) {
    for (auto& entry : index.entries) {
        sort_asset_ids(entry.dependencies);
        entry.dependencies.erase(
            std::ranges::unique(entry.dependencies, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }).begin(),
            entry.dependencies.end());
    }
    sort_package_entries(index.entries);
    sort_dependency_edges(index.dependencies);
    index.dependencies.erase(std::ranges::unique(index.dependencies, edge_same_identity).begin(),
                             index.dependencies.end());
}

} // namespace

CookedTilemapAuthoringResult author_cooked_tilemap_metadata(const CookedTilemapAuthoringDesc& desc) {
    CookedTilemapAuthoringResult result;
    if (!valid_output_path(desc.output_path)) {
        add_failure(result.failures, desc.tilemap_asset, desc.output_path, "tilemap metadata output path is invalid");
    }
    if (desc.source_revision == 0) {
        add_failure(result.failures, desc.tilemap_asset, desc.output_path,
                    "tilemap metadata source revision must be non-zero");
    }

    const auto document = make_document(desc);
    for (const auto& diagnostic : validate_tilemap_metadata_document(document)) {
        add_failure(result.failures, diagnostic.asset.value != 0 ? diagnostic.asset : desc.tilemap_asset,
                    desc.output_path, diagnostic.message);
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    result.content = serialize_tilemap_metadata_document(document);

    result.dependency_edges.push_back(AssetDependencyEdge{
        .asset = desc.tilemap_asset,
        .dependency = desc.atlas_page,
        .kind = AssetDependencyKind::tilemap_texture,
        .path = desc.output_path,
    });

    result.artifact = AssetCookedArtifact{
        .asset = desc.tilemap_asset,
        .kind = AssetKind::tilemap,
        .path = desc.output_path,
        .content = result.content,
        .source_revision = desc.source_revision,
        .dependencies = {desc.atlas_page},
    };
    return result;
}

CookedTilemapAuthoringResult write_cooked_tilemap_metadata(IFileSystem& filesystem,
                                                           const CookedTilemapAuthoringDesc& desc) {
    auto result = author_cooked_tilemap_metadata(desc);
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.output_path, result.content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.tilemap_asset, desc.output_path,
                    std::string("failed to write cooked tilemap metadata: ") + error.what());
        sort_failures(result.failures);
    }
    return result;
}

CookedTilemapPackageVerificationResult verify_cooked_tilemap_package_metadata(const AssetCookedPackageIndex& index,
                                                                              AssetId tilemap_asset,
                                                                              std::string_view tilemap_content) {
    CookedTilemapPackageVerificationResult result;
    TilemapMetadataDocument document;
    try {
        document = deserialize_tilemap_metadata_document(tilemap_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, tilemap_asset, {}, std::string{"tilemap metadata is invalid: "} + error.what());
        return result;
    }

    if (document.asset != tilemap_asset) {
        add_failure(result.failures, document.asset, {}, "tilemap metadata asset id does not match requested asset");
    }

    const auto* tilemap_entry = find_entry(index, tilemap_asset);
    if (tilemap_entry == nullptr) {
        add_failure(result.failures, tilemap_asset, {}, "tilemap package entry is missing");
        sort_failures(result.failures);
        return result;
    }
    if (tilemap_entry->kind != AssetKind::tilemap) {
        add_failure(result.failures, tilemap_asset, tilemap_entry->path, "tilemap package entry kind must be tilemap");
    }
    if (tilemap_entry->content_hash != hash_asset_cooked_content(tilemap_content)) {
        add_failure(result.failures, tilemap_asset, tilemap_entry->path,
                    "tilemap content hash does not match package entry");
    }

    const auto* page_entry = find_entry(index, document.atlas_page);
    if (page_entry == nullptr) {
        add_failure(result.failures, document.atlas_page, document.atlas_page_uri,
                    "tilemap atlas texture package entry is missing");
    } else {
        if (page_entry->kind != AssetKind::texture) {
            add_failure(result.failures, document.atlas_page, page_entry->path,
                        "tilemap atlas texture package entry is not a texture");
        }
        if (page_entry->path != document.atlas_page_uri) {
            add_failure(result.failures, document.atlas_page, page_entry->path,
                        "tilemap atlas texture package path does not match atlas page asset_uri");
        }
    }
    if (!has_dependency(*tilemap_entry, document.atlas_page)) {
        add_failure(result.failures, document.atlas_page, tilemap_entry->path,
                    "tilemap package entry is missing atlas texture dependency");
    }
    if (!has_tilemap_texture_edge(index, tilemap_asset, document.atlas_page, tilemap_entry->path)) {
        add_failure(result.failures, document.atlas_page, tilemap_entry->path,
                    "tilemap_texture dependency edge is missing");
    }
    for (const auto dependency : tilemap_entry->dependencies) {
        if (dependency != document.atlas_page) {
            add_failure(result.failures, dependency, tilemap_entry->path,
                        "tilemap package entry declares a dependency that is not the atlas texture page");
        }
    }

    sort_failures(result.failures);
    return result;
}

CookedTilemapPackageUpdateResult plan_cooked_tilemap_package_update(const CookedTilemapPackageUpdateDesc& desc) {
    CookedTilemapPackageUpdateResult result;
    validate_package_relative_path(result.failures, desc.tilemap.tilemap_asset, desc.package_index_path,
                                   "tilemap package index path");
    validate_package_relative_path(result.failures, desc.tilemap.tilemap_asset, desc.tilemap.output_path,
                                   "tilemap metadata output path");
    validate_package_relative_path(result.failures, desc.tilemap.atlas_page, desc.tilemap.atlas_page_uri,
                                   "tilemap atlas texture asset_uri");
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto authored = author_cooked_tilemap_metadata(desc.tilemap);
    if (!authored.succeeded()) {
        result.failures = authored.failures;
        sort_failures(result.failures);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.tilemap.tilemap_asset, desc.package_index_path,
                    std::string{"tilemap package index is invalid: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    if (has_path_collision(index, desc.tilemap.tilemap_asset, desc.tilemap.output_path)) {
        add_failure(result.failures, desc.tilemap.tilemap_asset, desc.tilemap.output_path,
                    "tilemap metadata output path collides with another package entry");
    }

    const auto* page_entry = find_entry(index, desc.tilemap.atlas_page);
    if (page_entry == nullptr) {
        add_failure(result.failures, desc.tilemap.atlas_page, desc.tilemap.atlas_page_uri,
                    "tilemap atlas texture package entry is missing");
    } else {
        if (page_entry->kind != AssetKind::texture) {
            add_failure(result.failures, desc.tilemap.atlas_page, page_entry->path,
                        "tilemap atlas texture package entry is not a texture");
        }
        if (page_entry->path != desc.tilemap.atlas_page_uri) {
            add_failure(result.failures, desc.tilemap.atlas_page, page_entry->path,
                        "tilemap atlas texture package path does not match atlas page asset_uri");
        }
    }

    auto* tilemap_entry = find_entry(index, desc.tilemap.tilemap_asset);
    if (tilemap_entry != nullptr && tilemap_entry->kind != AssetKind::tilemap) {
        add_failure(result.failures, desc.tilemap.tilemap_asset, tilemap_entry->path,
                    "tilemap package entry kind must be tilemap");
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (tilemap_entry == nullptr) {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = authored.artifact.asset,
            .kind = authored.artifact.kind,
            .path = authored.artifact.path,
            .content_hash = hash_asset_cooked_content(authored.content),
            .source_revision = authored.artifact.source_revision,
            .dependencies = authored.artifact.dependencies,
        });
    } else {
        tilemap_entry->kind = authored.artifact.kind;
        tilemap_entry->path = authored.artifact.path;
        tilemap_entry->content_hash = hash_asset_cooked_content(authored.content);
        tilemap_entry->source_revision = authored.artifact.source_revision;
        tilemap_entry->dependencies = authored.artifact.dependencies;
    }

    const auto removed_tilemap_deps = std::ranges::remove_if(
        index.dependencies,
        [asset = desc.tilemap.tilemap_asset](const AssetDependencyEdge& edge) { return edge.asset == asset; });
    index.dependencies.erase(removed_tilemap_deps.begin(), removed_tilemap_deps.end());
    index.dependencies.insert(index.dependencies.end(), authored.dependency_edges.begin(),
                              authored.dependency_edges.end());
    canonicalize_package_index(index);

    const auto verification =
        verify_cooked_tilemap_package_metadata(index, desc.tilemap.tilemap_asset, authored.content);
    if (!verification.succeeded()) {
        result.failures = verification.failures;
        sort_failures(result.failures);
        return result;
    }

    result.tilemap_content = authored.content;
    result.package_index_content = serialize_asset_cooked_package_index(index);
    append_changed_file(result.changed_files, desc.tilemap.output_path, result.tilemap_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

CookedTilemapPackageUpdateResult apply_cooked_tilemap_package_update(IFileSystem& filesystem,
                                                                     const CookedTilemapPackageApplyDesc& desc) {
    CookedTilemapPackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.tilemap.tilemap_asset, desc.package_index_path,
                    std::string{"failed to read tilemap package index: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    result = plan_cooked_tilemap_package_update(CookedTilemapPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .tilemap = desc.tilemap,
    });
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.tilemap.output_path, result.tilemap_content);
        filesystem.write_text(desc.package_index_path, result.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.tilemap.tilemap_asset, desc.package_index_path,
                    std::string{"failed to write tilemap package update: "} + error.what());
        sort_failures(result.failures);
    }
    return result;
}

} // namespace mirakana
