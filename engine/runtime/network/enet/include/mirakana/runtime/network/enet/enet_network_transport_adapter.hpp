// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/network_transport.hpp"

#include <memory>

namespace mirakana::runtime {

[[nodiscard]] std::unique_ptr<IRuntimeNetworkTransportAdapter> make_enet_network_transport_adapter();

} // namespace mirakana::runtime
