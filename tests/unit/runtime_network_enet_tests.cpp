// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/network/enet/enet_network_transport_adapter.hpp"
#include "mirakana/runtime/network_production_security.hpp"
#include "mirakana/runtime/network_transport.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/production_network_replication.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::vector<std::byte> bytes(std::string_view text) {
    std::vector<std::byte> payload;
    payload.reserve(text.size());
    for (const char ch : text) {
        payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return payload;
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkPacketDesc
make_packet(std::uint8_t channel_id, mirakana::runtime::RuntimeNetworkTransportDelivery delivery,
            std::string_view payload, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkPacketDesc{
        .channel_id = channel_id,
        .delivery = delivery,
        .payload = bytes(payload),
        .source_index = source_index,
    };
}

} // namespace

MK_TEST("enet network transport adapter reports first party capabilities without native handles") {
    const auto adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto capabilities = adapter->capabilities();

    MK_REQUIRE(capabilities.adapter_id == "enet");
    MK_REQUIRE(capabilities.available);
    MK_REQUIRE(capabilities.supports_loopback_exchange);
    MK_REQUIRE(capabilities.supports_reliable_ordered);
    MK_REQUIRE(capabilities.supports_unreliable_unordered);
    MK_REQUIRE(!capabilities.exposes_native_handles);
    MK_REQUIRE(capabilities.max_peers >= 1U);
    MK_REQUIRE(capabilities.max_channels >= 2U);
    MK_REQUIRE(capabilities.max_payload_bytes >= bytes("client-ping").size());
}

MK_TEST("enet network transport adapter runs selected loopback exchange through facade") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;

    auto adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client-ping", 1U)},
        .server_to_client_packets = {make_packet(0U, Delivery::reliable_ordered, "server-pong", 2U)},
        .peer_count = 1U,
        .channel_count = 2U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 128U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, adapter.get());

    MK_REQUIRE(result.status == Status::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.adapter_id == "enet");
    MK_REQUIRE(result.packets_sent == 2U);
    MK_REQUIRE(result.packets_received == 2U);
    MK_REQUIRE(result.server_received_packets.size() == 1U);
    MK_REQUIRE(result.server_received_packets[0].payload == bytes("client-ping"));
    MK_REQUIRE(result.server_received_packets[0].remote_peer_index == 0U);
    MK_REQUIRE(result.client_received_packets.size() == 1U);
    MK_REQUIRE(result.client_received_packets[0].payload == bytes("server-pong"));
    MK_REQUIRE(result.client_received_packets[0].remote_peer_index == 0U);
    MK_REQUIRE(!result.native_handles_exposed);
}

MK_TEST("enet network transport adapter preserves unreliable packet metadata") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;

    auto adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(1U, Delivery::unreliable_unordered, "client-unreliable", 4U)},
        .server_to_client_packets = {make_packet(1U, Delivery::unreliable_unordered, "server-unreliable", 5U)},
        .peer_count = 1U,
        .channel_count = 2U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 128U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, adapter.get());

    MK_REQUIRE(result.status == Status::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.server_received_packets.size() == 1U);
    MK_REQUIRE(result.server_received_packets[0].channel_id == 1U);
    MK_REQUIRE(result.server_received_packets[0].delivery == Delivery::unreliable_unordered);
    MK_REQUIRE(result.server_received_packets[0].payload == bytes("client-unreliable"));
    MK_REQUIRE(result.client_received_packets.size() == 1U);
    MK_REQUIRE(result.client_received_packets[0].channel_id == 1U);
    MK_REQUIRE(result.client_received_packets[0].delivery == Delivery::unreliable_unordered);
    MK_REQUIRE(result.client_received_packets[0].payload == bytes("server-unreliable"));
    MK_REQUIRE(!result.native_handles_exposed);
}

MK_TEST("enet network transport adapter supplies production security loopback host evidence") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Kind = mirakana::runtime::RuntimeNetworkProductionValidationKind;
    using Status = mirakana::runtime::RuntimeNetworkProductionSecurityStatus;

    auto adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets =
            {
                make_packet(0U, Delivery::reliable_ordered, "client-reliable", 10U),
                make_packet(1U, Delivery::unreliable_unordered, "client-unreliable", 11U),
            },
        .server_to_client_packets = {make_packet(0U, Delivery::reliable_ordered, "server-reliable", 12U)},
        .peer_count = 1U,
        .channel_count = 2U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 128U,
        .request_native_handles = false,
    };

    const auto loopback = mirakana::runtime::execute_runtime_network_loopback_exchange(request, adapter.get());
    const auto
        security_plan =
            mirakana::runtime::plan_runtime_network_production_security_gate(
                mirakana::runtime::RuntimeNetworkProductionSecurityRequest{
                    .threat_model =
                        mirakana::runtime::RuntimeNetworkProductionThreatModelEvidenceRow{
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
                        },
                    .foundation_plan =
                        mirakana::runtime::RuntimeNetworkFoundationPlan{
                            .status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus::planned,
                            .diagnostics = {},
                            .sessions =
                                {
                                    mirakana::runtime::RuntimeNetworkSessionPlanRow{
                                        .session_id = "arena",
                                        .topology = mirakana::runtime::RuntimeNetworkSessionTopology::listen_server,
                                        .local_role = mirakana::runtime::RuntimeNetworkLocalRole::host,
                                        .trust_boundary =
                                            mirakana::runtime::RuntimeNetworkTrustBoundary::untrusted_remote_peers,
                                        .transport_requirement_count = 4U,
                                        .replication_channel_count = 2U,
                                        .remote_session = true,
                                        .secure_remote_session = true,
                                        .source_index = 2U,
                                    },
                                },
                            .transports = {},
                            .channels = {},
                            .replay_prerequisites = {},
                            .remote_session_count = 1U,
                            .secure_remote_session_count = 1U,
                        },
                    .replication_plan =
                        mirakana::runtime::RuntimeNetworkReplicationPlan{
                            .status = mirakana::runtime::RuntimeNetworkReplicationStatus::ready,
                            .diagnostics = {},
                            .session_rows =
                                {
                                    mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
                                        .session_id = "arena",
                                        .world_id = "world.arena",
                                        .mode = mirakana::runtime::RuntimeNetworkReplicationMode::
                                            authoritative_snapshot,
                                        .fixed_tick_rate_hz = 60U,
                                        .max_players = 4U,
                                        .max_objects = 16U,
                                        .source_index = 3U,
                                    },
                                },
                            .object_rows = {},
                            .input_rows = {},
                            .snapshot_rows = {},
                            .sandbox_mutation_command_rows = {},
                            .sandbox_snapshot_delta_rows = {},
                            .rollback_rows = {},
                            .transport_evidence_rows =
                                {
                                    mirakana::runtime::RuntimeNetworkTransportEvidenceRow{
                                        .adapter_id = "enet_loopback",
                                        .loopback_validated = true,
                                        .reliable_validated = true,
                                        .unreliable_validated = true,
                                        .source_index = 4U,
                                    },
                                },
                            .replicated_object_count = 0U,
                            .input_row_count = 0U,
                            .snapshot_row_count = 0U,
                            .sandbox_mutation_command_count = 0U,
                            .sandbox_snapshot_delta_count = 0U,
                            .rollback_row_count = 0U,
                            .rejected_unsafe_row_count = 0U,
                            .replay_hash = 99U,
                            .requires_transport_host_evidence = true,
                            .has_transport_host_evidence = true,
                            .invoked_network_io = false,
                            .invoked_rollback_execution = false,
                            .invoked_world_mutation = false,
                        },
                    .loopback_exchange = loopback,
                    .validation_evidence_rows =
                        {
                            mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                                .kind = Kind::sequence_replay_rejection,
                                .evidence_id = "sequence-replay",
                                .reviewed = true,
                                .source_index = 5U,
                            },
                            mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                                .kind = Kind::input_command_validation,
                                .evidence_id = "input-command",
                                .reviewed = true,
                                .source_index = 6U,
                            },
                            mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                                .kind = Kind::snapshot_validation,
                                .evidence_id = "snapshot-validation",
                                .reviewed = true,
                                .source_index = 7U,
                            },
                            mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                                .kind = Kind::rollback_window_diagnostic,
                                .evidence_id = "rollback-window",
                                .reviewed = true,
                                .source_index = 8U,
                            },
                        },
                    .unsupported_online_claim_rows = {},
                    .row_budget = 64U,
                    .request_native_handles = false,
                    .invoked_external_network_io = false,
                    .invoked_threads = false,
                    .invoked_save_io = false,
                    .invoked_world_mutation = false,
                    .seed = 123U,
                });

    MK_REQUIRE(loopback.succeeded());
    MK_REQUIRE(security_plan.status == Status::ready);
    MK_REQUIRE(security_plan.succeeded());
    MK_REQUIRE(security_plan.loopback_host_evidence);
    MK_REQUIRE(security_plan.reliable_delivery_rows == 2U);
    MK_REQUIRE(security_plan.unreliable_delivery_rows == 1U);
    MK_REQUIRE(!security_plan.general_online_ready);
}

MK_TEST("enet network transport adapter remains behind native handle preflight") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    auto adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client-ping", 3U)},
        .server_to_client_packets = {},
        .peer_count = 1U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = true,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, adapter.get());

    MK_REQUIRE(result.status == Status::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(result.diagnostics.size() == 1U);
    MK_REQUIRE(result.diagnostics[0].code == Code::native_handles_requested);
}

int main() {
    return mirakana::test::run_all();
}
