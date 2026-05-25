// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/assets/sprite_atlas_packing.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct SpriteAtlasSourcePivot {
    float x{0.5F};
    float y{0.5F};
};

struct SpriteAtlasSourceSliceBorder {
    float left{0.0F};
    float bottom{0.0F};
    float right{0.0F};
    float top{0.0F};
};

struct SpriteAtlasSourcePagePolicyDesc {
    std::string mode{"single-page-tight-rgba8-texture-source"};
    std::string page_id{"page-0"};
    std::uint32_t page_count{1};
    std::uint32_t padding_pixels{0};
};

struct SpriteAtlasSourceFrameDesc {
    std::string frame_id;
    std::string source_path;
    TextureSourceDocument image;
    SpriteAtlasSourcePivot pivot;
    SpriteAtlasSourceSliceBorder slice_border;
};

struct SpriteAtlasSourceAuthoringDesc {
    std::string source_registry_path;
    std::string source_registry_content;
    AssetKey atlas_asset_key;
    std::string atlas_source_path;
    std::string atlas_imported_path;
    std::uint32_t max_side{sprite_atlas_packing_max_side};
    SpriteAtlasSourcePagePolicyDesc page_policy;
    std::string source_decoding{"provided-rgba8-texture-source"};
    std::string atlas_packing{"deterministic-sprite-atlas-rgba8-max-side"};
    std::vector<SpriteAtlasSourceFrameDesc> frames;

    std::string runtime_source_image_decoding{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string animation_semantics{"unsupported"};
    std::string editor_productization{"unsupported"};
    std::string free_form_edit{"unsupported"};
};

struct SpriteAtlasSourceChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct SpriteAtlasSourceFrameRow {
    std::string frame_id;
    std::string source_path;
    AssetKey atlas_asset_key;
    AssetId atlas_asset;
    std::string atlas_source_path;
    std::string atlas_imported_path;
    std::uint32_t page_index{0};
    std::string page_id;
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    float pivot_x{0.5F};
    float pivot_y{0.5F};
    float slice_border_left{0.0F};
    float slice_border_bottom{0.0F};
    float slice_border_right{0.0F};
    float slice_border_top{0.0F};
    std::uint64_t source_content_hash{0};
};

struct SpriteAtlasSourceAuthoringDiagnostic {
    std::string code;
    std::string message;
    std::string path;
    std::string frame_id;
    AssetKey asset_key;
};

struct SpriteAtlasSourceAuthoringPlan {
    TextureSourceDocument atlas_texture;
    std::string atlas_texture_content;
    std::string source_registry_content;
    std::vector<SpriteAtlasSourceChangedFile> changed_files;
    std::vector<SpriteAtlasSourceFrameRow> frame_rows;
    std::vector<SpriteAtlasSourceAuthoringDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] SpriteAtlasSourceAuthoringPlan
plan_sprite_atlas_source_authoring(const SpriteAtlasSourceAuthoringDesc& desc);

} // namespace mirakana
