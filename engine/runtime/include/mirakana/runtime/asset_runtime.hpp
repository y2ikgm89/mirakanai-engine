// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/physics/physics3d.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

struct RuntimeAssetHandle {
    std::uint32_t value{0};

    friend bool operator==(RuntimeAssetHandle lhs, RuntimeAssetHandle rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct RuntimeAssetRecord {
    RuntimeAssetHandle handle;
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
    std::vector<AssetId> dependencies;
    std::string content;
};

struct RuntimeAssetPackageDesc {
    std::string index_path;
    std::string content_root;
};

struct RuntimeAssetPackageLoadFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

template <typename Payload> struct RuntimePayloadAccessResult {
    Payload payload;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeTexturePayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::uint32_t width{0};
    std::uint32_t height{0};
    TextureSourcePixelFormat pixel_format{TextureSourcePixelFormat::unknown};
    std::uint64_t source_bytes{0};
    std::vector<std::uint8_t> bytes;
};

struct RuntimeMeshPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    bool has_normals{false};
    bool has_uvs{false};
    bool has_tangent_frame{false};
    std::vector<std::uint8_t> vertex_bytes;
    std::vector<std::uint8_t> index_bytes;
};

struct RuntimeMorphMeshCpuPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    MorphMeshCpuSourceDocument morph;
};

struct RuntimeAnimationFloatClipPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    AnimationFloatClipSourceDocument clip;
};

struct RuntimeAnimationQuaternionClipPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    AnimationQuaternionClipSourceDocument clip;
};

struct RuntimeSpriteAnimationFrame {
    float duration_seconds{0.0F};
    AssetId sprite;
    AssetId material;
    std::array<float, 2> size{1.0F, 1.0F};
    std::array<float, 4> tint{1.0F, 1.0F, 1.0F, 1.0F};
};

struct RuntimeSpriteAnimationPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::string target_node;
    bool loop{true};
    std::vector<RuntimeSpriteAnimationFrame> frames;
};

/// Cooked GPU skinning mesh (`GameEngine.CookedSkinnedMesh.v1`). Vertex layout is a fixed interleaved stride
/// (`runtime_skinned_mesh_vertex_stride_bytes` in `mirakana_runtime_rhi`) with position, normal, UV, tangent frame,
/// packed joint indices (`uint16x4`), and joint weights (`float32x4`). `joint_palette_bytes` stores one
/// row-major `float4x4` per joint in **little-endian float32** (64 bytes per joint), matching GPU constant-buffer
/// uploads for linear blend skinning at a fixed pose.
struct RuntimeSkinnedMeshPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    std::uint32_t joint_count{0};
    std::vector<std::uint8_t> vertex_bytes;
    std::vector<std::uint8_t> index_bytes;
    std::vector<std::uint8_t> joint_palette_bytes;
};

struct RuntimeAudioPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::uint32_t sample_rate{0};
    std::uint32_t channel_count{0};
    std::uint64_t frame_count{0};
    AudioSourceSampleFormat sample_format{AudioSourceSampleFormat::unknown};
    std::uint64_t source_bytes{0};
    std::vector<std::uint8_t> samples;
};

struct RuntimeMaterialPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    MaterialDefinition material;
    MaterialPipelineBindingMetadata binding_metadata;
};

struct RuntimeUiAtlasPage {
    AssetId asset;
    std::string asset_uri;
};

struct RuntimeUiAtlasImage {
    std::string resource_id;
    std::string asset_uri;
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct RuntimeUiAtlasGlyph {
    std::string font_family;
    std::uint32_t glyph{0};
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct RuntimeUiAtlasPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::vector<RuntimeUiAtlasPage> pages;
    std::vector<RuntimeUiAtlasImage> images;
    std::vector<RuntimeUiAtlasGlyph> glyphs;
};

struct RuntimeTilemapTile {
    std::string id;
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct RuntimeTilemapLayer {
    std::string name;
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool visible{true};
    std::vector<std::string> cells;
};

struct RuntimeTilemapPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    AssetId atlas_page;
    std::string atlas_page_uri;
    std::uint32_t tile_width{0};
    std::uint32_t tile_height{0};
    std::vector<RuntimeTilemapTile> tiles;
    std::vector<RuntimeTilemapLayer> layers;
};

struct RuntimeTilemapVisibleCellSampleResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t layer_count{0};
    std::size_t visible_layer_count{0};
    std::size_t tile_count{0};
    std::size_t non_empty_cell_count{0};
    std::size_t sampled_cell_count{0};
    std::size_t diagnostic_count{0};
};

struct RuntimePhysicsCollisionBody3DPayload {
    std::string name;
    PhysicsBody3DDesc body;
    std::string material;
    std::string compound;
};

struct RuntimePhysicsCollisionScene3DPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    PhysicsWorld3DConfig world_config{};
    std::vector<RuntimePhysicsCollisionBody3DPayload> bodies;
};

struct RuntimeScenePayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    std::string name;
    std::uint32_t node_count{0};
    std::string serialized_scene;
};

struct RuntimeSceneMaterialResolutionFailure {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeSceneMaterialResolutionResult {
    RuntimeScenePayload scene;
    std::vector<RuntimeMaterialPayload> materials;
    std::vector<RuntimeSceneMaterialResolutionFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

class RuntimeAssetPackage {
  public:
    explicit RuntimeAssetPackage(std::vector<RuntimeAssetRecord> records = {},
                                 std::vector<AssetDependencyEdge> dependency_edges = {});

    [[nodiscard]] const std::vector<RuntimeAssetRecord>& records() const noexcept;
    [[nodiscard]] const std::vector<AssetDependencyEdge>& dependency_edges() const noexcept;
    [[nodiscard]] const RuntimeAssetRecord* find(AssetId asset) const noexcept;
    [[nodiscard]] const RuntimeAssetRecord* find(RuntimeAssetHandle handle) const noexcept;
    [[nodiscard]] bool empty() const noexcept;

  private:
    std::vector<RuntimeAssetRecord> records_;
    std::vector<AssetDependencyEdge> dependency_edges_;
};

/// Determines which mounted package wins when the same `AssetId` appears in multiple resident packages.
enum class RuntimePackageMountOverlay : std::uint8_t {
    /// Earlier mounts in the `mounts` vector keep their asset rows; later duplicates are ignored.
    first_mount_wins,
    /// Later mounts overwrite earlier rows for the same `AssetId`.
    last_mount_wins,
};

/// Deterministically merges multiple loaded runtime packages into a single overlay package suitable for
/// `build_runtime_resource_catalog_v2`. Asset handles are reassigned in stable `AssetId` order; dependency
/// vectors are filtered to assets that survive the merge; dependency edges are unioned and deduplicated.
[[nodiscard]] RuntimeAssetPackage merge_runtime_asset_packages_overlay(const std::vector<RuntimeAssetPackage>& mounts,
                                                                       RuntimePackageMountOverlay overlay);

/// Deterministic coarse estimate of resident package bytes: sum of `RuntimeAssetRecord::content` sizes.
/// Matches host-gated package streaming budget checks; not GPU memory, allocator enforcement, or eviction.
[[nodiscard]] std::uint64_t estimate_runtime_asset_package_resident_bytes(const RuntimeAssetPackage& package) noexcept;

struct RuntimeAssetPackageLoadResult {
    RuntimeAssetPackage package;
    std::vector<RuntimeAssetPackageLoadFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeAssetPackageLoadResult load_runtime_asset_package(IFileSystem& filesystem,
                                                                       const RuntimeAssetPackageDesc& desc);

[[nodiscard]] RuntimePayloadAccessResult<RuntimeTexturePayload>
runtime_texture_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeMeshPayload> runtime_mesh_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeMorphMeshCpuPayload>
runtime_morph_mesh_cpu_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeAnimationFloatClipPayload>
runtime_animation_float_clip_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeAnimationQuaternionClipPayload>
runtime_animation_quaternion_clip_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeSpriteAnimationPayload>
runtime_sprite_animation_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeSkinnedMeshPayload>
runtime_skinned_mesh_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeAudioPayload> runtime_audio_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeMaterialPayload>
runtime_material_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeUiAtlasPayload>
runtime_ui_atlas_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeTilemapPayload>
runtime_tilemap_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimeTilemapVisibleCellSampleResult
sample_runtime_tilemap_visible_cells(const RuntimeTilemapPayload& tilemap);
[[nodiscard]] RuntimePayloadAccessResult<RuntimePhysicsCollisionScene3DPayload>
runtime_physics_collision_scene_3d_payload(const RuntimeAssetRecord& record);
[[nodiscard]] RuntimePayloadAccessResult<RuntimeScenePayload> runtime_scene_payload(const RuntimeAssetRecord& record);

[[nodiscard]] RuntimeSceneMaterialResolutionResult resolve_runtime_scene_materials(const RuntimeAssetPackage& package,
                                                                                   AssetId scene);

class RuntimeAssetPackageStore {
  public:
    void seed(RuntimeAssetPackage package);
    void stage(RuntimeAssetPackage package);
    [[nodiscard]] bool stage_if_loaded(RuntimeAssetPackageLoadResult result);
    [[nodiscard]] bool commit_safe_point();
    [[nodiscard]] bool unload_safe_point() noexcept;
    void rollback_pending() noexcept;

    [[nodiscard]] const RuntimeAssetPackage* active() const noexcept;
    [[nodiscard]] const RuntimeAssetPackage* pending() const noexcept;

  private:
    RuntimeAssetPackage active_;
    RuntimeAssetPackage pending_;
    bool has_active_{false};
    bool has_pending_{false};
};

} // namespace mirakana::runtime
