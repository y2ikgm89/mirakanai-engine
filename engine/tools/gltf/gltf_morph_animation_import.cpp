// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gltf_morph_animation_import.hpp"

#include "mirakana/math/vec.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
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
#endif

namespace mirakana {
namespace {

#if MK_HAS_ASSET_IMPORTERS

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

void reject_sparse_accessor(const fastgltf::Accessor& accessor, std::string_view name) {
    if (accessor.sparse.has_value()) {
        throw std::runtime_error(std::string(name) + " sparse accessors are unsupported for morph import");
    }
}

[[nodiscard]] fastgltf::Expected<fastgltf::Asset>
load_gltf_asset(const std::string_view document_bytes_utf8, const std::string_view source_path_for_external_buffers,
                std::string& diagnostic) {
    if (document_bytes_utf8.empty()) {
        diagnostic = "glTF document bytes are empty";
        return fastgltf::Error::InvalidGltf;
    }
    const auto* data = reinterpret_cast<const std::byte*>(document_bytes_utf8.data());
    auto buffer = fastgltf::GltfDataBuffer::FromBytes(data, document_bytes_utf8.size());
    if (buffer.error() != fastgltf::Error::None) {
        diagnostic = "failed to create glTF buffer: " + fastgltf_error_text(buffer.error());
        return buffer.error();
    }

    auto source_directory = std::filesystem::path{std::string(source_path_for_external_buffers)}.parent_path();
    std::error_code directory_error;
    if (source_directory.empty() || !std::filesystem::exists(source_directory, directory_error)) {
        source_directory = std::filesystem::current_path();
    }

    fastgltf::Parser parser;
    auto expected = parser.loadGltf(buffer.get(), source_directory,
                                    fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices);
    if (expected.error() != fastgltf::Error::None) {
        diagnostic = "failed to parse glTF: " + fastgltf_error_text(expected.error());
    }
    return expected;
}

[[nodiscard]] std::vector<Vec3> read_vec3_f32_accessor(const fastgltf::Asset& gltf, const fastgltf::Accessor& accessor,
                                                       std::string_view name) {
    reject_sparse_accessor(accessor, name);
    if (accessor.type != fastgltf::AccessorType::Vec3 || accessor.componentType != fastgltf::ComponentType::Float) {
        throw std::runtime_error(std::string(name) + " accessor must be float32 VEC3");
    }
    require_accessor_buffer_view(gltf, accessor, name);
    std::vector<Vec3> out;
    out.reserve(static_cast<std::size_t>(accessor.count));
    for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, accessor)) {
        out.push_back(Vec3{value[0], value[1], value[2]});
    }
    if (out.size() != accessor.count) {
        throw std::runtime_error(std::string(name) + " accessor iteration count mismatch");
    }
    return out;
}

[[nodiscard]] bool finite_unit_interval(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

#endif

void append_f32_le(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    bytes.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
}

} // namespace

GltfMorphMeshCpuImportReport
import_gltf_morph_mesh_cpu_primitive(const std::string_view document_bytes_utf8,
                                     const std::string_view source_path_for_external_buffers,
                                     const std::size_t mesh_index, const std::size_t primitive_index) {
    GltfMorphMeshCpuImportReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)mesh_index;
    (void)primitive_index;
    out.diagnostic = "asset importers are disabled for this MK_tools build";
    return out;
#else
    auto expected = load_gltf_asset(document_bytes_utf8, source_path_for_external_buffers, out.diagnostic);
    if (expected.error() != fastgltf::Error::None) {
        return out;
    }
    const auto& gltf = expected.get();

    if (mesh_index >= gltf.meshes.size()) {
        out.diagnostic = "glTF mesh index is out of range";
        return out;
    }
    const auto& mesh = gltf.meshes[mesh_index];
    if (primitive_index >= mesh.primitives.size()) {
        out.diagnostic = "glTF primitive index is out of range";
        return out;
    }
    const auto& primitive = mesh.primitives[primitive_index];
    if (primitive.type != fastgltf::PrimitiveType::Triangles) {
        out.diagnostic = "glTF morph import requires triangle primitives";
        return out;
    }
    if (primitive.dracoCompression != nullptr) {
        out.diagnostic = "glTF Draco-compressed primitives are unsupported for morph import";
        return out;
    }
    if (primitive.findAttribute("JOINTS_0") != primitive.attributes.end() ||
        primitive.findAttribute("WEIGHTS_0") != primitive.attributes.end()) {
        out.diagnostic =
            "glTF morph import rejects skinned primitives (JOINTS_0/WEIGHTS_0); use skin import for skinned meshes";
        return out;
    }

    if (primitive.targets.empty()) {
        out.diagnostic = "glTF morph import requires a non-empty primitive.targets array";
        return out;
    }

    try {
        const auto position = primitive.findAttribute("POSITION");
        if (position == primitive.attributes.end()) {
            out.diagnostic = "glTF morph import requires POSITION on the base primitive";
            return out;
        }
        const auto& position_accessor = require_accessor(gltf, position->accessorIndex, "glTF POSITION");
        reject_sparse_accessor(position_accessor, "glTF POSITION");
        if (position_accessor.type != fastgltf::AccessorType::Vec3 ||
            position_accessor.componentType != fastgltf::ComponentType::Float) {
            out.diagnostic = "glTF POSITION must be float32 VEC3 for morph import";
            return out;
        }
        require_accessor_buffer_view(gltf, position_accessor, "glTF POSITION");
        const std::size_t vertex_count = static_cast<std::size_t>(position_accessor.count);
        if (vertex_count == 0) {
            out.diagnostic = "glTF POSITION accessor must be non-empty";
            return out;
        }

        out.morph_mesh.bind_positions = read_vec3_f32_accessor(gltf, position_accessor, "glTF POSITION");

        const auto normal = primitive.findAttribute("NORMAL");
        if (normal != primitive.attributes.end()) {
            const auto& accessor = require_accessor(gltf, normal->accessorIndex, "glTF NORMAL");
            if (accessor.count != position_accessor.count) {
                out.diagnostic = "glTF NORMAL accessor count must match POSITION for morph import";
                return out;
            }
            out.morph_mesh.bind_normals = read_vec3_f32_accessor(gltf, accessor, "glTF NORMAL");
        }

        const auto tangent = primitive.findAttribute("TANGENT");
        if (tangent != primitive.attributes.end()) {
            const auto& accessor = require_accessor(gltf, tangent->accessorIndex, "glTF TANGENT");
            if (accessor.count != position_accessor.count) {
                out.diagnostic = "glTF TANGENT accessor count must match POSITION for morph import";
                return out;
            }
            out.morph_mesh.bind_tangents = read_vec3_f32_accessor(gltf, accessor, "glTF TANGENT");
        }

        const std::size_t morph_target_count = primitive.targets.size();
        out.morph_mesh.targets.clear();
        out.morph_mesh.targets.reserve(morph_target_count);

        for (std::size_t target_index = 0; target_index < morph_target_count; ++target_index) {
            AnimationMorphTargetCpuDesc target_desc;

            const auto pos_attr = primitive.findTargetAttribute(target_index, "POSITION");
            if (pos_attr != primitive.targets[target_index].end()) {
                const auto& accessor = require_accessor(gltf, pos_attr->accessorIndex, "glTF morph POSITION");
                if (accessor.count != position_accessor.count) {
                    out.diagnostic = "glTF morph POSITION delta accessor count must match base POSITION count";
                    return out;
                }
                target_desc.position_deltas = read_vec3_f32_accessor(gltf, accessor, "glTF morph POSITION");
            }

            const auto nrm_attr = primitive.findTargetAttribute(target_index, "NORMAL");
            if (nrm_attr != primitive.targets[target_index].end()) {
                const auto& accessor = require_accessor(gltf, nrm_attr->accessorIndex, "glTF morph NORMAL");
                if (accessor.count != position_accessor.count) {
                    out.diagnostic = "glTF morph NORMAL delta accessor count must match base POSITION count";
                    return out;
                }
                target_desc.normal_deltas = read_vec3_f32_accessor(gltf, accessor, "glTF morph NORMAL");
            }

            const auto tan_attr = primitive.findTargetAttribute(target_index, "TANGENT");
            if (tan_attr != primitive.targets[target_index].end()) {
                const auto& accessor = require_accessor(gltf, tan_attr->accessorIndex, "glTF morph TANGENT");
                if (accessor.count != position_accessor.count) {
                    out.diagnostic = "glTF morph TANGENT delta accessor count must match base POSITION count";
                    return out;
                }
                target_desc.tangent_deltas = read_vec3_f32_accessor(gltf, accessor, "glTF morph TANGENT");
            }

            if (target_desc.position_deltas.empty() && target_desc.normal_deltas.empty() &&
                target_desc.tangent_deltas.empty()) {
                out.diagnostic = "each glTF morph target must declare at least one of POSITION, NORMAL, or TANGENT";
                return out;
            }

            out.morph_mesh.targets.push_back(std::move(target_desc));
        }

        out.morph_mesh.target_weights.assign(morph_target_count, 0.0F);
        if (!mesh.weights.empty()) {
            if (mesh.weights.size() != morph_target_count) {
                out.diagnostic = "glTF mesh.weights length must match primitive morph target count";
                return out;
            }
            for (std::size_t i = 0; i < morph_target_count; ++i) {
                const float w = static_cast<float>(mesh.weights[i]);
                if (!finite_unit_interval(w)) {
                    out.diagnostic =
                        "glTF mesh.weights entries must be finite and in [0,1] for engine morph validation";
                    return out;
                }
                out.morph_mesh.target_weights[i] = w;
            }
        }

        const auto morph_diagnostics = validate_animation_morph_mesh_cpu_desc(out.morph_mesh);
        if (!morph_diagnostics.empty()) {
            out.diagnostic = morph_diagnostics.front().message;
            return out;
        }

        out.succeeded = true;
        return out;
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }
#endif
}

GltfMorphWeightsAnimationImportReport import_gltf_animation_morph_weights_for_mesh_primitive(
    const std::string_view document_bytes_utf8, const std::string_view source_path_for_external_buffers,
    const std::size_t animation_index, const std::size_t mesh_index, const std::size_t primitive_index,
    const std::size_t animated_node_index) {
    GltfMorphWeightsAnimationImportReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)animation_index;
    (void)mesh_index;
    (void)primitive_index;
    (void)animated_node_index;
    out.diagnostic = "asset importers are disabled for this MK_tools build";
    return out;
#else
    auto expected = load_gltf_asset(document_bytes_utf8, source_path_for_external_buffers, out.diagnostic);
    if (expected.error() != fastgltf::Error::None) {
        return out;
    }
    const auto& gltf = expected.get();

    if (animation_index >= gltf.animations.size()) {
        out.diagnostic = "glTF animation index is out of range";
        return out;
    }
    if (mesh_index >= gltf.meshes.size()) {
        out.diagnostic = "glTF mesh index is out of range";
        return out;
    }
    if (animated_node_index >= gltf.nodes.size()) {
        out.diagnostic = "glTF animated node index is out of range";
        return out;
    }

    const auto& mesh = gltf.meshes[mesh_index];
    if (primitive_index >= mesh.primitives.size()) {
        out.diagnostic = "glTF primitive index is out of range";
        return out;
    }
    const auto& primitive = mesh.primitives[primitive_index];
    if (primitive.targets.empty()) {
        out.diagnostic = "glTF morph weights animation requires morph targets on the selected primitive";
        return out;
    }
    const std::size_t morph_target_count = primitive.targets.size();

    const auto& node = gltf.nodes[animated_node_index];
    if (!node.meshIndex.has_value() || node.meshIndex.value() != mesh_index) {
        out.diagnostic = "glTF animated node must reference the same mesh_index passed to morph weights import";
        return out;
    }

    try {
        const auto& animation = gltf.animations[animation_index];
        std::size_t weights_channels = 0;
        std::size_t weights_sampler_index = 0;
        for (const auto& channel : animation.channels) {
            if (!channel.nodeIndex.has_value() || channel.nodeIndex.value() != animated_node_index) {
                continue;
            }
            if (channel.path != fastgltf::AnimationPath::Weights) {
                continue;
            }
            ++weights_channels;
            weights_sampler_index = channel.samplerIndex;
        }

        if (weights_channels == 0) {
            out.diagnostic = "glTF animation contains no weights channel for the selected node";
            return out;
        }
        if (weights_channels != 1) {
            out.diagnostic = "glTF animation contains duplicate weights channels for the same node";
            return out;
        }

        if (weights_sampler_index >= animation.samplers.size()) {
            out.diagnostic = "glTF animation weights channel references an unknown sampler";
            return out;
        }
        const auto& sampler = animation.samplers[weights_sampler_index];
        if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear) {
            out.diagnostic = "glTF morph weights animation sampler interpolation must be LINEAR";
            return out;
        }

        const auto& input_accessor = require_accessor(gltf, sampler.inputAccessor, "glTF animation sampler input");
        const auto& output_accessor = require_accessor(gltf, sampler.outputAccessor, "glTF animation sampler output");
        reject_sparse_accessor(input_accessor, "glTF animation sampler input");
        reject_sparse_accessor(output_accessor, "glTF animation sampler output");

        if (input_accessor.type != fastgltf::AccessorType::Scalar ||
            input_accessor.componentType != fastgltf::ComponentType::Float) {
            out.diagnostic = "glTF animation sampler input must be float scalar time keys";
            return out;
        }
        if (output_accessor.type != fastgltf::AccessorType::Scalar ||
            output_accessor.componentType != fastgltf::ComponentType::Float) {
            out.diagnostic = "glTF morph weights animation output must be float scalars";
            return out;
        }
        require_accessor_buffer_view(gltf, input_accessor, "glTF animation sampler input");
        require_accessor_buffer_view(gltf, output_accessor, "glTF animation sampler output");

        std::vector<float> times;
        for (const float time : fastgltf::iterateAccessor<float>(gltf, input_accessor)) {
            if (!std::isfinite(time) || time < 0.0F) {
                out.diagnostic = "glTF animation sampler input contains invalid time keys";
                return out;
            }
            if (!times.empty() && !(time > times.back())) {
                out.diagnostic = "glTF animation sampler input times must be strictly increasing";
                return out;
            }
            times.push_back(time);
        }
        if (times.empty()) {
            out.diagnostic = "glTF animation sampler input must contain at least one time key";
            return out;
        }

        const std::size_t expected_output_count = times.size() * morph_target_count;
        if (output_accessor.count != expected_output_count) {
            out.diagnostic =
                "glTF morph weights animation output accessor count must equal time key count times morph target count";
            return out;
        }

        std::vector<float> output_scalars;
        output_scalars.reserve(static_cast<std::size_t>(output_accessor.count));
        for (const float value : fastgltf::iterateAccessor<float>(gltf, output_accessor)) {
            output_scalars.push_back(value);
        }
        if (output_scalars.size() != output_accessor.count) {
            out.diagnostic = "glTF morph weights animation output iteration count mismatch";
            return out;
        }

        out.weights_track.morph_target_count = morph_target_count;
        out.weights_track.keyframes.clear();
        out.weights_track.keyframes.reserve(times.size());

        for (std::size_t time_index = 0; time_index < times.size(); ++time_index) {
            AnimationMorphWeightsKeyframeDesc row;
            row.time_seconds = times[time_index];
            row.weights.reserve(morph_target_count);
            const std::size_t base = time_index * morph_target_count;
            for (std::size_t morph_index = 0; morph_index < morph_target_count; ++morph_index) {
                const float w = output_scalars[base + morph_index];
                if (!finite_unit_interval(w)) {
                    out.diagnostic =
                        "glTF morph weights animation output samples must be finite and in [0,1] for engine validation";
                    return out;
                }
                row.weights.push_back(w);
            }
            out.weights_track.keyframes.push_back(std::move(row));
        }

        const auto track_diagnostics = validate_animation_morph_weights_track_desc(out.weights_track);
        if (!track_diagnostics.empty()) {
            out.diagnostic = track_diagnostics.front().message;
            return out;
        }

        out.succeeded = true;
        return out;
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }
#endif
}

GltfAnimationFloatClipImportReport import_gltf_morph_weights_animation_float_clip(
    const std::string_view document_bytes_utf8, const std::string_view source_path_for_external_buffers,
    const std::size_t animation_index, const std::size_t mesh_index, const std::size_t primitive_index,
    const std::size_t animated_node_index) {
    GltfAnimationFloatClipImportReport out;
    const auto weights = import_gltf_animation_morph_weights_for_mesh_primitive(
        document_bytes_utf8, source_path_for_external_buffers, animation_index, mesh_index, primitive_index,
        animated_node_index);
    if (!weights.succeeded) {
        out.diagnostic = weights.diagnostic;
        return out;
    }

    const auto& source_track = weights.weights_track;
    if (!is_valid_animation_morph_weights_track_desc(source_track)) {
        out.diagnostic = "glTF morph weights animation did not produce a valid weights track";
        return out;
    }

    out.clip.tracks.clear();
    out.clip.tracks.reserve(source_track.morph_target_count);
    for (std::size_t morph_index = 0; morph_index < source_track.morph_target_count; ++morph_index) {
        AnimationFloatClipTrackSourceDocument track;
        track.target = "gltf/node/" + std::to_string(animated_node_index) + "/weights/" + std::to_string(morph_index);
        track.time_seconds_bytes.reserve(source_track.keyframes.size() * 4U);
        track.value_bytes.reserve(source_track.keyframes.size() * 4U);
        for (const auto& keyframe : source_track.keyframes) {
            if (morph_index >= keyframe.weights.size()) {
                out.diagnostic = "glTF morph weights animation keyframe is missing a target weight";
                return out;
            }
            append_f32_le(track.time_seconds_bytes, keyframe.time_seconds);
            append_f32_le(track.value_bytes, keyframe.weights[morph_index]);
        }
        out.clip.tracks.push_back(std::move(track));
    }

    if (!is_valid_animation_float_clip_source_document(out.clip)) {
        out.diagnostic = "glTF morph weights animation produced an invalid animation float clip source document";
        return out;
    }

    out.succeeded = true;
    return out;
}

} // namespace mirakana
