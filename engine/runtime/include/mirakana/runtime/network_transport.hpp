// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

inline constexpr std::size_t runtime_network_transport_max_payload_bytes = 16U * 1024U;
inline constexpr std::uint32_t runtime_network_transport_max_channels = 32U;
inline constexpr std::uint32_t runtime_network_transport_max_peers = 64U;
inline constexpr std::uint32_t runtime_network_transport_loopback_peer_count = 1U;
inline constexpr std::uint32_t runtime_network_transport_max_service_iterations = 4096U;

enum class RuntimeNetworkTransportDelivery : std::uint8_t {
    reliable_ordered = 0,
    unreliable_unordered,
};

enum class RuntimeNetworkTransportAdapterStatus : std::uint8_t {
    completed = 0,
    unavailable,
    invalid_request,
    adapter_failed,
};

enum class RuntimeNetworkTransportDiagnosticCode : std::uint8_t {
    missing_adapter = 0,
    adapter_unavailable,
    loopback_exchange_unsupported,
    reliable_delivery_unsupported,
    unreliable_delivery_unsupported,
    native_handles_requested,
    invalid_peer_count,
    invalid_channel_count,
    invalid_channel,
    invalid_payload,
    invalid_service_budget,
    adapter_exception,
};

struct RuntimeNetworkTransportAdapterCapabilities {
    std::string adapter_id;
    bool available{false};
    bool supports_loopback_exchange{false};
    bool supports_reliable_ordered{false};
    bool supports_unreliable_unordered{false};
    bool exposes_native_handles{false};
    std::uint32_t max_peers{0U};
    std::uint32_t max_channels{0U};
    std::size_t max_payload_bytes{0U};
    std::uint32_t max_service_iterations{0U};
};

struct RuntimeNetworkPacketDesc {
    std::uint8_t channel_id{0U};
    RuntimeNetworkTransportDelivery delivery{RuntimeNetworkTransportDelivery::reliable_ordered};
    std::vector<std::byte> payload;
    // Caller-owned source row index for diagnostics; this is not a peer id.
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkLoopbackExchangeRequest {
    std::vector<RuntimeNetworkPacketDesc> client_to_server_packets;
    std::vector<RuntimeNetworkPacketDesc> server_to_client_packets;
    // v1 loopback exchanges support exactly one client peer and fail closed for larger peer counts.
    std::uint32_t peer_count{1U};
    std::uint32_t channel_count{1U};
    std::uint16_t port{0U};
    std::uint32_t service_timeout_ms{0U};
    std::uint32_t max_service_iterations{0U};
    bool request_native_handles{false};
};

struct RuntimeNetworkTransportDiagnostic {
    RuntimeNetworkTransportDiagnosticCode code{RuntimeNetworkTransportDiagnosticCode::missing_adapter};
    std::string adapter_id;
    std::uint32_t source_index{0U};
    std::uint8_t channel_id{0U};
    std::string message;
};

struct RuntimeNetworkReceivedPacketRow {
    std::uint8_t channel_id{0U};
    RuntimeNetworkTransportDelivery delivery{RuntimeNetworkTransportDelivery::reliable_ordered};
    std::vector<std::byte> payload;
    std::uint32_t remote_peer_index{0U};
};

struct RuntimeNetworkLoopbackExchangeResult {
    RuntimeNetworkTransportAdapterStatus status{RuntimeNetworkTransportAdapterStatus::invalid_request};
    std::string adapter_id;
    std::vector<RuntimeNetworkTransportDiagnostic> diagnostics;
    std::vector<RuntimeNetworkReceivedPacketRow> server_received_packets;
    std::vector<RuntimeNetworkReceivedPacketRow> client_received_packets;
    std::size_t packets_sent{0U};
    std::size_t packets_received{0U};
    std::size_t bytes_sent{0U};
    std::size_t bytes_received{0U};
    bool native_handles_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

class IRuntimeNetworkTransportAdapter {
  public:
    IRuntimeNetworkTransportAdapter() = default;
    IRuntimeNetworkTransportAdapter(const IRuntimeNetworkTransportAdapter&) = delete;
    IRuntimeNetworkTransportAdapter& operator=(const IRuntimeNetworkTransportAdapter&) = delete;
    IRuntimeNetworkTransportAdapter(IRuntimeNetworkTransportAdapter&&) = delete;
    IRuntimeNetworkTransportAdapter& operator=(IRuntimeNetworkTransportAdapter&&) = delete;
    virtual ~IRuntimeNetworkTransportAdapter() = default;

    [[nodiscard]] virtual RuntimeNetworkTransportAdapterCapabilities capabilities() const = 0;
    [[nodiscard]] virtual RuntimeNetworkLoopbackExchangeResult
    execute_loopback_exchange(const RuntimeNetworkLoopbackExchangeRequest& request) = 0;
};

/// Executes one reviewed loopback-only exchange through a caller-owned adapter.
/// The first-party facade validates payload, channel, delivery, service-budget, and native-handle policy before
/// dispatch. It does not expose sockets, middleware handles, external network readiness, encryption/authentication,
/// matchmaking, NAT traversal, replication, rollback, or game-state mutation.
[[nodiscard]] RuntimeNetworkLoopbackExchangeResult
execute_runtime_network_loopback_exchange(const RuntimeNetworkLoopbackExchangeRequest& request,
                                          IRuntimeNetworkTransportAdapter* adapter);

} // namespace mirakana::runtime
