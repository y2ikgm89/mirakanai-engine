// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/sandbox_world_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSandboxWorldPersistenceStatus : std::uint8_t {
    ready = 0,
    migration_required,
    recovery_required,
    invalid_request,
};

enum class RuntimeSandboxWorldPersistenceDiagnosticCode : std::uint8_t {
    missing_world_id = 0,
    invalid_schema_version,
    invalid_chunk,
    invalid_cell,
    invalid_path,
    row_budget_exceeded,
    size_budget_exceeded,
    unsupported_future_schema,
    missing_migration,
    invalid_migration_step,
    repairable_corruption,
    unrecoverable_corruption,
};

enum class RuntimeSandboxWorldAtomicSaveOperationKind : std::uint8_t {
    write_temp = 0,
    flush_temp,
    replace_target_with_temp,
    rollback_from_backup_on_failure,
};

struct RuntimeSandboxWorldPersistenceDiagnostic {
    RuntimeSandboxWorldPersistenceDiagnosticCode code{RuntimeSandboxWorldPersistenceDiagnosticCode::missing_world_id};
    std::string field;
    std::string expected;
    std::string actual;
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldPersistenceDiagnostic&) const = default;
};

struct RuntimeSandboxWorldPersistenceDocumentDesc {
    RuntimeSandboxWorld world;
    std::uint32_t schema_version{1U};
    std::string source_package_id;
    std::uint64_t seed{0U};
    std::vector<RuntimeSandboxWorldDirtyRegion> dirty_regions;
    std::size_t row_budget{1024U};
    std::size_t byte_budget{1024U * 1024U};
};

struct RuntimeSandboxWorldChunkSnapshotRow {
    std::string chunk_id;
    RuntimeSandboxCellCoord origin;
    RuntimeSandboxCellCoord size;
    std::uint64_t layer_mask{0U};
    std::size_t changed_cell_count{0U};
    std::string source_package_id;
    std::uint64_t content_hash{0U};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldChunkSnapshotRow&) const = default;
};

struct RuntimeSandboxWorldLayerSnapshotRow {
    std::string chunk_id;
    std::int32_t layer_id{0};
    std::size_t cell_count{0U};
    std::uint64_t content_hash{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldLayerSnapshotRow&) const = default;
};

struct RuntimeSandboxWorldChangedCellRow {
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::int32_t layer_id{0};
    std::string tile_id;
    std::uint64_t content_hash{0U};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldChangedCellRow&) const = default;
};

struct RuntimeSandboxWorldPersistenceDocumentPlan {
    RuntimeSandboxWorldPersistenceStatus status{RuntimeSandboxWorldPersistenceStatus::invalid_request};
    std::vector<RuntimeSandboxWorldPersistenceDiagnostic> diagnostics;
    std::string world_id;
    std::uint32_t schema_version{0U};
    std::string source_package_id;
    std::uint64_t seed{0U};
    std::uint64_t world_tick{0U};
    std::vector<RuntimeSandboxWorldChunkSnapshotRow> chunk_rows;
    std::vector<RuntimeSandboxWorldLayerSnapshotRow> layer_rows;
    std::vector<RuntimeSandboxWorldChangedCellRow> changed_cell_rows;
    std::string canonical_text;
    std::uint64_t content_hash{0U};
    std::size_t byte_size_estimate{0U};
    bool invoked_filesystem_io{false};
    bool invoked_platform_call{false};
    bool invoked_threading{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSandboxWorldSnapshotDiffDesc {
    RuntimeSandboxWorldPersistenceDocumentDesc document;
    std::vector<RuntimeSandboxWorldChunkSnapshotRow> previous_chunk_rows;
};

struct RuntimeSandboxWorldMigrationStepRow {
    std::uint32_t from_schema_version{0U};
    std::uint32_t to_schema_version{0U};
    std::string migration_id;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldMigrationStepRow&) const = default;
};

struct RuntimeSandboxWorldCorruptionRow {
    std::string key;
    bool repairable{false};
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldCorruptionRow&) const = default;
};

struct RuntimeSandboxWorldMigrationReviewDesc {
    std::uint32_t observed_schema_version{1U};
    std::uint32_t target_schema_version{1U};
    std::uint32_t minimum_supported_schema_version{1U};
    std::vector<RuntimeSandboxWorldMigrationStepRow> migration_steps;
    std::vector<RuntimeSandboxWorldCorruptionRow> corruption_rows;
    std::size_t row_budget{1024U};
};

struct RuntimeSandboxWorldMigrationReviewPlan {
    RuntimeSandboxWorldPersistenceStatus status{RuntimeSandboxWorldPersistenceStatus::invalid_request};
    std::vector<RuntimeSandboxWorldPersistenceDiagnostic> diagnostics;
    std::uint32_t observed_schema_version{0U};
    std::uint32_t target_schema_version{0U};
    std::vector<RuntimeSandboxWorldMigrationStepRow> migration_rows;
    std::vector<RuntimeSandboxWorldCorruptionRow> repair_rows;
    std::uint64_t replay_hash{0U};
    bool invoked_filesystem_io{false};
    bool invoked_platform_call{false};
    bool invoked_threading{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSandboxWorldAtomicSaveDesc {
    std::string target_path;
    std::string temp_path;
    std::string backup_path;
    std::uint64_t payload_content_hash{0U};
    std::size_t payload_byte_size{0U};
    bool require_flush{true};
    std::size_t row_budget{8U};
};

struct RuntimeSandboxWorldAtomicSaveOperationRow {
    std::uint32_t step_index{0U};
    RuntimeSandboxWorldAtomicSaveOperationKind kind{RuntimeSandboxWorldAtomicSaveOperationKind::write_temp};
    std::string source_path;
    std::string target_path;
    std::string backup_path;
    std::uint64_t payload_content_hash{0U};
    std::size_t payload_byte_size{0U};
    bool flush_required{false};
    bool rollback_only_on_failure{false};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldAtomicSaveOperationRow&) const = default;
};

struct RuntimeSandboxWorldAtomicSavePlan {
    RuntimeSandboxWorldPersistenceStatus status{RuntimeSandboxWorldPersistenceStatus::invalid_request};
    std::vector<RuntimeSandboxWorldPersistenceDiagnostic> diagnostics;
    std::vector<RuntimeSandboxWorldAtomicSaveOperationRow> operation_rows;
    std::uint64_t replay_hash{0U};
    bool invoked_filesystem_io{false};
    bool invoked_platform_call{false};
    bool invoked_threading{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeSandboxWorldPersistenceDocumentPlan
plan_runtime_sandbox_world_persistence_document(const RuntimeSandboxWorldPersistenceDocumentDesc& desc);

[[nodiscard]] RuntimeSandboxWorldPersistenceDocumentPlan
plan_runtime_sandbox_world_snapshot_diff(const RuntimeSandboxWorldSnapshotDiffDesc& desc);

[[nodiscard]] RuntimeSandboxWorldMigrationReviewPlan
review_runtime_sandbox_world_migration(const RuntimeSandboxWorldMigrationReviewDesc& desc);

[[nodiscard]] RuntimeSandboxWorldAtomicSavePlan
plan_runtime_sandbox_world_atomic_save(const RuntimeSandboxWorldAtomicSaveDesc& desc);

} // namespace mirakana::runtime
