// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/scene/prefab.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class PrefabOverrideKind { name, transform, components };

enum class PrefabVariantDiagnosticKind {
    invalid_base_prefab,
    invalid_variant_name,
    invalid_node_index,
    invalid_override_kind,
    duplicate_override,
    invalid_node_name,
    invalid_source_node_name,
    invalid_transform,
    invalid_components,
};

struct PrefabNodeOverride {
    std::uint32_t node_index{0};
    PrefabOverrideKind kind{PrefabOverrideKind::name};
    std::string name;
    Transform3D transform;
    SceneNodeComponents components;
    std::string source_node_name;
};

struct PrefabVariantDefinition {
    std::string name;
    PrefabDefinition base_prefab;
    std::vector<PrefabNodeOverride> overrides;
};

struct PrefabVariantDiagnostic {
    PrefabVariantDiagnosticKind kind{PrefabVariantDiagnosticKind::invalid_base_prefab};
    std::uint32_t node_index{0};
    PrefabOverrideKind override_kind{PrefabOverrideKind::name};
    std::string message;
};

struct PrefabVariantComposeResult {
    bool success{false};
    PrefabDefinition prefab;
    std::vector<PrefabVariantDiagnostic> diagnostics;
};

[[nodiscard]] std::string_view prefab_override_kind_label(PrefabOverrideKind kind) noexcept;
[[nodiscard]] std::string_view prefab_variant_diagnostic_kind_label(PrefabVariantDiagnosticKind kind) noexcept;
[[nodiscard]] std::vector<PrefabVariantDiagnostic>
validate_prefab_variant_definition(const PrefabVariantDefinition& variant);
[[nodiscard]] bool is_valid_prefab_variant_definition(const PrefabVariantDefinition& variant);
[[nodiscard]] PrefabVariantComposeResult compose_prefab_variant(const PrefabVariantDefinition& variant);
[[nodiscard]] std::string serialize_prefab_variant_definition(const PrefabVariantDefinition& variant);
[[nodiscard]] PrefabVariantDefinition deserialize_prefab_variant_definition(std::string_view text);
[[nodiscard]] PrefabVariantDefinition deserialize_prefab_variant_definition_for_review(std::string_view text);

} // namespace mirakana
