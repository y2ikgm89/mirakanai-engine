// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/physics_collision_runtime.hpp"
#include "mirakana/runtime/runtime_diagnostics.hpp"
#include "mirakana/runtime/session_services.hpp"

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::string cooked_texture_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=texture\n"
           "source.path=source/textures/player_albedo.texture_source\n"
           "texture.width=4\n"
           "texture.height=2\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=32\n";
}

[[nodiscard]] std::string cooked_ui_atlas_payload(mirakana::AssetId atlas_asset, mirakana::AssetId texture_asset,
                                                  std::string_view source_decoding = "unsupported",
                                                  std::string_view atlas_packing = "unsupported") {
    return "format=GameEngine.UiAtlas.v1\n"
           "asset.id=" +
           std::to_string(atlas_asset.value) +
           "\n"
           "asset.kind=ui_atlas\n"
           "source.decoding=" +
           std::string{source_decoding} +
           "\n"
           "atlas.packing=" +
           std::string{atlas_packing} +
           "\n"
           "page.count=1\n"
           "page.0.asset=" +
           std::to_string(texture_asset.value) +
           "\n"
           "page.0.asset_uri=assets/ui/hud-atlas.texture.geasset\n"
           "image.count=1\n"
           "image.0.resource_id=hud.icon\n"
           "image.0.asset_uri=assets/ui/hud-atlas.texture.geasset\n"
           "image.0.page=" +
           std::to_string(texture_asset.value) +
           "\n"
           "image.0.u0=0.25\n"
           "image.0.v0=0.5\n"
           "image.0.u1=0.75\n"
           "image.0.v1=1\n"
           "image.0.color=1,1,1,1\n"
           "glyph.count=1\n"
           "glyph.0.font_family=ui/body\n"
           "glyph.0.glyph=65\n"
           "glyph.0.page=" +
           std::to_string(texture_asset.value) +
           "\n"
           "glyph.0.u0=0\n"
           "glyph.0.v0=0\n"
           "glyph.0.u1=0.25\n"
           "glyph.0.v1=0.5\n"
           "glyph.0.color=0.9,0.9,0.9,1\n";
}

[[nodiscard]] std::string cooked_tilemap_payload(mirakana::AssetId tilemap_asset, mirakana::AssetId atlas_page,
                                                 std::string_view atlas_page_uri) {
    return "format=GameEngine.Tilemap.v1\n"
           "asset.id=" +
           std::to_string(tilemap_asset.value) +
           "\n"
           "asset.kind=tilemap\n"
           "source.decoding=unsupported\n"
           "atlas.packing=unsupported\n"
           "native_gpu_sprite_batching=unsupported\n"
           "atlas.page.asset=" +
           std::to_string(atlas_page.value) +
           "\n"
           "atlas.page.asset_uri=" +
           std::string{atlas_page_uri} +
           "\n"
           "tile.size=16,16\n"
           "tile.count=1\n"
           "tile.0.id=ground\n"
           "tile.0.page=" +
           std::to_string(atlas_page.value) +
           "\n"
           "tile.0.u0=0\n"
           "tile.0.v0=0\n"
           "tile.0.u1=1\n"
           "tile.0.v1=1\n"
           "tile.0.color=1,1,1,1\n"
           "layer.count=1\n"
           "layer.0.name=ground\n"
           "layer.0.width=1\n"
           "layer.0.height=1\n"
           "layer.0.visible=true\n"
           "layer.0.cells=ground\n";
}

[[nodiscard]] std::string cooked_physics_collision_scene_payload(mirakana::AssetId collision_scene,
                                                                 std::string_view second_body_name = "pickup_trigger",
                                                                 std::string_view second_body_shape = "sphere",
                                                                 std::string_view second_body_layer = "2",
                                                                 std::string_view backend_native = "unsupported") {
    return "format=GameEngine.PhysicsCollisionScene3D.v1\n"
           "asset.id=" +
           std::to_string(collision_scene.value) +
           "\n"
           "asset.kind=physics_collision_scene\n"
           "backend.native=" +
           std::string{backend_native} +
           "\n"
           "world.gravity=0,-9.80665,0\n"
           "body.count=2\n"
           "body.0.name=floor\n"
           "body.0.shape=aabb\n"
           "body.0.position=0,-0.5,0\n"
           "body.0.velocity=0,0,0\n"
           "body.0.dynamic=false\n"
           "body.0.mass=0\n"
           "body.0.linear_damping=0\n"
           "body.0.half_extents=5,0.5,5\n"
           "body.0.radius=0.5\n"
           "body.0.half_height=0.5\n"
           "body.0.layer=1\n"
           "body.0.mask=4294967295\n"
           "body.0.trigger=false\n"
           "body.0.material=stone\n"
           "body.0.compound=level_static\n"
           "body.1.name=" +
           std::string{second_body_name} +
           "\n"
           "body.1.shape=" +
           std::string{second_body_shape} +
           "\n"
           "body.1.position=0,0.75,0\n"
           "body.1.velocity=0,0,0\n"
           "body.1.dynamic=false\n"
           "body.1.mass=0\n"
           "body.1.linear_damping=0\n"
           "body.1.half_extents=0.5,0.5,0.5\n"
           "body.1.radius=0.75\n"
           "body.1.half_height=0.5\n"
           "body.1.layer=" +
           std::string{second_body_layer} +
           "\n"
           "body.1.mask=1\n"
           "body.1.trigger=true\n"
           "body.1.material=trigger\n";
}

[[nodiscard]] std::string cooked_sprite_animation_payload(mirakana::AssetId animation_asset, mirakana::AssetId sprite,
                                                          mirakana::AssetId material) {
    return "format=GameEngine.CookedSpriteAnimation.v1\n"
           "asset.id=" +
           std::to_string(animation_asset.value) +
           "\n"
           "asset.kind=sprite_animation\n"
           "target.node=Player\n"
           "playback.loop=true\n"
           "frame.count=2\n"
           "frame.0.duration_seconds=0.25\n"
           "frame.0.sprite=" +
           std::to_string(sprite.value) +
           "\n"
           "frame.0.material=" +
           std::to_string(material.value) +
           "\n"
           "frame.0.size=1.5,2\n"
           "frame.0.tint=0.2,0.7,1,1\n"
           "frame.1.duration_seconds=0.25\n"
           "frame.1.sprite=" +
           std::to_string(sprite.value) +
           "\n"
           "frame.1.material=" +
           std::to_string(material.value) +
           "\n"
           "frame.1.size=2,1.25\n"
           "frame.1.tint=1,0.4,0.2,1\n";
}

[[nodiscard]] mirakana::runtime::RuntimePhysicsCollisionScene3DPayload runtime_collision_scene_payload_for_bridge() {
    mirakana::PhysicsBody3DDesc floor;
    floor.position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    floor.dynamic = false;
    floor.mass = 0.0F;
    floor.half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    floor.collision_layer = 1U;
    floor.collision_mask = 0xFFFF'FFFFU;

    mirakana::PhysicsBody3DDesc probe;
    probe.position = mirakana::Vec3{.x = 0.0F, .y = 0.5F, .z = 0.0F};
    probe.dynamic = false;
    probe.mass = 0.0F;
    probe.half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    probe.collision_layer = 4U;
    probe.collision_mask = 1U;

    mirakana::PhysicsBody3DDesc trigger;
    trigger.position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    trigger.dynamic = false;
    trigger.mass = 0.0F;
    trigger.shape = mirakana::PhysicsShape3DKind::sphere;
    trigger.radius = 0.75F;
    trigger.trigger = true;
    trigger.collision_layer = 2U;
    trigger.collision_mask = 1U;

    mirakana::runtime::RuntimePhysicsCollisionScene3DPayload payload;
    payload.asset = mirakana::AssetId::from_name("physics/collision/main");
    payload.handle = mirakana::runtime::RuntimeAssetHandle{1};
    payload.bodies.push_back(
        mirakana::runtime::RuntimePhysicsCollisionBody3DPayload{.name = "floor", .body = floor, .material = "stone"});
    payload.bodies.push_back(mirakana::runtime::RuntimePhysicsCollisionBody3DPayload{
        .name = "collision_probe", .body = probe, .material = "metal"});
    payload.bodies.push_back(mirakana::runtime::RuntimePhysicsCollisionBody3DPayload{
        .name = "pickup_trigger", .body = trigger, .material = "trigger"});
    return payload;
}

[[nodiscard]] mirakana::runtime::RuntimeTilemapPayload runtime_tilemap_payload_for_sampling() {
    mirakana::runtime::RuntimeTilemapPayload payload;
    payload.asset = mirakana::AssetId::from_name("tilemaps/level");
    payload.handle = mirakana::runtime::RuntimeAssetHandle{7};
    payload.atlas_page = mirakana::AssetId::from_name("textures/tilemap-atlas");
    payload.atlas_page_uri = "assets/textures/tilemap-atlas.texture";
    payload.tile_width = 16;
    payload.tile_height = 16;
    payload.tiles.push_back(mirakana::runtime::RuntimeTilemapTile{
        .id = "grass",
        .page = payload.atlas_page,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 0.5F,
        .v1 = 0.5F,
        .color = {0.3F, 0.8F, 0.2F, 1.0F},
    });
    payload.tiles.push_back(mirakana::runtime::RuntimeTilemapTile{
        .id = "water",
        .page = payload.atlas_page,
        .u0 = 0.5F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 0.5F,
        .color = {0.1F, 0.3F, 1.0F, 1.0F},
    });
    payload.layers.push_back(mirakana::runtime::RuntimeTilemapLayer{
        .name = "ground",
        .width = 2,
        .height = 2,
        .visible = true,
        .cells = {"grass", "water", "", "grass"},
    });
    payload.layers.push_back(mirakana::runtime::RuntimeTilemapLayer{
        .name = "collision-preview",
        .width = 1,
        .height = 1,
        .visible = false,
        .cells = {"water"},
    });
    return payload;
}

[[nodiscard]] std::string cooked_mesh_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=mesh\n"
           "source.path=source/meshes/player.mesh_source\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=true\n"
           "mesh.has_uvs=true\n"
           "mesh.has_tangent_frame=true\n"
           "mesh.vertex_data_hex=000102\n"
           "mesh.index_data_hex=030405\n";
}

[[nodiscard]] std::string cooked_audio_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedAudio.v1\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=audio\n"
           "source.path=source/audio/hit.audio_source\n"
           "audio.sample_rate=48000\n"
           "audio.channel_count=2\n"
           "audio.frame_count=64\n"
           "audio.sample_format=float32\n"
           "audio.source_bytes=512\n";
}

[[nodiscard]] std::string cooked_scene_payload() {
    return "format=GameEngine.Scene.v1\n"
           "scene.name=Level01\n"
           "node.count=1\n"
           "node.1.name=Root\n"
           "node.1.parent=0\n"
           "node.1.position=0,0,0\n"
           "node.1.scale=1,1,1\n"
           "node.1.rotation=0,0,0\n";
}

[[nodiscard]] bool nearly_equal(float lhs, float rhs, float epsilon = 0.0001F) noexcept {
    return std::abs(lhs - rhs) <= epsilon;
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger key_trigger(mirakana::Key key) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{.kind = mirakana::runtime::RuntimeInputActionTriggerKind::key,
                                                        .key = key};
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger
gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{
        .kind = mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
        .key = mirakana::Key::unknown,
        .pointer_id = 0,
        .gamepad_id = gamepad_id,
        .gamepad_button = button};
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource key_axis_source(mirakana::Key negative_key,
                                                                        mirakana::Key positive_key) noexcept {
    mirakana::runtime::RuntimeInputAxisSource source;
    source.kind = mirakana::runtime::RuntimeInputAxisSourceKind::key_pair;
    source.negative_key = negative_key;
    source.positive_key = positive_key;
    return source;
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource gamepad_axis_source(mirakana::GamepadId gamepad_id,
                                                                            mirakana::GamepadAxis axis,
                                                                            float scale = 1.0F,
                                                                            float deadzone = 0.0F) noexcept {
    mirakana::runtime::RuntimeInputAxisSource source;
    source.kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis;
    source.gamepad_id = gamepad_id;
    source.gamepad_axis = axis;
    source.scale = scale;
    source.deadzone = deadzone;
    return source;
}

[[nodiscard]] bool
has_rebinding_diagnostic(const std::vector<mirakana::runtime::RuntimeInputRebindingDiagnostic>& diagnostics,
                         mirakana::runtime::RuntimeInputRebindingDiagnosticCode code) noexcept {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] const mirakana::runtime::RuntimeInputRebindingPresentationRow*
find_rebinding_presentation_row(const std::vector<mirakana::runtime::RuntimeInputRebindingPresentationRow>& rows,
                                const std::string& id) noexcept {
    for (const auto& row : rows) {
        if (row.id == id) {
            return &row;
        }
    }
    return nullptr;
}

} // namespace

MK_TEST("runtime asset package loads cooked payloads with stable handles") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";

    const auto index = mirakana::build_asset_cooked_package_index(
        {mirakana::AssetCookedArtifact{.asset = texture,
                                       .kind = mirakana::AssetKind::texture,
                                       .path = "assets/textures/player.texture",
                                       .content = payload,
                                       .source_revision = 3,
                                       .dependencies = {}}},
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player.texture", payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(
        fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "packages/main.geindex"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.package.records().size() == 1);

    const auto* by_asset = result.package.find(texture);
    MK_REQUIRE(by_asset != nullptr);
    MK_REQUIRE(by_asset->asset == texture);
    MK_REQUIRE(by_asset->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(by_asset->content == payload);
    MK_REQUIRE(by_asset->handle.value == 1);

    const auto* by_handle = result.package.find(by_asset->handle);
    MK_REQUIRE(by_handle == by_asset);
}

MK_TEST("runtime asset package rejects payload hash mismatches without partial package") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const auto good_payload = std::string{"format=GameEngine.CookedTexture.v1\ntexture.width=4\n"};
    const auto bad_payload = std::string{"format=GameEngine.CookedTexture.v1\ntexture.width=8\n"};
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = texture,
                                                                      .kind = mirakana::AssetKind::texture,
                                                                      .path = "assets/textures/player.texture",
                                                                      .content = good_payload,
                                                                      .source_revision = 3,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player.texture", bad_payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.package.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("hash") != std::string::npos);
}

MK_TEST("runtime asset package rejects dependencies missing from the package") {
    mirakana::MemoryFileSystem fs;
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const auto payload = std::string{"format=GameEngine.Material.v1\nmaterial.name=Player\n"};
    const auto index = mirakana::AssetCookedPackageIndex{
        .entries = {mirakana::AssetCookedPackageEntry{
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = mirakana::hash_asset_cooked_content(payload),
            .source_revision = 1,
            .dependencies = {texture},
        }},
        .dependencies = {},
    };
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/materials/player.material", payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.package.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("dependency") != std::string::npos);
}

MK_TEST("runtime asset package store stages and commits replacements at a safe point") {
    mirakana::runtime::RuntimeAssetPackageStore store;
    const auto texture = mirakana::AssetId::from_name("textures/player");

    mirakana::runtime::RuntimeAssetPackage initial({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 10,
            .source_revision = 1,
            .dependencies = {},
            .content = "v1",
        },
    });
    mirakana::runtime::RuntimeAssetPackage replacement({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 20,
            .source_revision = 2,
            .dependencies = {},
            .content = "v2",
        },
    });

    store.seed(std::move(initial));
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");

    store.stage(std::move(replacement));
    MK_REQUIRE(store.pending() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");

    MK_REQUIRE(store.commit_safe_point());
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v2");
}

MK_TEST("runtime asset package store keeps active package when staged load fails") {
    mirakana::runtime::RuntimeAssetPackageStore store;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 10,
            .source_revision = 1,
            .dependencies = {},
            .content = "v1",
        },
    }));

    mirakana::runtime::RuntimeAssetPackageLoadResult failed{
        .package = mirakana::runtime::RuntimeAssetPackage{},
        .failures = {mirakana::runtime::RuntimeAssetPackageLoadFailure{
            .asset = texture, .path = "assets/textures/player.texture", .diagnostic = "hash mismatch"}},
    };

    MK_REQUIRE(!store.stage_if_loaded(std::move(failed)));
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");
}

MK_TEST("runtime typed payload access decodes cooked texture mesh audio material and scene payloads") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto audio = mirakana::AssetId::from_name("audio/hit");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto scene = mirakana::AssetId::from_name("scenes/level01");

    const auto texture_payload = cooked_texture_payload(texture);
    const auto mesh_payload = cooked_mesh_payload(mesh);
    const auto audio_payload = cooked_audio_payload(audio);
    const auto material_payload = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
    const auto scene_payload = cooked_scene_payload();

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = texture,
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "assets/textures/player_albedo.texture",
                                          .content = texture_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mesh,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "assets/meshes/player.mesh",
                                          .content = mesh_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = audio,
                                          .kind = mirakana::AssetKind::audio,
                                          .path = "assets/audio/hit.audio",
                                          .content = audio_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = material,
                                          .kind = mirakana::AssetKind::material,
                                          .path = "assets/materials/player.material",
                                          .content = material_payload,
                                          .source_revision = 1,
                                          .dependencies = {texture}},
            mirakana::AssetCookedArtifact{.asset = scene,
                                          .kind = mirakana::AssetKind::scene,
                                          .path = "assets/scenes/level01.scene",
                                          .content = scene_payload,
                                          .source_revision = 1,
                                          .dependencies = {mesh, material}},
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player_albedo.texture", texture_payload);
    fs.write_text("assets/meshes/player.mesh", mesh_payload);
    fs.write_text("assets/audio/hit.audio", audio_payload);
    fs.write_text("assets/materials/player.material", material_payload);
    fs.write_text("assets/scenes/level01.scene", scene_payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});

    MK_REQUIRE(result.succeeded());

    const auto* texture_record = result.package.find(texture);
    MK_REQUIRE(texture_record != nullptr);
    const auto texture_access = mirakana::runtime::runtime_texture_payload(*texture_record);
    MK_REQUIRE(texture_access.succeeded());
    MK_REQUIRE(texture_access.payload.asset == texture);
    MK_REQUIRE(texture_access.payload.width == 4);
    MK_REQUIRE(texture_access.payload.height == 2);
    MK_REQUIRE(texture_access.payload.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
    MK_REQUIRE(texture_access.payload.source_bytes == 32);
    MK_REQUIRE(texture_access.payload.bytes.empty());

    const auto* mesh_record = result.package.find(mesh);
    MK_REQUIRE(mesh_record != nullptr);
    const auto mesh_access = mirakana::runtime::runtime_mesh_payload(*mesh_record);
    MK_REQUIRE(mesh_access.succeeded());
    MK_REQUIRE(mesh_access.payload.vertex_count == 3);
    MK_REQUIRE(mesh_access.payload.index_count == 3);
    MK_REQUIRE(mesh_access.payload.has_normals);
    MK_REQUIRE(mesh_access.payload.has_uvs);
    MK_REQUIRE(mesh_access.payload.has_tangent_frame);
    MK_REQUIRE(mesh_access.payload.vertex_bytes == std::vector<std::uint8_t>({0x00, 0x01, 0x02}));
    MK_REQUIRE(mesh_access.payload.index_bytes == std::vector<std::uint8_t>({0x03, 0x04, 0x05}));

    const auto* audio_record = result.package.find(audio);
    MK_REQUIRE(audio_record != nullptr);
    const auto audio_access = mirakana::runtime::runtime_audio_payload(*audio_record);
    MK_REQUIRE(audio_access.succeeded());
    MK_REQUIRE(audio_access.payload.sample_rate == 48000);
    MK_REQUIRE(audio_access.payload.channel_count == 2);
    MK_REQUIRE(audio_access.payload.frame_count == 64);
    MK_REQUIRE(audio_access.payload.sample_format == mirakana::AudioSourceSampleFormat::float32);

    const auto* material_record = result.package.find(material);
    MK_REQUIRE(material_record != nullptr);
    const auto material_access = mirakana::runtime::runtime_material_payload(*material_record);
    MK_REQUIRE(material_access.succeeded());
    MK_REQUIRE(material_access.payload.material.id == material);
    MK_REQUIRE(material_access.payload.binding_metadata.material == material);
    MK_REQUIRE(material_access.payload.binding_metadata.bindings.size() == 4);
    MK_REQUIRE(material_access.payload.binding_metadata.bindings[3].resource_kind ==
               mirakana::MaterialBindingResourceKind::sampler);
    MK_REQUIRE(material_access.payload.binding_metadata.bindings[3].semantic == "sampler.base_color");

    const auto* scene_record = result.package.find(scene);
    MK_REQUIRE(scene_record != nullptr);
    const auto scene_access = mirakana::runtime::runtime_scene_payload(*scene_record);
    MK_REQUIRE(scene_access.succeeded());
    MK_REQUIRE(scene_access.payload.name == "Level01");
    MK_REQUIRE(scene_access.payload.node_count == 1);
    MK_REQUIRE(scene_access.payload.serialized_scene == scene_payload);
}

MK_TEST("runtime typed payload access decodes cooked ui atlas metadata") {
    const auto atlas = mirakana::AssetId::from_name("ui/hud-atlas-metadata");
    const auto texture = mirakana::AssetId::from_name("textures/hud-atlas");
    const auto payload_text = cooked_ui_atlas_payload(atlas, texture);
    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = atlas,
        .kind = mirakana::AssetKind::ui_atlas,
        .path = "assets/ui/hud-atlas.uiatlas",
        .content_hash = mirakana::hash_asset_cooked_content(payload_text),
        .source_revision = 1,
        .dependencies = {texture},
        .content = payload_text,
    };

    const auto access = mirakana::runtime::runtime_ui_atlas_payload(record);

    MK_REQUIRE(access.succeeded());
    MK_REQUIRE(access.payload.asset == atlas);
    MK_REQUIRE(access.payload.pages.size() == 1);
    MK_REQUIRE(access.payload.pages[0].asset == texture);
    MK_REQUIRE(access.payload.images.size() == 1);
    MK_REQUIRE(access.payload.images[0].resource_id == "hud.icon");
    MK_REQUIRE(access.payload.images[0].asset_uri == "assets/ui/hud-atlas.texture.geasset");
    MK_REQUIRE(access.payload.images[0].page == texture);
    MK_REQUIRE(nearly_equal(access.payload.images[0].u0, 0.25F));
    MK_REQUIRE(nearly_equal(access.payload.images[0].v0, 0.5F));
    MK_REQUIRE(nearly_equal(access.payload.images[0].u1, 0.75F));
    MK_REQUIRE(nearly_equal(access.payload.images[0].v1, 1.0F));
    MK_REQUIRE(access.payload.glyphs.size() == 1);
    MK_REQUIRE(access.payload.glyphs[0].font_family == "ui/body");
    MK_REQUIRE(access.payload.glyphs[0].glyph == 65);
    MK_REQUIRE(access.payload.glyphs[0].page == texture);
    MK_REQUIRE(nearly_equal(access.payload.glyphs[0].u0, 0.0F));
    MK_REQUIRE(nearly_equal(access.payload.glyphs[0].v0, 0.0F));
    MK_REQUIRE(nearly_equal(access.payload.glyphs[0].u1, 0.25F));
    MK_REQUIRE(nearly_equal(access.payload.glyphs[0].v1, 0.5F));
}

MK_TEST("runtime typed payload access decodes cooked sprite animation frames") {
    const auto animation = mirakana::AssetId::from_name("animations/player-idle");
    const auto sprite = mirakana::AssetId::from_name("textures/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto payload_text = cooked_sprite_animation_payload(animation, sprite, material);
    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = animation,
        .kind = mirakana::AssetKind::sprite_animation,
        .path = "assets/animations/player.sprite_animation",
        .content_hash = mirakana::hash_asset_cooked_content(payload_text),
        .source_revision = 1,
        .dependencies = {sprite, material},
        .content = payload_text,
    };

    const auto access = mirakana::runtime::runtime_sprite_animation_payload(record);

    MK_REQUIRE(access.succeeded());
    MK_REQUIRE(access.payload.asset == animation);
    MK_REQUIRE(access.payload.target_node == "Player");
    MK_REQUIRE(access.payload.loop);
    MK_REQUIRE(access.payload.frames.size() == 2);
    MK_REQUIRE(access.payload.frames[0].sprite == sprite);
    MK_REQUIRE(access.payload.frames[0].material == material);
    MK_REQUIRE(nearly_equal(access.payload.frames[0].duration_seconds, 0.25F));
    MK_REQUIRE(nearly_equal(access.payload.frames[0].size[0], 1.5F));
    MK_REQUIRE(nearly_equal(access.payload.frames[0].size[1], 2.0F));
    MK_REQUIRE(nearly_equal(access.payload.frames[1].tint[0], 1.0F));
    MK_REQUIRE(nearly_equal(access.payload.frames[1].tint[1], 0.4F));
}

MK_TEST("runtime ui atlas metadata rejects unsupported production decode or packing claims") {
    const auto atlas = mirakana::AssetId::from_name("ui/hud-atlas-metadata");
    const auto texture = mirakana::AssetId::from_name("textures/hud-atlas");
    const auto source_decode_claim = cooked_ui_atlas_payload(atlas, texture, "ready", "unsupported");
    const mirakana::runtime::RuntimeAssetRecord source_decode_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = atlas,
        .kind = mirakana::AssetKind::ui_atlas,
        .path = "assets/ui/hud-atlas.uiatlas",
        .content_hash = mirakana::hash_asset_cooked_content(source_decode_claim),
        .source_revision = 1,
        .dependencies = {texture},
        .content = source_decode_claim,
    };

    const auto source_decode = mirakana::runtime::runtime_ui_atlas_payload(source_decode_record);

    MK_REQUIRE(!source_decode.succeeded());
    MK_REQUIRE(source_decode.diagnostic.find("source image decoding") != std::string::npos);

    const auto packing_claim = cooked_ui_atlas_payload(atlas, texture, "unsupported", "ready");
    const mirakana::runtime::RuntimeAssetRecord packing_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = atlas,
        .kind = mirakana::AssetKind::ui_atlas,
        .path = "assets/ui/hud-atlas.uiatlas",
        .content_hash = mirakana::hash_asset_cooked_content(packing_claim),
        .source_revision = 1,
        .dependencies = {texture},
        .content = packing_claim,
    };

    const auto packing = mirakana::runtime::runtime_ui_atlas_payload(packing_record);

    MK_REQUIRE(!packing.succeeded());
    MK_REQUIRE(packing.diagnostic.find("production atlas packing") != std::string::npos);
}

MK_TEST("runtime diagnostics validate tilemap atlas texture rows and dependency edges") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/tilemap-atlas");
    const auto tilemap = mirakana::AssetId::from_name("tilemaps/level");
    const auto texture_payload = cooked_texture_payload(texture);
    const auto tilemap_payload = cooked_tilemap_payload(tilemap, texture, "assets/textures/tilemap-atlas.texture");

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = texture,
                .kind = mirakana::AssetKind::texture,
                .path = "assets/textures/mismatched.texture",
                .content = texture_payload,
                .source_revision = 1,
                .dependencies = {},
            },
            mirakana::AssetCookedArtifact{
                .asset = tilemap,
                .kind = mirakana::AssetKind::tilemap,
                .path = "assets/tilemaps/level.tilemap",
                .content = tilemap_payload,
                .source_revision = 1,
                .dependencies = {texture},
            },
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/mismatched.texture", texture_payload);
    fs.write_text("assets/tilemaps/level.tilemap", tilemap_payload);

    const auto loaded = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(loaded.succeeded());

    const auto report = mirakana::runtime::inspect_runtime_asset_package(loaded.package);

    MK_REQUIRE(!report.succeeded());
    MK_REQUIRE(report.error_count() == 2);
    MK_REQUIRE(report.diagnostics[0].asset == tilemap);
    MK_REQUIRE(report.diagnostics[0].message.find("asset_uri") != std::string::npos);
    MK_REQUIRE(report.diagnostics[1].asset == tilemap);
    MK_REQUIRE(report.diagnostics[1].message.find("tilemap_texture") != std::string::npos);
}

MK_TEST("runtime diagnostics reject tilemap atlas dependencies that are not textures") {
    mirakana::MemoryFileSystem fs;
    const auto audio = mirakana::AssetId::from_name("audio/tilemap-atlas");
    const auto tilemap = mirakana::AssetId::from_name("tilemaps/level");
    const auto audio_payload = cooked_audio_payload(audio);
    const auto tilemap_payload = cooked_tilemap_payload(tilemap, audio, "assets/audio/tilemap-atlas.audio");

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = audio,
                .kind = mirakana::AssetKind::audio,
                .path = "assets/audio/tilemap-atlas.audio",
                .content = audio_payload,
                .source_revision = 1,
                .dependencies = {},
            },
            mirakana::AssetCookedArtifact{
                .asset = tilemap,
                .kind = mirakana::AssetKind::tilemap,
                .path = "assets/tilemaps/level.tilemap",
                .content = tilemap_payload,
                .source_revision = 1,
                .dependencies = {audio},
            },
        },
        {mirakana::AssetDependencyEdge{
            .asset = tilemap,
            .dependency = audio,
            .kind = mirakana::AssetDependencyKind::tilemap_texture,
            .path = "assets/tilemaps/level.tilemap",
        }});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/audio/tilemap-atlas.audio", audio_payload);
    fs.write_text("assets/tilemaps/level.tilemap", tilemap_payload);

    const auto loaded = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(loaded.succeeded());

    const auto report = mirakana::runtime::inspect_runtime_asset_package(loaded.package);

    MK_REQUIRE(!report.succeeded());
    MK_REQUIRE(report.error_count() == 1);
    MK_REQUIRE(report.diagnostics[0].asset == tilemap);
    MK_REQUIRE(report.diagnostics[0].message.find("not a texture") != std::string::npos);
}

MK_TEST("runtime samples visible tilemap cells into deterministic counters") {
    const auto result = mirakana::runtime::sample_runtime_tilemap_visible_cells(runtime_tilemap_payload_for_sampling());

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.layer_count == 2);
    MK_REQUIRE(result.visible_layer_count == 1);
    MK_REQUIRE(result.tile_count == 2);
    MK_REQUIRE(result.non_empty_cell_count == 3);
    MK_REQUIRE(result.sampled_cell_count == 3);
    MK_REQUIRE(result.diagnostic_count == 0);
}

MK_TEST("runtime diagnostics validate sprite animation texture and material dependency edges") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto animation = mirakana::AssetId::from_name("animations/player-idle");
    const auto texture_payload = cooked_texture_payload(texture);
    const auto material_payload = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::unlit,
        .surface_mode = mirakana::MaterialSurfaceMode::transparent,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = true,
    });
    const auto animation_payload = cooked_sprite_animation_payload(animation, texture, material);

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = texture,
                .kind = mirakana::AssetKind::texture,
                .path = "assets/textures/player.texture",
                .content = texture_payload,
                .source_revision = 1,
                .dependencies = {},
            },
            mirakana::AssetCookedArtifact{
                .asset = material,
                .kind = mirakana::AssetKind::material,
                .path = "assets/materials/player.material",
                .content = material_payload,
                .source_revision = 1,
                .dependencies = {texture},
            },
            mirakana::AssetCookedArtifact{
                .asset = animation,
                .kind = mirakana::AssetKind::sprite_animation,
                .path = "assets/animations/player.sprite_animation",
                .content = animation_payload,
                .source_revision = 1,
                .dependencies = {texture, material},
            },
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player.texture", texture_payload);
    fs.write_text("assets/materials/player.material", material_payload);
    fs.write_text("assets/animations/player.sprite_animation", animation_payload);

    const auto loaded = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(loaded.succeeded());

    const auto report = mirakana::runtime::inspect_runtime_asset_package(loaded.package);

    MK_REQUIRE(!report.succeeded());
    MK_REQUIRE(report.error_count() == 2);
    MK_REQUIRE(report.diagnostics[0].asset == animation);
    MK_REQUIRE(report.diagnostics[0].message.find("sprite_animation_material") != std::string::npos ||
               report.diagnostics[0].message.find("sprite_animation_texture") != std::string::npos);
    MK_REQUIRE(report.diagnostics[1].asset == animation);
    MK_REQUIRE(report.diagnostics[1].message.find("sprite_animation_material") != std::string::npos ||
               report.diagnostics[1].message.find("sprite_animation_texture") != std::string::npos);
}

MK_TEST("runtime scene workflow resolves dependent material payloads from a cooked package") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto scene = mirakana::AssetId::from_name("scenes/level01");

    const auto texture_payload = cooked_texture_payload(texture);
    const auto mesh_payload = cooked_mesh_payload(mesh);
    const auto material_payload = mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.1F, 0.2F, 0.3F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.8F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
    const auto scene_payload = cooked_scene_payload();

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = texture,
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "assets/textures/player_albedo.texture",
                                          .content = texture_payload,
                                          .source_revision = 1,
                                          .dependencies = {}},
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
                                          .dependencies = {texture}},
            mirakana::AssetCookedArtifact{.asset = scene,
                                          .kind = mirakana::AssetKind::scene,
                                          .path = "assets/scenes/level01.scene",
                                          .content = scene_payload,
                                          .source_revision = 1,
                                          .dependencies = {mesh, material}},
        },
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player_albedo.texture", texture_payload);
    fs.write_text("assets/meshes/player.mesh", mesh_payload);
    fs.write_text("assets/materials/player.material", material_payload);
    fs.write_text("assets/scenes/level01.scene", scene_payload);

    const auto package = mirakana::runtime::load_runtime_asset_package(fs, {.index_path = "packages/main.geindex"});
    MK_REQUIRE(package.succeeded());

    const auto resolved = mirakana::runtime::resolve_runtime_scene_materials(package.package, scene);

    MK_REQUIRE(resolved.succeeded());
    MK_REQUIRE(resolved.scene.asset == scene);
    MK_REQUIRE(resolved.scene.serialized_scene == scene_payload);
    MK_REQUIRE(resolved.materials.size() == 1);
    MK_REQUIRE(resolved.materials[0].asset == material);
    MK_REQUIRE(resolved.materials[0].material.factors.base_color[0] == 0.1F);
    MK_REQUIRE(resolved.materials[0].binding_metadata.bindings.size() == 4);
    MK_REQUIRE(resolved.failures.empty());
}

MK_TEST("runtime scene workflow reports malformed dependent materials without throwing") {
    const auto material = mirakana::AssetId::from_name("materials/broken");
    const auto scene = mirakana::AssetId::from_name("scenes/broken");
    const auto scene_payload = cooked_scene_payload();
    mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/broken.material",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.Material.v1\nmaterial.name=Broken\n",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = scene,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/broken.scene",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {material},
            .content = scene_payload,
        },
    });

    const auto resolved = mirakana::runtime::resolve_runtime_scene_materials(package, scene);

    MK_REQUIRE(!resolved.succeeded());
    MK_REQUIRE(resolved.scene.asset == scene);
    MK_REQUIRE(resolved.materials.empty());
    MK_REQUIRE(resolved.failures.size() == 1);
    MK_REQUIRE(resolved.failures[0].asset == material);
    MK_REQUIRE(resolved.failures[0].diagnostic.find("material") != std::string::npos);
}

MK_TEST("runtime typed payload access reports kind mismatch without throwing") {
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto mesh_payload = cooked_mesh_payload(mesh);
    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = mesh,
        .kind = mirakana::AssetKind::mesh,
        .path = "assets/meshes/player.mesh",
        .content_hash = mirakana::hash_asset_cooked_content(mesh_payload),
        .source_revision = 1,
        .dependencies = {},
        .content = mesh_payload,
    };

    const auto result = mirakana::runtime::runtime_texture_payload(record);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("texture") != std::string::npos);
}

MK_TEST("runtime typed payload access reports malformed cooked payloads without throwing") {
    const auto texture = mirakana::AssetId::from_name("textures/broken");
    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = texture,
        .kind = mirakana::AssetKind::texture,
        .path = "assets/textures/broken.texture",
        .content_hash = 0,
        .source_revision = 1,
        .dependencies = {},
        .content = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n",
    };

    const auto result = mirakana::runtime::runtime_texture_payload(record);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("missing") != std::string::npos);
}

MK_TEST("runtime physics collision scene payload decodes deterministic body rows") {
    const mirakana::AssetId collision_scene = mirakana::AssetId::from_name("physics/collision/main");
    const std::string payload = cooked_physics_collision_scene_payload(collision_scene);

    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(payload),
        .source_revision = 7,
        .dependencies = {},
        .content = payload,
    };

    const auto result = mirakana::runtime::runtime_physics_collision_scene_3d_payload(record);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.payload.asset == collision_scene);
    MK_REQUIRE(result.payload.bodies.size() == 2);
    MK_REQUIRE(result.payload.bodies[0].name == "floor");
    MK_REQUIRE(result.payload.bodies[0].material == "stone");
    MK_REQUIRE(result.payload.bodies[0].compound == "level_static");
    MK_REQUIRE(result.payload.bodies[0].body.shape == mirakana::PhysicsShape3DKind::aabb);
    MK_REQUIRE(!result.payload.bodies[0].body.dynamic);
    MK_REQUIRE(result.payload.bodies[1].name == "pickup_trigger");
    MK_REQUIRE(result.payload.bodies[1].body.shape == mirakana::PhysicsShape3DKind::sphere);
    MK_REQUIRE(result.payload.bodies[1].body.trigger);
    MK_REQUIRE(result.payload.bodies[1].body.collision_layer == 2U);
    MK_REQUIRE(result.payload.bodies[1].body.collision_mask == 1U);
}

MK_TEST("runtime physics collision scene payload rejects native backend requests") {
    const mirakana::AssetId collision_scene = mirakana::AssetId::from_name("physics/collision/main");
    const std::string payload =
        cooked_physics_collision_scene_payload(collision_scene, "pickup_trigger", "sphere", "2", "required");
    const mirakana::runtime::RuntimeAssetRecord record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(payload),
        .source_revision = 7,
        .dependencies = {},
        .content = payload,
    };

    const auto result = mirakana::runtime::runtime_physics_collision_scene_3d_payload(record);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("native") != std::string::npos);
}

MK_TEST("runtime physics collision scene payload rejects duplicate names and invalid body rows") {
    const mirakana::AssetId collision_scene = mirakana::AssetId::from_name("physics/collision/main");
    const std::string duplicate_name_payload = cooked_physics_collision_scene_payload(collision_scene, "floor");
    const mirakana::runtime::RuntimeAssetRecord duplicate_name_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(duplicate_name_payload),
        .source_revision = 7,
        .dependencies = {},
        .content = duplicate_name_payload,
    };

    const auto duplicate_name = mirakana::runtime::runtime_physics_collision_scene_3d_payload(duplicate_name_record);

    MK_REQUIRE(!duplicate_name.succeeded());
    MK_REQUIRE(duplicate_name.diagnostic.find("body name") != std::string::npos);

    const std::string invalid_layer_payload =
        cooked_physics_collision_scene_payload(collision_scene, "pickup_trigger", "sphere", "0");
    const mirakana::runtime::RuntimeAssetRecord invalid_layer_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(invalid_layer_payload),
        .source_revision = 7,
        .dependencies = {},
        .content = invalid_layer_payload,
    };

    const auto invalid_layer = mirakana::runtime::runtime_physics_collision_scene_3d_payload(invalid_layer_record);

    MK_REQUIRE(!invalid_layer.succeeded());
    MK_REQUIRE(invalid_layer.diagnostic.find("body is invalid") != std::string::npos);

    std::string invalid_compound_payload = cooked_physics_collision_scene_payload(collision_scene);
    invalid_compound_payload.replace(invalid_compound_payload.find("body.0.compound=level_static"),
                                     std::string{"body.0.compound=level_static"}.size(),
                                     "body.0.compound=bad=compound");
    const mirakana::runtime::RuntimeAssetRecord invalid_compound_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(invalid_compound_payload),
        .source_revision = 7,
        .dependencies = {},
        .content = invalid_compound_payload,
    };

    const auto invalid_compound =
        mirakana::runtime::runtime_physics_collision_scene_3d_payload(invalid_compound_record);

    MK_REQUIRE(!invalid_compound.succeeded());
    MK_REQUIRE(invalid_compound.diagnostic.find("compound") != std::string::npos);

    const std::string invalid_name_payload =
        cooked_physics_collision_scene_payload(collision_scene, "bad=name", "sphere", "2");
    const mirakana::runtime::RuntimeAssetRecord invalid_name_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(invalid_name_payload),
        .source_revision = 7,
        .dependencies = {},
        .content = invalid_name_payload,
    };

    const auto invalid_name = mirakana::runtime::runtime_physics_collision_scene_3d_payload(invalid_name_record);

    MK_REQUIRE(!invalid_name.succeeded());
    MK_REQUIRE(invalid_name.diagnostic.find("body name") != std::string::npos);

    std::string invalid_material_payload = cooked_physics_collision_scene_payload(collision_scene);
    invalid_material_payload.replace(invalid_material_payload.find("body.1.material=trigger"),
                                     std::string{"body.1.material=trigger"}.size(), "body.1.material=bad=material");
    const mirakana::runtime::RuntimeAssetRecord invalid_material_record{
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .asset = collision_scene,
        .kind = mirakana::AssetKind::physics_collision_scene,
        .path = "runtime/assets/physics/main.collision3d",
        .content_hash = mirakana::hash_asset_cooked_content(invalid_material_payload),
        .source_revision = 7,
        .dependencies = {},
        .content = invalid_material_payload,
    };

    const auto invalid_material =
        mirakana::runtime::runtime_physics_collision_scene_3d_payload(invalid_material_record);

    MK_REQUIRE(!invalid_material.succeeded());
    MK_REQUIRE(invalid_material.diagnostic.find("material") != std::string::npos);
}

MK_TEST("runtime physics collision scene bridge builds world contact and trigger evidence") {
    const auto payload = runtime_collision_scene_payload_for_bridge();

    const auto result = mirakana::runtime::build_physics_world_3d_from_runtime_collision_scene(payload);

    MK_REQUIRE(result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::none);
    MK_REQUIRE(result.bodies.size() == 3);
    MK_REQUIRE(result.world.bodies().size() == 3);
    MK_REQUIRE(!result.world.contacts().empty());
    MK_REQUIRE(result.world.trigger_overlaps().size() == 1);
}

MK_TEST("runtime physics collision scene bridge rejects duplicate body names") {
    auto payload = runtime_collision_scene_payload_for_bridge();
    payload.bodies[1].name = "floor";

    const auto result = mirakana::runtime::build_physics_world_3d_from_runtime_collision_scene(payload);

    MK_REQUIRE(result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::duplicate_name);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::duplicate_body_name);
}

MK_TEST("runtime diagnostics report package load failures deterministically") {
    const auto texture = mirakana::AssetId::from_name("textures/broken");
    const mirakana::runtime::RuntimeAssetPackageLoadResult loaded{
        .package = mirakana::runtime::RuntimeAssetPackage{},
        .failures = {mirakana::runtime::RuntimeAssetPackageLoadFailure{
            .asset = texture,
            .path = "assets/textures/broken.texture",
            .diagnostic = "cooked payload hash mismatch",
        }},
    };

    const auto report = mirakana::runtime::make_runtime_asset_package_load_diagnostic_report(loaded);

    MK_REQUIRE(!report.succeeded());
    MK_REQUIRE(report.error_count() == 1);
    MK_REQUIRE(report.warning_count() == 0);
    MK_REQUIRE(report.diagnostics.size() == 1);
    MK_REQUIRE(report.diagnostics[0].severity == mirakana::runtime::RuntimeDiagnosticSeverity::error);
    MK_REQUIRE(report.diagnostics[0].domain == mirakana::runtime::RuntimeDiagnosticDomain::asset_package);
    MK_REQUIRE(report.diagnostics[0].asset == texture);
    MK_REQUIRE(report.diagnostics[0].path == "assets/textures/broken.texture");
    MK_REQUIRE(report.diagnostics[0].message.find("hash") != std::string::npos);
    MK_REQUIRE(mirakana::runtime::runtime_diagnostic_severity_label(report.diagnostics[0].severity) == "Error");
    MK_REQUIRE(mirakana::runtime::runtime_diagnostic_domain_label(report.diagnostics[0].domain) == "Asset Package");
}

MK_TEST("runtime diagnostics inspect typed payload and scene material failures") {
    const auto texture = mirakana::AssetId::from_name("textures/valid");
    const auto mesh = mirakana::AssetId::from_name("meshes/broken");
    const auto material = mirakana::AssetId::from_name("materials/broken");
    const auto scene = mirakana::AssetId::from_name("scenes/level01");
    const auto texture_payload = cooked_texture_payload(texture);
    const auto scene_payload = cooked_scene_payload();

    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/valid.texture",
            .content_hash = mirakana::hash_asset_cooked_content(texture_payload),
            .source_revision = 1,
            .dependencies = {},
            .content = texture_payload,
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/broken.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.CookedMesh.v2\nmesh.vertex_count=3\n",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/broken.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.Material.v1\nmaterial.name=Broken\n",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{4},
            .asset = scene,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/level01.scene",
            .content_hash = 4,
            .source_revision = 1,
            .dependencies = {material},
            .content = scene_payload,
        },
    });

    const auto report = mirakana::runtime::inspect_runtime_asset_package(package);

    MK_REQUIRE(!report.succeeded());
    MK_REQUIRE(report.error_count() == 3);
    MK_REQUIRE(report.diagnostics.size() == 3);
    MK_REQUIRE(report.diagnostics[0].domain == mirakana::runtime::RuntimeDiagnosticDomain::payload);
    MK_REQUIRE(report.diagnostics[0].asset == mesh);
    MK_REQUIRE(report.diagnostics[0].kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(report.diagnostics[0].message.find("runtime mesh") != std::string::npos);
    MK_REQUIRE(report.diagnostics[1].domain == mirakana::runtime::RuntimeDiagnosticDomain::payload);
    MK_REQUIRE(report.diagnostics[1].asset == material);
    MK_REQUIRE(report.diagnostics[1].kind == mirakana::AssetKind::material);
    MK_REQUIRE(report.diagnostics[1].path == "assets/materials/broken.material");
    MK_REQUIRE(report.diagnostics[1].message.find("material") != std::string::npos);
    MK_REQUIRE(report.diagnostics[2].domain == mirakana::runtime::RuntimeDiagnosticDomain::scene);
    MK_REQUIRE(report.diagnostics[2].asset == material);
    MK_REQUIRE(report.diagnostics[2].kind == mirakana::AssetKind::material);
    MK_REQUIRE(report.diagnostics[2].path == "assets/materials/broken.material");
    MK_REQUIRE(report.diagnostics[2].message.find("material") != std::string::npos);
}

MK_TEST("runtime diagnostics report session document load failures") {
    const auto settings = mirakana::runtime::deserialize_runtime_settings("format=GameEngine.RuntimeSettings.v1\n"
                                                                          "schema.version=1\n"
                                                                          "entry.audio.master_volume=0.75\n"
                                                                          "entry.audio.master_volume=1.0\n");
    const auto settings_report =
        mirakana::runtime::make_runtime_session_diagnostic_report(settings, "settings/broken.settings");
    MK_REQUIRE(!settings_report.succeeded());
    MK_REQUIRE(settings_report.error_count() == 1);
    MK_REQUIRE(settings_report.diagnostics[0].domain == mirakana::runtime::RuntimeDiagnosticDomain::session);
    MK_REQUIRE(settings_report.diagnostics[0].path == "settings/broken.settings");
    MK_REQUIRE(settings_report.diagnostics[0].message.find("duplicate") != std::string::npos);

    const auto input = mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v2\n"
                                                                            "bind.jump=key:space,key:missing\n");
    const auto input_report = mirakana::runtime::make_runtime_session_diagnostic_report(input, "input/broken.geinput");
    MK_REQUIRE(!input_report.succeeded());
    MK_REQUIRE(input_report.diagnostics[0].domain == mirakana::runtime::RuntimeDiagnosticDomain::session);
    MK_REQUIRE(input_report.diagnostics[0].message.find("unsupported") != std::string::npos);

    mirakana::runtime::RuntimeSaveData save;
    save.set_value("slot", "1");
    const auto save_report = mirakana::runtime::make_runtime_session_diagnostic_report(
        mirakana::runtime::deserialize_runtime_save_data(mirakana::runtime::serialize_runtime_save_data(save)),
        "saves/slot01");
    MK_REQUIRE(save_report.succeeded());
    MK_REQUIRE(save_report.diagnostics.empty());
}

MK_TEST("runtime save data writes and loads deterministic key values") {
    mirakana::MemoryFileSystem fs;
    mirakana::runtime::RuntimeSaveData save;
    save.schema_version = 3;
    save.set_value("player.name", "Ada");
    save.set_value("player.level", "4");
    save.set_value("player.name", "Grace");

    mirakana::runtime::write_runtime_save_data(fs, "saves/slot01.gesave", save);

    const auto text = fs.read_text("saves/slot01.gesave");
    MK_REQUIRE(text.find("format=GameEngine.RuntimeSaveData.v1\n") == 0);
    MK_REQUIRE(text.find("schema.version=3\n") != std::string::npos);
    MK_REQUIRE(text.find("entry.player.level=4\nentry.player.name=Grace\n") != std::string::npos);

    const auto loaded = mirakana::runtime::load_runtime_save_data(fs, "saves/slot01.gesave");
    MK_REQUIRE(loaded.succeeded());
    MK_REQUIRE(loaded.data.schema_version == 3);
    MK_REQUIRE(loaded.data.value_or("player.name", "") == "Grace");
    MK_REQUIRE(loaded.data.value_or("player.level", "") == "4");
    MK_REQUIRE(loaded.data.value_or("missing", "fallback") == "fallback");
}

MK_TEST("runtime settings loads defaults and rejects malformed documents") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("settings/game.settings", "format=GameEngine.RuntimeSettings.v1\n"
                                            "schema.version=2\n"
                                            "entry.audio.master_volume=0.75\n"
                                            "entry.video.fullscreen=false\n");

    const auto loaded = mirakana::runtime::load_runtime_settings(fs, "settings/game.settings");
    MK_REQUIRE(loaded.succeeded());
    MK_REQUIRE(loaded.settings.schema_version == 2);
    MK_REQUIRE(loaded.settings.value_or("audio.master_volume", "1.0") == "0.75");
    MK_REQUIRE(loaded.settings.value_or("video.fullscreen", "true") == "false");

    fs.write_text("settings/broken.settings", "format=GameEngine.RuntimeSettings.v1\n"
                                              "schema.version=1\n"
                                              "entry.audio.master_volume=0.75\n"
                                              "entry.audio.master_volume=1.0\n");

    const auto broken = mirakana::runtime::load_runtime_settings(fs, "settings/broken.settings");
    MK_REQUIRE(!broken.succeeded());
    MK_REQUIRE(broken.diagnostic.find("duplicate") != std::string::npos);
}

MK_TEST("runtime localization catalog resolves text with key fallback") {
    mirakana::MemoryFileSystem fs;
    mirakana::runtime::RuntimeLocalizationCatalog catalog;
    catalog.locale = "en-US";
    catalog.set_text("menu.start", "Start");
    catalog.set_text("menu.quit", "Quit");

    mirakana::runtime::write_runtime_localization_catalog(fs, "locale/en-US.gelocale", catalog);

    const auto loaded = mirakana::runtime::load_runtime_localization_catalog(fs, "locale/en-US.gelocale");
    MK_REQUIRE(loaded.succeeded());
    MK_REQUIRE(loaded.catalog.locale == "en-US");
    MK_REQUIRE(loaded.catalog.text_or_key("menu.start") == "Start");
    MK_REQUIRE(loaded.catalog.text_or_key("menu.missing") == "menu.missing");
}

MK_TEST("runtime input action map evaluates virtual input bindings") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("move_left", mirakana::Key::left);
    actions.bind_key("jump", mirakana::Key::space);
    actions.bind_key("jump", mirakana::Key::up);

    mirakana::VirtualInput input;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &input;

    input.press(mirakana::Key::space);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));
    MK_REQUIRE(!actions.action_down("move_left", state));

    input.begin_frame();
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(!actions.action_pressed("jump", state));

    input.release(mirakana::Key::space);
    MK_REQUIRE(actions.action_released("jump", state));
}

MK_TEST("runtime input action map evaluates key pointer and gamepad triggers") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("jump", mirakana::Key::space);
    actions.bind_pointer("jump", mirakana::primary_pointer_id);
    actions.bind_gamepad_button("jump", mirakana::GamepadId{1}, mirakana::GamepadButton::south);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualPointerInput pointer;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.pointer = &pointer;
    state.gamepad = &gamepad;

    keyboard.press(mirakana::Key::space);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));
    keyboard.release(mirakana::Key::space);
    MK_REQUIRE(!actions.action_down("jump", state));
    MK_REQUIRE(actions.action_released("jump", state));

    keyboard.begin_frame();
    pointer.press(mirakana::PointerSample{.id = mirakana::primary_pointer_id,
                                          .kind = mirakana::PointerKind::mouse,
                                          .position = mirakana::Vec2{.x = 16.0F, .y = 24.0F}});
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));
    pointer.begin_frame();
    pointer.release(mirakana::primary_pointer_id);
    MK_REQUIRE(!actions.action_down("jump", state));
    MK_REQUIRE(actions.action_released("jump", state));

    pointer.begin_frame();
    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));
    gamepad.begin_frame();
    gamepad.release(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(!actions.action_down("jump", state));
    MK_REQUIRE(actions.action_released("jump", state));
}

MK_TEST("runtime input action transitions use logical action state across multiple keys") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("jump", mirakana::Key::space);
    actions.bind_key("jump", mirakana::Key::up);

    mirakana::VirtualInput input;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &input;

    input.press(mirakana::Key::space);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));

    input.begin_frame();
    input.press(mirakana::Key::up);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(!actions.action_pressed("jump", state));

    input.begin_frame();
    input.release(mirakana::Key::up);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(!actions.action_released("jump", state));

    input.release(mirakana::Key::space);
    MK_REQUIRE(!actions.action_down("jump", state));
    MK_REQUIRE(actions.action_released("jump", state));
}

MK_TEST("runtime input action transitions use logical action state across devices") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_pointer("jump", mirakana::primary_pointer_id);
    actions.bind_gamepad_button("jump", mirakana::GamepadId{1}, mirakana::GamepadButton::south);

    mirakana::VirtualPointerInput pointer;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.pointer = &pointer;
    state.gamepad = &gamepad;

    pointer.press(mirakana::PointerSample{.id = mirakana::primary_pointer_id,
                                          .kind = mirakana::PointerKind::mouse,
                                          .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(actions.action_pressed("jump", state));

    pointer.begin_frame();
    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(!actions.action_pressed("jump", state));

    gamepad.begin_frame();
    pointer.release(mirakana::primary_pointer_id);
    MK_REQUIRE(actions.action_down("jump", state));
    MK_REQUIRE(!actions.action_released("jump", state));

    pointer.begin_frame();
    gamepad.release(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(!actions.action_down("jump", state));
    MK_REQUIRE(actions.action_released("jump", state));
}

MK_TEST("runtime input action axes evaluate key pairs and gamepad axes") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis("move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F);
    actions.bind_gamepad_axis("look_y", mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, -1.0F, 0.2F);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;

    keyboard.press(mirakana::Key::left);
    MK_REQUIRE(actions.axis_value("move_x", state) == -1.0F);

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(actions.axis_value("move_x", state) == 0.0F);

    keyboard.release(mirakana::Key::left);
    MK_REQUIRE(actions.axis_value("move_x", state) == 1.0F);

    keyboard.release(mirakana::Key::right);
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.5F);
    MK_REQUIRE(nearly_equal(actions.axis_value("move_x", state), 0.33333334F));

    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.1F);
    MK_REQUIRE(actions.axis_value("move_x", state) == 0.0F);

    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.6F);
    MK_REQUIRE(nearly_equal(actions.axis_value("look_y", state), -0.5F));
}

MK_TEST("runtime input action axes choose strongest source deterministically") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_gamepad_axis("move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::right_x);
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis("move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;

    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.4F);
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_x, -0.75F);
    MK_REQUIRE(actions.axis_value("move_x", state) == -0.75F);

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(actions.axis_value("move_x", state) == 1.0F);

    keyboard.release(mirakana::Key::right);
    keyboard.press(mirakana::Key::left);
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F);
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_x, 0.0F);
    MK_REQUIRE(actions.axis_value("move_x", state) == -1.0F);
}

MK_TEST("runtime input action contexts select first active layer") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key_in_context("menu", "confirm", mirakana::Key::space);
    actions.bind_gamepad_button_in_context("gameplay", "confirm", mirakana::GamepadId{1},
                                           mirakana::GamepadButton::south);
    actions.bind_key_in_context("gameplay", "jump", mirakana::Key::up);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;

    mirakana::runtime::RuntimeInputContextStack stack;
    stack.active_contexts = {"menu", "gameplay"};

    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(!actions.action_down("confirm", state, stack));

    keyboard.press(mirakana::Key::space);
    MK_REQUIRE(actions.action_down("confirm", state, stack));
    MK_REQUIRE(actions.action_pressed("confirm", state, stack));

    keyboard.press(mirakana::Key::up);
    MK_REQUIRE(actions.action_down("jump", state, stack));
}

MK_TEST("runtime input action context axes select first active layer") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key_axis_in_context("menu", "move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;

    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.75F);

    mirakana::runtime::RuntimeInputContextStack menu_first;
    menu_first.active_contexts = {"menu", "gameplay"};
    MK_REQUIRE(actions.axis_value("move_x", state, menu_first) == 0.0F);

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(actions.axis_value("move_x", state, menu_first) == 1.0F);

    mirakana::runtime::RuntimeInputContextStack gameplay_first;
    gameplay_first.active_contexts = {"gameplay", "menu"};
    MK_REQUIRE(actions.axis_value("move_x", state, gameplay_first) == 0.75F);
}

MK_TEST("runtime input action contexts use default layer when stack is empty") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("confirm", mirakana::Key::space);
    actions.bind_key_in_context("menu", "confirm", mirakana::Key::escape);
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_key_axis_in_context("menu", "move_x", mirakana::Key::up, mirakana::Key::down);

    mirakana::VirtualInput keyboard;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;

    mirakana::runtime::RuntimeInputContextStack stack;

    keyboard.press(mirakana::Key::escape);
    MK_REQUIRE(!actions.action_down("confirm", state, stack));

    keyboard.press(mirakana::Key::space);
    MK_REQUIRE(actions.action_down("confirm", state, stack));

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(actions.axis_value("move_x", state, stack) == 1.0F);
}

MK_TEST("runtime input action contexts do not fall back to default when stack is active") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("confirm", mirakana::Key::space);
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::VirtualInput keyboard;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;

    mirakana::runtime::RuntimeInputContextStack stack;
    stack.active_contexts = {"menu"};

    keyboard.press(mirakana::Key::space);
    keyboard.press(mirakana::Key::right);

    MK_REQUIRE(!actions.action_down("confirm", state, stack));
    MK_REQUIRE(actions.axis_value("move_x", state, stack) == 0.0F);
}

MK_TEST("runtime input action duplicate bind calls are ignored") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    actions.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    actions.bind_pointer_in_context("gameplay", "confirm", mirakana::primary_pointer_id);
    actions.bind_pointer_in_context("gameplay", "confirm", mirakana::primary_pointer_id);
    actions.bind_gamepad_button_in_context("gameplay", "confirm", mirakana::GamepadId{1},
                                           mirakana::GamepadButton::south);
    actions.bind_gamepad_button_in_context("gameplay", "confirm", mirakana::GamepadId{1},
                                           mirakana::GamepadButton::south);
    actions.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x,
                                         1.0F, 0.25F);
    actions.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x,
                                         1.0F, 0.25F);

    const auto* confirm = actions.find("gameplay", "confirm");
    MK_REQUIRE(confirm != nullptr);
    MK_REQUIRE(confirm->triggers.size() == 3U);

    const auto* move_x = actions.find_axis("gameplay", "move_x");
    MK_REQUIRE(move_x != nullptr);
    MK_REQUIRE(move_x->sources.size() == 2U);
}

MK_TEST("runtime input action maps serialize with stable key names") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("jump", mirakana::Key::space);
    actions.bind_key("jump", mirakana::Key::up);
    actions.bind_key("move_left", mirakana::Key::left);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);
    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.default.jump=key:space,key:up\n"
                       "bind.default.move_left=key:left\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_actions(text);
    MK_REQUIRE(loaded.succeeded());

    mirakana::VirtualInput input;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &input;

    input.press(mirakana::Key::up);
    MK_REQUIRE(loaded.actions.action_down("jump", state));
}

MK_TEST("runtime input action maps support text edit key names") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("erase_backward", mirakana::Key::backspace);
    actions.bind_key("erase_forward", mirakana::Key::delete_key);
    actions.bind_key("line_end", mirakana::Key::end);
    actions.bind_key("line_start", mirakana::Key::home);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);
    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.default.erase_backward=key:backspace\n"
                       "bind.default.erase_forward=key:delete\n"
                       "bind.default.line_end=key:end\n"
                       "bind.default.line_start=key:home\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_actions(text);
    MK_REQUIRE(loaded.succeeded());

    mirakana::VirtualInput input;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &input;

    input.press(mirakana::Key::backspace);
    input.press(mirakana::Key::delete_key);
    input.press(mirakana::Key::home);
    input.press(mirakana::Key::end);
    MK_REQUIRE(loaded.actions.action_down("erase_backward", state));
    MK_REQUIRE(loaded.actions.action_down("erase_forward", state));
    MK_REQUIRE(loaded.actions.action_down("line_start", state));
    MK_REQUIRE(loaded.actions.action_down("line_end", state));
}

MK_TEST("runtime input action maps serialize canonical device triggers") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_gamepad_button("jump", mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    actions.bind_pointer("jump", mirakana::primary_pointer_id);
    actions.bind_key("jump", mirakana::Key::space);
    actions.bind_key("move_left", mirakana::Key::left);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);

    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.default.jump=key:space,pointer:1,gamepad:1:south\n"
                       "bind.default.move_left=key:left\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_actions(text);
    MK_REQUIRE(loaded.succeeded());

    mirakana::VirtualInput keyboard;
    mirakana::VirtualPointerInput pointer;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.pointer = &pointer;
    state.gamepad = &gamepad;

    pointer.press(mirakana::PointerSample{.id = mirakana::primary_pointer_id,
                                          .kind = mirakana::PointerKind::mouse,
                                          .position = mirakana::Vec2{.x = 2.0F, .y = 4.0F}});
    MK_REQUIRE(loaded.actions.action_down("jump", state));
    pointer.release(mirakana::primary_pointer_id);
    pointer.begin_frame();
    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(loaded.actions.action_down("jump", state));
}

MK_TEST("runtime input action maps serialize canonical v4 default axis documents") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_gamepad_axis("move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F);
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis("look_y", mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, -1.0F, 0.2F);
    actions.bind_key("jump", mirakana::Key::space);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);

    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.default.jump=key:space\n"
                       "axis.default.look_y=gamepad:1:right_y:-1:0.2\n"
                       "axis.default.move_x=keys:left:right,gamepad:1:left_x:1:0.25\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_actions(text);
    MK_REQUIRE(loaded.succeeded());

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(loaded.actions.axis_value("move_x", state) == 1.0F);

    keyboard.release(mirakana::Key::right);
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.6F);
    MK_REQUIRE(nearly_equal(loaded.actions.axis_value("look_y", state), -0.5F));
}

MK_TEST("runtime input action maps serialize canonical v4 context documents") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_gamepad_button_in_context("gameplay", "confirm", mirakana::GamepadId{1},
                                           mirakana::GamepadButton::south);
    actions.bind_key_in_context("menu", "confirm", mirakana::Key::space);
    actions.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x,
                                         1.0F, 0.25F);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);

    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.gameplay.confirm=gamepad:1:south\n"
                       "bind.menu.confirm=key:space\n"
                       "axis.gameplay.move_x=keys:left:right,gamepad:1:left_x:1:0.25\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_actions(text);
    MK_REQUIRE(loaded.succeeded());

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;
    mirakana::runtime::RuntimeInputContextStack stack;
    stack.active_contexts = {"menu", "gameplay"};

    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(!loaded.actions.action_down("confirm", state, stack));
    keyboard.press(mirakana::Key::space);
    MK_REQUIRE(loaded.actions.action_down("confirm", state, stack));
}

MK_TEST("runtime input action serialization is canonical across insertion order") {
    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key("jump", mirakana::Key::up);
    actions.bind_key("jump", mirakana::Key::space);

    const auto text = mirakana::runtime::serialize_runtime_input_actions(actions);

    MK_REQUIRE(text == "format=GameEngine.RuntimeInputActions.v4\n"
                       "bind.default.jump=key:space,key:up\n");
}

MK_TEST("runtime input rebinding profile applies digital and axis overrides") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_gamepad_button_in_context("gameplay", "confirm", mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::space)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, profile);

    MK_REQUIRE(applied.succeeded());
    MK_REQUIRE(applied.action_overrides_applied == 1U);
    MK_REQUIRE(applied.axis_overrides_applied == 1U);

    mirakana::VirtualInput keyboard;
    mirakana::VirtualGamepadInput gamepad;
    mirakana::runtime::RuntimeInputStateView state;
    state.keyboard = &keyboard;
    state.gamepad = &gamepad;
    mirakana::runtime::RuntimeInputContextStack stack;
    stack.active_contexts = {"gameplay"};

    gamepad.press(mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    MK_REQUIRE(!applied.actions.action_down("confirm", state, stack));

    keyboard.press(mirakana::Key::space);
    MK_REQUIRE(applied.actions.action_down("confirm", state, stack));

    keyboard.press(mirakana::Key::right);
    MK_REQUIRE(applied.actions.axis_value("move_x", state, stack) == 0.0F);

    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.5F);
    MK_REQUIRE(nearly_equal(applied.actions.axis_value("move_x", state, stack), 0.33333334F));
}

MK_TEST("runtime input rebinding capture creates action override from pressed key") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);

    mirakana::runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = "gameplay";
    request.action = "confirm";
    request.state.keyboard = &keyboard;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);

    MK_REQUIRE(result.captured());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::captured);
    MK_REQUIRE(result.trigger.has_value());
    MK_REQUIRE(result.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(result.trigger->key == mirakana::Key::up);
    MK_REQUIRE(result.action_override.context == "gameplay");
    MK_REQUIRE(result.action_override.action == "confirm");
    MK_REQUIRE(result.action_override.triggers.size() == 1);
    MK_REQUIRE(result.action_override.triggers[0].key == mirakana::Key::up);
    MK_REQUIRE(result.candidate_profile.profile_id == "player_one");
    MK_REQUIRE(result.candidate_profile.action_overrides.size() == 1);

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, result.candidate_profile);
    MK_REQUIRE(applied.succeeded());

    mirakana::runtime::RuntimeInputStateView applied_state;
    applied_state.keyboard = &keyboard;
    mirakana::runtime::RuntimeInputContextStack stack;
    stack.active_contexts = {"gameplay"};
    MK_REQUIRE(applied.actions.action_down("confirm", applied_state, stack));
}

MK_TEST("runtime input rebinding capture accepts text edit key ids") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "erase", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::backspace);

    mirakana::runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = "gameplay";
    request.action = "erase";
    request.state.keyboard = &keyboard;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);

    MK_REQUIRE(result.captured());
    MK_REQUIRE(result.trigger.has_value());
    MK_REQUIRE(result.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(result.trigger->key == mirakana::Key::backspace);
    MK_REQUIRE(result.action_override.triggers.size() == 1);
    MK_REQUIRE(result.action_override.triggers[0].key == mirakana::Key::backspace);

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, result.candidate_profile);
    MK_REQUIRE(applied.succeeded());
}

MK_TEST("runtime input rebinding capture waits when no allowed input is pressed") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::escape)}});

    mirakana::VirtualInput keyboard;
    mirakana::runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = "gameplay";
    request.action = "confirm";
    request.state.keyboard = &keyboard;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);

    MK_REQUIRE(result.waiting());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting);
    MK_REQUIRE(!result.trigger.has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.candidate_profile.profile_id == "player_one");
    MK_REQUIRE(result.candidate_profile.action_overrides.size() == 1);
    MK_REQUIRE(result.candidate_profile.action_overrides[0].triggers[0].key == mirakana::Key::escape);
}

MK_TEST("runtime input rebinding capture uses deterministic source priority") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    mirakana::VirtualPointerInput pointer;
    keyboard.press(mirakana::Key::up);
    pointer.press(mirakana::PointerSample{.id = mirakana::PointerId{7},
                                          .kind = mirakana::PointerKind::mouse,
                                          .position = mirakana::Vec2{.x = 2.0F, .y = 3.0F}});

    mirakana::runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = "gameplay";
    request.action = "confirm";
    request.state.keyboard = &keyboard;
    request.state.pointer = &pointer;

    const auto key_result = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);
    MK_REQUIRE(key_result.captured());
    MK_REQUIRE(key_result.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(key_result.trigger->key == mirakana::Key::up);

    request.capture_keyboard = false;
    const auto pointer_result = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);
    MK_REQUIRE(pointer_result.captured());
    MK_REQUIRE(pointer_result.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::pointer);
    MK_REQUIRE(pointer_result.trigger->pointer_id == mirakana::PointerId{7});
}

MK_TEST("runtime input rebinding capture blocks missing base and conflicting trigger candidates") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::escape);

    mirakana::runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = "gameplay";
    request.action = "missing";
    request.state.keyboard = &keyboard;

    const auto missing = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked);
    MK_REQUIRE(!missing.trigger.has_value());
    MK_REQUIRE(has_rebinding_diagnostic(missing.diagnostics, Code::missing_base_binding));

    request.action = "confirm";
    const auto conflict = mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, request);
    MK_REQUIRE(conflict.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked);
    MK_REQUIRE(conflict.trigger.has_value());
    MK_REQUIRE(conflict.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(conflict.trigger->key == mirakana::Key::escape);
    MK_REQUIRE(has_rebinding_diagnostic(conflict.diagnostics, Code::trigger_conflict));
}

MK_TEST("runtime input rebinding axis capture blocks missing base axis binding") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F);

    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest request;
    request.context = "gameplay";
    request.action = "missing_axis";
    request.state.gamepad = &gamepad;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, request);
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked);
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::missing_base_binding));
}

MK_TEST("runtime input rebinding axis capture waits when no axis exceeds deadzone") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 0.1F);

    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest request;
    request.context = "gameplay";
    request.action = "move_x";
    request.state.gamepad = &gamepad;
    request.capture_deadzone = 0.25F;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, request);
    MK_REQUIRE(result.waiting());
    MK_REQUIRE(!result.source.has_value());
}

MK_TEST("runtime input rebinding axis capture captures gamepad axis override") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F,
                                      0.15F);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.9F);

    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest request;
    request.context = "gameplay";
    request.action = "move_x";
    request.state.gamepad = &gamepad;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, request);
    MK_REQUIRE(result.captured());
    MK_REQUIRE(result.source.has_value());
    MK_REQUIRE(result.source->kind == mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis);
    MK_REQUIRE(result.source->gamepad_id == mirakana::GamepadId{1});
    MK_REQUIRE(result.source->gamepad_axis == mirakana::GamepadAxis::right_y);
    MK_REQUIRE(result.axis_override.sources.size() == 1);

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, result.candidate_profile);
    MK_REQUIRE(applied.succeeded());
}

MK_TEST("runtime input rebinding axis capture captures keyboard key pair when gamepad capture is disabled") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::up, mirakana::Key::down);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.begin_frame();
    keyboard.press(mirakana::Key::left);
    keyboard.press(mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest request;
    request.context = "gameplay";
    request.action = "move_x";
    request.state.keyboard = &keyboard;
    request.capture_gamepad_axes = false;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, request);
    MK_REQUIRE(result.captured());
    MK_REQUIRE(result.source.has_value());
    MK_REQUIRE(result.source->kind == mirakana::runtime::RuntimeInputAxisSourceKind::key_pair);
    MK_REQUIRE(result.source->negative_key == mirakana::Key::left);
    MK_REQUIRE(result.source->positive_key == mirakana::Key::right);
}

MK_TEST("runtime input rebinding focused capture waits and consumes gameplay input") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "rebinding.confirm";
    request.focused_id = "rebinding.confirm";
    request.modal_layer_id = "rebinding.confirm";
    request.armed = true;
    request.consume_gameplay_input = true;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.waiting());
    MK_REQUIRE(result.capture.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting);
    MK_REQUIRE(result.gameplay_input_consumed);
    MK_REQUIRE(result.focus_retained);
    MK_REQUIRE(result.active_capture_id == "rebinding.confirm");
    MK_REQUIRE(result.capture.candidate_profile.profile_id == "player_one");
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime input rebinding focused capture can retain focus without consuming gameplay input") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "rebinding.confirm";
    request.focused_id = "rebinding.confirm";
    request.modal_layer_id = "rebinding.confirm";
    request.armed = true;
    request.consume_gameplay_input = false;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.waiting());
    MK_REQUIRE(!result.gameplay_input_consumed);
    MK_REQUIRE(result.focus_retained);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime input rebinding focused capture captures candidate and consumes pressed input") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);

    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "rebinding.confirm";
    request.focused_id = "rebinding.confirm";
    request.modal_layer_id = "rebinding.confirm";
    request.armed = true;
    request.consume_gameplay_input = true;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.captured());
    MK_REQUIRE(result.capture.trigger.has_value());
    MK_REQUIRE(result.capture.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(result.capture.trigger->key == mirakana::Key::up);
    MK_REQUIRE(result.capture.candidate_profile.action_overrides.size() == 1);
    MK_REQUIRE(result.gameplay_input_consumed);
    MK_REQUIRE(!result.focus_retained);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime input rebinding focused capture consumes rejected pressed input") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::escape);

    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "rebinding.confirm";
    request.focused_id = "rebinding.confirm";
    request.modal_layer_id = "rebinding.confirm";
    request.armed = true;
    request.consume_gameplay_input = true;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.blocked());
    MK_REQUIRE(result.capture.trigger.has_value());
    MK_REQUIRE(result.capture.trigger->kind == mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(result.capture.trigger->key == mirakana::Key::escape);
    MK_REQUIRE(result.gameplay_input_consumed);
    MK_REQUIRE(!result.focus_retained);
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::trigger_conflict));
}

MK_TEST("runtime input rebinding focused capture blocks invalid focus guard before consuming input") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);

    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "rebinding.confirm";
    request.focused_id = "settings.close";
    request.modal_layer_id = "settings.close";
    request.armed = false;
    request.consume_gameplay_input = true;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.blocked());
    MK_REQUIRE(result.capture.status == mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked);
    MK_REQUIRE(!result.capture.trigger.has_value());
    MK_REQUIRE(!result.gameplay_input_consumed);
    MK_REQUIRE(!result.focus_retained);
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::capture_not_armed));
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::invalid_capture_focus));
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::modal_layer_mismatch));
}

MK_TEST("runtime input rebinding focused capture blocks empty capture id") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);

    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest request;
    request.capture.context = "gameplay";
    request.capture.action = "confirm";
    request.capture.state.keyboard = &keyboard;
    request.capture_id = "";
    request.focused_id = "";
    request.modal_layer_id = "";
    request.armed = true;
    request.consume_gameplay_input = true;

    const auto result = mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, request);

    MK_REQUIRE(result.blocked());
    MK_REQUIRE(!result.capture.trigger.has_value());
    MK_REQUIRE(!result.gameplay_input_consumed);
    MK_REQUIRE(!result.focus_retained);
    MK_REQUIRE(has_rebinding_diagnostic(result.diagnostics, Code::invalid_capture_id));
}

MK_TEST("runtime input rebinding presentation rows expose action trigger tokens") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_pointer_in_context("gameplay", "confirm", mirakana::primary_pointer_id);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});

    const auto model = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    MK_REQUIRE(model.ready());
    MK_REQUIRE(model.ready_for_display);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(model.has_profile_overrides);
    MK_REQUIRE(model.rows.size() == 1);

    const auto* row = find_rebinding_presentation_row(model.rows, "bind.gameplay.confirm");
    MK_REQUIRE(row != nullptr);
    MK_REQUIRE(row->kind == mirakana::runtime::RuntimeInputRebindingPresentationRowKind::action);
    MK_REQUIRE(row->context == "gameplay");
    MK_REQUIRE(row->action == "confirm");
    MK_REQUIRE(row->overridden);
    MK_REQUIRE(row->ready);
    MK_REQUIRE(row->base_tokens.size() == 2);
    MK_REQUIRE(row->base_tokens[0].kind == mirakana::runtime::RuntimeInputRebindingPresentationTokenKind::key);
    MK_REQUIRE(row->base_tokens[0].label == "key:space");
    MK_REQUIRE(row->base_tokens[0].glyph_lookup_key == "keyboard.key.space");
    MK_REQUIRE(row->base_tokens[1].kind == mirakana::runtime::RuntimeInputRebindingPresentationTokenKind::pointer);
    MK_REQUIRE(row->base_tokens[1].label == "pointer:1");
    MK_REQUIRE(row->base_tokens[1].glyph_lookup_key == "pointer.1");
    MK_REQUIRE(row->base_tokens[1].pointer_id == mirakana::primary_pointer_id);
    MK_REQUIRE(row->profile_tokens.size() == 1);
    MK_REQUIRE(row->profile_tokens[0].kind ==
               mirakana::runtime::RuntimeInputRebindingPresentationTokenKind::gamepad_button);
    MK_REQUIRE(row->profile_tokens[0].label == "gamepad:1:south");
    MK_REQUIRE(row->profile_tokens[0].glyph_lookup_key == "gamepad.1.button.south");
    MK_REQUIRE(row->profile_tokens[0].gamepad_id == mirakana::GamepadId{1});
}

MK_TEST("runtime input rebinding presentation rows expose text edit key tokens") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "erase", mirakana::Key::backspace);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "erase", .triggers = {key_trigger(mirakana::Key::delete_key)}});

    const auto model = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    MK_REQUIRE(model.ready());
    MK_REQUIRE(model.rows.size() == 1);

    const auto* row = find_rebinding_presentation_row(model.rows, "bind.gameplay.erase");
    MK_REQUIRE(row != nullptr);
    MK_REQUIRE(row->base_tokens.size() == 1);
    MK_REQUIRE(row->base_tokens[0].label == "key:backspace");
    MK_REQUIRE(row->base_tokens[0].glyph_lookup_key == "keyboard.key.backspace");
    MK_REQUIRE(row->profile_tokens.size() == 1);
    MK_REQUIRE(row->profile_tokens[0].label == "key:delete");
    MK_REQUIRE(row->profile_tokens[0].glyph_lookup_key == "keyboard.key.delete");
}

MK_TEST("runtime input rebinding presentation rows expose axis source tokens") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});

    const auto model = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    MK_REQUIRE(model.ready());
    MK_REQUIRE(model.rows.size() == 1);

    const auto* row = find_rebinding_presentation_row(model.rows, "axis.gameplay.move_x");
    MK_REQUIRE(row != nullptr);
    MK_REQUIRE(row->kind == mirakana::runtime::RuntimeInputRebindingPresentationRowKind::axis);
    MK_REQUIRE(row->overridden);
    MK_REQUIRE(row->base_tokens.size() == 1);
    MK_REQUIRE(row->base_tokens[0].kind == mirakana::runtime::RuntimeInputRebindingPresentationTokenKind::key_pair);
    MK_REQUIRE(row->base_tokens[0].label == "keys:left/right");
    MK_REQUIRE(row->base_tokens[0].glyph_lookup_key == "keyboard.axis.left.right");
    MK_REQUIRE(row->profile_tokens.size() == 1);
    MK_REQUIRE(row->profile_tokens[0].kind ==
               mirakana::runtime::RuntimeInputRebindingPresentationTokenKind::gamepad_axis);
    MK_REQUIRE(row->profile_tokens[0].label == "gamepad:1:left_x scale=1 deadzone=0.25");
    MK_REQUIRE(row->profile_tokens[0].glyph_lookup_key == "gamepad.1.axis.left_x");
    MK_REQUIRE(row->profile_tokens[0].gamepad_id == mirakana::GamepadId{1});
    MK_REQUIRE(row->profile_tokens[0].scale == 1.0F);
    MK_REQUIRE(row->profile_tokens[0].deadzone == 0.25F);
}

MK_TEST("runtime input rebinding presentation reports invalid profiles") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::up)}});

    const auto model = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    MK_REQUIRE(!model.ready());
    MK_REQUIRE(!model.ready_for_display);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(model.has_profile_overrides);
    MK_REQUIRE(has_rebinding_diagnostic(model.diagnostics, Code::duplicate_override));

    const auto* row = find_rebinding_presentation_row(model.rows, "bind.gameplay.confirm");
    MK_REQUIRE(row != nullptr);
    MK_REQUIRE(model.diagnostics.size() == 1);
    MK_REQUIRE(model.diagnostics[0].path == row->id);
    MK_REQUIRE(!row->ready);
    MK_REQUIRE(row->overridden);
    MK_REQUIRE(row->profile_tokens.size() == 1);
}

MK_TEST("runtime input rebinding profiles serialize and deserialize canonically") {
    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south),
                     key_trigger(mirakana::Key::space)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F),
                    key_axis_source(mirakana::Key::left, mirakana::Key::right)}});

    const auto text = mirakana::runtime::serialize_runtime_input_rebinding_profile(profile);

    MK_REQUIRE(text == "format=GameEngine.RuntimeInputRebindingProfile.v1\n"
                       "profile.id=player_one\n"
                       "bind.gameplay.confirm=key:space,gamepad:1:south\n"
                       "axis.gameplay.move_x=keys:left:right,gamepad:1:left_x:1:0.25\n");

    const auto loaded = mirakana::runtime::deserialize_runtime_input_rebinding_profile(text);
    MK_REQUIRE(loaded.succeeded());
    MK_REQUIRE(mirakana::runtime::serialize_runtime_input_rebinding_profile(loaded.profile) == text);

    mirakana::MemoryFileSystem fs;
    mirakana::runtime::write_runtime_input_rebinding_profile(fs, "profiles/player_one.geinputprofile", loaded.profile);
    const auto reloaded =
        mirakana::runtime::load_runtime_input_rebinding_profile(fs, "profiles/player_one.geinputprofile");
    MK_REQUIRE(reloaded.succeeded());
    MK_REQUIRE(mirakana::runtime::serialize_runtime_input_rebinding_profile(reloaded.profile) == text);
}

MK_TEST("runtime input rebinding profile validation reports malformed and conflicting rows") {
    using Code = mirakana::runtime::RuntimeInputRebindingDiagnosticCode;

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    const auto unsupported = mirakana::runtime::deserialize_runtime_input_rebinding_profile(
        "format=GameEngine.RuntimeInputRebindingProfile.v0\n"
        "profile.id=player_one\n");
    MK_REQUIRE(!unsupported.succeeded());
    MK_REQUIRE(unsupported.diagnostic.find("unsupported") != std::string::npos);

    mirakana::runtime::RuntimeInputRebindingProfile invalid_id;
    invalid_id.profile_id = "player one";
    MK_REQUIRE(has_rebinding_diagnostic(mirakana::runtime::validate_runtime_input_rebinding_profile(base, invalid_id),
                                        Code::invalid_profile_id));

    mirakana::runtime::RuntimeInputRebindingProfile missing_action;
    missing_action.profile_id = "player_one";
    missing_action.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "missing", .triggers = {key_trigger(mirakana::Key::space)}});
    MK_REQUIRE(has_rebinding_diagnostic(
        mirakana::runtime::validate_runtime_input_rebinding_profile(base, missing_action), Code::missing_base_binding));

    mirakana::runtime::RuntimeInputRebindingProfile duplicate_override;
    duplicate_override.profile_id = "player_one";
    duplicate_override.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::space)}});
    duplicate_override.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::up)}});
    MK_REQUIRE(
        has_rebinding_diagnostic(mirakana::runtime::validate_runtime_input_rebinding_profile(base, duplicate_override),
                                 Code::duplicate_override));

    mirakana::runtime::RuntimeInputRebindingProfile duplicate_trigger;
    duplicate_trigger.profile_id = "player_one";
    duplicate_trigger.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {key_trigger(mirakana::Key::space), key_trigger(mirakana::Key::space)}});
    MK_REQUIRE(has_rebinding_diagnostic(
        mirakana::runtime::validate_runtime_input_rebinding_profile(base, duplicate_trigger), Code::duplicate_trigger));

    mirakana::runtime::RuntimeInputRebindingProfile empty_override;
    empty_override.profile_id = "player_one";
    empty_override.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {}});
    empty_override.axis_overrides.push_back(
        mirakana::runtime::RuntimeInputRebindingAxisOverride{.context = "gameplay", .action = "move_x", .sources = {}});
    const auto empty_diagnostics = mirakana::runtime::validate_runtime_input_rebinding_profile(base, empty_override);
    MK_REQUIRE(has_rebinding_diagnostic(empty_diagnostics, Code::empty_override));

    mirakana::runtime::RuntimeInputRebindingProfile invalid_names;
    invalid_names.profile_id = "player_one";
    invalid_names.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "game play", .action = "confirm", .triggers = {key_trigger(mirakana::Key::space)}});
    invalid_names.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move x",
        .sources = {key_axis_source(mirakana::Key::left, mirakana::Key::right)}});
    const auto invalid_name_diagnostics =
        mirakana::runtime::validate_runtime_input_rebinding_profile(base, invalid_names);
    MK_REQUIRE(has_rebinding_diagnostic(invalid_name_diagnostics, Code::invalid_context));
    MK_REQUIRE(has_rebinding_diagnostic(invalid_name_diagnostics, Code::invalid_action));

    mirakana::runtime::RuntimeInputRebindingProfile duplicate_axis_source;
    duplicate_axis_source.profile_id = "player_one";
    duplicate_axis_source.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {key_axis_source(mirakana::Key::left, mirakana::Key::right),
                    key_axis_source(mirakana::Key::left, mirakana::Key::right)}});
    MK_REQUIRE(has_rebinding_diagnostic(
        mirakana::runtime::validate_runtime_input_rebinding_profile(base, duplicate_axis_source),
        Code::duplicate_axis_source));

    mirakana::runtime::RuntimeInputRebindingProfile conflict;
    conflict.profile_id = "player_one";
    conflict.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {key_trigger(mirakana::Key::space)}});
    conflict.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "cancel", .triggers = {key_trigger(mirakana::Key::space)}});
    MK_REQUIRE(has_rebinding_diagnostic(mirakana::runtime::validate_runtime_input_rebinding_profile(base, conflict),
                                        Code::trigger_conflict));
}

MK_TEST("runtime session documents reject malformed save localization and input action documents") {
    const auto bad_save = mirakana::runtime::deserialize_runtime_save_data("format=GameEngine.RuntimeSaveData.v1\n"
                                                                           "schema.version=1\n"
                                                                           "entry.player.name=bad\tvalue\n");
    MK_REQUIRE(!bad_save.succeeded());
    MK_REQUIRE(bad_save.diagnostic.find("control") != std::string::npos);

    const auto bad_localization =
        mirakana::runtime::deserialize_runtime_localization_catalog("format=GameEngine.RuntimeLocalizationCatalog.v1\n"
                                                                    "locale=en-US\n"
                                                                    "text.menu.start=Start\n"
                                                                    "text.menu.start=Again\n");
    MK_REQUIRE(!bad_localization.succeeded());
    MK_REQUIRE(bad_localization.diagnostic.find("duplicate") != std::string::npos);

    const auto bad_input =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=key:space,key:missing\n");
    MK_REQUIRE(!bad_input.succeeded());
    MK_REQUIRE(bad_input.diagnostic.find("unsupported") != std::string::npos);

    const auto old_input_format =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v1\n"
                                                             "bind.jump=space\n");
    MK_REQUIRE(!old_input_format.succeeded());
    MK_REQUIRE(old_input_format.diagnostic.find("unsupported") != std::string::npos);

    const auto old_v2_input_format =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v2\n"
                                                             "bind.jump=key:space\n");
    MK_REQUIRE(!old_v2_input_format.succeeded());
    MK_REQUIRE(old_v2_input_format.diagnostic.find("unsupported") != std::string::npos);

    const auto old_v3_input_format =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v3\n"
                                                             "bind.default.jump=key:space\n");
    MK_REQUIRE(!old_v3_input_format.succeeded());
    MK_REQUIRE(old_v3_input_format.diagnostic.find("unsupported") != std::string::npos);

    const auto bad_trigger_kind =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=touch:1\n");
    MK_REQUIRE(!bad_trigger_kind.succeeded());
    MK_REQUIRE(bad_trigger_kind.diagnostic.find("unsupported") != std::string::npos);

    const auto bad_pointer =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=pointer:0\n");
    MK_REQUIRE(!bad_pointer.succeeded());
    MK_REQUIRE(bad_pointer.diagnostic.find("pointer") != std::string::npos);

    const auto bad_gamepad =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=gamepad:1:unknown\n");
    MK_REQUIRE(!bad_gamepad.succeeded());
    MK_REQUIRE(bad_gamepad.diagnostic.find("gamepad") != std::string::npos);

    const auto duplicate_action =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=key:space\n"
                                                             "bind.default.jump=pointer:1\n");
    MK_REQUIRE(!duplicate_action.succeeded());
    MK_REQUIRE(duplicate_action.diagnostic.find("duplicate") != std::string::npos);

    const auto duplicate_trigger =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.default.jump=key:space,key:space\n");
    MK_REQUIRE(!duplicate_trigger.succeeded());
    MK_REQUIRE(duplicate_trigger.diagnostic.find("duplicate") != std::string::npos);

    const auto bad_axis_kind =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=pointer:delta_x\n");
    MK_REQUIRE(!bad_axis_kind.succeeded());
    MK_REQUIRE(bad_axis_kind.diagnostic.find("unsupported") != std::string::npos);

    const auto bad_axis_key =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=keys:missing:right\n");
    MK_REQUIRE(!bad_axis_key.succeeded());
    MK_REQUIRE(bad_axis_key.diagnostic.find("unsupported") != std::string::npos);

    const auto bad_axis_gamepad_id =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=gamepad:0:left_x:1:0\n");
    MK_REQUIRE(!bad_axis_gamepad_id.succeeded());
    MK_REQUIRE(bad_axis_gamepad_id.diagnostic.find("gamepad") != std::string::npos);

    const auto bad_axis_name =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=gamepad:1:missing:1:0\n");
    MK_REQUIRE(!bad_axis_name.succeeded());
    MK_REQUIRE(bad_axis_name.diagnostic.find("axis") != std::string::npos);

    const auto bad_axis_scale =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=gamepad:1:left_x:0:0\n");
    MK_REQUIRE(!bad_axis_scale.succeeded());
    MK_REQUIRE(bad_axis_scale.diagnostic.find("scale") != std::string::npos);

    const auto bad_axis_deadzone =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=gamepad:1:left_x:1:1\n");
    MK_REQUIRE(!bad_axis_deadzone.succeeded());
    MK_REQUIRE(bad_axis_deadzone.diagnostic.find("deadzone") != std::string::npos);

    const auto duplicate_axis_action =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=keys:left:right\n"
                                                             "axis.default.move_x=gamepad:1:left_x:1:0\n");
    MK_REQUIRE(!duplicate_axis_action.succeeded());
    MK_REQUIRE(duplicate_axis_action.diagnostic.find("duplicate") != std::string::npos);

    const auto duplicate_axis_source =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "axis.default.move_x=keys:left:right,keys:left:right\n");
    MK_REQUIRE(!duplicate_axis_source.succeeded());
    MK_REQUIRE(duplicate_axis_source.diagnostic.find("duplicate") != std::string::npos);

    const auto missing_context_action_separator =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.jump=key:space\n");
    MK_REQUIRE(!missing_context_action_separator.succeeded());
    MK_REQUIRE(missing_context_action_separator.diagnostic.find("context") != std::string::npos);

    const auto invalid_context_name =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.menu layer.confirm=key:space\n");
    MK_REQUIRE(!invalid_context_name.succeeded());
    MK_REQUIRE(invalid_context_name.diagnostic.find("context") != std::string::npos);

    const auto invalid_action_name =
        mirakana::runtime::deserialize_runtime_input_actions("format=GameEngine.RuntimeInputActions.v4\n"
                                                             "bind.menu.con firm=key:space\n");
    MK_REQUIRE(!invalid_action_name.succeeded());
    MK_REQUIRE(invalid_action_name.diagnostic.find("action") != std::string::npos);

    mirakana::MemoryFileSystem fs;
    mirakana::runtime::RuntimeSettings settings;
    bool rejected_path = false;
    try {
        mirakana::runtime::write_runtime_settings(fs, "settings\tbad.settings", settings);
    } catch (const std::invalid_argument&) {
        rejected_path = true;
    }
    MK_REQUIRE(rejected_path);
}

int main() {
    return mirakana::test::run_all();
}
