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

struct TextureSourceDocument {
    std::uint32_t width{0};
    std::uint32_t height{0};
    TextureSourcePixelFormat pixel_format{TextureSourcePixelFormat::unknown};
    std::vector<std::uint8_t> bytes;
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

[[nodiscard]] std::string_view audio_source_sample_format_name(AudioSourceSampleFormat sample_format) noexcept;
[[nodiscard]] AudioSourceSampleFormat parse_audio_source_sample_format(std::string_view value);
[[nodiscard]] std::uint32_t audio_source_bytes_per_sample(AudioSourceSampleFormat sample_format);
[[nodiscard]] std::uint64_t audio_source_uncompressed_bytes(const AudioSourceDocument& document);

[[nodiscard]] std::string_view
animation_transform_binding_component_name(AnimationTransformBindingComponent component) noexcept;
[[nodiscard]] AnimationTransformBindingComponent parse_animation_transform_binding_component(std::string_view value);

[[nodiscard]] std::string serialize_texture_source_document(const TextureSourceDocument& document);
[[nodiscard]] TextureSourceDocument deserialize_texture_source_document(std::string_view text);

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
