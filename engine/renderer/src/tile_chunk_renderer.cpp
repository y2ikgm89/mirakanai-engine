// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/tile_chunk_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <tuple>

namespace mirakana {
namespace {

struct OrderedCell {
    TileChunkCellRow row;
    std::uint64_t source_index{0};
};

[[nodiscard]] bool valid_extent(Extent2D extent) noexcept {
    return extent.width != 0U && extent.height != 0U;
}

[[nodiscard]] bool valid_uv_rect(SpriteUvRect rect) noexcept {
    return std::isfinite(rect.u0) && std::isfinite(rect.v0) && std::isfinite(rect.u1) && std::isfinite(rect.v1) &&
           rect.u0 >= 0.0F && rect.v0 >= 0.0F && rect.u1 <= 1.0F && rect.v1 <= 1.0F && rect.u0 < rect.u1 &&
           rect.v0 < rect.v1;
}

[[nodiscard]] bool cell_in_bounds(const TileChunkCellRow& cell, Extent2D chunk_size) noexcept {
    return cell.x >= 0 && cell.y >= 0 && static_cast<std::uint32_t>(cell.x) < chunk_size.width &&
           static_cast<std::uint32_t>(cell.y) < chunk_size.height;
}

[[nodiscard]] bool light_cell_in_bounds(const TileChunkLightCellRow& cell, Extent2D chunk_size) noexcept {
    return cell.x >= 0 && cell.y >= 0 && static_cast<std::uint32_t>(cell.x) < chunk_size.width &&
           static_cast<std::uint32_t>(cell.y) < chunk_size.height;
}

[[nodiscard]] bool dirty_region_in_bounds(const TileChunkDirtyRegion& region, Extent2D chunk_size) noexcept {
    if (region.full_chunk) {
        return true;
    }
    if (region.width == 0U || region.height == 0U || region.x < 0 || region.y < 0) {
        return false;
    }
    const auto right = static_cast<std::uint64_t>(region.x) + region.width;
    const auto bottom = static_cast<std::uint64_t>(region.y) + region.height;
    return right <= chunk_size.width && bottom <= chunk_size.height;
}

void add_diagnostic(TileChunkRendererPlan& plan, TileChunkRendererDiagnosticCode code, std::uint64_t row_index) {
    plan.diagnostics.push_back(TileChunkRendererDiagnostic{
        .code = code,
        .chunk_id = plan.chunk_id,
        .row_index = row_index,
    });
}

[[nodiscard]] TileChunkRenderPass pass_for_cell(const TileChunkCellRow& cell) noexcept {
    return cell.transparent ? TileChunkRenderPass::transparent : TileChunkRenderPass::opaque;
}

[[nodiscard]] bool ordered_cell_less(const OrderedCell& lhs, const OrderedCell& rhs) {
    const auto lhs_pass = pass_for_cell(lhs.row);
    const auto rhs_pass = pass_for_cell(rhs.row);
    return std::tuple{lhs_pass,  lhs.row.layer, lhs.row.material_id, lhs.row.atlas_page.value,
                      lhs.row.y, lhs.row.x,     lhs.source_index} <
           std::tuple{rhs_pass,  rhs.row.layer, rhs.row.material_id, rhs.row.atlas_page.value,
                      rhs.row.y, rhs.row.x,     rhs.source_index};
}

[[nodiscard]] SpriteCommand make_sprite_command(const TileChunkCellRow& cell, Extent2D tile_size) {
    return SpriteCommand{
        .transform =
            Transform2D{
                .position = Vec2{.x = static_cast<float>(cell.x * static_cast<std::int32_t>(tile_size.width)),
                                 .y = static_cast<float>(cell.y * static_cast<std::int32_t>(tile_size.height))},
                .scale = Vec2{.x = static_cast<float>(tile_size.width), .y = static_cast<float>(tile_size.height)},
            },
        .color = cell.color,
        .texture =
            SpriteTextureRegion{
                .enabled = true,
                .atlas_page = cell.atlas_page,
                .uv_rect = cell.uv_rect,
            },
    };
}

void append_or_extend_draw_row(TileChunkRendererPlan& plan, const TileChunkCellRow& cell, TileChunkRenderPass pass,
                               std::uint64_t sprite_index) {
    if (!plan.draw_rows.empty()) {
        auto& last = plan.draw_rows.back();
        if (last.pass == pass && last.layer == cell.layer && last.material_id == cell.material_id &&
            last.atlas_page == cell.atlas_page) {
            ++last.sprite_count;
            return;
        }
    }

    plan.draw_rows.push_back(TileChunkDrawRow{
        .pass = pass,
        .layer = cell.layer,
        .material_id = cell.material_id,
        .atlas_page = cell.atlas_page,
        .first_sprite = sprite_index,
        .sprite_count = 1U,
    });
}

[[nodiscard]] std::uint64_t chunk_cell_count(Extent2D chunk_size) noexcept {
    return static_cast<std::uint64_t>(chunk_size.width) * static_cast<std::uint64_t>(chunk_size.height);
}

void append_dirty_rebuild_rows(TileChunkRendererPlan& plan, const TileChunkRendererDesc& desc) {
    std::uint64_t total_affected_cells{0};
    std::uint64_t row_index{0};
    for (const auto& region : desc.dirty_regions) {
        if (!dirty_region_in_bounds(region, desc.chunk_size)) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_dirty_region, row_index);
            ++row_index;
            continue;
        }

        TileChunkDirtyRebuildRow row;
        if (region.full_chunk) {
            row = TileChunkDirtyRebuildRow{
                .x = 0,
                .y = 0,
                .width = desc.chunk_size.width,
                .height = desc.chunk_size.height,
                .full_chunk = true,
                .affected_cells = chunk_cell_count(desc.chunk_size),
            };
        } else {
            row = TileChunkDirtyRebuildRow{
                .x = region.x,
                .y = region.y,
                .width = region.width,
                .height = region.height,
                .full_chunk = false,
                .affected_cells = static_cast<std::uint64_t>(region.width) * static_cast<std::uint64_t>(region.height),
            };
        }
        total_affected_cells += row.affected_cells;
        plan.dirty_rebuild_rows.push_back(row);
        ++row_index;
    }

    if (desc.max_dirty_rebuild_cells != 0U && total_affected_cells > desc.max_dirty_rebuild_cells) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::dirty_rebuild_budget_exceeded, 0U);
    }
}

void append_light_rows(TileChunkRendererPlan& plan, const TileChunkRendererDesc& desc) {
    std::uint64_t row_index{0};
    for (const auto& light : desc.light_cells) {
        if (!light_cell_in_bounds(light, desc.chunk_size)) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_light_cell_bounds, row_index);
            ++row_index;
            continue;
        }
        const auto texture = desc.light_texture.value != 0U ? desc.light_texture : light.light_texture;
        if (texture.value == 0U) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::missing_light_texture, row_index);
            ++row_index;
            continue;
        }

        if (plan.light_texture_dependency.value == 0U) {
            plan.light_texture_dependency = texture;
        }
        if (light.changed) {
            ++plan.changed_light_cell_count;
        }
        plan.lightmap_rows.push_back(TileChunkLightmapRow{
            .x = light.x,
            .y = light.y,
            .color = light.color,
            .changed = light.changed,
            .light_texture = texture,
        });
        ++row_index;
    }

    if (desc.max_light_rows != 0U && static_cast<std::uint64_t>(plan.lightmap_rows.size()) > desc.max_light_rows) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::light_row_budget_exceeded, 0U);
    }
}

void finalize_status(TileChunkRendererPlan& plan) {
    if (plan.diagnostics.empty()) {
        plan.status = TileChunkRendererStatus::ready;
        return;
    }

    bool has_invalid_request{false};
    bool has_budget_exceeded{false};
    for (const auto& diagnostic : plan.diagnostics) {
        switch (diagnostic.code) {
        case TileChunkRendererDiagnosticCode::missing_chunk_id:
        case TileChunkRendererDiagnosticCode::invalid_chunk_extent:
        case TileChunkRendererDiagnosticCode::invalid_tile_extent:
            has_invalid_request = true;
            break;
        case TileChunkRendererDiagnosticCode::sprite_row_budget_exceeded:
        case TileChunkRendererDiagnosticCode::draw_row_budget_exceeded:
        case TileChunkRendererDiagnosticCode::dirty_rebuild_budget_exceeded:
        case TileChunkRendererDiagnosticCode::light_row_budget_exceeded:
            has_budget_exceeded = true;
            break;
        default:
            break;
        }
    }

    if (has_invalid_request) {
        plan.status = TileChunkRendererStatus::invalid_request;
    } else if (has_budget_exceeded) {
        plan.status = TileChunkRendererStatus::budget_exceeded;
    } else {
        plan.status = TileChunkRendererStatus::diagnostics;
    }
}

} // namespace

std::size_t TileChunkRendererPlan::diagnostic_count(TileChunkRendererDiagnosticCode code) const noexcept {
    std::size_t count{0U};
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

TileChunkRendererPlan plan_tile_chunk_renderer(const TileChunkRendererDesc& desc) {
    TileChunkRendererPlan plan;
    plan.chunk_id = std::string{desc.chunk_id};

    if (desc.chunk_id.empty()) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::missing_chunk_id, 0U);
    }
    if (!valid_extent(desc.chunk_size)) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_chunk_extent, 0U);
    }
    if (!valid_extent(desc.tile_size)) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_tile_extent, 0U);
    }
    if (desc.request_native_texture_ownership) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::unsupported_native_texture_ownership, 0U);
    }
    if (desc.request_backend_specific_submission) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::unsupported_backend_specific_submission, 0U);
    }

    std::set<std::tuple<std::int32_t, std::int32_t, std::int32_t>> occupied_cells;
    std::vector<OrderedCell> valid_cells;
    valid_cells.reserve(desc.cells.size());
    std::uint64_t row_index{0};
    for (const auto& cell : desc.cells) {
        if (!cell.visible) {
            ++row_index;
            continue;
        }
        ++plan.visible_cell_count;

        bool row_valid = true;
        if (!cell_in_bounds(cell, desc.chunk_size)) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_cell_bounds, row_index);
            row_valid = false;
        }
        if (cell.atlas_page.value == 0U) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::missing_atlas_page, row_index);
            row_valid = false;
        }
        if (!valid_uv_rect(cell.uv_rect)) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::invalid_uv_rect, row_index);
            row_valid = false;
        }

        const auto cell_key = std::tuple{cell.layer, cell.y, cell.x};
        if (cell_in_bounds(cell, desc.chunk_size) && !occupied_cells.insert(cell_key).second) {
            add_diagnostic(plan, TileChunkRendererDiagnosticCode::duplicate_cell, row_index);
            row_valid = false;
        }

        if (row_valid) {
            valid_cells.push_back(OrderedCell{.row = cell, .source_index = row_index});
        }
        ++row_index;
    }

    std::ranges::sort(valid_cells, ordered_cell_less);
    std::vector<SpriteCommand> sprites;
    sprites.reserve(valid_cells.size());
    for (const auto& ordered : valid_cells) {
        const auto pass = pass_for_cell(ordered.row);
        if (pass == TileChunkRenderPass::transparent) {
            ++plan.transparent_cell_count;
        } else {
            ++plan.opaque_cell_count;
        }

        const auto sprite_index = static_cast<std::uint64_t>(plan.sprite_rows.size());
        const auto sprite = make_sprite_command(ordered.row, desc.tile_size);
        plan.sprite_rows.push_back(TileChunkSpriteRow{
            .tile_id = ordered.row.tile_id,
            .material_id = ordered.row.material_id,
            .x = ordered.row.x,
            .y = ordered.row.y,
            .layer = ordered.row.layer,
            .pass = pass,
            .sprite = sprite,
        });
        sprites.push_back(sprite);
        append_or_extend_draw_row(plan, ordered.row, pass, sprite_index);
    }

    if (desc.max_sprite_rows != 0U && plan.visible_cell_count > desc.max_sprite_rows) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::sprite_row_budget_exceeded, 0U);
    }
    if (desc.max_draw_rows != 0U && static_cast<std::uint64_t>(plan.draw_rows.size()) > desc.max_draw_rows) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::draw_row_budget_exceeded, 0U);
    }

    plan.sprite_batch_plan = plan_sprite_batches(SpriteBatchPlanDesc{
        .sprites = sprites,
        .options = SpriteBatchPlanOptions{.require_atlas_backed_sprites = true},
    });
    if (!plan.sprite_batch_plan.succeeded()) {
        add_diagnostic(plan, TileChunkRendererDiagnosticCode::sprite_batch_plan_failed, 0U);
    }

    append_dirty_rebuild_rows(plan, desc);
    append_light_rows(plan, desc);
    finalize_status(plan);
    return plan;
}

} // namespace mirakana
