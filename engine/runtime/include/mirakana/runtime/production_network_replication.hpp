// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/genre_sandbox_world.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/world_entity_model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeNetworkReplicationStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class RuntimeNetworkReplicationMode : std::uint8_t {
    authoritative_snapshot = 0,
    deterministic_lockstep,
};

enum class RuntimeReplicationOwnership : std::uint8_t {
    server_owned = 0,
    client_owned,
    shared_reviewed,
};

enum class RuntimeReplicationSnapshotKind : std::uint8_t {
    full_state = 0,
    delta_state,
};

enum class RuntimeRollbackMode : std::uint8_t {
    disabled = 0,
    input_resimulation,
    lockstep_only,
};

enum class RuntimeNetworkReplicationDiagnosticCode : std::uint8_t {
    invalid_foundation_plan = 0,
    unsupported_replication_mode,
    unsupported_topology,
    missing_authoritative_session,
    invalid_session_row,
    unknown_channel_id,
    channel_authority_mismatch,
    duplicate_object_id,
    invalid_object_id,
    invalid_input_row,
    duplicate_input_sequence,
    non_monotonic_input_tick,
    invalid_snapshot_row,
    non_monotonic_snapshot_tick,
    unknown_snapshot_object,
    snapshot_byte_budget_exceeded,
    rollback_prerequisite_missing,
    rollback_window_exceeded,
    row_budget_exceeded,
    unsupported_host_claim,
    invalid_sandbox_mutation_command,
    duplicate_sandbox_mutation_command_id,
    duplicate_sandbox_mutation_sequence,
    non_monotonic_sandbox_mutation_tick,
    invalid_sandbox_snapshot_delta,
    unknown_sandbox_snapshot_command,
    sandbox_delta_byte_budget_exceeded,
};

struct RuntimeNetworkReplicationSessionDesc {
    std::string session_id;
    std::string world_id;
    RuntimeNetworkReplicationMode mode{RuntimeNetworkReplicationMode::authoritative_snapshot};
    std::uint32_t fixed_tick_rate_hz{0U};
    std::uint32_t max_players{0U};
    std::uint32_t max_objects{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeReplicatedObjectRow {
    std::string object_id;
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId region_id;
    RuntimeWorldComponentSchemaId schema_id;
    std::string channel_id;
    RuntimeReplicationOwnership ownership{RuntimeReplicationOwnership::server_owned};
    std::uint32_t priority{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeReplicationInputCommandRow {
    std::string player_id;
    std::string command_id;
    std::string channel_id;
    std::uint64_t target_tick{0U};
    std::uint64_t sequence{0U};
    std::uint64_t payload_hash{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeReplicationSnapshotRow {
    std::string snapshot_id;
    std::string channel_id;
    std::uint64_t tick{0U};
    RuntimeReplicationSnapshotKind kind{RuntimeReplicationSnapshotKind::full_state};
    std::vector<std::string> object_ids;
    std::uint64_t state_hash{0U};
    std::uint32_t byte_count{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkSandboxMutationCommandRow {
    std::string player_id;
    std::string command_id;
    std::string channel_id;
    std::uint64_t target_tick{0U};
    std::uint64_t sequence{0U};
    RuntimeSandboxMutationKind kind{RuntimeSandboxMutationKind::placement};
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    std::uint64_t payload_hash{0U};
    std::uint32_t byte_count{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkSandboxSnapshotDeltaRow {
    std::string delta_id;
    std::string channel_id;
    std::string chunk_id;
    std::uint64_t base_tick{0U};
    std::uint64_t target_tick{0U};
    std::vector<std::string> command_ids;
    std::uint32_t changed_cell_count{0U};
    std::uint64_t state_hash{0U};
    std::uint32_t byte_count{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeRollbackPolicyRow {
    RuntimeRollbackMode mode{RuntimeRollbackMode::disabled};
    std::uint32_t max_rollback_ticks{0U};
    std::uint32_t input_delay_ticks{0U};
    std::uint32_t snapshot_history_limit{0U};
    bool requires_deterministic_simulation{false};
    bool requires_ordered_inputs{false};
    bool requires_transport_host_evidence{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkTransportEvidenceRow {
    std::string adapter_id;
    bool loopback_validated{false};
    bool reliable_validated{false};
    bool unreliable_validated{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplicationRequest {
    RuntimeNetworkFoundationPlan foundation_plan;
    RuntimeNetworkReplicationSessionDesc session;
    std::vector<RuntimeReplicatedObjectRow> object_rows;
    std::vector<RuntimeReplicationInputCommandRow> input_rows;
    std::vector<RuntimeReplicationSnapshotRow> snapshot_rows;
    std::vector<RuntimeNetworkSandboxMutationCommandRow> sandbox_mutation_command_rows;
    std::vector<RuntimeNetworkSandboxSnapshotDeltaRow> sandbox_snapshot_delta_rows;
    RuntimeRollbackPolicyRow rollback_policy;
    std::vector<RuntimeNetworkTransportEvidenceRow> transport_evidence_rows;
    std::size_t row_budget{512U};
    std::uint64_t snapshot_byte_budget{65536U};
    std::uint64_t seed{0U};
};

struct RuntimeNetworkReplicationDiagnostic {
    RuntimeNetworkReplicationDiagnosticCode code{RuntimeNetworkReplicationDiagnosticCode::invalid_foundation_plan};
    std::string session_id;
    std::string row_id;
    std::string secondary_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplicationPlan {
    RuntimeNetworkReplicationStatus status{RuntimeNetworkReplicationStatus::invalid_request};
    std::vector<RuntimeNetworkReplicationDiagnostic> diagnostics;
    std::vector<RuntimeNetworkReplicationSessionDesc> session_rows;
    std::vector<RuntimeReplicatedObjectRow> object_rows;
    std::vector<RuntimeReplicationInputCommandRow> input_rows;
    std::vector<RuntimeReplicationSnapshotRow> snapshot_rows;
    std::vector<RuntimeNetworkSandboxMutationCommandRow> sandbox_mutation_command_rows;
    std::vector<RuntimeNetworkSandboxSnapshotDeltaRow> sandbox_snapshot_delta_rows;
    std::vector<RuntimeRollbackPolicyRow> rollback_rows;
    std::vector<RuntimeNetworkTransportEvidenceRow> transport_evidence_rows;
    std::size_t replicated_object_count{0U};
    std::size_t input_row_count{0U};
    std::size_t snapshot_row_count{0U};
    std::size_t sandbox_mutation_command_count{0U};
    std::size_t sandbox_snapshot_delta_count{0U};
    std::size_t rollback_row_count{0U};
    std::size_t rejected_unsafe_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool requires_transport_host_evidence{false};
    bool has_transport_host_evidence{false};
    bool invoked_network_io{false};
    bool invoked_rollback_execution{false};
    bool invoked_world_mutation{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews authoritative replication, input sequencing, snapshot, rollback, and optional host-evidence rows.
/// This value-only planner does not open sockets, execute transport IO, run rollback/prediction, mutate worlds,
/// create threads, expose native handles, provide matchmaking/NAT traversal, or claim broad multiplayer readiness.
[[nodiscard]] RuntimeNetworkReplicationPlan
plan_runtime_network_replication(const RuntimeNetworkReplicationRequest& request);

} // namespace mirakana::runtime
