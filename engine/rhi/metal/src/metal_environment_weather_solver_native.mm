// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/metal/metal_environment_weather_solver.hpp"

#include "metal_native_private.hpp"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace mirakana::rhi::metal {

namespace {

struct MetalEnvironmentWeatherSolverConstants {
    float effective_timestep_s{0.0F};
    float air_pressure_hpa{1013.25F};
    float mixing_height_m{1000.0F};
    std::uint32_t cell_count{0U};
};

[[nodiscard]] std::uint64_t fnv1a64(std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto byte : bytes) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

[[nodiscard]] bool has_nonzero_byte(std::span<const std::byte> bytes) noexcept {
    return std::ranges::any_of(bytes, [](std::byte byte) { return byte != std::byte{0}; });
}

[[nodiscard]] NSString* to_ns_string(std::string_view text) {
    if (text.empty()) {
        return nil;
    }
    return [[NSString alloc] initWithBytes:text.data() length:text.size() encoding:NSUTF8StringEncoding];
}

} // namespace

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

    if (desc.compute_function_name.empty()) {
        result.failure_stage = 4U;
        result.diagnostic = "Metal environment weather solver compute function name is required";
        return result;
    }

    auto native_device = create_native_device_and_command_queue(MetalNativeDeviceQueueDesc{.host = host});
    result.command_queue_ready = native_device.device.owns_command_queue();
    if (!native_device.created) {
        result.failure_stage = 5U;
        result.diagnostic = native_device.diagnostic;
        return result;
    }

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)native_device.device.impl_->device;
    id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)native_device.device.impl_->command_queue;
    NSString* metallib_path = to_ns_string(desc.metallib_path);
    if (metallib_path == nil) {
        result.failure_stage = 6U;
        result.diagnostic = "Metal environment weather solver metallib path is not UTF-8";
        return result;
    }

    NSError* error = nil;
    id<MTLLibrary> library = [metal_device newLibraryWithURL:[NSURL fileURLWithPath:metallib_path] error:&error];
    if (library == nil) {
        result.failure_stage = 7U;
        result.diagnostic = error != nil && error.localizedDescription.UTF8String != nullptr
                                ? std::string{error.localizedDescription.UTF8String}
                                : "Metal environment weather solver metallib load failed";
        return result;
    }
    result.metallib_valid = true;

    NSString* function_name = to_ns_string(desc.compute_function_name);
    id<MTLFunction> function = [library newFunctionWithName:function_name];
    if (function == nil) {
        result.failure_stage = 8U;
        result.diagnostic = "Metal environment weather solver metallib is missing compute entry point";
        return result;
    }

    id<MTLComputePipelineState> pipeline = [metal_device newComputePipelineStateWithFunction:function error:&error];
    if (pipeline == nil) {
        result.failure_stage = 9U;
        result.diagnostic = error != nil && error.localizedDescription.UTF8String != nullptr
                                ? std::string{error.localizedDescription.UTF8String}
                                : "Metal environment weather solver compute pipeline creation failed";
        return result;
    }
    result.compute_pipeline_ready = true;

    const auto input_bytes = static_cast<NSUInteger>(desc.cell_rows.size_bytes());
    const auto output_bytes = static_cast<NSUInteger>(result.output_buffer_size_bytes);
    const auto constants = MetalEnvironmentWeatherSolverConstants{
        .effective_timestep_s = desc.effective_timestep_s,
        .air_pressure_hpa = desc.air_pressure_hpa,
        .mixing_height_m = desc.mixing_height_m,
        .cell_count = result.cell_count,
    };

    id<MTLBuffer> input_buffer = [metal_device newBufferWithBytes:desc.cell_rows.data()
                                                           length:input_bytes
                                                          options:MTLResourceStorageModeShared];
    id<MTLBuffer> output_buffer =
        [metal_device newBufferWithLength:output_bytes options:MTLResourceStorageModeShared];
    id<MTLBuffer> constants_buffer = [metal_device newBufferWithBytes:&constants
                                                               length:sizeof(constants)
                                                              options:MTLResourceStorageModeShared];
    if (input_buffer == nil || output_buffer == nil || constants_buffer == nil) {
        result.failure_stage = 10U;
        result.diagnostic = "Metal environment weather solver buffer creation failed";
        return result;
    }

    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
    if (command_buffer == nil || compute_encoder == nil) {
        result.failure_stage = 11U;
        result.diagnostic = "Metal environment weather solver compute command creation failed";
        return result;
    }
    result.command_buffer_ready = true;

    [compute_encoder setComputePipelineState:pipeline];
    [compute_encoder setBuffer:input_buffer offset:0 atIndex:0];
    [compute_encoder setBuffer:output_buffer offset:0 atIndex:1];
    [compute_encoder setBuffer:constants_buffer offset:0 atIndex:2];
    result.buffer_bindings = 3U;

    const auto threads_per_group = std::min<NSUInteger>(64U, std::max<NSUInteger>(1U, pipeline.maxTotalThreadsPerThreadgroup));
    [compute_encoder dispatchThreads:MTLSizeMake(result.cell_count, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(threads_per_group, 1, 1)];
    [compute_encoder endEncoding];
    result.compute_dispatches = 1U;
    result.synchronization_points = 1U;

    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    if (command_buffer.status != MTLCommandBufferStatusCompleted) {
        result.failure_stage = 12U;
        result.diagnostic = "Metal environment weather solver command buffer failed";
        return result;
    }

    const auto* output = static_cast<const std::byte*>([output_buffer contents]);
    if (output == nullptr) {
        result.failure_stage = 13U;
        result.diagnostic = "Metal environment weather solver readback contents are unavailable";
        return result;
    }

    const auto output_span = std::span<const std::byte>{output, static_cast<std::size_t>(output_bytes)};
    result.output_readback_nonzero = has_nonzero_byte(output_span);
    result.output_checksum = fnv1a64(output_span);
    result.output_rows.resize(desc.cell_rows.size());
    std::memcpy(result.output_rows.data(), output, static_cast<std::size_t>(output_bytes));
    result.succeeded = result.output_readback_nonzero && result.output_checksum != 0U;
    result.executed_gpu_solver = result.succeeded;
    result.failure_stage = result.succeeded ? 0U : 14U;
    result.diagnostic =
        result.succeeded ? "Metal environment weather solver completed" : "Metal environment weather solver produced empty output";
    return result;
}

} // namespace mirakana::rhi::metal
