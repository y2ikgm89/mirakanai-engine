// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gltf_skin_animation_inspect.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

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

/// fastgltf stores glTF column-major MAT4; `mirakana::Mat4` uses row/column indices `at(row,col)` matching the same
/// mathematical matrix entries as glTF (M * column-vector convention).
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

#endif

} // namespace

GltfSkinAnimationInspectReport inspect_gltf_skin_animation(const std::string_view document_bytes_utf8,
                                                           const std::string_view source_path_for_external_buffers) {
    GltfSkinAnimationInspectReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    out.parse_succeeded = false;
    out.diagnostic = "asset importers are disabled for this MK_tools build";
    return out;
#else
    if (document_bytes_utf8.empty()) {
        out.diagnostic = "glTF document bytes are empty";
        return out;
    }
    const auto* data = reinterpret_cast<const std::byte*>(document_bytes_utf8.data());
    auto buffer = fastgltf::GltfDataBuffer::FromBytes(data, document_bytes_utf8.size());
    if (buffer.error() != fastgltf::Error::None) {
        out.diagnostic = "failed to create glTF buffer: " + fastgltf_error_text(buffer.error());
        return out;
    }

    auto source_directory = std::filesystem::path{std::string(source_path_for_external_buffers)}.parent_path();
    std::error_code directory_error;
    if (source_directory.empty() || !std::filesystem::exists(source_directory, directory_error)) {
        source_directory = std::filesystem::current_path();
    }

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(buffer.get(), source_directory, fastgltf::Options::LoadExternalBuffers);
    if (asset.error() != fastgltf::Error::None) {
        out.diagnostic = "failed to parse glTF: " + fastgltf_error_text(asset.error());
        return out;
    }
    const auto& gltf = asset.get();

    out.skins.reserve(gltf.skins.size());
    for (std::size_t skin_index = 0; skin_index < gltf.skins.size(); ++skin_index) {
        const auto& skin = gltf.skins[skin_index];
        GltfSkinInspectSummary summary;
        summary.skin_index = skin_index;
        summary.name = skin.name.empty() ? ("Skin " + std::to_string(skin_index)) : std::string(skin.name);
        summary.joint_count = skin.joints.size();
        summary.has_inverse_bind_matrices_accessor = skin.inverseBindMatrices.has_value();
        out.skins.push_back(std::move(summary));
    }

    out.animations.reserve(gltf.animations.size());
    for (std::size_t animation_index = 0; animation_index < gltf.animations.size(); ++animation_index) {
        const auto& animation = gltf.animations[animation_index];
        GltfAnimationInspectSummary summary;
        summary.animation_index = animation_index;
        summary.name =
            animation.name.empty() ? ("Animation " + std::to_string(animation_index)) : std::string(animation.name);
        summary.channel_count = animation.channels.size();
        out.animations.push_back(std::move(summary));
    }

    for (const auto& mesh : gltf.meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                continue;
            }
            const auto joints = primitive.findAttribute("JOINTS_0");
            const auto weights = primitive.findAttribute("WEIGHTS_0");
            if (joints != primitive.attributes.end() && weights != primitive.attributes.end()) {
                ++out.skinned_triangle_primitive_count;
            } else if (joints != primitive.attributes.end() || weights != primitive.attributes.end()) {
                out.warnings.push_back("triangle primitive declares only one of JOINTS_0 and WEIGHTS_0 (invalid glTF)");
            }
        }
    }

    out.parse_succeeded = true;
    return out;
#endif
}

GltfSkinBindExtractReport extract_gltf_skin_bind_data(const std::string_view document_bytes_utf8,
                                                      const std::string_view source_path_for_external_buffers,
                                                      const std::size_t skin_index) {
    GltfSkinBindExtractReport out;
#if !MK_HAS_ASSET_IMPORTERS
    (void)document_bytes_utf8;
    (void)source_path_for_external_buffers;
    (void)skin_index;
    out.diagnostic = "asset importers are disabled for this MK_tools build";
    return out;
#else
    if (document_bytes_utf8.empty()) {
        out.diagnostic = "glTF document bytes are empty";
        return out;
    }
    const auto* data = reinterpret_cast<const std::byte*>(document_bytes_utf8.data());
    auto buffer = fastgltf::GltfDataBuffer::FromBytes(data, document_bytes_utf8.size());
    if (buffer.error() != fastgltf::Error::None) {
        out.diagnostic = "failed to create glTF buffer: " + fastgltf_error_text(buffer.error());
        return out;
    }

    auto source_directory = std::filesystem::path{std::string(source_path_for_external_buffers)}.parent_path();
    std::error_code directory_error;
    if (source_directory.empty() || !std::filesystem::exists(source_directory, directory_error)) {
        source_directory = std::filesystem::current_path();
    }

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(buffer.get(), source_directory, fastgltf::Options::LoadExternalBuffers);
    if (asset.error() != fastgltf::Error::None) {
        out.diagnostic = "failed to parse glTF: " + fastgltf_error_text(asset.error());
        return out;
    }
    const auto& gltf = asset.get();

    if (skin_index >= gltf.skins.size()) {
        out.diagnostic = "glTF skin index is out of range";
        return out;
    }
    const auto& skin = gltf.skins[skin_index];

    try {
        out.joint_node_indices.reserve(skin.joints.size());
        for (const std::size_t node_index : skin.joints) {
            if (node_index >= gltf.nodes.size()) {
                out.diagnostic = "glTF skin joint references an unknown node index";
                return out;
            }
            if (node_index > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
                out.diagnostic = "glTF skin joint node index exceeds uint32 range";
                return out;
            }
            out.joint_node_indices.push_back(static_cast<std::uint32_t>(node_index));
        }

        const std::size_t joint_count = out.joint_node_indices.size();
        out.inverse_bind_matrices.assign(joint_count, Mat4::identity());

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
                out.inverse_bind_matrices[written] = mat4_from_fastgltf_column_major(matrix);
                ++written;
            }
            if (written != joint_count) {
                out.diagnostic = "glTF inverseBindMatrices iteration count mismatch";
                return out;
            }
        }
    } catch (const std::exception& ex) {
        out.diagnostic = ex.what();
        return out;
    }

    out.succeeded = true;
    return out;
#endif
}

} // namespace mirakana
