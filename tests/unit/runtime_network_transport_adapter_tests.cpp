// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/network_transport.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

[[nodiscard]] std::vector<std::byte> repeated_bytes(std::size_t size) {
    std::vector<std::byte> payload(size);
    for (std::size_t index = 0U; index < payload.size(); ++index) {
        payload[index] = static_cast<std::byte>(index & 0xffU);
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

[[nodiscard]] mirakana::runtime::RuntimeNetworkPacketDesc
make_packet(std::uint8_t channel_id, mirakana::runtime::RuntimeNetworkTransportDelivery delivery,
            std::vector<std::byte> payload, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeNetworkPacketDesc{
        .channel_id = channel_id,
        .delivery = delivery,
        .payload = std::move(payload),
        .source_index = source_index,
    };
}

class RecordingNetworkTransportAdapter final : public mirakana::runtime::IRuntimeNetworkTransportAdapter {
  public:
    explicit RecordingNetworkTransportAdapter(
        mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities capabilities)
        : capabilities_(std::move(capabilities)) {}

    [[nodiscard]] mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities capabilities() const override {
        return capabilities_;
    }

    [[nodiscard]] mirakana::runtime::RuntimeNetworkLoopbackExchangeResult
    execute_loopback_exchange(const mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest& request) override {
        called = true;
        mirakana::runtime::RuntimeNetworkLoopbackExchangeResult result;
        result.status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus::completed;
        result.adapter_id = capabilities_.adapter_id;
        result.packets_sent = request.client_to_server_packets.size() + request.server_to_client_packets.size();
        for (const auto& packet : request.client_to_server_packets) {
            result.bytes_sent += packet.payload.size();
            result.bytes_received += packet.payload.size();
            result.server_received_packets.push_back(mirakana::runtime::RuntimeNetworkReceivedPacketRow{
                .channel_id = packet.channel_id,
                .delivery = packet.delivery,
                .payload = packet.payload,
                .remote_peer_index = 0U,
            });
        }
        for (const auto& packet : request.server_to_client_packets) {
            result.bytes_sent += packet.payload.size();
            result.bytes_received += packet.payload.size();
            result.client_received_packets.push_back(mirakana::runtime::RuntimeNetworkReceivedPacketRow{
                .channel_id = packet.channel_id,
                .delivery = packet.delivery,
                .payload = packet.payload,
                .remote_peer_index = 0U,
            });
        }
        result.packets_received = result.server_received_packets.size() + result.client_received_packets.size();
        return result;
    }

    bool called{false};

  private:
    mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities capabilities_;
};

[[nodiscard]] mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities make_capabilities() {
    return mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities{
        .adapter_id = "recording",
        .available = true,
        .supports_loopback_exchange = true,
        .supports_reliable_ordered = true,
        .supports_unreliable_unordered = true,
        .exposes_native_handles = false,
        .max_peers = 4U,
        .max_channels = 4U,
        .max_payload_bytes = 1024U,
        .max_service_iterations = 128U,
    };
}

class ThrowingNetworkTransportAdapter final : public mirakana::runtime::IRuntimeNetworkTransportAdapter {
  public:
    [[nodiscard]] mirakana::runtime::RuntimeNetworkTransportAdapterCapabilities capabilities() const override {
        return make_capabilities();
    }

    [[nodiscard]] mirakana::runtime::RuntimeNetworkLoopbackExchangeResult
    execute_loopback_exchange(const mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest&) override {
        throw std::runtime_error{"transport exploded"};
    }
};

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeNetworkLoopbackExchangeResult& result,
                                           mirakana::runtime::RuntimeNetworkTransportDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime network transport facade fails closed without adapter") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "ping", 1U)},
        .server_to_client_packets = {},
        .peer_count = 1U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, nullptr);

    MK_REQUIRE(result.status == Status::unavailable);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(result.server_received_packets.empty());
    MK_REQUIRE(diagnostic_count(result, Code::missing_adapter) == 1U);
}

MK_TEST("runtime network transport facade rejects invalid requests before adapter dispatch") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    RecordingNetworkTransportAdapter adapter{make_capabilities()};
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets =
            {
                make_packet(5U, Delivery::reliable_ordered, "bad-channel", 2U),
                make_packet(0U, Delivery::unreliable_unordered, "", 3U),
            },
        .server_to_client_packets = {},
        .peer_count = 0U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 0U,
        .request_native_handles = true,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(result.status == Status::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!adapter.called);
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(diagnostic_count(result, Code::native_handles_requested) == 1U);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_peer_count) == 1U);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_channel) == 1U);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_payload) == 1U);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_service_budget) == 1U);
}

MK_TEST("runtime network transport facade rejects unsupported multi peer loopback before adapter dispatch") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    RecordingNetworkTransportAdapter adapter{make_capabilities()};
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client", 8U)},
        .server_to_client_packets = {},
        .peer_count = 2U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(result.status == Status::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!adapter.called);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_peer_count) == 1U);
}

MK_TEST("runtime network transport facade fails closed when adapter is unavailable") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    auto capabilities = make_capabilities();
    capabilities.available = false;
    RecordingNetworkTransportAdapter adapter{std::move(capabilities)};
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client", 10U)},
        .server_to_client_packets = {},
        .peer_count = 1U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(result.status == Status::unavailable);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!adapter.called);
    MK_REQUIRE(diagnostic_count(result, Code::adapter_unavailable) == 1U);
}

MK_TEST("runtime network transport facade enforces its payload ceiling even when adapter allows more") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    auto capabilities = make_capabilities();
    capabilities.max_payload_bytes = mirakana::runtime::runtime_network_transport_max_payload_bytes + 1U;
    RecordingNetworkTransportAdapter adapter{std::move(capabilities)};
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(
            0U, Delivery::reliable_ordered,
            repeated_bytes(mirakana::runtime::runtime_network_transport_max_payload_bytes + 1U), 9U)},
        .server_to_client_packets = {},
        .peer_count = 1U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(result.status == Status::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!adapter.called);
    MK_REQUIRE(diagnostic_count(result, Code::invalid_payload) == 1U);
}

MK_TEST("runtime network transport facade converts adapter exceptions to diagnostics") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;
    using Code = mirakana::runtime::RuntimeNetworkTransportDiagnosticCode;

    ThrowingNetworkTransportAdapter adapter;
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client", 11U)},
        .server_to_client_packets = {},
        .peer_count = 1U,
        .channel_count = 1U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(result.status == Status::adapter_failed);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.adapter_id == "recording");
    MK_REQUIRE(diagnostic_count(result, Code::adapter_exception) == 1U);
}

MK_TEST("runtime network transport facade dispatches first party loopback exchange") {
    using Delivery = mirakana::runtime::RuntimeNetworkTransportDelivery;
    using Status = mirakana::runtime::RuntimeNetworkTransportAdapterStatus;

    RecordingNetworkTransportAdapter adapter{make_capabilities()};
    const auto request = mirakana::runtime::RuntimeNetworkLoopbackExchangeRequest{
        .client_to_server_packets = {make_packet(0U, Delivery::reliable_ordered, "client", 4U)},
        .server_to_client_packets = {make_packet(1U, Delivery::unreliable_unordered, "server", 5U)},
        .peer_count = 1U,
        .channel_count = 2U,
        .port = 0U,
        .service_timeout_ms = 1000U,
        .max_service_iterations = 32U,
        .request_native_handles = false,
    };

    const auto result = mirakana::runtime::execute_runtime_network_loopback_exchange(request, &adapter);

    MK_REQUIRE(adapter.called);
    MK_REQUIRE(result.status == Status::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.adapter_id == "recording");
    MK_REQUIRE(result.packets_sent == 2U);
    MK_REQUIRE(result.packets_received == 2U);
    MK_REQUIRE(result.bytes_sent == 12U);
    MK_REQUIRE(result.bytes_received == 12U);
    MK_REQUIRE(result.server_received_packets[0].payload == bytes("client"));
    MK_REQUIRE(result.server_received_packets[0].remote_peer_index == 0U);
    MK_REQUIRE(result.client_received_packets[0].payload == bytes("server"));
    MK_REQUIRE(result.client_received_packets[0].remote_peer_index == 0U);
    MK_REQUIRE(!result.native_handles_exposed);
}

int main() {
    return mirakana::test::run_all();
}
