// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/networking_foundation.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeNetworkTransportRequirementDesc
make_transport(mirakana::runtime::RuntimeNetworkTransportCapabilityKind capability, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
        .capability = capability,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkReplicationChannelDesc
make_channel(std::string channel_id, mirakana::runtime::RuntimeNetworkReplicationAuthority authority,
             mirakana::runtime::RuntimeNetworkReplicationDelivery delivery, std::uint32_t tick_rate_hz,
             std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
        .channel_id = std::move(channel_id),
        .authority = authority,
        .delivery = delivery,
        .tick_rate_hz = tick_rate_hz,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkSessionDesc
make_session(std::string session_id, mirakana::runtime::RuntimeNetworkSessionTopology topology,
             mirakana::runtime::RuntimeNetworkLocalRole local_role,
             mirakana::runtime::RuntimeNetworkTrustBoundary trust_boundary,
             std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc> transports,
             std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc> channels,
             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc replay, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkSessionDesc{
        .session_id = std::move(session_id),
        .topology = topology,
        .local_role = local_role,
        .trust_boundary = trust_boundary,
        .transports = std::move(transports),
        .channels = std::move(channels),
        .replay = replay,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeNetworkFoundationPlan& plan,
                                           mirakana::runtime::RuntimeNetworkDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime networking foundation plans deterministic reviewed session rows") {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    const auto policy = mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
        .sessions =
            {
                make_session("arena", Topology::listen_server, Role::host, Trust::untrusted_remote_peers,
                             {
                                 make_transport(Capability::reliable_ordered, 3U),
                                 make_transport(Capability::encrypted_transport, 2U),
                                 make_transport(Capability::authenticated_peer, 1U),
                             },
                             {
                                 make_channel("state", Authority::server, Delivery::state_snapshot, 30U, 7U),
                                 make_channel("input", Authority::client, Delivery::unreliable_unordered, 60U, 6U),
                             },
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 42U,
                                 .fixed_tick_rate_hz = 60U,
                                 .deterministic_simulation = true,
                                 .ordered_inputs = true,
                                 .source_index = 9U,
                             },
                             4U),
                make_session("local", Topology::local_only, Role::offline, Trust::trusted_local,
                             {make_transport(Capability::reliable_ordered, 12U)},
                             {make_channel("solo_state", Authority::host, Delivery::reliable_ordered, 30U, 13U)},
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 7U,
                                 .fixed_tick_rate_hz = 30U,
                                 .deterministic_simulation = true,
                                 .ordered_inputs = true,
                                 .source_index = 14U,
                             },
                             10U),
            },
        .reviewed_transport_capabilities =
            {
                Capability::authenticated_peer,
                Capability::encrypted_transport,
                Capability::reliable_ordered,
                Capability::unreliable_unordered,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_network_foundation(policy);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.sessions.size() == 2U);
    MK_REQUIRE(plan.sessions[0].session_id == "arena");
    MK_REQUIRE(plan.sessions[0].topology == Topology::listen_server);
    MK_REQUIRE(plan.sessions[0].local_role == Role::host);
    MK_REQUIRE(plan.sessions[0].trust_boundary == Trust::untrusted_remote_peers);
    MK_REQUIRE(plan.sessions[0].transport_requirement_count == 3U);
    MK_REQUIRE(plan.sessions[0].replication_channel_count == 2U);
    MK_REQUIRE(plan.sessions[1].session_id == "local");
    MK_REQUIRE(plan.transports.size() == 4U);
    MK_REQUIRE(plan.transports[0].session_id == "arena");
    MK_REQUIRE(plan.transports[0].capability == Capability::authenticated_peer);
    MK_REQUIRE(plan.transports[1].capability == Capability::encrypted_transport);
    MK_REQUIRE(plan.transports[2].capability == Capability::reliable_ordered);
    MK_REQUIRE(plan.channels.size() == 3U);
    MK_REQUIRE(plan.channels[0].session_id == "arena");
    MK_REQUIRE(plan.channels[0].channel_id == "input");
    MK_REQUIRE(plan.channels[0].authority == Authority::client);
    MK_REQUIRE(plan.channels[1].channel_id == "state");
    MK_REQUIRE(plan.replay_prerequisites.size() == 2U);
    MK_REQUIRE(plan.remote_session_count == 1U);
    MK_REQUIRE(plan.secure_remote_session_count == 1U);
}

MK_TEST("runtime networking foundation rejects duplicate ids unsupported transports and unsafe remote defaults") {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Code = mirakana::runtime::RuntimeNetworkDiagnosticCode;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    const auto policy = mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
        .sessions =
            {
                make_session("", Topology::listen_server, Role::host, Trust::untrusted_remote_peers,
                             {make_transport(Capability::reliable_ordered, 1U)},
                             {make_channel("", Authority::server, Delivery::state_snapshot, 0U, 2U)},
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 0U,
                                 .fixed_tick_rate_hz = 0U,
                                 .deterministic_simulation = false,
                                 .ordered_inputs = false,
                                 .source_index = 3U,
                             },
                             4U),
                make_session("dupe", Topology::dedicated_server, Role::server, Trust::untrusted_remote_peers,
                             {
                                 make_transport(Capability::reliable_ordered, 5U),
                                 make_transport(Capability::nat_traversal, 6U),
                                 make_transport(Capability::native_transport_handle, 7U),
                             },
                             {
                                 make_channel("state", Authority::server, Delivery::state_snapshot, 60U, 8U),
                                 make_channel("state", Authority::server, Delivery::reliable_ordered, 60U, 9U),
                             },
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 10U,
                                 .fixed_tick_rate_hz = 60U,
                                 .deterministic_simulation = true,
                                 .ordered_inputs = true,
                                 .source_index = 11U,
                             },
                             10U),
                make_session("dupe", Topology::peer_to_peer, Role::peer, Trust::untrusted_remote_peers,
                             {make_transport(Capability::unreliable_unordered, 12U)},
                             {make_channel("input", Authority::client, Delivery::unreliable_unordered, 30U, 13U)},
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 12U,
                                 .fixed_tick_rate_hz = 30U,
                                 .deterministic_simulation = true,
                                 .ordered_inputs = true,
                                 .source_index = 14U,
                             },
                             15U),
            },
        .reviewed_transport_capabilities = {Capability::reliable_ordered, Capability::unreliable_unordered},
    };

    const auto plan = mirakana::runtime::plan_runtime_network_foundation(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.sessions.empty());
    MK_REQUIRE(plan.transports.empty());
    MK_REQUIRE(plan.channels.empty());
    MK_REQUIRE(plan.replay_prerequisites.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::missing_session_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_session_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_channel_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_channel_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unsupported_transport_capability) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::native_transport_capability_requested) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::insecure_remote_transport) == 6U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_replay_seed) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_replay_tick_rate) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_deterministic_simulation) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_ordered_inputs) == 1U);
}

MK_TEST("runtime networking foundation reports diagnostics in deterministic order") {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Code = mirakana::runtime::RuntimeNetworkDiagnosticCode;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    const auto policy = mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
        .sessions =
            {
                make_session("z_session", Topology::listen_server, Role::host, Trust::untrusted_remote_peers,
                             {make_transport(Capability::nat_traversal, 9U)},
                             {make_channel("z_channel", Authority::server, Delivery::state_snapshot, 0U, 8U)},
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 0U,
                                 .fixed_tick_rate_hz = 0U,
                                 .deterministic_simulation = false,
                                 .ordered_inputs = false,
                                 .source_index = 7U,
                             },
                             6U),
                make_session("a_session", Topology::listen_server, Role::host, Trust::untrusted_remote_peers,
                             {make_transport(Capability::reliable_ordered, 3U)},
                             {make_channel("a_channel", Authority::server, Delivery::state_snapshot, 0U, 2U)},
                             mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                 .replay_seed = 0U,
                                 .fixed_tick_rate_hz = 0U,
                                 .deterministic_simulation = false,
                                 .ordered_inputs = false,
                                 .source_index = 1U,
                             },
                             0U),
            },
        .reviewed_transport_capabilities = {Capability::reliable_ordered},
    };

    const auto plan = mirakana::runtime::plan_runtime_network_foundation(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(plan.diagnostics.size() == 15U);
    MK_REQUIRE(plan.diagnostics[0].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[0].code == Code::insecure_remote_transport);
    MK_REQUIRE(plan.diagnostics[1].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[1].code == Code::insecure_remote_transport);
    MK_REQUIRE(plan.diagnostics[2].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[2].channel_id == "a_channel");
    MK_REQUIRE(plan.diagnostics[2].code == Code::invalid_channel_tick_rate);
    MK_REQUIRE(plan.diagnostics[3].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[3].code == Code::missing_replay_seed);
    MK_REQUIRE(plan.diagnostics[4].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[4].code == Code::invalid_replay_tick_rate);
    MK_REQUIRE(plan.diagnostics[5].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[5].code == Code::missing_deterministic_simulation);
    MK_REQUIRE(plan.diagnostics[6].session_id == "a_session");
    MK_REQUIRE(plan.diagnostics[6].code == Code::missing_ordered_inputs);
    MK_REQUIRE(plan.diagnostics[7].session_id == "z_session");
    MK_REQUIRE(plan.diagnostics[7].code == Code::unsupported_transport_capability);
}

int main() {
    return mirakana::test::run_all();
}
