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

enum class TextureColorSpace : std::uint8_t { unknown, linear, srgb };

enum class TextureCompression : std::uint8_t { none, bc1, bc3, bc5, bc7, astc };

struct TextureImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
    TextureColorSpace color_space{TextureColorSpace::unknown};
    bool generate_mips{true};
    TextureCompression compression{TextureCompression::none};
};

struct MeshImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
    float scale{1.0F};
    bool generate_lods{false};
    bool generate_collision{false};
};

struct MorphMeshCpuImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
};

struct AnimationFloatClipImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
};

struct AnimationQuaternionClipImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
};

struct MaterialImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
    std::vector<AssetId> texture_dependencies;
};

struct AudioImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
    bool streaming{false};
};

struct SceneImportMetadata {
    AssetId id;
    std::string source_path;
    std::string imported_path;
    std::vector<AssetId> mesh_dependencies;
    std::vector<AssetId> material_dependencies;
    std::vector<AssetId> sprite_dependencies;
};

[[nodiscard]] bool is_valid_texture_import_metadata(const TextureImportMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_mesh_import_metadata(const MeshImportMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_morph_mesh_cpu_import_metadata(const MorphMeshCpuImportMetadata& metadata) noexcept;
[[nodiscard]] bool
is_valid_animation_float_clip_import_metadata(const AnimationFloatClipImportMetadata& metadata) noexcept;
[[nodiscard]] bool
is_valid_animation_quaternion_clip_import_metadata(const AnimationQuaternionClipImportMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_material_import_metadata(const MaterialImportMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_audio_import_metadata(const AudioImportMetadata& metadata) noexcept;
[[nodiscard]] bool is_valid_scene_import_metadata(const SceneImportMetadata& metadata) noexcept;

class AssetImportMetadataRegistry {
  public:
    bool try_add_texture(TextureImportMetadata metadata);
    bool try_add_mesh(MeshImportMetadata metadata);
    bool try_add_morph_mesh_cpu(MorphMeshCpuImportMetadata metadata);
    bool try_add_animation_float_clip(AnimationFloatClipImportMetadata metadata);
    bool try_add_animation_quaternion_clip(AnimationQuaternionClipImportMetadata metadata);
    bool try_add_material(MaterialImportMetadata metadata);
    bool try_add_audio(AudioImportMetadata metadata);
    bool try_add_scene(SceneImportMetadata metadata);

    void add_texture(TextureImportMetadata metadata);
    void add_mesh(MeshImportMetadata metadata);
    void add_morph_mesh_cpu(MorphMeshCpuImportMetadata metadata);
    void add_animation_float_clip(AnimationFloatClipImportMetadata metadata);
    void add_animation_quaternion_clip(AnimationQuaternionClipImportMetadata metadata);
    void add_material(MaterialImportMetadata metadata);
    void add_audio(AudioImportMetadata metadata);
    void add_scene(SceneImportMetadata metadata);

    [[nodiscard]] const TextureImportMetadata* find_texture(AssetId id) const noexcept;
    [[nodiscard]] const MeshImportMetadata* find_mesh(AssetId id) const noexcept;
    [[nodiscard]] const MorphMeshCpuImportMetadata* find_morph_mesh_cpu(AssetId id) const noexcept;
    [[nodiscard]] const AnimationFloatClipImportMetadata* find_animation_float_clip(AssetId id) const noexcept;
    [[nodiscard]] const AnimationQuaternionClipImportMetadata*
    find_animation_quaternion_clip(AssetId id) const noexcept;
    [[nodiscard]] const MaterialImportMetadata* find_material(AssetId id) const noexcept;
    [[nodiscard]] const AudioImportMetadata* find_audio(AssetId id) const noexcept;
    [[nodiscard]] const SceneImportMetadata* find_scene(AssetId id) const noexcept;

    [[nodiscard]] std::size_t texture_count() const noexcept;
    [[nodiscard]] std::size_t mesh_count() const noexcept;
    [[nodiscard]] std::size_t morph_mesh_cpu_count() const noexcept;
    [[nodiscard]] std::size_t animation_float_clip_count() const noexcept;
    [[nodiscard]] std::size_t animation_quaternion_clip_count() const noexcept;
    [[nodiscard]] std::size_t material_count() const noexcept;
    [[nodiscard]] std::size_t audio_count() const noexcept;
    [[nodiscard]] std::size_t scene_count() const noexcept;

    [[nodiscard]] std::vector<TextureImportMetadata> texture_records() const;
    [[nodiscard]] std::vector<MeshImportMetadata> mesh_records() const;
    [[nodiscard]] std::vector<MorphMeshCpuImportMetadata> morph_mesh_cpu_records() const;
    [[nodiscard]] std::vector<AnimationFloatClipImportMetadata> animation_float_clip_records() const;
    [[nodiscard]] std::vector<AnimationQuaternionClipImportMetadata> animation_quaternion_clip_records() const;
    [[nodiscard]] std::vector<MaterialImportMetadata> material_records() const;
    [[nodiscard]] std::vector<AudioImportMetadata> audio_records() const;
    [[nodiscard]] std::vector<SceneImportMetadata> scene_records() const;

  private:
    std::unordered_map<AssetId, TextureImportMetadata, AssetIdHash> textures_;
    std::unordered_map<AssetId, MeshImportMetadata, AssetIdHash> meshes_;
    std::unordered_map<AssetId, MorphMeshCpuImportMetadata, AssetIdHash> morph_meshes_cpu_;
    std::unordered_map<AssetId, AnimationFloatClipImportMetadata, AssetIdHash> animation_float_clips_;
    std::unordered_map<AssetId, AnimationQuaternionClipImportMetadata, AssetIdHash> animation_quaternion_clips_;
    std::unordered_map<AssetId, MaterialImportMetadata, AssetIdHash> materials_;
    std::unordered_map<AssetId, AudioImportMetadata, AssetIdHash> audio_;
    std::unordered_map<AssetId, SceneImportMetadata, AssetIdHash> scenes_;
};

} // namespace mirakana
