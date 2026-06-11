// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgMetalCapabilityStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class MavgMetalCapabilityKind : std::uint8_t {
    streamed_cluster_draw = 0,
    mesh_shader_execution,
    gpu_memory_residency,
    deformation_tier,
    ray_tracing_consistency,
    benchmark_evidence,
};

enum class MavgMetalCapabilityRowStatus : std::uint8_t {
    ready = 0,
    host_gated,
    unsupported,
};

enum class MavgMetalCapabilityDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_capability,
    duplicate_required_capability,
    unsupported_backend,
    missing_required_capability_row,
    duplicate_capability_row,
    invalid_capability_row,
    missing_backend_local_evidence,
    missing_apple_host_evidence,
    missing_host_validation_recipe,
    unreviewed_host_validation_recipe,
    missing_package_counter_evidence,
    unsupported_cross_backend_inference,
    unsupported_native_handle_claim,
    unsupported_gpu_execution_claim,
    unsupported_nanite_claim,
    row_budget_exceeded,
};

struct MavgMetalCapabilityRow {
    std::string capability_id;
    MavgMetalCapabilityKind capability{MavgMetalCapabilityKind::streamed_cluster_draw};
    rhi::BackendKind backend{rhi::BackendKind::metal};
    MavgMetalCapabilityRowStatus status{MavgMetalCapabilityRowStatus::host_gated};
    bool reviewed{false};
    bool backend_local_evidence{false};
    bool apple_host_validated{false};
    bool host_gate_required{false};
    std::string host_validation_recipe_id;
    std::string package_counter_id;
    bool request_cross_backend_inference{false};
    bool request_native_handle_access{false};
    bool request_gpu_execution{false};
    bool request_nanite_claim{false};
    std::uint32_t source_index{0U};
};

struct MavgMetalCapabilityRequest {
    std::vector<MavgMetalCapabilityKind> required_capabilities;
    std::vector<MavgMetalCapabilityRow> rows;
    std::size_t row_budget{128U};
    std::uint64_t seed{0U};
};

struct MavgMetalCapabilityDiagnostic {
    MavgMetalCapabilityDiagnosticCode code{MavgMetalCapabilityDiagnosticCode::none};
    MavgMetalCapabilityKind capability{MavgMetalCapabilityKind::streamed_cluster_draw};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct MavgMetalCapabilityPlan {
    MavgMetalCapabilityStatus status{MavgMetalCapabilityStatus::invalid_request};
    std::vector<MavgMetalCapabilityDiagnostic> diagnostics;
    std::vector<MavgMetalCapabilityKind> required_capabilities;
    std::vector<MavgMetalCapabilityRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t unsupported_row_count{0U};
    std::size_t host_validated_capability_count{0U};
    std::uint64_t replay_hash{0U};
    bool metal_mavg_ready{false};
    bool requires_apple_host_evidence{false};
    bool has_apple_host_evidence{false};
    bool executed_gpu_commands{false};
    bool exposed_native_handles{false};
    bool claimed_nanite_equivalence{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews Metal MAVG capability evidence rows without executing GPU work, exposing native handles,
/// or inferring Apple-host readiness from D3D12/Vulkan evidence.
[[nodiscard]] MavgMetalCapabilityPlan plan_mavg_metal_capabilities(const MavgMetalCapabilityRequest& request);

[[nodiscard]] const char* mavg_metal_capability_diagnostic_message(MavgMetalCapabilityDiagnosticCode code) noexcept;

} // namespace mirakana
