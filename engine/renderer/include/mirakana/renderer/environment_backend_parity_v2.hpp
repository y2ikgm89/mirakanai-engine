// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace mirakana {

enum class EnvironmentBackendParityV2Status : std::uint8_t {
    ready,
    host_evidence_required,
    blocked,
    no_rows,
};

enum class EnvironmentBackendParityV2Feature : std::uint8_t {
    physical_sky,
    height_fog,
    volumetric_fog,
    volumetric_cloud,
    cloud_layer,
    rain_precipitation,
    snow_precipitation,
    material_weathering,
    environment_lighting_ibl,
    postprocess_depth_input,
    texture_payload_rgba8_upload,
    texture_payload_bc7_or_astc_upload,
    weather_solver_gpu,
    debug_profiling_policy,
    quality_budget,
};

enum class EnvironmentBackendParityV2RowStatus : std::uint8_t {
    missing,
    ready,
    host_gated,
    blocked,
    unsupported,
};

struct EnvironmentBackendParityV2Row {
    rhi::BackendKind backend{rhi::BackendKind::null};
    EnvironmentBackendParityV2Feature feature{EnvironmentBackendParityV2Feature::physical_sky};
    std::string feature_id;
    EnvironmentBackendParityV2RowStatus status{EnvironmentBackendParityV2RowStatus::missing};
    std::string aggregate_recipe_id;
    std::string host_gate_id;
    bool host_matches{false};
    bool feature_present{false};
    bool backend_local_execution_ready{false};
    bool shader_or_pipeline_ready{false};
    bool synchronization_ready{false};
    bool readback_or_output_ready{false};
    bool quality_budget_ready{false};
    bool resource_budget_ready{false};
    bool package_counter_ready{false};
    bool native_handle_access{false};
    bool inferred_from_other_backend{false};
    bool diagnostics{false};
    std::uint32_t source_index{0U};
};

struct EnvironmentBackendParityV2Result {
    EnvironmentBackendParityV2Status status{EnvironmentBackendParityV2Status::no_rows};
    bool d3d12_ready{false};
    bool vulkan_ready{false};
    bool metal_ready{false};
    bool environment_backend_parity_ready{false};
    bool environment_all_platform_unconditional_ready{false};
    std::size_t required_features{15U};
    std::size_t required_backends{3U};
    std::size_t required_rows{45U};
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t blocked_rows{0U};
    std::size_t unsupported_rows{0U};
    std::size_t missing_rows{45U};
    std::size_t host_validated_backends{0U};
    bool invoked_gpu_commands{false};
    bool native_handle_access{false};
    bool inferred_from_other_backend{false};
    bool diagnostics{false};
};

[[nodiscard]] EnvironmentBackendParityV2Result
evaluate_environment_backend_parity_v2(std::span<const EnvironmentBackendParityV2Row> rows);

} // namespace mirakana
