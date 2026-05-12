// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/scene_renderer/scene_renderer.hpp"

#include "mirakana/animation/morph.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] Transform3D transform_from_world_matrix(const Mat4& world_from_node) noexcept {
    Transform3D transform;
    transform.position =
        Vec3{.x = world_from_node.at(0, 3), .y = world_from_node.at(1, 3), .z = world_from_node.at(2, 3)};

    const auto x_basis =
        Vec3{.x = world_from_node.at(0, 0), .y = world_from_node.at(1, 0), .z = world_from_node.at(2, 0)};
    const auto y_basis =
        Vec3{.x = world_from_node.at(0, 1), .y = world_from_node.at(1, 1), .z = world_from_node.at(2, 1)};
    const auto z_basis =
        Vec3{.x = world_from_node.at(0, 2), .y = world_from_node.at(1, 2), .z = world_from_node.at(2, 2)};
    transform.scale = Vec3{.x = length(x_basis), .y = length(y_basis), .z = length(z_basis)};
    if (transform.scale.x > 0.0F) {
        transform.rotation_radians.z = std::atan2(x_basis.y, x_basis.x);
    }
    return transform;
}

[[nodiscard]] Transform2D sprite_transform_from_world_matrix(const Mat4& world_from_node, Vec2 size) noexcept {
    Transform2D transform;
    transform.position = Vec2{.x = world_from_node.at(0, 3), .y = world_from_node.at(1, 3)};

    const auto x_basis = Vec2{.x = world_from_node.at(0, 0), .y = world_from_node.at(1, 0)};
    const auto y_basis = Vec2{.x = world_from_node.at(0, 1), .y = world_from_node.at(1, 1)};
    transform.scale = Vec2{.x = length(x_basis) * size.x, .y = length(y_basis) * size.y};
    if (transform.scale.x > 0.0F) {
        transform.rotation_radians = std::atan2(x_basis.y, x_basis.x);
    }
    return transform;
}

[[nodiscard]] Color color_from_tint(const std::array<float, 4>& tint) noexcept {
    return Color{.r = tint[0], .g = tint[1], .b = tint[2], .a = tint[3]};
}

[[nodiscard]] bool valid_sprite_animation_target(std::string_view target) noexcept {
    return !target.empty() && target.find('\n') == std::string_view::npos &&
           target.find('\r') == std::string_view::npos && target.find('\0') == std::string_view::npos;
}

[[nodiscard]] const runtime::RuntimeSpriteAnimationFrame*
select_runtime_sprite_animation_frame(const runtime::RuntimeSpriteAnimationPayload& animation, float time_seconds,
                                      std::size_t& selected_frame_index) noexcept {
    if (animation.frames.empty()) {
        return nullptr;
    }

    float total_duration = 0.0F;
    for (const auto& frame : animation.frames) {
        if (std::isfinite(frame.duration_seconds) && frame.duration_seconds > 0.0F) {
            total_duration += frame.duration_seconds;
        }
    }
    if (!(total_duration > 0.0F)) {
        return nullptr;
    }

    float sample_time = std::isfinite(time_seconds) ? time_seconds : 0.0F;
    if (animation.loop) {
        sample_time = std::fmod(sample_time, total_duration);
        if (sample_time < 0.0F) {
            sample_time += total_duration;
        }
    } else {
        sample_time = std::clamp(sample_time, 0.0F, total_duration);
    }

    float cursor = 0.0F;
    for (std::size_t index = 0; index < animation.frames.size(); ++index) {
        const auto& frame = animation.frames[index];
        if (!std::isfinite(frame.duration_seconds) || frame.duration_seconds <= 0.0F) {
            return nullptr;
        }
        cursor += frame.duration_seconds;
        if (sample_time < cursor || index + 1U == animation.frames.size()) {
            selected_frame_index = index;
            return &frame;
        }
    }
    return nullptr;
}

[[nodiscard]] Vec3 normalized_or(Vec3 value, Vec3 fallback) noexcept {
    const auto value_length = length(value);
    if (value_length <= 0.000001F) {
        return fallback;
    }
    return value * (1.0F / value_length);
}

[[nodiscard]] Mat4 view_from_world_matrix(const Mat4& world_from_node) noexcept {
    const auto position =
        Vec3{.x = world_from_node.at(0, 3), .y = world_from_node.at(1, 3), .z = world_from_node.at(2, 3)};
    const auto right =
        normalized_or(Vec3{.x = world_from_node.at(0, 0), .y = world_from_node.at(1, 0), .z = world_from_node.at(2, 0)},
                      Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    const auto up =
        normalized_or(Vec3{.x = world_from_node.at(0, 1), .y = world_from_node.at(1, 1), .z = world_from_node.at(2, 1)},
                      Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F});
    const auto forward =
        normalized_or(Vec3{.x = world_from_node.at(0, 2), .y = world_from_node.at(1, 2), .z = world_from_node.at(2, 2)},
                      Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F});

    Mat4 view = Mat4::identity();
    view.at(0, 0) = right.x;
    view.at(0, 1) = right.y;
    view.at(0, 2) = right.z;
    view.at(0, 3) = -dot(right, position);
    view.at(1, 0) = up.x;
    view.at(1, 1) = up.y;
    view.at(1, 2) = up.z;
    view.at(1, 3) = -dot(up, position);
    view.at(2, 0) = forward.x;
    view.at(2, 1) = forward.y;
    view.at(2, 2) = forward.z;
    view.at(2, 3) = -dot(forward, position);
    return view;
}

[[nodiscard]] Mat4 perspective_clip_from_view(const CameraComponent& camera, float aspect_ratio) {
    const auto tangent = std::tan(camera.vertical_fov_radians * 0.5F);
    if (tangent <= 0.0F || aspect_ratio <= 0.0F) {
        throw std::invalid_argument("camera perspective projection is invalid");
    }

    const auto depth = camera.near_plane - camera.far_plane;
    Mat4 clip;
    clip.at(0, 0) = 1.0F / (aspect_ratio * tangent);
    clip.at(1, 1) = 1.0F / tangent;
    clip.at(2, 2) = camera.far_plane / depth;
    clip.at(2, 3) = (camera.far_plane * camera.near_plane) / depth;
    clip.at(3, 2) = -1.0F;
    return clip;
}

[[nodiscard]] Mat4 orthographic_clip_from_view(const CameraComponent& camera, float aspect_ratio) {
    if (camera.orthographic_height <= 0.0F || aspect_ratio <= 0.0F) {
        throw std::invalid_argument("camera orthographic projection is invalid");
    }

    const auto width = camera.orthographic_height * aspect_ratio;
    const auto depth = camera.near_plane - camera.far_plane;
    Mat4 clip = Mat4::identity();
    clip.at(0, 0) = 2.0F / width;
    clip.at(1, 1) = 2.0F / camera.orthographic_height;
    clip.at(2, 2) = 1.0F / depth;
    clip.at(2, 3) = camera.near_plane / depth;
    return clip;
}

[[nodiscard]] Vec3 scene_light_direction(const Mat4& world_from_node) noexcept {
    const auto local_positive_z =
        normalized_or(Vec3{.x = world_from_node.at(0, 2), .y = world_from_node.at(1, 2), .z = world_from_node.at(2, 2)},
                      Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F});
    return local_positive_z * -1.0F;
}

[[nodiscard]] ShadowMapLightType shadow_light_type_from_scene_light(LightType type) noexcept {
    switch (type) {
    case LightType::directional:
        return ShadowMapLightType::directional;
    case LightType::point:
        return ShadowMapLightType::point;
    case LightType::spot:
        return ShadowMapLightType::spot;
    case LightType::unknown:
        return ShadowMapLightType::unknown;
    }
    return ShadowMapLightType::unknown;
}

[[nodiscard]] std::uint32_t count_to_shadow_uint32(std::size_t count) noexcept {
    if (count > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    return static_cast<std::uint32_t>(count);
}

[[nodiscard]] std::uint32_t select_scene_shadow_light_index(const SceneRenderPacket& packet) noexcept {
    for (std::size_t index = 0; index < packet.lights.size(); ++index) {
        if (packet.lights[index].light.type == LightType::directional && packet.lights[index].light.casts_shadows) {
            return count_to_shadow_uint32(index);
        }
    }
    for (std::size_t index = 0; index < packet.lights.size(); ++index) {
        if (packet.lights[index].light.casts_shadows) {
            return count_to_shadow_uint32(index);
        }
    }
    if (!packet.lights.empty()) {
        return 0;
    }
    return invalid_shadow_map_light_index();
}

[[nodiscard]] bool is_scene_shadow_light_index_available(const SceneRenderPacket& packet,
                                                         std::uint32_t index) noexcept {
    return index != invalid_shadow_map_light_index() && index < packet.lights.size();
}

[[nodiscard]] ShadowMapLightDesc shadow_light_desc_from_scene_packet(const SceneRenderPacket& packet) noexcept {
    const auto index = select_scene_shadow_light_index(packet);
    if (index == invalid_shadow_map_light_index()) {
        return ShadowMapLightDesc{};
    }
    const auto& light = packet.lights[index].light;
    return ShadowMapLightDesc{
        .type = shadow_light_type_from_scene_light(light.type),
        .casts_shadows = light.casts_shadows,
        .source_index = index,
    };
}

[[nodiscard]] Vec3 scene_mesh_center(const SceneRenderMesh& mesh) noexcept {
    return Vec3{
        .x = mesh.world_from_node.at(0, 3), .y = mesh.world_from_node.at(1, 3), .z = mesh.world_from_node.at(2, 3)};
}

struct SceneMeshCenterBounds {
    Vec3 minimum;
    Vec3 maximum;
    bool has_mesh{false};
};

[[nodiscard]] SceneMeshCenterBounds calculate_scene_mesh_center_bounds(const SceneRenderPacket& packet) noexcept {
    SceneMeshCenterBounds bounds;
    for (const auto& mesh : packet.meshes) {
        const auto center = scene_mesh_center(mesh);
        if (!bounds.has_mesh) {
            bounds.minimum = center;
            bounds.maximum = center;
            bounds.has_mesh = true;
            continue;
        }
        bounds.minimum.x = std::min(bounds.minimum.x, center.x);
        bounds.minimum.y = std::min(bounds.minimum.y, center.y);
        bounds.minimum.z = std::min(bounds.minimum.z, center.z);
        bounds.maximum.x = std::max(bounds.maximum.x, center.x);
        bounds.maximum.y = std::max(bounds.maximum.y, center.y);
        bounds.maximum.z = std::max(bounds.maximum.z, center.z);
    }
    return bounds;
}

[[nodiscard]] Vec3 center_of_bounds(const SceneMeshCenterBounds& bounds) noexcept {
    return Vec3{
        .x = (bounds.minimum.x + bounds.maximum.x) * 0.5F,
        .y = (bounds.minimum.y + bounds.maximum.y) * 0.5F,
        .z = (bounds.minimum.z + bounds.maximum.z) * 0.5F,
    };
}

[[nodiscard]] float scene_shadow_focus_radius(const SceneRenderPacket& packet, Vec3 center,
                                              float minimum_focus_radius) noexcept {
    auto radius = std::max(0.0F, minimum_focus_radius);
    for (const auto& mesh : packet.meshes) {
        radius = std::max(radius, length(scene_mesh_center(mesh) - center) + 1.0F);
    }
    return radius;
}

[[nodiscard]] Vec3 scene_shadow_light_direction_from_plan(const SceneRenderPacket& packet,
                                                          const ShadowMapPlan& shadow_map) noexcept {
    if (is_scene_shadow_light_index_available(packet, shadow_map.selected_light_index)) {
        return scene_light_direction(packet.lights[shadow_map.selected_light_index].world_from_node);
    }
    return Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
}

[[nodiscard]] bool is_valid_scene_mesh_gpu_binding(const MeshGpuBinding& binding) noexcept {
    return binding.vertex_buffer.value != 0 && binding.index_buffer.value != 0 && binding.vertex_stride != 0 &&
           binding.vertex_count != 0 && binding.index_count != 0 && binding.index_format != rhi::IndexFormat::unknown &&
           binding.owner_device != nullptr;
}

[[nodiscard]] bool is_valid_scene_material_gpu_binding(const MaterialGpuBinding& binding) noexcept {
    return binding.pipeline_layout.value != 0 && binding.descriptor_set.value != 0 && binding.owner_device != nullptr;
}

[[nodiscard]] bool contains_asset(const std::vector<AssetId>& assets, AssetId asset) noexcept {
    return std::ranges::contains(assets, asset);
}

[[nodiscard]] bool has_load_failure_for(const std::vector<RuntimeSceneRenderLoadFailure>& failures,
                                        AssetId asset) noexcept {
    return std::ranges::any_of(
        failures, [asset](const RuntimeSceneRenderLoadFailure& failure) { return failure.asset == asset; });
}

[[nodiscard]] std::vector<FloatAnimationTrackByteSource>
make_float_track_byte_sources(const AnimationFloatClipSourceDocument& clip) {
    std::vector<FloatAnimationTrackByteSource> sources;
    sources.reserve(clip.tracks.size());
    for (const auto& track : clip.tracks) {
        sources.push_back(FloatAnimationTrackByteSource{
            .target = track.target,
            .time_seconds_bytes =
                std::span<const std::uint8_t>{track.time_seconds_bytes.data(), track.time_seconds_bytes.size()},
            .value_bytes = std::span<const std::uint8_t>{track.value_bytes.data(), track.value_bytes.size()},
        });
    }
    return sources;
}

[[nodiscard]] float read_f32_le(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    const std::uint32_t bits = static_cast<std::uint32_t>(bytes[offset]) |
                               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                               (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

[[nodiscard]] std::vector<Vec3> unpack_vec3_stream(std::span<const std::uint8_t> bytes,
                                                   std::string_view diagnostic_name) {
    if ((bytes.size() % 12U) != 0U) {
        throw std::invalid_argument(std::string{diagnostic_name} + " vec3 byte stream length is invalid");
    }
    std::vector<Vec3> values;
    values.reserve(bytes.size() / 12U);
    for (std::size_t offset = 0; offset < bytes.size(); offset += 12U) {
        values.push_back(Vec3{
            .x = read_f32_le(bytes, offset),
            .y = read_f32_le(bytes, offset + 4U),
            .z = read_f32_le(bytes, offset + 8U),
        });
    }
    return values;
}

[[nodiscard]] std::vector<float> unpack_f32_stream(std::span<const std::uint8_t> bytes,
                                                   std::string_view diagnostic_name) {
    if ((bytes.size() % 4U) != 0U) {
        throw std::invalid_argument(std::string{diagnostic_name} + " f32 byte stream length is invalid");
    }
    std::vector<float> values;
    values.reserve(bytes.size() / 4U);
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4U) {
        values.push_back(read_f32_le(bytes, offset));
    }
    return values;
}

[[nodiscard]] AnimationMorphMeshCpuDesc
animation_morph_mesh_cpu_desc_from_runtime_payload(const MorphMeshCpuSourceDocument& document) {
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("runtime morph mesh CPU payload document is invalid");
    }

    AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = unpack_vec3_stream(document.bind_position_bytes, "runtime morph mesh CPU bind positions");
    desc.bind_normals = unpack_vec3_stream(document.bind_normal_bytes, "runtime morph mesh CPU bind normals");
    desc.bind_tangents = unpack_vec3_stream(document.bind_tangent_bytes, "runtime morph mesh CPU bind tangents");
    desc.target_weights = unpack_f32_stream(document.target_weight_bytes, "runtime morph mesh CPU target weights");
    desc.targets.reserve(document.targets.size());
    for (const auto& target : document.targets) {
        AnimationMorphTargetCpuDesc row;
        row.position_deltas = unpack_vec3_stream(target.position_delta_bytes, "runtime morph mesh CPU position deltas");
        row.normal_deltas = unpack_vec3_stream(target.normal_delta_bytes, "runtime morph mesh CPU normal deltas");
        row.tangent_deltas = unpack_vec3_stream(target.tangent_delta_bytes, "runtime morph mesh CPU tangent deltas");
        desc.targets.push_back(std::move(row));
    }

    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("runtime morph mesh CPU payload decodes to an invalid animation morph mesh");
    }
    return desc;
}

[[nodiscard]] const FloatAnimationCurveSample* find_sample(std::span<const FloatAnimationCurveSample> samples,
                                                           std::string_view target,
                                                           std::size_t* out_index = nullptr) noexcept {
    for (std::size_t index = 0; index < samples.size(); ++index) {
        if (samples[index].target == target) {
            if (out_index != nullptr) {
                *out_index = index;
            }
            return &samples[index];
        }
    }
    return nullptr;
}

void add_load_failure(std::vector<RuntimeSceneRenderLoadFailure>& failures, AssetId asset, std::string diagnostic) {
    if (!has_load_failure_for(failures, asset)) {
        failures.push_back(RuntimeSceneRenderLoadFailure{.asset = asset, .diagnostic = std::move(diagnostic)});
    }
}

[[nodiscard]] bool has_material_payload(const std::vector<runtime::RuntimeMaterialPayload>& materials,
                                        AssetId material) noexcept {
    return std::ranges::any_of(
        materials, [material](const runtime::RuntimeMaterialPayload& payload) { return payload.asset == material; });
}

void append_referenced_material_payload(const runtime::RuntimeAssetPackage& package,
                                        std::vector<runtime::RuntimeMaterialPayload>& materials,
                                        std::vector<RuntimeSceneRenderLoadFailure>& failures, AssetId material) {
    if (material.value == 0) {
        add_load_failure(failures, material, "runtime scene material asset id is invalid");
        return;
    }
    if (has_material_payload(materials, material) || has_load_failure_for(failures, material)) {
        return;
    }

    const auto* record = package.find(material);
    if (record == nullptr) {
        add_load_failure(failures, material, "runtime scene referenced material is missing");
        return;
    }
    if (record->kind != AssetKind::material) {
        add_load_failure(failures, material, "runtime scene referenced asset is not a material");
        return;
    }

    const auto payload = runtime::runtime_material_payload(*record);
    if (!payload.succeeded()) {
        add_load_failure(failures, material, payload.diagnostic);
        return;
    }
    materials.push_back(payload.payload);
}

void add_runtime_scene_diagnostics(std::vector<RuntimeSceneRenderLoadFailure>& failures,
                                   const std::vector<runtime_scene::RuntimeSceneDiagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        add_load_failure(failures, diagnostic.asset, diagnostic.message);
    }
}

void collect_runtime_scene_material_payloads(const runtime::RuntimeAssetPackage& package,
                                             const runtime_scene::RuntimeSceneInstance& instance,
                                             std::vector<runtime::RuntimeMaterialPayload>& materials,
                                             std::vector<RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& reference : instance.references) {
        if (reference.kind == runtime_scene::RuntimeSceneReferenceKind::material) {
            append_referenced_material_payload(package, materials, failures, reference.asset);
        }
    }
}

} // namespace

void pack_scene_pbr_frame_gpu(std::span<std::uint8_t> dst, const ScenePbrFrameGpuPackInput& input) noexcept {
    if (dst.size() < scene_pbr_frame_uniform_packed_bytes) {
        return;
    }
    std::memset(dst.data(), 0, scene_pbr_frame_uniform_packed_bytes);
    std::memcpy(dst.data(), input.camera.clip_from_world.values().data(), 64);
    std::memcpy(&dst[64], input.world_from_node.values().data(), 64);

    const float camera_block[4] = {input.camera_world_position.x, input.camera_world_position.y,
                                   input.camera_world_position.z, input.viewport_aspect};
    std::memcpy(&dst[128], camera_block, sizeof(camera_block));

    Vec3 light_dir{.x = 0.25F, .y = 0.35F, .z = 0.9F};
    float light_intensity = 1.0F;
    Vec3 light_color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    if (input.packet != nullptr) {
        for (const auto& light : input.packet->lights) {
            if (light.light.type == LightType::directional) {
                Vec3 local_positive_z{.x = light.world_from_node.at(0, 2),
                                      .y = light.world_from_node.at(1, 2),
                                      .z = light.world_from_node.at(2, 2)};
                const auto lz_len =
                    std::sqrt((local_positive_z.x * local_positive_z.x) + (local_positive_z.y * local_positive_z.y) +
                              (local_positive_z.z * local_positive_z.z));
                if (lz_len <= 1.0e-6F) {
                    local_positive_z = Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F};
                } else {
                    const auto inv_lz = 1.0F / lz_len;
                    local_positive_z = Vec3{.x = local_positive_z.x * inv_lz,
                                            .y = local_positive_z.y * inv_lz,
                                            .z = local_positive_z.z * inv_lz};
                }
                light_dir = Vec3{
                    .x = local_positive_z.x * -1.0F, .y = local_positive_z.y * -1.0F, .z = local_positive_z.z * -1.0F};
                light_color = light.light.color;
                light_intensity = light.light.intensity;
                break;
            }
        }
    }

    const auto light_length =
        std::sqrt((light_dir.x * light_dir.x) + (light_dir.y * light_dir.y) + (light_dir.z * light_dir.z));
    if (light_length > 1.0e-6F) {
        const auto inv = 1.0F / light_length;
        light_dir = Vec3{.x = light_dir.x * inv, .y = light_dir.y * inv, .z = light_dir.z * inv};
    }

    const float light_block[4] = {light_dir.x, light_dir.y, light_dir.z, light_intensity};
    std::memcpy(&dst[144], light_block, sizeof(light_block));
    const float color_block[4] = {light_color.x, light_color.y, light_color.z, 0.0F};
    std::memcpy(&dst[160], color_block, sizeof(color_block));
    const float ambient_block[4] = {input.ambient_rgb[0], input.ambient_rgb[1], input.ambient_rgb[2], 0.0F};
    std::memcpy(&dst[176], ambient_block, sizeof(ambient_block));
}

bool RuntimeSceneRenderLoadResult::succeeded() const noexcept {
    return failures.empty() && scene.has_value();
}

bool RuntimeSceneRenderInstance::succeeded() const noexcept {
    return failures.empty() && scene.has_value();
}

bool SceneMeshDrawPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

Color color_from_material(const MaterialDefinition& material) noexcept {
    return Color{
        .r = material.factors.base_color[0],
        .g = material.factors.base_color[1],
        .b = material.factors.base_color[2],
        .a = material.factors.base_color[3],
    };
}

bool SceneMaterialPalette::try_add(const MaterialDefinition& material) {
    if (!is_valid_material_definition(material) || find(material.id) != nullptr) {
        return false;
    }
    colors_.push_back(SceneMaterialColor{.material = material.id, .color = color_from_material(material)});
    return true;
}

bool SceneMaterialPalette::try_add_instance(const MaterialDefinition& parent,
                                            const MaterialInstanceDefinition& instance) {
    try {
        return try_add(compose_material_instance(parent, instance));
    } catch (const std::invalid_argument&) {
        return false;
    }
}

void SceneMaterialPalette::add(const MaterialDefinition& material) {
    if (!try_add(material)) {
        throw std::logic_error("scene material color could not be added");
    }
}

void SceneMaterialPalette::add_instance(const MaterialDefinition& parent, const MaterialInstanceDefinition& instance) {
    if (!try_add_instance(parent, instance)) {
        throw std::logic_error("scene material instance color could not be added");
    }
}

const Color* SceneMaterialPalette::find(AssetId material) const noexcept {
    for (const auto& color : colors_) {
        if (color.material == material) {
            return &color.color;
        }
    }
    return nullptr;
}

std::size_t SceneMaterialPalette::count() const noexcept {
    return colors_.size();
}

bool SceneGpuBindingPalette::try_add_mesh(AssetId mesh, MeshGpuBinding binding) {
    if (mesh.value == 0 || !is_valid_scene_mesh_gpu_binding(binding) || find_mesh(mesh) != nullptr) {
        return false;
    }
    meshes_.push_back(SceneMeshGpuBinding{.mesh = mesh, .binding = binding});
    return true;
}

bool SceneGpuBindingPalette::try_add_material(AssetId material, MaterialGpuBinding binding) {
    if (material.value == 0 || !is_valid_scene_material_gpu_binding(binding) || find_material(material) != nullptr) {
        return false;
    }
    materials_.push_back(SceneMaterialGpuBinding{.material = material, .binding = binding});
    return true;
}

void SceneGpuBindingPalette::add_mesh(AssetId mesh, MeshGpuBinding binding) {
    if (!try_add_mesh(mesh, binding)) {
        throw std::logic_error("scene mesh gpu binding could not be added");
    }
}

void SceneGpuBindingPalette::add_material(AssetId material, MaterialGpuBinding binding) {
    if (!try_add_material(material, binding)) {
        throw std::logic_error("scene material gpu binding could not be added");
    }
}

const MeshGpuBinding* SceneGpuBindingPalette::find_mesh(AssetId mesh) const noexcept {
    for (const auto& item : meshes_) {
        if (item.mesh == mesh) {
            return &item.binding;
        }
    }
    return nullptr;
}

const MaterialGpuBinding* SceneGpuBindingPalette::find_material(AssetId material) const noexcept {
    for (const auto& item : materials_) {
        if (item.material == material) {
            return &item.binding;
        }
    }
    return nullptr;
}

std::size_t SceneGpuBindingPalette::mesh_count() const noexcept {
    return meshes_.size();
}

std::size_t SceneGpuBindingPalette::material_count() const noexcept {
    return materials_.size();
}

[[nodiscard]] bool is_valid_scene_skinned_mesh_gpu_binding(const SkinnedMeshGpuBinding& binding) noexcept {
    return is_valid_scene_mesh_gpu_binding(binding.mesh) && binding.joint_palette_buffer.value != 0 &&
           binding.joint_descriptor_set.value != 0 && binding.joint_count != 0 && binding.owner_device != nullptr;
}

bool SceneSkinnedGpuBindingPalette::try_add_skinned_mesh(AssetId mesh, SkinnedMeshGpuBinding binding) {
    if (mesh.value == 0 || !is_valid_scene_skinned_mesh_gpu_binding(binding) || find_skinned_mesh(mesh) != nullptr) {
        return false;
    }
    skinned_meshes_.push_back(SceneSkinnedMeshGpuBinding{.mesh = mesh, .binding = binding});
    return true;
}

void SceneSkinnedGpuBindingPalette::add_skinned_mesh(AssetId mesh, SkinnedMeshGpuBinding binding) {
    if (!try_add_skinned_mesh(mesh, binding)) {
        throw std::logic_error("scene skinned mesh gpu binding could not be added");
    }
}

const SkinnedMeshGpuBinding* SceneSkinnedGpuBindingPalette::find_skinned_mesh(AssetId mesh) const noexcept {
    for (const auto& item : skinned_meshes_) {
        if (item.mesh == mesh) {
            return &item.binding;
        }
    }
    return nullptr;
}

std::size_t SceneSkinnedGpuBindingPalette::skinned_mesh_count() const noexcept {
    return skinned_meshes_.size();
}

std::span<const SceneSkinnedMeshGpuBinding> SceneSkinnedGpuBindingPalette::skinned_entries() const noexcept {
    return skinned_meshes_;
}

[[nodiscard]] bool is_valid_scene_morph_mesh_gpu_binding(const MorphMeshGpuBinding& binding) noexcept {
    const bool normal_stream_valid = (binding.normal_delta_buffer.value != 0) == (binding.normal_delta_bytes != 0);
    const bool tangent_stream_valid = (binding.tangent_delta_buffer.value != 0) == (binding.tangent_delta_bytes != 0);
    return binding.position_delta_buffer.value != 0 && binding.morph_weight_buffer.value != 0 &&
           binding.morph_descriptor_set.value != 0 && binding.vertex_count != 0 && binding.target_count != 0 &&
           binding.position_delta_bytes != 0 && binding.morph_weight_uniform_allocation_bytes != 0 &&
           binding.owner_device != nullptr && normal_stream_valid && tangent_stream_valid;
}

bool SceneMorphGpuBindingPalette::try_add_morph_mesh(AssetId morph_mesh, MorphMeshGpuBinding binding) {
    if (morph_mesh.value == 0 || !is_valid_scene_morph_mesh_gpu_binding(binding) ||
        find_morph_mesh(morph_mesh) != nullptr) {
        return false;
    }
    morph_meshes_.push_back(SceneMorphMeshGpuBinding{.morph_mesh = morph_mesh, .binding = binding});
    return true;
}

void SceneMorphGpuBindingPalette::add_morph_mesh(AssetId morph_mesh, MorphMeshGpuBinding binding) {
    if (!try_add_morph_mesh(morph_mesh, binding)) {
        throw std::logic_error("scene morph mesh gpu binding could not be added");
    }
}

const MorphMeshGpuBinding* SceneMorphGpuBindingPalette::find_morph_mesh(AssetId morph_mesh) const noexcept {
    for (const auto& item : morph_meshes_) {
        if (item.morph_mesh == morph_mesh) {
            return &item.binding;
        }
    }
    return nullptr;
}

std::size_t SceneMorphGpuBindingPalette::morph_mesh_count() const noexcept {
    return morph_meshes_.size();
}

std::span<const SceneMorphMeshGpuBinding> SceneMorphGpuBindingPalette::morph_entries() const noexcept {
    return morph_meshes_;
}

Color resolve_scene_mesh_color(const SceneRenderMesh& mesh, const SceneRenderSubmitDesc& desc) noexcept {
    if (desc.material_palette != nullptr) {
        if (const auto* color = desc.material_palette->find(mesh.renderer.material); color != nullptr) {
            return *color;
        }
    }
    return desc.fallback_mesh_color;
}

SceneCameraMatrices make_scene_camera_matrices(const SceneRenderCamera& camera, float aspect_ratio) {
    if (!is_valid_camera_component(camera.camera)) {
        throw std::invalid_argument("scene camera is invalid");
    }

    const auto view = view_from_world_matrix(camera.world_from_node);
    Mat4 clip;
    switch (camera.camera.projection) {
    case CameraProjectionMode::perspective:
        clip = perspective_clip_from_view(camera.camera, aspect_ratio);
        break;
    case CameraProjectionMode::orthographic:
        clip = orthographic_clip_from_view(camera.camera, aspect_ratio);
        break;
    case CameraProjectionMode::unknown:
        throw std::invalid_argument("scene camera projection is unsupported");
    }

    return SceneCameraMatrices{
        .view_from_world = view,
        .clip_from_view = clip,
        .clip_from_world = clip * view,
    };
}

SceneLightCommand make_scene_light_command(const SceneRenderLight& light) noexcept {
    return SceneLightCommand{
        .type = light.light.type,
        .position = Vec3{.x = light.world_from_node.at(0, 3),
                         .y = light.world_from_node.at(1, 3),
                         .z = light.world_from_node.at(2, 3)},
        .direction = scene_light_direction(light.world_from_node),
        .color = Color{.r = light.light.color.x, .g = light.light.color.y, .b = light.light.color.z, .a = 1.0F},
        .intensity = light.light.intensity,
        .range = light.light.range,
        .inner_cone_radians = light.light.inner_cone_radians,
        .outer_cone_radians = light.light.outer_cone_radians,
        .casts_shadows = light.light.casts_shadows,
    };
}

ShadowMapPlan build_scene_shadow_map_plan(const SceneRenderPacket& packet, SceneShadowMapDesc desc) {
    return build_shadow_map_plan(ShadowMapDesc{
        .light = shadow_light_desc_from_scene_packet(packet),
        .extent = desc.extent,
        .depth_format = desc.depth_format,
        .caster_count = count_to_shadow_uint32(packet.meshes.size()),
        .receiver_count = count_to_shadow_uint32(packet.meshes.size()),
        .directional_cascade_count = desc.directional_cascade_count,
    });
}

DirectionalShadowLightSpacePlan build_scene_directional_shadow_light_space_plan(const SceneRenderPacket& packet,
                                                                                const ShadowMapPlan& shadow_map,
                                                                                SceneShadowLightSpaceDesc desc) {
    const auto bounds = calculate_scene_mesh_center_bounds(packet);
    const auto focus_center = bounds.has_mesh ? center_of_bounds(bounds) : Vec3{};
    const auto focus_radius = scene_shadow_focus_radius(packet, focus_center, desc.minimum_focus_radius);
    const auto depth_radius =
        std::isfinite(desc.depth_padding) ? focus_radius + std::max(0.0F, desc.depth_padding) : -1.0F;

    DirectionalShadowCascadeCameraInput cascade_camera{};
    const DirectionalShadowCascadeCameraInput* cascade_camera_ptr = nullptr;
    if (shadow_map.directional_cascade_count > 1) {
        if (const auto* primary = packet.primary_camera(); primary != nullptr) {
            cascade_camera.world_from_camera = primary->world_from_node;
            cascade_camera.perspective = primary->camera.projection == CameraProjectionMode::perspective;
            cascade_camera.vertical_fov_radians = primary->camera.vertical_fov_radians;
            cascade_camera.orthographic_height = primary->camera.orthographic_height;
            cascade_camera.near_plane = primary->camera.near_plane;
            cascade_camera.far_plane = primary->camera.far_plane;
            cascade_camera.viewport_aspect = desc.viewport_aspect;
            cascade_camera.valid = is_valid_camera_component(primary->camera);
            cascade_camera_ptr = &cascade_camera;
        }
    }

    return build_directional_shadow_light_space_plan(DirectionalShadowLightSpaceDesc{
        .shadow_map = &shadow_map,
        .light_direction = scene_shadow_light_direction_from_plan(packet, shadow_map),
        .focus_center = focus_center,
        .focus_radius = focus_radius,
        .depth_radius = depth_radius,
        .texel_snap = desc.texel_snap,
        .cascade_camera = cascade_camera_ptr,
        .cascade_split_lambda = desc.cascade_split_lambda,
    });
}

MeshCommand make_scene_mesh_command(const SceneRenderMesh& mesh, Color color) noexcept {
    return MeshCommand{
        .transform = transform_from_world_matrix(mesh.world_from_node),
        .color = color,
        .mesh = mesh.renderer.mesh,
        .material = mesh.renderer.material,
        .world_from_node = mesh.world_from_node,
    };
}

MeshCommand make_scene_mesh_command(const SceneRenderMesh& mesh, const SceneRenderSubmitDesc& desc) noexcept {
    auto command = make_scene_mesh_command(mesh, resolve_scene_mesh_color(mesh, desc));
    if (desc.gpu_bindings != nullptr) {
        if (const auto* mesh_binding = desc.gpu_bindings->find_mesh(mesh.renderer.mesh); mesh_binding != nullptr) {
            command.mesh_binding = *mesh_binding;
        }
        if (const auto* material_binding = desc.gpu_bindings->find_material(mesh.renderer.material);
            material_binding != nullptr) {
            command.material_binding = *material_binding;
        }
    }
    if (desc.skinned_gpu_bindings != nullptr) {
        if (const auto* skinned = desc.skinned_gpu_bindings->find_skinned_mesh(mesh.renderer.mesh);
            skinned != nullptr) {
            command.gpu_skinning = true;
            command.skinned_mesh = *skinned;
            command.mesh_binding = skinned->mesh;
        }
    }
    return command;
}

SceneMeshDrawPlan plan_scene_mesh_draws(const SceneRenderPacket& packet) {
    SceneMeshDrawPlan plan;
    plan.mesh_count = packet.meshes.size();

    std::vector<AssetId> unique_meshes;
    std::vector<AssetId> unique_materials;
    unique_meshes.reserve(packet.meshes.size());
    unique_materials.reserve(packet.meshes.size());

    for (std::size_t index = 0; index < packet.meshes.size(); ++index) {
        const auto& mesh = packet.meshes[index];
        bool draw_ready = true;

        if (mesh.renderer.mesh.value == 0) {
            draw_ready = false;
            plan.diagnostics.push_back(SceneMeshDrawPlanDiagnostic{
                .code = SceneMeshDrawPlanDiagnosticCode::invalid_mesh_asset,
                .mesh_index = index,
                .asset = mesh.renderer.mesh,
                .message = "scene mesh draw references an invalid mesh asset id",
            });
        } else if (!contains_asset(unique_meshes, mesh.renderer.mesh)) {
            unique_meshes.push_back(mesh.renderer.mesh);
        }

        if (mesh.renderer.material.value == 0) {
            draw_ready = false;
            plan.diagnostics.push_back(SceneMeshDrawPlanDiagnostic{
                .code = SceneMeshDrawPlanDiagnosticCode::invalid_material_asset,
                .mesh_index = index,
                .asset = mesh.renderer.material,
                .message = "scene mesh draw references an invalid material asset id",
            });
        } else if (!contains_asset(unique_materials, mesh.renderer.material)) {
            unique_materials.push_back(mesh.renderer.material);
        }

        if (draw_ready) {
            ++plan.draw_count;
        }
    }

    plan.unique_mesh_count = unique_meshes.size();
    plan.unique_material_count = unique_materials.size();
    return plan;
}

SpriteCommand make_scene_sprite_command(const SceneRenderSprite& sprite) noexcept {
    return SpriteCommand{
        .transform = sprite_transform_from_world_matrix(sprite.world_from_node, sprite.renderer.size),
        .color = color_from_tint(sprite.renderer.tint),
        .texture = SpriteTextureRegion{.enabled = true,
                                       .atlas_page = sprite.renderer.sprite,
                                       .uv_rect = SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
    };
}

SpriteBatchPlan plan_scene_sprite_batches(const SceneRenderPacket& packet) {
    std::vector<SpriteCommand> sprites;
    sprites.reserve(packet.sprites.size());
    for (const auto& sprite : packet.sprites) {
        sprites.push_back(make_scene_sprite_command(sprite));
    }
    return plan_sprite_batches(sprites);
}

RuntimeSceneRenderLoadResult load_runtime_scene_render_data(const runtime::RuntimeAssetPackage& package,
                                                            AssetId scene) {
    RuntimeSceneRenderLoadResult result;

    auto loaded_scene = runtime_scene::instantiate_runtime_scene(package, scene);
    add_runtime_scene_diagnostics(result.failures, loaded_scene.diagnostics);

    std::vector<runtime::RuntimeMaterialPayload> materials;
    if (loaded_scene.instance.has_value()) {
        collect_runtime_scene_material_payloads(package, *loaded_scene.instance, materials, result.failures);
        result.scene = std::move(loaded_scene.instance->scene);
    }

    for (const auto& material : materials) {
        if (!result.material_palette.try_add(material.material)) {
            add_load_failure(result.failures, material.asset, "runtime scene material palette rejected material");
        }
    }

    return result;
}

RuntimeSceneRenderInstance instantiate_runtime_scene_render_data(const runtime::RuntimeAssetPackage& package,
                                                                 AssetId scene) {
    auto loaded = load_runtime_scene_render_data(package, scene);
    RuntimeSceneRenderInstance instance;
    instance.scene = std::move(loaded.scene);
    instance.material_palette = std::move(loaded.material_palette);
    instance.failures = std::move(loaded.failures);
    if (instance.scene.has_value()) {
        try {
            instance.render_packet = build_scene_render_packet(*instance.scene);
        } catch (const std::exception& error) {
            add_load_failure(instance.failures, scene,
                             std::string("runtime scene render packet build failed: ") + error.what());
        }
    }
    return instance;
}

RuntimeSceneRenderAnimationApplyResult sample_and_apply_runtime_scene_render_animation_float_clip(
    RuntimeSceneRenderInstance& instance, const AnimationFloatClipSourceDocument& clip,
    const AnimationTransformBindingSourceDocument& binding_source, float time_seconds) {
    RuntimeSceneRenderAnimationApplyResult result;
    if (!instance.scene.has_value()) {
        result.diagnostic = "runtime scene render instance has no scene";
        return result;
    }
    if (!is_valid_animation_float_clip_source_document(clip)) {
        result.diagnostic = "runtime scene render animation clip is invalid";
        return result;
    }

    std::vector<FloatAnimationTrack> tracks;
    try {
        tracks = make_float_animation_tracks_from_f32_bytes(make_float_track_byte_sources(clip));
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime scene render animation clip decode failed: "} + error.what();
        return result;
    }

    const auto samples = sample_float_animation_tracks(tracks, time_seconds);
    result.sampled_track_count = samples.size();

    runtime_scene::RuntimeSceneInstance runtime_instance{
        .scene_asset = AssetId{},
        .handle = runtime::RuntimeAssetHandle{},
        .scene = *instance.scene,
        .references = {},
    };
    const auto apply_result =
        runtime_scene::apply_runtime_scene_animation_transform_samples(runtime_instance, binding_source, samples);
    if (!apply_result.succeeded) {
        result.diagnostic = apply_result.diagnostic;
        result.applied_sample_count = apply_result.applied_sample_count;
        result.binding_diagnostics = apply_result.binding_diagnostics;
        return result;
    }

    try {
        auto rebuilt_packet = build_scene_render_packet(runtime_instance.scene);
        instance.scene = std::move(runtime_instance.scene);
        instance.render_packet = std::move(rebuilt_packet);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime scene render packet rebuild failed: "} + error.what();
        return result;
    }

    result.succeeded = true;
    result.applied_sample_count = apply_result.applied_sample_count;
    return result;
}

RuntimeSceneRenderAnimationApplyResult sample_and_apply_runtime_scene_render_animation_pose_3d(
    RuntimeSceneRenderInstance& instance, const AnimationSkeleton3dDesc& skeleton,
    const std::vector<AnimationJointTrack3dDesc>& tracks, float time_seconds) {
    RuntimeSceneRenderAnimationApplyResult result;
    if (!instance.scene.has_value()) {
        result.diagnostic = "runtime scene render instance has no scene";
        return result;
    }

    AnimationPose3d pose;
    try {
        pose = sample_animation_local_pose_3d(skeleton, tracks, time_seconds);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime scene render animation 3D pose sample failed: "} + error.what();
        return result;
    }
    result.sampled_track_count = tracks.size();

    runtime_scene::RuntimeSceneInstance runtime_instance{
        .scene_asset = AssetId{},
        .handle = runtime::RuntimeAssetHandle{},
        .scene = *instance.scene,
        .references = {},
    };
    const auto apply_result = runtime_scene::apply_runtime_scene_animation_pose_3d(runtime_instance, skeleton, pose);
    if (!apply_result.succeeded) {
        result.diagnostic = apply_result.diagnostic;
        result.applied_sample_count = apply_result.applied_sample_count;
        result.binding_diagnostics = apply_result.binding_diagnostics;
        return result;
    }

    try {
        auto rebuilt_packet = build_scene_render_packet(runtime_instance.scene);
        instance.scene = std::move(runtime_instance.scene);
        instance.render_packet = std::move(rebuilt_packet);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime scene render packet rebuild failed: "} + error.what();
        return result;
    }

    result.succeeded = true;
    result.applied_sample_count = apply_result.applied_sample_count;
    return result;
}

RuntimeSceneRenderSpriteAnimationApplyResult sample_and_apply_runtime_scene_render_sprite_animation(
    RuntimeSceneRenderInstance& instance, const runtime::RuntimeSpriteAnimationPayload& animation, float time_seconds) {
    RuntimeSceneRenderSpriteAnimationApplyResult result;
    if (!instance.scene.has_value()) {
        result.diagnostic = "runtime scene render instance has no scene";
        return result;
    }
    if (!valid_sprite_animation_target(animation.target_node)) {
        result.diagnostic = "runtime sprite animation target node is invalid";
        return result;
    }

    const runtime::RuntimeSpriteAnimationFrame* frame = nullptr;
    std::size_t selected_frame_index = 0;
    frame = select_runtime_sprite_animation_frame(animation, time_seconds, selected_frame_index);
    if (frame == nullptr) {
        result.diagnostic = "runtime sprite animation has no valid frame";
        return result;
    }
    result.sampled_frame_count = 1;
    result.selected_frame_index = selected_frame_index;

    SceneNodeId target_node_id{};
    for (const auto& node : instance.scene->nodes()) {
        if (node.name == animation.target_node) {
            target_node_id = node.id;
            break;
        }
    }
    auto* target_node = instance.scene->find_node(target_node_id);
    if (target_node == nullptr) {
        result.diagnostic = "runtime sprite animation target node was not found";
        return result;
    }
    if (!target_node->components.sprite_renderer.has_value()) {
        result.diagnostic = "runtime sprite animation target node has no sprite renderer";
        return result;
    }

    auto renderer = *target_node->components.sprite_renderer;
    renderer.sprite = frame->sprite;
    renderer.material = frame->material;
    renderer.size = Vec2{.x = frame->size[0], .y = frame->size[1]};
    renderer.tint = frame->tint;
    if (!is_valid_sprite_renderer_component(renderer)) {
        result.diagnostic = "runtime sprite animation frame produced an invalid sprite renderer";
        return result;
    }
    target_node->components.sprite_renderer = renderer;

    try {
        instance.render_packet = build_scene_render_packet(*instance.scene);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime scene render packet rebuild failed: "} + error.what();
        return result;
    }

    result.succeeded = true;
    result.applied_frame_count = 1;
    return result;
}

RuntimeMorphMeshCpuAnimationSampleResult
sample_runtime_morph_mesh_cpu_animation_float_clip(const runtime::RuntimeMorphMeshCpuPayload& morph,
                                                   const AnimationFloatClipSourceDocument& clip,
                                                   std::string_view target_prefix, float time_seconds) {
    RuntimeMorphMeshCpuAnimationSampleResult result;
    if (target_prefix.empty()) {
        result.diagnostic = "runtime morph mesh animation target prefix is empty";
        return result;
    }
    if (!is_valid_morph_mesh_cpu_source_document(morph.morph)) {
        result.diagnostic = "runtime morph mesh CPU payload is invalid";
        return result;
    }
    if (!is_valid_animation_float_clip_source_document(clip)) {
        result.diagnostic = "runtime morph mesh animation clip is invalid";
        return result;
    }

    std::vector<FloatAnimationTrack> tracks;
    try {
        tracks = make_float_animation_tracks_from_f32_bytes(make_float_track_byte_sources(clip));
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime morph mesh animation clip decode failed: "} + error.what();
        return result;
    }

    const auto samples = sample_float_animation_tracks(tracks, time_seconds);
    result.sampled_track_count = samples.size();

    AnimationMorphMeshCpuDesc morph_desc;
    try {
        morph_desc = animation_morph_mesh_cpu_desc_from_runtime_payload(morph.morph);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime morph mesh CPU payload decode failed: "} + error.what();
        return result;
    }

    std::vector<bool> used_samples(samples.size(), false);
    std::vector<float> target_weights;
    target_weights.reserve(morph_desc.targets.size());
    for (std::size_t index = 0; index < morph_desc.targets.size(); ++index) {
        const auto target = std::string{target_prefix} + std::to_string(index);
        std::size_t sample_index = 0;
        const auto* sample = find_sample(samples, target, &sample_index);
        if (sample == nullptr) {
            result.diagnostic = "missing morph weight sample: " + target;
            return result;
        }
        if (!std::isfinite(sample->value) || sample->value < 0.0F || sample->value > 1.0F) {
            result.diagnostic = "invalid morph weight sample: " + target;
            return result;
        }
        used_samples[sample_index] = true;
        target_weights.push_back(sample->value);
    }

    for (std::size_t index = 0; index < used_samples.size(); ++index) {
        if (!used_samples[index]) {
            result.diagnostic = "unused morph weight sample: " + samples[index].target;
            return result;
        }
    }

    morph_desc.target_weights = target_weights;
    result.applied_weight_count = target_weights.size();
    result.target_weights = std::move(target_weights);

    const auto diagnostics = validate_animation_morph_mesh_cpu_desc(morph_desc);
    if (!diagnostics.empty()) {
        result.diagnostic = "runtime morph mesh CPU animation weights are invalid: " + diagnostics.front().message;
        return result;
    }

    try {
        result.morphed_positions = apply_animation_morph_targets_positions_cpu(morph_desc);
    } catch (const std::exception& error) {
        result.diagnostic = std::string{"runtime morph mesh CPU position evaluation failed: "} + error.what();
        return result;
    }

    result.succeeded = true;
    return result;
}

SceneRenderSubmitResult submit_scene_render_packet(IRenderer& renderer, const SceneRenderPacket& packet,
                                                   SceneRenderSubmitDesc desc) {
    SceneRenderSubmitResult result;
    result.cameras_available = packet.cameras.size();
    result.lights_available = packet.lights.size();
    result.has_primary_camera = packet.primary_camera() != nullptr;

    for (const auto& mesh : packet.meshes) {
        if (desc.material_palette != nullptr && desc.material_palette->find(mesh.renderer.material) != nullptr) {
            ++result.material_colors_resolved;
        }
        if (desc.gpu_bindings != nullptr) {
            if (desc.gpu_bindings->find_mesh(mesh.renderer.mesh) != nullptr) {
                ++result.mesh_gpu_bindings_resolved;
            }
            if (desc.gpu_bindings->find_material(mesh.renderer.material) != nullptr) {
                ++result.material_gpu_bindings_resolved;
            }
        }
        if (desc.skinned_gpu_bindings != nullptr &&
            desc.skinned_gpu_bindings->find_skinned_mesh(mesh.renderer.mesh) != nullptr) {
            ++result.skinned_mesh_gpu_bindings_resolved;
        }
        if (desc.pbr_gpu != nullptr && desc.pbr_gpu->device != nullptr &&
            desc.pbr_gpu->scene_frame_uniform.value != 0) {
            std::array<std::uint8_t, scene_pbr_frame_uniform_packed_bytes> pbr_bytes{};
            pack_scene_pbr_frame_gpu(pbr_bytes, ScenePbrFrameGpuPackInput{
                                                    .packet = &packet,
                                                    .camera = desc.pbr_gpu->camera,
                                                    .world_from_node = mesh.world_from_node,
                                                    .camera_world_position = desc.pbr_gpu->camera_world_position,
                                                    .ambient_rgb = desc.pbr_gpu->ambient_rgb,
                                                    .viewport_aspect = desc.pbr_gpu->viewport_aspect,
                                                });
            desc.pbr_gpu->device->write_buffer(desc.pbr_gpu->scene_frame_uniform, 0, pbr_bytes);
        }
        renderer.draw_mesh(make_scene_mesh_command(mesh, desc));
        ++result.meshes_submitted;
    }

    for (const auto& sprite : packet.sprites) {
        renderer.draw_sprite(make_scene_sprite_command(sprite));
        ++result.sprites_submitted;
    }

    return result;
}

} // namespace mirakana
