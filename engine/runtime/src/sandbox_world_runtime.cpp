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
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "metal", "sdl", "sdl3", "imgui", "gpu",
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

} // namespace

bool RuntimeSandboxWorldBuildResult::succeeded() const noexcept {
    return status == RuntimeSandboxWorldRuntimeStatus::ready && diagnostics.empty();
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

} // namespace mirakana::runtime
