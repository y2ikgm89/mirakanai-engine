// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class AssetImportTextureColorSpace : std::uint8_t { srgb, linear };
enum class AssetImportTextureMipmapPolicy : std::uint8_t { none, generate_offline };
enum class AssetImportTextureAlphaPolicy : std::uint8_t { opaque, premultiplied, straight };
enum class AssetImportTextureCompressionIntent : std::uint8_t { none, bc7, astc, basis_reviewed };

enum class AssetImportMeshUpAxis : std::uint8_t { y, z };
enum class AssetImportMeshMaterialExtraction : std::uint8_t { none, source_references };

enum class AssetImportAudioDecodeMode : std::uint8_t { static_pcm, streaming_source_review };
enum class AssetImportAudioSampleFormat : std::uint8_t { pcm16, float32 };

struct AssetImportTexturePresetV1 {
    AssetImportTextureColorSpace color_space{AssetImportTextureColorSpace::srgb};
    AssetImportTextureMipmapPolicy mipmap_policy{AssetImportTextureMipmapPolicy::none};
    AssetImportTextureAlphaPolicy alpha_policy{AssetImportTextureAlphaPolicy::straight};
    AssetImportTextureCompressionIntent compression_intent{AssetImportTextureCompressionIntent::none};
};

struct AssetImportMeshPresetV1 {
    float unit_scale{1.0F};
    AssetImportMeshUpAxis up_axis{AssetImportMeshUpAxis::y};
    bool triangulate{true};
    bool generate_normals{false};
    bool generate_tangents{false};
    AssetImportMeshMaterialExtraction material_extraction{AssetImportMeshMaterialExtraction::source_references};
};

struct AssetImportAudioPresetV1 {
    AssetImportAudioDecodeMode decode_mode{AssetImportAudioDecodeMode::static_pcm};
    AssetImportAudioSampleFormat sample_format{AssetImportAudioSampleFormat::pcm16};
    bool loop{false};
    bool normalize_peak{false};
};

struct AssetImportPresetDefaultsV1 {
    AssetImportTexturePresetV1 texture;
    AssetImportMeshPresetV1 mesh;
    AssetImportAudioPresetV1 audio;
};

struct AssetImportPresetOverrideV1 {
    AssetKeyV2 asset_key;
    std::optional<AssetImportTexturePresetV1> texture;
    std::optional<AssetImportMeshPresetV1> mesh;
    std::optional<AssetImportAudioPresetV1> audio;
};

struct AssetImportPresetsDocumentV1 {
    AssetImportPresetDefaultsV1 defaults;
    std::vector<AssetImportPresetOverrideV1> overrides;
};

struct AssetImportPresetReviewV1 {
    std::vector<std::string> metadata;
    std::vector<std::string> diagnostics;
    bool ready{true};
};

[[nodiscard]] std::string_view asset_import_texture_color_space_label(AssetImportTextureColorSpace value) noexcept;
[[nodiscard]] std::string_view asset_import_texture_mipmap_policy_label(AssetImportTextureMipmapPolicy value) noexcept;
[[nodiscard]] std::string_view asset_import_texture_alpha_policy_label(AssetImportTextureAlphaPolicy value) noexcept;
[[nodiscard]] std::string_view
asset_import_texture_compression_intent_label(AssetImportTextureCompressionIntent value) noexcept;
[[nodiscard]] std::string_view asset_import_mesh_up_axis_label(AssetImportMeshUpAxis value) noexcept;
[[nodiscard]] std::string_view
asset_import_mesh_material_extraction_label(AssetImportMeshMaterialExtraction value) noexcept;
[[nodiscard]] std::string_view asset_import_audio_decode_mode_label(AssetImportAudioDecodeMode value) noexcept;
[[nodiscard]] std::string_view asset_import_audio_sample_format_label(AssetImportAudioSampleFormat value) noexcept;

[[nodiscard]] std::vector<std::string>
validate_asset_import_presets_document(const AssetImportPresetsDocumentV1& document);
[[nodiscard]] std::string serialize_asset_import_presets_document(const AssetImportPresetsDocumentV1& document);
[[nodiscard]] AssetImportPresetsDocumentV1 deserialize_asset_import_presets_document(std::string_view text);
[[nodiscard]] AssetImportPresetReviewV1
review_asset_import_preset_for_asset(const AssetImportPresetsDocumentV1& document, const AssetKeyV2& asset_key,
                                     AssetKind asset_kind);

} // namespace mirakana
