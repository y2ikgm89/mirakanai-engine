// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/placeholder_asset_tool.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/tools/registered_source_asset_cook_package_tool.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view placeholder_generator_id = "mirakana-placeholder-asset-tool-v1";
constexpr std::string_view proprietary_license_id = "LicenseRef-Proprietary";
constexpr std::uint32_t max_placeholder_texture_extent = 4096;
constexpr std::uint64_t max_placeholder_audio_frame_count = 384000;

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto code = static_cast<unsigned char>(character);
        return code < 0x20U || code == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find(';') != std::string_view::npos ||
        has_control_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool is_placeholder_supported_kind(AssetKind kind) noexcept {
    return kind == AssetKind::texture || kind == AssetKind::mesh || kind == AssetKind::material ||
           kind == AssetKind::audio;
}

void add_diagnostic(std::vector<PlaceholderAssetDiagnostic>& diagnostics, std::string code, std::string message,
                    std::string path = {}, AssetKeyV2 key = {}) {
    diagnostics.push_back(PlaceholderAssetDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .asset_key = std::move(key),
    });
}

void sort_diagnostics(std::vector<PlaceholderAssetDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const PlaceholderAssetDiagnostic& lhs, const PlaceholderAssetDiagnostic& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset_key.value != rhs.asset_key.value) {
            return lhs.asset_key.value < rhs.asset_key.value;
        }
        if (lhs.code != rhs.code) {
            return lhs.code < rhs.code;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] bool material_color_is_valid(const std::array<float, 4>& color) noexcept {
    return std::ranges::all_of(color,
                               [](float value) { return std::isfinite(value) && value >= 0.0F && value <= 1.0F; });
}

void validate_asset_request_shape(std::vector<PlaceholderAssetDiagnostic>& diagnostics,
                                  const PlaceholderAssetRequest& asset) {
    if (!is_placeholder_supported_kind(asset.asset_kind)) {
        add_diagnostic(diagnostics, "unsupported_asset_kind",
                       "placeholder generation supports texture, mesh, material, and audio assets only",
                       asset.source_path, asset.asset_key);
    }
    if (!is_safe_repository_path(asset.source_path)) {
        add_diagnostic(diagnostics, "unsafe_source_path", "source path must be a safe repository-relative path",
                       asset.source_path, asset.asset_key);
    }
    if (!is_safe_repository_path(asset.imported_path)) {
        add_diagnostic(diagnostics, "unsafe_imported_path", "imported path must be a safe repository-relative path",
                       asset.imported_path, asset.asset_key);
    }

    if (asset.asset_kind == AssetKind::texture && (asset.texture_width == 0 || asset.texture_height == 0 ||
                                                   asset.texture_width > max_placeholder_texture_extent ||
                                                   asset.texture_height > max_placeholder_texture_extent)) {
        add_diagnostic(diagnostics, "invalid_texture_dimensions",
                       "placeholder texture dimensions must be non-zero and within the supported extent",
                       asset.source_path, asset.asset_key);
    }
    if (asset.asset_kind == AssetKind::material && !material_color_is_valid(asset.material_base_color)) {
        add_diagnostic(diagnostics, "invalid_material_base_color",
                       "placeholder material base color values must be finite normalized floats", asset.source_path,
                       asset.asset_key);
    }
    if (asset.asset_kind == AssetKind::audio &&
        (asset.audio_sample_rate == 0 || asset.audio_sample_rate > 384000 || asset.audio_frame_count == 0 ||
         asset.audio_frame_count > max_placeholder_audio_frame_count)) {
        add_diagnostic(diagnostics, "invalid_audio_shape",
                       "placeholder audio sample rate and frame count must be non-zero and bounded", asset.source_path,
                       asset.asset_key);
    }
}

void validate_request_shape(std::vector<PlaceholderAssetDiagnostic>& diagnostics,
                            const PlaceholderAssetBundleRequest& request) {
    if (!is_safe_repository_path(request.source_registry_path) ||
        !ends_with(request.source_registry_path, ".geassets")) {
        add_diagnostic(diagnostics, "unsafe_source_registry_path",
                       "source registry path must be a safe repository-relative .geassets path",
                       request.source_registry_path);
    }

    std::unordered_set<std::string> seen_keys;
    for (const auto& asset : request.assets) {
        validate_asset_request_shape(diagnostics, asset);
        if (!seen_keys.insert(asset.asset_key.value).second) {
            add_diagnostic(diagnostics, "duplicate_asset_key", "placeholder asset key appears more than once",
                           asset.source_path, asset.asset_key);
        }
    }
}

[[nodiscard]] SourceAssetRegistryDocumentV1
parse_source_registry_content(std::vector<PlaceholderAssetDiagnostic>& diagnostics,
                              const PlaceholderAssetBundleRequest& request) {
    if (request.source_registry_content.empty()) {
        return {};
    }

    try {
        return deserialize_source_asset_registry_document(request.source_registry_content);
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_source_registry",
                       std::string{"failed to parse source asset registry: "} + error.what(),
                       request.source_registry_path);
    }
    return {};
}

[[nodiscard]] std::string registry_diagnostic_code(SourceAssetRegistryDiagnosticCodeV1 code) {
    switch (code) {
    case SourceAssetRegistryDiagnosticCodeV1::invalid_key:
        return "invalid_asset_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_key:
        return "duplicate_asset_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_asset_id:
        return "duplicate_asset_id";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_kind:
        return "unsupported_asset_kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_path:
        return "unsafe_source_path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_source_path:
        return "duplicate_source_path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_format:
        return "unsupported_source_format";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_imported_path:
        return "unsafe_imported_path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_imported_path:
        return "duplicate_imported_path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_kind:
        return "invalid_dependency_kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_target:
        return "invalid_dependency_target";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_key:
        return "invalid_dependency_key";
    case SourceAssetRegistryDiagnosticCodeV1::missing_dependency_key:
        return "missing_dependency_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_dependency:
        return "duplicate_dependency";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_identity_projection:
        return "invalid_identity_projection";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_import_metadata:
        return "invalid_import_metadata";
    }
    return "invalid_source_asset_registry";
}

void append_registry_diagnostics(std::vector<PlaceholderAssetDiagnostic>& diagnostics,
                                 const std::vector<SourceAssetRegistryDiagnosticV1>& registry_diagnostics,
                                 const std::string& source_registry_path) {
    for (const auto& diagnostic : registry_diagnostics) {
        add_diagnostic(diagnostics, registry_diagnostic_code(diagnostic.code), "source asset registry is invalid",
                       diagnostic.path.empty() ? source_registry_path : diagnostic.path, diagnostic.key);
    }
}

[[nodiscard]] bool source_asset_dependency_less(const SourceAssetDependencyRowV1& lhs,
                                                const SourceAssetDependencyRowV1& rhs) noexcept {
    const auto lhs_kind = source_asset_dependency_kind_name_v1(lhs.kind);
    const auto rhs_kind = source_asset_dependency_kind_name_v1(rhs.kind);
    if (lhs_kind != rhs_kind) {
        return lhs_kind < rhs_kind;
    }
    return lhs.key.value < rhs.key.value;
}

void canonicalize(SourceAssetRegistryDocumentV1& document) {
    for (auto& asset : document.assets) {
        std::ranges::sort(asset.dependencies, source_asset_dependency_less);
    }
    std::ranges::sort(document.assets, [](const SourceAssetRegistryRowV1& lhs, const SourceAssetRegistryRowV1& rhs) {
        return lhs.key.value < rhs.key.value;
    });
}

[[nodiscard]] bool same_registry_row(const SourceAssetRegistryRowV1& row, const PlaceholderAssetRequest& request) {
    return row.key.value == request.asset_key.value && row.kind == request.asset_kind &&
           row.source_path == request.source_path &&
           row.source_format == expected_source_asset_format_v1(request.asset_kind) &&
           row.imported_path == request.imported_path && row.dependencies.empty();
}

[[nodiscard]] const SourceAssetRegistryRowV1* find_row_by_key(const SourceAssetRegistryDocumentV1& document,
                                                              const AssetKeyV2& key) noexcept {
    const auto it =
        std::ranges::find_if(document.assets, [&key](const auto& row) { return row.key.value == key.value; });
    return it == document.assets.end() ? nullptr : &*it;
}

[[nodiscard]] std::string name_from_asset_key(const AssetKeyV2& key) {
    const auto separator = key.value.find_last_of('/');
    if (separator == std::string::npos) {
        return key.value;
    }
    return key.value.substr(separator + 1U);
}

void append_u32_le(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void append_i16_le(std::vector<std::uint8_t>& bytes, std::int16_t value) {
    const auto encoded = static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(encoded & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((encoded >> 8U) & 0xFFU));
}

void append_f32_le(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t encoded = 0;
    static_assert(sizeof encoded == sizeof value);
    std::memcpy(&encoded, &value, sizeof encoded);
    append_u32_le(bytes, encoded);
}

[[nodiscard]] std::uint32_t next_pattern_value(std::uint32_t value) noexcept {
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    return value;
}

[[nodiscard]] TextureSourceDocument make_texture_source(const PlaceholderAssetRequest& request) {
    TextureSourceDocument document;
    document.width = request.texture_width;
    document.height = request.texture_height;
    document.pixel_format = TextureSourcePixelFormat::rgba8_unorm;
    document.bytes.reserve(static_cast<std::size_t>(request.texture_width) * request.texture_height * 4U);

    std::uint32_t pattern = request.seed == 0 ? 1U : request.seed;
    for (std::uint32_t y = 0; y < request.texture_height; ++y) {
        for (std::uint32_t x = 0; x < request.texture_width; ++x) {
            pattern = next_pattern_value(pattern + x + y * 17U);
            const bool checker = ((x / 4U) + (y / 4U)) % 2U == 0U;
            document.bytes.push_back(static_cast<std::uint8_t>((pattern >> 0U) & 0xFFU));
            document.bytes.push_back(static_cast<std::uint8_t>((pattern >> 8U) & 0xFFU));
            document.bytes.push_back(checker ? 0xFFU : static_cast<std::uint8_t>((pattern >> 16U) & 0xFFU));
            document.bytes.push_back(0xFFU);
        }
    }
    return document;
}

[[nodiscard]] MeshSourceDocument make_mesh_source() {
    MeshSourceDocument document;
    document.vertex_count = 4;
    document.index_count = 6;
    document.has_normals = false;
    document.has_uvs = false;
    document.has_tangent_frame = false;

    for (const auto [x, y, z] : std::array{
             std::array{-0.5F, -0.5F, 0.0F},
             std::array{0.5F, -0.5F, 0.0F},
             std::array{0.5F, 0.5F, 0.0F},
             std::array{-0.5F, 0.5F, 0.0F},
         }) {
        append_f32_le(document.vertex_bytes, x);
        append_f32_le(document.vertex_bytes, y);
        append_f32_le(document.vertex_bytes, z);
    }

    for (const std::uint32_t index : {0U, 1U, 2U, 0U, 2U, 3U}) {
        append_u32_le(document.index_bytes, index);
    }
    return document;
}

[[nodiscard]] MaterialDefinition make_material_definition(const PlaceholderAssetRequest& request) {
    MaterialDefinition material;
    material.id = asset_id_from_key_v2(request.asset_key);
    material.name = name_from_asset_key(request.asset_key);
    material.shading_model = MaterialShadingModel::unlit;
    material.surface_mode = MaterialSurfaceMode::opaque;
    material.factors.base_color = request.material_base_color;
    material.double_sided = true;
    return material;
}

[[nodiscard]] AudioSourceDocument make_audio_source(const PlaceholderAssetRequest& request) {
    AudioSourceDocument document;
    document.sample_rate = request.audio_sample_rate;
    document.channel_count = 1;
    document.frame_count = request.audio_frame_count;
    document.sample_format = AudioSourceSampleFormat::pcm16;
    document.samples.reserve(static_cast<std::size_t>(request.audio_frame_count) * 2U);

    std::uint32_t pattern = request.seed == 0 ? 1U : request.seed;
    for (std::uint64_t frame = 0; frame < request.audio_frame_count; ++frame) {
        pattern = next_pattern_value(pattern + static_cast<std::uint32_t>(frame & 0xFFFFU));
        const auto amplitude = static_cast<std::int16_t>(static_cast<int>((pattern >> 17U) & 0x3FFU) - 512);
        append_i16_le(document.samples, amplitude);
    }
    return document;
}

[[nodiscard]] PlaceholderAssetChangedFile make_source_file(const PlaceholderAssetRequest& request) {
    PlaceholderAssetChangedFile file;
    file.path = request.source_path;
    file.document_kind = std::string{expected_source_asset_format_v1(request.asset_kind)};

    switch (request.asset_kind) {
    case AssetKind::texture:
        file.content = serialize_texture_source_document(make_texture_source(request));
        break;
    case AssetKind::mesh:
        file.content = serialize_mesh_source_document(make_mesh_source());
        break;
    case AssetKind::material:
        file.content = serialize_material_definition(make_material_definition(request));
        break;
    case AssetKind::audio:
        file.content = serialize_audio_source_document(make_audio_source(request));
        break;
    case AssetKind::unknown:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::scene:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
    case AssetKind::environment_profile:
    case AssetKind::environment_preset_pack:
    case AssetKind::mavg_cluster_graph:
        break;
    }

    file.content_hash = hash_asset_cooked_content(file.content);
    return file;
}

void append_changed_file(std::vector<PlaceholderAssetChangedFile>& files, std::string path, std::string document_kind,
                         std::string content) {
    PlaceholderAssetChangedFile file;
    file.path = std::move(path);
    file.document_kind = std::move(document_kind);
    file.content = std::move(content);
    file.content_hash = hash_asset_cooked_content(file.content);
    files.push_back(std::move(file));
}

void sort_changed_files(std::vector<PlaceholderAssetChangedFile>& files) {
    std::ranges::sort(files, [](const PlaceholderAssetChangedFile& lhs, const PlaceholderAssetChangedFile& rhs) {
        return lhs.path < rhs.path;
    });
}

void sort_provenance_rows(std::vector<PlaceholderAssetProvenanceRow>& rows) {
    std::ranges::sort(rows, [](const PlaceholderAssetProvenanceRow& lhs, const PlaceholderAssetProvenanceRow& rhs) {
        return lhs.asset_key.value < rhs.asset_key.value;
    });
}

} // namespace

PlaceholderAssetBundlePlan plan_placeholder_asset_bundle(const PlaceholderAssetBundleRequest& request) {
    PlaceholderAssetBundlePlan plan;
    validate_request_shape(plan.diagnostics, request);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }

    auto registry = parse_source_registry_content(plan.diagnostics, request);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }
    canonicalize(registry);

    for (const auto& asset : request.assets) {
        const SourceAssetRegistryRowV1 row{
            .key = asset.asset_key,
            .kind = asset.asset_kind,
            .source_path = asset.source_path,
            .source_format = std::string{expected_source_asset_format_v1(asset.asset_kind)},
            .imported_path = asset.imported_path,
            .dependencies = {},
        };

        if (const auto* existing = find_row_by_key(registry, asset.asset_key); existing != nullptr) {
            if (!same_registry_row(*existing, asset)) {
                add_diagnostic(plan.diagnostics, "duplicate_asset_key",
                               "placeholder asset key conflicts with existing source asset registration",
                               asset.source_path, asset.asset_key);
            }
        } else {
            registry.assets.push_back(row);
        }
    }

    canonicalize(registry);
    append_registry_diagnostics(plan.diagnostics, validate_source_asset_registry_document(registry),
                                request.source_registry_path);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }

    plan.source_registry_content = serialize_source_asset_registry_document(registry);
    for (const auto& asset : request.assets) {
        auto source_file = make_source_file(asset);
        plan.provenance_rows.push_back(PlaceholderAssetProvenanceRow{
            .asset_key = asset.asset_key,
            .asset = asset_id_from_key_v2(asset.asset_key),
            .asset_kind = asset.asset_kind,
            .source_path = asset.source_path,
            .source_format = std::string{expected_source_asset_format_v1(asset.asset_kind)},
            .imported_path = asset.imported_path,
            .generator = std::string{placeholder_generator_id},
            .license = std::string{proprietary_license_id},
            .seed = asset.seed,
            .content_hash = source_file.content_hash,
        });
        plan.changed_files.push_back(std::move(source_file));
    }

    append_changed_file(plan.changed_files, request.source_registry_path,
                        std::string{source_asset_registry_format_v1()}, plan.source_registry_content);
    sort_changed_files(plan.changed_files);
    sort_provenance_rows(plan.provenance_rows);
    return plan;
}

PlaceholderAssetCookPackagePlan plan_placeholder_asset_cook_package(const PlaceholderAssetCookPackageRequest& request) {
    PlaceholderAssetCookPackagePlan plan;
    plan.placeholder_plan = plan_placeholder_asset_bundle(request.placeholder_assets);
    if (!plan.placeholder_plan.succeeded()) {
        return plan;
    }

    RegisteredSourceAssetCookPackageRequest package_request;
    package_request.source_registry_path = request.placeholder_assets.source_registry_path;
    package_request.source_registry_content = plan.placeholder_plan.source_registry_content;
    package_request.package_index_path = request.package_index_path;
    package_request.package_index_content = request.package_index_content;
    package_request.source_revision = request.source_revision;
    package_request.selected_asset_keys.reserve(request.placeholder_assets.assets.size());
    package_request.source_files.reserve(plan.placeholder_plan.changed_files.size());

    for (const auto& asset : request.placeholder_assets.assets) {
        package_request.selected_asset_keys.push_back(asset.asset_key);
    }
    for (const auto& file : plan.placeholder_plan.changed_files) {
        if (file.path == request.placeholder_assets.source_registry_path) {
            continue;
        }
        package_request.source_files.push_back(RegisteredSourceAssetCookPackageSourceFile{
            .path = file.path,
            .content = file.content,
        });
    }

    plan.package_plan = plan_registered_source_asset_cook_package(package_request);
    return plan;
}

} // namespace mirakana
