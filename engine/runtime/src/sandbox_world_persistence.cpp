// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sandbox_world_persistence.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool is_safe_id(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    return std::ranges::all_of(value, [](char character) {
        return !is_ascii_control(character) && character != '/' && character != '\\' && character != ':' &&
               character != ';';
    });
}

[[nodiscard]] bool is_safe_project_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\') {
        return false;
    }
    if (std::ranges::any_of(path, [](char character) {
            return is_ascii_control(character) || character == '\\' || character == ':' || character == ';';
        })) {
        return false;
    }

    std::size_t begin = 0U;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
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

[[nodiscard]] bool same_coord(RuntimeSandboxCellCoord lhs, RuntimeSandboxCellCoord rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] RuntimeSandboxCellCoord chunk_origin(const RuntimeSandboxChunkRow& row) noexcept {
    return RuntimeSandboxCellCoord{.x = row.origin_x, .y = row.origin_y, .z = row.origin_z};
}

[[nodiscard]] RuntimeSandboxCellCoord chunk_size(const RuntimeSandboxChunkRow& row) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(row.size_x),
        .y = static_cast<std::int32_t>(row.size_y),
        .z = static_cast<std::int32_t>(row.size_z),
    };
}

[[nodiscard]] RuntimeSandboxCellCoord chunk_end_exclusive(const RuntimeSandboxChunkRow& row) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_x) + static_cast<std::int64_t>(row.size_x)),
        .y = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_y) + static_cast<std::int64_t>(row.size_y)),
        .z = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_z) + static_cast<std::int64_t>(row.size_z)),
    };
}

[[nodiscard]] bool contains_coord(const RuntimeSandboxChunkRow& chunk, RuntimeSandboxCellCoord coord) noexcept {
    const auto end = chunk_end_exclusive(chunk);
    return coord.x >= chunk.origin_x && coord.y >= chunk.origin_y && coord.z >= chunk.origin_z && coord.x < end.x &&
           coord.y < end.y && coord.z < end.z;
}

[[nodiscard]] std::uint64_t stable_hash(std::string_view text) noexcept {
    auto hash = fnv_offset;
    for (const char character : text) {
        hash ^= static_cast<unsigned char>(character);
        hash *= fnv_prime;
    }
    return hash == 0U ? 1U : hash;
}

void mix_hash(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void mix_hash(std::uint64_t& hash, std::string_view value) noexcept {
    mix_hash(hash, stable_hash(value));
}

void mix_hash(std::uint64_t& hash, RuntimeSandboxCellCoord coord) noexcept {
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.x)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.y)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.z)));
}

[[nodiscard]] std::uint64_t cell_hash(const RuntimeSandboxWorldChangedCellRow& row) noexcept {
    auto hash = fnv_offset;
    mix_hash(hash, row.chunk_id);
    mix_hash(hash, row.coord);
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.layer_id)));
    mix_hash(hash, row.tile_id);
    mix_hash(hash, row.source_index);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t layer_hash(std::string_view chunk_id, std::int32_t layer_id,
                                       const std::vector<RuntimeSandboxWorldChangedCellRow>& cells) noexcept {
    auto hash = fnv_offset;
    mix_hash(hash, chunk_id);
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(layer_id)));
    for (const auto& cell : cells) {
        if (cell.chunk_id == chunk_id && cell.layer_id == layer_id) {
            mix_hash(hash, cell.content_hash);
        }
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::uint64_t chunk_hash(const RuntimeSandboxChunkRow& chunk,
                                       const std::vector<RuntimeSandboxWorldChangedCellRow>& cells) noexcept {
    auto hash = fnv_offset;
    mix_hash(hash, chunk.chunk_id);
    mix_hash(hash, chunk.region_id);
    mix_hash(hash, chunk_origin(chunk));
    mix_hash(hash, chunk_size(chunk));
    mix_hash(hash, chunk.resident ? 1U : 0U);
    mix_hash(hash, chunk.persistent ? 1U : 0U);
    mix_hash(hash, chunk.source_index);
    for (const auto& cell : cells) {
        if (cell.chunk_id == chunk.chunk_id) {
            mix_hash(hash, cell.content_hash);
        }
    }
    return hash == 0U ? 1U : hash;
}

void add_diagnostic(std::vector<RuntimeSandboxWorldPersistenceDiagnostic>& diagnostics,
                    RuntimeSandboxWorldPersistenceDiagnosticCode code, std::string field, std::string expected,
                    std::string actual, std::string message, std::uint32_t source_index = 0U) {
    diagnostics.push_back(RuntimeSandboxWorldPersistenceDiagnostic{
        .code = code,
        .field = std::move(field),
        .expected = std::move(expected),
        .actual = std::move(actual),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(std::vector<RuntimeSandboxWorldPersistenceDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.field != rhs.field) {
            return lhs.field < rhs.field;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        if (lhs.expected != rhs.expected) {
            return lhs.expected < rhs.expected;
        }
        return lhs.actual < rhs.actual;
    });
}

void clear_document_outputs(RuntimeSandboxWorldPersistenceDocumentPlan& plan) {
    plan.chunk_rows.clear();
    plan.layer_rows.clear();
    plan.changed_cell_rows.clear();
    plan.canonical_text.clear();
    plan.content_hash = 0U;
    plan.byte_size_estimate = 0U;
}

void append_line(std::ostringstream& output, std::string_view key, std::string_view value) {
    output << key << '=' << value << '\n';
}

void append_line(std::ostringstream& output, std::string_view key, std::uint64_t value) {
    output << key << '=' << value << '\n';
}

void append_coord_line(std::ostringstream& output, std::string_view key, RuntimeSandboxCellCoord coord) {
    output << key << '=' << coord.x << ',' << coord.y << ',' << coord.z << '\n';
}

[[nodiscard]] std::string build_canonical_text(const RuntimeSandboxWorldPersistenceDocumentPlan& plan,
                                               std::string_view snapshot_kind) {
    std::ostringstream output;
    append_line(output, "format", "GameEngine.RuntimeSandboxWorldSnapshot.v1");
    append_line(output, "snapshot.kind", snapshot_kind);
    append_line(output, "world.id", plan.world_id);
    append_line(output, "schema.version", plan.schema_version);
    append_line(output, "source.package.id", plan.source_package_id);
    append_line(output, "seed", plan.seed);
    append_line(output, "world.tick", plan.world_tick);
    append_line(output, "chunk.count", plan.chunk_rows.size());
    append_line(output, "layer.count", plan.layer_rows.size());
    append_line(output, "cell.count", plan.changed_cell_rows.size());

    for (std::size_t index = 0U; index < plan.chunk_rows.size(); ++index) {
        const auto& row = plan.chunk_rows[index];
        const auto prefix = std::string{"chunk."} + std::to_string(index);
        append_line(output, prefix + ".id", row.chunk_id);
        append_coord_line(output, prefix + ".origin", row.origin);
        append_coord_line(output, prefix + ".size", row.size);
        append_line(output, prefix + ".layer_mask", row.layer_mask);
        append_line(output, prefix + ".changed_cell_count", row.changed_cell_count);
        append_line(output, prefix + ".source_package_id", row.source_package_id);
        append_line(output, prefix + ".content_hash", row.content_hash);
    }
    for (std::size_t index = 0U; index < plan.layer_rows.size(); ++index) {
        const auto& row = plan.layer_rows[index];
        const auto prefix = std::string{"layer."} + std::to_string(index);
        append_line(output, prefix + ".chunk", row.chunk_id);
        append_line(output, prefix + ".id", static_cast<std::uint64_t>(static_cast<std::int64_t>(row.layer_id)));
        append_line(output, prefix + ".cell_count", row.cell_count);
        append_line(output, prefix + ".content_hash", row.content_hash);
    }
    for (std::size_t index = 0U; index < plan.changed_cell_rows.size(); ++index) {
        const auto& row = plan.changed_cell_rows[index];
        const auto prefix = std::string{"cell."} + std::to_string(index);
        append_line(output, prefix + ".chunk", row.chunk_id);
        append_coord_line(output, prefix + ".coord", row.coord);
        append_line(output, prefix + ".layer", static_cast<std::uint64_t>(static_cast<std::int64_t>(row.layer_id)));
        append_line(output, prefix + ".tile", row.tile_id);
        append_line(output, prefix + ".content_hash", row.content_hash);
    }
    return output.str();
}

void finalize_document_plan(RuntimeSandboxWorldPersistenceDocumentPlan& plan, std::size_t row_budget,
                            std::size_t byte_budget, std::string_view snapshot_kind) {
    const auto row_count = plan.chunk_rows.size() + plan.layer_rows.size() + plan.changed_cell_rows.size();
    if (row_count > row_budget) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::row_budget_exceeded,
                       "row_budget", std::to_string(row_budget), std::to_string(row_count),
                       "runtime sandbox world persistence document exceeds its row budget");
    }

    plan.canonical_text = build_canonical_text(plan, snapshot_kind);
    plan.byte_size_estimate = plan.canonical_text.size();
    if (plan.byte_size_estimate > byte_budget) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::size_budget_exceeded,
                       "byte_budget", std::to_string(byte_budget), std::to_string(plan.byte_size_estimate),
                       "runtime sandbox world persistence document exceeds its byte budget");
    }

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan.diagnostics);
        plan.status = RuntimeSandboxWorldPersistenceStatus::invalid_request;
        clear_document_outputs(plan);
        return;
    }
    plan.content_hash = stable_hash(plan.canonical_text);
    plan.status = RuntimeSandboxWorldPersistenceStatus::ready;
}

[[nodiscard]] const RuntimeSandboxChunkRow* find_chunk(const RuntimeSandboxWorld& world, std::string_view chunk_id) {
    const auto iter =
        std::ranges::find_if(world.chunks, [chunk_id](const auto& chunk) { return chunk.chunk_id == chunk_id; });
    if (iter == world.chunks.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] bool chunk_id_included(const std::vector<std::string>& chunk_ids, std::string_view chunk_id) {
    return std::ranges::find(chunk_ids, chunk_id) != chunk_ids.end();
}

[[nodiscard]] std::vector<std::string> dirty_chunk_ids(const std::vector<RuntimeSandboxWorldDirtyRegion>& regions) {
    std::vector<std::string> result;
    for (const auto& region : regions) {
        if (region.chunk_id.empty() || !region.chunk_dirty) {
            continue;
        }
        if (!chunk_id_included(result, region.chunk_id)) {
            result.push_back(region.chunk_id);
        }
    }
    std::ranges::sort(result);
    return result;
}

[[nodiscard]] bool previous_chunk_matches(const std::vector<RuntimeSandboxWorldChunkSnapshotRow>& previous_rows,
                                          const RuntimeSandboxWorldChunkSnapshotRow& row) {
    const auto iter = std::ranges::find(previous_rows, row.chunk_id, &RuntimeSandboxWorldChunkSnapshotRow::chunk_id);
    return iter != previous_rows.end() && iter->content_hash == row.content_hash;
}

[[nodiscard]] std::uint64_t compute_migration_replay_hash(const RuntimeSandboxWorldMigrationReviewPlan& plan) noexcept {
    auto hash = fnv_offset;
    mix_hash(hash, plan.observed_schema_version);
    mix_hash(hash, plan.target_schema_version);
    for (const auto& row : plan.migration_rows) {
        mix_hash(hash, row.from_schema_version);
        mix_hash(hash, row.to_schema_version);
        mix_hash(hash, row.migration_id);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.repair_rows) {
        mix_hash(hash, row.key);
        mix_hash(hash, row.repairable ? 1U : 0U);
        mix_hash(hash, row.message);
        mix_hash(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool migration_chain_is_better(const std::vector<RuntimeSandboxWorldMigrationStepRow>& lhs,
                                             const std::vector<RuntimeSandboxWorldMigrationStepRow>& rhs) {
    if (lhs.size() != rhs.size()) {
        return lhs.size() < rhs.size();
    }
    for (std::size_t index = 0U; index < lhs.size(); ++index) {
        if (lhs[index].from_schema_version != rhs[index].from_schema_version) {
            return lhs[index].from_schema_version < rhs[index].from_schema_version;
        }
        if (lhs[index].to_schema_version != rhs[index].to_schema_version) {
            return lhs[index].to_schema_version < rhs[index].to_schema_version;
        }
        if (lhs[index].migration_id != rhs[index].migration_id) {
            return lhs[index].migration_id < rhs[index].migration_id;
        }
    }
    return false;
}

[[nodiscard]] std::uint64_t compute_atomic_replay_hash(const RuntimeSandboxWorldAtomicSavePlan& plan) noexcept {
    auto hash = fnv_offset;
    for (const auto& row : plan.operation_rows) {
        mix_hash(hash, row.step_index);
        mix_hash(hash, static_cast<std::uint64_t>(row.kind));
        mix_hash(hash, row.source_path);
        mix_hash(hash, row.target_path);
        mix_hash(hash, row.backup_path);
        mix_hash(hash, row.payload_content_hash);
        mix_hash(hash, row.payload_byte_size);
        mix_hash(hash, row.flush_required ? 1U : 0U);
        mix_hash(hash, row.rollback_only_on_failure ? 1U : 0U);
    }
    return hash == 0U ? 1U : hash;
}

} // namespace

bool RuntimeSandboxWorldPersistenceDocumentPlan::succeeded() const noexcept {
    return status == RuntimeSandboxWorldPersistenceStatus::ready && diagnostics.empty();
}

bool RuntimeSandboxWorldMigrationReviewPlan::succeeded() const noexcept {
    return status == RuntimeSandboxWorldPersistenceStatus::ready && diagnostics.empty();
}

bool RuntimeSandboxWorldAtomicSavePlan::succeeded() const noexcept {
    return status == RuntimeSandboxWorldPersistenceStatus::ready && diagnostics.empty();
}

RuntimeSandboxWorldPersistenceDocumentPlan
plan_runtime_sandbox_world_persistence_document(const RuntimeSandboxWorldPersistenceDocumentDesc& desc) {
    RuntimeSandboxWorldPersistenceDocumentPlan plan;
    plan.world_id = desc.world.world_id;
    plan.schema_version = desc.schema_version;
    plan.source_package_id = desc.source_package_id;
    plan.seed = desc.seed;
    plan.world_tick = desc.world.world_tick;

    if (!is_safe_id(desc.world.world_id)) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::missing_world_id, "world.id",
                       "path-safe id", desc.world.world_id,
                       "runtime sandbox world persistence requires a path-safe world id");
    }
    if (desc.schema_version == 0U) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_schema_version,
                       "schema.version", ">=1", std::to_string(desc.schema_version),
                       "runtime sandbox world persistence schema version must be non-zero");
    }
    if (!is_safe_project_path(desc.source_package_id)) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_path,
                       "source.package.id", "project-relative path", desc.source_package_id,
                       "runtime sandbox world persistence source package id must be project-relative");
    }

    std::vector<RuntimeSandboxWorldChangedCellRow> cells;
    cells.reserve(desc.world.cells.size());
    for (const auto& cell : desc.world.cells) {
        const auto* chunk = find_chunk(desc.world, cell.chunk_id);
        if (chunk == nullptr) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_cell, "cell.chunk",
                           "known chunk", cell.chunk_id,
                           "runtime sandbox world persistence cell references an unknown chunk", cell.source_index);
            continue;
        }
        if (!contains_coord(*chunk, cell.coord) || !is_safe_id(cell.block_id)) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_cell, "cell",
                           "in-bounds tile cell", cell.block_id,
                           "runtime sandbox world persistence cell must be in-bounds with a path-safe tile id",
                           cell.source_index);
            continue;
        }
        const auto layer_id = cell.coord.z - chunk->origin_z;
        RuntimeSandboxWorldChangedCellRow row{
            .chunk_id = cell.chunk_id,
            .coord = cell.coord,
            .layer_id = layer_id,
            .tile_id = cell.block_id,
            .content_hash = 0U,
            .source_index = cell.source_index,
        };
        row.content_hash = cell_hash(row);
        cells.push_back(std::move(row));
    }
    std::ranges::sort(cells, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        if (!same_coord(lhs.coord, rhs.coord)) {
            return coord_less(lhs.coord, rhs.coord);
        }
        return lhs.tile_id < rhs.tile_id;
    });

    for (const auto& chunk : desc.world.chunks) {
        if (!is_safe_id(chunk.chunk_id) || chunk.size_x == 0U || chunk.size_y == 0U || chunk.size_z == 0U) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_chunk, "chunk",
                           "valid path-safe chunk", chunk.chunk_id,
                           "runtime sandbox world persistence chunk must be path-safe and non-empty",
                           chunk.source_index);
            continue;
        }

        auto layer_mask = std::uint64_t{0U};
        auto changed_cell_count = std::size_t{0U};
        std::vector<std::int32_t> layer_ids;
        for (const auto& cell_row : cells) {
            if (cell_row.chunk_id != chunk.chunk_id) {
                continue;
            }
            ++changed_cell_count;
            if (cell_row.layer_id >= 0 && cell_row.layer_id < 64) {
                layer_mask |= 1ULL << static_cast<std::uint32_t>(cell_row.layer_id);
            }
            if (std::ranges::find(layer_ids, cell_row.layer_id) == layer_ids.end()) {
                layer_ids.push_back(cell_row.layer_id);
            }
        }
        std::ranges::sort(layer_ids);
        for (const auto layer_id : layer_ids) {
            auto cell_count = std::size_t{0U};
            for (const auto& cell_row : cells) {
                if (cell_row.chunk_id == chunk.chunk_id && cell_row.layer_id == layer_id) {
                    ++cell_count;
                }
            }
            plan.layer_rows.push_back(RuntimeSandboxWorldLayerSnapshotRow{
                .chunk_id = chunk.chunk_id,
                .layer_id = layer_id,
                .cell_count = cell_count,
                .content_hash = layer_hash(chunk.chunk_id, layer_id, cells),
            });
        }
        plan.chunk_rows.push_back(RuntimeSandboxWorldChunkSnapshotRow{
            .chunk_id = chunk.chunk_id,
            .origin = chunk_origin(chunk),
            .size = chunk_size(chunk),
            .layer_mask = layer_mask,
            .changed_cell_count = changed_cell_count,
            .source_package_id = desc.source_package_id,
            .content_hash = chunk_hash(chunk, cells),
            .source_index = chunk.source_index,
        });
    }
    std::ranges::sort(plan.chunk_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        return coord_less(lhs.origin, rhs.origin);
    });
    std::ranges::sort(plan.layer_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        return lhs.layer_id < rhs.layer_id;
    });
    plan.changed_cell_rows = std::move(cells);

    finalize_document_plan(plan, desc.row_budget, desc.byte_budget, "full");
    return plan;
}

RuntimeSandboxWorldPersistenceDocumentPlan
plan_runtime_sandbox_world_snapshot_diff(const RuntimeSandboxWorldSnapshotDiffDesc& desc) {
    auto plan = plan_runtime_sandbox_world_persistence_document(desc.document);
    if (!plan.succeeded()) {
        return plan;
    }

    auto include_chunks = dirty_chunk_ids(desc.document.dirty_regions);
    for (const auto& row : plan.chunk_rows) {
        if (!previous_chunk_matches(desc.previous_chunk_rows, row) &&
            !chunk_id_included(include_chunks, row.chunk_id)) {
            include_chunks.push_back(row.chunk_id);
        }
    }
    std::ranges::sort(include_chunks);

    std::erase_if(plan.chunk_rows,
                  [&include_chunks](const auto& row) { return !chunk_id_included(include_chunks, row.chunk_id); });
    std::erase_if(plan.layer_rows,
                  [&include_chunks](const auto& row) { return !chunk_id_included(include_chunks, row.chunk_id); });
    std::erase_if(plan.changed_cell_rows,
                  [&include_chunks](const auto& row) { return !chunk_id_included(include_chunks, row.chunk_id); });

    plan.canonical_text.clear();
    plan.content_hash = 0U;
    plan.byte_size_estimate = 0U;
    finalize_document_plan(plan, desc.document.row_budget, desc.document.byte_budget, "diff");
    return plan;
}

RuntimeSandboxWorldMigrationReviewPlan
review_runtime_sandbox_world_migration(const RuntimeSandboxWorldMigrationReviewDesc& desc) {
    RuntimeSandboxWorldMigrationReviewPlan plan;
    plan.observed_schema_version = desc.observed_schema_version;
    plan.target_schema_version = desc.target_schema_version;

    const auto request_row_count = desc.migration_steps.size() + desc.corruption_rows.size();
    if (request_row_count > desc.row_budget) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::row_budget_exceeded,
                       "row_budget", std::to_string(desc.row_budget), std::to_string(request_row_count),
                       "runtime sandbox world migration review exceeds its row budget");
    }
    if (desc.minimum_supported_schema_version == 0U ||
        desc.target_schema_version < desc.minimum_supported_schema_version) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_schema_version,
                       "minimum_supported_schema_version", "valid schema policy",
                       std::to_string(desc.minimum_supported_schema_version),
                       "runtime sandbox world migration review schema policy is invalid");
    }
    if (desc.observed_schema_version > desc.target_schema_version) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::unsupported_future_schema,
                       "observed_schema_version", std::to_string(desc.target_schema_version),
                       std::to_string(desc.observed_schema_version),
                       "runtime sandbox world migration review cannot load a future schema");
    }
    if (desc.observed_schema_version < desc.minimum_supported_schema_version || desc.observed_schema_version == 0U) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_schema_version,
                       "observed_schema_version", std::to_string(desc.minimum_supported_schema_version),
                       std::to_string(desc.observed_schema_version),
                       "runtime sandbox world migration review observed schema is unsupported");
    }

    for (const auto& corruption : desc.corruption_rows) {
        if (!corruption.repairable) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::unrecoverable_corruption,
                           corruption.key, "repairable corruption", corruption.message,
                           "runtime sandbox world corruption row is unrecoverable", corruption.source_index);
            continue;
        }
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::repairable_corruption,
                       corruption.key, "repairable corruption", corruption.message,
                       "runtime sandbox world corruption row can be repaired", corruption.source_index);
        plan.repair_rows.push_back(corruption);
    }

    std::vector<RuntimeSandboxWorldMigrationStepRow> valid_steps;
    for (const auto& step : desc.migration_steps) {
        if (step.from_schema_version >= step.to_schema_version || step.migration_id.empty() ||
            !is_safe_id(step.migration_id)) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_migration_step,
                           "migration_steps", "ascending schema step", step.migration_id,
                           "runtime sandbox world migration step is invalid", step.source_index);
            continue;
        }
        valid_steps.push_back(step);
    }
    std::ranges::sort(valid_steps, [](const auto& lhs, const auto& rhs) {
        if (lhs.from_schema_version != rhs.from_schema_version) {
            return lhs.from_schema_version < rhs.from_schema_version;
        }
        if (lhs.to_schema_version != rhs.to_schema_version) {
            return lhs.to_schema_version < rhs.to_schema_version;
        }
        return lhs.migration_id < rhs.migration_id;
    });

    const auto has_blocking_diagnostic = std::ranges::any_of(plan.diagnostics, [](const auto& diagnostic) {
        return diagnostic.code != RuntimeSandboxWorldPersistenceDiagnosticCode::repairable_corruption;
    });
    if (has_blocking_diagnostic) {
        sort_diagnostics(plan.diagnostics);
        plan.repair_rows.clear();
        plan.status = RuntimeSandboxWorldPersistenceStatus::invalid_request;
        return plan;
    }

    if (desc.observed_schema_version < desc.target_schema_version) {
        std::function<std::optional<std::vector<RuntimeSandboxWorldMigrationStepRow>>(std::uint32_t)> build_chain =
            [&](std::uint32_t schema_version) -> std::optional<std::vector<RuntimeSandboxWorldMigrationStepRow>> {
            if (schema_version == desc.target_schema_version) {
                return std::vector<RuntimeSandboxWorldMigrationStepRow>{};
            }
            std::optional<std::vector<RuntimeSandboxWorldMigrationStepRow>> best_chain;
            for (const auto& step : valid_steps) {
                if (step.from_schema_version != schema_version || step.to_schema_version > desc.target_schema_version) {
                    continue;
                }
                auto suffix = build_chain(step.to_schema_version);
                if (!suffix.has_value()) {
                    continue;
                }
                std::vector<RuntimeSandboxWorldMigrationStepRow> candidate{step};
                candidate.insert(candidate.end(), suffix->begin(), suffix->end());
                if (!best_chain.has_value() || migration_chain_is_better(candidate, *best_chain)) {
                    best_chain = std::move(candidate);
                }
            }
            return best_chain;
        };

        if (auto chain = build_chain(desc.observed_schema_version)) {
            plan.migration_rows = std::move(*chain);
        } else {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::missing_migration,
                           "schema.version", std::to_string(desc.target_schema_version),
                           std::to_string(desc.observed_schema_version),
                           "runtime sandbox world migration review requires a complete schema chain");
        }
    }

    if (!plan.diagnostics.empty()) {
        const auto only_repairable = std::ranges::all_of(plan.diagnostics, [](const auto& diagnostic) {
            return diagnostic.code == RuntimeSandboxWorldPersistenceDiagnosticCode::repairable_corruption;
        });
        sort_diagnostics(plan.diagnostics);
        if (!only_repairable) {
            plan.migration_rows.clear();
            plan.repair_rows.clear();
            plan.status = RuntimeSandboxWorldPersistenceStatus::invalid_request;
            return plan;
        }
        plan.status = RuntimeSandboxWorldPersistenceStatus::recovery_required;
        plan.replay_hash = compute_migration_replay_hash(plan);
        return plan;
    }

    plan.status = plan.migration_rows.empty() ? RuntimeSandboxWorldPersistenceStatus::ready
                                              : RuntimeSandboxWorldPersistenceStatus::migration_required;
    plan.replay_hash = compute_migration_replay_hash(plan);
    return plan;
}

RuntimeSandboxWorldAtomicSavePlan
plan_runtime_sandbox_world_atomic_save(const RuntimeSandboxWorldAtomicSaveDesc& desc) {
    RuntimeSandboxWorldAtomicSavePlan plan;
    const auto validate_path = [&plan](std::string_view path, std::string_view field) {
        if (!is_safe_project_path(path)) {
            add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_path,
                           std::string(field), "project-relative path", std::string(path),
                           "runtime sandbox world atomic-save path must be project-relative");
        }
    };
    validate_path(desc.target_path, "target_path");
    validate_path(desc.temp_path, "temp_path");
    validate_path(desc.backup_path, "backup_path");
    if (desc.payload_content_hash == 0U || desc.payload_byte_size == 0U) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_cell, "payload",
                       "non-zero hash and byte size", {},
                       "runtime sandbox world atomic-save payload evidence must be non-zero");
    }

    const auto expected_operations = desc.require_flush ? 4U : 3U;
    if (expected_operations > desc.row_budget) {
        add_diagnostic(plan.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::row_budget_exceeded,
                       "row_budget", std::to_string(desc.row_budget), std::to_string(expected_operations),
                       "runtime sandbox world atomic-save plan exceeds its operation row budget");
    }
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan.diagnostics);
        plan.status = RuntimeSandboxWorldPersistenceStatus::invalid_request;
        return plan;
    }

    plan.operation_rows.push_back(RuntimeSandboxWorldAtomicSaveOperationRow{
        .step_index = 0U,
        .kind = RuntimeSandboxWorldAtomicSaveOperationKind::write_temp,
        .source_path = {},
        .target_path = desc.temp_path,
        .backup_path = {},
        .payload_content_hash = desc.payload_content_hash,
        .payload_byte_size = desc.payload_byte_size,
        .flush_required = false,
        .rollback_only_on_failure = false,
    });
    if (desc.require_flush) {
        plan.operation_rows.push_back(RuntimeSandboxWorldAtomicSaveOperationRow{
            .step_index = 1U,
            .kind = RuntimeSandboxWorldAtomicSaveOperationKind::flush_temp,
            .source_path = desc.temp_path,
            .target_path = desc.temp_path,
            .backup_path = {},
            .payload_content_hash = desc.payload_content_hash,
            .payload_byte_size = desc.payload_byte_size,
            .flush_required = true,
            .rollback_only_on_failure = false,
        });
    }
    const auto replace_step = static_cast<std::uint32_t>(plan.operation_rows.size());
    plan.operation_rows.push_back(RuntimeSandboxWorldAtomicSaveOperationRow{
        .step_index = replace_step,
        .kind = RuntimeSandboxWorldAtomicSaveOperationKind::replace_target_with_temp,
        .source_path = desc.temp_path,
        .target_path = desc.target_path,
        .backup_path = desc.backup_path,
        .payload_content_hash = desc.payload_content_hash,
        .payload_byte_size = desc.payload_byte_size,
        .flush_required = desc.require_flush,
        .rollback_only_on_failure = false,
    });
    plan.operation_rows.push_back(RuntimeSandboxWorldAtomicSaveOperationRow{
        .step_index = static_cast<std::uint32_t>(plan.operation_rows.size()),
        .kind = RuntimeSandboxWorldAtomicSaveOperationKind::rollback_from_backup_on_failure,
        .source_path = desc.backup_path,
        .target_path = desc.target_path,
        .backup_path = desc.backup_path,
        .payload_content_hash = desc.payload_content_hash,
        .payload_byte_size = desc.payload_byte_size,
        .flush_required = desc.require_flush,
        .rollback_only_on_failure = true,
    });
    plan.status = RuntimeSandboxWorldPersistenceStatus::ready;
    plan.replay_hash = compute_atomic_replay_hash(plan);
    return plan;
}

} // namespace mirakana::runtime
