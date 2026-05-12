// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/// One triangle mesh primitive inside a glTF asset (editor / diagnostics; not a cook contract).
struct GltfMeshPrimitiveInspectRow {
    std::string mesh_name;
    std::size_t mesh_index{0};
    std::size_t primitive_index{0};
    std::size_t position_vertex_count{0};
    bool has_position{false};
    bool has_normal{false};
    bool has_texcoord0{false};
    bool indexed{false};
    bool triangles{false};
};

/// Read-only summary of glTF mesh primitives for editor UX and tooling (no cooked writes).
struct GltfMeshInspectReport {
    bool parse_succeeded{false};
    std::string diagnostic;
    std::vector<std::string> warnings;
    std::vector<GltfMeshPrimitiveInspectRow> rows;
};

/// Parses UTF-8 `.gltf` JSON (or GLB bytes) using the same external-buffer policy as mesh import.
/// When `MK_ENABLE_ASSET_IMPORTERS` is off, returns `parse_succeeded == false` with a stable diagnostic.
[[nodiscard]] GltfMeshInspectReport inspect_gltf_mesh_primitives(std::string_view document_bytes_utf8,
                                                                 std::string_view source_path_for_external_buffers);

} // namespace mirakana
