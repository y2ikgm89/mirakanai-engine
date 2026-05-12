// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

class SampleUiAudioAssetsGame final : public mirakana::GameApp {
  public:
    void on_start(mirakana::EngineContext& context) override {
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "ui/audio/assets started"});

        texture_ = asset_id_from_game_asset_key("sample/hud-atlas");
        clip_ = asset_id_from_game_asset_key("sample/music-loop");
        material_ = asset_id_from_game_asset_key("sample/hud-material");
        mesh_ = asset_id_from_game_asset_key("sample/quad-mesh");
        scene_ = asset_id_from_game_asset_key("sample/runtime-scene");

        assets_.add(mirakana::AssetRecord{
            .id = texture_, .kind = mirakana::AssetKind::texture, .path = "assets/ui/hud-atlas.texture.geasset"});
        assets_.add(mirakana::AssetRecord{
            .id = clip_, .kind = mirakana::AssetKind::audio, .path = "assets/audio/music-loop.audio.geasset"});
        assets_.add(mirakana::AssetRecord{
            .id = material_, .kind = mirakana::AssetKind::material, .path = "assets/materials/hud.material"});
        assets_.add(
            mirakana::AssetRecord{.id = mesh_, .kind = mirakana::AssetKind::mesh, .path = "assets/meshes/quad.mesh"});
        assets_.add(mirakana::AssetRecord{
            .id = scene_, .kind = mirakana::AssetKind::scene, .path = "assets/scenes/runtime.scene"});

        const mirakana::TextureSourceDocument texture_source{
            .width = 64, .height = 32, .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm};
        const mirakana::AudioSourceDocument audio_source{.sample_rate = 48000,
                                                         .channel_count = 2,
                                                         .frame_count = 48000,
                                                         .sample_format = mirakana::AudioSourceSampleFormat::float32};
        source_round_trip_ok_ =
            mirakana::is_valid_texture_source_document(mirakana::deserialize_texture_source_document(
                mirakana::serialize_texture_source_document(texture_source))) &&
            mirakana::is_valid_audio_source_document(
                mirakana::deserialize_audio_source_document(mirakana::serialize_audio_source_document(audio_source))) &&
            mirakana::texture_source_uncompressed_bytes(texture_source) == 8192U &&
            mirakana::audio_source_uncompressed_bytes(audio_source) == 384000U;

        mirakana::MaterialDefinition material;
        material.id = material_;
        material.name = "HUD Material";
        material.shading_model = mirakana::MaterialShadingModel::unlit;
        material.surface_mode = mirakana::MaterialSurfaceMode::transparent;
        material.texture_bindings.push_back(
            mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_});
        material_metadata_ = mirakana::build_material_pipeline_binding_metadata(material);
        runtime_scene_loaded_ = load_runtime_scene_package(material);

        ui_ok_ = ui_.try_add_element(mirakana::ui::ElementDesc{
                     .id = mirakana::ui::ElementId{"hud.root"},
                     .parent = mirakana::ui::ElementId{},
                     .role = mirakana::ui::SemanticRole::root,
                     .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F},
                 }) &&
                 ui_.try_add_element(mirakana::ui::ElementDesc{
                     .id = mirakana::ui::ElementId{"hud.panel"},
                     .parent = mirakana::ui::ElementId{"hud.root"},
                     .role = mirakana::ui::SemanticRole::panel,
                     .bounds = mirakana::ui::Rect{.x = 24.0F, .y = 24.0F, .width = 280.0F, .height = 96.0F},
                 }) &&
                 ui_.try_add_element(mirakana::ui::ElementDesc{
                     .id = mirakana::ui::ElementId{"hud.score"},
                     .parent = mirakana::ui::ElementId{"hud.panel"},
                     .role = mirakana::ui::SemanticRole::label,
                     .bounds = mirakana::ui::Rect{.x = 16.0F, .y = 12.0F, .width = 220.0F, .height = 24.0F},
                     .visible = true,
                     .enabled = true,
                     .text = mirakana::ui::TextContent{.label = "Score 0",
                                                       .localization_key = "hud.score",
                                                       .font_family = "engine-default"},
                 }) &&
                 ui_.try_add_element(mirakana::ui::ElementDesc{
                     .id = mirakana::ui::ElementId{"hud.pause"},
                     .parent = mirakana::ui::ElementId{"hud.panel"},
                     .role = mirakana::ui::SemanticRole::button,
                     .bounds = mirakana::ui::Rect{.x = 16.0F, .y = 52.0F, .width = 96.0F, .height = 28.0F},
                     .visible = true,
                     .enabled = true,
                     .text = mirakana::ui::TextContent{.label = "Pause",
                                                       .localization_key = "hud.pause",
                                                       .font_family = "engine-default"},
                     .image = mirakana::ui::ImageContent{.resource_id = "Pause game"},
                 });

        audio_samples_.push_back(mirakana::AudioClipSampleData{
            .clip = clip_,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 2,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 1536,
            .interleaved_float_samples = std::vector<float>(static_cast<std::size_t>(1536 * 2), 0.25F),
        });
        audio_clip_registered_ =
            mixer_.register_clip(mirakana::AudioClipDesc{.clip = clip_,
                                                         .sample_rate = 48000,
                                                         .channel_count = 2,
                                                         .frame_count = 1536,
                                                         .sample_format = mirakana::AudioSampleFormat::float32,
                                                         .streaming = false,
                                                         .buffered_frame_count = 1536});
        voice_ = mixer_.play(
            mirakana::AudioVoiceDesc{.clip = clip_, .bus = "master", .gain = 0.75F, .looping = true, .start_frame = 0});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        ++frames_;
        ui_text_updates_ok_ = ui_text_updates_ok_ &&
                              ui_.set_text(mirakana::ui::ElementId{"hud.score"},
                                           mirakana::ui::TextContent{.label = "Score " + std::to_string(frames_ * 100),
                                                                     .localization_key = "hud.score",
                                                                     .font_family = "engine-default"});

        const auto rendered = mixer_.render_interleaved_float(
            mirakana::AudioRenderRequest{
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 2,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .frame_count = 512,
                .device_frame = static_cast<std::uint64_t>(frames_ - 1) * 512U,
                .underrun_warning_threshold_frames = 128,
            },
            audio_samples_);
        audio_commands_ += rendered.plan.commands.size();
        audio_underruns_ += rendered.plan.underruns.size();
        audio_rendered_samples_ += rendered.interleaved_float_samples.size();

        return frames_ < 3;
    }

    [[nodiscard]] bool passed() const noexcept {
        return ui_ok_ && source_round_trip_ok_ && assets_.count() == 5 && ui_.size() == 4 &&
               mirakana::is_valid_material_pipeline_binding_metadata(material_metadata_) &&
               material_metadata_.requires_alpha_blending && audio_clip_registered_ &&
               voice_ != mirakana::null_audio_voice && runtime_scene_loaded_ && runtime_meshes_submitted_ == 1 &&
               ui_text_updates_ok_ && audio_commands_ == 3 && audio_underruns_ == 0 && audio_rendered_samples_ == 3072;
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] std::size_t ui_size() const noexcept {
        return ui_.size();
    }

  private:
    [[nodiscard]] bool load_runtime_scene_package(const mirakana::MaterialDefinition& material) {
        mirakana::Scene scene_document("Runtime Sample Scene");
        const auto mesh_node = scene_document.create_node("Preview Mesh");
        mirakana::SceneNodeComponents components;
        components.mesh_renderer =
            mirakana::MeshRendererComponent{.mesh = mesh_, .material = material_, .visible = true};
        scene_document.set_components(mesh_node, components);

        const auto texture_payload = std::string{"format=GameEngine.CookedTexture.v1\n"
                                                 "asset.id="} +
                                     std::to_string(texture_.value) +
                                     "\n"
                                     "asset.kind=texture\n"
                                     "source.path=source/ui/hud-atlas.texture\n"
                                     "texture.width=64\n"
                                     "texture.height=32\n"
                                     "texture.pixel_format=rgba8_unorm\n"
                                     "texture.source_bytes=8192\n";
        const auto mesh_payload = std::string{"format=GameEngine.CookedMesh.v2\n"
                                              "asset.id="} +
                                  std::to_string(mesh_.value) +
                                  "\n"
                                  "asset.kind=mesh\n"
                                  "source.path=source/meshes/quad.mesh\n"
                                  "mesh.vertex_count=3\n"
                                  "mesh.index_count=3\n"
                                  "mesh.has_normals=false\n"
                                  "mesh.has_uvs=false\n"
                                  "mesh.has_tangent_frame=false\n"
                                  "mesh.vertex_data_hex="
                                  "000000000000000000000000000000000000000000000000000000000000000000000000\n"
                                  "mesh.index_data_hex=000000000100000002000000\n";
        const auto material_payload = mirakana::serialize_material_definition(material);
        const auto scene_payload = mirakana::serialize_scene(scene_document);

        mirakana::MemoryFileSystem package_fs;
        const auto index = mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = texture_,
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "assets/ui/hud-atlas.texture.geasset",
                                              .content = texture_payload,
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = mesh_,
                                              .kind = mirakana::AssetKind::mesh,
                                              .path = "assets/meshes/quad.mesh",
                                              .content = mesh_payload,
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = material_,
                                              .kind = mirakana::AssetKind::material,
                                              .path = "assets/materials/hud.material",
                                              .content = material_payload,
                                              .source_revision = 1,
                                              .dependencies = {texture_}},
                mirakana::AssetCookedArtifact{.asset = scene_,
                                              .kind = mirakana::AssetKind::scene,
                                              .path = "assets/scenes/runtime.scene",
                                              .content = scene_payload,
                                              .source_revision = 1,
                                              .dependencies = {mesh_, material_}},
            },
            {});
        package_fs.write_text("packages/sample.geindex", mirakana::serialize_asset_cooked_package_index(index));
        package_fs.write_text("assets/ui/hud-atlas.texture.geasset", texture_payload);
        package_fs.write_text("assets/meshes/quad.mesh", mesh_payload);
        package_fs.write_text("assets/materials/hud.material", material_payload);
        package_fs.write_text("assets/scenes/runtime.scene", scene_payload);

        const auto package =
            mirakana::runtime::load_runtime_asset_package(package_fs, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = "packages/sample.geindex",
                                                                          .content_root = {},
                                                                      });
        if (!package.succeeded()) {
            return false;
        }

        const auto render_data = mirakana::load_runtime_scene_render_data(package.package, scene_);
        if (!render_data.succeeded() || !render_data.scene.has_value() || render_data.material_palette.count() != 1) {
            return false;
        }

        mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 64, .height = 64});
        const auto packet = mirakana::build_scene_render_packet(*render_data.scene);
        renderer.begin_frame();
        const auto submitted = mirakana::submit_scene_render_packet(
            renderer, packet,
            mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                                mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                            .material_palette = &render_data.material_palette});
        renderer.end_frame();

        runtime_meshes_submitted_ = submitted.meshes_submitted;
        return submitted.meshes_submitted == 1 && submitted.material_colors_resolved == 1 &&
               renderer.stats().meshes_submitted == 1;
    }

    mirakana::AssetRegistry assets_;
    mirakana::AudioMixer mixer_;
    mirakana::ui::UiDocument ui_;
    std::vector<mirakana::AudioClipSampleData> audio_samples_;
    mirakana::AssetId texture_;
    mirakana::AssetId clip_;
    mirakana::AssetId material_;
    mirakana::AssetId mesh_;
    mirakana::AssetId scene_;
    mirakana::AudioVoiceId voice_;
    mirakana::MaterialPipelineBindingMetadata material_metadata_;
    std::size_t audio_commands_{0};
    std::size_t audio_underruns_{0};
    std::size_t audio_rendered_samples_{0};
    std::size_t runtime_meshes_submitted_{0};
    int frames_{0};
    bool source_round_trip_ok_{false};
    bool ui_ok_{false};
    bool audio_clip_registered_{false};
    bool runtime_scene_loaded_{false};
    bool ui_text_updates_ok_{true};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    SampleUiAudioAssetsGame game;

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 8, .fixed_delta_seconds = 1.0 / 60.0});
    std::cout << "sample_ui_audio_assets frames=" << result.frames_run << " ui=" << game.ui_size() << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 3 && game.frames() == 3 &&
                   game.passed()
               ? 0
               : 1;
}
