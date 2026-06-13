// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class TextureSourcePixelFormat : std::uint8_t { unknown, r8_unorm, rg8_unorm, rgba8_unorm };

enum class AudioSourceSampleFormat : std::uint8_t { unknown, pcm16, float32 };

enum class TextureSourceKindV2 : std::uint8_t { unknown, openexr, ktx2_basis };

enum class TextureColorSpaceV2 : std::uint8_t { unknown, srgb, scene_linear, linear_data };

enum class TextureSamplerClassV2 : std::uint8_t { unknown, color, normal, data, environment_radiance };

enum class TexturePixelEncodingV2 : std::uint8_t { unknown, uint8_unorm, float16, float32, basis_etc1s, basis_uastc };

enum class TextureCompressionKindV2 : std::uint8_t { unknown, none, basis_lz, uastc, bc6h, bc7, astc_4x4 };

enum class TextureCookBackendV1 : std::uint8_t { unknown, d3d12, vulkan, metal_macos, vulkan_android, metal_ios };

enum class TextureCookTranscodeKindV1 : std::uint8_t { unknown, not_required, offline_policy, basis_transcode_policy };

struct TextureSourceDocument {
    std::uint32_t width{0};
    std::uint32_t height{0};
    TextureSourcePixelFormat pixel_format{TextureSourcePixelFormat::unknown};
    std::vector<std::uint8_t> bytes;
};

struct TextureSourceWindowV2 {
    std::int32_t min_x{0};
    std::int32_t min_y{0};
    std::int32_t max_x{-1};
    std::int32_t max_y{-1};
};

struct TextureOpenExrSourceReviewV2 {
    TextureSourceWindowV2 data_window;
    TextureSourceWindowV2 display_window;
    std::string channel_list;
    TexturePixelEncodingV2 pixel_encoding{TexturePixelEncodingV2::unknown};
    std::uint32_t channel_count{0};
    bool chromaticities_recorded{false};
    bool scene_linear_intent{false};
    bool multipart{false};
    bool deep{false};
    bool tiled{false};
};

struct TextureKtx2BasisSourceReviewV2 {
    std::string vk_format;
    std::uint32_t level_count{0};
    std::uint32_t layer_count{0};
    std::uint32_t face_count{0};
    TextureCompressionKindV2 supercompression{TextureCompressionKindV2::unknown};
    TexturePixelEncodingV2 basis_codec{TexturePixelEncodingV2::unknown};
    bool requires_transcoding{false};
};

struct TextureSourceDocumentV2 {
    std::string source_path;
    std::string source_hash;
    std::string provenance_id;
    std::string license_id;
    TextureSourceKindV2 source_kind{TextureSourceKindV2::unknown};
    std::uint32_t width{0};
    std::uint32_t height{0};
    TextureColorSpaceV2 color_space{TextureColorSpaceV2::unknown};
    TextureSamplerClassV2 sampler_class{TextureSamplerClassV2::unknown};
    TextureOpenExrSourceReviewV2 openexr;
    TextureKtx2BasisSourceReviewV2 ktx2_basis;
};

struct TextureCookBackendDecisionV1 {
    TextureCookBackendV1 backend{TextureCookBackendV1::unknown};
    std::string device_format;
    TextureCompressionKindV2 compression{TextureCompressionKindV2::unknown};
    TextureCookTranscodeKindV1 transcode{TextureCookTranscodeKindV1::unknown};
    std::uint64_t estimated_gpu_bytes{0};
    bool supported{false};
    bool host_validated{false};
    std::string diagnostic;
};

struct TextureCookMetadataDocumentV1 {
    TextureSourceDocumentV2 source;
    std::vector<TextureCookBackendDecisionV1> backend_decisions;
    std::uint64_t estimated_source_bytes{0};
    std::uint64_t estimated_decoded_bytes{0};
};

struct EnvironmentTextureGeassetMetadataDocumentV1 {
    std::string geasset_path;
    TextureCookMetadataDocumentV1 cook_metadata;
    std::uint32_t mip_count{0};
    std::uint64_t max_estimated_gpu_bytes{0};
    std::uint32_t unsupported_host_diagnostic_count{0};
    bool metadata_only{true};
};

struct EnvironmentAssetSourceDocumentV1 {
    std::string environment_profile_source_path;
    std::string radiance_texture_source_path;
    std::string provenance_id;
    std::string license_id;
    bool requires_scene_linear_radiance{false};
    bool texture_source_v2_required{false};
};

struct MeshSourceDocument {
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    bool has_normals{false};
    bool has_uvs{false};
    /// When true, `vertex_bytes` use the v2 tangent-space stride (position, normal, TEXCOORD_0, tangent vec4 with
    /// handedness in `w` per glTF tangent conventions). Requires `has_normals` and `has_uvs`.
    bool has_tangent_frame{false};
    std::vector<std::uint8_t> vertex_bytes;
    std::vector<std::uint8_t> index_bytes;
};

struct AudioSourceDocument {
    std::uint32_t sample_rate{0};
    std::uint32_t channel_count{0};
    std::uint64_t frame_count{0};
    AudioSourceSampleFormat sample_format{AudioSourceSampleFormat::unknown};
    std::vector<std::uint8_t> samples;
};

/// One morph target stream set: each non-empty vector must hold `vertex_count` float32x3 values (12 bytes per vertex),
/// little-endian, matching `MorphMeshCpuSourceDocument::vertex_count`.
struct MorphMeshCpuTargetSourceDocument {
    std::vector<std::uint8_t> position_delta_bytes;
    std::vector<std::uint8_t> normal_delta_bytes;
    std::vector<std::uint8_t> tangent_delta_bytes;
};

/// One scalar float track: `time_seconds` and `value` keyframes as concatenated little-endian `float32` blobs
/// (`keyframe_count` floats each), matching `GameEngine.AnimationFloatClipSource.v1` / cooked clip payloads.
struct AnimationFloatClipTrackSourceDocument {
    std::string target;
    std::vector<std::uint8_t> time_seconds_bytes;
    std::vector<std::uint8_t> value_bytes;
};

/// Multi-track scalar float clip (`GameEngine.AnimationFloatClipSource.v1`).
struct AnimationFloatClipSourceDocument {
    std::vector<AnimationFloatClipTrackSourceDocument> tracks;
};

/// One 3D TRS track for quaternion local-pose animation. Each non-empty component uses its own strictly increasing
/// little-endian float32 time blob. Translation and scale value blobs store float32 xyz rows; rotation stores
/// normalized float32 xyzw quaternion rows.
struct AnimationQuaternionClipTrackSourceDocument {
    std::string target;
    std::uint32_t joint_index{0};
    std::vector<std::uint8_t> translation_time_seconds_bytes;
    std::vector<std::uint8_t> translation_xyz_bytes;
    std::vector<std::uint8_t> rotation_time_seconds_bytes;
    std::vector<std::uint8_t> rotation_xyzw_bytes;
    std::vector<std::uint8_t> scale_time_seconds_bytes;
    std::vector<std::uint8_t> scale_xyz_bytes;
};

/// Multi-track 3D TRS quaternion clip (`GameEngine.AnimationQuaternionClipSource.v1`).
struct AnimationQuaternionClipSourceDocument {
    std::vector<AnimationQuaternionClipTrackSourceDocument> tracks;
};

enum class AnimationTransformBindingComponent : std::uint8_t {
    unknown,
    translation_x,
    translation_y,
    translation_z,
    rotation_z,
    scale_x,
    scale_y,
    scale_z,
};

/// One scalar curve target binding to a named transform component. `node_name` is caller-resolved and intentionally
/// does not require a scene dependency in `mirakana_assets`.
struct AnimationTransformBindingSourceRow {
    std::string target;
    std::string node_name;
    AnimationTransformBindingComponent component{AnimationTransformBindingComponent::unknown};
};

/// Authored scalar curve target to transform component bindings
/// (`GameEngine.AnimationTransformBindingSource.v1`).
struct AnimationTransformBindingSourceDocument {
    std::vector<AnimationTransformBindingSourceRow> bindings;
};

/// CPU morph mesh source aligned with glTF-style additive morph targets (`GameEngine.MorphMeshCpuSource.v1`).
/// `bind_position_bytes` holds `vertex_count` vec3f positions; optional bind normal/tangent bases follow the same
/// animation morph rules as `mirakana_animation` morph validation when normal/tangent delta streams are present.
struct MorphMeshCpuSourceDocument {
    std::uint32_t vertex_count{0};
    std::vector<std::uint8_t> bind_position_bytes;
    std::vector<std::uint8_t> bind_normal_bytes;
    std::vector<std::uint8_t> bind_tangent_bytes;
    std::vector<MorphMeshCpuTargetSourceDocument> targets;
    /// Little-endian float32 weights, one per target, each in `[0, 1]`.
    std::vector<std::uint8_t> target_weight_bytes;
};

[[nodiscard]] bool is_valid_texture_source_document(const TextureSourceDocument& document) noexcept;
[[nodiscard]] bool is_valid_texture_source_document_v2(const TextureSourceDocumentV2& document) noexcept;
[[nodiscard]] bool is_valid_texture_cook_metadata_document_v1(const TextureCookMetadataDocumentV1& document) noexcept;
[[nodiscard]] bool is_valid_environment_texture_geasset_metadata_document_v1(
    const EnvironmentTextureGeassetMetadataDocumentV1& document) noexcept;
[[nodiscard]] bool
is_valid_environment_asset_source_document_v1(const EnvironmentAssetSourceDocumentV1& document) noexcept;
[[nodiscard]] bool is_valid_mesh_source_document(const MeshSourceDocument& document) noexcept;

/// Interleaved vertex stride for `MeshSourceDocument` payloads (`nullopt` when flags are inconsistent).
[[nodiscard]] std::optional<std::uint32_t> mesh_source_vertex_stride_bytes(const MeshSourceDocument& document) noexcept;
[[nodiscard]] bool is_valid_audio_source_document(const AudioSourceDocument& document) noexcept;
[[nodiscard]] bool is_valid_morph_mesh_cpu_source_document(const MorphMeshCpuSourceDocument& document) noexcept;
[[nodiscard]] bool
is_valid_animation_float_clip_source_document(const AnimationFloatClipSourceDocument& document) noexcept;
[[nodiscard]] bool
is_valid_animation_quaternion_clip_source_document(const AnimationQuaternionClipSourceDocument& document) noexcept;
[[nodiscard]] bool
is_valid_animation_transform_binding_source_document(const AnimationTransformBindingSourceDocument& document) noexcept;

[[nodiscard]] std::string_view texture_source_pixel_format_name(TextureSourcePixelFormat pixel_format) noexcept;
[[nodiscard]] TextureSourcePixelFormat parse_texture_source_pixel_format(std::string_view value);
[[nodiscard]] std::uint32_t texture_source_bytes_per_pixel(TextureSourcePixelFormat pixel_format);
[[nodiscard]] std::uint64_t texture_source_uncompressed_bytes(const TextureSourceDocument& document);

[[nodiscard]] std::string_view texture_source_kind_name_v2(TextureSourceKindV2 kind) noexcept;
[[nodiscard]] TextureSourceKindV2 parse_texture_source_kind_v2(std::string_view value);
[[nodiscard]] std::string_view texture_color_space_name_v2(TextureColorSpaceV2 color_space) noexcept;
[[nodiscard]] TextureColorSpaceV2 parse_texture_color_space_v2(std::string_view value);
[[nodiscard]] std::string_view texture_sampler_class_name_v2(TextureSamplerClassV2 sampler_class) noexcept;
[[nodiscard]] TextureSamplerClassV2 parse_texture_sampler_class_v2(std::string_view value);
[[nodiscard]] std::string_view texture_pixel_encoding_name_v2(TexturePixelEncodingV2 encoding) noexcept;
[[nodiscard]] TexturePixelEncodingV2 parse_texture_pixel_encoding_v2(std::string_view value);
[[nodiscard]] std::string_view texture_compression_kind_name_v2(TextureCompressionKindV2 compression) noexcept;
[[nodiscard]] TextureCompressionKindV2 parse_texture_compression_kind_v2(std::string_view value);
[[nodiscard]] std::string_view texture_cook_backend_name_v1(TextureCookBackendV1 backend) noexcept;
[[nodiscard]] TextureCookBackendV1 parse_texture_cook_backend_v1(std::string_view value);
[[nodiscard]] std::string_view texture_cook_transcode_kind_name_v1(TextureCookTranscodeKindV1 transcode) noexcept;
[[nodiscard]] TextureCookTranscodeKindV1 parse_texture_cook_transcode_kind_v1(std::string_view value);

[[nodiscard]] std::string_view audio_source_sample_format_name(AudioSourceSampleFormat sample_format) noexcept;
[[nodiscard]] AudioSourceSampleFormat parse_audio_source_sample_format(std::string_view value);
[[nodiscard]] std::uint32_t audio_source_bytes_per_sample(AudioSourceSampleFormat sample_format);
[[nodiscard]] std::uint64_t audio_source_uncompressed_bytes(const AudioSourceDocument& document);

[[nodiscard]] std::string_view
animation_transform_binding_component_name(AnimationTransformBindingComponent component) noexcept;
[[nodiscard]] AnimationTransformBindingComponent parse_animation_transform_binding_component(std::string_view value);

[[nodiscard]] std::string serialize_texture_source_document(const TextureSourceDocument& document);
[[nodiscard]] TextureSourceDocument deserialize_texture_source_document(std::string_view text);
[[nodiscard]] std::string serialize_texture_source_document_v2(const TextureSourceDocumentV2& document);
[[nodiscard]] TextureSourceDocumentV2 deserialize_texture_source_document_v2(std::string_view text);
[[nodiscard]] std::string serialize_texture_cook_metadata_document_v1(const TextureCookMetadataDocumentV1& document);
[[nodiscard]] std::string
serialize_environment_texture_geasset_metadata_document_v1(const EnvironmentTextureGeassetMetadataDocumentV1& document);
[[nodiscard]] std::string
serialize_environment_asset_source_document_v1(const EnvironmentAssetSourceDocumentV1& document);
[[nodiscard]] EnvironmentAssetSourceDocumentV1 deserialize_environment_asset_source_document_v1(std::string_view text);

[[nodiscard]] std::string serialize_mesh_source_document(const MeshSourceDocument& document);
[[nodiscard]] MeshSourceDocument deserialize_mesh_source_document(std::string_view text);

[[nodiscard]] std::string serialize_audio_source_document(const AudioSourceDocument& document);
[[nodiscard]] AudioSourceDocument deserialize_audio_source_document(std::string_view text);

[[nodiscard]] std::string serialize_morph_mesh_cpu_source_document(const MorphMeshCpuSourceDocument& document);
[[nodiscard]] MorphMeshCpuSourceDocument deserialize_morph_mesh_cpu_source_document(std::string_view text);

[[nodiscard]] std::string
serialize_animation_float_clip_source_document(const AnimationFloatClipSourceDocument& document);
[[nodiscard]] AnimationFloatClipSourceDocument deserialize_animation_float_clip_source_document(std::string_view text);

[[nodiscard]] std::string
serialize_animation_quaternion_clip_source_document(const AnimationQuaternionClipSourceDocument& document);
[[nodiscard]] AnimationQuaternionClipSourceDocument
deserialize_animation_quaternion_clip_source_document(std::string_view text);

[[nodiscard]] std::string
serialize_animation_transform_binding_source_document(const AnimationTransformBindingSourceDocument& document);
[[nodiscard]] AnimationTransformBindingSourceDocument
deserialize_animation_transform_binding_source_document(std::string_view text);

/// Writes `clip.*` payload lines (no `format=` line) for embedding in cooked artifacts.
void write_animation_float_clip_document_payload(std::ostream& output,
                                                 const AnimationFloatClipSourceDocument& document);

/// Writes `clip.*` payload lines (no `format=` line) for embedding in cooked quaternion clip artifacts.
void write_animation_quaternion_clip_document_payload(std::ostream& output,
                                                      const AnimationQuaternionClipSourceDocument& document);

/// Writes `morph.*` payload lines (no `format=` line) for embedding in cooked artifacts.
void write_morph_mesh_cpu_document_payload(std::ostream& output, const MorphMeshCpuSourceDocument& document);

} // namespace mirakana
