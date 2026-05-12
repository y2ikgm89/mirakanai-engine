// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace mirakana::rhi {

enum class RhiHostPlatform {
    windows,
    linux,
    android,
    macos,
    ios,
    unknown,
};

enum class ShaderArtifactFormat {
    none,
    dxil,
    spirv,
    metallib,
};

enum class BackendProbeStatus {
    unavailable,
    available,
    unsupported_host,
    missing_runtime,
    missing_shader_artifacts,
    no_suitable_device,
    no_present_queue,
};

enum class BackendProbeStep {
    validate_host,
    load_runtime,
    create_dxgi_factory,
    enumerate_adapters,
    enumerate_physical_devices,
    query_queue_families,
    create_device,
    create_logical_device,
    create_default_device,
    create_command_queue,
    verify_shader_artifacts,
};

struct BackendCapabilityProfile {
    BackendKind backend{BackendKind::null};
    ShaderArtifactFormat shader_format{ShaderArtifactFormat::none};
    bool headless{false};
    bool native_device{false};
    bool swapchain{false};
    bool surface_presentation{false};
    bool descriptor_sets{false};
    bool pipeline_layouts{false};
    bool graphics_pipelines{false};
    bool explicit_resource_transitions{false};
    bool explicit_copy_queues{false};
    bool explicit_queue_family_selection{false};
    bool offline_shader_artifacts_required{false};
};

struct BackendProbePlan {
    BackendKind backend{BackendKind::null};
    std::array<BackendProbeStep, 8> steps{};
    std::size_t step_count{0};
};

struct BackendProbeResult {
    BackendKind backend{BackendKind::null};
    RhiHostPlatform host{RhiHostPlatform::unknown};
    BackendProbeStatus status{BackendProbeStatus::unavailable};
    BackendCapabilityProfile capabilities{};
    std::string diagnostic;
};

[[nodiscard]] std::string_view backend_kind_id(BackendKind backend) noexcept;
[[nodiscard]] std::string_view rhi_host_platform_id(RhiHostPlatform host) noexcept;
[[nodiscard]] std::string_view shader_artifact_format_id(ShaderArtifactFormat format) noexcept;
[[nodiscard]] std::string_view backend_probe_status_id(BackendProbeStatus status) noexcept;
[[nodiscard]] std::string_view backend_probe_step_id(BackendProbeStep step) noexcept;

[[nodiscard]] RhiHostPlatform current_rhi_host_platform() noexcept;
[[nodiscard]] BackendCapabilityProfile default_backend_capabilities(BackendKind backend) noexcept;
[[nodiscard]] bool is_backend_supported_on_host(BackendKind backend, RhiHostPlatform host) noexcept;
[[nodiscard]] std::array<BackendKind, 4> preferred_backend_order(RhiHostPlatform host) noexcept;
[[nodiscard]] BackendProbePlan backend_probe_plan(BackendKind backend) noexcept;
[[nodiscard]] BackendProbeResult make_backend_probe_result(BackendKind backend, RhiHostPlatform host,
                                                           BackendProbeStatus status, std::string_view diagnostic = {});

} // namespace mirakana::rhi
