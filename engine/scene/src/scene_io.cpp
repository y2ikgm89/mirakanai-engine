// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/scene.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mirakana {
namespace {

struct PendingNode {
    std::string name;
    std::uint32_t parent{0};
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 scale{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    Vec3 rotation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    SceneNodeComponents components;
    ScenePrefabSourceLink prefab_source;
    bool has_name{false};
    bool has_parent{false};
    bool has_position{false};
    bool has_scale{false};
    bool has_rotation{false};
    bool has_prefab_source_prefab_name{false};
    bool has_prefab_source_prefab_path{false};
    bool has_prefab_source_source_node_index{false};
    bool has_prefab_source_source_node_name{false};
};

void validate_text_field(std::string_view value, const char* name) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(name) + " must not be empty");
    }
    if (value.find_first_of("\r\n=") != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must not contain newlines or '='");
    }
}

void validate_optional_text_field(std::string_view value, const char* name) {
    if (value.find_first_of("\r\n=") != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must not contain newlines or '='");
    }
}

[[nodiscard]] bool has_any_prefab_source_field(const PendingNode& node) noexcept {
    return node.has_prefab_source_prefab_name || node.has_prefab_source_prefab_path ||
           node.has_prefab_source_source_node_index || node.has_prefab_source_source_node_name;
}

std::uint32_t parse_u32(std::string_view value, const char* name) {
    try {
        std::size_t parsed = 0;
        const auto number = std::stoul(std::string(value), &parsed, 10);
        if (parsed != value.size() || number > UINT32_MAX) {
            throw std::invalid_argument("invalid uint");
        }
        return static_cast<std::uint32_t>(number);
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(name) + " must be an unsigned integer");
    }
}

std::uint64_t parse_u64(std::string_view value, const char* name) {
    try {
        std::size_t parsed = 0;
        const auto number = std::stoull(std::string(value), &parsed, 10);
        if (parsed != value.size()) {
            throw std::invalid_argument("invalid uint64");
        }
        return static_cast<std::uint64_t>(number);
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(name) + " must be an unsigned integer");
    }
}

float parse_float(std::string_view value, const char* name) {
    try {
        std::size_t parsed = 0;
        const auto number = std::stof(std::string(value), &parsed);
        if (parsed != value.size()) {
            throw std::invalid_argument("invalid float");
        }
        return number;
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(name) + " must be a float");
    }
}

Vec3 parse_vec3(std::string_view value, const char* name) {
    const auto first = value.find(',');
    const auto second = first == std::string_view::npos ? std::string_view::npos : value.find(',', first + 1);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        value.find(',', second + 1) != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must use x,y,z");
    }

    return Vec3{
        .x = parse_float(value.substr(0, first), name),
        .y = parse_float(value.substr(first + 1, second - first - 1), name),
        .z = parse_float(value.substr(second + 1), name),
    };
}

Vec2 parse_vec2(std::string_view value, const char* name) {
    const auto separator = value.find(',');
    if (separator == std::string_view::npos || value.find(',', separator + 1) != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must use x,y");
    }

    return Vec2{
        .x = parse_float(value.substr(0, separator), name),
        .y = parse_float(value.substr(separator + 1), name),
    };
}

std::array<float, 4> parse_rgba(std::string_view value, const char* name) {
    const auto first = value.find(',');
    const auto second = first == std::string_view::npos ? std::string_view::npos : value.find(',', first + 1);
    const auto third = second == std::string_view::npos ? std::string_view::npos : value.find(',', second + 1);
    if (first == std::string_view::npos || second == std::string_view::npos || third == std::string_view::npos ||
        value.find(',', third + 1) != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must use r,g,b,a");
    }

    return std::array<float, 4>{
        parse_float(value.substr(0, first), name),
        parse_float(value.substr(first + 1, second - first - 1), name),
        parse_float(value.substr(second + 1, third - second - 1), name),
        parse_float(value.substr(third + 1), name),
    };
}

bool parse_bool(std::string_view value, const char* name) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string(name) + " must be true or false");
}

std::string_view camera_projection_name(CameraProjectionMode projection) noexcept {
    switch (projection) {
    case CameraProjectionMode::perspective:
        return "perspective";
    case CameraProjectionMode::orthographic:
        return "orthographic";
    case CameraProjectionMode::unknown:
        break;
    }
    return "unknown";
}

CameraProjectionMode parse_camera_projection(std::string_view value) {
    if (value == "perspective") {
        return CameraProjectionMode::perspective;
    }
    if (value == "orthographic") {
        return CameraProjectionMode::orthographic;
    }
    throw std::invalid_argument("camera projection is unsupported");
}

std::string_view light_type_name(LightType type) noexcept {
    switch (type) {
    case LightType::directional:
        return "directional";
    case LightType::point:
        return "point";
    case LightType::spot:
        return "spot";
    case LightType::unknown:
        break;
    }
    return "unknown";
}

LightType parse_light_type(std::string_view value) {
    if (value == "directional") {
        return LightType::directional;
    }
    if (value == "point") {
        return LightType::point;
    }
    if (value == "spot") {
        return LightType::spot;
    }
    throw std::invalid_argument("light type is unsupported");
}

CameraComponent& ensure_camera(SceneNodeComponents& components) {
    if (!components.camera.has_value()) {
        components.camera = CameraComponent{};
    }
    return *components.camera;
}

LightComponent& ensure_light(SceneNodeComponents& components) {
    if (!components.light.has_value()) {
        components.light = LightComponent{};
    }
    return *components.light;
}

MeshRendererComponent& ensure_mesh_renderer(SceneNodeComponents& components) {
    if (!components.mesh_renderer.has_value()) {
        components.mesh_renderer = MeshRendererComponent{};
    }
    return *components.mesh_renderer;
}

SpriteRendererComponent& ensure_sprite_renderer(SceneNodeComponents& components) {
    if (!components.sprite_renderer.has_value()) {
        components.sprite_renderer = SpriteRendererComponent{};
    }
    return *components.sprite_renderer;
}

void assign_camera_field(SceneNodeComponents& components, std::string_view field, std::string_view value) {
    auto& camera = ensure_camera(components);
    if (field == "projection") {
        camera.projection = parse_camera_projection(value);
    } else if (field == "vertical_fov_radians") {
        camera.vertical_fov_radians = parse_float(value, "camera vertical fov");
    } else if (field == "orthographic_height") {
        camera.orthographic_height = parse_float(value, "camera orthographic height");
    } else if (field == "near") {
        camera.near_plane = parse_float(value, "camera near plane");
    } else if (field == "far") {
        camera.far_plane = parse_float(value, "camera far plane");
    } else if (field == "primary") {
        camera.primary = parse_bool(value, "camera primary");
    } else {
        throw std::invalid_argument("unknown scene camera field");
    }
}

void assign_light_field(SceneNodeComponents& components, std::string_view field, std::string_view value) {
    auto& light = ensure_light(components);
    if (field == "type") {
        light.type = parse_light_type(value);
    } else if (field == "color") {
        light.color = parse_vec3(value, "light color");
    } else if (field == "intensity") {
        light.intensity = parse_float(value, "light intensity");
    } else if (field == "range") {
        light.range = parse_float(value, "light range");
    } else if (field == "inner_cone_radians") {
        light.inner_cone_radians = parse_float(value, "light inner cone");
    } else if (field == "outer_cone_radians") {
        light.outer_cone_radians = parse_float(value, "light outer cone");
    } else if (field == "casts_shadows") {
        light.casts_shadows = parse_bool(value, "light casts shadows");
    } else {
        throw std::invalid_argument("unknown scene light field");
    }
}

void assign_mesh_renderer_field(SceneNodeComponents& components, std::string_view field, std::string_view value) {
    auto& renderer = ensure_mesh_renderer(components);
    if (field == "mesh") {
        renderer.mesh = AssetId{parse_u64(value, "mesh renderer mesh")};
    } else if (field == "material") {
        renderer.material = AssetId{parse_u64(value, "mesh renderer material")};
    } else if (field == "visible") {
        renderer.visible = parse_bool(value, "mesh renderer visible");
    } else {
        throw std::invalid_argument("unknown scene mesh renderer field");
    }
}

void assign_sprite_renderer_field(SceneNodeComponents& components, std::string_view field, std::string_view value) {
    auto& renderer = ensure_sprite_renderer(components);
    if (field == "sprite") {
        renderer.sprite = AssetId{parse_u64(value, "sprite renderer sprite")};
    } else if (field == "material") {
        renderer.material = AssetId{parse_u64(value, "sprite renderer material")};
    } else if (field == "size") {
        renderer.size = parse_vec2(value, "sprite renderer size");
    } else if (field == "tint") {
        renderer.tint = parse_rgba(value, "sprite renderer tint");
    } else if (field == "visible") {
        renderer.visible = parse_bool(value, "sprite renderer visible");
    } else {
        throw std::invalid_argument("unknown scene sprite renderer field");
    }
}

void assign_prefab_source_field(PendingNode& node, std::string_view field, std::string_view value) {
    if (field == "prefab_name") {
        validate_text_field(value, "prefab source prefab name");
        node.prefab_source.prefab_name = std::string(value);
        node.has_prefab_source_prefab_name = true;
    } else if (field == "prefab_path") {
        validate_optional_text_field(value, "prefab source prefab path");
        node.prefab_source.prefab_path = std::string(value);
        node.has_prefab_source_prefab_path = true;
    } else if (field == "source_node_index") {
        node.prefab_source.source_node_index = parse_u32(value, "prefab source node index");
        node.has_prefab_source_source_node_index = true;
    } else if (field == "source_node_name") {
        validate_text_field(value, "prefab source node name");
        node.prefab_source.source_node_name = std::string(value);
        node.has_prefab_source_source_node_name = true;
    } else {
        throw std::invalid_argument("unknown scene prefab source field");
    }
}

void assign_node_field(PendingNode& node, std::string_view field, std::string_view value) {
    if (field == "name") {
        validate_text_field(value, "scene node name");
        node.name = std::string(value);
        node.has_name = true;
    } else if (field == "parent") {
        node.parent = parse_u32(value, "scene node parent");
        node.has_parent = true;
    } else if (field == "position") {
        node.position = parse_vec3(value, "scene node position");
        node.has_position = true;
    } else if (field == "scale") {
        node.scale = parse_vec3(value, "scene node scale");
        node.has_scale = true;
    } else if (field == "rotation") {
        node.rotation = parse_vec3(value, "scene node rotation");
        node.has_rotation = true;
    } else if (field.starts_with("camera.")) {
        assign_camera_field(node.components, field.substr(7), value);
    } else if (field.starts_with("light.")) {
        assign_light_field(node.components, field.substr(6), value);
    } else if (field.starts_with("mesh_renderer.")) {
        assign_mesh_renderer_field(node.components, field.substr(14), value);
    } else if (field.starts_with("sprite_renderer.")) {
        assign_sprite_renderer_field(node.components, field.substr(16), value);
    } else if (field.starts_with("prefab_source.")) {
        assign_prefab_source_field(node, field.substr(14), value);
    } else {
        throw std::invalid_argument("unknown scene node field");
    }
}

void assign_scene_key(std::string_view key, std::string_view value, bool& has_format, bool& has_name, bool& has_count,
                      std::string& scene_name, std::vector<PendingNode>& nodes) {
    if (key == "format") {
        if (value != "GameEngine.Scene.v1") {
            throw std::invalid_argument("unsupported scene format");
        }
        has_format = true;
        return;
    }
    if (key == "scene.name") {
        validate_text_field(value, "scene name");
        scene_name = std::string(value);
        has_name = true;
        return;
    }
    if (key == "node.count") {
        if (has_count) {
            throw std::invalid_argument("scene node count is duplicated");
        }
        nodes.resize(parse_u32(value, "scene node count"));
        has_count = true;
        return;
    }
    if (!key.starts_with("node.")) {
        throw std::invalid_argument("unknown scene key");
    }
    if (!has_count) {
        throw std::invalid_argument("scene node count must appear before node fields");
    }

    const auto after_prefix = key.substr(5);
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("scene node key must include a field");
    }

    const auto index = parse_u32(after_prefix.substr(0, separator), "scene node index");
    if (index == 0 || index > nodes.size()) {
        throw std::invalid_argument("scene node index is out of range");
    }

    assign_node_field(nodes[index - 1], after_prefix.substr(separator + 1), value);
}

void validate_pending_node(const PendingNode& node, std::size_t index, std::size_t count) {
    if (!node.has_name || !node.has_parent || !node.has_position || !node.has_scale || !node.has_rotation) {
        throw std::invalid_argument("scene node is incomplete");
    }
    if (node.parent > count) {
        throw std::invalid_argument("scene node parent is out of range");
    }
    if (node.parent == index + 1) {
        throw std::invalid_argument("scene node cannot parent itself");
    }
    if (!is_valid_scene_node_components(node.components)) {
        throw std::invalid_argument("scene node components are invalid");
    }
    if (has_any_prefab_source_field(node)) {
        if (!node.has_prefab_source_prefab_name || !node.has_prefab_source_source_node_index ||
            !node.has_prefab_source_source_node_name || !is_valid_scene_prefab_source_link(node.prefab_source)) {
            throw std::invalid_argument("scene prefab source link is incomplete");
        }
    }
}

void write_camera(std::ostringstream& output, SceneNodeId node, const CameraComponent& camera) {
    output << "node." << node.value << ".camera.projection=" << camera_projection_name(camera.projection) << '\n';
    output << "node." << node.value << ".camera.primary=" << (camera.primary ? "true" : "false") << '\n';
    output << "node." << node.value << ".camera.vertical_fov_radians=" << camera.vertical_fov_radians << '\n';
    output << "node." << node.value << ".camera.orthographic_height=" << camera.orthographic_height << '\n';
    output << "node." << node.value << ".camera.near=" << camera.near_plane << '\n';
    output << "node." << node.value << ".camera.far=" << camera.far_plane << '\n';
}

void write_light(std::ostringstream& output, SceneNodeId node, const LightComponent& light) {
    output << "node." << node.value << ".light.type=" << light_type_name(light.type) << '\n';
    output << "node." << node.value << ".light.color=" << light.color.x << ',' << light.color.y << ',' << light.color.z
           << '\n';
    output << "node." << node.value << ".light.intensity=" << light.intensity << '\n';
    output << "node." << node.value << ".light.range=" << light.range << '\n';
    output << "node." << node.value << ".light.inner_cone_radians=" << light.inner_cone_radians << '\n';
    output << "node." << node.value << ".light.outer_cone_radians=" << light.outer_cone_radians << '\n';
    output << "node." << node.value << ".light.casts_shadows=" << (light.casts_shadows ? "true" : "false") << '\n';
}

void write_mesh_renderer(std::ostringstream& output, SceneNodeId node, const MeshRendererComponent& renderer) {
    output << "node." << node.value << ".mesh_renderer.mesh=" << renderer.mesh.value << '\n';
    output << "node." << node.value << ".mesh_renderer.material=" << renderer.material.value << '\n';
    output << "node." << node.value << ".mesh_renderer.visible=" << (renderer.visible ? "true" : "false") << '\n';
}

void write_sprite_renderer(std::ostringstream& output, SceneNodeId node, const SpriteRendererComponent& renderer) {
    output << "node." << node.value << ".sprite_renderer.sprite=" << renderer.sprite.value << '\n';
    output << "node." << node.value << ".sprite_renderer.material=" << renderer.material.value << '\n';
    output << "node." << node.value << ".sprite_renderer.size=" << renderer.size.x << ',' << renderer.size.y << '\n';
    output << "node." << node.value << ".sprite_renderer.tint=" << renderer.tint[0] << ',' << renderer.tint[1] << ','
           << renderer.tint[2] << ',' << renderer.tint[3] << '\n';
    output << "node." << node.value << ".sprite_renderer.visible=" << (renderer.visible ? "true" : "false") << '\n';
}

void write_prefab_source(std::ostringstream& output, SceneNodeId node, const ScenePrefabSourceLink& link) {
    output << "node." << node.value << ".prefab_source.prefab_name=" << link.prefab_name << '\n';
    if (!link.prefab_path.empty()) {
        output << "node." << node.value << ".prefab_source.prefab_path=" << link.prefab_path << '\n';
    }
    output << "node." << node.value << ".prefab_source.source_node_index=" << link.source_node_index << '\n';
    output << "node." << node.value << ".prefab_source.source_node_name=" << link.source_node_name << '\n';
}

} // namespace

std::string serialize_scene(const Scene& scene) {
    validate_text_field(scene.name(), "scene name");

    std::ostringstream output;
    output << "format=GameEngine.Scene.v1\n";
    output << "scene.name=" << scene.name() << '\n';
    output << "node.count=" << scene.nodes().size() << '\n';

    for (const auto& node : scene.nodes()) {
        validate_text_field(node.name, "scene node name");
        if (!is_valid_scene_node_components(node.components)) {
            throw std::invalid_argument("scene node components are invalid");
        }
        if (node.prefab_source.has_value()) {
            if (!is_valid_scene_prefab_source_link(*node.prefab_source)) {
                throw std::invalid_argument("scene prefab source link is invalid");
            }
            validate_text_field(node.prefab_source->prefab_name, "prefab source prefab name");
            validate_optional_text_field(node.prefab_source->prefab_path, "prefab source prefab path");
            validate_text_field(node.prefab_source->source_node_name, "prefab source node name");
        }
        output << "node." << node.id.value << ".name=" << node.name << '\n';
        output << "node." << node.id.value << ".parent=" << node.parent.value << '\n';
        output << "node." << node.id.value << ".position=" << node.transform.position.x << ','
               << node.transform.position.y << ',' << node.transform.position.z << '\n';
        output << "node." << node.id.value << ".scale=" << node.transform.scale.x << ',' << node.transform.scale.y
               << ',' << node.transform.scale.z << '\n';
        output << "node." << node.id.value << ".rotation=" << node.transform.rotation_radians.x << ','
               << node.transform.rotation_radians.y << ',' << node.transform.rotation_radians.z << '\n';
        if (node.components.camera.has_value()) {
            write_camera(output, node.id, *node.components.camera);
        }
        if (node.components.light.has_value()) {
            write_light(output, node.id, *node.components.light);
        }
        if (node.components.mesh_renderer.has_value()) {
            write_mesh_renderer(output, node.id, *node.components.mesh_renderer);
        }
        if (node.components.sprite_renderer.has_value()) {
            write_sprite_renderer(output, node.id, *node.components.sprite_renderer);
        }
        if (node.prefab_source.has_value()) {
            write_prefab_source(output, node.id, *node.prefab_source);
        }
    }

    return output.str();
}

Scene deserialize_scene(std::string_view text) {
    bool has_format = false;
    bool has_name = false;
    bool has_count = false;
    std::string scene_name;
    std::vector<PendingNode> nodes;

    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("scene line must contain '='");
        }

        assign_scene_key(std::string_view(line).substr(0, separator), std::string_view(line).substr(separator + 1),
                         has_format, has_name, has_count, scene_name, nodes);
    }

    if (!has_format || !has_name || !has_count) {
        throw std::invalid_argument("scene format, name, or node count is missing");
    }

    Scene scene(scene_name);
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        validate_pending_node(nodes[index], index, nodes.size());
        const auto id = scene.create_node(nodes[index].name);
        auto* node = scene.find_node(id);
        node->transform.position = nodes[index].position;
        node->transform.scale = nodes[index].scale;
        node->transform.rotation_radians = nodes[index].rotation;
        scene.set_components(id, nodes[index].components);
        if (has_any_prefab_source_field(nodes[index])) {
            node->prefab_source = nodes[index].prefab_source;
        }
    }

    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (nodes[index].parent == 0) {
            continue;
        }
        scene.set_parent(SceneNodeId{static_cast<std::uint32_t>(index + 1)}, SceneNodeId{nodes[index].parent});
    }

    return scene;
}

} // namespace mirakana
