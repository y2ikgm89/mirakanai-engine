// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/material_graph.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace mirakana {
namespace {

constexpr std::string_view graph_format = "GameEngine.MaterialGraph.v1";

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_shading_model(MaterialShadingModel model) noexcept {
    switch (model) {
    case MaterialShadingModel::unlit:
    case MaterialShadingModel::lit:
        return true;
    case MaterialShadingModel::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_surface_mode(MaterialSurfaceMode mode) noexcept {
    switch (mode) {
    case MaterialSurfaceMode::opaque:
    case MaterialSurfaceMode::masked:
    case MaterialSurfaceMode::transparent:
        return true;
    case MaterialSurfaceMode::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool color_factor(float value) noexcept {
    return value >= 0.0F && value <= 1.0F;
}

[[nodiscard]] bool finite_non_negative(float value) noexcept {
    return value >= 0.0F && value <= 1024.0F;
}

[[nodiscard]] bool valid_texture_slot(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
    case MaterialTextureSlot::normal:
    case MaterialTextureSlot::metallic_roughness:
    case MaterialTextureSlot::emissive:
    case MaterialTextureSlot::occlusion:
        return true;
    case MaterialTextureSlot::unknown:
        break;
    }
    return false;
}

[[nodiscard]] std::string_view shading_model_name(MaterialShadingModel model) noexcept {
    switch (model) {
    case MaterialShadingModel::unlit:
        return "unlit";
    case MaterialShadingModel::lit:
        return "lit";
    case MaterialShadingModel::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] MaterialShadingModel parse_shading_model(std::string_view value) {
    if (value == "unlit") {
        return MaterialShadingModel::unlit;
    }
    if (value == "lit") {
        return MaterialShadingModel::lit;
    }
    throw std::invalid_argument("unsupported material graph shading model");
}

[[nodiscard]] std::string_view surface_mode_name(MaterialSurfaceMode mode) noexcept {
    switch (mode) {
    case MaterialSurfaceMode::opaque:
        return "opaque";
    case MaterialSurfaceMode::masked:
        return "masked";
    case MaterialSurfaceMode::transparent:
        return "transparent";
    case MaterialSurfaceMode::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] MaterialSurfaceMode parse_surface_mode(std::string_view value) {
    if (value == "opaque") {
        return MaterialSurfaceMode::opaque;
    }
    if (value == "masked") {
        return MaterialSurfaceMode::masked;
    }
    if (value == "transparent") {
        return MaterialSurfaceMode::transparent;
    }
    throw std::invalid_argument("unsupported material graph surface mode");
}

[[nodiscard]] std::string_view texture_slot_name(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return "base_color";
    case MaterialTextureSlot::normal:
        return "normal";
    case MaterialTextureSlot::metallic_roughness:
        return "metallic_roughness";
    case MaterialTextureSlot::emissive:
        return "emissive";
    case MaterialTextureSlot::occlusion:
        return "occlusion";
    case MaterialTextureSlot::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] MaterialTextureSlot parse_texture_slot(std::string_view value) {
    if (value == "base_color") {
        return MaterialTextureSlot::base_color;
    }
    if (value == "normal") {
        return MaterialTextureSlot::normal;
    }
    if (value == "metallic_roughness") {
        return MaterialTextureSlot::metallic_roughness;
    }
    if (value == "emissive") {
        return MaterialTextureSlot::emissive;
    }
    if (value == "occlusion") {
        return MaterialTextureSlot::occlusion;
    }
    throw std::invalid_argument("unsupported material graph texture slot");
}

[[nodiscard]] std::string float_text(float value) {
    std::string text(32, '\0');
    const auto [end, error] = std::to_chars(std::to_address(text.begin()), std::to_address(text.end()), value);
    if (error != std::errc{}) {
        throw std::invalid_argument("material graph float value could not be serialized");
    }
    text.resize(static_cast<std::size_t>(end - text.data()));
    return text;
}

[[nodiscard]] bool material_float_character(char value) noexcept {
    return (value >= '0' && value <= '9') || value == '+' || value == '-' || value == '.' || value == 'e' ||
           value == 'E';
}

[[nodiscard]] float parse_float(std::string_view value) {
    if (value.empty() || std::ranges::any_of(value, [](char c) { return !material_float_character(c); })) {
        throw std::invalid_argument("material graph float value is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument("material graph float value is invalid");
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
    throw std::invalid_argument("material graph bool value is invalid");
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_unique_key_lines(std::string_view text) {
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
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("material graph line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string(raw_line.substr(0, separator)), std::string(raw_line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument("material graph contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("material graph is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("material graph integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_count(const std::unordered_map<std::string, std::string>& values,
                                      const std::string& key) {
    return static_cast<std::size_t>(parse_u64(required_value(values, key)));
}

[[nodiscard]] std::string node_prefix(std::size_t index) {
    return "nodes." + std::to_string(index) + ".";
}

[[nodiscard]] std::string edge_prefix(std::size_t index) {
    return "edges." + std::to_string(index) + ".";
}

[[nodiscard]] std::string_view node_kind_name(MaterialGraphNodeKind kind) noexcept {
    switch (kind) {
    case MaterialGraphNodeKind::graph_output:
        return "graph_output";
    case MaterialGraphNodeKind::constant_vec4:
        return "constant_vec4";
    case MaterialGraphNodeKind::constant_vec3:
        return "constant_vec3";
    case MaterialGraphNodeKind::constant_scalar:
        return "constant_scalar";
    case MaterialGraphNodeKind::texture:
        return "texture";
    case MaterialGraphNodeKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] MaterialGraphNodeKind parse_node_kind(std::string_view value) {
    if (value == "graph_output") {
        return MaterialGraphNodeKind::graph_output;
    }
    if (value == "constant_vec4") {
        return MaterialGraphNodeKind::constant_vec4;
    }
    if (value == "constant_vec3") {
        return MaterialGraphNodeKind::constant_vec3;
    }
    if (value == "constant_scalar") {
        return MaterialGraphNodeKind::constant_scalar;
    }
    if (value == "texture") {
        return MaterialGraphNodeKind::texture;
    }
    return MaterialGraphNodeKind::unknown;
}

void write_vec4(std::ostringstream& output, const std::string& prefix, const std::array<float, 4>& v) {
    output << prefix << "vec4=" << float_text(v[0]) << ',' << float_text(v[1]) << ',' << float_text(v[2]) << ','
           << float_text(v[3]) << '\n';
}

void write_vec3(std::ostringstream& output, const std::string& prefix, const std::array<float, 3>& v) {
    output << prefix << "vec3=" << float_text(v[0]) << ',' << float_text(v[1]) << ',' << float_text(v[2]) << '\n';
}

[[nodiscard]] std::array<float, 4> parse_vec4(std::string_view text) {
    const auto first = text.find(',');
    const auto second = text.find(',', first == std::string_view::npos ? first : first + 1U);
    const auto third = text.find(',', second == std::string_view::npos ? second : second + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos || third == std::string_view::npos) {
        throw std::invalid_argument("material graph vec4 is invalid");
    }
    return {
        parse_float(text.substr(0, first)),
        parse_float(text.substr(first + 1U, second - first - 1U)),
        parse_float(text.substr(second + 1U, third - second - 1U)),
        parse_float(text.substr(third + 1U)),
    };
}

[[nodiscard]] std::array<float, 3> parse_vec3(std::string_view text) {
    const auto first = text.find(',');
    const auto second = text.find(',', first == std::string_view::npos ? first : first + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos) {
        throw std::invalid_argument("material graph vec3 is invalid");
    }
    return {
        parse_float(text.substr(0, first)),
        parse_float(text.substr(first + 1U, second - first - 1U)),
        parse_float(text.substr(second + 1U)),
    };
}

[[nodiscard]] MaterialGraphNode parse_node(const std::unordered_map<std::string, std::string>& values,
                                           std::size_t index) {
    const auto prefix = node_prefix(index);
    MaterialGraphNode node;
    node.id = required_value(values, prefix + "id");
    node.kind = parse_node_kind(required_value(values, prefix + "kind"));
    if (node.kind == MaterialGraphNodeKind::constant_vec4) {
        node.vec4 = parse_vec4(required_value(values, prefix + "vec4"));
    }
    if (node.kind == MaterialGraphNodeKind::constant_vec3) {
        node.vec3 = parse_vec3(required_value(values, prefix + "vec3"));
    }
    if (node.kind == MaterialGraphNodeKind::constant_scalar) {
        node.scalar_key = required_value(values, prefix + "scalar");
        node.scalar_value = parse_float(required_value(values, prefix + "scalar_value"));
    }
    if (node.kind == MaterialGraphNodeKind::texture) {
        node.texture_slot = parse_texture_slot(required_value(values, prefix + "texture_slot"));
        node.texture_id = AssetId{parse_u64(required_value(values, prefix + "texture_id"))};
    }
    return node;
}

[[nodiscard]] MaterialGraphEdge parse_edge(const std::unordered_map<std::string, std::string>& values,
                                           std::size_t index) {
    const auto prefix = edge_prefix(index);
    MaterialGraphEdge edge;
    edge.from_node = required_value(values, prefix + "from_node");
    edge.from_socket = required_value(values, prefix + "from_socket");
    edge.to_node = required_value(values, prefix + "to_node");
    edge.to_socket = required_value(values, prefix + "to_socket");
    return edge;
}

void write_node(std::ostringstream& output, std::size_t index, const MaterialGraphNode& node) {
    const auto prefix = node_prefix(index);
    output << prefix << "id=" << node.id << '\n';
    output << prefix << "kind=" << node_kind_name(node.kind) << '\n';
    if (node.kind == MaterialGraphNodeKind::constant_vec4) {
        write_vec4(output, prefix, node.vec4);
    }
    if (node.kind == MaterialGraphNodeKind::constant_vec3) {
        write_vec3(output, prefix, node.vec3);
    }
    if (node.kind == MaterialGraphNodeKind::constant_scalar) {
        output << prefix << "scalar=" << node.scalar_key << '\n';
        output << prefix << "scalar_value=" << float_text(node.scalar_value) << '\n';
    }
    if (node.kind == MaterialGraphNodeKind::texture) {
        output << prefix << "texture_slot=" << texture_slot_name(node.texture_slot) << '\n';
        output << prefix << "texture_id=" << node.texture_id.value << '\n';
    }
}

void write_edge(std::ostringstream& output, std::size_t index, const MaterialGraphEdge& edge) {
    const auto prefix = edge_prefix(index);
    output << prefix << "from_node=" << edge.from_node << '\n';
    output << prefix << "from_socket=" << edge.from_socket << '\n';
    output << prefix << "to_node=" << edge.to_node << '\n';
    output << prefix << "to_socket=" << edge.to_socket << '\n';
}

[[nodiscard]] const MaterialGraphNode* find_node(const MaterialGraphDesc& graph, std::string_view id) noexcept {
    for (const auto& node : graph.nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

[[nodiscard]] bool is_factor_socket(std::string_view socket) noexcept {
    return socket == "factor.base_color" || socket == "factor.emissive" || socket == "factor.metallic" ||
           socket == "factor.roughness";
}

[[nodiscard]] bool is_texture_socket(std::string_view socket) noexcept {
    return socket.starts_with("texture.");
}

[[nodiscard]] std::optional<MaterialTextureSlot> texture_slot_from_socket(std::string_view socket) {
    constexpr std::string_view prefix = "texture.";
    if (!socket.starts_with(prefix)) {
        return std::nullopt;
    }
    const auto rest = socket.substr(prefix.size());
    try {
        return parse_texture_slot(rest);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

[[nodiscard]] std::vector<MaterialTextureBinding> sorted_bindings(std::vector<MaterialTextureBinding> bindings) {
    std::ranges::sort(bindings, [](const MaterialTextureBinding& lhs, const MaterialTextureBinding& rhs) {
        return static_cast<int>(lhs.slot) < static_cast<int>(rhs.slot);
    });
    return bindings;
}

} // namespace

bool operator==(const MaterialGraphNode& lhs, const MaterialGraphNode& rhs) noexcept {
    return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.vec4 == rhs.vec4 && lhs.vec3 == rhs.vec3 &&
           lhs.scalar_value == rhs.scalar_value && lhs.scalar_key == rhs.scalar_key &&
           lhs.texture_slot == rhs.texture_slot && lhs.texture_id.value == rhs.texture_id.value;
}

bool operator==(const MaterialGraphEdge& lhs, const MaterialGraphEdge& rhs) noexcept {
    return lhs.from_node == rhs.from_node && lhs.from_socket == rhs.from_socket && lhs.to_node == rhs.to_node &&
           lhs.to_socket == rhs.to_socket;
}

bool operator==(const MaterialGraphDesc& lhs, const MaterialGraphDesc& rhs) noexcept {
    return lhs.material_id.value == rhs.material_id.value && lhs.material_name == rhs.material_name &&
           lhs.shading_model == rhs.shading_model && lhs.surface_mode == rhs.surface_mode &&
           lhs.double_sided == rhs.double_sided && lhs.output_node_id == rhs.output_node_id && lhs.nodes == rhs.nodes &&
           lhs.edges == rhs.edges;
}

std::string serialize_material_graph(const MaterialGraphDesc& graph) {
    const auto diagnostics = validate_material_graph(graph);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph is invalid for serialization");
    }

    std::ostringstream output;
    output << "format=" << graph_format << '\n';
    output << "material.id=" << graph.material_id.value << '\n';
    output << "material.name=" << graph.material_name << '\n';
    output << "material.shading=" << shading_model_name(graph.shading_model) << '\n';
    output << "material.surface=" << surface_mode_name(graph.surface_mode) << '\n';
    output << "material.double_sided=" << (graph.double_sided ? "true" : "false") << '\n';
    output << "graph.output_node=" << graph.output_node_id << '\n';
    output << "nodes.count=" << graph.nodes.size() << '\n';
    for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
        write_node(output, index, graph.nodes[index]);
    }
    output << "edges.count=" << graph.edges.size() << '\n';
    for (std::size_t index = 0; index < graph.edges.size(); ++index) {
        write_edge(output, index, graph.edges[index]);
    }
    return output.str();
}

MaterialGraphDesc deserialize_material_graph(std::string_view text) {
    const auto values = parse_unique_key_lines(text);
    if (required_value(values, "format") != graph_format) {
        throw std::invalid_argument("material graph format is unsupported");
    }

    MaterialGraphDesc graph;
    graph.material_id = AssetId{parse_u64(required_value(values, "material.id"))};
    graph.material_name = required_value(values, "material.name");
    graph.shading_model = parse_shading_model(required_value(values, "material.shading"));
    graph.surface_mode = parse_surface_mode(required_value(values, "material.surface"));
    graph.double_sided = parse_bool(required_value(values, "material.double_sided"));
    graph.output_node_id = required_value(values, "graph.output_node");

    const auto node_count = parse_count(values, "nodes.count");
    graph.nodes.reserve(node_count);
    for (std::size_t index = 0; index < node_count; ++index) {
        graph.nodes.push_back(parse_node(values, index));
    }

    const auto edge_count = parse_count(values, "edges.count");
    graph.edges.reserve(edge_count);
    for (std::size_t index = 0; index < edge_count; ++index) {
        graph.edges.push_back(parse_edge(values, index));
    }

    const auto diagnostics = validate_material_graph(graph);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph definition is invalid");
    }
    return graph;
}

std::vector<MaterialGraphDiagnostic> validate_material_graph(const MaterialGraphDesc& graph) {
    std::vector<MaterialGraphDiagnostic> diagnostics;

    if (graph.material_id.value == 0) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::missing_key,
                               .field = "material.id",
                               .message = "material id must be non-zero"});
    }
    if (!valid_token(graph.material_name)) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_format,
                               .field = "material.name",
                               .message = "material name token is invalid"});
    }
    if (!valid_shading_model(graph.shading_model)) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_format,
                               .field = "material.shading",
                               .message = "shading model is invalid"});
    }
    if (!valid_surface_mode(graph.surface_mode)) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_format,
                               .field = "material.surface",
                               .message = "surface mode is invalid"});
    }
    if (graph.output_node_id.empty() || !valid_token(graph.output_node_id)) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::missing_output_node,
                               .field = "graph.output_node",
                               .message = "graph output node id is missing or invalid"});
    }

    std::unordered_set<std::string> node_ids;
    for (const auto& node : graph.nodes) {
        if (node.id.empty() || !valid_token(node.id)) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_format,
                                   .field = "nodes.*.id",
                                   .message = "node id is missing or invalid"});
            continue;
        }
        if (!node_ids.insert(node.id).second) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::duplicate_node_id,
                                   .field = node.id,
                                   .message = "duplicate node id"});
        }
        if (node.kind == MaterialGraphNodeKind::unknown) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::unknown_node_kind,
                                   .field = node.id,
                                   .message = "node kind is unknown"});
        }
        if (node.kind == MaterialGraphNodeKind::constant_vec4) {
            if (!color_factor(node.vec4[0]) || !color_factor(node.vec4[1]) || !color_factor(node.vec4[2]) ||
                !color_factor(node.vec4[3])) {
                diagnostics.push_back(
                    {.code = MaterialGraphDiagnosticCode::invalid_factor_range,
                     .field = node.id,
                     .message = "constant_vec4 components must stay within the base-color factor range"});
            }
        }
        if (node.kind == MaterialGraphNodeKind::constant_vec3) {
            if (!finite_non_negative(node.vec3[0]) || !finite_non_negative(node.vec3[1]) ||
                !finite_non_negative(node.vec3[2])) {
                diagnostics.push_back(
                    {.code = MaterialGraphDiagnosticCode::invalid_factor_range,
                     .field = node.id,
                     .message = "constant_vec3 components must stay within the emissive factor range"});
            }
        }
        if (node.kind == MaterialGraphNodeKind::constant_scalar) {
            if (node.scalar_key != "metallic" && node.scalar_key != "roughness") {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_factor_range,
                                       .field = node.id,
                                       .message = "scalar node must target metallic or roughness"});
            }
            if (!std::isfinite(node.scalar_value)) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_factor_range,
                                       .field = node.id,
                                       .message = "scalar value is not finite"});
            } else if (!color_factor(node.scalar_value)) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_factor_range,
                                       .field = node.id,
                                       .message = "scalar factors must stay within the 0..1 range"});
            }
        }
        if (node.kind == MaterialGraphNodeKind::texture) {
            if (!valid_texture_slot(node.texture_slot)) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_texture_binding,
                                       .field = node.id,
                                       .message = "texture slot is invalid"});
            }
            if (node.texture_id.value == 0) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_texture_binding,
                                       .field = node.id,
                                       .message = "texture id must be non-zero for texture nodes"});
            }
        }
    }

    const auto* output_node = find_node(graph, graph.output_node_id);
    if (output_node == nullptr) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::missing_output_node,
                               .field = "graph.output_node",
                               .message = "graph output node id does not reference a node"});
    } else if (output_node->kind != MaterialGraphNodeKind::graph_output) {
        diagnostics.push_back({.code = MaterialGraphDiagnosticCode::missing_output_node,
                               .field = "graph.output_node",
                               .message = "graph output node must use kind graph_output"});
    }

    std::unordered_set<std::string> fed_sockets;
    for (const auto& edge : graph.edges) {
        if (edge.from_socket != "out") {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_edge_endpoint,
                                   .field = "edges.*.from_socket",
                                   .message = "only the out socket is supported on the source side"});
        }
        if (find_node(graph, edge.from_node) == nullptr) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_edge_endpoint,
                                   .field = "edges.*.from_node",
                                   .message = "edge references unknown from_node"});
        }
        if (edge.to_node != graph.output_node_id) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_output_target,
                                   .field = "edges.*.to_node",
                                   .message = "edges must terminate on the graph output node in v1"});
        }
        if (!is_factor_socket(edge.to_socket) && !is_texture_socket(edge.to_socket)) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_edge_endpoint,
                                   .field = "edges.*.to_socket",
                                   .message = "unsupported output socket name"});
        }
        if (const auto slot = texture_slot_from_socket(edge.to_socket)) {
            if (!valid_texture_slot(*slot)) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_edge_endpoint,
                                       .field = "edges.*.to_socket",
                                       .message = "texture socket uses unknown slot"});
            }
        } else if (!is_factor_socket(edge.to_socket)) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_edge_endpoint,
                                   .field = "edges.*.to_socket",
                                   .message = "socket is neither factor nor texture.*"});
        }

        const auto fed_key = edge.to_node + "|" + edge.to_socket;
        if (!fed_sockets.insert(fed_key).second) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::duplicate_output_feed,
                                   .field = edge.to_socket,
                                   .message = "duplicate feed into the same output socket"});
        }

        const auto* from = find_node(graph, edge.from_node);
        if (from != nullptr) {
            if (edge.to_socket == "factor.base_color" && from->kind != MaterialGraphNodeKind::constant_vec4) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                       .field = edge.from_node,
                                       .message = "factor.base_color requires a constant_vec4 source"});
            }
            if (edge.to_socket == "factor.emissive" && from->kind != MaterialGraphNodeKind::constant_vec3) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                       .field = edge.from_node,
                                       .message = "factor.emissive requires a constant_vec3 source"});
            }
            if (edge.to_socket == "factor.metallic") {
                if (from->kind != MaterialGraphNodeKind::constant_scalar || from->scalar_key != "metallic") {
                    diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                           .field = edge.from_node,
                                           .message = "factor.metallic requires a constant_scalar metallic node"});
                }
            }
            if (edge.to_socket == "factor.roughness") {
                if (from->kind != MaterialGraphNodeKind::constant_scalar || from->scalar_key != "roughness") {
                    diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                           .field = edge.from_node,
                                           .message = "factor.roughness requires a constant_scalar roughness node"});
                }
            }
            if (is_texture_socket(edge.to_socket) && from->kind != MaterialGraphNodeKind::texture) {
                diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                       .field = edge.from_node,
                                       .message = "texture sockets require a texture node source"});
            }
            if (from->kind == MaterialGraphNodeKind::texture) {
                const auto expected = texture_slot_from_socket(edge.to_socket);
                if (!expected || from->texture_slot != *expected) {
                    diagnostics.push_back({.code = MaterialGraphDiagnosticCode::lowering_type_mismatch,
                                           .field = edge.from_node,
                                           .message = "texture node slot must match the output socket slot"});
                }
            }
        }
    }

    return diagnostics;
}

MaterialDefinition lower_material_graph_to_definition(const MaterialGraphDesc& graph) {
    const auto diagnostics = validate_material_graph(graph);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph cannot be lowered while validation diagnostics are present");
    }

    MaterialDefinition material;
    material.id = graph.material_id;
    material.name = graph.material_name;
    material.shading_model = graph.shading_model;
    material.surface_mode = graph.surface_mode;
    material.double_sided = graph.double_sided;

    for (const auto& edge : graph.edges) {
        const auto* from = find_node(graph, edge.from_node);
        if (from == nullptr) {
            continue;
        }
        if (edge.to_socket == "factor.base_color" && from->kind == MaterialGraphNodeKind::constant_vec4) {
            material.factors.base_color = from->vec4;
        } else if (edge.to_socket == "factor.emissive" && from->kind == MaterialGraphNodeKind::constant_vec3) {
            material.factors.emissive = from->vec3;
        } else if (edge.to_socket == "factor.metallic" && from->kind == MaterialGraphNodeKind::constant_scalar) {
            material.factors.metallic = from->scalar_value;
        } else if (edge.to_socket == "factor.roughness" && from->kind == MaterialGraphNodeKind::constant_scalar) {
            material.factors.roughness = from->scalar_value;
        } else if (from->kind == MaterialGraphNodeKind::texture) {
            if (const auto slot = texture_slot_from_socket(edge.to_socket)) {
                material.texture_bindings.push_back(MaterialTextureBinding{.slot = *slot, .texture = from->texture_id});
            }
        }
    }

    material.texture_bindings = sorted_bindings(std::move(material.texture_bindings));

    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("lowered material definition is invalid");
    }
    return material;
}

} // namespace mirakana
