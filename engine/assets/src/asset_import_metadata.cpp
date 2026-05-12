// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_metadata.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_color_space(TextureColorSpace color_space) noexcept {
    switch (color_space) {
    case TextureColorSpace::linear:
    case TextureColorSpace::srgb:
        return true;
    case TextureColorSpace::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_dependency_list(const std::vector<AssetId>& dependencies) noexcept {
    for (std::size_t index = 0; index < dependencies.size(); ++index) {
        if (dependencies[index].value == 0) {
            return false;
        }
        for (std::size_t other = index + 1U; other < dependencies.size(); ++other) {
            if (dependencies[index] == dependencies[other]) {
                return false;
            }
        }
    }
    return true;
}

template <typename Metadata>
[[nodiscard]] std::vector<Metadata> sorted_records(const std::unordered_map<AssetId, Metadata, AssetIdHash>& records) {
    std::vector<Metadata> result;
    result.reserve(records.size());
    for (const auto& [_, metadata] : records) {
        result.push_back(metadata);
    }
    std::ranges::sort(result,
                      [](const Metadata& lhs, const Metadata& rhs) { return lhs.imported_path < rhs.imported_path; });
    return result;
}

} // namespace

bool is_valid_texture_import_metadata(const TextureImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path) &&
           valid_color_space(metadata.color_space);
}

bool is_valid_mesh_import_metadata(const MeshImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path) &&
           metadata.scale > 0.0F;
}

bool is_valid_morph_mesh_cpu_import_metadata(const MorphMeshCpuImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path);
}

bool is_valid_animation_float_clip_import_metadata(const AnimationFloatClipImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path);
}

bool is_valid_animation_quaternion_clip_import_metadata(
    const AnimationQuaternionClipImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path);
}

bool is_valid_material_import_metadata(const MaterialImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path) &&
           valid_dependency_list(metadata.texture_dependencies);
}

bool is_valid_audio_import_metadata(const AudioImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path);
}

bool is_valid_scene_import_metadata(const SceneImportMetadata& metadata) noexcept {
    return metadata.id.value != 0 && valid_token(metadata.source_path) && valid_token(metadata.imported_path) &&
           valid_dependency_list(metadata.mesh_dependencies) && valid_dependency_list(metadata.material_dependencies) &&
           valid_dependency_list(metadata.sprite_dependencies);
}

bool AssetImportMetadataRegistry::try_add_texture(TextureImportMetadata metadata) {
    if (!is_valid_texture_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = textures_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_mesh(MeshImportMetadata metadata) {
    if (!is_valid_mesh_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = meshes_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_morph_mesh_cpu(MorphMeshCpuImportMetadata metadata) {
    if (!is_valid_morph_mesh_cpu_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = morph_meshes_cpu_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_animation_float_clip(AnimationFloatClipImportMetadata metadata) {
    if (!is_valid_animation_float_clip_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = animation_float_clips_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_animation_quaternion_clip(AnimationQuaternionClipImportMetadata metadata) {
    if (!is_valid_animation_quaternion_clip_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = animation_quaternion_clips_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_material(MaterialImportMetadata metadata) {
    if (!is_valid_material_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = materials_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_audio(AudioImportMetadata metadata) {
    if (!is_valid_audio_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = audio_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

bool AssetImportMetadataRegistry::try_add_scene(SceneImportMetadata metadata) {
    if (!is_valid_scene_import_metadata(metadata)) {
        return false;
    }
    auto [_, inserted] = scenes_.emplace(metadata.id, std::move(metadata));
    return inserted;
}

void AssetImportMetadataRegistry::add_texture(TextureImportMetadata metadata) {
    if (!try_add_texture(std::move(metadata))) {
        throw std::logic_error("texture import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_mesh(MeshImportMetadata metadata) {
    if (!try_add_mesh(std::move(metadata))) {
        throw std::logic_error("mesh import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_morph_mesh_cpu(MorphMeshCpuImportMetadata metadata) {
    if (!try_add_morph_mesh_cpu(std::move(metadata))) {
        throw std::logic_error("morph mesh CPU import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_animation_float_clip(AnimationFloatClipImportMetadata metadata) {
    if (!try_add_animation_float_clip(std::move(metadata))) {
        throw std::logic_error("animation float clip import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_animation_quaternion_clip(AnimationQuaternionClipImportMetadata metadata) {
    if (!try_add_animation_quaternion_clip(std::move(metadata))) {
        throw std::logic_error("animation quaternion clip import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_material(MaterialImportMetadata metadata) {
    if (!try_add_material(std::move(metadata))) {
        throw std::logic_error("material import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_audio(AudioImportMetadata metadata) {
    if (!try_add_audio(std::move(metadata))) {
        throw std::logic_error("audio import metadata could not be added");
    }
}

void AssetImportMetadataRegistry::add_scene(SceneImportMetadata metadata) {
    if (!try_add_scene(std::move(metadata))) {
        throw std::logic_error("scene import metadata could not be added");
    }
}

const TextureImportMetadata* AssetImportMetadataRegistry::find_texture(AssetId id) const noexcept {
    const auto it = textures_.find(id);
    return it == textures_.end() ? nullptr : &it->second;
}

const MeshImportMetadata* AssetImportMetadataRegistry::find_mesh(AssetId id) const noexcept {
    const auto it = meshes_.find(id);
    return it == meshes_.end() ? nullptr : &it->second;
}

const MorphMeshCpuImportMetadata* AssetImportMetadataRegistry::find_morph_mesh_cpu(AssetId id) const noexcept {
    const auto it = morph_meshes_cpu_.find(id);
    return it == morph_meshes_cpu_.end() ? nullptr : &it->second;
}

const AnimationFloatClipImportMetadata*
AssetImportMetadataRegistry::find_animation_float_clip(AssetId id) const noexcept {
    const auto it = animation_float_clips_.find(id);
    return it == animation_float_clips_.end() ? nullptr : &it->second;
}

const AnimationQuaternionClipImportMetadata*
AssetImportMetadataRegistry::find_animation_quaternion_clip(AssetId id) const noexcept {
    const auto it = animation_quaternion_clips_.find(id);
    return it == animation_quaternion_clips_.end() ? nullptr : &it->second;
}

const MaterialImportMetadata* AssetImportMetadataRegistry::find_material(AssetId id) const noexcept {
    const auto it = materials_.find(id);
    return it == materials_.end() ? nullptr : &it->second;
}

const AudioImportMetadata* AssetImportMetadataRegistry::find_audio(AssetId id) const noexcept {
    const auto it = audio_.find(id);
    return it == audio_.end() ? nullptr : &it->second;
}

const SceneImportMetadata* AssetImportMetadataRegistry::find_scene(AssetId id) const noexcept {
    const auto it = scenes_.find(id);
    return it == scenes_.end() ? nullptr : &it->second;
}

std::size_t AssetImportMetadataRegistry::texture_count() const noexcept {
    return textures_.size();
}

std::size_t AssetImportMetadataRegistry::mesh_count() const noexcept {
    return meshes_.size();
}

std::size_t AssetImportMetadataRegistry::morph_mesh_cpu_count() const noexcept {
    return morph_meshes_cpu_.size();
}

std::size_t AssetImportMetadataRegistry::animation_float_clip_count() const noexcept {
    return animation_float_clips_.size();
}

std::size_t AssetImportMetadataRegistry::animation_quaternion_clip_count() const noexcept {
    return animation_quaternion_clips_.size();
}

std::size_t AssetImportMetadataRegistry::material_count() const noexcept {
    return materials_.size();
}

std::size_t AssetImportMetadataRegistry::audio_count() const noexcept {
    return audio_.size();
}

std::size_t AssetImportMetadataRegistry::scene_count() const noexcept {
    return scenes_.size();
}

std::vector<TextureImportMetadata> AssetImportMetadataRegistry::texture_records() const {
    return sorted_records(textures_);
}

std::vector<MeshImportMetadata> AssetImportMetadataRegistry::mesh_records() const {
    return sorted_records(meshes_);
}

std::vector<MorphMeshCpuImportMetadata> AssetImportMetadataRegistry::morph_mesh_cpu_records() const {
    return sorted_records(morph_meshes_cpu_);
}

std::vector<AnimationFloatClipImportMetadata> AssetImportMetadataRegistry::animation_float_clip_records() const {
    return sorted_records(animation_float_clips_);
}

std::vector<AnimationQuaternionClipImportMetadata>
AssetImportMetadataRegistry::animation_quaternion_clip_records() const {
    return sorted_records(animation_quaternion_clips_);
}

std::vector<MaterialImportMetadata> AssetImportMetadataRegistry::material_records() const {
    return sorted_records(materials_);
}

std::vector<AudioImportMetadata> AssetImportMetadataRegistry::audio_records() const {
    return sorted_records(audio_);
}

std::vector<SceneImportMetadata> AssetImportMetadataRegistry::scene_records() const {
    return sorted_records(scenes_);
}

} // namespace mirakana
