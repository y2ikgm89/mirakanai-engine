// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace mirakana {

enum class EnvironmentCommercialReadinessV2RowStatus : std::uint8_t {
    missing,
    ready,
    host_gated,
    dependency_gated,
    blocked,
    unsupported,
};

struct EnvironmentCommercialReadinessV2Row {
    std::string id;
    EnvironmentCommercialReadinessV2RowStatus status{EnvironmentCommercialReadinessV2RowStatus::missing};
    std::string evidence_recipe_id;
    std::string evidence_host_gate_id;
    bool native_handle_access{false};
    bool diagnostics{false};
};

struct EnvironmentCommercialReadinessV2Result {
    bool highest_commercial_ready{false};
    bool commercial_ready{false};
    std::size_t required_rows{0};
    std::size_t ready_rows{0};
    std::size_t host_gated_rows{0};
    std::size_t dependency_gated_rows{0};
    std::size_t blocked_rows{0};
    std::size_t unsupported_rows{0};
    bool native_handle_access{false};
    bool diagnostics{false};
};

[[nodiscard]] EnvironmentCommercialReadinessV2Result
evaluate_environment_commercial_readiness_v2(std::span<const EnvironmentCommercialReadinessV2Row> rows);

} // namespace mirakana
