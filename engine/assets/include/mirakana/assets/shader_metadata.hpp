// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mirakana {

enum class ShaderSourceLanguage { unknown, hlsl, glsl, msl, wgsl };

enum class ShaderSourceStage { unknown, vertex, fragment, compute };

enum class ShaderArtifactFormat { unknown, dxil, spirv, metal_ir, metallib };

enum class ShaderDescriptorResourceKind {
    unknown,
    uniform_buffer,
    storage_buffer,
    sampled_texture,
    storage_texture,
    sampler
};

struct ShaderGeneratedArtifact {
    std::string path;
    ShaderArtifactFormat format{ShaderArtifactFormat::unknown};
    std::string profile;
    std::string entry_point;
};

struct ShaderDescriptorReflection {
    std::uint32_t set{0};
    std::uint32_t binding{0};
    ShaderDescriptorResourceKind resource_kind{ShaderDescriptorResourceKind::unknown};
    ShaderSourceStage stage{ShaderSourceStage::unknown};
    std::uint32_t count{1};
    std::string semantic;
};

struct ShaderSourceMetadata {
    AssetId id;
    std::string source_path;
    ShaderSourceLanguage language{ShaderSourceLanguage::unknown};
    ShaderSourceStage stage{ShaderSourceStage::unknown};
    std::string entry_point;
    std::vector<std::string> defines;
    std::vector<ShaderGeneratedArtifact> artifacts;
    std::vector<ShaderDescriptorReflection> reflection;
};

[[nodiscard]] bool is_valid_shader_source_metadata(const ShaderSourceMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_shader_generated_artifact(const ShaderGeneratedArtifact& artifact) noexcept;
[[nodiscard]] bool is_valid_shader_descriptor_reflection(const ShaderDescriptorReflection& reflection) noexcept;

class ShaderMetadataRegistry {
  public:
    bool try_add(ShaderSourceMetadata metadata);
    void add(ShaderSourceMetadata metadata);

    [[nodiscard]] const ShaderSourceMetadata* find(AssetId id) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] std::vector<ShaderSourceMetadata> records() const;

    [[nodiscard]] bool try_add_artifact(AssetId id, ShaderGeneratedArtifact artifact);

  private:
    std::unordered_map<AssetId, ShaderSourceMetadata, AssetIdHash> shaders_;
};

} // namespace mirakana
