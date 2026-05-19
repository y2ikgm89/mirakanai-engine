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

struct SpriteAtlasSourceFrameDesc {
    std::string frame_id;
    std::string source_path;
    TextureSourceDocument image;
};

struct SpriteAtlasSourceAuthoringDesc {
    std::string source_registry_path;
    std::string source_registry_content;
    AssetKeyV2 atlas_asset_key;
    std::string atlas_source_path;
    std::string atlas_imported_path;
    std::uint32_t max_side{sprite_atlas_packing_max_side};
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
    AssetKeyV2 atlas_asset_key;
    AssetId atlas_asset;
    std::string atlas_source_path;
    std::string atlas_imported_path;
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::uint64_t source_content_hash{0};
};

struct SpriteAtlasSourceAuthoringDiagnostic {
    std::string code;
    std::string message;
    std::string path;
    std::string frame_id;
    AssetKeyV2 asset_key;
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
