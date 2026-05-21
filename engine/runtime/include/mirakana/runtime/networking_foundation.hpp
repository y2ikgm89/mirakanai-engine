// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeNetworkSessionTopology : std::uint8_t {
    local_only = 0,
    listen_server,
    dedicated_server,
    peer_to_peer,
};

enum class RuntimeNetworkLocalRole : std::uint8_t {
    offline = 0,
    host,
    server,
    client,
    peer,
};

enum class RuntimeNetworkTrustBoundary : std::uint8_t {
    trusted_local = 0,
    server_authoritative,
    untrusted_remote_peers,
};

enum class RuntimeNetworkTransportCapabilityKind : std::uint8_t {
    reliable_ordered = 0,
    unreliable_unordered,
    encrypted_transport,
    authenticated_peer,
    nat_traversal,
    platform_socket,
    native_transport_handle,
};

enum class RuntimeNetworkReplicationAuthority : std::uint8_t {
    host = 0,
    server,
    client,
    peer,
};

enum class RuntimeNetworkReplicationDelivery : std::uint8_t {
    reliable_ordered = 0,
    unreliable_unordered,
    state_snapshot,
};

enum class RuntimeNetworkFoundationPlanStatus : std::uint8_t {
    planned = 0,
    no_sessions,
    invalid_request,
};

enum class RuntimeNetworkDiagnosticCode : std::uint8_t {
    missing_session_id = 0,
    duplicate_session_id,
    unsupported_transport_capability,
    native_transport_capability_requested,
    insecure_remote_transport,
    missing_channel_id,
    duplicate_channel_id,
    invalid_channel_tick_rate,
    missing_replay_seed,
    invalid_replay_tick_rate,
    missing_deterministic_simulation,
    missing_ordered_inputs,
};

struct RuntimeNetworkTransportRequirementDesc {
    RuntimeNetworkTransportCapabilityKind capability{RuntimeNetworkTransportCapabilityKind::reliable_ordered};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplicationChannelDesc {
    std::string channel_id;
    RuntimeNetworkReplicationAuthority authority{RuntimeNetworkReplicationAuthority::server};
    RuntimeNetworkReplicationDelivery delivery{RuntimeNetworkReplicationDelivery::state_snapshot};
    std::uint32_t tick_rate_hz{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplayPrerequisiteDesc {
    std::uint64_t replay_seed{0U};
    std::uint32_t fixed_tick_rate_hz{0U};
    bool deterministic_simulation{false};
    bool ordered_inputs{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkSessionDesc {
    std::string session_id;
    RuntimeNetworkSessionTopology topology{RuntimeNetworkSessionTopology::local_only};
    RuntimeNetworkLocalRole local_role{RuntimeNetworkLocalRole::offline};
    RuntimeNetworkTrustBoundary trust_boundary{RuntimeNetworkTrustBoundary::trusted_local};
    std::vector<RuntimeNetworkTransportRequirementDesc> transports;
    std::vector<RuntimeNetworkReplicationChannelDesc> channels;
    RuntimeNetworkReplayPrerequisiteDesc replay;
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkFoundationPolicyDesc {
    std::vector<RuntimeNetworkSessionDesc> sessions;
    std::vector<RuntimeNetworkTransportCapabilityKind> reviewed_transport_capabilities;
};

struct RuntimeNetworkDiagnostic {
    RuntimeNetworkDiagnosticCode code{RuntimeNetworkDiagnosticCode::missing_session_id};
    std::string session_id;
    std::string channel_id;
    RuntimeNetworkTransportCapabilityKind capability{RuntimeNetworkTransportCapabilityKind::reliable_ordered};
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkSessionPlanRow {
    std::string session_id;
    RuntimeNetworkSessionTopology topology{RuntimeNetworkSessionTopology::local_only};
    RuntimeNetworkLocalRole local_role{RuntimeNetworkLocalRole::offline};
    RuntimeNetworkTrustBoundary trust_boundary{RuntimeNetworkTrustBoundary::trusted_local};
    std::size_t transport_requirement_count{0U};
    std::size_t replication_channel_count{0U};
    bool remote_session{false};
    bool secure_remote_session{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkTransportRequirementRow {
    std::string session_id;
    RuntimeNetworkTransportCapabilityKind capability{RuntimeNetworkTransportCapabilityKind::reliable_ordered};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplicationChannelRow {
    std::string session_id;
    std::string channel_id;
    RuntimeNetworkReplicationAuthority authority{RuntimeNetworkReplicationAuthority::server};
    RuntimeNetworkReplicationDelivery delivery{RuntimeNetworkReplicationDelivery::state_snapshot};
    std::uint32_t tick_rate_hz{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkReplayPrerequisiteRow {
    std::string session_id;
    std::uint64_t replay_seed{0U};
    std::uint32_t fixed_tick_rate_hz{0U};
    bool deterministic_simulation{false};
    bool ordered_inputs{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkFoundationPlan {
    RuntimeNetworkFoundationPlanStatus status{RuntimeNetworkFoundationPlanStatus::invalid_request};
    std::vector<RuntimeNetworkDiagnostic> diagnostics;
    std::vector<RuntimeNetworkSessionPlanRow> sessions;
    std::vector<RuntimeNetworkTransportRequirementRow> transports;
    std::vector<RuntimeNetworkReplicationChannelRow> channels;
    std::vector<RuntimeNetworkReplayPrerequisiteRow> replay_prerequisites;
    std::size_t remote_session_count{0U};
    std::size_t secure_remote_session_count{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans deterministic, value-only networking session/trust/transport/replication policy rows for generated gameplay.
/// This helper does not open sockets, create threads, perform network I/O, perform encryption/authentication
/// handshakes, expose native handles, run prediction/rollback, or add transport middleware.
[[nodiscard]] RuntimeNetworkFoundationPlan
plan_runtime_network_foundation(const RuntimeNetworkFoundationPolicyDesc& policy);

} // namespace mirakana::runtime
