// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class MaterialShadingModel : std::uint8_t { unknown, unlit, lit };

enum class MaterialSurfaceMode : std::uint8_t { unknown, opaque, masked, transparent };

enum class MaterialTextureSlot : std::uint8_t { unknown, base_color, normal, metallic_roughness, emissive, occlusion };

enum class MaterialBindingResourceKind : std::uint8_t { unknown, uniform_buffer, sampled_texture, sampler };

enum class MaterialShaderStageMask : std::uint8_t {
    none = 0,
    vertex = 1U << 0U,
    fragment = 1U << 1U,
};

[[nodiscard]] constexpr MaterialShaderStageMask operator|(MaterialShaderStageMask lhs,
                                                          MaterialShaderStageMask rhs) noexcept {
    return static_cast<MaterialShaderStageMask>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

struct MaterialFactors {
    std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissive{0.0F, 0.0F, 0.0F};
    float metallic{0.0F};
    float roughness{1.0F};
};

inline constexpr std::uint64_t material_factor_uniform_size_bytes = 64;

struct MaterialTextureBinding {
    MaterialTextureSlot slot{MaterialTextureSlot::unknown};
    AssetId texture;
};

struct MaterialDefinition {
    AssetId id;
    std::string name;
    MaterialShadingModel shading_model{MaterialShadingModel::lit};
    MaterialSurfaceMode surface_mode{MaterialSurfaceMode::opaque};
    MaterialFactors factors;
    std::vector<MaterialTextureBinding> texture_bindings;
    bool double_sided{false};
};

struct MaterialInstanceDefinition {
    AssetId id;
    std::string name;
    AssetId parent;
    std::optional<MaterialFactors> factor_overrides;
    std::vector<MaterialTextureBinding> texture_overrides;
};

struct MaterialPipelineBinding {
    std::uint32_t set{0};
    std::uint32_t binding{0};
    MaterialBindingResourceKind resource_kind{MaterialBindingResourceKind::unknown};
    MaterialShaderStageMask stages{MaterialShaderStageMask::none};
    MaterialTextureSlot texture_slot{MaterialTextureSlot::unknown};
    std::string semantic;
};

struct MaterialPipelineBindingMetadata {
    AssetId material;
    MaterialShadingModel shading_model{MaterialShadingModel::unknown};
    MaterialSurfaceMode surface_mode{MaterialSurfaceMode::unknown};
    bool double_sided{false};
    bool requires_alpha_test{false};
    bool requires_alpha_blending{false};
    std::vector<MaterialPipelineBinding> bindings;
};

[[nodiscard]] bool is_valid_material_texture_binding(const MaterialTextureBinding& binding) noexcept;
[[nodiscard]] bool is_valid_material_definition(const MaterialDefinition& material) noexcept;
[[nodiscard]] bool is_valid_material_instance_definition(const MaterialInstanceDefinition& material) noexcept;
[[nodiscard]] bool is_valid_material_pipeline_binding(const MaterialPipelineBinding& binding) noexcept;
[[nodiscard]] bool
is_valid_material_pipeline_binding_metadata(const MaterialPipelineBindingMetadata& metadata) noexcept;

[[nodiscard]] MaterialDefinition compose_material_instance(const MaterialDefinition& parent,
                                                           const MaterialInstanceDefinition& instance);
[[nodiscard]] MaterialPipelineBindingMetadata
build_material_pipeline_binding_metadata(const MaterialDefinition& material);

[[nodiscard]] std::string serialize_material_definition(const MaterialDefinition& material);
[[nodiscard]] MaterialDefinition deserialize_material_definition(std::string_view text);

[[nodiscard]] std::string serialize_material_instance_definition(const MaterialInstanceDefinition& material);
[[nodiscard]] MaterialInstanceDefinition deserialize_material_instance_definition(std::string_view text);

} // namespace mirakana
