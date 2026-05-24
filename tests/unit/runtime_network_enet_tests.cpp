// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/network/enet/enet_network_transport_adapter.hpp"
#include "mirakana/runtime/network_transport.hpp"

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
