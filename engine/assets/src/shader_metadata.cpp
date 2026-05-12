// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/shader_metadata.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_source_language(ShaderSourceLanguage language) noexcept {
    switch (language) {
    case ShaderSourceLanguage::hlsl:
    case ShaderSourceLanguage::glsl:
    case ShaderSourceLanguage::msl:
    case ShaderSourceLanguage::wgsl:
        return true;
    case ShaderSourceLanguage::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_source_stage(ShaderSourceStage stage) noexcept {
    switch (stage) {
    case ShaderSourceStage::vertex:
    case ShaderSourceStage::fragment:
    case ShaderSourceStage::compute:
        return true;
    case ShaderSourceStage::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_artifact_format(ShaderArtifactFormat format) noexcept {
    switch (format) {
    case ShaderArtifactFormat::dxil:
    case ShaderArtifactFormat::spirv:
    case ShaderArtifactFormat::metal_ir:
    case ShaderArtifactFormat::metallib:
        return true;
    case ShaderArtifactFormat::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_descriptor_resource_kind(ShaderDescriptorResourceKind kind) noexcept {
    switch (kind) {
    case ShaderDescriptorResourceKind::uniform_buffer:
    case ShaderDescriptorResourceKind::storage_buffer:
    case ShaderDescriptorResourceKind::sampled_texture:
    case ShaderDescriptorResourceKind::storage_texture:
    case ShaderDescriptorResourceKind::sampler:
        return true;
    case ShaderDescriptorResourceKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool same_artifact_identity(const ShaderGeneratedArtifact& lhs,
                                          const ShaderGeneratedArtifact& rhs) noexcept {
    return lhs.path == rhs.path && lhs.format == rhs.format && lhs.profile == rhs.profile &&
           lhs.entry_point == rhs.entry_point;
}

} // namespace

bool is_valid_shader_source_metadata(const ShaderSourceMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_source_language(metadata.language) &&
           valid_source_stage(metadata.stage) && valid_token(metadata.entry_point);
}

bool is_valid_shader_generated_artifact(const ShaderGeneratedArtifact& artifact) noexcept {
    return valid_token(artifact.path) && valid_artifact_format(artifact.format) && valid_token(artifact.profile) &&
           valid_token(artifact.entry_point);
}

bool is_valid_shader_descriptor_reflection(const ShaderDescriptorReflection& reflection) noexcept {
    return valid_descriptor_resource_kind(reflection.resource_kind) && valid_source_stage(reflection.stage) &&
           reflection.count > 0 && valid_token(reflection.semantic);
}

bool ShaderMetadataRegistry::try_add(ShaderSourceMetadata metadata) {
    if (!is_valid_shader_source_metadata(metadata)) {
        return false;
    }
    for (const auto& artifact : metadata.artifacts) {
        if (!is_valid_shader_generated_artifact(artifact)) {
            return false;
        }
    }
    for (std::size_t index = 0; index < metadata.reflection.size(); ++index) {
        if (!is_valid_shader_descriptor_reflection(metadata.reflection[index])) {
            return false;
        }
        for (std::size_t other = index + 1U; other < metadata.reflection.size(); ++other) {
            if (metadata.reflection[index].set == metadata.reflection[other].set &&
                metadata.reflection[index].binding == metadata.reflection[other].binding) {
                return false;
            }
        }
    }

    auto [_, inserted] = shaders_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

void ShaderMetadataRegistry::add(ShaderSourceMetadata metadata) {
    if (!try_add(std::move(metadata))) {
        throw std::logic_error("shader metadata could not be added");
    }
}

const ShaderSourceMetadata* ShaderMetadataRegistry::find(AssetId id) const noexcept {
    const auto it = shaders_.find(id);
    return it == shaders_.end() ? nullptr : &it->second;
}

std::size_t ShaderMetadataRegistry::count() const noexcept {
    return shaders_.size();
}

std::vector<ShaderSourceMetadata> ShaderMetadataRegistry::records() const {
    std::vector<ShaderSourceMetadata> result;
    result.reserve(shaders_.size());
    for (const auto& [_, metadata] : shaders_) {
        result.push_back(metadata);
    }
    std::ranges::sort(result, [](const ShaderSourceMetadata& lhs, const ShaderSourceMetadata& rhs) {
        return lhs.source_path < rhs.source_path;
    });
    return result;
}

bool ShaderMetadataRegistry::try_add_artifact(AssetId id, ShaderGeneratedArtifact artifact) {
    if (!is_valid_shader_generated_artifact(artifact)) {
        return false;
    }

    auto it = shaders_.find(id);
    if (it == shaders_.end()) {
        return false;
    }

    auto& artifacts = it->second.artifacts;
    const auto duplicate = std::ranges::find_if(artifacts, [&artifact](const ShaderGeneratedArtifact& existing) {
        return same_artifact_identity(existing, artifact);
    });
    if (duplicate != artifacts.end()) {
        return false;
    }

    artifacts.push_back(std::move(artifact));
    return true;
}

} // namespace mirakana
