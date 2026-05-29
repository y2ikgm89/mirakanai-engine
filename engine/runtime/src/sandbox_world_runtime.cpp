// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sandbox_world_runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

struct ChunkBounds {
    std::string chunk_id;
    RuntimeSandboxCellCoord origin;
    RuntimeSandboxCellCoord end_exclusive;
};

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_backend_token(std::string_view token) {
    constexpr auto forbidden = std::array<std::string_view, 11U>{
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "metal", "platform", "window", "imgui", "gpu",
    };
    return std::ranges::find(forbidden, token) != forbidden.end();
}

[[nodiscard]] bool has_backend_reference(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_backend_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_backend_token(token);
}

[[nodiscard]] bool same_coord(RuntimeSandboxCellCoord lhs, RuntimeSandboxCellCoord rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool coord_less(RuntimeSandboxCellCoord lhs, RuntimeSandboxCellCoord rhs) noexcept {
    if (lhs.x != rhs.x) {
        return lhs.x < rhs.x;
    }
    if (lhs.y != rhs.y) {
        return lhs.y < rhs.y;
    }
    return lhs.z < rhs.z;
}

[[nodiscard]] bool is_valid_chunk_extent(const RuntimeSandboxChunkRow& row) noexcept {
    if (row.size_x == 0U || row.size_y == 0U || row.size_z == 0U) {
        return false;
    }
    const auto end_x = static_cast<std::int64_t>(row.origin_x) + static_cast<std::int64_t>(row.size_x);
    const auto end_y = static_cast<std::int64_t>(row.origin_y) + static_cast<std::int64_t>(row.size_y);
    const auto end_z = static_cast<std::int64_t>(row.origin_z) + static_cast<std::int64_t>(row.size_z);
    return end_x <= static_cast<std::int64_t>(INT32_MAX) && end_y <= static_cast<std::int64_t>(INT32_MAX) &&
           end_z <= static_cast<std::int64_t>(INT32_MAX);
}

[[nodiscard]] RuntimeSandboxCellCoord end_coord(const RuntimeSandboxChunkRow& row) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_x) + static_cast<std::int64_t>(row.size_x)),
        .y = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_y) + static_cast<std::int64_t>(row.size_y)),
        .z = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_z) + static_cast<std::int64_t>(row.size_z)),
    };
}

[[nodiscard]] bool contains_coord(const ChunkBounds& chunk, RuntimeSandboxCellCoord coord) noexcept {
    return coord.x >= chunk.origin.x && coord.y >= chunk.origin.y && coord.z >= chunk.origin.z &&
           coord.x < chunk.end_exclusive.x && coord.y < chunk.end_exclusive.y && coord.z < chunk.end_exclusive.z;
}

[[nodiscard]] bool chunks_overlap(const ChunkBounds& lhs, const ChunkBounds& rhs) noexcept {
    return lhs.origin.x < rhs.end_exclusive.x && rhs.origin.x < lhs.end_exclusive.x &&
           lhs.origin.y < rhs.end_exclusive.y && rhs.origin.y < lhs.end_exclusive.y &&
           lhs.origin.z < rhs.end_exclusive.z && rhs.origin.z < lhs.end_exclusive.z;
}

[[nodiscard]] std::string make_cell_key(std::string_view chunk_id, RuntimeSandboxCellCoord coord) {
    auto key = std::string{chunk_id};
    key.push_back('\n');
    key.append(std::to_string(coord.x));
    key.push_back('\n');
    key.append(std::to_string(coord.y));
    key.push_back('\n');
    key.append(std::to_string(coord.z));
    return key;
}

[[nodiscard]] const ChunkBounds* find_chunk_by_id(const std::vector<ChunkBounds>& chunks, std::string_view id) {
    const auto iter = std::ranges::find_if(chunks, [id](const auto& chunk) { return chunk.chunk_id == id; });
    if (iter == chunks.end()) {
        return nullptr;
    }
    return &(*iter);
}

void add_diagnostic(RuntimeSandboxWorldBuildResult& result, RuntimeSandboxWorldRuntimeDiagnosticCode code,
                    const RuntimeSandboxWorldDesc& desc, std::string row_id, std::string chunk_id,
                    RuntimeSandboxCellCoord coord, std::string message, std::uint32_t source_index) {
    result.diagnostics.push_back(RuntimeSandboxWorldRuntimeDiagnostic{
        .code = code,
        .world_id = desc.world_id,
        .row_id = std::move(row_id),
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeSandboxWorldBuildResult& result) {
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.world_id != rhs.world_id) {
            return lhs.world_id < rhs.world_id;
        }
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (!same_coord(lhs.coord, rhs.coord)) {
            return coord_less(lhs.coord, rhs.coord);
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::size_t desc_row_count(const RuntimeSandboxWorldDesc& desc) noexcept {
    return desc.chunk_rows.size() + desc.existing_cell_rows.size();
}

[[nodiscard]] bool chunk_less(const RuntimeSandboxChunkRow& lhs, const RuntimeSandboxChunkRow& rhs) {
    if (lhs.origin_x != rhs.origin_x) {
        return lhs.origin_x < rhs.origin_x;
    }
    if (lhs.origin_y != rhs.origin_y) {
        return lhs.origin_y < rhs.origin_y;
    }
    if (lhs.origin_z != rhs.origin_z) {
        return lhs.origin_z < rhs.origin_z;
    }
    return lhs.chunk_id < rhs.chunk_id;
}

[[nodiscard]] bool cell_less(const RuntimeSandboxExistingCellRow& lhs, const RuntimeSandboxExistingCellRow& rhs) {
    if (!same_coord(lhs.coord, rhs.coord)) {
        return coord_less(lhs.coord, rhs.coord);
    }
    if (lhs.chunk_id != rhs.chunk_id) {
        return lhs.chunk_id < rhs.chunk_id;
    }
    return lhs.block_id < rhs.block_id;
}

[[nodiscard]] auto find_world_cell(std::vector<RuntimeSandboxExistingCellRow>& cells, std::string_view chunk_id,
                                   RuntimeSandboxCellCoord coord) {
    return std::ranges::find_if(cells, [chunk_id, coord](const auto& cell) {
        return cell.chunk_id == chunk_id && same_coord(cell.coord, coord);
    });
}

[[nodiscard]] auto find_world_cell(const std::vector<RuntimeSandboxExistingCellRow>& cells, std::string_view chunk_id,
                                   RuntimeSandboxCellCoord coord) {
    return std::ranges::find_if(cells, [chunk_id, coord](const auto& cell) {
        return cell.chunk_id == chunk_id && same_coord(cell.coord, coord);
    });
}

[[nodiscard]] const RuntimeSandboxTileMaterialRow*
find_material(const std::vector<RuntimeSandboxTileMaterialRow>& materials, std::string_view tile_id) {
    const auto iter =
        std::ranges::find_if(materials, [tile_id](const auto& material) { return material.tile_id == tile_id; });
    if (iter == materials.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] bool world_contains_cell(const std::vector<RuntimeSandboxChunkRow>& chunks, std::string_view chunk_id,
                                       RuntimeSandboxCellCoord coord) noexcept {
    const auto iter =
        std::ranges::find_if(chunks, [chunk_id](const auto& chunk) { return chunk.chunk_id == chunk_id; });
    if (iter == chunks.end()) {
        return false;
    }

    return contains_coord(
        ChunkBounds{
            .chunk_id = iter->chunk_id,
            .origin =
                RuntimeSandboxCellCoord{
                    .x = iter->origin_x,
                    .y = iter->origin_y,
                    .z = iter->origin_z,
                },
            .end_exclusive = end_coord(*iter),
        },
        coord);
}

void mix_hash(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void mix_hash(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(ch)));
    }
    mix_hash(hash, 0xffU);
}

void mix_hash(std::uint64_t& hash, RuntimeSandboxCellCoord coord) noexcept {
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.x)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.y)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.z)));
}

[[nodiscard]] std::uint64_t compute_snapshot_hash(const RuntimeSandboxWorld& world) noexcept {
    auto hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(hash, world.world_id);
    mix_hash(hash, world.world_tick);
    mix_hash(hash, world.chunks.size());
    mix_hash(hash, world.cells.size());
    for (const auto& chunk : world.chunks) {
        mix_hash(hash, chunk.chunk_id);
        mix_hash(hash, chunk.region_id);
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(chunk.origin_x)));
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(chunk.origin_y)));
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(chunk.origin_z)));
        mix_hash(hash, chunk.size_x);
        mix_hash(hash, chunk.size_y);
        mix_hash(hash, chunk.size_z);
        mix_hash(hash, chunk.resident ? 1U : 0U);
        mix_hash(hash, chunk.persistent ? 1U : 0U);
        mix_hash(hash, chunk.source_index);
    }
    for (const auto& cell : world.cells) {
        mix_hash(hash, cell.chunk_id);
        mix_hash(hash, cell.coord);
        mix_hash(hash, cell.block_id);
        mix_hash(hash, cell.destructible ? 1U : 0U);
        mix_hash(hash, cell.protected_cell ? 1U : 0U);
        mix_hash(hash, cell.source_index);
    }
    mix_hash(hash, world.invoked_persistence_io ? 1U : 0U);
    mix_hash(hash, world.invoked_package_io ? 1U : 0U);
    mix_hash(hash, world.invoked_renderer_upload ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t layer_mask_for(RuntimeSandboxCellCoord coord) noexcept {
    if (coord.z < 0 || coord.z >= 64) {
        return 0U;
    }
    return 1ULL << static_cast<std::uint64_t>(coord.z);
}

[[nodiscard]] RuntimeSandboxCellCoord next_cell_coord(RuntimeSandboxCellCoord coord) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(coord.x + 1),
        .y = static_cast<std::int32_t>(coord.y + 1),
        .z = static_cast<std::int32_t>(coord.z + 1),
    };
}

[[nodiscard]] std::uint64_t compute_dirty_region_hash(const RuntimeSandboxMutationRow& row,
                                                      std::string_view previous_block_id,
                                                      std::string_view new_block_id) noexcept {
    auto hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(hash, static_cast<std::uint64_t>(row.kind));
    mix_hash(hash, row.intent_id);
    mix_hash(hash, row.chunk_id);
    mix_hash(hash, row.coord);
    mix_hash(hash, previous_block_id);
    mix_hash(hash, new_block_id);
    mix_hash(hash, row.source_index);
    return hash == 0U ? 1U : hash;
}

void mix_hash(std::uint64_t& hash, const RuntimeSandboxWorldDirtyRegion& region) noexcept {
    mix_hash(hash, region.chunk_id);
    mix_hash(hash, region.intent_id);
    mix_hash(hash, region.min_coord);
    mix_hash(hash, region.max_coord_exclusive);
    mix_hash(hash, region.layer_mask);
    mix_hash(hash, region.previous_block_id);
    mix_hash(hash, region.new_block_id);
    mix_hash(hash, region.replay_hash);
    mix_hash(hash, region.chunk_dirty ? 1U : 0U);
}

[[nodiscard]] std::uint64_t compute_tile_row_hash(std::string_view chunk_id, RuntimeSandboxCellCoord coord,
                                                  std::string_view tile_id, std::uint32_t salt) noexcept {
    auto hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(hash, chunk_id);
    mix_hash(hash, coord);
    mix_hash(hash, tile_id);
    mix_hash(hash, salt);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] RuntimeSandboxCellCoord offset_coord(RuntimeSandboxCellCoord coord, std::int32_t x,
                                                   std::int32_t y) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(coord.x + x),
        .y = static_cast<std::int32_t>(coord.y + y),
        .z = coord.z,
    };
}

void add_diagnostic(RuntimeSandboxTileSimulationPlan& plan, RuntimeSandboxTileSimulationDiagnosticCode code,
                    std::string tile_id, std::string chunk_id, RuntimeSandboxCellCoord coord, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeSandboxTileSimulationDiagnostic{
        .code = code,
        .tile_id = std::move(tile_id),
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeSandboxTileSimulationPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.tile_id != rhs.tile_id) {
            return lhs.tile_id < rhs.tile_id;
        }
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        if (!same_coord(lhs.coord, rhs.coord)) {
            return coord_less(lhs.coord, rhs.coord);
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] bool material_less(const RuntimeSandboxTileMaterialRow& lhs, const RuntimeSandboxTileMaterialRow& rhs) {
    if (lhs.tile_id != rhs.tile_id) {
        return lhs.tile_id < rhs.tile_id;
    }
    return lhs.source_index < rhs.source_index;
}

[[nodiscard]] bool cell_row_less(const RuntimeSandboxTileCellRow& lhs, const RuntimeSandboxTileCellRow& rhs) {
    if (!same_coord(lhs.coord, rhs.coord)) {
        return coord_less(lhs.coord, rhs.coord);
    }
    if (lhs.chunk_id != rhs.chunk_id) {
        return lhs.chunk_id < rhs.chunk_id;
    }
    return lhs.tile_id < rhs.tile_id;
}

[[nodiscard]] bool span_row_less(const RuntimeSandboxTileCollisionSpanRow& lhs,
                                 const RuntimeSandboxTileCollisionSpanRow& rhs) {
    if (!same_coord(lhs.min_coord, rhs.min_coord)) {
        return coord_less(lhs.min_coord, rhs.min_coord);
    }
    if (lhs.chunk_id != rhs.chunk_id) {
        return lhs.chunk_id < rhs.chunk_id;
    }
    return lhs.tile_id < rhs.tile_id;
}

[[nodiscard]] bool light_row_less(const RuntimeSandboxLightPropagationRow& lhs,
                                  const RuntimeSandboxLightPropagationRow& rhs) {
    if (!same_coord(lhs.source_coord, rhs.source_coord)) {
        return coord_less(lhs.source_coord, rhs.source_coord);
    }
    if (!same_coord(lhs.target_coord, rhs.target_coord)) {
        return coord_less(lhs.target_coord, rhs.target_coord);
    }
    return lhs.source_tile_id < rhs.source_tile_id;
}

[[nodiscard]] bool liquid_row_less(const RuntimeSandboxLiquidFlowRow& lhs, const RuntimeSandboxLiquidFlowRow& rhs) {
    if (!same_coord(lhs.source_coord, rhs.source_coord)) {
        return coord_less(lhs.source_coord, rhs.source_coord);
    }
    if (!same_coord(lhs.target_coord, rhs.target_coord)) {
        return coord_less(lhs.target_coord, rhs.target_coord);
    }
    return lhs.source_tile_id < rhs.source_tile_id;
}

[[nodiscard]] bool scheduled_row_less(const RuntimeSandboxScheduledTileUpdateRow& lhs,
                                      const RuntimeSandboxScheduledTileUpdateRow& rhs) {
    if (!same_coord(lhs.coord, rhs.coord)) {
        return coord_less(lhs.coord, rhs.coord);
    }
    if (lhs.scheduled_tick != rhs.scheduled_tick) {
        return lhs.scheduled_tick < rhs.scheduled_tick;
    }
    return lhs.tile_id < rhs.tile_id;
}

[[nodiscard]] bool is_valid_material_row(const RuntimeSandboxTileMaterialRow& material,
                                         const RuntimeSandboxTileSimulationDesc& desc) {
    return is_valid_id(material.tile_id) && !has_backend_reference(material.tile_id) && material.render_layer >= 0 &&
           material.render_layer <= 63 && material.light_radius <= desc.light_radius_budget;
}

[[nodiscard]] std::uint64_t compute_collision_span_hash(const RuntimeSandboxTileCollisionSpanRow& row) noexcept {
    auto hash = compute_tile_row_hash(row.chunk_id, row.min_coord, row.tile_id, 11U);
    mix_hash(hash, row.max_coord_exclusive);
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.render_layer)));
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t compute_tile_cell_hash(const RuntimeSandboxTileCellRow& row) noexcept {
    auto hash = compute_tile_row_hash(row.chunk_id, row.coord, row.tile_id, 23U);
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.render_layer)));
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t compute_light_row_hash(const RuntimeSandboxLightPropagationRow& row) noexcept {
    auto hash = compute_tile_row_hash(row.chunk_id, row.source_coord, row.source_tile_id, 37U);
    mix_hash(hash, row.target_coord);
    mix_hash(hash, row.blocking_tile_id);
    mix_hash(hash, row.intensity);
    mix_hash(hash, row.blocked_by_solid ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t compute_liquid_row_hash(const RuntimeSandboxLiquidFlowRow& row) noexcept {
    auto hash = compute_tile_row_hash(row.chunk_id, row.source_coord, row.source_tile_id, 41U);
    mix_hash(hash, row.target_coord);
    mix_hash(hash, row.blocking_tile_id);
    mix_hash(hash, row.blocked ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t compute_scheduled_row_hash(const RuntimeSandboxScheduledTileUpdateRow& row) noexcept {
    auto hash = compute_tile_row_hash(row.chunk_id, row.coord, row.tile_id, 43U);
    mix_hash(hash, row.cadence_ticks);
    mix_hash(hash, row.scheduled_tick);
    return hash == 0U ? 1U : hash;
}

void clear_tile_simulation_outputs(RuntimeSandboxTileSimulationPlan& plan) {
    plan.solid_collision_spans.clear();
    plan.platform_collision_spans.clear();
    plan.liquid_cells.clear();
    plan.trigger_cells.clear();
    plan.scheduled_update_rows.clear();
    plan.light_rows.clear();
    plan.liquid_flow_rows.clear();
    plan.replay_hash = 0U;
}

[[nodiscard]] std::size_t tile_simulation_output_row_count(const RuntimeSandboxTileSimulationPlan& plan) noexcept {
    return plan.solid_collision_spans.size() + plan.platform_collision_spans.size() + plan.liquid_cells.size() +
           plan.trigger_cells.size() + plan.scheduled_update_rows.size() + plan.light_rows.size() +
           plan.liquid_flow_rows.size();
}

[[nodiscard]] bool sample_is_solid(const RuntimeSandboxCellSample& sample,
                                   const std::vector<RuntimeSandboxTileMaterialRow>& materials) {
    if (sample.status != RuntimeSandboxCellSampleStatus::occupied) {
        return false;
    }
    const auto* material = find_material(materials, sample.block_id);
    return material != nullptr && material->solid;
}

void append_light_rows(RuntimeSandboxTileSimulationPlan& plan, const RuntimeSandboxWorld& world,
                       const RuntimeSandboxExistingCellRow& cell, const RuntimeSandboxTileMaterialRow& material,
                       const RuntimeSandboxTileSimulationDesc& desc,
                       const std::vector<RuntimeSandboxTileMaterialRow>& materials) {
    const auto radius = std::min(material.light_radius, desc.light_radius_budget);
    auto source_row = RuntimeSandboxLightPropagationRow{
        .chunk_id = cell.chunk_id,
        .source_coord = cell.coord,
        .target_coord = cell.coord,
        .source_tile_id = cell.block_id,
        .intensity = radius + 1U,
    };
    source_row.replay_hash = compute_light_row_hash(source_row);
    plan.light_rows.push_back(std::move(source_row));

    constexpr auto directions =
        std::array<std::pair<std::int32_t, std::int32_t>, 4U>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    for (auto distance = std::uint32_t{1U}; distance <= radius; ++distance) {
        for (const auto [x, y] : directions) {
            const auto target =
                offset_coord(cell.coord, static_cast<std::int32_t>(x * static_cast<std::int32_t>(distance)),
                             static_cast<std::int32_t>(y * static_cast<std::int32_t>(distance)));
            const auto sample = sample_runtime_sandbox_cell(world, target);
            if (sample.status == RuntimeSandboxCellSampleStatus::missing_chunk) {
                continue;
            }
            auto row = RuntimeSandboxLightPropagationRow{
                .chunk_id = cell.chunk_id,
                .source_coord = cell.coord,
                .target_coord = target,
                .source_tile_id = cell.block_id,
                .blocking_tile_id = sample_is_solid(sample, materials) ? sample.block_id : std::string{},
                .intensity = radius - distance + 1U,
                .blocked_by_solid = sample_is_solid(sample, materials),
            };
            row.replay_hash = compute_light_row_hash(row);
            plan.light_rows.push_back(std::move(row));
        }
    }
}

void append_liquid_flow_row(RuntimeSandboxTileSimulationPlan& plan, const RuntimeSandboxWorld& world,
                            const RuntimeSandboxExistingCellRow& cell,
                            const std::vector<RuntimeSandboxTileMaterialRow>& materials) {
    const auto target = offset_coord(cell.coord, 1, 0);
    const auto sample = sample_runtime_sandbox_cell(world, target);
    auto blocked = sample.status == RuntimeSandboxCellSampleStatus::missing_chunk;
    auto blocking_tile_id = std::string{};
    if (sample.status == RuntimeSandboxCellSampleStatus::occupied) {
        const auto* target_material = find_material(materials, sample.block_id);
        blocked = target_material == nullptr || !target_material->replaceable;
        if (blocked) {
            blocking_tile_id = sample.block_id;
        }
    }

    auto row = RuntimeSandboxLiquidFlowRow{
        .chunk_id = cell.chunk_id,
        .source_coord = cell.coord,
        .target_coord = target,
        .source_tile_id = cell.block_id,
        .blocking_tile_id = std::move(blocking_tile_id),
        .blocked = blocked,
    };
    row.replay_hash = compute_liquid_row_hash(row);
    plan.liquid_flow_rows.push_back(std::move(row));
}

} // namespace

bool RuntimeSandboxWorldBuildResult::succeeded() const noexcept {
    return status == RuntimeSandboxWorldRuntimeStatus::ready && diagnostics.empty();
}

bool RuntimeSandboxTileSimulationPlan::succeeded() const noexcept {
    return status == RuntimeSandboxTileSimulationStatus::ready && diagnostics.empty();
}

RuntimeSandboxWorldBuildResult build_runtime_sandbox_world(const RuntimeSandboxWorldDesc& desc) {
    RuntimeSandboxWorldBuildResult result;
    result.world.world_id = desc.world_id;
    result.world.world_tick = desc.world_tick;

    if (!is_valid_id(desc.world_id)) {
        add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::missing_world_id, desc, desc.world_id, {}, {},
                       "runtime sandbox world id must be non-empty and path-safe", 0U);
    }
    if (has_backend_reference(desc.world_id)) {
        add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::unsupported_backend_reference, desc,
                       desc.world_id, {}, {},
                       "runtime sandbox world ids must not encode renderer/platform/backend references", 0U);
    }
    if (desc_row_count(desc) > desc.row_budget) {
        add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::row_budget_exceeded, desc, desc.world_id, {},
                       {}, "runtime sandbox world build exceeds its row budget", 0U);
    }
    if (desc.chunk_rows.empty()) {
        result.status = RuntimeSandboxWorldRuntimeStatus::no_chunks;
        add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk, desc, {}, {}, {},
                       "runtime sandbox worlds require at least one chunk row", 0U);
    }

    std::vector<std::string> chunk_ids;
    std::vector<ChunkBounds> chunk_bounds;
    chunk_ids.reserve(desc.chunk_rows.size());
    chunk_bounds.reserve(desc.chunk_rows.size());
    for (const auto& row : desc.chunk_rows) {
        auto valid = true;
        if (!is_valid_id(row.chunk_id) || !is_valid_id(row.region_id) || has_backend_reference(row.chunk_id) ||
            has_backend_reference(row.region_id)) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk, desc, row.chunk_id,
                           row.chunk_id, {}, "sandbox runtime chunks require path-safe backend-neutral ids",
                           row.source_index);
            valid = false;
        }
        if (std::ranges::find(chunk_ids, row.chunk_id) != chunk_ids.end()) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::duplicate_chunk, desc, row.chunk_id,
                           row.chunk_id, {}, "sandbox runtime chunk ids must be unique", row.source_index);
            valid = false;
        } else {
            chunk_ids.push_back(row.chunk_id);
        }
        if (!is_valid_chunk_extent(row)) {
            add_diagnostic(
                result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk, desc, row.chunk_id, row.chunk_id, {},
                "sandbox runtime chunk extents must be non-empty and fit int32 coordinates", row.source_index);
            valid = false;
        }
        if (valid) {
            const auto candidate = ChunkBounds{
                .chunk_id = row.chunk_id,
                .origin = RuntimeSandboxCellCoord{.x = row.origin_x, .y = row.origin_y, .z = row.origin_z},
                .end_exclusive = end_coord(row),
            };
            const auto overlap = std::ranges::find_if(
                chunk_bounds, [&candidate](const auto& existing) { return chunks_overlap(existing, candidate); });
            if (overlap != chunk_bounds.end()) {
                add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk, desc, row.chunk_id,
                               row.chunk_id, candidate.origin,
                               "sandbox runtime chunk extents must not overlap existing chunks", row.source_index);
            } else {
                chunk_bounds.push_back(candidate);
            }
        }
    }

    std::vector<std::string> cell_keys;
    cell_keys.reserve(desc.existing_cell_rows.size());
    for (const auto& row : desc.existing_cell_rows) {
        auto valid = true;
        const auto* chunk = find_chunk_by_id(chunk_bounds, row.chunk_id);
        if (chunk == nullptr) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::unknown_cell_chunk, desc, row.block_id,
                           row.chunk_id, row.coord, "sandbox runtime cells must reference a valid chunk",
                           row.source_index);
            valid = false;
        } else if (!contains_coord(*chunk, row.coord)) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_cell, desc, row.block_id,
                           row.chunk_id, row.coord, "sandbox runtime cell coordinates must be inside their chunk",
                           row.source_index);
            valid = false;
        }
        if (!is_valid_id(row.block_id) || has_backend_reference(row.block_id)) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_cell, desc, row.block_id,
                           row.chunk_id, row.coord, "sandbox runtime cells require backend-neutral block ids",
                           row.source_index);
            valid = false;
        }
        const auto key = make_cell_key(row.chunk_id, row.coord);
        if (std::ranges::find(cell_keys, key) != cell_keys.end()) {
            add_diagnostic(result, RuntimeSandboxWorldRuntimeDiagnosticCode::duplicate_cell, desc, row.block_id,
                           row.chunk_id, row.coord, "sandbox runtime cells must be unique by chunk and coordinate",
                           row.source_index);
            valid = false;
        } else {
            cell_keys.push_back(key);
        }
        if (valid) {
            result.world.cells.push_back(row);
        }
    }

    if (!result.diagnostics.empty()) {
        if (result.status != RuntimeSandboxWorldRuntimeStatus::no_chunks) {
            result.status = RuntimeSandboxWorldRuntimeStatus::invalid_request;
        }
        result.world.chunks.clear();
        result.world.cells.clear();
        result.world.chunk_count = 0U;
        result.world.cell_count = 0U;
        result.world.snapshot_hash = 0U;
        sort_diagnostics(result);
        return result;
    }

    result.world.chunks = desc.chunk_rows;
    std::ranges::sort(result.world.chunks, chunk_less);
    std::ranges::sort(result.world.cells, cell_less);
    result.world.chunk_count = result.world.chunks.size();
    result.world.cell_count = result.world.cells.size();
    result.world.snapshot_hash = compute_snapshot_hash(result.world);
    result.status = RuntimeSandboxWorldRuntimeStatus::ready;
    return result;
}

RuntimeSandboxCellSample sample_runtime_sandbox_cell(const RuntimeSandboxWorld& world, RuntimeSandboxCellCoord coord) {
    std::vector<ChunkBounds> chunk_bounds;
    chunk_bounds.reserve(world.chunks.size());
    for (const auto& row : world.chunks) {
        if (is_valid_chunk_extent(row)) {
            chunk_bounds.push_back(ChunkBounds{
                .chunk_id = row.chunk_id,
                .origin = RuntimeSandboxCellCoord{.x = row.origin_x, .y = row.origin_y, .z = row.origin_z},
                .end_exclusive = end_coord(row),
            });
        }
    }

    const auto chunk_iter =
        std::ranges::find_if(chunk_bounds, [coord](const auto& chunk) { return contains_coord(chunk, coord); });
    if (chunk_iter == chunk_bounds.end()) {
        return RuntimeSandboxCellSample{.status = RuntimeSandboxCellSampleStatus::missing_chunk, .coord = coord};
    }

    const auto cell_iter =
        std::ranges::find_if(world.cells, [chunk_id = std::string_view{chunk_iter->chunk_id}, coord](const auto& cell) {
            return cell.chunk_id == chunk_id && same_coord(cell.coord, coord);
        });
    if (cell_iter == world.cells.end()) {
        return RuntimeSandboxCellSample{
            .status = RuntimeSandboxCellSampleStatus::empty,
            .chunk_id = chunk_iter->chunk_id,
            .coord = coord,
        };
    }

    return RuntimeSandboxCellSample{
        .status = RuntimeSandboxCellSampleStatus::occupied,
        .chunk_id = cell_iter->chunk_id,
        .coord = cell_iter->coord,
        .block_id = cell_iter->block_id,
        .destructible = cell_iter->destructible,
        .protected_cell = cell_iter->protected_cell,
    };
}

RuntimeSandboxWorldSnapshot snapshot_runtime_sandbox_world(const RuntimeSandboxWorld& world) {
    return RuntimeSandboxWorldSnapshot{
        .world_id = world.world_id,
        .world_tick = world.world_tick,
        .chunk_count = world.chunks.size(),
        .cell_count = world.cells.size(),
        .hash = compute_snapshot_hash(world),
    };
}

RuntimeSandboxWorldMutationExecutionResult
apply_runtime_sandbox_world_mutations(const RuntimeSandboxWorld& world, const RuntimeSandboxWorldMutationPlan& plan) {
    RuntimeSandboxWorldMutationExecutionResult result;
    result.world = world;

    if (!plan.succeeded()) {
        result.status = RuntimeSandboxWorldMutationExecutionStatus::rejected_plan;
        result.rejected_mutation_count = plan.mutation_rows.size();
        return result;
    }

    result.status = RuntimeSandboxWorldMutationExecutionStatus::ready;
    for (const auto& row : plan.mutation_rows) {
        if (row.status != RuntimeSandboxMutationStatus::accepted) {
            ++result.rejected_mutation_count;
            continue;
        }

        auto previous_block_id = std::string{};
        auto new_block_id = std::string{};
        auto applied = false;
        if (!world_contains_cell(result.world.chunks, row.chunk_id, row.coord)) {
            ++result.rejected_mutation_count;
            continue;
        }
        auto cell_iter = find_world_cell(result.world.cells, row.chunk_id, row.coord);
        if (row.kind == RuntimeSandboxMutationKind::placement) {
            if (cell_iter != result.world.cells.end()) {
                ++result.rejected_mutation_count;
                continue;
            }
            result.world.cells.push_back(RuntimeSandboxExistingCellRow{
                .chunk_id = row.chunk_id,
                .coord = row.coord,
                .block_id = row.block_id,
                .destructible = true,
                .protected_cell = false,
                .source_index = row.source_index,
            });
            new_block_id = row.block_id;
            applied = true;
        } else {
            if (cell_iter == result.world.cells.end()) {
                ++result.rejected_mutation_count;
                continue;
            }
            previous_block_id = cell_iter->block_id;
            result.world.cells.erase(cell_iter);
            applied = true;
        }

        if (applied) {
            ++result.applied_mutation_count;
            const auto region_hash = compute_dirty_region_hash(row, previous_block_id, new_block_id);
            result.dirty_regions.push_back(RuntimeSandboxWorldDirtyRegion{
                .chunk_id = row.chunk_id,
                .intent_id = row.intent_id,
                .min_coord = row.coord,
                .max_coord_exclusive = next_cell_coord(row.coord),
                .layer_mask = layer_mask_for(row.coord),
                .previous_block_id = std::move(previous_block_id),
                .new_block_id = std::move(new_block_id),
                .replay_hash = region_hash,
                .chunk_dirty = true,
            });
        }
    }

    std::ranges::sort(result.world.cells, cell_less);
    result.world.chunk_count = result.world.chunks.size();
    result.world.cell_count = result.world.cells.size();
    result.world.snapshot_hash = compute_snapshot_hash(result.world);

    auto replay_hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(replay_hash, plan.replay_hash);
    mix_hash(replay_hash, result.world.snapshot_hash);
    mix_hash(replay_hash, result.applied_mutation_count);
    mix_hash(replay_hash, result.rejected_mutation_count);
    for (const auto& region : result.dirty_regions) {
        mix_hash(replay_hash, region);
    }
    result.replay_hash = replay_hash == 0U ? 1U : replay_hash;
    return result;
}

RuntimeSandboxTileSimulationPlan plan_runtime_sandbox_tile_simulation(const RuntimeSandboxWorld& world,
                                                                      const RuntimeSandboxTileSimulationDesc& desc) {
    RuntimeSandboxTileSimulationPlan plan;
    plan.material_count = desc.material_rows.size();

    if (desc.material_rows.size() + world.cells.size() + desc.dirty_regions.size() > desc.row_budget) {
        add_diagnostic(plan, RuntimeSandboxTileSimulationDiagnosticCode::row_budget_exceeded, {}, {}, {},
                       "runtime sandbox tile simulation input exceeds its row budget", 0U);
    }

    std::vector<std::string> tile_ids;
    tile_ids.reserve(desc.material_rows.size());
    for (const auto& material : desc.material_rows) {
        if (!is_valid_material_row(material, desc)) {
            add_diagnostic(
                plan, RuntimeSandboxTileSimulationDiagnosticCode::invalid_material, material.tile_id, {}, {},
                "tile material rows require backend-neutral ids, render layers in [0, 63], and bounded light "
                "radii",
                material.source_index);
        }

        if (std::ranges::find(tile_ids, material.tile_id) != tile_ids.end()) {
            add_diagnostic(plan, RuntimeSandboxTileSimulationDiagnosticCode::duplicate_material, material.tile_id, {},
                           {}, "tile material ids must be unique", material.source_index);
        } else {
            tile_ids.push_back(material.tile_id);
        }
    }

    for (const auto& cell : world.cells) {
        if (find_material(desc.material_rows, cell.block_id) == nullptr) {
            add_diagnostic(plan, RuntimeSandboxTileSimulationDiagnosticCode::unknown_cell_material, cell.block_id,
                           cell.chunk_id, cell.coord, "runtime sandbox cells must resolve to a tile material row",
                           cell.source_index);
        }
    }

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        return plan;
    }

    auto materials = desc.material_rows;
    std::ranges::sort(materials, material_less);
    plan.status = RuntimeSandboxTileSimulationStatus::ready;

    auto liquid_updates = std::uint32_t{0U};
    for (const auto& cell : world.cells) {
        const auto* material = find_material(materials, cell.block_id);
        if (material == nullptr) {
            continue;
        }

        if (material->solid) {
            auto row = RuntimeSandboxTileCollisionSpanRow{
                .chunk_id = cell.chunk_id,
                .min_coord = cell.coord,
                .max_coord_exclusive = next_cell_coord(cell.coord),
                .tile_id = cell.block_id,
                .render_layer = material->render_layer,
            };
            row.replay_hash = compute_collision_span_hash(row);
            plan.solid_collision_spans.push_back(std::move(row));
        }
        if (material->platform) {
            auto row = RuntimeSandboxTileCollisionSpanRow{
                .chunk_id = cell.chunk_id,
                .min_coord = cell.coord,
                .max_coord_exclusive = next_cell_coord(cell.coord),
                .tile_id = cell.block_id,
                .render_layer = material->render_layer,
            };
            row.replay_hash = compute_collision_span_hash(row);
            plan.platform_collision_spans.push_back(std::move(row));
        }
        if (material->liquid) {
            auto row = RuntimeSandboxTileCellRow{
                .chunk_id = cell.chunk_id,
                .coord = cell.coord,
                .tile_id = cell.block_id,
                .render_layer = material->render_layer,
            };
            row.replay_hash = compute_tile_cell_hash(row);
            plan.liquid_cells.push_back(std::move(row));
            if (liquid_updates < desc.liquid_update_budget) {
                append_liquid_flow_row(plan, world, cell, materials);
                ++liquid_updates;
            }
        }
        if (material->trigger) {
            auto row = RuntimeSandboxTileCellRow{
                .chunk_id = cell.chunk_id,
                .coord = cell.coord,
                .tile_id = cell.block_id,
                .render_layer = material->render_layer,
            };
            row.replay_hash = compute_tile_cell_hash(row);
            plan.trigger_cells.push_back(std::move(row));
        }
        if (material->update_cadence_ticks > 0U) {
            auto row = RuntimeSandboxScheduledTileUpdateRow{
                .chunk_id = cell.chunk_id,
                .coord = cell.coord,
                .tile_id = cell.block_id,
                .cadence_ticks = material->update_cadence_ticks,
                .scheduled_tick = world.world_tick + material->update_cadence_ticks,
            };
            row.replay_hash = compute_scheduled_row_hash(row);
            plan.scheduled_update_rows.push_back(std::move(row));
        }
        if (material->light_emitter) {
            append_light_rows(plan, world, cell, *material, desc, materials);
        }
    }

    std::ranges::sort(plan.solid_collision_spans, span_row_less);
    std::ranges::sort(plan.platform_collision_spans, span_row_less);
    std::ranges::sort(plan.liquid_cells, cell_row_less);
    std::ranges::sort(plan.trigger_cells, cell_row_less);
    std::ranges::sort(plan.scheduled_update_rows, scheduled_row_less);
    std::ranges::sort(plan.liquid_flow_rows, liquid_row_less);

    if (tile_simulation_output_row_count(plan) > desc.row_budget) {
        add_diagnostic(plan, RuntimeSandboxTileSimulationDiagnosticCode::row_budget_exceeded, {}, {}, {},
                       "runtime sandbox tile simulation output exceeds its row budget", 0U);
        plan.status = RuntimeSandboxTileSimulationStatus::invalid_request;
        clear_tile_simulation_outputs(plan);
        sort_diagnostics(plan);
        return plan;
    }

    auto replay_hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(replay_hash, world.snapshot_hash);
    mix_hash(replay_hash, plan.material_count);
    for (const auto& material : materials) {
        mix_hash(replay_hash, material.tile_id);
        mix_hash(replay_hash, material.solid ? 1U : 0U);
        mix_hash(replay_hash, material.platform ? 1U : 0U);
        mix_hash(replay_hash, material.liquid ? 1U : 0U);
        mix_hash(replay_hash, material.light_emitter ? 1U : 0U);
        mix_hash(replay_hash, material.replaceable ? 1U : 0U);
        mix_hash(replay_hash, material.trigger ? 1U : 0U);
        mix_hash(replay_hash, material.update_cadence_ticks);
        mix_hash(replay_hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(material.render_layer)));
        mix_hash(replay_hash, material.light_radius);
        mix_hash(replay_hash, material.source_index);
    }
    for (const auto& region : desc.dirty_regions) {
        mix_hash(replay_hash, region);
    }
    for (const auto& row : plan.solid_collision_spans) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.platform_collision_spans) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.liquid_cells) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.trigger_cells) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.scheduled_update_rows) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.light_rows) {
        mix_hash(replay_hash, row.replay_hash);
    }
    for (const auto& row : plan.liquid_flow_rows) {
        mix_hash(replay_hash, row.replay_hash);
    }
    plan.replay_hash = replay_hash == 0U ? 1U : replay_hash;
    return plan;
}

} // namespace mirakana::runtime
