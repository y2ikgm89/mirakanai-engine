// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/sandbox_world_authoring.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view sandbox_world_authoring_format = "GameEngine.SandboxWorldAuthoringReview.v1";

[[nodiscard]] bool is_safe_token(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return std::isalnum(character) != 0 || character == '_' || character == '-' || character == '/' ||
               character == '.';
    });
}

[[nodiscard]] bool is_safe_material_tag(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return std::isalnum(character) != 0 || character == '_' || character == '-';
    });
}

[[nodiscard]] bool is_valid_uv(float u0, float v0, float u1, float v1) noexcept {
    return u0 >= 0.0F && v0 >= 0.0F && u1 <= 1.0F && v1 <= 1.0F && u0 < u1 && v0 < v1;
}

[[nodiscard]] std::string bool_text(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string_view to_string(SandboxWorldTileCollisionKind value) noexcept {
    switch (value) {
    case SandboxWorldTileCollisionKind::empty:
        return "empty";
    case SandboxWorldTileCollisionKind::solid:
        return "solid";
    case SandboxWorldTileCollisionKind::platform:
        return "platform";
    case SandboxWorldTileCollisionKind::liquid:
        return "liquid";
    case SandboxWorldTileCollisionKind::trigger:
        return "trigger";
    }
    return "empty";
}

[[nodiscard]] std::string_view to_string(SandboxWorldTileUpdatePolicy value) noexcept {
    switch (value) {
    case SandboxWorldTileUpdatePolicy::none:
        return "none";
    case SandboxWorldTileUpdatePolicy::scheduled:
        return "scheduled";
    case SandboxWorldTileUpdatePolicy::random_tick:
        return "random_tick";
    }
    return "none";
}

[[nodiscard]] std::string_view to_string(SandboxWorldBrushShape value) noexcept {
    switch (value) {
    case SandboxWorldBrushShape::single_cell:
        return "single_cell";
    case SandboxWorldBrushShape::rectangle:
        return "rectangle";
    case SandboxWorldBrushShape::flood_fill:
        return "flood_fill";
    }
    return "single_cell";
}

[[nodiscard]] std::string_view to_string(SandboxWorldBrushReplacementPolicy value) noexcept {
    switch (value) {
    case SandboxWorldBrushReplacementPolicy::empty_only:
        return "empty_only";
    case SandboxWorldBrushReplacementPolicy::matching_tile:
        return "matching_tile";
    case SandboxWorldBrushReplacementPolicy::any:
        return "any";
    }
    return "empty_only";
}

[[nodiscard]] std::string_view to_string(SandboxWorldBrushSymmetry value) noexcept {
    switch (value) {
    case SandboxWorldBrushSymmetry::none:
        return "none";
    case SandboxWorldBrushSymmetry::mirror_x:
        return "mirror_x";
    case SandboxWorldBrushSymmetry::mirror_y:
        return "mirror_y";
    case SandboxWorldBrushSymmetry::four_way:
        return "four_way";
    }
    return "none";
}

[[nodiscard]] std::string_view to_string(SandboxWorldBrushFillPolicy value) noexcept {
    switch (value) {
    case SandboxWorldBrushFillPolicy::paint:
        return "paint";
    case SandboxWorldBrushFillPolicy::replace:
        return "replace";
    case SandboxWorldBrushFillPolicy::erase:
        return "erase";
    }
    return "paint";
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0U) {
            joined.push_back(',');
        }
        joined += values[i];
    }
    return joined;
}

void add_diagnostic(SandboxWorldAuthoringReviewResult& result, SandboxWorldAuthoringDiagnosticCode code,
                    std::string message, std::string id = {}, std::string path = {}, std::uint32_t source_index = 0) {
    result.diagnostics.push_back(SandboxWorldAuthoringDiagnostic{
        .code = code,
        .id = std::move(id),
        .path = std::move(path),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void validate_tile_definitions(SandboxWorldAuthoringReviewResult& result,
                               const SandboxWorldAuthoringPackageDesc& package,
                               std::unordered_set<std::string>& tile_ids) {
    std::unordered_set<std::string> tilemap_tile_ids;
    tilemap_tile_ids.reserve(package.tilemap.tiles.size());
    for (const auto& tile : package.tilemap.tiles) {
        if (!tile.id.empty()) {
            tilemap_tile_ids.insert(tile.id);
        }
    }

    tile_ids.reserve(package.tile_definitions.size());
    for (const auto& tile : package.tile_definitions) {
        if (tile.tile_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_tile_id,
                           "sandbox world tile definition requires tile_id", {}, {}, tile.source_index);
        } else if (!tile_ids.insert(tile.tile_id).second) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::duplicate_tile_id,
                           "sandbox world tile_id appears more than once", tile.tile_id, {}, tile.source_index);
        }
        if (!tile.tile_id.empty() && !tilemap_tile_ids.contains(tile.tile_id)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unknown_tile_definition,
                           "sandbox world tile definition must reference a cooked tilemap tile", tile.tile_id, {},
                           tile.source_index);
        }
        if (tile.atlas_frame_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_atlas_frame_id,
                           "sandbox world tile definition requires atlas_frame_id", tile.tile_id, {},
                           tile.source_index);
        }
        if (tile.atlas_page.value == 0U) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_atlas_page,
                           "sandbox world tile definition requires atlas_page", tile.tile_id, {}, tile.source_index);
        }
        if (!is_valid_uv(tile.u0, tile.v0, tile.u1, tile.v1)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_uv_rect,
                           "sandbox world tile definition uv rect is invalid", tile.tile_id, {}, tile.source_index);
        }
        for (const auto& tag : tile.material_tags) {
            if (!is_safe_material_tag(tag)) {
                add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_material_tag,
                               "sandbox world tile material tags must be safe identifiers", tile.tile_id, tag,
                               tile.source_index);
            }
        }
        if (!is_safe_token(tile.localization_key)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_localization_key,
                           "sandbox world tile definition requires a safe localization key", tile.tile_id, {},
                           tile.source_index);
        }
        if (!is_safe_token(tile.accessibility_label_key)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_accessibility_label_key,
                           "sandbox world tile definition requires a safe accessibility label key", tile.tile_id, {},
                           tile.source_index);
        }
        if (tile.provenance.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_provenance,
                           "sandbox world tile definition requires provenance", tile.tile_id, {}, tile.source_index);
        }
        if (tile.license_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_license_id,
                           "sandbox world tile definition requires license_id", tile.tile_id, {}, tile.source_index);
        }
    }

    for (const auto& tile : package.tilemap.tiles) {
        if (!tile.id.empty() && !tile_ids.contains(tile.id)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_tile_definition,
                           "cooked tilemap tile is missing a sandbox world tile definition", tile.id);
        }
    }
}

void validate_brushes(SandboxWorldAuthoringReviewResult& result, const SandboxWorldAuthoringPackageDesc& package,
                      const std::unordered_set<std::string>& tile_ids) {
    std::unordered_set<std::string> brush_ids;
    brush_ids.reserve(package.palette_brushes.size());
    for (const auto& brush : package.palette_brushes) {
        if (brush.brush_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_brush_id,
                           "sandbox world palette brush requires brush_id", {}, {}, brush.source_index);
        } else if (!brush_ids.insert(brush.brush_id).second) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::duplicate_brush_id,
                           "sandbox world palette brush id appears more than once", brush.brush_id, {},
                           brush.source_index);
        }
        if (brush.layer_mask == 0U) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_layer_mask,
                           "sandbox world palette brush requires a non-zero layer mask", brush.brush_id, {},
                           brush.source_index);
        }
        if (brush.tile_id.empty() || !tile_ids.contains(brush.tile_id)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unknown_brush_tile,
                           "sandbox world palette brush references an unknown tile", brush.brush_id, brush.tile_id,
                           brush.source_index);
        }
        if (!is_safe_tilemap_package_relative_path(brush.preview_path)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_brush_path,
                           "sandbox world palette brush preview path must be package-relative", brush.brush_id,
                           brush.preview_path, brush.source_index);
        }
    }
}

void validate_object_rules(SandboxWorldAuthoringReviewResult& result,
                           const std::vector<SandboxWorldObjectPlacementRuleRow>& rules,
                           const std::unordered_set<std::string>& tile_ids, std::string_view owner_id,
                           std::uint32_t source_index) {
    for (const auto& rule : rules) {
        if (rule.object_id.empty() || rule.anchor_tile_id.empty() || !tile_ids.contains(rule.anchor_tile_id) ||
            rule.max_per_chunk == 0U || rule.spawn_weight == 0U) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_object_placement_rule,
                           "sandbox world object placement rules require object id, known anchor tile, max count, and "
                           "spawn weight",
                           std::string{owner_id}, rule.object_id, source_index);
        }
    }
}

void validate_allowed_tiles(SandboxWorldAuthoringReviewResult& result, const std::vector<std::string>& allowed_tiles,
                            const std::unordered_set<std::string>& tile_ids, std::string_view owner_id,
                            std::uint32_t source_index) {
    if (allowed_tiles.empty()) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unknown_allowed_tile,
                       "sandbox world row requires at least one allowed tile id", std::string{owner_id}, {},
                       source_index);
        return;
    }
    for (const auto& tile_id : allowed_tiles) {
        if (!tile_ids.contains(tile_id)) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unknown_allowed_tile,
                           "sandbox world row references an unknown allowed tile", std::string{owner_id}, tile_id,
                           source_index);
        }
    }
}

void validate_chunk_templates(SandboxWorldAuthoringReviewResult& result,
                              const SandboxWorldAuthoringPackageDesc& package,
                              const std::unordered_set<std::string>& tile_ids) {
    std::unordered_set<std::string> template_ids;
    template_ids.reserve(package.chunk_templates.size());
    for (const auto& templ : package.chunk_templates) {
        if (templ.template_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_chunk_template_id,
                           "sandbox world chunk template requires template_id", {}, {}, templ.source_index);
        } else if (!template_ids.insert(templ.template_id).second) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::duplicate_chunk_template_id,
                           "sandbox world chunk template id appears more than once", templ.template_id, {},
                           templ.source_index);
        }
        if (templ.width == 0U || templ.height == 0U) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_chunk_template_extent,
                           "sandbox world chunk template dimensions must be non-zero", templ.template_id, {},
                           templ.source_index);
        }
        validate_allowed_tiles(result, templ.allowed_tile_ids, tile_ids, templ.template_id, templ.source_index);
        validate_object_rules(result, templ.object_placement_rules, tile_ids, templ.template_id, templ.source_index);
    }
}

void validate_procedural_seeds(SandboxWorldAuthoringReviewResult& result,
                               const SandboxWorldAuthoringPackageDesc& package,
                               const std::unordered_set<std::string>& tile_ids) {
    std::unordered_set<std::string> generator_ids;
    generator_ids.reserve(package.procedural_seeds.size());
    for (const auto& seed : package.procedural_seeds) {
        if (seed.generator_id.empty()) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_generator_id,
                           "sandbox world procedural seed requires generator_id", {}, {}, seed.source_index);
        } else if (!generator_ids.insert(seed.generator_id).second) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::duplicate_generator_id,
                           "sandbox world procedural generator id appears more than once", seed.generator_id, {},
                           seed.source_index);
        }
        if (seed.width == 0U || seed.height == 0U) {
            add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_procedural_seed_extent,
                           "sandbox world procedural seed dimensions must be non-zero", seed.generator_id, {},
                           seed.source_index);
        }
        validate_allowed_tiles(result, seed.allowed_tile_ids, tile_ids, seed.generator_id, seed.source_index);
        validate_object_rules(result, seed.object_placement_rules, tile_ids, seed.generator_id, seed.source_index);
    }
}

[[nodiscard]] std::string serialize_review_document(const SandboxWorldAuthoringPackageDesc& package) {
    std::ostringstream output;
    output << "format=" << sandbox_world_authoring_format << '\n';
    output << "review_document.path=" << package.review_document_path << '\n';
    output << "tilemap.output_path=" << package.tilemap.output_path << '\n';
    output << "tile.count=" << package.tile_definitions.size() << '\n';
    for (std::size_t i = 0; i < package.tile_definitions.size(); ++i) {
        const auto& tile = package.tile_definitions[i];
        output << "tile." << i << ".id=" << tile.tile_id << '\n';
        output << "tile." << i << ".atlas_frame_id=" << tile.atlas_frame_id << '\n';
        output << "tile." << i << ".atlas_page=" << tile.atlas_page.value << '\n';
        output << "tile." << i << ".uv=" << tile.u0 << ',' << tile.v0 << ',' << tile.u1 << ',' << tile.v1 << '\n';
        output << "tile." << i << ".collision_kind=" << to_string(tile.collision_kind) << '\n';
        output << "tile." << i << ".material_tags=" << join_values(tile.material_tags) << '\n';
        output << "tile." << i << ".emits_light=" << bool_text(tile.emits_light) << '\n';
        output << "tile." << i << ".light_radius=" << tile.light_radius << '\n';
        output << "tile." << i << ".liquid=" << bool_text(tile.liquid) << '\n';
        output << "tile." << i << ".update_policy=" << to_string(tile.update_policy) << '\n';
        output << "tile." << i << ".localization_key=" << tile.localization_key << '\n';
        output << "tile." << i << ".accessibility_label_key=" << tile.accessibility_label_key << '\n';
        output << "tile." << i << ".provenance=" << tile.provenance << '\n';
        output << "tile." << i << ".license_id=" << tile.license_id << '\n';
    }

    output << "brush.count=" << package.palette_brushes.size() << '\n';
    for (std::size_t i = 0; i < package.palette_brushes.size(); ++i) {
        const auto& brush = package.palette_brushes[i];
        output << "brush." << i << ".id=" << brush.brush_id << '\n';
        output << "brush." << i << ".shape=" << to_string(brush.shape) << '\n';
        output << "brush." << i << ".layer_mask=" << brush.layer_mask << '\n';
        output << "brush." << i << ".tile_id=" << brush.tile_id << '\n';
        output << "brush." << i << ".replacement_policy=" << to_string(brush.replacement_policy) << '\n';
        output << "brush." << i << ".symmetry=" << to_string(brush.symmetry) << '\n';
        output << "brush." << i << ".fill_policy=" << to_string(brush.fill_policy) << '\n';
        output << "brush." << i << ".preview_path=" << brush.preview_path << '\n';
    }

    output << "chunk_template.count=" << package.chunk_templates.size() << '\n';
    for (std::size_t i = 0; i < package.chunk_templates.size(); ++i) {
        const auto& templ = package.chunk_templates[i];
        output << "chunk_template." << i << ".id=" << templ.template_id << '\n';
        output << "chunk_template." << i << ".dimensions=" << templ.width << 'x' << templ.height << '\n';
        output << "chunk_template." << i << ".allowed_tile_ids=" << join_values(templ.allowed_tile_ids) << '\n';
        output << "chunk_template." << i << ".object_rule_count=" << templ.object_placement_rules.size() << '\n';
    }

    output << "procedural_seed.count=" << package.procedural_seeds.size() << '\n';
    for (std::size_t i = 0; i < package.procedural_seeds.size(); ++i) {
        const auto& seed = package.procedural_seeds[i];
        output << "procedural_seed." << i << ".generator_id=" << seed.generator_id << '\n';
        output << "procedural_seed." << i << ".seed=" << seed.seed << '\n';
        output << "procedural_seed." << i << ".dimensions=" << seed.width << 'x' << seed.height << '\n';
        output << "procedural_seed." << i << ".allowed_tile_ids=" << join_values(seed.allowed_tile_ids) << '\n';
        output << "procedural_seed." << i << ".object_rule_count=" << seed.object_placement_rules.size() << '\n';
    }
    return output.str();
}

void append_changed_file(std::vector<SandboxWorldAuthoringChangedFile>& files, std::string path,
                         std::string document_kind, std::string content) {
    files.push_back(SandboxWorldAuthoringChangedFile{
        .path = std::move(path),
        .document_kind = std::move(document_kind),
        .content = std::move(content),
        .content_hash = 0,
    });
    files.back().content_hash = hash_asset_cooked_content(files.back().content);
}

void append_tilemap_failures(SandboxWorldAuthoringReviewResult& result,
                             const std::vector<CookedTilemapAuthoringFailure>& failures) {
    for (const auto& failure : failures) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::tilemap_package_update_failed, failure.diagnostic,
                       std::to_string(failure.asset.value), failure.path);
    }
}

struct TransactionSnapshotFile {
    std::string path;
    bool existed{false};
    std::string content;
};

[[nodiscard]] std::vector<TransactionSnapshotFile>
snapshot_changed_files(IFileSystem& filesystem, const std::vector<SandboxWorldAuthoringChangedFile>& changed_files) {
    std::vector<TransactionSnapshotFile> snapshots;
    snapshots.reserve(changed_files.size());
    for (const auto& changed : changed_files) {
        TransactionSnapshotFile snapshot;
        snapshot.path = changed.path;
        snapshot.existed = filesystem.exists(changed.path);
        if (snapshot.existed) {
            snapshot.content = filesystem.read_text(changed.path);
        }
        snapshots.push_back(std::move(snapshot));
    }
    return snapshots;
}

void restore_snapshot_file(IFileSystem& filesystem, const TransactionSnapshotFile& snapshot) {
    if (snapshot.existed) {
        filesystem.write_text(snapshot.path, snapshot.content);
    } else if (filesystem.exists(snapshot.path)) {
        filesystem.remove(snapshot.path);
    }
}

void write_changed_files_transactionally(IFileSystem& filesystem,
                                         const std::vector<SandboxWorldAuthoringChangedFile>& changed_files) {
    const auto snapshots = snapshot_changed_files(filesystem, changed_files);
    std::vector<std::size_t> attempted_indices;
    attempted_indices.reserve(changed_files.size());

    try {
        for (std::size_t i = 0; i < changed_files.size(); ++i) {
            attempted_indices.push_back(i);
            filesystem.write_text(changed_files[i].path, changed_files[i].content);
        }
    } catch (const std::exception& error) {
        std::string diagnostic = error.what();
        std::string rollback_diagnostic;
        for (auto index = attempted_indices.size(); index > 0U; --index) {
            const auto attempted_index = attempted_indices[index - 1U];
            try {
                restore_snapshot_file(filesystem, snapshots[attempted_index]);
            } catch (const std::exception& rollback_error) {
                if (!rollback_diagnostic.empty()) {
                    rollback_diagnostic += "; ";
                }
                rollback_diagnostic += rollback_error.what();
            }
        }
        if (!rollback_diagnostic.empty()) {
            diagnostic += "; rollback failed: ";
            diagnostic += rollback_diagnostic;
        }
        throw std::runtime_error(diagnostic);
    }
}

} // namespace

std::string_view sandbox_world_authoring_review_format_v1() noexcept {
    return sandbox_world_authoring_format;
}

SandboxWorldAuthoringReviewResult review_sandbox_world_authoring_package(const SandboxWorldAuthoringReviewDesc& desc) {
    SandboxWorldAuthoringReviewResult result;
    result.tile_definition_rows = static_cast<std::uint32_t>(desc.package.tile_definitions.size());
    result.palette_brush_rows = static_cast<std::uint32_t>(desc.package.palette_brushes.size());
    result.chunk_template_rows = static_cast<std::uint32_t>(desc.package.chunk_templates.size());
    result.procedural_seed_rows = static_cast<std::uint32_t>(desc.package.procedural_seeds.size());

    if (desc.package.review_document_path.empty()) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::missing_review_document_path,
                       "sandbox world authoring review document path is required");
    } else if (!is_safe_tilemap_package_relative_path(desc.package.review_document_path)) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_review_document_path,
                       "sandbox world authoring review document path must be package-relative", {},
                       desc.package.review_document_path);
    }
    if (!is_safe_tilemap_package_relative_path(desc.package_index_path)) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::invalid_package_path,
                       "sandbox world package index path must be package-relative", {}, desc.package_index_path);
    }
    if (desc.package.request_external_image_decoding) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unsupported_external_image_decoding,
                       "sandbox world authoring review does not execute external image decoding");
    }
    if (desc.package.request_external_download) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unsupported_external_download,
                       "sandbox world authoring review does not execute external downloads");
    }
    if (desc.package.request_importer_plugin) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::unsupported_importer_plugin,
                       "sandbox world authoring review does not execute arbitrary importer plugins");
    }

    std::unordered_set<std::string> tile_ids;
    validate_tile_definitions(result, desc.package, tile_ids);
    validate_brushes(result, desc.package, tile_ids);
    validate_chunk_templates(result, desc.package, tile_ids);
    validate_procedural_seeds(result, desc.package, tile_ids);

    if (!result.diagnostics.empty()) {
        result.status = SandboxWorldAuthoringStatus::invalid_request;
        return result;
    }

    result.review_document_content = serialize_review_document(desc.package);
    result.deterministic_preview_hash = hash_asset_cooked_content(result.review_document_content);
    result.tilemap_package_update = plan_cooked_tilemap_package_update(CookedTilemapPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = desc.package_index_content,
        .tilemap = desc.package.tilemap,
    });
    if (!result.tilemap_package_update.succeeded()) {
        append_tilemap_failures(result, result.tilemap_package_update.failures);
        result.status = SandboxWorldAuthoringStatus::invalid_request;
        return result;
    }

    append_changed_file(result.changed_files, desc.package.tilemap.output_path, "GameEngine.Tilemap.v1",
                        result.tilemap_package_update.tilemap_content);
    append_changed_file(result.changed_files, desc.package.review_document_path,
                        std::string{sandbox_world_authoring_format}, result.review_document_content);
    append_changed_file(result.changed_files, desc.package_index_path, "GameEngine.CookedPackageIndex.v1",
                        result.tilemap_package_update.package_index_content);
    result.status = SandboxWorldAuthoringStatus::ready;
    return result;
}

SandboxWorldAuthoringReviewResult apply_sandbox_world_authoring_package(IFileSystem& filesystem,
                                                                        const SandboxWorldAuthoringApplyDesc& desc) {
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        SandboxWorldAuthoringReviewResult result;
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::apply_read_failed,
                       std::string{"failed to read sandbox world package index: "} + error.what(), {},
                       desc.package_index_path);
        result.status = SandboxWorldAuthoringStatus::invalid_request;
        return result;
    }

    auto result = review_sandbox_world_authoring_package(SandboxWorldAuthoringReviewDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = std::move(package_index_content),
        .package = desc.package,
    });
    if (!result.succeeded()) {
        return result;
    }

    result.package_apply_invoked = true;
    try {
        write_changed_files_transactionally(filesystem, result.changed_files);
    } catch (const std::exception& error) {
        add_diagnostic(result, SandboxWorldAuthoringDiagnosticCode::apply_write_failed,
                       std::string{"failed to write sandbox world authoring package update: "} + error.what(), {},
                       desc.package_index_path);
        result.status = SandboxWorldAuthoringStatus::invalid_request;
    }
    return result;
}

} // namespace mirakana
