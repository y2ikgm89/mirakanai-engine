// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana {

enum class PhysicsCollisionQueryBatchStatus : std::uint8_t { completed, invalid_request };

enum class PhysicsCollisionQueryBatchDiagnostic : std::uint8_t { none, query_budget_exceeded };

enum class PhysicsCollisionQueryRowStatus : std::uint8_t { hit, no_hit, invalid_request };

enum class PhysicsCollisionQueryRowDiagnostic : std::uint8_t { none, invalid_request };

} // namespace mirakana
