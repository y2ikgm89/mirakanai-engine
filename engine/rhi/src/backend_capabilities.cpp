// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/backend_capabilities.hpp"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace mirakana::rhi {
namespace {

[[nodiscard]] BackendCapabilityProfile inactive_capabilities(BackendKind backend) noexcept {
    auto capabilities = default_backend_capabilities(backend);
    capabilities.headless = backend == BackendKind::null;
    capabilities.native_device = false;
    capabilities.swapchain = false;
    capabilities.surface_presentation = false;
    capabilities.descriptor_sets = false;
    capabilities.pipeline_layouts = false;
    capabilities.graphics_pipelines = false;
    capabilities.explicit_resource_transitions = false;
    capabilities.explicit_copy_queues = false;
    capabilities.explicit_queue_family_selection = false;
    return capabilities;
}

} // namespace

std::string_view backend_kind_id(BackendKind backend) noexcept {
    switch (backend) {
    case BackendKind::null:
        return "null";
    case BackendKind::d3d12:
        return "d3d12";
    case BackendKind::vulkan:
        return "vulkan";
    case BackendKind::metal:
        return "metal";
    }
    return "unknown";
}

std::string_view rhi_host_platform_id(RhiHostPlatform host) noexcept {
    switch (host) {
    case RhiHostPlatform::windows:
        return "windows";
    case RhiHostPlatform::linux:
        return "linux";
    case RhiHostPlatform::android:
        return "android";
    case RhiHostPlatform::macos:
        return "macos";
    case RhiHostPlatform::ios:
        return "ios";
    case RhiHostPlatform::unknown:
        return "unknown";
    }
    return "unknown";
}

std::string_view shader_artifact_format_id(ShaderArtifactFormat format) noexcept {
    switch (format) {
    case ShaderArtifactFormat::none:
        return "none";
    case ShaderArtifactFormat::dxil:
        return "dxil";
    case ShaderArtifactFormat::spirv:
        return "spirv";
    case ShaderArtifactFormat::metallib:
        return "metallib";
    }
    return "unknown";
}

std::string_view backend_probe_status_id(BackendProbeStatus status) noexcept {
    switch (status) {
    case BackendProbeStatus::unavailable:
        return "unavailable";
    case BackendProbeStatus::available:
        return "available";
    case BackendProbeStatus::unsupported_host:
        return "unsupported_host";
    case BackendProbeStatus::missing_runtime:
        return "missing_runtime";
    case BackendProbeStatus::missing_shader_artifacts:
        return "missing_shader_artifacts";
    case BackendProbeStatus::no_suitable_device:
        return "no_suitable_device";
    case BackendProbeStatus::no_present_queue:
        return "no_present_queue";
    }
    return "unknown";
}

std::string_view backend_probe_step_id(BackendProbeStep step) noexcept {
    switch (step) {
    case BackendProbeStep::validate_host:
        return "validate_host";
    case BackendProbeStep::load_runtime:
        return "load_runtime";
    case BackendProbeStep::create_dxgi_factory:
        return "create_dxgi_factory";
    case BackendProbeStep::enumerate_adapters:
        return "enumerate_adapters";
    case BackendProbeStep::enumerate_physical_devices:
        return "enumerate_physical_devices";
    case BackendProbeStep::query_queue_families:
        return "query_queue_families";
    case BackendProbeStep::create_device:
        return "create_device";
    case BackendProbeStep::create_logical_device:
        return "create_logical_device";
    case BackendProbeStep::create_default_device:
        return "create_default_device";
    case BackendProbeStep::create_command_queue:
        return "create_command_queue";
    case BackendProbeStep::verify_shader_artifacts:
        return "verify_shader_artifacts";
    }
    return "unknown";
}

RhiHostPlatform current_rhi_host_platform() noexcept {
#if defined(_WIN32)
    return RhiHostPlatform::windows;
#elif defined(__ANDROID__)
    return RhiHostPlatform::android;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    return RhiHostPlatform::ios;
#else
    return RhiHostPlatform::macos;
#endif
#elif defined(__linux__)
    return RhiHostPlatform::linux;
#else
    return RhiHostPlatform::unknown;
#endif
}

BackendCapabilityProfile default_backend_capabilities(BackendKind backend) noexcept {
    switch (backend) {
    case BackendKind::null:
        return BackendCapabilityProfile{
            .backend = BackendKind::null,
            .shader_format = ShaderArtifactFormat::none,
            .headless = true,
            .native_device = false,
            .swapchain = false,
            .surface_presentation = false,
            .descriptor_sets = false,
            .pipeline_layouts = false,
            .graphics_pipelines = false,
            .explicit_resource_transitions = false,
            .explicit_copy_queues = false,
            .explicit_queue_family_selection = false,
            .offline_shader_artifacts_required = false,
        };
    case BackendKind::d3d12:
        return BackendCapabilityProfile{
            .backend = BackendKind::d3d12,
            .shader_format = ShaderArtifactFormat::dxil,
            .headless = false,
            .native_device = true,
            .swapchain = true,
            .surface_presentation = true,
            .descriptor_sets = true,
            .pipeline_layouts = true,
            .graphics_pipelines = true,
            .explicit_resource_transitions = true,
            .explicit_copy_queues = true,
            .explicit_queue_family_selection = false,
            .offline_shader_artifacts_required = true,
        };
    case BackendKind::vulkan:
        return BackendCapabilityProfile{
            .backend = BackendKind::vulkan,
            .shader_format = ShaderArtifactFormat::spirv,
            .headless = false,
            .native_device = true,
            .swapchain = true,
            .surface_presentation = true,
            .descriptor_sets = true,
            .pipeline_layouts = true,
            .graphics_pipelines = true,
            .explicit_resource_transitions = true,
            .explicit_copy_queues = true,
            .explicit_queue_family_selection = true,
            .offline_shader_artifacts_required = true,
        };
    case BackendKind::metal:
        return BackendCapabilityProfile{
            .backend = BackendKind::metal,
            .shader_format = ShaderArtifactFormat::metallib,
            .headless = false,
            .native_device = true,
            .swapchain = true,
            .surface_presentation = true,
            .descriptor_sets = true,
            .pipeline_layouts = true,
            .graphics_pipelines = true,
            .explicit_resource_transitions = false,
            .explicit_copy_queues = true,
            .explicit_queue_family_selection = false,
            .offline_shader_artifacts_required = true,
        };
    }
    return BackendCapabilityProfile{};
}

bool is_backend_supported_on_host(BackendKind backend, RhiHostPlatform host) noexcept {
    switch (backend) {
    case BackendKind::null:
        return true;
    case BackendKind::d3d12:
        return host == RhiHostPlatform::windows;
    case BackendKind::vulkan:
        return host == RhiHostPlatform::windows || host == RhiHostPlatform::linux || host == RhiHostPlatform::android;
    case BackendKind::metal:
        return host == RhiHostPlatform::macos || host == RhiHostPlatform::ios;
    }
    return false;
}

std::array<BackendKind, 4> preferred_backend_order(RhiHostPlatform host) noexcept {
    switch (host) {
    case RhiHostPlatform::windows:
        return {BackendKind::d3d12, BackendKind::vulkan, BackendKind::metal, BackendKind::null};
    case RhiHostPlatform::macos:
    case RhiHostPlatform::ios:
        return {BackendKind::metal, BackendKind::vulkan, BackendKind::d3d12, BackendKind::null};
    case RhiHostPlatform::linux:
    case RhiHostPlatform::android:
    case RhiHostPlatform::unknown:
        return {BackendKind::vulkan, BackendKind::d3d12, BackendKind::metal, BackendKind::null};
    }
    return {BackendKind::vulkan, BackendKind::d3d12, BackendKind::metal, BackendKind::null};
}

BackendProbePlan backend_probe_plan(BackendKind backend) noexcept {
    switch (backend) {
    case BackendKind::null:
        return BackendProbePlan{
            .backend = BackendKind::null,
            .steps = {BackendProbeStep::validate_host},
            .step_count = 1,
        };
    case BackendKind::d3d12:
        return BackendProbePlan{
            .backend = BackendKind::d3d12,
            .steps = {BackendProbeStep::validate_host, BackendProbeStep::load_runtime,
                      BackendProbeStep::create_dxgi_factory, BackendProbeStep::enumerate_adapters,
                      BackendProbeStep::create_device, BackendProbeStep::create_command_queue,
                      BackendProbeStep::verify_shader_artifacts},
            .step_count = 7,
        };
    case BackendKind::vulkan:
        return BackendProbePlan{
            .backend = BackendKind::vulkan,
            .steps = {BackendProbeStep::validate_host, BackendProbeStep::load_runtime,
                      BackendProbeStep::enumerate_physical_devices, BackendProbeStep::query_queue_families,
                      BackendProbeStep::create_logical_device, BackendProbeStep::verify_shader_artifacts},
            .step_count = 6,
        };
    case BackendKind::metal:
        return BackendProbePlan{
            .backend = BackendKind::metal,
            .steps = {BackendProbeStep::validate_host, BackendProbeStep::load_runtime,
                      BackendProbeStep::create_default_device, BackendProbeStep::create_command_queue,
                      BackendProbeStep::verify_shader_artifacts},
            .step_count = 5,
        };
    }
    return BackendProbePlan{};
}

BackendProbeResult make_backend_probe_result(BackendKind backend, RhiHostPlatform host, BackendProbeStatus status,
                                             std::string_view diagnostic) {
    const auto message = diagnostic.empty() ? backend_probe_status_id(status) : diagnostic;
    return BackendProbeResult{
        .backend = backend,
        .host = host,
        .status = status,
        .capabilities = status == BackendProbeStatus::available ? default_backend_capabilities(backend)
                                                                : inactive_capabilities(backend),
        .diagnostic = std::string{message},
    };
}

} // namespace mirakana::rhi
