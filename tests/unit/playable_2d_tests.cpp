// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/scene/playable_2d.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <vector>

namespace {

[[nodiscard]] mirakana::Scene make_valid_2d_scene(mirakana::AssetId sprite, mirakana::AssetId material) {
    mirakana::Scene scene("2D Test Scene");
    const auto camera = scene.create_node("Main Camera");
    const auto player = scene.create_node("Player");

    mirakana::SceneNodeComponents camera_components;
    camera_components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::orthographic,
        .vertical_fov_radians = 1.04719758F,
        .orthographic_height = 12.0F,
        .near_plane = 0.1F,
        .far_plane = 100.0F,
        .primary = true,
    };
    scene.set_components(camera, camera_components);

    mirakana::SceneNodeComponents player_components;
    player_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = sprite,
        .material = material,
        .size = mirakana::Vec2{.x = 1.5F, .y = 2.0F},
        .tint = {0.3F, 0.8F, 1.0F, 1.0F},
        .visible = true,
    };
    scene.set_components(player, player_components);

    return scene;
}

[[nodiscard]] mirakana::ui::RendererSubmission make_hud_submission() {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"hud.root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc score;
    score.id = mirakana::ui::ElementId{"hud.score"};
    score.parent = root.id;
    score.role = mirakana::ui::SemanticRole::label;
    score.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 24.0F};
    score.text.label = "Score 100";
    score.style.background_token = "hud.panel";
    MK_REQUIRE(document.try_add_element(score));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 180.0F});
    return mirakana::ui::build_renderer_submission(document, layout);
}

} // namespace

MK_TEST("playable 2d scene validation accepts an orthographic camera and visible sprite") {
    const auto sprite = mirakana::AssetId::from_name("sprites/player");
    const auto material = mirakana::AssetId::from_name("materials/player_sprite");
    const auto scene = make_valid_2d_scene(sprite, material);

    const auto validation = mirakana::validate_playable_2d_scene(scene);

    MK_REQUIRE(validation.succeeded());
    MK_REQUIRE(validation.visible_sprite_count == 1);
    MK_REQUIRE(validation.visible_mesh_count == 0);
    MK_REQUIRE(validation.camera_count == 1);
    MK_REQUIRE(validation.has_primary_orthographic_camera);
    MK_REQUIRE(validation.diagnostics.empty());
}

MK_TEST("playable 2d scene validation rejects missing camera and visible 3d mesh claims") {
    auto scene = make_valid_2d_scene(mirakana::AssetId::from_name("sprites/player"),
                                     mirakana::AssetId::from_name("materials/player_sprite"));
    const auto mesh = scene.create_node("Unexpected 3D Mesh");

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/cube"),
        .material = mirakana::AssetId::from_name("materials/cube"),
        .visible = true,
    };
    scene.set_components(mesh, mesh_components);

    auto* camera = scene.find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(camera != nullptr);
    camera->components.camera.reset();

    const auto validation = mirakana::validate_playable_2d_scene(scene);

    MK_REQUIRE(!validation.succeeded());
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(
        validation, mirakana::Playable2DSceneDiagnosticCode::missing_primary_orthographic_camera));
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(validation,
                                                    mirakana::Playable2DSceneDiagnosticCode::visible_mesh_renderer));
}

MK_TEST("playable 2d scene validation rejects ambiguous or invalid public diagnostics") {
    auto scene = make_valid_2d_scene(mirakana::AssetId::from_name("sprites/player"),
                                     mirakana::AssetId::from_name("materials/player_sprite"));

    const auto second_camera = scene.create_node("Second Camera");
    mirakana::SceneNodeComponents second_camera_components;
    second_camera_components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::orthographic,
        .vertical_fov_radians = 1.04719758F,
        .orthographic_height = 12.0F,
        .near_plane = 0.1F,
        .far_plane = 100.0F,
        .primary = true,
    };
    scene.set_components(second_camera, second_camera_components);

    const auto invalid_sprite = scene.create_node("Invalid Sprite");
    mirakana::SceneNodeComponents invalid_sprite_components;
    invalid_sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId{},
        .material = mirakana::AssetId::from_name("materials/invalid"),
        .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = true,
    };
    auto* invalid_sprite_node = scene.find_node(invalid_sprite);
    MK_REQUIRE(invalid_sprite_node != nullptr);
    invalid_sprite_node->components = invalid_sprite_components;

    const auto validation = mirakana::validate_playable_2d_scene(scene);

    MK_REQUIRE(!validation.succeeded());
    MK_REQUIRE(validation.primary_camera_count == 2);
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(validation,
                                                    mirakana::Playable2DSceneDiagnosticCode::multiple_primary_cameras));
    MK_REQUIRE(
        mirakana::has_playable_2d_diagnostic(validation, mirakana::Playable2DSceneDiagnosticCode::invalid_components));
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(validation,
                                                    mirakana::Playable2DSceneDiagnosticCode::invalid_sprite_renderer));
}

MK_TEST("playable 2d scene validation reports perspective primary camera and desc toggles") {
    auto scene = make_valid_2d_scene(mirakana::AssetId::from_name("sprites/player"),
                                     mirakana::AssetId::from_name("materials/player_sprite"));

    auto* camera = scene.find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(camera != nullptr);
    MK_REQUIRE(camera->components.camera.has_value());
    camera->components.camera->projection = mirakana::CameraProjectionMode::perspective;

    auto* player = scene.find_node(mirakana::SceneNodeId{2});
    MK_REQUIRE(player != nullptr);
    MK_REQUIRE(player->components.sprite_renderer.has_value());
    player->components.sprite_renderer->visible = false;

    const auto strict_validation = mirakana::validate_playable_2d_scene(scene);
    MK_REQUIRE(!strict_validation.succeeded());
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(
        strict_validation, mirakana::Playable2DSceneDiagnosticCode::missing_primary_orthographic_camera));
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(
        strict_validation, mirakana::Playable2DSceneDiagnosticCode::primary_camera_not_orthographic));
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(strict_validation,
                                                    mirakana::Playable2DSceneDiagnosticCode::missing_visible_sprite));

    const auto loose_validation =
        mirakana::validate_playable_2d_scene(scene, mirakana::Playable2DSceneValidationDesc{
                                                        .require_primary_orthographic_camera = false,
                                                        .require_visible_sprite = false,
                                                        .reject_visible_mesh_renderers = true,
                                                    });
    MK_REQUIRE(!mirakana::has_playable_2d_diagnostic(
        loose_validation, mirakana::Playable2DSceneDiagnosticCode::missing_primary_orthographic_camera));
    MK_REQUIRE(!mirakana::has_playable_2d_diagnostic(loose_validation,
                                                     mirakana::Playable2DSceneDiagnosticCode::missing_visible_sprite));
    MK_REQUIRE(mirakana::has_playable_2d_diagnostic(
        loose_validation, mirakana::Playable2DSceneDiagnosticCode::primary_camera_not_orthographic));
}

MK_TEST("playable 2d source tree proof composes input sprite hud and audio without native handles") {
    const auto sprite = mirakana::AssetId::from_name("sprites/player");
    const auto material = mirakana::AssetId::from_name("materials/player_sprite");
    auto scene = make_valid_2d_scene(sprite, material);

    mirakana::runtime::RuntimeInputActionMap actions;
    actions.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
    actions.bind_key("jump", mirakana::Key::space);

    mirakana::VirtualInput input;
    input.press(mirakana::Key::right);
    input.press(mirakana::Key::space);

    mirakana::AudioMixer mixer;
    const auto cue = mirakana::AssetId::from_name("audio/jump");
    MK_REQUIRE(
        mixer.register_clip(mirakana::AudioClipDesc{cue, 48000, 1, 4, mirakana::AudioSampleFormat::float32, false, 4}));
    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = cue,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .interleaved_float_samples = {0.25F, 0.5F, 0.25F, 0.0F},
        },
    };

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::UiRendererTheme theme;
    MK_REQUIRE(theme.try_add(mirakana::UiThemeColor{"hud.panel", mirakana::Color{0.1F, 0.1F, 0.1F, 1.0F}}));

    auto* player = scene.find_node(mirakana::SceneNodeId{2});
    MK_REQUIRE(player != nullptr);

    std::size_t audio_commands = 0;
    for (int frame = 0; frame < 3; ++frame) {
        const mirakana::runtime::RuntimeInputStateView input_state{
            .keyboard = &input,
            .pointer = nullptr,
            .gamepad = nullptr,
        };
        const auto move_x = actions.axis_value("move_x", input_state);
        player->transform.position.x += move_x;
        if (frame == 0 && actions.action_pressed("jump", input_state)) {
            MK_REQUIRE(mixer.play(mirakana::AudioVoiceDesc{cue, "master", 1.0F, false}).value != 0);
        }

        renderer.begin_frame();
        const auto packet = mirakana::build_scene_render_packet(scene);
        const auto scene_submit = mirakana::submit_scene_render_packet(renderer, packet);
        const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer, make_hud_submission(),
                                                                        mirakana::UiRenderSubmitDesc{.theme = &theme});
        renderer.end_frame();

        const auto audio = mixer.render_interleaved_float(
            mirakana::AudioRenderRequest{
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .frame_count = 4,
                .device_frame = static_cast<std::uint64_t>(frame) * 4U,
                .underrun_warning_threshold_frames = 4,
            },
            samples);
        audio_commands += audio.plan.commands.size();

        MK_REQUIRE(scene_submit.sprites_submitted == 1);
        MK_REQUIRE(scene_submit.has_primary_camera);
        MK_REQUIRE(hud_submit.boxes_submitted == 1);
        input.begin_frame();
    }

    const auto stats = renderer.stats();
    MK_REQUIRE(player->transform.position.x == 3.0F);
    MK_REQUIRE(stats.frames_finished == 3);
    MK_REQUIRE(stats.sprites_submitted == 6);
    MK_REQUIRE(audio_commands == 1);
}

int main() {
    return mirakana::test::run_all();
}
