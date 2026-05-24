// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/network_transport.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace mirakana::runtime {
namespace {

void add_diagnostic(RuntimeNetworkLoopbackExchangeResult& result, RuntimeNetworkTransportDiagnosticCode code,
                    std::string adapter_id, std::uint32_t source_index, std::uint8_t channel_id, std::string message) {
    result.diagnostics.push_back(RuntimeNetworkTransportDiagnostic{
        .code = code,
        .adapter_id = std::move(adapter_id),
        .source_index = source_index,
        .channel_id = channel_id,
        .message = std::move(message),
    });
}

void sort_diagnostics(RuntimeNetworkLoopbackExchangeResult& result) {
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.channel_id != rhs.channel_id) {
            return lhs.channel_id < rhs.channel_id;
        }
        return lhs.adapter_id < rhs.adapter_id;
    });
}

[[nodiscard]] bool delivery_supported(RuntimeNetworkTransportDelivery delivery,
                                      const RuntimeNetworkTransportAdapterCapabilities& capabilities) noexcept {
    switch (delivery) {
    case RuntimeNetworkTransportDelivery::reliable_ordered:
        return capabilities.supports_reliable_ordered;
    case RuntimeNetworkTransportDelivery::unreliable_unordered:
        return capabilities.supports_unreliable_unordered;
    }
    return false;
}

[[nodiscard]] RuntimeNetworkTransportDiagnosticCode
unsupported_delivery_code(RuntimeNetworkTransportDelivery delivery) noexcept {
    switch (delivery) {
    case RuntimeNetworkTransportDelivery::reliable_ordered:
        return RuntimeNetworkTransportDiagnosticCode::reliable_delivery_unsupported;
    case RuntimeNetworkTransportDelivery::unreliable_unordered:
        return RuntimeNetworkTransportDiagnosticCode::unreliable_delivery_unsupported;
    }
    return RuntimeNetworkTransportDiagnosticCode::unreliable_delivery_unsupported;
}

void validate_packet(RuntimeNetworkLoopbackExchangeResult& result,
                     const RuntimeNetworkTransportAdapterCapabilities& capabilities,
                     const RuntimeNetworkPacketDesc& packet, std::uint32_t channel_count) {
    const std::size_t max_payload_bytes =
        std::min(capabilities.max_payload_bytes, runtime_network_transport_max_payload_bytes);
    if (packet.channel_id >= channel_count) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_channel, capabilities.adapter_id,
                       packet.source_index, packet.channel_id, "packet channel must be lower than channel_count");
    }
    if (packet.payload.empty() || packet.payload.size() > max_payload_bytes) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_payload, capabilities.adapter_id,
                       packet.source_index, packet.channel_id,
                       "packet payload must be non-empty and within the facade and adapter payload limits");
    }
    if (!delivery_supported(packet.delivery, capabilities)) {
        add_diagnostic(result, unsupported_delivery_code(packet.delivery), capabilities.adapter_id, packet.source_index,
                       packet.channel_id, "packet delivery mode is not supported by this adapter");
    }
}

void validate_request(RuntimeNetworkLoopbackExchangeResult& result,
                      const RuntimeNetworkTransportAdapterCapabilities& capabilities,
                      const RuntimeNetworkLoopbackExchangeRequest& request) {
    if (!capabilities.available) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_unavailable, capabilities.adapter_id, 0U,
                       0U, "network transport adapter is not available");
    }
    if (!capabilities.supports_loopback_exchange) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::loopback_exchange_unsupported,
                       capabilities.adapter_id, 0U, 0U, "network transport adapter does not support loopback exchange");
    }
    if (request.request_native_handles || capabilities.exposes_native_handles) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::native_handles_requested, capabilities.adapter_id,
                       0U, 0U, "network transport facade does not expose native handles");
    }
    if (request.peer_count != runtime_network_transport_loopback_peer_count ||
        request.peer_count > capabilities.max_peers || request.peer_count > runtime_network_transport_max_peers) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_peer_count, capabilities.adapter_id, 0U,
                       0U,
                       "peer_count must be exactly one for the v1 loopback exchange and within the adapter peer limit");
    }
    if (request.channel_count == 0U || request.channel_count > capabilities.max_channels ||
        request.channel_count > runtime_network_transport_max_channels) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_channel_count, capabilities.adapter_id,
                       0U, 0U, "channel_count must be non-zero and within the adapter channel limit");
    }
    if (request.max_service_iterations == 0U || request.max_service_iterations > capabilities.max_service_iterations ||
        request.max_service_iterations > runtime_network_transport_max_service_iterations) {
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::invalid_service_budget, capabilities.adapter_id,
                       0U, 0U, "max_service_iterations must be non-zero and within the adapter service budget");
    }

    for (const auto& packet : request.client_to_server_packets) {
        validate_packet(result, capabilities, packet, request.channel_count);
    }
    for (const auto& packet : request.server_to_client_packets) {
        validate_packet(result, capabilities, packet, request.channel_count);
    }
}

} // namespace

bool RuntimeNetworkLoopbackExchangeResult::succeeded() const noexcept {
    return status == RuntimeNetworkTransportAdapterStatus::completed && diagnostics.empty() && !native_handles_exposed;
}

RuntimeNetworkLoopbackExchangeResult
execute_runtime_network_loopback_exchange(const RuntimeNetworkLoopbackExchangeRequest& request,
                                          IRuntimeNetworkTransportAdapter* adapter) {
    RuntimeNetworkLoopbackExchangeResult result;
    if (adapter == nullptr) {
        result.status = RuntimeNetworkTransportAdapterStatus::unavailable;
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::missing_adapter, {}, 0U, 0U,
                       "network transport adapter is required");
        return result;
    }

    RuntimeNetworkTransportAdapterCapabilities capabilities;
    try {
        capabilities = adapter->capabilities();
    } catch (const std::exception& error) {
        result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception, {}, 0U, 0U, error.what());
        return result;
    } catch (...) {
        result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception, {}, 0U, 0U,
                       "network transport adapter raised an unknown exception while reporting capabilities");
        return result;
    }

    result.adapter_id = capabilities.adapter_id;
    validate_request(result, capabilities, request);
    if (!result.diagnostics.empty()) {
        result.status = capabilities.available ? RuntimeNetworkTransportAdapterStatus::invalid_request
                                               : RuntimeNetworkTransportAdapterStatus::unavailable;
        sort_diagnostics(result);
        return result;
    }

    try {
        result = adapter->execute_loopback_exchange(request);
        if (result.adapter_id.empty()) {
            result.adapter_id = capabilities.adapter_id;
        }
        if (result.native_handles_exposed) {
            add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::native_handles_requested, result.adapter_id,
                           0U, 0U, "network transport adapter attempted to expose native handles");
            result.native_handles_exposed = false;
            result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        }
        if (!result.diagnostics.empty()) {
            sort_diagnostics(result);
        }
        return result;
    } catch (const std::exception& error) {
        result = RuntimeNetworkLoopbackExchangeResult{};
        result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        result.adapter_id = capabilities.adapter_id;
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception, capabilities.adapter_id, 0U,
                       0U, error.what());
        return result;
    } catch (...) {
        result = RuntimeNetworkLoopbackExchangeResult{};
        result.status = RuntimeNetworkTransportAdapterStatus::adapter_failed;
        result.adapter_id = capabilities.adapter_id;
        add_diagnostic(result, RuntimeNetworkTransportDiagnosticCode::adapter_exception, capabilities.adapter_id, 0U,
                       0U, "network transport adapter raised an unknown exception during loopback exchange");
        return result;
    }
}

} // namespace mirakana::runtime
