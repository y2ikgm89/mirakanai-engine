// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gltf_skin_animation_import.hpp"

#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
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

constexpr float gltf_joint_quaternion_xy_epsilon = 1.0e-4F;

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

[[nodiscard]] Mat4 mat4_from_fastgltf_column_major(const fastgltf::math::fmat4x4& source) noexcept {
    const float* column_major = source.data();
    Mat4 out;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            out.at(row, column) = column_major[column * 4U + row];
        }
    }
    return out;
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

[[nodiscard]] std::vector<std::size_t> build_node_parent_table(const fastgltf::Asset& asset) {
    std::vector<std::size_t> parents(asset.nodes.size(), static_cast<std::size_t>(-1));
    for (std::size_t node_index = 0; node_index < asset.nodes.size(); ++node_index) {
        for (const std::size_t child_index : asset.nodes[node_index].children) {
            if (child_index < parents.size()) {
                parents[child_index] = node_index;
            }
        }
    }
    return parents;
}

[[nodiscard]] bool joint_rest_from_gltf_node(const fastgltf::Node& node, JointLocalTransform& out_rest,
                                             std::string& diagnostic) {
    const auto* trs = std::get_if<fastgltf::TRS>(&node.transform);
    if (trs == nullptr) {
        diagnostic =
            "glTF node transform must be TRS after DecomposeNodeMatrices (matrix-only nodes are unsupported here)";
        return false;
    }

    const auto& t = trs->translation;
    const auto& r = trs->rotation;
    const auto& s = trs->scale;

    const float qx = r.x();
    const float qy = r.y();
    const float qz = r.z();
    const float qw = r.w();
    if (!std::isfinite(qx) || !std::isfinite(qy) || !std::isfinite(qz) || !std::isfinite(qw)) {
        diagnostic = "glTF joint rotation quaternion contains non-finite components";
        return false;
    }
    if (std::abs(qx) > gltf_joint_quaternion_xy_epsilon || std::abs(qy) > gltf_joint_quaternion_xy_epsilon) {
        diagnostic =
            "glTF joint rotation is not z-axis-only; GameEngine.AnimationSkeletonDesc uses JointLocalTransform "
            "rotation_z_radians only (see gltf-animation-skin-import-v1)";
        return false;
    }

    const float quat_len = std::sqrt((qx * qx) + (qy * qy) + (qz * qz) + (qw * qw));
    if (!std::isfinite(quat_len) || quat_len <= 1.0e-6F) {
        diagnostic = "glTF joint rotation quaternion has invalid length";
        return false;
    }

    const float rz = 2.0F * std::atan2(qz / quat_len, qw / quat_len);

    out_rest.translation = Vec3{t[0], t[1], t[2]};
    out_rest.rotation_z_radians = rz;
    out_rest.scale = Vec3{s[0], s[1], s[2]};

    if (out_rest.scale.x <= 0.0F || out_rest.scale.y <= 0.0F || out_rest.scale.z <= 0.0F) {
        diagnostic =
            "glTF joint scale must be strictly positive for GameEngine.AnimationSkeletonDesc (non-positive scale is "
            "unsupported)";
        return false;
    }
    return true;
}

[[nodiscard]] std::size_t skeleton_parent_for_skin_joint(const std::vector<std::size_t>& parents,
                                                         const std::vector<std::size_t>& skin_joint_nodes,
                                                         const std::size_t skin_joint_index, std::string& diagnostic) {
    const std::size_t node_index = skin_joint_nodes[skin_joint_index];
    if (node_index >= parents.size()) {
        diagnostic = "glTF skin joint node index is out of range";
        return animation_no_parent;
    }
    const std::size_t parent_node = parents[node_index];
    if (parent_node == static_cast<std::size_t>(-1)) {
        return animation_no_parent;
    }
    for (std::size_t joint_index = 0; joint_index < skin_joint_nodes.size(); ++joint_index) {
        if (skin_joint_nodes[joint_index] == parent_node) {
            if (joint_index >= skin_joint_index) {
                diagnostic = "glTF skin joint parent ordering is invalid for AnimationSkeletonDesc";
                return animation_no_parent;
            }
            return joint_index;
        }
    }
    return animation_no_parent;
}

[[nodiscard]] bool build_skeleton_desc_from_gltf_skin(const fastgltf::Asset& gltf, const std::size_t skin_index,
                                                      AnimationSkeletonDesc& out_skeleton, std::string& diagnostic) {
    if (skin_index >= gltf.skins.size()) {
        diagnostic = "glTF skin index is out of range";
        return false;
    }
    const auto& skin = gltf.skins[skin_index];
    const std::size_t joint_count = skin.joints.size();
    if (joint_count == 0) {
        diagnostic = "glTF skin joint list is empty";
        return false;
    }

    std::vector<std::size_t> skin_joint_nodes;
    skin_joint_nodes.reserve(joint_count);
    for (std::size_t joint_index = 0; joint_index < joint_count; ++joint_index) {
        skin_joint_nodes.push_back(skin.joints[joint_index]);
    }

    const auto parents = build_node_parent_table(gltf);

    out_skeleton.joints.clear();
    out_skeleton.joints.reserve(joint_count);
    for (std::size_t joint_index = 0; joint_index < joint_count; ++joint_index) {
        const std::size_t node_index = skin_joint_nodes[joint_index];
        if (node_index >= gltf.nodes.size()) {
            diagnostic = "glTF skin joint references an unknown node index";
            return false;
        }
        JointLocalTransform rest{};
        std::string rest_diagnostic;
        if (!joint_rest_from_gltf_node(gltf.nodes[node_index], rest, rest_diagnostic)) {
            diagnostic = std::move(rest_diagnostic);
            return false;
        }

        const std::size_t parent_skeleton_index =
            skeleton_parent_for_skin_joint(parents, skin_joint_nodes, joint_index, diagnostic);
        if (!diagnostic.empty()) {
            return false;
        }

        AnimationSkeletonJointDesc joint_desc;
        joint_desc.name = "j" + std::to_string(joint_index);
        joint_desc.parent_index = parent_skeleton_index;
        joint_desc.rest = rest;
        out_skeleton.joints.push_back(std::move(joint_desc));
    }

    const auto skeleton_diagnostics = validate_animation_skeleton_desc(out_skeleton);
    if (!skeleton_diagnostics.empty()) {
        diagnostic = skeleton_diagnostics.front().message;
        return false;
    }
    return true;
}

void append_vertex_influences_from_indices_and_weights(const std::uint32_t j0, const std::uint32_t j1,
                                                       const std::uint32_t j2, const std::uint32_t j3, const float w0,
                                                       const float w1, const float w2, const float w3,
                                                       const std::size_t joint_count,
                                                       AnimationSkinVertexWeights& out_vertex,
                                                       std::string& diagnostic) {
    const auto add = [&](std::uint32_t joint_index, float weight) {
        if (weight <= 0.0F || !std::isfinite(weight)) {
            return;
        }
        if (joint_index >= joint_count) {
            diagnostic = "glTF JOINTS_0 references a skin joint index out of range";
            return;
        }
        for (auto& existing : out_vertex.influences) {
            if (existing.skin_joint_index == joint_index) {
                existing.weight += weight;
                return;
            }
        }
        if (out_vertex.influences.size() >= animation_skin_max_influences) {
            diagnostic = "glTF vertex declares more than four unique joint influences after merging duplicates";
            return;
        }
        out_vertex.influences.push_back(
            AnimationSkinVertexInfluence{.skin_joint_index = joint_index, .weight = weight});
    };

    add(j0, w0);
    if (!diagnostic.empty()) {
        return;
    }
    add(j1, w1);
    if (!diagnostic.empty()) {
        return;
    }
    add(j2, w2);
    if (!diagnostic.empty()) {
        return;
    }
    add(j3, w3);
}

#endif

} // namespace

GltfSkinSkeletonAndSkinPayloadImportReport import_gltf_skin_skeleton_and_skin_payload(
    const std::string_view document_bytes_utf8, const std::string_view source_path_for_external_buffers,
    const std::size_t skin_index, const std::size_t mesh_index, const std::size_t primitive_index) {
    GltfSkinSkeletonAndSkinPayloadImportReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)skin_index;
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

    if (skin_index >= gltf.skins.size()) {
        out.diagnostic = "glTF skin index is out of range";
        return out;
    }
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
        out.diagnostic = "glTF primitive must use triangle mode for skin import";
        return out;
    }

    const auto position = primitive.findAttribute("POSITION");
    const auto joints0 = primitive.findAttribute("JOINTS_0");
    const auto weights0 = primitive.findAttribute("WEIGHTS_0");
    if (position == primitive.attributes.end() || joints0 == primitive.attributes.end() ||
        weights0 == primitive.attributes.end()) {
        out.diagnostic = "glTF skinned primitive must declare POSITION, JOINTS_0, and WEIGHTS_0";
        return out;
    }

    try {
        const auto& skin = gltf.skins[skin_index];
        const std::size_t joint_count = skin.joints.size();

        const auto& position_accessor = require_accessor(gltf, position->accessorIndex, "glTF POSITION");
        const auto& joints_accessor = require_accessor(gltf, joints0->accessorIndex, "glTF JOINTS_0");
        const auto& weights_accessor = require_accessor(gltf, weights0->accessorIndex, "glTF WEIGHTS_0");

        if (joints_accessor.type != fastgltf::AccessorType::Vec4 ||
            weights_accessor.type != fastgltf::AccessorType::Vec4 ||
            weights_accessor.componentType != fastgltf::ComponentType::Float) {
            out.diagnostic = "glTF JOINTS_0 must be VEC4 and WEIGHTS_0 must be float32 VEC4";
            return out;
        }

        if (position_accessor.count != joints_accessor.count || position_accessor.count != weights_accessor.count) {
            out.diagnostic = "glTF POSITION, JOINTS_0, and WEIGHTS_0 accessors must have matching counts";
            return out;
        }

        require_accessor_buffer_view(gltf, position_accessor, "glTF POSITION");
        require_accessor_buffer_view(gltf, joints_accessor, "glTF JOINTS_0");
        require_accessor_buffer_view(gltf, weights_accessor, "glTF WEIGHTS_0");

        if (!build_skeleton_desc_from_gltf_skin(gltf, skin_index, out.skeleton, out.diagnostic)) {
            return out;
        }

        std::vector<Mat4> inverse_bind_matrices(joint_count, Mat4::identity());
        if (skin.inverseBindMatrices.has_value()) {
            const auto& accessor = require_accessor(gltf, skin.inverseBindMatrices.value(), "glTF inverseBindMatrices");
            if (accessor.type != fastgltf::AccessorType::Mat4 ||
                accessor.componentType != fastgltf::ComponentType::Float) {
                out.diagnostic = "glTF inverseBindMatrices accessor must be float32 MAT4";
                return out;
            }
            if (accessor.count != joint_count) {
                out.diagnostic = "glTF inverseBindMatrices accessor count must match skin joint count";
                return out;
            }
            require_accessor_buffer_view(gltf, accessor, "glTF inverseBindMatrices");
            std::size_t written = 0;
            for (const auto& matrix : fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(gltf, accessor)) {
                inverse_bind_matrices[written] = mat4_from_fastgltf_column_major(matrix);
                ++written;
            }
            if (written != joint_count) {
                out.diagnostic = "glTF inverseBindMatrices iteration count mismatch";
                return out;
            }
        }

        out.skin_payload.vertex_count = static_cast<std::uint32_t>(position_accessor.count);
        out.skin_payload.joints.clear();
        out.skin_payload.joints.reserve(joint_count);
        for (std::size_t joint_index = 0; joint_index < joint_count; ++joint_index) {
            out.skin_payload.joints.push_back(AnimationSkinJointDesc{
                .skeleton_joint_index = joint_index, .inverse_bind_matrix = inverse_bind_matrices[joint_index]});
        }

        out.skin_payload.vertices.clear();
        out.skin_payload.vertices.reserve(static_cast<std::size_t>(position_accessor.count));

        std::vector<std::array<std::uint32_t, 4>> joint_rows;
        joint_rows.reserve(static_cast<std::size_t>(position_accessor.count));
        if (joints_accessor.componentType == fastgltf::ComponentType::UnsignedByte) {
            for (const auto& joints : fastgltf::iterateAccessor<fastgltf::math::u8vec4>(gltf, joints_accessor)) {
                joint_rows.push_back({static_cast<std::uint32_t>(joints[0]), static_cast<std::uint32_t>(joints[1]),
                                      static_cast<std::uint32_t>(joints[2]), static_cast<std::uint32_t>(joints[3])});
            }
        } else if (joints_accessor.componentType == fastgltf::ComponentType::UnsignedShort) {
            for (const auto& joints : fastgltf::iterateAccessor<fastgltf::math::u16vec4>(gltf, joints_accessor)) {
                joint_rows.push_back({static_cast<std::uint32_t>(joints[0]), static_cast<std::uint32_t>(joints[1]),
                                      static_cast<std::uint32_t>(joints[2]), static_cast<std::uint32_t>(joints[3])});
            }
        } else if (joints_accessor.componentType == fastgltf::ComponentType::UnsignedInt) {
            for (const auto& joints : fastgltf::iterateAccessor<fastgltf::math::u32vec4>(gltf, joints_accessor)) {
                joint_rows.push_back({joints[0], joints[1], joints[2], joints[3]});
            }
        } else {
            out.diagnostic = "glTF JOINTS_0 component type must be UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT";
            return out;
        }

        std::vector<fastgltf::math::fvec4> weight_rows;
        weight_rows.reserve(static_cast<std::size_t>(position_accessor.count));
        for (const auto& weights : fastgltf::iterateAccessor<fastgltf::math::fvec4>(gltf, weights_accessor)) {
            weight_rows.push_back(weights);
        }

        if (joint_rows.size() != weight_rows.size()) {
            out.diagnostic = "glTF JOINTS_0 and WEIGHTS_0 iterators produced different vertex counts";
            return out;
        }

        for (std::size_t vertex_index = 0; vertex_index < joint_rows.size(); ++vertex_index) {
            const auto& joints = joint_rows[vertex_index];
            const auto& weights = weight_rows[vertex_index];
            AnimationSkinVertexWeights vertex_weights;
            std::string infl_diag;
            append_vertex_influences_from_indices_and_weights(joints[0], joints[1], joints[2], joints[3], weights[0],
                                                              weights[1], weights[2], weights[3], joint_count,
                                                              vertex_weights, infl_diag);
            if (!infl_diag.empty()) {
                out.diagnostic = std::move(infl_diag);
                return out;
            }
            if (vertex_weights.influences.empty()) {
                out.diagnostic = "glTF skinned vertex has no positive joint weights";
                return out;
            }
            out.skin_payload.vertices.push_back(std::move(vertex_weights));
        }

        const auto skin_diagnostics = validate_animation_skin_payload(out.skeleton, out.skin_payload);
        if (!skin_diagnostics.empty()) {
            out.diagnostic = skin_diagnostics.front().message;
            return out;
        }

        out.skin_payload = normalize_animation_skin_payload(out.skeleton, out.skin_payload);
        out.succeeded = true;
        return out;
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }
#endif
}

GltfAnimationJointTracksImportReport
import_gltf_animation_joint_tracks_for_skin(const std::string_view document_bytes_utf8,
                                            const std::string_view source_path_for_external_buffers,
                                            const std::size_t skin_index, const std::size_t animation_index) {
    GltfAnimationJointTracksImportReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)skin_index;
    (void)animation_index;
    out.diagnostic = "asset importers are disabled for this MK_tools build";
    return out;
#else
    auto expected = load_gltf_asset(document_bytes_utf8, source_path_for_external_buffers, out.diagnostic);
    if (expected.error() != fastgltf::Error::None) {
        return out;
    }
    const auto& gltf = expected.get();

    if (skin_index >= gltf.skins.size()) {
        out.diagnostic = "glTF skin index is out of range";
        return out;
    }
    if (animation_index >= gltf.animations.size()) {
        out.diagnostic = "glTF animation index is out of range";
        return out;
    }

    const auto& skin = gltf.skins[skin_index];
    std::unordered_map<std::size_t, std::size_t> node_to_skeleton_joint;
    node_to_skeleton_joint.reserve(skin.joints.size());
    for (std::size_t joint_index = 0; joint_index < skin.joints.size(); ++joint_index) {
        node_to_skeleton_joint[skin.joints[joint_index]] = joint_index;
    }

    try {
        std::unordered_map<std::size_t, AnimationJointTrackDesc> tracks_by_joint;

        const auto& animation = gltf.animations[animation_index];
        for (const auto& channel : animation.channels) {
            if (!channel.nodeIndex.has_value()) {
                continue;
            }
            const std::size_t node_index = channel.nodeIndex.value();
            const auto mapped = node_to_skeleton_joint.find(node_index);
            if (mapped == node_to_skeleton_joint.end()) {
                continue;
            }
            const std::size_t skeleton_joint_index = mapped->second;

            if (channel.samplerIndex >= animation.samplers.size()) {
                out.diagnostic = "glTF animation channel references an unknown sampler";
                return out;
            }
            const auto& sampler = animation.samplers[channel.samplerIndex];
            if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear) {
                out.diagnostic = "glTF animation sampler interpolation must be LINEAR for engine import";
                return out;
            }

            const auto& input_accessor = require_accessor(gltf, sampler.inputAccessor, "glTF animation sampler input");
            const auto& output_accessor =
                require_accessor(gltf, sampler.outputAccessor, "glTF animation sampler output");
            if (input_accessor.type != fastgltf::AccessorType::Scalar ||
                input_accessor.componentType != fastgltf::ComponentType::Float) {
                out.diagnostic = "glTF animation sampler input must be float scalar time keys";
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

            auto& track = tracks_by_joint[skeleton_joint_index];
            track.joint_index = skeleton_joint_index;

            if (channel.path == fastgltf::AnimationPath::Translation) {
                if (!track.translation_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate translation channels for the same skin joint";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF translation animation output must be float32 VEC3";
                    return out;
                }
                if (output_accessor.count != times.size()) {
                    out.diagnostic = "glTF translation animation output count must match input time count";
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    track.translation_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
            } else if (channel.path == fastgltf::AnimationPath::Rotation) {
                if (!track.rotation_z_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate rotation channels for the same skin joint";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec4 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF rotation animation output must be float32 VEC4 quaternions";
                    return out;
                }
                if (output_accessor.count != times.size()) {
                    out.diagnostic = "glTF rotation animation output count must match input time count";
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec4>(gltf, output_accessor)) {
                    const float qx = value[0];
                    const float qy = value[1];
                    const float qz = value[2];
                    const float qw = value[3];
                    if (std::abs(qx) > gltf_joint_quaternion_xy_epsilon ||
                        std::abs(qy) > gltf_joint_quaternion_xy_epsilon) {
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
            } else if (channel.path == fastgltf::AnimationPath::Scale) {
                if (!track.scale_keyframes.empty()) {
                    out.diagnostic = "glTF animation declares duplicate scale channels for the same skin joint";
                    return out;
                }
                if (output_accessor.type != fastgltf::AccessorType::Vec3 ||
                    output_accessor.componentType != fastgltf::ComponentType::Float) {
                    out.diagnostic = "glTF scale animation output must be float32 VEC3";
                    return out;
                }
                if (output_accessor.count != times.size()) {
                    out.diagnostic = "glTF scale animation output count must match input time count";
                    return out;
                }
                std::size_t index = 0;
                for (const auto& value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, output_accessor)) {
                    if (value[0] <= 0.0F || value[1] <= 0.0F || value[2] <= 0.0F) {
                        out.diagnostic = "glTF scale animation output must be strictly positive component-wise";
                        return out;
                    }
                    track.scale_keyframes.push_back(
                        Vec3Keyframe{.time_seconds = times[index], .value = Vec3{value[0], value[1], value[2]}});
                    ++index;
                }
            } else {
                out.diagnostic =
                    "glTF animation channel path is unsupported for engine import (only translation, rotation, scale)";
                return out;
            }
        }

        out.joint_tracks.clear();
        out.joint_tracks.reserve(tracks_by_joint.size());
        std::vector<std::size_t> joint_indices;
        joint_indices.reserve(tracks_by_joint.size());
        for (const auto& entry : tracks_by_joint) {
            joint_indices.push_back(entry.first);
        }
        std::sort(joint_indices.begin(), joint_indices.end());
        for (const std::size_t joint_index : joint_indices) {
            out.joint_tracks.push_back(std::move(tracks_by_joint[joint_index]));
        }

        AnimationSkeletonDesc skeleton_for_tracks;
        if (!build_skeleton_desc_from_gltf_skin(gltf, skin_index, skeleton_for_tracks, out.diagnostic)) {
            return out;
        }

        if (out.joint_tracks.empty()) {
            out.diagnostic = "glTF animation contains no channels targeting the selected skin joints";
            return out;
        }

        if (!is_valid_animation_joint_tracks(skeleton_for_tracks, out.joint_tracks)) {
            out.diagnostic = "imported glTF animation joint tracks failed engine validation";
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

} // namespace mirakana
