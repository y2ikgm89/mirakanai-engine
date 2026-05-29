// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/tile_chunk_renderer.hpp"

#include "test_framework.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::TileChunkCellRow cell(std::int32_t x, std::int32_t y, std::int32_t layer, std::string material,
                                              mirakana::AssetId atlas, mirakana::SpriteUvRect uv,
                                              bool transparent = false) {
    return mirakana::TileChunkCellRow{
        .tile_id = "tile",
        .material_id = std::move(material),
        .x = x,
        .y = y,
        .layer = layer,
        .visible = true,
        .transparent = transparent,
        .atlas_page = atlas,
        .uv_rect = uv,
        .color = mirakana::Color{.r = 0.5F, .g = 0.75F, .b = 1.0F, .a = transparent ? 0.5F : 1.0F},
    };
}

[[nodiscard]] mirakana::TileChunkRendererPlan make_basic_plan() {
    const auto atlas_a = mirakana::AssetId::from_name("textures/tile-atlas-a");
    const auto atlas_b = mirakana::AssetId::from_name("textures/tile-atlas-b");
    const mirakana::SpriteUvRect grass_uv{.u0 = 0.0F, .v0 = 0.0F, .u1 = 0.5F, .v1 = 0.5F};
    const mirakana::SpriteUvRect rock_uv{.u0 = 0.5F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 0.5F};
    const mirakana::SpriteUvRect leaf_uv{.u0 = 0.0F, .v0 = 0.5F, .u1 = 0.5F, .v1 = 1.0F};
    std::vector<mirakana::TileChunkCellRow> cells{
        cell(1, 0, 1, "foliage", atlas_a, leaf_uv, true),
        cell(0, 0, 0, "ground", atlas_a, grass_uv),
        cell(1, 0, 0, "ground", atlas_a, grass_uv),
        cell(2, 0, 0, "rock", atlas_b, rock_uv),
        mirakana::TileChunkCellRow{
            .tile_id = "hidden",
            .material_id = "ground",
            .x = 3,
            .y = 0,
            .layer = 0,
            .visible = false,
            .atlas_page = atlas_a,
            .uv_rect = grass_uv,
        },
    };

    return mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .cells = cells,
        .max_sprite_rows = 8,
        .max_draw_rows = 8,
    });
}

} // namespace

MK_TEST("tile chunk renderer builds deterministic opaque transparent material batches") {
    const auto atlas_a = mirakana::AssetId::from_name("textures/tile-atlas-a");
    const auto atlas_b = mirakana::AssetId::from_name("textures/tile-atlas-b");
    const auto plan = make_basic_plan();

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::TileChunkRendererStatus::ready);
    MK_REQUIRE(plan.visible_cell_count == 4);
    MK_REQUIRE(plan.opaque_cell_count == 3);
    MK_REQUIRE(plan.transparent_cell_count == 1);
    MK_REQUIRE(plan.sprite_rows.size() == 4);
    MK_REQUIRE(plan.draw_rows.size() == 3);
    MK_REQUIRE(plan.sprite_batch_plan.succeeded());
    MK_REQUIRE(plan.sprite_batch_plan.sprite_count == 4);
    MK_REQUIRE(plan.sprite_batch_plan.draw_count == 3);
    MK_REQUIRE(plan.sprite_batch_plan.texture_bind_count == 3);

    MK_REQUIRE(plan.draw_rows[0].pass == mirakana::TileChunkRenderPass::opaque);
    MK_REQUIRE(plan.draw_rows[0].layer == 0);
    MK_REQUIRE(plan.draw_rows[0].material_id == "ground");
    MK_REQUIRE(plan.draw_rows[0].atlas_page == atlas_a);
    MK_REQUIRE(plan.draw_rows[0].first_sprite == 0);
    MK_REQUIRE(plan.draw_rows[0].sprite_count == 2);
    MK_REQUIRE(plan.draw_rows[1].material_id == "rock");
    MK_REQUIRE(plan.draw_rows[1].atlas_page == atlas_b);
    MK_REQUIRE(plan.draw_rows[2].pass == mirakana::TileChunkRenderPass::transparent);
    MK_REQUIRE(plan.draw_rows[2].material_id == "foliage");
    MK_REQUIRE(plan.draw_rows[2].atlas_page == atlas_a);

    MK_REQUIRE(plan.sprite_rows[0].tile_id == "tile");
    MK_REQUIRE(plan.sprite_rows[0].x == 0);
    MK_REQUIRE(plan.sprite_rows[0].y == 0);
    MK_REQUIRE(plan.sprite_rows[0].sprite.transform.position.x == 0.0F);
    MK_REQUIRE(plan.sprite_rows[0].sprite.transform.position.y == 0.0F);
    MK_REQUIRE(plan.sprite_rows[0].sprite.transform.scale.x == 16.0F);
    MK_REQUIRE(plan.sprite_rows[0].sprite.transform.scale.y == 16.0F);
    MK_REQUIRE(plan.sprite_rows[1].sprite.transform.position.x == 16.0F);
    MK_REQUIRE(plan.sprite_rows[3].pass == mirakana::TileChunkRenderPass::transparent);
}

MK_TEST("tile chunk renderer budgets dirty region rebuild rows") {
    const auto clean_plan = mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .max_dirty_rebuild_cells = 1,
    });
    MK_REQUIRE(clean_plan.succeeded());
    MK_REQUIRE(clean_plan.dirty_rebuild_rows.empty());

    const std::array<mirakana::TileChunkDirtyRegion, 2> dirty_regions{
        mirakana::TileChunkDirtyRegion{.x = 1, .y = 1, .width = 2, .height = 1},
        mirakana::TileChunkDirtyRegion{.full_chunk = true},
    };
    const auto plan = mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .dirty_regions = dirty_regions,
        .max_dirty_rebuild_cells = 20,
    });
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.dirty_rebuild_rows.size() == 2);
    MK_REQUIRE(plan.dirty_rebuild_rows[0].affected_cells == 2);
    MK_REQUIRE(!plan.dirty_rebuild_rows[0].full_chunk);
    MK_REQUIRE(plan.dirty_rebuild_rows[1].affected_cells == 16);
    MK_REQUIRE(plan.dirty_rebuild_rows[1].full_chunk);

    const auto over_budget = mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .dirty_regions = dirty_regions,
        .max_dirty_rebuild_cells = 4,
    });
    MK_REQUIRE(!over_budget.succeeded());
    MK_REQUIRE(over_budget.status == mirakana::TileChunkRendererStatus::budget_exceeded);
    MK_REQUIRE(over_budget.diagnostics.size() == 1);
    MK_REQUIRE(over_budget.diagnostics[0].code ==
               mirakana::TileChunkRendererDiagnosticCode::dirty_rebuild_budget_exceeded);
}

MK_TEST("tile chunk renderer emits renderer neutral lightmap rows") {
    const auto light_texture = mirakana::AssetId::from_name("textures/lightmap/chunk_0_0");
    const std::array<mirakana::TileChunkLightCellRow, 2> light_cells{
        mirakana::TileChunkLightCellRow{
            .x = 0,
            .y = 0,
            .color = mirakana::Color{.r = 1.0F, .g = 0.85F, .b = 0.65F, .a = 1.0F},
            .changed = true,
            .light_texture = light_texture,
        },
        mirakana::TileChunkLightCellRow{
            .x = 1,
            .y = 0,
            .color = mirakana::Color{.r = 0.25F, .g = 0.35F, .b = 0.9F, .a = 1.0F},
            .changed = false,
            .light_texture = light_texture,
        },
    };

    const auto plan = mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .light_cells = light_cells,
        .light_texture = light_texture,
        .max_light_rows = 4,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.lightmap_rows.size() == 2);
    MK_REQUIRE(plan.changed_light_cell_count == 1);
    MK_REQUIRE(plan.light_texture_dependency == light_texture);
    MK_REQUIRE(plan.lightmap_rows[0].changed);
    MK_REQUIRE(plan.lightmap_rows[0].color.r == 1.0F);
    MK_REQUIRE(!plan.invoked_backend_specific_submission);
    MK_REQUIRE(!plan.invoked_native_texture_ownership);
}

MK_TEST("tile chunk renderer rejects invalid cells and backend ownership claims") {
    const auto atlas = mirakana::AssetId::from_name("textures/tile-atlas-a");
    const std::array<mirakana::TileChunkCellRow, 4> cells{
        cell(0, 0, 0, "ground", atlas, mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}),
        cell(0, 0, 0, "ground", atlas, mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}),
        cell(4, 0, 0, "ground", atlas, mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}),
        cell(1, 0, 0, "ground", mirakana::AssetId{},
             mirakana::SpriteUvRect{.u0 = 0.75F, .v0 = 0.0F, .u1 = 0.25F, .v1 = 1.0F}),
    };

    const auto plan = mirakana::plan_tile_chunk_renderer(mirakana::TileChunkRendererDesc{
        .chunk_id = "chunk/town/0_0",
        .chunk_size = mirakana::Extent2D{.width = 4, .height = 4},
        .tile_size = mirakana::Extent2D{.width = 16, .height = 16},
        .cells = cells,
        .max_sprite_rows = 2,
        .request_native_texture_ownership = true,
        .request_backend_specific_submission = true,
    });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::TileChunkRendererStatus::budget_exceeded);
    MK_REQUIRE(plan.invoked_native_texture_ownership == false);
    MK_REQUIRE(plan.invoked_backend_specific_submission == false);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::duplicate_cell) == 1);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::invalid_cell_bounds) == 1);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::missing_atlas_page) == 1);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::invalid_uv_rect) == 1);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::sprite_row_budget_exceeded) == 1);
    MK_REQUIRE(plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::unsupported_native_texture_ownership) ==
               1);
    MK_REQUIRE(
        plan.diagnostic_count(mirakana::TileChunkRendererDiagnosticCode::unsupported_backend_specific_submission) == 1);
}

int main() {
    return mirakana::test::run_all();
}
