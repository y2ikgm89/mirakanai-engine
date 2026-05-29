// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/genre_sandbox_world.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSandboxWorldRuntimeStatus : std::uint8_t {
    ready = 0,
    no_chunks,
    invalid_request,
};

enum class RuntimeSandboxWorldRuntimeDiagnosticCode : std::uint8_t {
    missing_world_id = 0,
    unsupported_backend_reference,
    invalid_chunk,
    duplicate_chunk,
    invalid_cell,
    duplicate_cell,
    unknown_cell_chunk,
    row_budget_exceeded,
};

enum class RuntimeSandboxCellSampleStatus : std::uint8_t {
    occupied = 0,
    empty,
    missing_chunk,
};

struct RuntimeSandboxWorldDesc {
    std::string world_id;
    std::uint64_t world_tick{0U};
    std::vector<RuntimeSandboxChunkRow> chunk_rows;
    std::vector<RuntimeSandboxExistingCellRow> existing_cell_rows;
    std::size_t row_budget{1024U};
};

struct RuntimeSandboxWorldRuntimeDiagnostic {
    RuntimeSandboxWorldRuntimeDiagnosticCode code{RuntimeSandboxWorldRuntimeDiagnosticCode::missing_world_id};
    std::string world_id;
    std::string row_id;
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldRuntimeDiagnostic&) const = default;
};

struct RuntimeSandboxWorld {
    std::string world_id;
    std::uint64_t world_tick{0U};
    std::vector<RuntimeSandboxChunkRow> chunks;
    std::vector<RuntimeSandboxExistingCellRow> cells;
    std::size_t chunk_count{0U};
    std::size_t cell_count{0U};
    std::uint64_t snapshot_hash{0U};
    bool invoked_persistence_io{false};
    bool invoked_package_io{false};
    bool invoked_renderer_upload{false};
};

struct RuntimeSandboxWorldSnapshot {
    std::string world_id;
    std::uint64_t world_tick{0U};
    std::size_t chunk_count{0U};
    std::size_t cell_count{0U};
    std::uint64_t hash{0U};
};

struct RuntimeSandboxWorldBuildResult {
    RuntimeSandboxWorldRuntimeStatus status{RuntimeSandboxWorldRuntimeStatus::invalid_request};
    RuntimeSandboxWorld world;
    std::vector<RuntimeSandboxWorldRuntimeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSandboxCellSample {
    RuntimeSandboxCellSampleStatus status{RuntimeSandboxCellSampleStatus::missing_chunk};
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    bool destructible{false};
    bool protected_cell{false};
};

/// Builds a deterministic in-memory sandbox world from reviewed chunk and cell rows.
/// This API owns no renderer, package, persistence, platform, thread, native, SDL3, or editor side effects.
[[nodiscard]] RuntimeSandboxWorldBuildResult build_runtime_sandbox_world(const RuntimeSandboxWorldDesc& desc);

[[nodiscard]] RuntimeSandboxCellSample sample_runtime_sandbox_cell(const RuntimeSandboxWorld& world,
                                                                   RuntimeSandboxCellCoord coord);

[[nodiscard]] RuntimeSandboxWorldSnapshot snapshot_runtime_sandbox_world(const RuntimeSandboxWorld& world);

} // namespace mirakana::runtime
