// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/tilemap_tool.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class SandboxWorldTileCollisionKind : std::uint8_t {
    empty,
    solid,
    platform,
    liquid,
    trigger,
};

enum class SandboxWorldTileUpdatePolicy : std::uint8_t {
    none,
    scheduled,
    random_tick,
};

enum class SandboxWorldBrushShape : std::uint8_t {
    single_cell,
    rectangle,
    flood_fill,
};

enum class SandboxWorldBrushReplacementPolicy : std::uint8_t {
    empty_only,
    matching_tile,
    any,
};

enum class SandboxWorldBrushSymmetry : std::uint8_t {
    none,
    mirror_x,
    mirror_y,
    four_way,
};

enum class SandboxWorldBrushFillPolicy : std::uint8_t {
    paint,
    replace,
    erase,
};

enum class SandboxWorldAuthoringStatus : std::uint8_t {
    ready,
    invalid_request,
};

enum class SandboxWorldAuthoringDiagnosticCode : std::uint8_t {
    none,
    missing_review_document_path,
    invalid_review_document_path,
    missing_tile_id,
    duplicate_tile_id,
    missing_atlas_frame_id,
    missing_atlas_page,
    invalid_uv_rect,
    invalid_material_tag,
    invalid_localization_key,
    invalid_accessibility_label_key,
    missing_provenance,
    missing_license_id,
    missing_tile_definition,
    unknown_tile_definition,
    invalid_brush_id,
    duplicate_brush_id,
    unknown_brush_tile,
    invalid_layer_mask,
    invalid_brush_path,
    invalid_chunk_template_id,
    duplicate_chunk_template_id,
    invalid_chunk_template_extent,
    unknown_allowed_tile,
    invalid_object_placement_rule,
    invalid_generator_id,
    duplicate_generator_id,
    invalid_procedural_seed_extent,
    invalid_package_path,
    invalid_package_index,
    tilemap_package_update_failed,
    apply_read_failed,
    apply_write_failed,
    unsupported_external_image_decoding,
    unsupported_external_download,
    unsupported_importer_plugin,
};

struct SandboxWorldObjectPlacementRuleRow {
    std::string object_id;
    std::string anchor_tile_id;
    std::uint32_t max_per_chunk{0};
    std::uint32_t spawn_weight{0};
};

struct SandboxWorldTileDefinitionRow {
    std::string tile_id;
    std::string atlas_frame_id;
    AssetId atlas_page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    SandboxWorldTileCollisionKind collision_kind{SandboxWorldTileCollisionKind::empty};
    std::vector<std::string> material_tags;
    bool emits_light{false};
    std::uint32_t light_radius{0};
    bool liquid{false};
    SandboxWorldTileUpdatePolicy update_policy{SandboxWorldTileUpdatePolicy::none};
    std::string localization_key;
    std::string accessibility_label_key;
    std::string provenance;
    std::string license_id;
    std::uint32_t source_index{0};
};

struct SandboxWorldPaletteBrushRow {
    std::string brush_id;
    SandboxWorldBrushShape shape{SandboxWorldBrushShape::single_cell};
    std::uint32_t layer_mask{0};
    std::string tile_id;
    SandboxWorldBrushReplacementPolicy replacement_policy{SandboxWorldBrushReplacementPolicy::empty_only};
    SandboxWorldBrushSymmetry symmetry{SandboxWorldBrushSymmetry::none};
    SandboxWorldBrushFillPolicy fill_policy{SandboxWorldBrushFillPolicy::paint};
    std::string preview_path;
    std::uint32_t source_index{0};
};

struct SandboxWorldChunkTemplateRow {
    std::string template_id;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::string> allowed_tile_ids;
    std::vector<SandboxWorldObjectPlacementRuleRow> object_placement_rules;
    std::uint32_t source_index{0};
};

struct SandboxWorldProceduralSeedRow {
    std::string generator_id;
    std::uint64_t seed{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::string> allowed_tile_ids;
    std::vector<SandboxWorldObjectPlacementRuleRow> object_placement_rules;
    std::uint32_t source_index{0};
};

struct SandboxWorldAuthoringPackageDesc {
    std::string review_document_path;
    CookedTilemapAuthoringDesc tilemap;
    std::vector<SandboxWorldTileDefinitionRow> tile_definitions;
    std::vector<SandboxWorldPaletteBrushRow> palette_brushes;
    std::vector<SandboxWorldChunkTemplateRow> chunk_templates;
    std::vector<SandboxWorldProceduralSeedRow> procedural_seeds;
    bool request_external_image_decoding{false};
    bool request_external_download{false};
    bool request_importer_plugin{false};
};

struct SandboxWorldAuthoringReviewDesc {
    std::string package_index_path;
    std::string package_index_content;
    SandboxWorldAuthoringPackageDesc package;
};

struct SandboxWorldAuthoringApplyDesc {
    std::string package_index_path;
    SandboxWorldAuthoringPackageDesc package;
};

struct SandboxWorldAuthoringChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct SandboxWorldAuthoringDiagnostic {
    SandboxWorldAuthoringDiagnosticCode code{SandboxWorldAuthoringDiagnosticCode::none};
    std::string id;
    std::string path;
    std::string message;
    std::uint32_t source_index{0};
};

struct SandboxWorldAuthoringReviewResult {
    SandboxWorldAuthoringStatus status{SandboxWorldAuthoringStatus::invalid_request};
    std::uint32_t tile_definition_rows{0};
    std::uint32_t palette_brush_rows{0};
    std::uint32_t chunk_template_rows{0};
    std::uint32_t procedural_seed_rows{0};
    std::uint64_t deterministic_preview_hash{0};
    std::string review_document_content;
    CookedTilemapPackageUpdateResult tilemap_package_update;
    std::vector<SandboxWorldAuthoringChangedFile> changed_files;
    std::vector<SandboxWorldAuthoringDiagnostic> diagnostics;
    bool external_image_decoding_invoked{false};
    bool external_download_invoked{false};
    bool importer_plugin_invoked{false};
    bool package_apply_invoked{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == SandboxWorldAuthoringStatus::ready && diagnostics.empty();
    }
};

[[nodiscard]] std::string_view sandbox_world_authoring_review_format_v1() noexcept;
[[nodiscard]] SandboxWorldAuthoringReviewResult
review_sandbox_world_authoring_package(const SandboxWorldAuthoringReviewDesc& desc);
[[nodiscard]] SandboxWorldAuthoringReviewResult
apply_sandbox_world_authoring_package(IFileSystem& filesystem, const SandboxWorldAuthoringApplyDesc& desc);

} // namespace mirakana
