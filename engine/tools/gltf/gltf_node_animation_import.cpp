// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gltf_node_animation_import.hpp"

#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
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

constexpr float gltf_node_quaternion_xy_epsilon = 1.0e-4F;

[[nodiscard]] std::string fastgltf_error_text(const fastgltf::Error error) {
    return std::string(fastgltf::getErrorMessage(error));
}

void require_loaded_buffer_view(const fastgltf::Asset& asset, const std::size_t buffer_view_index,
                                const std::string_view name) {
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

[[nodiscard]] const fastgltf::Accessor& require_accessor(const fastgltf::Asset& asset, const std::size_t accessor_index,
                                                         const std::string_view name) {
    if (accessor_index >= asset.accessors.size()) {
        throw std::runtime_error(std::string(name) + " references an unknown accessor");
    }
    return asset.accessors[accessor_index];
}

void require_accessor_buffer_view(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor,
                                  const std::string_view name) {
    if (!accessor.bufferViewIndex.has_value()) {
        throw std::runtime_error(std::string(name) + " accessor does not contain loadable buffer data");
    }
    require_loaded_buffer_view(asset, accessor.bufferViewIndex.value(), name);
}

void reject_sparse_accessor(const fastgltf::Accessor& accessor, const std::string_view name) {
    if (accessor.sparse.has_value()) {
        throw std::runtime_error(std::string(name) + " sparse accessors are unsupported for node animation import");
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

[[nodiscard]] std::vector<float> read_time_keys(const fastgltf::Asset& gltf, const fastgltf::Accessor& accessor,
                                                std::string& diagnostic) {
    reject_sparse_accessor(accessor, "glTF animation sampler input");
    if (accessor.type != fastgltf::AccessorType::Scalar || accessor.componentType != fastgltf::ComponentType::Float) {
        diagnostic = "glTF animation sampler input must be float scalar time keys";
        return {};
    }
    require_accessor_buffer_view(gltf, accessor, "glTF animation sampler input");

    std::vector<float> times;
    times.reserve(static_cast<std::size_t>(accessor.count));
    for (const float time : fastgltf::iterateAccessor<float>(gltf, accessor)) {
        if (!std::isfinite(time) || time < 0.0F) {
            diagnostic = "glTF animation sampler input contains invalid time keys";
            return {};
        }
        if (!times.empty() && !(time > times.back())) {
            diagnostic = "glTF animation sampler input times must be strictly increasing";
            return {};
        }
        times.push_back(time);
    }
    if (times.empty()) {
        diagnostic = "glTF animation sampler input must contain at least one time key";
        return {};
    }
    if (times.size() != accessor.count) {
        diagnostic = "glTF animation sampler input iteration count mismatch";
        return {};
    }
    return times;
}

[[nodiscard]] bool require_output_count(const fastgltf::Accessor& accessor, const std::size_t time_count,
                                        const std::string_view label, std::string& diagnostic) {
    if (accessor.count != time_count) {
        diagnostic = std::string(label) + " output count must match input time count";
        return false;
    }
    return true;
}

[[nodiscard]] bool is_supported_gltf_rotation_output_accessor(const fastgltf::Accessor& accessor) {
    if (accessor.type != fastgltf::AccessorType::Vec4) {
        return false;
    }
    if (accessor.componentType == fastgltf::ComponentType::Float) {
        return true;
    }
    if (!accessor.normalized) {
        return false;
    }
    return accessor.componentType == fastgltf::ComponentType::Byte ||
           accessor.componentType == fastgltf::ComponentType::UnsignedByte ||
           accessor.componentType == fastgltf::ComponentType::Short ||
           accessor.componentType == fastgltf::ComponentType::UnsignedShort;
}

#endif

void append_f32_le(std::vector<std::uint8_t>& bytes, const float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    bytes.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
}

void append_float_clip_track(AnimationFloatClipSourceDocument& clip, const std::string& target,
                             const std::vector<FloatKeyframe>& keyframes) {
    AnimationFloatClipTrackSourceDocument track;
    track.target = target;
    track.time_seconds_bytes.reserve(keyframes.size() * sizeof(float));
    track.value_bytes.reserve(keyframes.size() * sizeof(float));
    for (const auto& keyframe : keyframes) {
        append_f32_le(track.time_seconds_bytes, keyframe.time_seconds);
        append_f32_le(track.value_bytes, keyframe.value);
    }
    clip.tracks.push_back(std::move(track));
}

void append_vec3_float_clip_tracks(AnimationFloatClipSourceDocument& clip, const std::string& target_prefix,
                                   const std::vector<Vec3Keyframe>& keyframes) {
    std::vector<FloatKeyframe> x_keyframes;
    std::vector<FloatKeyframe> y_keyframes;
    std::vector<FloatKeyframe> z_keyframes;
    x_keyframes.reserve(keyframes.size());
    y_keyframes.reserve(keyframes.size());
    z_keyframes.reserve(keyframes.size());
    for (const auto& keyframe : keyframes) {
        x_keyframes.push_back(FloatKeyframe{.time_seconds = keyframe.time_seconds, .value = keyframe.value.x});
        y_keyframes.push_back(FloatKeyframe{.time_seconds = keyframe.time_seconds, .value = keyframe.value.y});
        z_keyframes.push_back(FloatKeyframe{.time_seconds = keyframe.time_seconds, .value = keyframe.value.z});
    }
    append_float_clip_track(clip, target_prefix + "/x", x_keyframes);
    append_float_clip_track(clip, target_prefix + "/y", y_keyframes);
    append_float_clip_track(clip, target_prefix + "/z", z_keyframes);
}

void append_vec3_quaternion_clip_component(std::vector<std::uint8_t>& time_seconds_bytes,
                                           std::vector<std::uint8_t>& xyz_bytes,
                                           const std::vector<Vec3Keyframe>& keyframes) {
    time_seconds_bytes.reserve(keyframes.size() * sizeof(float));
    xyz_bytes.reserve(keyframes.size() * 3U * sizeof(float));
    for (const auto& keyframe : keyframes) {
        append_f32_le(time_seconds_bytes, keyframe.time_seconds);
        append_f32_le(xyz_bytes, keyframe.value.x);
        append_f32_le(xyz_bytes, keyframe.value.y);
        append_f32_le(xyz_bytes, keyframe.value.z);
    }
}

void append_quat_quaternion_clip_component(std::vector<std::uint8_t>& time_seconds_bytes,
                                           std::vector<std::uint8_t>& xyzw_bytes,
                                           const std::vector<QuatKeyframe>& keyframes) {
    time_seconds_bytes.reserve(keyframes.size() * sizeof(float));
    xyzw_bytes.reserve(keyframes.size() * 4U * sizeof(float));
    for (const auto& keyframe : keyframes) {
        append_f32_le(time_seconds_bytes, keyframe.time_seconds);
        append_f32_le(xyzw_bytes, keyframe.value.x);
        append_f32_le(xyzw_bytes, keyframe.value.y);
        append_f32_le(xyzw_bytes, keyframe.value.z);
        append_f32_le(xyzw_bytes, keyframe.value.w);
    }
}

[[nodiscard]] std::string gltf_node_target_prefix(std::size_t node_index);

void append_quaternion_clip_track(AnimationQuaternionClipSourceDocument& clip,
                                  const GltfNodeTransformAnimationTrack3d& node_track) {
    AnimationQuaternionClipTrackSourceDocument track;
    track.target = gltf_node_target_prefix(node_track.node_index);
    track.joint_index = static_cast<std::uint32_t>(node_track.node_index);
    append_vec3_quaternion_clip_component(track.translation_time_seconds_bytes, track.translation_xyz_bytes,
                                          node_track.translation_keyframes);
    append_quat_quaternion_clip_component(track.rotation_time_seconds_bytes, track.rotation_xyzw_bytes,
                                          node_track.rotation_keyframes);
    append_vec3_quaternion_clip_component(track.scale_time_seconds_bytes, track.scale_xyz_bytes,
                                          node_track.scale_keyframes);
    clip.tracks.push_back(std::move(track));
}

[[nodiscard]] std::string gltf_node_fallback_name(const std::size_t node_index) {
    return std::string{"gltf_node_"} + std::to_string(node_index);
}

[[nodiscard]] std::string gltf_node_target_prefix(const std::size_t node_index) {
    return std::string{"gltf/node/"} + std::to_string(node_index);
}

void append_transform_binding(AnimationTransformBindingSourceDocument& binding_source, std::string target,
                              const std::string& node_name, const AnimationTransformBindingComponent component) {
    binding_source.bindings.push_back(AnimationTransformBindingSourceRow{
        .target = std::move(target),
        .node_name = node_name,
        .component = component,
    });
}

void append_vec3_transform_bindings(AnimationTransformBindingSourceDocument& binding_source,
                                    const std::string& target_prefix, const std::string& node_name,
                                    const AnimationTransformBindingComponent x_component,
                                    const AnimationTransformBindingComponent y_component,
                                    const AnimationTransformBindingComponent z_component) {
    append_transform_binding(binding_source, target_prefix + "/x", node_name, x_component);
    append_transform_binding(binding_source, target_prefix + "/y", node_name, y_component);
    append_transform_binding(binding_source, target_prefix + "/z", node_name, z_component);
}

} // namespace

GltfNodeTransformAnimationImportReport
import_gltf_node_transform_animation_tracks(const std::string_view document_bytes_utf8,
                                            const std::string_view source_path_for_external_buffers,
                                            const std::size_t animation_index) {
    GltfNodeTransformAnimationImportReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)animation_index;
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

    try {
        std::unordered_map<std::size_t, GltfNodeTransformAnimationTrack> tracks_by_node;
        const auto& animation = gltf.animations[animation_index];
        for (const auto& channel : animation.channels) {
            if (!channel.nodeIndex.has_value()) {
                out.diagnostic = "glTF node transform animation channel target must reference a node";
                return out;
            }
            const std::size_t node_index = channel.nodeIndex.value();
            if (node_index >= gltf.nodes.size()) {
                out.diagnostic = "glTF node transform animation channel target node is out of range";
                return out;
            }
            if (channel.samplerIndex >= animation.samplers.size()) {
                out.diagnostic = "glTF animation channel references an unknown sampler";
                return out;
            }
            const auto& sampler = animation.samplers[channel.samplerIndex];
            if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear) {
                out.diagnostic = "glTF node transform animation sampler interpolation must be LINEAR";
                return out;
            }

            const auto& input_accessor = require_accessor(gltf, sampler.inputAccessor, "glTF animation sampler input");
            const auto& output_accessor =
                require_accessor(gltf, sampler.outputAccessor, "glTF animation sampler output");
            reject_sparse_accessor(output_accessor, "glTF animation sampler output");
            require_accessor_buffer_view(gltf, output_accessor, "glTF animation sampler output");
            const auto times = read_time_keys(gltf, input_accessor, out.diagnostic);
            if (!out.diagnostic.empty()) {
                return out;
            }

            auto& track = tracks_by_node[node_index];
            track.node_index = node_index;

            if (channel.path == fastgltf::AnimationPath::Translation) {
                if (!track.translation_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate translation channels for the same node";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF translation animation output must be float32 VEC3";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF translation animation",
                                          out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    track.translation_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
                if (track.translation_keyframes.size() != times.size() ||
                    !is_valid_vec3_keyframes(track.translation_keyframes)) {
                    out.diagnostic = "imported glTF translation animation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Rotation) {
                if (!track.rotation_z_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate rotation channels for the same node";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec4 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF rotation animation output must be float32 VEC4 quaternions";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF rotation animation", out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec4>(gltf, output_accessor)) {
                    const float qx = value[0];
                    const float qy = value[1];
                    const float qz = value[2];
                    const float qw = value[3];
                    if (!std::isfinite(qx) || !std::isfinite(qy) || !std::isfinite(qz) || !std::isfinite(qw)) {
                        out.diagnostic = "glTF animated rotation quaternion contains non-finite components";
                        return out;
                    }
                    if (std::abs(qx) > gltf_node_quaternion_xy_epsilon ||
                        std::abs(qy) > gltf_node_quaternion_xy_epsilon) {
                        out.diagnostic =
                            "glTF animated rotation is not z-axis-only (unsupported for rotation_z_keyframes import)";
                        return out;
                    }
                    const float quat_len = std::sqrt((qx * qx) + (qy * qy) + (qz * qz) + (qw * qw));
                    if (!std::isfinite(quat_len) || quat_len <= 1.0e-6F) {
                        out.diagnostic = "glTF animated rotation quaternion has invalid length";
                        return out;
                    }
                    const float rz = 2.0F * std::atan2(qz / quat_len, qw / quat_len);
                    track.rotation_z_keyframes.push_back(FloatKeyframe{.time_seconds = times[index], .value = rz});
                    ++index;
                }
                if (track.rotation_z_keyframes.size() != times.size() ||
                    !is_valid_float_keyframes(track.rotation_z_keyframes)) {
                    out.diagnostic = "imported glTF rotation animation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Scale) {
                if (!track.scale_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate scale channels for the same node";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF scale animation output must be float32 VEC3";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF scale animation", out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    if (!std::isfinite(value[0]) || !std::isfinite(value[1]) || !std::isfinite(value[2]) ||
                        value[0] <= 0.0F || value[1] <= 0.0F || value[2] <= 0.0F) {
                        out.diagnostic = "glTF scale animation output must be finite and strictly positive";
                        return out;
                    }
                    track.scale_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
                if (track.scale_keyframes.size() != times.size() || !is_valid_vec3_keyframes(track.scale_keyframes)) {
                    out.diagnostic = "imported glTF scale animation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Weights) {
                out.diagnostic = "glTF weights animation channels must use the morph weights importer";
                return out;
            } else {
                out.diagnostic =
                    "glTF animation channel path is unsupported for node transform import (only translation, "
                    "rotation, scale)";
                return out;
            }
        }

        if (tracks_by_node.empty()) {
            out.diagnostic = "glTF animation contains no node transform channels";
            return out;
        }

        std::vector<std::size_t> node_indices;
        node_indices.reserve(tracks_by_node.size());
        for (const auto& entry : tracks_by_node) {
            node_indices.push_back(entry.first);
        }
        std::sort(node_indices.begin(), node_indices.end());

        out.node_tracks.clear();
        out.node_tracks.reserve(node_indices.size());
        for (const std::size_t node_index : node_indices) {
            auto& track = tracks_by_node[node_index];
            track.node_name = gltf.nodes[node_index].name.empty() ? gltf_node_fallback_name(node_index)
                                                                  : std::string{gltf.nodes[node_index].name};
            out.node_tracks.push_back(std::move(track));
        }
        out.succeeded = true;
        return out;
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }
#endif
}

GltfNodeTransformAnimationImport3dReport
import_gltf_node_transform_animation_tracks_3d(const std::string_view document_bytes_utf8,
                                               const std::string_view source_path_for_external_buffers,
                                               const std::size_t animation_index) {
    GltfNodeTransformAnimationImport3dReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)animation_index;
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

    try {
        std::unordered_map<std::size_t, GltfNodeTransformAnimationTrack3d> tracks_by_node;
        const auto& animation = gltf.animations[animation_index];
        for (const auto& channel : animation.channels) {
            if (!channel.nodeIndex.has_value()) {
                out.diagnostic = "glTF node transform animation channel target must reference a node";
                return out;
            }
            const std::size_t node_index = channel.nodeIndex.value();
            if (node_index >= gltf.nodes.size()) {
                out.diagnostic = "glTF node transform animation channel target node is out of range";
                return out;
            }
            if (channel.samplerIndex >= animation.samplers.size()) {
                out.diagnostic = "glTF animation channel references an unknown sampler";
                return out;
            }
            const auto& sampler = animation.samplers[channel.samplerIndex];
            if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear) {
                out.diagnostic = "glTF node transform animation sampler interpolation must be LINEAR";
                return out;
            }

            const auto& input_accessor = require_accessor(gltf, sampler.inputAccessor, "glTF animation sampler input");
            const auto& output_accessor =
                require_accessor(gltf, sampler.outputAccessor, "glTF animation sampler output");
            reject_sparse_accessor(output_accessor, "glTF animation sampler output");
            require_accessor_buffer_view(gltf, output_accessor, "glTF animation sampler output");
            const auto times = read_time_keys(gltf, input_accessor, out.diagnostic);
            if (!out.diagnostic.empty()) {
                return out;
            }

            auto& track = tracks_by_node[node_index];
            track.node_index = node_index;

            if (channel.path == fastgltf::AnimationPath::Translation) {
                if (!track.translation_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate translation channels for the same node";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF translation animation output must be float32 VEC3";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF translation animation",
                                          out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    track.translation_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
                if (track.translation_keyframes.size() != times.size() ||
                    !is_valid_vec3_keyframes(track.translation_keyframes)) {
                    out.diagnostic = "imported glTF translation animation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Rotation) {
                if (!track.rotation_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate rotation channels for the same node";
                    return out;
                }
                if (!is_supported_gltf_rotation_output_accessor(output_accessor)) {
                    out.diagnostic =
                        "glTF quaternion rotation animation output must be VEC4 float or normalized integer values";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF quaternion rotation animation",
                                          out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec4>(gltf, output_accessor)) {
                    const Quat quat{value[0], value[1], value[2], value[3]};
                    if (!is_finite_quat(quat)) {
                        out.diagnostic = "glTF animated rotation quaternion contains non-finite components";
                        return out;
                    }
                    if (!is_normalized_quat(quat)) {
                        out.diagnostic = "glTF animated rotation quaternion must be normalized";
                        return out;
                    }
                    track.rotation_keyframes.push_back(QuatKeyframe{.time_seconds = times[index], .value = quat});
                    ++index;
                }
                if (track.rotation_keyframes.size() != times.size() ||
                    !is_valid_quat_keyframes(track.rotation_keyframes)) {
                    out.diagnostic = "imported glTF quaternion rotation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Scale) {
                if (!track.scale_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate scale channels for the same node";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF scale animation output must be float32 VEC3";
                    return out;
                }
                if (!require_output_count(output_accessor, times.size(), "glTF scale animation", out.diagnostic)) {
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    if (!std::isfinite(value[0]) || !std::isfinite(value[1]) || !std::isfinite(value[2]) ||
                        value[0] <= 0.0F || value[1] <= 0.0F || value[2] <= 0.0F) {
                        out.diagnostic = "glTF scale animation output must be finite and strictly positive";
                        return out;
                    }
                    track.scale_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
                if (track.scale_keyframes.size() != times.size() || !is_valid_vec3_keyframes(track.scale_keyframes)) {
                    out.diagnostic = "imported glTF scale animation keyframes failed engine validation";
                    return out;
                }
            } else if (channel.path == fastgltf::AnimationPath::Weights) {
                out.diagnostic = "glTF weights animation channels must use the morph weights importer";
                return out;
            } else {
                out.diagnostic =
                    "glTF animation channel path is unsupported for 3D node transform import (only translation, "
                    "rotation, scale)";
                return out;
            }
        }

        if (tracks_by_node.empty()) {
            out.diagnostic = "glTF animation contains no 3D node transform channels";
            return out;
        }

        std::vector<std::size_t> node_indices;
        node_indices.reserve(tracks_by_node.size());
        for (const auto& entry : tracks_by_node) {
            node_indices.push_back(entry.first);
        }
        std::sort(node_indices.begin(), node_indices.end());

        out.node_tracks.clear();
        out.node_tracks.reserve(node_indices.size());
        for (const std::size_t node_index : node_indices) {
            auto& track = tracks_by_node[node_index];
            track.node_name = gltf.nodes[node_index].name.empty() ? gltf_node_fallback_name(node_index)
                                                                  : std::string{gltf.nodes[node_index].name};
            out.node_tracks.push_back(std::move(track));
        }
        out.succeeded = true;
        return out;
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }
#endif
}

GltfNodeTransformAnimationFloatClipImportReport
import_gltf_node_transform_animation_float_clip(const std::string_view document_bytes_utf8,
                                                const std::string_view source_path_for_external_buffers,
                                                const std::size_t animation_index) {
    GltfNodeTransformAnimationFloatClipImportReport out;
    const auto imported = import_gltf_node_transform_animation_tracks(
        document_bytes_utf8, source_path_for_external_buffers, animation_index);
    if (!imported.succeeded) {
        out.diagnostic = imported.diagnostic;
        return out;
    }

    for (const auto& node_track : imported.node_tracks) {
        const auto node_prefix = gltf_node_target_prefix(node_track.node_index);
        if (!node_track.translation_keyframes.empty()) {
            append_vec3_float_clip_tracks(out.clip, node_prefix + "/translation", node_track.translation_keyframes);
        }
        if (!node_track.rotation_z_keyframes.empty()) {
            append_float_clip_track(out.clip, node_prefix + "/rotation_z", node_track.rotation_z_keyframes);
        }
        if (!node_track.scale_keyframes.empty()) {
            append_vec3_float_clip_tracks(out.clip, node_prefix + "/scale", node_track.scale_keyframes);
        }
    }

    if (!is_valid_animation_float_clip_source_document(out.clip)) {
        out.diagnostic = "imported glTF node transform animation float clip failed engine validation";
        return out;
    }

    out.succeeded = true;
    return out;
}

GltfNodeTransformAnimationQuaternionClipImportReport
import_gltf_node_transform_animation_quaternion_clip(const std::string_view document_bytes_utf8,
                                                     const std::string_view source_path_for_external_buffers,
                                                     const std::size_t animation_index) {
    GltfNodeTransformAnimationQuaternionClipImportReport out;
    const auto imported = import_gltf_node_transform_animation_tracks_3d(
        document_bytes_utf8, source_path_for_external_buffers, animation_index);
    if (!imported.succeeded) {
        out.diagnostic = imported.diagnostic;
        return out;
    }

    for (const auto& node_track : imported.node_tracks) {
        if (node_track.node_index > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
            out.diagnostic = "imported glTF node transform animation quaternion clip node index exceeds uint32 range";
            return out;
        }
        append_quaternion_clip_track(out.clip, node_track);
    }

    if (!is_valid_animation_quaternion_clip_source_document(out.clip)) {
        out.diagnostic = "imported glTF node transform animation quaternion clip failed engine validation";
        return out;
    }

    out.succeeded = true;
    return out;
}

GltfNodeTransformAnimationBindingSourceImportReport
import_gltf_node_transform_animation_binding_source(const std::string_view document_bytes_utf8,
                                                    const std::string_view source_path_for_external_buffers,
                                                    const std::size_t animation_index) {
    GltfNodeTransformAnimationBindingSourceImportReport out;
    const auto imported = import_gltf_node_transform_animation_tracks(
        document_bytes_utf8, source_path_for_external_buffers, animation_index);
    if (!imported.succeeded) {
        out.diagnostic = imported.diagnostic;
        return out;
    }

    for (const auto& node_track : imported.node_tracks) {
        const auto node_prefix = gltf_node_target_prefix(node_track.node_index);
        const auto node_name =
            node_track.node_name.empty() ? gltf_node_fallback_name(node_track.node_index) : node_track.node_name;
        if (!node_track.translation_keyframes.empty()) {
            append_vec3_transform_bindings(out.binding_source, node_prefix + "/translation", node_name,
                                           AnimationTransformBindingComponent::translation_x,
                                           AnimationTransformBindingComponent::translation_y,
                                           AnimationTransformBindingComponent::translation_z);
        }
        if (!node_track.rotation_z_keyframes.empty()) {
            append_transform_binding(out.binding_source, node_prefix + "/rotation_z", node_name,
                                     AnimationTransformBindingComponent::rotation_z);
        }
        if (!node_track.scale_keyframes.empty()) {
            append_vec3_transform_bindings(
                out.binding_source, node_prefix + "/scale", node_name, AnimationTransformBindingComponent::scale_x,
                AnimationTransformBindingComponent::scale_y, AnimationTransformBindingComponent::scale_z);
        }
    }

    if (!is_valid_animation_transform_binding_source_document(out.binding_source)) {
        out.diagnostic = "imported glTF node transform animation binding source failed engine validation";
        return out;
    }

    out.succeeded = true;
    return out;
}

} // namespace mirakana
