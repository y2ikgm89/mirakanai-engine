// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_presets.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view asset_import_presets_format_v1 = "GameEngine.AssetImportPresets.v1";

struct TexturePresetText {
    bool has_color_space{false};
    bool has_mipmap_policy{false};
    bool has_alpha_policy{false};
    bool has_compression_intent{false};
    AssetImportTexturePresetV1 preset;
};

struct MeshPresetText {
    bool has_unit_scale{false};
    bool has_up_axis{false};
    bool has_triangulate{false};
    bool has_generate_normals{false};
    bool has_generate_tangents{false};
    bool has_material_extraction{false};
    AssetImportMeshPresetV1 preset;
};

struct AudioPresetText {
    bool has_decode_mode{false};
    bool has_sample_format{false};
    bool has_loop{false};
    bool has_normalize_peak{false};
    AssetImportAudioPresetV1 preset;
};

struct OverridePresetText {
    bool has_asset_key{false};
    AssetKeyV2 asset_key;
    TexturePresetText texture;
    MeshPresetText mesh;
    AudioPresetText audio;
};

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool contains_ascii_whitespace(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool valid_asset_key(std::string_view key) noexcept {
    return !key.empty() && key.front() != '/' && key.find('\\') == std::string_view::npos &&
           key.find(':') == std::string_view::npos && key.find(';') == std::string_view::npos &&
           !contains_ascii_whitespace(key) && !contains_parent_segment(key) &&
           !std::ranges::any_of(key, is_ascii_control);
}

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_texture_color_space(AssetImportTextureColorSpace value) noexcept {
    switch (value) {
    case AssetImportTextureColorSpace::srgb:
    case AssetImportTextureColorSpace::linear:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_texture_mipmap_policy(AssetImportTextureMipmapPolicy value) noexcept {
    switch (value) {
    case AssetImportTextureMipmapPolicy::none:
    case AssetImportTextureMipmapPolicy::generate_offline:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_texture_alpha_policy(AssetImportTextureAlphaPolicy value) noexcept {
    switch (value) {
    case AssetImportTextureAlphaPolicy::opaque:
    case AssetImportTextureAlphaPolicy::premultiplied:
    case AssetImportTextureAlphaPolicy::straight:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_texture_compression_intent(AssetImportTextureCompressionIntent value) noexcept {
    switch (value) {
    case AssetImportTextureCompressionIntent::none:
    case AssetImportTextureCompressionIntent::bc7:
    case AssetImportTextureCompressionIntent::astc:
    case AssetImportTextureCompressionIntent::basis_reviewed:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_mesh_up_axis(AssetImportMeshUpAxis value) noexcept {
    switch (value) {
    case AssetImportMeshUpAxis::y:
    case AssetImportMeshUpAxis::z:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_mesh_material_extraction(AssetImportMeshMaterialExtraction value) noexcept {
    switch (value) {
    case AssetImportMeshMaterialExtraction::none:
    case AssetImportMeshMaterialExtraction::source_references:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_audio_decode_mode(AssetImportAudioDecodeMode value) noexcept {
    switch (value) {
    case AssetImportAudioDecodeMode::static_pcm:
    case AssetImportAudioDecodeMode::streaming_source_review:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_audio_sample_format(AssetImportAudioSampleFormat value) noexcept {
    switch (value) {
    case AssetImportAudioSampleFormat::pcm16:
    case AssetImportAudioSampleFormat::float32:
        return true;
    }
    return false;
}

void push(std::vector<std::string>& diagnostics, std::string_view prefix, std::string_view code) {
    diagnostics.push_back(std::string{prefix} + "." + std::string{code});
}

void validate_texture_preset(std::vector<std::string>& diagnostics, std::string_view prefix,
                             const AssetImportTexturePresetV1& preset) {
    if (!valid_texture_color_space(preset.color_space)) {
        push(diagnostics, prefix, "invalid_color_space");
    }
    if (!valid_texture_mipmap_policy(preset.mipmap_policy)) {
        push(diagnostics, prefix, "invalid_mipmap_policy");
    }
    if (!valid_texture_alpha_policy(preset.alpha_policy)) {
        push(diagnostics, prefix, "invalid_alpha_policy");
    }
    if (!valid_texture_compression_intent(preset.compression_intent)) {
        push(diagnostics, prefix, "invalid_compression_intent");
    }
    if (preset.alpha_policy == AssetImportTextureAlphaPolicy::premultiplied &&
        preset.compression_intent == AssetImportTextureCompressionIntent::basis_reviewed) {
        push(diagnostics, prefix, "basis_reviewed_premultiplied_alpha_requires_transcode_review");
    }
}

void validate_mesh_preset(std::vector<std::string>& diagnostics, std::string_view prefix,
                          const AssetImportMeshPresetV1& preset) {
    if (!std::isfinite(preset.unit_scale) || preset.unit_scale <= 0.0F || preset.unit_scale > 1000000.0F) {
        push(diagnostics, prefix, "invalid_unit_scale");
    }
    if (!valid_mesh_up_axis(preset.up_axis)) {
        push(diagnostics, prefix, "invalid_up_axis");
    }
    if (!valid_mesh_material_extraction(preset.material_extraction)) {
        push(diagnostics, prefix, "invalid_material_extraction");
    }
    if (preset.generate_tangents && !preset.generate_normals) {
        push(diagnostics, prefix, "tangent_generation_requires_normals");
    }
}

void validate_audio_preset(std::vector<std::string>& diagnostics, std::string_view prefix,
                           const AssetImportAudioPresetV1& preset) {
    if (!valid_audio_decode_mode(preset.decode_mode)) {
        push(diagnostics, prefix, "invalid_decode_mode");
    }
    if (!valid_audio_sample_format(preset.sample_format)) {
        push(diagnostics, prefix, "invalid_sample_format");
    }
    if (preset.decode_mode == AssetImportAudioDecodeMode::streaming_source_review && preset.normalize_peak) {
        push(diagnostics, prefix, "streaming_decode_cannot_normalize_peak");
    }
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value, std::string_view message) {
    if (value.empty()) {
        throw std::invalid_argument(std::string{message});
    }
    std::size_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string{message});
        }
        const auto digit = static_cast<std::size_t>(character - '0');
        if (parsed > (std::numeric_limits<std::size_t>::max() - digit) / 10U) {
            throw std::invalid_argument(std::string{message});
        }
        parsed = (parsed * 10U) + digit;
    }
    return parsed;
}

[[nodiscard]] float parse_float(std::string_view value) {
    if (value.empty() || !clean_text(value) || contains_ascii_whitespace(value)) {
        throw std::invalid_argument("asset import presets float value is invalid");
    }

    std::istringstream input{std::string{value}};
    input.imbue(std::locale::classic());
    float parsed = 0.0F;
    input >> parsed;
    if (input.fail() || !input.eof() || !std::isfinite(parsed)) {
        throw std::invalid_argument("asset import presets float value is invalid");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("asset import presets bool value is invalid");
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] std::string float_text(float value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("asset import presets float value is invalid");
    }
    for (int precision = 1; precision <= std::numeric_limits<float>::max_digits10; ++precision) {
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << std::setprecision(precision) << value;
        const auto text = output.str();
        if (parse_float(text) == value) {
            return text;
        }
    }

    std::ostringstream fallback;
    fallback.imbue(std::locale::classic());
    fallback << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    return fallback.str();
}

[[nodiscard]] AssetImportTextureColorSpace parse_texture_color_space(std::string_view value) {
    if (value == "srgb") {
        return AssetImportTextureColorSpace::srgb;
    }
    if (value == "linear") {
        return AssetImportTextureColorSpace::linear;
    }
    throw std::invalid_argument("asset import presets texture color space is unsupported");
}

[[nodiscard]] AssetImportTextureMipmapPolicy parse_texture_mipmap_policy(std::string_view value) {
    if (value == "none") {
        return AssetImportTextureMipmapPolicy::none;
    }
    if (value == "generate_offline") {
        return AssetImportTextureMipmapPolicy::generate_offline;
    }
    throw std::invalid_argument("asset import presets texture mipmap policy is unsupported");
}

[[nodiscard]] AssetImportTextureAlphaPolicy parse_texture_alpha_policy(std::string_view value) {
    if (value == "opaque") {
        return AssetImportTextureAlphaPolicy::opaque;
    }
    if (value == "premultiplied") {
        return AssetImportTextureAlphaPolicy::premultiplied;
    }
    if (value == "straight") {
        return AssetImportTextureAlphaPolicy::straight;
    }
    throw std::invalid_argument("asset import presets texture alpha policy is unsupported");
}

[[nodiscard]] AssetImportTextureCompressionIntent parse_texture_compression_intent(std::string_view value) {
    if (value == "none") {
        return AssetImportTextureCompressionIntent::none;
    }
    if (value == "bc7") {
        return AssetImportTextureCompressionIntent::bc7;
    }
    if (value == "astc") {
        return AssetImportTextureCompressionIntent::astc;
    }
    if (value == "basis_reviewed") {
        return AssetImportTextureCompressionIntent::basis_reviewed;
    }
    throw std::invalid_argument("asset import presets texture compression intent is unsupported");
}

[[nodiscard]] AssetImportMeshUpAxis parse_mesh_up_axis(std::string_view value) {
    if (value == "y") {
        return AssetImportMeshUpAxis::y;
    }
    if (value == "z") {
        return AssetImportMeshUpAxis::z;
    }
    throw std::invalid_argument("asset import presets mesh up axis is unsupported");
}

[[nodiscard]] AssetImportMeshMaterialExtraction parse_mesh_material_extraction(std::string_view value) {
    if (value == "none") {
        return AssetImportMeshMaterialExtraction::none;
    }
    if (value == "source_references") {
        return AssetImportMeshMaterialExtraction::source_references;
    }
    throw std::invalid_argument("asset import presets mesh material extraction is unsupported");
}

[[nodiscard]] AssetImportAudioDecodeMode parse_audio_decode_mode(std::string_view value) {
    if (value == "static_pcm") {
        return AssetImportAudioDecodeMode::static_pcm;
    }
    if (value == "streaming_source_review") {
        return AssetImportAudioDecodeMode::streaming_source_review;
    }
    throw std::invalid_argument("asset import presets audio decode mode is unsupported");
}

[[nodiscard]] AssetImportAudioSampleFormat parse_audio_sample_format(std::string_view value) {
    if (value == "pcm16") {
        return AssetImportAudioSampleFormat::pcm16;
    }
    if (value == "float32") {
        return AssetImportAudioSampleFormat::float32;
    }
    throw std::invalid_argument("asset import presets audio sample format is unsupported");
}

void parse_texture_field(TexturePresetText& text, std::string_view field, std::string_view value) {
    if (field == "color_space") {
        text.has_color_space = true;
        text.preset.color_space = parse_texture_color_space(value);
    } else if (field == "mipmap_policy") {
        text.has_mipmap_policy = true;
        text.preset.mipmap_policy = parse_texture_mipmap_policy(value);
    } else if (field == "alpha_policy") {
        text.has_alpha_policy = true;
        text.preset.alpha_policy = parse_texture_alpha_policy(value);
    } else if (field == "compression_intent") {
        text.has_compression_intent = true;
        text.preset.compression_intent = parse_texture_compression_intent(value);
    } else {
        throw std::invalid_argument("asset import presets texture field is unsupported");
    }
}

void parse_mesh_field(MeshPresetText& text, std::string_view field, std::string_view value) {
    if (field == "unit_scale") {
        text.has_unit_scale = true;
        text.preset.unit_scale = parse_float(value);
    } else if (field == "up_axis") {
        text.has_up_axis = true;
        text.preset.up_axis = parse_mesh_up_axis(value);
    } else if (field == "triangulate") {
        text.has_triangulate = true;
        text.preset.triangulate = parse_bool(value);
    } else if (field == "generate_normals") {
        text.has_generate_normals = true;
        text.preset.generate_normals = parse_bool(value);
    } else if (field == "generate_tangents") {
        text.has_generate_tangents = true;
        text.preset.generate_tangents = parse_bool(value);
    } else if (field == "material_extraction") {
        text.has_material_extraction = true;
        text.preset.material_extraction = parse_mesh_material_extraction(value);
    } else {
        throw std::invalid_argument("asset import presets mesh field is unsupported");
    }
}

void parse_audio_field(AudioPresetText& text, std::string_view field, std::string_view value) {
    if (field == "decode_mode") {
        text.has_decode_mode = true;
        text.preset.decode_mode = parse_audio_decode_mode(value);
    } else if (field == "sample_format") {
        text.has_sample_format = true;
        text.preset.sample_format = parse_audio_sample_format(value);
    } else if (field == "loop") {
        text.has_loop = true;
        text.preset.loop = parse_bool(value);
    } else if (field == "normalize_peak") {
        text.has_normalize_peak = true;
        text.preset.normalize_peak = parse_bool(value);
    } else {
        throw std::invalid_argument("asset import presets audio field is unsupported");
    }
}

[[nodiscard]] bool complete(const TexturePresetText& text) noexcept {
    return text.has_color_space && text.has_mipmap_policy && text.has_alpha_policy && text.has_compression_intent;
}

[[nodiscard]] bool complete(const MeshPresetText& text) noexcept {
    return text.has_unit_scale && text.has_up_axis && text.has_triangulate && text.has_generate_normals &&
           text.has_generate_tangents && text.has_material_extraction;
}

[[nodiscard]] bool complete(const AudioPresetText& text) noexcept {
    return text.has_decode_mode && text.has_sample_format && text.has_loop && text.has_normalize_peak;
}

[[nodiscard]] bool touched(const TexturePresetText& text) noexcept {
    return text.has_color_space || text.has_mipmap_policy || text.has_alpha_policy || text.has_compression_intent;
}

[[nodiscard]] bool touched(const MeshPresetText& text) noexcept {
    return text.has_unit_scale || text.has_up_axis || text.has_triangulate || text.has_generate_normals ||
           text.has_generate_tangents || text.has_material_extraction;
}

[[nodiscard]] bool touched(const AudioPresetText& text) noexcept {
    return text.has_decode_mode || text.has_sample_format || text.has_loop || text.has_normalize_peak;
}

void parse_defaults_field(TexturePresetText& texture, MeshPresetText& mesh, AudioPresetText& audio,
                          std::string_view key, std::string_view value) {
    constexpr std::string_view texture_prefix = "defaults.texture.";
    constexpr std::string_view mesh_prefix = "defaults.mesh.";
    constexpr std::string_view audio_prefix = "defaults.audio.";
    if (key.starts_with(texture_prefix)) {
        parse_texture_field(texture, key.substr(texture_prefix.size()), value);
    } else if (key.starts_with(mesh_prefix)) {
        parse_mesh_field(mesh, key.substr(mesh_prefix.size()), value);
    } else if (key.starts_with(audio_prefix)) {
        parse_audio_field(audio, key.substr(audio_prefix.size()), value);
    } else {
        throw std::invalid_argument("asset import presets text contains unsupported defaults key");
    }
}

void parse_override_field(std::unordered_map<std::size_t, OverridePresetText>& overrides, std::string_view key,
                          std::string_view value) {
    constexpr std::string_view prefix = "override.";
    if (!key.starts_with(prefix)) {
        throw std::invalid_argument("asset import presets text contains unsupported key");
    }

    const auto after_prefix = key.substr(prefix.size());
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("asset import presets override key is malformed");
    }
    const auto ordinal =
        parse_ordinal(after_prefix.substr(0, separator), "asset import presets override ordinal is invalid");
    const auto field = after_prefix.substr(separator + 1U);
    auto& row = overrides[ordinal];

    if (field == "asset_key") {
        row.has_asset_key = true;
        row.asset_key.value = std::string{value};
        return;
    }

    constexpr std::string_view texture_prefix = "texture.";
    constexpr std::string_view mesh_prefix = "mesh.";
    constexpr std::string_view audio_prefix = "audio.";
    if (field.starts_with(texture_prefix)) {
        parse_texture_field(row.texture, field.substr(texture_prefix.size()), value);
    } else if (field.starts_with(mesh_prefix)) {
        parse_mesh_field(row.mesh, field.substr(mesh_prefix.size()), value);
    } else if (field.starts_with(audio_prefix)) {
        parse_audio_field(row.audio, field.substr(audio_prefix.size()), value);
    } else {
        throw std::invalid_argument("asset import presets override field is unsupported");
    }
}

void write_texture(std::ostringstream& output, std::string_view prefix, const AssetImportTexturePresetV1& preset) {
    output << prefix << ".color_space=" << asset_import_texture_color_space_label(preset.color_space) << '\n';
    output << prefix << ".mipmap_policy=" << asset_import_texture_mipmap_policy_label(preset.mipmap_policy) << '\n';
    output << prefix << ".alpha_policy=" << asset_import_texture_alpha_policy_label(preset.alpha_policy) << '\n';
    output << prefix
           << ".compression_intent=" << asset_import_texture_compression_intent_label(preset.compression_intent)
           << '\n';
}

void write_mesh(std::ostringstream& output, std::string_view prefix, const AssetImportMeshPresetV1& preset) {
    output << prefix << ".unit_scale=" << float_text(preset.unit_scale) << '\n';
    output << prefix << ".up_axis=" << asset_import_mesh_up_axis_label(preset.up_axis) << '\n';
    output << prefix << ".triangulate=" << bool_text(preset.triangulate) << '\n';
    output << prefix << ".generate_normals=" << bool_text(preset.generate_normals) << '\n';
    output << prefix << ".generate_tangents=" << bool_text(preset.generate_tangents) << '\n';
    output << prefix
           << ".material_extraction=" << asset_import_mesh_material_extraction_label(preset.material_extraction)
           << '\n';
}

void write_audio(std::ostringstream& output, std::string_view prefix, const AssetImportAudioPresetV1& preset) {
    output << prefix << ".decode_mode=" << asset_import_audio_decode_mode_label(preset.decode_mode) << '\n';
    output << prefix << ".sample_format=" << asset_import_audio_sample_format_label(preset.sample_format) << '\n';
    output << prefix << ".loop=" << bool_text(preset.loop) << '\n';
    output << prefix << ".normalize_peak=" << bool_text(preset.normalize_peak) << '\n';
}

[[nodiscard]] std::vector<std::string> texture_metadata(const AssetImportTexturePresetV1& preset) {
    return {
        "texture.color_space=" + std::string{asset_import_texture_color_space_label(preset.color_space)},
        "texture.mipmap_policy=" + std::string{asset_import_texture_mipmap_policy_label(preset.mipmap_policy)},
        "texture.alpha_policy=" + std::string{asset_import_texture_alpha_policy_label(preset.alpha_policy)},
        "texture.compression_intent=" +
            std::string{asset_import_texture_compression_intent_label(preset.compression_intent)},
    };
}

[[nodiscard]] std::vector<std::string> mesh_metadata(const AssetImportMeshPresetV1& preset) {
    return {
        "mesh.unit_scale=" + float_text(preset.unit_scale),
        "mesh.up_axis=" + std::string{asset_import_mesh_up_axis_label(preset.up_axis)},
        "mesh.triangulate=" + std::string{bool_text(preset.triangulate)},
        "mesh.generate_normals=" + std::string{bool_text(preset.generate_normals)},
        "mesh.generate_tangents=" + std::string{bool_text(preset.generate_tangents)},
        "mesh.material_extraction=" +
            std::string{asset_import_mesh_material_extraction_label(preset.material_extraction)},
    };
}

[[nodiscard]] std::vector<std::string> audio_metadata(const AssetImportAudioPresetV1& preset) {
    return {
        "audio.decode_mode=" + std::string{asset_import_audio_decode_mode_label(preset.decode_mode)},
        "audio.sample_format=" + std::string{asset_import_audio_sample_format_label(preset.sample_format)},
        "audio.loop=" + std::string{bool_text(preset.loop)},
        "audio.normalize_peak=" + std::string{bool_text(preset.normalize_peak)},
    };
}

[[nodiscard]] bool exactly_one_override_kind(const AssetImportPresetOverrideV1& row) noexcept {
    const int count =
        (row.texture.has_value() ? 1 : 0) + (row.mesh.has_value() ? 1 : 0) + (row.audio.has_value() ? 1 : 0);
    return count == 1;
}

[[nodiscard]] const AssetImportPresetOverrideV1*
find_unique_override_for_review(const AssetImportPresetsDocumentV1& document, const AssetKeyV2& asset_key,
                                std::vector<std::string>& diagnostics) {
    if (!valid_asset_key(asset_key.value)) {
        push(diagnostics, "asset", "invalid_asset_key");
        return nullptr;
    }

    const AssetImportPresetOverrideV1* matching_override = nullptr;
    for (std::size_t index = 0; index < document.overrides.size(); ++index) {
        const auto& row = document.overrides[index];
        if (row.asset_key.value != asset_key.value) {
            continue;
        }

        const std::string prefix = "override." + std::to_string(index);
        if (!valid_asset_key(row.asset_key.value)) {
            push(diagnostics, prefix, "invalid_asset_key");
        }
        if (!exactly_one_override_kind(row)) {
            push(diagnostics, prefix, "requires_exactly_one_preset_kind");
        }
        if (matching_override != nullptr) {
            push(diagnostics, prefix, "duplicate_asset_key");
        } else {
            matching_override = &row;
        }
    }

    return diagnostics.empty() ? matching_override : nullptr;
}

} // namespace

std::string_view asset_import_texture_color_space_label(AssetImportTextureColorSpace value) noexcept {
    switch (value) {
    case AssetImportTextureColorSpace::srgb:
        return "srgb";
    case AssetImportTextureColorSpace::linear:
        return "linear";
    }
    return "invalid";
}

std::string_view asset_import_texture_mipmap_policy_label(AssetImportTextureMipmapPolicy value) noexcept {
    switch (value) {
    case AssetImportTextureMipmapPolicy::none:
        return "none";
    case AssetImportTextureMipmapPolicy::generate_offline:
        return "generate_offline";
    }
    return "invalid";
}

std::string_view asset_import_texture_alpha_policy_label(AssetImportTextureAlphaPolicy value) noexcept {
    switch (value) {
    case AssetImportTextureAlphaPolicy::opaque:
        return "opaque";
    case AssetImportTextureAlphaPolicy::premultiplied:
        return "premultiplied";
    case AssetImportTextureAlphaPolicy::straight:
        return "straight";
    }
    return "invalid";
}

std::string_view asset_import_texture_compression_intent_label(AssetImportTextureCompressionIntent value) noexcept {
    switch (value) {
    case AssetImportTextureCompressionIntent::none:
        return "none";
    case AssetImportTextureCompressionIntent::bc7:
        return "bc7";
    case AssetImportTextureCompressionIntent::astc:
        return "astc";
    case AssetImportTextureCompressionIntent::basis_reviewed:
        return "basis_reviewed";
    }
    return "invalid";
}

std::string_view asset_import_mesh_up_axis_label(AssetImportMeshUpAxis value) noexcept {
    switch (value) {
    case AssetImportMeshUpAxis::y:
        return "y";
    case AssetImportMeshUpAxis::z:
        return "z";
    }
    return "invalid";
}

std::string_view asset_import_mesh_material_extraction_label(AssetImportMeshMaterialExtraction value) noexcept {
    switch (value) {
    case AssetImportMeshMaterialExtraction::none:
        return "none";
    case AssetImportMeshMaterialExtraction::source_references:
        return "source_references";
    }
    return "invalid";
}

std::string_view asset_import_audio_decode_mode_label(AssetImportAudioDecodeMode value) noexcept {
    switch (value) {
    case AssetImportAudioDecodeMode::static_pcm:
        return "static_pcm";
    case AssetImportAudioDecodeMode::streaming_source_review:
        return "streaming_source_review";
    }
    return "invalid";
}

std::string_view asset_import_audio_sample_format_label(AssetImportAudioSampleFormat value) noexcept {
    switch (value) {
    case AssetImportAudioSampleFormat::pcm16:
        return "pcm16";
    case AssetImportAudioSampleFormat::float32:
        return "float32";
    }
    return "invalid";
}

std::vector<std::string> validate_asset_import_presets_document(const AssetImportPresetsDocumentV1& document) {
    std::vector<std::string> diagnostics;
    validate_texture_preset(diagnostics, "defaults.texture", document.defaults.texture);
    validate_mesh_preset(diagnostics, "defaults.mesh", document.defaults.mesh);
    validate_audio_preset(diagnostics, "defaults.audio", document.defaults.audio);

    std::unordered_set<std::string> asset_keys;
    for (std::size_t index = 0; index < document.overrides.size(); ++index) {
        const auto& row = document.overrides[index];
        const std::string prefix = "override." + std::to_string(index);
        if (!valid_asset_key(row.asset_key.value)) {
            push(diagnostics, prefix, "invalid_asset_key");
        } else if (!asset_keys.insert(row.asset_key.value).second) {
            push(diagnostics, prefix, "duplicate_asset_key");
        }
        if (!exactly_one_override_kind(row)) {
            push(diagnostics, prefix, "requires_exactly_one_preset_kind");
        }
        if (row.texture.has_value()) {
            validate_texture_preset(diagnostics, prefix + ".texture", *row.texture);
        }
        if (row.mesh.has_value()) {
            validate_mesh_preset(diagnostics, prefix + ".mesh", *row.mesh);
        }
        if (row.audio.has_value()) {
            validate_audio_preset(diagnostics, prefix + ".audio", *row.audio);
        }
    }

    return diagnostics;
}

std::string serialize_asset_import_presets_document(const AssetImportPresetsDocumentV1& document) {
    const auto diagnostics = validate_asset_import_presets_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import presets document is invalid");
    }

    std::ostringstream output;
    output << "format=" << asset_import_presets_format_v1 << '\n';
    write_texture(output, "defaults.texture", document.defaults.texture);
    write_mesh(output, "defaults.mesh", document.defaults.mesh);
    write_audio(output, "defaults.audio", document.defaults.audio);
    output << "override.count=" << document.overrides.size() << '\n';
    for (std::size_t index = 0; index < document.overrides.size(); ++index) {
        const auto& row = document.overrides[index];
        const std::string prefix = "override." + std::to_string(index);
        output << prefix << ".asset_key=" << row.asset_key.value << '\n';
        if (row.texture.has_value()) {
            write_texture(output, prefix + ".texture", *row.texture);
        } else if (row.mesh.has_value()) {
            write_mesh(output, prefix + ".mesh", *row.mesh);
        } else if (row.audio.has_value()) {
            write_audio(output, prefix + ".audio", *row.audio);
        }
    }
    return output.str();
}

AssetImportPresetsDocumentV1 deserialize_asset_import_presets_document(std::string_view text) {
    bool saw_format = false;
    bool saw_override_count = false;
    std::size_t override_count = 0;
    std::unordered_set<std::string> seen_keys;
    TexturePresetText default_texture;
    MeshPresetText default_mesh;
    AudioPresetText default_audio;
    std::unordered_map<std::size_t, OverridePresetText> override_rows;

    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("asset import presets text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset import presets line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string{key}).second) {
            throw std::invalid_argument("asset import presets text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != asset_import_presets_format_v1) {
                throw std::invalid_argument("asset import presets format is unsupported");
            }
        } else if (key == "override.count") {
            saw_override_count = true;
            override_count = parse_ordinal(value, "asset import presets override count is invalid");
        } else if (key.starts_with("defaults.")) {
            parse_defaults_field(default_texture, default_mesh, default_audio, key, value);
        } else {
            parse_override_field(override_rows, key, value);
        }
    }

    if (!saw_format) {
        throw std::invalid_argument("asset import presets format is missing");
    }
    if (!complete(default_texture) || !complete(default_mesh) || !complete(default_audio)) {
        throw std::invalid_argument("asset import presets defaults are incomplete");
    }
    if (!saw_override_count) {
        throw std::invalid_argument("asset import presets override count is missing");
    }
    if (override_rows.size() != override_count) {
        throw std::invalid_argument("asset import presets override count does not match rows");
    }

    AssetImportPresetsDocumentV1 document;
    document.defaults.texture = default_texture.preset;
    document.defaults.mesh = default_mesh.preset;
    document.defaults.audio = default_audio.preset;
    document.overrides.reserve(override_count);
    for (std::size_t index = 0; index < override_count; ++index) {
        const auto it = override_rows.find(index);
        if (it == override_rows.end()) {
            throw std::invalid_argument("asset import presets override ordinals are not contiguous");
        }
        const auto& text_row = it->second;
        if (!text_row.has_asset_key) {
            throw std::invalid_argument("asset import presets override asset key is missing");
        }

        AssetImportPresetOverrideV1 row;
        row.asset_key = text_row.asset_key;
        if (touched(text_row.texture)) {
            if (!complete(text_row.texture)) {
                throw std::invalid_argument("asset import presets texture override is incomplete");
            }
            row.texture = text_row.texture.preset;
        }
        if (touched(text_row.mesh)) {
            if (!complete(text_row.mesh)) {
                throw std::invalid_argument("asset import presets mesh override is incomplete");
            }
            row.mesh = text_row.mesh.preset;
        }
        if (touched(text_row.audio)) {
            if (!complete(text_row.audio)) {
                throw std::invalid_argument("asset import presets audio override is incomplete");
            }
            row.audio = text_row.audio.preset;
        }
        document.overrides.push_back(std::move(row));
    }

    const auto diagnostics = validate_asset_import_presets_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import presets document is invalid");
    }
    return document;
}

AssetImportPresetReviewV1 review_asset_import_preset_for_asset(const AssetImportPresetsDocumentV1& document,
                                                               const AssetKeyV2& asset_key, AssetKind asset_kind) {
    AssetImportPresetReviewV1 review;
    const auto* override = find_unique_override_for_review(document, asset_key, review.diagnostics);
    if (!review.diagnostics.empty()) {
        review.ready = false;
        return review;
    }

    if (asset_kind == AssetKind::texture) {
        const auto preset =
            override != nullptr && override->texture.has_value() ? *override->texture : document.defaults.texture;
        review.metadata = texture_metadata(preset);
        validate_texture_preset(review.diagnostics, "asset.texture", preset);
    } else if (asset_kind == AssetKind::mesh) {
        const auto preset =
            override != nullptr && override->mesh.has_value() ? *override->mesh : document.defaults.mesh;
        review.metadata = mesh_metadata(preset);
        validate_mesh_preset(review.diagnostics, "asset.mesh", preset);
    } else if (asset_kind == AssetKind::audio) {
        const auto preset =
            override != nullptr && override->audio.has_value() ? *override->audio : document.defaults.audio;
        review.metadata = audio_metadata(preset);
        validate_audio_preset(review.diagnostics, "asset.audio", preset);
    }

    review.ready = review.diagnostics.empty();
    return review;
}

} // namespace mirakana
