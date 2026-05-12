// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::Scene make_renderable_scene() {
    mirakana::Scene scene("level-01");
    const auto root = scene.create_node("Root");
    const auto camera_node = scene.create_node("MainCamera");
    const auto mesh_node = scene.create_node("Player");
    const auto hidden_mesh_node = scene.create_node("Hidden");
    const auto sprite_node = scene.create_node("Nameplate");
    const auto hidden_sprite_node = scene.create_node("HiddenSprite");

    scene.find_node(root)->transform.position = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F};
    scene.find_node(root)->transform.scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F};
    scene.find_node(camera_node)->transform.position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    scene.find_node(mesh_node)->transform.position = mirakana::Vec3{.x = 0.0F, .y = 3.0F, .z = 0.0F};
    scene.find_node(sprite_node)->transform.position = mirakana::Vec3{.x = 2.0F, .y = 1.0F, .z = 0.0F};
    scene.set_parent(camera_node, root);
    scene.set_parent(mesh_node, root);
    scene.set_parent(sprite_node, root);

    mirakana::SceneNodeComponents camera_components;
    camera_components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::perspective,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 10.0F,
        .near_plane = 0.1F,
        .far_plane = 500.0F,
        .primary = true,
    };
    scene.set_components(camera_node, camera_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    mirakana::SceneNodeComponents hidden_mesh_components;
    hidden_mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/hidden"),
        .material = mirakana::AssetId::from_name("materials/hidden"),
        .visible = false,
    };
    scene.set_components(hidden_mesh_node, hidden_mesh_components);

    mirakana::SceneNodeComponents sprite_components;
    sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/nameplate"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 3.0F, .y = 0.5F},
        .tint = {0.2F, 0.4F, 0.6F, 1.0F},
        .visible = true,
    };
    scene.set_components(sprite_node, sprite_components);

    mirakana::SceneNodeComponents hidden_sprite_components;
    hidden_sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/hidden"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = false,
    };
    scene.set_components(hidden_sprite_node, hidden_sprite_components);

    return scene;
}

[[nodiscard]] std::string package_mesh_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=mesh\n"
           "source.path=source/meshes/player.mesh_source\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=false\n"
           "mesh.has_uvs=false\n"
           "mesh.has_tangent_frame=false\n";
}

[[nodiscard]] mirakana::Scene make_package_renderable_scene(mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::Scene scene("PackageLevel");
    const auto node = scene.create_node("Player");

    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(node, components);

    return scene;
}

[[nodiscard]] mirakana::Scene make_transform_animation_scene(mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::Scene scene("AnimatedPackageLevel");
    const auto node = scene.create_node("PackagedMesh");

    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(node, components);

    return scene;
}

[[nodiscard]] mirakana::AnimationFloatClipSourceDocument make_translation_x_animation_clip() {
    mirakana::AnimationFloatClipSourceDocument document;
    document.tracks.push_back(mirakana::AnimationFloatClipTrackSourceDocument{
        .target = "gltf/node/0/translation/x",
        .time_seconds_bytes =
            std::vector<std::uint8_t>{
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x80,
                0x3f,
            },
        .value_bytes =
            std::vector<std::uint8_t>{
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x3f,
            },
    });
    return document;
}

[[nodiscard]] mirakana::AnimationTransformBindingSourceDocument make_packaged_mesh_translation_binding() {
    mirakana::AnimationTransformBindingSourceDocument document;
    document.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/0/translation/x",
        .node_name = "PackagedMesh",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    return document;
}

[[nodiscard]] mirakana::AnimationSkeleton3dDesc make_packaged_mesh_quaternion_skeleton() {
    return mirakana::AnimationSkeleton3dDesc{
        std::vector<mirakana::AnimationSkeleton3dJointDesc>{
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "PackagedMesh", .parent_index = mirakana::animation_no_parent, .rest = {}},
        },
    };
}

[[nodiscard]] std::vector<mirakana::AnimationJointTrack3dDesc> make_packaged_mesh_quaternion_tracks() {
    mirakana::AnimationJointTrack3dDesc track;
    track.joint_index = 0;
    track.rotation_keyframes.push_back(
        mirakana::QuatKeyframe{.time_seconds = 0.0F, .value = mirakana::Quat::identity()});
    track.rotation_keyframes.push_back(mirakana::QuatKeyframe{
        .time_seconds = 1.0F,
        .value = mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}, 1.57079637F)});
    return std::vector<mirakana::AnimationJointTrack3dDesc>{std::move(track)};
}

[[nodiscard]] mirakana::Scene make_sprite_animation_scene(mirakana::AssetId sprite, mirakana::AssetId material) {
    mirakana::Scene scene("SpriteAnimationPackageLevel");
    const auto node = scene.create_node("Player");

    mirakana::SceneNodeComponents components;
    components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = sprite,
        .material = material,
        .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = true,
    };
    scene.set_components(node, components);

    return scene;
}

[[nodiscard]] mirakana::runtime::RuntimeSpriteAnimationPayload
make_runtime_sprite_animation_payload(mirakana::AssetId sprite, mirakana::AssetId material) {
    mirakana::runtime::RuntimeSpriteAnimationPayload payload;
    payload.asset = mirakana::AssetId::from_name("animations/player-idle");
    payload.handle = mirakana::runtime::RuntimeAssetHandle{7};
    payload.target_node = "Player";
    payload.loop = true;
    payload.frames.push_back(mirakana::runtime::RuntimeSpriteAnimationFrame{
        .duration_seconds = 0.25F,
        .sprite = sprite,
        .material = material,
        .size = {1.0F, 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
    });
    payload.frames.push_back(mirakana::runtime::RuntimeSpriteAnimationFrame{
        .duration_seconds = 0.25F,
        .sprite = sprite,
        .material = material,
        .size = {2.0F, 1.25F},
        .tint = {1.0F, 0.4F, 0.2F, 1.0F},
    });
    return payload;
}

void append_f32_le(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    bytes.push_back(static_cast<std::uint8_t>(bits & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xffU));
}

void append_vec3_le(std::vector<std::uint8_t>& bytes, mirakana::Vec3 value) {
    append_f32_le(bytes, value.x);
    append_f32_le(bytes, value.y);
    append_f32_le(bytes, value.z);
}

[[nodiscard]] mirakana::runtime::RuntimeMorphMeshCpuPayload make_runtime_morph_mesh_payload() {
    mirakana::MorphMeshCpuSourceDocument document;
    document.vertex_count = 1;
    append_vec3_le(document.bind_position_bytes, mirakana::Vec3{.x = -0.5F, .y = 0.0F, .z = 0.0F});
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3_le(target.position_delta_bytes, mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    document.targets.push_back(std::move(target));
    append_f32_le(document.target_weight_bytes, 0.0F);
    MK_REQUIRE(mirakana::is_valid_morph_mesh_cpu_source_document(document));
    return mirakana::runtime::RuntimeMorphMeshCpuPayload{
        .asset = mirakana::AssetId::from_name("morphs/packaged-mesh"),
        .handle = mirakana::runtime::RuntimeAssetHandle{42},
        .morph = std::move(document),
    };
}

[[nodiscard]] mirakana::AnimationFloatClipSourceDocument make_morph_weight_animation_clip() {
    mirakana::AnimationFloatClipSourceDocument document;
    std::vector<std::uint8_t> times;
    append_f32_le(times, 0.0F);
    append_f32_le(times, 1.0F);
    std::vector<std::uint8_t> values;
    append_f32_le(values, 0.0F);
    append_f32_le(values, 0.5F);
    document.tracks.push_back(mirakana::AnimationFloatClipTrackSourceDocument{
        .target = "gltf/node/0/weights/0",
        .time_seconds_bytes = std::move(times),
        .value_bytes = std::move(values),
    });
    MK_REQUIRE(mirakana::is_valid_animation_float_clip_source_document(document));
    return document;
}

} // namespace

MK_TEST("scene renderer converts scene mesh packets into mesh commands") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::Color{.r = 0.25F, .g = 0.5F, .b = 0.75F, .a = 1.0F});

    MK_REQUIRE(command.mesh == mirakana::AssetId::from_name("meshes/player"));
    MK_REQUIRE(command.material == mirakana::AssetId::from_name("materials/player"));
    MK_REQUIRE(command.transform.position == (mirakana::Vec3{10.0F, 6.0F, 0.0F}));
    MK_REQUIRE(command.transform.scale == (mirakana::Vec3{2.0F, 2.0F, 2.0F}));
    MK_REQUIRE(command.color.r == 0.25F);
    MK_REQUIRE(command.color.g == 0.5F);
    MK_REQUIRE(command.color.b == 0.75F);
}

MK_TEST("scene renderer extracts rotation and scale from scene world matrices") {
    auto scene = make_renderable_scene();
    scene.find_node(mirakana::SceneNodeId{1})->transform.rotation_radians =
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.57079637F};
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto command = mirakana::make_scene_mesh_command(packet.meshes[0],
                                                           mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F});

    MK_REQUIRE(std::abs(command.transform.position.x - 4.0F) < 0.0001F);
    MK_REQUIRE(std::abs(command.transform.position.y) < 0.0001F);
    MK_REQUIRE(std::abs(command.transform.scale.x - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(command.transform.scale.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(command.transform.rotation_radians.z - 1.57079637F) < 0.0001F);
}

MK_TEST("scene renderer submits visible meshes to an active renderer frame") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});

    renderer.begin_frame();
    const auto result = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 0.8F, .g = 0.9F, .b = 1.0F, .a = 1.0F}});
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(result.meshes_submitted == 1);
    MK_REQUIRE(result.sprites_submitted == 1);
    MK_REQUIRE(result.cameras_available == 1);
    MK_REQUIRE(result.lights_available == 0);
    MK_REQUIRE(result.has_primary_camera);
    MK_REQUIRE(stats.meshes_submitted == 1);
    MK_REQUIRE(stats.sprites_submitted == 1);
}

MK_TEST("scene renderer converts scene sprite packets into sprite commands") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto command = mirakana::make_scene_sprite_command(packet.sprites[0]);

    MK_REQUIRE(command.transform.position == (mirakana::Vec2{14.0F, 2.0F}));
    MK_REQUIRE(command.transform.scale == (mirakana::Vec2{6.0F, 1.0F}));
    MK_REQUIRE(command.color.r == 0.2F);
    MK_REQUIRE(command.color.g == 0.4F);
    MK_REQUIRE(command.color.b == 0.6F);
    MK_REQUIRE(command.color.a == 1.0F);
    MK_REQUIRE(command.texture.enabled);
    MK_REQUIRE(command.texture.atlas_page == mirakana::AssetId::from_name("sprites/nameplate"));
    MK_REQUIRE(command.texture.uv_rect.u0 == 0.0F);
    MK_REQUIRE(command.texture.uv_rect.v0 == 0.0F);
    MK_REQUIRE(command.texture.uv_rect.u1 == 1.0F);
    MK_REQUIRE(command.texture.uv_rect.v1 == 1.0F);
}

MK_TEST("scene sprite batch telemetry plans scene packet sprite batches") {
    mirakana::SceneRenderPacket packet;
    packet.sprites.push_back(mirakana::SceneRenderSprite{
        .node = mirakana::SceneNodeId{1},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::SpriteRendererComponent{
                .sprite = mirakana::AssetId::from_name("sprites/player"),
                .material = mirakana::AssetId::from_name("materials/sprite"),
                .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                .visible = true,
            },
    });
    packet.sprites.push_back(mirakana::SceneRenderSprite{
        .node = mirakana::SceneNodeId{2},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::SpriteRendererComponent{
                .sprite = mirakana::AssetId::from_name("sprites/player"),
                .material = mirakana::AssetId::from_name("materials/sprite"),
                .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                .visible = true,
            },
    });
    packet.sprites.push_back(mirakana::SceneRenderSprite{
        .node = mirakana::SceneNodeId{3},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::SpriteRendererComponent{
                .sprite = mirakana::AssetId::from_name("sprites/effect"),
                .material = mirakana::AssetId::from_name("materials/sprite"),
                .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                .visible = true,
            },
    });

    const auto plan = mirakana::plan_scene_sprite_batches(packet);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.sprite_count == 3);
    MK_REQUIRE(plan.textured_sprite_count == 3);
    MK_REQUIRE(plan.draw_count == 2);
    MK_REQUIRE(plan.texture_bind_count == 2);
    MK_REQUIRE(plan.batches.size() == 2);
    MK_REQUIRE(plan.batches[0].kind == mirakana::SpriteBatchKind::textured);
    MK_REQUIRE(plan.batches[0].first_sprite == 0);
    MK_REQUIRE(plan.batches[0].sprite_count == 2);
    MK_REQUIRE(plan.batches[0].atlas_page == mirakana::AssetId::from_name("sprites/player"));
    MK_REQUIRE(plan.batches[1].first_sprite == 2);
    MK_REQUIRE(plan.batches[1].sprite_count == 1);
    MK_REQUIRE(plan.batches[1].atlas_page == mirakana::AssetId::from_name("sprites/effect"));
}

MK_TEST("scene sprite batch telemetry forwards invalid texture diagnostics") {
    mirakana::SceneRenderPacket packet;
    packet.sprites.push_back(mirakana::SceneRenderSprite{
        .node = mirakana::SceneNodeId{1},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::SpriteRendererComponent{
                .sprite = mirakana::AssetId{},
                .material = mirakana::AssetId::from_name("materials/sprite"),
                .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                .visible = true,
            },
    });

    const auto plan = mirakana::plan_scene_sprite_batches(packet);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.sprite_count == 1);
    MK_REQUIRE(plan.textured_sprite_count == 0);
    MK_REQUIRE(plan.diagnostics.size() == 1);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::SpriteBatchDiagnosticCode::missing_texture_atlas);
    MK_REQUIRE(plan.diagnostics[0].sprite_index == 0);
}

MK_TEST("scene mesh package telemetry plans scene packet mesh draws") {
    mirakana::SceneRenderPacket packet;
    packet.meshes.push_back(mirakana::SceneRenderMesh{
        .node = mirakana::SceneNodeId{1},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::MeshRendererComponent{
                .mesh = mirakana::AssetId::from_name("meshes/hero"),
                .material = mirakana::AssetId::from_name("materials/hero"),
                .visible = true,
            },
    });
    packet.meshes.push_back(mirakana::SceneRenderMesh{
        .node = mirakana::SceneNodeId{2},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::MeshRendererComponent{
                .mesh = mirakana::AssetId::from_name("meshes/hero"),
                .material = mirakana::AssetId::from_name("materials/hero"),
                .visible = true,
            },
    });
    packet.meshes.push_back(mirakana::SceneRenderMesh{
        .node = mirakana::SceneNodeId{3},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::MeshRendererComponent{
                .mesh = mirakana::AssetId::from_name("meshes/prop"),
                .material = mirakana::AssetId::from_name("materials/prop"),
                .visible = true,
            },
    });

    const auto plan = mirakana::plan_scene_mesh_draws(packet);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.mesh_count == 3);
    MK_REQUIRE(plan.draw_count == 3);
    MK_REQUIRE(plan.unique_mesh_count == 2);
    MK_REQUIRE(plan.unique_material_count == 2);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("scene mesh package telemetry reports invalid mesh and material ids") {
    mirakana::SceneRenderPacket packet;
    packet.meshes.push_back(mirakana::SceneRenderMesh{
        .node = mirakana::SceneNodeId{1},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::MeshRendererComponent{
                .mesh = mirakana::AssetId{},
                .material = mirakana::AssetId::from_name("materials/valid"),
                .visible = true,
            },
    });
    packet.meshes.push_back(mirakana::SceneRenderMesh{
        .node = mirakana::SceneNodeId{2},
        .world_from_node = mirakana::Mat4::identity(),
        .renderer =
            mirakana::MeshRendererComponent{
                .mesh = mirakana::AssetId::from_name("meshes/valid"),
                .material = mirakana::AssetId{},
                .visible = true,
            },
    });

    const auto plan = mirakana::plan_scene_mesh_draws(packet);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.mesh_count == 2);
    MK_REQUIRE(plan.draw_count == 0);
    MK_REQUIRE(plan.unique_mesh_count == 1);
    MK_REQUIRE(plan.unique_material_count == 1);
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::SceneMeshDrawPlanDiagnosticCode::invalid_mesh_asset);
    MK_REQUIRE(plan.diagnostics[0].mesh_index == 0);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::SceneMeshDrawPlanDiagnosticCode::invalid_material_asset);
    MK_REQUIRE(plan.diagnostics[1].mesh_index == 1);
}

MK_TEST("scene renderer builds camera matrices from scene cameras") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto camera = mirakana::make_scene_camera_matrices(packet.cameras[0], 16.0F / 9.0F);

    MK_REQUIRE(std::abs(camera.view_from_world.at(0, 3) + 12.0F) < 0.0001F);
    MK_REQUIRE(std::abs(camera.view_from_world.at(1, 3)) < 0.0001F);
    MK_REQUIRE(std::abs(camera.clip_from_view.at(0, 0) - 1.029649F) < 0.0001F);
    MK_REQUIRE(std::abs(camera.clip_from_view.at(1, 1) - 1.830487F) < 0.0001F);
    MK_REQUIRE(camera.clip_from_view.at(3, 2) == -1.0F);
}

MK_TEST("scene renderer builds light commands from scene lights") {
    mirakana::Scene scene("lighting");
    const auto light_node = scene.create_node("Key");
    scene.find_node(light_node)->transform.position = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F};
    scene.find_node(light_node)->transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 1.57079637F, .z = 0.0F};

    mirakana::SceneNodeComponents components;
    components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.8F, .z = 0.6F},
        .intensity = 3.0F,
        .range = 25.0F,
        .inner_cone_radians = 0.2F,
        .outer_cone_radians = 0.6F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto command = mirakana::make_scene_light_command(packet.lights[0]);

    MK_REQUIRE(command.type == mirakana::LightType::directional);
    MK_REQUIRE(command.position == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
    MK_REQUIRE(std::abs(command.direction.x + 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(command.direction.y) < 0.0001F);
    MK_REQUIRE(std::abs(command.direction.z) < 0.0001F);
    MK_REQUIRE(command.color.r == 1.0F);
    MK_REQUIRE(command.color.g == 0.8F);
    MK_REQUIRE(command.color.b == 0.6F);
    MK_REQUIRE(command.intensity == 3.0F);
    MK_REQUIRE(command.range == 25.0F);
    MK_REQUIRE(command.casts_shadows);
}

MK_TEST("scene renderer builds a shadow map plan from the first directional shadow light") {
    mirakana::Scene scene("shadow-map");
    const auto light_node = scene.create_node("Sun");
    const auto mesh_node = scene.create_node("Caster");
    const auto hidden_mesh_node = scene.create_node("HiddenCaster");

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/caster"),
        .material = mirakana::AssetId::from_name("materials/caster"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    mirakana::SceneNodeComponents hidden_mesh_components;
    hidden_mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/hidden"),
        .material = mirakana::AssetId::from_name("materials/hidden"),
        .visible = false,
    };
    scene.set_components(hidden_mesh_node, hidden_mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto plan = mirakana::build_scene_shadow_map_plan(
        packet, mirakana::SceneShadowMapDesc{
                    .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
                    .depth_format = mirakana::rhi::Format::depth24_stencil8,
                });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.selected_light_index == 0);
    MK_REQUIRE(plan.caster_count == 1);
    MK_REQUIRE(plan.receiver_count == 1);
    MK_REQUIRE(plan.depth_texture.usage ==
               (mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource));
}

MK_TEST("scene renderer prefers the first directional shadow light over unsupported shadow lights") {
    mirakana::Scene scene("shadow-map-selection");
    const auto point_light = scene.create_node("Point");
    const auto directional_light = scene.create_node("Sun");
    const auto mesh_node = scene.create_node("Caster");

    mirakana::SceneNodeComponents point_components;
    point_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::point,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 10.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(point_light, point_components);

    mirakana::SceneNodeComponents directional_components;
    directional_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(directional_light, directional_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/caster"),
        .material = mirakana::AssetId::from_name("materials/caster"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto plan = mirakana::build_scene_shadow_map_plan(
        packet, mirakana::SceneShadowMapDesc{
                    .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
                    .depth_format = mirakana::rhi::Format::depth24_stencil8,
                });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.selected_light_index == 1);
    MK_REQUIRE(plan.light.type == mirakana::ShadowMapLightType::directional);
}

MK_TEST("scene renderer reports unsupported shadow light types through the shadow foundation") {
    mirakana::Scene scene("shadow-map-point");
    const auto light_node = scene.create_node("Point");
    const auto mesh_node = scene.create_node("Caster");

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::point,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 10.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/caster"),
        .material = mirakana::AssetId::from_name("materials/caster"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto plan = mirakana::build_scene_shadow_map_plan(
        packet, mirakana::SceneShadowMapDesc{
                    .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
                    .depth_format = mirakana::rhi::Format::depth24_stencil8,
                });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.selected_light_index == 0);
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(plan, mirakana::ShadowMapDiagnosticCode::unsupported_light_type));
}

MK_TEST("scene renderer builds a stable directional shadow light-space policy from mesh bounds") {
    mirakana::Scene scene("shadow-light-space");
    const auto light_node = scene.create_node("Sun");
    const auto left_mesh = scene.create_node("LeftCaster");
    const auto right_mesh = scene.create_node("RightCaster");

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    scene.find_node(left_mesh)->transform.position = mirakana::Vec3{.x = -3.0F, .y = 0.0F, .z = 1.0F};
    scene.find_node(right_mesh)->transform.position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = -1.0F};

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/caster"),
        .material = mirakana::AssetId::from_name("materials/caster"),
        .visible = true,
    };
    scene.set_components(left_mesh, mesh_components);
    scene.set_components(right_mesh, mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto shadow_plan = mirakana::build_scene_shadow_map_plan(
        packet, mirakana::SceneShadowMapDesc{
                    .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
                    .depth_format = mirakana::rhi::Format::depth24_stencil8,
                });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto light_space = mirakana::build_scene_directional_shadow_light_space_plan(
        packet, shadow_plan, mirakana::SceneShadowLightSpaceDesc{});

    MK_REQUIRE(light_space.succeeded());
    MK_REQUIRE(light_space.focus_center == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(light_space.focus_radius >= 5.0F);
    MK_REQUIRE(light_space.depth_radius >= light_space.focus_radius);
    MK_REQUIRE(light_space.texel_world_size > 0.0F);
}

MK_TEST("scene renderer reports invalid stable directional shadow depth padding") {
    mirakana::Scene scene("shadow-light-space-invalid-depth");
    const auto light_node = scene.create_node("Sun");
    const auto mesh_node = scene.create_node("Caster");

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/caster"),
        .material = mirakana::AssetId::from_name("materials/caster"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto shadow_plan = mirakana::build_scene_shadow_map_plan(
        packet, mirakana::SceneShadowMapDesc{
                    .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
                });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto light_space = mirakana::build_scene_directional_shadow_light_space_plan(
        packet, shadow_plan,
        mirakana::SceneShadowLightSpaceDesc{
            .minimum_focus_radius = 4.0F,
            .depth_padding = std::numeric_limits<float>::quiet_NaN(),
        });

    MK_REQUIRE(!light_space.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        light_space, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_depth_radius));
}

MK_TEST("scene renderer resolves mesh colors from material definitions") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);

    mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/player"),
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = std::array<float, 4>{0.1F, 0.2F, 0.3F, 0.4F},
                .emissive = std::array<float, 3>{0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.8F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };

    mirakana::SceneMaterialPalette palette;
    MK_REQUIRE(palette.try_add(material));
    MK_REQUIRE(palette.count() == 1);

    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::SceneRenderSubmitDesc{
                              .fallback_mesh_color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                              .material_palette = &palette});

    MK_REQUIRE(command.color.r == 0.1F);
    MK_REQUIRE(command.color.g == 0.2F);
    MK_REQUIRE(command.color.b == 0.3F);
    MK_REQUIRE(command.color.a == 0.4F);

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    renderer.begin_frame();
    const auto result = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = &palette});
    renderer.end_frame();

    MK_REQUIRE(result.material_colors_resolved == 1);
}

MK_TEST("scene renderer resolves material instance colors") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);

    mirakana::MaterialDefinition parent{
        .id = mirakana::AssetId::from_name("materials/player-parent"),
        .name = "Player Parent",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = std::array<float, 4>{0.1F, 0.2F, 0.3F, 1.0F},
                .emissive = std::array<float, 3>{0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.8F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };

    mirakana::MaterialInstanceDefinition instance;
    instance.id = mirakana::AssetId::from_name("materials/player");
    instance.name = "Player Runtime";
    instance.parent = parent.id;
    instance.factor_overrides = mirakana::MaterialFactors{
        .base_color = std::array<float, 4>{0.9F, 0.1F, 0.2F, 0.7F},
        .emissive = std::array<float, 3>{0.0F, 0.0F, 0.0F},
        .metallic = 0.0F,
        .roughness = 0.5F,
    };

    mirakana::SceneMaterialPalette palette;
    MK_REQUIRE(palette.try_add_instance(parent, instance));

    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::SceneRenderSubmitDesc{
                              .fallback_mesh_color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                              .material_palette = &palette});

    MK_REQUIRE(command.color.r == 0.9F);
    MK_REQUIRE(command.color.g == 0.1F);
    MK_REQUIRE(command.color.b == 0.2F);
    MK_REQUIRE(command.color.a == 0.7F);
}

MK_TEST("scene renderer resolves gpu bindings from scene asset ids") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);
    mirakana::rhi::NullRhiDevice device;

    mirakana::SceneGpuBindingPalette gpu_bindings;
    const auto mesh_binding = mirakana::MeshGpuBinding{
        .vertex_buffer = mirakana::rhi::BufferHandle{10},
        .index_buffer = mirakana::rhi::BufferHandle{11},
        .vertex_count = 3,
        .index_count = 3,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = 48,
        .index_format = mirakana::rhi::IndexFormat::uint32,
        .owner_device = &device,
    };
    const auto material_binding = mirakana::MaterialGpuBinding{
        .pipeline_layout = mirakana::rhi::PipelineLayoutHandle{12},
        .descriptor_set = mirakana::rhi::DescriptorSetHandle{13},
        .descriptor_set_index = 0,
        .owner_device = &device,
    };

    MK_REQUIRE(gpu_bindings.try_add_mesh(mirakana::AssetId::from_name("meshes/player"), mesh_binding));
    MK_REQUIRE(gpu_bindings.try_add_material(mirakana::AssetId::from_name("materials/player"), material_binding));
    MK_REQUIRE(gpu_bindings.mesh_count() == 1);
    MK_REQUIRE(gpu_bindings.material_count() == 1);
    MK_REQUIRE(!gpu_bindings.try_add_mesh(mirakana::AssetId::from_name("meshes/player"), mesh_binding));

    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::SceneRenderSubmitDesc{
                              .fallback_mesh_color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                              .material_palette = nullptr,
                              .gpu_bindings = &gpu_bindings});

    MK_REQUIRE(command.mesh_binding.vertex_buffer.value == 10);
    MK_REQUIRE(command.mesh_binding.index_buffer.value == 11);
    MK_REQUIRE(command.mesh_binding.vertex_stride == 48);
    MK_REQUIRE(command.mesh_binding.index_format == mirakana::rhi::IndexFormat::uint32);
    MK_REQUIRE(command.material_binding.pipeline_layout.value == 12);
    MK_REQUIRE(command.material_binding.descriptor_set.value == 13);
    MK_REQUIRE(command.material_binding.owner_device == &device);

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    renderer.begin_frame();
    const auto result = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &gpu_bindings});
    renderer.end_frame();

    MK_REQUIRE(result.mesh_gpu_bindings_resolved == 1);
    MK_REQUIRE(result.material_gpu_bindings_resolved == 1);
}

MK_TEST("scene renderer registers uploaded runtime meshes for rhi frame submission") {
    const auto scene = make_renderable_scene();
    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    mirakana::rhi::NullRhiDevice device;

    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{7},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x3f}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
    };
    const auto upload = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);
    MK_REQUIRE(upload.succeeded());

    mirakana::SceneGpuBindingPalette gpu_bindings;
    MK_REQUIRE(gpu_bindings.try_add_mesh(mesh, mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(upload)));
    MK_REQUIRE(!gpu_bindings.try_add_mesh(mesh, mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(upload)));

    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::SceneRenderSubmitDesc{
                              .fallback_mesh_color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                              .material_palette = nullptr,
                              .gpu_bindings = &gpu_bindings});
    MK_REQUIRE(command.mesh_binding.vertex_buffer.value == upload.vertex_buffer.value);
    MK_REQUIRE(command.mesh_binding.index_buffer.value == upload.index_buffer.value);
    MK_REQUIRE(command.mesh_binding.vertex_count == upload.vertex_count);
    MK_REQUIRE(command.mesh_binding.index_count == upload.index_count);
    MK_REQUIRE(command.mesh_binding.vertex_stride == upload.vertex_stride);
    MK_REQUIRE(command.mesh_binding.index_format == upload.index_format);
    MK_REQUIRE(command.mesh_binding.owner_device == &device);

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = upload.vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    const auto result = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &gpu_bindings});
    renderer.end_frame();

    const auto stats = device.stats();
    MK_REQUIRE(result.mesh_gpu_bindings_resolved == 1);
    MK_REQUIRE(stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(stats.index_buffer_bindings == 1);
    MK_REQUIRE(stats.indexed_draw_calls == 1);
    MK_REQUIRE(stats.indices_submitted == 3);
}

MK_TEST("scene renderer rejects failed runtime mesh uploads for gpu binding registration") {
    mirakana::rhi::NullRhiDevice device;
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mirakana::AssetId::from_name("meshes/broken"),
        .handle = mirakana::runtime::RuntimeAssetHandle{8},
        .vertex_count = 0,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };
    const auto upload = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    mirakana::SceneGpuBindingPalette gpu_bindings;
    MK_REQUIRE(!upload.succeeded());
    MK_REQUIRE(!gpu_bindings.try_add_mesh(payload.asset, mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(upload)));
    MK_REQUIRE(gpu_bindings.mesh_count() == 0);
}

MK_TEST("scene renderer loads runtime package scene and material palette") {
    mirakana::MemoryFileSystem fs;
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto scene_asset = mirakana::AssetId::from_name("scenes/package_level");

    const auto mesh_payload = package_mesh_payload(mesh);
    const auto material_payload = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.2F, 0.4F, 0.6F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.7F,
            },
        .texture_bindings = {},
        .double_sided = false,
    });
    const auto scene_payload = mirakana::serialize_scene(make_package_renderable_scene(mesh, material));

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mesh,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "assets/meshes/player.mesh",
                                          .content = mesh_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = material,
                                          .kind = mirakana::AssetKind::material,
                                          .path = "assets/materials/player.material",
                                          .content = material_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = scene_asset,
                                          .kind = mirakana::AssetKind::scene,
                                          .path = "assets/scenes/package_level.scene",
                                          .content = scene_payload,
                                          .source_revision = 1,
                                          .dependencies = {mesh, material}},
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/meshes/player.mesh", mesh_payload);
    fs.write_text("assets/materials/player.material", material_payload);
    fs.write_text("assets/scenes/package_level.scene", scene_payload);

    const auto package = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(package.succeeded());

    const auto loaded = mirakana::load_runtime_scene_render_data(package.package, scene_asset);

    MK_REQUIRE(loaded.succeeded());
    MK_REQUIRE(loaded.scene.has_value());
    MK_REQUIRE(loaded.scene->name() == "PackageLevel");
    MK_REQUIRE(loaded.material_palette.count() == 1);

    const auto packet = mirakana::build_scene_render_packet(*loaded.scene);
    MK_REQUIRE(packet.meshes.size() == 1);
    const auto command = mirakana::make_scene_mesh_command(
        packet.meshes[0], mirakana::SceneRenderSubmitDesc{
                              .fallback_mesh_color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                              .material_palette = &loaded.material_palette});
    MK_REQUIRE(command.color.r == 0.2F);
    MK_REQUIRE(command.color.g == 0.4F);
    MK_REQUIRE(command.color.b == 0.6F);
    MK_REQUIRE(command.color.a == 1.0F);
}

MK_TEST("scene renderer instantiates runtime package scene render packet") {
    mirakana::MemoryFileSystem fs;
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto scene_asset = mirakana::AssetId::from_name("scenes/package_level");

    const auto mesh_payload = package_mesh_payload(mesh);
    const auto material_payload = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.2F, 0.4F, 0.6F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.7F,
            },
        .texture_bindings = {},
        .double_sided = false,
    });
    const auto scene_payload = mirakana::serialize_scene(make_package_renderable_scene(mesh, material));

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mesh,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "assets/meshes/player.mesh",
                                          .content = mesh_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = material,
                                          .kind = mirakana::AssetKind::material,
                                          .path = "assets/materials/player.material",
                                          .content = material_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = scene_asset,
                                          .kind = mirakana::AssetKind::scene,
                                          .path = "assets/scenes/package_level.scene",
                                          .content = scene_payload,
                                          .source_revision = 1,
                                          .dependencies = {mesh, material}},
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/meshes/player.mesh", mesh_payload);
    fs.write_text("assets/materials/player.material", material_payload);
    fs.write_text("assets/scenes/package_level.scene", scene_payload);

    const auto package = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(package.succeeded());

    const auto instance = mirakana::instantiate_runtime_scene_render_data(package.package, scene_asset);

    MK_REQUIRE(instance.succeeded());
    MK_REQUIRE(instance.scene.has_value());
    MK_REQUIRE(instance.scene->name() == "PackageLevel");
    MK_REQUIRE(instance.material_palette.count() == 1);
    MK_REQUIRE(instance.render_packet.meshes.size() == 1);

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, instance.render_packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = &instance.material_palette});
    renderer.end_frame();

    MK_REQUIRE(submitted.meshes_submitted == 1);
    MK_REQUIRE(submitted.material_colors_resolved == 1);
}

MK_TEST("scene renderer applies animation float clip bindings to runtime render instance") {
    const auto mesh = mirakana::AssetId::from_name("meshes/animated");
    const auto material = mirakana::AssetId::from_name("materials/animated");

    mirakana::RuntimeSceneRenderInstance instance;
    instance.scene = make_transform_animation_scene(mesh, material);
    instance.render_packet = mirakana::build_scene_render_packet(*instance.scene);
    MK_REQUIRE(instance.render_packet.meshes.size() == 1);

    const auto result = mirakana::sample_and_apply_runtime_scene_render_animation_float_clip(
        instance, make_translation_x_animation_clip(), make_packaged_mesh_translation_binding(), 1.0F);

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.sampled_track_count == 1);
    MK_REQUIRE(result.applied_sample_count == 1);
    MK_REQUIRE(result.binding_diagnostics.empty());
    const auto* node = instance.scene->find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(node != nullptr);
    MK_REQUIRE(std::abs(node->transform.position.x - 0.5F) < 0.0001F);
    MK_REQUIRE(instance.render_packet.meshes.size() == 1);
    const auto command = mirakana::make_scene_mesh_command(instance.render_packet.meshes[0],
                                                           mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F});
    MK_REQUIRE(std::abs(command.transform.position.x - 0.5F) < 0.0001F);
}

MK_TEST("scene renderer samples runtime sprite animation frames into render packet") {
    const auto sprite = mirakana::AssetId::from_name("textures/player");
    const auto material = mirakana::AssetId::from_name("materials/player");

    mirakana::RuntimeSceneRenderInstance instance;
    instance.scene = make_sprite_animation_scene(sprite, material);
    instance.render_packet = mirakana::build_scene_render_packet(*instance.scene);
    MK_REQUIRE(instance.render_packet.sprites.size() == 1);

    const auto result = mirakana::sample_and_apply_runtime_scene_render_sprite_animation(
        instance, make_runtime_sprite_animation_payload(sprite, material), 0.25F);

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.sampled_frame_count == 1);
    MK_REQUIRE(result.applied_frame_count == 1);
    MK_REQUIRE(result.selected_frame_index == 1);
    MK_REQUIRE(instance.render_packet.sprites.size() == 1);
    MK_REQUIRE(instance.render_packet.sprites[0].renderer.sprite == sprite);
    MK_REQUIRE(instance.render_packet.sprites[0].renderer.material == material);
    MK_REQUIRE(std::abs(instance.render_packet.sprites[0].renderer.size.x - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(instance.render_packet.sprites[0].renderer.size.y - 1.25F) < 0.0001F);
    MK_REQUIRE(std::abs(instance.render_packet.sprites[0].renderer.tint[1] - 0.4F) < 0.0001F);
}

MK_TEST("scene renderer reports missing scene for animation float clip application") {
    mirakana::RuntimeSceneRenderInstance instance;

    const auto result = mirakana::sample_and_apply_runtime_scene_render_animation_float_clip(
        instance, make_translation_x_animation_clip(), make_packaged_mesh_translation_binding(), 1.0F);

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.diagnostic.find("scene") != std::string::npos);
    MK_REQUIRE(result.sampled_track_count == 0);
    MK_REQUIRE(result.applied_sample_count == 0);
}

MK_TEST("scene renderer applies sampled quaternion animation tracks to runtime render instance") {
    const auto mesh = mirakana::AssetId::from_name("meshes/animated");
    const auto material = mirakana::AssetId::from_name("materials/animated");

    mirakana::RuntimeSceneRenderInstance instance;
    instance.scene = make_transform_animation_scene(mesh, material);
    instance.render_packet = mirakana::build_scene_render_packet(*instance.scene);
    MK_REQUIRE(instance.render_packet.meshes.size() == 1);

    const auto tracks = make_packaged_mesh_quaternion_tracks();
    const auto result = mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d(
        instance, make_packaged_mesh_quaternion_skeleton(), tracks, 1.0F);

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.sampled_track_count == 1);
    MK_REQUIRE(result.applied_sample_count == 1);
    MK_REQUIRE(result.binding_diagnostics.empty());
    const auto* node = instance.scene->find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(node != nullptr);
    MK_REQUIRE(std::abs(node->transform.rotation_radians.z - 1.57079637F) < 0.0001F);
    MK_REQUIRE(instance.render_packet.meshes.size() == 1);
    const auto command = mirakana::make_scene_mesh_command(instance.render_packet.meshes[0],
                                                           mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F});
    MK_REQUIRE(std::abs(command.transform.rotation_radians.z - 1.57079637F) < 0.0001F);
}

MK_TEST("scene renderer samples runtime morph mesh payload with animation float clip weights") {
    const auto result = mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip(
        make_runtime_morph_mesh_payload(), make_morph_weight_animation_clip(), "gltf/node/0/weights/", 1.0F);

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.sampled_track_count == 1);
    MK_REQUIRE(result.applied_weight_count == 1);
    MK_REQUIRE(result.target_weights.size() == 1);
    MK_REQUIRE(std::abs(result.target_weights[0] - 0.5F) < 0.0001F);
    MK_REQUIRE(result.morphed_positions.size() == 1);
    MK_REQUIRE(std::abs(result.morphed_positions[0].x) < 0.0001F);
    MK_REQUIRE(std::abs(result.morphed_positions[0].y) < 0.0001F);
    MK_REQUIRE(std::abs(result.morphed_positions[0].z) < 0.0001F);
}

MK_TEST("scene renderer reports missing runtime morph weight clip targets") {
    const auto result = mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip(
        make_runtime_morph_mesh_payload(), make_morph_weight_animation_clip(), "gltf/node/1/weights/", 1.0F);

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.diagnostic.find("missing morph weight sample") != std::string::npos);
    MK_REQUIRE(result.sampled_track_count == 1);
    MK_REQUIRE(result.applied_weight_count == 0);
    MK_REQUIRE(result.morphed_positions.empty());
}

MK_TEST("scene renderer reports cyclic scene payloads during runtime scene instantiation") {
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto scene_asset = mirakana::AssetId::from_name("scenes/cyclic");
    const auto scene_payload = std::string{"format=GameEngine.Scene.v1\n"
                                           "scene.name=Cyclic\n"
                                           "node.count=2\n"
                                           "node.1.name=A\n"
                                           "node.1.parent=2\n"
                                           "node.1.position=0,0,0\n"
                                           "node.1.scale=1,1,1\n"
                                           "node.1.rotation=0,0,0\n"
                                           "node.1.mesh_renderer.mesh=" +
                                           std::to_string(mesh.value) +
                                           "\n"
                                           "node.1.mesh_renderer.material=" +
                                           std::to_string(material.value) +
                                           "\n"
                                           "node.1.mesh_renderer.visible=true\n"
                                           "node.2.name=B\n"
                                           "node.2.parent=1\n"
                                           "node.2.position=0,0,0\n"
                                           "node.2.scale=1,1,1\n"
                                           "node.2.rotation=0,0,0\n"};
    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = package_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                .id = material,
                .name = "Player",
                .shading_model = mirakana::MaterialShadingModel::lit,
                .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                .factors = mirakana::MaterialFactors{},
                .texture_bindings = {},
                .double_sided = false,
            }),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = scene_asset,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/cyclic.scene",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {mesh, material},
            .content = scene_payload,
        },
    });

    const auto instance = mirakana::instantiate_runtime_scene_render_data(package, scene_asset);

    MK_REQUIRE(!instance.succeeded());
    MK_REQUIRE(!instance.scene.has_value());
    MK_REQUIRE(instance.render_packet.meshes.empty());
    MK_REQUIRE(instance.failures.size() == 1);
    MK_REQUIRE(instance.failures[0].asset == scene_asset);
    MK_REQUIRE(instance.failures[0].diagnostic.find("runtime scene deserialize failed") != std::string::npos);
    MK_REQUIRE(instance.failures[0].diagnostic.find("invalid scene parenting") != std::string::npos);
}

MK_TEST("scene renderer reports missing package material referenced by scene") {
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/missing");
    const auto scene_asset = mirakana::AssetId::from_name("scenes/missing_material");
    const auto scene_payload = mirakana::serialize_scene(make_package_renderable_scene(mesh, material));

    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = package_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = scene_asset,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/missing_material.scene",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {mesh},
            .content = scene_payload,
        },
    });

    const auto loaded = mirakana::load_runtime_scene_render_data(package, scene_asset);

    MK_REQUIRE(!loaded.succeeded());
    MK_REQUIRE(loaded.scene.has_value());
    MK_REQUIRE(loaded.material_palette.count() == 0);
    MK_REQUIRE(loaded.failures.size() == 1);
    MK_REQUIRE(loaded.failures[0].asset == material);
    MK_REQUIRE(loaded.failures[0].diagnostic.find("material") != std::string::npos);
}

MK_TEST("scene renderer reports malformed runtime scene payloads before render submission") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/broken");
    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = scene_asset,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/broken.scene",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.Scene.v1\nscene.name=Broken\nnode.count=1\n",
        },
    });

    const auto loaded = mirakana::load_runtime_scene_render_data(package, scene_asset);

    MK_REQUIRE(!loaded.succeeded());
    MK_REQUIRE(!loaded.scene.has_value());
    MK_REQUIRE(loaded.failures.size() == 1);
    MK_REQUIRE(loaded.failures[0].asset == scene_asset);
    MK_REQUIRE(loaded.failures[0].diagnostic.find("scene") != std::string::npos);
}

int main() {
    return mirakana::test::run_all();
}
