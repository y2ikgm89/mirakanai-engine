// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"
#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/assets/material_graph.hpp"
#include "mirakana/assets/material_graph_shader_export.hpp"
#include "mirakana/assets/sprite_atlas_packing.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"
#include "mirakana/tools/asset_file_scanner.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/tools/asset_import_tool.hpp"
#include "mirakana/tools/asset_package_tool.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"
#include "mirakana/tools/gltf_morph_animation_import.hpp"
#include "mirakana/tools/gltf_node_animation_import.hpp"
#include "mirakana/tools/gltf_skin_animation_import.hpp"
#include "mirakana/tools/gltf_skin_animation_inspect.hpp"
#include "mirakana/tools/material_graph_shader_pipeline.hpp"
#include "mirakana/tools/material_tool.hpp"
#include "mirakana/tools/physics_collision_package_tool.hpp"
#include "mirakana/tools/registered_source_asset_cook_package_tool.hpp"
#include "mirakana/tools/runtime_scene_package_validation_tool.hpp"
#include "mirakana/tools/scene_prefab_authoring_tool.hpp"
#include "mirakana/tools/scene_tool.hpp"
#include "mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"
#include "mirakana/tools/shader_compile_action.hpp"
#include "mirakana/tools/shader_tool_process.hpp"
#include "mirakana/tools/shader_toolchain.hpp"
#include "mirakana/tools/source_asset_registration_tool.hpp"
#include "mirakana/tools/source_image_decode.hpp"
#include "mirakana/tools/tilemap_tool.hpp"
#include "mirakana/tools/ui_atlas_tool.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

class CapturingProcessRunner final : public mirakana::IProcessRunner {
  public:
    [[nodiscard]] mirakana::ProcessResult run(const mirakana::ProcessCommand& command) override {
        captured = command;
        ++calls;
        return next_result;
    }

    mirakana::ProcessResult next_result{
        .launched = true, .exit_code = 0, .diagnostic = {}, .stdout_text = {}, .stderr_text = {}};
    mirakana::ProcessCommand captured;
    int calls{0};
};

class CountingFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view /*path*/) const override {
        ++exists_calls;
        return false;
    }

    [[nodiscard]] bool is_directory(std::string_view /*path*/) const override {
        ++is_directory_calls;
        return false;
    }

    [[nodiscard]] std::string read_text(std::string_view /*path*/) const override {
        ++read_calls;
        return {};
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view /*root*/) const override {
        ++list_calls;
        return {};
    }

    void write_text(std::string_view /*path*/, std::string_view /*text*/) override {
        ++write_calls;
    }

    void remove(std::string_view /*path*/) override {
        ++remove_calls;
    }

    void remove_empty_directory(std::string_view /*path*/) override {
        ++remove_empty_directory_calls;
    }

    mutable int exists_calls{0};
    mutable int is_directory_calls{0};
    mutable int read_calls{0};
    mutable int list_calls{0};
    int write_calls{0};
    int remove_calls{0};
    int remove_empty_directory_calls{0};
};

class CountingShaderToolRunner final : public mirakana::IShaderToolRunner {
  public:
    [[nodiscard]] mirakana::ShaderToolRunResult run(const mirakana::ShaderCompileCommand& command) override {
        ++calls;
        last_command = command;
        if (fail) {
            return mirakana::ShaderToolRunResult{.exit_code = 17,
                                                 .diagnostic = "compile failed",
                                                 .stdout_text = {},
                                                 .stderr_text = "compiler error",
                                                 .artifact = command.artifact};
        }
        return mirakana::ShaderToolRunResult{.exit_code = 0,
                                             .diagnostic = "compiled",
                                             .stdout_text = "compiler output",
                                             .stderr_text = {},
                                             .artifact = command.artifact};
    }

    int calls{0};
    bool fail{false};
    mirakana::ShaderCompileCommand last_command;
};

class CountingShaderArtifactValidatorRunner final : public mirakana::IShaderArtifactValidatorRunner {
  public:
    [[nodiscard]] mirakana::ShaderArtifactValidationResult
    run(const mirakana::ShaderArtifactValidationCommand& command) override {
        ++calls;
        last_command = command;
        if (fail) {
            return mirakana::ShaderArtifactValidationResult{.exit_code = 2,
                                                            .diagnostic = "validation failed",
                                                            .stdout_text = {},
                                                            .stderr_text = "invalid spirv",
                                                            .artifact = command.artifact};
        }
        return mirakana::ShaderArtifactValidationResult{.exit_code = 0,
                                                        .diagnostic = "validated",
                                                        .stdout_text = "ok",
                                                        .stderr_text = {},
                                                        .artifact = command.artifact};
    }

    int calls{0};
    bool fail{false};
    mirakana::ShaderArtifactValidationCommand last_command;
};

class ThrowingExistsFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override {
        throw std::runtime_error("exists denied: " + std::string{path});
    }

    [[nodiscard]] bool is_directory(std::string_view /*path*/) const override {
        return false;
    }

    [[nodiscard]] std::string read_text(std::string_view /*path*/) const override {
        return {};
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view /*root*/) const override {
        return {};
    }

    void write_text(std::string_view /*path*/, std::string_view /*text*/) override {}
    void remove(std::string_view /*path*/) override {}
    void remove_empty_directory(std::string_view /*path*/) override {}
};

class ThrowingWriteFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override {
        return files.find(std::string{path}) != files.end();
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        const std::string prefix = std::string{path} + "/";
        return std::ranges::any_of(files, [&prefix](const auto& file) { return file.first.rfind(prefix, 0) == 0; });
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        const auto it = files.find(std::string{path});
        if (it == files.end()) {
            throw std::runtime_error("missing file: " + std::string{path});
        }
        return it->second;
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view /*root*/) const override {
        return {};
    }

    void write_text(std::string_view path, std::string_view text) override {
        ++write_calls;
        if (path == failing_write_path) {
            if (failing_write_attempts_remaining == 0) {
                files[std::string{path}] = std::string{text};
                return;
            }
            if (failing_write_attempts_remaining > 0) {
                --failing_write_attempts_remaining;
            }
            throw std::runtime_error("write denied: " + std::string{path});
        }
        files[std::string{path}] = std::string{text};
    }

    void remove(std::string_view path) override {
        ++remove_calls;
        files.erase(std::string{path});
    }

    void remove_empty_directory(std::string_view path) override {
        ++remove_empty_directory_calls;
        removed_directories.emplace_back(path);
    }

    std::unordered_map<std::string, std::string> files;
    std::string failing_write_path;
    int failing_write_attempts_remaining{-1};
    std::vector<std::string> removed_directories;
    int write_calls{0};
    int remove_calls{0};
    int remove_empty_directory_calls{0};
};

[[nodiscard]] std::string byte_string(std::initializer_list<unsigned char> bytes) {
    std::string result;
    result.reserve(bytes.size());
    for (const auto byte : bytes) {
        result.push_back(static_cast<char>(byte));
    }
    return result;
}

void append_le_u16(std::string& output, std::uint16_t value) {
    output.push_back(static_cast<char>(value & 0xFFU));
    output.push_back(static_cast<char>((value >> 8U) & 0xFFU));
}

void append_le_u32(std::string& output, std::uint32_t value) {
    output.push_back(static_cast<char>(value & 0xFFU));
    output.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<char>((value >> 24U) & 0xFFU));
}

void append_le_f32(std::string& output, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(output, bits);
}

[[nodiscard]] mirakana::CookedUiAtlasAuthoringDesc make_cooked_ui_atlas_authoring_desc() {
    mirakana::CookedUiAtlasAuthoringDesc desc;
    desc.atlas_asset = mirakana::AssetId{100};
    desc.output_path = "runtime/assets/ui/hud.uiatlas";
    desc.source_revision = 7;
    desc.pages.push_back(mirakana::CookedUiAtlasPageDesc{
        mirakana::AssetId{20},
        "runtime/assets/ui/page-b.texture.geasset",
    });
    desc.pages.push_back(mirakana::CookedUiAtlasPageDesc{
        mirakana::AssetId{10},
        "runtime/assets/ui/page-a.texture.geasset",
    });
    desc.images.push_back(mirakana::CookedUiAtlasImageDesc{
        .resource_id = "hud/icon",
        .asset_uri = "ui://hud/icon",
        .page = mirakana::AssetId{20},
        .u0 = 0.25F,
        .v0 = 0.5F,
        .u1 = 0.75F,
        .v1 = 1.0F,
        .color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F},
    });
    desc.images.push_back(mirakana::CookedUiAtlasImageDesc{
        .resource_id = "hud/background",
        .asset_uri = "ui://hud/background",
        .page = mirakana::AssetId{10},
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 0.5F,
        .v1 = 0.5F,
        .color = std::array<float, 4>{0.5F, 0.25F, 1.0F, 1.0F},
    });
    return desc;
}

[[nodiscard]] std::string expected_cooked_ui_atlas_metadata() {
    return "format=GameEngine.UiAtlas.v1\n"
           "asset.id=100\n"
           "asset.kind=ui_atlas\n"
           "source.decoding=unsupported\n"
           "atlas.packing=unsupported\n"
           "page.count=2\n"
           "page.0.asset=10\n"
           "page.0.asset_uri=runtime/assets/ui/page-a.texture.geasset\n"
           "page.1.asset=20\n"
           "page.1.asset_uri=runtime/assets/ui/page-b.texture.geasset\n"
           "image.count=2\n"
           "image.0.resource_id=hud/background\n"
           "image.0.asset_uri=ui://hud/background\n"
           "image.0.page=10\n"
           "image.0.u0=0\n"
           "image.0.v0=0\n"
           "image.0.u1=0.5\n"
           "image.0.v1=0.5\n"
           "image.0.color=0.5,0.25,1,1\n"
           "image.1.resource_id=hud/icon\n"
           "image.1.asset_uri=ui://hud/icon\n"
           "image.1.page=20\n"
           "image.1.u0=0.25\n"
           "image.1.v0=0.5\n"
           "image.1.u1=0.75\n"
           "image.1.v1=1\n"
           "image.1.color=1,1,1,1\n"
           "glyph.count=0\n";
}

[[nodiscard]] std::string ui_atlas_base_package_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/ui/page-a.texture.geasset",
                                          .content = "page-a",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/ui/page-b.texture.geasset",
                                          .content = "page-b",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::ui::ImageDecodeResult make_decoded_ui_pixel(std::byte red, std::byte green, std::byte blue,
                                                                    std::byte alpha) {
    return mirakana::ui::ImageDecodeResult{
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::ui::ImageDecodePixelFormat::rgba8_unorm,
        .pixels = {red, green, blue, alpha},
    };
}

[[nodiscard]] mirakana::PackedUiAtlasAuthoringDesc make_packed_ui_atlas_authoring_desc() {
    mirakana::PackedUiAtlasAuthoringDesc desc;
    desc.atlas_asset = mirakana::AssetId{500};
    desc.atlas_page_asset = mirakana::AssetId{501};
    desc.atlas_metadata_output_path = "runtime/assets/ui/generated_hud.uiatlas";
    desc.atlas_page_output_path = "runtime/assets/ui/generated_hud_page.texture";
    desc.source_revision = 9;
    desc.images.push_back(mirakana::PackedUiAtlasImageDesc{
        .resource_id = "hud/icon",
        .asset_uri = "ui://hud/icon",
        .decoded_image = make_decoded_ui_pixel(std::byte{0xFF}, std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}),
        .color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F},
    });
    desc.images.push_back(mirakana::PackedUiAtlasImageDesc{
        .resource_id = "hud/background",
        .asset_uri = "ui://hud/background",
        .decoded_image = make_decoded_ui_pixel(std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}),
        .color = std::array<float, 4>{0.5F, 0.5F, 1.0F, 1.0F},
    });
    return desc;
}

[[nodiscard]] mirakana::PackedUiGlyphAtlasAuthoringDesc make_packed_ui_glyph_atlas_authoring_desc() {
    mirakana::PackedUiGlyphAtlasAuthoringDesc desc;
    desc.atlas_asset = mirakana::AssetId{600};
    desc.atlas_page_asset = mirakana::AssetId{601};
    desc.atlas_metadata_output_path = "runtime/assets/ui/generated_body_glyphs.uiatlas";
    desc.atlas_page_output_path = "runtime/assets/ui/generated_body_glyphs.texture";
    desc.source_revision = 10;
    desc.glyphs.push_back(mirakana::PackedUiGlyphAtlasGlyphDesc{
        .font_family = "ui/body",
        .glyph = 66,
        .rasterized_glyph = make_decoded_ui_pixel(std::byte{0x00}, std::byte{0x00}, std::byte{0xFF}, std::byte{0xFF}),
        .color = std::array<float, 4>{0.9F, 0.9F, 0.9F, 1.0F},
    });
    desc.glyphs.push_back(mirakana::PackedUiGlyphAtlasGlyphDesc{
        .font_family = "ui/body",
        .glyph = 65,
        .rasterized_glyph = make_decoded_ui_pixel(std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}),
        .color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F},
    });
    return desc;
}

[[nodiscard]] mirakana::CookedTilemapAuthoringDesc make_cooked_tilemap_authoring_desc() {
    mirakana::CookedTilemapAuthoringDesc desc;
    desc.tilemap_asset = mirakana::AssetId{300};
    desc.output_path = "runtime/assets/2d/level.tilemap";
    desc.source_revision = 11;
    desc.atlas_page = mirakana::AssetId{10};
    desc.atlas_page_uri = "runtime/assets/2d/player.texture.geasset";
    desc.tile_width = 16;
    desc.tile_height = 16;
    desc.tiles.push_back(mirakana::CookedTilemapTileDesc{
        .id = "water",
        .page = mirakana::AssetId{10},
        .u0 = 0.5F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 0.5F,
        .color = std::array<float, 4>{0.25F, 0.5F, 1.0F, 1.0F},
    });
    desc.tiles.push_back(mirakana::CookedTilemapTileDesc{
        .id = "grass",
        .page = mirakana::AssetId{10},
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 0.5F,
        .v1 = 0.5F,
        .color = std::array<float, 4>{0.5F, 1.0F, 0.25F, 1.0F},
    });
    desc.layers.push_back(mirakana::CookedTilemapLayerDesc{
        .name = "ground",
        .width = 2,
        .height = 2,
        .visible = true,
        .cells = {"grass", "water", "", "grass"},
    });
    return desc;
}

[[nodiscard]] std::string expected_cooked_tilemap_metadata() {
    return "format=GameEngine.Tilemap.v1\n"
           "asset.id=300\n"
           "asset.kind=tilemap\n"
           "source.decoding=unsupported\n"
           "atlas.packing=unsupported\n"
           "native_gpu_sprite_batching=unsupported\n"
           "atlas.page.asset=10\n"
           "atlas.page.asset_uri=runtime/assets/2d/player.texture.geasset\n"
           "tile.size=16,16\n"
           "tile.count=2\n"
           "tile.0.id=grass\n"
           "tile.0.page=10\n"
           "tile.0.u0=0\n"
           "tile.0.v0=0\n"
           "tile.0.u1=0.5\n"
           "tile.0.v1=0.5\n"
           "tile.0.color=0.5,1,0.25,1\n"
           "tile.1.id=water\n"
           "tile.1.page=10\n"
           "tile.1.u0=0.5\n"
           "tile.1.v0=0\n"
           "tile.1.u1=1\n"
           "tile.1.v1=0.5\n"
           "tile.1.color=0.25,0.5,1,1\n"
           "layer.count=1\n"
           "layer.0.name=ground\n"
           "layer.0.width=2\n"
           "layer.0.height=2\n"
           "layer.0.visible=true\n"
           "layer.0.cells=grass,water,,grass\n";
}

[[nodiscard]] std::string tilemap_base_package_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/2d/player.texture.geasset",
                                          .content = "page",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::PhysicsCollisionPackageUpdateDesc make_physics_collision_package_update_desc() {
    mirakana::PhysicsCollisionPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(mirakana::AssetCookedPackageIndex{});
    update.collision.collision_asset = mirakana::AssetId{700};
    update.collision.output_path = "runtime/assets/physics/main.collision3d";
    update.collision.source_revision = 19;
    update.collision.world_config.gravity = mirakana::Vec3{.x = 0.0F, .y = -9.80665F, .z = 0.0F};

    mirakana::PhysicsBody3DDesc floor;
    floor.dynamic = false;
    floor.shape = mirakana::PhysicsShape3DKind::aabb;
    floor.position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    floor.half_extents = mirakana::Vec3{.x = 8.0F, .y = 0.5F, .z = 8.0F};
    floor.collision_layer = 1U;
    floor.collision_mask = 0xFFFF'FFFFU;
    update.collision.bodies.push_back(mirakana::PhysicsCollisionPackageBodyDesc{
        .name = "floor", .body = floor, .material = "stone", .compound = "level_static"});

    mirakana::PhysicsBody3DDesc collision_probe;
    collision_probe.dynamic = false;
    collision_probe.shape = mirakana::PhysicsShape3DKind::aabb;
    collision_probe.position = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F};
    collision_probe.half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    collision_probe.radius = 1.2345678F;
    collision_probe.collision_layer = 4U;
    collision_probe.collision_mask = 1U;
    update.collision.bodies.push_back(mirakana::PhysicsCollisionPackageBodyDesc{
        .name = "collision_probe", .body = collision_probe, .material = "metal"});

    mirakana::PhysicsBody3DDesc pickup_trigger;
    pickup_trigger.dynamic = false;
    pickup_trigger.shape = mirakana::PhysicsShape3DKind::sphere;
    pickup_trigger.position = mirakana::Vec3{.x = 1.0F, .y = 0.25F, .z = 0.0F};
    pickup_trigger.radius = 0.75F;
    pickup_trigger.collision_layer = 2U;
    pickup_trigger.collision_mask = 1U;
    pickup_trigger.trigger = true;
    update.collision.bodies.push_back(mirakana::PhysicsCollisionPackageBodyDesc{
        .name = "pickup_trigger", .body = pickup_trigger, .material = "gold", .compound = "interaction_triggers"});

    return update;
}

[[nodiscard]] std::string expected_physics_collision_package_payload() {
    return "format=GameEngine.PhysicsCollisionScene3D.v1\n"
           "asset.id=700\n"
           "asset.kind=physics_collision_scene\n"
           "backend.native=unsupported\n"
           "world.gravity=0,-9.80665,0\n"
           "body.count=3\n"
           "body.0.name=floor\n"
           "body.0.shape=aabb\n"
           "body.0.position=0,0,0\n"
           "body.0.velocity=0,0,0\n"
           "body.0.dynamic=false\n"
           "body.0.mass=1\n"
           "body.0.linear_damping=0\n"
           "body.0.half_extents=8,0.5,8\n"
           "body.0.radius=0.5\n"
           "body.0.half_height=0.5\n"
           "body.0.layer=1\n"
           "body.0.mask=4294967295\n"
           "body.0.trigger=false\n"
           "body.0.material=stone\n"
           "body.0.compound=level_static\n"
           "body.1.name=collision_probe\n"
           "body.1.shape=aabb\n"
           "body.1.position=0,0.25,0\n"
           "body.1.velocity=0,0,0\n"
           "body.1.dynamic=false\n"
           "body.1.mass=1\n"
           "body.1.linear_damping=0\n"
           "body.1.half_extents=0.5,0.5,0.5\n"
           "body.1.radius=1.2345678\n"
           "body.1.half_height=0.5\n"
           "body.1.layer=4\n"
           "body.1.mask=1\n"
           "body.1.trigger=false\n"
           "body.1.material=metal\n"
           "body.2.name=pickup_trigger\n"
           "body.2.shape=sphere\n"
           "body.2.position=1,0.25,0\n"
           "body.2.velocity=0,0,0\n"
           "body.2.dynamic=false\n"
           "body.2.mass=1\n"
           "body.2.linear_damping=0\n"
           "body.2.half_extents=0.5,0.5,0.5\n"
           "body.2.radius=0.75\n"
           "body.2.half_height=0.5\n"
           "body.2.layer=2\n"
           "body.2.mask=1\n"
           "body.2.trigger=true\n"
           "body.2.material=gold\n"
           "body.2.compound=interaction_triggers\n";
}

[[nodiscard]] mirakana::MaterialInstanceDefinition make_material_instance_definition() {
    mirakana::MaterialFactors factors;
    factors.base_color = {0.25F, 0.5F, 1.0F, 1.0F};
    factors.emissive = {0.0F, 0.0F, 0.25F};
    factors.metallic = 0.0F;
    factors.roughness = 0.35F;
    return mirakana::MaterialInstanceDefinition{
        .id = mirakana::AssetId{300},
        .name = "Blue Instance",
        .parent = mirakana::AssetId{100},
        .factor_overrides = factors,
        .texture_overrides =
            {
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::emissive,
                                                 .texture = mirakana::AssetId{20}},
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                 .texture = mirakana::AssetId{10}},
            },
    };
}

[[nodiscard]] mirakana::MaterialDefinition make_base_material_definition() {
    return mirakana::MaterialDefinition{
        .id = mirakana::AssetId{100},
        .name = "Base Material",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {},
        .double_sided = false,
    };
}

[[nodiscard]] std::string material_instance_base_package_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{100},
                                          .kind = mirakana::AssetKind::material,
                                          .path = "runtime/assets/materials/base.material",
                                          .content =
                                              mirakana::serialize_material_definition(make_base_material_definition()),
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/textures/albedo.texture",
                                          .content = "albedo",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/textures/emissive.texture",
                                          .content = "emissive",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::MaterialGraphDesc make_material_graph_package_graph() {
    mirakana::MaterialGraphDesc graph{};
    graph.material_id = mirakana::AssetId{400};
    graph.material_name = "Graph Material";
    graph.shading_model = mirakana::MaterialShadingModel::lit;
    graph.surface_mode = mirakana::MaterialSurfaceMode::masked;
    graph.double_sided = true;
    graph.output_node_id = "out";

    mirakana::MaterialGraphNode output;
    output.id = "out";
    output.kind = mirakana::MaterialGraphNodeKind::graph_output;

    mirakana::MaterialGraphNode base_color;
    base_color.id = "base_color";
    base_color.kind = mirakana::MaterialGraphNodeKind::constant_vec4;
    base_color.vec4 = {0.75F, 0.25F, 0.125F, 0.5F};

    mirakana::MaterialGraphNode emissive;
    emissive.id = "emissive";
    emissive.kind = mirakana::MaterialGraphNodeKind::constant_vec3;
    emissive.vec3 = {0.0F, 0.25F, 0.5F};

    mirakana::MaterialGraphNode albedo_texture;
    albedo_texture.id = "albedo_texture";
    albedo_texture.kind = mirakana::MaterialGraphNodeKind::texture;
    albedo_texture.texture_slot = mirakana::MaterialTextureSlot::base_color;
    albedo_texture.texture_id = mirakana::AssetId{10};

    mirakana::MaterialGraphNode emissive_texture;
    emissive_texture.id = "emissive_texture";
    emissive_texture.kind = mirakana::MaterialGraphNodeKind::texture;
    emissive_texture.texture_slot = mirakana::MaterialTextureSlot::emissive;
    emissive_texture.texture_id = mirakana::AssetId{20};

    graph.nodes = {output, base_color, emissive, albedo_texture, emissive_texture};
    graph.edges = {
        mirakana::MaterialGraphEdge{
            .from_node = "base_color", .from_socket = "out", .to_node = "out", .to_socket = "factor.base_color"},
        mirakana::MaterialGraphEdge{
            .from_node = "emissive", .from_socket = "out", .to_node = "out", .to_socket = "factor.emissive"},
        mirakana::MaterialGraphEdge{
            .from_node = "albedo_texture", .from_socket = "out", .to_node = "out", .to_socket = "texture.base_color"},
        mirakana::MaterialGraphEdge{
            .from_node = "emissive_texture", .from_socket = "out", .to_node = "out", .to_socket = "texture.emissive"},
    };
    return graph;
}

[[nodiscard]] std::string material_graph_package_base_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/textures/albedo.texture",
                                          .content = "albedo",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/textures/emissive.texture",
                                          .content = "emissive",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::MaterialGraphPackageUpdateDesc make_material_graph_package_update_desc() {
    mirakana::MaterialGraphPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = material_graph_package_base_index_content();
    update.material_graph_content = mirakana::serialize_material_graph(make_material_graph_package_graph());
    update.output_path = "runtime/assets/materials/graph.material";
    update.source_revision = 17;
    return update;
}

[[nodiscard]] std::string empty_package_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::AssetCookedPackageIndex{});
}

[[nodiscard]] std::string registered_cook_texture_source_content() {
    return "format=GameEngine.TextureSource.v1\n"
           "texture.width=4\n"
           "texture.height=2\n"
           "texture.pixel_format=rgba8_unorm\n";
}

[[nodiscard]] std::string registered_cook_mesh_source_content() {
    return "format=GameEngine.MeshSource.v2\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=false\n"
           "mesh.has_uvs=false\n"
           "mesh.has_tangent_frame=false\n";
}

[[nodiscard]] std::string registered_cook_material_source_content() {
    const auto material = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/hero"});
    const auto texture = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/hero"});
    return mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "Hero Material",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
}

[[nodiscard]] mirakana::SourceAssetRegistryDocumentV1 make_registered_cook_source_registry() {
    mirakana::SourceAssetRegistryDocumentV1 registry;
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/materials/hero"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/hero.material",
        .source_format = "GameEngine.Material.v1",
        .imported_path = "runtime/assets/materials/hero.material",
        .dependencies = {mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::material_texture,
                                                              .key = mirakana::AssetKeyV2{"assets/textures/hero"}}},
    });
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/meshes/cube"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/cube.mesh_source",
        .source_format = "GameEngine.MeshSource.v2",
        .imported_path = "runtime/assets/meshes/cube.mesh",
        .dependencies = {},
    });
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/textures/hero"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/hero.texture_source",
        .source_format = "GameEngine.TextureSource.v1",
        .imported_path = "runtime/assets/textures/hero.texture",
        .dependencies = {},
    });
    return registry;
}

[[nodiscard]] mirakana::RegisteredSourceAssetCookPackageRequest make_registered_cook_request() {
    mirakana::RegisteredSourceAssetCookPackageRequest request;
    request.kind = mirakana::RegisteredSourceAssetCookPackageCommandKind::cook_registered_source_assets;
    request.source_registry_path = "source/assets/game.geassets";
    request.source_registry_content =
        mirakana::serialize_source_asset_registry_document(make_registered_cook_source_registry());
    request.package_index_path = "runtime/game.geindex";
    request.package_index_content = empty_package_index_content();
    request.selected_asset_keys = {mirakana::AssetKeyV2{"assets/textures/hero"},
                                   mirakana::AssetKeyV2{"assets/materials/hero"}};
    request.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = registered_cook_texture_source_content()},
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/materials/hero.material",
                                                             .content = registered_cook_material_source_content()},
    };
    request.source_revision = 31;
    return request;
}

[[nodiscard]] mirakana::RegisteredSourceAssetCookPackageRequest make_registered_scene_workflow_cook_request() {
    auto request = make_registered_cook_request();
    request.selected_asset_keys = {
        mirakana::AssetKeyV2{"assets/meshes/cube"},
        mirakana::AssetKeyV2{"assets/textures/hero"},
        mirakana::AssetKeyV2{"assets/materials/hero"},
    };
    request.source_files.insert(request.source_files.begin(), mirakana::RegisteredSourceAssetCookPackageSourceFile{
                                                                  .path = "source/meshes/cube.mesh_source",
                                                                  .content = registered_cook_mesh_source_content()});
    return request;
}

[[nodiscard]] std::string registered_cook_conflicting_package_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = mirakana::AssetId{999},
                .kind = mirakana::AssetKind::texture,
                .path = "runtime/assets/textures/hero.texture",
                .content = "other texture",
                .source_revision = 1,
                .dependencies = {},
            },
        },
        {}));
}

[[nodiscard]] mirakana::Scene make_scene_package_scene() {
    mirakana::Scene scene{"Packaged Scene"};
    const auto root = scene.create_node("Root");
    auto* root_node = scene.find_node(root);
    root_node->transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    root_node->transform.scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    root_node->transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F};

    mirakana::SceneNodeComponents components;
    components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::perspective,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 10.0F,
        .near_plane = 0.1F,
        .far_plane = 500.0F,
        .primary = true,
    };
    components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId{101}, .material = mirakana::AssetId{201}, .visible = true};
    components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId{301},
        .material = mirakana::AssetId{201},
        .size = mirakana::Vec2{.x = 2.0F, .y = 3.0F},
        .tint = std::array<float, 4>{1.0F, 0.5F, 0.25F, 1.0F},
        .visible = true,
    };
    scene.set_components(root, components);
    return scene;
}

[[nodiscard]] std::string scene_package_base_index_content() {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{101},
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "runtime/assets/scene/ship.mesh",
                                          .content = "mesh",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{201},
                                          .kind = mirakana::AssetKind::material,
                                          .path = "runtime/assets/scene/ship.material",
                                          .content = "material",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{301},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/scene/ship-sprite.texture.geasset",
                                          .content = "sprite",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::ScenePackageUpdateDesc make_scene_package_update_desc() {
    mirakana::ScenePackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = scene_package_base_index_content();
    update.output_path = "runtime/assets/scene/packaged.scene";
    update.source_revision = 13;
    update.scene_asset = mirakana::AssetId{901};
    update.scene = make_scene_package_scene();
    update.mesh_dependencies = {mirakana::AssetId{101}};
    update.material_dependencies = {mirakana::AssetId{201}};
    update.sprite_dependencies = {mirakana::AssetId{301}};
    return update;
}

[[nodiscard]] mirakana::SceneDocumentV2 make_scene_prefab_authoring_scene_v2() {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Authoring Level";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/root"}, .name = "Root"});
    return scene;
}

[[nodiscard]] mirakana::SceneDocumentV2 make_runtime_migration_scene_v2() {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Migrated Level";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/root"}, .name = "Root"});
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/child"}, .name = "Child"});
    scene.nodes[1].parent = mirakana::AuthoringId{"node/root"};
    scene.nodes[1].transform.position = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F};
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/root/camera"},
        .node = mirakana::AuthoringId{"node/root"},
        .type = mirakana::SceneComponentTypeId{"camera"},
        .properties =
            {
                {.name = "projection", .value = "orthographic"},
                {.name = "primary", .value = "true"},
                {.name = "vertical_fov_radians", .value = "1.1"},
                {.name = "orthographic_height", .value = "8"},
                {.name = "near_plane", .value = "0.05"},
                {.name = "far_plane", .value = "500"},
            },
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/root/mesh"},
        .node = mirakana::AuthoringId{"node/root"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties =
            {
                {.name = "mesh", .value = "assets/meshes/cube"},
                {.name = "material", .value = "assets/materials/base"},
                {.name = "visible", .value = "true"},
            },
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/child/light"},
        .node = mirakana::AuthoringId{"node/child"},
        .type = mirakana::SceneComponentTypeId{"light"},
        .properties =
            {
                {.name = "type", .value = "point"},
                {.name = "color", .value = "0.5,0.25,1"},
                {.name = "intensity", .value = "2"},
                {.name = "range", .value = "30"},
                {.name = "casts_shadows", .value = "true"},
            },
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/child/sprite"},
        .node = mirakana::AuthoringId{"node/child"},
        .type = mirakana::SceneComponentTypeId{"sprite_renderer"},
        .properties =
            {
                {.name = "sprite", .value = "assets/textures/hero"},
                {.name = "material", .value = "assets/materials/base"},
                {.name = "size", .value = "2,3"},
                {.name = "tint", .value = "1,0.5,0.25,1"},
                {.name = "visible", .value = "false"},
            },
    });
    return scene;
}

[[nodiscard]] mirakana::SourceAssetRegistryDocumentV1 make_runtime_migration_source_registry() {
    mirakana::SourceAssetRegistryDocumentV1 registry;
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/materials/base"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/base.material",
        .source_format = "GameEngine.Material.v1",
        .imported_path = "runtime/assets/materials/base.material",
        .dependencies = {},
    });
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/meshes/cube"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/cube.gemesh",
        .source_format = "GameEngine.MeshSource.v2",
        .imported_path = "runtime/assets/meshes/cube.mesh",
        .dependencies = {},
    });
    registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/textures/hero"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/hero.getexture",
        .source_format = "GameEngine.TextureSource.v1",
        .imported_path = "runtime/assets/textures/hero.texture",
        .dependencies = {},
    });
    return registry;
}

[[nodiscard]] std::string runtime_migration_package_index_content() {
    const auto mesh = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/meshes/cube"});
    const auto material = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/base"});
    const auto sprite = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/hero"});
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mesh,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "runtime/assets/meshes/cube.mesh",
                                          .content = "mesh",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = material,
                                          .kind = mirakana::AssetKind::material,
                                          .path = "runtime/assets/materials/base.material",
                                          .content = "material",
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = sprite,
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/textures/hero.texture",
                                          .content = "sprite",
                                          .source_revision = 1,
                                          .dependencies = {}},
        },
        {}));
}

[[nodiscard]] mirakana::SceneV2RuntimePackageMigrationRequest make_runtime_migration_request() {
    mirakana::SceneV2RuntimePackageMigrationRequest request;
    request.kind = mirakana::SceneV2RuntimePackageMigrationCommandKind::migrate_scene_v2_runtime_package;
    request.scene_v2_path = "source/scenes/level.scene";
    request.scene_v2_content = mirakana::serialize_scene_document_v2(make_runtime_migration_scene_v2());
    request.source_registry_path = "source/assets/game.geassets";
    request.source_registry_content =
        mirakana::serialize_source_asset_registry_document(make_runtime_migration_source_registry());
    request.package_index_path = "runtime/game.geindex";
    request.package_index_content = runtime_migration_package_index_content();
    request.output_scene_path = "runtime/assets/scenes/level.scene";
    request.scene_asset_key = mirakana::AssetKeyV2{"assets/scenes/level"};
    request.source_revision = 21;
    return request;
}

[[nodiscard]] mirakana::SceneDocumentV2 make_registered_scene_workflow_scene_v2() {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Registered Runtime Level";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/root"}, .name = "Root"});
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/prop"}, .name = "Prop"});
    scene.nodes[1].parent = mirakana::AuthoringId{"node/root"};
    scene.nodes[1].transform.position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 1.0F};
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/root/camera"},
        .node = mirakana::AuthoringId{"node/root"},
        .type = mirakana::SceneComponentTypeId{"camera"},
        .properties =
            {
                {.name = "projection", .value = "orthographic"},
                {.name = "primary", .value = "true"},
                {.name = "orthographic_height", .value = "6"},
                {.name = "near_plane", .value = "0.05"},
                {.name = "far_plane", .value = "250"},
            },
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/prop/mesh"},
        .node = mirakana::AuthoringId{"node/prop"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties =
            {
                {.name = "mesh", .value = "assets/meshes/cube"},
                {.name = "material", .value = "assets/materials/hero"},
                {.name = "visible", .value = "true"},
            },
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/prop/sprite"},
        .node = mirakana::AuthoringId{"node/prop"},
        .type = mirakana::SceneComponentTypeId{"sprite_renderer"},
        .properties =
            {
                {.name = "sprite", .value = "assets/textures/hero"},
                {.name = "material", .value = "assets/materials/hero"},
                {.name = "size", .value = "1,2"},
                {.name = "visible", .value = "true"},
            },
    });
    return scene;
}

[[nodiscard]] mirakana::SceneV2RuntimePackageMigrationRequest
make_registered_scene_workflow_migration_request(std::string package_index_content = empty_package_index_content()) {
    mirakana::SceneV2RuntimePackageMigrationRequest request;
    request.kind = mirakana::SceneV2RuntimePackageMigrationCommandKind::migrate_scene_v2_runtime_package;
    request.scene_v2_path = "source/scenes/registered-level.scene";
    request.scene_v2_content = mirakana::serialize_scene_document_v2(make_registered_scene_workflow_scene_v2());
    request.source_registry_path = "source/assets/game.geassets";
    request.source_registry_content =
        mirakana::serialize_source_asset_registry_document(make_registered_cook_source_registry());
    request.package_index_path = "runtime/game.geindex";
    request.package_index_content = std::move(package_index_content);
    request.output_scene_path = "runtime/assets/scenes/registered-level.scene";
    request.scene_asset_key = mirakana::AssetKeyV2{"assets/scenes/registered-level"};
    request.source_revision = 41;
    return request;
}

struct RuntimeScenePackageValidationFixture {
    mirakana::AssetKeyV2 scene_key{"assets/scenes/validation-level"};
    mirakana::AssetId scene_asset{mirakana::asset_id_from_key_v2(scene_key)};
    mirakana::AssetId mesh_asset{mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/meshes/validation-cube"})};
    mirakana::AssetId material_asset{
        mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/validation-hero"})};
    mirakana::AssetId sprite_asset{
        mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/validation-sprite"})};
    std::string package_index_path{"runtime/validation.geindex"};
    std::string scene_path{"runtime/assets/scenes/validation-level.scene"};
    std::string mesh_path{"runtime/assets/meshes/validation-cube.mesh"};
    std::string material_path{"runtime/assets/materials/validation-hero.material"};
    std::string sprite_path{"runtime/assets/textures/validation-sprite.texture"};
    mirakana::RuntimeScenePackageValidationRequest request;
};

[[nodiscard]] mirakana::Scene make_runtime_scene_validation_scene(mirakana::AssetId mesh, mirakana::AssetId material,
                                                                  mirakana::AssetId sprite) {
    mirakana::Scene scene{"Validated Runtime Level"};
    const auto root = scene.create_node("Root");
    const auto prop = scene.create_node("Prop");
    scene.set_parent(prop, root);

    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    components.sprite_renderer = mirakana::SpriteRendererComponent{.sprite = sprite,
                                                                   .material = material,
                                                                   .size = mirakana::Vec2{.x = 1.0F, .y = 2.0F},
                                                                   .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                                                                   .visible = true};
    scene.set_components(prop, components);
    return scene;
}

void configure_runtime_scene_validation_request(RuntimeScenePackageValidationFixture& fixture) {
    fixture.request.kind = mirakana::RuntimeScenePackageValidationCommandKind::validate_runtime_scene_package;
    fixture.request.package_index_path = fixture.package_index_path;
    fixture.request.scene_asset_key = fixture.scene_key;
    fixture.request.validate_asset_references = true;
}

void write_runtime_scene_validation_fixture(mirakana::MemoryFileSystem& fs,
                                            RuntimeScenePackageValidationFixture& fixture,
                                            std::vector<mirakana::AssetCookedArtifact> extra_artifacts = {},
                                            std::string scene_payload = {}) {
    configure_runtime_scene_validation_request(fixture);
    if (scene_payload.empty()) {
        scene_payload = mirakana::serialize_scene(
            make_runtime_scene_validation_scene(fixture.mesh_asset, fixture.material_asset, fixture.sprite_asset));
    }

    std::vector<mirakana::AssetCookedArtifact> artifacts{
        mirakana::AssetCookedArtifact{.asset = fixture.scene_asset,
                                      .kind = mirakana::AssetKind::scene,
                                      .path = fixture.scene_path,
                                      .content = scene_payload,
                                      .source_revision = 7,
                                      .dependencies = {}},
    };
    artifacts.insert(artifacts.end(), extra_artifacts.begin(), extra_artifacts.end());

    fs.write_text(fixture.package_index_path, mirakana::serialize_asset_cooked_package_index(
                                                  mirakana::build_asset_cooked_package_index(artifacts, {})));
    for (const auto& artifact : artifacts) {
        fs.write_text(artifact.path, artifact.content);
    }
}

void write_valid_runtime_scene_validation_fixture(mirakana::MemoryFileSystem& fs,
                                                  RuntimeScenePackageValidationFixture& fixture) {
    write_runtime_scene_validation_fixture(fs, fixture,
                                           {
                                               mirakana::AssetCookedArtifact{.asset = fixture.mesh_asset,
                                                                             .kind = mirakana::AssetKind::mesh,
                                                                             .path = fixture.mesh_path,
                                                                             .content = "mesh",
                                                                             .source_revision = 7,
                                                                             .dependencies = {}},
                                               mirakana::AssetCookedArtifact{.asset = fixture.material_asset,
                                                                             .kind = mirakana::AssetKind::material,
                                                                             .path = fixture.material_path,
                                                                             .content = "material",
                                                                             .source_revision = 7,
                                                                             .dependencies = {}},
                                               mirakana::AssetCookedArtifact{.asset = fixture.sprite_asset,
                                                                             .kind = mirakana::AssetKind::texture,
                                                                             .path = fixture.sprite_path,
                                                                             .content = "sprite",
                                                                             .source_revision = 7,
                                                                             .dependencies = {}},
                                           });
}

[[nodiscard]] std::string runtime_migration_scene_v2_with_duplicate_component_id_content() {
    auto content = mirakana::serialize_scene_document_v2(make_runtime_migration_scene_v2());
    content += "component.4.id=component/root/camera\n";
    content += "component.4.node=node/root\n";
    content += "component.4.type=camera\n";
    content += "component.4.property.0.name=projection\n";
    content += "component.4.property.0.value=perspective\n";
    return content;
}

[[nodiscard]] mirakana::PrefabDocumentV2 make_scene_prefab_authoring_prefab_v2() {
    mirakana::PrefabDocumentV2 prefab;
    prefab.name = "Enemy";
    prefab.scene.name = "Enemy Scene";
    prefab.scene.nodes.push_back(
        mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/enemy-root"}, .name = "EnemyRoot"});
    prefab.scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/enemy/mesh"},
        .node = mirakana::AuthoringId{"node/enemy-root"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "material", .value = "assets/materials/enemy"},
                       {.name = "mesh", .value = "assets/meshes/enemy"}},
    });
    return prefab;
}

[[nodiscard]] bool text_contains(std::string_view text, std::string_view needle) {
    return text.find(needle) != std::string_view::npos;
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::CookedUiAtlasAuthoringFailure>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.diagnostic.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::CookedTilemapAuthoringFailure>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.diagnostic.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::PhysicsCollisionPackageDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(failures, [needle](const auto& failure) {
        return failure.message.find(needle) != std::string::npos || failure.code.find(needle) != std::string::npos;
    });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::MaterialInstancePackageUpdateFailure>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.diagnostic.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::ScenePackageUpdateFailure>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.diagnostic.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::ScenePrefabAuthoringDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.message.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::SourceAssetRegistrationDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(
        failures, [needle](const auto& failure) { return failure.message.find(needle) != std::string::npos; });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::SceneV2RuntimePackageMigrationDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(failures, [needle](const auto& failure) {
        return failure.message.find(needle) != std::string::npos || failure.code.find(needle) != std::string::npos;
    });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::RegisteredSourceAssetCookPackageDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(failures, [needle](const auto& failure) {
        return failure.message.find(needle) != std::string::npos || failure.code.find(needle) != std::string::npos;
    });
}

[[nodiscard]] bool failures_contain(const std::vector<mirakana::RuntimeScenePackageValidationDiagnostic>& failures,
                                    std::string_view needle) {
    return std::ranges::any_of(failures, [needle](const auto& failure) {
        return failure.message.find(needle) != std::string::npos || failure.code.find(needle) != std::string::npos;
    });
}

[[nodiscard]] std::string base64_encode(std::string_view bytes) {
    constexpr auto alphabet = std::string_view{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
    std::string encoded;
    encoded.reserve(((bytes.size() + 2U) / 3U) * 4U);
    for (std::size_t index = 0; index < bytes.size(); index += 3U) {
        const auto remaining = bytes.size() - index;
        const auto first = static_cast<unsigned char>(bytes[index]);
        const auto second = remaining > 1U ? static_cast<unsigned char>(bytes[index + 1U]) : 0U;
        const auto third = remaining > 2U ? static_cast<unsigned char>(bytes[index + 2U]) : 0U;
        const auto packed = (static_cast<std::uint32_t>(first) << 16U) | (static_cast<std::uint32_t>(second) << 8U) |
                            static_cast<std::uint32_t>(third);
        encoded.push_back(alphabet[(packed >> 18U) & 0x3FU]);
        encoded.push_back(alphabet[(packed >> 12U) & 0x3FU]);
        encoded.push_back(remaining > 1U ? alphabet[(packed >> 6U) & 0x3FU] : '=');
        encoded.push_back(remaining > 2U ? alphabet[packed & 0x3FU] : '=');
    }
    return encoded;
}

[[nodiscard]] std::string gltf_data_uri(std::string_view bytes) {
    return "data:application/octet-stream;base64," + base64_encode(bytes);
}

[[nodiscard]] std::string hex_string(std::string_view bytes) {
    constexpr auto digits = std::string_view{"0123456789abcdef"};
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto value : bytes) {
        const auto byte = static_cast<unsigned char>(value);
        encoded.push_back(digits[(byte >> 4U) & 0x0FU]);
        encoded.push_back(digits[byte & 0x0FU]);
    }
    return encoded;
}

[[nodiscard]] std::string wav_pcm16_stereo_with_data_bytes(std::uint32_t data_bytes) {
    std::string result;
    result.reserve(static_cast<std::size_t>(44U) + data_bytes);
    result.append("RIFF", 4);
    append_le_u32(result, 36U + data_bytes);
    result.append("WAVE", 4);
    result.append("fmt ", 4);
    append_le_u32(result, 16U);
    append_le_u16(result, 1U);
    append_le_u16(result, 2U);
    append_le_u32(result, 44100U);
    append_le_u32(result, 176400U);
    append_le_u16(result, 4U);
    append_le_u16(result, 16U);
    result.append("data", 4);
    append_le_u32(result, data_bytes);
    result.resize(result.size() + data_bytes, '\0');
    return result;
}

class FakeExternalAssetImporter final : public mirakana::IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const mirakana::AssetImportAction& action) const noexcept override {
        return action.source_path == "source/textures/player.png" ||
               action.source_path == "source/meshes/player.gltf" || action.source_path == "source/audio/hit.wav";
    }

    [[nodiscard]] std::string import_source_document(mirakana::IFileSystem& filesystem,
                                                     const mirakana::AssetImportAction& action) override {
        ++calls;
        const auto bytes = filesystem.read_text(action.source_path);
        if (bytes.empty()) {
            throw std::invalid_argument("external source is empty");
        }
        if (action.kind == mirakana::AssetImportActionKind::texture) {
            return mirakana::serialize_texture_source_document(mirakana::TextureSourceDocument{
                .width = 64, .height = 32, .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm});
        }
        if (action.kind == mirakana::AssetImportActionKind::mesh) {
            return mirakana::serialize_mesh_source_document(mirakana::MeshSourceDocument{.vertex_count = 12,
                                                                                         .index_count = 18,
                                                                                         .has_normals = true,
                                                                                         .has_uvs = true,
                                                                                         .has_tangent_frame = true});
        }
        if (action.kind == mirakana::AssetImportActionKind::audio) {
            return mirakana::serialize_audio_source_document(
                mirakana::AudioSourceDocument{.sample_rate = 44100,
                                              .channel_count = 2,
                                              .frame_count = 100,
                                              .sample_format = mirakana::AudioSourceSampleFormat::pcm16});
        }
        throw std::invalid_argument("unsupported external source kind");
    }

    int calls{0};
};

[[nodiscard]] mirakana::ShaderSourceMetadata fullscreen_shader() {
    return mirakana::ShaderSourceMetadata{
        .id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl"),
        .source_path = "assets/shaders/fullscreen.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
    };
}

[[nodiscard]] mirakana::ShaderCompileRequest fullscreen_compile_request() {
    return mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    };
}

[[nodiscard]] mirakana::ShaderSourceMetadata fullscreen_fragment_shader() {
    return mirakana::ShaderSourceMetadata{
        .id = mirakana::AssetId::from_name("shaders/fullscreen_ps.hlsl"),
        .source_path = "assets/shaders/fullscreen.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::fragment,
        .entry_point = "ps_main",
    };
}

[[nodiscard]] mirakana::ShaderCompileRequest fullscreen_fragment_compile_request() {
    return mirakana::ShaderCompileRequest{
        .source = fullscreen_fragment_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.ps.dxil",
        .profile = "ps_6_7",
    };
}

[[nodiscard]] mirakana::ShaderCompileExecutionRequest
fullscreen_execution_request(const mirakana::ShaderCompileRequest& request) {
    return mirakana::ShaderCompileExecutionRequest{
        .compile_request = request,
        .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                               .executable_path = "toolchains/dxc/bin/dxc.exe",
                                               .version = "dxcompiler 1.8.2505"},
        .cache_index_path = "out/shaders/shader-cache.gecache",
    };
}

void write_fullscreen_shader_sources(mirakana::MemoryFileSystem& fs) {
    fs.write_text("assets/shaders/fullscreen.hlsl", "#include \"common.hlsl\"\n"
                                                    "float4 vs_main() : SV_Position { return 0; }\n"
                                                    "float4 ps_main() : SV_Target { return shared_color(); }\n");
    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 1; }\n");
}

} // namespace

MK_TEST("material graph shader export serializes emits hlsl and plans shader compile execution requests") {
    const auto material_id = mirakana::AssetId::from_name("materials/pipeline_graph");
    const auto texture_id = mirakana::AssetId::from_name("textures/pipeline.albedo");

    mirakana::MaterialGraphDesc graph{};
    graph.material_id = material_id;
    graph.material_name = "PipelineGraph";
    graph.shading_model = mirakana::MaterialShadingModel::lit;
    graph.surface_mode = mirakana::MaterialSurfaceMode::opaque;
    graph.double_sided = false;
    graph.output_node_id = "root_out";

    mirakana::MaterialGraphNode out_node;
    out_node.id = "root_out";
    out_node.kind = mirakana::MaterialGraphNodeKind::graph_output;

    mirakana::MaterialGraphNode bc;
    bc.id = "bc";
    bc.kind = mirakana::MaterialGraphNodeKind::constant_vec4;
    bc.vec4 = {0.2F, 0.4F, 0.6F, 1.0F};

    mirakana::MaterialGraphNode tex;
    tex.id = "tex0";
    tex.kind = mirakana::MaterialGraphNodeKind::texture;
    tex.texture_slot = mirakana::MaterialTextureSlot::base_color;
    tex.texture_id = texture_id;

    graph.nodes = {out_node, bc, tex};
    graph.edges = {
        mirakana::MaterialGraphEdge{
            .from_node = "bc", .from_socket = "out", .to_node = "root_out", .to_socket = "factor.base_color"},
        mirakana::MaterialGraphEdge{
            .from_node = "tex0", .from_socket = "out", .to_node = "root_out", .to_socket = "texture.base_color"},
    };

    mirakana::MaterialGraphShaderExportDesc export_desc;
    export_desc.export_id = mirakana::AssetId::from_name("exports/pipeline_graph_shader");
    export_desc.export_name = "pipeline_graph_shader";
    export_desc.material_graph_path = "assets/materials/pipeline.materialgraph";
    export_desc.hlsl_source_path = "assets/shaders/mg_pipeline.hlsl";
    export_desc.vertex_entry = "VSMain";
    export_desc.fragment_entry = "PSMain";

    const auto serialized = mirakana::serialize_material_graph_shader_export(export_desc);
    const auto round_trip = mirakana::deserialize_material_graph_shader_export(serialized);
    MK_REQUIRE(round_trip == export_desc);

    const auto hlsl = mirakana::emit_material_graph_reviewed_hlsl_v0(graph);
    MK_REQUIRE(hlsl.find("VSMain") != std::string::npos);
    MK_REQUIRE(hlsl.find("PSMain") != std::string::npos);
    MK_REQUIRE(hlsl.find("register(b6)") != std::string::npos);

    mirakana::MemoryFileSystem fs;
    fs.write_text(export_desc.hlsl_source_path, hlsl);

    mirakana::ShaderToolDescriptor dxc_tool;
    dxc_tool.kind = mirakana::ShaderToolKind::dxc;
    dxc_tool.executable_path = "toolchains/dxc/dxc.exe";
    dxc_tool.version = "test";
    dxc_tool.supports_spirv_codegen = true;

    const auto plan = mirakana::plan_material_graph_shader_pipeline(
        fs, export_desc, "out/material_graph_shader_exports", "shader_cache/index.txt", dxc_tool);
    MK_REQUIRE(plan.ok);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.execution_requests.size() == 4);
    MK_REQUIRE(plan.execution_requests[0].compile_request.target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(plan.execution_requests[1].compile_request.target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(plan.execution_requests[2].compile_request.target == mirakana::ShaderCompileTarget::vulkan_spirv);
    MK_REQUIRE(plan.execution_requests[3].compile_request.target == mirakana::ShaderCompileTarget::vulkan_spirv);

    const auto command = mirakana::make_shader_compile_command(plan.execution_requests[0].compile_request);
    MK_REQUIRE(command.executable == "dxc");
    MK_REQUIRE(plan.execution_requests[2].compile_request.target == mirakana::ShaderCompileTarget::vulkan_spirv);
    const auto spirv_command = mirakana::make_shader_compile_command(plan.execution_requests[2].compile_request);
    MK_REQUIRE(spirv_command.arguments[0] == "-spirv");
}

MK_TEST("shader tool process runner maps commands and captures output") {
    CapturingProcessRunner process_runner;
    process_runner.next_result = mirakana::ProcessResult{
        .launched = true,
        .exit_code = 0,
        .diagnostic = {},
        .stdout_text = "compiled shader\n",
        .stderr_text = {},
    };

    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner, mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                                            .working_directory = "games/sample"});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    const auto result = mirakana::run_shader_tool_command(shader_runner, command);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.stdout_text == "compiled shader\n");
    MK_REQUIRE(result.artifact.path == "out/shaders/fullscreen.vs.dxil");
    MK_REQUIRE(process_runner.calls == 1);
    MK_REQUIRE(process_runner.captured.executable == "dxc");
    MK_REQUIRE(process_runner.captured.working_directory == "games/sample");
    MK_REQUIRE(process_runner.captured.arguments == command.arguments);
}

MK_TEST("shader tool process runner can use reviewed executable override") {
    CapturingProcessRunner process_runner;
    process_runner.next_result = mirakana::ProcessResult{
        .launched = true, .exit_code = 0, .diagnostic = {}, .stdout_text = {}, .stderr_text = {}};

    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner, mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                                            .working_directory = "games/sample",
                                                            .executable_override = "toolchains/dxc/bin/dxc.exe"});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    const auto result = shader_runner.run(command);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(process_runner.captured.executable == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(process_runner.captured.arguments == command.arguments);
}

MK_TEST("shader tool process runner can use reviewed absolute executable override") {
    CapturingProcessRunner process_runner;
    process_runner.next_result = mirakana::ProcessResult{
        .launched = true, .exit_code = 0, .diagnostic = {}, .stdout_text = {}, .stderr_text = {}};

    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                            .working_directory = "games/sample",
                                            .executable_override =
                                                "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe"});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    const auto result = shader_runner.run(command);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(process_runner.captured.executable ==
               "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe");
    MK_REQUIRE(process_runner.captured.arguments == command.arguments);
}

MK_TEST("shader tool process runner rejects shell executable overrides") {
    CapturingProcessRunner process_runner;
    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                            .working_directory = "games/sample",
                                            .executable_override = "C:/Windows/System32/powershell.exe"});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    bool rejected = false;
    try {
        (void)shader_runner.run(command);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
    MK_REQUIRE(process_runner.calls == 0);
}

MK_TEST("shader tool process runner rejects artifacts outside output policy") {
    CapturingProcessRunner process_runner;
    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders", .working_directory = {}});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/other/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    bool rejected = false;
    try {
        (void)mirakana::run_shader_tool_command(shader_runner, command);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
    MK_REQUIRE(process_runner.calls == 0);
}

MK_TEST("shader tool process runner forwards launch diagnostics") {
    CapturingProcessRunner process_runner;
    process_runner.next_result = mirakana::ProcessResult{
        .launched = false,
        .exit_code = 123,
        .diagnostic = "CreateProcessW failed",
        .stdout_text = {},
        .stderr_text = "tool missing",
    };

    mirakana::ShaderToolProcessRunner shader_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders", .working_directory = {}});
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    });

    const auto result = mirakana::run_shader_tool_command(shader_runner, command);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.exit_code == 123);
    MK_REQUIRE(result.diagnostic == "CreateProcessW failed");
    MK_REQUIRE(result.stderr_text == "tool missing");
}

MK_TEST("spirv shader validation command uses validator executable and process policy") {
    const mirakana::ShaderGeneratedArtifact artifact{
        .path = "out/shaders/fullscreen.vs.spv",
        .format = mirakana::ShaderArtifactFormat::spirv,
        .profile = "vs_6_7",
        .entry_point = "vs_main",
    };

    const auto command = mirakana::make_spirv_shader_validation_command(artifact);

    MK_REQUIRE(command.executable == "spirv-val");
    MK_REQUIRE(command.arguments.size() == 1);
    MK_REQUIRE(command.arguments[0] == "out/shaders/fullscreen.vs.spv");
    MK_REQUIRE(command.artifact.path == artifact.path);
    MK_REQUIRE(mirakana::is_safe_shader_artifact_validation_command(command));

    CapturingProcessRunner process_runner;
    mirakana::ShaderArtifactValidationProcessRunner validator_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                            .working_directory = "games/sample",
                                            .executable_override = "toolchains/vulkan/bin/spirv-val.exe"});

    const auto result = mirakana::run_shader_artifact_validation_command(validator_runner, command);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(process_runner.calls == 1);
    MK_REQUIRE(process_runner.captured.executable == "toolchains/vulkan/bin/spirv-val.exe");
    MK_REQUIRE(process_runner.captured.arguments == command.arguments);
    MK_REQUIRE(process_runner.captured.working_directory == "games/sample");
}

MK_TEST("spirv shader validation process runner can use reviewed absolute executable override") {
    const mirakana::ShaderGeneratedArtifact artifact{
        .path = "out/shaders/fullscreen.vs.spv",
        .format = mirakana::ShaderArtifactFormat::spirv,
        .profile = "vs_6_7",
        .entry_point = "vs_main",
    };
    const auto command = mirakana::make_spirv_shader_validation_command(artifact);

    CapturingProcessRunner process_runner;
    mirakana::ShaderArtifactValidationProcessRunner validator_runner(
        process_runner,
        mirakana::ShaderToolExecutionPolicy{.artifact_output_root = "out/shaders",
                                            .working_directory = "games/sample",
                                            .executable_override = "C:/VulkanSDK/1.4.321.1/Bin/spirv-val.exe"});

    const auto result = mirakana::run_shader_artifact_validation_command(validator_runner, command);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(process_runner.calls == 1);
    MK_REQUIRE(process_runner.captured.executable == "C:/VulkanSDK/1.4.321.1/Bin/spirv-val.exe");
    MK_REQUIRE(process_runner.captured.arguments == command.arguments);
}

MK_TEST("shader artifact validation action checks existing spirv artifacts") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("out/shaders/fullscreen.vs.spv", "compiled spirv bytes");
    const mirakana::ShaderGeneratedArtifact artifact{
        .path = "out/shaders/fullscreen.vs.spv",
        .format = mirakana::ShaderArtifactFormat::spirv,
        .profile = "vs_6_7",
        .entry_point = "vs_main",
    };

    CountingShaderArtifactValidatorRunner runner;
    const auto result = mirakana::execute_shader_artifact_validation_action(
        fs, runner,
        mirakana::ShaderArtifactValidationExecutionRequest{
            .artifact = artifact,
            .validator = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::spirv_val,
                                                        .executable_path = "toolchains/vulkan/bin/spirv-val.exe",
                                                        .version = "SPIRV-Tools 2025.1"},
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.artifact_checked);
    MK_REQUIRE(runner.calls == 1);
    MK_REQUIRE(runner.last_command.arguments[0] == "out/shaders/fullscreen.vs.spv");

    CountingShaderArtifactValidatorRunner missing_runner;
    const auto missing = mirakana::execute_shader_artifact_validation_action(
        fs, missing_runner,
        mirakana::ShaderArtifactValidationExecutionRequest{
            .artifact = mirakana::ShaderGeneratedArtifact{.path = "out/shaders/missing.spv",
                                                          .format = mirakana::ShaderArtifactFormat::spirv,
                                                          .profile = "vs_6_7",
                                                          .entry_point = "vs_main"},
            .validator = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::spirv_val,
                                                        .executable_path = "toolchains/vulkan/bin/spirv-val.exe",
                                                        .version = "SPIRV-Tools 2025.1"},
        });

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(!missing.artifact_checked);
    MK_REQUIRE(missing_runner.calls == 0);
    MK_REQUIRE(missing.validation_result.diagnostic.find("missing") != std::string::npos);
}

MK_TEST("shader toolchain discovery finds versioned tools deterministically") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("toolchains/dxc/bin/dxc.exe", "binary marker");
    fs.write_text("toolchains/dxc/bin/dxc.version", "dxcompiler 1.8.2505\n");
    fs.write_text("toolchains/dxc/bin/dxc.spirv-codegen", "enabled\n");
    fs.write_text("toolchains/vulkan/bin/spirv-val.exe", "binary marker");
    fs.write_text("toolchains/vulkan/bin/spirv-val.version", "SPIRV-Tools 2025.1\n");
    fs.write_text("toolchains/apple/bin/metal", "binary marker");
    fs.write_text("toolchains/apple/bin/metallib", "binary marker");
    fs.write_text("toolchains/apple/bin/metal.version", "metal 32023.45");

    const auto tools = mirakana::discover_shader_tools(
        fs, mirakana::ShaderToolDiscoveryRequest{
                .search_roots = {"toolchains/apple/bin", "toolchains/vulkan/bin", "toolchains/dxc/bin"}});

    MK_REQUIRE(tools.size() == 4);
    MK_REQUIRE(tools[0].kind == mirakana::ShaderToolKind::dxc);
    MK_REQUIRE(tools[0].executable_path == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(tools[0].version == "dxcompiler 1.8.2505");
    MK_REQUIRE(tools[0].supports_spirv_codegen);
    MK_REQUIRE(tools[1].kind == mirakana::ShaderToolKind::spirv_val);
    MK_REQUIRE(tools[1].executable_path == "toolchains/vulkan/bin/spirv-val.exe");
    MK_REQUIRE(tools[1].version == "SPIRV-Tools 2025.1");
    MK_REQUIRE(tools[2].kind == mirakana::ShaderToolKind::metal);
    MK_REQUIRE(tools[2].executable_path == "toolchains/apple/bin/metal");
    MK_REQUIRE(tools[2].version == "metal 32023.45");
    MK_REQUIRE(tools[3].kind == mirakana::ShaderToolKind::metallib);
    MK_REQUIRE(tools[3].executable_path == "toolchains/apple/bin/metallib");
    MK_REQUIRE(tools[3].version == "unknown");
}

MK_TEST("shader toolchain readiness reports backend-specific missing tools") {
    const std::vector<mirakana::ShaderToolDescriptor> dxc_only{
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                       .version = "dxcompiler 1.8.2505"},
    };

    const auto partial = mirakana::evaluate_shader_toolchain_readiness(dxc_only);

    MK_REQUIRE(partial.dxc_available);
    MK_REQUIRE(partial.ready_for_d3d12_dxil());
    MK_REQUIRE(!partial.ready_for_vulkan_spirv());
    MK_REQUIRE(!partial.ready_for_metal_library());
    MK_REQUIRE(partial.diagnostics.size() == 4);
    MK_REQUIRE(partial.diagnostics[0].find("SPIR-V CodeGen") != std::string::npos);
    MK_REQUIRE(partial.diagnostics[1].find("spirv-val") != std::string::npos);
    MK_REQUIRE(partial.diagnostics[2].find("metal") != std::string::npos);
    MK_REQUIRE(partial.diagnostics[3].find("metallib") != std::string::npos);

    const std::vector<mirakana::ShaderToolDescriptor> validator_with_dxil_only_dxc{
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                       .version = "dxcompiler 1.8.2505"},
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::spirv_val,
                                       .executable_path = "toolchains/vulkan/bin/spirv-val.exe",
                                       .version = "SPIRV-Tools 2025.1"},
    };
    const auto no_spirv_codegen = mirakana::evaluate_shader_toolchain_readiness(validator_with_dxil_only_dxc);
    MK_REQUIRE(no_spirv_codegen.ready_for_d3d12_dxil());
    MK_REQUIRE(!no_spirv_codegen.ready_for_vulkan_spirv());
    MK_REQUIRE(no_spirv_codegen.diagnostics[0].find("SPIR-V CodeGen") != std::string::npos);

    const std::vector<mirakana::ShaderToolDescriptor> complete{
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                       .version = "dxcompiler 1.8.2505",
                                       .supports_spirv_codegen = true},
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::spirv_val,
                                       .executable_path = "toolchains/vulkan/bin/spirv-val.exe",
                                       .version = "SPIRV-Tools 2025.1"},
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::metal,
                                       .executable_path = "toolchains/apple/bin/metal",
                                       .version = "metal 32023.45"},
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::metallib,
                                       .executable_path = "toolchains/apple/bin/metallib",
                                       .version = "unknown"},
    };

    const auto ready = mirakana::evaluate_shader_toolchain_readiness(complete);

    MK_REQUIRE(ready.ready_for_d3d12_dxil());
    MK_REQUIRE(ready.ready_for_vulkan_spirv());
    MK_REQUIRE(ready.ready_for_metal_library());
    MK_REQUIRE(ready.diagnostics.empty());
}

MK_TEST("shader artifact provenance invalidates cache when inputs change") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("assets/shaders/fullscreen.hlsl",
                  "#include \"common.hlsl\"\nfloat4 vs_main() : SV_Position { return 0; }\n");
    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 1; }\n");
    fs.write_text("out/shaders/fullscreen.vs.dxil", "compiled bytes");

    const auto request = mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    };
    const auto command = mirakana::make_shader_compile_command(request);
    const auto provenance = mirakana::build_shader_artifact_provenance(
        fs, request, command,
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                       .version = "dxcompiler 1.8.2505"});

    MK_REQUIRE(provenance.inputs.size() == 2);
    MK_REQUIRE(provenance.inputs[0].path == "assets/shaders/common.hlsl");
    MK_REQUIRE(provenance.inputs[1].path == "assets/shaders/fullscreen.hlsl");
    MK_REQUIRE(mirakana::is_shader_artifact_cache_valid(fs, provenance));

    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 0; }\n");

    MK_REQUIRE(!mirakana::is_shader_artifact_cache_valid(fs, provenance));
}

MK_TEST("shader artifact provenance serializes deterministic records") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("assets/shaders/fullscreen.hlsl",
                  "#include \"common.hlsl\"\nfloat4 vs_main() : SV_Position { return 0; }\n");
    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 1; }\n");
    fs.write_text("out/shaders/fullscreen.vs.dxil", "compiled bytes");

    const auto request = mirakana::ShaderCompileRequest{
        .source = fullscreen_shader(),
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
    };
    const auto command = mirakana::make_shader_compile_command(request);
    const auto provenance = mirakana::build_shader_artifact_provenance(
        fs, request, command,
        mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                       .version = "dxcompiler 1.8.2505"});

    const auto text = mirakana::serialize_shader_artifact_provenance(provenance);
    const auto restored = mirakana::deserialize_shader_artifact_provenance(text);

    MK_REQUIRE(text.find("format=GameEngine.ShaderArtifactProvenance.v1\n") == 0);
    MK_REQUIRE(text.find("tool.kind=dxc\n") != std::string::npos);
    MK_REQUIRE(text.find("input.1.path=assets/shaders/common.hlsl\n") != std::string::npos);
    MK_REQUIRE(restored.artifact.path == "out/shaders/fullscreen.vs.dxil");
    MK_REQUIRE(restored.tool.executable_path == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(restored.inputs.size() == 2);
    MK_REQUIRE(restored.inputs[0].digest == provenance.inputs[0].digest);
}

MK_TEST("shader artifact cache index serializes deterministic entries") {
    mirakana::ShaderArtifactCacheIndex index;
    mirakana::upsert_shader_artifact_cache_entry(index, mirakana::ShaderArtifactCacheIndexEntry{
                                                            .artifact_path = "out/shaders/z.ps.dxil",
                                                            .provenance_path = "out/shaders/z.ps.dxil.geprovenance",
                                                            .command_fingerprint = "bbb",
                                                        });
    mirakana::upsert_shader_artifact_cache_entry(index, mirakana::ShaderArtifactCacheIndexEntry{
                                                            .artifact_path = "out/shaders/a.vs.dxil",
                                                            .provenance_path = "out/shaders/a.vs.dxil.geprovenance",
                                                            .command_fingerprint = "aaa",
                                                        });

    const auto text = mirakana::serialize_shader_artifact_cache_index(index);
    const auto restored = mirakana::deserialize_shader_artifact_cache_index(text);

    MK_REQUIRE(text.find("format=GameEngine.ShaderArtifactCacheIndex.v1\n") == 0);
    MK_REQUIRE(text.find("entry.1.artifact=out/shaders/a.vs.dxil\n") != std::string::npos);
    MK_REQUIRE(restored.entries.size() == 2);
    MK_REQUIRE(restored.entries[0].artifact_path == "out/shaders/a.vs.dxil");
    MK_REQUIRE(restored.entries[1].artifact_path == "out/shaders/z.ps.dxil");
}

MK_TEST("shader compile action writes artifact provenance and reuses valid cache") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);

    CountingShaderToolRunner first_runner;
    const auto first = mirakana::execute_shader_compile_action(
        fs, first_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(!first.cache_hit);
    MK_REQUIRE(first.artifact_written);
    MK_REQUIRE(first_runner.calls == 1);
    MK_REQUIRE(fs.exists("out/shaders/fullscreen.vs.dxil"));
    MK_REQUIRE(fs.exists("out/shaders/fullscreen.vs.dxil.geprovenance"));
    MK_REQUIRE(fs.exists("out/shaders/shader-cache.gecache"));

    const auto index = mirakana::load_shader_artifact_cache_index(fs, "out/shaders/shader-cache.gecache");
    MK_REQUIRE(index.entries.size() == 1);
    MK_REQUIRE(index.entries[0].artifact_path == "out/shaders/fullscreen.vs.dxil");

    CountingShaderToolRunner cached_runner;
    cached_runner.fail = true;
    const auto cached = mirakana::execute_shader_compile_action(
        fs, cached_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });

    MK_REQUIRE(cached.succeeded());
    MK_REQUIRE(cached.cache_hit);
    MK_REQUIRE(!cached.artifact_written);
    MK_REQUIRE(cached_runner.calls == 0);
}

MK_TEST("shader compile action invalidates cache when source changes") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);

    CountingShaderToolRunner first_runner;
    (void)mirakana::execute_shader_compile_action(
        fs, first_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });

    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 0; }\n");

    CountingShaderToolRunner second_runner;
    const auto second = mirakana::execute_shader_compile_action(
        fs, second_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });

    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(!second.cache_hit);
    MK_REQUIRE(second_runner.calls == 1);
}

MK_TEST("shader hot reload plan uses provenance to gate pipeline recreation") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);

    CountingShaderToolRunner runner;
    (void)mirakana::execute_shader_compile_action(
        fs, runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });

    const auto up_to_date = mirakana::build_shader_hot_reload_plan(
        fs, mirakana::ShaderCompileExecutionRequest{
                .compile_request = fullscreen_compile_request(),
                .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                       .version = "dxcompiler 1.8.2505"},
                .cache_index_path = "out/shaders/shader-cache.gecache",
            });
    MK_REQUIRE(up_to_date.artifact.path == "out/shaders/fullscreen.vs.dxil");
    MK_REQUIRE(up_to_date.artifact_exists);
    MK_REQUIRE(up_to_date.provenance_exists);
    MK_REQUIRE(up_to_date.inputs_current);
    MK_REQUIRE(!up_to_date.compile_required);
    MK_REQUIRE(!up_to_date.pipeline_recreation_required);

    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 0.5; }\n");

    const auto stale = mirakana::build_shader_hot_reload_plan(
        fs, mirakana::ShaderCompileExecutionRequest{
                .compile_request = fullscreen_compile_request(),
                .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                       .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                       .version = "dxcompiler 1.8.2505"},
                .cache_index_path = "out/shaders/shader-cache.gecache",
            });
    MK_REQUIRE(stale.artifact_exists);
    MK_REQUIRE(stale.provenance_exists);
    MK_REQUIRE(!stale.inputs_current);
    MK_REQUIRE(stale.compile_required);
    MK_REQUIRE(stale.pipeline_recreation_required);
    MK_REQUIRE(stale.diagnostic.find("stale") != std::string::npos);
}

MK_TEST("shader pipeline recreation plan follows compile action results") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);

    CountingShaderToolRunner first_runner;
    const auto first = mirakana::execute_shader_compile_action(
        fs, first_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });
    const auto first_recreation = mirakana::build_shader_pipeline_recreation_plan(first);
    MK_REQUIRE(first_recreation.recreate_pipeline);
    MK_REQUIRE(first_recreation.artifact.path == "out/shaders/fullscreen.vs.dxil");
    MK_REQUIRE(first_recreation.target == mirakana::ShaderCompileTarget::d3d12_dxil);

    CountingShaderToolRunner cached_runner;
    cached_runner.fail = true;
    const auto cached = mirakana::execute_shader_compile_action(
        fs, cached_runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
        });
    const auto cached_recreation = mirakana::build_shader_pipeline_recreation_plan(cached);
    MK_REQUIRE(!cached_recreation.recreate_pipeline);
    MK_REQUIRE(cached_recreation.diagnostic.find("cache hit") != std::string::npos);
}

MK_TEST("shader compile action requires real artifact when marker writes are disabled") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);

    CountingShaderToolRunner runner;
    const auto result = mirakana::execute_shader_compile_action(
        fs, runner,
        mirakana::ShaderCompileExecutionRequest{
            .compile_request = fullscreen_compile_request(),
            .tool = mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                   .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                   .version = "dxcompiler 1.8.2505"},
            .cache_index_path = "out/shaders/shader-cache.gecache",
            .allow_cache = true,
            .write_artifact_marker = false,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.cache_hit);
    MK_REQUIRE(!result.artifact_written);
    MK_REQUIRE(result.tool_result.diagnostic.find("artifact") != std::string::npos);
    MK_REQUIRE(!fs.exists("out/shaders/fullscreen.vs.dxil.geprovenance"));
    MK_REQUIRE(!fs.exists("out/shaders/shader-cache.gecache"));
}

MK_TEST("shader pipeline cache plan batches hot reload and repairs missing cache index") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);
    const std::vector<mirakana::ShaderCompileExecutionRequest> requests{
        fullscreen_execution_request(fullscreen_compile_request()),
        fullscreen_execution_request(fullscreen_fragment_compile_request()),
    };

    CountingShaderToolRunner runner;
    for (const auto& request : requests) {
        const auto result = mirakana::execute_shader_compile_action(fs, runner, request);
        MK_REQUIRE(result.succeeded());
        MK_REQUIRE(!result.cache_hit);
    }
    const auto vertex_artifact = fs.read_text("out/shaders/fullscreen.vs.dxil");
    const auto fragment_artifact = fs.read_text("out/shaders/fullscreen.ps.dxil");
    fs.remove("out/shaders/shader-cache.gecache");

    const auto plan = mirakana::build_shader_pipeline_cache_plan(fs, requests);

    MK_REQUIRE(plan.ok);
    MK_REQUIRE(plan.entries.size() == 2);
    MK_REQUIRE(plan.compile_required_count == 0);
    MK_REQUIRE(plan.pipeline_recreation_required_count == 0);
    MK_REQUIRE(plan.cache_index_update_required_count == 2);
    MK_REQUIRE(plan.entries[0].artifact.path == "out/shaders/fullscreen.ps.dxil");
    MK_REQUIRE(plan.entries[0].cache_index_path == "out/shaders/shader-cache.gecache");
    MK_REQUIRE(plan.entries[0].cache_index_update_required);
    MK_REQUIRE(!plan.entries[0].cache_index_exists);
    MK_REQUIRE(!plan.entries[0].cache_entry_exists);
    MK_REQUIRE(!plan.entries[0].cache_entry_current);

    const auto reconciled = mirakana::reconcile_shader_pipeline_cache_index(fs, requests);

    MK_REQUIRE(reconciled.plan.ok);
    MK_REQUIRE(reconciled.written_cache_index_paths.size() == 1);
    MK_REQUIRE(reconciled.written_cache_index_paths[0] == "out/shaders/shader-cache.gecache");
    MK_REQUIRE(fs.read_text("out/shaders/fullscreen.vs.dxil") == vertex_artifact);
    MK_REQUIRE(fs.read_text("out/shaders/fullscreen.ps.dxil") == fragment_artifact);

    const auto index = mirakana::load_shader_artifact_cache_index(fs, "out/shaders/shader-cache.gecache");
    MK_REQUIRE(index.entries.size() == 2);
    MK_REQUIRE(index.entries[0].artifact_path == "out/shaders/fullscreen.ps.dxil");
    MK_REQUIRE(index.entries[1].artifact_path == "out/shaders/fullscreen.vs.dxil");
}

MK_TEST("shader pipeline cache plan rejects empty compile request list") {
    mirakana::MemoryFileSystem fs;
    const auto plan = mirakana::build_shader_pipeline_cache_plan(fs, {});
    MK_REQUIRE(!plan.ok);
    MK_REQUIRE(plan.entries.empty());
    MK_REQUIRE(plan.compile_required_count == 0);
    MK_REQUIRE(plan.pipeline_recreation_required_count == 0);
    MK_REQUIRE(plan.cache_index_update_required_count == 0);
    MK_REQUIRE(plan.diagnostics.size() == 1);
    MK_REQUIRE(plan.diagnostics[0].find("at least one request") != std::string::npos);

    const auto reconciled = mirakana::reconcile_shader_pipeline_cache_index(fs, {});
    MK_REQUIRE(!reconciled.plan.ok);
    MK_REQUIRE(reconciled.written_cache_index_paths.empty());
}

MK_TEST("shader pipeline cache plan keeps stale inputs on compile path") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);
    const std::vector<mirakana::ShaderCompileExecutionRequest> requests{
        fullscreen_execution_request(fullscreen_compile_request()),
        fullscreen_execution_request(fullscreen_fragment_compile_request()),
    };

    CountingShaderToolRunner runner;
    for (const auto& request : requests) {
        (void)mirakana::execute_shader_compile_action(fs, runner, request);
    }

    fs.write_text("assets/shaders/common.hlsl", "float4 shared_color() { return 0.5; }\n");

    const auto plan = mirakana::build_shader_pipeline_cache_plan(fs, requests);

    MK_REQUIRE(plan.ok);
    MK_REQUIRE(plan.entries.size() == 2);
    MK_REQUIRE(plan.compile_required_count == 2);
    MK_REQUIRE(plan.pipeline_recreation_required_count == 2);
    MK_REQUIRE(plan.cache_index_update_required_count == 0);
    MK_REQUIRE(!plan.entries[0].cache_index_update_required);
    MK_REQUIRE(!plan.entries[0].cache_entry_current);
    MK_REQUIRE(plan.entries[0].hot_reload.compile_required);
    MK_REQUIRE(plan.entries[0].hot_reload.pipeline_recreation_required);
}

MK_TEST("shader pipeline cache reconciliation rebuilds malformed selected index from current provenance") {
    mirakana::MemoryFileSystem fs;
    write_fullscreen_shader_sources(fs);
    const std::vector<mirakana::ShaderCompileExecutionRequest> requests{
        fullscreen_execution_request(fullscreen_compile_request()),
        fullscreen_execution_request(fullscreen_fragment_compile_request()),
    };

    CountingShaderToolRunner runner;
    for (const auto& request : requests) {
        (void)mirakana::execute_shader_compile_action(fs, runner, request);
    }
    fs.write_text("out/shaders/shader-cache.gecache", "format=not-a-cache\n");

    const auto plan = mirakana::build_shader_pipeline_cache_plan(fs, requests);

    MK_REQUIRE(plan.ok);
    MK_REQUIRE(plan.entries.size() == 2);
    MK_REQUIRE(plan.compile_required_count == 0);
    MK_REQUIRE(plan.pipeline_recreation_required_count == 0);
    MK_REQUIRE(plan.cache_index_update_required_count == 2);
    MK_REQUIRE(plan.entries[0].cache_index_exists);
    MK_REQUIRE(plan.entries[0].cache_diagnostic.find("invalid") != std::string::npos);

    const auto reconciled = mirakana::reconcile_shader_pipeline_cache_index(fs, requests);
    const auto index = mirakana::load_shader_artifact_cache_index(fs, "out/shaders/shader-cache.gecache");

    MK_REQUIRE(reconciled.plan.ok);
    MK_REQUIRE(reconciled.written_cache_index_paths.size() == 1);
    MK_REQUIRE(index.entries.size() == 2);
}

MK_TEST("asset import executor writes cooked artifacts through filesystem") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    fs.write_text("source/textures/player_albedo.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                                  "texture.width=4\n"
                                                                  "texture.height=2\n"
                                                                  "texture.pixel_format=rgba8_unorm\n");
    fs.write_text("source/meshes/player.mesh_source", "format=GameEngine.MeshSource.v2\n"
                                                      "mesh.vertex_count=24\n"
                                                      "mesh.index_count=36\n"
                                                      "mesh.has_normals=false\n"
                                                      "mesh.has_uvs=false\n"
                                                      "mesh.has_tangent_frame=false\n");
    fs.write_text("source/audio/hit.audio_source", "format=GameEngine.AudioSource.v1\n"
                                                   "audio.sample_rate=48000\n"
                                                   "audio.channel_count=2\n"
                                                   "audio.frame_count=24000\n"
                                                   "audio.sample_format=pcm16\n");
    fs.write_text("source/scenes/level.scene", "format=GameEngine.Scene.v1\n"
                                               "scene.name=Level\n"
                                               "node.count=1\n"
                                               "node.1.name=Root\n"
                                               "node.1.parent=0\n"
                                               "node.1.position=0,0,0\n"
                                               "node.1.scale=1,1,1\n"
                                               "node.1.rotation=0,0,0\n");
    fs.write_text("source/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player_albedo.texture_source",
        .imported_path = "assets/textures/player_albedo.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/player.mesh_source",
        .imported_path = "assets/meshes/player.mesh",
        .scale = 1.0F,
        .generate_lods = false,
        .generate_collision = true,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_audio(mirakana::AudioImportMetadata{
        .id = audio_id,
        .source_path = "source/audio/hit.audio_source",
        .imported_path = "assets/audio/hit.audio",
        .streaming = false,
    });
    imports.add_scene(mirakana::SceneImportMetadata{
        .id = scene_id,
        .source_path = "source/scenes/level.scene",
        .imported_path = "assets/scenes/level.scene",
        .mesh_dependencies = {mesh_id},
        .material_dependencies = {material_id},
        .sprite_dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, mirakana::build_asset_import_plan(imports));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.imported.size() == 5);
    MK_REQUIRE(fs.exists("assets/audio/hit.audio"));
    MK_REQUIRE(fs.exists("assets/materials/player.material"));
    MK_REQUIRE(fs.exists("assets/meshes/player.mesh"));
    MK_REQUIRE(fs.exists("assets/scenes/level.scene"));
    MK_REQUIRE(fs.exists("assets/textures/player_albedo.texture"));
    MK_REQUIRE(fs.read_text("assets/materials/player.material").find("format=GameEngine.Material.v1\n") == 0);
    MK_REQUIRE(fs.read_text("assets/textures/player_albedo.texture").find("format=GameEngine.CookedTexture.v1\n") == 0);
    MK_REQUIRE(fs.read_text("assets/textures/player_albedo.texture").find("texture.width=4\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/textures/player_albedo.texture").find("texture.source_bytes=32\n") !=
               std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/player.mesh").find("format=GameEngine.CookedMesh.v2\n") == 0);
    MK_REQUIRE(fs.read_text("assets/meshes/player.mesh").find("mesh.vertex_count=24\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/player.mesh").find("mesh.has_normals=false\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/hit.audio").find("format=GameEngine.CookedAudio.v1\n") == 0);
    MK_REQUIRE(fs.read_text("assets/audio/hit.audio").find("audio.frame_count=24000\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/hit.audio").find("audio.source_bytes=96000\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/scenes/level.scene").find("format=GameEngine.Scene.v1\n") == 0);
    MK_REQUIRE(fs.read_text("assets/scenes/level.scene").find("scene.name=Level\n") != std::string::npos);
}

MK_TEST("tilemap tooling authors deterministic package data and tilemap_texture rows") {
    const auto desc = make_cooked_tilemap_authoring_desc();
    const auto expected = expected_cooked_tilemap_metadata();

    const auto authored = mirakana::author_cooked_tilemap_metadata(desc);

    MK_REQUIRE(authored.succeeded());
    MK_REQUIRE(authored.failures.empty());
    MK_REQUIRE(authored.content == expected);
    MK_REQUIRE(authored.artifact.asset == desc.tilemap_asset);
    MK_REQUIRE(authored.artifact.kind == mirakana::AssetKind::tilemap);
    MK_REQUIRE(authored.artifact.path == desc.output_path);
    MK_REQUIRE(authored.artifact.content == expected);
    MK_REQUIRE(authored.artifact.source_revision == 11);
    MK_REQUIRE(authored.artifact.dependencies == std::vector<mirakana::AssetId>({mirakana::AssetId{10}}));
    MK_REQUIRE(authored.dependency_edges.size() == 1);
    MK_REQUIRE(authored.dependency_edges[0].asset == desc.tilemap_asset);
    MK_REQUIRE(authored.dependency_edges[0].dependency == mirakana::AssetId{10});
    MK_REQUIRE(authored.dependency_edges[0].kind == mirakana::AssetDependencyKind::tilemap_texture);
    MK_REQUIRE(authored.dependency_edges[0].path == desc.output_path);

    mirakana::MemoryFileSystem fs;
    const auto written = mirakana::write_cooked_tilemap_metadata(fs, desc);
    MK_REQUIRE(written.succeeded());
    MK_REQUIRE(fs.read_text(desc.output_path) == expected);
}

MK_TEST("tilemap tooling rejects malformed tilemaps and production capability claims") {
    auto desc = make_cooked_tilemap_authoring_desc();
    desc.layers[0].cells[1] = "missing";
    auto missing_tile = mirakana::author_cooked_tilemap_metadata(desc);
    MK_REQUIRE(!missing_tile.succeeded());
    MK_REQUIRE(failures_contain(missing_tile.failures, "unknown tile id"));

    desc = make_cooked_tilemap_authoring_desc();
    desc.tiles[0].u0 = desc.tiles[0].u1;
    auto malformed_uv = mirakana::author_cooked_tilemap_metadata(desc);
    MK_REQUIRE(!malformed_uv.succeeded());
    MK_REQUIRE(failures_contain(malformed_uv.failures, "uv rect is invalid"));

    desc = make_cooked_tilemap_authoring_desc();
    desc.source_decoding = "ready";
    auto source_decode_claim = mirakana::author_cooked_tilemap_metadata(desc);
    MK_REQUIRE(!source_decode_claim.succeeded());
    MK_REQUIRE(failures_contain(source_decode_claim.failures, "source image decoding is not supported"));

    desc = make_cooked_tilemap_authoring_desc();
    desc.atlas_packing = "ready";
    auto packing_claim = mirakana::author_cooked_tilemap_metadata(desc);
    MK_REQUIRE(!packing_claim.succeeded());
    MK_REQUIRE(failures_contain(packing_claim.failures, "production atlas packing is not supported"));

    desc = make_cooked_tilemap_authoring_desc();
    desc.native_gpu_sprite_batching = "ready";
    auto native_gpu_claim = mirakana::author_cooked_tilemap_metadata(desc);
    MK_REQUIRE(!native_gpu_claim.succeeded());
    MK_REQUIRE(failures_contain(native_gpu_claim.failures, "native GPU sprite batching is not supported"));
}

MK_TEST("tilemap apply tooling dry-runs tilemap and package index changes") {
    const auto desc = make_cooked_tilemap_authoring_desc();
    mirakana::CookedTilemapPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = tilemap_base_package_index_content();
    update.tilemap = desc;

    const auto result = mirakana::plan_cooked_tilemap_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.tilemap_content == expected_cooked_tilemap_metadata());
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == desc.output_path);
    MK_REQUIRE(result.changed_files[0].content == expected_cooked_tilemap_metadata());
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto verification =
        mirakana::verify_cooked_tilemap_package_metadata(updated_index, desc.tilemap_asset, result.tilemap_content);
    MK_REQUIRE(verification.succeeded());
}

MK_TEST("tilemap apply tooling rejects missing page rows and unsafe paths") {
    mirakana::CookedTilemapPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = tilemap_base_package_index_content();
    update.tilemap = make_cooked_tilemap_authoring_desc();
    update.tilemap.output_path = "../level.tilemap";

    const auto unsafe = mirakana::plan_cooked_tilemap_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.failures, "package-relative"));

    update.tilemap = make_cooked_tilemap_authoring_desc();
    auto index = mirakana::deserialize_asset_cooked_package_index(tilemap_base_package_index_content());
    index.entries[0].kind = mirakana::AssetKind::material;
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);

    const auto wrong_page_kind = mirakana::plan_cooked_tilemap_package_update(update);
    MK_REQUIRE(!wrong_page_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_page_kind.failures, "tilemap atlas texture package entry is not a texture"));
}

MK_TEST("physics collision package tooling authors deterministic payload and package row") {
    const auto update = make_physics_collision_package_update_desc();
    const auto expected = expected_physics_collision_package_payload();

    const auto authored = mirakana::author_physics_collision_package_scene(update.collision);

    MK_REQUIRE(authored.succeeded());
    MK_REQUIRE(authored.diagnostics.empty());
    MK_REQUIRE(authored.content == expected);
    MK_REQUIRE(authored.artifact.asset == update.collision.collision_asset);
    MK_REQUIRE(authored.artifact.kind == mirakana::AssetKind::physics_collision_scene);
    MK_REQUIRE(authored.artifact.path == update.collision.output_path);
    MK_REQUIRE(authored.artifact.content == expected);
    MK_REQUIRE(authored.artifact.source_revision == update.collision.source_revision);
    MK_REQUIRE(authored.artifact.dependencies.empty());

    const auto result = mirakana::plan_physics_collision_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.collision_content == expected);
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == update.collision.output_path);
    MK_REQUIRE(result.changed_files[0].content == expected);
    MK_REQUIRE(result.changed_files[0].content_hash == mirakana::hash_asset_cooked_content(expected));
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto entry_it = std::ranges::find_if(updated_index.entries, [&update](const auto& entry) {
        return entry.asset == update.collision.collision_asset;
    });
    MK_REQUIRE(entry_it != updated_index.entries.end());
    MK_REQUIRE(entry_it->kind == mirakana::AssetKind::physics_collision_scene);
    MK_REQUIRE(entry_it->path == update.collision.output_path);
    MK_REQUIRE(entry_it->content_hash == mirakana::hash_asset_cooked_content(expected));
    MK_REQUIRE(entry_it->source_revision == update.collision.source_revision);
    MK_REQUIRE(entry_it->dependencies.empty());
    MK_REQUIRE(updated_index.dependencies.empty());

    const auto verification = mirakana::verify_physics_collision_package_scene(
        updated_index, update.collision.collision_asset, result.collision_content);
    MK_REQUIRE(verification.succeeded());
}

MK_TEST("physics collision package tooling applies only after validation passes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/sample.geindex",
                  mirakana::serialize_asset_cooked_package_index(mirakana::AssetCookedPackageIndex{}));

    mirakana::PhysicsCollisionPackageApplyDesc apply;
    apply.package_index_path = "runtime/sample.geindex";
    apply.collision = make_physics_collision_package_update_desc().collision;

    const auto result = mirakana::apply_physics_collision_package_update(fs, apply);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text(apply.collision.output_path) == expected_physics_collision_package_payload());
    MK_REQUIRE(fs.read_text(apply.package_index_path) == result.package_index_content);

    auto invalid = apply;
    invalid.collision.bodies[1].name = invalid.collision.bodies[0].name;
    const auto collision_before = fs.read_text(apply.collision.output_path);
    const auto index_before = fs.read_text(apply.package_index_path);
    const auto failed = mirakana::apply_physics_collision_package_update(fs, invalid);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.diagnostics, "duplicate_collision_body_name"));
    MK_REQUIRE(fs.read_text(apply.collision.output_path) == collision_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);

    ThrowingWriteFileSystem failing_fs;
    failing_fs.files[apply.package_index_path] = index_before;
    failing_fs.files[apply.collision.output_path] = collision_before;
    failing_fs.failing_write_path = apply.package_index_path;
    failing_fs.failing_write_attempts_remaining = 1;

    const auto write_failed = mirakana::apply_physics_collision_package_update(failing_fs, apply);

    MK_REQUIRE(!write_failed.succeeded());
    MK_REQUIRE(failures_contain(write_failed.diagnostics, "write_collision_package_update_failed"));
    MK_REQUIRE(failing_fs.read_text(apply.collision.output_path) == collision_before);
    MK_REQUIRE(failing_fs.read_text(apply.package_index_path) == index_before);
}

MK_TEST("physics collision package tooling rejects unsafe paths and invalid body rows") {
    auto update = make_physics_collision_package_update_desc();
    update.collision.output_path = "../main.collision3d";
    const auto unsafe = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.diagnostics, "unsafe_collision_path"));

    update = make_physics_collision_package_update_desc();
    update.package_index_path = update.collision.output_path;
    const auto alias = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!alias.succeeded());
    MK_REQUIRE(failures_contain(alias.diagnostics, "package index path must not alias collision output path"));

    update = make_physics_collision_package_update_desc();
    update.collision.bodies[1].name = update.collision.bodies[0].name;
    const auto duplicate_name = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!duplicate_name.succeeded());
    MK_REQUIRE(failures_contain(duplicate_name.diagnostics, "duplicate_collision_body_name"));

    update = make_physics_collision_package_update_desc();
    update.collision.native_backend = "physx";
    const auto native_backend = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!native_backend.succeeded());
    MK_REQUIRE(failures_contain(native_backend.diagnostics, "unsupported_native_backend"));

    update = make_physics_collision_package_update_desc();
    update.collision.bodies[0].compound = "level=static";
    const auto invalid_compound = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!invalid_compound.succeeded());
    MK_REQUIRE(failures_contain(invalid_compound.diagnostics, "invalid_collision_compound"));

    update = make_physics_collision_package_update_desc();
    update.collision.bodies[0].body.half_extents = mirakana::Vec3{.x = -1.0F, .y = 0.5F, .z = 0.5F};
    const auto invalid_body = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!invalid_body.succeeded());
    MK_REQUIRE(failures_contain(invalid_body.diagnostics, "invalid_collision_body"));
}

MK_TEST("physics collision package tooling rejects inconsistent package index rows") {
    auto update = make_physics_collision_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{999},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = update.collision.output_path,
                                              .content = "other",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto path_collision = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!path_collision.succeeded());
    MK_REQUIRE(failures_contain(path_collision.diagnostics, "collision output path collides"));

    update = make_physics_collision_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = update.collision.collision_asset,
                                              .kind = mirakana::AssetKind::material,
                                              .path = "runtime/assets/physics/old.material",
                                              .content = "material",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto wrong_existing_kind = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!wrong_existing_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_existing_kind.diagnostics,
                                "existing collision package entry kind must be physics_collision_scene"));

    update = make_physics_collision_package_update_desc();
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(
        mirakana::AssetCookedPackageIndex{.entries = {mirakana::AssetCookedPackageEntry{
                                              .asset = mirakana::AssetId{999},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/physics/./bad.texture",
                                              .content_hash = 1,
                                              .source_revision = 1,
                                              .dependencies = {},
                                          }},
                                          .dependencies = {}});
    const auto unsafe_existing_path = mirakana::plan_physics_collision_package_update(update);
    MK_REQUIRE(!unsafe_existing_path.succeeded());
    MK_REQUIRE(failures_contain(unsafe_existing_path.diagnostics, "package index entry path"));
}

MK_TEST("ui atlas tooling authors deterministic cooked metadata and package rows") {
    const auto desc = make_cooked_ui_atlas_authoring_desc();
    const auto expected = expected_cooked_ui_atlas_metadata();

    const auto authored = mirakana::author_cooked_ui_atlas_metadata(desc);

    MK_REQUIRE(authored.succeeded());
    MK_REQUIRE(authored.failures.empty());
    MK_REQUIRE(authored.content == expected);
    MK_REQUIRE(authored.artifact.asset == desc.atlas_asset);
    MK_REQUIRE(authored.artifact.kind == mirakana::AssetKind::ui_atlas);
    MK_REQUIRE(authored.artifact.path == desc.output_path);
    MK_REQUIRE(authored.artifact.content == expected);
    MK_REQUIRE(authored.artifact.source_revision == 7);
    MK_REQUIRE(authored.artifact.dependencies ==
               std::vector<mirakana::AssetId>({mirakana::AssetId{10}, mirakana::AssetId{20}}));
    MK_REQUIRE(authored.dependency_edges.size() == 2);
    MK_REQUIRE(authored.dependency_edges[0].asset == desc.atlas_asset);
    MK_REQUIRE(authored.dependency_edges[0].dependency == mirakana::AssetId{10});
    MK_REQUIRE(authored.dependency_edges[0].kind == mirakana::AssetDependencyKind::ui_atlas_texture);
    MK_REQUIRE(authored.dependency_edges[0].path == desc.output_path);
    MK_REQUIRE(authored.dependency_edges[1].dependency == mirakana::AssetId{20});

    mirakana::MemoryFileSystem fs;
    const auto written = mirakana::write_cooked_ui_atlas_metadata(fs, desc);
    MK_REQUIRE(written.succeeded());
    MK_REQUIRE(fs.read_text(desc.output_path) == expected);

    const auto page_a_content = std::string{"page-a-texture"};
    const auto page_b_content = std::string{"page-b-texture"};
    auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/ui/page-a.texture.geasset",
                                          .content = page_a_content,
                                          .source_revision = 1,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                          .kind = mirakana::AssetKind::texture,
                                          .path = "runtime/assets/ui/page-b.texture.geasset",
                                          .content = page_b_content,
                                          .source_revision = 1,
                                          .dependencies = {}},
            authored.artifact,
        },
        authored.dependency_edges);

    const auto verification = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(verification.succeeded());
    MK_REQUIRE(verification.failures.empty());
}

MK_TEST("ui atlas tooling rejects duplicate bindings malformed uvs and unsupported claims") {
    auto desc = make_cooked_ui_atlas_authoring_desc();
    desc.images[1].resource_id = desc.images[0].resource_id;
    auto duplicate_resource = mirakana::author_cooked_ui_atlas_metadata(desc);
    MK_REQUIRE(!duplicate_resource.succeeded());
    MK_REQUIRE(failures_contain(duplicate_resource.failures, "resource_id is duplicated"));

    desc = make_cooked_ui_atlas_authoring_desc();
    desc.images[1].asset_uri = desc.images[0].asset_uri;
    auto duplicate_uri = mirakana::author_cooked_ui_atlas_metadata(desc);
    MK_REQUIRE(!duplicate_uri.succeeded());
    MK_REQUIRE(failures_contain(duplicate_uri.failures, "asset_uri is duplicated"));

    desc = make_cooked_ui_atlas_authoring_desc();
    desc.images[0].u0 = desc.images[0].u1;
    auto malformed_uv = mirakana::author_cooked_ui_atlas_metadata(desc);
    MK_REQUIRE(!malformed_uv.succeeded());
    MK_REQUIRE(failures_contain(malformed_uv.failures, "uv rect is invalid"));

    desc = make_cooked_ui_atlas_authoring_desc();
    desc.source_decoding = "ready";
    auto source_decode_claim = mirakana::author_cooked_ui_atlas_metadata(desc);
    MK_REQUIRE(!source_decode_claim.succeeded());
    MK_REQUIRE(failures_contain(source_decode_claim.failures, "source image decoding is not supported"));

    desc = make_cooked_ui_atlas_authoring_desc();
    desc.atlas_packing = "ready";
    auto packing_claim = mirakana::author_cooked_ui_atlas_metadata(desc);
    MK_REQUIRE(!packing_claim.succeeded());
    MK_REQUIRE(failures_contain(packing_claim.failures, "production atlas packing is not supported"));
}

MK_TEST("ui atlas tooling verifies package index kind hash and ui_atlas_texture dependency rows") {
    const auto desc = make_cooked_ui_atlas_authoring_desc();
    const auto expected = expected_cooked_ui_atlas_metadata();
    const auto hash = mirakana::hash_asset_cooked_content(expected);
    mirakana::AssetCookedPackageIndex index{
        .entries =
            {
                mirakana::AssetCookedPackageEntry{.asset = desc.atlas_asset,
                                                  .kind = mirakana::AssetKind::ui_atlas,
                                                  .path = desc.output_path,
                                                  .content_hash = hash,
                                                  .source_revision = 7,
                                                  .dependencies = {mirakana::AssetId{10}, mirakana::AssetId{20}}},
                mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{10},
                                                  .kind = mirakana::AssetKind::texture,
                                                  .path = "runtime/assets/ui/page-a.texture.geasset",
                                                  .content_hash = 10,
                                                  .source_revision = 1,
                                                  .dependencies = {}},
                mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{20},
                                                  .kind = mirakana::AssetKind::texture,
                                                  .path = "runtime/assets/ui/page-b.texture.geasset",
                                                  .content_hash = 20,
                                                  .source_revision = 1,
                                                  .dependencies = {}},
            },
        .dependencies =
            {
                mirakana::AssetDependencyEdge{.asset = desc.atlas_asset,
                                              .dependency = mirakana::AssetId{10},
                                              .kind = mirakana::AssetDependencyKind::ui_atlas_texture,
                                              .path = desc.output_path},
            },
    };

    auto missing_edge = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(!missing_edge.succeeded());
    MK_REQUIRE(failures_contain(missing_edge.failures, "ui_atlas_texture dependency edge is missing"));

    index.dependencies.push_back(mirakana::AssetDependencyEdge{.asset = desc.atlas_asset,
                                                               .dependency = mirakana::AssetId{20},
                                                               .kind = mirakana::AssetDependencyKind::ui_atlas_texture,
                                                               .path = desc.output_path});
    auto ok = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(ok.succeeded());

    index.entries[0].kind = mirakana::AssetKind::scene;
    auto wrong_kind = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(!wrong_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_kind.failures, "package entry kind must be ui_atlas"));

    index.entries[0].kind = mirakana::AssetKind::ui_atlas;
    index.entries[0].content_hash = 1;
    auto wrong_hash = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(!wrong_hash.succeeded());
    MK_REQUIRE(failures_contain(wrong_hash.failures, "content hash does not match"));
}

MK_TEST("ui atlas tooling rejects missing and non texture page package references") {
    const auto desc = make_cooked_ui_atlas_authoring_desc();
    const auto expected = expected_cooked_ui_atlas_metadata();
    const auto hash = mirakana::hash_asset_cooked_content(expected);
    mirakana::AssetCookedPackageIndex index{
        .entries =
            {
                mirakana::AssetCookedPackageEntry{.asset = desc.atlas_asset,
                                                  .kind = mirakana::AssetKind::ui_atlas,
                                                  .path = desc.output_path,
                                                  .content_hash = hash,
                                                  .source_revision = 7,
                                                  .dependencies = {mirakana::AssetId{10}, mirakana::AssetId{20}}},
                mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{10},
                                                  .kind = mirakana::AssetKind::texture,
                                                  .path = "runtime/assets/ui/page-a.texture.geasset",
                                                  .content_hash = 10,
                                                  .source_revision = 1,
                                                  .dependencies = {}},
            },
        .dependencies =
            {
                mirakana::AssetDependencyEdge{.asset = desc.atlas_asset,
                                              .dependency = mirakana::AssetId{10},
                                              .kind = mirakana::AssetDependencyKind::ui_atlas_texture,
                                              .path = desc.output_path},
                mirakana::AssetDependencyEdge{.asset = desc.atlas_asset,
                                              .dependency = mirakana::AssetId{20},
                                              .kind = mirakana::AssetDependencyKind::ui_atlas_texture,
                                              .path = desc.output_path},
            },
    };

    auto missing_page = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(!missing_page.succeeded());
    MK_REQUIRE(failures_contain(missing_page.failures, "texture page package entry is missing"));

    index.entries.push_back(mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{20},
                                                              .kind = mirakana::AssetKind::material,
                                                              .path = "runtime/assets/ui/page-b.texture.geasset",
                                                              .content_hash = 20,
                                                              .source_revision = 1,
                                                              .dependencies = {}});
    auto non_texture_page = mirakana::verify_cooked_ui_atlas_package_metadata(index, desc.atlas_asset, expected);
    MK_REQUIRE(!non_texture_page.succeeded());
    MK_REQUIRE(failures_contain(non_texture_page.failures, "texture page package entry is not a texture"));
}

MK_TEST("ui atlas apply tooling dry-runs atlas and package index changes") {
    const auto desc = make_cooked_ui_atlas_authoring_desc();
    mirakana::CookedUiAtlasPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = ui_atlas_base_package_index_content();
    update.atlas = desc;

    const auto result = mirakana::plan_cooked_ui_atlas_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.atlas_content == expected_cooked_ui_atlas_metadata());
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == desc.output_path);
    MK_REQUIRE(result.changed_files[0].content == expected_cooked_ui_atlas_metadata());
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto verification =
        mirakana::verify_cooked_ui_atlas_package_metadata(updated_index, desc.atlas_asset, result.atlas_content);
    MK_REQUIRE(verification.succeeded());
}

MK_TEST("ui atlas apply tooling applies only after validation passes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/sample.geindex", ui_atlas_base_package_index_content());

    mirakana::CookedUiAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/sample.geindex";
    apply.atlas = make_cooked_ui_atlas_authoring_desc();

    const auto result = mirakana::apply_cooked_ui_atlas_package_update(fs, apply);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text(apply.atlas.output_path) == expected_cooked_ui_atlas_metadata());
    MK_REQUIRE(fs.read_text(apply.package_index_path) == result.package_index_content);

    auto invalid = apply;
    invalid.atlas.images[0].u0 = invalid.atlas.images[0].u1;
    const auto atlas_before = fs.read_text(apply.atlas.output_path);
    const auto index_before = fs.read_text(apply.package_index_path);
    const auto failed = mirakana::apply_cooked_ui_atlas_package_update(fs, invalid);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.failures, "uv rect is invalid"));
    MK_REQUIRE(fs.read_text(apply.atlas.output_path) == atlas_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);
}

MK_TEST("ui atlas apply tooling rejects unsafe paths and inconsistent page rows") {
    mirakana::CookedUiAtlasPackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = ui_atlas_base_package_index_content();
    update.atlas = make_cooked_ui_atlas_authoring_desc();
    update.atlas.output_path = "../hud.uiatlas";

    const auto unsafe = mirakana::plan_cooked_ui_atlas_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.failures, "package-relative"));

    update.atlas = make_cooked_ui_atlas_authoring_desc();
    auto index = mirakana::deserialize_asset_cooked_package_index(ui_atlas_base_package_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{10}) {
            entry.path = "runtime/assets/ui/wrong.texture.geasset";
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);

    const auto inconsistent = mirakana::plan_cooked_ui_atlas_package_update(update);
    MK_REQUIRE(!inconsistent.succeeded());
    MK_REQUIRE(failures_contain(inconsistent.failures, "does not match atlas page asset_uri"));
}

MK_TEST("packed runtime UI atlas authoring maps decoded images into texture page and metadata") {
    const auto desc = make_packed_ui_atlas_authoring_desc();

    const auto result = mirakana::author_packed_ui_atlas_from_decoded_images(desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.atlas_texture.width == 2);
    MK_REQUIRE(result.atlas_texture.height == 1);
    MK_REQUIRE(result.atlas_texture.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
    MK_REQUIRE(result.atlas_texture.bytes ==
               std::vector<std::uint8_t>({0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF}));
    MK_REQUIRE(result.atlas_texture_content.find("format=GameEngine.CookedTexture.v1\n") == 0);
    MK_REQUIRE(result.atlas_texture_content.find("texture.width=2\n") != std::string::npos);
    MK_REQUIRE(result.atlas_texture_content.find("texture.height=1\n") != std::string::npos);
    MK_REQUIRE(result.atlas_texture_content.find("texture.data_hex=ff0000ff0000ffff\n") != std::string::npos);

    const auto metadata = mirakana::deserialize_ui_atlas_metadata_document(result.atlas_metadata_content);
    MK_REQUIRE(metadata.asset == desc.atlas_asset);
    MK_REQUIRE(metadata.source_decoding == "decoded-image-adapter");
    MK_REQUIRE(metadata.atlas_packing == "deterministic-sprite-atlas-rgba8-max-side");
    MK_REQUIRE(metadata.pages.size() == 1);
    MK_REQUIRE(metadata.pages[0].asset == desc.atlas_page_asset);
    MK_REQUIRE(metadata.pages[0].asset_uri == desc.atlas_page_output_path);
    MK_REQUIRE(metadata.images.size() == 2);
    MK_REQUIRE(metadata.images[0].resource_id == "hud/background");
    MK_REQUIRE(metadata.images[0].uv.u0 == 0.5F);
    MK_REQUIRE(metadata.images[0].uv.v0 == 0.0F);
    MK_REQUIRE(metadata.images[0].uv.u1 == 1.0F);
    MK_REQUIRE(metadata.images[0].uv.v1 == 1.0F);
    MK_REQUIRE(metadata.images[1].resource_id == "hud/icon");
    MK_REQUIRE(metadata.images[1].uv.u0 == 0.0F);
    MK_REQUIRE(metadata.images[1].uv.v0 == 0.0F);
    MK_REQUIRE(metadata.images[1].uv.u1 == 0.5F);
    MK_REQUIRE(metadata.images[1].uv.v1 == 1.0F);
}

MK_TEST("packed runtime UI atlas package update writes texture page metadata and package index") {
    mirakana::PackedUiAtlasPackageUpdateDesc update;
    update.package_index_path = "runtime/generated.geindex";
    update.package_index_content = empty_package_index_content();
    update.atlas = make_packed_ui_atlas_authoring_desc();

    const auto result = mirakana::plan_packed_ui_atlas_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.changed_files.size() == 3);
    MK_REQUIRE(result.changed_files[0].path == update.atlas.atlas_page_output_path);
    MK_REQUIRE(result.changed_files[0].content == result.atlas_texture_content);
    MK_REQUIRE(result.changed_files[1].path == update.atlas.atlas_metadata_output_path);
    MK_REQUIRE(result.changed_files[1].content == result.atlas_metadata_content);
    MK_REQUIRE(result.changed_files[2].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[2].content == result.package_index_content);

    const auto index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto verification = mirakana::verify_cooked_ui_atlas_package_metadata(index, update.atlas.atlas_asset,
                                                                                result.atlas_metadata_content);
    MK_REQUIRE(verification.succeeded());
    const auto page_entry = std::ranges::find_if(index.entries, [&](const mirakana::AssetCookedPackageEntry& entry) {
        return entry.asset == update.atlas.atlas_page_asset;
    });
    const auto atlas_entry = std::ranges::find_if(index.entries, [&](const mirakana::AssetCookedPackageEntry& entry) {
        return entry.asset == update.atlas.atlas_asset;
    });
    MK_REQUIRE(page_entry != index.entries.end());
    MK_REQUIRE(page_entry->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(page_entry->path == update.atlas.atlas_page_output_path);
    MK_REQUIRE(atlas_entry != index.entries.end());
    MK_REQUIRE(atlas_entry->kind == mirakana::AssetKind::ui_atlas);
    MK_REQUIRE(atlas_entry->path == update.atlas.atlas_metadata_output_path);
}

MK_TEST("packed runtime UI atlas rejects invalid decoded images and package path collisions") {
    auto invalid = make_packed_ui_atlas_authoring_desc();
    invalid.images[0].decoded_image.pixel_format = mirakana::ui::ImageDecodePixelFormat::unknown;
    const auto invalid_format = mirakana::author_packed_ui_atlas_from_decoded_images(invalid);
    MK_REQUIRE(!invalid_format.succeeded());
    MK_REQUIRE(failures_contain(invalid_format.failures, "decoded image must be RGBA8"));

    invalid = make_packed_ui_atlas_authoring_desc();
    invalid.images[0].decoded_image.pixels.pop_back();
    const auto invalid_bytes = mirakana::author_packed_ui_atlas_from_decoded_images(invalid);
    MK_REQUIRE(!invalid_bytes.succeeded());
    MK_REQUIRE(failures_contain(invalid_bytes.failures, "decoded image pixel byte count"));

    mirakana::PackedUiAtlasPackageUpdateDesc collision;
    collision.package_index_path = "runtime/generated.geindex";
    collision.atlas = make_packed_ui_atlas_authoring_desc();
    collision.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{999},
                                           .kind = mirakana::AssetKind::texture,
                                           .path = collision.atlas.atlas_page_output_path,
                                           .content = "existing",
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {}));
    const auto collision_result = mirakana::plan_packed_ui_atlas_package_update(collision);
    MK_REQUIRE(!collision_result.succeeded());
    MK_REQUIRE(failures_contain(collision_result.failures, "ui atlas page output path collides"));

    mirakana::PackedUiAtlasPackageUpdateDesc noncanonical_existing_path;
    noncanonical_existing_path.package_index_path = "runtime/generated.geindex";
    noncanonical_existing_path.atlas = make_packed_ui_atlas_authoring_desc();
    noncanonical_existing_path.package_index_content = mirakana::serialize_asset_cooked_package_index(
        mirakana::AssetCookedPackageIndex{.entries = {mirakana::AssetCookedPackageEntry{
                                              .asset = mirakana::AssetId{999},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/ui/./generated_hud_page.texture",
                                              .content_hash = 1,
                                              .source_revision = 1,
                                              .dependencies = {},
                                          }},
                                          .dependencies = {}});
    const auto noncanonical_existing_path_result =
        mirakana::plan_packed_ui_atlas_package_update(noncanonical_existing_path);
    MK_REQUIRE(!noncanonical_existing_path_result.succeeded());
    MK_REQUIRE(failures_contain(noncanonical_existing_path_result.failures,
                                "ui atlas package index entry path must be a package-relative safe path"));

    mirakana::PackedUiAtlasPackageUpdateDesc package_index_page_alias;
    package_index_page_alias.package_index_path = "runtime/assets/ui/generated_hud_page.texture";
    package_index_page_alias.package_index_content = empty_package_index_content();
    package_index_page_alias.atlas = make_packed_ui_atlas_authoring_desc();
    const auto package_index_page_alias_result =
        mirakana::plan_packed_ui_atlas_package_update(package_index_page_alias);
    MK_REQUIRE(!package_index_page_alias_result.succeeded());
    MK_REQUIRE(failures_contain(package_index_page_alias_result.failures,
                                "ui atlas package index path must not alias page output path"));

    mirakana::PackedUiAtlasPackageUpdateDesc package_index_metadata_alias;
    package_index_metadata_alias.package_index_path = "runtime/assets/ui/generated_hud.uiatlas";
    package_index_metadata_alias.package_index_content = empty_package_index_content();
    package_index_metadata_alias.atlas = make_packed_ui_atlas_authoring_desc();
    const auto package_index_metadata_alias_result =
        mirakana::plan_packed_ui_atlas_package_update(package_index_metadata_alias);
    MK_REQUIRE(!package_index_metadata_alias_result.succeeded());
    MK_REQUIRE(failures_contain(package_index_metadata_alias_result.failures,
                                "ui atlas package index path must not alias metadata output path"));

    mirakana::PackedUiAtlasPackageUpdateDesc dot_segment_alias;
    dot_segment_alias.package_index_path = "runtime/assets/ui/./generated_hud.uiatlas";
    dot_segment_alias.package_index_content = empty_package_index_content();
    dot_segment_alias.atlas = make_packed_ui_atlas_authoring_desc();
    const auto dot_segment_alias_result = mirakana::plan_packed_ui_atlas_package_update(dot_segment_alias);
    MK_REQUIRE(!dot_segment_alias_result.succeeded());
    MK_REQUIRE(failures_contain(dot_segment_alias_result.failures, "package-relative safe path"));

    mirakana::PackedUiAtlasPackageUpdateDesc empty_segment_alias;
    empty_segment_alias.package_index_path = "runtime/assets/ui//generated_hud.uiatlas";
    empty_segment_alias.package_index_content = empty_package_index_content();
    empty_segment_alias.atlas = make_packed_ui_atlas_authoring_desc();
    const auto empty_segment_alias_result = mirakana::plan_packed_ui_atlas_package_update(empty_segment_alias);
    MK_REQUIRE(!empty_segment_alias_result.succeeded());
    MK_REQUIRE(failures_contain(empty_segment_alias_result.failures, "package-relative safe path"));

    auto source_decode_claim = make_packed_ui_atlas_authoring_desc();
    source_decode_claim.source_decoding = "rasterized-glyph-adapter";
    const auto source_decode_claim_result = mirakana::author_packed_ui_atlas_from_decoded_images(source_decode_claim);
    MK_REQUIRE(!source_decode_claim_result.succeeded());
    MK_REQUIRE(failures_contain(source_decode_claim_result.failures,
                                "ui atlas source decoding must be decoded-image-adapter"));

    auto packing_claim = make_packed_ui_atlas_authoring_desc();
    packing_claim.atlas_packing = "deterministic-glyph-atlas-rgba8-max-side";
    const auto packing_claim_result = mirakana::author_packed_ui_atlas_from_decoded_images(packing_claim);
    MK_REQUIRE(!packing_claim_result.succeeded());
    MK_REQUIRE(failures_contain(packing_claim_result.failures,
                                "ui atlas packing must be deterministic-sprite-atlas-rgba8-max-side"));
}

MK_TEST("packed runtime UI atlas apply leaves existing files unchanged when validation fails") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/generated.geindex", empty_package_index_content());
    fs.write_text("runtime/assets/ui/generated_hud_page.texture", "old texture");
    fs.write_text("runtime/assets/ui/generated_hud.uiatlas", "old metadata");

    mirakana::PackedUiAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_atlas_authoring_desc();
    apply.atlas.images[0].decoded_image.pixels.clear();

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto before_texture = fs.read_text(apply.atlas.atlas_page_output_path);
    const auto before_metadata = fs.read_text(apply.atlas.atlas_metadata_output_path);
    const auto result = mirakana::apply_packed_ui_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "decoded image pixel byte count"));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_page_output_path) == before_texture);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_metadata_output_path) == before_metadata);
}

MK_TEST("packed runtime UI atlas apply removes new files and directories when a write fails") {
    ThrowingWriteFileSystem fs;
    fs.files["runtime/generated.geindex"] = empty_package_index_content();

    mirakana::PackedUiAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_atlas_authoring_desc();
    fs.failing_write_path = apply.atlas.atlas_metadata_output_path;

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto result = mirakana::apply_packed_ui_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed writing " + apply.atlas.atlas_metadata_output_path));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(!fs.exists(apply.atlas.atlas_page_output_path));
    MK_REQUIRE(!fs.exists(apply.atlas.atlas_metadata_output_path));
    MK_REQUIRE(std::ranges::find(fs.removed_directories, "runtime/assets/ui") != fs.removed_directories.end());
}

MK_TEST("packed runtime UI atlas rooted apply keeps regular parent files when nested write fails") {
    const auto root = std::filesystem::current_path() / "ge-packed-ui-atlas-regular-parent-rollback-test";
    std::filesystem::remove_all(root);

    mirakana::RootedFileSystem fs(root);
    fs.write_text("runtime/generated.geindex", empty_package_index_content());
    fs.write_text("runtime/assets", "not a directory");

    mirakana::PackedUiAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_atlas_authoring_desc();

    const auto result = mirakana::apply_packed_ui_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed writing " + apply.atlas.atlas_page_output_path));
    MK_REQUIRE(fs.exists("runtime/assets"));
    MK_REQUIRE(fs.read_text("runtime/assets") == "not a directory");

    std::filesystem::remove_all(root);
}

MK_TEST("packed runtime UI atlas apply rolls back files when a write fails") {
    ThrowingWriteFileSystem fs;
    fs.files["runtime/generated.geindex"] = empty_package_index_content();
    fs.files["runtime/assets/ui/generated_hud_page.texture"] = "old texture";
    fs.files["runtime/assets/ui/generated_hud.uiatlas"] = "old metadata";

    mirakana::PackedUiAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_atlas_authoring_desc();
    fs.failing_write_path = apply.atlas.atlas_metadata_output_path;

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto before_texture = fs.read_text(apply.atlas.atlas_page_output_path);
    const auto before_metadata = fs.read_text(apply.atlas.atlas_metadata_output_path);
    const auto result = mirakana::apply_packed_ui_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed to write packed ui atlas package update"));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_page_output_path) == before_texture);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_metadata_output_path) == before_metadata);
}

MK_TEST("packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata") {
    const auto desc = make_packed_ui_glyph_atlas_authoring_desc();

    const auto result = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.atlas_texture.width == 2);
    MK_REQUIRE(result.atlas_texture.height == 1);
    MK_REQUIRE(result.atlas_texture.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
    MK_REQUIRE(result.atlas_texture.bytes ==
               std::vector<std::uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF}));
    MK_REQUIRE(result.atlas_texture_content.find("format=GameEngine.CookedTexture.v1\n") == 0);
    MK_REQUIRE(result.atlas_texture_content.find("texture.width=2\n") != std::string::npos);
    MK_REQUIRE(result.atlas_texture_content.find("texture.height=1\n") != std::string::npos);
    MK_REQUIRE(result.atlas_texture_content.find("texture.data_hex=ffffffff0000ffff\n") != std::string::npos);

    const auto metadata = mirakana::deserialize_ui_atlas_metadata_document(result.atlas_metadata_content);
    MK_REQUIRE(metadata.asset == desc.atlas_asset);
    MK_REQUIRE(metadata.source_decoding == "rasterized-glyph-adapter");
    MK_REQUIRE(metadata.atlas_packing == "deterministic-glyph-atlas-rgba8-max-side");
    MK_REQUIRE(metadata.pages.size() == 1);
    MK_REQUIRE(metadata.pages[0].asset == desc.atlas_page_asset);
    MK_REQUIRE(metadata.pages[0].asset_uri == desc.atlas_page_output_path);
    MK_REQUIRE(metadata.images.empty());
    MK_REQUIRE(metadata.glyphs.size() == 2);
    MK_REQUIRE(metadata.glyphs[0].font_family == "ui/body");
    MK_REQUIRE(metadata.glyphs[0].glyph == 65);
    MK_REQUIRE(metadata.glyphs[0].uv.u0 == 0.0F);
    MK_REQUIRE(metadata.glyphs[0].uv.v0 == 0.0F);
    MK_REQUIRE(metadata.glyphs[0].uv.u1 == 0.5F);
    MK_REQUIRE(metadata.glyphs[0].uv.v1 == 1.0F);
    MK_REQUIRE(metadata.glyphs[1].glyph == 66);
    MK_REQUIRE(metadata.glyphs[1].uv.u0 == 0.5F);
    MK_REQUIRE(metadata.glyphs[1].uv.v0 == 0.0F);
    MK_REQUIRE(metadata.glyphs[1].uv.u1 == 1.0F);
    MK_REQUIRE(metadata.glyphs[1].uv.v1 == 1.0F);
}

MK_TEST("packed runtime UI glyph atlas authoring is deterministic across glyph input order") {
    auto canonical = make_packed_ui_glyph_atlas_authoring_desc();
    auto reversed = canonical;
    std::ranges::reverse(reversed.glyphs);

    const auto canonical_result = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(canonical);
    const auto reversed_result = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(reversed);

    MK_REQUIRE(canonical_result.succeeded());
    MK_REQUIRE(reversed_result.succeeded());
    MK_REQUIRE(canonical_result.atlas_texture_content == reversed_result.atlas_texture_content);
    MK_REQUIRE(canonical_result.atlas_metadata_content == reversed_result.atlas_metadata_content);
}

MK_TEST("packed runtime UI glyph atlas package update writes texture page metadata and package index") {
    mirakana::PackedUiGlyphAtlasPackageUpdateDesc update;
    update.package_index_path = "runtime/generated.geindex";
    update.package_index_content = empty_package_index_content();
    update.atlas = make_packed_ui_glyph_atlas_authoring_desc();

    const auto result = mirakana::plan_packed_ui_glyph_atlas_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.changed_files.size() == 3);
    MK_REQUIRE(result.changed_files[0].path == update.atlas.atlas_page_output_path);
    MK_REQUIRE(result.changed_files[0].content == result.atlas_texture_content);
    MK_REQUIRE(result.changed_files[1].path == update.atlas.atlas_metadata_output_path);
    MK_REQUIRE(result.changed_files[1].content == result.atlas_metadata_content);
    MK_REQUIRE(result.changed_files[2].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[2].content == result.package_index_content);

    const auto index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto verification = mirakana::verify_cooked_ui_atlas_package_metadata(index, update.atlas.atlas_asset,
                                                                                result.atlas_metadata_content);
    MK_REQUIRE(verification.succeeded());
    const auto page_entry = std::ranges::find_if(index.entries, [&](const mirakana::AssetCookedPackageEntry& entry) {
        return entry.asset == update.atlas.atlas_page_asset;
    });
    const auto atlas_entry = std::ranges::find_if(index.entries, [&](const mirakana::AssetCookedPackageEntry& entry) {
        return entry.asset == update.atlas.atlas_asset;
    });
    MK_REQUIRE(page_entry != index.entries.end());
    MK_REQUIRE(page_entry->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(page_entry->path == update.atlas.atlas_page_output_path);
    MK_REQUIRE(atlas_entry != index.entries.end());
    MK_REQUIRE(atlas_entry->kind == mirakana::AssetKind::ui_atlas);
    MK_REQUIRE(atlas_entry->path == update.atlas.atlas_metadata_output_path);
}

MK_TEST("packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions") {
    auto invalid = make_packed_ui_glyph_atlas_authoring_desc();
    invalid.glyphs[0].rasterized_glyph.pixel_format = mirakana::ui::ImageDecodePixelFormat::unknown;
    const auto invalid_format = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(invalid);
    MK_REQUIRE(!invalid_format.succeeded());
    MK_REQUIRE(failures_contain(invalid_format.failures, "rasterized glyph must be RGBA8"));

    invalid = make_packed_ui_glyph_atlas_authoring_desc();
    invalid.glyphs[0].rasterized_glyph.pixels.pop_back();
    const auto invalid_bytes = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(invalid);
    MK_REQUIRE(!invalid_bytes.succeeded());
    MK_REQUIRE(failures_contain(invalid_bytes.failures, "rasterized glyph pixel byte count"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc collision;
    collision.package_index_path = "runtime/generated.geindex";
    collision.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    collision.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{999},
                                           .kind = mirakana::AssetKind::texture,
                                           .path = collision.atlas.atlas_page_output_path,
                                           .content = "existing",
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {}));
    const auto collision_result = mirakana::plan_packed_ui_glyph_atlas_package_update(collision);
    MK_REQUIRE(!collision_result.succeeded());
    MK_REQUIRE(failures_contain(collision_result.failures, "ui glyph atlas page output path collides"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc noncanonical_existing_path;
    noncanonical_existing_path.package_index_path = "runtime/generated.geindex";
    noncanonical_existing_path.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    noncanonical_existing_path.package_index_content = mirakana::serialize_asset_cooked_package_index(
        mirakana::AssetCookedPackageIndex{.entries = {mirakana::AssetCookedPackageEntry{
                                              .asset = mirakana::AssetId{999},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/ui/./generated_body_glyphs.texture",
                                              .content_hash = 1,
                                              .source_revision = 1,
                                              .dependencies = {},
                                          }},
                                          .dependencies = {}});
    const auto noncanonical_existing_path_result =
        mirakana::plan_packed_ui_glyph_atlas_package_update(noncanonical_existing_path);
    MK_REQUIRE(!noncanonical_existing_path_result.succeeded());
    MK_REQUIRE(failures_contain(noncanonical_existing_path_result.failures,
                                "ui atlas package index entry path must be a package-relative safe path"));

    invalid = make_packed_ui_glyph_atlas_authoring_desc();
    invalid.glyphs[0].font_family.clear();
    const auto invalid_identity = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(invalid);
    MK_REQUIRE(!invalid_identity.succeeded());
    MK_REQUIRE(failures_contain(invalid_identity.failures, "rasterized glyph identity is missing"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc package_index_page_alias;
    package_index_page_alias.package_index_path = "runtime/assets/ui/generated_body_glyphs.texture";
    package_index_page_alias.package_index_content = empty_package_index_content();
    package_index_page_alias.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    const auto package_index_page_alias_result =
        mirakana::plan_packed_ui_glyph_atlas_package_update(package_index_page_alias);
    MK_REQUIRE(!package_index_page_alias_result.succeeded());
    MK_REQUIRE(failures_contain(package_index_page_alias_result.failures,
                                "ui glyph atlas package index path must not alias page output path"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc package_index_metadata_alias;
    package_index_metadata_alias.package_index_path = "runtime/assets/ui/generated_body_glyphs.uiatlas";
    package_index_metadata_alias.package_index_content = empty_package_index_content();
    package_index_metadata_alias.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    const auto package_index_metadata_alias_result =
        mirakana::plan_packed_ui_glyph_atlas_package_update(package_index_metadata_alias);
    MK_REQUIRE(!package_index_metadata_alias_result.succeeded());
    MK_REQUIRE(failures_contain(package_index_metadata_alias_result.failures,
                                "ui glyph atlas package index path must not alias metadata output path"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc dot_segment_alias;
    dot_segment_alias.package_index_path = "runtime/assets/ui/./generated_body_glyphs.uiatlas";
    dot_segment_alias.package_index_content = empty_package_index_content();
    dot_segment_alias.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    const auto dot_segment_alias_result = mirakana::plan_packed_ui_glyph_atlas_package_update(dot_segment_alias);
    MK_REQUIRE(!dot_segment_alias_result.succeeded());
    MK_REQUIRE(failures_contain(dot_segment_alias_result.failures, "package-relative safe path"));

    mirakana::PackedUiGlyphAtlasPackageUpdateDesc empty_segment_alias;
    empty_segment_alias.package_index_path = "runtime/assets/ui//generated_body_glyphs.uiatlas";
    empty_segment_alias.package_index_content = empty_package_index_content();
    empty_segment_alias.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    const auto empty_segment_alias_result = mirakana::plan_packed_ui_glyph_atlas_package_update(empty_segment_alias);
    MK_REQUIRE(!empty_segment_alias_result.succeeded());
    MK_REQUIRE(failures_contain(empty_segment_alias_result.failures, "package-relative safe path"));

    auto source_decode_claim = make_packed_ui_glyph_atlas_authoring_desc();
    source_decode_claim.source_decoding = "decoded-image-adapter";
    const auto source_decode_claim_result =
        mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(source_decode_claim);
    MK_REQUIRE(!source_decode_claim_result.succeeded());
    MK_REQUIRE(failures_contain(source_decode_claim_result.failures,
                                "ui glyph atlas source decoding must be rasterized-glyph-adapter"));

    auto packing_claim = make_packed_ui_glyph_atlas_authoring_desc();
    packing_claim.atlas_packing = "deterministic-sprite-atlas-rgba8-max-side";
    const auto packing_claim_result = mirakana::author_packed_ui_glyph_atlas_from_rasterized_glyphs(packing_claim);
    MK_REQUIRE(!packing_claim_result.succeeded());
    MK_REQUIRE(failures_contain(packing_claim_result.failures,
                                "ui glyph atlas packing must be deterministic-glyph-atlas-rgba8-max-side"));
}

MK_TEST("packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/generated.geindex", empty_package_index_content());
    fs.write_text("runtime/assets/ui/generated_body_glyphs.texture", "old texture");
    fs.write_text("runtime/assets/ui/generated_body_glyphs.uiatlas", "old metadata");

    mirakana::PackedUiGlyphAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    apply.atlas.glyphs[0].rasterized_glyph.pixels.clear();

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto before_texture = fs.read_text(apply.atlas.atlas_page_output_path);
    const auto before_metadata = fs.read_text(apply.atlas.atlas_metadata_output_path);
    const auto result = mirakana::apply_packed_ui_glyph_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "rasterized glyph pixel byte count"));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_page_output_path) == before_texture);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_metadata_output_path) == before_metadata);
}

MK_TEST("packed runtime UI glyph atlas apply removes new files and directories when a write fails") {
    ThrowingWriteFileSystem fs;
    fs.files["runtime/generated.geindex"] = empty_package_index_content();

    mirakana::PackedUiGlyphAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    fs.failing_write_path = apply.atlas.atlas_metadata_output_path;

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto result = mirakana::apply_packed_ui_glyph_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed writing " + apply.atlas.atlas_metadata_output_path));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(!fs.exists(apply.atlas.atlas_page_output_path));
    MK_REQUIRE(!fs.exists(apply.atlas.atlas_metadata_output_path));
    MK_REQUIRE(std::ranges::find(fs.removed_directories, "runtime/assets/ui") != fs.removed_directories.end());
}

MK_TEST("packed runtime UI glyph atlas rooted apply keeps regular parent files when nested write fails") {
    const auto root = std::filesystem::current_path() / "ge-packed-ui-glyph-atlas-regular-parent-rollback-test";
    std::filesystem::remove_all(root);

    mirakana::RootedFileSystem fs(root);
    fs.write_text("runtime/generated.geindex", empty_package_index_content());
    fs.write_text("runtime/assets", "not a directory");

    mirakana::PackedUiGlyphAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_glyph_atlas_authoring_desc();

    const auto result = mirakana::apply_packed_ui_glyph_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed writing " + apply.atlas.atlas_page_output_path));
    MK_REQUIRE(fs.exists("runtime/assets"));
    MK_REQUIRE(fs.read_text("runtime/assets") == "not a directory");

    std::filesystem::remove_all(root);
}

MK_TEST("packed runtime UI glyph atlas apply rolls back files when a write fails") {
    ThrowingWriteFileSystem fs;
    fs.files["runtime/generated.geindex"] = empty_package_index_content();
    fs.files["runtime/assets/ui/generated_body_glyphs.texture"] = "old texture";
    fs.files["runtime/assets/ui/generated_body_glyphs.uiatlas"] = "old metadata";

    mirakana::PackedUiGlyphAtlasPackageApplyDesc apply;
    apply.package_index_path = "runtime/generated.geindex";
    apply.atlas = make_packed_ui_glyph_atlas_authoring_desc();
    fs.failing_write_path = apply.atlas.atlas_metadata_output_path;

    const auto before_index = fs.read_text(apply.package_index_path);
    const auto before_texture = fs.read_text(apply.atlas.atlas_page_output_path);
    const auto before_metadata = fs.read_text(apply.atlas.atlas_metadata_output_path);
    const auto result = mirakana::apply_packed_ui_glyph_atlas_package_update(fs, apply);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.failures, "failed to write packed ui glyph atlas package update"));
    MK_REQUIRE(fs.read_text(apply.package_index_path) == before_index);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_page_output_path) == before_texture);
    MK_REQUIRE(fs.read_text(apply.atlas.atlas_metadata_output_path) == before_metadata);
}

MK_TEST("scene prefab authoring dry-runs create scene add node and component changes") {
    mirakana::ScenePrefabAuthoringRequest create;
    create.kind = mirakana::ScenePrefabAuthoringCommandKind::create_scene;
    create.scene_path = "source/scenes/level.scene";
    create.scene = make_scene_prefab_authoring_scene_v2();

    const auto created = mirakana::plan_scene_prefab_authoring(create);

    MK_REQUIRE(created.succeeded());
    MK_REQUIRE(created.diagnostics.empty());
    MK_REQUIRE(created.scene_content == mirakana::serialize_scene_document_v2(create.scene));
    MK_REQUIRE(created.changed_files.size() == 1);
    MK_REQUIRE(created.changed_files[0].path == create.scene_path);
    MK_REQUIRE(created.changed_files[0].content == created.scene_content);
    MK_REQUIRE(created.model_mutations.size() == 1);
    MK_REQUIRE(created.model_mutations[0].kind == "create_scene");

    mirakana::ScenePrefabAuthoringRequest add_node;
    add_node.kind = mirakana::ScenePrefabAuthoringCommandKind::add_node;
    add_node.scene_path = create.scene_path;
    add_node.scene_content = created.scene_content;
    add_node.node_id = mirakana::AuthoringId{"node/player"};
    add_node.node_name = "Player";
    add_node.parent_id = mirakana::AuthoringId{"node/root"};
    add_node.node_transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};

    const auto node_added = mirakana::plan_scene_prefab_authoring(add_node);

    MK_REQUIRE(node_added.succeeded());
    MK_REQUIRE(text_contains(node_added.scene_content, "node.1.id=node/player\n"));
    MK_REQUIRE(text_contains(node_added.scene_content, "node.1.parent=node/root\n"));
    MK_REQUIRE(text_contains(node_added.scene_content, "node.1.position=1 2 3\n"));
    MK_REQUIRE(node_added.model_mutations[0].kind == "add_node");

    mirakana::ScenePrefabAuthoringRequest add_component;
    add_component.kind = mirakana::ScenePrefabAuthoringCommandKind::add_or_update_component;
    add_component.scene_path = create.scene_path;
    add_component.scene_content = node_added.scene_content;
    add_component.component_id = mirakana::AuthoringId{"component/player/mesh"};
    add_component.component_node_id = mirakana::AuthoringId{"node/player"};
    add_component.component_type = mirakana::SceneComponentTypeId{"mesh_renderer"};
    add_component.component_properties = {{.name = "mesh", .value = "assets/meshes/player"},
                                          {.name = "material", .value = "assets/materials/player"}};

    const auto component_added = mirakana::plan_scene_prefab_authoring(add_component);

    MK_REQUIRE(component_added.succeeded());
    MK_REQUIRE(text_contains(component_added.scene_content, "component.0.id=component/player/mesh\n"));
    MK_REQUIRE(text_contains(component_added.scene_content, "component.0.type=mesh_renderer\n"));
    MK_REQUIRE(text_contains(component_added.scene_content, "component.0.property.0.name=material\n"));
    MK_REQUIRE(text_contains(component_added.scene_content, "component.0.property.1.name=mesh\n"));
    MK_REQUIRE(component_added.model_mutations[0].kind == "add_or_update_component");
}

MK_TEST("scene prefab authoring dry-runs create prefab and instantiate prefab") {
    mirakana::ScenePrefabAuthoringRequest create_prefab;
    create_prefab.kind = mirakana::ScenePrefabAuthoringCommandKind::create_prefab;
    create_prefab.prefab_path = "source/prefabs/enemy.prefab";
    create_prefab.prefab = make_scene_prefab_authoring_prefab_v2();

    const auto prefab_created = mirakana::plan_scene_prefab_authoring(create_prefab);

    MK_REQUIRE(prefab_created.succeeded());
    MK_REQUIRE(prefab_created.prefab_content == mirakana::serialize_prefab_document_v2(create_prefab.prefab));
    MK_REQUIRE(prefab_created.changed_files.size() == 1);
    MK_REQUIRE(prefab_created.changed_files[0].path == create_prefab.prefab_path);
    MK_REQUIRE(prefab_created.model_mutations[0].kind == "create_prefab");

    mirakana::ScenePrefabAuthoringRequest instantiate;
    instantiate.kind = mirakana::ScenePrefabAuthoringCommandKind::instantiate_prefab;
    instantiate.scene_path = "source/scenes/level.scene";
    instantiate.scene_content = mirakana::serialize_scene_document_v2(make_scene_prefab_authoring_scene_v2());
    instantiate.prefab_path = create_prefab.prefab_path;
    instantiate.prefab_content = prefab_created.prefab_content;
    instantiate.parent_id = mirakana::AuthoringId{"node/root"};
    instantiate.instance_id_prefix = "enemy/001/";
    instantiate.instance_name_prefix = "Enemy 001 ";
    instantiate.node_transform.position = mirakana::Vec3{.x = 9.0F, .y = 8.0F, .z = 7.0F};

    const auto instantiated = mirakana::plan_scene_prefab_authoring(instantiate);

    MK_REQUIRE(instantiated.succeeded());
    MK_REQUIRE(text_contains(instantiated.scene_content, "node.1.id=enemy/001/node/enemy-root\n"));
    MK_REQUIRE(text_contains(instantiated.scene_content, "node.1.name=Enemy 001 EnemyRoot\n"));
    MK_REQUIRE(text_contains(instantiated.scene_content, "node.1.parent=node/root\n"));
    MK_REQUIRE(text_contains(instantiated.scene_content, "node.1.position=9 8 7\n"));
    MK_REQUIRE(text_contains(instantiated.scene_content, "component.0.id=enemy/001/component/enemy/mesh\n"));
    MK_REQUIRE(text_contains(instantiated.scene_content, "component.0.node=enemy/001/node/enemy-root\n"));
    MK_REQUIRE(instantiated.model_mutations[0].kind == "instantiate_prefab");
}

MK_TEST("scene prefab authoring apply writes only validated deterministic changes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("source/scenes/level.scene",
                  mirakana::serialize_scene_document_v2(make_scene_prefab_authoring_scene_v2()));

    mirakana::ScenePrefabAuthoringRequest add_node;
    add_node.kind = mirakana::ScenePrefabAuthoringCommandKind::add_node;
    add_node.scene_path = "source/scenes/level.scene";
    add_node.node_id = mirakana::AuthoringId{"node/player"};
    add_node.node_name = "Player";
    add_node.parent_id = mirakana::AuthoringId{"node/root"};

    const auto applied = mirakana::apply_scene_prefab_authoring(fs, add_node);

    MK_REQUIRE(applied.succeeded());
    MK_REQUIRE(applied.changed_files.size() == 1);
    MK_REQUIRE(applied.changed_files[0].path == add_node.scene_path);
    MK_REQUIRE(applied.changed_files[0].content_hash == mirakana::hash_asset_cooked_content(applied.scene_content));
    MK_REQUIRE(fs.read_text(add_node.scene_path) == applied.scene_content);
    MK_REQUIRE(applied.scene_content ==
               mirakana::serialize_scene_document_v2(mirakana::deserialize_scene_document_v2(applied.scene_content)));

    auto duplicate = add_node;
    duplicate.node_id = mirakana::AuthoringId{"node/root"};
    duplicate.node_name = "DuplicateRoot";
    const auto before = fs.read_text(add_node.scene_path);
    const auto failed = mirakana::apply_scene_prefab_authoring(fs, duplicate);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.diagnostics, "duplicate authoring id"));
    MK_REQUIRE(fs.read_text(add_node.scene_path) == before);
}

MK_TEST("scene prefab authoring rejects unsafe paths unsupported payloads stale prefab refs and free form edits") {
    mirakana::ScenePrefabAuthoringRequest unsafe;
    unsafe.kind = mirakana::ScenePrefabAuthoringCommandKind::create_scene;
    unsafe.scene_path = "../level.scene";
    unsafe.scene = make_scene_prefab_authoring_scene_v2();

    const auto unsafe_result = mirakana::plan_scene_prefab_authoring(unsafe);

    MK_REQUIRE(!unsafe_result.succeeded());
    MK_REQUIRE(failures_contain(unsafe_result.diagnostics, "safe repository-relative"));

    CountingFileSystem counting_fs;
    const auto unsafe_apply = mirakana::apply_scene_prefab_authoring(counting_fs, unsafe);
    MK_REQUIRE(!unsafe_apply.succeeded());
    MK_REQUIRE(failures_contain(unsafe_apply.diagnostics, "safe repository-relative"));
    MK_REQUIRE(counting_fs.exists_calls == 0);
    MK_REQUIRE(counting_fs.read_calls == 0);
    MK_REQUIRE(counting_fs.write_calls == 0);

    mirakana::ScenePrefabAuthoringRequest line_injection;
    line_injection.kind = mirakana::ScenePrefabAuthoringCommandKind::add_node;
    line_injection.scene_path = "source/scenes/level.scene";
    line_injection.scene_content = mirakana::serialize_scene_document_v2(make_scene_prefab_authoring_scene_v2());
    line_injection.node_id = mirakana::AuthoringId{"node/injected"};
    line_injection.node_name = "Injected\ncomponent.0.id=component/injected";

    const auto injection_result = mirakana::plan_scene_prefab_authoring(line_injection);

    MK_REQUIRE(!injection_result.succeeded());
    MK_REQUIRE(failures_contain(injection_result.diagnostics, "line-oriented text value"));

    mirakana::ScenePrefabAuthoringRequest unsupported_payload;
    unsupported_payload.kind = mirakana::ScenePrefabAuthoringCommandKind::add_or_update_component;
    unsupported_payload.scene_path = "source/scenes/level.scene";
    unsupported_payload.scene_content = mirakana::serialize_scene_document_v2(make_scene_prefab_authoring_scene_v2());
    unsupported_payload.component_id = mirakana::AuthoringId{"component/root/raw"};
    unsupported_payload.component_node_id = mirakana::AuthoringId{"node/root"};
    unsupported_payload.component_type = mirakana::SceneComponentTypeId{"mesh_renderer"};
    unsupported_payload.component_payload_format = "json";

    const auto payload_result = mirakana::plan_scene_prefab_authoring(unsupported_payload);

    MK_REQUIRE(!payload_result.succeeded());
    MK_REQUIRE(failures_contain(payload_result.diagnostics, "unsupported component payload"));

    mirakana::ScenePrefabAuthoringRequest stale_prefab;
    stale_prefab.kind = mirakana::ScenePrefabAuthoringCommandKind::instantiate_prefab;
    stale_prefab.scene_path = "source/scenes/level.scene";
    stale_prefab.scene_content = mirakana::serialize_scene_document_v2(make_scene_prefab_authoring_scene_v2());
    stale_prefab.prefab_path = "source/prefabs/stale.prefab";
    stale_prefab.instance_id_prefix = "inst/stale/";
    stale_prefab.instance_name_prefix = "Stale ";
    stale_prefab.prefab_content = "format=GameEngine.Prefab.v2\n"
                                  "prefab.name=Stale\n"
                                  "scene.name=Stale Scene\n"
                                  "component.0.id=component/stale/mesh\n"
                                  "component.0.node=node/missing\n"
                                  "component.0.type=mesh_renderer\n"
                                  "component.0.property.0.name=mesh\n"
                                  "component.0.property.0.value=assets/meshes/stale\n";

    const auto stale_result = mirakana::plan_scene_prefab_authoring(stale_prefab);

    MK_REQUIRE(!stale_result.succeeded());
    MK_REQUIRE(failures_contain(stale_result.diagnostics, "stale prefab reference"));

    mirakana::ScenePrefabAuthoringRequest free_form;
    free_form.kind = mirakana::ScenePrefabAuthoringCommandKind::free_form_edit;
    free_form.scene_path = "source/scenes/level.scene";
    free_form.free_form_edit = "replace text";

    const auto free_form_result = mirakana::plan_scene_prefab_authoring(free_form);

    MK_REQUIRE(!free_form_result.succeeded());
    MK_REQUIRE(failures_contain(free_form_result.diagnostics, "free-form edits are not supported"));

    auto runtime_migration = free_form;
    runtime_migration.kind = mirakana::ScenePrefabAuthoringCommandKind::create_scene;
    runtime_migration.free_form_edit = "unsupported";
    runtime_migration.scene = make_scene_prefab_authoring_scene_v2();
    runtime_migration.runtime_package_migration = "ready";
    const auto migration_result = mirakana::plan_scene_prefab_authoring(runtime_migration);
    MK_REQUIRE(!migration_result.succeeded());
    MK_REQUIRE(failures_contain(migration_result.diagnostics, "Scene v2 runtime package migration is not supported"));
}

MK_TEST("source asset registration dry-runs registry and import metadata changes") {
    mirakana::SourceAssetRegistrationRequest request;
    request.kind = mirakana::SourceAssetRegistrationCommandKind::register_source_asset;
    request.source_registry_path = "source/assets/game.geassets";
    request.source_registry_content = "format=GameEngine.SourceAssetRegistry.v1\n"
                                      "asset.0.key=assets/materials/base\n"
                                      "asset.0.id=4082249533130855295\n"
                                      "asset.0.kind=material\n"
                                      "asset.0.source=source/materials/base.material\n"
                                      "asset.0.source_format=GameEngine.Material.v1\n"
                                      "asset.0.imported=intermediate/imported/materials/base.material\n";
    request.asset_key = mirakana::AssetKeyV2{"assets/textures/hero"};
    request.asset_kind = mirakana::AssetKind::texture;
    request.source_path = "source/textures/hero.getexture";
    request.source_format = "GameEngine.TextureSource.v1";
    request.imported_path = "intermediate/imported/textures/hero.texture";

    const auto planned = mirakana::plan_source_asset_registration(request);

    MK_REQUIRE(planned.succeeded());
    MK_REQUIRE(planned.changed_files.size() == 1);
    MK_REQUIRE(planned.changed_files[0].path == request.source_registry_path);
    MK_REQUIRE(planned.changed_files[0].document_kind == "GameEngine.SourceAssetRegistry.v1");
    MK_REQUIRE(planned.changed_files[0].content == planned.source_registry_content);
    MK_REQUIRE(planned.changed_files[0].content_hash ==
               mirakana::hash_asset_cooked_content(planned.source_registry_content));
    MK_REQUIRE(text_contains(planned.source_registry_content, "asset.1.key=assets/textures/hero\n"));
    MK_REQUIRE(text_contains(planned.source_registry_content, "asset.1.kind=texture\n"));
    MK_REQUIRE(text_contains(planned.source_registry_content, "asset.1.source=source/textures/hero.getexture\n"));
    MK_REQUIRE(text_contains(planned.source_registry_content, "asset.1.source_format=GameEngine.TextureSource.v1\n"));
    MK_REQUIRE(text_contains(planned.source_registry_content,
                             "asset.1.imported=intermediate/imported/textures/hero.texture\n"));
    MK_REQUIRE(planned.asset_identity_projection.assets.size() == 2);
    MK_REQUIRE(planned.model_mutations.size() == 1);
    MK_REQUIRE(planned.model_mutations[0].kind == "register_source_asset");
    MK_REQUIRE(planned.import_metadata.size() == 1);
    MK_REQUIRE(planned.import_metadata[0].kind == mirakana::AssetImportActionKind::texture);
    MK_REQUIRE(planned.import_metadata[0].source_path == request.source_path);
    MK_REQUIRE(planned.import_metadata[0].imported_path == request.imported_path);
}

MK_TEST("source asset registration apply writes only validated deterministic registry changes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("source/assets/game.geassets", "format=GameEngine.SourceAssetRegistry.v1\n"
                                                 "asset.0.key=assets/materials/base\n"
                                                 "asset.0.id=4082249533130855295\n"
                                                 "asset.0.kind=material\n"
                                                 "asset.0.source=source/materials/base.material\n"
                                                 "asset.0.source_format=GameEngine.Material.v1\n"
                                                 "asset.0.imported=intermediate/imported/materials/base.material\n");

    mirakana::SourceAssetRegistrationRequest request;
    request.kind = mirakana::SourceAssetRegistrationCommandKind::register_source_asset;
    request.source_registry_path = "source/assets/game.geassets";
    request.asset_key = mirakana::AssetKeyV2{"assets/audio/theme"};
    request.asset_kind = mirakana::AssetKind::audio;
    request.source_path = "source/audio/theme.geaudio";
    request.source_format = "GameEngine.AudioSource.v1";
    request.imported_path = "intermediate/imported/audio/theme.audio";

    const auto applied = mirakana::apply_source_asset_registration(fs, request);

    MK_REQUIRE(applied.succeeded());
    MK_REQUIRE(applied.changed_files.size() == 1);
    MK_REQUIRE(applied.changed_files[0].path == request.source_registry_path);
    MK_REQUIRE(fs.read_text(request.source_registry_path) == applied.source_registry_content);
    MK_REQUIRE(applied.source_registry_content ==
               mirakana::serialize_source_asset_registry_document(
                   mirakana::deserialize_source_asset_registry_document(applied.source_registry_content)));
    MK_REQUIRE(text_contains(applied.source_registry_content, "key=assets/audio/theme\n"));
    MK_REQUIRE(text_contains(applied.source_registry_content, "kind=audio\n"));
    MK_REQUIRE(mirakana::validate_asset_identity_document_v2(applied.asset_identity_projection).empty());

    auto duplicate = request;
    duplicate.asset_key = mirakana::AssetKeyV2{"assets/materials/base"};
    duplicate.source_path = "source/audio/duplicate.geaudio";
    const auto before = fs.read_text(request.source_registry_path);
    const auto failed = mirakana::apply_source_asset_registration(fs, duplicate);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.diagnostics, "duplicate asset key"));
    MK_REQUIRE(fs.read_text(request.source_registry_path) == before);
}

MK_TEST("source asset registration rejects unsafe paths unsupported formats external claims and free form edits") {
    mirakana::SourceAssetRegistrationRequest unsafe;
    unsafe.kind = mirakana::SourceAssetRegistrationCommandKind::register_source_asset;
    unsafe.source_registry_path = "../assets/game.geassets";
    unsafe.asset_key = mirakana::AssetKeyV2{"assets/textures/hero"};
    unsafe.asset_kind = mirakana::AssetKind::texture;
    unsafe.source_path = "source/textures/hero.getexture";
    unsafe.source_format = "GameEngine.TextureSource.v1";
    unsafe.imported_path = "intermediate/imported/textures/hero.texture";

    const auto unsafe_result = mirakana::plan_source_asset_registration(unsafe);

    MK_REQUIRE(!unsafe_result.succeeded());
    MK_REQUIRE(failures_contain(unsafe_result.diagnostics, "safe repository-relative"));

    CountingFileSystem counting_fs;
    const auto unsafe_apply = mirakana::apply_source_asset_registration(counting_fs, unsafe);
    MK_REQUIRE(!unsafe_apply.succeeded());
    MK_REQUIRE(failures_contain(unsafe_apply.diagnostics, "safe repository-relative"));
    MK_REQUIRE(counting_fs.exists_calls == 0);
    MK_REQUIRE(counting_fs.read_calls == 0);
    MK_REQUIRE(counting_fs.write_calls == 0);

    auto unsafe_control_path = unsafe;
    unsafe_control_path.source_registry_path = "source/assets/game\t.geassets";
    const auto unsafe_control_result = mirakana::plan_source_asset_registration(unsafe_control_path);
    MK_REQUIRE(!unsafe_control_result.succeeded());
    MK_REQUIRE(failures_contain(unsafe_control_result.diagnostics, "safe repository-relative"));

    auto unsupported_format = unsafe;
    unsupported_format.source_registry_path = "source/assets/game.geassets";
    unsupported_format.source_format = "image/png";
    const auto format_result = mirakana::plan_source_asset_registration(unsupported_format);
    MK_REQUIRE(!format_result.succeeded());
    MK_REQUIRE(failures_contain(format_result.diagnostics, "unsupported source format"));

    auto external_claim = unsupported_format;
    external_claim.source_format = "GameEngine.TextureSource.v1";
    external_claim.external_importer = "ready";
    const auto external_result = mirakana::plan_source_asset_registration(external_claim);
    MK_REQUIRE(!external_result.succeeded());
    MK_REQUIRE(failures_contain(external_result.diagnostics, "external importer execution is not supported"));

    mirakana::SourceAssetRegistrationRequest free_form;
    free_form.kind = mirakana::SourceAssetRegistrationCommandKind::free_form_edit;
    free_form.source_registry_path = "source/assets/game.geassets";
    free_form.free_form_edit = "append raw text";
    const auto free_form_result = mirakana::plan_source_asset_registration(free_form);
    MK_REQUIRE(!free_form_result.succeeded());
    MK_REQUIRE(failures_contain(free_form_result.diagnostics, "free-form edits are not supported"));

    auto package_claim = unsupported_format;
    package_claim.source_format = "GameEngine.TextureSource.v1";
    package_claim.package_cooking = "ready";
    package_claim.renderer_rhi_residency = "ready";
    package_claim.package_streaming = "ready";
    package_claim.material_graph = "ready";
    package_claim.shader_graph = "ready";
    package_claim.live_shader_generation = "ready";
    package_claim.editor_productization = "ready";
    const auto package_result = mirakana::plan_source_asset_registration(package_claim);
    MK_REQUIRE(!package_result.succeeded());
    MK_REQUIRE(failures_contain(package_result.diagnostics, "package cooking is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "renderer/RHI residency is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "package streaming is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "material graph is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "shader graph is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "live shader generation is not supported"));
    MK_REQUIRE(failures_contain(package_result.diagnostics, "editor productization is not supported"));
}

MK_TEST("source asset registration validates dependency targets and canonical dry-run rows") {
    mirakana::SourceAssetRegistryDocumentV1 base_registry;
    base_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/meshes/cube"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/cube.gemesh",
        .source_format = "GameEngine.MeshSource.v2",
        .imported_path = "intermediate/imported/meshes/cube.mesh",
        .dependencies = {},
    });
    base_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/materials/hero"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/hero.material",
        .source_format = "GameEngine.Material.v1",
        .imported_path = "intermediate/imported/materials/hero.material",
        .dependencies = {},
    });
    base_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = mirakana::AssetKeyV2{"assets/textures/hero"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/hero.getexture",
        .source_format = "GameEngine.TextureSource.v1",
        .imported_path = "intermediate/imported/textures/hero.texture",
        .dependencies = {},
    });

    mirakana::SourceAssetRegistrationRequest scene_request;
    scene_request.kind = mirakana::SourceAssetRegistrationCommandKind::register_source_asset;
    scene_request.source_registry_path = "source/assets/game.geassets";
    scene_request.source_registry_content = mirakana::serialize_source_asset_registry_document(base_registry);
    scene_request.asset_key = mirakana::AssetKeyV2{"assets/scenes/level"};
    scene_request.asset_kind = mirakana::AssetKind::scene;
    scene_request.source_path = "source/scenes/level.scene";
    scene_request.source_format = "GameEngine.Scene.v1";
    scene_request.imported_path = "intermediate/imported/scenes/level.scene";
    scene_request.dependency_rows = {
        mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::scene_sprite,
                                             .key = mirakana::AssetKeyV2{"assets/textures/hero"}},
        mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::scene_mesh,
                                             .key = mirakana::AssetKeyV2{"assets/meshes/cube"}},
        mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::scene_material,
                                             .key = mirakana::AssetKeyV2{"assets/materials/hero"}},
    };

    const auto planned_scene = mirakana::plan_source_asset_registration(scene_request);

    MK_REQUIRE(planned_scene.succeeded());
    MK_REQUIRE(planned_scene.model_mutations.size() == 1);
    MK_REQUIRE(planned_scene.import_metadata.size() == 1);
    MK_REQUIRE(planned_scene.model_mutations[0].dependency_rows.size() == 3);
    MK_REQUIRE(planned_scene.import_metadata[0].dependency_rows.size() == 3);
    MK_REQUIRE(planned_scene.model_mutations[0].dependency_rows[0].kind ==
               mirakana::AssetDependencyKind::scene_material);
    MK_REQUIRE(planned_scene.model_mutations[0].dependency_rows[1].kind == mirakana::AssetDependencyKind::scene_mesh);
    MK_REQUIRE(planned_scene.model_mutations[0].dependency_rows[2].kind == mirakana::AssetDependencyKind::scene_sprite);
    MK_REQUIRE(planned_scene.import_metadata[0].dependency_rows[0].kind ==
               mirakana::AssetDependencyKind::scene_material);
    const auto planned_registry =
        mirakana::deserialize_source_asset_registry_document(planned_scene.source_registry_content);
    const auto scene_row = std::ranges::find_if(planned_registry.assets,
                                                [](const auto& row) { return row.key.value == "assets/scenes/level"; });
    MK_REQUIRE(scene_row != planned_registry.assets.end());
    MK_REQUIRE(scene_row->dependencies.size() == 3);
    MK_REQUIRE(scene_row->dependencies[0].kind == mirakana::AssetDependencyKind::scene_material);
    MK_REQUIRE(scene_row->dependencies[1].kind == mirakana::AssetDependencyKind::scene_mesh);
    MK_REQUIRE(scene_row->dependencies[2].kind == mirakana::AssetDependencyKind::scene_sprite);

    auto duplicate_noop = scene_request;
    duplicate_noop.source_registry_content = planned_scene.source_registry_content;
    const auto duplicate_result = mirakana::plan_source_asset_registration(duplicate_noop);
    MK_REQUIRE(duplicate_result.succeeded());
    MK_REQUIRE(duplicate_result.changed_files.empty());
    MK_REQUIRE(duplicate_result.model_mutations.empty());
    MK_REQUIRE(duplicate_result.import_metadata.empty());

    auto wrong_kind = scene_request;
    wrong_kind.asset_key = mirakana::AssetKeyV2{"assets/materials/broken"};
    wrong_kind.asset_kind = mirakana::AssetKind::material;
    wrong_kind.source_path = "source/materials/broken.material";
    wrong_kind.source_format = "GameEngine.Material.v1";
    wrong_kind.imported_path = "intermediate/imported/materials/broken.material";
    wrong_kind.dependency_rows = {
        mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::material_texture,
                                             .key = mirakana::AssetKeyV2{"assets/meshes/cube"}},
    };
    const auto wrong_kind_result = mirakana::plan_source_asset_registration(wrong_kind);
    MK_REQUIRE(!wrong_kind_result.succeeded());
    MK_REQUIRE(
        failures_contain(wrong_kind_result.diagnostics, "dependency target kind does not match dependency kind"));

    auto self_dependency = wrong_kind;
    self_dependency.asset_key = mirakana::AssetKeyV2{"assets/materials/self"};
    self_dependency.source_path = "source/materials/self.material";
    self_dependency.imported_path = "intermediate/imported/materials/self.material";
    self_dependency.dependency_rows = {
        mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::material_texture,
                                             .key = mirakana::AssetKeyV2{"assets/materials/self"}},
    };
    const auto self_result = mirakana::plan_source_asset_registration(self_dependency);
    MK_REQUIRE(!self_result.succeeded());
    MK_REQUIRE(failures_contain(self_result.diagnostics, "dependency target kind does not match dependency kind"));
}

MK_TEST("registered source asset cook package rejects empty selected asset keys") {
    auto request = make_registered_cook_request();
    request.selected_asset_keys.clear();

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "missing_selected_asset_keys"));
    MK_REQUIRE(failures_contain(result.diagnostics, "at least one registered source asset key"));
}

MK_TEST("registered source asset cook package rejects duplicate selected asset keys") {
    auto request = make_registered_cook_request();
    request.selected_asset_keys = {
        mirakana::AssetKeyV2{"assets/textures/hero"},
        mirakana::AssetKeyV2{"assets/textures/hero"},
    };

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "duplicate_selected_asset_key"));
    MK_REQUIRE(failures_contain(result.diagnostics, "selected asset key is duplicated"));
}

MK_TEST("registered source asset cook package rejects zero source revision") {
    auto request = make_registered_cook_request();
    request.source_revision = 0;

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "invalid_source_revision"));
    MK_REQUIRE(failures_contain(result.diagnostics, "non-zero"));
}

MK_TEST("registered source asset cook package rejects invalid selected asset keys") {
    auto request = make_registered_cook_request();
    request.selected_asset_keys = {mirakana::AssetKeyV2{R"(bad\slash)"}};

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "invalid_selected_asset_key"));
    MK_REQUIRE(failures_contain(result.diagnostics, "valid AssetKeyV2"));
}

MK_TEST("registered source asset cook package rejects duplicate inline source file paths") {
    auto request = make_registered_cook_request();
    request.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = registered_cook_texture_source_content()},
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = registered_cook_material_source_content()},
    };

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "duplicate_source_file"));
    MK_REQUIRE(failures_contain(result.diagnostics, "source file payload path is duplicated"));
}

MK_TEST("registered source asset cook package rejects non geassets source registry path") {
    auto request = make_registered_cook_request();
    request.source_registry_path = "source/assets/registry.txt";

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "unsafe_source_registry_path"));
    MK_REQUIRE(failures_contain(result.diagnostics, ".geassets"));
}

MK_TEST("registered source asset cook package rejects invalid source registry document") {
    auto request = make_registered_cook_request();
    request.source_registry_content = "not-json {";

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "invalid_source_registry"));
    MK_REQUIRE(failures_contain(result.diagnostics, "failed to parse"));
}

MK_TEST("registered source asset cook package dry-runs selected rows into cooked artifacts and package index") {
    const auto request = make_registered_cook_request();

    const auto result = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.changed_files.size() == 3);
    MK_REQUIRE(result.changed_files[0].path == "runtime/assets/materials/hero.material");
    MK_REQUIRE(result.changed_files[0].document_kind == "GameEngine.Material.v1");
    MK_REQUIRE(result.changed_files[0].content == registered_cook_material_source_content());
    MK_REQUIRE(result.changed_files[0].content_hash ==
               mirakana::hash_asset_cooked_content(result.changed_files[0].content));
    MK_REQUIRE(result.changed_files[1].path == "runtime/assets/textures/hero.texture");
    MK_REQUIRE(result.changed_files[1].document_kind == "GameEngine.CookedTexture.v1");
    MK_REQUIRE(text_contains(result.changed_files[1].content, "format=GameEngine.CookedTexture.v1\n"));
    MK_REQUIRE(text_contains(result.changed_files[1].content, "texture.width=4\n"));
    MK_REQUIRE(result.changed_files[2].path == request.package_index_path);
    MK_REQUIRE(result.changed_files[2].document_kind == "GameEngine.CookedPackageIndex.v1");
    MK_REQUIRE(result.changed_files[2].content == result.package_index_content);

    MK_REQUIRE(result.model_mutations.size() == 2);
    MK_REQUIRE(result.model_mutations[0].kind == "cook_registered_source_asset");
    MK_REQUIRE(result.model_mutations[0].target_path == "runtime/assets/materials/hero.material");
    MK_REQUIRE(result.model_mutations[0].asset_key.value == "assets/materials/hero");
    MK_REQUIRE(result.model_mutations[0].dependency_rows.size() == 1);
    MK_REQUIRE(result.model_mutations[1].target_path == "runtime/assets/textures/hero.texture");
    MK_REQUIRE(result.model_mutations[1].asset_key.value == "assets/textures/hero");
    MK_REQUIRE(result.validation_recipes.size() == 3);
    MK_REQUIRE(result.unsupported_gap_ids.size() >= 3);
    MK_REQUIRE(result.undo_token == "placeholder-only");

    const auto index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto texture = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/hero"});
    const auto material = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/hero"});
    MK_REQUIRE(index.entries.size() == 2);
    MK_REQUIRE(index.dependencies.size() == 1);
    MK_REQUIRE(index.dependencies[0].asset == material);
    MK_REQUIRE(index.dependencies[0].dependency == texture);
    MK_REQUIRE(index.dependencies[0].kind == mirakana::AssetDependencyKind::material_texture);
}

MK_TEST("registered source asset cook package apply rereads registry sources and index before deterministic writes") {
    mirakana::MemoryFileSystem fs;
    const auto request = make_registered_cook_request();
    fs.write_text(request.source_registry_path, request.source_registry_content);
    fs.write_text(request.package_index_path, request.package_index_content);
    fs.write_text("source/textures/hero.texture_source", registered_cook_texture_source_content());
    fs.write_text("source/materials/hero.material", registered_cook_material_source_content());

    auto apply_request = request;
    apply_request.source_registry_content = "stale registry content must be ignored by apply";
    apply_request.package_index_content = "stale package index content must be ignored by apply";
    apply_request.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = "stale texture content"},
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/materials/hero.material",
                                                             .content = "stale material content"},
    };

    const auto result = mirakana::apply_registered_source_asset_cook_package(fs, apply_request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.changed_files.size() == 3);
    MK_REQUIRE(result.changed_files[0].path == "runtime/assets/materials/hero.material");
    MK_REQUIRE(result.changed_files[1].path == "runtime/assets/textures/hero.texture");
    MK_REQUIRE(result.changed_files[2].path == request.package_index_path);
    MK_REQUIRE(fs.read_text("runtime/assets/materials/hero.material") == result.changed_files[0].content);
    MK_REQUIRE(fs.read_text("runtime/assets/textures/hero.texture") == result.changed_files[1].content);
    MK_REQUIRE(fs.read_text(request.package_index_path) == result.package_index_content);

    fs.write_text("source/textures/hero.texture_source", "malformed texture payload");
    const auto material_before = fs.read_text("runtime/assets/materials/hero.material");
    const auto texture_before = fs.read_text("runtime/assets/textures/hero.texture");
    const auto index_before = fs.read_text(request.package_index_path);

    const auto failed = mirakana::apply_registered_source_asset_cook_package(fs, request);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.diagnostics, "asset import failed"));
    MK_REQUIRE(fs.read_text("runtime/assets/materials/hero.material") == material_before);
    MK_REQUIRE(fs.read_text("runtime/assets/textures/hero.texture") == texture_before);
    MK_REQUIRE(fs.read_text(request.package_index_path) == index_before);
}

MK_TEST("registered source asset cook package rejects missing rows unsupported formats and unsafe paths") {
    auto missing = make_registered_cook_request();
    missing.selected_asset_keys = {mirakana::AssetKeyV2{"assets/textures/missing"}};
    missing.source_files = {};
    const auto missing_result = mirakana::plan_registered_source_asset_cook_package(missing);
    MK_REQUIRE(!missing_result.succeeded());
    MK_REQUIRE(failures_contain(missing_result.diagnostics, "source asset key is missing"));

    auto unsupported_format = make_registered_cook_request();
    const auto format_needle = std::string{"asset.2.source_format=GameEngine.TextureSource.v1\n"};
    const auto format_pos = unsupported_format.source_registry_content.find(format_needle);
    MK_REQUIRE(format_pos != std::string::npos);
    unsupported_format.source_registry_content.replace(format_pos, format_needle.size(),
                                                       "asset.2.source_format=image/png\n");
    const auto format_result = mirakana::plan_registered_source_asset_cook_package(unsupported_format);
    MK_REQUIRE(!format_result.succeeded());
    MK_REQUIRE(failures_contain(format_result.diagnostics, "unsupported source format"));

    auto unsafe = make_registered_cook_request();
    unsafe.package_index_path = "../runtime/game.geindex";
    const auto unsafe_result = mirakana::plan_registered_source_asset_cook_package(unsafe);
    MK_REQUIRE(!unsafe_result.succeeded());
    MK_REQUIRE(failures_contain(unsafe_result.diagnostics, "package-relative"));

    CountingFileSystem counting_fs;
    const auto unsafe_apply = mirakana::apply_registered_source_asset_cook_package(counting_fs, unsafe);
    MK_REQUIRE(!unsafe_apply.succeeded());
    MK_REQUIRE(failures_contain(unsafe_apply.diagnostics, "package-relative"));
    MK_REQUIRE(counting_fs.exists_calls == 0);
    MK_REQUIRE(counting_fs.read_calls == 0);
    MK_REQUIRE(counting_fs.write_calls == 0);

    auto unselected_dependency = make_registered_cook_request();
    unselected_dependency.selected_asset_keys = {mirakana::AssetKeyV2{"assets/materials/hero"}};
    unselected_dependency.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/materials/hero.material",
                                                             .content = registered_cook_material_source_content()},
    };
    const auto unselected_result = mirakana::plan_registered_source_asset_cook_package(unselected_dependency);
    MK_REQUIRE(!unselected_result.succeeded());
    MK_REQUIRE(failures_contain(unselected_result.diagnostics, "dependencies must be selected explicitly"));
}

MK_TEST("registered source asset cook package registry closure expands registered dependencies only") {
    auto closure_request = make_registered_cook_request();
    closure_request.dependency_expansion =
        mirakana::RegisteredSourceAssetCookDependencyExpansion::registered_source_registry_closure;
    closure_request.dependency_cooking = "registry_closure";
    closure_request.selected_asset_keys = {mirakana::AssetKeyV2{"assets/materials/hero"}};
    closure_request.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/materials/hero.material",
                                                             .content = registered_cook_material_source_content()},
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = registered_cook_texture_source_content()},
    };

    const auto closure_result = mirakana::plan_registered_source_asset_cook_package(closure_request);
    MK_REQUIRE(closure_result.succeeded());
    MK_REQUIRE(closure_result.changed_files.size() == 3);
    MK_REQUIRE(closure_result.model_mutations.size() == 2);

    auto bad_sentinel = closure_request;
    bad_sentinel.dependency_cooking = "unsupported";
    const auto bad_closure_sentinel = mirakana::plan_registered_source_asset_cook_package(bad_sentinel);
    MK_REQUIRE(!bad_closure_sentinel.succeeded());
    MK_REQUIRE(failures_contain(bad_closure_sentinel.diagnostics, "dependency_cooking must be registry_closure"));

    auto explicit_with_registry_claim = make_registered_cook_request();
    explicit_with_registry_claim.dependency_cooking = "registry_closure";
    const auto explicit_bad = mirakana::plan_registered_source_asset_cook_package(explicit_with_registry_claim);
    MK_REQUIRE(!explicit_bad.succeeded());
    MK_REQUIRE(failures_contain(explicit_bad.diagnostics,
                                "dependency_cooking must remain unsupported when dependency_expansion is"));
}

MK_TEST("registered source asset cook package rejects malformed payloads and package index conflicts") {
    auto malformed = make_registered_cook_request();
    malformed.selected_asset_keys = {mirakana::AssetKeyV2{"assets/textures/hero"}};
    malformed.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{
            .path = "source/textures/hero.texture_source",
            .content = "format=GameEngine.TextureSource.v1\ntexture.width=bad\n"},
    };
    const auto malformed_result = mirakana::plan_registered_source_asset_cook_package(malformed);
    MK_REQUIRE(!malformed_result.succeeded());
    MK_REQUIRE(failures_contain(malformed_result.diagnostics, "asset import failed"));

    auto missing_payload = malformed;
    missing_payload.source_files = {};
    const auto missing_payload_result = mirakana::plan_registered_source_asset_cook_package(missing_payload);
    MK_REQUIRE(!missing_payload_result.succeeded());
    MK_REQUIRE(failures_contain(missing_payload_result.diagnostics, "source payload is missing"));

    auto conflict = malformed;
    conflict.source_files = {
        mirakana::RegisteredSourceAssetCookPackageSourceFile{.path = "source/textures/hero.texture_source",
                                                             .content = registered_cook_texture_source_content()},
    };
    conflict.package_index_content = registered_cook_conflicting_package_index_content();
    const auto conflict_result = mirakana::plan_registered_source_asset_cook_package(conflict);
    MK_REQUIRE(!conflict_result.succeeded());
    MK_REQUIRE(failures_contain(conflict_result.diagnostics, "package index conflict"));
}

MK_TEST("registered source asset cook package rejects unsupported claims and free form edits") {
    auto request = make_registered_cook_request();
    request.dependency_cooking = "ready";
    request.external_importer_execution = "ready";
    request.renderer_rhi_residency = "ready";
    request.package_streaming = "ready";
    request.material_graph = "ready";
    request.shader_graph = "ready";
    request.live_shader_generation = "ready";
    request.editor_productization = "ready";
    request.metal_readiness = "ready";
    request.public_native_rhi_handles = "ready";
    request.general_production_renderer_quality = "ready";
    request.arbitrary_shell = "ready";
    request.free_form_edit = "ready";

    const auto unsupported = mirakana::plan_registered_source_asset_cook_package(request);

    MK_REQUIRE(!unsupported.succeeded());
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "broad dependency cooking is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "external importer execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "renderer/RHI residency is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "package streaming is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "material graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "shader graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "live shader generation is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "editor productization is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "Metal readiness is host-gated"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "public native/RHI handles are not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "general production renderer quality is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "arbitrary shell execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "free-form edits are not supported"));

    mirakana::RegisteredSourceAssetCookPackageRequest free_form;
    free_form.kind = mirakana::RegisteredSourceAssetCookPackageCommandKind::free_form_edit;
    free_form.source_registry_path = "source/assets/game.geassets";
    free_form.package_index_path = "runtime/game.geindex";
    free_form.free_form_edit = "append raw package text";
    const auto free_form_result = mirakana::plan_registered_source_asset_cook_package(free_form);
    MK_REQUIRE(!free_form_result.succeeded());
    MK_REQUIRE(failures_contain(free_form_result.diagnostics, "free-form edits are not supported"));
}

MK_TEST("registered source asset workflow cooks migrates loads and instantiates runtime scene") {
    mirakana::MemoryFileSystem fs;
    const auto cook_request = make_registered_scene_workflow_cook_request();
    fs.write_text(cook_request.source_registry_path, cook_request.source_registry_content);
    fs.write_text(cook_request.package_index_path, cook_request.package_index_content);
    for (const auto& source : cook_request.source_files) {
        fs.write_text(source.path, source.content);
    }

    const auto cooked = mirakana::apply_registered_source_asset_cook_package(fs, cook_request);

    MK_REQUIRE(cooked.succeeded());
    MK_REQUIRE(cooked.changed_files.size() == 4);
    MK_REQUIRE(cooked.changed_files[0].path == "runtime/assets/materials/hero.material");
    MK_REQUIRE(cooked.changed_files[1].path == "runtime/assets/meshes/cube.mesh");
    MK_REQUIRE(cooked.changed_files[2].path == "runtime/assets/textures/hero.texture");
    MK_REQUIRE(cooked.changed_files[3].path == cook_request.package_index_path);

    const auto migration_request = make_registered_scene_workflow_migration_request(cooked.package_index_content);
    fs.write_text(migration_request.scene_v2_path, migration_request.scene_v2_content);

    const auto migrated = mirakana::apply_scene_v2_runtime_package_migration(fs, migration_request);

    MK_REQUIRE(migrated.succeeded());
    MK_REQUIRE(migrated.changed_files.size() == 2);
    MK_REQUIRE(migrated.changed_files[0].path == migration_request.output_scene_path);
    MK_REQUIRE(migrated.changed_files[1].path == migration_request.package_index_path);

    const auto package = mirakana::runtime::load_runtime_asset_package(
        fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "runtime/game.geindex", .content_root = ""});
    MK_REQUIRE(package.succeeded());
    MK_REQUIRE(package.package.records().size() == 4);

    const auto scene_asset = mirakana::asset_id_from_key_v2(migration_request.scene_asset_key);
    const auto instantiated = mirakana::runtime_scene::instantiate_runtime_scene(package.package, scene_asset);
    MK_REQUIRE(instantiated.succeeded());
    MK_REQUIRE(instantiated.instance.has_value());
    MK_REQUIRE(instantiated.instance->scene.name() == "Registered Runtime Level");
    MK_REQUIRE(instantiated.instance->references.size() == 4);
    MK_REQUIRE(instantiated.instance->references[0].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::mesh);
    MK_REQUIRE(instantiated.instance->references[1].kind ==
               mirakana::runtime_scene::RuntimeSceneReferenceKind::material);
    MK_REQUIRE(instantiated.instance->references[2].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::sprite);
    MK_REQUIRE(instantiated.instance->references[3].kind ==
               mirakana::runtime_scene::RuntimeSceneReferenceKind::material);
}

MK_TEST("registered source asset workflow rejects skipped or stale cooked package prerequisites") {
    auto skipped_cook = make_registered_scene_workflow_migration_request();
    const auto skipped_result = mirakana::plan_scene_v2_runtime_package_migration(skipped_cook);
    MK_REQUIRE(!skipped_result.succeeded());
    MK_REQUIRE(failures_contain(skipped_result.diagnostics, "scene mesh package entry is missing"));
    MK_REQUIRE(failures_contain(skipped_result.diagnostics, "scene material package entry is missing"));
    MK_REQUIRE(failures_contain(skipped_result.diagnostics, "scene sprite package entry is missing"));

    const auto cooked =
        mirakana::plan_registered_source_asset_cook_package(make_registered_scene_workflow_cook_request());
    MK_REQUIRE(cooked.succeeded());
    auto stale_index = mirakana::deserialize_asset_cooked_package_index(cooked.package_index_content);
    const auto texture = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/hero"});
    const auto removed_entries =
        std::ranges::remove_if(stale_index.entries, [texture](const mirakana::AssetCookedPackageEntry& entry) {
            return entry.asset == texture;
        });
    stale_index.entries.erase(removed_entries.begin(), removed_entries.end());
    const auto removed_deps =
        std::ranges::remove_if(stale_index.dependencies, [texture](const mirakana::AssetDependencyEdge& edge) {
            return edge.asset == texture || edge.dependency == texture;
        });
    stale_index.dependencies.erase(removed_deps.begin(), removed_deps.end());

    auto stale_package =
        make_registered_scene_workflow_migration_request(mirakana::serialize_asset_cooked_package_index(stale_index));
    const auto stale_result = mirakana::plan_scene_v2_runtime_package_migration(stale_package);
    MK_REQUIRE(!stale_result.succeeded());
    MK_REQUIRE(failures_contain(stale_result.diagnostics, "scene sprite package entry is missing"));
}

MK_TEST("runtime scene package validation dry-runs and executes package scene instantiation") {
    mirakana::MemoryFileSystem fs;
    RuntimeScenePackageValidationFixture fixture;
    write_valid_runtime_scene_validation_fixture(fs, fixture);

    const auto dry_run = mirakana::plan_runtime_scene_package_validation(fixture.request);

    MK_REQUIRE(dry_run.succeeded());
    MK_REQUIRE(dry_run.summary.package_index_path == fixture.package_index_path);
    MK_REQUIRE(dry_run.summary.scene_asset_key.value == fixture.scene_key.value);
    MK_REQUIRE(dry_run.summary.scene_asset == fixture.scene_asset);
    MK_REQUIRE(dry_run.validation_recipes.size() == 3);
    MK_REQUIRE(dry_run.unsupported_gap_ids.size() >= 3);
    MK_REQUIRE(dry_run.undo_token == "placeholder-only");

    const auto executed = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

    MK_REQUIRE(executed.succeeded());
    MK_REQUIRE(executed.summary.package_index_path == fixture.package_index_path);
    MK_REQUIRE(executed.summary.content_root.empty());
    MK_REQUIRE(executed.summary.scene_asset == fixture.scene_asset);
    MK_REQUIRE(executed.summary.scene_name == "Validated Runtime Level");
    MK_REQUIRE(executed.summary.scene_node_count == 2);
    MK_REQUIRE(executed.summary.package_record_count == 4);
    MK_REQUIRE(executed.summary.references.size() == 4);
    MK_REQUIRE(executed.summary.references[0].reference_kind == "mesh");
    MK_REQUIRE(executed.summary.references[0].asset == fixture.mesh_asset);
    MK_REQUIRE(executed.summary.references[0].expected_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(executed.summary.references[0].actual_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(executed.summary.references[2].reference_kind == "sprite");
    MK_REQUIRE(executed.summary.references[2].expected_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(executed.summary.references[2].actual_kind == mirakana::AssetKind::texture);
}

MK_TEST("runtime scene package validation reports package load and scene diagnostics") {
    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        write_valid_runtime_scene_validation_fixture(fs, fixture);
        fs.remove(fixture.mesh_path);

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(failures_contain(result.diagnostics, "runtime_package_load_failed"));
        MK_REQUIRE(failures_contain(result.diagnostics, "missing cooked payload"));
    }

    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        write_runtime_scene_validation_fixture(fs, fixture,
                                               {
                                                   mirakana::AssetCookedArtifact{.asset = fixture.mesh_asset,
                                                                                 .kind = mirakana::AssetKind::mesh,
                                                                                 .path = fixture.mesh_path,
                                                                                 .content = "mesh",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                                   mirakana::AssetCookedArtifact{.asset = fixture.material_asset,
                                                                                 .kind = mirakana::AssetKind::material,
                                                                                 .path = fixture.material_path,
                                                                                 .content = "material",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                               });

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(result.summary.scene_name == "Validated Runtime Level");
        MK_REQUIRE(failures_contain(result.diagnostics, "missing_referenced_asset"));
    }

    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        write_runtime_scene_validation_fixture(fs, fixture,
                                               {
                                                   mirakana::AssetCookedArtifact{.asset = fixture.mesh_asset,
                                                                                 .kind = mirakana::AssetKind::material,
                                                                                 .path = fixture.mesh_path,
                                                                                 .content = "mesh",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                                   mirakana::AssetCookedArtifact{.asset = fixture.material_asset,
                                                                                 .kind = mirakana::AssetKind::material,
                                                                                 .path = fixture.material_path,
                                                                                 .content = "material",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                                   mirakana::AssetCookedArtifact{.asset = fixture.sprite_asset,
                                                                                 .kind = mirakana::AssetKind::texture,
                                                                                 .path = fixture.sprite_path,
                                                                                 .content = "sprite",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                               });

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(failures_contain(result.diagnostics, "referenced_asset_kind_mismatch"));
        MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::mesh);
        MK_REQUIRE(result.diagnostics[0].actual_kind == mirakana::AssetKind::material);
    }
}

MK_TEST("runtime scene package validation reports scene row and payload failures") {
    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        write_runtime_scene_validation_fixture(fs, fixture,
                                               {
                                                   mirakana::AssetCookedArtifact{.asset = fixture.mesh_asset,
                                                                                 .kind = mirakana::AssetKind::mesh,
                                                                                 .path = fixture.mesh_path,
                                                                                 .content = "mesh",
                                                                                 .source_revision = 7,
                                                                                 .dependencies = {}},
                                               });
        fixture.request.scene_asset_key = mirakana::AssetKeyV2{"assets/scenes/missing"};

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(failures_contain(result.diagnostics, "missing_scene_asset"));
    }

    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        configure_runtime_scene_validation_request(fixture);
        const std::string scene_payload = mirakana::serialize_scene(
            make_runtime_scene_validation_scene(fixture.mesh_asset, fixture.material_asset, fixture.sprite_asset));
        const std::vector<mirakana::AssetCookedArtifact> artifacts{
            mirakana::AssetCookedArtifact{.asset = fixture.scene_asset,
                                          .kind = mirakana::AssetKind::texture,
                                          .path = fixture.scene_path,
                                          .content = scene_payload,
                                          .source_revision = 7,
                                          .dependencies = {}},
        };
        fs.write_text(fixture.package_index_path, mirakana::serialize_asset_cooked_package_index(
                                                      mirakana::build_asset_cooked_package_index(artifacts, {})));
        fs.write_text(fixture.scene_path, scene_payload);

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(failures_contain(result.diagnostics, "wrong_asset_kind"));
    }

    {
        mirakana::MemoryFileSystem fs;
        RuntimeScenePackageValidationFixture fixture;
        write_runtime_scene_validation_fixture(fs, fixture, {},
                                               "format=GameEngine.Scene.v1\nscene.name=Broken\nnode.count=1\n");

        const auto result = mirakana::execute_runtime_scene_package_validation(fs, fixture.request);

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(failures_contain(result.diagnostics, "malformed_scene_payload"));
    }
}

MK_TEST("runtime scene package validation rejects unsafe paths unsupported claims and free form edits") {
    auto unsafe = RuntimeScenePackageValidationFixture{};
    configure_runtime_scene_validation_request(unsafe);
    unsafe.request.package_index_path = "../runtime/validation.geindex";
    const auto unsafe_result = mirakana::plan_runtime_scene_package_validation(unsafe.request);
    MK_REQUIRE(!unsafe_result.succeeded());
    MK_REQUIRE(failures_contain(unsafe_result.diagnostics, "package-relative"));

    CountingFileSystem counting_fs;
    const auto unsafe_execute = mirakana::execute_runtime_scene_package_validation(counting_fs, unsafe.request);
    MK_REQUIRE(!unsafe_execute.succeeded());
    MK_REQUIRE(counting_fs.read_calls == 0);

    auto invalid_key = RuntimeScenePackageValidationFixture{};
    configure_runtime_scene_validation_request(invalid_key);
    invalid_key.request.scene_asset_key = mirakana::AssetKeyV2{"bad key"};
    const auto invalid_key_result = mirakana::plan_runtime_scene_package_validation(invalid_key.request);
    MK_REQUIRE(!invalid_key_result.succeeded());
    MK_REQUIRE(failures_contain(invalid_key_result.diagnostics, "invalid_scene_asset_key"));

    auto unsupported = RuntimeScenePackageValidationFixture{};
    configure_runtime_scene_validation_request(unsupported);
    unsupported.request.package_cooking = "ready";
    unsupported.request.runtime_source_parsing = "ready";
    unsupported.request.external_importer_execution = "ready";
    unsupported.request.renderer_rhi_residency = "ready";
    unsupported.request.package_streaming = "ready";
    unsupported.request.material_graph = "ready";
    unsupported.request.shader_graph = "ready";
    unsupported.request.live_shader_generation = "ready";
    unsupported.request.editor_productization = "ready";
    unsupported.request.metal_readiness = "ready";
    unsupported.request.public_native_rhi_handles = "ready";
    unsupported.request.general_production_renderer_quality = "ready";
    unsupported.request.arbitrary_shell = "ready";
    unsupported.request.free_form_edit = "ready";
    const auto unsupported_result = mirakana::plan_runtime_scene_package_validation(unsupported.request);
    MK_REQUIRE(!unsupported_result.succeeded());
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "package cooking is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "runtime source parsing is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "external importer execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "renderer/RHI residency is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "package streaming is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "material graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "shader graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "live shader generation is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "editor productization is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "Metal readiness is host-gated"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "public native/RHI handles are not supported"));
    MK_REQUIRE(
        failures_contain(unsupported_result.diagnostics, "general production renderer quality is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "arbitrary shell execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported_result.diagnostics, "free-form edits are not supported"));

    mirakana::RuntimeScenePackageValidationRequest free_form;
    free_form.kind = mirakana::RuntimeScenePackageValidationCommandKind::free_form_edit;
    free_form.package_index_path = "runtime/validation.geindex";
    free_form.scene_asset_key = mirakana::AssetKeyV2{"assets/scenes/validation-level"};
    free_form.free_form_edit = "append raw package text";
    const auto free_form_result = mirakana::plan_runtime_scene_package_validation(free_form);
    MK_REQUIRE(!free_form_result.succeeded());
    MK_REQUIRE(failures_contain(free_form_result.diagnostics, "free-form edits are not supported"));
}

MK_TEST("scene v2 runtime package migration dry-runs scene and package index changes") {
    const auto request = make_runtime_migration_request();

    const auto result = mirakana::plan_scene_v2_runtime_package_migration(request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.scene_v1_content ==
               mirakana::serialize_scene(mirakana::deserialize_scene(result.scene_v1_content)));
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == request.output_scene_path);
    MK_REQUIRE(result.changed_files[0].document_kind == "GameEngine.Scene.v1");
    MK_REQUIRE(result.changed_files[0].content == result.scene_v1_content);
    MK_REQUIRE(result.changed_files[0].content_hash == mirakana::hash_asset_cooked_content(result.scene_v1_content));
    MK_REQUIRE(result.changed_files[1].path == request.package_index_path);
    MK_REQUIRE(result.changed_files[1].document_kind == "GameEngine.CookedPackageIndex.v1");
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    MK_REQUIRE(text_contains(result.scene_v1_content, "format=GameEngine.Scene.v1\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "scene.name=Migrated Level\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.count=2\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.2.parent=1\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.1.camera.projection=orthographic\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.1.camera.primary=true\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.2.light.type=point\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.2.light.color=0.5,0.25,1\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content, "node.2.sprite_renderer.visible=false\n"));

    const auto mesh = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/meshes/cube"});
    const auto material = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/base"});
    const auto sprite = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/hero"});
    MK_REQUIRE(
        text_contains(result.scene_v1_content, "node.1.mesh_renderer.mesh=" + std::to_string(mesh.value) + "\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content,
                             "node.1.mesh_renderer.material=" + std::to_string(material.value) + "\n"));
    MK_REQUIRE(
        text_contains(result.scene_v1_content, "node.2.sprite_renderer.sprite=" + std::to_string(sprite.value) + "\n"));
    MK_REQUIRE(text_contains(result.scene_v1_content,
                             "node.2.sprite_renderer.material=" + std::to_string(material.value) + "\n"));

    MK_REQUIRE(result.model_mutations.size() == 1);
    MK_REQUIRE(result.model_mutations[0].kind == "migrate_scene_v2_runtime_package");
    MK_REQUIRE(result.model_mutations[0].target_path == request.output_scene_path);
    MK_REQUIRE(result.model_mutations[0].scene_asset_key.value == request.scene_asset_key.value);
    MK_REQUIRE(result.model_mutations[0].scene_asset == mirakana::asset_id_from_key_v2(request.scene_asset_key));
    MK_REQUIRE(result.model_mutations[0].placement_rows.size() == 5);
    MK_REQUIRE(result.model_mutations[0].placement_rows[0].placement == "scene.component.mesh_renderer.material");
    MK_REQUIRE(result.model_mutations[0].placement_rows[0].key.value == "assets/materials/base");
    MK_REQUIRE(result.model_mutations[0].placement_rows[0].id == material);
    MK_REQUIRE(result.model_mutations[0].placement_rows[0].kind == mirakana::AssetKind::material);
    MK_REQUIRE(result.model_mutations[0].placement_rows[0].source_path == "source/materials/base.material");
    MK_REQUIRE(result.model_mutations[0].placement_rows[1].placement == "scene.component.mesh_renderer.mesh");
    MK_REQUIRE(result.model_mutations[0].placement_rows[1].key.value == "assets/meshes/cube");
    MK_REQUIRE(result.model_mutations[0].placement_rows[1].id == mesh);
    MK_REQUIRE(result.model_mutations[0].placement_rows[1].kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(result.model_mutations[0].placement_rows[1].source_path == "source/meshes/cube.gemesh");
    MK_REQUIRE(result.model_mutations[0].placement_rows[2].placement == "scene.component.sprite_renderer.material");
    MK_REQUIRE(result.model_mutations[0].placement_rows[2].key.value == "assets/materials/base");
    MK_REQUIRE(result.model_mutations[0].placement_rows[2].id == material);
    MK_REQUIRE(result.model_mutations[0].placement_rows[2].kind == mirakana::AssetKind::material);
    MK_REQUIRE(result.model_mutations[0].placement_rows[2].source_path == "source/materials/base.material");
    MK_REQUIRE(result.model_mutations[0].placement_rows[3].placement == "scene.component.sprite_renderer.sprite");
    MK_REQUIRE(result.model_mutations[0].placement_rows[3].key.value == "assets/textures/hero");
    MK_REQUIRE(result.model_mutations[0].placement_rows[3].id == sprite);
    MK_REQUIRE(result.model_mutations[0].placement_rows[3].kind == mirakana::AssetKind::texture);
    MK_REQUIRE(result.model_mutations[0].placement_rows[3].source_path == "source/textures/hero.getexture");
    MK_REQUIRE(result.model_mutations[0].placement_rows[4].placement == "scene.runtime_package");
    MK_REQUIRE(result.model_mutations[0].placement_rows[4].key.value == request.scene_asset_key.value);
    MK_REQUIRE(result.model_mutations[0].placement_rows[4].id ==
               mirakana::asset_id_from_key_v2(request.scene_asset_key));
    MK_REQUIRE(result.model_mutations[0].placement_rows[4].kind == mirakana::AssetKind::scene);
    MK_REQUIRE(result.model_mutations[0].placement_rows[4].source_path == request.scene_v2_path);
    MK_REQUIRE(result.model_mutations[0].dependency_rows.size() == 3);
    MK_REQUIRE(result.model_mutations[0].dependency_rows[0].kind == mirakana::AssetDependencyKind::scene_material);
    MK_REQUIRE(result.model_mutations[0].dependency_rows[1].kind == mirakana::AssetDependencyKind::scene_mesh);
    MK_REQUIRE(result.model_mutations[0].dependency_rows[2].kind == mirakana::AssetDependencyKind::scene_sprite);
    MK_REQUIRE(result.validation_recipes.size() == 3);
    MK_REQUIRE(result.unsupported_gap_ids.size() >= 3);
    MK_REQUIRE(result.undo_token == "placeholder-only");

    const auto index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto scene_asset = mirakana::asset_id_from_key_v2(request.scene_asset_key);
    const auto scene_entry =
        std::ranges::find_if(index.entries, [scene_asset](const auto& entry) { return entry.asset == scene_asset; });
    MK_REQUIRE(scene_entry != index.entries.end());
    MK_REQUIRE(scene_entry->kind == mirakana::AssetKind::scene);
    MK_REQUIRE(scene_entry->path == request.output_scene_path);
    auto expected_dependencies = std::vector<mirakana::AssetId>{mesh, material, sprite};
    std::ranges::sort(expected_dependencies,
                      [](mirakana::AssetId lhs, mirakana::AssetId rhs) { return lhs.value < rhs.value; });
    MK_REQUIRE(scene_entry->dependencies == expected_dependencies);
}

MK_TEST("scene v2 runtime package migration apply rereads validated paths and writes deterministic changed files") {
    mirakana::MemoryFileSystem fs;
    const auto request = make_runtime_migration_request();
    fs.write_text(request.scene_v2_path, request.scene_v2_content);
    fs.write_text(request.source_registry_path, request.source_registry_content);
    fs.write_text(request.package_index_path, request.package_index_content);

    auto apply_request = request;
    apply_request.scene_v2_content = "stale inline scene content must be ignored by apply";
    apply_request.source_registry_content = "stale inline registry content must be ignored by apply";
    apply_request.package_index_content = "stale inline package content must be ignored by apply";

    const auto result = mirakana::apply_scene_v2_runtime_package_migration(fs, apply_request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == request.output_scene_path);
    MK_REQUIRE(result.changed_files[1].path == request.package_index_path);
    MK_REQUIRE(fs.read_text(request.output_scene_path) == result.scene_v1_content);
    MK_REQUIRE(fs.read_text(request.package_index_path) == result.package_index_content);

    auto duplicate = request;
    duplicate.scene_v2_content = runtime_migration_scene_v2_with_duplicate_component_id_content();
    fs.write_text(request.scene_v2_path, duplicate.scene_v2_content);
    const auto scene_before = fs.read_text(request.output_scene_path);
    const auto index_before = fs.read_text(request.package_index_path);

    const auto failed = mirakana::apply_scene_v2_runtime_package_migration(fs, request);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.diagnostics, "duplicate component id"));
    MK_REQUIRE(fs.read_text(request.output_scene_path) == scene_before);
    MK_REQUIRE(fs.read_text(request.package_index_path) == index_before);
}

MK_TEST("scene v2 runtime package migration rejects source asset row and component mapping errors") {
    auto request = make_runtime_migration_request();
    auto missing_asset_scene = make_runtime_migration_scene_v2();
    missing_asset_scene.components[1].properties[0].value = "assets/meshes/missing";
    request.scene_v2_content = mirakana::serialize_scene_document_v2(missing_asset_scene);
    const auto missing_asset = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!missing_asset.succeeded());
    MK_REQUIRE(failures_contain(missing_asset.diagnostics, "source asset key is missing"));

    request = make_runtime_migration_request();
    auto wrong_registry = make_runtime_migration_source_registry();
    for (auto& row : wrong_registry.assets) {
        if (row.key.value == "assets/textures/hero") {
            row.kind = mirakana::AssetKind::material;
            row.source_path = "source/materials/hero.material";
            row.source_format = "GameEngine.Material.v1";
            row.imported_path = "runtime/assets/materials/hero.material";
        }
    }
    request.source_registry_content = mirakana::serialize_source_asset_registry_document(wrong_registry);
    const auto wrong_kind = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!wrong_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_kind.diagnostics, "must reference a texture source asset"));

    request = make_runtime_migration_request();
    request.scene_v2_content = runtime_migration_scene_v2_with_duplicate_component_id_content();
    const auto duplicate_component = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!duplicate_component.succeeded());
    MK_REQUIRE(failures_contain(duplicate_component.diagnostics, "duplicate component id"));

    request = make_runtime_migration_request();
    auto malformed_scene = make_runtime_migration_scene_v2();
    malformed_scene.components[0].properties[2].value = "not-a-number";
    request.scene_v2_content = mirakana::serialize_scene_document_v2(malformed_scene);
    const auto malformed = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!malformed.succeeded());
    MK_REQUIRE(failures_contain(malformed.diagnostics, "numeric value is invalid"));

    request = make_runtime_migration_request();
    auto unsupported_property_scene = make_runtime_migration_scene_v2();
    unsupported_property_scene.components[0].properties.push_back({.name = "aperture", .value = "2"});
    request.scene_v2_content = mirakana::serialize_scene_document_v2(unsupported_property_scene);
    const auto unsupported_property = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!unsupported_property.succeeded());
    MK_REQUIRE(failures_contain(unsupported_property.diagnostics, "unsupported component property"));

    request = make_runtime_migration_request();
    auto unsupported_component_scene = make_runtime_migration_scene_v2();
    unsupported_component_scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/root/tilemap"},
        .node = mirakana::AuthoringId{"node/root"},
        .type = mirakana::SceneComponentTypeId{"tilemap"},
        .properties = {{.name = "tileset", .value = "assets/textures/hero"}},
    });
    request.scene_v2_content = mirakana::serialize_scene_document_v2(unsupported_component_scene);
    const auto unsupported_component = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!unsupported_component.succeeded());
    MK_REQUIRE(failures_contain(unsupported_component.diagnostics, "unsupported component type"));
}

MK_TEST("scene v2 runtime package migration rejects unsafe paths unsupported claims and free form edits") {
    auto request = make_runtime_migration_request();
    request.scene_v2_path = "../source/scenes/level.scene";
    const auto unsafe = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.diagnostics, "safe repository-relative"));

    CountingFileSystem counting_fs;
    const auto unsafe_apply = mirakana::apply_scene_v2_runtime_package_migration(counting_fs, request);
    MK_REQUIRE(!unsafe_apply.succeeded());
    MK_REQUIRE(failures_contain(unsafe_apply.diagnostics, "safe repository-relative"));
    MK_REQUIRE(counting_fs.exists_calls == 0);
    MK_REQUIRE(counting_fs.read_calls == 0);
    MK_REQUIRE(counting_fs.write_calls == 0);

    request = make_runtime_migration_request();
    request.output_scene_path = "runtime/../assets/scenes/level.scene";
    const auto unsafe_output = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!unsafe_output.succeeded());
    MK_REQUIRE(failures_contain(unsafe_output.diagnostics, "package-relative"));

    request = make_runtime_migration_request();
    request.output_scene_path = request.scene_v2_path;
    const auto aliased_output = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!aliased_output.succeeded());
    MK_REQUIRE(failures_contain(aliased_output.diagnostics, "must not alias an input path"));

    CountingFileSystem alias_counting_fs;
    const auto aliased_apply = mirakana::apply_scene_v2_runtime_package_migration(alias_counting_fs, request);
    MK_REQUIRE(!aliased_apply.succeeded());
    MK_REQUIRE(failures_contain(aliased_apply.diagnostics, "must not alias an input path"));
    MK_REQUIRE(alias_counting_fs.read_calls == 0);
    MK_REQUIRE(alias_counting_fs.write_calls == 0);

    request = make_runtime_migration_request();
    request.scene_asset_key = mirakana::AssetKeyV2{"assets/scenes/bad key"};
    const auto invalid_scene_key = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!invalid_scene_key.succeeded());
    MK_REQUIRE(failures_contain(invalid_scene_key.diagnostics, "valid AssetKeyV2"));

    request = make_runtime_migration_request();
    request.package_cooking = "ready";
    request.dependent_asset_cooking = "ready";
    request.external_importer_execution = "ready";
    request.renderer_rhi_residency = "ready";
    request.package_streaming = "ready";
    request.material_graph = "ready";
    request.shader_graph = "ready";
    request.live_shader_generation = "ready";
    request.editor_productization = "ready";
    request.metal_readiness = "ready";
    request.public_native_rhi_handles = "ready";
    request.arbitrary_shell = "ready";
    request.free_form_edit = "ready";
    const auto unsupported = mirakana::plan_scene_v2_runtime_package_migration(request);
    MK_REQUIRE(!unsupported.succeeded());
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "package cooking is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "dependent asset cooking is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "external importer execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "renderer/RHI residency is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "package streaming is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "material graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "shader graph is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "live shader generation is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "editor productization is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "Metal readiness is host-gated"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "public native/RHI handles are not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "arbitrary shell execution is not supported"));
    MK_REQUIRE(failures_contain(unsupported.diagnostics, "free-form edits are not supported"));

    mirakana::SceneV2RuntimePackageMigrationRequest free_form;
    free_form.kind = mirakana::SceneV2RuntimePackageMigrationCommandKind::free_form_edit;
    free_form.scene_v2_path = "source/scenes/level.scene";
    free_form.source_registry_path = "source/assets/game.geassets";
    free_form.package_index_path = "runtime/game.geindex";
    free_form.output_scene_path = "runtime/assets/scenes/level.scene";
    free_form.free_form_edit = "append raw scene package text";
    const auto free_form_result = mirakana::plan_scene_v2_runtime_package_migration(free_form);
    MK_REQUIRE(!free_form_result.succeeded());
    MK_REQUIRE(failures_contain(free_form_result.diagnostics, "free-form edits are not supported"));
}

MK_TEST("scene v2 runtime package migration apply reports centralized scene package write failures") {
    auto request = make_runtime_migration_request();
    ThrowingWriteFileSystem fs;
    fs.files[request.scene_v2_path] = request.scene_v2_content;
    fs.files[request.source_registry_path] = request.source_registry_content;
    fs.files[request.package_index_path] = request.package_index_content;
    fs.failing_write_path = request.package_index_path;

    const auto result = mirakana::apply_scene_v2_runtime_package_migration(fs, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(failures_contain(result.diagnostics, "failed to write"));
    MK_REQUIRE(fs.write_calls >= 1);
}

MK_TEST("material instance apply tooling dry-runs material and package index changes") {
    mirakana::MaterialInstancePackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = material_instance_base_package_index_content();
    update.output_path = "runtime/assets/materials/blue.material";
    update.source_revision = 9;
    update.instance = make_material_instance_definition();

    const auto result = mirakana::plan_material_instance_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.material_content == mirakana::serialize_material_instance_definition(update.instance));
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == update.output_path);
    MK_REQUIRE(result.changed_files[0].content == result.material_content);
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto entry_it = std::ranges::find_if(updated_index.entries,
                                               [](const auto& entry) { return entry.asset == mirakana::AssetId{300}; });
    MK_REQUIRE(entry_it != updated_index.entries.end());
    MK_REQUIRE(entry_it->kind == mirakana::AssetKind::material);
    MK_REQUIRE(entry_it->path == update.output_path);
    MK_REQUIRE(entry_it->content_hash == mirakana::hash_asset_cooked_content(result.material_content));
    MK_REQUIRE(entry_it->source_revision == 9);
    MK_REQUIRE(entry_it->dependencies ==
               std::vector<mirakana::AssetId>({mirakana::AssetId{10}, mirakana::AssetId{20}}));

    std::size_t texture_edges = 0;
    for (const auto& edge : updated_index.dependencies) {
        if (edge.asset == mirakana::AssetId{300}) {
            MK_REQUIRE(edge.kind == mirakana::AssetDependencyKind::material_texture);
            MK_REQUIRE(edge.path == update.output_path);
            ++texture_edges;
        }
    }
    MK_REQUIRE(texture_edges == 2);
}

MK_TEST("material instance apply tooling applies only after validation passes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/sample.geindex", material_instance_base_package_index_content());

    mirakana::MaterialInstancePackageApplyDesc apply;
    apply.package_index_path = "runtime/sample.geindex";
    apply.output_path = "runtime/assets/materials/blue.material";
    apply.source_revision = 9;
    apply.instance = make_material_instance_definition();

    const auto result = mirakana::apply_material_instance_package_update(fs, apply);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text(apply.output_path) == result.material_content);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == result.package_index_content);

    auto invalid = apply;
    invalid.instance.texture_overrides.push_back(mirakana::MaterialTextureBinding{
        .slot = mirakana::MaterialTextureSlot::base_color, .texture = mirakana::AssetId{20}});
    const auto material_before = fs.read_text(apply.output_path);
    const auto index_before = fs.read_text(apply.package_index_path);
    const auto failed = mirakana::apply_material_instance_package_update(fs, invalid);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.failures, "material instance definition is invalid"));
    MK_REQUIRE(fs.read_text(apply.output_path) == material_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);
}

MK_TEST("material instance apply tooling rejects unsafe paths inconsistent rows and unsupported claims") {
    mirakana::MaterialInstancePackageUpdateDesc update;
    update.package_index_path = "runtime/sample.geindex";
    update.package_index_content = material_instance_base_package_index_content();
    update.output_path = "../blue.material";
    update.source_revision = 9;
    update.instance = make_material_instance_definition();

    const auto unsafe = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.failures, "package-relative"));

    update.output_path = "runtime/assets/materials/blue.material";
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{10},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/textures/albedo.texture",
                                              .content = "albedo",
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/textures/emissive.texture",
                                              .content = "emissive",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto missing_base = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!missing_base.succeeded());
    MK_REQUIRE(failures_contain(missing_base.failures, "base material package entry is missing"));

    auto index = mirakana::deserialize_asset_cooked_package_index(material_instance_base_package_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{20}) {
            entry.kind = mirakana::AssetKind::material;
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto non_texture = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!non_texture.succeeded());
    MK_REQUIRE(failures_contain(non_texture.failures, "texture override package entry is not a texture"));

    update.package_index_content = material_instance_base_package_index_content();
    update.material_graph = "ready";
    const auto material_graph = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!material_graph.succeeded());
    MK_REQUIRE(failures_contain(material_graph.failures, "material graph is not supported"));

    update.material_graph = "unsupported";
    update.shader_graph = "ready";
    const auto shader_graph = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!shader_graph.succeeded());
    MK_REQUIRE(failures_contain(shader_graph.failures, "shader graph is not supported"));

    update.shader_graph = "unsupported";
    update.live_shader_generation = "ready";
    const auto live_shader = mirakana::plan_material_instance_package_update(update);
    MK_REQUIRE(!live_shader.succeeded());
    MK_REQUIRE(failures_contain(live_shader.failures, "live shader generation is not supported"));
}

MK_TEST("material graph package tooling lowers graph into runtime material package row") {
    const auto update = make_material_graph_package_update_desc();
    const auto graph = make_material_graph_package_graph();
    const auto expected_material = mirakana::lower_material_graph_to_definition(graph);
    const auto expected_content = mirakana::serialize_material_definition(expected_material);

    const auto result = mirakana::plan_material_graph_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.material_content == expected_content);
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == update.output_path);
    MK_REQUIRE(result.changed_files[0].content == expected_content);
    MK_REQUIRE(result.changed_files[0].content_hash == mirakana::hash_asset_cooked_content(expected_content));
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto entry_it = std::ranges::find_if(updated_index.entries,
                                               [](const auto& entry) { return entry.asset == mirakana::AssetId{400}; });
    MK_REQUIRE(entry_it != updated_index.entries.end());
    MK_REQUIRE(entry_it->kind == mirakana::AssetKind::material);
    MK_REQUIRE(entry_it->path == update.output_path);
    MK_REQUIRE(entry_it->content_hash == mirakana::hash_asset_cooked_content(expected_content));
    MK_REQUIRE(entry_it->source_revision == update.source_revision);
    MK_REQUIRE(entry_it->dependencies ==
               std::vector<mirakana::AssetId>({mirakana::AssetId{10}, mirakana::AssetId{20}}));

    std::size_t texture_edges = 0;
    for (const auto& edge : updated_index.dependencies) {
        if (edge.asset != mirakana::AssetId{400}) {
            continue;
        }
        MK_REQUIRE(edge.kind == mirakana::AssetDependencyKind::material_texture);
        MK_REQUIRE(edge.path == update.output_path);
        MK_REQUIRE(edge.dependency == mirakana::AssetId{10} || edge.dependency == mirakana::AssetId{20});
        ++texture_edges;
    }
    MK_REQUIRE(texture_edges == 2);
}

MK_TEST("material graph package tooling applies only after validation passes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("source/materials/graph.materialgraph",
                  mirakana::serialize_material_graph(make_material_graph_package_graph()));
    fs.write_text("runtime/sample.geindex", material_graph_package_base_index_content());

    mirakana::MaterialGraphPackageApplyDesc apply;
    apply.package_index_path = "runtime/sample.geindex";
    apply.material_graph_path = "source/materials/graph.materialgraph";
    apply.output_path = "runtime/assets/materials/graph.material";
    apply.source_revision = 17;

    const auto result = mirakana::apply_material_graph_package_update(fs, apply);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text(apply.output_path) == result.material_content);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == result.package_index_content);

    auto invalid = apply;
    invalid.output_path = "../graph.material";
    const auto material_before = fs.read_text(apply.output_path);
    const auto index_before = fs.read_text(apply.package_index_path);
    const auto failed = mirakana::apply_material_graph_package_update(fs, invalid);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.failures, "package-relative"));
    MK_REQUIRE(fs.read_text(apply.output_path) == material_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);

    invalid = apply;
    invalid.material_graph_path = "../graph.materialgraph";
    const auto unsafe_graph_path = mirakana::apply_material_graph_package_update(fs, invalid);
    MK_REQUIRE(!unsafe_graph_path.succeeded());
    MK_REQUIRE(failures_contain(unsafe_graph_path.failures, "package-relative"));
    MK_REQUIRE(fs.read_text(apply.output_path) == material_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);
}

MK_TEST("material graph package tooling rejects inconsistent rows and unsupported claims") {
    auto update = make_material_graph_package_update_desc();
    update.output_path = "../graph.material";
    const auto unsafe = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.failures, "package-relative"));

    update = make_material_graph_package_update_desc();
    update.package_index_path = "../sample.geindex";
    const auto unsafe_index = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!unsafe_index.succeeded());
    MK_REQUIRE(failures_contain(unsafe_index.failures, "package-relative"));

    update = make_material_graph_package_update_desc();
    update.source_revision = 0;
    const auto zero_revision = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!zero_revision.succeeded());
    MK_REQUIRE(failures_contain(zero_revision.failures, "source revision must be non-zero"));

    update = make_material_graph_package_update_desc();
    update.material_graph_content = "format=GameEngine.MaterialGraph.v1\n";
    const auto malformed = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!malformed.succeeded());
    MK_REQUIRE(failures_contain(malformed.failures, "material graph is invalid"));

    update = make_material_graph_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{20},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/textures/emissive.texture",
                                              .content = "emissive",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto missing_texture = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!missing_texture.succeeded());
    MK_REQUIRE(failures_contain(missing_texture.failures, "texture package entry is missing"));

    update = make_material_graph_package_update_desc();
    auto index = mirakana::deserialize_asset_cooked_package_index(material_graph_package_base_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{20}) {
            entry.kind = mirakana::AssetKind::material;
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto wrong_kind = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!wrong_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_kind.failures, "texture package entry is not a texture"));

    update = make_material_graph_package_update_desc();
    index = mirakana::deserialize_asset_cooked_package_index(material_graph_package_base_index_content());
    index.entries.push_back(mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{401},
                                                              .kind = mirakana::AssetKind::material,
                                                              .path = update.output_path,
                                                              .content_hash = 7,
                                                              .source_revision = 1,
                                                              .dependencies = {}});
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto path_collision = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!path_collision.succeeded());
    MK_REQUIRE(failures_contain(path_collision.failures, "material graph output path is already used"));

    update = make_material_graph_package_update_desc();
    index = mirakana::deserialize_asset_cooked_package_index(material_graph_package_base_index_content());
    index.entries.push_back(mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{400},
                                                              .kind = mirakana::AssetKind::texture,
                                                              .path = update.output_path,
                                                              .content_hash = 7,
                                                              .source_revision = 1,
                                                              .dependencies = {}});
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto wrong_existing_kind = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!wrong_existing_kind.succeeded());
    MK_REQUIRE(
        failures_contain(wrong_existing_kind.failures, "existing material graph package entry kind must be material"));

    update = make_material_graph_package_update_desc();
    update.shader_graph = "ready";
    const auto shader_graph = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!shader_graph.succeeded());
    MK_REQUIRE(failures_contain(shader_graph.failures, "shader graph is not supported"));

    update = make_material_graph_package_update_desc();
    update.live_shader_generation = "ready";
    const auto live_shader = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!live_shader.succeeded());
    MK_REQUIRE(failures_contain(live_shader.failures, "live shader generation is not supported"));

    update = make_material_graph_package_update_desc();
    update.renderer_rhi_residency = "ready";
    const auto residency = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!residency.succeeded());
    MK_REQUIRE(failures_contain(residency.failures, "renderer/RHI residency is not supported"));

    update = make_material_graph_package_update_desc();
    update.package_streaming = "ready";
    const auto streaming = mirakana::plan_material_graph_package_update(update);
    MK_REQUIRE(!streaming.succeeded());
    MK_REQUIRE(failures_contain(streaming.failures, "package streaming is not supported"));
}

MK_TEST("scene package apply tooling dry-runs scene and package index changes") {
    auto update = make_scene_package_update_desc();

    const auto result = mirakana::plan_scene_package_update(update);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.scene_content == mirakana::serialize_scene(update.scene));
    MK_REQUIRE(result.changed_files.size() == 2);
    MK_REQUIRE(result.changed_files[0].path == update.output_path);
    MK_REQUIRE(result.changed_files[0].content == result.scene_content);
    MK_REQUIRE(result.changed_files[0].content_hash == mirakana::hash_asset_cooked_content(result.scene_content));
    MK_REQUIRE(result.changed_files[1].path == update.package_index_path);
    MK_REQUIRE(result.changed_files[1].content == result.package_index_content);

    const auto updated_index = mirakana::deserialize_asset_cooked_package_index(result.package_index_content);
    const auto entry_it = std::ranges::find_if(updated_index.entries,
                                               [](const auto& entry) { return entry.asset == mirakana::AssetId{901}; });
    MK_REQUIRE(entry_it != updated_index.entries.end());
    MK_REQUIRE(entry_it->kind == mirakana::AssetKind::scene);
    MK_REQUIRE(entry_it->path == update.output_path);
    MK_REQUIRE(entry_it->content_hash == mirakana::hash_asset_cooked_content(result.scene_content));
    MK_REQUIRE(entry_it->source_revision == update.source_revision);
    MK_REQUIRE(entry_it->dependencies == std::vector<mirakana::AssetId>(
                                             {mirakana::AssetId{101}, mirakana::AssetId{201}, mirakana::AssetId{301}}));

    bool saw_mesh_edge = false;
    bool saw_material_edge = false;
    bool saw_sprite_edge = false;
    for (const auto& edge : updated_index.dependencies) {
        if (edge.asset != mirakana::AssetId{901}) {
            continue;
        }
        if (edge.dependency == mirakana::AssetId{101}) {
            saw_mesh_edge = true;
            MK_REQUIRE(edge.kind == mirakana::AssetDependencyKind::scene_mesh);
            MK_REQUIRE(edge.path == "runtime/assets/scene/ship.mesh");
        } else if (edge.dependency == mirakana::AssetId{201}) {
            saw_material_edge = true;
            MK_REQUIRE(edge.kind == mirakana::AssetDependencyKind::scene_material);
            MK_REQUIRE(edge.path == "runtime/assets/scene/ship.material");
        } else if (edge.dependency == mirakana::AssetId{301}) {
            saw_sprite_edge = true;
            MK_REQUIRE(edge.kind == mirakana::AssetDependencyKind::scene_sprite);
            MK_REQUIRE(edge.path == "runtime/assets/scene/ship-sprite.texture.geasset");
        }
    }
    MK_REQUIRE(saw_mesh_edge);
    MK_REQUIRE(saw_material_edge);
    MK_REQUIRE(saw_sprite_edge);
}

MK_TEST("scene package apply tooling applies only after validation passes") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("runtime/sample.geindex", scene_package_base_index_content());

    mirakana::ScenePackageApplyDesc apply;
    apply.package_index_path = "runtime/sample.geindex";
    apply.output_path = "runtime/assets/scene/packaged.scene";
    apply.source_revision = 13;
    apply.scene_asset = mirakana::AssetId{901};
    apply.scene = make_scene_package_scene();
    apply.mesh_dependencies = {mirakana::AssetId{101}};
    apply.material_dependencies = {mirakana::AssetId{201}};
    apply.sprite_dependencies = {mirakana::AssetId{301}};

    const auto result = mirakana::apply_scene_package_update(fs, apply);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text(apply.output_path) == result.scene_content);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == result.package_index_content);

    auto invalid = apply;
    invalid.scene.find_node(mirakana::SceneNodeId{1})->transform.position.x = std::numeric_limits<float>::infinity();
    const auto scene_before = fs.read_text(apply.output_path);
    const auto index_before = fs.read_text(apply.package_index_path);
    const auto failed = mirakana::apply_scene_package_update(fs, invalid);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failures_contain(failed.failures, "scene transform must be finite"));
    MK_REQUIRE(fs.read_text(apply.output_path) == scene_before);
    MK_REQUIRE(fs.read_text(apply.package_index_path) == index_before);
}

MK_TEST("scene package apply tooling rejects duplicate paths missing and wrong dependency rows") {
    auto update = make_scene_package_update_desc();
    update.output_path = "../packaged.scene";
    const auto unsafe = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(failures_contain(unsafe.failures, "package-relative"));

    update = make_scene_package_update_desc();
    update.output_path = "runtime/assets/scene/ship.mesh";
    const auto path_collision = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!path_collision.succeeded());
    MK_REQUIRE(failures_contain(path_collision.failures, "scene output path collides"));

    update = make_scene_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{201},
                                              .kind = mirakana::AssetKind::material,
                                              .path = "runtime/assets/scene/ship.material",
                                              .content = "material",
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{301},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/scene/ship-sprite.texture.geasset",
                                              .content = "sprite",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto missing_mesh = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!missing_mesh.succeeded());
    MK_REQUIRE(failures_contain(missing_mesh.failures, "scene mesh package entry is missing"));

    update = make_scene_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{101},
                                              .kind = mirakana::AssetKind::mesh,
                                              .path = "runtime/assets/scene/ship.mesh",
                                              .content = "mesh",
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{301},
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "runtime/assets/scene/ship-sprite.texture.geasset",
                                              .content = "sprite",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto missing_material = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!missing_material.succeeded());
    MK_REQUIRE(failures_contain(missing_material.failures, "scene material package entry is missing"));

    update = make_scene_package_update_desc();
    update.package_index_content =
        mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{101},
                                              .kind = mirakana::AssetKind::mesh,
                                              .path = "runtime/assets/scene/ship.mesh",
                                              .content = "mesh",
                                              .source_revision = 1,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = mirakana::AssetId{201},
                                              .kind = mirakana::AssetKind::material,
                                              .path = "runtime/assets/scene/ship.material",
                                              .content = "material",
                                              .source_revision = 1,
                                              .dependencies = {}},
            },
            {}));
    const auto missing_sprite = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!missing_sprite.succeeded());
    MK_REQUIRE(failures_contain(missing_sprite.failures, "scene sprite package entry is missing"));

    update = make_scene_package_update_desc();
    auto index = mirakana::deserialize_asset_cooked_package_index(scene_package_base_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{101}) {
            entry.path = "../ship.mesh";
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto unsafe_dependency_path = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!unsafe_dependency_path.succeeded());
    MK_REQUIRE(
        failures_contain(unsafe_dependency_path.failures, "scene mesh package entry path must be package-relative"));

    update = make_scene_package_update_desc();
    index = mirakana::deserialize_asset_cooked_package_index(scene_package_base_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{201}) {
            entry.kind = mirakana::AssetKind::texture;
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto wrong_material_kind = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!wrong_material_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_material_kind.failures, "scene material package entry is not a material"));

    update = make_scene_package_update_desc();
    index = mirakana::deserialize_asset_cooked_package_index(scene_package_base_index_content());
    for (auto& entry : index.entries) {
        if (entry.asset == mirakana::AssetId{301}) {
            entry.kind = mirakana::AssetKind::material;
        }
    }
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto wrong_sprite_kind = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!wrong_sprite_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_sprite_kind.failures, "scene sprite package entry is not a texture"));

    update = make_scene_package_update_desc();
    index = mirakana::deserialize_asset_cooked_package_index(scene_package_base_index_content());
    index.entries.push_back(mirakana::AssetCookedPackageEntry{.asset = mirakana::AssetId{901},
                                                              .kind = mirakana::AssetKind::material,
                                                              .path = "runtime/assets/scene/existing.material",
                                                              .content_hash = 7,
                                                              .source_revision = 1,
                                                              .dependencies = {}});
    update.package_index_content = mirakana::serialize_asset_cooked_package_index(index);
    const auto wrong_existing_kind = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!wrong_existing_kind.succeeded());
    MK_REQUIRE(failures_contain(wrong_existing_kind.failures, "existing scene package entry kind must be scene"));
}

MK_TEST("scene package apply tooling rejects malformed scene payloads and unsupported claims") {
    auto update = make_scene_package_update_desc();
    update.scene.find_node(mirakana::SceneNodeId{1})->components.mesh_renderer->mesh = mirakana::AssetId{};

    const auto malformed = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!malformed.succeeded());
    MK_REQUIRE(failures_contain(malformed.failures, "scene payload is invalid"));

    update = make_scene_package_update_desc();
    update.editor_productization = "ready";
    const auto editor = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!editor.succeeded());
    MK_REQUIRE(failures_contain(editor.failures, "editor productization is not supported"));

    update = make_scene_package_update_desc();
    update.prefab_mutation = "ready";
    const auto prefab = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!prefab.succeeded());
    MK_REQUIRE(failures_contain(prefab.failures, "prefab mutation is not supported"));

    update = make_scene_package_update_desc();
    update.runtime_source_import = "ready";
    const auto source_import = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!source_import.succeeded());
    MK_REQUIRE(failures_contain(source_import.failures, "runtime source import is not supported"));

    update = make_scene_package_update_desc();
    update.renderer_rhi_residency = "ready";
    const auto residency = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!residency.succeeded());
    MK_REQUIRE(failures_contain(residency.failures, "renderer/RHI residency is not supported"));

    update = make_scene_package_update_desc();
    update.package_streaming = "ready";
    const auto streaming = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!streaming.succeeded());
    MK_REQUIRE(failures_contain(streaming.failures, "package streaming is not supported"));

    update = make_scene_package_update_desc();
    update.material_graph = "ready";
    const auto material_graph = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!material_graph.succeeded());
    MK_REQUIRE(failures_contain(material_graph.failures, "material graph is not supported"));

    update = make_scene_package_update_desc();
    update.shader_graph = "ready";
    const auto shader_graph = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!shader_graph.succeeded());
    MK_REQUIRE(failures_contain(shader_graph.failures, "shader graph is not supported"));

    update = make_scene_package_update_desc();
    update.live_shader_generation = "ready";
    const auto live_shader = mirakana::plan_scene_package_update(update);
    MK_REQUIRE(!live_shader.succeeded());
    MK_REQUIRE(failures_contain(live_shader.failures, "live shader generation is not supported"));
}

MK_TEST("asset package assembly builds runtime-loadable index from imported scene package") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/package.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/package.mesh");
    const auto material_id = mirakana::AssetId::from_name("materials/package.material");
    const auto scene_id = mirakana::AssetId::from_name("scenes/package.scene");

    fs.write_text("source/textures/package.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                            "texture.width=1\n"
                                                            "texture.height=1\n"
                                                            "texture.pixel_format=rgba8_unorm\n");
    fs.write_text("source/meshes/package.mesh_source", "format=GameEngine.MeshSource.v2\n"
                                                       "mesh.vertex_count=3\n"
                                                       "mesh.index_count=3\n"
                                                       "mesh.has_normals=false\n"
                                                       "mesh.has_uvs=false\n"
                                                       "mesh.has_tangent_frame=false\n");
    fs.write_text("source/materials/package.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "PackageMaterial",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));
    fs.write_text("source/scenes/package.scene",
                  "format=GameEngine.Scene.v1\n"
                  "scene.name=Packaged\n"
                  "node.count=1\n"
                  "node.1.name=Root\n"
                  "node.1.parent=0\n"
                  "node.1.position=0,0,0\n"
                  "node.1.scale=1,1,1\n"
                  "node.1.rotation=0,0,0\n"
                  "node.1.mesh_renderer.mesh=" +
                      std::to_string(mesh_id.value) + "\nnode.1.mesh_renderer.material=" +
                      std::to_string(material_id.value) + "\nnode.1.mesh_renderer.visible=true\n");

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/package.texture_source",
        .imported_path = "assets/textures/package.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/package.mesh_source",
        .imported_path = "assets/meshes/package.mesh",
        .scale = 1.0F,
        .generate_lods = false,
        .generate_collision = true,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/package.material",
        .imported_path = "assets/materials/package.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_scene(mirakana::SceneImportMetadata{
        .id = scene_id,
        .source_path = "source/scenes/package.scene",
        .imported_path = "assets/scenes/package.scene",
        .mesh_dependencies = {mesh_id},
        .material_dependencies = {material_id},
        .sprite_dependencies = {},
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    const auto import_result = mirakana::execute_asset_import_plan(fs, plan);
    MK_REQUIRE(import_result.succeeded());

    const auto assembly = mirakana::assemble_asset_cooked_package(fs, plan, import_result);
    MK_REQUIRE(assembly.succeeded());
    MK_REQUIRE(assembly.index.entries.size() == 4);
    MK_REQUIRE(assembly.index.dependencies.size() == 3);

    mirakana::write_asset_cooked_package_index(fs, "assets/package.geindex", assembly.index);
    const auto load = mirakana::runtime::load_runtime_asset_package(
        fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "assets/package.geindex", .content_root = ""});
    MK_REQUIRE(load.succeeded());
    MK_REQUIRE(load.package.records().size() == 4);

    const auto* scene_record = load.package.find(scene_id);
    MK_REQUIRE(scene_record != nullptr);
    MK_REQUIRE(scene_record->kind == mirakana::AssetKind::scene);
    MK_REQUIRE(scene_record->dependencies.size() == 2);
    MK_REQUIRE(scene_record->dependencies[0] == mesh_id);
    MK_REQUIRE(scene_record->dependencies[1] == material_id);

    const auto scene_payload = mirakana::runtime::runtime_scene_payload(*scene_record);
    MK_REQUIRE(scene_payload.succeeded());
    MK_REQUIRE(scene_payload.payload.name == "Packaged");
}

MK_TEST("asset package assembly rejects failed imports missing artifacts duplicates and package-external deps") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/package_missing");
    const auto material_id = mirakana::AssetId::from_name("materials/package_external");
    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/package_missing.texture_source",
        .output_path = "assets/textures/package_missing.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = material_id,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "source/materials/package_external.material",
        .output_path = "assets/materials/package_external.material",
        .dependencies = {texture_id},
    });
    plan.dependencies.push_back(mirakana::AssetDependencyEdge{
        .asset = material_id,
        .dependency = texture_id,
        .kind = mirakana::AssetDependencyKind::material_texture,
        .path = "assets/textures/package_missing.texture",
    });

    mirakana::AssetImportExecutionResult failed_import;
    failed_import.failures.push_back(mirakana::AssetImportFailure{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/package_missing.texture_source",
        .output_path = "assets/textures/package_missing.texture",
        .diagnostic = "decode failed",
    });
    const auto failed_assembly = mirakana::assemble_asset_cooked_package(fs, plan, failed_import);
    MK_REQUIRE(!failed_assembly.succeeded());
    MK_REQUIRE(failed_assembly.failures.size() == 1);
    MK_REQUIRE(failed_assembly.failures[0].diagnostic.find("decode failed") != std::string::npos);

    mirakana::AssetImportExecutionResult missing_artifact;
    missing_artifact.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_missing.texture",
    });
    const auto missing_assembly = mirakana::assemble_asset_cooked_package(fs, plan, missing_artifact);
    MK_REQUIRE(!missing_assembly.succeeded());
    bool found_missing_artifact = false;
    for (const auto& failure : missing_assembly.failures) {
        found_missing_artifact =
            found_missing_artifact || failure.diagnostic.find("missing cooked artifact") != std::string::npos;
    }
    MK_REQUIRE(found_missing_artifact);

    fs.write_text("assets/textures/package_missing.texture",
                  "format=GameEngine.CookedTexture.v1\n"
                  "asset.id=" +
                      std::to_string(texture_id.value) +
                      "\nasset.kind=texture\n"
                      "source.path=source/textures/package_missing.texture_source\n"
                      "texture.width=1\n"
                      "texture.height=1\n"
                      "texture.pixel_format=rgba8_unorm\n"
                      "texture.source_bytes=4\n");
    mirakana::AssetImportExecutionResult duplicate_import;
    duplicate_import.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_missing.texture",
    });
    duplicate_import.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_missing.texture",
    });
    const auto duplicate_assembly = mirakana::assemble_asset_cooked_package(fs, plan, duplicate_import);
    MK_REQUIRE(!duplicate_assembly.succeeded());
    bool found_duplicate = false;
    for (const auto& failure : duplicate_assembly.failures) {
        found_duplicate = found_duplicate || failure.diagnostic.find("duplicated") != std::string::npos;
    }
    MK_REQUIRE(found_duplicate);

    fs.write_text("assets/materials/package_external.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "PackageExternal",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));
    mirakana::AssetImportExecutionResult package_external;
    package_external.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = material_id,
        .kind = mirakana::AssetImportActionKind::material,
        .output_path = "assets/materials/package_external.material",
    });
    const auto external_assembly = mirakana::assemble_asset_cooked_package(fs, plan, package_external);
    MK_REQUIRE(!external_assembly.succeeded());
    bool found_package_failure = false;
    for (const auto& failure : external_assembly.failures) {
        found_package_failure = found_package_failure || failure.diagnostic.find("package") != std::string::npos;
    }
    MK_REQUIRE(found_package_failure);
}

MK_TEST("asset package assembly rejects incomplete imports and missing declared dependency edges") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/package_complete");
    const auto material_id = mirakana::AssetId::from_name("materials/package_edge");
    const auto audio_id = mirakana::AssetId::from_name("audio/package_complete");

    fs.write_text("assets/textures/package_complete.texture",
                  "format=GameEngine.CookedTexture.v1\n"
                  "asset.id=" +
                      std::to_string(texture_id.value) +
                      "\nasset.kind=texture\n"
                      "source.path=source/textures/package_complete.texture_source\n"
                      "texture.width=1\n"
                      "texture.height=1\n"
                      "texture.pixel_format=rgba8_unorm\n"
                      "texture.source_bytes=4\n");
    fs.write_text("assets/materials/package_edge.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "PackageEdge",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    mirakana::AssetImportPlan incomplete_plan;
    incomplete_plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/package_complete.texture_source",
        .output_path = "assets/textures/package_complete.texture",
        .dependencies = {},
    });
    incomplete_plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/package_complete.audio_source",
        .output_path = "assets/audio/package_complete.audio",
        .dependencies = {},
    });

    mirakana::AssetImportExecutionResult incomplete_import;
    incomplete_import.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_complete.texture",
    });
    const auto incomplete_assembly = mirakana::assemble_asset_cooked_package(fs, incomplete_plan, incomplete_import);
    MK_REQUIRE(!incomplete_assembly.succeeded());
    MK_REQUIRE(incomplete_assembly.failures.size() == 1);
    MK_REQUIRE(incomplete_assembly.failures[0].asset == audio_id);
    MK_REQUIRE(incomplete_assembly.failures[0].diagnostic.find("missing imported artifact") != std::string::npos);

    mirakana::AssetImportPlan missing_edge_plan;
    missing_edge_plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/package_complete.texture_source",
        .output_path = "assets/textures/package_complete.texture",
        .dependencies = {},
    });
    missing_edge_plan.actions.push_back(mirakana::AssetImportAction{
        .id = material_id,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "source/materials/package_edge.material",
        .output_path = "assets/materials/package_edge.material",
        .dependencies = {texture_id},
    });

    mirakana::AssetImportExecutionResult missing_edge_import;
    missing_edge_import.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_complete.texture",
    });
    missing_edge_import.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = material_id,
        .kind = mirakana::AssetImportActionKind::material,
        .output_path = "assets/materials/package_edge.material",
    });
    const auto missing_edge_assembly =
        mirakana::assemble_asset_cooked_package(fs, missing_edge_plan, missing_edge_import);
    MK_REQUIRE(!missing_edge_assembly.succeeded());
    MK_REQUIRE(missing_edge_assembly.failures.size() == 1);
    MK_REQUIRE(missing_edge_assembly.failures[0].asset == material_id);
    MK_REQUIRE(missing_edge_assembly.failures[0].diagnostic.find("dependency edge") != std::string::npos);
}

MK_TEST("asset package assembly reports filesystem exists failures as assembly failures") {
    ThrowingExistsFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/package_exists_failure");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/package_exists_failure.texture_source",
        .output_path = "assets/textures/package_exists_failure.texture",
        .dependencies = {},
    });

    mirakana::AssetImportExecutionResult import_result;
    import_result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .output_path = "assets/textures/package_exists_failure.texture",
    });

    const auto assembly = mirakana::assemble_asset_cooked_package(fs, plan, import_result);
    MK_REQUIRE(!assembly.succeeded());
    MK_REQUIRE(assembly.failures.size() == 1);
    MK_REQUIRE(assembly.failures[0].asset == texture_id);
    MK_REQUIRE(assembly.failures[0].diagnostic.find("exists denied") != std::string::npos);
}

MK_TEST("asset import executor rejects malformed scene sources without artifacts") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/rollback_albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/rollback");
    const auto material_id = mirakana::AssetId::from_name("materials/rollback");
    const auto scene_id = mirakana::AssetId::from_name("scenes/broken");
    fs.write_text("source/textures/rollback.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                             "texture.width=1\n"
                                                             "texture.height=1\n"
                                                             "texture.pixel_format=rgba8_unorm\n");
    fs.write_text("source/meshes/rollback.mesh_source", "format=GameEngine.MeshSource.v2\n"
                                                        "mesh.vertex_count=3\n"
                                                        "mesh.index_count=3\n"
                                                        "mesh.has_normals=false\n"
                                                        "mesh.has_uvs=false\n"
                                                        "mesh.has_tangent_frame=false\n");
    fs.write_text("source/materials/rollback.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Rollback",
                      .shading_model = mirakana::MaterialShadingModel::unlit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));
    fs.write_text("source/scenes/broken.scene", "format=GameEngine.Scene.v1\n"
                                                "scene.name=Broken\n"
                                                "node.count=1\n"
                                                "node.1.name=Root\n"
                                                "node.1.parent=99\n"
                                                "node.1.position=0,0,0\n"
                                                "node.1.scale=1,1,1\n");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/rollback.texture_source",
        .output_path = "assets/textures/rollback.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/rollback.mesh_source",
        .output_path = "assets/meshes/rollback.mesh",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = material_id,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "source/materials/rollback.material",
        .output_path = "assets/materials/rollback.material",
        .dependencies = {texture_id},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = scene_id,
        .kind = mirakana::AssetImportActionKind::scene,
        .source_path = "source/scenes/broken.scene",
        .output_path = "assets/scenes/broken.scene",
        .dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.imported.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == scene_id);
    MK_REQUIRE(result.failures[0].diagnostic.find("scene") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/textures/rollback.texture"));
    MK_REQUIRE(!fs.exists("assets/meshes/rollback.mesh"));
    MK_REQUIRE(!fs.exists("assets/materials/rollback.material"));
    MK_REQUIRE(!fs.exists("assets/scenes/broken.scene"));
}

MK_TEST("asset import executor preserves first-party source byte payloads") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/test_pixels");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/test_triangle");
    const auto audio_id = mirakana::AssetId::from_name("audio/test_click");

    fs.write_text("source/textures/test.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                         "texture.width=2\n"
                                                         "texture.height=1\n"
                                                         "texture.pixel_format=rgba8_unorm\n"
                                                         "texture.data_hex=0001020304050607\n");
    fs.write_text("source/meshes/test.mesh_source",
                  "format=GameEngine.MeshSource.v2\n"
                  "mesh.vertex_count=3\n"
                  "mesh.index_count=3\n"
                  "mesh.has_normals=false\n"
                  "mesh.has_uvs=false\n"
                  "mesh.has_tangent_frame=false\n"
                  "mesh.vertex_data_hex="
                  "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223\n"
                  "mesh.index_data_hex=000000000100000002000000\n");
    fs.write_text("source/audio/test.audio_source", "format=GameEngine.AudioSource.v1\n"
                                                    "audio.sample_rate=48000\n"
                                                    "audio.channel_count=2\n"
                                                    "audio.frame_count=1\n"
                                                    "audio.sample_format=pcm16\n"
                                                    "audio.data_hex=00010203\n");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/test.texture_source",
        .output_path = "assets/textures/test.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/test.mesh_source",
        .output_path = "assets/meshes/test.mesh",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/test.audio_source",
        .output_path = "assets/audio/test.audio",
        .dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, plan);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(fs.read_text("assets/textures/test.texture").find("texture.data_hex=0001020304050607\n") !=
               std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/test.mesh")
                   .find("mesh.vertex_data_hex="
                         "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223\n") !=
               std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/test.mesh").find("mesh.index_data_hex=000000000100000002000000\n") !=
               std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/test.audio").find("audio.data_hex=00010203\n") != std::string::npos);
}

MK_TEST("asset import executor rejects invalid first-party byte payloads without artifacts") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/bad_pixels");
    const auto audio_id = mirakana::AssetId::from_name("audio/bad_samples");

    fs.write_text("source/textures/bad.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                        "texture.width=2\n"
                                                        "texture.height=1\n"
                                                        "texture.pixel_format=rgba8_unorm\n"
                                                        "texture.data_hex=00010203\n");
    fs.write_text("source/audio/bad.audio_source", "format=GameEngine.AudioSource.v1\n"
                                                   "audio.sample_rate=48000\n"
                                                   "audio.channel_count=2\n"
                                                   "audio.frame_count=1\n"
                                                   "audio.sample_format=pcm16\n"
                                                   "audio.data_hex=zz\n");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/bad.texture_source",
        .output_path = "assets/textures/bad.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/bad.audio_source",
        .output_path = "assets/audio/bad.audio",
        .dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.imported.empty());
    MK_REQUIRE(result.failures.size() == 2);
    MK_REQUIRE(!fs.exists("assets/textures/bad.texture"));
    MK_REQUIRE(!fs.exists("assets/audio/bad.audio"));
}

MK_TEST("asset import executor routes external sources through tool import adapters") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");

    fs.write_text("source/textures/player.png", "fake-png-bytes");
    fs.write_text("source/meshes/player.gltf", "fake-gltf-json");
    fs.write_text("source/audio/hit.wav", "fake-wav-bytes");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/player.png",
        .output_path = "assets/textures/player.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/player.gltf",
        .output_path = "assets/meshes/player.mesh",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/hit.wav",
        .output_path = "assets/audio/hit.audio",
        .dependencies = {},
    });

    FakeExternalAssetImporter importer;
    mirakana::AssetImportExecutionOptions options;
    options.external_importers.push_back(&importer);

    const auto result = mirakana::execute_asset_import_plan(fs, plan, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.imported.size() == 3);
    MK_REQUIRE(importer.calls == 3);
    MK_REQUIRE(fs.read_text("assets/textures/player.texture").find("texture.width=64\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/player.mesh").find("mesh.vertex_count=12\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/hit.audio").find("audio.frame_count=100\n") != std::string::npos);
}

MK_TEST("asset import executor rejects external source without a tool adapter") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    fs.write_text("source/textures/player.png", "fake-png-bytes");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/player.png",
        .output_path = "assets/textures/player.texture",
        .dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.imported.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("unsupported") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/textures/player.texture"));
}

MK_TEST("default external asset import adapters expose optional feature boundary") {
    mirakana::ExternalAssetImportAdapters adapters;
    auto options = adapters.options();

    MK_REQUIRE(options.external_importers.size() == 4);
    MK_REQUIRE(adapters.png_textures.supports(mirakana::AssetImportAction{
        mirakana::AssetId::from_name("textures/player"),
        mirakana::AssetImportActionKind::texture,
        "source/textures/player.png",
        "assets/textures/player.texture",
        {},
    }));
    MK_REQUIRE(adapters.gltf_meshes.supports(mirakana::AssetImportAction{
        mirakana::AssetId::from_name("meshes/player"),
        mirakana::AssetImportActionKind::mesh,
        "source/meshes/player.gltf",
        "assets/meshes/player.mesh",
        {},
    }));
    MK_REQUIRE(adapters.gltf_morph_meshes_cpu.supports(mirakana::AssetImportAction{
        mirakana::AssetId::from_name("meshes/morph"),
        mirakana::AssetImportActionKind::morph_mesh_cpu,
        "source/meshes/morph.gltf",
        "assets/meshes/morph.morph_mesh_cpu",
        {},
    }));
    MK_REQUIRE(adapters.audio_sources.supports(mirakana::AssetImportAction{
        mirakana::AssetId::from_name("audio/hit"),
        mirakana::AssetImportActionKind::audio,
        "source/audio/hit.wav",
        "assets/audio/hit.audio",
        {},
    }));
    MK_REQUIRE(!adapters.audio_sources.supports(mirakana::AssetImportAction{
        mirakana::AssetId::from_name("audio/hit"),
        mirakana::AssetImportActionKind::audio,
        "source/audio/hit.ogg",
        "assets/audio/hit.audio",
        {},
    }));

    if (!mirakana::external_asset_importers_available()) {
        mirakana::MemoryFileSystem fs;
        const auto texture_id = mirakana::AssetId::from_name("textures/player");
        fs.write_text("source/textures/player.png", "fake-png-bytes");
        mirakana::AssetImportPlan plan;
        plan.actions.push_back(mirakana::AssetImportAction{
            .id = texture_id,
            .kind = mirakana::AssetImportActionKind::texture,
            .source_path = "source/textures/player.png",
            .output_path = "assets/textures/player.texture",
            .dependencies = {},
        });

        const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

        MK_REQUIRE(!result.succeeded());
        MK_REQUIRE(result.failures.size() == 1);
        MK_REQUIRE(result.failures[0].diagnostic.find("asset-importers feature is disabled") != std::string::npos);
    }
}

MK_TEST("default external asset import adapters decode audited dependency formats when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/white");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/triangle");
    const auto audio_id = mirakana::AssetId::from_name("audio/click");

    fs.write_text(
        "source/textures/white.png",
        byte_string({
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00,
            0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x01,
            0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
        }));
    fs.write_text("source/audio/click.wav",
                  byte_string({
                      0x52, 0x49, 0x46, 0x46, 0x28, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
                      0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x10, 0xB1, 0x02, 0x00,
                      0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  }));
    std::string triangle_buffer;
    append_le_f32(triangle_buffer, 1.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 2.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 3.0F);
    append_le_u16(triangle_buffer, 2U);
    append_le_u16(triangle_buffer, 1U);
    append_le_u16(triangle_buffer, 0U);
    fs.write_text("source/meshes/triangle.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(triangle_buffer.size()) + R"(,"uri":")" + gltf_data_uri(triangle_buffer) +
                      "\"}],"
                      "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}],"
                      "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                      "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/white.png",
        .output_path = "assets/textures/white.texture",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/triangle.gltf",
        .output_path = "assets/meshes/triangle.mesh",
        .dependencies = {},
    });
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/click.wav",
        .output_path = "assets/audio/click.audio",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    if (!result.succeeded()) {
        throw std::runtime_error(result.failures.empty() ? "external asset import failed without diagnostics"
                                                         : result.failures[0].diagnostic);
    }
    MK_REQUIRE(fs.read_text("assets/textures/white.texture").find("texture.width=1\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/textures/white.texture").find("texture.data_hex=00000000\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/triangle.mesh").find("mesh.vertex_count=3\n") != std::string::npos);
    MK_REQUIRE(
        fs.read_text("assets/meshes/triangle.mesh")
            .find("mesh.vertex_data_hex=0000803f0000000000000000000000000000004000000000000000000000000000004040\n") !=
        std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/triangle.mesh").find("mesh.index_data_hex=020000000100000000000000\n") !=
               std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/click.audio").find("audio.sample_rate=44100\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/audio/click.audio").find("audio.data_hex=0000000000000000\n") != std::string::npos);
}

MK_TEST("default external gltf importer counts mixed indexed and non indexed primitives when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto mesh_id = mirakana::AssetId::from_name("meshes/mixed");
    std::string mixed_buffer;
    append_le_f32(mixed_buffer, 1.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 2.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 3.0F);
    append_le_u16(mixed_buffer, 2U);
    append_le_u16(mixed_buffer, 0U);
    append_le_u16(mixed_buffer, 1U);
    append_le_f32(mixed_buffer, 10.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 11.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 12.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 13.0F);
    append_le_f32(mixed_buffer, 0.0F);
    append_le_f32(mixed_buffer, 0.0F);
    fs.write_text("source/meshes/mixed.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(mixed_buffer.size()) + R"(,"uri":")" + gltf_data_uri(mixed_buffer) +
                      "\"}],"
                      "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6},"
                      "{\"buffer\":0,\"byteOffset\":42,\"byteLength\":48}],"
                      "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},"
                      "{\"bufferView\":2,\"componentType\":5126,\"count\":4,\"type\":\"VEC3\"}],"
                      "\"meshes\":[{\"primitives\":["
                      "{\"attributes\":{\"POSITION\":0},\"indices\":1},"
                      "{\"attributes\":{\"POSITION\":2}}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/mixed.gltf",
        .output_path = "assets/meshes/mixed.mesh",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    if (!result.succeeded()) {
        throw std::runtime_error(result.failures.empty() ? "external glTF import failed without diagnostics"
                                                         : result.failures[0].diagnostic);
    }
    MK_REQUIRE(fs.read_text("assets/meshes/mixed.mesh").find("mesh.vertex_count=7\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/mixed.mesh").find("mesh.index_count=7\n") != std::string::npos);
    MK_REQUIRE(fs.read_text("assets/meshes/mixed.mesh")
                   .find("mesh.index_data_hex=02000000000000000100000003000000040000000500000006000000\n") !=
               std::string::npos);
}

MK_TEST("default external gltf importer cooks interleaved position normal uv vertices when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto mesh_id = mirakana::AssetId::from_name("meshes/lit-triangle");
    std::string source_buffer;
    std::string expected_vertices;
    const auto append_vertex = [&](float px, float py, float pz, float nx, float ny, float nz, float u, float v,
                                   float tx, float ty, float tz, float tw) {
        append_le_f32(expected_vertices, px);
        append_le_f32(expected_vertices, py);
        append_le_f32(expected_vertices, pz);
        append_le_f32(expected_vertices, nx);
        append_le_f32(expected_vertices, ny);
        append_le_f32(expected_vertices, nz);
        append_le_f32(expected_vertices, u);
        append_le_f32(expected_vertices, v);
        append_le_f32(expected_vertices, tx);
        append_le_f32(expected_vertices, ty);
        append_le_f32(expected_vertices, tz);
        append_le_f32(expected_vertices, tw);
    };

    append_le_f32(source_buffer, -1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    for (int tangent_vertex = 0; tangent_vertex < 3; ++tangent_vertex) {
        append_le_f32(source_buffer, 1.0F);
        append_le_f32(source_buffer, 0.0F);
        append_le_f32(source_buffer, 0.0F);
        append_le_f32(source_buffer, 1.0F);
    }
    append_le_u16(source_buffer, 2U);
    append_le_u16(source_buffer, 1U);
    append_le_u16(source_buffer, 0U);
    append_vertex(-1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F);
    append_vertex(1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F);
    append_vertex(0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F);

    fs.write_text("source/meshes/lit-triangle.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(source_buffer.size()) + R"(,"uri":")" + gltf_data_uri(source_buffer) +
                      "\"}],"
                      "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":72,\"byteLength\":24},"
                      "{\"buffer\":0,\"byteOffset\":96,\"byteLength\":48},"
                      "{\"buffer\":0,\"byteOffset\":144,\"byteLength\":6}],"
                      "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC2\"},"
                      "{\"bufferView\":3,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},"
                      "{\"bufferView\":4,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                      "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,"
                      "\"TEXCOORD_0\":2,\"TANGENT\":3},\"indices\":4}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/lit-triangle.gltf",
        .output_path = "assets/meshes/lit-triangle.mesh",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    if (!result.succeeded()) {
        throw std::runtime_error(result.failures.empty() ? "external lit glTF import failed without diagnostics"
                                                         : result.failures[0].diagnostic);
    }
    const auto cooked = fs.read_text("assets/meshes/lit-triangle.mesh");
    MK_REQUIRE(cooked.find("mesh.has_normals=true\n") != std::string::npos);
    MK_REQUIRE(cooked.find("mesh.has_uvs=true\n") != std::string::npos);
    MK_REQUIRE(cooked.find("mesh.has_tangent_frame=true\n") != std::string::npos);
    MK_REQUIRE(cooked.find("mesh.vertex_data_hex=" + hex_string(expected_vertices) + "\n") != std::string::npos);
    MK_REQUIRE(cooked.find("mesh.index_data_hex=020000000100000000000000\n") != std::string::npos);
}

MK_TEST("default external gltf importer rejects partial lit vertex attributes when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto mesh_id = mirakana::AssetId::from_name("meshes/partial-lit");
    std::string source_buffer;
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 0.0F);
    append_le_f32(source_buffer, 1.0F);
    fs.write_text("source/meshes/partial-lit.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(source_buffer.size()) + R"(,"uri":")" + gltf_data_uri(source_buffer) +
                      "\"}],"
                      "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":36}],"
                      "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"}],"
                      "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1}}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/partial-lit.gltf",
        .output_path = "assets/meshes/partial-lit.mesh",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("NORMAL and TEXCOORD_0") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/meshes/partial-lit.mesh"));
}

MK_TEST("default external gltf importer rejects triangle primitives without position payload when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto mesh_id = mirakana::AssetId::from_name("meshes/missing-position");
    std::string index_buffer;
    append_le_u16(index_buffer, 0U);
    append_le_u16(index_buffer, 1U);
    append_le_u16(index_buffer, 2U);
    fs.write_text("source/meshes/missing-position.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(index_buffer.size()) + R"(,"uri":")" + gltf_data_uri(index_buffer) +
                      "\"}],"
                      "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":6}],"
                      "\"accessors\":[{\"bufferView\":0,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                      "\"meshes\":[{\"primitives\":[{\"attributes\":{},\"indices\":0}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/missing-position.gltf",
        .output_path = "assets/meshes/missing-position.mesh",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("POSITION") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/meshes/missing-position.mesh"));
}

MK_TEST("default external audio importer rejects oversized decoded payloads when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::MemoryFileSystem fs;
    const auto audio_id = mirakana::AssetId::from_name("audio/too-long");
    fs.write_text("source/audio/too-long.wav", wav_pcm16_stereo_with_data_bytes((32U * 1024U * 1024U) + 4U));

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = audio_id,
        .kind = mirakana::AssetImportActionKind::audio,
        .source_path = "source/audio/too-long.wav",
        .output_path = "assets/audio/too-long.audio",
        .dependencies = {},
    });

    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("decoded audio size exceeds importer limits") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/audio/too-long.audio"));
}

MK_TEST("asset import executor reports missing source without partial success") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/missing");
    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/missing.txt",
        .output_path = "assets/textures/missing.texture",
        .dependencies = {},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.imported.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].asset == texture_id);
    MK_REQUIRE(result.failures[0].diagnostic.find("missing source") != std::string::npos);
    MK_REQUIRE(!fs.exists("assets/textures/missing.texture"));
}

MK_TEST("asset runtime recook execution stages imported artifacts for safe replacement") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    fs.write_text("source/textures/player_albedo.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                                  "texture.width=8\n"
                                                                  "texture.height=4\n"
                                                                  "texture.pixel_format=rgba8_unorm\n");

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player_albedo.texture_source",
        .imported_path = "assets/textures/player_albedo.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });

    mirakana::AssetRuntimeReplacementState replacements;
    replacements.seed({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player_albedo.texture", .revision = 20, .size_bytes = 64},
    });

    const std::vector<mirakana::AssetHotReloadRecookRequest> requests{
        mirakana::AssetHotReloadRecookRequest{
            .asset = texture_id,
            .source_asset = texture_id,
            .trigger_path = "source/textures/player_albedo.texture_source",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::source_modified,
            .previous_revision = 20,
            .current_revision = 21,
            .ready_tick = 104,
        },
    };

    const auto result =
        mirakana::execute_asset_runtime_recook(fs, mirakana::build_asset_import_plan(imports), replacements, requests);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.import_result.imported.size() == 1);
    MK_REQUIRE(result.apply_results.size() == 1);
    MK_REQUIRE(result.apply_results[0].kind == mirakana::AssetHotReloadApplyResultKind::staged);
    MK_REQUIRE(replacements.pending_count() == 1);
    MK_REQUIRE(replacements.find_active(texture_id)->revision == 20);
    MK_REQUIRE(fs.read_text("assets/textures/player_albedo.texture").find("texture.width=8\n") != std::string::npos);

    const auto applied = replacements.commit_safe_point();
    MK_REQUIRE(applied.size() == 1);
    MK_REQUIRE(applied[0].kind == mirakana::AssetHotReloadApplyResultKind::applied);
    MK_REQUIRE(replacements.find_active(texture_id)->revision == 21);
}

MK_TEST("asset runtime recook execution rolls back every requested asset on transactional failure") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    fs.write_text("source/textures/player_albedo.texture_source", "format=GameEngine.TextureSource.v1\n"
                                                                  "texture.width=8\n"
                                                                  "texture.height=4\n"
                                                                  "texture.pixel_format=rgba8_unorm\n");

    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/missing.mesh_source",
        .output_path = "assets/meshes/player.mesh",
        .dependencies = {},
    });
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture_id,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/player_albedo.texture_source",
        .output_path = "assets/textures/player_albedo.texture",
        .dependencies = {},
    });

    mirakana::AssetRuntimeReplacementState replacements;
    replacements.seed({
        mirakana::AssetFileSnapshot{
            .asset = mesh_id, .path = "assets/meshes/player.mesh", .revision = 7, .size_bytes = 80},
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player_albedo.texture", .revision = 20, .size_bytes = 64},
    });

    const std::vector<mirakana::AssetHotReloadRecookRequest> requests{
        mirakana::AssetHotReloadRecookRequest{
            .asset = mesh_id,
            .source_asset = mesh_id,
            .trigger_path = "source/meshes/missing.mesh_source",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::source_modified,
            .previous_revision = 7,
            .current_revision = 8,
            .ready_tick = 104,
        },
        mirakana::AssetHotReloadRecookRequest{
            .asset = texture_id,
            .source_asset = texture_id,
            .trigger_path = "source/textures/player_albedo.texture_source",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::source_modified,
            .previous_revision = 20,
            .current_revision = 21,
            .ready_tick = 104,
        },
    };

    const auto result = mirakana::execute_asset_runtime_recook(fs, import_plan, replacements, requests);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.import_result.failures.size() == 1);
    MK_REQUIRE(result.apply_results.size() == 2);
    MK_REQUIRE(result.apply_results[0].kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back);
    MK_REQUIRE(result.apply_results[1].kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back);
    MK_REQUIRE(replacements.pending_count() == 0);
    MK_REQUIRE(replacements.find_active(mesh_id)->revision == 7);
    MK_REQUIRE(replacements.find_active(texture_id)->revision == 20);
    MK_REQUIRE(!fs.exists("assets/textures/player_albedo.texture"));
}

MK_TEST("asset file scanner creates deterministic hot reload snapshots") {
    mirakana::MemoryFileSystem fs;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    fs.write_text("assets/textures/player.texture", "texture-v1");
    fs.write_text("assets/meshes/player.mesh", "mesh-v1");

    mirakana::AssetRegistry assets;
    assets.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});
    assets.add(
        mirakana::AssetRecord{.id = mesh_id, .kind = mirakana::AssetKind::mesh, .path = "assets/meshes/player.mesh"});

    const auto scan = mirakana::scan_asset_files_for_hot_reload(fs, assets);

    MK_REQUIRE(scan.complete());
    MK_REQUIRE(scan.snapshots.size() == 2);
    MK_REQUIRE(scan.snapshots[0].path == "assets/meshes/player.mesh");
    MK_REQUIRE(scan.snapshots[1].path == "assets/textures/player.texture");
    MK_REQUIRE(scan.snapshots[0].size_bytes == 7);
    MK_REQUIRE(scan.snapshots[0].revision != 0);

    mirakana::AssetHotReloadTracker tracker;
    MK_REQUIRE(tracker.update(scan.snapshots).size() == 2);

    fs.write_text("assets/textures/player.texture", "texture-v2");
    const auto changed = mirakana::scan_asset_files_for_hot_reload(fs, assets);
    const auto events = tracker.update(changed.snapshots);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::AssetHotReloadEventKind::modified);
    MK_REQUIRE(events[0].asset == texture_id);
}

MK_TEST("asset file scanner reports missing registered files") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry assets;
    const auto texture_id = mirakana::AssetId::from_name("textures/missing");
    assets.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/missing.texture"});

    const auto scan = mirakana::scan_asset_files_for_hot_reload(fs, assets);

    MK_REQUIRE(!scan.complete());
    MK_REQUIRE(scan.snapshots.empty());
    MK_REQUIRE(scan.missing.size() == 1);
    MK_REQUIRE(scan.missing[0].asset == texture_id);
    MK_REQUIRE(scan.missing[0].path == "assets/textures/missing.texture");
}

MK_TEST("decode audited png plus sprite atlas packing composes a 2x1 atlas when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    static constexpr auto k1x1_png = std::to_array<unsigned char>({
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00,
        0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x01,
        0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    });
    const auto decoded =
        mirakana::decode_audited_png_rgba8({reinterpret_cast<const std::uint8_t*>(k1x1_png.data()), k1x1_png.size()});
    MK_REQUIRE(decoded.width == 1 && decoded.height == 1);
    MK_REQUIRE(decoded.bytes.size() == 4);

    std::vector<std::uint8_t> left = decoded.bytes;
    std::vector<std::uint8_t> right = decoded.bytes;
    std::vector<mirakana::SpriteAtlasPackingItemView> items = {
        {.width = 1, .height = 1, .rgba8_pixels = left},
        {.width = 1, .height = 1, .rgba8_pixels = right},
    };
    const auto packed = mirakana::pack_sprite_atlas_rgba8_max_side(items);
    MK_REQUIRE(std::holds_alternative<mirakana::SpriteAtlasRgba8PackingOutput>(packed));
    const auto& out = std::get<mirakana::SpriteAtlasRgba8PackingOutput>(packed);
    MK_REQUIRE(out.atlas.width == 2 && out.atlas.height == 1);
    MK_REQUIRE(mirakana::is_valid_texture_source_document(out.atlas));
}

MK_TEST("runtime UI PNG image decoding adapter fails closed when importers are disabled") {
    if (mirakana::external_asset_importers_available()) {
        return;
    }

    mirakana::PngImageDecodingAdapter adapter;
    mirakana::ui::ImageDecodeRequest request;
    request.asset_uri = "assets/ui/icon.png";
    request.bytes = {std::byte{0x66}, std::byte{0x61}, std::byte{0x6b}, std::byte{0x65}};

    const auto result = mirakana::ui::decode_image_request(adapter, request);

    MK_REQUIRE(result.decoded);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.image.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_result);
}

MK_TEST("runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }

    static constexpr auto k1x1_png = std::to_array<unsigned char>({
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00,
        0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x01,
        0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    });

    mirakana::ui::ImageDecodeRequest request;
    request.asset_uri = "assets/ui/icon.png";
    request.bytes.reserve(k1x1_png.size());
    for (const unsigned char value : k1x1_png) {
        request.bytes.push_back(static_cast<std::byte>(value));
    }

    mirakana::PngImageDecodingAdapter adapter;
    const auto result = mirakana::ui::decode_image_request(adapter, request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.image.has_value());
    MK_REQUIRE(result.image->width == 1);
    MK_REQUIRE(result.image->height == 1);
    MK_REQUIRE(result.image->pixel_format == mirakana::ui::ImageDecodePixelFormat::rgba8_unorm);
    MK_REQUIRE(result.image->pixels.size() == 4);

    mirakana::ui::ImageDecodeRequest invalid_request;
    invalid_request.asset_uri = "assets/ui/broken.png";
    invalid_request.bytes = {std::byte{0x6e}, std::byte{0x6f}};
    const auto invalid = mirakana::ui::decode_image_request(adapter, invalid_request);

    MK_REQUIRE(invalid.decoded);
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(!invalid.image.has_value());
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_result);
}

MK_TEST("gltf mesh inspect lists indexed triangle primitive when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string triangle_buffer;
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 2.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 3.0F);
    append_le_u16(triangle_buffer, 2U);
    append_le_u16(triangle_buffer, 1U);
    append_le_u16(triangle_buffer, 0U);
    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(triangle_buffer.size()) + R"(,"uri":")" + gltf_data_uri(triangle_buffer) +
        "\"}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}],"
        "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}]}";

    const auto report = mirakana::inspect_gltf_mesh_primitives(document, "source/meshes/triangle.gltf");
    MK_REQUIRE(report.parse_succeeded);
    MK_REQUIRE(report.diagnostic.empty());
    MK_REQUIRE(report.rows.size() == 1);
    MK_REQUIRE(report.rows[0].mesh_name == "Mesh 0");
    MK_REQUIRE(report.rows[0].position_vertex_count == 3);
    MK_REQUIRE(report.rows[0].indexed);
    MK_REQUIRE(!report.rows[0].has_normal);
    MK_REQUIRE(!report.rows[0].has_texcoord0);
}

MK_TEST("gltf skin animation inspect counts skinned triangle primitive when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
    }
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 1.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    append_le_u16(buf, 0U);
    append_le_u16(buf, 1U);
    append_le_u16(buf, 2U);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                                 "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":60,\"byteLength\":48},"
                                 "{\"buffer\":0,\"byteOffset\":108,\"byteLength\":6}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                                 "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"JOINTS_0\":1,"
                                 "\"WEIGHTS_0\":2},\"indices\":3}]}]}";

    const auto report = mirakana::inspect_gltf_skin_animation(document, "source/meshes/skin_tri.gltf");
    MK_REQUIRE(report.parse_succeeded);
    MK_REQUIRE(report.diagnostic.empty());
    MK_REQUIRE(report.skins.empty());
    MK_REQUIRE(report.animations.empty());
    MK_REQUIRE(report.skinned_triangle_primitive_count == 1);
}

MK_TEST("gltf skin bind extract reads single-joint identity IBM") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string ibm;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            append_le_f32(ibm, (r == c) ? 1.0F : 0.0F);
        }
    }
    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(ibm.size()) + R"(,"uri":")" + gltf_data_uri(ibm) +
        "\"}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" +
        std::to_string(ibm.size()) +
        "}],"
        "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"}],"
        "\"nodes\":[{\"name\":\"Joint0\"}],"
        "\"skins\":[{\"joints\":[0],\"inverseBindMatrices\":0}]}";

    const auto report = mirakana::extract_gltf_skin_bind_data(document, "source/skins/one.gltf", 0);
    MK_REQUIRE(report.succeeded);
    MK_REQUIRE(report.diagnostic.empty());
    MK_REQUIRE(report.joint_node_indices.size() == 1);
    MK_REQUIRE(report.joint_node_indices[0] == 0);
    MK_REQUIRE(report.inverse_bind_matrices.size() == 1);
    MK_REQUIRE(report.inverse_bind_matrices[0].at(0, 0) == 1.0F);
    MK_REQUIRE(report.inverse_bind_matrices[0].at(3, 3) == 1.0F);
    MK_REQUIRE(report.inverse_bind_matrices[0].at(0, 3) == 0.0F);
}

MK_TEST("gltf skin bind extract defaults IBM to identity when omitted") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    const std::string document = "{\"asset\":{\"version\":\"2.0\"},"
                                 "\"nodes\":[{\"name\":\"Joint0\"}],"
                                 "\"skins\":[{\"joints\":[0]}]}";

    const auto report = mirakana::extract_gltf_skin_bind_data(document, "source/skins/no_ibm.gltf", 0);
    MK_REQUIRE(report.succeeded);
    MK_REQUIRE(report.inverse_bind_matrices.size() == 1);
    MK_REQUIRE(report.inverse_bind_matrices[0].at(0, 0) == 1.0F);
    MK_REQUIRE(report.inverse_bind_matrices[0].at(3, 3) == 1.0F);
}

MK_TEST("gltf skin bind extract rejects IBM count mismatch") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string ibm;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            append_le_f32(ibm, (r == c) ? 1.0F : 0.0F);
        }
    }
    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(ibm.size()) + R"(,"uri":")" + gltf_data_uri(ibm) +
        "\"}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" +
        std::to_string(ibm.size()) +
        "}],"
        "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"}],"
        "\"nodes\":[{\"name\":\"A\"},{\"name\":\"B\"}],"
        "\"skins\":[{\"joints\":[0,1],\"inverseBindMatrices\":0}]}";

    const auto report = mirakana::extract_gltf_skin_bind_data(document, "source/skins/bad.gltf", 0);
    MK_REQUIRE(!report.succeeded);
    MK_REQUIRE(report.diagnostic.find("joint count") != std::string::npos);
}

MK_TEST("gltf skin skeleton and skin payload import succeeds for single-joint skinned triangle when importers are "
        "enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            append_le_f32(buf, (r == c) ? 1.0F : 0.0F);
        }
    }
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
    }
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 1.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    append_le_u16(buf, 0U);
    append_le_u16(buf, 1U);
    append_le_u16(buf, 2U);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.1F);
    append_le_f32(buf, 0.0F);
    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
        "\"}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":64},"
        "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":100,\"byteLength\":24},"
        "{\"buffer\":0,\"byteOffset\":124,\"byteLength\":48},"
        "{\"buffer\":0,\"byteOffset\":172,\"byteLength\":6},"
        "{\"buffer\":0,\"byteOffset\":178,\"byteLength\":8},"
        "{\"buffer\":0,\"byteOffset\":186,\"byteLength\":24}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"},"
        "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"VEC4\"},"
        "{\"bufferView\":3,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},"
        "{\"bufferView\":4,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},"
        "{\"bufferView\":5,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "{\"bufferView\":6,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
        "\"nodes\":[{\"name\":\"j0\"}],"
        "\"skins\":[{\"joints\":[0],\"inverseBindMatrices\":0}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":1,\"JOINTS_0\":2,\"WEIGHTS_0\":3},\"indices\":4}]}]"
        ","
        "\"animations\":[{\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}],"
        "\"samplers\":[{\"input\":5,\"output\":6,\"interpolation\":\"LINEAR\"}]}]}";

    const auto skin_import =
        mirakana::import_gltf_skin_skeleton_and_skin_payload(document, "source/meshes/skin_import_tri.gltf", 0, 0, 0);
    MK_REQUIRE(skin_import.succeeded);
    MK_REQUIRE(skin_import.diagnostic.empty());
    MK_REQUIRE(skin_import.skeleton.joints.size() == 1);
    MK_REQUIRE(skin_import.skeleton.joints[0].name == "j0");
    MK_REQUIRE(skin_import.skin_payload.vertex_count == 3);
    MK_REQUIRE(skin_import.skin_payload.vertices.size() == 3);
    MK_REQUIRE(mirakana::is_valid_animation_skin_payload(skin_import.skeleton, skin_import.skin_payload));

    const auto anim_import =
        mirakana::import_gltf_animation_joint_tracks_for_skin(document, "source/meshes/skin_import_tri.gltf", 0, 0);
    MK_REQUIRE(anim_import.succeeded);
    MK_REQUIRE(anim_import.joint_tracks.size() == 1);
    MK_REQUIRE(anim_import.joint_tracks[0].joint_index == 0);
    MK_REQUIRE(anim_import.joint_tracks[0].translation_keyframes.size() == 2);
    MK_REQUIRE(mirakana::is_valid_animation_joint_tracks(skin_import.skeleton, anim_import.joint_tracks));
}

MK_TEST("gltf skin skeleton import rejects non z-axis-only joint rotation when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            append_le_f32(buf, (r == c) ? 1.0F : 0.0F);
        }
    }
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
    }
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 1.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    append_le_u16(buf, 0U);
    append_le_u16(buf, 1U);
    append_le_u16(buf, 2U);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":64},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":36},"
                                 "{\"buffer\":0,\"byteOffset\":100,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":124,\"byteLength\":48},"
                                 "{\"buffer\":0,\"byteOffset\":172,\"byteLength\":6}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":4,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                                 "\"nodes\":[{\"name\":\"j0\",\"rotation\":[0.70710677,0.0,0.0,0.70710677]}],"
                                 "\"skins\":[{\"joints\":[0],\"inverseBindMatrices\":0}],"
                                 "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":1,\"JOINTS_0\":2,"
                                 "\"WEIGHTS_0\":3},\"indices\":4}]}]}";

    const auto skin_import =
        mirakana::import_gltf_skin_skeleton_and_skin_payload(document, "source/meshes/bad_rot_skin.gltf", 0, 0, 0);
    MK_REQUIRE(!skin_import.succeeded);
    MK_REQUIRE(skin_import.diagnostic.find("z-axis-only") != std::string::npos);
}

MK_TEST("gltf node transform animation import reads LINEAR translation rotation and scale channels when importers are "
        "enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 4.0F);
    append_le_f32(buf, 6.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 3.0F);
    append_le_f32(buf, 4.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":32,\"byteLength\":32},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{\"name\":\"scale_node\"},{\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}},"
                                 "{\"sampler\":1,\"target\":{\"node\":1,\"path\":\"rotation\"}},"
                                 "{\"sampler\":2,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                                 "\"samplers\":["
                                 "{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported =
        mirakana::import_gltf_node_transform_animation_tracks(document, "source/animations/node_transform.gltf", 0);
    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(imported.node_tracks.size() == 2);
    MK_REQUIRE(imported.node_tracks[0].node_index == 0);
    MK_REQUIRE(imported.node_tracks[0].scale_keyframes.size() == 2);
    MK_REQUIRE(imported.node_tracks[1].node_index == 1);
    MK_REQUIRE(imported.node_tracks[1].translation_keyframes.size() == 2);
    MK_REQUIRE(imported.node_tracks[1].rotation_z_keyframes.size() == 2);

    const auto scale = mirakana::sample_vec3_keyframes(imported.node_tracks[0].scale_keyframes, 0.5F);
    MK_REQUIRE(std::abs(scale.x - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(scale.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(scale.z - 2.5F) < 0.0001F);
    const auto translation = mirakana::sample_vec3_keyframes(imported.node_tracks[1].translation_keyframes, 0.5F);
    MK_REQUIRE(std::abs(translation.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(translation.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(translation.z - 3.0F) < 0.0001F);
    const auto rotation = mirakana::sample_float_keyframes(imported.node_tracks[1].rotation_z_keyframes, 1.0F);
    MK_REQUIRE(std::abs(rotation - 1.5707964F) < 0.0001F);
}

MK_TEST("gltf node transform animation 3D import reads full quaternion rotation channels when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 4.0F);
    append_le_f32(buf, 6.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 3.0F);
    append_le_f32(buf, 4.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":32,\"byteLength\":32},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{\"name\":\"scale_node\"},{\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}},"
                                 "{\"sampler\":1,\"target\":{\"node\":1,\"path\":\"rotation\"}},"
                                 "{\"sampler\":2,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                                 "\"samplers\":["
                                 "{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto legacy =
        mirakana::import_gltf_node_transform_animation_tracks(document, "source/animations/node_transform_3d.gltf", 0);
    MK_REQUIRE(!legacy.succeeded);
    MK_REQUIRE(legacy.diagnostic.find("z-axis-only") != std::string::npos);

    const auto imported = mirakana::import_gltf_node_transform_animation_tracks_3d(
        document, "source/animations/node_transform_3d.gltf", 0);
    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(imported.node_tracks.size() == 2);
    MK_REQUIRE(imported.node_tracks[0].node_index == 0);
    MK_REQUIRE(imported.node_tracks[0].scale_keyframes.size() == 2);
    MK_REQUIRE(imported.node_tracks[1].node_index == 1);
    MK_REQUIRE(imported.node_tracks[1].node_name == "animated_node");
    MK_REQUIRE(imported.node_tracks[1].translation_keyframes.size() == 2);
    MK_REQUIRE(imported.node_tracks[1].rotation_keyframes.size() == 2);
    MK_REQUIRE(mirakana::is_valid_quat_keyframes(imported.node_tracks[1].rotation_keyframes));

    const auto sampled_rotation = mirakana::sample_quat_keyframes(imported.node_tracks[1].rotation_keyframes, 0.5F);
    MK_REQUIRE(mirakana::is_normalized_quat(sampled_rotation));
    const auto rotated_x = mirakana::rotate(sampled_rotation, mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    MK_REQUIRE(std::abs(rotated_x.x - 0.7071F) < 0.0005F);
    MK_REQUIRE(std::abs(rotated_x.y) < 0.0005F);
    MK_REQUIRE(std::abs(rotated_x.z + 0.7071F) < 0.0005F);

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {mirakana::AnimationSkeleton3dJointDesc{.name = "scale_node", .parent_index = mirakana::animation_no_parent},
         mirakana::AnimationSkeleton3dJointDesc{.name = "animated_node", .parent_index = 0U}}};
    const std::vector<mirakana::AnimationJointTrack3dDesc> joint_tracks{
        mirakana::AnimationJointTrack3dDesc{
            .joint_index = imported.node_tracks[1].node_index,
            .translation_keyframes = imported.node_tracks[1].translation_keyframes,
            .rotation_keyframes = imported.node_tracks[1].rotation_keyframes,
        },
        mirakana::AnimationJointTrack3dDesc{
            .joint_index = imported.node_tracks[0].node_index,
            .scale_keyframes = imported.node_tracks[0].scale_keyframes,
        },
    };
    MK_REQUIRE(mirakana::is_valid_animation_joint_tracks_3d(skeleton, joint_tracks));
    const auto pose = mirakana::sample_animation_local_pose_3d(skeleton, joint_tracks, 0.5F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.z - 3.0F) < 0.0001F);
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[1].rotation));
}

MK_TEST("gltf node transform animation 3D import rejects invalid quaternion channels when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    const auto make_prefix = [](const std::string& bytes) {
        return std::string{"{\"asset\":{\"version\":\"2.0\"},"
                           "\"buffers\":[{\"byteLength\":"} +
               std::to_string(bytes.size()) + R"(,"uri":")" + gltf_data_uri(bytes) +
               "\"}],"
               "\"bufferViews\":["
               "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
               "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":32},"
               "{\"buffer\":0,\"byteOffset\":40,\"byteLength\":12},"
               "{\"buffer\":0,\"byteOffset\":40,\"byteLength\":24}],"
               "\"accessors\":["
               "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
               "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
               "{\"bufferView\":2,\"componentType\":5126,\"count\":1,\"type\":\"VEC3\"},"
               "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
               "\"nodes\":[{\"name\":\"animated_node\"}],";
    };
    const std::string base_prefix = make_prefix(buf);
    std::string invalid_quat_buf = buf;
    std::string invalid_quat_w;
    append_le_f32(invalid_quat_w, 2.0F);
    invalid_quat_buf.replace(8U + (3U * sizeof(float)), invalid_quat_w.size(), invalid_quat_w);
    const std::string invalid_prefix = make_prefix(invalid_quat_buf);

    const auto invalid_quat = mirakana::import_gltf_node_transform_animation_tracks_3d(
        invalid_prefix +
            "\"animations\":[{\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"rotation\"}}],"
            "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}]}]}",
        "source/animations/bad_quat_node_transform_3d.gltf", 0);
    MK_REQUIRE(!invalid_quat.succeeded);
    MK_REQUIRE(invalid_quat.diagnostic.find("normalized") != std::string::npos);

    const auto unsupported_interpolation = mirakana::import_gltf_node_transform_animation_tracks_3d(
        base_prefix + "\"animations\":[{\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"rotation\"}}],"
                      "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"STEP\"}]}]}",
        "source/animations/step_quat_node_transform_3d.gltf", 0);
    MK_REQUIRE(!unsupported_interpolation.succeeded);
    MK_REQUIRE(unsupported_interpolation.diagnostic.find("LINEAR") != std::string::npos);

    const auto mismatched_scale_count = mirakana::import_gltf_node_transform_animation_tracks_3d(
        base_prefix + "\"animations\":[{\"channels\":[{\"sampler\":1,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                      "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                      "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"}]}]}",
        "source/animations/bad_scale_count_node_transform_3d.gltf", 0);
    MK_REQUIRE(!mismatched_scale_count.succeeded);
    MK_REQUIRE(mismatched_scale_count.diagnostic.find("count") != std::string::npos);

    const auto invalid_scale = mirakana::import_gltf_node_transform_animation_tracks_3d(
        base_prefix + "\"animations\":[{\"channels\":[{\"sampler\":1,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                      "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                      "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}",
        "source/animations/bad_scale_node_transform_3d.gltf", 0);
    MK_REQUIRE(!invalid_scale.succeeded);
    MK_REQUIRE(invalid_scale.diagnostic.find("positive") != std::string::npos);

    const auto duplicate_rotation = mirakana::import_gltf_node_transform_animation_tracks_3d(
        base_prefix + "\"animations\":[{\"channels\":["
                      "{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"rotation\"}},"
                      "{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"rotation\"}}],"
                      "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}]}]}",
        "source/animations/duplicate_quat_node_transform_3d.gltf", 0);
    MK_REQUIRE(!duplicate_rotation.succeeded);
    MK_REQUIRE(duplicate_rotation.diagnostic.find("duplicate rotation") != std::string::npos);
}

MK_TEST("gltf node transform animation import rejects duplicate transform channels when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}},"
                                 "{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}],"
                                 "\"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported = mirakana::import_gltf_node_transform_animation_tracks(
        document, "source/animations/duplicate_node_transform.gltf", 0);
    MK_REQUIRE(!imported.succeeded);
    MK_REQUIRE(imported.diagnostic.find("duplicate translation") != std::string::npos);
}

MK_TEST("gltf node transform animation imports as cooked quaternion clip and samples at runtime") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 4.0F);
    append_le_f32(buf, 6.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 3.0F);
    append_le_f32(buf, 4.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":32,\"byteLength\":32},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{\"name\":\"scale_node\"},{\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}},"
                                 "{\"sampler\":1,\"target\":{\"node\":1,\"path\":\"rotation\"}},"
                                 "{\"sampler\":2,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                                 "\"samplers\":["
                                 "{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported = mirakana::import_gltf_node_transform_animation_quaternion_clip(
        document, "source/animations/node_transform_quaternion_clip.gltf", 0);
    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(mirakana::is_valid_animation_quaternion_clip_source_document(imported.clip));
    MK_REQUIRE(imported.clip.tracks.size() == 2U);
    MK_REQUIRE(imported.clip.tracks[0].target == "gltf/node/0");
    MK_REQUIRE(imported.clip.tracks[0].joint_index == 0U);
    MK_REQUIRE(imported.clip.tracks[1].target == "gltf/node/1");
    MK_REQUIRE(imported.clip.tracks[1].joint_index == 1U);

    mirakana::MemoryFileSystem fs;
    fs.write_text("source/animation/node_transform.animation_quaternion_clip_source",
                  mirakana::serialize_animation_quaternion_clip_source_document(imported.clip));
    const auto clip_id = mirakana::AssetId::from_name("animation/node-transform-quaternion");
    mirakana::AssetImportPlan cook_plan;
    cook_plan.actions.push_back(mirakana::AssetImportAction{
        .id = clip_id,
        .kind = mirakana::AssetImportActionKind::animation_quaternion_clip,
        .source_path = "source/animation/node_transform.animation_quaternion_clip_source",
        .output_path = "runtime/assets/animation/node_transform.animation_quaternion_clip",
        .dependencies = {},
    });
    const auto cook_result =
        mirakana::execute_asset_import_plan(fs, cook_plan, mirakana::AssetImportExecutionOptions{});
    MK_REQUIRE(cook_result.succeeded());
    const auto cooked = fs.read_text("runtime/assets/animation/node_transform.animation_quaternion_clip");
    MK_REQUIRE(cooked.find("format=GameEngine.CookedAnimationQuaternionClip.v1") != std::string::npos);
    MK_REQUIRE(cooked.find("asset.kind=animation_quaternion_clip") != std::string::npos);

    mirakana::runtime::RuntimeAssetPackageLoadResult package;
    {
        mirakana::MemoryFileSystem pkg_fs;
        const auto index = mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = clip_id,
                                           .kind = mirakana::AssetKind::animation_quaternion_clip,
                                           .path = "runtime/assets/animation/node_transform.animation_quaternion_clip",
                                           .content = cooked,
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {});
        pkg_fs.write_text("pkg/index.geindex", mirakana::serialize_asset_cooked_package_index(index));
        pkg_fs.write_text("runtime/assets/animation/node_transform.animation_quaternion_clip", cooked);
        package = mirakana::runtime::load_runtime_asset_package(
            pkg_fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "pkg/index.geindex", .content_root = ""});
    }
    MK_REQUIRE(package.succeeded());
    const auto* clip_record = package.package.find(clip_id);
    MK_REQUIRE(clip_record != nullptr);
    const auto clip_payload = mirakana::runtime::runtime_animation_quaternion_clip_payload(*clip_record);
    MK_REQUIRE(clip_payload.succeeded());

    std::vector<mirakana::AnimationJointTrack3dByteSource> sources;
    sources.reserve(clip_payload.payload.clip.tracks.size());
    for (const auto& track : clip_payload.payload.clip.tracks) {
        sources.push_back(mirakana::AnimationJointTrack3dByteSource{
            .joint_index = track.joint_index,
            .target = track.target,
            .translation_time_seconds_bytes = track.translation_time_seconds_bytes,
            .translation_xyz_bytes = track.translation_xyz_bytes,
            .rotation_time_seconds_bytes = track.rotation_time_seconds_bytes,
            .rotation_xyzw_bytes = track.rotation_xyzw_bytes,
            .scale_time_seconds_bytes = track.scale_time_seconds_bytes,
            .scale_xyz_bytes = track.scale_xyz_bytes,
        });
    }
    const auto joint_tracks = mirakana::make_animation_joint_tracks_3d_from_f32_bytes(sources);
    const mirakana::AnimationSkeleton3dDesc skeleton{
        {mirakana::AnimationSkeleton3dJointDesc{.name = "scale_node", .parent_index = mirakana::animation_no_parent},
         mirakana::AnimationSkeleton3dJointDesc{.name = "animated_node", .parent_index = 0U}},
    };
    const auto pose = mirakana::sample_animation_local_pose_3d(skeleton, joint_tracks, 0.5F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].translation.z - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].rotation.z - 0.38268343F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].rotation.w - 0.9238795F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.x - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.z - 2.5F) < 0.0001F);
}

MK_TEST("gltf node transform animation imports as cooked float clip and samples at runtime") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 4.0F);
    append_le_f32(buf, 6.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 3.0F);
    append_le_f32(buf, 4.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":32,\"byteLength\":32},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{\"name\":\"scale_node\"},{\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}},"
                                 "{\"sampler\":1,\"target\":{\"node\":1,\"path\":\"rotation\"}},"
                                 "{\"sampler\":2,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                                 "\"samplers\":["
                                 "{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported = mirakana::import_gltf_node_transform_animation_float_clip(
        document, "source/animations/node_transform_clip.gltf", 0);
    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(imported.clip.tracks.size() == 7);
    MK_REQUIRE(imported.clip.tracks[0].target == "gltf/node/0/scale/x");
    MK_REQUIRE(imported.clip.tracks[1].target == "gltf/node/0/scale/y");
    MK_REQUIRE(imported.clip.tracks[2].target == "gltf/node/0/scale/z");
    MK_REQUIRE(imported.clip.tracks[3].target == "gltf/node/1/translation/x");
    MK_REQUIRE(imported.clip.tracks[4].target == "gltf/node/1/translation/y");
    MK_REQUIRE(imported.clip.tracks[5].target == "gltf/node/1/translation/z");
    MK_REQUIRE(imported.clip.tracks[6].target == "gltf/node/1/rotation_z");

    mirakana::MemoryFileSystem fs;
    fs.write_text("source/animation/node_transform.animation_float_clip_source",
                  mirakana::serialize_animation_float_clip_source_document(imported.clip));
    const auto clip_id = mirakana::AssetId::from_name("animation/node-transform");
    mirakana::AssetImportPlan cook_plan;
    cook_plan.actions.push_back(mirakana::AssetImportAction{
        .id = clip_id,
        .kind = mirakana::AssetImportActionKind::animation_float_clip,
        .source_path = "source/animation/node_transform.animation_float_clip_source",
        .output_path = "runtime/assets/animation/node_transform.animation_float_clip",
        .dependencies = {},
    });
    const auto cook_result =
        mirakana::execute_asset_import_plan(fs, cook_plan, mirakana::AssetImportExecutionOptions{});
    MK_REQUIRE(cook_result.succeeded());
    const auto cooked = fs.read_text("runtime/assets/animation/node_transform.animation_float_clip");

    mirakana::runtime::RuntimeAssetPackageLoadResult package;
    {
        mirakana::MemoryFileSystem pkg_fs;
        const auto index = mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = clip_id,
                                           .kind = mirakana::AssetKind::animation_float_clip,
                                           .path = "runtime/assets/animation/node_transform.animation_float_clip",
                                           .content = cooked,
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {});
        pkg_fs.write_text("pkg/index.geindex", mirakana::serialize_asset_cooked_package_index(index));
        pkg_fs.write_text("runtime/assets/animation/node_transform.animation_float_clip", cooked);
        package = mirakana::runtime::load_runtime_asset_package(
            pkg_fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "pkg/index.geindex", .content_root = ""});
    }
    MK_REQUIRE(package.succeeded());
    const auto* clip_record = package.package.find(clip_id);
    MK_REQUIRE(clip_record != nullptr);
    const auto clip_payload = mirakana::runtime::runtime_animation_float_clip_payload(*clip_record);
    MK_REQUIRE(clip_payload.succeeded());

    std::vector<mirakana::FloatAnimationTrackByteSource> sources;
    sources.reserve(clip_payload.payload.clip.tracks.size());
    for (const auto& track : clip_payload.payload.clip.tracks) {
        sources.push_back(mirakana::FloatAnimationTrackByteSource{
            .target = track.target,
            .time_seconds_bytes = track.time_seconds_bytes,
            .value_bytes = track.value_bytes,
        });
    }
    const auto tracks = mirakana::make_float_animation_tracks_from_f32_bytes(sources);
    const auto samples = mirakana::sample_float_animation_tracks(tracks, 0.5F);
    MK_REQUIRE(samples.size() == 7);
    std::unordered_map<std::string, float> sample_by_target;
    for (const auto& sample : samples) {
        sample_by_target.emplace(sample.target, sample.value);
    }
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/0/scale/x") - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/0/scale/y") - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/0/scale/z") - 2.5F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/1/translation/x") - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/1/translation/y") - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/1/translation/z") - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(sample_by_target.at("gltf/node/1/rotation_z") - 0.7853982F) < 0.0001F);
}

MK_TEST("gltf node transform animation imports transform binding source rows") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 4.0F);
    append_le_f32(buf, 6.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 0.70710677F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 3.0F);
    append_le_f32(buf, 4.0F);
    const std::string document = std::string{"{\"asset\":{\"version\":\"2.0\"},"
                                             "\"buffers\":[{\"byteLength\":"} +
                                 std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                                 "\"}],"
                                 "\"bufferViews\":["
                                 "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":8},"
                                 "{\"buffer\":0,\"byteOffset\":8,\"byteLength\":24},"
                                 "{\"buffer\":0,\"byteOffset\":32,\"byteLength\":32},"
                                 "{\"buffer\":0,\"byteOffset\":64,\"byteLength\":24}],"
                                 "\"accessors\":["
                                 "{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                 "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"},"
                                 "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"},"
                                 "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
                                 "\"nodes\":[{}, {\"name\":\"animated_node\"}],"
                                 "\"animations\":[{\"channels\":["
                                 "{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"translation\"}},"
                                 "{\"sampler\":1,\"target\":{\"node\":1,\"path\":\"rotation\"}},"
                                 "{\"sampler\":2,\"target\":{\"node\":0,\"path\":\"scale\"}}],"
                                 "\"samplers\":["
                                 "{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":2,\"interpolation\":\"LINEAR\"},"
                                 "{\"input\":0,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported = mirakana::import_gltf_node_transform_animation_binding_source(
        document, "source/animations/node_transform_binding_source.gltf", 0);

    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(mirakana::is_valid_animation_transform_binding_source_document(imported.binding_source));
    MK_REQUIRE(imported.binding_source.bindings.size() == 7);
    MK_REQUIRE(imported.binding_source.bindings[0].target == "gltf/node/0/scale/x");
    MK_REQUIRE(imported.binding_source.bindings[0].node_name == "gltf_node_0");
    MK_REQUIRE(imported.binding_source.bindings[0].component == mirakana::AnimationTransformBindingComponent::scale_x);
    MK_REQUIRE(imported.binding_source.bindings[1].target == "gltf/node/0/scale/y");
    MK_REQUIRE(imported.binding_source.bindings[1].component == mirakana::AnimationTransformBindingComponent::scale_y);
    MK_REQUIRE(imported.binding_source.bindings[2].target == "gltf/node/0/scale/z");
    MK_REQUIRE(imported.binding_source.bindings[2].component == mirakana::AnimationTransformBindingComponent::scale_z);
    MK_REQUIRE(imported.binding_source.bindings[3].target == "gltf/node/1/translation/x");
    MK_REQUIRE(imported.binding_source.bindings[3].node_name == "animated_node");
    MK_REQUIRE(imported.binding_source.bindings[3].component ==
               mirakana::AnimationTransformBindingComponent::translation_x);
    MK_REQUIRE(imported.binding_source.bindings[6].target == "gltf/node/1/rotation_z");
    MK_REQUIRE(imported.binding_source.bindings[6].component ==
               mirakana::AnimationTransformBindingComponent::rotation_z);

    const auto serialized = mirakana::serialize_animation_transform_binding_source_document(imported.binding_source);
    MK_REQUIRE(serialized.find("format=GameEngine.AnimationTransformBindingSource.v1\n") == 0);
    MK_REQUIRE(serialized.find("binding.count=7\n") != std::string::npos);
}

MK_TEST("default external gltf importer rejects skinning attributes on mesh primitives when enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    mirakana::MemoryFileSystem fs;
    const auto mesh_id = mirakana::AssetId::from_name("meshes/skin-triangle");
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 2.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
        append_le_u16(buf, 0U);
    }
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 1.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    append_le_u16(buf, 0U);
    append_le_u16(buf, 1U);
    append_le_u16(buf, 2U);
    fs.write_text("source/meshes/skin-triangle.gltf",
                  std::string{"{\"asset\":{\"version\":\"2.0\"},"
                              "\"buffers\":[{\"byteLength\":"} +
                      std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
                      "\"}],"
                      "\"bufferViews\":["
                      "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
                      "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":24},"
                      "{\"buffer\":0,\"byteOffset\":60,\"byteLength\":48},"
                      "{\"buffer\":0,\"byteOffset\":108,\"byteLength\":6}],"
                      "\"accessors\":["
                      "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
                      "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"VEC4\"},"
                      "{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"},"
                      "{\"bufferView\":3,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
                      "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"JOINTS_0\":1,\"WEIGHTS_0\":2},"
                      "\"indices\":3}]}]}");

    mirakana::AssetImportPlan plan;
    plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh_id,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/skin-triangle.gltf",
        .output_path = "assets/meshes/skin-triangle.mesh",
        .dependencies = {},
    });
    mirakana::ExternalAssetImportAdapters adapters;
    const auto result = mirakana::execute_asset_import_plan(fs, plan, adapters.options());
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("skinning") != std::string::npos);
}

MK_TEST("gltf morph mesh and weights animation import succeeds when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 0.1F * static_cast<float>(i == 0 ? 1 : 0));
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.25F);
    append_le_f32(buf, 0.75F);

    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
        "\"}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":72,\"byteLength\":8},"
        "{\"buffer\":0,\"byteOffset\":80,\"byteLength\":8}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"weights\":[0.25],\"primitives\":[{\"attributes\":{\"POSITION\":0},"
        "\"targets\":[{\"POSITION\":1}]}]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"animations\":[{\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"weights\"}}],"
        "\"samplers\":[{\"input\":2,\"output\":3,\"interpolation\":\"LINEAR\"}]}]}";

    const auto morph = mirakana::import_gltf_morph_mesh_cpu_primitive(document, "source/meshes/morph_tri.gltf", 0, 0);
    MK_REQUIRE(morph.succeeded);
    MK_REQUIRE(morph.diagnostic.empty());
    MK_REQUIRE(morph.morph_mesh.targets.size() == 1);
    MK_REQUIRE(morph.morph_mesh.target_weights.size() == 1);
    MK_REQUIRE(std::abs(morph.morph_mesh.target_weights[0] - 0.25F) < 0.0001F);

    const auto weights_anim = mirakana::import_gltf_animation_morph_weights_for_mesh_primitive(
        document, "source/meshes/morph_tri.gltf", 0, 0, 0, 0);
    MK_REQUIRE(weights_anim.succeeded);
    MK_REQUIRE(weights_anim.diagnostic.empty());
    const auto sampled = mirakana::sample_animation_morph_weights_at_time(weights_anim.weights_track, 0.5F);
    MK_REQUIRE(sampled.size() == 1);
    MK_REQUIRE(std::abs(sampled[0] - 0.5F) < 0.0001F);

    mirakana::MemoryFileSystem fs;
    fs.write_text("source/meshes/morph_cook_tri.gltf", document);
    const auto morph_id = mirakana::AssetId::from_name("meshes/morph-cook-tri");
    mirakana::AssetImportPlan cook_plan;
    cook_plan.actions.push_back(mirakana::AssetImportAction{
        .id = morph_id,
        .kind = mirakana::AssetImportActionKind::morph_mesh_cpu,
        .source_path = "source/meshes/morph_cook_tri.gltf",
        .output_path = "runtime/assets/morph/morph_cook_tri.morph_mesh_cpu",
        .dependencies = {},
    });
    mirakana::ExternalAssetImportAdapters cook_adapters;
    const auto cook_result = mirakana::execute_asset_import_plan(fs, cook_plan, cook_adapters.options());
    MK_REQUIRE(cook_result.succeeded());
    const auto cooked = fs.read_text("runtime/assets/morph/morph_cook_tri.morph_mesh_cpu");
    MK_REQUIRE(cooked.find("format=GameEngine.CookedMorphMeshCpu.v1") != std::string::npos);
    MK_REQUIRE(cooked.find("asset.kind=morph_mesh_cpu") != std::string::npos);
    MK_REQUIRE(cooked.find("morph.vertex_count=3") != std::string::npos);
    MK_REQUIRE(cooked.find("morph.target_count=1") != std::string::npos);

    mirakana::runtime::RuntimeAssetPackageLoadResult package;
    {
        mirakana::MemoryFileSystem pkg_fs;
        const auto index =
            mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                           .asset = morph_id,
                                                           .kind = mirakana::AssetKind::morph_mesh_cpu,
                                                           .path = "runtime/assets/morph/morph_cook_tri.morph_mesh_cpu",
                                                           .content = cooked,
                                                           .source_revision = 1,
                                                           .dependencies = {},
                                                       }},
                                                       {});
        pkg_fs.write_text("pkg/index.geindex", mirakana::serialize_asset_cooked_package_index(index));
        pkg_fs.write_text("runtime/assets/morph/morph_cook_tri.morph_mesh_cpu", cooked);
        package = mirakana::runtime::load_runtime_asset_package(
            pkg_fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "pkg/index.geindex", .content_root = ""});
    }
    MK_REQUIRE(package.succeeded());
    const auto* morph_record = package.package.find(morph_id);
    MK_REQUIRE(morph_record != nullptr);
    const auto morph_payload = mirakana::runtime::runtime_morph_mesh_cpu_payload(*morph_record);
    MK_REQUIRE(morph_payload.succeeded());
    MK_REQUIRE(morph_payload.payload.morph.vertex_count == 3U);
    MK_REQUIRE(morph_payload.payload.morph.targets.size() == 1U);
}

MK_TEST("gltf morph weights animation imports as cooked float clip and samples at runtime") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    std::string buf;
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.0F);
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 0.1F * static_cast<float>(i == 0 ? 1 : 0));
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.0F);
    }
    for (int i = 0; i < 3; ++i) {
        append_le_f32(buf, 0.0F);
        append_le_f32(buf, 0.2F * static_cast<float>(i == 1 ? 1 : 0));
        append_le_f32(buf, 0.0F);
    }
    append_le_f32(buf, 0.0F);
    append_le_f32(buf, 1.0F);
    append_le_f32(buf, 0.25F);
    append_le_f32(buf, 0.5F);
    append_le_f32(buf, 0.75F);
    append_le_f32(buf, 1.0F);

    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(buf.size()) + R"(,"uri":")" + gltf_data_uri(buf) +
        "\"}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":72,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":108,\"byteLength\":8},"
        "{\"buffer\":0,\"byteOffset\":116,\"byteLength\":16}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "{\"bufferView\":4,\"componentType\":5126,\"count\":4,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"weights\":[0.25,0.5],\"primitives\":[{\"attributes\":{\"POSITION\":0},"
        "\"targets\":[{\"POSITION\":1},{\"POSITION\":2}]}]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"animations\":[{\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"weights\"}}],"
        "\"samplers\":[{\"input\":3,\"output\":4,\"interpolation\":\"LINEAR\"}]}]}";

    const auto imported = mirakana::import_gltf_morph_weights_animation_float_clip(
        document, "source/meshes/morph_weights_clip.gltf", 0, 0, 0, 0);
    MK_REQUIRE(imported.succeeded);
    MK_REQUIRE(imported.diagnostic.empty());
    MK_REQUIRE(imported.clip.tracks.size() == 2);
    MK_REQUIRE(imported.clip.tracks[0].target == "gltf/node/0/weights/0");
    MK_REQUIRE(imported.clip.tracks[1].target == "gltf/node/0/weights/1");

    mirakana::MemoryFileSystem fs;
    fs.write_text("source/animation/morph_weights.animation_float_clip_source",
                  mirakana::serialize_animation_float_clip_source_document(imported.clip));
    const auto clip_id = mirakana::AssetId::from_name("animation/morph-weights");
    mirakana::AssetImportPlan cook_plan;
    cook_plan.actions.push_back(mirakana::AssetImportAction{
        .id = clip_id,
        .kind = mirakana::AssetImportActionKind::animation_float_clip,
        .source_path = "source/animation/morph_weights.animation_float_clip_source",
        .output_path = "runtime/assets/animation/morph_weights.animation_float_clip",
        .dependencies = {},
    });
    const auto cook_result =
        mirakana::execute_asset_import_plan(fs, cook_plan, mirakana::AssetImportExecutionOptions{});
    MK_REQUIRE(cook_result.succeeded());
    const auto cooked = fs.read_text("runtime/assets/animation/morph_weights.animation_float_clip");

    mirakana::runtime::RuntimeAssetPackageLoadResult package;
    {
        mirakana::MemoryFileSystem pkg_fs;
        const auto index = mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = clip_id,
                                           .kind = mirakana::AssetKind::animation_float_clip,
                                           .path = "runtime/assets/animation/morph_weights.animation_float_clip",
                                           .content = cooked,
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {});
        pkg_fs.write_text("pkg/index.geindex", mirakana::serialize_asset_cooked_package_index(index));
        pkg_fs.write_text("runtime/assets/animation/morph_weights.animation_float_clip", cooked);
        package = mirakana::runtime::load_runtime_asset_package(
            pkg_fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "pkg/index.geindex", .content_root = ""});
    }
    MK_REQUIRE(package.succeeded());
    const auto* clip_record = package.package.find(clip_id);
    MK_REQUIRE(clip_record != nullptr);
    const auto clip_payload = mirakana::runtime::runtime_animation_float_clip_payload(*clip_record);
    MK_REQUIRE(clip_payload.succeeded());

    std::vector<mirakana::FloatAnimationTrackByteSource> sources;
    sources.reserve(clip_payload.payload.clip.tracks.size());
    for (const auto& track : clip_payload.payload.clip.tracks) {
        sources.push_back(mirakana::FloatAnimationTrackByteSource{
            .target = track.target,
            .time_seconds_bytes = track.time_seconds_bytes,
            .value_bytes = track.value_bytes,
        });
    }
    const auto tracks = mirakana::make_float_animation_tracks_from_f32_bytes(sources);
    const auto samples = mirakana::sample_float_animation_tracks(tracks, 0.5F);
    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].target == "gltf/node/0/weights/0");
    MK_REQUIRE(std::abs(samples[0].value - 0.5F) < 0.0001F);
    MK_REQUIRE(samples[1].target == "gltf/node/0/weights/1");
    MK_REQUIRE(std::abs(samples[1].value - 0.75F) < 0.0001F);
}

MK_TEST("animation float clip cook loads through runtime package") {
    mirakana::AnimationFloatClipSourceDocument document;
    mirakana::AnimationFloatClipTrackSourceDocument track;
    track.target = "demo.scalar";
    std::string times_buf;
    append_le_f32(times_buf, 0.0F);
    append_le_f32(times_buf, 0.5F);
    std::string values_buf;
    append_le_f32(values_buf, 10.0F);
    append_le_f32(values_buf, 20.0F);
    track.time_seconds_bytes.assign(times_buf.begin(), times_buf.end());
    track.value_bytes.assign(values_buf.begin(), values_buf.end());
    document.tracks.push_back(std::move(track));
    const auto source = mirakana::serialize_animation_float_clip_source_document(document);

    mirakana::MemoryFileSystem fs;
    fs.write_text("source/animation/demo.animation_float_clip_source", source);
    const auto clip_id = mirakana::AssetId::from_name("animation/demo-clip");
    mirakana::AssetImportPlan cook_plan;
    cook_plan.actions.push_back(mirakana::AssetImportAction{
        .id = clip_id,
        .kind = mirakana::AssetImportActionKind::animation_float_clip,
        .source_path = "source/animation/demo.animation_float_clip_source",
        .output_path = "runtime/assets/animation/demo.animation_float_clip",
        .dependencies = {},
    });
    const auto cook_result =
        mirakana::execute_asset_import_plan(fs, cook_plan, mirakana::AssetImportExecutionOptions{});
    MK_REQUIRE(cook_result.succeeded());
    const auto cooked = fs.read_text("runtime/assets/animation/demo.animation_float_clip");
    MK_REQUIRE(cooked.find("format=GameEngine.CookedAnimationFloatClip.v1") != std::string::npos);
    MK_REQUIRE(cooked.find("asset.kind=animation_float_clip") != std::string::npos);
    MK_REQUIRE(cooked.find("clip.track_count=1") != std::string::npos);

    mirakana::runtime::RuntimeAssetPackageLoadResult package;
    {
        mirakana::MemoryFileSystem pkg_fs;
        const auto index = mirakana::build_asset_cooked_package_index(
            {mirakana::AssetCookedArtifact{.asset = clip_id,
                                           .kind = mirakana::AssetKind::animation_float_clip,
                                           .path = "runtime/assets/animation/demo.animation_float_clip",
                                           .content = cooked,
                                           .source_revision = 1,
                                           .dependencies = {}}},
            {});
        pkg_fs.write_text("pkg/index.geindex", mirakana::serialize_asset_cooked_package_index(index));
        pkg_fs.write_text("runtime/assets/animation/demo.animation_float_clip", cooked);
        package = mirakana::runtime::load_runtime_asset_package(
            pkg_fs, mirakana::runtime::RuntimeAssetPackageDesc{.index_path = "pkg/index.geindex", .content_root = ""});
    }
    MK_REQUIRE(package.succeeded());
    const auto* clip_record = package.package.find(clip_id);
    MK_REQUIRE(clip_record != nullptr);
    const auto clip_payload = mirakana::runtime::runtime_animation_float_clip_payload(*clip_record);
    MK_REQUIRE(clip_payload.succeeded());
    MK_REQUIRE(clip_payload.payload.clip.tracks.size() == 1U);
    MK_REQUIRE(clip_payload.payload.clip.tracks[0].target == "demo.scalar");
}

int main() {
    return mirakana::test::run_all();
}
