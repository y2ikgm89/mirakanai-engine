// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/metal/metal_environment_weather_solver.hpp"

namespace mirakana::rhi::metal {

static_assert(sizeof(MetalEnvironmentWeatherSolverCellRow) == metal_environment_weather_solver_cell_row_stride_bytes);
static_assert(sizeof(MetalEnvironmentWeatherSolverOutputRow) ==
              metal_environment_weather_solver_output_row_stride_bytes);

#if !defined(__APPLE__)
MetalEnvironmentWeatherSolverResult
dispatch_environment_weather_solver(const MetalEnvironmentWeatherSolverDesc& desc) noexcept {
    MetalEnvironmentWeatherSolverResult result;
    result.cell_count = static_cast<std::uint32_t>(desc.cell_rows.size());
    result.output_buffer_size_bytes =
        static_cast<std::uint64_t>(result.cell_count) * metal_environment_weather_solver_output_row_stride_bytes;

    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    if (!supports_host(host)) {
        result.host_evidence_required = true;
        result.failure_stage = 1U;
        result.diagnostic = "Metal environment weather solver requires an Apple host";
        return result;
    }

    if (desc.cell_rows.empty()) {
        result.failure_stage = 2U;
        result.diagnostic = "Metal environment weather solver cells are required";
        return result;
    }

    if (desc.metallib_path.empty()) {
        result.failure_stage = 3U;
        result.diagnostic = "Metal environment weather solver metallib path is required";
        return result;
    }

    result.host_evidence_required = true;
    result.failure_stage = 4U;
    result.diagnostic = "Metal environment weather solver Objective-C++ execution is unavailable on this host";
    return result;
}
#endif

} // namespace mirakana::rhi::metal
