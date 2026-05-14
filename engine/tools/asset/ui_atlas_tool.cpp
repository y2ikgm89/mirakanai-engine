// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/ui_atlas_tool.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>

namespace mirakana {
namespace {

constexpr std::string_view packed_ui_atlas_source_decoding = "decoded-image-adapter";
constexpr std::string_view packed_ui_atlas_packing = "deterministic-sprite-atlas-rgba8-max-side";
constexpr std::string_view packed_ui_glyph_atlas_source_decoding = "rasterized-glyph-adapter";
constexpr std::string_view packed_ui_glyph_atlas_packing = "deterministic-glyph-atlas-rgba8-max-side";

void add_failure(std::vector<CookedUiAtlasAuthoringFailure>& failures, AssetId asset, std::string path,
                 std::string diagnostic) {
    failures.push_back(CookedUiAtlasAuthoringFailure{
        .asset = asset,
        .path = std::move(path),
        .diagnostic = std::move(diagnostic),
    });
}

void sort_failures(std::vector<CookedUiAtlasAuthoringFailure>& failures) {
    std::ranges::sort(failures, [](const CookedUiAtlasAuthoringFailure& lhs, const CookedUiAtlasAuthoringFailure& rhs) {
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
    if (path.empty() || path.find('\0') != std::string_view::npos || path.find('\n') != std::string_view::npos ||
        path.find('\r') != std::string_view::npos || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos || path.front() == '/') {
        return false;
    }

    std::size_t segment_start = 0;
    while (segment_start <= path.size()) {
        const auto segment_end = path.find('/', segment_start);
        const auto segment =
            path.substr(segment_start, segment_end == std::string_view::npos ? path.size() - segment_start
                                                                             : segment_end - segment_start);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (segment.find("..") != std::string_view::npos) {
            return false;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_start = segment_end + 1U;
    }
    return true;
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

[[nodiscard]] UiAtlasMetadataDocument make_document(const CookedUiAtlasAuthoringDesc& desc) {
    UiAtlasMetadataDocument document;
    document.asset = desc.atlas_asset;
    document.source_decoding = desc.source_decoding;
    document.atlas_packing = desc.atlas_packing;
    document.pages = desc.pages;
    document.images.reserve(desc.images.size());
    for (const auto& image : desc.images) {
        document.images.push_back(UiAtlasMetadataImage{
            .resource_id = image.resource_id,
            .asset_uri = image.asset_uri,
            .page = image.page,
            .uv = UiAtlasUvRect{.u0 = image.u0, .v0 = image.v0, .u1 = image.u1, .v1 = image.v1},
            .color = image.color,
        });
    }
    document.glyphs.reserve(desc.glyphs.size());
    for (const auto& glyph : desc.glyphs) {
        document.glyphs.push_back(UiAtlasMetadataGlyph{
            .font_family = glyph.font_family,
            .glyph = glyph.glyph,
            .page = glyph.page,
            .uv = UiAtlasUvRect{.u0 = glyph.u0, .v0 = glyph.v0, .u1 = glyph.u1, .v1 = glyph.v1},
            .color = glyph.color,
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

[[nodiscard]] bool has_ui_atlas_texture_edge(const AssetCookedPackageIndex& index, AssetId atlas_asset,
                                             AssetId texture_asset, std::string_view atlas_path) noexcept {
    return std::ranges::any_of(index.dependencies,
                               [atlas_asset, texture_asset, atlas_path](const AssetDependencyEdge& edge) {
                                   return edge.asset == atlas_asset && edge.dependency == texture_asset &&
                                          edge.kind == AssetDependencyKind::ui_atlas_texture && edge.path == atlas_path;
                               });
}

void append_changed_file(std::vector<CookedUiAtlasPackageChangedFile>& files, std::string path, std::string content) {
    files.push_back(CookedUiAtlasPackageChangedFile{
        .path = std::move(path),
        .content = std::move(content),
        .content_hash = 0,
    });
    files.back().content_hash = hash_asset_cooked_content(files.back().content);
}

struct ChangedFileSnapshot {
    std::string path;
    bool existed{false};
    std::string content;
};

struct ChangedFileTransactionSnapshot {
    std::vector<ChangedFileSnapshot> files;
    std::vector<std::string> created_directories;
};

void append_unique_directory(std::vector<std::string>& directories, std::string directory) {
    if (std::ranges::find(directories, directory) == directories.end()) {
        directories.push_back(std::move(directory));
    }
}

void append_missing_parent_directories(IFileSystem& filesystem, std::vector<std::string>& directories,
                                       std::string_view path) {
    for (std::size_t position = path.find('/'); position != std::string_view::npos;
         position = path.find('/', position + 1U)) {
        const auto directory = std::string{path.substr(0, position)};
        if (!directory.empty() && !filesystem.exists(directory)) {
            append_unique_directory(directories, directory);
        }
    }
}

[[nodiscard]] std::vector<ChangedFileSnapshot>
snapshot_changed_files(IFileSystem& filesystem, const std::vector<CookedUiAtlasPackageChangedFile>& changed_files) {
    std::vector<ChangedFileSnapshot> snapshots;
    snapshots.reserve(changed_files.size());
    for (const auto& changed : changed_files) {
        ChangedFileSnapshot snapshot;
        snapshot.path = changed.path;
        snapshot.existed = filesystem.exists(changed.path);
        if (snapshot.existed) {
            snapshot.content = filesystem.read_text(changed.path);
        }
        snapshots.push_back(std::move(snapshot));
    }
    return snapshots;
}

[[nodiscard]] ChangedFileTransactionSnapshot
snapshot_changed_file_transaction(IFileSystem& filesystem,
                                  const std::vector<CookedUiAtlasPackageChangedFile>& changed_files) {
    ChangedFileTransactionSnapshot snapshot;
    snapshot.files = snapshot_changed_files(filesystem, changed_files);
    for (const auto& changed : changed_files) {
        append_missing_parent_directories(filesystem, snapshot.created_directories, changed.path);
    }
    return snapshot;
}

void restore_changed_file(IFileSystem& filesystem, const ChangedFileSnapshot& snapshot) {
    if (snapshot.existed) {
        filesystem.write_text(snapshot.path, snapshot.content);
    } else {
        filesystem.remove(snapshot.path);
    }
}

void remove_created_directories(IFileSystem& filesystem, const std::vector<std::string>& directories,
                                std::string& rollback_diagnostic) {
    for (const auto& directory : std::views::reverse(directories)) {
        try {
            filesystem.remove_empty_directory(directory);
        } catch (const std::exception& error) {
            if (!rollback_diagnostic.empty()) {
                rollback_diagnostic += "; ";
            }
            rollback_diagnostic += error.what();
        }
    }
}

void write_changed_files_transactionally(IFileSystem& filesystem,
                                         const std::vector<CookedUiAtlasPackageChangedFile>& changed_files) {
    const auto snapshot = snapshot_changed_file_transaction(filesystem, changed_files);
    std::vector<std::size_t> attempted_indices;
    attempted_indices.reserve(changed_files.size());
    try {
        for (std::size_t i = 0; i < changed_files.size(); ++i) {
            attempted_indices.push_back(i);
            const auto& changed = changed_files[i];
            try {
                filesystem.write_text(changed.path, changed.content);
            } catch (const std::exception& error) {
                throw std::runtime_error("failed writing " + changed.path + ": " + error.what());
            }
        }
    } catch (const std::exception& error) {
        std::string diagnostic = error.what();
        std::string rollback_diagnostic;
        for (const auto attempted_index : std::views::reverse(attempted_indices)) {
            try {
                restore_changed_file(filesystem, snapshot.files[attempted_index]);
            } catch (const std::exception& rollback_error) {
                if (!rollback_diagnostic.empty()) {
                    rollback_diagnostic += "; ";
                }
                rollback_diagnostic += rollback_error.what();
            }
        }
        remove_created_directories(filesystem, snapshot.created_directories, rollback_diagnostic);
        if (!rollback_diagnostic.empty()) {
            diagnostic += "; rollback failed: ";
            diagnostic += rollback_diagnostic;
        }
        throw std::runtime_error(diagnostic);
    }
}

[[nodiscard]] char hex_digit(std::uint8_t value) noexcept {
    return static_cast<char>(value < 10U ? '0' + value : 'a' + (value - 10U));
}

[[nodiscard]] std::string encode_hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        encoded.push_back(hex_digit(static_cast<std::uint8_t>(byte >> 4U)));
        encoded.push_back(hex_digit(static_cast<std::uint8_t>(byte & 0x0FU)));
    }
    return encoded;
}

[[nodiscard]] std::string cooked_texture_payload(AssetId asset, std::string_view source_path,
                                                 const TextureSourceDocument& texture) {
    std::ostringstream output;
    output << "format=GameEngine.CookedTexture.v1\n";
    output << "asset.id=" << asset.value << '\n';
    output << "asset.kind=texture\n";
    output << "source.path=" << source_path << '\n';
    output << "texture.width=" << texture.width << '\n';
    output << "texture.height=" << texture.height << '\n';
    output << "texture.pixel_format=" << texture_source_pixel_format_name(texture.pixel_format) << '\n';
    output << "texture.source_bytes=" << texture_source_uncompressed_bytes(texture) << '\n';
    if (!texture.bytes.empty()) {
        output << "texture.data_hex=" << encode_hex_bytes(texture.bytes) << '\n';
    }
    return output.str();
}

void validate_package_relative_path(std::vector<CookedUiAtlasAuthoringFailure>& failures, AssetId asset,
                                    std::string_view path, std::string_view field) {
    if (!valid_output_path(path)) {
        add_failure(failures, asset, std::string{path}, std::string{field} + " must be a package-relative safe path");
    }
}

void validate_distinct_package_paths(std::vector<CookedUiAtlasAuthoringFailure>& failures, AssetId asset,
                                     std::string_view path, std::string_view other_path, std::string_view diagnostic) {
    if (!path.empty() && path == other_path) {
        add_failure(failures, asset, std::string{path}, std::string{diagnostic});
    }
}

void validate_package_index_entry_paths(std::vector<CookedUiAtlasAuthoringFailure>& failures,
                                        const AssetCookedPackageIndex& index) {
    for (const auto& entry : index.entries) {
        validate_package_relative_path(failures, entry.asset, entry.path, "ui atlas package index entry path");
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

[[nodiscard]] bool expected_rgba8_size(std::uint32_t width, std::uint32_t height, std::size_t& out) noexcept {
    const auto bytes = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) * 4ULL;
    if (bytes > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        return false;
    }
    out = static_cast<std::size_t>(bytes);
    return true;
}

void validate_packed_ui_atlas_authoring_desc(std::vector<CookedUiAtlasAuthoringFailure>& failures,
                                             const PackedUiAtlasAuthoringDesc& desc) {
    validate_package_relative_path(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                                   "ui atlas metadata output path");
    validate_package_relative_path(failures, desc.atlas_page_asset, desc.atlas_page_output_path,
                                   "ui atlas page output path");
    if (desc.source_revision == 0) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui atlas source revision must be non-zero");
    }
    if (desc.source_decoding != packed_ui_atlas_source_decoding) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui atlas source decoding must be decoded-image-adapter");
    }
    if (desc.atlas_packing != packed_ui_atlas_packing) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui atlas packing must be deterministic-sprite-atlas-rgba8-max-side");
    }
    if (desc.images.empty()) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui atlas requires at least one decoded image");
    }

    for (const auto& image : desc.images) {
        if (image.resource_id.empty() || image.asset_uri.empty()) {
            add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                        "decoded UI image identity is missing");
        }
        if (image.decoded_image.pixel_format != ui::ImageDecodePixelFormat::rgba8_unorm) {
            add_failure(failures, desc.atlas_asset, image.asset_uri, "decoded image must be RGBA8");
            continue;
        }

        std::size_t expected_bytes = 0;
        const auto size_is_safe =
            expected_rgba8_size(image.decoded_image.width, image.decoded_image.height, expected_bytes);
        if (!size_is_safe || image.decoded_image.width == 0 || image.decoded_image.height == 0 ||
            image.decoded_image.pixels.size() != expected_bytes) {
            add_failure(failures, desc.atlas_asset, image.asset_uri,
                        "decoded image pixel byte count must equal width*height*4");
        }
    }
}

void validate_packed_ui_glyph_atlas_authoring_desc(std::vector<CookedUiAtlasAuthoringFailure>& failures,
                                                   const PackedUiGlyphAtlasAuthoringDesc& desc) {
    validate_package_relative_path(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                                   "ui glyph atlas metadata output path");
    validate_package_relative_path(failures, desc.atlas_page_asset, desc.atlas_page_output_path,
                                   "ui glyph atlas page output path");
    if (desc.source_revision == 0) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui glyph atlas source revision must be non-zero");
    }
    if (desc.source_decoding != packed_ui_glyph_atlas_source_decoding) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui glyph atlas source decoding must be rasterized-glyph-adapter");
    }
    if (desc.atlas_packing != packed_ui_glyph_atlas_packing) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui glyph atlas packing must be deterministic-glyph-atlas-rgba8-max-side");
    }
    if (desc.glyphs.empty()) {
        add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                    "ui glyph atlas requires at least one rasterized glyph");
    }

    for (const auto& glyph : desc.glyphs) {
        if (glyph.font_family.empty() || glyph.glyph == 0) {
            add_failure(failures, desc.atlas_asset, desc.atlas_metadata_output_path,
                        "rasterized glyph identity is missing");
        }
        if (glyph.rasterized_glyph.pixel_format != ui::ImageDecodePixelFormat::rgba8_unorm) {
            add_failure(failures, desc.atlas_asset, glyph.font_family, "rasterized glyph must be RGBA8");
            continue;
        }

        std::size_t expected_bytes = 0;
        const auto size_is_safe =
            expected_rgba8_size(glyph.rasterized_glyph.width, glyph.rasterized_glyph.height, expected_bytes);
        if (!size_is_safe || glyph.rasterized_glyph.width == 0 || glyph.rasterized_glyph.height == 0 ||
            glyph.rasterized_glyph.pixels.size() != expected_bytes) {
            add_failure(failures, desc.atlas_asset, glyph.font_family,
                        "rasterized glyph pixel byte count must equal width*height*4");
        }
    }
}

} // namespace

CookedUiAtlasAuthoringResult author_cooked_ui_atlas_metadata(const CookedUiAtlasAuthoringDesc& desc) {
    CookedUiAtlasAuthoringResult result;
    if (!valid_output_path(desc.output_path)) {
        add_failure(result.failures, desc.atlas_asset, desc.output_path, "ui atlas metadata output path is invalid");
    }
    if (desc.source_revision == 0) {
        add_failure(result.failures, desc.atlas_asset, desc.output_path,
                    "ui atlas metadata source revision must be non-zero");
    }

    const auto document = make_document(desc);
    for (const auto& diagnostic : validate_ui_atlas_metadata_document(document)) {
        add_failure(result.failures, diagnostic.asset.value != 0 ? diagnostic.asset : desc.atlas_asset,
                    desc.output_path, diagnostic.message);
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    result.content = serialize_ui_atlas_metadata_document(document);

    std::vector<AssetId> page_assets;
    page_assets.reserve(desc.pages.size());
    for (const auto& page : desc.pages) {
        page_assets.push_back(page.asset);
    }
    sort_asset_ids(page_assets);
    page_assets.erase(std::ranges::unique(page_assets, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }).begin(),
                      page_assets.end());

    result.dependency_edges.reserve(page_assets.size());
    for (const auto page_asset : page_assets) {
        result.dependency_edges.push_back(AssetDependencyEdge{
            .asset = desc.atlas_asset,
            .dependency = page_asset,
            .kind = AssetDependencyKind::ui_atlas_texture,
            .path = desc.output_path,
        });
    }
    sort_dependency_edges(result.dependency_edges);

    result.artifact = AssetCookedArtifact{
        .asset = desc.atlas_asset,
        .kind = AssetKind::ui_atlas,
        .path = desc.output_path,
        .content = result.content,
        .source_revision = desc.source_revision,
        .dependencies = std::move(page_assets),
    };
    return result;
}

CookedUiAtlasAuthoringResult write_cooked_ui_atlas_metadata(IFileSystem& filesystem,
                                                            const CookedUiAtlasAuthoringDesc& desc) {
    auto result = author_cooked_ui_atlas_metadata(desc);
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.output_path, result.content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas_asset, desc.output_path,
                    std::string("failed to write cooked ui atlas metadata: ") + error.what());
        sort_failures(result.failures);
    }
    return result;
}

CookedUiAtlasPackageVerificationResult verify_cooked_ui_atlas_package_metadata(const AssetCookedPackageIndex& index,
                                                                               AssetId atlas_asset,
                                                                               std::string_view atlas_content) {
    CookedUiAtlasPackageVerificationResult result;
    UiAtlasMetadataDocument document;
    try {
        document = deserialize_ui_atlas_metadata_document(atlas_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, atlas_asset, {}, std::string{"ui atlas metadata is invalid: "} + error.what());
        return result;
    }

    if (document.asset != atlas_asset) {
        add_failure(result.failures, document.asset, {}, "ui atlas metadata asset id does not match requested asset");
    }

    const auto* atlas_entry = find_entry(index, atlas_asset);
    if (atlas_entry == nullptr) {
        add_failure(result.failures, atlas_asset, {}, "ui atlas package entry is missing");
        sort_failures(result.failures);
        return result;
    }
    if (atlas_entry->kind != AssetKind::ui_atlas) {
        add_failure(result.failures, atlas_asset, atlas_entry->path, "ui atlas package entry kind must be ui_atlas");
    }
    if (atlas_entry->content_hash != hash_asset_cooked_content(atlas_content)) {
        add_failure(result.failures, atlas_asset, atlas_entry->path,
                    "ui atlas content hash does not match package entry");
    }

    std::unordered_set<AssetId, AssetIdHash> page_assets;
    for (const auto& page : document.pages) {
        page_assets.insert(page.asset);
        const auto* page_entry = find_entry(index, page.asset);
        if (page_entry == nullptr) {
            add_failure(result.failures, page.asset, page.asset_uri, "ui atlas texture page package entry is missing");
            continue;
        }
        if (page_entry->kind != AssetKind::texture) {
            add_failure(result.failures, page.asset, page_entry->path,
                        "ui atlas texture page package entry is not a texture");
        }
        if (page_entry->path != page.asset_uri) {
            add_failure(result.failures, page.asset, page_entry->path,
                        "ui atlas texture page asset_uri does not match package path");
        }
        if (!has_dependency(*atlas_entry, page.asset)) {
            add_failure(result.failures, page.asset, atlas_entry->path,
                        "ui atlas package entry is missing texture dependency");
        }
        if (!has_ui_atlas_texture_edge(index, atlas_asset, page.asset, atlas_entry->path)) {
            add_failure(result.failures, page.asset, atlas_entry->path, "ui_atlas_texture dependency edge is missing");
        }
    }

    for (const auto dependency : atlas_entry->dependencies) {
        if (page_assets.find(dependency) == page_assets.end()) {
            add_failure(result.failures, dependency, atlas_entry->path,
                        "ui atlas package entry declares a dependency that is not a texture page");
        }
    }

    sort_failures(result.failures);
    return result;
}

CookedUiAtlasPackageUpdateResult plan_cooked_ui_atlas_package_update(const CookedUiAtlasPackageUpdateDesc& desc) {
    CookedUiAtlasPackageUpdateResult result;
    validate_package_relative_path(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                   "ui atlas package index path");
    validate_package_relative_path(result.failures, desc.atlas.atlas_asset, desc.atlas.output_path,
                                   "ui atlas metadata output path");
    validate_distinct_package_paths(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                    desc.atlas.output_path,
                                    "ui atlas package index path must not alias metadata output path");
    for (const auto& page : desc.atlas.pages) {
        validate_package_relative_path(result.failures, page.asset, page.asset_uri, "ui atlas texture page asset_uri");
        validate_distinct_package_paths(result.failures, page.asset, desc.package_index_path, page.asset_uri,
                                        "ui atlas package index path must not alias texture page asset_uri");
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto authored = author_cooked_ui_atlas_metadata(desc.atlas);
    if (!authored.succeeded()) {
        result.failures = authored.failures;
        sort_failures(result.failures);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"ui atlas package index is invalid: "} + error.what());
        sort_failures(result.failures);
        return result;
    }
    validate_package_index_entry_paths(result.failures, index);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (has_path_collision(index, desc.atlas.atlas_asset, desc.atlas.output_path)) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.atlas.output_path,
                    "ui atlas metadata output path collides with another package entry");
    }

    for (const auto& page : desc.atlas.pages) {
        const auto* page_entry = find_entry(index, page.asset);
        if (page_entry == nullptr) {
            add_failure(result.failures, page.asset, page.asset_uri, "ui atlas texture page package entry is missing");
            continue;
        }
        if (page_entry->kind != AssetKind::texture) {
            add_failure(result.failures, page.asset, page_entry->path,
                        "ui atlas texture page package entry is not a texture");
        }
        if (page_entry->path != page.asset_uri) {
            add_failure(result.failures, page.asset, page_entry->path,
                        "ui atlas texture page package path does not match atlas page asset_uri");
        }
    }

    auto* atlas_entry = find_entry(index, desc.atlas.atlas_asset);
    if (atlas_entry != nullptr && atlas_entry->kind != AssetKind::ui_atlas) {
        add_failure(result.failures, desc.atlas.atlas_asset, atlas_entry->path,
                    "ui atlas package entry kind must be ui_atlas");
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (atlas_entry == nullptr) {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = authored.artifact.asset,
            .kind = authored.artifact.kind,
            .path = authored.artifact.path,
            .content_hash = hash_asset_cooked_content(authored.content),
            .source_revision = authored.artifact.source_revision,
            .dependencies = authored.artifact.dependencies,
        });
    } else {
        atlas_entry->kind = authored.artifact.kind;
        atlas_entry->path = authored.artifact.path;
        atlas_entry->content_hash = hash_asset_cooked_content(authored.content);
        atlas_entry->source_revision = authored.artifact.source_revision;
        atlas_entry->dependencies = authored.artifact.dependencies;
    }

    const auto removed_atlas_deps =
        std::ranges::remove_if(index.dependencies, [asset = desc.atlas.atlas_asset](const AssetDependencyEdge& edge) {
            return edge.asset == asset;
        });
    index.dependencies.erase(removed_atlas_deps.begin(), removed_atlas_deps.end());
    index.dependencies.insert(index.dependencies.end(), authored.dependency_edges.begin(),
                              authored.dependency_edges.end());
    canonicalize_package_index(index);

    const auto verification = verify_cooked_ui_atlas_package_metadata(index, desc.atlas.atlas_asset, authored.content);
    if (!verification.succeeded()) {
        result.failures = verification.failures;
        sort_failures(result.failures);
        return result;
    }

    result.atlas_content = authored.content;
    result.package_index_content = serialize_asset_cooked_package_index(index);
    append_changed_file(result.changed_files, desc.atlas.output_path, result.atlas_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

CookedUiAtlasPackageUpdateResult apply_cooked_ui_atlas_package_update(IFileSystem& filesystem,
                                                                      const CookedUiAtlasPackageApplyDesc& desc) {
    CookedUiAtlasPackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to read ui atlas package index: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    result = plan_cooked_ui_atlas_package_update(CookedUiAtlasPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .atlas = desc.atlas,
    });
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.atlas.output_path, result.atlas_content);
        filesystem.write_text(desc.package_index_path, result.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to write ui atlas package update: "} + error.what());
        sort_failures(result.failures);
    }
    return result;
}

PackedUiAtlasAuthoringResult author_packed_ui_atlas_from_decoded_images(const PackedUiAtlasAuthoringDesc& desc) {
    PackedUiAtlasAuthoringResult result;
    validate_packed_ui_atlas_authoring_desc(result.failures, desc);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    std::vector<std::vector<std::uint8_t>> rgba_pixels;
    rgba_pixels.reserve(desc.images.size());
    std::vector<SpriteAtlasPackingItemView> packing_items;
    packing_items.reserve(desc.images.size());
    for (const auto& image : desc.images) {
        auto& pixels = rgba_pixels.emplace_back();
        pixels.reserve(image.decoded_image.pixels.size());
        for (const auto pixel : image.decoded_image.pixels) {
            pixels.push_back(static_cast<std::uint8_t>(pixel));
        }
        packing_items.push_back(SpriteAtlasPackingItemView{
            .width = image.decoded_image.width,
            .height = image.decoded_image.height,
            .rgba8_pixels = std::span<const std::uint8_t>{pixels},
        });
    }

    const auto packed_variant = pack_sprite_atlas_rgba8_max_side(packing_items, desc.max_side);
    const auto* packed = std::get_if<SpriteAtlasRgba8PackingOutput>(&packed_variant);
    if (packed == nullptr) {
        const auto& diagnostic = std::get<SpriteAtlasPackingDiagnostic>(packed_variant);
        add_failure(result.failures, desc.atlas_asset, desc.atlas_metadata_output_path, diagnostic.message);
        sort_failures(result.failures);
        return result;
    }

    result.atlas_texture = packed->atlas;
    result.atlas_texture_content =
        cooked_texture_payload(desc.atlas_page_asset, desc.atlas_metadata_output_path, result.atlas_texture);
    result.atlas_page_artifact = AssetCookedArtifact{
        .asset = desc.atlas_page_asset,
        .kind = AssetKind::texture,
        .path = desc.atlas_page_output_path,
        .content = result.atlas_texture_content,
        .source_revision = desc.source_revision,
        .dependencies = {},
    };

    CookedUiAtlasAuthoringDesc metadata;
    metadata.atlas_asset = desc.atlas_asset;
    metadata.output_path = desc.atlas_metadata_output_path;
    metadata.source_revision = desc.source_revision;
    metadata.source_decoding = desc.source_decoding;
    metadata.atlas_packing = desc.atlas_packing;
    metadata.pages.push_back(CookedUiAtlasPageDesc{
        desc.atlas_page_asset,
        desc.atlas_page_output_path,
    });
    metadata.images.reserve(desc.images.size());
    for (std::size_t i = 0; i < desc.images.size(); ++i) {
        const auto& image = desc.images[i];
        const auto& placement = packed->placements[i];
        const auto atlas_width = static_cast<float>(packed->atlas.width);
        const auto atlas_height = static_cast<float>(packed->atlas.height);
        metadata.images.push_back(CookedUiAtlasImageDesc{
            .resource_id = image.resource_id,
            .asset_uri = image.asset_uri,
            .page = desc.atlas_page_asset,
            .u0 = static_cast<float>(placement.x) / atlas_width,
            .v0 = static_cast<float>(placement.y) / atlas_height,
            .u1 = static_cast<float>(placement.x + placement.width) / atlas_width,
            .v1 = static_cast<float>(placement.y + placement.height) / atlas_height,
            .color = image.color,
        });
    }

    result.atlas_metadata_desc = std::move(metadata);
    result.atlas_metadata = author_cooked_ui_atlas_metadata(result.atlas_metadata_desc);
    if (!result.atlas_metadata.succeeded()) {
        result.failures = result.atlas_metadata.failures;
        sort_failures(result.failures);
        return result;
    }
    result.atlas_metadata_content = result.atlas_metadata.content;
    return result;
}

PackedUiAtlasPackageUpdateResult plan_packed_ui_atlas_package_update(const PackedUiAtlasPackageUpdateDesc& desc) {
    PackedUiAtlasPackageUpdateResult result;
    validate_package_relative_path(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                   "ui atlas package index path");
    validate_distinct_package_paths(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                    desc.atlas.atlas_metadata_output_path,
                                    "ui atlas package index path must not alias metadata output path");
    validate_distinct_package_paths(result.failures, desc.atlas.atlas_page_asset, desc.package_index_path,
                                    desc.atlas.atlas_page_output_path,
                                    "ui atlas package index path must not alias page output path");
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto authored = author_packed_ui_atlas_from_decoded_images(desc.atlas);
    if (!authored.succeeded()) {
        result.failures = authored.failures;
        sort_failures(result.failures);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"packed ui atlas package index is invalid: "} + error.what());
        sort_failures(result.failures);
        return result;
    }
    validate_package_index_entry_paths(result.failures, index);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (has_path_collision(index, desc.atlas.atlas_page_asset, desc.atlas.atlas_page_output_path)) {
        add_failure(result.failures, desc.atlas.atlas_page_asset, desc.atlas.atlas_page_output_path,
                    "ui atlas page output path collides with another package entry");
    }
    auto* page_entry = find_entry(index, desc.atlas.atlas_page_asset);
    if (page_entry != nullptr && page_entry->kind != AssetKind::texture) {
        add_failure(result.failures, desc.atlas.atlas_page_asset, page_entry->path,
                    "ui atlas page package entry kind must be texture");
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (page_entry == nullptr) {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = authored.atlas_page_artifact.asset,
            .kind = authored.atlas_page_artifact.kind,
            .path = authored.atlas_page_artifact.path,
            .content_hash = hash_asset_cooked_content(authored.atlas_page_artifact.content),
            .source_revision = authored.atlas_page_artifact.source_revision,
            .dependencies = authored.atlas_page_artifact.dependencies,
        });
    } else {
        page_entry->kind = authored.atlas_page_artifact.kind;
        page_entry->path = authored.atlas_page_artifact.path;
        page_entry->content_hash = hash_asset_cooked_content(authored.atlas_page_artifact.content);
        page_entry->source_revision = authored.atlas_page_artifact.source_revision;
        page_entry->dependencies = authored.atlas_page_artifact.dependencies;
    }
    canonicalize_package_index(index);

    const auto metadata_update = plan_cooked_ui_atlas_package_update(CookedUiAtlasPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = serialize_asset_cooked_package_index(index),
        .atlas = authored.atlas_metadata_desc,
    });
    if (!metadata_update.succeeded()) {
        result.failures = metadata_update.failures;
        sort_failures(result.failures);
        return result;
    }

    result.atlas_texture = authored.atlas_texture;
    result.atlas_texture_content = authored.atlas_texture_content;
    result.atlas_metadata_content = metadata_update.atlas_content;
    result.package_index_content = metadata_update.package_index_content;
    append_changed_file(result.changed_files, desc.atlas.atlas_page_output_path, result.atlas_texture_content);
    result.changed_files.insert(result.changed_files.end(), metadata_update.changed_files.begin(),
                                metadata_update.changed_files.end());
    return result;
}

PackedUiAtlasPackageUpdateResult apply_packed_ui_atlas_package_update(IFileSystem& filesystem,
                                                                      const PackedUiAtlasPackageApplyDesc& desc) {
    PackedUiAtlasPackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to read packed ui atlas package index: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    result = plan_packed_ui_atlas_package_update(PackedUiAtlasPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .atlas = desc.atlas,
    });
    if (!result.succeeded()) {
        return result;
    }

    try {
        write_changed_files_transactionally(filesystem, result.changed_files);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to write packed ui atlas package update: "} + error.what());
        sort_failures(result.failures);
    }
    return result;
}

PackedUiGlyphAtlasAuthoringResult
author_packed_ui_glyph_atlas_from_rasterized_glyphs(const PackedUiGlyphAtlasAuthoringDesc& desc) {
    PackedUiGlyphAtlasAuthoringResult result;
    validate_packed_ui_glyph_atlas_authoring_desc(result.failures, desc);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    std::vector<std::vector<std::uint8_t>> rgba_pixels;
    rgba_pixels.reserve(desc.glyphs.size());
    std::vector<std::size_t> glyph_order;
    glyph_order.reserve(desc.glyphs.size());
    for (std::size_t i = 0; i < desc.glyphs.size(); ++i) {
        glyph_order.push_back(i);
    }
    std::ranges::sort(glyph_order, [&desc](std::size_t lhs_index, std::size_t rhs_index) {
        const auto& lhs = desc.glyphs[lhs_index];
        const auto& rhs = desc.glyphs[rhs_index];
        if (lhs.font_family != rhs.font_family) {
            return lhs.font_family < rhs.font_family;
        }
        if (lhs.glyph != rhs.glyph) {
            return lhs.glyph < rhs.glyph;
        }
        return lhs_index < rhs_index;
    });

    std::vector<SpriteAtlasPackingItemView> packing_items;
    packing_items.reserve(desc.glyphs.size());
    for (const auto glyph_index : glyph_order) {
        const auto& glyph = desc.glyphs[glyph_index];
        auto& pixels = rgba_pixels.emplace_back();
        pixels.reserve(glyph.rasterized_glyph.pixels.size());
        for (const auto pixel : glyph.rasterized_glyph.pixels) {
            pixels.push_back(static_cast<std::uint8_t>(pixel));
        }
        packing_items.push_back(SpriteAtlasPackingItemView{
            .width = glyph.rasterized_glyph.width,
            .height = glyph.rasterized_glyph.height,
            .rgba8_pixels = std::span<const std::uint8_t>{pixels},
        });
    }

    const auto packed_variant = pack_sprite_atlas_rgba8_max_side(packing_items, desc.max_side);
    const auto* packed = std::get_if<SpriteAtlasRgba8PackingOutput>(&packed_variant);
    if (packed == nullptr) {
        const auto& diagnostic = std::get<SpriteAtlasPackingDiagnostic>(packed_variant);
        add_failure(result.failures, desc.atlas_asset, desc.atlas_metadata_output_path, diagnostic.message);
        sort_failures(result.failures);
        return result;
    }

    result.atlas_texture = packed->atlas;
    result.atlas_texture_content =
        cooked_texture_payload(desc.atlas_page_asset, desc.atlas_metadata_output_path, result.atlas_texture);
    result.atlas_page_artifact = AssetCookedArtifact{
        .asset = desc.atlas_page_asset,
        .kind = AssetKind::texture,
        .path = desc.atlas_page_output_path,
        .content = result.atlas_texture_content,
        .source_revision = desc.source_revision,
        .dependencies = {},
    };

    CookedUiAtlasAuthoringDesc metadata;
    metadata.atlas_asset = desc.atlas_asset;
    metadata.output_path = desc.atlas_metadata_output_path;
    metadata.source_revision = desc.source_revision;
    metadata.source_decoding = desc.source_decoding;
    metadata.atlas_packing = desc.atlas_packing;
    metadata.pages.push_back(CookedUiAtlasPageDesc{
        desc.atlas_page_asset,
        desc.atlas_page_output_path,
    });
    metadata.glyphs.reserve(glyph_order.size());
    for (std::size_t i = 0; i < glyph_order.size(); ++i) {
        const auto& glyph = desc.glyphs[glyph_order[i]];
        const auto& placement = packed->placements[i];
        const auto atlas_width = static_cast<float>(packed->atlas.width);
        const auto atlas_height = static_cast<float>(packed->atlas.height);
        metadata.glyphs.push_back(CookedUiAtlasGlyphDesc{
            .font_family = glyph.font_family,
            .glyph = glyph.glyph,
            .page = desc.atlas_page_asset,
            .u0 = static_cast<float>(placement.x) / atlas_width,
            .v0 = static_cast<float>(placement.y) / atlas_height,
            .u1 = static_cast<float>(placement.x + placement.width) / atlas_width,
            .v1 = static_cast<float>(placement.y + placement.height) / atlas_height,
            .color = glyph.color,
        });
    }

    result.atlas_metadata_desc = std::move(metadata);
    result.atlas_metadata = author_cooked_ui_atlas_metadata(result.atlas_metadata_desc);
    if (!result.atlas_metadata.succeeded()) {
        result.failures = result.atlas_metadata.failures;
        sort_failures(result.failures);
        return result;
    }
    result.atlas_metadata_content = result.atlas_metadata.content;
    return result;
}

PackedUiGlyphAtlasPackageUpdateResult
plan_packed_ui_glyph_atlas_package_update(const PackedUiGlyphAtlasPackageUpdateDesc& desc) {
    PackedUiGlyphAtlasPackageUpdateResult result;
    validate_package_relative_path(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                   "ui glyph atlas package index path");
    validate_distinct_package_paths(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                                    desc.atlas.atlas_metadata_output_path,
                                    "ui glyph atlas package index path must not alias metadata output path");
    validate_distinct_package_paths(result.failures, desc.atlas.atlas_page_asset, desc.package_index_path,
                                    desc.atlas.atlas_page_output_path,
                                    "ui glyph atlas package index path must not alias page output path");
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto authored = author_packed_ui_glyph_atlas_from_rasterized_glyphs(desc.atlas);
    if (!authored.succeeded()) {
        result.failures = authored.failures;
        sort_failures(result.failures);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"ui glyph atlas package index is invalid: "} + error.what());
        sort_failures(result.failures);
        return result;
    }
    validate_package_index_entry_paths(result.failures, index);
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (has_path_collision(index, desc.atlas.atlas_page_asset, desc.atlas.atlas_page_output_path)) {
        add_failure(result.failures, desc.atlas.atlas_page_asset, desc.atlas.atlas_page_output_path,
                    "ui glyph atlas page output path collides with another package entry");
    }
    auto* page_entry = find_entry(index, desc.atlas.atlas_page_asset);
    if (page_entry != nullptr && page_entry->kind != AssetKind::texture) {
        add_failure(result.failures, desc.atlas.atlas_page_asset, page_entry->path,
                    "ui glyph atlas page package entry kind must be texture");
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    if (page_entry == nullptr) {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = authored.atlas_page_artifact.asset,
            .kind = authored.atlas_page_artifact.kind,
            .path = authored.atlas_page_artifact.path,
            .content_hash = hash_asset_cooked_content(authored.atlas_page_artifact.content),
            .source_revision = authored.atlas_page_artifact.source_revision,
            .dependencies = authored.atlas_page_artifact.dependencies,
        });
    } else {
        page_entry->kind = authored.atlas_page_artifact.kind;
        page_entry->path = authored.atlas_page_artifact.path;
        page_entry->content_hash = hash_asset_cooked_content(authored.atlas_page_artifact.content);
        page_entry->source_revision = authored.atlas_page_artifact.source_revision;
        page_entry->dependencies = authored.atlas_page_artifact.dependencies;
    }
    canonicalize_package_index(index);

    const auto metadata_update = plan_cooked_ui_atlas_package_update(CookedUiAtlasPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = serialize_asset_cooked_package_index(index),
        .atlas = authored.atlas_metadata_desc,
    });
    if (!metadata_update.succeeded()) {
        result.failures = metadata_update.failures;
        sort_failures(result.failures);
        return result;
    }

    result.atlas_texture = authored.atlas_texture;
    result.atlas_texture_content = authored.atlas_texture_content;
    result.atlas_metadata_content = metadata_update.atlas_content;
    result.package_index_content = metadata_update.package_index_content;
    append_changed_file(result.changed_files, desc.atlas.atlas_page_output_path, result.atlas_texture_content);
    result.changed_files.insert(result.changed_files.end(), metadata_update.changed_files.begin(),
                                metadata_update.changed_files.end());
    return result;
}

PackedUiGlyphAtlasPackageUpdateResult
apply_packed_ui_glyph_atlas_package_update(IFileSystem& filesystem, const PackedUiGlyphAtlasPackageApplyDesc& desc) {
    PackedUiGlyphAtlasPackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to read packed ui glyph atlas package index: "} + error.what());
        sort_failures(result.failures);
        return result;
    }

    result = plan_packed_ui_glyph_atlas_package_update(PackedUiGlyphAtlasPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .atlas = desc.atlas,
    });
    if (!result.succeeded()) {
        return result;
    }

    try {
        write_changed_files_transactionally(filesystem, result.changed_files);
    } catch (const std::exception& error) {
        add_failure(result.failures, desc.atlas.atlas_asset, desc.package_index_path,
                    std::string{"failed to write packed ui glyph atlas package update: "} + error.what());
        sort_failures(result.failures);
    }
    return result;
}

} // namespace mirakana
