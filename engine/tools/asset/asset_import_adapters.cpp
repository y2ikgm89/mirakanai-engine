// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_import_adapters.hpp"

#include "mirakana/tools/source_image_decode.hpp"

#include "mirakana/assets/asset_source_format.hpp"

#include "mirakana/tools/gltf_morph_animation_import.hpp"
#include "mirakana/tools/morph_mesh_cpu_source_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

#ifndef MK_HAS_ASSET_IMPORTERS
#define MK_HAS_ASSET_IMPORTERS 0
#endif

#if MK_HAS_ASSET_IMPORTERS
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#endif

namespace mirakana {
namespace {

constexpr std::size_t max_gltf_mesh_payload_bytes = 512ULL * 1024ULL * 1024ULL;
constexpr std::size_t max_audio_decoded_bytes = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] std::string lower_extension(std::string_view path) {
    std::filesystem::path parsed{std::string(path)};
    auto extension = parsed.extension().generic_string();
    std::ranges::transform(extension, extension.begin(),
                           [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension;
}

[[nodiscard]] bool has_extension(std::string_view path, std::initializer_list<std::string_view> extensions) {
    const auto extension = lower_extension(path);
    return std::ranges::any_of(extensions, [&extension](std::string_view item) { return extension == item; });
}

[[noreturn]] void throw_feature_disabled() {
    throw std::runtime_error("asset-importers feature is disabled for this build");
}

#if MK_HAS_ASSET_IMPORTERS

[[nodiscard]] std::uint32_t checked_u32(std::size_t value, std::string_view name) {
    if (value > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
        throw std::overflow_error(std::string(name) + " exceeds uint32 range");
    }
    return static_cast<std::uint32_t>(value);
}

void checked_add(std::size_t& target, std::size_t value, std::string_view name) {
    if (value > (std::numeric_limits<std::size_t>::max)() - target) {
        throw std::overflow_error(std::string(name) + " overflows");
    }
    target += value;
}

[[nodiscard]] std::size_t checked_payload_bytes(std::size_t count, std::size_t stride, std::string_view name) {
    if (stride == 0 || count > (std::numeric_limits<std::size_t>::max)() / stride) {
        throw std::overflow_error(std::string(name) + " byte count overflows");
    }
    const auto bytes = count * stride;
    if (bytes > max_gltf_mesh_payload_bytes) {
        throw std::runtime_error(std::string(name) + " size exceeds importer limits");
    }
    return bytes;
}

void reserve_payload_bytes(std::vector<std::uint8_t>& target, std::size_t additional, std::string_view name) {
    if (additional > (std::numeric_limits<std::size_t>::max)() - target.size()) {
        throw std::overflow_error(std::string(name) + " byte count overflows");
    }
    const auto total = target.size() + additional;
    if (total > max_gltf_mesh_payload_bytes) {
        throw std::runtime_error(std::string(name) + " size exceeds importer limits");
    }
    target.reserve(total);
}

void append_le_u32(std::vector<std::uint8_t>& output, std::uint32_t value) {
    output.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void append_le_f32(std::vector<std::uint8_t>& output, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(output, bits);
}

[[nodiscard]] std::size_t checked_audio_sample_bytes(ma_uint64 frames, ma_uint32 channels,
                                                     std::uint32_t bytes_per_sample) {
    if (channels == 0 || bytes_per_sample == 0) {
        throw std::runtime_error("decoded audio format is unsupported");
    }
    if (frames > (std::numeric_limits<ma_uint64>::max)() / channels) {
        throw std::overflow_error("decoded audio frame sample count overflows");
    }
    const auto sample_count = frames * channels;
    if (sample_count > (std::numeric_limits<ma_uint64>::max)() / bytes_per_sample) {
        throw std::overflow_error("decoded audio byte count overflows");
    }
    const auto byte_count = sample_count * bytes_per_sample;
    if (byte_count > static_cast<ma_uint64>((std::numeric_limits<std::size_t>::max)())) {
        throw std::overflow_error("decoded audio byte count overflows");
    }
    if (byte_count > static_cast<ma_uint64>(max_audio_decoded_bytes)) {
        throw std::runtime_error("decoded audio size exceeds importer limits");
    }
    return static_cast<std::size_t>(byte_count);
}

[[nodiscard]] std::string fastgltf_error_text(fastgltf::Error error) {
    return std::string(fastgltf::getErrorMessage(error));
}

void require_loaded_buffer_view(const fastgltf::Asset& asset, std::size_t buffer_view_index, std::string_view name) {
    if (buffer_view_index >= asset.bufferViews.size()) {
        throw std::runtime_error(std::string(name) + " references an unknown buffer view");
    }
    const auto& buffer_view = asset.bufferViews[buffer_view_index];
    if (buffer_view.bufferIndex >= asset.buffers.size()) {
        throw std::runtime_error(std::string(name) + " references an unknown buffer");
    }
    const auto& source = asset.buffers[buffer_view.bufferIndex].data;
    if (!std::holds_alternative<fastgltf::sources::Array>(source) &&
        !std::holds_alternative<fastgltf::sources::Vector>(source) &&
        !std::holds_alternative<fastgltf::sources::ByteView>(source)) {
        throw std::runtime_error(std::string(name) + " buffer data is not loaded");
    }
}

[[nodiscard]] const fastgltf::Accessor& require_accessor(const fastgltf::Asset& asset, std::size_t accessor_index,
                                                         std::string_view name) {
    if (accessor_index >= asset.accessors.size()) {
        throw std::runtime_error(std::string(name) + " references an unknown accessor");
    }
    return asset.accessors[accessor_index];
}

void require_accessor_buffer_view(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor,
                                  std::string_view name) {
    if (!accessor.bufferViewIndex.has_value()) {
        throw std::runtime_error(std::string(name) + " accessor does not contain loadable buffer data");
    }
    require_loaded_buffer_view(asset, accessor.bufferViewIndex.value(), name);
}

void append_gltf_positions(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor,
                           std::vector<std::uint8_t>& vertex_bytes) {
    if (accessor.type != fastgltf::AccessorType::Vec3 || accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error("glTF POSITION accessor must be float32 VEC3");
    }
    if (accessor.count == 0) {
        throw std::runtime_error("glTF POSITION accessor must not be empty");
    }
    require_accessor_buffer_view(asset, accessor, "glTF POSITION");
    reserve_payload_bytes(vertex_bytes, checked_payload_bytes(accessor.count, 12U, "glTF vertex payload"),
                          "glTF vertex payload");

    for (const auto position : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor)) {
        append_le_f32(vertex_bytes, position[0]);
        append_le_f32(vertex_bytes, position[1]);
        append_le_f32(vertex_bytes, position[2]);
    }
}

void append_gltf_tangent_space_vertices(const fastgltf::Asset& asset, const fastgltf::Accessor& position_accessor,
                                        const fastgltf::Accessor& normal_accessor,
                                        const fastgltf::Accessor& uv_accessor,
                                        const fastgltf::Accessor& tangent_accessor,
                                        std::vector<std::uint8_t>& vertex_bytes) {
    if (position_accessor.type != fastgltf::AccessorType::Vec3 ||
        position_accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error("glTF POSITION accessor must be float32 VEC3");
    }
    if (normal_accessor.type != fastgltf::AccessorType::Vec3 ||
        normal_accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error("glTF NORMAL accessor must be float32 VEC3");
    }
    if (uv_accessor.type != fastgltf::AccessorType::Vec2 ||
        uv_accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error("glTF TEXCOORD_0 accessor must be float32 VEC2");
    }
    if (tangent_accessor.type != fastgltf::AccessorType::Vec4 ||
        tangent_accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error("glTF TANGENT accessor must be float32 VEC4");
    }
    if (position_accessor.count == 0) {
        throw std::runtime_error("glTF POSITION accessor must not be empty");
    }
    if (normal_accessor.count != position_accessor.count || uv_accessor.count != position_accessor.count ||
        tangent_accessor.count != position_accessor.count) {
        throw std::runtime_error("glTF NORMAL, TEXCOORD_0, and TANGENT accessors must match POSITION vertex count");
    }

    require_accessor_buffer_view(asset, position_accessor, "glTF POSITION");
    require_accessor_buffer_view(asset, normal_accessor, "glTF NORMAL");
    require_accessor_buffer_view(asset, uv_accessor, "glTF TEXCOORD_0");
    require_accessor_buffer_view(asset, tangent_accessor, "glTF TANGENT");
    reserve_payload_bytes(vertex_bytes, checked_payload_bytes(position_accessor.count, 48U, "glTF vertex payload"),
                          "glTF vertex payload");

    std::vector<fastgltf::math::fvec3> normals;
    normals.reserve(normal_accessor.count);
    for (const auto normal : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, normal_accessor)) {
        normals.push_back(normal);
    }

    std::vector<fastgltf::math::fvec2> uvs;
    uvs.reserve(uv_accessor.count);
    for (const auto uv : fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, uv_accessor)) {
        uvs.push_back(uv);
    }

    std::vector<fastgltf::math::fvec4> tangents;
    tangents.reserve(tangent_accessor.count);
    for (const auto tangent : fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, tangent_accessor)) {
        tangents.push_back(tangent);
    }

    std::size_t index = 0;
    for (const auto position : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, position_accessor)) {
        const auto& normal = normals[index];
        const auto& uv = uvs[index];
        const auto& tangent = tangents[index];
        append_le_f32(vertex_bytes, position[0]);
        append_le_f32(vertex_bytes, position[1]);
        append_le_f32(vertex_bytes, position[2]);
        append_le_f32(vertex_bytes, normal[0]);
        append_le_f32(vertex_bytes, normal[1]);
        append_le_f32(vertex_bytes, normal[2]);
        append_le_f32(vertex_bytes, uv[0]);
        append_le_f32(vertex_bytes, uv[1]);
        append_le_f32(vertex_bytes, tangent[0]);
        append_le_f32(vertex_bytes, tangent[1]);
        append_le_f32(vertex_bytes, tangent[2]);
        append_le_f32(vertex_bytes, tangent[3]);
        ++index;
    }
}

void append_gltf_indices(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor, std::uint32_t vertex_base,
                         std::size_t primitive_vertex_count, std::vector<std::uint8_t>& index_bytes) {
    if (accessor.type != fastgltf::AccessorType::Scalar) {
        throw std::runtime_error("glTF index accessor must be scalar");
    }
    if (accessor.componentType != fastgltf::ComponentType::UnsignedByte &&
        accessor.componentType != fastgltf::ComponentType::UnsignedShort &&
        accessor.componentType != fastgltf::ComponentType::UnsignedInt) {
        throw std::runtime_error("glTF index accessor component type is unsupported");
    }
    require_accessor_buffer_view(asset, accessor, "glTF index");
    reserve_payload_bytes(index_bytes, checked_payload_bytes(accessor.count, 4U, "glTF index payload"),
                          "glTF index payload");

    for (const auto index : fastgltf::iterateAccessor<std::uint32_t>(asset, accessor)) {
        if (index >= primitive_vertex_count) {
            throw std::runtime_error("glTF index references a vertex outside POSITION accessor");
        }
        if (index > (std::numeric_limits<std::uint32_t>::max)() - vertex_base) {
            throw std::overflow_error("glTF index value overflows uint32");
        }
        append_le_u32(index_bytes, vertex_base + index);
    }
}

void append_generated_gltf_indices(std::uint32_t vertex_base, std::size_t primitive_vertex_count,
                                   std::vector<std::uint8_t>& index_bytes) {
    reserve_payload_bytes(index_bytes, checked_payload_bytes(primitive_vertex_count, 4U, "glTF index payload"),
                          "glTF index payload");
    for (std::size_t index = 0; index < primitive_vertex_count; ++index) {
        if (index > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)() - vertex_base)) {
            throw std::overflow_error("glTF generated index value overflows uint32");
        }
        append_le_u32(index_bytes, vertex_base + static_cast<std::uint32_t>(index));
    }
}

[[nodiscard]] MeshSourceDocument mesh_document_from_gltf(std::string_view bytes, std::string_view source_path) {
    const auto* data = reinterpret_cast<const std::byte*>(bytes.data());
    auto buffer = fastgltf::GltfDataBuffer::FromBytes(data, bytes.size());
    if (buffer.error() != fastgltf::Error::None) {
        throw std::runtime_error("failed to create glTF buffer: " + fastgltf_error_text(buffer.error()));
    }

    auto source_directory = std::filesystem::path{std::string(source_path)}.parent_path();
    std::error_code directory_error;
    if (source_directory.empty() || !std::filesystem::exists(source_directory, directory_error)) {
        source_directory = std::filesystem::current_path();
    }

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(buffer.get(), source_directory, fastgltf::Options::LoadExternalBuffers);
    if (asset.error() != fastgltf::Error::None) {
        throw std::runtime_error("failed to parse glTF: " + fastgltf_error_text(asset.error()));
    }
    const auto& gltf = asset.get();

    std::size_t vertex_count = 0;
    std::size_t index_count = 0;
    bool has_normals = false;
    bool has_uvs = false;
    bool has_tangent_frame = false;
    bool vertex_layout_seen = false;
    bool mesh_uses_tangent_space = false;
    std::vector<std::uint8_t> vertex_bytes;
    std::vector<std::uint8_t> index_bytes;

    for (const auto& mesh : gltf.meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                continue;
            }

            const auto position = primitive.findAttribute("POSITION");
            if (position == primitive.attributes.end()) {
                throw std::runtime_error("glTF triangle primitive is missing POSITION");
            }
            const auto joints0 = primitive.findAttribute("JOINTS_0");
            const auto weights0 = primitive.findAttribute("WEIGHTS_0");
            if (joints0 != primitive.attributes.end() || weights0 != primitive.attributes.end()) {
                throw std::runtime_error(
                    "glTF primitive declares skinning attributes (JOINTS_0/WEIGHTS_0); mesh cook does not yet import "
                    "skins or animations (see gltf-animation-skin-import-v1); remove skinning from the primitive or "
                    "wait for the skin/animation cook path");
            }
            const auto vertex_base = checked_u32(vertex_count, "glTF vertex base");
            const auto& position_accessor = require_accessor(gltf, position->accessorIndex, "glTF POSITION");
            const auto primitive_vertex_count = position_accessor.count;
            const auto normal = primitive.findAttribute("NORMAL");
            const auto uv = primitive.findAttribute("TEXCOORD_0");
            if ((normal == primitive.attributes.end()) != (uv == primitive.attributes.end())) {
                throw std::runtime_error("glTF primitive must provide NORMAL and TEXCOORD_0 together");
            }
            const bool primitive_tangent_space = normal != primitive.attributes.end();
            if (vertex_layout_seen && primitive_tangent_space != mesh_uses_tangent_space) {
                throw std::runtime_error("glTF mesh primitives must use one vertex layout");
            }
            vertex_layout_seen = true;
            mesh_uses_tangent_space = primitive_tangent_space;
            if (primitive_tangent_space) {
                const auto tangent = primitive.findAttribute("TANGENT");
                if (tangent == primitive.attributes.end()) {
                    throw std::runtime_error(
                        "glTF lit primitive is missing TANGENT (required for GameEngine.MeshSource.v2 tangent-space "
                        "vertices)");
                }
                const auto& normal_accessor = require_accessor(gltf, normal->accessorIndex, "glTF NORMAL");
                const auto& uv_accessor = require_accessor(gltf, uv->accessorIndex, "glTF TEXCOORD_0");
                const auto& tangent_accessor = require_accessor(gltf, tangent->accessorIndex, "glTF TANGENT");
                append_gltf_tangent_space_vertices(gltf, position_accessor, normal_accessor, uv_accessor,
                                                   tangent_accessor, vertex_bytes);
                has_normals = true;
                has_uvs = true;
                has_tangent_frame = true;
            } else {
                append_gltf_positions(gltf, position_accessor, vertex_bytes);
            }
            checked_add(vertex_count, primitive_vertex_count, "glTF vertex count");

            if (primitive.indicesAccessor.has_value()) {
                const auto& indices_accessor = require_accessor(gltf, primitive.indicesAccessor.value(), "glTF index");
                append_gltf_indices(gltf, indices_accessor, vertex_base, primitive_vertex_count, index_bytes);
                checked_add(index_count, indices_accessor.count, "glTF index count");
            } else {
                append_generated_gltf_indices(vertex_base, primitive_vertex_count, index_bytes);
                checked_add(index_count, primitive_vertex_count, "glTF index count");
            }
        }
    }

    return MeshSourceDocument{
        checked_u32(vertex_count, "glTF vertex count"),
        checked_u32(index_count, "glTF index count"),
        has_normals,
        has_uvs,
        has_tangent_frame,
        std::move(vertex_bytes),
        std::move(index_bytes),
    };
}

#endif

} // namespace

bool PngTextureExternalAssetImporter::supports(const AssetImportAction& action) const noexcept {
    return action.kind == AssetImportActionKind::texture && has_extension(action.source_path, {".png"});
}

std::string PngTextureExternalAssetImporter::import_source_document(IFileSystem& filesystem,
                                                                    const AssetImportAction& action) {
#if MK_HAS_ASSET_IMPORTERS
    const auto bytes = filesystem.read_text(action.source_path);
    if (bytes.empty()) {
        throw std::invalid_argument("PNG source is empty");
    }
    const auto* png_bytes = reinterpret_cast<const std::uint8_t*>(bytes.data());
    const auto decoded = decode_audited_png_rgba8({png_bytes, bytes.size()});
    return serialize_texture_source_document(decoded);
#else
    (void)filesystem;
    (void)action;
    throw_feature_disabled();
#endif
}

bool GltfMeshExternalAssetImporter::supports(const AssetImportAction& action) const noexcept {
    return action.kind == AssetImportActionKind::mesh && has_extension(action.source_path, {".gltf", ".glb"});
}

std::string GltfMeshExternalAssetImporter::import_source_document(IFileSystem& filesystem,
                                                                  const AssetImportAction& action) {
#if MK_HAS_ASSET_IMPORTERS
    const auto bytes = filesystem.read_text(action.source_path);
    if (bytes.empty()) {
        throw std::invalid_argument("glTF source is empty");
    }
    return serialize_mesh_source_document(mesh_document_from_gltf(bytes, action.source_path));
#else
    (void)filesystem;
    (void)action;
    throw_feature_disabled();
#endif
}

bool GltfMorphMeshCpuExternalAssetImporter::supports(const AssetImportAction& action) const noexcept {
    return action.kind == AssetImportActionKind::morph_mesh_cpu && has_extension(action.source_path, {".gltf", ".glb"});
}

std::string GltfMorphMeshCpuExternalAssetImporter::import_source_document(IFileSystem& filesystem,
                                                                          const AssetImportAction& action) {
#if MK_HAS_ASSET_IMPORTERS
    const auto bytes = filesystem.read_text(action.source_path);
    if (bytes.empty()) {
        throw std::invalid_argument("glTF source is empty");
    }
    for (std::size_t mesh_index = 0; mesh_index < 4096U; ++mesh_index) {
        bool mesh_exhausted = false;
        for (std::size_t primitive_index = 0; primitive_index < 4096U; ++primitive_index) {
            const auto report =
                import_gltf_morph_mesh_cpu_primitive(bytes, action.source_path, mesh_index, primitive_index);
            if (report.succeeded) {
                return serialize_morph_mesh_cpu_source_document(
                    morph_mesh_cpu_source_document_from_animation_desc(report.morph_mesh));
            }
            if (report.diagnostic.find("mesh index is out of range") != std::string::npos) {
                mesh_exhausted = true;
                break;
            }
            if (report.diagnostic.find("primitive index is out of range") != std::string::npos) {
                break;
            }
        }
        if (mesh_exhausted) {
            break;
        }
    }
    throw std::invalid_argument(
        "glTF morph_mesh_cpu import found no importable morph triangle primitive (requires POSITION, targets, no "
        "skinning, no Draco, no sparse accessors)");
#else
    (void)filesystem;
    (void)action;
    throw_feature_disabled();
#endif
}

bool AudioExternalAssetImporter::supports(const AssetImportAction& action) const noexcept {
    return action.kind == AssetImportActionKind::audio && has_extension(action.source_path, {".wav", ".mp3", ".flac"});
}

std::string AudioExternalAssetImporter::import_source_document(IFileSystem& filesystem,
                                                               const AssetImportAction& action) {
#if MK_HAS_ASSET_IMPORTERS
    const auto bytes = filesystem.read_text(action.source_path);
    if (bytes.empty()) {
        throw std::invalid_argument("audio source is empty");
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder{};
    if (ma_decoder_init_memory(bytes.data(), bytes.size(), &config, &decoder) != MA_SUCCESS) {
        throw std::runtime_error("failed to initialize audio decoder");
    }
    const auto decoder_guard = std::unique_ptr<ma_decoder, decltype(&ma_decoder_uninit)>(&decoder, ma_decoder_uninit);

    ma_format format = ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint32 sample_rate = 0;
    if (ma_decoder_get_data_format(&decoder, &format, &channels, &sample_rate, nullptr, 0) != MA_SUCCESS) {
        throw std::runtime_error("failed to read audio format");
    }

    ma_uint64 frames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frames) != MA_SUCCESS || frames == 0) {
        throw std::runtime_error("failed to read audio length");
    }

    const auto sample_format =
        format == ma_format_f32 ? AudioSourceSampleFormat::float32 : AudioSourceSampleFormat::pcm16;
    std::vector<std::uint8_t> samples(
        checked_audio_sample_bytes(frames, channels, audio_source_bytes_per_sample(sample_format)));
    ma_uint64 frames_read = 0;
    if (ma_decoder_read_pcm_frames(&decoder, samples.data(), frames, &frames_read) != MA_SUCCESS ||
        frames_read != frames) {
        throw std::runtime_error("failed to decode complete audio payload");
    }

    return serialize_audio_source_document(
        AudioSourceDocument{sample_rate, channels, frames, sample_format, std::move(samples)});
#else
    (void)filesystem;
    (void)action;
    throw_feature_disabled();
#endif
}

AssetImportExecutionOptions ExternalAssetImportAdapters::options() {
    AssetImportExecutionOptions result;
    result.external_importers = {
        &png_textures,
        &gltf_meshes,
        &gltf_morph_meshes_cpu,
        &audio_sources,
    };
    return result;
}

bool external_asset_importers_available() noexcept {
    return MK_HAS_ASSET_IMPORTERS != 0;
}

} // namespace mirakana
