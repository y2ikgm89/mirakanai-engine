// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/network/enet/enet_network_transport_adapter.hpp"

#include <enet/enet.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

inline constexpr const char* loopback_host = "127.0.0.1";

class EnetLibraryLease {
  public:
    EnetLibraryLease() {
        const std::lock_guard lock{mutex()};
        if (ref_count() == 0U && enet_initialize() != 0) {
            throw std::runtime_error("ENet initialization failed");
        }
        ++ref_count();
    }

    EnetLibraryLease(const EnetLibraryLease&) = delete;
    EnetLibraryLease& operator=(const EnetLibraryLease&) = delete;
    EnetLibraryLease(EnetLibraryLease&&) = delete;
    EnetLibraryLease& operator=(EnetLibraryLease&&) = delete;

    ~EnetLibraryLease() {
        const std::lock_guard lock{mutex()};
        if (ref_count() > 0U) {
            --ref_count();
            if (ref_count() == 0U) {
                enet_deinitialize();
            }
        }
    }

  private:
    [[nodiscard]] static std::mutex& mutex() {
        static std::mutex value;
        return value;
    }

    [[nodiscard]] static std::size_t& ref_count() {
        static std::size_t value{0U};
        return value;
    }
};

struct HostDestroyer {
    void operator()(ENetHost* host) const noexcept {
        if (host != nullptr) {
            enet_host_destroy(host);
        }
    }
};

using HostPtr = std::unique_ptr<ENetHost, HostDestroyer>;

void add_diagnostic(RuntimeNetworkLoopbackExchangeResult& result, RuntimeNetworkTransportDiagnosticCode code,
                    std::string message, std::uint32_t source_index = 0U, std::uint8_t channel_id = 0U) {
    result.diagnostics.push_back(RuntimeNetworkTransportDiagnostic{
        .code = code,
        .adapter_id = "enet",
        .source_index = source_index,
        .channel_id = channel_id,
        .message = std::move(message),
    });
}

[[nodiscard]] std::uint32_t packet_flags(RuntimeNetworkTransportDelivery delivery) noexcept {
    switch (delivery) {
    case RuntimeNetworkTransportDelivery::reliable_ordered:
        return ENET_PACKET_FLAG_RELIABLE;
    case RuntimeNetworkTransportDelivery::unreliable_unordered:
        return 0U;
    }
    return 0U;
}

[[nodiscard]] RuntimeNetworkTransportDelivery delivery_from_packet(const ENetPacket& packet) noexcept {
    return (packet.flags & ENET_PACKET_FLAG_RELIABLE) != 0U ? RuntimeNetworkTransportDelivery::reliable_ordered
                                                            : RuntimeNetworkTransportDelivery::unreliable_unordered;
}

[[nodiscard]] std::optional<HostPtr> create_server_host(std::uint16_t requested_port, std::size_t peer_count,
                                                        std::size_t channel_count, std::uint16_t& selected_port) {
    ENetAddress address{};
    if (enet_address_set_host(&address, loopback_host) != 0) {
        return std::nullopt;
    }
    address.port = requested_port;

    HostPtr server{enet_host_create(&address, peer_count, channel_count, 0U, 0U)};
    if (server != nullptr) {
        selected_port = server->address.port;
        return server;
    }
    return std::nullopt;
}

[[nodiscard]] HostPtr create_client_host(std::size_t peer_count, std::size_t channel_count) {
    return HostPtr{enet_host_create(nullptr, peer_count, channel_count, 0U, 0U)};
}

void destroy_received_packet(ENetEvent& event) noexcept {
    if (event.packet != nullptr) {
        enet_packet_destroy(event.packet);
        event.packet = nullptr;
    }
}

void record_receive(RuntimeNetworkLoopbackExchangeResult& result, ENetEvent& event, bool server_receive) {
    std::vector<std::byte> payload;
    payload.reserve(event.packet->dataLength);
    for (std::size_t index = 0U; index < event.packet->dataLength; ++index) {
        payload.push_back(static_cast<std::byte>(event.packet->data[index]));
    }

    auto row = RuntimeNetworkReceivedPacketRow{
        .channel_id = event.channelID,
        .delivery = delivery_from_packet(*event.packet),
        .payload = std::move(payload),
        .remote_peer_index = 0U,
    };
    result.bytes_received += row.payload.size();
    if (server_receive) {
        result.server_received_packets.push_back(std::move(row));
    } else {
        result.client_received_packets.push_back(std::move(row));
    }
    result.packets_received = result.server_received_packets.size() + result.client_received_packets.size();
}

[[nodiscard]] bool service_host(RuntimeNetworkLoopbackExchangeResult& result, ENetHost& host, bool server_host,
                                ENetPeer*& server_peer, std::uint32_t timeout_ms) {
    ENetEvent event{};
    while (true) {
        const int status = enet_host_service(&host, &event, timeout_ms);
        if (status < 0) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                           "ENet host service returned an error");
            return false;
        }
        if (status == 0) {
            return true;
        }

        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            if (server_host) {
                server_peer = event.peer;
            }
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            record_receive(result, event, server_host);
            destroy_received_packet(event);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
        case ENET_EVENT_TYPE_NONE:
            break;
        }
        timeout_ms = 0U;
    }
}

[[nodiscard]] bool pump_hosts(RuntimeNetworkLoopbackExchangeResult& result, ENetHost& server, ENetHost& client,
                              ENetPeer*& server_peer, const RuntimeNetworkLoopbackExchangeRequest& request,
                              std::size_t expected_server_packets, std::size_t expected_client_packets) {
    for (std::uint32_t iteration = 0U; iteration < request.max_service_iterations; ++iteration) {
        if (!service_host(result, server, true, server_peer, request.service_timeout_ms)) {
            return false;
        }
        if (!service_host(result, client, false, server_peer, request.service_timeout_ms)) {
            return false;
        }
        if (server_peer != nullptr && result.server_received_packets.size() >= expected_server_packets &&
            result.client_received_packets.size() >= expected_client_packets) {
            return true;
        }
    }
    add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_service_budget,
                   "ENet loopback exchange exhausted the service iteration budget");
    return false;
}

[[nodiscard]] bool send_packet(RuntimeNetworkLoopbackExchangeResult& result, ENetPeer* peer,
                               const RuntimeNetworkPacketDesc& packet) {
    ENetPacket* enet_packet = enet_packet_create(static_cast<const void*>(packet.payload.data()), packet.payload.size(),
                                                 packet_flags(packet.delivery));
    if (enet_packet == nullptr) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                       "ENet packet allocation failed", packet.source_index, packet.channel_id);
        return false;
    }

    if (enet_peer_send(peer, packet.channel_id, enet_packet) != 0) {
        enet_packet_destroy(enet_packet);
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception, "ENet peer send failed",
                       packet.source_index, packet.channel_id);
        return false;
    }

    ++result.packets_sent;
    result.bytes_sent += packet.payload.size();
    return true;
}

class EnetNetworkTransportAdapter final : public IRuntimeNetworkTransportAdapter {
  public:
    [[nodiscard]] RuntimeNetworkTransportAdapterCapabilities capabilities() const override {
        return RuntimeNetworkTransportAdapterCapabilities{
            .adapter_id = "enet",
            .available = true,
            .supports_loopback_exchange = true,
            .supports_reliable_ordered = true,
            .supports_unreliable_unordered = true,
            .exposes_native_handles = false,
            .max_peers = runtime_network_transport_loopback_peer_count,
            .max_channels = runtime_network_transport_max_channels,
            .max_payload_bytes = runtime_network_transport_max_payload_bytes,
            .max_service_iterations = runtime_network_transport_max_service_iterations,
        };
    }

    [[nodiscard]] RuntimeNetworkLoopbackExchangeResult
    execute_loopback_exchange(const RuntimeNetworkLoopbackExchangeRequest& request) override {
        EnetLibraryLease lease;

        RuntimeNetworkLoopbackExchangeResult result;
        result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        result.adapter_id = "enet";

        std::uint16_t selected_port{0U};
        auto maybe_server = create_server_host(request.port, request.peer_count, request.channel_count, selected_port);
        if (!maybe_server) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                           "ENet server host creation failed");
            return result;
        }
        auto server = std::move(*maybe_server);

        auto client = create_client_host(1U, request.channel_count);
        if (client == nullptr) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                           "ENet client host creation failed");
            return result;
        }

        ENetAddress address{};
        if (enet_address_set_host(&address, loopback_host) != 0) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                           "ENet loopback address resolution failed");
            return result;
        }
        address.port = selected_port;

        ENetPeer* client_peer = enet_host_connect(client.get(), &address, request.channel_count, 0U);
        if (client_peer == nullptr) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                           "ENet client peer connection allocation failed");
            return result;
        }

        ENetPeer* server_peer = nullptr;
        if (!pump_hosts(result, *server, *client, server_peer, request, 0U, 0U) || server_peer == nullptr) {
            enet_peer_reset(client_peer);
            if (result.diagnostics.empty()) {
                add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception,
                               "ENet loopback connection did not complete");
            }
            return result;
        }

        for (const auto& packet : request.client_to_server_packets) {
            if (!send_packet(result, client_peer, packet)) {
                enet_peer_reset(client_peer);
                return result;
            }
        }
        for (const auto& packet : request.server_to_client_packets) {
            if (!send_packet(result, server_peer, packet)) {
                enet_peer_reset(client_peer);
                return result;
            }
        }

        enet_host_flush(client.get());
        enet_host_flush(server.get());

        if (!pump_hosts(result, *server, *client, server_peer, request, request.client_to_server_packets.size(),
                        request.server_to_client_packets.size())) {
            enet_peer_reset(client_peer);
            return result;
        }

        enet_peer_disconnect(client_peer, 0U);
        for (std::uint32_t iteration = 0U; iteration < request.max_service_iterations; ++iteration) {
            ENetEvent event{};
            const int client_status = enet_host_service(client.get(), &event, 0U);
            if (client_status > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
                destroy_received_packet(event);
            }
            const int server_status = enet_host_service(server.get(), &event, 0U);
            if (server_status > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
                destroy_received_packet(event);
            }
            if (client_status <= 0 && server_status <= 0) {
                break;
            }
        }

        result.status = RuntimeNetworkTransportAdapterStatus::completed;
        return result;
    }
};

} // namespace

std::unique_ptr<IRuntimeNetworkTransportAdapter> make_enet_network_transport_adapter() {
    return std::make_unique<EnetNetworkTransportAdapter>();
}

} // namespace mirakana::runtime
