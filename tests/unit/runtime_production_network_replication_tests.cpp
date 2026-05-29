// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/production_network_replication.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeNetworkLocalRole;
using mirakana::runtime::RuntimeNetworkReplicationAuthority;
using mirakana::runtime::RuntimeNetworkReplicationDelivery;
using mirakana::runtime::RuntimeNetworkReplicationDiagnosticCode;
using mirakana::runtime::RuntimeNetworkReplicationMode;
using mirakana::runtime::RuntimeNetworkReplicationStatus;
using mirakana::runtime::RuntimeNetworkSessionTopology;
using mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
using mirakana::runtime::RuntimeNetworkTrustBoundary;
using mirakana::runtime::RuntimeReplicationOwnership;
using mirakana::runtime::RuntimeReplicationSnapshotKind;
using mirakana::runtime::RuntimeRollbackMode;
using mirakana::runtime::RuntimeSandboxMutationKind;

[[nodiscard]] mirakana::runtime::RuntimeNetworkTransportRequirementDesc
make_transport(RuntimeNetworkTransportCapabilityKind capability, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
        .capability = capability,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationChannelDesc
make_foundation_channel(std::string channel_id, RuntimeNetworkReplicationAuthority authority,
                        RuntimeNetworkReplicationDelivery delivery, std::uint32_t tick_rate_hz,
                        std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
        .channel_id = std::move(channel_id),
        .authority = authority,
        .delivery = delivery,
        .tick_rate_hz = tick_rate_hz,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkFoundationPlan
make_foundation_plan(RuntimeNetworkSessionTopology topology = RuntimeNetworkSessionTopology::listen_server,
                     RuntimeNetworkLocalRole role = RuntimeNetworkLocalRole::host,
                     RuntimeNetworkReplicationAuthority state_authority = RuntimeNetworkReplicationAuthority::server,
                     RuntimeNetworkReplicationAuthority input_authority = RuntimeNetworkReplicationAuthority::client,
                     bool deterministic_simulation = true, bool ordered_inputs = true,
                     std::uint32_t tick_rate_hz = 60U) {
    const auto policy = mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
        .sessions =
            {
                mirakana::runtime::RuntimeNetworkSessionDesc{
                    .session_id = "arena",
                    .topology = topology,
                    .local_role = role,
                    .trust_boundary = RuntimeNetworkTrustBoundary::untrusted_remote_peers,
                    .transports =
                        {
                            make_transport(RuntimeNetworkTransportCapabilityKind::reliable_ordered, 1U),
                            make_transport(RuntimeNetworkTransportCapabilityKind::unreliable_unordered, 2U),
                            make_transport(RuntimeNetworkTransportCapabilityKind::encrypted_transport, 3U),
                            make_transport(RuntimeNetworkTransportCapabilityKind::authenticated_peer, 4U),
                        },
                    .channels =
                        {
                            make_foundation_channel("state", state_authority,
                                                    RuntimeNetworkReplicationDelivery::state_snapshot, tick_rate_hz,
                                                    5U),
                            make_foundation_channel("input", input_authority,
                                                    RuntimeNetworkReplicationDelivery::unreliable_unordered,
                                                    tick_rate_hz, 6U),
                        },
                    .replay =
                        mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                            .replay_seed = 42U,
                            .fixed_tick_rate_hz = tick_rate_hz,
                            .deterministic_simulation = deterministic_simulation,
                            .ordered_inputs = ordered_inputs,
                            .source_index = 7U,
                        },
                    .source_index = 8U,
                },
            },
        .reviewed_transport_capabilities =
            {
                RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                RuntimeNetworkTransportCapabilityKind::unreliable_unordered,
                RuntimeNetworkTransportCapabilityKind::encrypted_transport,
                RuntimeNetworkTransportCapabilityKind::authenticated_peer,
            },
    };
    return mirakana::runtime::plan_runtime_network_foundation(policy);
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationSessionDesc make_session(std::uint32_t tick_rate_hz = 60U) {
    return mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
        .session_id = "arena",
        .world_id = "world.arena",
        .mode = RuntimeNetworkReplicationMode::authoritative_snapshot,
        .fixed_tick_rate_hz = tick_rate_hz,
        .max_players = 4U,
        .max_objects = 16U,
        .source_index = 1U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicatedObjectRow
make_object(std::string object_id, std::string entity_id, std::string channel_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicatedObjectRow{
        .object_id = std::move(object_id),
        .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = std::move(entity_id)},
        .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
        .schema_id = mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
        .channel_id = std::move(channel_id),
        .ownership = RuntimeReplicationOwnership::server_owned,
        .priority = 10U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicationInputCommandRow
make_input(std::string player_id, std::string command_id, std::uint64_t tick, std::uint64_t sequence,
           std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicationInputCommandRow{
        .player_id = std::move(player_id),
        .command_id = std::move(command_id),
        .channel_id = "input",
        .target_tick = tick,
        .sequence = sequence,
        .payload_hash = 1000U + sequence,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicationSnapshotRow
make_snapshot(std::string snapshot_id, std::uint64_t tick, std::vector<std::string> object_ids,
              std::uint32_t byte_count, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicationSnapshotRow{
        .snapshot_id = std::move(snapshot_id),
        .channel_id = "state",
        .tick = tick,
        .kind = RuntimeReplicationSnapshotKind::delta_state,
        .object_ids = std::move(object_ids),
        .state_hash = 9000U + tick,
        .byte_count = byte_count,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRollbackPolicyRow make_rollback(RuntimeRollbackMode mode) {
    return mirakana::runtime::RuntimeRollbackPolicyRow{
        .mode = mode,
        .max_rollback_ticks = 8U,
        .input_delay_ticks = 2U,
        .snapshot_history_limit = 16U,
        .requires_deterministic_simulation = true,
        .requires_ordered_inputs = true,
        .requires_transport_host_evidence = true,
        .source_index = 20U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkTransportEvidenceRow make_transport_evidence() {
    return mirakana::runtime::RuntimeNetworkTransportEvidenceRow{
        .adapter_id = "enet.loopback",
        .loopback_validated = true,
        .reliable_validated = true,
        .unreliable_validated = true,
        .source_index = 30U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationRequest make_valid_request(bool include_transport_evidence) {
    return mirakana::runtime::RuntimeNetworkReplicationRequest{
        .foundation_plan = make_foundation_plan(),
        .session = make_session(),
        .object_rows =
            {
                make_object("player.0", "entity.player0", "state", 1U),
                make_object("crate.0", "entity.crate0", "state", 2U),
            },
        .input_rows =
            {
                make_input("player.0", "move.left", 100U, 1U, 1U),
                make_input("player.0", "move.right", 101U, 2U, 2U),
            },
        .snapshot_rows =
            {
                make_snapshot("snapshot.100", 100U, {"player.0", "crate.0"}, 256U, 1U),
                make_snapshot("snapshot.101", 101U, {"player.0", "crate.0"}, 128U, 2U),
            },
        .sandbox_mutation_command_rows = {},
        .sandbox_snapshot_delta_rows = {},
        .rollback_policy = make_rollback(RuntimeRollbackMode::input_resimulation),
        .transport_evidence_rows = include_transport_evidence
                                       ? std::vector{make_transport_evidence()}
                                       : std::vector<mirakana::runtime::RuntimeNetworkTransportEvidenceRow>{},
        .row_budget = 32U,
        .snapshot_byte_budget = 1024U,
        .seed = 99U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeNetworkReplicationPlan& plan,
                                           RuntimeNetworkReplicationDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("production network replication plans authoritative rows with host transport evidence") {
    const auto plan = mirakana::runtime::plan_runtime_network_replication(make_valid_request(true));

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.session_rows.size() == 1U);
    MK_REQUIRE(plan.session_rows[0].session_id == "arena");
    MK_REQUIRE(plan.session_rows[0].world_id == "world.arena");
    MK_REQUIRE(plan.object_rows.size() == 2U);
    MK_REQUIRE(plan.object_rows[0].object_id == "crate.0");
    MK_REQUIRE(plan.object_rows[1].object_id == "player.0");
    MK_REQUIRE(plan.input_rows.size() == 2U);
    MK_REQUIRE(plan.snapshot_rows.size() == 2U);
    MK_REQUIRE(plan.rollback_rows.size() == 1U);
    MK_REQUIRE(plan.transport_evidence_rows.size() == 1U);
    MK_REQUIRE(plan.replicated_object_count == 2U);
    MK_REQUIRE(plan.input_row_count == 2U);
    MK_REQUIRE(plan.snapshot_row_count == 2U);
    MK_REQUIRE(plan.rollback_row_count == 1U);
    MK_REQUIRE(plan.rejected_unsafe_row_count == 0U);
    MK_REQUIRE(plan.requires_transport_host_evidence);
    MK_REQUIRE(plan.has_transport_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_network_io);
    MK_REQUIRE(!plan.invoked_rollback_execution);
    MK_REQUIRE(!plan.invoked_world_mutation);
}

MK_TEST("production network replication separates reviewed planning from missing host transport evidence") {
    const auto plan = mirakana::runtime::plan_runtime_network_replication(make_valid_request(false));

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.replicated_object_count == 2U);
    MK_REQUIRE(plan.input_row_count == 2U);
    MK_REQUIRE(plan.snapshot_row_count == 2U);
    MK_REQUIRE(plan.rollback_row_count == 1U);
    MK_REQUIRE(plan.requires_transport_host_evidence);
    MK_REQUIRE(!plan.has_transport_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_network_io);
}

MK_TEST("production network replication rejects unsupported lockstep modes") {
    auto request = make_valid_request(true);
    request.session.mode = RuntimeNetworkReplicationMode::deterministic_lockstep;
    request.rollback_policy.mode = RuntimeRollbackMode::lockstep_only;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.session_rows.empty());
    MK_REQUIRE(plan.object_rows.empty());
    MK_REQUIRE(plan.input_rows.empty());
    MK_REQUIRE(plan.snapshot_rows.empty());
    MK_REQUIRE(plan.rollback_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_replication_mode) == 2U);
    MK_REQUIRE(plan.rejected_unsafe_row_count == 2U);
}

MK_TEST("production network replication rejects unsupported topology unknown channels and authority mismatches") {
    auto request = make_valid_request(true);
    request.foundation_plan =
        make_foundation_plan(RuntimeNetworkSessionTopology::peer_to_peer, RuntimeNetworkLocalRole::peer);
    request.object_rows.push_back(make_object("player.0", "entity.dupe", "missing", 3U));
    request.input_rows.push_back(mirakana::runtime::RuntimeReplicationInputCommandRow{
        .player_id = "player.1",
        .command_id = "bad.snapshot.channel",
        .channel_id = "state",
        .target_tick = 102U,
        .sequence = 1U,
        .payload_hash = 100U,
        .source_index = 4U,
    });
    request.snapshot_rows.push_back(mirakana::runtime::RuntimeReplicationSnapshotRow{
        .snapshot_id = "snapshot.bad",
        .channel_id = "input",
        .tick = 102U,
        .kind = RuntimeReplicationSnapshotKind::full_state,
        .object_ids = {"player.0"},
        .state_hash = 100U,
        .byte_count = 32U,
        .source_index = 5U,
    });

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.session_rows.empty());
    MK_REQUIRE(plan.object_rows.empty());
    MK_REQUIRE(plan.input_rows.empty());
    MK_REQUIRE(plan.snapshot_rows.empty());
    MK_REQUIRE(plan.rollback_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_topology) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::missing_authoritative_session) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_channel_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch) == 2U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_object_id) == 1U);
}

MK_TEST("production network replication rejects nonmonotonic input and snapshot timelines") {
    auto request = make_valid_request(true);
    request.input_rows = {
        make_input("player.0", "move.1", 100U, 10U, 1U),
        make_input("player.0", "move.2", 99U, 11U, 2U),
        make_input("player.0", "move.3", 101U, 10U, 3U),
    };
    request.snapshot_rows = {
        make_snapshot("snapshot.100", 100U, {"player.0"}, 64U, 1U),
        make_snapshot("snapshot.090", 90U, {"player.0"}, 64U, 2U),
    };

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_input_sequence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::non_monotonic_input_tick) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::non_monotonic_snapshot_tick) == 1U);
}

MK_TEST("production network replication rejects rollback when deterministic prerequisites are absent") {
    auto request = make_valid_request(true);
    request.foundation_plan.replay_prerequisites[0].deterministic_simulation = false;
    request.foundation_plan.replay_prerequisites[0].ordered_inputs = false;
    request.rollback_policy.max_rollback_ticks = 17U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_prerequisite_missing) == 2U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_window_exceeded) == 1U);
}

MK_TEST("production network replication rejects budgets unsupported host claims and invalid foundation plans") {
    auto request = make_valid_request(true);
    request.foundation_plan.status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus::invalid_request;
    request.snapshot_rows[0].byte_count = 2048U;
    request.transport_evidence_rows.push_back(mirakana::runtime::RuntimeNetworkTransportEvidenceRow{
        .adapter_id = "bad.evidence",
        .loopback_validated = true,
        .reliable_validated = false,
        .unreliable_validated = true,
        .source_index = 31U,
    });
    request.row_budget = 2U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_foundation_plan) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::snapshot_byte_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_host_claim) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::row_budget_exceeded) == 1U);
}

MK_TEST("production network replication counts rejected unsafe rows uniquely") {
    auto request = make_valid_request(true);
    request.input_rows = {
        make_input("player.0", "", 100U, 1U, 1U),
    };
    request.input_rows[0].channel_id = "missing";
    request.input_rows[0].payload_hash = 0U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_input_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_channel_id) == 1U);
    MK_REQUIRE(plan.diagnostics.size() == 2U);
    MK_REQUIRE(plan.rejected_unsafe_row_count == 1U);
}

MK_TEST("production network replication reviews sandbox mutation commands and snapshot deltas") {
    auto request = make_valid_request(true);
    request.sandbox_mutation_command_rows = {
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "place.stone.0",
            .channel_id = "input",
            .target_tick = 102U,
            .sequence = 3U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 1, .y = 2, .z = 0},
            .block_id = "block.stone",
            .payload_hash = 7003U,
            .byte_count = 48U,
            .source_index = 40U,
        },
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "destroy.dirt.0",
            .channel_id = "input",
            .target_tick = 103U,
            .sequence = 4U,
            .kind = RuntimeSandboxMutationKind::destruction,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 2, .y = 2, .z = 0},
            .block_id = {},
            .payload_hash = 7004U,
            .byte_count = 32U,
            .source_index = 41U,
        },
    };
    request.sandbox_snapshot_delta_rows = {
        mirakana::runtime::RuntimeNetworkSandboxSnapshotDeltaRow{
            .delta_id = "sandbox.delta.103",
            .channel_id = "state",
            .chunk_id = "chunk.0",
            .base_tick = 101U,
            .target_tick = 103U,
            .command_ids = {"place.stone.0", "destroy.dirt.0"},
            .changed_cell_count = 2U,
            .state_hash = 8003U,
            .byte_count = 96U,
            .source_index = 42U,
        },
    };
    request.row_budget = 64U;
    request.snapshot_byte_budget = 4096U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.sandbox_mutation_command_count == 2U);
    MK_REQUIRE(plan.sandbox_snapshot_delta_count == 1U);
    MK_REQUIRE(plan.sandbox_mutation_command_rows[0].command_id == "place.stone.0");
    MK_REQUIRE(plan.sandbox_mutation_command_rows[1].command_id == "destroy.dirt.0");
    MK_REQUIRE(plan.sandbox_snapshot_delta_rows[0].delta_id == "sandbox.delta.103");
    MK_REQUIRE(plan.sandbox_snapshot_delta_rows[0].changed_cell_count == 2U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_network_io);
    MK_REQUIRE(!plan.invoked_rollback_execution);
    MK_REQUIRE(!plan.invoked_world_mutation);
}

MK_TEST("production network replication rejects sandbox mutation replay authority and delta abuse") {
    auto request = make_valid_request(true);
    request.sandbox_mutation_command_rows = {
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "place.1",
            .channel_id = "input",
            .target_tick = 105U,
            .sequence = 8U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 1, .y = 1, .z = 0},
            .block_id = "block.stone",
            .payload_hash = 8001U,
            .byte_count = 48U,
            .source_index = 50U,
        },
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "place.2",
            .channel_id = "input",
            .target_tick = 104U,
            .sequence = 8U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 2, .y = 1, .z = 0},
            .block_id = "block.stone",
            .payload_hash = 8002U,
            .byte_count = 48U,
            .source_index = 51U,
        },
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "state.channel.command",
            .channel_id = "state",
            .target_tick = 106U,
            .sequence = 9U,
            .kind = RuntimeSandboxMutationKind::destruction,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 3, .y = 1, .z = 0},
            .payload_hash = 8003U,
            .byte_count = 24U,
            .source_index = 52U,
        },
    };
    request.sandbox_snapshot_delta_rows = {
        mirakana::runtime::RuntimeNetworkSandboxSnapshotDeltaRow{
            .delta_id = "bad.delta",
            .channel_id = "input",
            .chunk_id = "chunk.0",
            .base_tick = 108U,
            .target_tick = 107U,
            .command_ids = {"missing.command"},
            .changed_cell_count = 0U,
            .state_hash = 0U,
            .byte_count = 2048U,
            .source_index = 60U,
        },
    };
    request.snapshot_byte_budget = 1024U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.sandbox_mutation_command_rows.empty());
    MK_REQUIRE(plan.sandbox_snapshot_delta_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_sandbox_mutation_sequence) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::non_monotonic_sandbox_mutation_tick) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch) == 2U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_sandbox_snapshot_delta) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_sandbox_snapshot_command) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::sandbox_delta_byte_budget_exceeded) ==
               1U);
}

MK_TEST("production network replication rejects duplicate sandbox mutation command ids") {
    auto request = make_valid_request(true);
    request.sandbox_mutation_command_rows = {
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "place.duplicate",
            .channel_id = "input",
            .target_tick = 102U,
            .sequence = 3U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 1, .y = 2, .z = 0},
            .block_id = "block.stone",
            .payload_hash = 7003U,
            .byte_count = 48U,
            .source_index = 70U,
        },
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.1",
            .command_id = "place.duplicate",
            .channel_id = "input",
            .target_tick = 103U,
            .sequence = 1U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.1",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 2, .y = 2, .z = 0},
            .block_id = "block.dirt",
            .payload_hash = 7004U,
            .byte_count = 48U,
            .source_index = 71U,
        },
    };
    request.sandbox_snapshot_delta_rows = {
        mirakana::runtime::RuntimeNetworkSandboxSnapshotDeltaRow{
            .delta_id = "sandbox.delta.duplicate",
            .channel_id = "state",
            .chunk_id = "chunk.0",
            .base_tick = 101U,
            .target_tick = 103U,
            .command_ids = {"place.duplicate"},
            .changed_cell_count = 1U,
            .state_hash = 8103U,
            .byte_count = 96U,
            .source_index = 72U,
        },
    };

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.sandbox_mutation_command_rows.empty());
    MK_REQUIRE(plan.sandbox_snapshot_delta_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_sandbox_mutation_command_id) ==
               1U);
}

MK_TEST("production network replication applies one byte budget across snapshots and sandbox deltas") {
    auto request = make_valid_request(true);
    request.sandbox_mutation_command_rows = {
        mirakana::runtime::RuntimeNetworkSandboxMutationCommandRow{
            .player_id = "player.0",
            .command_id = "place.stone.0",
            .channel_id = "input",
            .target_tick = 102U,
            .sequence = 3U,
            .kind = RuntimeSandboxMutationKind::placement,
            .chunk_id = "chunk.0",
            .coord = mirakana::runtime::RuntimeSandboxCellCoord{.x = 1, .y = 2, .z = 0},
            .block_id = "block.stone",
            .payload_hash = 7003U,
            .byte_count = 48U,
            .source_index = 80U,
        },
    };
    request.sandbox_snapshot_delta_rows = {
        mirakana::runtime::RuntimeNetworkSandboxSnapshotDeltaRow{
            .delta_id = "sandbox.delta.102",
            .channel_id = "state",
            .chunk_id = "chunk.0",
            .base_tick = 101U,
            .target_tick = 102U,
            .command_ids = {"place.stone.0"},
            .changed_cell_count = 1U,
            .state_hash = 8102U,
            .byte_count = 700U,
            .source_index = 81U,
        },
    };
    request.snapshot_byte_budget = 1024U;

    const auto plan = mirakana::runtime::plan_runtime_network_replication(request);

    MK_REQUIRE(plan.status == RuntimeNetworkReplicationStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.snapshot_rows.empty());
    MK_REQUIRE(plan.sandbox_snapshot_delta_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::sandbox_delta_byte_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkReplicationDiagnosticCode::snapshot_byte_budget_exceeded) == 0U);
}

int main() {
    return mirakana::test::run_all();
}
