// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/sprite_atlas_tool.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>

namespace mirakana {
namespace {

constexpr std::string_view sprite_atlas_source_decoding = "provided-rgba8-texture-source";
constexpr std::string_view sprite_atlas_packing = "deterministic-sprite-atlas-rgba8-max-side";
constexpr std::string_view sprite_atlas_page_policy = "single-page-tight-rgba8-texture-source";

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

[[nodiscard]] bool valid_frame_segment(std::string_view segment) noexcept {
    if (segment.empty() || segment == "." || segment == "..") {
        return false;
    }
    return std::ranges::all_of(segment, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.';
    });
}

[[nodiscard]] bool valid_frame_id(std::string_view id) noexcept {
    if (id.empty() || id.front() == '/' || id.find('\\') != std::string_view::npos ||
        id.find(':') != std::string_view::npos || id.find(';') != std::string_view::npos ||
        id.find(' ') != std::string_view::npos || has_control_character(id)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= id.size()) {
        const auto end = id.find('/', begin);
        const auto segment = id.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (!valid_frame_segment(segment)) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

void add_diagnostic(std::vector<SpriteAtlasSourceAuthoringDiagnostic>& diagnostics, std::string code,
                    std::string message, std::string path = {}, std::string frame_id = {}, AssetKey asset_key = {}) {
    diagnostics.push_back(SpriteAtlasSourceAuthoringDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .frame_id = std::move(frame_id),
        .asset_key = std::move(asset_key),
    });
}

void sort_diagnostics(std::vector<SpriteAtlasSourceAuthoringDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.frame_id != rhs.frame_id) {
            return lhs.frame_id < rhs.frame_id;
        }
        if (lhs.asset_key.value != rhs.asset_key.value) {
            return lhs.asset_key.value < rhs.asset_key.value;
        }
        return lhs.code < rhs.code;
    });
}

[[nodiscard]] std::uint64_t expected_rgba8_byte_count(const TextureSourceDocument& image) noexcept {
    return static_cast<std::uint64_t>(image.width) * image.height * 4ULL;
}

[[nodiscard]] bool valid_rgba8_byte_count(const TextureSourceDocument& image) noexcept {
    return expected_rgba8_byte_count(image) == image.bytes.size();
}

[[nodiscard]] bool valid_normalized(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

[[nodiscard]] bool valid_frame_pivot(const SpriteAtlasSourcePivot& pivot) noexcept {
    return valid_normalized(pivot.x) && valid_normalized(pivot.y);
}

[[nodiscard]] bool valid_frame_slice_border(const SpriteAtlasSourceSliceBorder& border) noexcept {
    return valid_normalized(border.left) && valid_normalized(border.bottom) && valid_normalized(border.right) &&
           valid_normalized(border.top) && border.left + border.right < 0.98F && border.bottom + border.top < 0.98F;
}

void validate_request_shape(std::vector<SpriteAtlasSourceAuthoringDiagnostic>& diagnostics,
                            const SpriteAtlasSourceAuthoringDesc& desc) {
    if (!is_safe_repository_path(desc.source_registry_path) || !ends_with(desc.source_registry_path, ".geassets")) {
        add_diagnostic(diagnostics, "unsafe_source_registry_path",
                       "source registry path must be a safe repository-relative .geassets path",
                       desc.source_registry_path, {}, desc.atlas_asset_key);
    }
    if (!is_safe_repository_path(desc.atlas_source_path) || !ends_with(desc.atlas_source_path, ".texture_source")) {
        add_diagnostic(diagnostics, "unsafe_atlas_source_path",
                       "atlas source path must be a safe repository-relative .texture_source path",
                       desc.atlas_source_path, {}, desc.atlas_asset_key);
    }
    if (!is_safe_repository_path(desc.atlas_imported_path)) {
        add_diagnostic(diagnostics, "unsafe_atlas_imported_path",
                       "atlas imported path must be a safe repository-relative path", desc.atlas_imported_path, {},
                       desc.atlas_asset_key);
    }
    if (desc.max_side == 0 || desc.max_side > sprite_atlas_packing_max_side) {
        add_diagnostic(diagnostics, "invalid_max_side", "sprite atlas max_side must be non-zero and bounded",
                       desc.atlas_source_path, {}, desc.atlas_asset_key);
    }
    if (desc.page_policy.mode != sprite_atlas_page_policy || desc.page_policy.page_count != 1 ||
        desc.page_policy.padding_pixels != 0) {
        add_diagnostic(diagnostics, "unsupported_page_policy",
                       "sprite atlas authoring supports only single-page tight RGBA8 texture source pages",
                       desc.atlas_source_path, desc.page_policy.page_id, desc.atlas_asset_key);
    }
    if (!valid_frame_id(desc.page_policy.page_id)) {
        add_diagnostic(diagnostics, "invalid_page_id", "sprite atlas page id must be a safe identifier",
                       desc.atlas_source_path, desc.page_policy.page_id, desc.atlas_asset_key);
    }
    if (desc.source_decoding != sprite_atlas_source_decoding) {
        add_diagnostic(diagnostics, "unsupported_source_decoding",
                       "sprite atlas source decoding must be provided-rgba8-texture-source", desc.atlas_source_path, {},
                       desc.atlas_asset_key);
    }
    if (desc.atlas_packing != sprite_atlas_packing) {
        add_diagnostic(diagnostics, "unsupported_atlas_packing",
                       "sprite atlas packing must be deterministic-sprite-atlas-rgba8-max-side", desc.atlas_source_path,
                       {}, desc.atlas_asset_key);
    }

    for (const auto [field, value] : {
             std::pair{"runtime_source_image_decoding", std::string_view{desc.runtime_source_image_decoding}},
             std::pair{"renderer_rhi_residency", std::string_view{desc.renderer_rhi_residency}},
             std::pair{"package_streaming", std::string_view{desc.package_streaming}},
             std::pair{"animation_semantics", std::string_view{desc.animation_semantics}},
             std::pair{"editor_productization", std::string_view{desc.editor_productization}},
             std::pair{"free_form_edit", std::string_view{desc.free_form_edit}},
         }) {
        if (value != "unsupported") {
            add_diagnostic(diagnostics, std::string{"unsupported_"} + field,
                           std::string{"sprite atlas authoring does not support "} + field, desc.atlas_source_path, {},
                           desc.atlas_asset_key);
        }
    }

    if (desc.frames.empty()) {
        add_diagnostic(diagnostics, "missing_frame", "sprite atlas authoring requires at least one frame",
                       desc.atlas_source_path, {}, desc.atlas_asset_key);
    }

    std::unordered_set<std::string> frame_ids;
    for (const auto& frame : desc.frames) {
        if (!valid_frame_id(frame.frame_id)) {
            add_diagnostic(diagnostics, "invalid_frame_id",
                           "sprite atlas frame id must be a slash-separated safe identifier", frame.source_path,
                           frame.frame_id, desc.atlas_asset_key);
        } else if (!frame_ids.insert(frame.frame_id).second) {
            add_diagnostic(diagnostics, "duplicate_frame_id", "sprite atlas frame id appears more than once",
                           frame.source_path, frame.frame_id, desc.atlas_asset_key);
        }
        if (!is_safe_repository_path(frame.source_path)) {
            add_diagnostic(diagnostics, "unsafe_frame_source_path",
                           "sprite atlas frame source path must be a safe repository-relative path", frame.source_path,
                           frame.frame_id, desc.atlas_asset_key);
        }
        if (frame.image.pixel_format != TextureSourcePixelFormat::rgba8_unorm) {
            add_diagnostic(diagnostics, "unsupported_pixel_format", "sprite atlas frame image must be RGBA8",
                           frame.source_path, frame.frame_id, desc.atlas_asset_key);
        }
        if (frame.image.width == 0 || frame.image.height == 0 || frame.image.width > desc.max_side ||
            frame.image.height > desc.max_side) {
            add_diagnostic(diagnostics, "invalid_frame_dimensions",
                           "sprite atlas frame dimensions must be non-zero and within max_side", frame.source_path,
                           frame.frame_id, desc.atlas_asset_key);
        }
        if (!valid_rgba8_byte_count(frame.image)) {
            add_diagnostic(diagnostics, "invalid_pixel_byte_count",
                           "sprite atlas frame pixel byte count must equal width * height * 4", frame.source_path,
                           frame.frame_id, desc.atlas_asset_key);
        }
        if (!valid_frame_pivot(frame.pivot)) {
            add_diagnostic(diagnostics, "invalid_frame_pivot",
                           "sprite atlas frame pivot must be finite normalized coordinates", frame.source_path,
                           frame.frame_id, desc.atlas_asset_key);
        }
        if (!valid_frame_slice_border(frame.slice_border)) {
            add_diagnostic(diagnostics, "invalid_frame_slice_border",
                           "sprite atlas frame slice border must be normalized and leave a center region",
                           frame.source_path, frame.frame_id, desc.atlas_asset_key);
        }
    }
}

[[nodiscard]] std::vector<SpriteAtlasSourceFrameDesc>
canonical_frame_order(const std::vector<SpriteAtlasSourceFrameDesc>& frames) {
    auto ordered = frames;
    std::ranges::sort(ordered, [](const auto& lhs, const auto& rhs) {
        if (lhs.frame_id != rhs.frame_id) {
            return lhs.frame_id < rhs.frame_id;
        }
        return lhs.source_path < rhs.source_path;
    });
    return ordered;
}

[[nodiscard]] std::string packing_diagnostic_code(SpriteAtlasPackingDiagnosticCode code) {
    switch (code) {
    case SpriteAtlasPackingDiagnosticCode::ok:
        return "ok";
    case SpriteAtlasPackingDiagnosticCode::empty_items:
        return "empty_items";
    case SpriteAtlasPackingDiagnosticCode::too_many_items:
        return "too_many_items";
    case SpriteAtlasPackingDiagnosticCode::zero_dimension:
        return "zero_dimension";
    case SpriteAtlasPackingDiagnosticCode::dimension_exceeds_limit:
        return "dimension_exceeds_limit";
    case SpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side:
        return "atlas_exceeds_max_side";
    case SpriteAtlasPackingDiagnosticCode::rgba_byte_size_mismatch:
        return "rgba_byte_size_mismatch";
    }
    return "sprite_atlas_packing_failed";
}

[[nodiscard]] std::string packing_diagnostic_message(SpriteAtlasPackingDiagnosticCode code) {
    return "sprite atlas packing failed: " + packing_diagnostic_code(code);
}

[[nodiscard]] SourceAssetRegistryDocument parse_registry(SpriteAtlasSourceAuthoringPlan& plan,
                                                         const SpriteAtlasSourceAuthoringDesc& desc) {
    if (desc.source_registry_content.empty()) {
        return {};
    }
    try {
        return deserialize_source_asset_registry_document(desc.source_registry_content);
    } catch (const std::exception& error) {
        add_diagnostic(plan.diagnostics, "invalid_source_registry",
                       std::string{"failed to parse source asset registry: "} + error.what(), desc.source_registry_path,
                       {}, desc.atlas_asset_key);
    }
    return {};
}

[[nodiscard]] bool same_atlas_registry_row(const SourceAssetRegistryRow& row,
                                           const SpriteAtlasSourceAuthoringDesc& desc) {
    return row.key.value == desc.atlas_asset_key.value && row.kind == AssetKind::texture &&
           row.source_path == desc.atlas_source_path &&
           row.source_format == expected_source_asset_format(AssetKind::texture) &&
           row.imported_path == desc.atlas_imported_path && row.dependencies.empty();
}

void canonicalize(SourceAssetRegistryDocument& document) {
    for (auto& asset : document.assets) {
        std::ranges::sort(asset.dependencies, [](const auto& lhs, const auto& rhs) {
            const auto lhs_kind = source_asset_dependency_kind_name(lhs.kind);
            const auto rhs_kind = source_asset_dependency_kind_name(rhs.kind);
            if (lhs_kind != rhs_kind) {
                return lhs_kind < rhs_kind;
            }
            return lhs.key.value < rhs.key.value;
        });
    }
    std::ranges::sort(document.assets, [](const auto& lhs, const auto& rhs) { return lhs.key.value < rhs.key.value; });
}

void add_or_validate_atlas_registry_row(SpriteAtlasSourceAuthoringPlan& plan, SourceAssetRegistryDocument& registry,
                                        const SpriteAtlasSourceAuthoringDesc& desc) {
    const auto existing = std::ranges::find_if(
        registry.assets, [&desc](const auto& row) { return row.key.value == desc.atlas_asset_key.value; });
    if (existing != registry.assets.end()) {
        if (!same_atlas_registry_row(*existing, desc)) {
            add_diagnostic(plan.diagnostics, "duplicate_asset_key",
                           "sprite atlas asset key conflicts with existing source asset registration",
                           desc.atlas_source_path, {}, desc.atlas_asset_key);
        }
        return;
    }

    registry.assets.push_back(SourceAssetRegistryRow{
        .key = desc.atlas_asset_key,
        .kind = AssetKind::texture,
        .source_path = desc.atlas_source_path,
        .source_format = std::string{expected_source_asset_format(AssetKind::texture)},
        .imported_path = desc.atlas_imported_path,
        .dependencies = {},
    });
}

void append_registry_diagnostics(SpriteAtlasSourceAuthoringPlan& plan,
                                 const std::vector<SourceAssetRegistryDiagnostic>& diagnostics,
                                 const SpriteAtlasSourceAuthoringDesc& desc) {
    for (const auto& diagnostic : diagnostics) {
        add_diagnostic(plan.diagnostics, "invalid_source_registry", "source asset registry is invalid",
                       diagnostic.path.empty() ? desc.source_registry_path : diagnostic.path, {}, diagnostic.key);
    }
}

void append_changed_file(std::vector<SpriteAtlasSourceChangedFile>& files, std::string path, std::string document_kind,
                         std::string content) {
    files.push_back(SpriteAtlasSourceChangedFile{
        .path = std::move(path),
        .document_kind = std::move(document_kind),
        .content = std::move(content),
        .content_hash = 0,
    });
    files.back().content_hash = hash_asset_cooked_content(files.back().content);
}

void append_frame_rows(SpriteAtlasSourceAuthoringPlan& plan, const SpriteAtlasSourceAuthoringDesc& desc,
                       const std::vector<SpriteAtlasSourceFrameDesc>& frames,
                       const std::vector<SpriteAtlasPackedPlacement>& placements) {
    plan.frame_rows.reserve(placements.size());
    const auto atlas_asset = asset_id_from_key(desc.atlas_asset_key);
    for (std::size_t index = 0; index < placements.size(); ++index) {
        const auto& frame = frames[index];
        const auto& placement = placements[index];
        const auto source_content = serialize_texture_source_document(frame.image);
        plan.frame_rows.push_back(SpriteAtlasSourceFrameRow{
            .frame_id = frame.frame_id,
            .source_path = frame.source_path,
            .atlas_asset_key = desc.atlas_asset_key,
            .atlas_asset = atlas_asset,
            .atlas_source_path = desc.atlas_source_path,
            .atlas_imported_path = desc.atlas_imported_path,
            .page_index = 0,
            .page_id = desc.page_policy.page_id,
            .x = placement.x,
            .y = placement.y,
            .width = placement.width,
            .height = placement.height,
            .u0 = static_cast<float>(placement.x) / static_cast<float>(plan.atlas_texture.width),
            .v0 = static_cast<float>(placement.y) / static_cast<float>(plan.atlas_texture.height),
            .u1 = static_cast<float>(placement.x + placement.width) / static_cast<float>(plan.atlas_texture.width),
            .v1 = static_cast<float>(placement.y + placement.height) / static_cast<float>(plan.atlas_texture.height),
            .pivot_x = frame.pivot.x,
            .pivot_y = frame.pivot.y,
            .slice_border_left = frame.slice_border.left,
            .slice_border_bottom = frame.slice_border.bottom,
            .slice_border_right = frame.slice_border.right,
            .slice_border_top = frame.slice_border.top,
            .source_content_hash = hash_asset_cooked_content(source_content),
        });
    }
}

} // namespace

SpriteAtlasSourceAuthoringPlan plan_sprite_atlas_source_authoring(const SpriteAtlasSourceAuthoringDesc& desc) {
    SpriteAtlasSourceAuthoringPlan plan;
    validate_request_shape(plan.diagnostics, desc);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }

    auto frames = canonical_frame_order(desc.frames);
    std::vector<SpriteAtlasPackingItemView> items;
    items.reserve(frames.size());
    for (const auto& frame : frames) {
        items.push_back(SpriteAtlasPackingItemView{
            .width = frame.image.width,
            .height = frame.image.height,
            .rgba8_pixels = frame.image.bytes,
        });
    }

    auto packed = pack_sprite_atlas_rgba8_max_side(items, desc.max_side);
    if (const auto* diagnostic = std::get_if<SpriteAtlasPackingDiagnostic>(&packed); diagnostic != nullptr) {
        add_diagnostic(plan.diagnostics, packing_diagnostic_code(diagnostic->code),
                       packing_diagnostic_message(diagnostic->code), desc.atlas_source_path, {}, desc.atlas_asset_key);
        sort_diagnostics(plan.diagnostics);
        return plan;
    }

    auto output = std::get<SpriteAtlasRgba8PackingOutput>(std::move(packed));
    plan.atlas_texture = std::move(output.atlas);
    plan.atlas_texture_content = serialize_texture_source_document(plan.atlas_texture);

    auto registry = parse_registry(plan, desc);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }
    add_or_validate_atlas_registry_row(plan, registry, desc);
    canonicalize(registry);
    append_registry_diagnostics(plan, validate_source_asset_registry_document(registry), desc);
    if (!plan.succeeded()) {
        sort_diagnostics(plan.diagnostics);
        return plan;
    }

    plan.source_registry_content = serialize_source_asset_registry_document(registry);
    append_changed_file(plan.changed_files, desc.atlas_source_path,
                        std::string{expected_source_asset_format(AssetKind::texture)}, plan.atlas_texture_content);
    append_changed_file(plan.changed_files, desc.source_registry_path, std::string{source_asset_registry_format()},
                        plan.source_registry_content);
    append_frame_rows(plan, desc, frames, output.placements);
    return plan;
}

} // namespace mirakana
