// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_source_format.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view texture_source_format = "GameEngine.TextureSource.v1";
constexpr std::string_view texture_source_format_v2 = "GameEngine.TextureSource.v2";
constexpr std::string_view texture_cook_metadata_format_v1 = "GameEngine.CookedTextureMetadata.v1";
constexpr std::string_view environment_texture_geasset_metadata_format_v1 =
    "GameEngine.EnvironmentTextureGeassetMetadata.v1";
constexpr std::string_view environment_asset_source_format_v1 = "GameEngine.EnvironmentAssetSource.v1";
constexpr std::string_view mesh_source_format = "GameEngine.MeshSource.v2";
constexpr std::string_view audio_source_format = "GameEngine.AudioSource.v1";
constexpr std::string_view morph_mesh_cpu_source_format = "GameEngine.MorphMeshCpuSource.v1";
constexpr std::string_view animation_float_clip_source_format = "GameEngine.AnimationFloatClipSource.v1";
constexpr std::string_view animation_quaternion_clip_source_format = "GameEngine.AnimationQuaternionClipSource.v1";
constexpr std::string_view animation_transform_binding_source_format = "GameEngine.AnimationTransformBindingSource.v1";
constexpr std::uint32_t max_morph_mesh_cpu_vertex_count = 1'000'000U;
constexpr std::uint32_t max_morph_mesh_cpu_target_count = 4096U;
constexpr std::uint32_t max_animation_float_clip_track_count = 4096U;
constexpr std::uint32_t max_animation_float_clip_keyframe_count = 1'000'000U;
constexpr std::uint32_t max_animation_quaternion_clip_track_count = 4096U;
constexpr std::uint32_t max_animation_quaternion_clip_keyframe_count = 1'000'000U;
constexpr std::uint32_t max_animation_transform_binding_count = 4096U;
constexpr std::uint32_t max_texture_extent = 16384;
constexpr std::uint32_t max_audio_sample_rate = 384000;
constexpr std::uint32_t max_audio_channel_count = 64;

[[nodiscard]] std::unordered_map<std::string, std::string> parse_key_values(std::string_view text,
                                                                            std::string_view diagnostic_name) {
    std::unordered_map<std::string, std::string> values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                         : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (raw_line.empty()) {
            continue;
        }
        if (raw_line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line contains carriage return");
        }
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string(raw_line.substr(0, separator)), std::string(raw_line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument(std::string(diagnostic_name) + " contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key, std::string_view diagnostic_name) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] const std::string* optional_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) noexcept {
    const auto it = values.find(key);
    return it == values.end() ? nullptr : &it->second;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view diagnostic_name) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view diagnostic_name) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::int32_t parse_i32(std::string_view value, std::string_view diagnostic_name) {
    std::int32_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value, std::string_view diagnostic_name) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string(diagnostic_name) + " bool value is invalid");
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] bool clean_text_token(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool hex_digit(char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

[[nodiscard]] bool sha256_token(std::string_view value) noexcept {
    constexpr std::string_view prefix = "sha256:";
    return value.starts_with(prefix) && value.size() == prefix.size() + 64U && clean_text_token(value) &&
           std::ranges::all_of(value.substr(prefix.size()), hex_digit);
}

[[nodiscard]] bool safe_package_relative_path(std::string_view value, std::string_view suffix) noexcept {
    if (!clean_text_token(value) || !value.ends_with(suffix) || value.starts_with('/') ||
        value.find('\\') != std::string_view::npos || value.find(':') != std::string_view::npos ||
        value.find(';') != std::string_view::npos) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find('/', begin);
        const auto segment = value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
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

[[nodiscard]] bool texture_window_valid(const TextureSourceWindowV2& window) noexcept {
    return window.max_x >= window.min_x && window.max_y >= window.min_y;
}

[[nodiscard]] std::uint32_t checked_window_extent(std::int32_t min_value, std::int32_t max_value) noexcept {
    if (max_value < min_value) {
        return 0;
    }
    const auto extent = static_cast<std::int64_t>(max_value) - static_cast<std::int64_t>(min_value) + 1;
    if (extent <= 0 || extent > std::numeric_limits<std::uint32_t>::max()) {
        return 0;
    }
    return static_cast<std::uint32_t>(extent);
}

[[nodiscard]] std::uint32_t texture_window_width(const TextureSourceWindowV2& window) noexcept {
    return checked_window_extent(window.min_x, window.max_x);
}

[[nodiscard]] std::uint32_t texture_window_height(const TextureSourceWindowV2& window) noexcept {
    return checked_window_extent(window.min_y, window.max_y);
}

[[nodiscard]] std::string texture_window_text(const TextureSourceWindowV2& window) {
    return std::to_string(window.min_x) + "," + std::to_string(window.min_y) + "," + std::to_string(window.max_x) +
           "," + std::to_string(window.max_y);
}

[[nodiscard]] TextureSourceWindowV2 parse_texture_window(std::string_view value, std::string_view diagnostic_name) {
    std::size_t begin = 0;
    std::size_t field_index = 0;
    std::int32_t fields[4] = {};
    while (begin <= value.size()) {
        if (field_index >= 4U) {
            throw std::invalid_argument(std::string(diagnostic_name) + " window has too many fields");
        }
        const auto end = value.find(',', begin);
        const auto field = value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        fields[field_index++] = parse_i32(field, diagnostic_name);
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    if (field_index != 4U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " window has too few fields");
    }
    return TextureSourceWindowV2{.min_x = fields[0], .min_y = fields[1], .max_x = fields[2], .max_y = fields[3]};
}

[[nodiscard]] bool openexr_review_valid(const TextureSourceDocumentV2& document) noexcept {
    const auto& review = document.openexr;
    if (document.color_space != TextureColorSpaceV2::scene_linear || !review.scene_linear_intent) {
        return false;
    }
    if (!texture_window_valid(review.data_window) || !texture_window_valid(review.display_window)) {
        return false;
    }
    if (texture_window_width(review.data_window) != document.width ||
        texture_window_height(review.data_window) != document.height) {
        return false;
    }
    if (!clean_text_token(review.channel_list) || review.channel_count == 0) {
        return false;
    }
    if (review.pixel_encoding != TexturePixelEncodingV2::float16 &&
        review.pixel_encoding != TexturePixelEncodingV2::float32) {
        return false;
    }
    if (review.multipart || review.deep) {
        return false;
    }
    return review.chromaticities_recorded;
}

[[nodiscard]] bool ktx2_basis_review_valid(const TextureSourceDocumentV2& document) noexcept {
    const auto& review = document.ktx2_basis;
    if (!clean_text_token(review.vk_format) || review.level_count == 0 || review.layer_count == 0) {
        return false;
    }
    if (review.face_count != 1U && review.face_count != 6U) {
        return false;
    }
    if (review.supercompression != TextureCompressionKindV2::basis_lz &&
        review.supercompression != TextureCompressionKindV2::uastc) {
        return false;
    }
    if (review.basis_codec != TexturePixelEncodingV2::basis_etc1s &&
        review.basis_codec != TexturePixelEncodingV2::basis_uastc) {
        return false;
    }
    return review.requires_transcoding;
}

[[nodiscard]] std::uint32_t texture_source_v2_mip_count(const TextureSourceDocumentV2& document) noexcept {
    if (document.source_kind == TextureSourceKindV2::ktx2_basis) {
        return document.ktx2_basis.level_count;
    }
    return 1U;
}

[[nodiscard]] bool texture_cook_backend_known(TextureCookBackendV1 backend) noexcept {
    switch (backend) {
    case TextureCookBackendV1::d3d12:
    case TextureCookBackendV1::vulkan:
    case TextureCookBackendV1::metal_macos:
    case TextureCookBackendV1::vulkan_android:
    case TextureCookBackendV1::metal_ios:
        return true;
    case TextureCookBackendV1::unknown:
        break;
    }
    return false;
}

[[nodiscard]] std::uint8_t texture_cook_backend_sort_key(TextureCookBackendV1 backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] bool texture_cook_backend_decision_valid(const TextureCookBackendDecisionV1& decision) noexcept {
    if (!texture_cook_backend_known(decision.backend) || !clean_text_token(decision.device_format) ||
        decision.compression == TextureCompressionKindV2::unknown ||
        decision.transcode == TextureCookTranscodeKindV1::unknown || decision.estimated_gpu_bytes == 0) {
        return false;
    }
    if ((!decision.supported || !decision.host_validated) && !clean_text_token(decision.diagnostic)) {
        return false;
    }
    if (decision.supported && decision.host_validated && !decision.diagnostic.empty()) {
        return false;
    }
    return true;
}

[[nodiscard]] bool has_required_texture_cook_backends(const std::vector<TextureCookBackendDecisionV1>& decisions) {
    bool d3d12 = false;
    bool vulkan = false;
    bool metal_macos = false;
    bool vulkan_android = false;
    bool metal_ios = false;
    for (const auto& decision : decisions) {
        switch (decision.backend) {
        case TextureCookBackendV1::d3d12:
            if (d3d12) {
                return false;
            }
            d3d12 = true;
            break;
        case TextureCookBackendV1::vulkan:
            if (vulkan) {
                return false;
            }
            vulkan = true;
            break;
        case TextureCookBackendV1::metal_macos:
            if (metal_macos) {
                return false;
            }
            metal_macos = true;
            break;
        case TextureCookBackendV1::vulkan_android:
            if (vulkan_android) {
                return false;
            }
            vulkan_android = true;
            break;
        case TextureCookBackendV1::metal_ios:
            if (metal_ios) {
                return false;
            }
            metal_ios = true;
            break;
        case TextureCookBackendV1::unknown:
            return false;
        }
    }
    return d3d12 && vulkan && metal_macos && vulkan_android && metal_ios;
}

[[nodiscard]] std::uint64_t
max_estimated_texture_gpu_bytes(const std::vector<TextureCookBackendDecisionV1>& decisions) noexcept {
    std::uint64_t max_bytes = 0;
    for (const auto& decision : decisions) {
        max_bytes = std::max(max_bytes, decision.estimated_gpu_bytes);
    }
    return max_bytes;
}

[[nodiscard]] std::uint32_t
unsupported_texture_host_diagnostic_count(const std::vector<TextureCookBackendDecisionV1>& decisions) noexcept {
    std::uint32_t count = 0;
    for (const auto& decision : decisions) {
        if (!decision.host_validated || !decision.supported) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::uint8_t hex_value(char value, std::string_view diagnostic_name) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(10 + value - 'a');
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<std::uint8_t>(10 + value - 'A');
    }
    throw std::invalid_argument(std::string(diagnostic_name) + " hex byte payload is invalid");
}

[[nodiscard]] std::vector<std::uint8_t>
parse_optional_hex_bytes(const std::unordered_map<std::string, std::string>& values, const std::string& key,
                         std::string_view diagnostic_name) {
    const auto* encoded = optional_value(values, key);
    if (encoded == nullptr || encoded->empty()) {
        return {};
    }
    if ((encoded->size() % 2U) != 0U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " hex byte payload length is invalid");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(encoded->size() / 2U);
    for (std::size_t index = 0; index < encoded->size(); index += 2U) {
        const auto high = hex_value((*encoded)[index], diagnostic_name);
        const auto low = hex_value((*encoded)[index + 1U], diagnostic_name);
        bytes.push_back(static_cast<std::uint8_t>((high << 4U) | low));
    }
    return bytes;
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

[[nodiscard]] bool finite_f32(float value) noexcept {
    return std::isfinite(static_cast<double>(value));
}

[[nodiscard]] float read_float32(const std::vector<std::uint8_t>& bytes, std::size_t offset) noexcept {
    float value = 0.0F;
    const auto source = std::span<const std::uint8_t>{bytes};
    std::memcpy(&value, source.subspan(offset).data(), sizeof(float));
    return value;
}

[[nodiscard]] bool le_bytes_all_finite_f32(const std::vector<std::uint8_t>& bytes) noexcept {
    if ((bytes.size() % 4U) != 0U) {
        return false;
    }
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4U) {
        float decoded = 0.0F;
        decoded = read_float32(bytes, offset);
        if (!finite_f32(decoded)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool morph_vec3_stream_length_ok(const std::vector<std::uint8_t>& bytes,
                                               std::uint64_t vertex_count) noexcept {
    if (bytes.empty()) {
        return true;
    }
    const auto expected = vertex_count * 12ULL;
    if (expected > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        return false;
    }
    return bytes.size() == static_cast<std::size_t>(expected);
}

[[nodiscard]] bool morph_vec3_stream_finite(const std::vector<std::uint8_t>& bytes) noexcept {
    if (bytes.empty()) {
        return true;
    }
    if ((bytes.size() % 12U) != 0U) {
        return false;
    }
    return le_bytes_all_finite_f32(bytes);
}

[[nodiscard]] bool weight_in_unit_interval(float weight) noexcept {
    return finite_f32(weight) && weight >= 0.0F && weight <= 1.0F;
}

[[nodiscard]] bool morph_target_weight_bytes_valid(const std::vector<std::uint8_t>& bytes,
                                                   std::size_t target_count) noexcept {
    const auto expected = target_count * 4U;
    if (bytes.size() != expected) {
        return false;
    }
    if (!le_bytes_all_finite_f32(bytes)) {
        return false;
    }
    for (std::size_t index = 0; index < target_count; ++index) {
        float weight = 0.0F;
        weight = read_float32(bytes, index * 4U);
        if (!weight_in_unit_interval(weight)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string morph_target_key_prefix(std::size_t index) {
    return std::string{"morph.target."} + std::to_string(index) + ".";
}

[[nodiscard]] std::string clip_track_key_prefix(std::size_t index) {
    return std::string{"clip.track."} + std::to_string(index) + ".";
}

void write_quaternion_clip_component_payload(std::ostream& output, const std::string& prefix,
                                             std::string_view component, std::string_view values_key,
                                             const std::vector<std::uint8_t>& time_seconds_bytes,
                                             const std::vector<std::uint8_t>& value_bytes) {
    output << prefix << component << "_keyframe_count=" << (time_seconds_bytes.size() / 4U) << '\n';
    if (!time_seconds_bytes.empty()) {
        output << prefix << component << "_times_hex=" << encode_hex_bytes(time_seconds_bytes) << '\n';
        output << prefix << values_key << '=' << encode_hex_bytes(value_bytes) << '\n';
    }
}

[[nodiscard]] std::string transform_binding_key_prefix(std::size_t index) {
    return std::string{"binding."} + std::to_string(index) + ".";
}

[[nodiscard]] bool animation_float_clip_target_token_ok(std::string_view target) noexcept {
    if (target.empty()) {
        return false;
    }
    if (target.find('\n') != std::string_view::npos || target.find('\r') != std::string_view::npos ||
        target.find('\0') != std::string_view::npos) {
        return false;
    }
    return true;
}

[[nodiscard]] bool animation_transform_binding_component_ok(AnimationTransformBindingComponent component) noexcept {
    switch (component) {
    case AnimationTransformBindingComponent::translation_x:
    case AnimationTransformBindingComponent::translation_y:
    case AnimationTransformBindingComponent::translation_z:
    case AnimationTransformBindingComponent::rotation_z:
    case AnimationTransformBindingComponent::scale_x:
    case AnimationTransformBindingComponent::scale_y:
    case AnimationTransformBindingComponent::scale_z:
        return true;
    case AnimationTransformBindingComponent::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool transform_binding_row_ok(const AnimationTransformBindingSourceRow& row) noexcept {
    return animation_float_clip_target_token_ok(row.target) && animation_float_clip_target_token_ok(row.node_name) &&
           animation_transform_binding_component_ok(row.component);
}

[[nodiscard]] bool times_strictly_increasing_f32_le(const std::vector<std::uint8_t>& bytes) noexcept {
    if ((bytes.size() % 4U) != 0U || bytes.empty()) {
        return false;
    }
    const auto count = bytes.size() / 4U;
    for (std::size_t index = 0; index < count; ++index) {
        float sample = 0.0F;
        sample = read_float32(bytes, index * 4U);
        if (!finite_f32(sample)) {
            return false;
        }
    }
    if (count < 2U) {
        return true;
    }
    float previous = 0.0F;
    previous = read_float32(bytes, 0);
    for (std::size_t index = 1; index < count; ++index) {
        float sample = 0.0F;
        sample = read_float32(bytes, index * 4U);
        if (!(sample > previous)) {
            return false;
        }
        previous = sample;
    }
    return true;
}

[[nodiscard]] bool times_strictly_increasing_non_negative_f32_le(const std::vector<std::uint8_t>& bytes) noexcept {
    if ((bytes.size() % 4U) != 0U || bytes.empty()) {
        return false;
    }
    float previous = -1.0F;
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4U) {
        float sample = 0.0F;
        sample = read_float32(bytes, offset);
        if (!finite_f32(sample) || sample < 0.0F || sample <= previous) {
            return false;
        }
        previous = sample;
    }
    return true;
}

[[nodiscard]] bool animation_float_clip_track_payload_ok(const AnimationFloatClipTrackSourceDocument& track) noexcept {
    if (!animation_float_clip_target_token_ok(track.target)) {
        return false;
    }
    if (track.time_seconds_bytes.empty() || track.time_seconds_bytes.size() != track.value_bytes.size()) {
        return false;
    }
    if ((track.time_seconds_bytes.size() % 4U) != 0U) {
        return false;
    }
    const auto keyframes = track.time_seconds_bytes.size() / 4U;
    if (keyframes > static_cast<std::size_t>(max_animation_float_clip_keyframe_count)) {
        return false;
    }
    if (!le_bytes_all_finite_f32(track.time_seconds_bytes) || !le_bytes_all_finite_f32(track.value_bytes)) {
        return false;
    }
    return times_strictly_increasing_f32_le(track.time_seconds_bytes);
}

[[nodiscard]] bool quaternion_clip_vec3_rows_positive(const std::vector<std::uint8_t>& bytes) noexcept {
    if ((bytes.size() % 12U) != 0U) {
        return false;
    }
    for (std::size_t offset = 0; offset < bytes.size(); offset += 12U) {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        x = read_float32(bytes, offset);
        y = read_float32(bytes, offset + 4U);
        z = read_float32(bytes, offset + 8U);
        if (!finite_f32(x) || !finite_f32(y) || !finite_f32(z) || x <= 0.0F || y <= 0.0F || z <= 0.0F) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool quaternion_clip_rows_normalized(const std::vector<std::uint8_t>& bytes) noexcept {
    if ((bytes.size() % 16U) != 0U) {
        return false;
    }
    for (std::size_t offset = 0; offset < bytes.size(); offset += 16U) {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float w = 1.0F;
        x = read_float32(bytes, offset);
        y = read_float32(bytes, offset + 4U);
        z = read_float32(bytes, offset + 8U);
        w = read_float32(bytes, offset + 12U);
        const auto length_squared = (x * x) + (y * y) + (z * z) + (w * w);
        if (!finite_f32(x) || !finite_f32(y) || !finite_f32(z) || !finite_f32(w) ||
            std::abs(length_squared - 1.0F) > 0.0001F) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool quaternion_clip_component_payload_ok(const std::vector<std::uint8_t>& time_seconds_bytes,
                                                        const std::vector<std::uint8_t>& value_bytes,
                                                        std::size_t value_stride_bytes) noexcept {
    if (time_seconds_bytes.empty() && value_bytes.empty()) {
        return true;
    }
    if (time_seconds_bytes.empty() || value_bytes.empty() || (time_seconds_bytes.size() % 4U) != 0U) {
        return false;
    }
    const auto keyframes = time_seconds_bytes.size() / 4U;
    if (keyframes > static_cast<std::size_t>(max_animation_quaternion_clip_keyframe_count) ||
        value_bytes.size() != keyframes * value_stride_bytes) {
        return false;
    }
    return times_strictly_increasing_non_negative_f32_le(time_seconds_bytes) && le_bytes_all_finite_f32(value_bytes);
}

[[nodiscard]] bool
animation_quaternion_clip_track_payload_ok(const AnimationQuaternionClipTrackSourceDocument& track) noexcept {
    if (!animation_float_clip_target_token_ok(track.target)) {
        return false;
    }
    const bool has_translation = !track.translation_time_seconds_bytes.empty() || !track.translation_xyz_bytes.empty();
    const bool has_rotation = !track.rotation_time_seconds_bytes.empty() || !track.rotation_xyzw_bytes.empty();
    const bool has_scale = !track.scale_time_seconds_bytes.empty() || !track.scale_xyz_bytes.empty();
    if (!has_translation && !has_rotation && !has_scale) {
        return false;
    }
    if (!quaternion_clip_component_payload_ok(track.translation_time_seconds_bytes, track.translation_xyz_bytes, 12U)) {
        return false;
    }
    if (!quaternion_clip_component_payload_ok(track.rotation_time_seconds_bytes, track.rotation_xyzw_bytes, 16U) ||
        !quaternion_clip_rows_normalized(track.rotation_xyzw_bytes)) {
        return false;
    }
    if (!quaternion_clip_component_payload_ok(track.scale_time_seconds_bytes, track.scale_xyz_bytes, 12U) ||
        !quaternion_clip_vec3_rows_positive(track.scale_xyz_bytes)) {
        return false;
    }
    return true;
}

} // namespace

bool is_valid_texture_source_document(const TextureSourceDocument& document) noexcept {
    if (document.width == 0 || document.height == 0 || document.width > max_texture_extent ||
        document.height > max_texture_extent || texture_source_bytes_per_pixel(document.pixel_format) == 0) {
        return false;
    }
    if (document.bytes.empty()) {
        return true;
    }
    return document.bytes.size() ==
           static_cast<std::size_t>(static_cast<std::uint64_t>(document.width) *
                                    static_cast<std::uint64_t>(document.height) *
                                    static_cast<std::uint64_t>(texture_source_bytes_per_pixel(document.pixel_format)));
}

bool is_valid_texture_source_document_v2(const TextureSourceDocumentV2& document) noexcept {
    if (!clean_text_token(document.source_path) || !sha256_token(document.source_hash) ||
        !clean_text_token(document.provenance_id) || !clean_text_token(document.license_id)) {
        return false;
    }
    if (document.width == 0 || document.height == 0 || document.width > max_texture_extent ||
        document.height > max_texture_extent || document.color_space == TextureColorSpaceV2::unknown ||
        document.sampler_class == TextureSamplerClassV2::unknown) {
        return false;
    }

    switch (document.source_kind) {
    case TextureSourceKindV2::openexr:
        return openexr_review_valid(document);
    case TextureSourceKindV2::ktx2_basis:
        return ktx2_basis_review_valid(document);
    case TextureSourceKindV2::unknown:
        break;
    }
    return false;
}

bool is_valid_texture_cook_metadata_document_v1(const TextureCookMetadataDocumentV1& document) noexcept {
    if (!is_valid_texture_source_document_v2(document.source) || document.estimated_source_bytes == 0 ||
        document.estimated_decoded_bytes == 0 || document.backend_decisions.size() != 5U ||
        !has_required_texture_cook_backends(document.backend_decisions)) {
        return false;
    }
    return std::ranges::all_of(document.backend_decisions, texture_cook_backend_decision_valid);
}

bool is_valid_environment_texture_geasset_metadata_document_v1(
    const EnvironmentTextureGeassetMetadataDocumentV1& document) noexcept {
    if (!safe_package_relative_path(document.geasset_path, ".geasset") ||
        !is_valid_texture_cook_metadata_document_v1(document.cook_metadata) || !document.metadata_only) {
        return false;
    }
    if (document.mip_count != texture_source_v2_mip_count(document.cook_metadata.source)) {
        return false;
    }
    if (document.max_estimated_gpu_bytes != max_estimated_texture_gpu_bytes(document.cook_metadata.backend_decisions)) {
        return false;
    }
    if (document.unsupported_host_diagnostic_count !=
        unsupported_texture_host_diagnostic_count(document.cook_metadata.backend_decisions)) {
        return false;
    }
    return true;
}

bool is_valid_environment_asset_source_document_v1(const EnvironmentAssetSourceDocumentV1& document) noexcept {
    return clean_text_token(document.environment_profile_source_path) &&
           clean_text_token(document.radiance_texture_source_path) && clean_text_token(document.provenance_id) &&
           clean_text_token(document.license_id) && document.requires_scene_linear_radiance &&
           document.texture_source_v2_required;
}

bool is_valid_mesh_source_document(const MeshSourceDocument& document) noexcept {
    if (document.vertex_count == 0 || document.index_count == 0) {
        return false;
    }
    if (document.has_normals != document.has_uvs) {
        return false;
    }
    if (document.has_normals && !document.has_tangent_frame) {
        return false;
    }
    if (document.has_tangent_frame && (!document.has_normals || !document.has_uvs)) {
        return false;
    }
    if (!document.vertex_bytes.empty()) {
        const auto stride = mesh_source_vertex_stride_bytes(document);
        if (!stride.has_value() || *stride == 0) {
            return false;
        }
        const auto expected = static_cast<std::uint64_t>(document.vertex_count) * static_cast<std::uint64_t>(*stride);
        if (expected > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
            return false;
        }
        return document.vertex_bytes.size() == static_cast<std::size_t>(expected);
    }
    return true;
}

std::optional<std::uint32_t> mesh_source_vertex_stride_bytes(const MeshSourceDocument& document) noexcept {
    if (document.has_normals && document.has_uvs && document.has_tangent_frame) {
        return 48;
    }
    if (!document.has_normals && !document.has_uvs && !document.has_tangent_frame) {
        return 12;
    }
    return std::nullopt;
}

bool is_valid_audio_source_document(const AudioSourceDocument& document) noexcept {
    if (document.sample_rate == 0 || document.sample_rate > max_audio_sample_rate || document.channel_count == 0 ||
        document.channel_count > max_audio_channel_count || document.frame_count == 0 ||
        audio_source_bytes_per_sample(document.sample_format) == 0) {
        return false;
    }
    if (document.samples.empty()) {
        return true;
    }

    const auto bytes_per_frame = static_cast<std::uint64_t>(document.channel_count) *
                                 static_cast<std::uint64_t>(audio_source_bytes_per_sample(document.sample_format));
    if (document.frame_count > std::numeric_limits<std::uint64_t>::max() / bytes_per_frame) {
        return false;
    }
    const auto expected = document.frame_count * bytes_per_frame;
    return expected <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) &&
           document.samples.size() == static_cast<std::size_t>(expected);
}

std::string_view texture_source_pixel_format_name(TextureSourcePixelFormat pixel_format) noexcept {
    switch (pixel_format) {
    case TextureSourcePixelFormat::r8_unorm:
        return "r8_unorm";
    case TextureSourcePixelFormat::rg8_unorm:
        return "rg8_unorm";
    case TextureSourcePixelFormat::rgba8_unorm:
        return "rgba8_unorm";
    case TextureSourcePixelFormat::unknown:
        break;
    }
    return "unknown";
}

TextureSourcePixelFormat parse_texture_source_pixel_format(std::string_view value) {
    if (value == "r8_unorm") {
        return TextureSourcePixelFormat::r8_unorm;
    }
    if (value == "rg8_unorm") {
        return TextureSourcePixelFormat::rg8_unorm;
    }
    if (value == "rgba8_unorm") {
        return TextureSourcePixelFormat::rgba8_unorm;
    }
    throw std::invalid_argument("texture source pixel format is unsupported");
}

std::uint32_t texture_source_bytes_per_pixel(TextureSourcePixelFormat pixel_format) {
    switch (pixel_format) {
    case TextureSourcePixelFormat::r8_unorm:
        return 1;
    case TextureSourcePixelFormat::rg8_unorm:
        return 2;
    case TextureSourcePixelFormat::rgba8_unorm:
        return 4;
    case TextureSourcePixelFormat::unknown:
        break;
    }
    return 0;
}

std::uint64_t texture_source_uncompressed_bytes(const TextureSourceDocument& document) {
    if (!is_valid_texture_source_document(document)) {
        throw std::invalid_argument("texture source document is invalid");
    }
    return static_cast<std::uint64_t>(document.width) * static_cast<std::uint64_t>(document.height) *
           static_cast<std::uint64_t>(texture_source_bytes_per_pixel(document.pixel_format));
}

std::string_view texture_source_kind_name_v2(TextureSourceKindV2 kind) noexcept {
    switch (kind) {
    case TextureSourceKindV2::openexr:
        return "openexr";
    case TextureSourceKindV2::ktx2_basis:
        return "ktx2_basis";
    case TextureSourceKindV2::unknown:
        break;
    }
    return "unknown";
}

TextureSourceKindV2 parse_texture_source_kind_v2(std::string_view value) {
    if (value == "openexr") {
        return TextureSourceKindV2::openexr;
    }
    if (value == "ktx2_basis") {
        return TextureSourceKindV2::ktx2_basis;
    }
    throw std::invalid_argument("texture source v2 kind is unsupported");
}

std::string_view texture_color_space_name_v2(TextureColorSpaceV2 color_space) noexcept {
    switch (color_space) {
    case TextureColorSpaceV2::srgb:
        return "srgb";
    case TextureColorSpaceV2::scene_linear:
        return "scene_linear";
    case TextureColorSpaceV2::linear_data:
        return "linear_data";
    case TextureColorSpaceV2::unknown:
        break;
    }
    return "unknown";
}

TextureColorSpaceV2 parse_texture_color_space_v2(std::string_view value) {
    if (value == "srgb") {
        return TextureColorSpaceV2::srgb;
    }
    if (value == "scene_linear") {
        return TextureColorSpaceV2::scene_linear;
    }
    if (value == "linear_data") {
        return TextureColorSpaceV2::linear_data;
    }
    throw std::invalid_argument("texture source v2 color space is unsupported");
}

std::string_view texture_sampler_class_name_v2(TextureSamplerClassV2 sampler_class) noexcept {
    switch (sampler_class) {
    case TextureSamplerClassV2::color:
        return "color";
    case TextureSamplerClassV2::normal:
        return "normal";
    case TextureSamplerClassV2::data:
        return "data";
    case TextureSamplerClassV2::environment_radiance:
        return "environment_radiance";
    case TextureSamplerClassV2::unknown:
        break;
    }
    return "unknown";
}

TextureSamplerClassV2 parse_texture_sampler_class_v2(std::string_view value) {
    if (value == "color") {
        return TextureSamplerClassV2::color;
    }
    if (value == "normal") {
        return TextureSamplerClassV2::normal;
    }
    if (value == "data") {
        return TextureSamplerClassV2::data;
    }
    if (value == "environment_radiance") {
        return TextureSamplerClassV2::environment_radiance;
    }
    throw std::invalid_argument("texture source v2 sampler class is unsupported");
}

std::string_view texture_pixel_encoding_name_v2(TexturePixelEncodingV2 encoding) noexcept {
    switch (encoding) {
    case TexturePixelEncodingV2::uint8_unorm:
        return "uint8_unorm";
    case TexturePixelEncodingV2::float16:
        return "float16";
    case TexturePixelEncodingV2::float32:
        return "float32";
    case TexturePixelEncodingV2::basis_etc1s:
        return "basis_etc1s";
    case TexturePixelEncodingV2::basis_uastc:
        return "basis_uastc";
    case TexturePixelEncodingV2::unknown:
        break;
    }
    return "unknown";
}

TexturePixelEncodingV2 parse_texture_pixel_encoding_v2(std::string_view value) {
    if (value == "uint8_unorm") {
        return TexturePixelEncodingV2::uint8_unorm;
    }
    if (value == "float16") {
        return TexturePixelEncodingV2::float16;
    }
    if (value == "float32") {
        return TexturePixelEncodingV2::float32;
    }
    if (value == "basis_etc1s") {
        return TexturePixelEncodingV2::basis_etc1s;
    }
    if (value == "basis_uastc") {
        return TexturePixelEncodingV2::basis_uastc;
    }
    throw std::invalid_argument("texture source v2 pixel encoding is unsupported");
}

std::string_view texture_compression_kind_name_v2(TextureCompressionKindV2 compression) noexcept {
    switch (compression) {
    case TextureCompressionKindV2::none:
        return "none";
    case TextureCompressionKindV2::basis_lz:
        return "basis_lz";
    case TextureCompressionKindV2::uastc:
        return "uastc";
    case TextureCompressionKindV2::bc6h:
        return "bc6h";
    case TextureCompressionKindV2::bc7:
        return "bc7";
    case TextureCompressionKindV2::astc_4x4:
        return "astc_4x4";
    case TextureCompressionKindV2::unknown:
        break;
    }
    return "unknown";
}

TextureCompressionKindV2 parse_texture_compression_kind_v2(std::string_view value) {
    if (value == "none") {
        return TextureCompressionKindV2::none;
    }
    if (value == "basis_lz") {
        return TextureCompressionKindV2::basis_lz;
    }
    if (value == "uastc") {
        return TextureCompressionKindV2::uastc;
    }
    if (value == "bc6h") {
        return TextureCompressionKindV2::bc6h;
    }
    if (value == "bc7") {
        return TextureCompressionKindV2::bc7;
    }
    if (value == "astc_4x4") {
        return TextureCompressionKindV2::astc_4x4;
    }
    throw std::invalid_argument("texture source v2 compression kind is unsupported");
}

std::string_view texture_cook_backend_name_v1(TextureCookBackendV1 backend) noexcept {
    switch (backend) {
    case TextureCookBackendV1::d3d12:
        return "d3d12";
    case TextureCookBackendV1::vulkan:
        return "vulkan";
    case TextureCookBackendV1::metal_macos:
        return "metal_macos";
    case TextureCookBackendV1::vulkan_android:
        return "vulkan_android";
    case TextureCookBackendV1::metal_ios:
        return "metal_ios";
    case TextureCookBackendV1::unknown:
        break;
    }
    return "unknown";
}

TextureCookBackendV1 parse_texture_cook_backend_v1(std::string_view value) {
    if (value == "d3d12") {
        return TextureCookBackendV1::d3d12;
    }
    if (value == "vulkan") {
        return TextureCookBackendV1::vulkan;
    }
    if (value == "metal_macos") {
        return TextureCookBackendV1::metal_macos;
    }
    if (value == "vulkan_android") {
        return TextureCookBackendV1::vulkan_android;
    }
    if (value == "metal_ios") {
        return TextureCookBackendV1::metal_ios;
    }
    throw std::invalid_argument("texture cook backend is unsupported");
}

std::string_view texture_cook_transcode_kind_name_v1(TextureCookTranscodeKindV1 transcode) noexcept {
    switch (transcode) {
    case TextureCookTranscodeKindV1::not_required:
        return "not_required";
    case TextureCookTranscodeKindV1::offline_policy:
        return "offline_policy";
    case TextureCookTranscodeKindV1::basis_transcode_policy:
        return "basis_transcode_policy";
    case TextureCookTranscodeKindV1::unknown:
        break;
    }
    return "unknown";
}

TextureCookTranscodeKindV1 parse_texture_cook_transcode_kind_v1(std::string_view value) {
    if (value == "not_required") {
        return TextureCookTranscodeKindV1::not_required;
    }
    if (value == "offline_policy") {
        return TextureCookTranscodeKindV1::offline_policy;
    }
    if (value == "basis_transcode_policy") {
        return TextureCookTranscodeKindV1::basis_transcode_policy;
    }
    throw std::invalid_argument("texture cook transcode kind is unsupported");
}

std::string_view audio_source_sample_format_name(AudioSourceSampleFormat sample_format) noexcept {
    switch (sample_format) {
    case AudioSourceSampleFormat::pcm16:
        return "pcm16";
    case AudioSourceSampleFormat::float32:
        return "float32";
    case AudioSourceSampleFormat::unknown:
        break;
    }
    return "unknown";
}

AudioSourceSampleFormat parse_audio_source_sample_format(std::string_view value) {
    if (value == "pcm16") {
        return AudioSourceSampleFormat::pcm16;
    }
    if (value == "float32") {
        return AudioSourceSampleFormat::float32;
    }
    throw std::invalid_argument("audio source sample format is unsupported");
}

std::uint32_t audio_source_bytes_per_sample(AudioSourceSampleFormat sample_format) {
    switch (sample_format) {
    case AudioSourceSampleFormat::pcm16:
        return 2;
    case AudioSourceSampleFormat::float32:
        return 4;
    case AudioSourceSampleFormat::unknown:
        break;
    }
    return 0;
}

std::uint64_t audio_source_uncompressed_bytes(const AudioSourceDocument& document) {
    if (!is_valid_audio_source_document(document)) {
        throw std::invalid_argument("audio source document is invalid");
    }
    const auto bytes_per_sample = audio_source_bytes_per_sample(document.sample_format);
    if (bytes_per_sample == 0) {
        throw std::invalid_argument("audio source document sample format is invalid for byte size calculation");
    }
    const auto bytes_per_frame =
        static_cast<std::uint64_t>(document.channel_count) * static_cast<std::uint64_t>(bytes_per_sample);
    if (document.frame_count > std::numeric_limits<std::uint64_t>::max() / bytes_per_frame) {
        throw std::overflow_error("audio source document byte size overflows");
    }
    return document.frame_count * bytes_per_frame;
}

std::string_view
animation_transform_binding_component_name(const AnimationTransformBindingComponent component) noexcept {
    switch (component) {
    case AnimationTransformBindingComponent::translation_x:
        return "translation_x";
    case AnimationTransformBindingComponent::translation_y:
        return "translation_y";
    case AnimationTransformBindingComponent::translation_z:
        return "translation_z";
    case AnimationTransformBindingComponent::rotation_z:
        return "rotation_z";
    case AnimationTransformBindingComponent::scale_x:
        return "scale_x";
    case AnimationTransformBindingComponent::scale_y:
        return "scale_y";
    case AnimationTransformBindingComponent::scale_z:
        return "scale_z";
    case AnimationTransformBindingComponent::unknown:
        break;
    }
    return "unknown";
}

AnimationTransformBindingComponent parse_animation_transform_binding_component(const std::string_view value) {
    if (value == "translation_x") {
        return AnimationTransformBindingComponent::translation_x;
    }
    if (value == "translation_y") {
        return AnimationTransformBindingComponent::translation_y;
    }
    if (value == "translation_z") {
        return AnimationTransformBindingComponent::translation_z;
    }
    if (value == "rotation_z") {
        return AnimationTransformBindingComponent::rotation_z;
    }
    if (value == "scale_x") {
        return AnimationTransformBindingComponent::scale_x;
    }
    if (value == "scale_y") {
        return AnimationTransformBindingComponent::scale_y;
    }
    if (value == "scale_z") {
        return AnimationTransformBindingComponent::scale_z;
    }
    throw std::invalid_argument("animation transform binding component is unsupported");
}

std::string serialize_texture_source_document(const TextureSourceDocument& document) {
    if (!is_valid_texture_source_document(document)) {
        throw std::invalid_argument("texture source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << texture_source_format << '\n';
    output << "texture.width=" << document.width << '\n';
    output << "texture.height=" << document.height << '\n';
    output << "texture.pixel_format=" << texture_source_pixel_format_name(document.pixel_format) << '\n';
    if (!document.bytes.empty()) {
        output << "texture.data_hex=" << encode_hex_bytes(document.bytes) << '\n';
    }
    return output.str();
}

TextureSourceDocument deserialize_texture_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "texture source");
    if (required_value(values, "format", "texture source") != texture_source_format) {
        throw std::invalid_argument("texture source format is unsupported");
    }

    TextureSourceDocument document{
        .width = parse_u32(required_value(values, "texture.width", "texture source"), "texture source"),
        .height = parse_u32(required_value(values, "texture.height", "texture source"), "texture source"),
        .pixel_format =
            parse_texture_source_pixel_format(required_value(values, "texture.pixel_format", "texture source")),
        .bytes = parse_optional_hex_bytes(values, "texture.data_hex", "texture source"),
    };
    if (!is_valid_texture_source_document(document)) {
        throw std::invalid_argument("texture source document is invalid");
    }
    return document;
}

std::string serialize_texture_source_document_v2(const TextureSourceDocumentV2& document) {
    if (!is_valid_texture_source_document_v2(document)) {
        throw std::invalid_argument("texture source v2 document is invalid");
    }
    std::ostringstream output;
    output << "format=" << texture_source_format_v2 << '\n';
    output << "texture.source_path=" << document.source_path << '\n';
    output << "texture.source_hash=" << document.source_hash << '\n';
    output << "texture.provenance_id=" << document.provenance_id << '\n';
    output << "texture.license_id=" << document.license_id << '\n';
    output << "texture.source_kind=" << texture_source_kind_name_v2(document.source_kind) << '\n';
    output << "texture.width=" << document.width << '\n';
    output << "texture.height=" << document.height << '\n';
    output << "texture.color_space=" << texture_color_space_name_v2(document.color_space) << '\n';
    output << "texture.sampler_class=" << texture_sampler_class_name_v2(document.sampler_class) << '\n';
    if (document.source_kind == TextureSourceKindV2::openexr) {
        output << "openexr.data_window=" << texture_window_text(document.openexr.data_window) << '\n';
        output << "openexr.display_window=" << texture_window_text(document.openexr.display_window) << '\n';
        output << "openexr.channel_list=" << document.openexr.channel_list << '\n';
        output << "openexr.pixel_encoding=" << texture_pixel_encoding_name_v2(document.openexr.pixel_encoding) << '\n';
        output << "openexr.channel_count=" << document.openexr.channel_count << '\n';
        output << "openexr.chromaticities_recorded=" << bool_text(document.openexr.chromaticities_recorded) << '\n';
        output << "openexr.scene_linear_intent=" << bool_text(document.openexr.scene_linear_intent) << '\n';
        output << "openexr.multipart=" << bool_text(document.openexr.multipart) << '\n';
        output << "openexr.deep=" << bool_text(document.openexr.deep) << '\n';
        output << "openexr.tiled=" << bool_text(document.openexr.tiled) << '\n';
    } else if (document.source_kind == TextureSourceKindV2::ktx2_basis) {
        output << "ktx2.vk_format=" << document.ktx2_basis.vk_format << '\n';
        output << "ktx2.level_count=" << document.ktx2_basis.level_count << '\n';
        output << "ktx2.layer_count=" << document.ktx2_basis.layer_count << '\n';
        output << "ktx2.face_count=" << document.ktx2_basis.face_count << '\n';
        output << "ktx2.supercompression=" << texture_compression_kind_name_v2(document.ktx2_basis.supercompression)
               << '\n';
        output << "ktx2.basis_codec=" << texture_pixel_encoding_name_v2(document.ktx2_basis.basis_codec) << '\n';
        output << "ktx2.requires_transcoding=" << bool_text(document.ktx2_basis.requires_transcoding) << '\n';
    }
    return output.str();
}

TextureSourceDocumentV2 deserialize_texture_source_document_v2(std::string_view text) {
    const auto values = parse_key_values(text, "texture source v2");
    const auto& format = required_value(values, "format", "texture source v2");
    if (format == texture_source_format) {
        throw std::invalid_argument("texture source v2 rejects legacy GameEngine.TextureSource.v1");
    }
    if (format != texture_source_format_v2) {
        throw std::invalid_argument("texture source v2 format is unsupported");
    }

    TextureSourceDocumentV2 document{
        .source_path = required_value(values, "texture.source_path", "texture source v2"),
        .source_hash = required_value(values, "texture.source_hash", "texture source v2"),
        .provenance_id = required_value(values, "texture.provenance_id", "texture source v2"),
        .license_id = required_value(values, "texture.license_id", "texture source v2"),
        .source_kind = parse_texture_source_kind_v2(required_value(values, "texture.source_kind", "texture source v2")),
        .width = parse_u32(required_value(values, "texture.width", "texture source v2"), "texture source v2"),
        .height = parse_u32(required_value(values, "texture.height", "texture source v2"), "texture source v2"),
        .color_space = parse_texture_color_space_v2(required_value(values, "texture.color_space", "texture source v2")),
        .sampler_class =
            parse_texture_sampler_class_v2(required_value(values, "texture.sampler_class", "texture source v2")),
        .openexr = {},
        .ktx2_basis = {},
    };

    if (document.source_kind == TextureSourceKindV2::openexr) {
        document.openexr = TextureOpenExrSourceReviewV2{
            .data_window = parse_texture_window(required_value(values, "openexr.data_window", "texture source v2"),
                                                "texture source v2"),
            .display_window = parse_texture_window(
                required_value(values, "openexr.display_window", "texture source v2"), "texture source v2"),
            .channel_list = required_value(values, "openexr.channel_list", "texture source v2"),
            .pixel_encoding =
                parse_texture_pixel_encoding_v2(required_value(values, "openexr.pixel_encoding", "texture source v2")),
            .channel_count =
                parse_u32(required_value(values, "openexr.channel_count", "texture source v2"), "texture source v2"),
            .chromaticities_recorded = parse_bool(
                required_value(values, "openexr.chromaticities_recorded", "texture source v2"), "texture source v2"),
            .scene_linear_intent = parse_bool(
                required_value(values, "openexr.scene_linear_intent", "texture source v2"), "texture source v2"),
            .multipart =
                parse_bool(required_value(values, "openexr.multipart", "texture source v2"), "texture source v2"),
            .deep = parse_bool(required_value(values, "openexr.deep", "texture source v2"), "texture source v2"),
            .tiled = parse_bool(required_value(values, "openexr.tiled", "texture source v2"), "texture source v2"),
        };
    } else if (document.source_kind == TextureSourceKindV2::ktx2_basis) {
        document.ktx2_basis = TextureKtx2BasisSourceReviewV2{
            .vk_format = required_value(values, "ktx2.vk_format", "texture source v2"),
            .level_count =
                parse_u32(required_value(values, "ktx2.level_count", "texture source v2"), "texture source v2"),
            .layer_count =
                parse_u32(required_value(values, "ktx2.layer_count", "texture source v2"), "texture source v2"),
            .face_count =
                parse_u32(required_value(values, "ktx2.face_count", "texture source v2"), "texture source v2"),
            .supercompression =
                parse_texture_compression_kind_v2(required_value(values, "ktx2.supercompression", "texture source v2")),
            .basis_codec =
                parse_texture_pixel_encoding_v2(required_value(values, "ktx2.basis_codec", "texture source v2")),
            .requires_transcoding = parse_bool(required_value(values, "ktx2.requires_transcoding", "texture source v2"),
                                               "texture source v2"),
        };
    }

    if (!is_valid_texture_source_document_v2(document)) {
        throw std::invalid_argument("texture source v2 document is invalid");
    }
    return document;
}

std::string serialize_texture_cook_metadata_document_v1(const TextureCookMetadataDocumentV1& document) {
    if (!is_valid_texture_cook_metadata_document_v1(document)) {
        throw std::invalid_argument("texture cook metadata v1 document is invalid");
    }
    auto decisions = document.backend_decisions;
    std::ranges::sort(decisions, [](const auto& lhs, const auto& rhs) {
        return texture_cook_backend_sort_key(lhs.backend) < texture_cook_backend_sort_key(rhs.backend);
    });

    std::uint32_t unsupported_host_diagnostic_count = 0;
    for (const auto& decision : decisions) {
        if (!decision.host_validated || !decision.supported) {
            ++unsupported_host_diagnostic_count;
        }
    }

    std::ostringstream output;
    output << "format=" << texture_cook_metadata_format_v1 << '\n';
    output << "source.format=" << texture_source_format_v2 << '\n';
    output << "source.path=" << document.source.source_path << '\n';
    output << "source.hash=" << document.source.source_hash << '\n';
    output << "source.provenance_id=" << document.source.provenance_id << '\n';
    output << "source.license_id=" << document.source.license_id << '\n';
    output << "source.kind=" << texture_source_kind_name_v2(document.source.source_kind) << '\n';
    output << "texture.width=" << document.source.width << '\n';
    output << "texture.height=" << document.source.height << '\n';
    output << "texture.color_space=" << texture_color_space_name_v2(document.source.color_space) << '\n';
    output << "texture.sampler_class=" << texture_sampler_class_name_v2(document.source.sampler_class) << '\n';
    output << "texture.mip_count=" << texture_source_v2_mip_count(document.source) << '\n';
    output << "texture.estimated_source_bytes=" << document.estimated_source_bytes << '\n';
    output << "texture.estimated_decoded_bytes=" << document.estimated_decoded_bytes << '\n';
    output << "texture.backend_policy_count=" << decisions.size() << '\n';
    for (std::size_t index = 0; index < decisions.size(); ++index) {
        const auto& decision = decisions[index];
        const auto prefix = std::string{"texture.backend."} + std::to_string(index) + ".";
        output << prefix << "backend=" << texture_cook_backend_name_v1(decision.backend) << '\n';
        output << prefix << "device_format=" << decision.device_format << '\n';
        output << prefix << "compression=" << texture_compression_kind_name_v2(decision.compression) << '\n';
        output << prefix << "transcode=" << texture_cook_transcode_kind_name_v1(decision.transcode) << '\n';
        output << prefix << "estimated_gpu_bytes=" << decision.estimated_gpu_bytes << '\n';
        output << prefix << "supported=" << bool_text(decision.supported) << '\n';
        output << prefix << "host_validated=" << bool_text(decision.host_validated) << '\n';
        if (!decision.diagnostic.empty()) {
            output << prefix << "diagnostic=" << decision.diagnostic << '\n';
        }
    }
    output << "texture.unsupported_host_diagnostic_count=" << unsupported_host_diagnostic_count << '\n';
    return output.str();
}

std::string serialize_environment_texture_geasset_metadata_document_v1(
    const EnvironmentTextureGeassetMetadataDocumentV1& document) {
    if (!is_valid_environment_texture_geasset_metadata_document_v1(document)) {
        throw std::invalid_argument("environment texture geasset metadata v1 document is invalid");
    }
    auto decisions = document.cook_metadata.backend_decisions;
    std::ranges::sort(decisions, [](const auto& lhs, const auto& rhs) {
        return texture_cook_backend_sort_key(lhs.backend) < texture_cook_backend_sort_key(rhs.backend);
    });

    const auto& source = document.cook_metadata.source;
    std::ostringstream output;
    output << "format=" << environment_texture_geasset_metadata_format_v1 << '\n';
    output << "asset.path=" << document.geasset_path << '\n';
    output << "asset.kind=environment_texture\n";
    output << "asset.payload=metadata_only\n";
    output << "source.format=" << texture_source_format_v2 << '\n';
    output << "source.path=" << source.source_path << '\n';
    output << "source.hash=" << source.source_hash << '\n';
    output << "source.provenance_id=" << source.provenance_id << '\n';
    output << "source.license_id=" << source.license_id << '\n';
    output << "source.kind=" << texture_source_kind_name_v2(source.source_kind) << '\n';
    output << "texture.width=" << source.width << '\n';
    output << "texture.height=" << source.height << '\n';
    output << "texture.color_space=" << texture_color_space_name_v2(source.color_space) << '\n';
    output << "texture.sampler_class=" << texture_sampler_class_name_v2(source.sampler_class) << '\n';
    output << "texture.mip_count=" << document.mip_count << '\n';
    output << "texture.estimated_source_bytes=" << document.cook_metadata.estimated_source_bytes << '\n';
    output << "texture.estimated_decoded_bytes=" << document.cook_metadata.estimated_decoded_bytes << '\n';
    output << "texture.max_estimated_gpu_bytes=" << document.max_estimated_gpu_bytes << '\n';
    output << "texture.backend_policy_count=" << decisions.size() << '\n';
    for (std::size_t index = 0; index < decisions.size(); ++index) {
        const auto& decision = decisions[index];
        const auto prefix = std::string{"texture.backend."} + std::to_string(index) + ".";
        output << prefix << "backend=" << texture_cook_backend_name_v1(decision.backend) << '\n';
        output << prefix << "device_format=" << decision.device_format << '\n';
        output << prefix << "compression=" << texture_compression_kind_name_v2(decision.compression) << '\n';
        output << prefix << "transcode=" << texture_cook_transcode_kind_name_v1(decision.transcode) << '\n';
        output << prefix << "estimated_gpu_bytes=" << decision.estimated_gpu_bytes << '\n';
        output << prefix << "supported=" << bool_text(decision.supported) << '\n';
        output << prefix << "host_validated=" << bool_text(decision.host_validated) << '\n';
        if (!decision.diagnostic.empty()) {
            output << prefix << "diagnostic=" << decision.diagnostic << '\n';
        }
    }
    output << "texture.unsupported_host_diagnostic_count=" << document.unsupported_host_diagnostic_count << '\n';
    return output.str();
}

std::string serialize_environment_asset_source_document_v1(const EnvironmentAssetSourceDocumentV1& document) {
    if (!is_valid_environment_asset_source_document_v1(document)) {
        throw std::invalid_argument("environment asset source v1 document is invalid");
    }
    std::ostringstream output;
    output << "format=" << environment_asset_source_format_v1 << '\n';
    output << "environment.profile_source_path=" << document.environment_profile_source_path << '\n';
    output << "environment.radiance_texture_source_path=" << document.radiance_texture_source_path << '\n';
    output << "environment.provenance_id=" << document.provenance_id << '\n';
    output << "environment.license_id=" << document.license_id << '\n';
    output << "environment.requires_scene_linear_radiance=" << bool_text(document.requires_scene_linear_radiance)
           << '\n';
    output << "environment.texture_source_v2_required=" << bool_text(document.texture_source_v2_required) << '\n';
    return output.str();
}

EnvironmentAssetSourceDocumentV1 deserialize_environment_asset_source_document_v1(std::string_view text) {
    const auto values = parse_key_values(text, "environment asset source v1");
    if (required_value(values, "format", "environment asset source v1") != environment_asset_source_format_v1) {
        throw std::invalid_argument("environment asset source v1 format is unsupported");
    }

    EnvironmentAssetSourceDocumentV1 document{
        .environment_profile_source_path =
            required_value(values, "environment.profile_source_path", "environment asset source v1"),
        .radiance_texture_source_path =
            required_value(values, "environment.radiance_texture_source_path", "environment asset source v1"),
        .provenance_id = required_value(values, "environment.provenance_id", "environment asset source v1"),
        .license_id = required_value(values, "environment.license_id", "environment asset source v1"),
        .requires_scene_linear_radiance = parse_bool(
            required_value(values, "environment.requires_scene_linear_radiance", "environment asset source v1"),
            "environment asset source v1"),
        .texture_source_v2_required =
            parse_bool(required_value(values, "environment.texture_source_v2_required", "environment asset source v1"),
                       "environment asset source v1"),
    };
    if (!is_valid_environment_asset_source_document_v1(document)) {
        throw std::invalid_argument("environment asset source v1 document is invalid");
    }
    return document;
}

std::string serialize_mesh_source_document(const MeshSourceDocument& document) {
    if (!is_valid_mesh_source_document(document)) {
        throw std::invalid_argument("mesh source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << mesh_source_format << '\n';
    output << "mesh.vertex_count=" << document.vertex_count << '\n';
    output << "mesh.index_count=" << document.index_count << '\n';
    output << "mesh.has_normals=" << bool_text(document.has_normals) << '\n';
    output << "mesh.has_uvs=" << bool_text(document.has_uvs) << '\n';
    output << "mesh.has_tangent_frame=" << bool_text(document.has_tangent_frame) << '\n';
    if (!document.vertex_bytes.empty()) {
        output << "mesh.vertex_data_hex=" << encode_hex_bytes(document.vertex_bytes) << '\n';
    }
    if (!document.index_bytes.empty()) {
        output << "mesh.index_data_hex=" << encode_hex_bytes(document.index_bytes) << '\n';
    }
    return output.str();
}

MeshSourceDocument deserialize_mesh_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "mesh source");
    if (required_value(values, "format", "mesh source") != mesh_source_format) {
        throw std::invalid_argument("mesh source format is unsupported");
    }

    MeshSourceDocument document{
        .vertex_count = parse_u32(required_value(values, "mesh.vertex_count", "mesh source"), "mesh source"),
        .index_count = parse_u32(required_value(values, "mesh.index_count", "mesh source"), "mesh source"),
        .has_normals = parse_bool(required_value(values, "mesh.has_normals", "mesh source"), "mesh source"),
        .has_uvs = parse_bool(required_value(values, "mesh.has_uvs", "mesh source"), "mesh source"),
        .has_tangent_frame = parse_bool(required_value(values, "mesh.has_tangent_frame", "mesh source"), "mesh source"),
        .vertex_bytes = parse_optional_hex_bytes(values, "mesh.vertex_data_hex", "mesh source"),
        .index_bytes = parse_optional_hex_bytes(values, "mesh.index_data_hex", "mesh source"),
    };
    if (!is_valid_mesh_source_document(document)) {
        throw std::invalid_argument("mesh source document is invalid");
    }
    return document;
}

std::string serialize_audio_source_document(const AudioSourceDocument& document) {
    if (!is_valid_audio_source_document(document)) {
        throw std::invalid_argument("audio source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << audio_source_format << '\n';
    output << "audio.sample_rate=" << document.sample_rate << '\n';
    output << "audio.channel_count=" << document.channel_count << '\n';
    output << "audio.frame_count=" << document.frame_count << '\n';
    output << "audio.sample_format=" << audio_source_sample_format_name(document.sample_format) << '\n';
    if (!document.samples.empty()) {
        output << "audio.data_hex=" << encode_hex_bytes(document.samples) << '\n';
    }
    return output.str();
}

AudioSourceDocument deserialize_audio_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "audio source");
    if (required_value(values, "format", "audio source") != audio_source_format) {
        throw std::invalid_argument("audio source format is unsupported");
    }

    AudioSourceDocument document{
        .sample_rate = parse_u32(required_value(values, "audio.sample_rate", "audio source"), "audio source"),
        .channel_count = parse_u32(required_value(values, "audio.channel_count", "audio source"), "audio source"),
        .frame_count = parse_u64(required_value(values, "audio.frame_count", "audio source"), "audio source"),
        .sample_format =
            parse_audio_source_sample_format(required_value(values, "audio.sample_format", "audio source")),
        .samples = parse_optional_hex_bytes(values, "audio.data_hex", "audio source"),
    };
    if (!is_valid_audio_source_document(document)) {
        throw std::invalid_argument("audio source document is invalid");
    }
    return document;
}

bool is_valid_morph_mesh_cpu_source_document(const MorphMeshCpuSourceDocument& document) noexcept {
    if (document.vertex_count == 0U || document.vertex_count > max_morph_mesh_cpu_vertex_count) {
        return false;
    }
    const auto vertex_count64 = static_cast<std::uint64_t>(document.vertex_count);
    if (!morph_vec3_stream_length_ok(document.bind_position_bytes, vertex_count64) ||
        document.bind_position_bytes.empty()) {
        return false;
    }
    if (!morph_vec3_stream_finite(document.bind_position_bytes)) {
        return false;
    }
    if (!morph_vec3_stream_length_ok(document.bind_normal_bytes, vertex_count64) ||
        !morph_vec3_stream_finite(document.bind_normal_bytes)) {
        return false;
    }
    if (!document.bind_normal_bytes.empty() &&
        document.bind_normal_bytes.size() != document.bind_position_bytes.size()) {
        return false;
    }
    if (!morph_vec3_stream_length_ok(document.bind_tangent_bytes, vertex_count64) ||
        !morph_vec3_stream_finite(document.bind_tangent_bytes)) {
        return false;
    }
    if (!document.bind_tangent_bytes.empty() &&
        document.bind_tangent_bytes.size() != document.bind_position_bytes.size()) {
        return false;
    }
    if (document.targets.empty() ||
        document.targets.size() > static_cast<std::size_t>(max_morph_mesh_cpu_target_count)) {
        return false;
    }
    if (!morph_target_weight_bytes_valid(document.target_weight_bytes, document.targets.size())) {
        return false;
    }

    bool any_normal_morph = false;
    bool any_tangent_morph = false;
    for (const auto& target : document.targets) {
        const auto has_pos = !target.position_delta_bytes.empty();
        const auto has_nrm = !target.normal_delta_bytes.empty();
        const auto has_tan = !target.tangent_delta_bytes.empty();
        if (!has_pos && !has_nrm && !has_tan) {
            return false;
        }
        if (!morph_vec3_stream_length_ok(target.position_delta_bytes, vertex_count64) ||
            !morph_vec3_stream_length_ok(target.normal_delta_bytes, vertex_count64) ||
            !morph_vec3_stream_length_ok(target.tangent_delta_bytes, vertex_count64)) {
            return false;
        }
        if (!morph_vec3_stream_finite(target.position_delta_bytes) ||
            !morph_vec3_stream_finite(target.normal_delta_bytes) ||
            !morph_vec3_stream_finite(target.tangent_delta_bytes)) {
            return false;
        }
        if (has_nrm) {
            any_normal_morph = true;
        }
        if (has_tan) {
            any_tangent_morph = true;
        }
    }

    if (any_normal_morph) {
        if (document.bind_normal_bytes.size() != document.bind_position_bytes.size()) {
            return false;
        }
    } else if (!document.bind_normal_bytes.empty()) {
        return false;
    }

    if (any_tangent_morph) {
        if (document.bind_tangent_bytes.size() != document.bind_position_bytes.size()) {
            return false;
        }
    } else if (!document.bind_tangent_bytes.empty()) {
        return false;
    }

    return true;
}

std::string serialize_morph_mesh_cpu_source_document(const MorphMeshCpuSourceDocument& document) {
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("morph mesh CPU source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << morph_mesh_cpu_source_format << '\n';
    write_morph_mesh_cpu_document_payload(output, document);
    return output.str();
}

void write_morph_mesh_cpu_document_payload(std::ostream& output, const MorphMeshCpuSourceDocument& document) {
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("morph mesh CPU source document is invalid");
    }
    output << "morph.vertex_count=" << document.vertex_count << '\n';
    output << "morph.target_count=" << document.targets.size() << '\n';
    output << "morph.bind_positions_hex=" << encode_hex_bytes(document.bind_position_bytes) << '\n';
    if (!document.bind_normal_bytes.empty()) {
        output << "morph.bind_normals_hex=" << encode_hex_bytes(document.bind_normal_bytes) << '\n';
    }
    if (!document.bind_tangent_bytes.empty()) {
        output << "morph.bind_tangents_hex=" << encode_hex_bytes(document.bind_tangent_bytes) << '\n';
    }
    output << "morph.target_weights_hex=" << encode_hex_bytes(document.target_weight_bytes) << '\n';
    for (std::size_t index = 0; index < document.targets.size(); ++index) {
        const auto& target = document.targets[index];
        const auto prefix = morph_target_key_prefix(index);
        if (!target.position_delta_bytes.empty()) {
            output << prefix << "position_deltas_hex=" << encode_hex_bytes(target.position_delta_bytes) << '\n';
        }
        if (!target.normal_delta_bytes.empty()) {
            output << prefix << "normal_deltas_hex=" << encode_hex_bytes(target.normal_delta_bytes) << '\n';
        }
        if (!target.tangent_delta_bytes.empty()) {
            output << prefix << "tangent_deltas_hex=" << encode_hex_bytes(target.tangent_delta_bytes) << '\n';
        }
    }
}

MorphMeshCpuSourceDocument deserialize_morph_mesh_cpu_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "morph mesh CPU source");
    if (required_value(values, "format", "morph mesh CPU source") != morph_mesh_cpu_source_format) {
        throw std::invalid_argument("morph mesh CPU source format is unsupported");
    }

    const auto vertex_count =
        parse_u32(required_value(values, "morph.vertex_count", "morph mesh CPU source"), "morph mesh CPU source");
    if (vertex_count == 0U || vertex_count > max_morph_mesh_cpu_vertex_count) {
        throw std::invalid_argument("morph mesh CPU source vertex_count is invalid");
    }
    const auto target_count =
        parse_u32(required_value(values, "morph.target_count", "morph mesh CPU source"), "morph mesh CPU source");
    if (target_count == 0U || target_count > max_morph_mesh_cpu_target_count) {
        throw std::invalid_argument("morph mesh CPU source target_count is invalid");
    }

    MorphMeshCpuSourceDocument document{
        .vertex_count = vertex_count,
        .bind_position_bytes = parse_optional_hex_bytes(values, "morph.bind_positions_hex", "morph mesh CPU source"),
        .bind_normal_bytes = parse_optional_hex_bytes(values, "morph.bind_normals_hex", "morph mesh CPU source"),
        .bind_tangent_bytes = parse_optional_hex_bytes(values, "morph.bind_tangents_hex", "morph mesh CPU source"),
        .targets = {},
        .target_weight_bytes = parse_optional_hex_bytes(values, "morph.target_weights_hex", "morph mesh CPU source"),
    };
    document.targets.resize(static_cast<std::size_t>(target_count));
    for (std::size_t index = 0; index < document.targets.size(); ++index) {
        const auto prefix = morph_target_key_prefix(index);
        document.targets[index].position_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "position_deltas_hex", "morph mesh CPU source");
        document.targets[index].normal_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "normal_deltas_hex", "morph mesh CPU source");
        document.targets[index].tangent_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "tangent_deltas_hex", "morph mesh CPU source");
    }

    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("morph mesh CPU source document is invalid");
    }
    return document;
}

bool is_valid_animation_float_clip_source_document(const AnimationFloatClipSourceDocument& document) noexcept {
    if (document.tracks.empty() ||
        document.tracks.size() > static_cast<std::size_t>(max_animation_float_clip_track_count)) {
        return false;
    }
    return std::ranges::all_of(document.tracks,
                               [](const auto& track) { return animation_float_clip_track_payload_ok(track); });
}

bool is_valid_animation_quaternion_clip_source_document(
    const AnimationQuaternionClipSourceDocument& document) noexcept {
    if (document.tracks.empty() ||
        document.tracks.size() > static_cast<std::size_t>(max_animation_quaternion_clip_track_count)) {
        return false;
    }
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto& track = document.tracks[index];
        if (!animation_quaternion_clip_track_payload_ok(track)) {
            return false;
        }
        for (std::size_t other = index + 1U; other < document.tracks.size(); ++other) {
            if (document.tracks[other].target == track.target ||
                document.tracks[other].joint_index == track.joint_index) {
                return false;
            }
        }
    }
    return true;
}

bool is_valid_animation_transform_binding_source_document(
    const AnimationTransformBindingSourceDocument& document) noexcept {
    if (document.bindings.empty() ||
        document.bindings.size() > static_cast<std::size_t>(max_animation_transform_binding_count)) {
        return false;
    }
    for (std::size_t index = 0; index < document.bindings.size(); ++index) {
        const auto& binding = document.bindings[index];
        if (!transform_binding_row_ok(binding)) {
            return false;
        }
        for (std::size_t other = index + 1U; other < document.bindings.size(); ++other) {
            const auto& other_binding = document.bindings[other];
            if (other_binding.target == binding.target) {
                return false;
            }
            if (other_binding.node_name == binding.node_name && other_binding.component == binding.component) {
                return false;
            }
        }
    }
    return true;
}

void write_animation_float_clip_document_payload(std::ostream& output,
                                                 const AnimationFloatClipSourceDocument& document) {
    if (!is_valid_animation_float_clip_source_document(document)) {
        throw std::invalid_argument("animation float clip source document is invalid");
    }
    output << "clip.track_count=" << document.tracks.size() << '\n';
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto& track = document.tracks[index];
        const auto prefix = clip_track_key_prefix(index);
        output << prefix << "target=" << track.target << '\n';
        const auto keyframe_count = track.time_seconds_bytes.size() / 4U;
        output << prefix << "keyframe_count=" << keyframe_count << '\n';
        output << prefix << "times_hex=" << encode_hex_bytes(track.time_seconds_bytes) << '\n';
        output << prefix << "values_hex=" << encode_hex_bytes(track.value_bytes) << '\n';
    }
}

std::string serialize_animation_float_clip_source_document(const AnimationFloatClipSourceDocument& document) {
    if (!is_valid_animation_float_clip_source_document(document)) {
        throw std::invalid_argument("animation float clip source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << animation_float_clip_source_format << '\n';
    write_animation_float_clip_document_payload(output, document);
    return output.str();
}

AnimationFloatClipSourceDocument deserialize_animation_float_clip_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "animation float clip source");
    if (required_value(values, "format", "animation float clip source") != animation_float_clip_source_format) {
        throw std::invalid_argument("animation float clip source format is unsupported");
    }

    const auto track_count = parse_u32(required_value(values, "clip.track_count", "animation float clip source"),
                                       "animation float clip source");
    if (track_count == 0U || track_count > max_animation_float_clip_track_count) {
        throw std::invalid_argument("animation float clip source track_count is invalid");
    }

    AnimationFloatClipSourceDocument document;
    document.tracks.resize(static_cast<std::size_t>(track_count));
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto prefix = clip_track_key_prefix(index);
        AnimationFloatClipTrackSourceDocument track;
        track.target = required_value(values, prefix + "target", "animation float clip source");
        const auto keyframe_count =
            parse_u32(required_value(values, prefix + "keyframe_count", "animation float clip source"),
                      "animation float clip source");
        if (keyframe_count == 0U || keyframe_count > max_animation_float_clip_keyframe_count) {
            throw std::invalid_argument("animation float clip source keyframe_count is invalid");
        }
        track.time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "times_hex", "animation float clip source");
        track.value_bytes = parse_optional_hex_bytes(values, prefix + "values_hex", "animation float clip source");
        if (track.time_seconds_bytes.size() != static_cast<std::size_t>(keyframe_count) * 4U ||
            track.value_bytes.size() != static_cast<std::size_t>(keyframe_count) * 4U) {
            throw std::invalid_argument("animation float clip source track byte length is invalid");
        }
        document.tracks[index] = std::move(track);
    }

    if (!is_valid_animation_float_clip_source_document(document)) {
        throw std::invalid_argument("animation float clip source document is invalid");
    }
    return document;
}

void write_animation_quaternion_clip_document_payload(std::ostream& output,
                                                      const AnimationQuaternionClipSourceDocument& document) {
    if (!is_valid_animation_quaternion_clip_source_document(document)) {
        throw std::invalid_argument("animation quaternion clip source document is invalid");
    }
    output << "clip.track_count=" << document.tracks.size() << '\n';
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto& track = document.tracks[index];
        const auto prefix = clip_track_key_prefix(index);
        output << prefix << "target=" << track.target << '\n';
        output << prefix << "joint_index=" << track.joint_index << '\n';
        write_quaternion_clip_component_payload(output, prefix, "translation", "translations_xyz_hex",
                                                track.translation_time_seconds_bytes, track.translation_xyz_bytes);
        write_quaternion_clip_component_payload(output, prefix, "rotation", "rotations_xyzw_hex",
                                                track.rotation_time_seconds_bytes, track.rotation_xyzw_bytes);
        write_quaternion_clip_component_payload(output, prefix, "scale", "scales_xyz_hex",
                                                track.scale_time_seconds_bytes, track.scale_xyz_bytes);
    }
}

std::string serialize_animation_quaternion_clip_source_document(const AnimationQuaternionClipSourceDocument& document) {
    if (!is_valid_animation_quaternion_clip_source_document(document)) {
        throw std::invalid_argument("animation quaternion clip source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << animation_quaternion_clip_source_format << '\n';
    write_animation_quaternion_clip_document_payload(output, document);
    return output.str();
}

AnimationQuaternionClipSourceDocument deserialize_animation_quaternion_clip_source_document(std::string_view text) {
    const auto values = parse_key_values(text, "animation quaternion clip source");
    if (required_value(values, "format", "animation quaternion clip source") !=
        animation_quaternion_clip_source_format) {
        throw std::invalid_argument("animation quaternion clip source format is unsupported");
    }

    const auto track_count = parse_u32(required_value(values, "clip.track_count", "animation quaternion clip source"),
                                       "animation quaternion clip source");
    if (track_count == 0U || track_count > max_animation_quaternion_clip_track_count) {
        throw std::invalid_argument("animation quaternion clip source track_count is invalid");
    }

    AnimationQuaternionClipSourceDocument document;
    document.tracks.resize(static_cast<std::size_t>(track_count));
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto prefix = clip_track_key_prefix(index);
        auto& track = document.tracks[index];
        track.target = required_value(values, prefix + "target", "animation quaternion clip source");
        track.joint_index =
            parse_u32(required_value(values, prefix + "joint_index", "animation quaternion clip source"),
                      "animation quaternion clip source");

        const auto translation_count =
            parse_u32(required_value(values, prefix + "translation_keyframe_count", "animation quaternion clip source"),
                      "animation quaternion clip source");
        const auto rotation_count =
            parse_u32(required_value(values, prefix + "rotation_keyframe_count", "animation quaternion clip source"),
                      "animation quaternion clip source");
        const auto scale_count =
            parse_u32(required_value(values, prefix + "scale_keyframe_count", "animation quaternion clip source"),
                      "animation quaternion clip source");
        if (translation_count > max_animation_quaternion_clip_keyframe_count ||
            rotation_count > max_animation_quaternion_clip_keyframe_count ||
            scale_count > max_animation_quaternion_clip_keyframe_count) {
            throw std::invalid_argument("animation quaternion clip source keyframe_count is invalid");
        }

        track.translation_time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "translation_times_hex", "animation quaternion clip source");
        track.translation_xyz_bytes =
            parse_optional_hex_bytes(values, prefix + "translations_xyz_hex", "animation quaternion clip source");
        track.rotation_time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "rotation_times_hex", "animation quaternion clip source");
        track.rotation_xyzw_bytes =
            parse_optional_hex_bytes(values, prefix + "rotations_xyzw_hex", "animation quaternion clip source");
        track.scale_time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "scale_times_hex", "animation quaternion clip source");
        track.scale_xyz_bytes =
            parse_optional_hex_bytes(values, prefix + "scales_xyz_hex", "animation quaternion clip source");

        if (track.translation_time_seconds_bytes.size() != static_cast<std::size_t>(translation_count) * 4U ||
            track.translation_xyz_bytes.size() != static_cast<std::size_t>(translation_count) * 12U ||
            track.rotation_time_seconds_bytes.size() != static_cast<std::size_t>(rotation_count) * 4U ||
            track.rotation_xyzw_bytes.size() != static_cast<std::size_t>(rotation_count) * 16U ||
            track.scale_time_seconds_bytes.size() != static_cast<std::size_t>(scale_count) * 4U ||
            track.scale_xyz_bytes.size() != static_cast<std::size_t>(scale_count) * 12U) {
            throw std::invalid_argument("animation quaternion clip source track byte length is invalid");
        }
    }

    if (!is_valid_animation_quaternion_clip_source_document(document)) {
        throw std::invalid_argument("animation quaternion clip source document is invalid");
    }
    return document;
}

std::string
serialize_animation_transform_binding_source_document(const AnimationTransformBindingSourceDocument& document) {
    if (!is_valid_animation_transform_binding_source_document(document)) {
        throw std::invalid_argument("animation transform binding source document is invalid");
    }
    std::ostringstream output;
    output << "format=" << animation_transform_binding_source_format << '\n';
    output << "binding.count=" << document.bindings.size() << '\n';
    for (std::size_t index = 0; index < document.bindings.size(); ++index) {
        const auto& binding = document.bindings[index];
        const auto prefix = transform_binding_key_prefix(index);
        output << prefix << "target=" << binding.target << '\n';
        output << prefix << "node_name=" << binding.node_name << '\n';
        output << prefix << "component=" << animation_transform_binding_component_name(binding.component) << '\n';
    }
    return output.str();
}

AnimationTransformBindingSourceDocument
deserialize_animation_transform_binding_source_document(const std::string_view text) {
    const auto values = parse_key_values(text, "animation transform binding source");
    if (required_value(values, "format", "animation transform binding source") !=
        animation_transform_binding_source_format) {
        throw std::invalid_argument("animation transform binding source format is unsupported");
    }

    const auto binding_count = parse_u32(required_value(values, "binding.count", "animation transform binding source"),
                                         "animation transform binding source");
    if (binding_count == 0U || binding_count > max_animation_transform_binding_count) {
        throw std::invalid_argument("animation transform binding source binding count is invalid");
    }

    AnimationTransformBindingSourceDocument document;
    document.bindings.resize(static_cast<std::size_t>(binding_count));
    for (std::size_t index = 0; index < document.bindings.size(); ++index) {
        const auto prefix = transform_binding_key_prefix(index);
        AnimationTransformBindingSourceRow binding;
        binding.target = required_value(values, prefix + "target", "animation transform binding source");
        binding.node_name = required_value(values, prefix + "node_name", "animation transform binding source");
        binding.component = parse_animation_transform_binding_component(
            required_value(values, prefix + "component", "animation transform binding source"));
        document.bindings[index] = std::move(binding);
    }

    if (!is_valid_animation_transform_binding_source_document(document)) {
        throw std::invalid_argument("animation transform binding source document is invalid");
    }
    return document;
}

} // namespace mirakana
