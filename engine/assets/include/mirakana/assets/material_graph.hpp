// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class MaterialGraphNodeKind {
    unknown,
    graph_output,
    constant_vec4,
    constant_vec3,
    constant_scalar,
    texture,
};

struct MaterialGraphNode {
    std::string id;
    MaterialGraphNodeKind kind{MaterialGraphNodeKind::unknown};
    std::array<float, 4> vec4{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> vec3{0.0F, 0.0F, 0.0F};
    float scalar_value{0.0F};
    std::string scalar_key;
    MaterialTextureSlot texture_slot{MaterialTextureSlot::unknown};
    AssetId texture_id{};
};

struct MaterialGraphEdge {
    std::string from_node;
    std::string from_socket;
    std::string to_node;
    std::string to_socket;
};

struct MaterialGraphDesc {
    AssetId material_id{};
    std::string material_name;
    MaterialShadingModel shading_model{MaterialShadingModel::lit};
    MaterialSurfaceMode surface_mode{MaterialSurfaceMode::opaque};
    bool double_sided{false};
    std::string output_node_id;
    std::vector<MaterialGraphNode> nodes;
    std::vector<MaterialGraphEdge> edges;
};

[[nodiscard]] bool operator==(const MaterialGraphNode& lhs, const MaterialGraphNode& rhs) noexcept;
[[nodiscard]] bool operator==(const MaterialGraphEdge& lhs, const MaterialGraphEdge& rhs) noexcept;
[[nodiscard]] bool operator==(const MaterialGraphDesc& lhs, const MaterialGraphDesc& rhs) noexcept;

enum class MaterialGraphDiagnosticCode {
    unknown,
    invalid_format,
    duplicate_key,
    missing_key,
    duplicate_node_id,
    missing_output_node,
    invalid_output_target,
    unknown_node_kind,
    invalid_edge_endpoint,
    duplicate_output_feed,
    lowering_type_mismatch,
    invalid_texture_binding,
    invalid_factor_range,
};

struct MaterialGraphDiagnostic {
    MaterialGraphDiagnosticCode code{MaterialGraphDiagnosticCode::unknown};
    std::string field;
    std::string message;
};

[[nodiscard]] std::string serialize_material_graph(const MaterialGraphDesc& graph);
[[nodiscard]] MaterialGraphDesc deserialize_material_graph(std::string_view text);
[[nodiscard]] std::vector<MaterialGraphDiagnostic> validate_material_graph(const MaterialGraphDesc& graph);
[[nodiscard]] MaterialDefinition lower_material_graph_to_definition(const MaterialGraphDesc& graph);

} // namespace mirakana
