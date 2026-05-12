// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/material.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iterator>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mirakana {
namespace {

constexpr std::string_view material_format = "GameEngine.Material.v1";
constexpr std::string_view material_instance_format = "GameEngine.MaterialInstance.v1";

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

[[nodiscard]] bool valid_binding_resource_kind(MaterialBindingResourceKind kind) noexcept {
    switch (kind) {
    case MaterialBindingResourceKind::uniform_buffer:
    case MaterialBindingResourceKind::sampled_texture:
    case MaterialBindingResourceKind::sampler:
        return true;
    case MaterialBindingResourceKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool has_shader_stage(MaterialShaderStageMask stages) noexcept {
    return stages != MaterialShaderStageMask::none;
}

[[nodiscard]] bool finite_non_negative(float value) noexcept {
    return value >= 0.0F && value <= 1024.0F;
}

[[nodiscard]] bool color_factor(float value) noexcept {
    return value >= 0.0F && value <= 1.0F;
}

[[nodiscard]] bool valid_factors(const MaterialFactors& factors) noexcept {
    return color_factor(factors.base_color[0]) && color_factor(factors.base_color[1]) &&
           color_factor(factors.base_color[2]) && color_factor(factors.base_color[3]) &&
           finite_non_negative(factors.emissive[0]) && finite_non_negative(factors.emissive[1]) &&
           finite_non_negative(factors.emissive[2]) && color_factor(factors.metallic) &&
           color_factor(factors.roughness);
}

[[nodiscard]] std::vector<MaterialTextureBinding> sorted_bindings(std::vector<MaterialTextureBinding> bindings) {
    std::ranges::sort(bindings, [](const MaterialTextureBinding& lhs, const MaterialTextureBinding& rhs) {
        return static_cast<int>(lhs.slot) < static_cast<int>(rhs.slot);
    });
    return bindings;
}

[[nodiscard]] bool duplicate_slots(const std::vector<MaterialTextureBinding>& bindings) noexcept {
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        for (std::size_t other = index + 1U; other < bindings.size(); ++other) {
            if (bindings[index].slot == bindings[other].slot) {
                return true;
            }
        }
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
    throw std::invalid_argument("unsupported material shading model");
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
    throw std::invalid_argument("unsupported material surface mode");
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
    throw std::invalid_argument("unsupported material texture slot");
}

[[nodiscard]] std::uint32_t binding_for_texture_slot(MaterialTextureSlot slot) {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return 1;
    case MaterialTextureSlot::normal:
        return 2;
    case MaterialTextureSlot::metallic_roughness:
        return 3;
    case MaterialTextureSlot::emissive:
        return 4;
    case MaterialTextureSlot::occlusion:
        return 5;
    case MaterialTextureSlot::unknown:
        break;
    }
    throw std::invalid_argument("material texture slot has no descriptor binding");
}

[[nodiscard]] std::uint32_t sampler_binding_for_texture_slot(MaterialTextureSlot slot) {
    return binding_for_texture_slot(slot) + 15U;
}

[[nodiscard]] std::string semantic_for_texture_slot(MaterialTextureSlot slot) {
    return "texture." + std::string(texture_slot_name(slot));
}

[[nodiscard]] std::string sampler_semantic_for_texture_slot(MaterialTextureSlot slot) {
    return "sampler." + std::string(texture_slot_name(slot));
}

[[nodiscard]] std::string float_text(float value) {
    std::string text(32, '\0');
    const auto [end, error] = std::to_chars(std::to_address(text.begin()), std::to_address(text.end()), value);
    if (error != std::errc{}) {
        throw std::invalid_argument("material float value could not be serialized");
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
        throw std::invalid_argument("material float value is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument("material float value is invalid");
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
    throw std::invalid_argument("material bool value is invalid");
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_key_values(std::string_view text) {
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
            throw std::invalid_argument("material line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string(raw_line.substr(0, separator)), std::string(raw_line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument("material contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("material is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("material integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_count(const std::unordered_map<std::string, std::string>& values,
                                      const std::string& key) {
    return static_cast<std::size_t>(parse_u64(required_value(values, key)));
}

void write_factors(std::ostringstream& output, const MaterialFactors& factors) {
    output << "factor.base_color=" << float_text(factors.base_color[0]) << ',' << float_text(factors.base_color[1])
           << ',' << float_text(factors.base_color[2]) << ',' << float_text(factors.base_color[3]) << '\n';
    output << "factor.emissive=" << float_text(factors.emissive[0]) << ',' << float_text(factors.emissive[1]) << ','
           << float_text(factors.emissive[2]) << '\n';
    output << "factor.metallic=" << float_text(factors.metallic) << '\n';
    output << "factor.roughness=" << float_text(factors.roughness) << '\n';
}

[[nodiscard]] MaterialFactors parse_factors(const std::unordered_map<std::string, std::string>& values) {
    MaterialFactors factors;

    const auto base = required_value(values, "factor.base_color");
    const auto first = base.find(',');
    const auto second = base.find(',', first == std::string::npos ? first : first + 1U);
    const auto third = base.find(',', second == std::string::npos ? second : second + 1U);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos) {
        throw std::invalid_argument("material base color factor is invalid");
    }
    factors.base_color = {
        parse_float(base.substr(0, first)),
        parse_float(base.substr(first + 1U, second - first - 1U)),
        parse_float(base.substr(second + 1U, third - second - 1U)),
        parse_float(base.substr(third + 1U)),
    };

    const auto emissive = required_value(values, "factor.emissive");
    const auto emissive_first = emissive.find(',');
    const auto emissive_second =
        emissive.find(',', emissive_first == std::string::npos ? emissive_first : emissive_first + 1U);
    if (emissive_first == std::string::npos || emissive_second == std::string::npos) {
        throw std::invalid_argument("material emissive factor is invalid");
    }
    factors.emissive = {
        parse_float(emissive.substr(0, emissive_first)),
        parse_float(emissive.substr(emissive_first + 1U, emissive_second - emissive_first - 1U)),
        parse_float(emissive.substr(emissive_second + 1U)),
    };
    factors.metallic = parse_float(required_value(values, "factor.metallic"));
    factors.roughness = parse_float(required_value(values, "factor.roughness"));
    return factors;
}

[[nodiscard]] std::vector<MaterialTextureBinding>
parse_texture_bindings(const std::unordered_map<std::string, std::string>& values) {
    const auto texture_count = parse_count(values, "texture.count");
    std::vector<MaterialTextureBinding> bindings;
    bindings.reserve(texture_count);
    for (std::size_t index = 0; index < texture_count; ++index) {
        const auto ordinal = index + 1U;
        const auto prefix = std::string("texture.") + std::to_string(ordinal);
        bindings.push_back(MaterialTextureBinding{
            .slot = parse_texture_slot(required_value(values, prefix + ".slot")),
            .texture = AssetId{parse_u64(required_value(values, prefix + ".id"))},
        });
    }
    return sorted_bindings(std::move(bindings));
}

void write_texture_bindings(std::ostringstream& output, const std::vector<MaterialTextureBinding>& bindings) {
    output << "texture.count=" << bindings.size() << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        const auto ordinal = index + 1U;
        output << "texture." << ordinal << ".slot=" << texture_slot_name(bindings[index].slot) << '\n';
        output << "texture." << ordinal << ".id=" << bindings[index].texture.value << '\n';
    }
}

} // namespace

bool is_valid_material_texture_binding(const MaterialTextureBinding& binding) noexcept {
    return valid_texture_slot(binding.slot) && binding.texture.value != 0;
}

bool is_valid_material_definition(const MaterialDefinition& material) noexcept {
    if (material.id.value == 0 || !valid_token(material.name) || !valid_shading_model(material.shading_model) ||
        !valid_surface_mode(material.surface_mode) || !valid_factors(material.factors) ||
        duplicate_slots(material.texture_bindings)) {
        return false;
    }
    if (!std::ranges::all_of(material.texture_bindings,
                             [](const auto& binding) { return is_valid_material_texture_binding(binding); })) {
        return false;
    }
    return true;
}

bool is_valid_material_instance_definition(const MaterialInstanceDefinition& material) noexcept {
    if (material.id.value == 0 || material.parent.value == 0 || material.id == material.parent ||
        !valid_token(material.name) || duplicate_slots(material.texture_overrides)) {
        return false;
    }
    if (material.factor_overrides.has_value() && !valid_factors(*material.factor_overrides)) {
        return false;
    }
    return std::ranges::all_of(material.texture_overrides,
                               [](const auto& binding) { return is_valid_material_texture_binding(binding); });
}

bool is_valid_material_pipeline_binding(const MaterialPipelineBinding& binding) noexcept {
    if (!valid_binding_resource_kind(binding.resource_kind) || !has_shader_stage(binding.stages) ||
        !valid_token(binding.semantic)) {
        return false;
    }
    if ((binding.resource_kind == MaterialBindingResourceKind::sampled_texture ||
         binding.resource_kind == MaterialBindingResourceKind::sampler) &&
        !valid_texture_slot(binding.texture_slot)) {
        return false;
    }
    if (binding.resource_kind == MaterialBindingResourceKind::uniform_buffer &&
        binding.texture_slot != MaterialTextureSlot::unknown) {
        return false;
    }
    return true;
}

bool is_valid_material_pipeline_binding_metadata(const MaterialPipelineBindingMetadata& metadata) noexcept {
    if (metadata.material.value == 0 || !valid_shading_model(metadata.shading_model) ||
        !valid_surface_mode(metadata.surface_mode) || metadata.bindings.empty()) {
        return false;
    }
    for (std::size_t index = 0; index < metadata.bindings.size(); ++index) {
        const auto& binding = metadata.bindings[index];
        if (!is_valid_material_pipeline_binding(binding)) {
            return false;
        }
        for (std::size_t other = index + 1U; other < metadata.bindings.size(); ++other) {
            if (binding.set == metadata.bindings[other].set && binding.binding == metadata.bindings[other].binding) {
                return false;
            }
        }
    }
    return !(metadata.requires_alpha_test && metadata.requires_alpha_blending);
}

MaterialDefinition compose_material_instance(const MaterialDefinition& parent,
                                             const MaterialInstanceDefinition& instance) {
    if (!is_valid_material_definition(parent)) {
        throw std::invalid_argument("parent material definition is invalid");
    }
    if (!is_valid_material_instance_definition(instance)) {
        throw std::invalid_argument("material instance definition is invalid");
    }
    if (parent.id != instance.parent) {
        throw std::invalid_argument("material instance parent does not match parent material");
    }

    MaterialDefinition material = parent;
    material.id = instance.id;
    material.name = instance.name;
    if (instance.factor_overrides.has_value()) {
        material.factors = *instance.factor_overrides;
    }

    for (const auto& override_binding : instance.texture_overrides) {
        const auto existing =
            std::ranges::find_if(material.texture_bindings, [override_binding](const MaterialTextureBinding& binding) {
                return binding.slot == override_binding.slot;
            });
        if (existing == material.texture_bindings.end()) {
            material.texture_bindings.push_back(override_binding);
        } else {
            *existing = override_binding;
        }
    }
    material.texture_bindings = sorted_bindings(std::move(material.texture_bindings));

    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("composed material definition is invalid");
    }
    return material;
}

MaterialPipelineBindingMetadata build_material_pipeline_binding_metadata(const MaterialDefinition& material) {
    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("material definition is invalid for binding metadata");
    }

    MaterialPipelineBindingMetadata metadata{
        .material = material.id,
        .shading_model = material.shading_model,
        .surface_mode = material.surface_mode,
        .double_sided = material.double_sided,
        .requires_alpha_test = material.surface_mode == MaterialSurfaceMode::masked,
        .requires_alpha_blending = material.surface_mode == MaterialSurfaceMode::transparent,
        .bindings = {},
    };

    metadata.bindings.push_back(MaterialPipelineBinding{
        .set = 0,
        .binding = 0,
        .resource_kind = MaterialBindingResourceKind::uniform_buffer,
        .stages = MaterialShaderStageMask::fragment,
        .texture_slot = MaterialTextureSlot::unknown,
        .semantic = "material_factors",
    });

    metadata.bindings.push_back(MaterialPipelineBinding{
        .set = 0,
        .binding = 6,
        .resource_kind = MaterialBindingResourceKind::uniform_buffer,
        .stages = MaterialShaderStageMask::vertex | MaterialShaderStageMask::fragment,
        .texture_slot = MaterialTextureSlot::unknown,
        .semantic = "scene_pbr_frame",
    });

    for (const auto& texture : sorted_bindings(material.texture_bindings)) {
        metadata.bindings.push_back(MaterialPipelineBinding{
            .set = 0,
            .binding = binding_for_texture_slot(texture.slot),
            .resource_kind = MaterialBindingResourceKind::sampled_texture,
            .stages = MaterialShaderStageMask::fragment,
            .texture_slot = texture.slot,
            .semantic = semantic_for_texture_slot(texture.slot),
        });
        metadata.bindings.push_back(MaterialPipelineBinding{
            .set = 0,
            .binding = sampler_binding_for_texture_slot(texture.slot),
            .resource_kind = MaterialBindingResourceKind::sampler,
            .stages = MaterialShaderStageMask::fragment,
            .texture_slot = texture.slot,
            .semantic = sampler_semantic_for_texture_slot(texture.slot),
        });
    }

    if (!is_valid_material_pipeline_binding_metadata(metadata)) {
        throw std::invalid_argument("material pipeline binding metadata is invalid");
    }
    return metadata;
}

std::string serialize_material_definition(const MaterialDefinition& material) {
    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("material definition is invalid");
    }

    const auto bindings = sorted_bindings(material.texture_bindings);
    std::ostringstream output;
    output << "format=" << material_format << '\n';
    output << "material.id=" << material.id.value << '\n';
    output << "material.name=" << material.name << '\n';
    output << "material.shading=" << shading_model_name(material.shading_model) << '\n';
    output << "material.surface=" << surface_mode_name(material.surface_mode) << '\n';
    output << "material.double_sided=" << (material.double_sided ? "true" : "false") << '\n';
    write_factors(output, material.factors);
    write_texture_bindings(output, bindings);
    return output.str();
}

MaterialDefinition deserialize_material_definition(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != material_format) {
        throw std::invalid_argument("material format is unsupported");
    }

    MaterialDefinition material;
    material.id = AssetId{parse_u64(required_value(values, "material.id"))};
    material.name = required_value(values, "material.name");
    material.shading_model = parse_shading_model(required_value(values, "material.shading"));
    material.surface_mode = parse_surface_mode(required_value(values, "material.surface"));
    material.double_sided = parse_bool(required_value(values, "material.double_sided"));
    material.factors = parse_factors(values);
    material.texture_bindings = parse_texture_bindings(values);

    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("material definition is invalid");
    }
    return material;
}

std::string serialize_material_instance_definition(const MaterialInstanceDefinition& material) {
    if (!is_valid_material_instance_definition(material)) {
        throw std::invalid_argument("material instance definition is invalid");
    }

    const auto bindings = sorted_bindings(material.texture_overrides);
    std::ostringstream output;
    output << "format=" << material_instance_format << '\n';
    output << "instance.id=" << material.id.value << '\n';
    output << "instance.name=" << material.name << '\n';
    output << "instance.parent=" << material.parent.value << '\n';
    output << "factor.override=" << (material.factor_overrides.has_value() ? "true" : "false") << '\n';
    if (material.factor_overrides.has_value()) {
        write_factors(output, *material.factor_overrides);
    }
    write_texture_bindings(output, bindings);
    return output.str();
}

MaterialInstanceDefinition deserialize_material_instance_definition(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != material_instance_format) {
        throw std::invalid_argument("material instance format is unsupported");
    }

    MaterialInstanceDefinition material;
    material.id = AssetId{parse_u64(required_value(values, "instance.id"))};
    material.name = required_value(values, "instance.name");
    material.parent = AssetId{parse_u64(required_value(values, "instance.parent"))};
    if (parse_bool(required_value(values, "factor.override"))) {
        material.factor_overrides = parse_factors(values);
    }
    material.texture_overrides = parse_texture_bindings(values);

    if (!is_valid_material_instance_definition(material)) {
        throw std::invalid_argument("material instance definition is invalid");
    }
    return material;
}

} // namespace mirakana
