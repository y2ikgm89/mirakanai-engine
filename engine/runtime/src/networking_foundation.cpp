// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/networking_foundation.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
}

[[nodiscard]] bool contains_capability(const std::vector<RuntimeNetworkTransportCapabilityKind>& capabilities,
                                       RuntimeNetworkTransportCapabilityKind capability) {
    return std::ranges::any_of(capabilities, [capability](RuntimeNetworkTransportCapabilityKind candidate) {
        return candidate == capability;
    });
}

[[nodiscard]] bool is_remote_topology(RuntimeNetworkSessionTopology topology) noexcept {
    switch (topology) {
    case RuntimeNetworkSessionTopology::local_only:
        return false;
    case RuntimeNetworkSessionTopology::listen_server:
    case RuntimeNetworkSessionTopology::dedicated_server:
    case RuntimeNetworkSessionTopology::peer_to_peer:
        return true;
    }
    return true;
}

[[nodiscard]] bool is_native_transport_capability(RuntimeNetworkTransportCapabilityKind capability) noexcept {
    switch (capability) {
    case RuntimeNetworkTransportCapabilityKind::platform_socket:
    case RuntimeNetworkTransportCapabilityKind::native_transport_handle:
        return true;
    case RuntimeNetworkTransportCapabilityKind::reliable_ordered:
    case RuntimeNetworkTransportCapabilityKind::unreliable_unordered:
    case RuntimeNetworkTransportCapabilityKind::encrypted_transport:
    case RuntimeNetworkTransportCapabilityKind::authenticated_peer:
    case RuntimeNetworkTransportCapabilityKind::nat_traversal:
        return false;
    }
    return true;
}

[[nodiscard]] bool requires_remote_security(const RuntimeNetworkSessionDesc& session) noexcept {
    return is_remote_topology(session.topology) &&
           session.trust_boundary == RuntimeNetworkTrustBoundary::untrusted_remote_peers;
}

[[nodiscard]] bool has_transport(const RuntimeNetworkSessionDesc& session,
                                 RuntimeNetworkTransportCapabilityKind capability) {
    return std::ranges::any_of(session.transports, [capability](const RuntimeNetworkTransportRequirementDesc& row) {
        return row.capability == capability;
    });
}

[[nodiscard]] bool has_secure_remote_transports(const RuntimeNetworkSessionDesc& session) {
    return has_transport(session, RuntimeNetworkTransportCapabilityKind::encrypted_transport) &&
           has_transport(session, RuntimeNetworkTransportCapabilityKind::authenticated_peer);
}

void add_diagnostic(RuntimeNetworkFoundationPlan& plan, RuntimeNetworkDiagnosticCode code, std::string session_id,
                    std::string channel_id, RuntimeNetworkTransportCapabilityKind capability, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeNetworkDiagnostic{
        .code = code,
        .session_id = std::move(session_id),
        .channel_id = std::move(channel_id),
        .capability = capability,
        .message = std::move(message),
        .source_index = source_index,
    });
}

void validate_transport(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkFoundationPolicyDesc& policy,
                        const RuntimeNetworkSessionDesc& session,
                        const RuntimeNetworkTransportRequirementDesc& transport) {
    if (is_native_transport_capability(transport.capability)) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::native_transport_capability_requested, session.session_id,
                       {}, transport.capability,
                       "networking foundation does not expose platform sockets or native transport handles",
                       transport.source_index);
        return;
    }

    if (!contains_capability(policy.reviewed_transport_capabilities, transport.capability)) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::unsupported_transport_capability, session.session_id, {},
                       transport.capability, "networking transport capability is not in the reviewed capability list",
                       transport.source_index);
    }
}

void validate_remote_security(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkSessionDesc& session) {
    if (!requires_remote_security(session)) {
        return;
    }

    if (!has_transport(session, RuntimeNetworkTransportCapabilityKind::encrypted_transport)) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::insecure_remote_transport, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::encrypted_transport,
                       "untrusted remote sessions require an explicit encrypted_transport capability",
                       session.source_index);
    }
    if (!has_transport(session, RuntimeNetworkTransportCapabilityKind::authenticated_peer)) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::insecure_remote_transport, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::authenticated_peer,
                       "untrusted remote sessions require an explicit authenticated_peer capability",
                       session.source_index);
    }
}

void validate_channels(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkSessionDesc& session) {
    std::vector<std::string> channel_ids;
    channel_ids.reserve(session.channels.size());

    for (const auto& channel : session.channels) {
        if (!is_valid_id(channel.channel_id)) {
            add_diagnostic(plan, RuntimeNetworkDiagnosticCode::missing_channel_id, session.session_id,
                           channel.channel_id, RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                           "network replication channel id must be non-empty and path-safe", channel.source_index);
        } else if (contains_id(channel_ids, channel.channel_id)) {
            add_diagnostic(plan, RuntimeNetworkDiagnosticCode::duplicate_channel_id, session.session_id,
                           channel.channel_id, RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                           "network replication channel ids must be unique per session", channel.source_index);
        } else {
            channel_ids.push_back(channel.channel_id);
        }

        if (channel.tick_rate_hz == 0U) {
            add_diagnostic(plan, RuntimeNetworkDiagnosticCode::invalid_channel_tick_rate, session.session_id,
                           channel.channel_id, RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                           "network replication channel tick rate must be non-zero", channel.source_index);
        }
    }
}

void validate_replay(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkSessionDesc& session) {
    if (session.replay.replay_seed == 0U) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::missing_replay_seed, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                       "network replay prerequisites require a non-zero replay seed", session.replay.source_index);
    }
    if (session.replay.fixed_tick_rate_hz == 0U) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::invalid_replay_tick_rate, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                       "network replay prerequisites require a non-zero fixed tick rate", session.replay.source_index);
    }
    if (!session.replay.deterministic_simulation) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::missing_deterministic_simulation, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                       "network replay prerequisites require deterministic simulation acknowledgement",
                       session.replay.source_index);
    }
    if (!session.replay.ordered_inputs) {
        add_diagnostic(plan, RuntimeNetworkDiagnosticCode::missing_ordered_inputs, session.session_id, {},
                       RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                       "network replay prerequisites require ordered input acknowledgement",
                       session.replay.source_index);
    }
}

void validate_policy(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkFoundationPolicyDesc& policy) {
    std::vector<std::string> session_ids;
    session_ids.reserve(policy.sessions.size());

    for (const auto& session : policy.sessions) {
        if (!is_valid_id(session.session_id)) {
            add_diagnostic(plan, RuntimeNetworkDiagnosticCode::missing_session_id, session.session_id, {},
                           RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                           "network session id must be non-empty and path-safe", session.source_index);
        } else if (contains_id(session_ids, session.session_id)) {
            add_diagnostic(plan, RuntimeNetworkDiagnosticCode::duplicate_session_id, session.session_id, {},
                           RuntimeNetworkTransportCapabilityKind::reliable_ordered,
                           "network session ids must be unique", session.source_index);
        } else {
            session_ids.push_back(session.session_id);
        }

        validate_remote_security(plan, session);
        for (const auto& transport : session.transports) {
            validate_transport(plan, policy, session, transport);
        }
        validate_channels(plan, session);
        validate_replay(plan, session);
    }
}

void append_rows(RuntimeNetworkFoundationPlan& plan, const RuntimeNetworkSessionDesc& session) {
    const bool remote_session = is_remote_topology(session.topology);
    const bool secure_remote_session = remote_session && has_secure_remote_transports(session);

    if (remote_session) {
        ++plan.remote_session_count;
    }
    if (secure_remote_session) {
        ++plan.secure_remote_session_count;
    }

    plan.sessions.push_back(RuntimeNetworkSessionPlanRow{
        .session_id = session.session_id,
        .topology = session.topology,
        .local_role = session.local_role,
        .trust_boundary = session.trust_boundary,
        .transport_requirement_count = session.transports.size(),
        .replication_channel_count = session.channels.size(),
        .remote_session = remote_session,
        .secure_remote_session = secure_remote_session,
        .source_index = session.source_index,
    });

    for (const auto& transport : session.transports) {
        plan.transports.push_back(RuntimeNetworkTransportRequirementRow{
            .session_id = session.session_id,
            .capability = transport.capability,
            .source_index = transport.source_index,
        });
    }

    for (const auto& channel : session.channels) {
        plan.channels.push_back(RuntimeNetworkReplicationChannelRow{
            .session_id = session.session_id,
            .channel_id = channel.channel_id,
            .authority = channel.authority,
            .delivery = channel.delivery,
            .tick_rate_hz = channel.tick_rate_hz,
            .source_index = channel.source_index,
        });
    }

    plan.replay_prerequisites.push_back(RuntimeNetworkReplayPrerequisiteRow{
        .session_id = session.session_id,
        .replay_seed = session.replay.replay_seed,
        .fixed_tick_rate_hz = session.replay.fixed_tick_rate_hz,
        .deterministic_simulation = session.replay.deterministic_simulation,
        .ordered_inputs = session.replay.ordered_inputs,
        .source_index = session.replay.source_index,
    });
}

void sort_rows(RuntimeNetworkFoundationPlan& plan) {
    std::ranges::sort(plan.sessions, [](const auto& lhs, const auto& rhs) {
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        return lhs.source_index < rhs.source_index;
    });

    std::ranges::sort(plan.transports, [](const auto& lhs, const auto& rhs) {
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return static_cast<std::uint8_t>(lhs.capability) < static_cast<std::uint8_t>(rhs.capability);
    });

    std::ranges::sort(plan.channels, [](const auto& lhs, const auto& rhs) {
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        if (lhs.channel_id != rhs.channel_id) {
            return lhs.channel_id < rhs.channel_id;
        }
        return lhs.source_index < rhs.source_index;
    });

    std::ranges::sort(plan.replay_prerequisites, [](const auto& lhs, const auto& rhs) {
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(RuntimeNetworkFoundationPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.channel_id != rhs.channel_id) {
            return lhs.channel_id < rhs.channel_id;
        }
        if (lhs.capability != rhs.capability) {
            return static_cast<std::uint8_t>(lhs.capability) < static_cast<std::uint8_t>(rhs.capability);
        }
        return lhs.source_index < rhs.source_index;
    });
}

} // namespace

bool RuntimeNetworkFoundationPlan::succeeded() const noexcept {
    return status == RuntimeNetworkFoundationPlanStatus::planned ||
           status == RuntimeNetworkFoundationPlanStatus::no_sessions;
}

RuntimeNetworkFoundationPlan plan_runtime_network_foundation(const RuntimeNetworkFoundationPolicyDesc& policy) {
    RuntimeNetworkFoundationPlan plan;

    validate_policy(plan, policy);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeNetworkFoundationPlanStatus::invalid_request;
        return plan;
    }

    for (const auto& session : policy.sessions) {
        append_rows(plan, session);
    }
    sort_rows(plan);

    plan.status = policy.sessions.empty() ? RuntimeNetworkFoundationPlanStatus::no_sessions
                                          : RuntimeNetworkFoundationPlanStatus::planned;
    return plan;
}

} // namespace mirakana::runtime
