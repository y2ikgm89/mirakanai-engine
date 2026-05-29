// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/sprite_batch.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class TileChunkRenderPass : std::uint8_t { opaque, transparent };

enum class TileChunkRendererStatus : std::uint8_t {
    ready,
    invalid_request,
    diagnostics,
    budget_exceeded,
};

enum class TileChunkRendererDiagnosticCode : std::uint8_t {
    none,
    missing_chunk_id,
    invalid_chunk_extent,
    invalid_tile_extent,
    duplicate_cell,
    invalid_cell_bounds,
    missing_atlas_page,
    invalid_uv_rect,
    invalid_dirty_region,
    dirty_rebuild_budget_exceeded,
    missing_light_texture,
    invalid_light_cell_bounds,
    sprite_row_budget_exceeded,
    draw_row_budget_exceeded,
    light_row_budget_exceeded,
    sprite_batch_plan_failed,
    unsupported_native_texture_ownership,
    unsupported_backend_specific_submission,
};

struct TileChunkCellRow {
    std::string tile_id;
    std::string material_id;
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t layer{0};
    bool visible{true};
    bool transparent{false};
    AssetId atlas_page;
    SpriteUvRect uv_rect;
    Color color;
};

struct TileChunkDirtyRegion {
    std::int32_t x{0};
    std::int32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool full_chunk{false};
};

struct TileChunkLightCellRow {
    std::int32_t x{0};
    std::int32_t y{0};
    Color color;
    bool changed{false};
    AssetId light_texture;
};

struct TileChunkRendererDiagnostic {
    TileChunkRendererDiagnosticCode code{TileChunkRendererDiagnosticCode::none};
    std::string chunk_id;
    std::uint64_t row_index{0};
};

struct TileChunkSpriteRow {
    std::string tile_id;
    std::string material_id;
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t layer{0};
    TileChunkRenderPass pass{TileChunkRenderPass::opaque};
    SpriteCommand sprite;
};

struct TileChunkDrawRow {
    TileChunkRenderPass pass{TileChunkRenderPass::opaque};
    std::int32_t layer{0};
    std::string material_id;
    AssetId atlas_page;
    std::uint64_t first_sprite{0};
    std::uint64_t sprite_count{0};
};

struct TileChunkDirtyRebuildRow {
    std::int32_t x{0};
    std::int32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool full_chunk{false};
    std::uint64_t affected_cells{0};
};

struct TileChunkLightmapRow {
    std::int32_t x{0};
    std::int32_t y{0};
    Color color;
    bool changed{false};
    AssetId light_texture;
};

struct TileChunkRendererDesc {
    std::string_view chunk_id;
    Extent2D chunk_size;
    Extent2D tile_size;
    std::span<const TileChunkCellRow> cells;
    std::span<const TileChunkDirtyRegion> dirty_regions;
    std::span<const TileChunkLightCellRow> light_cells;
    AssetId light_texture;
    std::uint64_t max_sprite_rows{0};
    std::uint64_t max_draw_rows{0};
    std::uint64_t max_dirty_rebuild_cells{0};
    std::uint64_t max_light_rows{0};
    bool request_native_texture_ownership{false};
    bool request_backend_specific_submission{false};
};

struct TileChunkRendererPlan {
    TileChunkRendererStatus status{TileChunkRendererStatus::invalid_request};
    std::string chunk_id;
    std::vector<TileChunkSpriteRow> sprite_rows;
    std::vector<TileChunkDrawRow> draw_rows;
    std::vector<TileChunkDirtyRebuildRow> dirty_rebuild_rows;
    std::vector<TileChunkLightmapRow> lightmap_rows;
    std::vector<TileChunkRendererDiagnostic> diagnostics;
    SpriteBatchPlan sprite_batch_plan;
    AssetId light_texture_dependency;
    std::uint64_t visible_cell_count{0};
    std::uint64_t opaque_cell_count{0};
    std::uint64_t transparent_cell_count{0};
    std::uint64_t changed_light_cell_count{0};
    bool invoked_backend_specific_submission{false};
    bool invoked_native_texture_ownership{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == TileChunkRendererStatus::ready && diagnostics.empty();
    }

    [[nodiscard]] std::size_t diagnostic_count(TileChunkRendererDiagnosticCode code) const noexcept;
};

[[nodiscard]] TileChunkRendererPlan plan_tile_chunk_renderer(const TileChunkRendererDesc& desc);

} // namespace mirakana
