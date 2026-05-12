// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gltf_mesh_inspect.hpp"

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#ifndef MK_HAS_ASSET_IMPORTERS
#define MK_HAS_ASSET_IMPORTERS 0
#endif

#if MK_HAS_ASSET_IMPORTERS
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#endif

namespace mirakana {
namespace {

#if MK_HAS_ASSET_IMPORTERS

[[nodiscard]] std::string fastgltf_error_text(fastgltf::Error error) {
    return std::string(fastgltf::getErrorMessage(error));
}

[[nodiscard]] const fastgltf::Accessor& require_accessor(const fastgltf::Asset& asset, std::size_t accessor_index,
                                                         std::string_view context) {
    if (accessor_index >= asset.accessors.size()) {
        throw std::runtime_error(std::string(context) + " references an unknown accessor");
    }
    return asset.accessors[accessor_index];
}

#endif

} // namespace

GltfMeshInspectReport inspect_gltf_mesh_primitives(const std::string_view document_bytes_utf8,
                                                   const std::string_view source_path_for_external_buffers) {
    GltfMeshInspectReport out;
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

    for (std::size_t mesh_index = 0; mesh_index < gltf.meshes.size(); ++mesh_index) {
        const auto& mesh = gltf.meshes[mesh_index];
        const std::string mesh_name =
            mesh.name.empty() ? ("Mesh " + std::to_string(mesh_index)) : std::string(mesh.name);

        for (std::size_t primitive_index = 0; primitive_index < mesh.primitives.size(); ++primitive_index) {
            const auto& primitive = mesh.primitives[primitive_index];
            GltfMeshPrimitiveInspectRow row;
            row.mesh_name = mesh_name;
            row.mesh_index = mesh_index;
            row.primitive_index = primitive_index;
            row.triangles = primitive.type == fastgltf::PrimitiveType::Triangles;
            row.indexed = primitive.indicesAccessor.has_value();

            if (!row.triangles) {
                out.warnings.push_back(mesh_name + " primitive " + std::to_string(primitive_index) +
                                       " skipped (non-triangle mode)");
                continue;
            }

            const auto position = primitive.findAttribute("POSITION");
            if (position == primitive.attributes.end()) {
                out.warnings.push_back(mesh_name + " primitive " + std::to_string(primitive_index) +
                                       " skipped (missing POSITION)");
                continue;
            }

            try {
                const auto& position_accessor =
                    require_accessor(gltf, position->accessorIndex, "glTF POSITION for inspect");
                row.has_position = true;
                row.position_vertex_count = position_accessor.count;
            } catch (const std::exception& ex) {
                out.warnings.push_back(mesh_name + " primitive " + std::to_string(primitive_index) +
                                       " POSITION inspect failed: " + ex.what());
                continue;
            }

            const auto normal = primitive.findAttribute("NORMAL");
            const auto uv = primitive.findAttribute("TEXCOORD_0");
            row.has_normal = normal != primitive.attributes.end();
            row.has_texcoord0 = uv != primitive.attributes.end();
            if (row.has_normal != row.has_texcoord0) {
                out.warnings.push_back(mesh_name + " primitive " + std::to_string(primitive_index) +
                                       " has mismatched NORMAL vs TEXCOORD_0 presence (import may reject)");
            }

            out.rows.push_back(std::move(row));
        }
    }

    out.parse_succeeded = true;
    return out;
#endif
}

} // namespace mirakana
