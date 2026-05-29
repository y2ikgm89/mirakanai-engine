// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/network_production_security.hpp"
#include "mirakana/runtime/network_transport.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/production_network_replication.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeNetworkLocalRole;
using mirakana::runtime::RuntimeNetworkProductionSecurityDiagnosticCode;
using mirakana::runtime::RuntimeNetworkProductionSecurityStatus;
using mirakana::runtime::RuntimeNetworkReplicationAuthority;
using mirakana::runtime::RuntimeNetworkReplicationDelivery;
using mirakana::runtime::RuntimeNetworkReplicationMode;
using mirakana::runtime::RuntimeNetworkSessionTopology;
using mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
using mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
using mirakana::runtime::RuntimeNetworkTransportDelivery;
using mirakana::runtime::RuntimeNetworkTrustBoundary;
using mirakana::runtime::RuntimeReplicationOwnership;
using mirakana::runtime::RuntimeReplicationSnapshotKind;
using mirakana::runtime::RuntimeRollbackMode;

[[nodiscard]] std::vector<std::byte> bytes(std::string_view text) {
    std::vector<std::byte> payload;
    payload.reserve(text.size());
    for (const char ch : text) {
        payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return payload;
}

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

[[nodiscard]] mirakana::runtime::RuntimeNetworkFoundationPlan make_foundation_plan() {
    const auto policy = mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
        .sessions =
            {
                mirakana::runtime::RuntimeNetworkSessionDesc{
                    .session_id = "arena",
                    .topology = RuntimeNetworkSessionTopology::listen_server,
                    .local_role = RuntimeNetworkLocalRole::host,
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
                            make_foundation_channel("state", RuntimeNetworkReplicationAuthority::server,
                                                    RuntimeNetworkReplicationDelivery::state_snapshot, 60U, 5U),
                            make_foundation_channel("input", RuntimeNetworkReplicationAuthority::client,
                                                    RuntimeNetworkReplicationDelivery::unreliable_unordered, 60U, 6U),
                        },
                    .replay =
                        mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                            .replay_seed = 42U,
                            .fixed_tick_rate_hz = 60U,
                            .deterministic_simulation = true,
                            .ordered_inputs = true,
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

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationSessionDesc make_session() {
    return mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
        .session_id = "arena",
        .world_id = "world.arena",
        .mode = RuntimeNetworkReplicationMode::authoritative_snapshot,
        .fixed_tick_rate_hz = 60U,
        .max_players = 4U,
        .max_objects = 16U,
        .source_index = 1U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicatedObjectRow make_object(std::string object_id, std::string entity_id,
                                                                        std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicatedObjectRow{
        .object_id = std::move(object_id),
        .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = std::move(entity_id)},
        .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
        .schema_id = mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
        .channel_id = "state",
        .ownership = RuntimeReplicationOwnership::server_owned,
        .priority = 10U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicationInputCommandRow
make_input(std::string command_id, std::uint64_t tick, std::uint64_t sequence, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicationInputCommandRow{
        .player_id = "player.0",
        .command_id = std::move(command_id),
        .channel_id = "input",
        .target_tick = tick,
        .sequence = sequence,
        .payload_hash = 1000U + sequence,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeReplicationSnapshotRow
make_snapshot(std::string snapshot_id, std::uint64_t tick, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeReplicationSnapshotRow{
        .snapshot_id = std::move(snapshot_id),
        .channel_id = "state",
        .tick = tick,
        .kind = RuntimeReplicationSnapshotKind::delta_state,
        .object_ids = {"player.0", "crate.0"},
        .state_hash = 9000U + tick,
        .byte_count = 128U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationRequest make_replication_request(bool transport_evidence) {
    return mirakana::runtime::RuntimeNetworkReplicationRequest{
        .foundation_plan = make_foundation_plan(),
        .session = make_session(),
        .object_rows =
            {
                make_object("player.0", "entity.player0", 1U),
                make_object("crate.0", "entity.crate0", 2U),
            },
        .input_rows =
            {
                make_input("move.left", 100U, 1U, 1U),
                make_input("move.right", 101U, 2U, 2U),
            },
        .snapshot_rows =
            {
                make_snapshot("snapshot.100", 100U, 1U),
                make_snapshot("snapshot.101", 101U, 2U),
            },
        .sandbox_mutation_command_rows = {},
        .sandbox_snapshot_delta_rows = {},
        .rollback_policy =
            mirakana::runtime::RuntimeRollbackPolicyRow{
                .mode = RuntimeRollbackMode::input_resimulation,
                .max_rollback_ticks = 8U,
                .input_delay_ticks = 2U,
                .snapshot_history_limit = 16U,
                .requires_deterministic_simulation = true,
                .requires_ordered_inputs = true,
                .requires_transport_host_evidence = true,
                .source_index = 20U,
            },
        .transport_evidence_rows = transport_evidence
                                       ? std::vector{mirakana::runtime::RuntimeNetworkTransportEvidenceRow{
                                             .adapter_id = "enet_loopback",
                                             .loopback_validated = true,
                                             .reliable_validated = true,
                                             .unreliable_validated = true,
                                             .source_index = 30U,
                                         }}
                                       : std::vector<mirakana::runtime::RuntimeNetworkTransportEvidenceRow>{},
        .row_budget = 32U,
        .snapshot_byte_budget = 1024U,
        .seed = 99U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkLoopbackExchangeResult make_loopback_result() {
    return mirakana::runtime::RuntimeNetworkLoopbackExchangeResult{
        .status = RuntimeNetworkTransportAdapterStatus::completed,
        .adapter_id = "enet",
        .diagnostics = {},
        .server_received_packets =
            {
                mirakana::runtime::RuntimeNetworkReceivedPacketRow{
                    .channel_id = 0U,
                    .delivery = RuntimeNetworkTransportDelivery::reliable_ordered,
                    .payload = bytes("client-reliable"),
                    .remote_peer_index = 0U,
                },
                mirakana::runtime::RuntimeNetworkReceivedPacketRow{
                    .channel_id = 1U,
                    .delivery = RuntimeNetworkTransportDelivery::unreliable_unordered,
                    .payload = bytes("client-unreliable"),
                    .remote_peer_index = 0U,
                },
            },
        .client_received_packets =
            {
                mirakana::runtime::RuntimeNetworkReceivedPacketRow{
                    .channel_id = 0U,
                    .delivery = RuntimeNetworkTransportDelivery::reliable_ordered,
                    .payload = bytes("server-reliable"),
                    .remote_peer_index = 0U,
                },
            },
        .packets_sent = 3U,
        .packets_received = 3U,
        .bytes_sent = 47U,
        .bytes_received = 47U,
        .native_handles_exposed = false,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkProductionThreatModelEvidenceRow make_threat_model() {
    return mirakana::runtime::RuntimeNetworkProductionThreatModelEvidenceRow{
        .evidence_id = "docs/specs/2026-05-26-networking-production-security-threat-model.md",
        .attacker_capabilities_reviewed = true,
        .trust_boundaries_reviewed = true,
        .packet_tampering_reviewed = true,
        .packet_replay_reviewed = true,
        .authentication_gap_reviewed = true,
        .denial_of_service_reviewed = true,
        .nat_matchmaking_exclusions_reviewed = true,
        .save_rollback_abuse_reviewed = true,
        .official_sources_reviewed = true,
        .source_index = 1U,
    };
}

[[nodiscard]] std::vector<mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow> make_validation_evidence() {
    using Kind = mirakana::runtime::RuntimeNetworkProductionValidationKind;
    return {
        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
            .kind = Kind::sequence_replay_rejection,
            .evidence_id = "duplicate-sequence-and-nonmonotonic-input-tests",
            .reviewed = true,
            .source_index = 10U,
        },
        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
            .kind = Kind::input_command_validation,
            .evidence_id = "input-command-channel-authority-tests",
            .reviewed = true,
            .source_index = 11U,
        },
        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
            .kind = Kind::snapshot_validation,
            .evidence_id = "snapshot-object-byte-budget-tests",
            .reviewed = true,
            .source_index = 12U,
        },
        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
            .kind = Kind::rollback_window_diagnostic,
            .evidence_id = "rollback-window-prerequisite-tests",
            .reviewed = true,
            .source_index = 13U,
        },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkProductionSecurityRequest make_security_request() {
    const auto foundation = make_foundation_plan();
    const auto replication = mirakana::runtime::plan_runtime_network_replication(make_replication_request(true));
    return mirakana::runtime::RuntimeNetworkProductionSecurityRequest{
        .threat_model = make_threat_model(),
        .foundation_plan = foundation,
        .replication_plan = replication,
        .loopback_exchange = make_loopback_result(),
        .validation_evidence_rows = make_validation_evidence(),
        .unsupported_online_claim_rows = {},
        .row_budget = 64U,
        .request_native_handles = false,
        .invoked_external_network_io = false,
        .invoked_threads = false,
        .invoked_save_io = false,
        .invoked_world_mutation = false,
        .seed = 777U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeNetworkProductionSecurityPlan& plan,
                                           RuntimeNetworkProductionSecurityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime network production security gate accepts threat modeled loopback and replication evidence") {
    const auto plan = mirakana::runtime::plan_runtime_network_production_security_gate(make_security_request());

    MK_REQUIRE(plan.status == RuntimeNetworkProductionSecurityStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.threat_model_reviewed);
    MK_REQUIRE(plan.loopback_host_evidence);
    MK_REQUIRE(plan.replication_evidence_ready);
    MK_REQUIRE(!plan.general_online_ready);
    MK_REQUIRE(plan.session_lifecycle_rows == 4U);
    MK_REQUIRE(plan.connection_state_rows == 3U);
    MK_REQUIRE(plan.channel_policy_rows == 2U);
    MK_REQUIRE(plan.reliable_delivery_rows == 2U);
    MK_REQUIRE(plan.unreliable_delivery_rows == 1U);
    MK_REQUIRE(plan.sequence_replay_rejection_rows == 1U);
    MK_REQUIRE(plan.input_command_validation_rows == 1U);
    MK_REQUIRE(plan.snapshot_validation_rows == 1U);
    MK_REQUIRE(plan.rollback_window_diagnostic_rows == 1U);
    MK_REQUIRE(plan.unsupported_online_claim_rows == 0U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_external_network_io);
    MK_REQUIRE(!plan.invoked_threads);
    MK_REQUIRE(!plan.invoked_save_io);
    MK_REQUIRE(!plan.invoked_world_mutation);
}

MK_TEST("runtime network production security gate separates missing loopback host evidence") {
    auto request = make_security_request();
    request.loopback_exchange = mirakana::runtime::RuntimeNetworkLoopbackExchangeResult{};
    request.replication_plan = mirakana::runtime::plan_runtime_network_replication(make_replication_request(false));

    const auto plan = mirakana::runtime::plan_runtime_network_production_security_gate(request);

    MK_REQUIRE(plan.status == RuntimeNetworkProductionSecurityStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.threat_model_reviewed);
    MK_REQUIRE(!plan.loopback_host_evidence);
    MK_REQUIRE(!plan.replication_evidence_ready);
    MK_REQUIRE(plan.session_lifecycle_rows == 1U);
    MK_REQUIRE(plan.connection_state_rows == 1U);
    MK_REQUIRE(plan.channel_policy_rows == 2U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_loopback_host_evidence) ==
               1U);
    MK_REQUIRE(diagnostic_count(
                   plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_replication_host_evidence) == 1U);
    MK_REQUIRE(!plan.general_online_ready);
}

MK_TEST("runtime network production security gate rejects unsupported online service claims") {
    using ClaimKind = mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimKind;

    auto request = make_security_request();
    request.unsupported_online_claim_rows = {
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::encryption_execution,
            .claim_id = "tls-over-enet",
            .requested = true,
            .source_index = 30U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::peer_authentication,
            .claim_id = "account-auth",
            .requested = true,
            .source_index = 31U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::matchmaking,
            .claim_id = "matchmaker",
            .requested = true,
            .source_index = 32U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::nat_traversal,
            .claim_id = "nat-punch",
            .requested = true,
            .source_index = 33U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::cloud_service,
            .claim_id = "cloud-relay",
            .requested = true,
            .source_index = 34U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::internet_remote_execution,
            .claim_id = "internet-ready",
            .requested = true,
            .source_index = 35U,
        },
        mirakana::runtime::RuntimeNetworkUnsupportedOnlineClaimRow{
            .kind = ClaimKind::broad_multiplayer_readiness,
            .claim_id = "commercial-online-ready",
            .requested = true,
            .source_index = 36U,
        },
    };

    const auto plan = mirakana::runtime::plan_runtime_network_production_security_gate(request);

    MK_REQUIRE(plan.status == RuntimeNetworkProductionSecurityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.unsupported_online_claim_rows == 7U);
    MK_REQUIRE(
        diagnostic_count(plan, RuntimeNetworkProductionSecurityDiagnosticCode::unsupported_online_service_claim) == 7U);
    MK_REQUIRE(!plan.general_online_ready);
    MK_REQUIRE(plan.evidence_rows.empty());
}

MK_TEST("runtime network production security gate rejects incomplete threat model validation gaps and side effects") {
    auto request = make_security_request();
    request.threat_model.packet_replay_reviewed = false;
    request.validation_evidence_rows.pop_back();
    request.request_native_handles = true;
    request.invoked_external_network_io = true;
    request.invoked_threads = true;
    request.invoked_save_io = true;
    request.invoked_world_mutation = true;

    const auto plan = mirakana::runtime::plan_runtime_network_production_security_gate(request);

    MK_REQUIRE(plan.status == RuntimeNetworkProductionSecurityStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(!plan.threat_model_reviewed);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkProductionSecurityDiagnosticCode::incomplete_threat_model) == 1U);
    MK_REQUIRE(diagnostic_count(
                   plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_rollback_window_diagnostic) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkProductionSecurityDiagnosticCode::native_handle_exposure) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RuntimeNetworkProductionSecurityDiagnosticCode::side_effect_claim) == 4U);
    MK_REQUIRE(plan.evidence_rows.empty());
}

int main() {
    return mirakana::test::run_all();
}
