// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene/prefab_overrides.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view variant_format = "GameEngine.PrefabVariant.v1";
constexpr std::string_view prefab_format = "GameEngine.Prefab.v1";

struct OverrideKey {
    std::uint32_t node_index{0};
    PrefabOverrideKind kind{PrefabOverrideKind::name};

    friend constexpr bool operator==(OverrideKey lhs, OverrideKey rhs) noexcept {
        return lhs.node_index == rhs.node_index && lhs.kind == rhs.kind;
    }
};

struct OverrideKeyHash {
    [[nodiscard]] std::size_t operator()(OverrideKey key) const noexcept {
        return (static_cast<std::size_t>(key.node_index) << 2U) ^
               static_cast<std::size_t>(static_cast<unsigned int>(key.kind));
    }
};

struct PendingVariantOverride {
    bool has_node{false};
    bool has_kind{false};
    bool has_name{false};
    bool has_position{false};
    bool has_scale{false};
    bool has_rotation{false};
    std::uint32_t node_index{0};
    PrefabOverrideKind kind{PrefabOverrideKind::name};
    std::string name;
    Transform3D transform;
    std::vector<std::pair<std::string, std::string>> component_fields;
    std::string source_node_name;
};

[[nodiscard]] bool valid_name(std::string_view name) noexcept {
    return !name.empty() && name.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool valid_override_kind(PrefabOverrideKind kind) noexcept {
    switch (kind) {
    case PrefabOverrideKind::name:
    case PrefabOverrideKind::transform:
    case PrefabOverrideKind::components:
        return true;
    }
    return false;
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool valid_transform(Transform3D transform) noexcept {
    return finite_vec3(transform.position) && finite_vec3(transform.rotation_radians) && finite_vec3(transform.scale) &&
           transform.scale.x > 0.0F && transform.scale.y > 0.0F && transform.scale.z > 0.0F;
}

void add_diagnostic(std::vector<PrefabVariantDiagnostic>& diagnostics, PrefabVariantDiagnosticKind kind,
                    std::uint32_t node_index, PrefabOverrideKind override_kind, std::string message) {
    diagnostics.push_back(PrefabVariantDiagnostic{
        .kind = kind,
        .node_index = node_index,
        .override_kind = override_kind,
        .message = std::move(message),
    });
}

[[nodiscard]] bool duplicate_already_reported(const std::unordered_set<OverrideKey, OverrideKeyHash>& reported,
                                              OverrideKey key) {
    return reported.find(key) != reported.end();
}

[[nodiscard]] std::string to_string(std::string_view value) {
    return std::string(value);
}

[[nodiscard]] bool parse_bool(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("boolean value is invalid");
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view label) {
    const auto text = to_string(value);
    std::size_t consumed = 0;
    const auto parsed = std::stof(text, &consumed);
    if (consumed != text.size() || !std::isfinite(parsed)) {
        throw std::invalid_argument(to_string(label) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view label) {
    if (value.empty() || value.front() == '-') {
        throw std::invalid_argument(to_string(label) + " is invalid");
    }

    const auto text = to_string(value);
    std::size_t consumed = 0;
    const auto parsed = std::stoull(text, &consumed, 10);
    if (consumed != text.size() || parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(to_string(label) + " is invalid");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] Vec3 parse_vec3(std::string_view value, std::string_view label) {
    const auto first = value.find(',');
    const auto second = first == std::string_view::npos ? std::string_view::npos : value.find(',', first + 1U);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        value.find(',', second + 1U) != std::string_view::npos) {
        throw std::invalid_argument(to_string(label) + " must contain three comma separated values");
    }

    return Vec3{
        .x = parse_float(value.substr(0, first), label),
        .y = parse_float(value.substr(first + 1U, second - first - 1U), label),
        .z = parse_float(value.substr(second + 1U), label),
    };
}

[[nodiscard]] PrefabOverrideKind parse_override_kind(std::string_view value) {
    if (value == prefab_override_kind_label(PrefabOverrideKind::name)) {
        return PrefabOverrideKind::name;
    }
    if (value == prefab_override_kind_label(PrefabOverrideKind::transform)) {
        return PrefabOverrideKind::transform;
    }
    if (value == prefab_override_kind_label(PrefabOverrideKind::components)) {
        return PrefabOverrideKind::components;
    }
    throw std::invalid_argument("prefab override kind is unsupported");
}

[[nodiscard]] std::string transform_value(Vec3 value) {
    std::ostringstream output;
    output << value.x << ',' << value.y << ',' << value.z;
    return output.str();
}

[[nodiscard]] std::vector<std::string> split_lines(std::string_view text) {
    std::vector<std::string> lines;
    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

[[nodiscard]] std::string indent_prefab_definition(const PrefabDefinition& prefab) {
    std::ostringstream output;
    for (const auto& line : split_lines(serialize_prefab_definition(prefab))) {
        output << "base." << line << '\n';
    }
    return output.str();
}

[[nodiscard]] std::string extract_base_prefab_text(const std::vector<std::pair<std::string, std::string>>& fields) {
    std::ostringstream output;
    for (const auto& [key, value] : fields) {
        if (key.starts_with("base.")) {
            output << key.substr(5) << '=' << value << '\n';
        }
    }
    return output.str();
}

[[nodiscard]] std::string components_to_prefab_text(const SceneNodeComponents& components) {
    PrefabDefinition prefab;
    prefab.name = "components.prefab";
    PrefabNodeTemplate node;
    node.name = "Components";
    node.components = components;
    prefab.nodes.push_back(node);
    return serialize_prefab_definition(prefab);
}

[[nodiscard]] std::string serialize_component_override_fields(std::uint32_t override_index,
                                                              const SceneNodeComponents& components) {
    std::ostringstream output;
    constexpr std::string_view node_prefix = "node.1.";
    for (const auto& line : split_lines(components_to_prefab_text(components))) {
        if (!line.starts_with(node_prefix)) {
            continue;
        }

        const auto field = std::string_view(line).substr(node_prefix.size());
        if (!field.starts_with("camera.") && !field.starts_with("light.") && !field.starts_with("mesh_renderer.") &&
            !field.starts_with("sprite_renderer.")) {
            continue;
        }

        output << "override." << override_index << '.' << field << '\n';
    }
    return output.str();
}

[[nodiscard]] SceneNodeComponents
deserialize_component_override_fields(const std::vector<std::pair<std::string, std::string>>& component_fields) {
    std::ostringstream prefab_text;
    prefab_text << "format=" << prefab_format << '\n';
    prefab_text << "prefab.name=components.prefab\n";
    prefab_text << "node.count=1\n";
    prefab_text << "node.1.name=Components\n";
    prefab_text << "node.1.parent=0\n";
    prefab_text << "node.1.position=0,0,0\n";
    prefab_text << "node.1.scale=1,1,1\n";
    prefab_text << "node.1.rotation=0,0,0\n";

    for (const auto& [field, value] : component_fields) {
        prefab_text << "node.1." << field << '=' << value << '\n';
    }

    const auto prefab = deserialize_prefab_definition(prefab_text.str());
    return prefab.nodes.front().components;
}

void assign_override_field(PendingVariantOverride& pending, std::string_view field, std::string_view value) {
    if (field == "node") {
        pending.node_index = parse_u32(value, "prefab override node");
        pending.has_node = true;
    } else if (field == "kind") {
        pending.kind = parse_override_kind(value);
        pending.has_kind = true;
    } else if (field == "source_node_name") {
        pending.source_node_name = std::string(value);
    } else if (field == "name") {
        pending.name = std::string(value);
        pending.has_name = true;
    } else if (field == "position") {
        pending.transform.position = parse_vec3(value, "prefab override position");
        pending.has_position = true;
    } else if (field == "scale") {
        pending.transform.scale = parse_vec3(value, "prefab override scale");
        pending.has_scale = true;
    } else if (field == "rotation") {
        pending.transform.rotation_radians = parse_vec3(value, "prefab override rotation");
        pending.has_rotation = true;
    } else if (field.starts_with("camera.") || field.starts_with("light.") || field.starts_with("mesh_renderer.") ||
               field.starts_with("sprite_renderer.")) {
        pending.component_fields.emplace_back(std::string(field), std::string(value));
    } else {
        throw std::invalid_argument("unknown prefab variant override field");
    }
}

void assign_override_key(std::string_view key, std::string_view value, std::vector<PendingVariantOverride>& overrides) {
    constexpr std::string_view prefix = "override.";
    if (!key.starts_with(prefix)) {
        throw std::invalid_argument("unknown prefab variant key");
    }

    const auto after_prefix = key.substr(prefix.size());
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("prefab variant override key must include a field");
    }

    const auto index = parse_u32(after_prefix.substr(0, separator), "prefab override index");
    if (index == 0 || index > overrides.size()) {
        throw std::invalid_argument("prefab override index is out of range");
    }

    assign_override_field(overrides[index - 1U], after_prefix.substr(separator + 1U), value);
}

[[nodiscard]] bool has_transform_fields(const PendingVariantOverride& pending) noexcept {
    return pending.has_position || pending.has_scale || pending.has_rotation;
}

[[nodiscard]] PrefabNodeOverride finalize_pending_override(const PendingVariantOverride& pending) {
    if (!pending.has_node || !pending.has_kind) {
        throw std::invalid_argument("prefab variant override is incomplete");
    }

    PrefabNodeOverride override;
    override.node_index = pending.node_index;
    override.kind = pending.kind;
    override.source_node_name = pending.source_node_name;

    switch (pending.kind) {
    case PrefabOverrideKind::name:
        if (!pending.has_name || has_transform_fields(pending) || !pending.component_fields.empty()) {
            throw std::invalid_argument("prefab variant name override is incomplete");
        }
        override.name = pending.name;
        break;
    case PrefabOverrideKind::transform:
        if (!pending.has_position || !pending.has_scale || !pending.has_rotation || pending.has_name ||
            !pending.component_fields.empty()) {
            throw std::invalid_argument("prefab variant transform override is incomplete");
        }
        override.transform = pending.transform;
        break;
    case PrefabOverrideKind::components:
        if (pending.has_name || has_transform_fields(pending)) {
            throw std::invalid_argument("prefab variant component override fields are invalid");
        }
        override.components = deserialize_component_override_fields(pending.component_fields);
        break;
    }

    return override;
}

[[nodiscard]] PrefabVariantDefinition parse_prefab_variant_definition(std::string_view text) {
    bool has_format = false;
    bool has_name = false;
    bool has_override_count = false;
    std::string variant_name;
    std::vector<std::pair<std::string, std::string>> fields;
    std::vector<PendingVariantOverride> pending_overrides;

    std::istringstream input{std::string(text)};
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("prefab variant line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1U);
        fields.emplace_back(std::string(key), std::string(value));

        if (key == "format") {
            if (value != variant_format) {
                throw std::invalid_argument("unsupported prefab variant format");
            }
            has_format = true;
        } else if (key == "variant.name") {
            variant_name = std::string(value);
            has_name = true;
        } else if (key.starts_with("base.")) {
            continue;
        } else if (key == "override.count") {
            if (has_override_count) {
                throw std::invalid_argument("prefab variant override count is duplicated");
            }
            pending_overrides.resize(parse_u32(value, "prefab variant override count"));
            has_override_count = true;
        } else if (key.starts_with("override.")) {
            if (!has_override_count) {
                throw std::invalid_argument("prefab variant override count must appear before override fields");
            }
            assign_override_key(key, value, pending_overrides);
        } else {
            throw std::invalid_argument("unknown prefab variant key");
        }
    }

    if (!has_format || !has_name || !has_override_count) {
        throw std::invalid_argument("prefab variant format, name, or override count is missing");
    }

    PrefabVariantDefinition variant;
    variant.name = std::move(variant_name);
    variant.base_prefab = deserialize_prefab_definition(extract_base_prefab_text(fields));
    variant.overrides.reserve(pending_overrides.size());
    for (const auto& pending : pending_overrides) {
        variant.overrides.push_back(finalize_pending_override(pending));
    }

    return variant;
}

} // namespace

std::string_view prefab_override_kind_label(PrefabOverrideKind kind) noexcept {
    switch (kind) {
    case PrefabOverrideKind::name:
        return "name";
    case PrefabOverrideKind::transform:
        return "transform";
    case PrefabOverrideKind::components:
        return "components";
    }
    return "unknown";
}

std::string_view prefab_variant_diagnostic_kind_label(PrefabVariantDiagnosticKind kind) noexcept {
    switch (kind) {
    case PrefabVariantDiagnosticKind::invalid_base_prefab:
        return "invalid_base_prefab";
    case PrefabVariantDiagnosticKind::invalid_variant_name:
        return "invalid_variant_name";
    case PrefabVariantDiagnosticKind::invalid_node_index:
        return "invalid_node_index";
    case PrefabVariantDiagnosticKind::invalid_override_kind:
        return "invalid_override_kind";
    case PrefabVariantDiagnosticKind::duplicate_override:
        return "duplicate_override";
    case PrefabVariantDiagnosticKind::invalid_node_name:
        return "invalid_node_name";
    case PrefabVariantDiagnosticKind::invalid_source_node_name:
        return "invalid_source_node_name";
    case PrefabVariantDiagnosticKind::invalid_transform:
        return "invalid_transform";
    case PrefabVariantDiagnosticKind::invalid_components:
        return "invalid_components";
    }
    return "unknown";
}

std::vector<PrefabVariantDiagnostic> validate_prefab_variant_definition(const PrefabVariantDefinition& variant) {
    std::vector<PrefabVariantDiagnostic> diagnostics;

    if (!is_valid_prefab_definition(variant.base_prefab)) {
        add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_base_prefab, 0, PrefabOverrideKind::name,
                       "base prefab is invalid");
    }
    if (!valid_name(variant.name)) {
        add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_variant_name, 0, PrefabOverrideKind::name,
                       "variant name is invalid");
    }

    std::unordered_set<OverrideKey, OverrideKeyHash> seen_overrides;
    std::unordered_set<OverrideKey, OverrideKeyHash> reported_duplicates;

    for (const auto& override : variant.overrides) {
        const bool node_valid = override.node_index != 0 && override.node_index <= variant.base_prefab.nodes.size();
        if (!node_valid) {
            add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_node_index, override.node_index,
                           override.kind, "override node index is out of range");
        }
        if (!valid_override_kind(override.kind)) {
            add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_override_kind, override.node_index,
                           override.kind, "override kind is unsupported");
            continue;
        }
        if (!override.source_node_name.empty() && !valid_name(override.source_node_name)) {
            add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_source_node_name, override.node_index,
                           override.kind, "source node name hint is invalid");
        }

        const OverrideKey key{.node_index = override.node_index, .kind = override.kind};
        if (!seen_overrides.insert(key).second && !duplicate_already_reported(reported_duplicates, key)) {
            reported_duplicates.insert(key);
            add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::duplicate_override, override.node_index,
                           override.kind, "override duplicates an earlier node/kind pair");
        }

        switch (override.kind) {
        case PrefabOverrideKind::name:
            if (!valid_name(override.name)) {
                add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_node_name, override.node_index,
                               override.kind, "name override is invalid");
            }
            break;
        case PrefabOverrideKind::transform:
            if (!valid_transform(override.transform)) {
                add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_transform, override.node_index,
                               override.kind, "transform override is invalid");
            }
            break;
        case PrefabOverrideKind::components:
            if (!is_valid_scene_node_components(override.components)) {
                add_diagnostic(diagnostics, PrefabVariantDiagnosticKind::invalid_components, override.node_index,
                               override.kind, "component override is invalid");
            }
            break;
        }
    }

    return diagnostics;
}

bool is_valid_prefab_variant_definition(const PrefabVariantDefinition& variant) {
    return validate_prefab_variant_definition(variant).empty();
}

PrefabVariantComposeResult compose_prefab_variant(const PrefabVariantDefinition& variant) {
    PrefabVariantComposeResult result;
    result.prefab = variant.base_prefab;
    result.diagnostics = validate_prefab_variant_definition(variant);
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.prefab.name = variant.name;
    for (const auto& override : variant.overrides) {
        auto& node = result.prefab.nodes[override.node_index - 1U];
        switch (override.kind) {
        case PrefabOverrideKind::name:
            node.name = override.name;
            break;
        case PrefabOverrideKind::transform:
            node.transform = override.transform;
            break;
        case PrefabOverrideKind::components:
            node.components = override.components;
            break;
        }
    }

    result.success = true;
    return result;
}

std::string serialize_prefab_variant_definition(const PrefabVariantDefinition& variant) {
    const auto diagnostics = validate_prefab_variant_definition(variant);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("prefab variant definition is invalid");
    }

    std::ostringstream output;
    output << "format=" << variant_format << '\n';
    output << "variant.name=" << variant.name << '\n';
    output << indent_prefab_definition(variant.base_prefab);
    output << "override.count=" << variant.overrides.size() << '\n';

    for (std::size_t index = 0; index < variant.overrides.size(); ++index) {
        const auto ordinal = static_cast<std::uint32_t>(index + 1U);
        const auto& override = variant.overrides[index];
        output << "override." << ordinal << ".node=" << override.node_index << '\n';
        output << "override." << ordinal << ".kind=" << prefab_override_kind_label(override.kind) << '\n';
        if (!override.source_node_name.empty()) {
            output << "override." << ordinal << ".source_node_name=" << override.source_node_name << '\n';
        }

        switch (override.kind) {
        case PrefabOverrideKind::name:
            output << "override." << ordinal << ".name=" << override.name << '\n';
            break;
        case PrefabOverrideKind::transform:
            output << "override." << ordinal << ".position=" << transform_value(override.transform.position) << '\n';
            output << "override." << ordinal << ".scale=" << transform_value(override.transform.scale) << '\n';
            output << "override." << ordinal << ".rotation=" << transform_value(override.transform.rotation_radians)
                   << '\n';
            break;
        case PrefabOverrideKind::components:
            output << serialize_component_override_fields(ordinal, override.components);
            break;
        }
    }

    return output.str();
}

PrefabVariantDefinition deserialize_prefab_variant_definition(std::string_view text) {
    auto variant = parse_prefab_variant_definition(text);

    if (!is_valid_prefab_variant_definition(variant)) {
        throw std::invalid_argument("prefab variant definition is invalid");
    }

    return variant;
}

PrefabVariantDefinition deserialize_prefab_variant_definition_for_review(std::string_view text) {
    return parse_prefab_variant_definition(text);
}

} // namespace mirakana
