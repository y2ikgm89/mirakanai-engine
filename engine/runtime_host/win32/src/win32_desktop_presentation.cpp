// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"

#include "scene_gpu_binding_injecting_renderer.hpp"

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/renderer/cloud_layer_policy.hpp"
#include "mirakana/renderer/debug_profiling_policy.hpp"
#include "mirakana/renderer/environment_fog_policy.hpp"
#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/renderer/physical_sky_policy.hpp"
#include "mirakana/renderer/postprocess_policy.hpp"
#include "mirakana/renderer/precipitation_policy.hpp"
#include "mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/renderer/scene_scale_policy.hpp"
#include "mirakana/renderer/volumetric_cloud_policy.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#endif

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN)
#include "mirakana/rhi/vulkan/vulkan_backend.hpp"
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {
using runtime_host_win32_detail::SceneComputeMorphMeshBinding;
using runtime_host_win32_detail::SceneGpuBindingInjectingRenderer;

constexpr std::uint32_t kVulkanPrecipitationDescriptorSetBindings{4};
constexpr std::uint32_t kVulkanPrecipitationSamplerBindingShift{20};
constexpr std::uint32_t kVulkanPrecipitationConstantsBindingShift{40};
constexpr std::uint32_t kVulkanVolumetricFogDescriptorSetBindings{4};
constexpr std::uint32_t kVulkanVolumetricCloudDescriptorSetBindings{5};
constexpr std::uint32_t kVulkanVolumetricCloudSamplerBindingShift{20};
constexpr std::uint32_t kVulkanVolumetricCloudConstantsBindingShift{40};

struct SurfaceProbe {
    rhi::SurfaceHandle surface;
    Win32DesktopPresentationFallbackReason failure_reason{Win32DesktopPresentationFallbackReason::none};
    std::string diagnostic;
};

struct NativeRendererCreateResult {
    bool succeeded{false};
    Win32DesktopPresentationFallbackReason failure_reason{Win32DesktopPresentationFallbackReason::none};
    std::string diagnostic;
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    Win32D3d12SwapChainPlan swapchain_plan;
    Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    std::vector<Win32DesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    Win32DesktopPresentationPostprocessStatus postprocess_status{
        Win32DesktopPresentationPostprocessStatus::not_requested};
    std::vector<Win32DesktopPresentationPostprocessDiagnostic> postprocess_diagnostics;
    bool postprocess_depth_input_requested{false};
    bool postprocess_depth_input_ready{false};
    bool environment_fog_requested{false};
    bool environment_fog_constant_buffer_ready{false};
    std::uint64_t environment_fog_constant_buffer_bytes{0};
    bool environment_fog_vulkan_package_requested{false};
    bool environment_fog_vulkan_package_shader_contract_evidence_ready{false};
    bool environment_fog_vulkan_package_evidence_ready{false};
    bool environment_fog_vulkan_package_constant_buffer_ready{false};
    std::uint64_t environment_fog_vulkan_package_constant_buffer_bytes{0};
    bool physical_sky_vulkan_package_requested{false};
    bool physical_sky_vulkan_package_shader_contract_evidence_ready{false};
    bool physical_sky_vulkan_package_evidence_ready{false};
    bool physical_sky_vulkan_package_execution_evidence_ready{false};
    bool physical_sky_vulkan_package_constant_buffer_ready{false};
    std::uint64_t physical_sky_vulkan_package_constant_buffer_bytes{0};
    bool physical_sky_vulkan_package_allocates_lut_textures{false};
    bool physical_sky_vulkan_package_invokes_backend{false};
    bool physical_sky_vulkan_package_exposes_native_handles{false};
    std::uint32_t physical_sky_vulkan_package_constant_layout_rows{0};
    std::uint32_t physical_sky_vulkan_package_lut_intent_rows{0};
    std::uint32_t physical_sky_vulkan_package_policy_diagnostics_count{0};
    bool physical_sky_requested{false};
    bool physical_sky_shader_contract_evidence_ready{false};
    bool physical_sky_package_evidence_ready{false};
    bool physical_sky_execution_evidence_ready{false};
    bool physical_sky_constant_buffer_ready{false};
    std::uint64_t physical_sky_constant_buffer_bytes{0};
    bool physical_sky_allocates_lut_textures{false};
    bool physical_sky_invokes_backend{false};
    bool physical_sky_exposes_native_handles{false};
    std::uint32_t physical_sky_constant_layout_rows{0};
    std::uint32_t physical_sky_lut_intent_rows{0};
    std::uint32_t physical_sky_policy_diagnostics_count{0};
    bool cloud_layer_requested{false};
    bool cloud_layer_shader_contract_evidence_ready{false};
    bool cloud_layer_package_evidence_ready{false};
    bool cloud_layer_execution_evidence_ready{false};
    bool cloud_layer_uploads_textures{false};
    bool cloud_layer_invokes_backend{false};
    bool cloud_layer_exposes_native_handles{false};
    bool cloud_layer_uses_volumetric_clouds{false};
    bool cloud_layer_uses_latlong_projection{false};
    bool cloud_layer_uses_flow_map{false};
    std::uint32_t cloud_layer_texture_rows{0};
    std::uint32_t cloud_layer_visual_rows{0};
    std::uint32_t cloud_layer_ibl_rows{0};
    std::uint32_t cloud_layer_shader_contract_rows{0};
    std::uint32_t cloud_layer_quality_rows{0};
    std::uint32_t cloud_layer_policy_diagnostics_count{0};
    std::uint64_t cloud_layer_renderer_draws{0};
    bool environment_precipitation_requested{false};
    EnvironmentWeatherKind environment_precipitation_weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind environment_precipitation_kind{EnvironmentPrecipitationKind::none};
    bool environment_precipitation_shader_contract_evidence_ready{false};
    bool environment_precipitation_package_evidence_ready{false};
    bool environment_precipitation_execution_evidence_ready{false};
    bool environment_precipitation_uploads_particle_buffers{false};
    bool environment_precipitation_invokes_backend{false};
    bool environment_precipitation_exposes_native_handles{false};
    bool environment_precipitation_mutates_materials{false};
    bool environment_precipitation_plays_audio{false};
    std::uint64_t environment_precipitation_renderer_draws{0};
    bool environment_precipitation_depth_occlusion_readback{false};
    bool environment_precipitation_uses_camera_near_particles{false};
    bool environment_precipitation_uses_scene_depth_occlusion{false};
    std::uint32_t environment_precipitation_weather_rows{0};
    std::uint32_t environment_precipitation_particle_rows{0};
    std::uint32_t environment_precipitation_occlusion_rows{0};
    std::uint32_t environment_precipitation_wetness_rows{0};
    std::uint32_t environment_precipitation_audio_handoff_rows{0};
    std::uint32_t environment_precipitation_shader_rows{0};
    std::uint32_t environment_precipitation_quality_rows{0};
    std::uint32_t environment_precipitation_policy_diagnostics_count{0};
    bool environment_precipitation_vulkan_requested{false};
    EnvironmentWeatherKind environment_precipitation_vulkan_weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind environment_precipitation_vulkan_kind{EnvironmentPrecipitationKind::none};
    bool environment_precipitation_vulkan_shader_contract_evidence_ready{false};
    bool environment_precipitation_vulkan_package_evidence_ready{false};
    bool environment_precipitation_vulkan_execution_evidence_ready{false};
    bool environment_precipitation_vulkan_uses_camera_near_particles{false};
    bool environment_precipitation_vulkan_uses_scene_depth_occlusion{false};
    std::uint32_t environment_precipitation_vulkan_weather_rows{0};
    std::uint32_t environment_precipitation_vulkan_particle_rows{0};
    std::uint32_t environment_precipitation_vulkan_occlusion_rows{0};
    std::uint32_t environment_precipitation_vulkan_wetness_rows{0};
    std::uint32_t environment_precipitation_vulkan_audio_handoff_rows{0};
    std::uint32_t environment_precipitation_vulkan_shader_rows{0};
    std::uint32_t environment_precipitation_vulkan_quality_rows{0};
    std::uint64_t environment_precipitation_vulkan_particle_buffer_uploads{0};
    std::uint64_t environment_precipitation_vulkan_backend_invocations{0};
    std::uint64_t environment_precipitation_vulkan_renderer_draws{0};
    bool environment_precipitation_vulkan_depth_occlusion_readback{false};
    std::uint32_t environment_precipitation_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_precipitation_vulkan_synchronization2_barriers{0};
    bool environment_precipitation_vulkan_exposes_native_handles{false};
    bool environment_precipitation_vulkan_mutates_materials{false};
    bool environment_precipitation_vulkan_plays_audio{false};
    std::uint32_t environment_precipitation_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_fog_requested{false};
    bool environment_volumetric_fog_shader_contract_evidence_ready{false};
    bool environment_volumetric_fog_package_evidence_ready{false};
    bool environment_volumetric_fog_execution_evidence_ready{false};
    bool environment_volumetric_fog_froxel_output_ready{false};
    bool environment_volumetric_fog_scene_depth_ready{false};
    std::uint64_t environment_volumetric_fog_compute_dispatches{0};
    bool environment_volumetric_fog_exposes_native_handles{false};
    std::uint32_t environment_volumetric_fog_policy_diagnostics_count{0};
    bool environment_volumetric_fog_vulkan_requested{false};
    bool environment_volumetric_fog_vulkan_shader_contract_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_package_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_execution_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_froxel_output_ready{false};
    bool environment_volumetric_fog_vulkan_scene_depth_ready{false};
    std::uint64_t environment_volumetric_fog_vulkan_compute_dispatches{0};
    std::uint32_t environment_volumetric_fog_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_volumetric_fog_vulkan_synchronization2_barriers{0};
    bool environment_volumetric_fog_vulkan_froxel_readback_nonzero{false};
    bool environment_volumetric_fog_vulkan_exposes_native_handles{false};
    std::uint32_t environment_volumetric_fog_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_cloud_vulkan_requested{false};
    bool environment_volumetric_cloud_vulkan_shader_contract_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_package_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_execution_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_weather_map_ready{false};
    bool environment_volumetric_cloud_vulkan_shape_noise_ready{false};
    bool environment_volumetric_cloud_vulkan_erosion_noise_ready{false};
    bool environment_volumetric_cloud_vulkan_uploads_volume_textures{false};
    bool environment_volumetric_cloud_vulkan_invokes_backend{false};
    std::uint64_t environment_volumetric_cloud_vulkan_renderer_draws{0};
    std::uint64_t environment_volumetric_cloud_vulkan_raymarch_passes{0};
    std::uint32_t environment_volumetric_cloud_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_volumetric_cloud_vulkan_synchronization2_barriers{0};
    bool environment_volumetric_cloud_vulkan_readback_nonzero{false};
    bool environment_volumetric_cloud_vulkan_exposes_native_handles{false};
    bool environment_volumetric_cloud_vulkan_plays_audio{false};
    bool environment_volumetric_cloud_vulkan_executes_precipitation{false};
    std::uint32_t environment_volumetric_cloud_vulkan_map_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_layer_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_lighting_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_raymarch_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_temporal_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_shadow_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_storm_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_shader_contract_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_quality_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_cloud_requested{false};
    bool environment_volumetric_cloud_shader_contract_evidence_ready{false};
    bool environment_volumetric_cloud_package_evidence_ready{false};
    bool environment_volumetric_cloud_execution_evidence_ready{false};
    bool environment_volumetric_cloud_weather_map_ready{false};
    bool environment_volumetric_cloud_shape_noise_ready{false};
    bool environment_volumetric_cloud_erosion_noise_ready{false};
    bool environment_volumetric_cloud_uploads_volume_textures{false};
    bool environment_volumetric_cloud_invokes_backend{false};
    std::uint64_t environment_volumetric_cloud_renderer_draws{0};
    std::uint64_t environment_volumetric_cloud_raymarch_passes{0};
    bool environment_volumetric_cloud_readback_nonzero{false};
    bool environment_volumetric_cloud_exposes_native_handles{false};
    bool environment_volumetric_cloud_plays_audio{false};
    bool environment_volumetric_cloud_executes_precipitation{false};
    std::uint32_t environment_volumetric_cloud_map_rows{0};
    std::uint32_t environment_volumetric_cloud_layer_rows{0};
    std::uint32_t environment_volumetric_cloud_lighting_rows{0};
    std::uint32_t environment_volumetric_cloud_raymarch_rows{0};
    std::uint32_t environment_volumetric_cloud_temporal_rows{0};
    std::uint32_t environment_volumetric_cloud_shadow_rows{0};
    std::uint32_t environment_volumetric_cloud_storm_rows{0};
    std::uint32_t environment_volumetric_cloud_shader_contract_rows{0};
    std::uint32_t environment_volumetric_cloud_quality_rows{0};
    std::uint32_t environment_volumetric_cloud_policy_diagnostics_count{0};
    Win32DesktopPresentationDirectionalShadowStatus directional_shadow_status{
        Win32DesktopPresentationDirectionalShadowStatus::not_requested};
    std::vector<Win32DesktopPresentationDirectionalShadowDiagnostic> directional_shadow_diagnostics;
    bool directional_shadow_requested{false};
    bool directional_shadow_ready{false};
    Win32DesktopPresentationDirectionalShadowFilterMode directional_shadow_filter_mode{
        Win32DesktopPresentationDirectionalShadowFilterMode::none};
    std::uint32_t directional_shadow_filter_tap_count{0};
    float directional_shadow_filter_radius_texels{0.0F};
    std::uint32_t directional_shadow_cascade_count{0};
    std::uint32_t directional_shadow_cascade_tile_width{0};
    std::uint32_t directional_shadow_atlas_width{0};
    std::uint32_t directional_shadow_atlas_height{0};
    std::uint32_t directional_shadow_light_space_cascades{0};
    std::uint32_t directional_shadow_cascade_splits{0};
    Win32DesktopPresentationNativeUiOverlayStatus native_ui_overlay_status{
        Win32DesktopPresentationNativeUiOverlayStatus::not_requested};
    std::vector<Win32DesktopPresentationNativeUiOverlayDiagnostic> native_ui_overlay_diagnostics;
    bool native_ui_overlay_requested{false};
    bool native_ui_overlay_ready{false};
    Win32DesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        Win32DesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    std::vector<Win32DesktopPresentationNativeUiTextureOverlayDiagnostic> native_ui_texture_overlay_diagnostics;
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::size_t framegraph_passes{0};
    class SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};
};

struct SceneRendererRequestValidation {
    bool valid{false};
    std::string diagnostic;
};

struct CloudLayerRuntimeBinding {
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    rhi::PipelineLayoutHandle pipeline_layout;
    rhi::DescriptorSetHandle descriptor_set;
    rhi::GraphicsPipelineHandle pipeline;
    std::uint64_t texture_uploads{0};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty() && descriptor_set_layout.value != 0 && pipeline_layout.value != 0 &&
               descriptor_set.value != 0 && pipeline.value != 0;
    }
};

struct PrecipitationRuntimeProbeResult {
    std::uint64_t particle_buffer_uploads{0};
    std::uint64_t backend_invocations{0};
    std::uint64_t renderer_draws{0};
    std::uint32_t synchronization2_barriers{0};
    bool depth_occlusion_readback{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty() && particle_buffer_uploads > 0 && backend_invocations > 0 && renderer_draws > 0 &&
               depth_occlusion_readback;
    }
};

struct VolumetricCloudRuntimeProbeResult {
    std::uint64_t weather_map_uploads{0};
    std::uint64_t shape_noise_uploads{0};
    std::uint64_t erosion_noise_uploads{0};
    std::uint64_t backend_invocations{0};
    std::uint64_t renderer_draws{0};
    std::uint64_t raymarch_passes{0};
    std::uint32_t descriptor_set_bindings{0};
    std::uint32_t synchronization2_barriers{0};
    bool readback_nonzero{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty() && weather_map_uploads > 0 && shape_noise_uploads > 0 && erosion_noise_uploads > 0 &&
               backend_invocations > 0 && renderer_draws > 0 && raymarch_passes > 0 && readback_nonzero;
    }
};

struct VolumetricCloudRuntimeProbeBindings {
    std::uint32_t weather_map_binding{0};
    std::uint32_t shape_noise_binding{0};
    std::uint32_t erosion_noise_binding{0};
    std::uint32_t sampler_binding{0};
    std::uint32_t constants_binding{0};
    bool transition_upload_textures_from_undefined{false};
};

[[nodiscard]] constexpr VolumetricCloudRuntimeProbeBindings d3d12_volumetric_cloud_probe_bindings() noexcept {
    return VolumetricCloudRuntimeProbeBindings{
        .weather_map_binding = volumetric_cloud_weather_map_binding(),
        .shape_noise_binding = volumetric_cloud_shape_noise_binding(),
        .erosion_noise_binding = volumetric_cloud_erosion_noise_binding(),
        .sampler_binding = volumetric_cloud_sampler_binding(),
        .constants_binding = volumetric_cloud_constants_binding(),
        .transition_upload_textures_from_undefined = false,
    };
}

[[nodiscard]] constexpr VolumetricCloudRuntimeProbeBindings vulkan_volumetric_cloud_probe_bindings() noexcept {
    return VolumetricCloudRuntimeProbeBindings{
        .weather_map_binding = volumetric_cloud_weather_map_binding(),
        .shape_noise_binding = volumetric_cloud_shape_noise_binding(),
        .erosion_noise_binding = volumetric_cloud_erosion_noise_binding(),
        .sampler_binding = volumetric_cloud_sampler_binding() + kVulkanVolumetricCloudSamplerBindingShift,
        .constants_binding = volumetric_cloud_constants_binding() + kVulkanVolumetricCloudConstantsBindingShift,
        .transition_upload_textures_from_undefined = true,
    };
}

struct VolumetricFogRuntimeProbeResult {
    std::uint64_t compute_dispatches{0};
    std::uint32_t descriptor_set_bindings{0};
    std::uint32_t synchronization2_barriers{0};
    bool scene_depth_ready{false};
    bool froxel_output_ready{false};
    bool readback_nonzero{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty() && compute_dispatches > 0 &&
               descriptor_set_bindings == kVulkanVolumetricFogDescriptorSetBindings && synchronization2_barriers > 0 &&
               scene_depth_ready && froxel_output_ready && readback_nonzero;
    }
};

[[nodiscard]] Win32DesktopPresentationDirectionalShadowDiagnostic
make_directional_shadow_diagnostic(Win32DesktopPresentationDirectionalShadowStatus status, std::string message);
[[nodiscard]] Win32DesktopPresentationNativeUiOverlayDiagnostic
make_native_ui_overlay_diagnostic(Win32DesktopPresentationNativeUiOverlayStatus status, std::string message);
[[nodiscard]] Win32DesktopPresentationNativeUiTextureOverlayDiagnostic
make_native_ui_texture_overlay_diagnostic(Win32DesktopPresentationNativeUiTextureOverlayStatus status,
                                          std::string message);

[[nodiscard]] bool has_extent(Extent2D extent) noexcept {
    return extent.width != 0 && extent.height != 0;
}

[[nodiscard]] bool valid_swapchain_format(rhi::Format format) noexcept {
    return format == rhi::Format::rgba8_unorm || format == rhi::Format::bgra8_unorm;
}

[[nodiscard]] Win32DesktopPresentationDirectionalShadowFilterMode
to_presentation_filter_mode(ShadowReceiverFilterMode mode) noexcept {
    switch (mode) {
    case ShadowReceiverFilterMode::none:
        return Win32DesktopPresentationDirectionalShadowFilterMode::none;
    case ShadowReceiverFilterMode::fixed_pcf_3x3:
        return Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3;
    }
    return Win32DesktopPresentationDirectionalShadowFilterMode::none;
}

[[nodiscard]] bool has_shader_bytecode(const Win32DesktopPresentationShaderBytecode& shader) noexcept {
    return !shader.entry_point.empty() && !shader.bytecode.empty();
}

[[nodiscard]] rhi::BufferHandle create_environment_fog_constants_buffer(rhi::IRhiDevice& device,
                                                                        const EnvironmentFogPolicyDesc& desc) {
    std::array<std::uint8_t, environment_fog_constants_byte_size()> constants{};
    pack_environment_fog_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

[[nodiscard]] rhi::BufferHandle create_physical_sky_constants_buffer(rhi::IRhiDevice& device,
                                                                     const PhysicalSkyPolicyDesc& desc) {
    std::array<std::uint8_t, physical_sky_constants_byte_size()> constants{};
    pack_physical_sky_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

[[nodiscard]] rhi::BufferHandle create_cloud_layer_constants_buffer(rhi::IRhiDevice& device,
                                                                    const CloudLayerPolicyDesc& desc) {
    std::array<std::uint8_t, cloud_layer_constants_byte_size()> constants{};
    pack_cloud_layer_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

[[nodiscard]] rhi::BufferHandle create_precipitation_constants_buffer(rhi::IRhiDevice& device,
                                                                      const PrecipitationPolicyDesc& desc) {
    std::array<std::uint8_t, precipitation_constants_byte_size()> constants{};
    pack_precipitation_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

[[nodiscard]] rhi::BufferHandle create_volumetric_fog_constants_buffer(rhi::IRhiDevice& device,
                                                                       const VolumetricFogPolicyDesc& desc) {
    std::array<std::uint8_t, volumetric_fog_constants_byte_size()> constants{};
    pack_volumetric_fog_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

[[nodiscard]] rhi::BufferHandle create_volumetric_cloud_constants_buffer(rhi::IRhiDevice& device,
                                                                         const VolumetricCloudPolicyDesc& desc) {
    std::array<std::uint8_t, volumetric_cloud_constants_byte_size()> constants{};
    pack_volumetric_cloud_constants(constants, desc);
    auto buffer = device.create_buffer(rhi::BufferDesc{
        .size_bytes = static_cast<std::uint64_t>(constants.size()),
        .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
    });
    if (buffer.value != 0) {
        device.write_buffer(buffer, 0, std::span<const std::uint8_t>{constants.data(), constants.size()});
    }
    return buffer;
}

void append_f32_le(std::vector<std::uint8_t>& bytes, float value) {
    std::array<std::uint8_t, sizeof(float)> storage{};
    std::memcpy(storage.data(), &value, sizeof(float));
    bytes.insert(bytes.end(), storage.begin(), storage.end());
}

[[nodiscard]] runtime_rhi::RuntimeTextureUploadResult
upload_runtime_texture_asset(rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package,
                             std::string_view asset_ref, std::string_view backend_name, std::string_view feature_name) {
    const auto* record = package.find(asset_id_from_key_v2(AssetKeyV2{.value = std::string{asset_ref}}));
    if (record == nullptr) {
        return runtime_rhi::RuntimeTextureUploadResult{
            .diagnostic = std::string{backend_name} + " " + std::string{feature_name} +
                          " texture asset is missing: " + std::string{asset_ref},
        };
    }
    if (record->kind != AssetKind::texture) {
        return runtime_rhi::RuntimeTextureUploadResult{
            .diagnostic = std::string{backend_name} + " " + std::string{feature_name} +
                          " asset is not a cooked texture payload: " + std::string{asset_ref},
        };
    }

    const auto payload = runtime::runtime_texture_payload(*record);
    if (!payload.succeeded()) {
        return runtime_rhi::RuntimeTextureUploadResult{
            .diagnostic = std::string{backend_name} + " " + std::string{feature_name} +
                          " texture payload is invalid: " + payload.diagnostic,
        };
    }

    auto upload = runtime_rhi::upload_runtime_texture(device, payload.payload);
    if (!upload.succeeded()) {
        upload.diagnostic = std::string{backend_name} + " " + std::string{feature_name} +
                            " texture upload failed: " + upload.diagnostic;
    }
    return upload;
}

[[nodiscard]] runtime_rhi::RuntimeTextureUploadResult
upload_cloud_layer_texture(rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package,
                           std::string_view asset_ref, std::string_view backend_name) {
    return upload_runtime_texture_asset(device, package, asset_ref, backend_name, "cloud layer");
}

[[nodiscard]] std::uint64_t volumetric_fog_output_byte_size(const VolumetricFogFroxelGridDesc& grid) noexcept {
    const auto voxel_count = static_cast<std::uint64_t>(grid.width) * static_cast<std::uint64_t>(grid.height) *
                             static_cast<std::uint64_t>(grid.depth_slices);
    return voxel_count * sizeof(std::uint32_t);
}

[[nodiscard]] std::uint32_t div_round_up(std::uint32_t value, std::uint32_t divisor) noexcept {
    return divisor == 0 ? 0 : (value + divisor - 1U) / divisor;
}

[[nodiscard]] CloudLayerRuntimeBinding
create_cloud_layer_runtime_binding(rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package,
                                   const CloudLayerPolicyDesc& desc,
                                   const Win32DesktopPresentationShaderBytecode& vertex_shader_bytecode,
                                   const Win32DesktopPresentationShaderBytecode& fragment_shader_bytecode,
                                   rhi::Format color_format, rhi::Format depth_format, std::string_view backend_name) {
    try {
        if (!has_shader_bytecode(vertex_shader_bytecode) || !has_shader_bytecode(fragment_shader_bytecode)) {
            return CloudLayerRuntimeBinding{
                .diagnostic = std::string{backend_name} +
                              " cloud layer renderer execution requires vertex and fragment shader bytecode",
            };
        }

        const auto cloud_upload =
            upload_cloud_layer_texture(device, package, desc.layer.cloud_map_asset_ref, backend_name);
        if (!cloud_upload.succeeded() || cloud_upload.texture.value == 0) {
            return CloudLayerRuntimeBinding{.diagnostic = cloud_upload.diagnostic};
        }
        const auto flow_upload =
            upload_cloud_layer_texture(device, package, desc.layer.flow_map_asset_ref, backend_name);
        if (!flow_upload.succeeded() || flow_upload.texture.value == 0) {
            return CloudLayerRuntimeBinding{.diagnostic = flow_upload.diagnostic};
        }

        const auto constants = create_cloud_layer_constants_buffer(device, desc);
        if (constants.value == 0) {
            return CloudLayerRuntimeBinding{
                .diagnostic = std::string{backend_name} + " cloud layer constants buffer creation failed",
            };
        }

        const auto sampler = device.create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::linear,
            .mag_filter = rhi::SamplerFilter::linear,
            .address_u = rhi::SamplerAddressMode::repeat,
            .address_v = rhi::SamplerAddressMode::repeat,
            .address_w = rhi::SamplerAddressMode::repeat,
        });
        if (sampler.value == 0) {
            return CloudLayerRuntimeBinding{
                .diagnostic = std::string{backend_name} + " cloud layer sampler creation failed",
            };
        }

        const auto descriptor_set_layout = device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = cloud_layer_cloud_map_binding(),
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = cloud_layer_flow_map_binding(),
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = cloud_layer_sampler_binding(),
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = cloud_layer_constants_binding(),
                .type = rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
        }});
        const auto descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = cloud_layer_cloud_map_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, cloud_upload.texture)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = cloud_layer_flow_map_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, flow_upload.texture)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = cloud_layer_sampler_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::sampler(sampler)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = cloud_layer_constants_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer, constants)},
        });

        const auto pipeline_layout = device.create_pipeline_layout(
            rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_set_layout}, .push_constant_bytes = 0});
        const auto vertex_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = vertex_shader_bytecode.entry_point,
            .bytecode_size = vertex_shader_bytecode.bytecode.size(),
            .bytecode = vertex_shader_bytecode.bytecode.data(),
        });
        const auto fragment_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = fragment_shader_bytecode.entry_point,
            .bytecode_size = fragment_shader_bytecode.bytecode.size(),
            .bytecode = fragment_shader_bytecode.bytecode.data(),
        });
        const auto pipeline = device.create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = color_format,
            .depth_format = depth_format,
            .topology = rhi::PrimitiveTopology::triangle_list,
        });

        return CloudLayerRuntimeBinding{
            .descriptor_set_layout = descriptor_set_layout,
            .pipeline_layout = pipeline_layout,
            .descriptor_set = descriptor_set,
            .pipeline = pipeline,
            .texture_uploads = (cloud_upload.copy_recorded ? 1U : 0U) + (flow_upload.copy_recorded ? 1U : 0U),
            .diagnostic = {},
        };
    } catch (const std::exception& exception) {
        return CloudLayerRuntimeBinding{
            .diagnostic =
                std::string{backend_name} + " cloud layer runtime binding creation failed: " + exception.what(),
        };
    }
}

[[nodiscard]] VolumetricFogRuntimeProbeResult
execute_volumetric_fog_runtime_probe(rhi::IRhiDevice& device, const VolumetricFogPolicyDesc& desc,
                                     const Win32DesktopPresentationShaderBytecode& compute_shader_bytecode,
                                     std::string_view backend_name) {
    try {
        if (!has_shader_bytecode(compute_shader_bytecode)) {
            return VolumetricFogRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric fog renderer execution requires compute SPIR-V",
            };
        }
        const auto output_byte_size = volumetric_fog_output_byte_size(desc.froxel_grid);
        if (output_byte_size == 0) {
            return VolumetricFogRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric fog froxel output size is zero",
            };
        }

        const auto scene_color = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::render_target,
        });
        const auto scene_depth = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::depth24_stencil8,
            .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
        });
        const auto constants = create_volumetric_fog_constants_buffer(device, desc);
        const auto froxel_output = device.create_buffer(rhi::BufferDesc{
            .size_bytes = output_byte_size,
            .usage = rhi::BufferUsage::storage | rhi::BufferUsage::copy_source,
        });
        const auto readback = device.create_buffer(rhi::BufferDesc{
            .size_bytes = output_byte_size,
            .usage = rhi::BufferUsage::copy_destination,
        });
        const auto sampler = device.create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::nearest,
            .mag_filter = rhi::SamplerFilter::nearest,
            .address_u = rhi::SamplerAddressMode::clamp_to_edge,
            .address_v = rhi::SamplerAddressMode::clamp_to_edge,
            .address_w = rhi::SamplerAddressMode::clamp_to_edge,
        });
        if (scene_color.value == 0 || scene_depth.value == 0 || constants.value == 0 || froxel_output.value == 0 ||
            readback.value == 0 || sampler.value == 0) {
            return VolumetricFogRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric fog probe resource creation failed",
            };
        }

        auto depth_commands = device.begin_command_list(rhi::QueueKind::graphics);
        depth_commands->transition_texture(scene_color, rhi::ResourceState::undefined,
                                           rhi::ResourceState::render_target);
        depth_commands->transition_texture(scene_depth, rhi::ResourceState::undefined, rhi::ResourceState::depth_write);
        depth_commands->begin_render_pass(rhi::RenderPassDesc{
            .color =
                rhi::RenderPassColorAttachment{
                    .texture = scene_color,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .swapchain_frame = rhi::SwapchainFrameHandle{},
                    .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                },
            .depth =
                rhi::RenderPassDepthAttachment{
                    .texture = scene_depth,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .clear_depth = rhi::ClearDepthValue{1.0F},
                },
        });
        depth_commands->end_render_pass();
        depth_commands->transition_texture(scene_depth, rhi::ResourceState::depth_write,
                                           rhi::ResourceState::shader_read);
        depth_commands->close();
        const auto depth_fence = device.submit(*depth_commands);
        device.wait(depth_fence);

        const auto descriptor_set_layout = device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = volumetric_fog_scene_depth_texture_binding(),
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::compute,
            },
            rhi::DescriptorBindingDesc{
                .binding = volumetric_fog_scene_depth_sampler_binding(),
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::compute,
            },
            rhi::DescriptorBindingDesc{
                .binding = volumetric_fog_constants_binding(),
                .type = rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::compute,
            },
            rhi::DescriptorBindingDesc{
                .binding = volumetric_fog_froxel_output_buffer_binding(),
                .type = rhi::DescriptorType::storage_buffer,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::compute,
            },
        }});
        const auto descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = volumetric_fog_scene_depth_texture_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, scene_depth)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = volumetric_fog_scene_depth_sampler_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::sampler(sampler)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = volumetric_fog_constants_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer, constants)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = volumetric_fog_froxel_output_buffer_binding(),
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer, froxel_output)},
        });
        const auto pipeline_layout =
            device.create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_set_layout}});
        const auto compute_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::compute,
            .entry_point = compute_shader_bytecode.entry_point,
            .bytecode_size = compute_shader_bytecode.bytecode.size(),
            .bytecode = compute_shader_bytecode.bytecode.data(),
        });
        const auto pipeline = device.create_compute_pipeline(rhi::ComputePipelineDesc{
            .layout = pipeline_layout,
            .compute_shader = compute_shader,
        });
        if (descriptor_set_layout.value == 0 || descriptor_set.value == 0 || pipeline_layout.value == 0 ||
            compute_shader.value == 0 || pipeline.value == 0) {
            return VolumetricFogRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric fog probe pipeline creation failed",
            };
        }

        const auto before_stats = device.stats();
        auto compute_commands = device.begin_command_list(rhi::QueueKind::compute);
        compute_commands->bind_compute_pipeline(pipeline);
        compute_commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
        compute_commands->dispatch(div_round_up(desc.froxel_grid.width, 4U), div_round_up(desc.froxel_grid.height, 4U),
                                   div_round_up(desc.froxel_grid.depth_slices, 4U));
        // Wildcard aliasing barriers are the current backend-neutral RHI memory dependency hook;
        // Vulkan lowers this to a synchronization2 vkCmdPipelineBarrier2 memory barrier.
        compute_commands->texture_aliasing_barrier(rhi::TextureHandle{}, rhi::TextureHandle{});
        compute_commands->copy_buffer(froxel_output, readback,
                                      rhi::BufferCopyRegion{
                                          .source_offset = 0,
                                          .destination_offset = 0,
                                          .size_bytes = output_byte_size,
                                      });
        compute_commands->close();
        const auto compute_fence = device.submit(*compute_commands);
        device.wait(compute_fence);

        const auto read_size = std::min<std::uint64_t>(output_byte_size, 4096ULL);
        const auto readback_bytes = device.read_buffer(readback, 0, read_size);
        const auto readback_nonzero =
            std::ranges::any_of(readback_bytes, [](const std::uint8_t value) { return value != 0; });
        if (!readback_nonzero) {
            return VolumetricFogRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric fog probe readback stayed zero",
            };
        }

        const auto after_stats = device.stats();
        const auto compute_dispatches = after_stats.compute_dispatches > before_stats.compute_dispatches
                                            ? after_stats.compute_dispatches - before_stats.compute_dispatches
                                            : 0U;
        const auto synchronization2_barriers =
            after_stats.texture_aliasing_barriers > before_stats.texture_aliasing_barriers
                ? static_cast<std::uint32_t>(std::min<std::uint64_t>(after_stats.texture_aliasing_barriers -
                                                                         before_stats.texture_aliasing_barriers,
                                                                     std::numeric_limits<std::uint32_t>::max()))
                : 0U;
        return VolumetricFogRuntimeProbeResult{
            .compute_dispatches = compute_dispatches,
            .descriptor_set_bindings = kVulkanVolumetricFogDescriptorSetBindings,
            .synchronization2_barriers = synchronization2_barriers,
            .scene_depth_ready = true,
            .froxel_output_ready = true,
            .readback_nonzero = true,
            .diagnostic = {},
        };
    } catch (const std::exception& exception) {
        return VolumetricFogRuntimeProbeResult{
            .diagnostic = std::string{backend_name} + " volumetric fog runtime probe failed: " + exception.what(),
        };
    }
}

[[nodiscard]] PrecipitationRuntimeProbeResult execute_precipitation_runtime_probe(
    rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package, const PrecipitationPolicyDesc& desc,
    const Win32DesktopPresentationShaderBytecode& vertex_shader_bytecode,
    const Win32DesktopPresentationShaderBytecode& fragment_shader_bytecode, std::string_view backend_name) {
    try {
        if (!has_shader_bytecode(vertex_shader_bytecode) || !has_shader_bytecode(fragment_shader_bytecode)) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic = std::string{backend_name} +
                              " precipitation renderer execution requires vertex and fragment shader bytecode",
            };
        }

        const auto particle_upload = upload_runtime_texture_asset(device, package, "sample/desktop-runtime/texture",
                                                                  backend_name, "environment precipitation");
        if (!particle_upload.succeeded() || particle_upload.texture.value == 0) {
            return PrecipitationRuntimeProbeResult{.diagnostic = particle_upload.diagnostic};
        }

        const auto scene_color = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::render_target,
        });
        const auto unoccluded_depth = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::depth24_stencil8,
            .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
        });
        const auto occluded_depth = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::depth24_stencil8,
            .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
        });
        const auto unoccluded_target = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::copy_source,
        });
        const auto occluded_target = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::copy_source,
        });
        const auto constants = create_precipitation_constants_buffer(device, desc);
        const auto quad_vertices = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 6U * 16U,
            .usage = rhi::BufferUsage::vertex | rhi::BufferUsage::copy_source,
        });
        const auto instances = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 32U,
            .usage = rhi::BufferUsage::vertex | rhi::BufferUsage::copy_source,
        });
        const auto unoccluded_readback = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 256U * 64U,
            .usage = rhi::BufferUsage::copy_destination,
        });
        const auto occluded_readback = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 256U * 64U,
            .usage = rhi::BufferUsage::copy_destination,
        });
        const auto sampler = device.create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::nearest,
            .mag_filter = rhi::SamplerFilter::nearest,
            .address_u = rhi::SamplerAddressMode::clamp_to_edge,
            .address_v = rhi::SamplerAddressMode::clamp_to_edge,
            .address_w = rhi::SamplerAddressMode::clamp_to_edge,
        });
        if (scene_color.value == 0 || unoccluded_depth.value == 0 || occluded_depth.value == 0 ||
            unoccluded_target.value == 0 || occluded_target.value == 0 || constants.value == 0 ||
            quad_vertices.value == 0 || instances.value == 0 || unoccluded_readback.value == 0 ||
            occluded_readback.value == 0 || sampler.value == 0) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " precipitation probe resource creation failed",
            };
        }

        const bool use_vulkan_descriptor_binding_shift = backend_name == "Vulkan";
        const std::uint32_t particle_texture_binding = precipitation_particle_texture_binding();
        const std::uint32_t scene_depth_texture_binding = precipitation_scene_depth_texture_binding();
        const std::uint32_t sampler_binding =
            use_vulkan_descriptor_binding_shift
                ? precipitation_sampler_binding() + kVulkanPrecipitationSamplerBindingShift
                : precipitation_sampler_binding();
        const std::uint32_t constants_binding =
            use_vulkan_descriptor_binding_shift
                ? precipitation_constants_binding() + kVulkanPrecipitationConstantsBindingShift
                : precipitation_constants_binding();

        std::vector<std::uint8_t> quad_bytes;
        quad_bytes.reserve(6U * 16U);
        for (const auto& row : std::array<std::array<float, 4>, 6>{{
                 {{-1.0F, -1.0F, 0.0F, 1.0F}},
                 {{-1.0F, 1.0F, 0.0F, 0.0F}},
                 {{1.0F, -1.0F, 1.0F, 1.0F}},
                 {{1.0F, -1.0F, 1.0F, 1.0F}},
                 {{-1.0F, 1.0F, 0.0F, 0.0F}},
                 {{1.0F, 1.0F, 1.0F, 0.0F}},
             }}) {
            for (const float value : row) {
                append_f32_le(quad_bytes, value);
            }
        }
        std::vector<std::uint8_t> instance_bytes;
        instance_bytes.reserve(32U);
        for (const float value : std::array<float, 8>{0.0F, 0.0F, 0.0F, 40.0F, 0.0F, 0.0F, 0.0F, 1.0F}) {
            append_f32_le(instance_bytes, value);
        }
        device.write_buffer(quad_vertices, 0, quad_bytes);
        device.write_buffer(instances, 0, instance_bytes);

        const auto descriptor_set_layout = device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = particle_texture_binding,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = scene_depth_texture_binding,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = sampler_binding,
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = constants_binding,
                .type = rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::vertex | rhi::ShaderStageVisibility::fragment,
            },
        }});
        const auto unoccluded_descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        const auto occluded_descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        const auto update_precipitation_descriptor_set = [&](const rhi::DescriptorSetHandle set,
                                                             const rhi::TextureHandle depth_texture) {
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = particle_texture_binding,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture,
                                                               particle_upload.texture)},
            });
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = scene_depth_texture_binding,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, depth_texture)},
            });
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = sampler_binding,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::sampler(sampler)},
            });
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = constants_binding,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer, constants)},
            });
        };
        update_precipitation_descriptor_set(unoccluded_descriptor_set, unoccluded_depth);
        update_precipitation_descriptor_set(occluded_descriptor_set, occluded_depth);

        const auto pipeline_layout =
            device.create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_set_layout}});
        const auto vertex_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = vertex_shader_bytecode.entry_point,
            .bytecode_size = vertex_shader_bytecode.bytecode.size(),
            .bytecode = vertex_shader_bytecode.bytecode.data(),
        });
        const auto fragment_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = fragment_shader_bytecode.entry_point,
            .bytecode_size = fragment_shader_bytecode.bytecode.size(),
            .bytecode = fragment_shader_bytecode.bytecode.data(),
        });
        const auto pipeline = device.create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::rgba8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = rhi::PrimitiveTopology::triangle_list,
            .vertex_buffers =
                {rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 16, .input_rate = rhi::VertexInputRate::vertex},
                 rhi::VertexBufferLayoutDesc{.binding = 1, .stride = 32, .input_rate = rhi::VertexInputRate::instance}},
            .vertex_attributes = {rhi::VertexAttributeDesc{.location = 0,
                                                           .binding = 0,
                                                           .offset = 0,
                                                           .format = rhi::VertexFormat::float32x2,
                                                           .semantic = rhi::VertexSemantic::position,
                                                           .semantic_index = 0},
                                  rhi::VertexAttributeDesc{.location = 1,
                                                           .binding = 0,
                                                           .offset = 8,
                                                           .format = rhi::VertexFormat::float32x2,
                                                           .semantic = rhi::VertexSemantic::texcoord,
                                                           .semantic_index = 0},
                                  rhi::VertexAttributeDesc{.location = 2,
                                                           .binding = 1,
                                                           .offset = 0,
                                                           .format = rhi::VertexFormat::float32x4,
                                                           .semantic = rhi::VertexSemantic::texcoord,
                                                           .semantic_index = 1},
                                  rhi::VertexAttributeDesc{.location = 3,
                                                           .binding = 1,
                                                           .offset = 16,
                                                           .format = rhi::VertexFormat::float32x4,
                                                           .semantic = rhi::VertexSemantic::texcoord,
                                                           .semantic_index = 2}},
        });
        if (descriptor_set_layout.value == 0 || unoccluded_descriptor_set.value == 0 ||
            occluded_descriptor_set.value == 0 || pipeline_layout.value == 0 || vertex_shader.value == 0 ||
            fragment_shader.value == 0 || pipeline.value == 0) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " precipitation probe pipeline creation failed",
            };
        }

        const rhi::BufferTextureCopyRegion readback_region{
            .buffer_offset = 0,
            .buffer_row_length = 64,
            .buffer_image_height = 64,
            .texture_offset = rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        };

        const auto before_stats = device.stats();
        auto commands = device.begin_command_list(rhi::QueueKind::graphics);
        commands->transition_texture(scene_color, rhi::ResourceState::undefined, rhi::ResourceState::render_target);
        const auto clear_depth_for_sampling = [&](const rhi::TextureHandle depth_texture, const float depth_value) {
            commands->transition_texture(depth_texture, rhi::ResourceState::undefined, rhi::ResourceState::depth_write);
            commands->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = scene_color,
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = rhi::SwapchainFrameHandle{},
                        .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                    },
                .depth =
                    rhi::RenderPassDepthAttachment{
                        .texture = depth_texture,
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .clear_depth = rhi::ClearDepthValue{depth_value},
                    },
            });
            commands->end_render_pass();
            commands->transition_texture(depth_texture, rhi::ResourceState::depth_write,
                                         rhi::ResourceState::shader_read);
        };
        const auto render_precipitation_probe = [&](const rhi::TextureHandle target,
                                                    const rhi::DescriptorSetHandle descriptor_set) {
            const auto target_before_state =
                use_vulkan_descriptor_binding_shift ? rhi::ResourceState::undefined : rhi::ResourceState::copy_source;
            commands->transition_texture(target, target_before_state, rhi::ResourceState::render_target);
            commands->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = target,
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = rhi::SwapchainFrameHandle{},
                        .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
                    },
            });
            commands->bind_graphics_pipeline(pipeline);
            commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
            commands->bind_vertex_buffer(
                rhi::VertexBufferBinding{.buffer = quad_vertices, .offset = 0, .stride = 16, .binding = 0});
            commands->bind_vertex_buffer(
                rhi::VertexBufferBinding{.buffer = instances, .offset = 0, .stride = 32, .binding = 1});
            commands->draw(6, 1);
            commands->end_render_pass();
            commands->transition_texture(target, rhi::ResourceState::render_target, rhi::ResourceState::copy_source);
        };

        clear_depth_for_sampling(unoccluded_depth, 1.0F);
        clear_depth_for_sampling(occluded_depth, 0.0F);
        render_precipitation_probe(unoccluded_target, unoccluded_descriptor_set);
        render_precipitation_probe(occluded_target, occluded_descriptor_set);
        commands->copy_texture_to_buffer(unoccluded_target, unoccluded_readback, readback_region);
        commands->copy_texture_to_buffer(occluded_target, occluded_readback, readback_region);
        commands->close();

        const auto fence = device.submit(*commands);
        device.wait(fence);
        const auto unoccluded_bytes = device.read_buffer(unoccluded_readback, 0, 256U * 64U);
        const auto occluded_bytes = device.read_buffer(occluded_readback, 0, 256U * 64U);
        const auto center_pixel = (32U * 256U) + (32U * 4U);
        if (unoccluded_bytes.size() < center_pixel + 4U || occluded_bytes.size() < center_pixel + 4U) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " precipitation probe readback was incomplete",
            };
        }
        const auto unoccluded_alpha = unoccluded_bytes[center_pixel + 3U];
        const auto occluded_alpha = occluded_bytes[center_pixel + 3U];
        if (unoccluded_alpha < 128U ||
            (unoccluded_bytes[center_pixel] == 0U && unoccluded_bytes[center_pixel + 1U] == 0U &&
             unoccluded_bytes[center_pixel + 2U] == 0U)) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic =
                    std::string{backend_name} + " precipitation probe readback did not contain an unoccluded particle",
            };
        }
        if (occluded_alpha > 8U || occluded_alpha >= unoccluded_alpha) {
            return PrecipitationRuntimeProbeResult{
                .diagnostic =
                    std::string{backend_name} + " precipitation probe readback did not prove scene-depth occlusion",
            };
        }

        const auto after_stats = device.stats();
        const auto renderer_draws =
            after_stats.draw_calls > before_stats.draw_calls ? after_stats.draw_calls - before_stats.draw_calls : 0U;
        const auto transition_delta =
            use_vulkan_descriptor_binding_shift && after_stats.resource_transitions > before_stats.resource_transitions
                ? after_stats.resource_transitions - before_stats.resource_transitions
                : 0U;
        const auto synchronization2_barriers = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(transition_delta, std::numeric_limits<std::uint32_t>::max()));
        return PrecipitationRuntimeProbeResult{
            .particle_buffer_uploads = 1U,
            .backend_invocations = 1U,
            .renderer_draws = renderer_draws,
            .synchronization2_barriers = synchronization2_barriers,
            .depth_occlusion_readback = true,
            .diagnostic = {},
        };
    } catch (const std::exception& exception) {
        return PrecipitationRuntimeProbeResult{
            .diagnostic = std::string{backend_name} + " precipitation runtime probe failed: " + exception.what(),
        };
    }
}

[[nodiscard]] VolumetricCloudRuntimeProbeResult execute_volumetric_cloud_runtime_probe(
    rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package, const VolumetricCloudPolicyDesc& desc,
    const Win32DesktopPresentationShaderBytecode& vertex_shader_bytecode,
    const Win32DesktopPresentationShaderBytecode& fragment_shader_bytecode, std::string_view backend_name,
    const VolumetricCloudRuntimeProbeBindings bindings = d3d12_volumetric_cloud_probe_bindings()) {
    try {
        if (!has_shader_bytecode(vertex_shader_bytecode) || !has_shader_bytecode(fragment_shader_bytecode)) {
            return VolumetricCloudRuntimeProbeResult{
                .diagnostic = std::string{backend_name} +
                              " volumetric cloud renderer execution requires vertex and fragment shader bytecode",
            };
        }

        const auto weather_upload = upload_runtime_texture_asset(device, package, desc.layer.weather_map_asset_ref,
                                                                 backend_name, "environment volumetric cloud");
        if (!weather_upload.succeeded() || weather_upload.texture.value == 0) {
            return VolumetricCloudRuntimeProbeResult{.diagnostic = weather_upload.diagnostic};
        }

        const auto shape_upload = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 4096,
            .usage = rhi::BufferUsage::copy_source,
        });
        const auto erosion_upload = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 4096,
            .usage = rhi::BufferUsage::copy_source,
        });
        const auto shape_texture = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 4, .height = 4, .depth = 4},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::copy_destination | rhi::TextureUsage::shader_resource,
        });
        const auto erosion_texture = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 4, .height = 4, .depth = 4},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::copy_destination | rhi::TextureUsage::shader_resource,
        });
        const auto constants = create_volumetric_cloud_constants_buffer(device, desc);
        const auto target = device.create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::render_target | rhi::TextureUsage::copy_source,
        });
        const auto readback = device.create_buffer(rhi::BufferDesc{
            .size_bytes = 256U * 64U,
            .usage = rhi::BufferUsage::copy_destination,
        });
        const auto sampler = device.create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::nearest,
            .mag_filter = rhi::SamplerFilter::nearest,
            .address_u = rhi::SamplerAddressMode::clamp_to_edge,
            .address_v = rhi::SamplerAddressMode::clamp_to_edge,
            .address_w = rhi::SamplerAddressMode::clamp_to_edge,
        });
        if (shape_upload.value == 0 || erosion_upload.value == 0 || shape_texture.value == 0 ||
            erosion_texture.value == 0 || constants.value == 0 || target.value == 0 || readback.value == 0 ||
            sampler.value == 0) {
            return VolumetricCloudRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric cloud probe resource creation failed",
            };
        }

        std::array<std::uint8_t, 4096> shape_bytes{};
        for (std::size_t offset = 0; offset < shape_bytes.size(); offset += 4U) {
            shape_bytes[offset + 0U] = 255U;
            shape_bytes[offset + 1U] = 255U;
            shape_bytes[offset + 2U] = 255U;
            shape_bytes[offset + 3U] = 255U;
        }
        std::array<std::uint8_t, 4096> erosion_bytes{};
        for (std::size_t offset = 0; offset < erosion_bytes.size(); offset += 4U) {
            erosion_bytes[offset + 3U] = 255U;
        }
        device.write_buffer(shape_upload, 0, shape_bytes);
        device.write_buffer(erosion_upload, 0, erosion_bytes);

        const auto descriptor_set_layout = device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = bindings.weather_map_binding,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = bindings.shape_noise_binding,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = bindings.erosion_noise_binding,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = bindings.sampler_binding,
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = bindings.constants_binding,
                .type = rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
        }});
        const auto descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = bindings.weather_map_binding,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture,
                                                           weather_upload.texture)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = bindings.shape_noise_binding,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, shape_texture)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = bindings.erosion_noise_binding,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, erosion_texture)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = bindings.sampler_binding,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::sampler(sampler)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = bindings.constants_binding,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer, constants)},
        });

        const auto pipeline_layout =
            device.create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {descriptor_set_layout}});
        const auto vertex_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = vertex_shader_bytecode.entry_point,
            .bytecode_size = vertex_shader_bytecode.bytecode.size(),
            .bytecode = vertex_shader_bytecode.bytecode.data(),
        });
        const auto fragment_shader = device.create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = fragment_shader_bytecode.entry_point,
            .bytecode_size = fragment_shader_bytecode.bytecode.size(),
            .bytecode = fragment_shader_bytecode.bytecode.data(),
        });
        const auto pipeline = device.create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::rgba8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = rhi::PrimitiveTopology::triangle_list,
        });
        if (descriptor_set_layout.value == 0 || descriptor_set.value == 0 || pipeline_layout.value == 0 ||
            vertex_shader.value == 0 || fragment_shader.value == 0 || pipeline.value == 0) {
            return VolumetricCloudRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric cloud probe pipeline creation failed",
            };
        }

        const rhi::BufferTextureCopyRegion volume_footprint{
            .buffer_offset = 0,
            .buffer_row_length = 64,
            .buffer_image_height = 4,
            .texture_offset = rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = rhi::Extent3D{.width = 4, .height = 4, .depth = 4},
        };
        const rhi::BufferTextureCopyRegion readback_footprint{
            .buffer_offset = 0,
            .buffer_row_length = 64,
            .buffer_image_height = 64,
            .texture_offset = rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        };

        const auto before_stats = device.stats();
        auto commands = device.begin_command_list(rhi::QueueKind::graphics);
        if (bindings.transition_upload_textures_from_undefined) {
            commands->transition_texture(shape_texture, rhi::ResourceState::undefined,
                                         rhi::ResourceState::copy_destination);
            commands->transition_texture(erosion_texture, rhi::ResourceState::undefined,
                                         rhi::ResourceState::copy_destination);
        }
        commands->copy_buffer_to_texture(shape_upload, shape_texture, volume_footprint);
        commands->copy_buffer_to_texture(erosion_upload, erosion_texture, volume_footprint);
        commands->transition_texture(shape_texture, rhi::ResourceState::copy_destination,
                                     rhi::ResourceState::shader_read);
        commands->transition_texture(erosion_texture, rhi::ResourceState::copy_destination,
                                     rhi::ResourceState::shader_read);
        commands->transition_texture(target,
                                     bindings.transition_upload_textures_from_undefined
                                         ? rhi::ResourceState::undefined
                                         : rhi::ResourceState::copy_source,
                                     rhi::ResourceState::render_target);
        commands->begin_render_pass(rhi::RenderPassDesc{
            .color =
                rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = rhi::LoadAction::clear,
                    .store_action = rhi::StoreAction::store,
                    .swapchain_frame = rhi::SwapchainFrameHandle{},
                    .clear_color = rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.0F},
                },
        });
        commands->bind_graphics_pipeline(pipeline);
        commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
        commands->draw(3, 1);
        commands->end_render_pass();
        commands->transition_texture(target, rhi::ResourceState::render_target, rhi::ResourceState::copy_source);
        commands->copy_texture_to_buffer(target, readback, readback_footprint);
        commands->close();

        const auto fence = device.submit(*commands);
        device.wait(fence);

        const auto bytes = device.read_buffer(readback, 0, 256U * 64U);
        const auto center_pixel = (32U * 256U) + (32U * 4U);
        if (bytes.size() < center_pixel + 4U) {
            return VolumetricCloudRuntimeProbeResult{
                .diagnostic = std::string{backend_name} + " volumetric cloud probe readback was incomplete",
            };
        }
        const bool readback_nonzero = bytes[center_pixel + 0U] > 12U && bytes[center_pixel + 1U] > 12U &&
                                      bytes[center_pixel + 2U] > 12U && bytes[center_pixel + 3U] > 12U;
        if (!readback_nonzero) {
            return VolumetricCloudRuntimeProbeResult{
                .diagnostic = std::string{backend_name} +
                              " volumetric cloud probe readback did not contain raymarched cloud output",
            };
        }

        const auto after_stats = device.stats();
        const auto renderer_draws =
            after_stats.draw_calls > before_stats.draw_calls ? after_stats.draw_calls - before_stats.draw_calls : 0U;
        const auto transition_delta = after_stats.resource_transitions > before_stats.resource_transitions
                                          ? after_stats.resource_transitions - before_stats.resource_transitions
                                          : 0U;
        const auto synchronization2_barriers = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(transition_delta, std::numeric_limits<std::uint32_t>::max()));
        return VolumetricCloudRuntimeProbeResult{
            .weather_map_uploads = weather_upload.copy_recorded ? 1U : 0U,
            .shape_noise_uploads = 1U,
            .erosion_noise_uploads = 1U,
            .backend_invocations = 1U,
            .renderer_draws = renderer_draws,
            .raymarch_passes = renderer_draws,
            .descriptor_set_bindings = kVulkanVolumetricCloudDescriptorSetBindings,
            .synchronization2_barriers = synchronization2_barriers,
            .readback_nonzero = true,
            .diagnostic = {},
        };
    } catch (const std::exception& exception) {
        return VolumetricCloudRuntimeProbeResult{
            .diagnostic = std::string{backend_name} + " volumetric cloud runtime probe failed: " + exception.what(),
        };
    }
}

[[nodiscard]] PhysicalSkyPolicyDesc sample_physical_sky_policy_desc() {
    return PhysicalSkyPolicyDesc{
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] EnvironmentCloudLayerDesc sample_cloud_layer_package_desc() {
    return EnvironmentCloudLayerDesc{
        .mode = EnvironmentCloudLayerMode::equirectangular_2d,
        .coverage = 0.45F,
        .opacity = 0.8F,
        .altitude_m = 2400.0F,
        .wind_velocity_mps = Vec2{.x = 8.0F, .y = 1.5F},
        .cloud_map_asset_ref = "sample/desktop-runtime/texture",
        .flow_map_asset_ref = "sample/desktop-runtime/cloud-flow",
        .sky_tint_response = Vec3{.x = 0.78F, .y = 0.84F, .z = 0.92F},
        .time_of_day_response = 0.55F,
        .ibl_contribution_mode = EnvironmentCloudIblContributionMode::sky_tint_only,
        .ibl_contribution = 0.25F,
    };
}

[[nodiscard]] bool runtime_package_has_texture_asset(const runtime::RuntimeAssetPackage* package,
                                                     std::string_view asset_ref) {
    if (package == nullptr) {
        return false;
    }
    const auto* record = package->find(asset_id_from_key_v2(AssetKeyV2{.value = std::string{asset_ref}}));
    return record != nullptr && record->kind == AssetKind::texture;
}

[[nodiscard]] bool cloud_layer_package_texture_evidence_ready(const runtime::RuntimeAssetPackage* package,
                                                              const CloudLayerPolicyDesc& desc) {
    return runtime_package_has_texture_asset(package, desc.layer.cloud_map_asset_ref) &&
           runtime_package_has_texture_asset(package, desc.layer.flow_map_asset_ref);
}

[[nodiscard]] EnvironmentPrecipitationPlan sample_environment_precipitation_plan() {
    EnvironmentProfileDesc profile{};
    profile.id = "sample_desktop_runtime_precipitation";
    profile.weather = EnvironmentWeatherKind::storm;
    profile.precipitation = EnvironmentPrecipitationDesc{
        .kind = EnvironmentPrecipitationKind::rain,
        .intensity = 0.8F,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 8.5F,
        .wind_speed_mps = 6.0F,
    };
    return plan_environment_precipitation(EnvironmentPrecipitationPlanDesc{
        .environment = profile,
        .scene_geometry_occlusion_required = true,
        .occlusion_policy_available = true,
    });
}

[[nodiscard]] PrecipitationPolicyDesc sample_environment_precipitation_policy_desc() {
    return PrecipitationPolicyDesc{
        .environment_plan = sample_environment_precipitation_plan(),
        .quality_tier = PrecipitationQualityTier::balanced,
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] bool
environment_precipitation_package_texture_evidence_ready(const runtime::RuntimeAssetPackage* package) {
    return runtime_package_has_texture_asset(package, "sample/desktop-runtime/texture");
}

[[nodiscard]] std::span<const VolumetricCloudAtmosphericLightDesc>
sample_environment_volumetric_cloud_lights() noexcept {
    static constexpr std::array lights{
        VolumetricCloudAtmosphericLightDesc{
            .direction = Vec3{.x = 0.3F, .y = -1.0F, .z = 0.1F},
            .color = Vec3{.x = 1.0F, .y = 0.95F, .z = 0.85F},
            .illuminance_lux = 100000.0F,
            .casts_cloud_shadows = true,
            .source_index = 0U,
        },
    };
    return lights;
}

[[nodiscard]] VolumetricCloudPolicyDesc sample_environment_volumetric_cloud_policy_desc() {
    return VolumetricCloudPolicyDesc{
        .layer =
            VolumetricCloudLayerDesc{
                .weather_map_asset_ref = "sample/desktop-runtime/texture",
                .shape_noise_asset_ref = "sample/desktop-runtime/volumetric-cloud-shape-noise-3d",
                .erosion_noise_asset_ref = "sample/desktop-runtime/volumetric-cloud-erosion-noise-3d",
                .coverage = 0.82F,
                .density = 0.92F,
                .altitude_min_m = 1000.0F,
                .altitude_max_m = 4000.0F,
                .wind_velocity_mps = Vec2{.x = 0.0F, .y = 0.0F},
            },
        .quality_tier = VolumetricCloudQualityTier::balanced,
        .raymarch =
            VolumetricCloudRaymarchBudgetDesc{
                .primary_steps = 48U,
                .light_steps = 8U,
                .shadow_mode = VolumetricCloudShadowMode::beer_shadow_map_intent,
                .temporal_reprojection_enabled = true,
                .temporal_history_weight = 0.9F,
            },
        .atmospheric_lights = sample_environment_volumetric_cloud_lights(),
        .storm =
            VolumetricCloudStormDesc{
                .enabled = true,
                .lightning_flash_intensity = 10000.0F,
                .lightning_direction = Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .thunder_delay_seconds = 0.0F,
                .cloud_darkening = 0.05F,
                .precipitation_boost = 0.2F,
                .wind_gust_mps = 0.0F,
                .exposure_response = 0.2F,
            },
        .shader_contract_evidence_ready = true,
        .package_evidence_ready = true,
        .execution_evidence_ready = true,
        .request_ready_promotion = true,
    };
}

[[nodiscard]] bool environment_volumetric_cloud_package_evidence_ready(const runtime::RuntimeAssetPackage* package,
                                                                       const VolumetricCloudPolicyDesc& desc) {
    return runtime_package_has_texture_asset(package, desc.layer.weather_map_asset_ref) &&
           !desc.layer.shape_noise_asset_ref.empty() && !desc.layer.erosion_noise_asset_ref.empty();
}

void apply_cloud_layer_plan(NativeRendererCreateResult& result, const CloudLayerPolicyPlan& plan) noexcept {
    result.cloud_layer_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.cloud_layer_package_evidence_ready = plan.package_evidence_ready;
    result.cloud_layer_execution_evidence_ready = plan.execution_evidence_ready;
    result.cloud_layer_uploads_textures = plan.uploads_textures;
    result.cloud_layer_invokes_backend = plan.invokes_backend;
    result.cloud_layer_exposes_native_handles = plan.exposes_native_handles;
    result.cloud_layer_uses_volumetric_clouds = plan.uses_volumetric_clouds;
    result.cloud_layer_texture_rows = static_cast<std::uint32_t>(plan.texture_rows.size());
    result.cloud_layer_visual_rows = static_cast<std::uint32_t>(plan.visual_rows.size());
    result.cloud_layer_ibl_rows = static_cast<std::uint32_t>(plan.ibl_rows.size());
    result.cloud_layer_shader_contract_rows = static_cast<std::uint32_t>(plan.shader_contract_rows.size());
    result.cloud_layer_quality_rows = static_cast<std::uint32_t>(plan.quality_rows.size());
    result.cloud_layer_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    result.cloud_layer_renderer_draws = plan.renderer_draws;
    if (!plan.shader_contract_rows.empty()) {
        result.cloud_layer_uses_latlong_projection = plan.shader_contract_rows.front().uses_latlong_projection;
        result.cloud_layer_uses_flow_map = plan.shader_contract_rows.front().uses_flow_map;
    }
}

void apply_physical_sky_plan(NativeRendererCreateResult& result, const PhysicalSkyPolicyPlan& plan) noexcept {
    result.physical_sky_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.physical_sky_package_evidence_ready = plan.package_evidence_ready;
    result.physical_sky_execution_evidence_ready = plan.execution_evidence_ready;
    result.physical_sky_allocates_lut_textures = plan.allocates_lut_textures;
    result.physical_sky_invokes_backend = plan.invokes_backend;
    result.physical_sky_exposes_native_handles = plan.exposes_native_handles;
    result.physical_sky_constant_layout_rows = static_cast<std::uint32_t>(plan.constant_layout_rows.size());
    result.physical_sky_lut_intent_rows = static_cast<std::uint32_t>(plan.lut_intent_rows.size());
    result.physical_sky_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
}

void apply_vulkan_physical_sky_package_plan(NativeRendererCreateResult& result,
                                            const PhysicalSkyPolicyPlan& plan) noexcept {
    result.physical_sky_vulkan_package_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.physical_sky_vulkan_package_evidence_ready = plan.package_evidence_ready;
    result.physical_sky_vulkan_package_execution_evidence_ready = plan.execution_evidence_ready;
    result.physical_sky_vulkan_package_allocates_lut_textures = plan.allocates_lut_textures;
    result.physical_sky_vulkan_package_invokes_backend = plan.invokes_backend;
    result.physical_sky_vulkan_package_exposes_native_handles = plan.exposes_native_handles;
    result.physical_sky_vulkan_package_constant_layout_rows =
        static_cast<std::uint32_t>(plan.constant_layout_rows.size());
    result.physical_sky_vulkan_package_lut_intent_rows = static_cast<std::uint32_t>(plan.lut_intent_rows.size());
    result.physical_sky_vulkan_package_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
}

void apply_precipitation_plan(NativeRendererCreateResult& result, const PrecipitationPolicyDesc& desc,
                              const PrecipitationPolicyPlan& plan) noexcept {
    result.environment_precipitation_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_precipitation_package_evidence_ready = plan.package_evidence_ready;
    result.environment_precipitation_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_precipitation_uploads_particle_buffers = plan.uploads_particle_buffers;
    result.environment_precipitation_invokes_backend = plan.invokes_backend;
    result.environment_precipitation_exposes_native_handles = plan.exposes_native_handles;
    result.environment_precipitation_mutates_materials = plan.mutates_materials;
    result.environment_precipitation_plays_audio = plan.plays_audio;
    result.environment_precipitation_renderer_draws = plan.renderer_draws;
    result.environment_precipitation_depth_occlusion_readback = plan.depth_occlusion_readback;
    result.environment_precipitation_weather_rows =
        static_cast<std::uint32_t>(desc.environment_plan.weather_rows.size());
    result.environment_precipitation_particle_rows =
        static_cast<std::uint32_t>(desc.environment_plan.particle_rows.size());
    result.environment_precipitation_occlusion_rows =
        static_cast<std::uint32_t>(desc.environment_plan.occlusion_rows.size());
    result.environment_precipitation_wetness_rows = static_cast<std::uint32_t>(plan.wetness_rows.size());
    result.environment_precipitation_audio_handoff_rows = static_cast<std::uint32_t>(plan.audio_handoff_rows.size());
    result.environment_precipitation_shader_rows = static_cast<std::uint32_t>(plan.shader_rows.size());
    result.environment_precipitation_quality_rows = static_cast<std::uint32_t>(plan.quality_rows.size());
    result.environment_precipitation_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!desc.environment_plan.weather_rows.empty()) {
        result.environment_precipitation_weather = desc.environment_plan.weather_rows.front().weather;
        result.environment_precipitation_kind = desc.environment_plan.weather_rows.front().precipitation_kind;
    }
    if (!plan.shader_rows.empty()) {
        result.environment_precipitation_uses_camera_near_particles =
            plan.shader_rows.front().uses_camera_near_particles;
        result.environment_precipitation_uses_scene_depth_occlusion =
            plan.shader_rows.front().uses_scene_depth_occlusion;
    }
}

void apply_vulkan_precipitation_plan(NativeRendererCreateResult& result, const PrecipitationPolicyDesc& desc,
                                     const PrecipitationPolicyPlan& plan) noexcept {
    result.environment_precipitation_vulkan_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_precipitation_vulkan_package_evidence_ready = plan.package_evidence_ready;
    result.environment_precipitation_vulkan_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_precipitation_vulkan_particle_buffer_uploads =
        plan.uploads_particle_buffers ? desc.particle_buffer_upload_count : 0U;
    result.environment_precipitation_vulkan_backend_invocations =
        plan.invokes_backend ? desc.backend_invocation_count : 0U;
    result.environment_precipitation_vulkan_renderer_draws = plan.renderer_draws;
    result.environment_precipitation_vulkan_depth_occlusion_readback = plan.depth_occlusion_readback;
    result.environment_precipitation_vulkan_descriptor_set_bindings =
        plan.invokes_backend ? kVulkanPrecipitationDescriptorSetBindings : 0U;
    result.environment_precipitation_vulkan_exposes_native_handles = plan.exposes_native_handles;
    result.environment_precipitation_vulkan_mutates_materials = plan.mutates_materials;
    result.environment_precipitation_vulkan_plays_audio = plan.plays_audio;
    result.environment_precipitation_vulkan_weather_rows =
        static_cast<std::uint32_t>(desc.environment_plan.weather_rows.size());
    result.environment_precipitation_vulkan_particle_rows =
        static_cast<std::uint32_t>(desc.environment_plan.particle_rows.size());
    result.environment_precipitation_vulkan_occlusion_rows =
        static_cast<std::uint32_t>(desc.environment_plan.occlusion_rows.size());
    result.environment_precipitation_vulkan_wetness_rows = static_cast<std::uint32_t>(plan.wetness_rows.size());
    result.environment_precipitation_vulkan_audio_handoff_rows =
        static_cast<std::uint32_t>(plan.audio_handoff_rows.size());
    result.environment_precipitation_vulkan_shader_rows = static_cast<std::uint32_t>(plan.shader_rows.size());
    result.environment_precipitation_vulkan_quality_rows = static_cast<std::uint32_t>(plan.quality_rows.size());
    result.environment_precipitation_vulkan_policy_diagnostics_count =
        static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!desc.environment_plan.weather_rows.empty()) {
        result.environment_precipitation_vulkan_weather = desc.environment_plan.weather_rows.front().weather;
        result.environment_precipitation_vulkan_kind = desc.environment_plan.weather_rows.front().precipitation_kind;
    }
    if (!plan.shader_rows.empty()) {
        result.environment_precipitation_vulkan_uses_camera_near_particles =
            plan.shader_rows.front().uses_camera_near_particles;
        result.environment_precipitation_vulkan_uses_scene_depth_occlusion =
            plan.shader_rows.front().uses_scene_depth_occlusion;
    }
}

void apply_volumetric_fog_plan(NativeRendererCreateResult& result, const VolumetricFogPolicyPlan& plan) noexcept {
    result.environment_volumetric_fog_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_volumetric_fog_package_evidence_ready = plan.package_evidence_ready;
    result.environment_volumetric_fog_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_volumetric_fog_scene_depth_ready = plan.scene_depth_available;
    result.environment_volumetric_fog_froxel_output_ready =
        plan.package_evidence_ready && plan.execution_evidence_ready;
    result.environment_volumetric_fog_compute_dispatches =
        result.environment_volumetric_fog_froxel_output_ready ? 1U : 0U;
    result.environment_volumetric_fog_exposes_native_handles = plan.exposes_native_handles;
    result.environment_volumetric_fog_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
}

void apply_vulkan_volumetric_fog_plan(NativeRendererCreateResult& result, const VolumetricFogPolicyPlan& plan,
                                      const VolumetricFogRuntimeProbeResult& probe) noexcept {
    result.environment_volumetric_fog_vulkan_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_volumetric_fog_vulkan_package_evidence_ready = plan.package_evidence_ready;
    result.environment_volumetric_fog_vulkan_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_volumetric_fog_vulkan_scene_depth_ready = plan.scene_depth_available && probe.scene_depth_ready;
    result.environment_volumetric_fog_vulkan_froxel_output_ready = plan.ready() && probe.froxel_output_ready;
    result.environment_volumetric_fog_vulkan_compute_dispatches = probe.compute_dispatches;
    result.environment_volumetric_fog_vulkan_descriptor_set_bindings = probe.descriptor_set_bindings;
    result.environment_volumetric_fog_vulkan_synchronization2_barriers = probe.synchronization2_barriers;
    result.environment_volumetric_fog_vulkan_froxel_readback_nonzero = probe.readback_nonzero;
    result.environment_volumetric_fog_vulkan_exposes_native_handles = plan.exposes_native_handles;
    result.environment_volumetric_fog_vulkan_policy_diagnostics_count =
        static_cast<std::uint32_t>(plan.diagnostics.size());
}

void apply_volumetric_cloud_plan(NativeRendererCreateResult& result, const VolumetricCloudPolicyPlan& plan) noexcept {
    result.environment_volumetric_cloud_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_volumetric_cloud_package_evidence_ready = plan.package_evidence_ready;
    result.environment_volumetric_cloud_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_volumetric_cloud_uploads_volume_textures = plan.uploads_volume_textures;
    result.environment_volumetric_cloud_invokes_backend = plan.invokes_backend;
    result.environment_volumetric_cloud_raymarch_passes = plan.raymarch_passes;
    result.environment_volumetric_cloud_readback_nonzero = plan.readback_nonzero;
    result.environment_volumetric_cloud_exposes_native_handles = plan.exposes_native_handles;
    result.environment_volumetric_cloud_plays_audio = plan.plays_audio;
    result.environment_volumetric_cloud_executes_precipitation = plan.executes_precipitation;
    result.environment_volumetric_cloud_map_rows = static_cast<std::uint32_t>(plan.map_rows.size());
    result.environment_volumetric_cloud_layer_rows = static_cast<std::uint32_t>(plan.layer_rows.size());
    result.environment_volumetric_cloud_lighting_rows = static_cast<std::uint32_t>(plan.lighting_rows.size());
    result.environment_volumetric_cloud_raymarch_rows = static_cast<std::uint32_t>(plan.raymarch_rows.size());
    result.environment_volumetric_cloud_temporal_rows = static_cast<std::uint32_t>(plan.temporal_rows.size());
    result.environment_volumetric_cloud_shadow_rows = static_cast<std::uint32_t>(plan.shadow_rows.size());
    result.environment_volumetric_cloud_storm_rows = static_cast<std::uint32_t>(plan.storm_rows.size());
    result.environment_volumetric_cloud_shader_contract_rows =
        static_cast<std::uint32_t>(plan.shader_contract_rows.size());
    result.environment_volumetric_cloud_quality_rows = static_cast<std::uint32_t>(plan.quality_rows.size());
    result.environment_volumetric_cloud_policy_diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!plan.map_rows.empty()) {
        result.environment_volumetric_cloud_weather_map_ready = plan.map_rows.front().weather_map_ready;
        result.environment_volumetric_cloud_shape_noise_ready = plan.map_rows.front().shape_noise_ready;
        result.environment_volumetric_cloud_erosion_noise_ready = plan.map_rows.front().erosion_noise_ready;
    }
}

void apply_vulkan_volumetric_cloud_plan(NativeRendererCreateResult& result, const VolumetricCloudPolicyPlan& plan,
                                        const VolumetricCloudRuntimeProbeResult& probe) noexcept {
    result.environment_volumetric_cloud_vulkan_shader_contract_evidence_ready = plan.shader_contract_evidence_ready;
    result.environment_volumetric_cloud_vulkan_package_evidence_ready = plan.package_evidence_ready;
    result.environment_volumetric_cloud_vulkan_execution_evidence_ready = plan.execution_evidence_ready;
    result.environment_volumetric_cloud_vulkan_uploads_volume_textures = plan.uploads_volume_textures;
    result.environment_volumetric_cloud_vulkan_invokes_backend = plan.invokes_backend;
    result.environment_volumetric_cloud_vulkan_renderer_draws = probe.renderer_draws;
    result.environment_volumetric_cloud_vulkan_raymarch_passes = plan.raymarch_passes;
    result.environment_volumetric_cloud_vulkan_descriptor_set_bindings = probe.descriptor_set_bindings;
    result.environment_volumetric_cloud_vulkan_synchronization2_barriers = probe.synchronization2_barriers;
    result.environment_volumetric_cloud_vulkan_readback_nonzero = plan.readback_nonzero && probe.readback_nonzero;
    result.environment_volumetric_cloud_vulkan_exposes_native_handles = plan.exposes_native_handles;
    result.environment_volumetric_cloud_vulkan_plays_audio = plan.plays_audio;
    result.environment_volumetric_cloud_vulkan_executes_precipitation = plan.executes_precipitation;
    result.environment_volumetric_cloud_vulkan_map_rows = static_cast<std::uint32_t>(plan.map_rows.size());
    result.environment_volumetric_cloud_vulkan_layer_rows = static_cast<std::uint32_t>(plan.layer_rows.size());
    result.environment_volumetric_cloud_vulkan_lighting_rows = static_cast<std::uint32_t>(plan.lighting_rows.size());
    result.environment_volumetric_cloud_vulkan_raymarch_rows = static_cast<std::uint32_t>(plan.raymarch_rows.size());
    result.environment_volumetric_cloud_vulkan_temporal_rows = static_cast<std::uint32_t>(plan.temporal_rows.size());
    result.environment_volumetric_cloud_vulkan_shadow_rows = static_cast<std::uint32_t>(plan.shadow_rows.size());
    result.environment_volumetric_cloud_vulkan_storm_rows = static_cast<std::uint32_t>(plan.storm_rows.size());
    result.environment_volumetric_cloud_vulkan_shader_contract_rows =
        static_cast<std::uint32_t>(plan.shader_contract_rows.size());
    result.environment_volumetric_cloud_vulkan_quality_rows = static_cast<std::uint32_t>(plan.quality_rows.size());
    result.environment_volumetric_cloud_vulkan_policy_diagnostics_count =
        static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!plan.map_rows.empty()) {
        result.environment_volumetric_cloud_vulkan_weather_map_ready = plan.map_rows.front().weather_map_ready;
        result.environment_volumetric_cloud_vulkan_shape_noise_ready = plan.map_rows.front().shape_noise_ready;
        result.environment_volumetric_cloud_vulkan_erosion_noise_ready = plan.map_rows.front().erosion_noise_ready;
    }
}

void append_unique_asset(std::vector<AssetId>& assets, AssetId asset) {
    if (asset.value == 0) {
        return;
    }
    if (std::ranges::find(assets, asset) == assets.end()) {
        assets.push_back(asset);
    }
}

void append_morph_binding_assets(std::vector<AssetId>& assets,
                                 std::span<const Win32DesktopPresentationSceneMorphMeshBinding> bindings) {
    for (const auto& binding : bindings) {
        append_unique_asset(assets, binding.morph_mesh);
    }
}

[[nodiscard]] bool has_morph_binding_for_mesh(std::span<const Win32DesktopPresentationSceneMorphMeshBinding> bindings,
                                              AssetId mesh) noexcept {
    return std::ranges::any_of(bindings, [mesh](const Win32DesktopPresentationSceneMorphMeshBinding& binding) {
        return binding.mesh == mesh;
    });
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_position_vertex_buffers() {
    return {rhi::VertexBufferLayoutDesc{
        .binding = 0,
        .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
        .input_rate = rhi::VertexInputRate::vertex,
    }};
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_tangent_frame_vertex_buffers() {
    return {
        rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 1,
            .stride = runtime_rhi::runtime_morph_normal_delta_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 2,
            .stride = runtime_rhi::runtime_morph_tangent_delta_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_skinned_vertex_buffers() {
    return {
        rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 3,
            .stride = runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_position_vertex_attributes() {
    return {rhi::VertexAttributeDesc{
        .location = 0,
        .binding = 0,
        .offset = 0,
        .format = rhi::VertexFormat::float32x3,
        .semantic = rhi::VertexSemantic::position,
        .semantic_index = 0,
    }};
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_skinned_vertex_attributes() {
    return {
        rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::position,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 3,
            .offset = 48,
            .format = rhi::VertexFormat::uint16x4,
            .semantic = rhi::VertexSemantic::joint_indices,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 3,
            .offset = 56,
            .format = rhi::VertexFormat::float32x4,
            .semantic = rhi::VertexSemantic::joint_weights,
            .semantic_index = 0,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_tangent_frame_vertex_attributes() {
    return {
        rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::position,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 1,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::normal,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 2,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::tangent,
            .semantic_index = 0,
        },
    };
}

[[nodiscard]] const runtime_scene_rhi::RuntimeSceneMeshGpuResource*
find_uploaded_scene_mesh(const runtime_scene_rhi::RuntimeSceneGpuBindingResult& bindings, AssetId mesh) noexcept {
    const auto it = std::ranges::find_if(
        bindings.mesh_uploads,
        [mesh](const runtime_scene_rhi::RuntimeSceneMeshGpuResource& upload) { return upload.mesh == mesh; });
    return it == bindings.mesh_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] const runtime_scene_rhi::RuntimeSceneMorphMeshGpuResource*
find_uploaded_scene_morph_mesh(const runtime_scene_rhi::RuntimeSceneGpuBindingResult& bindings,
                               AssetId morph_mesh) noexcept {
    const auto it = std::ranges::find_if(
        bindings.morph_mesh_uploads, [morph_mesh](const runtime_scene_rhi::RuntimeSceneMorphMeshGpuResource& upload) {
            return upload.morph_mesh == morph_mesh;
        });
    return it == bindings.morph_mesh_uploads.end() ? nullptr : &*it;
}

struct SceneComputeMorphBuildResult {
    std::vector<SceneComputeMorphMeshBinding> bindings;
    std::string diagnostic;
    std::size_t queue_waits{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct SceneComputeMorphDispatchResult {
    std::string diagnostic;
    std::size_t dispatches{0};
    std::size_t queue_waits{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] bool has_postprocess_bytecode(const Win32DesktopPresentationShaderBytecode& vertex,
                                            const Win32DesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool has_directional_shadow_bytecode(const Win32DesktopPresentationShaderBytecode& vertex,
                                                   const Win32DesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool has_native_ui_overlay_bytecode(const Win32DesktopPresentationShaderBytecode& vertex,
                                                  const Win32DesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool valid_d3d12_renderer_request(const Win32DesktopPresentationD3d12RendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay &&
        !has_native_ui_overlay_bytecode(desc->native_sprite_overlay_vertex_shader,
                                        desc->native_sprite_overlay_fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay_textures &&
        (!desc->enable_native_sprite_overlay || desc->native_sprite_overlay_package == nullptr ||
         desc->native_sprite_overlay_atlas_asset.value == 0)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool valid_vulkan_renderer_request(const Win32DesktopPresentationVulkanRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay &&
        !has_native_ui_overlay_bytecode(desc->native_sprite_overlay_vertex_shader,
                                        desc->native_sprite_overlay_fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay_textures &&
        (!desc->enable_native_sprite_overlay || desc->native_sprite_overlay_package == nullptr ||
         desc->native_sprite_overlay_atlas_asset.value == 0)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool same_vertex_buffer_layout(const rhi::VertexBufferLayoutDesc& lhs,
                                             const rhi::VertexBufferLayoutDesc& rhs) noexcept {
    return lhs.binding == rhs.binding && lhs.stride == rhs.stride && lhs.input_rate == rhs.input_rate;
}

[[nodiscard]] bool same_vertex_attribute(const rhi::VertexAttributeDesc& lhs,
                                         const rhi::VertexAttributeDesc& rhs) noexcept {
    return lhs.location == rhs.location && lhs.binding == rhs.binding && lhs.offset == rhs.offset &&
           lhs.format == rhs.format && lhs.semantic == rhs.semantic && lhs.semantic_index == rhs.semantic_index;
}

[[nodiscard]] bool same_vertex_buffer_layouts(std::span<const rhi::VertexBufferLayoutDesc> lhs,
                                              std::span<const rhi::VertexBufferLayoutDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::ranges::find_if(lhs, [&expected](const rhi::VertexBufferLayoutDesc& item) {
            return same_vertex_buffer_layout(item, expected);
        });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool same_vertex_attributes(std::span<const rhi::VertexAttributeDesc> lhs,
                                          std::span<const rhi::VertexAttributeDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::ranges::find_if(
            lhs, [&expected](const rhi::VertexAttributeDesc& item) { return same_vertex_attribute(item, expected); });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

void append_unique_mesh(std::vector<AssetId>& meshes, AssetId mesh) {
    const auto found = std::ranges::find(meshes, mesh);
    if (found == meshes.end()) {
        meshes.push_back(mesh);
    }
}

[[nodiscard]] bool scene_packet_references_skinned_mesh(const runtime::RuntimeAssetPackage& package,
                                                        const SceneRenderPacket& packet) noexcept {
    for (const auto& mesh : packet.meshes) {
        const auto* record = package.find(mesh.renderer.mesh);
        if (record != nullptr && record->kind == AssetKind::skinned_mesh) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] SceneRendererRequestValidation validate_scene_renderer_mesh_layout_request(
    const runtime::RuntimeAssetPackage& package, const SceneRenderPacket& packet,
    std::span<const rhi::VertexBufferLayoutDesc> static_vertex_buffers,
    std::span<const rhi::VertexAttributeDesc> static_vertex_attributes,
    std::span<const rhi::VertexBufferLayoutDesc> skinned_vertex_buffers,
    std::span<const rhi::VertexAttributeDesc> skinned_vertex_attributes,
    std::span<const Win32DesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings,
    std::string_view backend_name) {
    std::vector<AssetId> meshes;
    for (const auto& mesh : packet.meshes) {
        append_unique_mesh(meshes, mesh.renderer.mesh);
    }
    for (const auto mesh : meshes) {
        const auto* record = package.find(mesh);
        if (record == nullptr) {
            return SceneRendererRequestValidation{
                .valid = false,
                .diagnostic = std::string(backend_name) +
                              " scene renderer request references a mesh without cooked payload layout metadata.",
            };
        }
        if (record->kind == AssetKind::mesh) {
            const auto payload = runtime::runtime_mesh_payload(*record);
            if (!payload.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request mesh payload is invalid: " + payload.diagnostic,
                };
            }
            const auto layout = runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload.payload);
            if (!layout.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request mesh layout is invalid: " + layout.diagnostic,
                };
            }
            if (!same_vertex_buffer_layouts(static_vertex_buffers, layout.vertex_buffers) ||
                !same_vertex_attributes(static_vertex_attributes, layout.vertex_attributes)) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request vertex input does not match cooked mesh payload layout.",
                };
            }
            continue;
        }
        if (record->kind == AssetKind::skinned_mesh) {
            if (skinned_vertex_buffers.empty() || skinned_vertex_attributes.empty()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic =
                        std::string(backend_name) +
                        " scene renderer request references a skinned mesh but does not provide skinned vertex input.",
                };
            }
            const auto payload = runtime::runtime_skinned_mesh_payload(*record);
            if (!payload.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request skinned mesh payload is invalid: " + payload.diagnostic,
                };
            }
            const auto layout = runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(payload.payload);
            if (!layout.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request skinned mesh layout is invalid: " + layout.diagnostic,
                };
            }
            auto expected_vertex_buffers = layout.vertex_buffers;
            auto expected_vertex_attributes = layout.vertex_attributes;
            if (has_morph_binding_for_mesh(compute_morph_skinned_mesh_bindings, mesh)) {
                expected_vertex_buffers = compute_morph_skinned_vertex_buffers();
                expected_vertex_attributes = compute_morph_skinned_vertex_attributes();
            }
            if (!same_vertex_buffer_layouts(skinned_vertex_buffers, expected_vertex_buffers) ||
                !same_vertex_attributes(skinned_vertex_attributes, expected_vertex_attributes)) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic =
                        std::string(backend_name) +
                        " scene renderer request skinned vertex input does not match cooked skinned mesh layout.",
                };
            }
            continue;
        }
        return SceneRendererRequestValidation{
            .valid = false,
            .diagnostic =
                std::string(backend_name) + " scene renderer request references an unsupported mesh asset kind.",
        };
    }
    return SceneRendererRequestValidation{.valid = true, .diagnostic = {}};
}

[[nodiscard]] bool
valid_d3d12_scene_renderer_request(const Win32DesktopPresentationD3d12SceneRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader) ||
        desc->package == nullptr || desc->packet == nullptr || desc->packet->meshes.empty() ||
        desc->vertex_buffers.empty() || desc->vertex_attributes.empty()) {
        return false;
    }
    if (scene_packet_references_skinned_mesh(*desc->package, *desc->packet)) {
        if (!has_shader_bytecode(desc->skinned_vertex_shader) || desc->skinned_vertex_buffers.empty() ||
            desc->skinned_vertex_attributes.empty()) {
            return false;
        }
        if (!desc->morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_skinned_mesh_bindings.empty() &&
            (!has_shader_bytecode(desc->compute_morph_skinned_shader) || desc->enable_directional_shadow_smoke)) {
            return false;
        }
        if (desc->enable_directional_shadow_smoke && !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
            return false;
        }
    } else if (!desc->compute_morph_skinned_mesh_bindings.empty()) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && !has_shader_bytecode(desc->morph_vertex_shader)) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && desc->enable_directional_shadow_smoke &&
        !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
        return false;
    }
    if (!desc->compute_morph_mesh_bindings.empty() &&
        (!has_shader_bytecode(desc->compute_morph_vertex_shader) || !has_shader_bytecode(desc->compute_morph_shader) ||
         desc->enable_directional_shadow_smoke)) {
        return false;
    }
    if (desc->enable_cloud_layer_renderer_execution &&
        (!has_shader_bytecode(desc->cloud_layer_vertex_shader) ||
         !has_shader_bytecode(desc->cloud_layer_fragment_shader) || desc->enable_directional_shadow_smoke)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool
valid_vulkan_scene_renderer_request(const Win32DesktopPresentationVulkanSceneRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader) ||
        desc->package == nullptr || desc->packet == nullptr || desc->packet->meshes.empty() ||
        desc->vertex_buffers.empty() || desc->vertex_attributes.empty()) {
        return false;
    }
    if (scene_packet_references_skinned_mesh(*desc->package, *desc->packet)) {
        if (!has_shader_bytecode(desc->skinned_vertex_shader) || desc->skinned_vertex_buffers.empty() ||
            desc->skinned_vertex_attributes.empty()) {
            return false;
        }
        if (!desc->morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_skinned_mesh_bindings.empty() &&
            (!has_shader_bytecode(desc->compute_morph_skinned_shader) || desc->enable_directional_shadow_smoke)) {
            return false;
        }
        if (desc->enable_directional_shadow_smoke && !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
            return false;
        }
    } else if (!desc->compute_morph_skinned_mesh_bindings.empty()) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && !has_shader_bytecode(desc->morph_vertex_shader)) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && desc->enable_directional_shadow_smoke &&
        !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
        return false;
    }
    if (!desc->compute_morph_mesh_bindings.empty() &&
        (!has_shader_bytecode(desc->compute_morph_vertex_shader) || !has_shader_bytecode(desc->compute_morph_shader) ||
         desc->enable_directional_shadow_smoke)) {
        return false;
    }
    if (desc->enable_environment_precipitation_renderer_execution &&
        (!has_shader_bytecode(desc->precipitation_vertex_shader) ||
         !has_shader_bytecode(desc->precipitation_fragment_shader) || desc->enable_directional_shadow_smoke)) {
        return false;
    }
    if (desc->enable_environment_volumetric_fog_renderer_execution &&
        (!has_shader_bytecode(desc->volumetric_fog_compute_shader) || !desc->enable_postprocess ||
         !desc->enable_postprocess_depth_input || desc->enable_directional_shadow_smoke)) {
        return false;
    }
    if (desc->enable_environment_volumetric_cloud_renderer_execution &&
        (!has_shader_bytecode(desc->volumetric_cloud_vertex_shader) ||
         !has_shader_bytecode(desc->volumetric_cloud_fragment_shader) || !desc->enable_postprocess ||
         !desc->enable_postprocess_depth_input || desc->enable_directional_shadow_smoke)) {
        return false;
    }
    return true;
}

[[nodiscard]] Win32DesktopPresentationBackend
requested_backend_from_desc(const Win32DesktopPresentationDesc& desc) noexcept {
    if (desc.prefer_vulkan) {
        return Win32DesktopPresentationBackend::vulkan;
    }
    if (desc.prefer_d3d12) {
        return Win32DesktopPresentationBackend::d3d12;
    }
    return Win32DesktopPresentationBackend::null_renderer;
}

[[nodiscard]] Win32DesktopPresentationBackendReportStatus
backend_report_status_from_fallback_reason(Win32DesktopPresentationFallbackReason reason) noexcept {
    switch (reason) {
    case Win32DesktopPresentationFallbackReason::none:
        return Win32DesktopPresentationBackendReportStatus::ready;
    case Win32DesktopPresentationFallbackReason::native_window_unavailable:
        return Win32DesktopPresentationBackendReportStatus::native_window_unavailable;
    case Win32DesktopPresentationFallbackReason::native_backend_unavailable:
        return Win32DesktopPresentationBackendReportStatus::native_backend_unavailable;
    case Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable:
        return Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
    }
    return Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_renderer_request() {
    return NativeRendererCreateResult{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 renderer creation requires non-empty vertex and fragment shader bytecode; using "
                      "NullRenderer fallback.",
    };
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_renderer_request() {
    return NativeRendererCreateResult{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic =
            "Vulkan renderer creation requires non-empty vertex and fragment SPIR-V bytecode; using NullRenderer "
            "fallback.",
    };
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_scene_renderer_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene renderer creation requires non-empty shader bytecode, package, render packet, and "
                      "vertex input "
                      "metadata; using NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_postprocess_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene postprocess creation requires non-empty postprocess vertex and fragment shader "
                      "bytecode; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(Win32DesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_scene_renderer_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene renderer creation requires non-empty SPIR-V shader bytecode, package, render "
                      "packet, and vertex "
                      "input metadata; using NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_postprocess_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene postprocess creation requires non-empty postprocess vertex and fragment SPIR-V "
                      "bytecode; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(Win32DesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_postprocess_depth_input_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene postprocess depth input requires scene postprocess to be enabled; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(Win32DesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    result.postprocess_depth_input_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_postprocess_depth_input_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene postprocess depth input requires scene postprocess to be enabled; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(Win32DesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    result.postprocess_depth_input_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_directional_shadow_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::invalid_request;
    result.directional_shadow_diagnostics.push_back(
        make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    result.directional_shadow_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_directional_shadow_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::invalid_request;
    result.directional_shadow_diagnostics.push_back(
        make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    result.directional_shadow_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_native_ui_overlay_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::invalid_request;
    result.native_ui_overlay_diagnostics.push_back(
        make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    result.native_ui_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_native_ui_texture_overlay_request(std::string message) {
    auto result = invalid_d3d12_native_ui_overlay_request(std::move(message));
    result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::invalid_request;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    result.native_ui_texture_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_native_ui_overlay_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::invalid_request;
    result.native_ui_overlay_diagnostics.push_back(
        make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    result.native_ui_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_native_ui_texture_overlay_request(std::string message) {
    auto result = invalid_vulkan_native_ui_overlay_request(std::move(message));
    result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::invalid_request;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    result.native_ui_texture_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_directional_shadow_request() {
    return invalid_d3d12_directional_shadow_request(
        "D3D12 directional shadow smoke requires non-empty shadow vertex and fragment shader bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_native_ui_overlay_request() {
    return invalid_d3d12_native_ui_overlay_request(
        "D3D12 native UI overlay requires non-empty overlay vertex and fragment shader bytecode; using NullRenderer "
        "fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_native_ui_overlay_request() {
    return invalid_vulkan_native_ui_overlay_request(
        "Vulkan native UI overlay requires non-empty overlay vertex and fragment SPIR-V bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_directional_shadow_request() {
    return invalid_vulkan_directional_shadow_request(
        "Vulkan directional shadow smoke requires non-empty shadow vertex and fragment SPIR-V bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] SurfaceProbe probe_d3d12_surface(const win32::Win32Window& window) {
    const auto native_window = window.native_window_token();
    if (native_window == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = Win32DesktopPresentationFallbackReason::native_window_unavailable,
            .diagnostic = "Win32 window handle is unavailable; using NullRenderer fallback.",
        };
    }

    return SurfaceProbe{
        .surface = rhi::SurfaceHandle{native_window},
        .failure_reason = Win32DesktopPresentationFallbackReason::none,
        .diagnostic = {},
    };
}

[[nodiscard]] SurfaceProbe probe_vulkan_surface(const win32::Win32Window& window) {
    const auto native_window = window.native_window_token();
    if (native_window == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = Win32DesktopPresentationFallbackReason::native_window_unavailable,
            .diagnostic = "Win32 window handle is unavailable for Vulkan presentation; using NullRenderer fallback.",
        };
    }

    return SurfaceProbe{
        .surface = rhi::SurfaceHandle{native_window},
        .failure_reason = Win32DesktopPresentationFallbackReason::none,
        .diagnostic = {},
    };
}

[[nodiscard]] Win32DesktopPresentationDiagnostic make_diagnostic(Win32DesktopPresentationFallbackReason reason,
                                                                 std::string message) {
    return Win32DesktopPresentationDiagnostic{
        .reason = reason,
        .message = std::move(message),
    };
}

[[nodiscard]] Win32DesktopPresentationSceneGpuBindingDiagnostic
make_scene_gpu_diagnostic(Win32DesktopPresentationSceneGpuBindingStatus status, std::string message) {
    return Win32DesktopPresentationSceneGpuBindingDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] Win32DesktopPresentationPostprocessDiagnostic
make_postprocess_diagnostic(Win32DesktopPresentationPostprocessStatus status, std::string message) {
    return Win32DesktopPresentationPostprocessDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] Win32DesktopPresentationDirectionalShadowDiagnostic
make_directional_shadow_diagnostic(Win32DesktopPresentationDirectionalShadowStatus status, std::string message) {
    return Win32DesktopPresentationDirectionalShadowDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] Win32DesktopPresentationNativeUiOverlayDiagnostic
make_native_ui_overlay_diagnostic(Win32DesktopPresentationNativeUiOverlayStatus status, std::string message) {
    return Win32DesktopPresentationNativeUiOverlayDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] Win32DesktopPresentationNativeUiTextureOverlayDiagnostic
make_native_ui_texture_overlay_diagnostic(Win32DesktopPresentationNativeUiTextureOverlayStatus status,
                                          std::string message) {
    return Win32DesktopPresentationNativeUiTextureOverlayDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

void mark_directional_shadow_result(NativeRendererCreateResult& result,
                                    Win32DesktopPresentationDirectionalShadowStatus status, std::string message) {
    result.directional_shadow_status = status;
    result.directional_shadow_requested = true;
    result.directional_shadow_ready = false;
    result.directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(status, std::move(message)));
}

void mark_native_ui_overlay_result(NativeRendererCreateResult& result,
                                   Win32DesktopPresentationNativeUiOverlayStatus status, std::string message) {
    result.native_ui_overlay_status = status;
    result.native_ui_overlay_requested = true;
    result.native_ui_overlay_ready = false;
    result.native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(status, std::move(message)));
}

void mark_native_ui_texture_overlay_result(NativeRendererCreateResult& result,
                                           Win32DesktopPresentationNativeUiTextureOverlayStatus status,
                                           std::string message) {
    result.native_ui_texture_overlay_status = status;
    result.native_ui_texture_overlay_requested = true;
    result.native_ui_texture_overlay_atlas_ready = false;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(status, std::move(message)));
}

[[nodiscard]] SceneComputeMorphBuildResult
build_scene_compute_morph_bindings(rhi::IRhiDevice& device,
                                   const runtime_scene_rhi::RuntimeSceneGpuBindingResult& gpu_bindings,
                                   std::span<const Win32DesktopPresentationSceneMorphMeshBinding> requested_bindings,
                                   const Win32DesktopPresentationShaderBytecode& compute_shader_bytecode,
                                   bool enable_tangent_frame_output, std::string_view backend_name) {
    SceneComputeMorphBuildResult result;
    if (requested_bindings.empty()) {
        return result;
    }
    if (!has_shader_bytecode(compute_shader_bytecode)) {
        result.diagnostic =
            std::string{backend_name} + " scene compute morph requires non-empty compute shader bytecode";
        return result;
    }

    const auto compute_shader = device.create_shader(rhi::ShaderDesc{
        .stage = rhi::ShaderStage::compute,
        .entry_point = compute_shader_bytecode.entry_point,
        .bytecode_size = compute_shader_bytecode.bytecode.size(),
        .bytecode = compute_shader_bytecode.bytecode.data(),
    });

    result.bindings.reserve(requested_bindings.size());
    for (const auto& requested : requested_bindings) {
        const auto* mesh_upload = find_uploaded_scene_mesh(gpu_bindings, requested.mesh);
        if (mesh_upload == nullptr) {
            result.diagnostic = std::string{backend_name} +
                                " scene compute morph requested a mesh that was not uploaded by the scene GPU palette";
            return result;
        }
        const auto* morph_upload = find_uploaded_scene_morph_mesh(gpu_bindings, requested.morph_mesh);
        if (morph_upload == nullptr) {
            result.diagnostic =
                std::string{backend_name} +
                " scene compute morph requested a morph mesh that was not uploaded by the scene GPU palette";
            return result;
        }

        runtime_rhi::RuntimeMorphMeshComputeBindingOptions options;
        options.output_position_usage =
            rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
        if (enable_tangent_frame_output) {
            options.output_normal_usage =
                rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
            options.output_tangent_usage =
                rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
        }
        const auto compute_binding = runtime_rhi::create_runtime_morph_mesh_compute_binding(
            device, mesh_upload->upload, morph_upload->upload, options);
        if (!compute_binding.succeeded()) {
            result.diagnostic =
                std::string{backend_name} + " scene compute morph binding failed: " + compute_binding.diagnostic;
            return result;
        }

        const auto compute_layout = device.create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
        const auto compute_pipeline = device.create_compute_pipeline(
            rhi::ComputePipelineDesc{.layout = compute_layout, .compute_shader = compute_shader});
        auto commands = device.begin_command_list(rhi::QueueKind::compute);
        commands->bind_compute_pipeline(compute_pipeline);
        commands->bind_descriptor_set(compute_layout, 0, compute_binding.descriptor_set);
        commands->dispatch(compute_binding.vertex_count, 1, 1);
        commands->close();
        const auto fence = device.submit(*commands);
        device.wait_for_queue(rhi::QueueKind::graphics, fence);
        ++result.queue_waits;

        const auto output_mesh_binding =
            enable_tangent_frame_output
                ? runtime_rhi::make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(mesh_upload->upload,
                                                                                                compute_binding)
                : runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload->upload, compute_binding);
        if (output_mesh_binding.vertex_buffer.value == 0 || output_mesh_binding.owner_device != &device) {
            result.diagnostic = std::string{backend_name} + " scene compute morph output mesh binding failed";
            return result;
        }
        result.bindings.push_back(SceneComputeMorphMeshBinding{
            .mesh = requested.mesh,
            .morph_mesh = requested.morph_mesh,
            .mesh_binding = output_mesh_binding,
        });
    }
    return result;
}

[[nodiscard]] SceneComputeMorphDispatchResult dispatch_scene_compute_morph_skinned_bindings(
    rhi::IRhiDevice& device,
    std::span<const runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshGpuResource> bindings,
    const Win32DesktopPresentationShaderBytecode& compute_shader_bytecode, std::string_view backend_name) {
    SceneComputeMorphDispatchResult result;
    if (bindings.empty()) {
        return result;
    }
    if (!has_shader_bytecode(compute_shader_bytecode)) {
        result.diagnostic =
            std::string{backend_name} + " scene compute morph skinned path requires non-empty compute shader bytecode";
        return result;
    }

    const auto compute_shader = device.create_shader(rhi::ShaderDesc{
        .stage = rhi::ShaderStage::compute,
        .entry_point = compute_shader_bytecode.entry_point,
        .bytecode_size = compute_shader_bytecode.bytecode.size(),
        .bytecode = compute_shader_bytecode.bytecode.data(),
    });

    for (const auto& binding : bindings) {
        if (!binding.compute_binding.succeeded() || binding.compute_binding.descriptor_set_layout.value == 0 ||
            binding.compute_binding.descriptor_set.value == 0 || binding.compute_binding.vertex_count == 0) {
            result.diagnostic = std::string{backend_name} + " scene compute morph skinned binding is incomplete";
            return result;
        }
        const auto compute_layout = device.create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {binding.compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
        const auto compute_pipeline = device.create_compute_pipeline(
            rhi::ComputePipelineDesc{.layout = compute_layout, .compute_shader = compute_shader});
        auto commands = device.begin_command_list(rhi::QueueKind::compute);
        commands->bind_compute_pipeline(compute_pipeline);
        commands->bind_descriptor_set(compute_layout, 0, binding.compute_binding.descriptor_set);
        commands->dispatch(binding.compute_binding.vertex_count, 1, 1);
        commands->close();
        const auto fence = device.submit(*commands);
        device.wait_for_queue(rhi::QueueKind::graphics, fence);
        ++result.dispatches;
        ++result.queue_waits;
    }
    return result;
}

[[nodiscard]] NativeUiOverlayAtlasBinding make_native_ui_overlay_atlas_binding(
    rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package, AssetId atlas_asset,
    std::string_view backend_name, std::vector<Win32DesktopPresentationNativeUiTextureOverlayDiagnostic>& diagnostics) {
    if (atlas_asset.value == 0) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            Win32DesktopPresentationNativeUiTextureOverlayStatus::invalid_request,
            std::string{backend_name} + " textured native UI overlay requires a non-zero atlas asset id."));
        return {};
    }

    const auto* record = package.find(atlas_asset);
    if (record == nullptr) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            Win32DesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} +
                " textured native UI overlay atlas asset is missing from the runtime package."));
        return {};
    }
    if (record->kind != AssetKind::texture) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            Win32DesktopPresentationNativeUiTextureOverlayStatus::invalid_request,
            std::string{backend_name} + " textured native UI overlay atlas asset is not a cooked texture payload."));
        return {};
    }

    const auto payload = runtime::runtime_texture_payload(*record);
    if (!payload.succeeded()) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            Win32DesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} + " textured native UI overlay atlas payload is invalid: " + payload.diagnostic));
        return {};
    }

    auto upload = runtime_rhi::upload_runtime_texture(device, payload.payload);
    if (!upload.succeeded()) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            Win32DesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} + " textured native UI overlay atlas upload failed: " + upload.diagnostic));
        return {};
    }

    auto sampler = device.create_sampler(rhi::SamplerDesc{});
    return NativeUiOverlayAtlasBinding{
        .atlas_page = atlas_asset,
        .texture = upload.texture,
        .sampler = sampler,
        .owner_device = upload.owner_device,
    };
}

[[nodiscard]] rhi::Format postprocess_scene_depth_format(bool enable_depth_input) noexcept {
    return enable_depth_input ? rhi::Format::depth24_stencil8 : rhi::Format::unknown;
}

[[nodiscard]] rhi::DepthStencilStateDesc postprocess_scene_depth_state(bool enable_depth_input) noexcept {
    if (!enable_depth_input) {
        return {};
    }
    return rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = rhi::CompareOp::less_equal};
}

[[nodiscard]] ShadowMapPlan make_scene_directional_shadow_plan(const SceneRenderPacket& packet,
                                                               Extent2D window_extent) {
    const auto tile_dim =
        std::min<std::uint32_t>(512u, std::max(128u, std::min(window_extent.width, window_extent.height) / 2u));
    return build_scene_shadow_map_plan(packet, SceneShadowMapDesc{
                                                   .extent = rhi::Extent2D{.width = tile_dim, .height = tile_dim},
                                                   .depth_format = rhi::Format::depth24_stencil8,
                                                   .directional_cascade_count = 4,
                                               });
}

[[nodiscard]] ShadowReceiverPlan make_scene_shadow_receiver_plan(const ShadowMapPlan& shadow_plan,
                                                                 const DirectionalShadowLightSpacePlan& light_space) {
    return build_shadow_receiver_plan(ShadowReceiverDesc{
        .shadow_map = &shadow_plan,
        .light_space = &light_space,
        .depth_bias = 0.003F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.42F,
    });
}

[[nodiscard]] std::string directional_shadow_plan_diagnostic(std::string_view backend_name,
                                                             const ShadowMapPlan& shadow_plan,
                                                             const DirectionalShadowLightSpacePlan& light_space,
                                                             const ShadowReceiverPlan& receiver_plan) {
    if (!shadow_plan.succeeded()) {
        return std::string{backend_name} +
               " directional shadow smoke requires one shadow-casting directional light and at least one mesh caster/"
               "receiver; using NullRenderer fallback.";
    }
    if (!light_space.succeeded()) {
        return std::string{backend_name} +
               " directional shadow light-space plan is invalid; using NullRenderer fallback.";
    }
    if (!receiver_plan.succeeded()) {
        return std::string{backend_name} + " directional shadow receiver plan is invalid; using NullRenderer fallback.";
    }
    return {};
}

[[nodiscard]] NativeRendererCreateResult create_d3d12_renderer(const Win32DesktopPresentationDesc& desc,
                                                               rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
    const bool native_sprite_overlay_requested =
        desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay;
    const bool native_sprite_texture_overlay_requested =
        desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures;
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::d3d12::create_rhi_device(rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = desc.prefer_warp,
            .enable_debug_layer = desc.enable_debug_layer,
        });
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "D3D12 device creation is unavailable; using NullRenderer fallback.",
            };
            if (native_sprite_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto pipeline_layout =
            device->create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->fragment_shader.bytecode.data(),
        });
        rhi::ShaderHandle native_sprite_overlay_vertex_shader;
        rhi::ShaderHandle native_sprite_overlay_fragment_shader;
        NativeUiOverlayAtlasBinding native_sprite_overlay_atlas;
        if (native_sprite_overlay_requested) {
            native_sprite_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
            });
            native_sprite_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
            });
        }
        std::vector<Win32DesktopPresentationNativeUiTextureOverlayDiagnostic> texture_overlay_diagnostics;
        if (native_sprite_texture_overlay_requested) {
            native_sprite_overlay_atlas =
                make_native_ui_overlay_atlas_binding(*device, *desc.d3d12_renderer->native_sprite_overlay_package,
                                                     desc.d3d12_renderer->native_sprite_overlay_atlas_asset,
                                                     "D3D12 native 2D sprite overlay", texture_overlay_diagnostics);
            if (native_sprite_overlay_atlas.texture.value == 0 || native_sprite_overlay_atlas.sampler.value == 0) {
                NativeRendererCreateResult result{
                    .succeeded = false,
                    .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                    .diagnostic = "D3D12 native 2D sprite overlay atlas upload failed; using NullRenderer fallback.",
                };
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
                result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
                result.native_ui_texture_overlay_requested = true;
                result.native_ui_texture_overlay_atlas_ready = false;
                result.native_ui_texture_overlay_diagnostics = std::move(texture_overlay_diagnostics);
                return result;
            }
        }
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.d3d12_renderer->topology,
            .vertex_buffers = desc.d3d12_renderer->vertex_buffers,
            .vertex_attributes = desc.d3d12_renderer->vertex_attributes,
        });
        auto renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
            .native_sprite_overlay_color_format = rhi::Format::bgra8_unorm,
            .native_sprite_overlay_vertex_shader = native_sprite_overlay_vertex_shader,
            .native_sprite_overlay_fragment_shader = native_sprite_overlay_fragment_shader,
            .native_sprite_overlay_atlas = native_sprite_overlay_atlas,
            .enable_native_sprite_overlay = native_sprite_overlay_requested,
            .enable_native_sprite_overlay_textures = native_sprite_texture_overlay_requested,
        });

        NativeRendererCreateResult result{
            .succeeded = true,
            .failure_reason = Win32DesktopPresentationFallbackReason::none,
            .diagnostic = {},
            .device = std::move(device),
            .renderer = std::move(renderer),
        };
        if (native_sprite_overlay_requested) {
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
            result.native_ui_overlay_ready = true;
        }
        if (native_sprite_texture_overlay_requested) {
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
            result.native_ui_texture_overlay_atlas_ready = true;
        }
        return result;
    } catch (const std::exception&) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "D3D12 renderer creation failed; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed,
                                                  result.diagnostic);
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        Win32DesktopPresentationFallbackReason::native_backend_unavailable,
        "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    if (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay) {
        mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

[[nodiscard]] NativeRendererCreateResult create_d3d12_scene_renderer(const Win32DesktopPresentationDesc& desc,
                                                                     rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
    if (desc.d3d12_scene_renderer == nullptr) {
        return missing_d3d12_scene_renderer_request();
    }
    const bool directional_shadow_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke;
    const bool native_ui_overlay_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay;
    const bool native_ui_texture_overlay_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures;
    const bool scene_compute_morph_mesh_requested =
        desc.d3d12_scene_renderer != nullptr &&
        has_shader_bytecode(desc.d3d12_scene_renderer->compute_morph_vertex_shader) &&
        has_shader_bytecode(desc.d3d12_scene_renderer->compute_morph_shader) &&
        !desc.d3d12_scene_renderer->compute_morph_mesh_bindings.empty();
    if (native_ui_texture_overlay_requested && !native_ui_overlay_requested) {
        return invalid_d3d12_native_ui_texture_overlay_request(
            "D3D12 textured native UI overlay requires native UI overlay to be enabled; using NullRenderer fallback.");
    }
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::unavailable,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_ui_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::d3d12::create_rhi_device(rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = desc.prefer_warp,
            .enable_debug_layer = desc.enable_debug_layer,
        });
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "D3D12 device creation is unavailable; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::unavailable,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto create_scene_graphics_pipeline =
            [&device](std::string_view label,
                      const rhi::GraphicsPipelineDesc& pipeline_desc) -> rhi::GraphicsPipelineHandle {
            try {
                return device->create_graphics_pipeline(pipeline_desc);
            } catch (const std::exception& exception) {
                throw std::invalid_argument(std::string(label) + " failed: " + exception.what());
            }
        };

        const float directional_shadow_viewport_aspect =
            desc.extent.height > 0 ? static_cast<float>(desc.extent.width) / static_cast<float>(desc.extent.height)
                                   : (16.0F / 9.0F);

        ShadowMapPlan shadow_map_plan{};
        DirectionalShadowLightSpacePlan directional_light_space_plan{};
        ShadowReceiverPlan shadow_receiver_plan{};
        rhi::DescriptorSetLayoutHandle shadow_receiver_layout;
        runtime_scene_rhi::RuntimeSceneGpuBindingOptions gpu_binding_options;
        gpu_binding_options.morph_mesh_assets = desc.d3d12_scene_renderer->morph_mesh_assets;
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->compute_morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings);
        for (const auto& binding : desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings) {
            gpu_binding_options.compute_morph_skinned_mesh_bindings.push_back(
                runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshBinding{.mesh = binding.mesh,
                                                                              .morph_mesh = binding.morph_mesh});
        }
        if (scene_compute_morph_mesh_requested) {
            gpu_binding_options.mesh_upload.vertex_usage =
                gpu_binding_options.mesh_upload.vertex_usage | rhi::BufferUsage::storage;
        }
        if (directional_shadow_requested) {
            shadow_map_plan = make_scene_directional_shadow_plan(*desc.d3d12_scene_renderer->packet, desc.extent);
            directional_light_space_plan = build_scene_directional_shadow_light_space_plan(
                *desc.d3d12_scene_renderer->packet, shadow_map_plan,
                SceneShadowLightSpaceDesc{.viewport_aspect = directional_shadow_viewport_aspect});
            shadow_receiver_plan = make_scene_shadow_receiver_plan(shadow_map_plan, directional_light_space_plan);
            if (const auto diagnostic = directional_shadow_plan_diagnostic(
                    "D3D12", shadow_map_plan, directional_light_space_plan, shadow_receiver_plan);
                !diagnostic.empty()) {
                return invalid_d3d12_directional_shadow_request(diagnostic);
            }
            shadow_receiver_layout = device->create_descriptor_set_layout(shadow_receiver_plan.descriptor_set_layout);
            gpu_binding_options.additional_pipeline_descriptor_set_layouts.push_back(shadow_receiver_layout);
        }

        auto gpu_upload =
            runtime_scene_rhi::execute_runtime_scene_gpu_upload(runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc{
                .device = device.get(),
                .package = desc.d3d12_scene_renderer->package,
                .packet = desc.d3d12_scene_renderer->packet,
                .binding_options = gpu_binding_options,
            });
        auto& gpu_bindings = gpu_upload.bindings;
        if (!gpu_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "D3D12 scene GPU binding creation failed; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            for (const auto& failure : gpu_bindings.failures) {
                result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                    result.scene_gpu_status, "scene GPU binding failure asset=" + std::to_string(failure.asset.value) +
                                                 ": " + failure.diagnostic));
            }
            return result;
        }
        if (gpu_bindings.material_pipeline_layouts.empty()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic =
                    "D3D12 scene GPU binding creation did not produce a material pipeline layout; using NullRenderer "
                    "fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        auto compute_morph_bindings = build_scene_compute_morph_bindings(
            *device, gpu_bindings, desc.d3d12_scene_renderer->compute_morph_mesh_bindings,
            desc.d3d12_scene_renderer->compute_morph_shader,
            desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output, "D3D12");
        if (!compute_morph_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_bindings.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        auto compute_morph_skinned_dispatch = dispatch_scene_compute_morph_skinned_bindings(
            *device, gpu_bindings.compute_morph_skinned_mesh_bindings,
            desc.d3d12_scene_renderer->compute_morph_skinned_shader, "D3D12");
        if (!compute_morph_skinned_dispatch.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_skinned_dispatch.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_scene_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->vertex_shader.bytecode.data(),
        });
        rhi::ShaderHandle compute_morph_scene_vertex_shader{};
        if (scene_compute_morph_mesh_requested) {
            compute_morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->compute_morph_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
            });
        }
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_scene_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->fragment_shader.bytecode.data(),
        });
        const bool scene_gpu_skinning_requested = has_shader_bytecode(desc.d3d12_scene_renderer->skinned_vertex_shader);
        const bool scene_gpu_morph_requested = has_shader_bytecode(desc.d3d12_scene_renderer->morph_vertex_shader) &&
                                               !desc.d3d12_scene_renderer->morph_mesh_bindings.empty();
        rhi::ShaderHandle skinned_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle skinned_scene_graphics_pipeline{};
        if (scene_gpu_skinning_requested) {
            skinned_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->skinned_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->skinned_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->skinned_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle morph_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle morph_scene_graphics_pipeline{};
        if (scene_gpu_morph_requested) {
            morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->morph_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->morph_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->morph_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle shifted_scene_fragment_shader_handle = fragment_shader;
        const bool shifted_shadow_receiver_fragment_requested =
            directional_shadow_requested && (scene_gpu_skinning_requested || scene_gpu_morph_requested) &&
            has_shader_bytecode(desc.d3d12_scene_renderer->skinned_scene_fragment_shader);
        if (shifted_shadow_receiver_fragment_requested) {
            shifted_scene_fragment_shader_handle = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.bytecode.data(),
            });
        }
        const auto scene_fragment_shader_handle =
            shifted_shadow_receiver_fragment_requested ? shifted_scene_fragment_shader_handle : fragment_shader;
        const bool shadow_stage_skinned =
            scene_gpu_skinning_requested && scene_packet_references_skinned_mesh(*desc.d3d12_scene_renderer->package,
                                                                                 *desc.d3d12_scene_renderer->packet);
        const auto& shadow_stage_vertex_buffers = shadow_stage_skinned
                                                      ? desc.d3d12_scene_renderer->skinned_vertex_buffers
                                                      : desc.d3d12_scene_renderer->vertex_buffers;
        const auto& shadow_stage_vertex_attributes = shadow_stage_skinned
                                                         ? desc.d3d12_scene_renderer->skinned_vertex_attributes
                                                         : desc.d3d12_scene_renderer->vertex_attributes;
        rhi::GraphicsPipelineHandle shadow_pipeline;
        if (directional_shadow_requested) {
            const auto shadow_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->shadow_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->shadow_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->shadow_vertex_shader.bytecode.data(),
            });
            const auto shadow_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->shadow_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->shadow_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->shadow_fragment_shader.bytecode.data(),
            });
            const auto shadow_pipeline_layout = device->create_pipeline_layout(
                rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
            shadow_pipeline = create_scene_graphics_pipeline(
                "D3D12 shadow pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = shadow_pipeline_layout,
                    .vertex_shader = shadow_vertex_shader,
                    .fragment_shader = shadow_fragment_shader,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = rhi::Format::depth24_stencil8,
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = shadow_stage_vertex_buffers,
                    .vertex_attributes = shadow_stage_vertex_attributes,
                    .depth_state = rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                              .depth_write_enabled = true,
                                                              .depth_compare = rhi::CompareOp::less_equal},
                });
        }
        const bool postprocess_depth_input_requested = desc.d3d12_scene_renderer->enable_postprocess_depth_input;
        const bool enable_postprocess_depth_input =
            desc.d3d12_scene_renderer->enable_postprocess && postprocess_depth_input_requested;
        const bool environment_fog_requested = desc.d3d12_scene_renderer->enable_environment_fog;
        const bool physical_sky_requested = desc.d3d12_scene_renderer->enable_physical_sky_package_evidence;
        const bool cloud_layer_renderer_execution_requested =
            desc.d3d12_scene_renderer->enable_cloud_layer_renderer_execution;
        const bool cloud_layer_requested =
            desc.d3d12_scene_renderer->enable_cloud_layer_package_evidence || cloud_layer_renderer_execution_requested;
        const bool environment_precipitation_renderer_execution_requested =
            desc.d3d12_scene_renderer->enable_environment_precipitation_renderer_execution;
        const bool environment_precipitation_requested =
            desc.d3d12_scene_renderer->enable_environment_precipitation_package_evidence ||
            environment_precipitation_renderer_execution_requested;
        const bool environment_volumetric_fog_requested =
            desc.d3d12_scene_renderer->enable_environment_volumetric_fog_package_evidence;
        const bool environment_volumetric_cloud_renderer_execution_requested =
            desc.d3d12_scene_renderer->enable_environment_volumetric_cloud_renderer_execution;
        const bool environment_volumetric_cloud_requested =
            desc.d3d12_scene_renderer->enable_environment_volumetric_cloud_package_evidence ||
            environment_volumetric_cloud_renderer_execution_requested;
        const auto compute_morph_vertex_buffers = desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output
                                                      ? compute_morph_tangent_frame_vertex_buffers()
                                                      : compute_morph_position_vertex_buffers();
        const auto compute_morph_vertex_attributes =
            desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output
                ? compute_morph_tangent_frame_vertex_attributes()
                : compute_morph_position_vertex_attributes();
        const auto pipeline = create_scene_graphics_pipeline(
            "D3D12 scene pipeline",
            rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = scene_compute_morph_mesh_requested ? compute_morph_scene_vertex_shader : vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.d3d12_scene_renderer->topology,
                .vertex_buffers = scene_compute_morph_mesh_requested ? compute_morph_vertex_buffers
                                                                     : desc.d3d12_scene_renderer->vertex_buffers,
                .vertex_attributes = scene_compute_morph_mesh_requested ? compute_morph_vertex_attributes
                                                                        : desc.d3d12_scene_renderer->vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        if (scene_gpu_skinning_requested) {
            skinned_scene_graphics_pipeline = create_scene_graphics_pipeline(
                "D3D12 skinned scene pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = gpu_bindings.material_pipeline_layouts.front(),
                    .vertex_shader = skinned_scene_vertex_shader,
                    .fragment_shader = scene_fragment_shader_handle,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = desc.d3d12_scene_renderer->skinned_vertex_buffers,
                    .vertex_attributes = desc.d3d12_scene_renderer->skinned_vertex_attributes,
                    .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
                });
        }
        if (scene_gpu_morph_requested) {
            morph_scene_graphics_pipeline = create_scene_graphics_pipeline(
                "D3D12 morph scene pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = gpu_bindings.material_pipeline_layouts.front(),
                    .vertex_shader = morph_scene_vertex_shader,
                    .fragment_shader = scene_fragment_shader_handle,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = desc.d3d12_scene_renderer->vertex_buffers,
                    .vertex_attributes = desc.d3d12_scene_renderer->vertex_attributes,
                    .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
                });
        }
        std::unique_ptr<IRenderer> frame_renderer;
        NativeRendererCreateResult result;
        result.succeeded = true;
        result.failure_reason = Win32DesktopPresentationFallbackReason::none;
        result.postprocess_depth_input_requested = postprocess_depth_input_requested;
        result.environment_fog_requested = environment_fog_requested;
        result.physical_sky_requested = physical_sky_requested;
        result.cloud_layer_requested = cloud_layer_requested;
        result.environment_precipitation_requested = environment_precipitation_requested;
        result.environment_volumetric_fog_requested = environment_volumetric_fog_requested;
        result.environment_volumetric_cloud_requested = environment_volumetric_cloud_requested;
        result.directional_shadow_requested = directional_shadow_requested;
        result.native_ui_overlay_requested = native_ui_overlay_requested;
        result.native_ui_texture_overlay_requested = native_ui_texture_overlay_requested;
        CloudLayerRuntimeBinding cloud_layer_binding;
        if (physical_sky_requested) {
            auto physical_sky_desc = desc.d3d12_scene_renderer->physical_sky;
            physical_sky_desc.package_evidence_ready =
                physical_sky_desc.package_evidence_ready && desc.d3d12_scene_renderer->package != nullptr;
            const auto physical_sky_plan = plan_physical_sky_policy(physical_sky_desc);
            apply_physical_sky_plan(result, physical_sky_plan);
            if (!physical_sky_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "D3D12 physical sky package evidence failed the physical sky policy; using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
            const auto physical_sky_constants_buffer = create_physical_sky_constants_buffer(*device, physical_sky_desc);
            result.physical_sky_constant_buffer_ready = physical_sky_constants_buffer.value != 0;
            result.physical_sky_constant_buffer_bytes = static_cast<std::uint64_t>(physical_sky_constants_byte_size());
            if (!result.physical_sky_constant_buffer_ready) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic = "D3D12 physical sky constants buffer creation failed; using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
        }
        if (cloud_layer_requested) {
            auto cloud_layer_desc = desc.d3d12_scene_renderer->cloud_layer;
            cloud_layer_desc.package_evidence_ready =
                cloud_layer_desc.package_evidence_ready &&
                cloud_layer_package_texture_evidence_ready(desc.d3d12_scene_renderer->package, cloud_layer_desc);
            cloud_layer_desc.request_texture_upload =
                cloud_layer_desc.request_texture_upload || cloud_layer_renderer_execution_requested;
            cloud_layer_desc.request_backend_execution =
                cloud_layer_desc.request_backend_execution || cloud_layer_renderer_execution_requested;
            const auto cloud_layer_plan = plan_cloud_layer_policy(cloud_layer_desc);
            apply_cloud_layer_plan(result, cloud_layer_plan);
            if (!cloud_layer_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "D3D12 cloud layer package evidence failed the cloud layer policy; using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
            if (cloud_layer_renderer_execution_requested) {
                cloud_layer_binding = create_cloud_layer_runtime_binding(
                    *device, *desc.d3d12_scene_renderer->package, cloud_layer_desc,
                    desc.d3d12_scene_renderer->cloud_layer_vertex_shader,
                    desc.d3d12_scene_renderer->cloud_layer_fragment_shader, rhi::Format::bgra8_unorm,
                    postprocess_scene_depth_format(enable_postprocess_depth_input), "D3D12");
                if (!cloud_layer_binding.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = cloud_layer_binding.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    return result;
                }
                result.cloud_layer_uploads_textures = cloud_layer_binding.texture_uploads > 0;
                result.cloud_layer_invokes_backend = true;
            }
        }
        if (environment_precipitation_requested) {
            auto precipitation_desc = desc.d3d12_scene_renderer->environment_precipitation;
            precipitation_desc.package_evidence_ready =
                precipitation_desc.package_evidence_ready &&
                environment_precipitation_package_texture_evidence_ready(desc.d3d12_scene_renderer->package);
            if (environment_precipitation_renderer_execution_requested) {
                const auto precipitation_probe = execute_precipitation_runtime_probe(
                    *device, *desc.d3d12_scene_renderer->package, precipitation_desc,
                    desc.d3d12_scene_renderer->precipitation_vertex_shader,
                    desc.d3d12_scene_renderer->precipitation_fragment_shader, "D3D12");
                if (!precipitation_probe.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = precipitation_probe.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    return result;
                }
                precipitation_desc.request_particle_buffer_upload = true;
                precipitation_desc.request_backend_execution = true;
                precipitation_desc.particle_buffer_upload_count = precipitation_probe.particle_buffer_uploads;
                precipitation_desc.backend_invocation_count = precipitation_probe.backend_invocations;
                precipitation_desc.renderer_draw_count = precipitation_probe.renderer_draws;
                precipitation_desc.depth_occlusion_readback_proven = precipitation_probe.depth_occlusion_readback;
            }
            const auto precipitation_plan = plan_precipitation_policy(precipitation_desc);
            apply_precipitation_plan(result, precipitation_desc, precipitation_plan);
            if (!precipitation_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "D3D12 environment precipitation package evidence failed the precipitation policy; using "
                    "NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
        }
        if (environment_volumetric_cloud_requested) {
            auto volumetric_cloud_desc = desc.d3d12_scene_renderer->environment_volumetric_cloud;
            volumetric_cloud_desc.package_evidence_ready =
                volumetric_cloud_desc.package_evidence_ready &&
                environment_volumetric_cloud_package_evidence_ready(desc.d3d12_scene_renderer->package,
                                                                    volumetric_cloud_desc);
            VolumetricCloudRuntimeProbeResult volumetric_cloud_probe;
            if (environment_volumetric_cloud_renderer_execution_requested) {
                if (desc.d3d12_scene_renderer->package == nullptr) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = "D3D12 volumetric cloud renderer execution requires a runtime package; using "
                                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    return result;
                }
                volumetric_cloud_probe = execute_volumetric_cloud_runtime_probe(
                    *device, *desc.d3d12_scene_renderer->package, volumetric_cloud_desc,
                    desc.d3d12_scene_renderer->volumetric_cloud_vertex_shader,
                    desc.d3d12_scene_renderer->volumetric_cloud_fragment_shader, "D3D12");
                if (!volumetric_cloud_probe.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = volumetric_cloud_probe.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    return result;
                }
                volumetric_cloud_desc.request_volume_texture_upload = true;
                volumetric_cloud_desc.request_backend_execution = true;
                volumetric_cloud_desc.weather_map_upload_count = volumetric_cloud_probe.weather_map_uploads;
                volumetric_cloud_desc.shape_noise_upload_count = volumetric_cloud_probe.shape_noise_uploads;
                volumetric_cloud_desc.erosion_noise_upload_count = volumetric_cloud_probe.erosion_noise_uploads;
                volumetric_cloud_desc.backend_invocation_count = volumetric_cloud_probe.backend_invocations;
                volumetric_cloud_desc.raymarch_pass_count = volumetric_cloud_probe.raymarch_passes;
                volumetric_cloud_desc.readback_nonzero_proven = volumetric_cloud_probe.readback_nonzero;
            }
            const auto volumetric_cloud_plan = plan_volumetric_cloud_policy(volumetric_cloud_desc);
            apply_volumetric_cloud_plan(result, volumetric_cloud_plan);
            result.environment_volumetric_cloud_renderer_draws = volumetric_cloud_probe.renderer_draws;
            if (!volumetric_cloud_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "D3D12 environment volumetric cloud package evidence failed the volumetric cloud policy; "
                    "using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
        }
        if (environment_volumetric_fog_requested) {
            if (!enable_postprocess_depth_input) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "D3D12 volumetric fog package evidence requires scene-depth input; using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
            auto volumetric_fog_desc = desc.d3d12_scene_renderer->environment_volumetric_fog;
            volumetric_fog_desc.scene_depth_available = true;
            volumetric_fog_desc.request_ready_promotion = true;
            const auto volumetric_fog_plan = plan_volumetric_fog_policy(volumetric_fog_desc);
            apply_volumetric_fog_plan(result, volumetric_fog_plan);
            if (!volumetric_fog_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic = "D3D12 volumetric fog package evidence failed the volumetric fog policy; using "
                                    "NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
        }
        if (desc.d3d12_scene_renderer->enable_postprocess) {
            rhi::BufferHandle environment_fog_constants_buffer;
            if (environment_fog_requested) {
                if (!enable_postprocess_depth_input || directional_shadow_requested) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "D3D12 environment fog package evidence requires depth-aware non-shadow scene postprocess; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                auto environment_fog_desc = desc.d3d12_scene_renderer->environment_fog;
                environment_fog_desc.scene_depth_available = enable_postprocess_depth_input;
                environment_fog_desc.shader_contract_evidence_ready =
                    has_shader_bytecode(desc.d3d12_scene_renderer->postprocess_fragment_shader);
                const auto environment_fog_plan = plan_environment_fog_policy(environment_fog_desc);
                if (!environment_fog_plan.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "D3D12 environment fog package evidence failed the environment fog policy; using "
                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                environment_fog_constants_buffer =
                    create_environment_fog_constants_buffer(*device, environment_fog_desc);
                result.environment_fog_constant_buffer_ready = environment_fog_constants_buffer.value != 0;
                result.environment_fog_constant_buffer_bytes =
                    static_cast<std::uint64_t>(environment_fog_constants_byte_size());
                if (!result.environment_fog_constant_buffer_ready) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "D3D12 environment fog package evidence could not create the fog constant buffer; using "
                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
            }
            const auto postprocess_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->postprocess_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->postprocess_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->postprocess_vertex_shader.bytecode.data(),
            });
            const auto postprocess_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->postprocess_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->postprocess_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->postprocess_fragment_shader.bytecode.data(),
            });
            rhi::ShaderHandle native_ui_overlay_vertex_shader;
            rhi::ShaderHandle native_ui_overlay_fragment_shader;
            if (native_ui_overlay_requested) {
                native_ui_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::vertex,
                    .entry_point = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.entry_point,
                    .bytecode_size = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
                    .bytecode = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                });
                native_ui_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::fragment,
                    .entry_point = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.entry_point,
                    .bytecode_size = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
                    .bytecode = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                });
            }
            NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
            if (native_ui_texture_overlay_requested) {
                native_ui_overlay_atlas =
                    make_native_ui_overlay_atlas_binding(*device, *desc.d3d12_scene_renderer->package,
                                                         desc.d3d12_scene_renderer->native_ui_overlay_atlas_asset,
                                                         "D3D12", result.native_ui_texture_overlay_diagnostics);
                if (native_ui_overlay_atlas.texture.value == 0) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = result.native_ui_texture_overlay_diagnostics.empty()
                                            ? "D3D12 textured native UI overlay atlas binding failed; using "
                                              "NullRenderer fallback."
                                            : result.native_ui_texture_overlay_diagnostics.front().message +
                                                  "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                                  result.diagnostic);
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
                    return result;
                }
            }
            std::unique_ptr<IRenderer> postprocess_renderer;
            if (directional_shadow_requested) {
                std::array<std::uint8_t, shadow_receiver_constants_byte_size()> shadow_cb{};
                Mat4 camera_view = Mat4::identity();
                if (const auto* primary = desc.d3d12_scene_renderer->packet->primary_camera(); primary != nullptr) {
                    camera_view =
                        make_scene_camera_matrices(*primary, directional_shadow_viewport_aspect).view_from_world;
                }
                pack_shadow_receiver_constants(shadow_cb, directional_light_space_plan,
                                               shadow_map_plan.directional_cascade_count, camera_view);
                const bool scene_deformation_descriptor_set =
                    gpu_bindings.skinned_joint_descriptor_set_layout.value != 0 ||
                    gpu_bindings.morph_descriptor_set_layout.value != 0;
                const std::uint32_t shadow_receiver_descriptor_set_index = scene_deformation_descriptor_set ? 2u : 1u;
                auto shadow_renderer =
                    std::make_unique<RhiDirectionalShadowSmokeFrameRenderer>(RhiDirectionalShadowSmokeFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .scene_pipeline_layout = gpu_bindings.material_pipeline_layouts.front(),
                        .shadow_graphics_pipeline = shadow_pipeline,
                        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .scene_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_filter_mode = shadow_receiver_plan.filter_mode,
                        .shadow_filter_radius_texels = shadow_receiver_plan.filter_radius_texels,
                        .shadow_filter_tap_count = shadow_receiver_plan.filter_tap_count,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                        .shadow_depth_atlas_extent = Extent2D{.width = shadow_map_plan.depth_texture.extent.width,
                                                              .height = shadow_map_plan.depth_texture.extent.height},
                        .directional_shadow_cascade_count = shadow_map_plan.directional_cascade_count,
                        .shadow_receiver_constants_initial = shadow_cb,
                        .shadow_receiver_descriptor_set_index = shadow_receiver_descriptor_set_index,
                    });
                result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::ready;
                result.directional_shadow_ready = shadow_renderer->directional_shadow_ready();
                result.directional_shadow_filter_mode =
                    to_presentation_filter_mode(shadow_renderer->shadow_filter_mode());
                result.directional_shadow_filter_tap_count = shadow_renderer->shadow_filter_tap_count();
                result.directional_shadow_filter_radius_texels = shadow_renderer->shadow_filter_radius_texels();
                const auto shadow_atlas_extent = shadow_renderer->shadow_atlas_extent();
                result.directional_shadow_cascade_count = shadow_renderer->directional_shadow_cascade_count();
                result.directional_shadow_cascade_tile_width = shadow_renderer->shadow_cascade_tile_width();
                result.directional_shadow_atlas_width = shadow_atlas_extent.width;
                result.directional_shadow_atlas_height = shadow_atlas_extent.height;
                result.directional_shadow_light_space_cascades =
                    static_cast<std::uint32_t>(directional_light_space_plan.clip_from_world_cascades.size());
                result.directional_shadow_cascade_splits =
                    static_cast<std::uint32_t>(directional_light_space_plan.cascade_split_distances.size());
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = shadow_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready = shadow_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = shadow_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(shadow_renderer);
            } else {
                auto color_postprocess_renderer =
                    std::make_unique<RhiPostprocessFrameRenderer>(RhiPostprocessFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .postprocess_first_uniform_buffer = environment_fog_constants_buffer,
                        .wait_for_completion = true,
                        .enable_depth_input = enable_postprocess_depth_input,
                        .depth_format = rhi::Format::depth24_stencil8,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                        .cloud_layer_graphics_pipeline = cloud_layer_binding.pipeline,
                        .cloud_layer_pipeline_layout = cloud_layer_binding.pipeline_layout,
                        .cloud_layer_descriptor_set = cloud_layer_binding.descriptor_set,
                        .enable_cloud_layer = cloud_layer_renderer_execution_requested,
                    });
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = color_postprocess_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready =
                        color_postprocess_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = color_postprocess_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(color_postprocess_renderer);
            }
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::ready;
            result.postprocess_depth_input_ready = enable_postprocess_depth_input;
            frame_renderer = std::move(postprocess_renderer);
        } else {
            frame_renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
                .device = device.get(),
                .extent = desc.extent,
                .color_texture = rhi::TextureHandle{},
                .swapchain = swapchain,
                .graphics_pipeline = pipeline,
                .wait_for_completion = true,
                .skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                .morph_graphics_pipeline = morph_scene_graphics_pipeline,
                .cloud_layer_graphics_pipeline = cloud_layer_binding.pipeline,
                .cloud_layer_pipeline_layout = cloud_layer_binding.pipeline_layout,
                .cloud_layer_descriptor_set = cloud_layer_binding.descriptor_set,
                .enable_cloud_layer = cloud_layer_renderer_execution_requested,
            });
        }
        auto scene_renderer = std::make_unique<SceneGpuBindingInjectingRenderer>(
            std::move(frame_renderer), std::move(gpu_bindings), desc.d3d12_scene_renderer->morph_mesh_bindings,
            std::move(compute_morph_bindings.bindings), compute_morph_bindings.queue_waits,
            compute_morph_skinned_dispatch.dispatches, compute_morph_skinned_dispatch.queue_waits);
        auto* scene_renderer_ptr = scene_renderer.get();

        result.device = std::move(device);
        result.renderer = std::move(scene_renderer);
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::ready;
        result.scene_gpu_renderer = scene_renderer_ptr;
        return result;
    } catch (const std::exception& exception) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "D3D12 scene renderer creation failed: " + std::string(exception.what()) +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess) {
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            result.postprocess_depth_input_requested = desc.d3d12_scene_renderer->enable_postprocess_depth_input;
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
            result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::failed;
            result.directional_shadow_requested = true;
            result.directional_shadow_diagnostics.push_back(
                make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) {
            result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::failed;
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_diagnostics.push_back(
                make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
            result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_diagnostics.push_back(
                make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        Win32DesktopPresentationFallbackReason::native_backend_unavailable,
        "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
        mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::unavailable,
                                       result.diagnostic);
    }
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) {
        mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN)
[[nodiscard]] rhi::vulkan::VulkanSwapchainCreatePlan
make_vulkan_presentation_swapchain_mapping_plan(rhi::vulkan::VulkanRuntimeDevice& device, rhi::SurfaceHandle surface,
                                                Extent2D extent, rhi::Format format, bool vsync) {
    rhi::vulkan::VulkanSwapchainCreatePlan plan;
    if (surface.value == 0 || extent.width == 0 || extent.height == 0 || format == rhi::Format::unknown ||
        !device.has_graphics_queue() || !device.has_present_queue()) {
        plan.diagnostic = "Vulkan presentation swapchain mapping requires a ready surface and graphics/present queues";
        return plan;
    }

    plan.supported = true;
    plan.extent = rhi::Extent2D{.width = extent.width, .height = extent.height};
    plan.format = format;
    plan.image_count = 2;
    plan.image_view_count = 2;
    plan.present_mode = vsync ? rhi::vulkan::VulkanPresentMode::fifo : rhi::vulkan::VulkanPresentMode::mailbox;
    plan.acquire_before_render = true;
    plan.diagnostic = "Vulkan presentation swapchain mapping ready";
    return plan;
}

[[nodiscard]] rhi::vulkan::VulkanDynamicRenderingPlan
make_vulkan_presentation_dynamic_rendering_plan(rhi::vulkan::VulkanRuntimeDevice& device, Extent2D extent,
                                                rhi::Format color_format) {
    rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = rhi::Extent2D{.width = extent.width, .height = extent.height};
    rendering_desc.color_attachments.push_back(rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = color_format,
        .load_action = rhi::LoadAction::clear,
        .store_action = rhi::StoreAction::store,
    });
    return rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, device.command_plan());
}

[[nodiscard]] rhi::vulkan::VulkanFrameSynchronizationPlan
make_vulkan_presentation_frame_synchronization_plan(rhi::vulkan::VulkanRuntimeDevice& device) {
    rhi::vulkan::VulkanFrameSynchronizationDesc sync_desc;
    sync_desc.readback_required = true;
    sync_desc.present_required = true;
    return rhi::vulkan::build_frame_synchronization_plan(sync_desc, device.command_plan());
}

[[nodiscard]] bool probe_vulkan_runtime_command_pool_ready(rhi::vulkan::VulkanRuntimeDevice& device) {
    auto pool_result = rhi::vulkan::create_runtime_command_pool(device, {});
    return pool_result.created && pool_result.pool.owns_pool() && pool_result.pool.owns_primary_command_buffer();
}

[[nodiscard]] bool probe_vulkan_runtime_descriptor_binding_ready(rhi::vulkan::VulkanRuntimeDevice& device) {
    auto pool_result = rhi::vulkan::create_runtime_command_pool(device, {});
    if (!pool_result.created || !pool_result.pool.owns_primary_command_buffer()) {
        return false;
    }

    rhi::vulkan::VulkanRuntimeDescriptorSetLayoutDesc layout_desc;
    layout_desc.layout.bindings.push_back(rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = rhi::DescriptorType::uniform_buffer,
        .count = 1,
        .stages = rhi::ShaderStageVisibility::vertex,
    });
    auto layout_result = rhi::vulkan::create_runtime_descriptor_set_layout(device, layout_desc);
    if (!layout_result.created) {
        return false;
    }
    auto set_result = rhi::vulkan::create_runtime_descriptor_set(device, layout_result.layout, {});
    if (!set_result.created) {
        return false;
    }

    rhi::vulkan::VulkanRuntimePipelineLayoutDesc pipeline_layout_desc;
    pipeline_layout_desc.descriptor_set_layouts.push_back(&layout_result.layout);
    auto pipeline_layout_result = rhi::vulkan::create_runtime_pipeline_layout(device, pipeline_layout_desc);
    if (!pipeline_layout_result.created) {
        return false;
    }
    if (!pool_result.pool.begin_primary_command_buffer()) {
        return false;
    }

    const auto bind_result = rhi::vulkan::record_runtime_descriptor_set_binding(
        device, pool_result.pool, pipeline_layout_result.layout, set_result.set, {});
    const auto ended = pool_result.pool.end_primary_command_buffer();
    return bind_result.recorded && ended;
}

[[nodiscard]] bool probe_vulkan_runtime_depth_mapping_ready(rhi::vulkan::VulkanRuntimeDevice& device,
                                                            rhi::Format color_format) {
    rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = rhi::Extent2D{.width = 1, .height = 1};
    rendering_desc.color_attachments.push_back(rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = color_format,
        .load_action = rhi::LoadAction::clear,
        .store_action = rhi::StoreAction::store,
    });
    rendering_desc.has_depth_attachment = true;
    rendering_desc.depth_format = rhi::Format::depth24_stencil8;

    const auto rendering_plan = rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, device.command_plan());
    const auto depth_barrier =
        rhi::vulkan::build_texture_transition_barrier(rhi::ResourceState::undefined, rhi::ResourceState::depth_write);
    return rendering_plan.supported && rendering_plan.depth_attachment_enabled && depth_barrier.supported;
}

[[nodiscard]] rhi::vulkan::VulkanRhiDeviceMappingPlan build_vulkan_presentation_mapping_plan(
    rhi::vulkan::VulkanRuntimeDevice& device, const rhi::vulkan::VulkanSpirvShaderArtifactValidation& vertex_validation,
    const rhi::vulkan::VulkanSpirvShaderArtifactValidation& fragment_validation, rhi::SurfaceHandle surface,
    Extent2D extent, rhi::Format color_format, bool vsync,
    const rhi::vulkan::VulkanSpirvShaderArtifactValidation* compute_validation = nullptr) {
    rhi::vulkan::VulkanRhiDeviceMappingDesc mapping_desc;
    mapping_desc.command_pool_ready = probe_vulkan_runtime_command_pool_ready(device);
    mapping_desc.swapchain =
        make_vulkan_presentation_swapchain_mapping_plan(device, surface, extent, color_format, vsync);
    mapping_desc.dynamic_rendering = make_vulkan_presentation_dynamic_rendering_plan(device, extent, color_format);
    mapping_desc.frame_synchronization = make_vulkan_presentation_frame_synchronization_plan(device);
    mapping_desc.vertex_shader = vertex_validation;
    mapping_desc.fragment_shader = fragment_validation;
    if (compute_validation != nullptr) {
        mapping_desc.compute_shader = *compute_validation;
        mapping_desc.compute_dispatch_ready = compute_validation->valid && device.command_plan().supported;
    }
    mapping_desc.descriptor_binding_ready = probe_vulkan_runtime_descriptor_binding_ready(device);
    mapping_desc.visible_clear_readback_ready = mapping_desc.swapchain.supported &&
                                                mapping_desc.dynamic_rendering.supported &&
                                                mapping_desc.frame_synchronization.supported;
    mapping_desc.visible_draw_readback_ready =
        mapping_desc.visible_clear_readback_ready && vertex_validation.valid && fragment_validation.valid;
    mapping_desc.visible_texture_sampling_readback_ready =
        mapping_desc.visible_draw_readback_ready && mapping_desc.descriptor_binding_ready;
    mapping_desc.visible_depth_readback_ready =
        mapping_desc.visible_draw_readback_ready && probe_vulkan_runtime_depth_mapping_ready(device, color_format);
    return rhi::vulkan::build_rhi_device_mapping_plan(mapping_desc);
}
#endif

[[nodiscard]] NativeRendererCreateResult create_vulkan_renderer(const Win32DesktopPresentationDesc& desc,
                                                                rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN)
    if (desc.vulkan_renderer == nullptr || !valid_vulkan_renderer_request(desc.vulkan_renderer)) {
        return NativeRendererCreateResult{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan renderer request is incomplete; using NullRenderer fallback.",
        };
    }
    const bool native_sprite_overlay_requested =
        desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay;
    const bool native_sprite_texture_overlay_requested =
        desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures;
    const auto vertex_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::vertex,
            .bytecode = desc.vulkan_renderer->vertex_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_renderer->vertex_shader.bytecode.size(),
        });
    if (!vertex_validation.valid) {
        return NativeRendererCreateResult{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan vertex SPIR-V validation failed: " + vertex_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
    }

    const auto fragment_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::fragment,
            .bytecode = desc.vulkan_renderer->fragment_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_renderer->fragment_shader.bytecode.size(),
        });
    if (!fragment_validation.valid) {
        return NativeRendererCreateResult{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan fragment SPIR-V validation failed: " + fragment_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
    }
    if (native_sprite_overlay_requested) {
        const auto native_sprite_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
            });
        if (!native_sprite_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native 2D sprite overlay vertex SPIR-V validation failed: " +
                              native_sprite_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }

        const auto native_sprite_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
            });
        if (!native_sprite_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native 2D sprite overlay fragment SPIR-V validation failed: " +
                              native_sprite_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
    }

    auto runtime_device = rhi::vulkan::create_runtime_device({}, {}, {}, surface);
    if (!runtime_device.created) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "Vulkan runtime device creation failed: " + runtime_device.diagnostic +
                          "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    const auto mapping_plan =
        build_vulkan_presentation_mapping_plan(runtime_device.device, vertex_validation, fragment_validation, surface,
                                               desc.extent, rhi::Format::bgra8_unorm, desc.vsync);
    if (!mapping_plan.supported) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan IRhiDevice mapping is unavailable: " + mapping_plan.diagnostic +
                          "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::vulkan::create_rhi_device(std::move(runtime_device.device), mapping_plan);
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "Vulkan device creation is unavailable; using NullRenderer fallback.",
            };
            if (native_sprite_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto pipeline_layout =
            device->create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.vulkan_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.vulkan_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.vulkan_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.vulkan_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.vulkan_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.vulkan_renderer->fragment_shader.bytecode.data(),
        });
        rhi::ShaderHandle native_sprite_overlay_vertex_shader;
        rhi::ShaderHandle native_sprite_overlay_fragment_shader;
        NativeUiOverlayAtlasBinding native_sprite_overlay_atlas;
        if (native_sprite_overlay_requested) {
            native_sprite_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
            });
            native_sprite_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
            });
        }
        std::vector<Win32DesktopPresentationNativeUiTextureOverlayDiagnostic> texture_overlay_diagnostics;
        if (native_sprite_texture_overlay_requested) {
            native_sprite_overlay_atlas =
                make_native_ui_overlay_atlas_binding(*device, *desc.vulkan_renderer->native_sprite_overlay_package,
                                                     desc.vulkan_renderer->native_sprite_overlay_atlas_asset,
                                                     "Vulkan native 2D sprite overlay", texture_overlay_diagnostics);
            if (native_sprite_overlay_atlas.texture.value == 0 || native_sprite_overlay_atlas.sampler.value == 0) {
                NativeRendererCreateResult result{
                    .succeeded = false,
                    .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                    .diagnostic = "Vulkan native 2D sprite overlay atlas upload failed; using NullRenderer fallback.",
                };
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
                result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
                result.native_ui_texture_overlay_requested = true;
                result.native_ui_texture_overlay_atlas_ready = false;
                result.native_ui_texture_overlay_diagnostics = std::move(texture_overlay_diagnostics);
                return result;
            }
        }
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.vulkan_renderer->topology,
            .vertex_buffers = desc.vulkan_renderer->vertex_buffers,
            .vertex_attributes = desc.vulkan_renderer->vertex_attributes,
        });
        auto renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
            .native_sprite_overlay_color_format = rhi::Format::bgra8_unorm,
            .native_sprite_overlay_vertex_shader = native_sprite_overlay_vertex_shader,
            .native_sprite_overlay_fragment_shader = native_sprite_overlay_fragment_shader,
            .native_sprite_overlay_atlas = native_sprite_overlay_atlas,
            .enable_native_sprite_overlay = native_sprite_overlay_requested,
            .enable_native_sprite_overlay_textures = native_sprite_texture_overlay_requested,
        });

        NativeRendererCreateResult result{
            .succeeded = true,
            .failure_reason = Win32DesktopPresentationFallbackReason::none,
            .diagnostic = {},
            .device = std::move(device),
            .renderer = std::move(renderer),
        };
        if (native_sprite_overlay_requested) {
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
            result.native_ui_overlay_ready = true;
        }
        if (native_sprite_texture_overlay_requested) {
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
            result.native_ui_texture_overlay_atlas_ready = true;
        }
        return result;
    } catch (const std::exception& error) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic =
                std::string{"Vulkan renderer creation failed: "} + error.what() + "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(result, Win32DesktopPresentationNativeUiTextureOverlayStatus::failed,
                                                  result.diagnostic);
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        Win32DesktopPresentationFallbackReason::native_backend_unavailable,
        "Vulkan runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    if (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay) {
        mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

[[nodiscard]] NativeRendererCreateResult create_vulkan_scene_renderer(const Win32DesktopPresentationDesc& desc,
                                                                      rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN)
    if (desc.vulkan_scene_renderer == nullptr) {
        return missing_vulkan_scene_renderer_request();
    }
    const bool directional_shadow_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke;
    const bool native_ui_overlay_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay;
    const bool native_ui_texture_overlay_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures;
    const bool scene_compute_morph_mesh_requested =
        desc.vulkan_scene_renderer != nullptr &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_vertex_shader) &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_shader) &&
        !desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty();
    const bool scene_compute_morph_skinned_requested =
        desc.vulkan_scene_renderer != nullptr &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_skinned_shader) &&
        !desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings.empty();
    if (native_ui_texture_overlay_requested && !native_ui_overlay_requested) {
        return invalid_vulkan_native_ui_texture_overlay_request(
            "Vulkan textured native UI overlay requires native UI overlay to be enabled; using NullRenderer fallback.");
    }
    const auto vertex_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::vertex,
            .bytecode = desc.vulkan_scene_renderer->vertex_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_scene_renderer->vertex_shader.bytecode.size(),
        });
    if (!vertex_validation.valid) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene vertex SPIR-V validation failed: " + vertex_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    const auto fragment_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::fragment,
            .bytecode = desc.vulkan_scene_renderer->fragment_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_scene_renderer->fragment_shader.bytecode.size(),
        });
    if (!fragment_validation.valid) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene fragment SPIR-V validation failed: " + fragment_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    if (!desc.vulkan_scene_renderer->morph_mesh_bindings.empty()) {
        const auto morph_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.size(),
            });
        if (!morph_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene morph vertex SPIR-V validation failed: " +
                              morph_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation compute_morph_shader_validation;
    if (has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_shader)) {
        compute_morph_shader_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::compute,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_shader.bytecode.size(),
            });
        if (!compute_morph_shader_validation.valid) {
            const char* diagnostic_prefix = desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty()
                                                ? "Vulkan scene compute mapping SPIR-V validation failed: "
                                                : "Vulkan scene compute morph compute SPIR-V validation failed: ";
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = std::string{diagnostic_prefix} + compute_morph_shader_validation.diagnostic +
                              "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }
    if (!desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty()) {
        const auto compute_morph_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
            });
        if (!compute_morph_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene compute morph vertex SPIR-V validation failed: " +
                              compute_morph_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }
    rhi::vulkan::VulkanSpirvShaderArtifactValidation compute_morph_skinned_shader_validation;
    if (!desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings.empty()) {
        compute_morph_skinned_shader_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::compute,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_skinned_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_skinned_shader.bytecode.size(),
            });
        if (!compute_morph_skinned_shader_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene compute morph skinned compute SPIR-V validation failed: " +
                              compute_morph_skinned_shader_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation postprocess_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation postprocess_fragment_validation;
    if (desc.vulkan_scene_renderer->enable_postprocess) {
        postprocess_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.size(),
            });
        if (!postprocess_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan postprocess vertex SPIR-V validation failed: " +
                              postprocess_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        postprocess_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.size(),
            });
        if (!postprocess_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan postprocess fragment SPIR-V validation failed: " +
                              postprocess_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation native_ui_overlay_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation native_ui_overlay_fragment_validation;
    if (native_ui_overlay_requested) {
        native_ui_overlay_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
            });
        if (!native_ui_overlay_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native UI overlay vertex SPIR-V validation failed: " +
                              native_ui_overlay_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            return result;
        }

        native_ui_overlay_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
            });
        if (!native_ui_overlay_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native UI overlay fragment SPIR-V validation failed: " +
                              native_ui_overlay_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation shadow_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation shadow_fragment_validation;
    if (desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
        shadow_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.size(),
            });
        if (!shadow_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan directional shadow vertex SPIR-V validation failed: " +
                              shadow_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        shadow_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.size(),
            });
        if (!shadow_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan directional shadow fragment SPIR-V validation failed: " +
                              shadow_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    auto runtime_device = rhi::vulkan::create_runtime_device({}, {}, {}, surface);
    if (!runtime_device.created) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "Vulkan runtime device creation failed: " + runtime_device.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::unavailable,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        return result;
    }

    const auto* mapping_compute_validation =
        scene_compute_morph_mesh_requested
            ? &compute_morph_shader_validation
            : (scene_compute_morph_skinned_requested
                   ? &compute_morph_skinned_shader_validation
                   : (compute_morph_shader_validation.valid ? &compute_morph_shader_validation : nullptr));
    const auto mapping_plan = build_vulkan_presentation_mapping_plan(
        runtime_device.device, vertex_validation, fragment_validation, surface, desc.extent, rhi::Format::bgra8_unorm,
        desc.vsync, mapping_compute_validation);
    if (!mapping_plan.supported) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene IRhiDevice mapping is unavailable: " + mapping_plan.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::vulkan::create_rhi_device(std::move(runtime_device.device), mapping_plan);
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "Vulkan device creation is unavailable; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::unavailable,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            return result;
        }

        const float directional_shadow_viewport_aspect =
            desc.extent.height > 0 ? static_cast<float>(desc.extent.width) / static_cast<float>(desc.extent.height)
                                   : (16.0F / 9.0F);

        ShadowMapPlan shadow_map_plan{};
        DirectionalShadowLightSpacePlan directional_light_space_plan{};
        ShadowReceiverPlan shadow_receiver_plan{};
        rhi::DescriptorSetLayoutHandle shadow_receiver_layout;
        runtime_scene_rhi::RuntimeSceneGpuBindingOptions gpu_binding_options;
        gpu_binding_options.morph_mesh_assets = desc.vulkan_scene_renderer->morph_mesh_assets;
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->compute_morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings);
        for (const auto& binding : desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings) {
            gpu_binding_options.compute_morph_skinned_mesh_bindings.push_back(
                runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshBinding{.mesh = binding.mesh,
                                                                              .morph_mesh = binding.morph_mesh});
        }
        if (scene_compute_morph_mesh_requested) {
            gpu_binding_options.mesh_upload.vertex_usage =
                gpu_binding_options.mesh_upload.vertex_usage | rhi::BufferUsage::storage;
        }
        if (directional_shadow_requested) {
            shadow_map_plan = make_scene_directional_shadow_plan(*desc.vulkan_scene_renderer->packet, desc.extent);
            directional_light_space_plan = build_scene_directional_shadow_light_space_plan(
                *desc.vulkan_scene_renderer->packet, shadow_map_plan,
                SceneShadowLightSpaceDesc{.viewport_aspect = directional_shadow_viewport_aspect});
            shadow_receiver_plan = make_scene_shadow_receiver_plan(shadow_map_plan, directional_light_space_plan);
            if (const auto diagnostic = directional_shadow_plan_diagnostic(
                    "Vulkan", shadow_map_plan, directional_light_space_plan, shadow_receiver_plan);
                !diagnostic.empty()) {
                return invalid_vulkan_directional_shadow_request(diagnostic);
            }
            shadow_receiver_layout = device->create_descriptor_set_layout(shadow_receiver_plan.descriptor_set_layout);
            gpu_binding_options.additional_pipeline_descriptor_set_layouts.push_back(shadow_receiver_layout);
        }

        auto gpu_upload =
            runtime_scene_rhi::execute_runtime_scene_gpu_upload(runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc{
                .device = device.get(),
                .package = desc.vulkan_scene_renderer->package,
                .packet = desc.vulkan_scene_renderer->packet,
                .binding_options = gpu_binding_options,
            });
        auto& gpu_bindings = gpu_upload.bindings;
        if (!gpu_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene GPU binding creation failed; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            for (const auto& failure : gpu_bindings.failures) {
                result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                    result.scene_gpu_status, "scene GPU binding failure asset=" + std::to_string(failure.asset.value) +
                                                 ": " + failure.diagnostic));
            }
            return result;
        }
        if (gpu_bindings.material_pipeline_layouts.empty()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic =
                    "Vulkan scene GPU binding creation did not produce a material pipeline layout; using NullRenderer "
                    "fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, Win32DesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
        auto compute_morph_bindings = build_scene_compute_morph_bindings(
            *device, gpu_bindings, desc.vulkan_scene_renderer->compute_morph_mesh_bindings,
            desc.vulkan_scene_renderer->compute_morph_shader,
            desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output, "Vulkan");
        if (!compute_morph_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_bindings.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
        auto compute_morph_skinned_dispatch = dispatch_scene_compute_morph_skinned_bindings(
            *device, gpu_bindings.compute_morph_skinned_mesh_bindings,
            desc.vulkan_scene_renderer->compute_morph_skinned_shader, "Vulkan");
        if (!compute_morph_skinned_dispatch.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_skinned_dispatch.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.vulkan_scene_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.vulkan_scene_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.vulkan_scene_renderer->vertex_shader.bytecode.data(),
        });
        rhi::ShaderHandle compute_morph_scene_vertex_shader{};
        if (scene_compute_morph_mesh_requested) {
            compute_morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->compute_morph_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
            });
        }
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.vulkan_scene_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.vulkan_scene_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.vulkan_scene_renderer->fragment_shader.bytecode.data(),
        });
        const bool scene_gpu_skinning_requested =
            has_shader_bytecode(desc.vulkan_scene_renderer->skinned_vertex_shader);
        const bool scene_gpu_morph_requested = has_shader_bytecode(desc.vulkan_scene_renderer->morph_vertex_shader) &&
                                               !desc.vulkan_scene_renderer->morph_mesh_bindings.empty();
        rhi::ShaderHandle skinned_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle skinned_scene_graphics_pipeline{};
        if (scene_gpu_skinning_requested) {
            skinned_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->skinned_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->skinned_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->skinned_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle morph_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle morph_scene_graphics_pipeline{};
        if (scene_gpu_morph_requested) {
            morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->morph_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle shifted_scene_fragment_shader_handle = fragment_shader;
        const bool shifted_shadow_receiver_fragment_requested =
            directional_shadow_requested && (scene_gpu_skinning_requested || scene_gpu_morph_requested) &&
            has_shader_bytecode(desc.vulkan_scene_renderer->skinned_scene_fragment_shader);
        if (shifted_shadow_receiver_fragment_requested) {
            shifted_scene_fragment_shader_handle = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.bytecode.data(),
            });
        }
        const auto scene_fragment_shader_handle =
            shifted_shadow_receiver_fragment_requested ? shifted_scene_fragment_shader_handle : fragment_shader;
        const bool shadow_stage_skinned =
            scene_gpu_skinning_requested && scene_packet_references_skinned_mesh(*desc.vulkan_scene_renderer->package,
                                                                                 *desc.vulkan_scene_renderer->packet);
        const auto& shadow_stage_vertex_buffers = shadow_stage_skinned
                                                      ? desc.vulkan_scene_renderer->skinned_vertex_buffers
                                                      : desc.vulkan_scene_renderer->vertex_buffers;
        const auto& shadow_stage_vertex_attributes = shadow_stage_skinned
                                                         ? desc.vulkan_scene_renderer->skinned_vertex_attributes
                                                         : desc.vulkan_scene_renderer->vertex_attributes;
        rhi::GraphicsPipelineHandle shadow_pipeline;
        if (directional_shadow_requested) {
            const auto shadow_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->shadow_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.data(),
            });
            const auto shadow_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->shadow_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.data(),
            });
            const auto shadow_pipeline_layout = device->create_pipeline_layout(
                rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
            shadow_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = shadow_pipeline_layout,
                .vertex_shader = shadow_vertex_shader,
                .fragment_shader = shadow_fragment_shader,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = rhi::Format::depth24_stencil8,
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = shadow_stage_vertex_buffers,
                .vertex_attributes = shadow_stage_vertex_attributes,
                .depth_state = rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                          .depth_write_enabled = true,
                                                          .depth_compare = rhi::CompareOp::less_equal},
            });
        }
        const bool postprocess_depth_input_requested = desc.vulkan_scene_renderer->enable_postprocess_depth_input;
        const bool enable_postprocess_depth_input =
            desc.vulkan_scene_renderer->enable_postprocess && postprocess_depth_input_requested;
        const bool environment_fog_requested = desc.vulkan_scene_renderer->enable_environment_fog;
        const bool physical_sky_vulkan_package_requested =
            desc.vulkan_scene_renderer->enable_physical_sky_package_evidence;
        const bool environment_precipitation_vulkan_renderer_execution_requested =
            desc.vulkan_scene_renderer->enable_environment_precipitation_renderer_execution;
        const bool environment_precipitation_vulkan_requested =
            desc.vulkan_scene_renderer->enable_environment_precipitation_package_evidence ||
            environment_precipitation_vulkan_renderer_execution_requested;
        const bool environment_volumetric_fog_vulkan_renderer_execution_requested =
            desc.vulkan_scene_renderer->enable_environment_volumetric_fog_renderer_execution;
        const bool environment_volumetric_cloud_vulkan_renderer_execution_requested =
            desc.vulkan_scene_renderer->enable_environment_volumetric_cloud_renderer_execution;
        const auto compute_morph_vertex_buffers = desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output
                                                      ? compute_morph_tangent_frame_vertex_buffers()
                                                      : compute_morph_position_vertex_buffers();
        const auto compute_morph_vertex_attributes =
            desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output
                ? compute_morph_tangent_frame_vertex_attributes()
                : compute_morph_position_vertex_attributes();
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = gpu_bindings.material_pipeline_layouts.front(),
            .vertex_shader = scene_compute_morph_mesh_requested ? compute_morph_scene_vertex_shader : vertex_shader,
            .fragment_shader = scene_fragment_shader_handle,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
            .topology = desc.vulkan_scene_renderer->topology,
            .vertex_buffers = scene_compute_morph_mesh_requested ? compute_morph_vertex_buffers
                                                                 : desc.vulkan_scene_renderer->vertex_buffers,
            .vertex_attributes = scene_compute_morph_mesh_requested ? compute_morph_vertex_attributes
                                                                    : desc.vulkan_scene_renderer->vertex_attributes,
            .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
        });
        if (scene_gpu_skinning_requested) {
            skinned_scene_graphics_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = skinned_scene_vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = desc.vulkan_scene_renderer->skinned_vertex_buffers,
                .vertex_attributes = desc.vulkan_scene_renderer->skinned_vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        }
        if (scene_gpu_morph_requested) {
            morph_scene_graphics_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = morph_scene_vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = desc.vulkan_scene_renderer->vertex_buffers,
                .vertex_attributes = desc.vulkan_scene_renderer->vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        }
        std::unique_ptr<IRenderer> frame_renderer;
        NativeRendererCreateResult result;
        result.succeeded = true;
        result.failure_reason = Win32DesktopPresentationFallbackReason::none;
        result.postprocess_depth_input_requested = postprocess_depth_input_requested;
        result.environment_fog_requested = environment_fog_requested;
        result.environment_fog_vulkan_package_requested = environment_fog_requested;
        result.physical_sky_vulkan_package_requested = physical_sky_vulkan_package_requested;
        result.environment_precipitation_vulkan_requested = environment_precipitation_vulkan_requested;
        result.environment_volumetric_fog_vulkan_requested =
            environment_volumetric_fog_vulkan_renderer_execution_requested;
        result.environment_volumetric_cloud_vulkan_requested =
            environment_volumetric_cloud_vulkan_renderer_execution_requested;
        result.directional_shadow_requested = directional_shadow_requested;
        result.native_ui_overlay_requested = native_ui_overlay_requested;
        result.native_ui_texture_overlay_requested = native_ui_texture_overlay_requested;
        if (physical_sky_vulkan_package_requested) {
            auto physical_sky_desc = desc.vulkan_scene_renderer->physical_sky;
            physical_sky_desc.shader_contract_evidence_ready = physical_sky_desc.shader_contract_evidence_ready &&
                                                               vertex_validation.valid && fragment_validation.valid;
            physical_sky_desc.package_evidence_ready =
                physical_sky_desc.package_evidence_ready && desc.vulkan_scene_renderer->package != nullptr;
            physical_sky_desc.execution_evidence_ready =
                physical_sky_desc.execution_evidence_ready && pipeline.value != 0;
            const auto physical_sky_plan = plan_physical_sky_policy(physical_sky_desc);
            apply_vulkan_physical_sky_package_plan(result, physical_sky_plan);
            if (!physical_sky_plan.ready()) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "Vulkan physical sky package evidence failed the physical sky policy; using NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
            const auto physical_sky_constants_buffer = create_physical_sky_constants_buffer(*device, physical_sky_desc);
            result.physical_sky_vulkan_package_constant_buffer_ready = physical_sky_constants_buffer.value != 0;
            result.physical_sky_vulkan_package_constant_buffer_bytes =
                static_cast<std::uint64_t>(physical_sky_constants_byte_size());
            if (!result.physical_sky_vulkan_package_constant_buffer_ready) {
                result.succeeded = false;
                result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                result.diagnostic =
                    "Vulkan physical sky package evidence could not create the physical sky constant buffer; using "
                    "NullRenderer fallback.";
                result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                result.scene_gpu_diagnostics.push_back(
                    make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                return result;
            }
        }
        if (desc.vulkan_scene_renderer->enable_postprocess) {
            rhi::BufferHandle environment_fog_constants_buffer;
            if (environment_fog_requested) {
                if (!enable_postprocess_depth_input || directional_shadow_requested) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan environment fog package evidence requires depth-aware non-shadow scene postprocess; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                auto environment_fog_desc = desc.vulkan_scene_renderer->environment_fog;
                environment_fog_desc.scene_depth_available = true;
                environment_fog_desc.shader_contract_evidence_ready =
                    postprocess_vertex_validation.valid && postprocess_fragment_validation.valid;
                environment_fog_desc.package_evidence_ready =
                    environment_fog_desc.package_evidence_ready &&
                    has_shader_bytecode(desc.vulkan_scene_renderer->postprocess_vertex_shader) &&
                    has_shader_bytecode(desc.vulkan_scene_renderer->postprocess_fragment_shader) &&
                    desc.vulkan_scene_renderer->package != nullptr;
                const auto environment_fog_plan = plan_environment_fog_policy(environment_fog_desc);
                result.environment_fog_vulkan_package_shader_contract_evidence_ready =
                    environment_fog_plan.shader_contract_evidence_ready;
                result.environment_fog_vulkan_package_evidence_ready = environment_fog_plan.package_evidence_ready;
                if (!environment_fog_plan.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan environment fog package evidence failed the environment fog policy; using "
                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                environment_fog_constants_buffer =
                    create_environment_fog_constants_buffer(*device, environment_fog_desc);
                result.environment_fog_vulkan_package_constant_buffer_ready =
                    environment_fog_constants_buffer.value != 0;
                result.environment_fog_vulkan_package_constant_buffer_bytes =
                    static_cast<std::uint64_t>(environment_fog_constants_byte_size());
                if (!result.environment_fog_vulkan_package_constant_buffer_ready) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan environment fog package evidence could not create the fog constant buffer; using "
                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
            }
            if (environment_precipitation_vulkan_requested) {
                if (!enable_postprocess_depth_input || directional_shadow_requested) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan precipitation renderer execution requires depth-aware non-shadow scene postprocess; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                auto precipitation_desc = desc.vulkan_scene_renderer->environment_precipitation;
                std::uint32_t precipitation_synchronization2_barriers = 0U;
                precipitation_desc.package_evidence_ready =
                    precipitation_desc.package_evidence_ready &&
                    environment_precipitation_package_texture_evidence_ready(desc.vulkan_scene_renderer->package);
                if (environment_precipitation_vulkan_renderer_execution_requested) {
                    const auto precipitation_vertex_validation =
                        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                            .stage = rhi::ShaderStage::vertex,
                            .bytecode = desc.vulkan_scene_renderer->precipitation_vertex_shader.bytecode.data(),
                            .bytecode_size = desc.vulkan_scene_renderer->precipitation_vertex_shader.bytecode.size(),
                        });
                    const auto precipitation_fragment_validation =
                        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                            .stage = rhi::ShaderStage::fragment,
                            .bytecode = desc.vulkan_scene_renderer->precipitation_fragment_shader.bytecode.data(),
                            .bytecode_size = desc.vulkan_scene_renderer->precipitation_fragment_shader.bytecode.size(),
                        });
                    precipitation_desc.shader_contract_evidence_ready =
                        precipitation_desc.shader_contract_evidence_ready && precipitation_vertex_validation.valid &&
                        precipitation_fragment_validation.valid;
                    if (!precipitation_vertex_validation.valid || !precipitation_fragment_validation.valid) {
                        result.succeeded = false;
                        result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                        result.diagnostic =
                            std::string{"Vulkan precipitation SPIR-V validation failed: "} +
                            (!precipitation_vertex_validation.valid ? precipitation_vertex_validation.diagnostic
                                                                    : precipitation_fragment_validation.diagnostic) +
                            "; using NullRenderer fallback.";
                        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                        result.scene_gpu_diagnostics.push_back(
                            make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                        result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                        result.postprocess_diagnostics.push_back(
                            make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                        return result;
                    }
                    const auto precipitation_probe = execute_precipitation_runtime_probe(
                        *device, *desc.vulkan_scene_renderer->package, precipitation_desc,
                        desc.vulkan_scene_renderer->precipitation_vertex_shader,
                        desc.vulkan_scene_renderer->precipitation_fragment_shader, "Vulkan");
                    if (!precipitation_probe.succeeded()) {
                        result.succeeded = false;
                        result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                        result.diagnostic = precipitation_probe.diagnostic + "; using NullRenderer fallback.";
                        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                        result.scene_gpu_diagnostics.push_back(
                            make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                        result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                        result.postprocess_diagnostics.push_back(
                            make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                        return result;
                    }
                    precipitation_desc.request_particle_buffer_upload = true;
                    precipitation_desc.request_backend_execution = true;
                    precipitation_desc.particle_buffer_upload_count = precipitation_probe.particle_buffer_uploads;
                    precipitation_desc.backend_invocation_count = precipitation_probe.backend_invocations;
                    precipitation_desc.renderer_draw_count = precipitation_probe.renderer_draws;
                    precipitation_desc.depth_occlusion_readback_proven = precipitation_probe.depth_occlusion_readback;
                    precipitation_synchronization2_barriers = precipitation_probe.synchronization2_barriers;
                }
                const auto precipitation_plan = plan_precipitation_policy(precipitation_desc);
                apply_vulkan_precipitation_plan(result, precipitation_desc, precipitation_plan);
                result.environment_precipitation_vulkan_synchronization2_barriers =
                    precipitation_synchronization2_barriers;
                if (!precipitation_plan.ready()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan precipitation renderer execution failed the precipitation policy; using "
                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
            }
            if (environment_volumetric_fog_vulkan_renderer_execution_requested) {
                if (!enable_postprocess_depth_input || directional_shadow_requested) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan volumetric fog renderer execution requires depth-aware non-shadow scene postprocess; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                const auto volumetric_fog_compute_validation =
                    rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                        .stage = rhi::ShaderStage::compute,
                        .bytecode = desc.vulkan_scene_renderer->volumetric_fog_compute_shader.bytecode.data(),
                        .bytecode_size = desc.vulkan_scene_renderer->volumetric_fog_compute_shader.bytecode.size(),
                    });
                if (!volumetric_fog_compute_validation.valid) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = "Vulkan volumetric fog compute SPIR-V validation failed: " +
                                        volumetric_fog_compute_validation.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }

                auto volumetric_fog_desc = desc.vulkan_scene_renderer->environment_volumetric_fog;
                volumetric_fog_desc.scene_depth_available = true;
                volumetric_fog_desc.shader_contract_evidence_ready =
                    volumetric_fog_desc.shader_contract_evidence_ready && volumetric_fog_compute_validation.valid;
                volumetric_fog_desc.package_evidence_ready =
                    volumetric_fog_desc.package_evidence_ready &&
                    has_shader_bytecode(desc.vulkan_scene_renderer->volumetric_fog_compute_shader) &&
                    desc.vulkan_scene_renderer->package != nullptr;

                const auto volumetric_fog_probe = execute_volumetric_fog_runtime_probe(
                    *device, volumetric_fog_desc, desc.vulkan_scene_renderer->volumetric_fog_compute_shader, "Vulkan");
                if (!volumetric_fog_probe.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = volumetric_fog_probe.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                volumetric_fog_desc.execution_evidence_ready = true;
                volumetric_fog_desc.request_ready_promotion = true;
                const auto volumetric_fog_plan = plan_volumetric_fog_policy(volumetric_fog_desc);
                apply_vulkan_volumetric_fog_plan(result, volumetric_fog_plan, volumetric_fog_probe);
                if (!volumetric_fog_plan.ready()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = "Vulkan volumetric fog renderer execution failed the volumetric fog policy; "
                                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
            }
            if (environment_volumetric_cloud_vulkan_renderer_execution_requested) {
                if (!enable_postprocess_depth_input || directional_shadow_requested) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan volumetric cloud renderer execution requires depth-aware non-shadow scene postprocess; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                if (desc.vulkan_scene_renderer->package == nullptr) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = "Vulkan volumetric cloud renderer execution requires a runtime package; using "
                                        "NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                const auto volumetric_cloud_vertex_validation =
                    rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                        .stage = rhi::ShaderStage::vertex,
                        .bytecode = desc.vulkan_scene_renderer->volumetric_cloud_vertex_shader.bytecode.data(),
                        .bytecode_size = desc.vulkan_scene_renderer->volumetric_cloud_vertex_shader.bytecode.size(),
                    });
                const auto volumetric_cloud_fragment_validation =
                    rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                        .stage = rhi::ShaderStage::fragment,
                        .bytecode = desc.vulkan_scene_renderer->volumetric_cloud_fragment_shader.bytecode.data(),
                        .bytecode_size = desc.vulkan_scene_renderer->volumetric_cloud_fragment_shader.bytecode.size(),
                    });
                if (!volumetric_cloud_vertex_validation.valid || !volumetric_cloud_fragment_validation.valid) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        std::string{"Vulkan volumetric cloud SPIR-V validation failed: "} +
                        (!volumetric_cloud_vertex_validation.valid ? volumetric_cloud_vertex_validation.diagnostic
                                                                   : volumetric_cloud_fragment_validation.diagnostic) +
                        "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }

                auto volumetric_cloud_desc = desc.vulkan_scene_renderer->environment_volumetric_cloud;
                volumetric_cloud_desc.shader_contract_evidence_ready =
                    volumetric_cloud_desc.shader_contract_evidence_ready && volumetric_cloud_vertex_validation.valid &&
                    volumetric_cloud_fragment_validation.valid;
                volumetric_cloud_desc.package_evidence_ready =
                    volumetric_cloud_desc.package_evidence_ready &&
                    environment_volumetric_cloud_package_evidence_ready(desc.vulkan_scene_renderer->package,
                                                                        volumetric_cloud_desc);
                const auto volumetric_cloud_probe = execute_volumetric_cloud_runtime_probe(
                    *device, *desc.vulkan_scene_renderer->package, volumetric_cloud_desc,
                    desc.vulkan_scene_renderer->volumetric_cloud_vertex_shader,
                    desc.vulkan_scene_renderer->volumetric_cloud_fragment_shader, "Vulkan",
                    vulkan_volumetric_cloud_probe_bindings());
                if (!volumetric_cloud_probe.succeeded()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = volumetric_cloud_probe.diagnostic + "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
                volumetric_cloud_desc.execution_evidence_ready = true;
                volumetric_cloud_desc.request_ready_promotion = true;
                volumetric_cloud_desc.request_volume_texture_upload = true;
                volumetric_cloud_desc.request_backend_execution = true;
                volumetric_cloud_desc.weather_map_upload_count = volumetric_cloud_probe.weather_map_uploads;
                volumetric_cloud_desc.shape_noise_upload_count = volumetric_cloud_probe.shape_noise_uploads;
                volumetric_cloud_desc.erosion_noise_upload_count = volumetric_cloud_probe.erosion_noise_uploads;
                volumetric_cloud_desc.backend_invocation_count = volumetric_cloud_probe.backend_invocations;
                volumetric_cloud_desc.raymarch_pass_count = volumetric_cloud_probe.raymarch_passes;
                volumetric_cloud_desc.readback_nonzero_proven = volumetric_cloud_probe.readback_nonzero;
                const auto volumetric_cloud_plan = plan_volumetric_cloud_policy(volumetric_cloud_desc);
                apply_vulkan_volumetric_cloud_plan(result, volumetric_cloud_plan, volumetric_cloud_probe);
                if (!volumetric_cloud_plan.ready()) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic =
                        "Vulkan volumetric cloud renderer execution failed the volumetric cloud policy; "
                        "using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
                    result.postprocess_diagnostics.push_back(
                        make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
                    return result;
                }
            }
            const auto postprocess_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->postprocess_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.data(),
            });
            const auto postprocess_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->postprocess_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.data(),
            });
            rhi::ShaderHandle native_ui_overlay_vertex_shader;
            rhi::ShaderHandle native_ui_overlay_fragment_shader;
            if (native_ui_overlay_requested) {
                native_ui_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::vertex,
                    .entry_point = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.entry_point,
                    .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
                    .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                });
                native_ui_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::fragment,
                    .entry_point = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.entry_point,
                    .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
                    .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                });
            }
            NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
            if (native_ui_texture_overlay_requested) {
                native_ui_overlay_atlas =
                    make_native_ui_overlay_atlas_binding(*device, *desc.vulkan_scene_renderer->package,
                                                         desc.vulkan_scene_renderer->native_ui_overlay_atlas_asset,
                                                         "Vulkan", result.native_ui_texture_overlay_diagnostics);
                if (native_ui_overlay_atlas.texture.value == 0) {
                    result.succeeded = false;
                    result.failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = result.native_ui_texture_overlay_diagnostics.empty()
                                            ? "Vulkan textured native UI overlay atlas binding failed; using "
                                              "NullRenderer fallback."
                                            : result.native_ui_texture_overlay_diagnostics.front().message +
                                                  "; using NullRenderer fallback.";
                    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    mark_native_ui_overlay_result(result, Win32DesktopPresentationNativeUiOverlayStatus::failed,
                                                  result.diagnostic);
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
                    return result;
                }
            }
            std::unique_ptr<IRenderer> postprocess_renderer;
            if (directional_shadow_requested) {
                std::array<std::uint8_t, shadow_receiver_constants_byte_size()> shadow_cb{};
                Mat4 camera_view = Mat4::identity();
                if (const auto* primary = desc.vulkan_scene_renderer->packet->primary_camera(); primary != nullptr) {
                    camera_view =
                        make_scene_camera_matrices(*primary, directional_shadow_viewport_aspect).view_from_world;
                }
                pack_shadow_receiver_constants(shadow_cb, directional_light_space_plan,
                                               shadow_map_plan.directional_cascade_count, camera_view);
                const bool scene_deformation_descriptor_set =
                    gpu_bindings.skinned_joint_descriptor_set_layout.value != 0 ||
                    gpu_bindings.morph_descriptor_set_layout.value != 0;
                const std::uint32_t shadow_receiver_descriptor_set_index = scene_deformation_descriptor_set ? 2u : 1u;
                auto shadow_renderer =
                    std::make_unique<RhiDirectionalShadowSmokeFrameRenderer>(RhiDirectionalShadowSmokeFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .scene_pipeline_layout = gpu_bindings.material_pipeline_layouts.front(),
                        .shadow_graphics_pipeline = shadow_pipeline,
                        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .scene_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_filter_mode = shadow_receiver_plan.filter_mode,
                        .shadow_filter_radius_texels = shadow_receiver_plan.filter_radius_texels,
                        .shadow_filter_tap_count = shadow_receiver_plan.filter_tap_count,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                        .shadow_depth_atlas_extent = Extent2D{.width = shadow_map_plan.depth_texture.extent.width,
                                                              .height = shadow_map_plan.depth_texture.extent.height},
                        .directional_shadow_cascade_count = shadow_map_plan.directional_cascade_count,
                        .shadow_receiver_constants_initial = shadow_cb,
                        .shadow_receiver_descriptor_set_index = shadow_receiver_descriptor_set_index,
                    });
                result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::ready;
                result.directional_shadow_ready = shadow_renderer->directional_shadow_ready();
                result.directional_shadow_filter_mode =
                    to_presentation_filter_mode(shadow_renderer->shadow_filter_mode());
                result.directional_shadow_filter_tap_count = shadow_renderer->shadow_filter_tap_count();
                result.directional_shadow_filter_radius_texels = shadow_renderer->shadow_filter_radius_texels();
                const auto shadow_atlas_extent = shadow_renderer->shadow_atlas_extent();
                result.directional_shadow_cascade_count = shadow_renderer->directional_shadow_cascade_count();
                result.directional_shadow_cascade_tile_width = shadow_renderer->shadow_cascade_tile_width();
                result.directional_shadow_atlas_width = shadow_atlas_extent.width;
                result.directional_shadow_atlas_height = shadow_atlas_extent.height;
                result.directional_shadow_light_space_cascades =
                    static_cast<std::uint32_t>(directional_light_space_plan.clip_from_world_cascades.size());
                result.directional_shadow_cascade_splits =
                    static_cast<std::uint32_t>(directional_light_space_plan.cascade_split_distances.size());
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = shadow_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready = shadow_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = shadow_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(shadow_renderer);
            } else {
                auto color_postprocess_renderer =
                    std::make_unique<RhiPostprocessFrameRenderer>(RhiPostprocessFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .enable_depth_input = enable_postprocess_depth_input,
                        .depth_format = rhi::Format::depth24_stencil8,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                    });
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = color_postprocess_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status =
                        Win32DesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready =
                        color_postprocess_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = color_postprocess_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(color_postprocess_renderer);
            }
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::ready;
            result.postprocess_depth_input_ready = enable_postprocess_depth_input;
            frame_renderer = std::move(postprocess_renderer);
        } else {
            frame_renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
                .device = device.get(),
                .extent = desc.extent,
                .color_texture = rhi::TextureHandle{},
                .swapchain = swapchain,
                .graphics_pipeline = pipeline,
                .wait_for_completion = true,
                .skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                .morph_graphics_pipeline = morph_scene_graphics_pipeline,
            });
        }
        auto scene_renderer = std::make_unique<SceneGpuBindingInjectingRenderer>(
            std::move(frame_renderer), std::move(gpu_bindings), desc.vulkan_scene_renderer->morph_mesh_bindings,
            std::move(compute_morph_bindings.bindings), compute_morph_bindings.queue_waits,
            compute_morph_skinned_dispatch.dispatches, compute_morph_skinned_dispatch.queue_waits);
        auto* scene_renderer_ptr = scene_renderer.get();

        result.device = std::move(device);
        result.renderer = std::move(scene_renderer);
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::ready;
        result.scene_gpu_renderer = scene_renderer_ptr;
        return result;
    } catch (const std::exception& error) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = std::string{"Vulkan scene renderer creation failed: "} + error.what() +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess) {
            result.postprocess_status = Win32DesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            result.postprocess_depth_input_requested = desc.vulkan_scene_renderer->enable_postprocess_depth_input;
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
            result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::failed;
            result.directional_shadow_requested = true;
            result.directional_shadow_diagnostics.push_back(
                make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) {
            result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::failed;
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_diagnostics.push_back(
                make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
            result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::failed;
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_diagnostics.push_back(
                make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        Win32DesktopPresentationFallbackReason::native_backend_unavailable,
        "Vulkan runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    result.scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
        result.directional_shadow_status = Win32DesktopPresentationDirectionalShadowStatus::unavailable;
        result.directional_shadow_requested = true;
        result.directional_shadow_diagnostics.push_back(
            make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    }
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) {
        result.native_ui_overlay_status = Win32DesktopPresentationNativeUiOverlayStatus::unavailable;
        result.native_ui_overlay_requested = true;
        result.native_ui_overlay_diagnostics.push_back(
            make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    }
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
        result.native_ui_texture_overlay_status = Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable;
        result.native_ui_texture_overlay_requested = true;
        result.native_ui_texture_overlay_diagnostics.push_back(
            make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    }
    return result;
#endif
}

[[nodiscard]] bool quality_gate_requested(const Win32DesktopPresentationQualityGateDesc& desc) noexcept {
    return desc.require_scene_gpu_bindings || desc.require_postprocess || desc.require_postprocess_depth_input ||
           desc.require_directional_shadow || desc.require_directional_shadow_filtering;
}

[[nodiscard]] bool postprocess_policy_requested(const Win32DesktopPresentationReport& report) noexcept {
    return report.postprocess_status != Win32DesktopPresentationPostprocessStatus::not_requested ||
           report.postprocess_depth_input_requested;
}

[[nodiscard]] bool scene_scale_policy_requested(const Win32DesktopPresentationSceneScalePolicyDesc& desc) noexcept {
    return desc.require_scene_gpu_bindings || desc.require_backend_instancing_evidence ||
           desc.require_performance_measurement;
}

[[nodiscard]] bool gpu_memory_policy_requested(const Win32DesktopPresentationGpuMemoryPolicyDesc& desc) noexcept {
    return desc.require_scene_gpu_bindings || desc.require_backend_memory_evidence ||
           desc.require_os_video_memory_budget || desc.require_declared_budget_evidence ||
           desc.require_residency_pressure_evidence || desc.require_package_counter_evidence ||
           desc.declared_local_budget_bytes > 0;
}

[[nodiscard]] rhi::BackendKind postprocess_policy_backend(Win32DesktopPresentationBackend backend) noexcept {
    switch (backend) {
    case Win32DesktopPresentationBackend::null_renderer:
        return rhi::BackendKind::null;
    case Win32DesktopPresentationBackend::d3d12:
        return rhi::BackendKind::d3d12;
    case Win32DesktopPresentationBackend::vulkan:
        return rhi::BackendKind::vulkan;
    }
    return rhi::BackendKind::null;
}

[[nodiscard]] bool
debug_profiling_policy_requested(const Win32DesktopPresentationDebugProfilingPolicyDesc& desc) noexcept {
    return desc.require_scene_gpu_bindings || desc.require_backend_profiling_evidence ||
           desc.require_cpu_profile_zone_evidence || desc.require_trace_capture_handoff_evidence ||
           desc.require_package_counter_evidence;
}

[[nodiscard]] bool debug_profiling_policy_backend_evidence_ready(const Win32DesktopPresentationReport& report,
                                                                 bool require_backend_profiling_evidence) noexcept {
    if (!require_backend_profiling_evidence) {
        return false;
    }

    return mirakana::debug_profiling_policy_backend_evidence_ready(mirakana::DebugProfilingBackendEvidenceDesc{
        .backend = postprocess_policy_backend(report.selected_backend),
        .gpu_timestamp_ticks_per_second = report.rhi_gpu_timestamp_ticks_per_second,
        .gpu_debug_scopes_begun = report.rhi_gpu_debug_scopes_begun,
        .gpu_debug_scopes_ended = report.rhi_gpu_debug_scopes_ended,
        .gpu_debug_markers_inserted = report.rhi_gpu_debug_markers_inserted,
        .framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed,
        .framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded,
    });
}

[[nodiscard]] std::uint64_t gpu_memory_policy_upload_bytes(const Win32DesktopPresentationReport& report) noexcept {
    return report.rhi_bytes_written + report.scene_gpu_stats.uploaded_texture_bytes +
           report.scene_gpu_stats.uploaded_mesh_bytes + report.scene_gpu_stats.uploaded_morph_bytes +
           report.scene_gpu_stats.uploaded_material_factor_bytes;
}

[[nodiscard]] bool gpu_memory_policy_transient_pressure_ready(const Win32DesktopPresentationReport& report) noexcept {
    return report.rhi_transient_heap_allocations > 0 || report.rhi_transient_placed_allocations > 0 ||
           report.rhi_transient_placed_resources_alive > 0 ||
           report.renderer_stats.framegraph_barrier_steps_executed > 0;
}

[[nodiscard]] bool gpu_memory_policy_backend_memory_evidence_ready(const Win32DesktopPresentationReport& report,
                                                                   bool require_backend_memory_evidence) noexcept {
    if (!require_backend_memory_evidence) {
        return false;
    }

    const auto& memory = report.rhi_memory_diagnostics;
    return mirakana::gpu_memory_policy_backend_evidence_ready(mirakana::GpuMemoryBackendEvidenceDesc{
        .backend = postprocess_policy_backend(report.selected_backend),
        .committed_byte_estimate_available = memory.committed_resources_byte_estimate_available,
        .committed_resources_byte_estimate = memory.committed_resources_byte_estimate,
        .upload_bytes_written = gpu_memory_policy_upload_bytes(report),
        .transient_heap_allocations = report.rhi_transient_heap_allocations,
        .transient_placed_allocations = report.rhi_transient_placed_allocations,
        .transient_placed_resources_alive = report.rhi_transient_placed_resources_alive,
        .framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed,
        .os_video_memory_budget_available = memory.os_video_memory_budget_available,
    });
}

[[nodiscard]] std::uint32_t scene_scale_policy_count(std::uint64_t value) noexcept {
    constexpr auto max_value = static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(std::min(value, max_value));
}

[[nodiscard]] std::uint32_t
quality_gate_expected_framegraph_passes(const Win32DesktopPresentationQualityGateDesc& desc) noexcept {
    if (desc.require_directional_shadow) {
        return 3;
    }
    if (desc.require_postprocess || desc.require_postprocess_depth_input) {
        return 2;
    }
    return 0;
}

[[nodiscard]] std::uint64_t
quality_gate_expected_framegraph_render_passes(const Win32DesktopPresentationQualityGateDesc& desc) noexcept {
    const auto expected_passes = quality_gate_expected_framegraph_passes(desc);
    if (expected_passes == 0) {
        return 0;
    }
    if (desc.expected_frames == 0) {
        return expected_passes;
    }
    return static_cast<std::uint64_t>(expected_passes) * desc.expected_frames;
}

[[nodiscard]] std::uint64_t
quality_gate_expected_framegraph_barrier_steps(const Win32DesktopPresentationQualityGateDesc& desc) noexcept {
    const auto expected_frames = static_cast<std::uint64_t>(desc.expected_frames);
    if (desc.require_directional_shadow) {
        if (expected_frames == 0) {
            return 9;
        }
        return 9 + ((expected_frames - 1) * 6);
    }
    if (desc.require_postprocess_depth_input) {
        return expected_frames == 0 ? 5 : 1 + (expected_frames * 4);
    }
    if (desc.require_postprocess) {
        return expected_frames == 0 ? 2 : expected_frames * 2;
    }
    return 0;
}

} // namespace

struct Win32DesktopPresentation::Impl {
    Win32DesktopPresentationBackend requested_backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationBackend backend{Win32DesktopPresentationBackend::null_renderer};
    Win32DesktopPresentationFallbackReason fallback_reason{Win32DesktopPresentationFallbackReason::none};
    bool allow_null_fallback{true};
    Win32D3d12SwapChainPlan swapchain_plan;
    bool d3d12_tearing_requested{false};
    bool d3d12_tearing_supported{false};
    bool d3d12_tearing_active{false};
    Win32DesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        Win32DesktopPresentationSceneGpuBindingStatus::not_requested};
    Win32DesktopPresentationPostprocessStatus postprocess_status{
        Win32DesktopPresentationPostprocessStatus::not_requested};
    bool postprocess_depth_input_requested{false};
    bool postprocess_depth_input_ready{false};
    bool environment_fog_requested{false};
    bool environment_fog_constant_buffer_ready{false};
    std::uint64_t environment_fog_constant_buffer_bytes{0};
    bool environment_fog_vulkan_package_requested{false};
    bool environment_fog_vulkan_package_shader_contract_evidence_ready{false};
    bool environment_fog_vulkan_package_evidence_ready{false};
    bool environment_fog_vulkan_package_constant_buffer_ready{false};
    std::uint64_t environment_fog_vulkan_package_constant_buffer_bytes{0};
    bool physical_sky_vulkan_package_requested{false};
    bool physical_sky_vulkan_package_shader_contract_evidence_ready{false};
    bool physical_sky_vulkan_package_evidence_ready{false};
    bool physical_sky_vulkan_package_execution_evidence_ready{false};
    bool physical_sky_vulkan_package_constant_buffer_ready{false};
    std::uint64_t physical_sky_vulkan_package_constant_buffer_bytes{0};
    bool physical_sky_vulkan_package_allocates_lut_textures{false};
    bool physical_sky_vulkan_package_invokes_backend{false};
    bool physical_sky_vulkan_package_exposes_native_handles{false};
    std::uint32_t physical_sky_vulkan_package_constant_layout_rows{0};
    std::uint32_t physical_sky_vulkan_package_lut_intent_rows{0};
    std::uint32_t physical_sky_vulkan_package_policy_diagnostics_count{0};
    bool physical_sky_requested{false};
    bool physical_sky_shader_contract_evidence_ready{false};
    bool physical_sky_package_evidence_ready{false};
    bool physical_sky_execution_evidence_ready{false};
    bool physical_sky_constant_buffer_ready{false};
    std::uint64_t physical_sky_constant_buffer_bytes{0};
    bool physical_sky_allocates_lut_textures{false};
    bool physical_sky_invokes_backend{false};
    bool physical_sky_exposes_native_handles{false};
    std::uint32_t physical_sky_constant_layout_rows{0};
    std::uint32_t physical_sky_lut_intent_rows{0};
    std::uint32_t physical_sky_policy_diagnostics_count{0};
    bool cloud_layer_requested{false};
    bool cloud_layer_shader_contract_evidence_ready{false};
    bool cloud_layer_package_evidence_ready{false};
    bool cloud_layer_execution_evidence_ready{false};
    bool cloud_layer_uploads_textures{false};
    bool cloud_layer_invokes_backend{false};
    bool cloud_layer_exposes_native_handles{false};
    bool cloud_layer_uses_volumetric_clouds{false};
    bool cloud_layer_uses_latlong_projection{false};
    bool cloud_layer_uses_flow_map{false};
    std::uint32_t cloud_layer_texture_rows{0};
    std::uint32_t cloud_layer_visual_rows{0};
    std::uint32_t cloud_layer_ibl_rows{0};
    std::uint32_t cloud_layer_shader_contract_rows{0};
    std::uint32_t cloud_layer_quality_rows{0};
    std::uint32_t cloud_layer_policy_diagnostics_count{0};
    std::uint64_t cloud_layer_renderer_draws{0};
    bool environment_precipitation_requested{false};
    EnvironmentWeatherKind environment_precipitation_weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind environment_precipitation_kind{EnvironmentPrecipitationKind::none};
    bool environment_precipitation_shader_contract_evidence_ready{false};
    bool environment_precipitation_package_evidence_ready{false};
    bool environment_precipitation_execution_evidence_ready{false};
    bool environment_precipitation_uploads_particle_buffers{false};
    bool environment_precipitation_invokes_backend{false};
    bool environment_precipitation_exposes_native_handles{false};
    bool environment_precipitation_mutates_materials{false};
    bool environment_precipitation_plays_audio{false};
    std::uint64_t environment_precipitation_renderer_draws{0};
    bool environment_precipitation_depth_occlusion_readback{false};
    bool environment_precipitation_uses_camera_near_particles{false};
    bool environment_precipitation_uses_scene_depth_occlusion{false};
    std::uint32_t environment_precipitation_weather_rows{0};
    std::uint32_t environment_precipitation_particle_rows{0};
    std::uint32_t environment_precipitation_occlusion_rows{0};
    std::uint32_t environment_precipitation_wetness_rows{0};
    std::uint32_t environment_precipitation_audio_handoff_rows{0};
    std::uint32_t environment_precipitation_shader_rows{0};
    std::uint32_t environment_precipitation_quality_rows{0};
    std::uint32_t environment_precipitation_policy_diagnostics_count{0};
    bool environment_precipitation_vulkan_requested{false};
    EnvironmentWeatherKind environment_precipitation_vulkan_weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind environment_precipitation_vulkan_kind{EnvironmentPrecipitationKind::none};
    bool environment_precipitation_vulkan_shader_contract_evidence_ready{false};
    bool environment_precipitation_vulkan_package_evidence_ready{false};
    bool environment_precipitation_vulkan_execution_evidence_ready{false};
    bool environment_precipitation_vulkan_uses_camera_near_particles{false};
    bool environment_precipitation_vulkan_uses_scene_depth_occlusion{false};
    std::uint32_t environment_precipitation_vulkan_weather_rows{0};
    std::uint32_t environment_precipitation_vulkan_particle_rows{0};
    std::uint32_t environment_precipitation_vulkan_occlusion_rows{0};
    std::uint32_t environment_precipitation_vulkan_wetness_rows{0};
    std::uint32_t environment_precipitation_vulkan_audio_handoff_rows{0};
    std::uint32_t environment_precipitation_vulkan_shader_rows{0};
    std::uint32_t environment_precipitation_vulkan_quality_rows{0};
    std::uint64_t environment_precipitation_vulkan_particle_buffer_uploads{0};
    std::uint64_t environment_precipitation_vulkan_backend_invocations{0};
    std::uint64_t environment_precipitation_vulkan_renderer_draws{0};
    bool environment_precipitation_vulkan_depth_occlusion_readback{false};
    std::uint32_t environment_precipitation_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_precipitation_vulkan_synchronization2_barriers{0};
    bool environment_precipitation_vulkan_exposes_native_handles{false};
    bool environment_precipitation_vulkan_mutates_materials{false};
    bool environment_precipitation_vulkan_plays_audio{false};
    std::uint32_t environment_precipitation_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_fog_requested{false};
    bool environment_volumetric_fog_shader_contract_evidence_ready{false};
    bool environment_volumetric_fog_package_evidence_ready{false};
    bool environment_volumetric_fog_execution_evidence_ready{false};
    bool environment_volumetric_fog_froxel_output_ready{false};
    bool environment_volumetric_fog_scene_depth_ready{false};
    std::uint64_t environment_volumetric_fog_compute_dispatches{0};
    bool environment_volumetric_fog_exposes_native_handles{false};
    std::uint32_t environment_volumetric_fog_policy_diagnostics_count{0};
    bool environment_volumetric_fog_vulkan_requested{false};
    bool environment_volumetric_fog_vulkan_shader_contract_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_package_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_execution_evidence_ready{false};
    bool environment_volumetric_fog_vulkan_froxel_output_ready{false};
    bool environment_volumetric_fog_vulkan_scene_depth_ready{false};
    std::uint64_t environment_volumetric_fog_vulkan_compute_dispatches{0};
    std::uint32_t environment_volumetric_fog_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_volumetric_fog_vulkan_synchronization2_barriers{0};
    bool environment_volumetric_fog_vulkan_froxel_readback_nonzero{false};
    bool environment_volumetric_fog_vulkan_exposes_native_handles{false};
    std::uint32_t environment_volumetric_fog_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_cloud_vulkan_requested{false};
    bool environment_volumetric_cloud_vulkan_shader_contract_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_package_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_execution_evidence_ready{false};
    bool environment_volumetric_cloud_vulkan_weather_map_ready{false};
    bool environment_volumetric_cloud_vulkan_shape_noise_ready{false};
    bool environment_volumetric_cloud_vulkan_erosion_noise_ready{false};
    bool environment_volumetric_cloud_vulkan_uploads_volume_textures{false};
    bool environment_volumetric_cloud_vulkan_invokes_backend{false};
    std::uint64_t environment_volumetric_cloud_vulkan_renderer_draws{0};
    std::uint64_t environment_volumetric_cloud_vulkan_raymarch_passes{0};
    std::uint32_t environment_volumetric_cloud_vulkan_descriptor_set_bindings{0};
    std::uint32_t environment_volumetric_cloud_vulkan_synchronization2_barriers{0};
    bool environment_volumetric_cloud_vulkan_readback_nonzero{false};
    bool environment_volumetric_cloud_vulkan_exposes_native_handles{false};
    bool environment_volumetric_cloud_vulkan_plays_audio{false};
    bool environment_volumetric_cloud_vulkan_executes_precipitation{false};
    std::uint32_t environment_volumetric_cloud_vulkan_map_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_layer_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_lighting_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_raymarch_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_temporal_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_shadow_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_storm_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_shader_contract_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_quality_rows{0};
    std::uint32_t environment_volumetric_cloud_vulkan_policy_diagnostics_count{0};
    bool environment_volumetric_cloud_requested{false};
    bool environment_volumetric_cloud_shader_contract_evidence_ready{false};
    bool environment_volumetric_cloud_package_evidence_ready{false};
    bool environment_volumetric_cloud_execution_evidence_ready{false};
    bool environment_volumetric_cloud_weather_map_ready{false};
    bool environment_volumetric_cloud_shape_noise_ready{false};
    bool environment_volumetric_cloud_erosion_noise_ready{false};
    bool environment_volumetric_cloud_uploads_volume_textures{false};
    bool environment_volumetric_cloud_invokes_backend{false};
    std::uint64_t environment_volumetric_cloud_renderer_draws{0};
    std::uint64_t environment_volumetric_cloud_raymarch_passes{0};
    bool environment_volumetric_cloud_readback_nonzero{false};
    bool environment_volumetric_cloud_exposes_native_handles{false};
    bool environment_volumetric_cloud_plays_audio{false};
    bool environment_volumetric_cloud_executes_precipitation{false};
    std::uint32_t environment_volumetric_cloud_map_rows{0};
    std::uint32_t environment_volumetric_cloud_layer_rows{0};
    std::uint32_t environment_volumetric_cloud_lighting_rows{0};
    std::uint32_t environment_volumetric_cloud_raymarch_rows{0};
    std::uint32_t environment_volumetric_cloud_temporal_rows{0};
    std::uint32_t environment_volumetric_cloud_shadow_rows{0};
    std::uint32_t environment_volumetric_cloud_storm_rows{0};
    std::uint32_t environment_volumetric_cloud_shader_contract_rows{0};
    std::uint32_t environment_volumetric_cloud_quality_rows{0};
    std::uint32_t environment_volumetric_cloud_policy_diagnostics_count{0};
    Win32DesktopPresentationDirectionalShadowStatus directional_shadow_status{
        Win32DesktopPresentationDirectionalShadowStatus::not_requested};
    bool directional_shadow_requested{false};
    bool directional_shadow_ready{false};
    Win32DesktopPresentationDirectionalShadowFilterMode directional_shadow_filter_mode{
        Win32DesktopPresentationDirectionalShadowFilterMode::none};
    std::uint32_t directional_shadow_filter_tap_count{0};
    float directional_shadow_filter_radius_texels{0.0F};
    std::uint32_t directional_shadow_cascade_count{0};
    std::uint32_t directional_shadow_cascade_tile_width{0};
    std::uint32_t directional_shadow_atlas_width{0};
    std::uint32_t directional_shadow_atlas_height{0};
    std::uint32_t directional_shadow_light_space_cascades{0};
    std::uint32_t directional_shadow_cascade_splits{0};
    Win32DesktopPresentationNativeUiOverlayStatus native_ui_overlay_status{
        Win32DesktopPresentationNativeUiOverlayStatus::not_requested};
    bool native_ui_overlay_requested{false};
    bool native_ui_overlay_ready{false};
    Win32DesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        Win32DesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::size_t framegraph_passes{0};
    std::vector<Win32DesktopPresentationBackendReport> backend_reports;
    std::vector<Win32DesktopPresentationDiagnostic> diagnostics;
    std::vector<Win32DesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    std::vector<Win32DesktopPresentationPostprocessDiagnostic> postprocess_diagnostics;
    std::vector<Win32DesktopPresentationDirectionalShadowDiagnostic> directional_shadow_diagnostics;
    std::vector<Win32DesktopPresentationNativeUiOverlayDiagnostic> native_ui_overlay_diagnostics;
    std::vector<Win32DesktopPresentationNativeUiTextureOverlayDiagnostic> native_ui_texture_overlay_diagnostics;
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};

    void apply_environment_fog_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_fog_requested = environment_fog_requested || renderer_result.environment_fog_requested;
        environment_fog_constant_buffer_ready = renderer_result.environment_fog_constant_buffer_ready;
        environment_fog_constant_buffer_bytes = renderer_result.environment_fog_constant_buffer_bytes;
        environment_fog_vulkan_package_requested =
            environment_fog_vulkan_package_requested || renderer_result.environment_fog_vulkan_package_requested;
        environment_fog_vulkan_package_shader_contract_evidence_ready =
            renderer_result.environment_fog_vulkan_package_shader_contract_evidence_ready;
        environment_fog_vulkan_package_evidence_ready = renderer_result.environment_fog_vulkan_package_evidence_ready;
        environment_fog_vulkan_package_constant_buffer_ready =
            renderer_result.environment_fog_vulkan_package_constant_buffer_ready;
        environment_fog_vulkan_package_constant_buffer_bytes =
            renderer_result.environment_fog_vulkan_package_constant_buffer_bytes;
    }

    void apply_physical_sky_result(const NativeRendererCreateResult& renderer_result) noexcept {
        physical_sky_vulkan_package_requested =
            physical_sky_vulkan_package_requested || renderer_result.physical_sky_vulkan_package_requested;
        physical_sky_vulkan_package_shader_contract_evidence_ready =
            renderer_result.physical_sky_vulkan_package_shader_contract_evidence_ready;
        physical_sky_vulkan_package_evidence_ready = renderer_result.physical_sky_vulkan_package_evidence_ready;
        physical_sky_vulkan_package_execution_evidence_ready =
            renderer_result.physical_sky_vulkan_package_execution_evidence_ready;
        physical_sky_vulkan_package_constant_buffer_ready =
            renderer_result.physical_sky_vulkan_package_constant_buffer_ready;
        physical_sky_vulkan_package_constant_buffer_bytes =
            renderer_result.physical_sky_vulkan_package_constant_buffer_bytes;
        physical_sky_vulkan_package_allocates_lut_textures =
            renderer_result.physical_sky_vulkan_package_allocates_lut_textures;
        physical_sky_vulkan_package_invokes_backend = renderer_result.physical_sky_vulkan_package_invokes_backend;
        physical_sky_vulkan_package_exposes_native_handles =
            renderer_result.physical_sky_vulkan_package_exposes_native_handles;
        physical_sky_vulkan_package_constant_layout_rows =
            renderer_result.physical_sky_vulkan_package_constant_layout_rows;
        physical_sky_vulkan_package_lut_intent_rows = renderer_result.physical_sky_vulkan_package_lut_intent_rows;
        physical_sky_vulkan_package_policy_diagnostics_count =
            renderer_result.physical_sky_vulkan_package_policy_diagnostics_count;
        physical_sky_requested = physical_sky_requested || renderer_result.physical_sky_requested;
        physical_sky_shader_contract_evidence_ready = renderer_result.physical_sky_shader_contract_evidence_ready;
        physical_sky_package_evidence_ready = renderer_result.physical_sky_package_evidence_ready;
        physical_sky_execution_evidence_ready = renderer_result.physical_sky_execution_evidence_ready;
        physical_sky_constant_buffer_ready = renderer_result.physical_sky_constant_buffer_ready;
        physical_sky_constant_buffer_bytes = renderer_result.physical_sky_constant_buffer_bytes;
        physical_sky_allocates_lut_textures = renderer_result.physical_sky_allocates_lut_textures;
        physical_sky_invokes_backend = renderer_result.physical_sky_invokes_backend;
        physical_sky_exposes_native_handles = renderer_result.physical_sky_exposes_native_handles;
        physical_sky_constant_layout_rows = renderer_result.physical_sky_constant_layout_rows;
        physical_sky_lut_intent_rows = renderer_result.physical_sky_lut_intent_rows;
        physical_sky_policy_diagnostics_count = renderer_result.physical_sky_policy_diagnostics_count;
    }

    void apply_cloud_layer_result(const NativeRendererCreateResult& renderer_result) noexcept {
        cloud_layer_requested = cloud_layer_requested || renderer_result.cloud_layer_requested;
        cloud_layer_shader_contract_evidence_ready = renderer_result.cloud_layer_shader_contract_evidence_ready;
        cloud_layer_package_evidence_ready = renderer_result.cloud_layer_package_evidence_ready;
        cloud_layer_execution_evidence_ready = renderer_result.cloud_layer_execution_evidence_ready;
        cloud_layer_uploads_textures = renderer_result.cloud_layer_uploads_textures;
        cloud_layer_invokes_backend = renderer_result.cloud_layer_invokes_backend;
        cloud_layer_exposes_native_handles = renderer_result.cloud_layer_exposes_native_handles;
        cloud_layer_uses_volumetric_clouds = renderer_result.cloud_layer_uses_volumetric_clouds;
        cloud_layer_uses_latlong_projection = renderer_result.cloud_layer_uses_latlong_projection;
        cloud_layer_uses_flow_map = renderer_result.cloud_layer_uses_flow_map;
        cloud_layer_texture_rows = renderer_result.cloud_layer_texture_rows;
        cloud_layer_visual_rows = renderer_result.cloud_layer_visual_rows;
        cloud_layer_ibl_rows = renderer_result.cloud_layer_ibl_rows;
        cloud_layer_shader_contract_rows = renderer_result.cloud_layer_shader_contract_rows;
        cloud_layer_quality_rows = renderer_result.cloud_layer_quality_rows;
        cloud_layer_policy_diagnostics_count = renderer_result.cloud_layer_policy_diagnostics_count;
        cloud_layer_renderer_draws = renderer_result.cloud_layer_renderer_draws;
    }

    void apply_precipitation_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_precipitation_requested =
            environment_precipitation_requested || renderer_result.environment_precipitation_requested;
        environment_precipitation_weather = renderer_result.environment_precipitation_weather;
        environment_precipitation_kind = renderer_result.environment_precipitation_kind;
        environment_precipitation_shader_contract_evidence_ready =
            renderer_result.environment_precipitation_shader_contract_evidence_ready;
        environment_precipitation_package_evidence_ready =
            renderer_result.environment_precipitation_package_evidence_ready;
        environment_precipitation_execution_evidence_ready =
            renderer_result.environment_precipitation_execution_evidence_ready;
        environment_precipitation_uploads_particle_buffers =
            renderer_result.environment_precipitation_uploads_particle_buffers;
        environment_precipitation_invokes_backend = renderer_result.environment_precipitation_invokes_backend;
        environment_precipitation_exposes_native_handles =
            renderer_result.environment_precipitation_exposes_native_handles;
        environment_precipitation_mutates_materials = renderer_result.environment_precipitation_mutates_materials;
        environment_precipitation_plays_audio = renderer_result.environment_precipitation_plays_audio;
        environment_precipitation_renderer_draws = renderer_result.environment_precipitation_renderer_draws;
        environment_precipitation_depth_occlusion_readback =
            renderer_result.environment_precipitation_depth_occlusion_readback;
        environment_precipitation_uses_camera_near_particles =
            renderer_result.environment_precipitation_uses_camera_near_particles;
        environment_precipitation_uses_scene_depth_occlusion =
            renderer_result.environment_precipitation_uses_scene_depth_occlusion;
        environment_precipitation_weather_rows = renderer_result.environment_precipitation_weather_rows;
        environment_precipitation_particle_rows = renderer_result.environment_precipitation_particle_rows;
        environment_precipitation_occlusion_rows = renderer_result.environment_precipitation_occlusion_rows;
        environment_precipitation_wetness_rows = renderer_result.environment_precipitation_wetness_rows;
        environment_precipitation_audio_handoff_rows = renderer_result.environment_precipitation_audio_handoff_rows;
        environment_precipitation_shader_rows = renderer_result.environment_precipitation_shader_rows;
        environment_precipitation_quality_rows = renderer_result.environment_precipitation_quality_rows;
        environment_precipitation_policy_diagnostics_count =
            renderer_result.environment_precipitation_policy_diagnostics_count;
    }

    void apply_vulkan_precipitation_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_precipitation_vulkan_requested =
            environment_precipitation_vulkan_requested || renderer_result.environment_precipitation_vulkan_requested;
        environment_precipitation_vulkan_weather = renderer_result.environment_precipitation_vulkan_weather;
        environment_precipitation_vulkan_kind = renderer_result.environment_precipitation_vulkan_kind;
        environment_precipitation_vulkan_shader_contract_evidence_ready =
            renderer_result.environment_precipitation_vulkan_shader_contract_evidence_ready;
        environment_precipitation_vulkan_package_evidence_ready =
            renderer_result.environment_precipitation_vulkan_package_evidence_ready;
        environment_precipitation_vulkan_execution_evidence_ready =
            renderer_result.environment_precipitation_vulkan_execution_evidence_ready;
        environment_precipitation_vulkan_uses_camera_near_particles =
            renderer_result.environment_precipitation_vulkan_uses_camera_near_particles;
        environment_precipitation_vulkan_uses_scene_depth_occlusion =
            renderer_result.environment_precipitation_vulkan_uses_scene_depth_occlusion;
        environment_precipitation_vulkan_weather_rows = renderer_result.environment_precipitation_vulkan_weather_rows;
        environment_precipitation_vulkan_particle_rows = renderer_result.environment_precipitation_vulkan_particle_rows;
        environment_precipitation_vulkan_occlusion_rows =
            renderer_result.environment_precipitation_vulkan_occlusion_rows;
        environment_precipitation_vulkan_wetness_rows = renderer_result.environment_precipitation_vulkan_wetness_rows;
        environment_precipitation_vulkan_audio_handoff_rows =
            renderer_result.environment_precipitation_vulkan_audio_handoff_rows;
        environment_precipitation_vulkan_shader_rows = renderer_result.environment_precipitation_vulkan_shader_rows;
        environment_precipitation_vulkan_quality_rows = renderer_result.environment_precipitation_vulkan_quality_rows;
        environment_precipitation_vulkan_particle_buffer_uploads =
            renderer_result.environment_precipitation_vulkan_particle_buffer_uploads;
        environment_precipitation_vulkan_backend_invocations =
            renderer_result.environment_precipitation_vulkan_backend_invocations;
        environment_precipitation_vulkan_renderer_draws =
            renderer_result.environment_precipitation_vulkan_renderer_draws;
        environment_precipitation_vulkan_depth_occlusion_readback =
            renderer_result.environment_precipitation_vulkan_depth_occlusion_readback;
        environment_precipitation_vulkan_descriptor_set_bindings =
            renderer_result.environment_precipitation_vulkan_descriptor_set_bindings;
        environment_precipitation_vulkan_synchronization2_barriers =
            renderer_result.environment_precipitation_vulkan_synchronization2_barriers;
        environment_precipitation_vulkan_exposes_native_handles =
            renderer_result.environment_precipitation_vulkan_exposes_native_handles;
        environment_precipitation_vulkan_mutates_materials =
            renderer_result.environment_precipitation_vulkan_mutates_materials;
        environment_precipitation_vulkan_plays_audio = renderer_result.environment_precipitation_vulkan_plays_audio;
        environment_precipitation_vulkan_policy_diagnostics_count =
            renderer_result.environment_precipitation_vulkan_policy_diagnostics_count;
    }

    void apply_volumetric_fog_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_volumetric_fog_requested =
            environment_volumetric_fog_requested || renderer_result.environment_volumetric_fog_requested;
        environment_volumetric_fog_shader_contract_evidence_ready =
            renderer_result.environment_volumetric_fog_shader_contract_evidence_ready;
        environment_volumetric_fog_package_evidence_ready =
            renderer_result.environment_volumetric_fog_package_evidence_ready;
        environment_volumetric_fog_execution_evidence_ready =
            renderer_result.environment_volumetric_fog_execution_evidence_ready;
        environment_volumetric_fog_froxel_output_ready = renderer_result.environment_volumetric_fog_froxel_output_ready;
        environment_volumetric_fog_scene_depth_ready = renderer_result.environment_volumetric_fog_scene_depth_ready;
        environment_volumetric_fog_compute_dispatches = renderer_result.environment_volumetric_fog_compute_dispatches;
        environment_volumetric_fog_exposes_native_handles =
            renderer_result.environment_volumetric_fog_exposes_native_handles;
        environment_volumetric_fog_policy_diagnostics_count =
            renderer_result.environment_volumetric_fog_policy_diagnostics_count;
    }

    void apply_vulkan_volumetric_fog_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_volumetric_fog_vulkan_requested =
            environment_volumetric_fog_vulkan_requested || renderer_result.environment_volumetric_fog_vulkan_requested;
        environment_volumetric_fog_vulkan_shader_contract_evidence_ready =
            renderer_result.environment_volumetric_fog_vulkan_shader_contract_evidence_ready;
        environment_volumetric_fog_vulkan_package_evidence_ready =
            renderer_result.environment_volumetric_fog_vulkan_package_evidence_ready;
        environment_volumetric_fog_vulkan_execution_evidence_ready =
            renderer_result.environment_volumetric_fog_vulkan_execution_evidence_ready;
        environment_volumetric_fog_vulkan_froxel_output_ready =
            renderer_result.environment_volumetric_fog_vulkan_froxel_output_ready;
        environment_volumetric_fog_vulkan_scene_depth_ready =
            renderer_result.environment_volumetric_fog_vulkan_scene_depth_ready;
        environment_volumetric_fog_vulkan_compute_dispatches =
            renderer_result.environment_volumetric_fog_vulkan_compute_dispatches;
        environment_volumetric_fog_vulkan_descriptor_set_bindings =
            renderer_result.environment_volumetric_fog_vulkan_descriptor_set_bindings;
        environment_volumetric_fog_vulkan_synchronization2_barriers =
            renderer_result.environment_volumetric_fog_vulkan_synchronization2_barriers;
        environment_volumetric_fog_vulkan_froxel_readback_nonzero =
            renderer_result.environment_volumetric_fog_vulkan_froxel_readback_nonzero;
        environment_volumetric_fog_vulkan_exposes_native_handles =
            renderer_result.environment_volumetric_fog_vulkan_exposes_native_handles;
        environment_volumetric_fog_vulkan_policy_diagnostics_count =
            renderer_result.environment_volumetric_fog_vulkan_policy_diagnostics_count;
    }

    void apply_vulkan_volumetric_cloud_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_volumetric_cloud_vulkan_requested = environment_volumetric_cloud_vulkan_requested ||
                                                        renderer_result.environment_volumetric_cloud_vulkan_requested;
        environment_volumetric_cloud_vulkan_shader_contract_evidence_ready =
            renderer_result.environment_volumetric_cloud_vulkan_shader_contract_evidence_ready;
        environment_volumetric_cloud_vulkan_package_evidence_ready =
            renderer_result.environment_volumetric_cloud_vulkan_package_evidence_ready;
        environment_volumetric_cloud_vulkan_execution_evidence_ready =
            renderer_result.environment_volumetric_cloud_vulkan_execution_evidence_ready;
        environment_volumetric_cloud_vulkan_weather_map_ready =
            renderer_result.environment_volumetric_cloud_vulkan_weather_map_ready;
        environment_volumetric_cloud_vulkan_shape_noise_ready =
            renderer_result.environment_volumetric_cloud_vulkan_shape_noise_ready;
        environment_volumetric_cloud_vulkan_erosion_noise_ready =
            renderer_result.environment_volumetric_cloud_vulkan_erosion_noise_ready;
        environment_volumetric_cloud_vulkan_uploads_volume_textures =
            renderer_result.environment_volumetric_cloud_vulkan_uploads_volume_textures;
        environment_volumetric_cloud_vulkan_invokes_backend =
            renderer_result.environment_volumetric_cloud_vulkan_invokes_backend;
        environment_volumetric_cloud_vulkan_renderer_draws =
            renderer_result.environment_volumetric_cloud_vulkan_renderer_draws;
        environment_volumetric_cloud_vulkan_raymarch_passes =
            renderer_result.environment_volumetric_cloud_vulkan_raymarch_passes;
        environment_volumetric_cloud_vulkan_descriptor_set_bindings =
            renderer_result.environment_volumetric_cloud_vulkan_descriptor_set_bindings;
        environment_volumetric_cloud_vulkan_synchronization2_barriers =
            renderer_result.environment_volumetric_cloud_vulkan_synchronization2_barriers;
        environment_volumetric_cloud_vulkan_readback_nonzero =
            renderer_result.environment_volumetric_cloud_vulkan_readback_nonzero;
        environment_volumetric_cloud_vulkan_exposes_native_handles =
            renderer_result.environment_volumetric_cloud_vulkan_exposes_native_handles;
        environment_volumetric_cloud_vulkan_plays_audio =
            renderer_result.environment_volumetric_cloud_vulkan_plays_audio;
        environment_volumetric_cloud_vulkan_executes_precipitation =
            renderer_result.environment_volumetric_cloud_vulkan_executes_precipitation;
        environment_volumetric_cloud_vulkan_map_rows = renderer_result.environment_volumetric_cloud_vulkan_map_rows;
        environment_volumetric_cloud_vulkan_layer_rows = renderer_result.environment_volumetric_cloud_vulkan_layer_rows;
        environment_volumetric_cloud_vulkan_lighting_rows =
            renderer_result.environment_volumetric_cloud_vulkan_lighting_rows;
        environment_volumetric_cloud_vulkan_raymarch_rows =
            renderer_result.environment_volumetric_cloud_vulkan_raymarch_rows;
        environment_volumetric_cloud_vulkan_temporal_rows =
            renderer_result.environment_volumetric_cloud_vulkan_temporal_rows;
        environment_volumetric_cloud_vulkan_shadow_rows =
            renderer_result.environment_volumetric_cloud_vulkan_shadow_rows;
        environment_volumetric_cloud_vulkan_storm_rows = renderer_result.environment_volumetric_cloud_vulkan_storm_rows;
        environment_volumetric_cloud_vulkan_shader_contract_rows =
            renderer_result.environment_volumetric_cloud_vulkan_shader_contract_rows;
        environment_volumetric_cloud_vulkan_quality_rows =
            renderer_result.environment_volumetric_cloud_vulkan_quality_rows;
        environment_volumetric_cloud_vulkan_policy_diagnostics_count =
            renderer_result.environment_volumetric_cloud_vulkan_policy_diagnostics_count;
    }

    void apply_volumetric_cloud_result(const NativeRendererCreateResult& renderer_result) noexcept {
        environment_volumetric_cloud_requested =
            environment_volumetric_cloud_requested || renderer_result.environment_volumetric_cloud_requested;
        environment_volumetric_cloud_shader_contract_evidence_ready =
            renderer_result.environment_volumetric_cloud_shader_contract_evidence_ready;
        environment_volumetric_cloud_package_evidence_ready =
            renderer_result.environment_volumetric_cloud_package_evidence_ready;
        environment_volumetric_cloud_execution_evidence_ready =
            renderer_result.environment_volumetric_cloud_execution_evidence_ready;
        environment_volumetric_cloud_weather_map_ready = renderer_result.environment_volumetric_cloud_weather_map_ready;
        environment_volumetric_cloud_shape_noise_ready = renderer_result.environment_volumetric_cloud_shape_noise_ready;
        environment_volumetric_cloud_erosion_noise_ready =
            renderer_result.environment_volumetric_cloud_erosion_noise_ready;
        environment_volumetric_cloud_uploads_volume_textures =
            renderer_result.environment_volumetric_cloud_uploads_volume_textures;
        environment_volumetric_cloud_invokes_backend = renderer_result.environment_volumetric_cloud_invokes_backend;
        environment_volumetric_cloud_renderer_draws = renderer_result.environment_volumetric_cloud_renderer_draws;
        environment_volumetric_cloud_raymarch_passes = renderer_result.environment_volumetric_cloud_raymarch_passes;
        environment_volumetric_cloud_readback_nonzero = renderer_result.environment_volumetric_cloud_readback_nonzero;
        environment_volumetric_cloud_exposes_native_handles =
            renderer_result.environment_volumetric_cloud_exposes_native_handles;
        environment_volumetric_cloud_plays_audio = renderer_result.environment_volumetric_cloud_plays_audio;
        environment_volumetric_cloud_executes_precipitation =
            renderer_result.environment_volumetric_cloud_executes_precipitation;
        environment_volumetric_cloud_map_rows = renderer_result.environment_volumetric_cloud_map_rows;
        environment_volumetric_cloud_layer_rows = renderer_result.environment_volumetric_cloud_layer_rows;
        environment_volumetric_cloud_lighting_rows = renderer_result.environment_volumetric_cloud_lighting_rows;
        environment_volumetric_cloud_raymarch_rows = renderer_result.environment_volumetric_cloud_raymarch_rows;
        environment_volumetric_cloud_temporal_rows = renderer_result.environment_volumetric_cloud_temporal_rows;
        environment_volumetric_cloud_shadow_rows = renderer_result.environment_volumetric_cloud_shadow_rows;
        environment_volumetric_cloud_storm_rows = renderer_result.environment_volumetric_cloud_storm_rows;
        environment_volumetric_cloud_shader_contract_rows =
            renderer_result.environment_volumetric_cloud_shader_contract_rows;
        environment_volumetric_cloud_quality_rows = renderer_result.environment_volumetric_cloud_quality_rows;
        environment_volumetric_cloud_policy_diagnostics_count =
            renderer_result.environment_volumetric_cloud_policy_diagnostics_count;
    }
};

Win32DesktopPresentation::Win32DesktopPresentation(const Win32DesktopPresentationDesc& desc)
    : impl_(std::make_unique<Impl>()) {
    if (desc.window == nullptr) {
        throw std::invalid_argument("win32 desktop presentation requires a window");
    }
    if (!has_extent(desc.extent)) {
        throw std::invalid_argument("win32 desktop presentation extent must be non-zero");
    }

    impl_->requested_backend = requested_backend_from_desc(desc);
    impl_->allow_null_fallback = desc.allow_null_fallback;
    impl_->d3d12_tearing_requested = desc.request_tearing;
    impl_->swapchain_plan = plan_win32_d3d12_swapchain(Win32D3d12SwapChainPlanDesc{
        .extent = desc.extent,
        .format = rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = desc.vsync,
        .request_tearing = desc.request_tearing,
        .tearing_supported = false,
    });
    impl_->postprocess_depth_input_requested =
        desc.prefer_vulkan
            ? desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess_depth_input
            : desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                  desc.d3d12_scene_renderer->enable_postprocess_depth_input;
    impl_->environment_fog_requested =
        desc.prefer_vulkan ? desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_environment_fog
                           : desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                                 desc.d3d12_scene_renderer->enable_environment_fog;
    impl_->environment_fog_vulkan_package_requested = desc.prefer_vulkan && desc.vulkan_scene_renderer != nullptr &&
                                                      desc.vulkan_scene_renderer->enable_environment_fog;
    impl_->cloud_layer_requested = desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                                   (desc.d3d12_scene_renderer->enable_cloud_layer_package_evidence ||
                                    desc.d3d12_scene_renderer->enable_cloud_layer_renderer_execution);
    impl_->environment_precipitation_requested =
        desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
        (desc.d3d12_scene_renderer->enable_environment_precipitation_package_evidence ||
         desc.d3d12_scene_renderer->enable_environment_precipitation_renderer_execution);
    impl_->environment_precipitation_vulkan_requested =
        desc.prefer_vulkan && desc.vulkan_scene_renderer != nullptr &&
        (desc.vulkan_scene_renderer->enable_environment_precipitation_package_evidence ||
         desc.vulkan_scene_renderer->enable_environment_precipitation_renderer_execution);
    impl_->environment_volumetric_fog_requested =
        desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
        desc.d3d12_scene_renderer->enable_environment_volumetric_fog_package_evidence;
    impl_->environment_volumetric_fog_vulkan_requested =
        desc.prefer_vulkan && desc.vulkan_scene_renderer != nullptr &&
        desc.vulkan_scene_renderer->enable_environment_volumetric_fog_renderer_execution;
    impl_->environment_volumetric_cloud_vulkan_requested =
        desc.prefer_vulkan && desc.vulkan_scene_renderer != nullptr &&
        desc.vulkan_scene_renderer->enable_environment_volumetric_cloud_renderer_execution;
    impl_->environment_volumetric_cloud_requested =
        desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
        (desc.d3d12_scene_renderer->enable_environment_volumetric_cloud_package_evidence ||
         desc.d3d12_scene_renderer->enable_environment_volumetric_cloud_renderer_execution);
    impl_->directional_shadow_requested =
        desc.prefer_vulkan
            ? desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke
            : desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                  desc.d3d12_scene_renderer->enable_directional_shadow_smoke;
    impl_->native_ui_overlay_requested =
        desc.prefer_vulkan
            ? (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) ||
                  (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay)
            : desc.prefer_d3d12 &&
                  ((desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) ||
                   (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay));
    impl_->native_ui_texture_overlay_requested =
        desc.prefer_vulkan
            ? (desc.vulkan_scene_renderer != nullptr &&
               desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) ||
                  (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures)
            : desc.prefer_d3d12 &&
                  ((desc.d3d12_scene_renderer != nullptr &&
                    desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) ||
                   (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures));

    Win32DesktopPresentationFallbackReason fallback_reason{
        Win32DesktopPresentationFallbackReason::native_backend_unavailable};
    std::string fallback_message{"Native presentation is not selected; using NullRenderer fallback."};

    auto record_backend_report = [&](Win32DesktopPresentationBackend backend,
                                     Win32DesktopPresentationBackendReportStatus status,
                                     Win32DesktopPresentationFallbackReason reason, std::string message) {
        impl_->backend_reports.push_back(Win32DesktopPresentationBackendReport{
            .backend = backend,
            .status = status,
            .fallback_reason = reason,
            .diagnostic = message,
        });
        fallback_reason = reason;
        fallback_message = std::move(message);
    };

    auto record_backend_ready = [&](Win32DesktopPresentationBackend backend, std::string message) {
        impl_->backend_reports.push_back(Win32DesktopPresentationBackendReport{
            .backend = backend,
            .status = Win32DesktopPresentationBackendReportStatus::ready,
            .fallback_reason = Win32DesktopPresentationFallbackReason::none,
            .diagnostic = std::move(message),
        });
        impl_->fallback_reason = Win32DesktopPresentationFallbackReason::none;
    };

    if (desc.prefer_vulkan) {
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess_depth_input &&
            !desc.vulkan_scene_renderer->enable_postprocess) {
            const auto invalid_request = invalid_vulkan_postprocess_depth_input_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->postprocess_status = invalid_request.postprocess_status;
            impl_->postprocess_diagnostics = invalid_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_requested =
                impl_->postprocess_depth_input_requested || invalid_request.postprocess_depth_input_requested;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_native_ui_overlay_textures &&
                   !desc.vulkan_scene_renderer->enable_native_ui_overlay) {
            const auto invalid_request = invalid_vulkan_native_ui_texture_overlay_request(
                "Vulkan textured native UI overlay requires the native UI overlay pass to be enabled; using "
                "NullRenderer fallback.");
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay &&
                   !desc.vulkan_scene_renderer->enable_postprocess) {
            const auto invalid_request =
                desc.vulkan_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_vulkan_native_ui_texture_overlay_request(
                          "Vulkan textured native UI overlay requires scene postprocess to be enabled; using "
                          "NullRenderer fallback.")
                    : invalid_vulkan_native_ui_overlay_request(
                          "Vulkan native UI overlay requires scene postprocess to be enabled; using NullRenderer "
                          "fallback.");
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = invalid_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_directional_shadow_smoke &&
                   (!desc.vulkan_scene_renderer->enable_postprocess ||
                    !desc.vulkan_scene_renderer->enable_postprocess_depth_input)) {
            const auto invalid_request = invalid_vulkan_directional_shadow_request(
                "Vulkan directional shadow smoke requires scene postprocess and postprocess depth input to be "
                "enabled; using NullRenderer fallback.");
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = invalid_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = invalid_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || invalid_request.directional_shadow_requested;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_directional_shadow_smoke &&
                   !has_directional_shadow_bytecode(desc.vulkan_scene_renderer->shadow_vertex_shader,
                                                    desc.vulkan_scene_renderer->shadow_fragment_shader)) {
            const auto missing_request = missing_vulkan_directional_shadow_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = missing_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = missing_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || missing_request.directional_shadow_requested;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess &&
                   !has_postprocess_bytecode(desc.vulkan_scene_renderer->postprocess_vertex_shader,
                                             desc.vulkan_scene_renderer->postprocess_fragment_shader)) {
            const auto missing_request = missing_vulkan_postprocess_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->postprocess_status = missing_request.postprocess_status;
            impl_->postprocess_diagnostics = missing_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay &&
                   !has_native_ui_overlay_bytecode(desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader,
                                                   desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader)) {
            const auto missing_request =
                desc.vulkan_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_vulkan_native_ui_texture_overlay_request(
                          "Vulkan textured native UI overlay requires non-empty overlay vertex and fragment SPIR-V "
                          "bytecode; using NullRenderer fallback.")
                    : missing_vulkan_native_ui_overlay_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = missing_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = missing_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || missing_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = missing_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = missing_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || missing_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = missing_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   !valid_vulkan_scene_renderer_request(desc.vulkan_scene_renderer)) {
            const auto missing_request = missing_vulkan_scene_renderer_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
        } else if (desc.vulkan_renderer != nullptr && !valid_vulkan_renderer_request(desc.vulkan_renderer)) {
            const auto missing_request = missing_vulkan_renderer_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else if (desc.vulkan_scene_renderer == nullptr && desc.vulkan_renderer == nullptr) {
            const auto missing_request = missing_vulkan_renderer_request();
            record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else {
            bool scene_request_valid = true;
            if (desc.vulkan_scene_renderer != nullptr) {
                const auto scene_request = validate_scene_renderer_mesh_layout_request(
                    *desc.vulkan_scene_renderer->package, *desc.vulkan_scene_renderer->packet,
                    desc.vulkan_scene_renderer->vertex_buffers, desc.vulkan_scene_renderer->vertex_attributes,
                    desc.vulkan_scene_renderer->skinned_vertex_buffers,
                    desc.vulkan_scene_renderer->skinned_vertex_attributes,
                    desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings, "Vulkan");
                if (!scene_request.valid) {
                    record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                          Win32DesktopPresentationBackendReportStatus::missing_request,
                                          Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                                          scene_request.diagnostic);
                    impl_->scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
                    impl_->scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(impl_->scene_gpu_status, scene_request.diagnostic));
                    scene_request_valid = false;
                }
            }
            if (scene_request_valid) {
                const auto surface = probe_vulkan_surface(*desc.window);
                if (surface.surface.value == 0) {
                    record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                          Win32DesktopPresentationBackendReportStatus::native_window_unavailable,
                                          surface.failure_reason, surface.diagnostic);
                    if (desc.vulkan_scene_renderer != nullptr) {
                        impl_->scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
                        impl_->scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                            impl_->scene_gpu_status,
                            "Vulkan scene GPU bindings require a native Vulkan presentation surface; using "
                            "NullRenderer fallback."));
                        if (desc.vulkan_scene_renderer->enable_postprocess) {
                            impl_->postprocess_status = Win32DesktopPresentationPostprocessStatus::unavailable;
                            impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                impl_->postprocess_status,
                                "Vulkan scene postprocess requires a native Vulkan presentation surface; using "
                                "NullRenderer fallback."));
                            if (desc.vulkan_scene_renderer->enable_postprocess_depth_input) {
                                impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                    impl_->postprocess_status,
                                    "Vulkan scene postprocess depth input requires a native Vulkan presentation "
                                    "surface; using NullRenderer fallback."));
                            }
                        }
                        if (desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
                            impl_->directional_shadow_status =
                                Win32DesktopPresentationDirectionalShadowStatus::unavailable;
                            impl_->directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(
                                impl_->directional_shadow_status,
                                "Vulkan directional shadow smoke requires a native Vulkan presentation surface; "
                                "using NullRenderer fallback."));
                        }
                        if (desc.vulkan_scene_renderer->enable_native_ui_overlay) {
                            impl_->native_ui_overlay_status =
                                Win32DesktopPresentationNativeUiOverlayStatus::unavailable;
                            impl_->native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(
                                impl_->native_ui_overlay_status,
                                "Vulkan native UI overlay requires a native Vulkan presentation surface; using "
                                "NullRenderer fallback."));
                        }
                        if (desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
                            impl_->native_ui_texture_overlay_status =
                                Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable;
                            impl_->native_ui_texture_overlay_requested = true;
                            impl_->native_ui_texture_overlay_atlas_ready = false;
                            impl_->native_ui_texture_overlay_diagnostics.push_back(
                                make_native_ui_texture_overlay_diagnostic(
                                    impl_->native_ui_texture_overlay_status,
                                    "Vulkan textured native UI overlay requires a native Vulkan presentation surface; "
                                    "using NullRenderer fallback."));
                        }
                    }
                } else if (desc.vulkan_scene_renderer != nullptr) {
                    auto renderer_result = create_vulkan_scene_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = Win32DesktopPresentationBackend::vulkan;
                        impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                        impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                        impl_->postprocess_status = renderer_result.postprocess_status;
                        impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                        impl_->postprocess_depth_input_requested = impl_->postprocess_depth_input_requested ||
                                                                   renderer_result.postprocess_depth_input_requested;
                        impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                        impl_->apply_environment_fog_result(renderer_result);
                        impl_->apply_physical_sky_result(renderer_result);
                        impl_->apply_cloud_layer_result(renderer_result);
                        impl_->apply_precipitation_result(renderer_result);
                        impl_->apply_vulkan_precipitation_result(renderer_result);
                        impl_->apply_volumetric_fog_result(renderer_result);
                        impl_->apply_vulkan_volumetric_fog_result(renderer_result);
                        impl_->apply_vulkan_volumetric_cloud_result(renderer_result);
                        impl_->apply_volumetric_cloud_result(renderer_result);
                        impl_->directional_shadow_requested =
                            impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                        impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                        impl_->directional_shadow_diagnostics =
                            std::move(renderer_result.directional_shadow_diagnostics);
                        impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                        impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                        impl_->directional_shadow_filter_tap_count =
                            renderer_result.directional_shadow_filter_tap_count;
                        impl_->directional_shadow_filter_radius_texels =
                            renderer_result.directional_shadow_filter_radius_texels;
                        impl_->directional_shadow_cascade_count = renderer_result.directional_shadow_cascade_count;
                        impl_->directional_shadow_cascade_tile_width =
                            renderer_result.directional_shadow_cascade_tile_width;
                        impl_->directional_shadow_atlas_width = renderer_result.directional_shadow_atlas_width;
                        impl_->directional_shadow_atlas_height = renderer_result.directional_shadow_atlas_height;
                        impl_->directional_shadow_light_space_cascades =
                            renderer_result.directional_shadow_light_space_cascades;
                        impl_->directional_shadow_cascade_splits = renderer_result.directional_shadow_cascade_splits;
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        impl_->framegraph_passes = renderer_result.framegraph_passes;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->scene_gpu_renderer = renderer_result.scene_gpu_renderer;
                        record_backend_ready(Win32DesktopPresentationBackend::vulkan, "Vulkan scene renderer ready.");
                        return;
                    }
                    record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                    impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                    impl_->postprocess_status = renderer_result.postprocess_status;
                    impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                    impl_->postprocess_depth_input_requested =
                        impl_->postprocess_depth_input_requested || renderer_result.postprocess_depth_input_requested;
                    impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                    impl_->apply_environment_fog_result(renderer_result);
                    impl_->apply_physical_sky_result(renderer_result);
                    impl_->apply_cloud_layer_result(renderer_result);
                    impl_->apply_precipitation_result(renderer_result);
                    impl_->apply_vulkan_precipitation_result(renderer_result);
                    impl_->apply_volumetric_fog_result(renderer_result);
                    impl_->apply_vulkan_volumetric_fog_result(renderer_result);
                    impl_->apply_vulkan_volumetric_cloud_result(renderer_result);
                    impl_->apply_volumetric_cloud_result(renderer_result);
                    impl_->directional_shadow_requested =
                        impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                    impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                    impl_->directional_shadow_diagnostics = std::move(renderer_result.directional_shadow_diagnostics);
                    impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                    impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                    impl_->directional_shadow_filter_tap_count = renderer_result.directional_shadow_filter_tap_count;
                    impl_->directional_shadow_filter_radius_texels =
                        renderer_result.directional_shadow_filter_radius_texels;
                    impl_->directional_shadow_cascade_count = renderer_result.directional_shadow_cascade_count;
                    impl_->directional_shadow_cascade_tile_width =
                        renderer_result.directional_shadow_cascade_tile_width;
                    impl_->directional_shadow_atlas_width = renderer_result.directional_shadow_atlas_width;
                    impl_->directional_shadow_atlas_height = renderer_result.directional_shadow_atlas_height;
                    impl_->directional_shadow_light_space_cascades =
                        renderer_result.directional_shadow_light_space_cascades;
                    impl_->directional_shadow_cascade_splits = renderer_result.directional_shadow_cascade_splits;
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                    impl_->framegraph_passes = renderer_result.framegraph_passes;
                } else {
                    auto renderer_result = create_vulkan_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = Win32DesktopPresentationBackend::vulkan;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        record_backend_ready(Win32DesktopPresentationBackend::vulkan, "Vulkan renderer ready.");
                        return;
                    }
                    record_backend_report(Win32DesktopPresentationBackend::vulkan,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                }
            }
        }
    } else if (desc.prefer_d3d12) {
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess_depth_input &&
            !desc.d3d12_scene_renderer->enable_postprocess) {
            const auto invalid_request = invalid_d3d12_postprocess_depth_input_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->postprocess_status = invalid_request.postprocess_status;
            impl_->postprocess_diagnostics = invalid_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_requested =
                impl_->postprocess_depth_input_requested || invalid_request.postprocess_depth_input_requested;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr &&
                   desc.d3d12_scene_renderer->enable_native_ui_overlay_textures &&
                   !desc.d3d12_scene_renderer->enable_native_ui_overlay) {
            const auto invalid_request = invalid_d3d12_native_ui_texture_overlay_request(
                "D3D12 textured native UI overlay requires the native UI overlay pass to be enabled; using "
                "NullRenderer fallback.");
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay &&
                   !desc.d3d12_scene_renderer->enable_postprocess) {
            const auto invalid_request =
                desc.d3d12_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_d3d12_native_ui_texture_overlay_request(
                          "D3D12 textured native UI overlay requires scene postprocess to be enabled; using "
                          "NullRenderer fallback.")
                    : invalid_d3d12_native_ui_overlay_request(
                          "D3D12 native UI overlay requires scene postprocess to be enabled; using NullRenderer "
                          "fallback.");
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = invalid_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke &&
                   (!desc.d3d12_scene_renderer->enable_postprocess ||
                    !desc.d3d12_scene_renderer->enable_postprocess_depth_input)) {
            const auto invalid_request = invalid_d3d12_directional_shadow_request(
                "D3D12 directional shadow smoke requires scene postprocess and postprocess depth input to be "
                "enabled; using NullRenderer fallback.");
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = invalid_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = invalid_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || invalid_request.directional_shadow_requested;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke &&
                   !has_directional_shadow_bytecode(desc.d3d12_scene_renderer->shadow_vertex_shader,
                                                    desc.d3d12_scene_renderer->shadow_fragment_shader)) {
            const auto missing_request = missing_d3d12_directional_shadow_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = missing_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = missing_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || missing_request.directional_shadow_requested;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess &&
                   !has_postprocess_bytecode(desc.d3d12_scene_renderer->postprocess_vertex_shader,
                                             desc.d3d12_scene_renderer->postprocess_fragment_shader)) {
            const auto missing_request = missing_d3d12_postprocess_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->postprocess_status = missing_request.postprocess_status;
            impl_->postprocess_diagnostics = missing_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay &&
                   !has_native_ui_overlay_bytecode(desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader,
                                                   desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader)) {
            const auto missing_request =
                desc.d3d12_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_d3d12_native_ui_texture_overlay_request(
                          "D3D12 textured native UI overlay requires non-empty overlay vertex and fragment shader "
                          "bytecode; using NullRenderer fallback.")
                    : missing_d3d12_native_ui_overlay_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = missing_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = missing_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || missing_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = missing_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = missing_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || missing_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = missing_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.d3d12_scene_renderer != nullptr &&
                   !valid_d3d12_scene_renderer_request(desc.d3d12_scene_renderer)) {
            const auto missing_request = missing_d3d12_scene_renderer_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
        } else if (desc.d3d12_renderer != nullptr && !valid_d3d12_renderer_request(desc.d3d12_renderer)) {
            const auto missing_request = missing_d3d12_renderer_request();
            record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                  Win32DesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else {
            bool scene_request_valid = true;
            if (desc.d3d12_scene_renderer != nullptr) {
                const auto scene_request = validate_scene_renderer_mesh_layout_request(
                    *desc.d3d12_scene_renderer->package, *desc.d3d12_scene_renderer->packet,
                    desc.d3d12_scene_renderer->vertex_buffers, desc.d3d12_scene_renderer->vertex_attributes,
                    desc.d3d12_scene_renderer->skinned_vertex_buffers,
                    desc.d3d12_scene_renderer->skinned_vertex_attributes,
                    desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings, "D3D12");
                if (!scene_request.valid) {
                    record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                          Win32DesktopPresentationBackendReportStatus::missing_request,
                                          Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                                          scene_request.diagnostic);
                    impl_->scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::invalid_request;
                    impl_->scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(impl_->scene_gpu_status, scene_request.diagnostic));
                    scene_request_valid = false;
                }
            }
            if (scene_request_valid) {
                const auto surface = probe_d3d12_surface(*desc.window);
                if (surface.surface.value == 0) {
                    record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                          Win32DesktopPresentationBackendReportStatus::native_window_unavailable,
                                          surface.failure_reason, surface.diagnostic);
                    if (desc.d3d12_scene_renderer != nullptr) {
                        impl_->scene_gpu_status = Win32DesktopPresentationSceneGpuBindingStatus::unavailable;
                        impl_->scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                            impl_->scene_gpu_status,
                            "D3D12 scene GPU bindings require a native D3D12 presentation surface; using "
                            "NullRenderer fallback."));
                        if (desc.d3d12_scene_renderer->enable_postprocess) {
                            impl_->postprocess_status = Win32DesktopPresentationPostprocessStatus::unavailable;
                            impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                impl_->postprocess_status,
                                "D3D12 scene postprocess requires a native D3D12 presentation surface; using "
                                "NullRenderer fallback."));
                            if (desc.d3d12_scene_renderer->enable_postprocess_depth_input) {
                                impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                    impl_->postprocess_status,
                                    "D3D12 scene postprocess depth input requires a native D3D12 presentation "
                                    "surface; using NullRenderer fallback."));
                            }
                        }
                        if (desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
                            impl_->directional_shadow_status =
                                Win32DesktopPresentationDirectionalShadowStatus::unavailable;
                            impl_->directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(
                                impl_->directional_shadow_status,
                                "D3D12 directional shadow smoke requires a native D3D12 presentation surface; "
                                "using NullRenderer fallback."));
                        }
                        if (desc.d3d12_scene_renderer->enable_native_ui_overlay) {
                            impl_->native_ui_overlay_status =
                                Win32DesktopPresentationNativeUiOverlayStatus::unavailable;
                            impl_->native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(
                                impl_->native_ui_overlay_status,
                                "D3D12 native UI overlay requires a native D3D12 presentation surface; using "
                                "NullRenderer fallback."));
                        }
                        if (desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
                            impl_->native_ui_texture_overlay_status =
                                Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable;
                            impl_->native_ui_texture_overlay_requested = true;
                            impl_->native_ui_texture_overlay_atlas_ready = false;
                            impl_->native_ui_texture_overlay_diagnostics.push_back(
                                make_native_ui_texture_overlay_diagnostic(
                                    impl_->native_ui_texture_overlay_status,
                                    "D3D12 textured native UI overlay requires a native D3D12 presentation surface; "
                                    "using NullRenderer fallback."));
                        }
                    }
                } else if (desc.d3d12_scene_renderer != nullptr) {
                    auto renderer_result = create_d3d12_scene_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = Win32DesktopPresentationBackend::d3d12;
                        impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                        impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                        impl_->postprocess_status = renderer_result.postprocess_status;
                        impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                        impl_->postprocess_depth_input_requested = impl_->postprocess_depth_input_requested ||
                                                                   renderer_result.postprocess_depth_input_requested;
                        impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                        impl_->apply_environment_fog_result(renderer_result);
                        impl_->apply_physical_sky_result(renderer_result);
                        impl_->apply_cloud_layer_result(renderer_result);
                        impl_->apply_precipitation_result(renderer_result);
                        impl_->apply_vulkan_precipitation_result(renderer_result);
                        impl_->apply_volumetric_fog_result(renderer_result);
                        impl_->apply_vulkan_volumetric_fog_result(renderer_result);
                        impl_->apply_vulkan_volumetric_cloud_result(renderer_result);
                        impl_->apply_volumetric_cloud_result(renderer_result);
                        impl_->directional_shadow_requested =
                            impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                        impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                        impl_->directional_shadow_diagnostics =
                            std::move(renderer_result.directional_shadow_diagnostics);
                        impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                        impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                        impl_->directional_shadow_filter_tap_count =
                            renderer_result.directional_shadow_filter_tap_count;
                        impl_->directional_shadow_filter_radius_texels =
                            renderer_result.directional_shadow_filter_radius_texels;
                        impl_->directional_shadow_cascade_count = renderer_result.directional_shadow_cascade_count;
                        impl_->directional_shadow_cascade_tile_width =
                            renderer_result.directional_shadow_cascade_tile_width;
                        impl_->directional_shadow_atlas_width = renderer_result.directional_shadow_atlas_width;
                        impl_->directional_shadow_atlas_height = renderer_result.directional_shadow_atlas_height;
                        impl_->directional_shadow_light_space_cascades =
                            renderer_result.directional_shadow_light_space_cascades;
                        impl_->directional_shadow_cascade_splits = renderer_result.directional_shadow_cascade_splits;
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        impl_->framegraph_passes = renderer_result.framegraph_passes;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->scene_gpu_renderer = renderer_result.scene_gpu_renderer;
                        record_backend_ready(Win32DesktopPresentationBackend::d3d12, "D3D12 scene renderer ready.");
                        return;
                    }
                    record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                    impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                    impl_->postprocess_status = renderer_result.postprocess_status;
                    impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                    impl_->postprocess_depth_input_requested =
                        impl_->postprocess_depth_input_requested || renderer_result.postprocess_depth_input_requested;
                    impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                    impl_->apply_environment_fog_result(renderer_result);
                    impl_->apply_physical_sky_result(renderer_result);
                    impl_->apply_cloud_layer_result(renderer_result);
                    impl_->apply_precipitation_result(renderer_result);
                    impl_->apply_vulkan_precipitation_result(renderer_result);
                    impl_->apply_volumetric_fog_result(renderer_result);
                    impl_->apply_vulkan_volumetric_fog_result(renderer_result);
                    impl_->apply_vulkan_volumetric_cloud_result(renderer_result);
                    impl_->apply_volumetric_cloud_result(renderer_result);
                    impl_->directional_shadow_requested =
                        impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                    impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                    impl_->directional_shadow_diagnostics = std::move(renderer_result.directional_shadow_diagnostics);
                    impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                    impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                    impl_->directional_shadow_filter_tap_count = renderer_result.directional_shadow_filter_tap_count;
                    impl_->directional_shadow_filter_radius_texels =
                        renderer_result.directional_shadow_filter_radius_texels;
                    impl_->directional_shadow_cascade_count = renderer_result.directional_shadow_cascade_count;
                    impl_->directional_shadow_cascade_tile_width =
                        renderer_result.directional_shadow_cascade_tile_width;
                    impl_->directional_shadow_atlas_width = renderer_result.directional_shadow_atlas_width;
                    impl_->directional_shadow_atlas_height = renderer_result.directional_shadow_atlas_height;
                    impl_->directional_shadow_light_space_cascades =
                        renderer_result.directional_shadow_light_space_cascades;
                    impl_->directional_shadow_cascade_splits = renderer_result.directional_shadow_cascade_splits;
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                    impl_->framegraph_passes = renderer_result.framegraph_passes;
                } else if (desc.d3d12_renderer == nullptr) {
                    const auto missing_request = missing_d3d12_renderer_request();
                    record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                          Win32DesktopPresentationBackendReportStatus::missing_request,
                                          missing_request.failure_reason, missing_request.diagnostic);
                } else {
                    auto renderer_result = create_d3d12_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = Win32DesktopPresentationBackend::d3d12;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        record_backend_ready(Win32DesktopPresentationBackend::d3d12, "D3D12 renderer ready.");
                        return;
                    }
                    record_backend_report(Win32DesktopPresentationBackend::d3d12,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                }
            }
        }
    } else {
        record_backend_report(
            Win32DesktopPresentationBackend::null_renderer, Win32DesktopPresentationBackendReportStatus::not_requested,
            Win32DesktopPresentationFallbackReason::none, "Native presentation is not selected; using NullRenderer.");
    }

    if (!desc.allow_null_fallback) {
        throw std::logic_error(fallback_message);
    }

    impl_->backend = Win32DesktopPresentationBackend::null_renderer;
    impl_->fallback_reason = fallback_reason;
    impl_->diagnostics.push_back(make_diagnostic(fallback_reason, std::move(fallback_message)));
    impl_->renderer = std::make_unique<NullRenderer>(desc.extent);
}

Win32DesktopPresentation::~Win32DesktopPresentation() = default;

IRenderer& Win32DesktopPresentation::renderer() noexcept {
    return *impl_->renderer;
}

const IRenderer& Win32DesktopPresentation::renderer() const noexcept {
    return *impl_->renderer;
}

Win32DesktopPresentationBackend Win32DesktopPresentation::backend() const noexcept {
    return impl_->backend;
}

std::string_view Win32DesktopPresentation::backend_name() const noexcept {
    return win32_desktop_presentation_backend_name(impl_->backend);
}

Win32DesktopPresentationReport Win32DesktopPresentation::report() const noexcept {
    const auto renderer_stats = impl_->renderer != nullptr ? impl_->renderer->stats() : RendererStats{};
    const auto rhi_stats = impl_->device != nullptr ? impl_->device->stats() : rhi::RhiStats{};
    return Win32DesktopPresentationReport{
        .requested_backend = impl_->requested_backend,
        .selected_backend = impl_->backend,
        .fallback_reason = impl_->fallback_reason,
        .used_null_fallback = impl_->backend == Win32DesktopPresentationBackend::null_renderer &&
                              impl_->requested_backend != Win32DesktopPresentationBackend::null_renderer,
        .allow_null_fallback = impl_->allow_null_fallback,
        .swapchain_plan = impl_->swapchain_plan,
        .d3d12_tearing_requested = impl_->d3d12_tearing_requested,
        .d3d12_tearing_supported = impl_->d3d12_tearing_supported,
        .d3d12_tearing_active = impl_->d3d12_tearing_active,
        .present_status = rhi_stats.present_calls > 0 ? Win32DesktopPresentationPresentStatus::ready
                                                      : Win32DesktopPresentationPresentStatus::not_requested,
        .resize_status = rhi_stats.swapchain_resizes > 0 ? Win32DesktopPresentationResizeStatus::ready
                                                         : Win32DesktopPresentationResizeStatus::not_requested,
        .scene_gpu_status = impl_->scene_gpu_status,
        .scene_gpu_stats = scene_gpu_binding_stats(),
        .postprocess_status = impl_->postprocess_status,
        .postprocess_depth_input_requested = impl_->postprocess_depth_input_requested,
        .postprocess_depth_input_ready = impl_->postprocess_depth_input_ready,
        .environment_fog_requested = impl_->environment_fog_requested,
        .environment_fog_constant_buffer_ready = impl_->environment_fog_constant_buffer_ready,
        .environment_fog_constant_buffer_bytes = impl_->environment_fog_constant_buffer_bytes,
        .environment_fog_vulkan_package_requested = impl_->environment_fog_vulkan_package_requested,
        .environment_fog_vulkan_package_shader_contract_evidence_ready =
            impl_->environment_fog_vulkan_package_shader_contract_evidence_ready,
        .environment_fog_vulkan_package_evidence_ready = impl_->environment_fog_vulkan_package_evidence_ready,
        .environment_fog_vulkan_package_constant_buffer_ready =
            impl_->environment_fog_vulkan_package_constant_buffer_ready,
        .environment_fog_vulkan_package_constant_buffer_bytes =
            impl_->environment_fog_vulkan_package_constant_buffer_bytes,
        .physical_sky_vulkan_package_requested = impl_->physical_sky_vulkan_package_requested,
        .physical_sky_vulkan_package_shader_contract_evidence_ready =
            impl_->physical_sky_vulkan_package_shader_contract_evidence_ready,
        .physical_sky_vulkan_package_evidence_ready = impl_->physical_sky_vulkan_package_evidence_ready,
        .physical_sky_vulkan_package_execution_evidence_ready =
            impl_->physical_sky_vulkan_package_execution_evidence_ready,
        .physical_sky_vulkan_package_constant_buffer_ready = impl_->physical_sky_vulkan_package_constant_buffer_ready,
        .physical_sky_vulkan_package_constant_buffer_bytes = impl_->physical_sky_vulkan_package_constant_buffer_bytes,
        .physical_sky_vulkan_package_allocates_lut_textures = impl_->physical_sky_vulkan_package_allocates_lut_textures,
        .physical_sky_vulkan_package_invokes_backend = impl_->physical_sky_vulkan_package_invokes_backend,
        .physical_sky_vulkan_package_exposes_native_handles = impl_->physical_sky_vulkan_package_exposes_native_handles,
        .physical_sky_vulkan_package_constant_layout_rows = impl_->physical_sky_vulkan_package_constant_layout_rows,
        .physical_sky_vulkan_package_lut_intent_rows = impl_->physical_sky_vulkan_package_lut_intent_rows,
        .physical_sky_vulkan_package_policy_diagnostics_count =
            impl_->physical_sky_vulkan_package_policy_diagnostics_count,
        .physical_sky_requested = impl_->physical_sky_requested,
        .physical_sky_shader_contract_evidence_ready = impl_->physical_sky_shader_contract_evidence_ready,
        .physical_sky_package_evidence_ready = impl_->physical_sky_package_evidence_ready,
        .physical_sky_execution_evidence_ready = impl_->physical_sky_execution_evidence_ready,
        .physical_sky_constant_buffer_ready = impl_->physical_sky_constant_buffer_ready,
        .physical_sky_constant_buffer_bytes = impl_->physical_sky_constant_buffer_bytes,
        .physical_sky_allocates_lut_textures = impl_->physical_sky_allocates_lut_textures,
        .physical_sky_invokes_backend = impl_->physical_sky_invokes_backend,
        .physical_sky_exposes_native_handles = impl_->physical_sky_exposes_native_handles,
        .physical_sky_constant_layout_rows = impl_->physical_sky_constant_layout_rows,
        .physical_sky_lut_intent_rows = impl_->physical_sky_lut_intent_rows,
        .physical_sky_policy_diagnostics_count = impl_->physical_sky_policy_diagnostics_count,
        .cloud_layer_requested = impl_->cloud_layer_requested,
        .cloud_layer_shader_contract_evidence_ready = impl_->cloud_layer_shader_contract_evidence_ready,
        .cloud_layer_package_evidence_ready = impl_->cloud_layer_package_evidence_ready,
        .cloud_layer_execution_evidence_ready = impl_->cloud_layer_execution_evidence_ready,
        .cloud_layer_uploads_textures = impl_->cloud_layer_uploads_textures,
        .cloud_layer_invokes_backend = impl_->cloud_layer_invokes_backend,
        .cloud_layer_exposes_native_handles = impl_->cloud_layer_exposes_native_handles,
        .cloud_layer_uses_volumetric_clouds = impl_->cloud_layer_uses_volumetric_clouds,
        .cloud_layer_uses_latlong_projection = impl_->cloud_layer_uses_latlong_projection,
        .cloud_layer_uses_flow_map = impl_->cloud_layer_uses_flow_map,
        .cloud_layer_texture_rows = impl_->cloud_layer_texture_rows,
        .cloud_layer_visual_rows = impl_->cloud_layer_visual_rows,
        .cloud_layer_ibl_rows = impl_->cloud_layer_ibl_rows,
        .cloud_layer_shader_contract_rows = impl_->cloud_layer_shader_contract_rows,
        .cloud_layer_quality_rows = impl_->cloud_layer_quality_rows,
        .cloud_layer_policy_diagnostics_count = impl_->cloud_layer_policy_diagnostics_count,
        .cloud_layer_renderer_draws = renderer_stats.cloud_layer_draws,
        .environment_precipitation_requested = impl_->environment_precipitation_requested,
        .environment_precipitation_weather = impl_->environment_precipitation_weather,
        .environment_precipitation_kind = impl_->environment_precipitation_kind,
        .environment_precipitation_shader_contract_evidence_ready =
            impl_->environment_precipitation_shader_contract_evidence_ready,
        .environment_precipitation_package_evidence_ready = impl_->environment_precipitation_package_evidence_ready,
        .environment_precipitation_execution_evidence_ready = impl_->environment_precipitation_execution_evidence_ready,
        .environment_precipitation_uploads_particle_buffers = impl_->environment_precipitation_uploads_particle_buffers,
        .environment_precipitation_invokes_backend = impl_->environment_precipitation_invokes_backend,
        .environment_precipitation_exposes_native_handles = impl_->environment_precipitation_exposes_native_handles,
        .environment_precipitation_mutates_materials = impl_->environment_precipitation_mutates_materials,
        .environment_precipitation_plays_audio = impl_->environment_precipitation_plays_audio,
        .environment_precipitation_renderer_draws = impl_->environment_precipitation_renderer_draws,
        .environment_precipitation_depth_occlusion_readback = impl_->environment_precipitation_depth_occlusion_readback,
        .environment_precipitation_uses_camera_near_particles =
            impl_->environment_precipitation_uses_camera_near_particles,
        .environment_precipitation_uses_scene_depth_occlusion =
            impl_->environment_precipitation_uses_scene_depth_occlusion,
        .environment_precipitation_weather_rows = impl_->environment_precipitation_weather_rows,
        .environment_precipitation_particle_rows = impl_->environment_precipitation_particle_rows,
        .environment_precipitation_occlusion_rows = impl_->environment_precipitation_occlusion_rows,
        .environment_precipitation_wetness_rows = impl_->environment_precipitation_wetness_rows,
        .environment_precipitation_audio_handoff_rows = impl_->environment_precipitation_audio_handoff_rows,
        .environment_precipitation_shader_rows = impl_->environment_precipitation_shader_rows,
        .environment_precipitation_quality_rows = impl_->environment_precipitation_quality_rows,
        .environment_precipitation_policy_diagnostics_count = impl_->environment_precipitation_policy_diagnostics_count,
        .environment_precipitation_vulkan_requested = impl_->environment_precipitation_vulkan_requested,
        .environment_precipitation_vulkan_weather = impl_->environment_precipitation_vulkan_weather,
        .environment_precipitation_vulkan_kind = impl_->environment_precipitation_vulkan_kind,
        .environment_precipitation_vulkan_shader_contract_evidence_ready =
            impl_->environment_precipitation_vulkan_shader_contract_evidence_ready,
        .environment_precipitation_vulkan_package_evidence_ready =
            impl_->environment_precipitation_vulkan_package_evidence_ready,
        .environment_precipitation_vulkan_execution_evidence_ready =
            impl_->environment_precipitation_vulkan_execution_evidence_ready,
        .environment_precipitation_vulkan_uses_camera_near_particles =
            impl_->environment_precipitation_vulkan_uses_camera_near_particles,
        .environment_precipitation_vulkan_uses_scene_depth_occlusion =
            impl_->environment_precipitation_vulkan_uses_scene_depth_occlusion,
        .environment_precipitation_vulkan_weather_rows = impl_->environment_precipitation_vulkan_weather_rows,
        .environment_precipitation_vulkan_particle_rows = impl_->environment_precipitation_vulkan_particle_rows,
        .environment_precipitation_vulkan_occlusion_rows = impl_->environment_precipitation_vulkan_occlusion_rows,
        .environment_precipitation_vulkan_wetness_rows = impl_->environment_precipitation_vulkan_wetness_rows,
        .environment_precipitation_vulkan_audio_handoff_rows =
            impl_->environment_precipitation_vulkan_audio_handoff_rows,
        .environment_precipitation_vulkan_shader_rows = impl_->environment_precipitation_vulkan_shader_rows,
        .environment_precipitation_vulkan_quality_rows = impl_->environment_precipitation_vulkan_quality_rows,
        .environment_precipitation_vulkan_particle_buffer_uploads =
            impl_->environment_precipitation_vulkan_particle_buffer_uploads,
        .environment_precipitation_vulkan_backend_invocations =
            impl_->environment_precipitation_vulkan_backend_invocations,
        .environment_precipitation_vulkan_renderer_draws = impl_->environment_precipitation_vulkan_renderer_draws,
        .environment_precipitation_vulkan_depth_occlusion_readback =
            impl_->environment_precipitation_vulkan_depth_occlusion_readback,
        .environment_precipitation_vulkan_descriptor_set_bindings =
            impl_->environment_precipitation_vulkan_descriptor_set_bindings,
        .environment_precipitation_vulkan_synchronization2_barriers =
            impl_->environment_precipitation_vulkan_synchronization2_barriers,
        .environment_precipitation_vulkan_exposes_native_handles =
            impl_->environment_precipitation_vulkan_exposes_native_handles,
        .environment_precipitation_vulkan_mutates_materials = impl_->environment_precipitation_vulkan_mutates_materials,
        .environment_precipitation_vulkan_plays_audio = impl_->environment_precipitation_vulkan_plays_audio,
        .environment_precipitation_vulkan_policy_diagnostics_count =
            impl_->environment_precipitation_vulkan_policy_diagnostics_count,
        .environment_volumetric_fog_requested = impl_->environment_volumetric_fog_requested,
        .environment_volumetric_fog_shader_contract_evidence_ready =
            impl_->environment_volumetric_fog_shader_contract_evidence_ready,
        .environment_volumetric_fog_package_evidence_ready = impl_->environment_volumetric_fog_package_evidence_ready,
        .environment_volumetric_fog_execution_evidence_ready =
            impl_->environment_volumetric_fog_execution_evidence_ready,
        .environment_volumetric_fog_froxel_output_ready = impl_->environment_volumetric_fog_froxel_output_ready,
        .environment_volumetric_fog_scene_depth_ready = impl_->environment_volumetric_fog_scene_depth_ready,
        .environment_volumetric_fog_compute_dispatches = impl_->environment_volumetric_fog_compute_dispatches,
        .environment_volumetric_fog_exposes_native_handles = impl_->environment_volumetric_fog_exposes_native_handles,
        .environment_volumetric_fog_policy_diagnostics_count =
            impl_->environment_volumetric_fog_policy_diagnostics_count,
        .environment_volumetric_fog_vulkan_requested = impl_->environment_volumetric_fog_vulkan_requested,
        .environment_volumetric_fog_vulkan_shader_contract_evidence_ready =
            impl_->environment_volumetric_fog_vulkan_shader_contract_evidence_ready,
        .environment_volumetric_fog_vulkan_package_evidence_ready =
            impl_->environment_volumetric_fog_vulkan_package_evidence_ready,
        .environment_volumetric_fog_vulkan_execution_evidence_ready =
            impl_->environment_volumetric_fog_vulkan_execution_evidence_ready,
        .environment_volumetric_fog_vulkan_froxel_output_ready =
            impl_->environment_volumetric_fog_vulkan_froxel_output_ready,
        .environment_volumetric_fog_vulkan_scene_depth_ready =
            impl_->environment_volumetric_fog_vulkan_scene_depth_ready,
        .environment_volumetric_fog_vulkan_compute_dispatches =
            impl_->environment_volumetric_fog_vulkan_compute_dispatches,
        .environment_volumetric_fog_vulkan_descriptor_set_bindings =
            impl_->environment_volumetric_fog_vulkan_descriptor_set_bindings,
        .environment_volumetric_fog_vulkan_synchronization2_barriers =
            impl_->environment_volumetric_fog_vulkan_synchronization2_barriers,
        .environment_volumetric_fog_vulkan_froxel_readback_nonzero =
            impl_->environment_volumetric_fog_vulkan_froxel_readback_nonzero,
        .environment_volumetric_fog_vulkan_exposes_native_handles =
            impl_->environment_volumetric_fog_vulkan_exposes_native_handles,
        .environment_volumetric_fog_vulkan_policy_diagnostics_count =
            impl_->environment_volumetric_fog_vulkan_policy_diagnostics_count,
        .environment_volumetric_cloud_vulkan_requested = impl_->environment_volumetric_cloud_vulkan_requested,
        .environment_volumetric_cloud_vulkan_shader_contract_evidence_ready =
            impl_->environment_volumetric_cloud_vulkan_shader_contract_evidence_ready,
        .environment_volumetric_cloud_vulkan_package_evidence_ready =
            impl_->environment_volumetric_cloud_vulkan_package_evidence_ready,
        .environment_volumetric_cloud_vulkan_execution_evidence_ready =
            impl_->environment_volumetric_cloud_vulkan_execution_evidence_ready,
        .environment_volumetric_cloud_vulkan_weather_map_ready =
            impl_->environment_volumetric_cloud_vulkan_weather_map_ready,
        .environment_volumetric_cloud_vulkan_shape_noise_ready =
            impl_->environment_volumetric_cloud_vulkan_shape_noise_ready,
        .environment_volumetric_cloud_vulkan_erosion_noise_ready =
            impl_->environment_volumetric_cloud_vulkan_erosion_noise_ready,
        .environment_volumetric_cloud_vulkan_uploads_volume_textures =
            impl_->environment_volumetric_cloud_vulkan_uploads_volume_textures,
        .environment_volumetric_cloud_vulkan_invokes_backend =
            impl_->environment_volumetric_cloud_vulkan_invokes_backend,
        .environment_volumetric_cloud_vulkan_renderer_draws = impl_->environment_volumetric_cloud_vulkan_renderer_draws,
        .environment_volumetric_cloud_vulkan_raymarch_passes =
            impl_->environment_volumetric_cloud_vulkan_raymarch_passes,
        .environment_volumetric_cloud_vulkan_descriptor_set_bindings =
            impl_->environment_volumetric_cloud_vulkan_descriptor_set_bindings,
        .environment_volumetric_cloud_vulkan_synchronization2_barriers =
            impl_->environment_volumetric_cloud_vulkan_synchronization2_barriers,
        .environment_volumetric_cloud_vulkan_readback_nonzero =
            impl_->environment_volumetric_cloud_vulkan_readback_nonzero,
        .environment_volumetric_cloud_vulkan_exposes_native_handles =
            impl_->environment_volumetric_cloud_vulkan_exposes_native_handles,
        .environment_volumetric_cloud_vulkan_plays_audio = impl_->environment_volumetric_cloud_vulkan_plays_audio,
        .environment_volumetric_cloud_vulkan_executes_precipitation =
            impl_->environment_volumetric_cloud_vulkan_executes_precipitation,
        .environment_volumetric_cloud_vulkan_map_rows = impl_->environment_volumetric_cloud_vulkan_map_rows,
        .environment_volumetric_cloud_vulkan_layer_rows = impl_->environment_volumetric_cloud_vulkan_layer_rows,
        .environment_volumetric_cloud_vulkan_lighting_rows = impl_->environment_volumetric_cloud_vulkan_lighting_rows,
        .environment_volumetric_cloud_vulkan_raymarch_rows = impl_->environment_volumetric_cloud_vulkan_raymarch_rows,
        .environment_volumetric_cloud_vulkan_temporal_rows = impl_->environment_volumetric_cloud_vulkan_temporal_rows,
        .environment_volumetric_cloud_vulkan_shadow_rows = impl_->environment_volumetric_cloud_vulkan_shadow_rows,
        .environment_volumetric_cloud_vulkan_storm_rows = impl_->environment_volumetric_cloud_vulkan_storm_rows,
        .environment_volumetric_cloud_vulkan_shader_contract_rows =
            impl_->environment_volumetric_cloud_vulkan_shader_contract_rows,
        .environment_volumetric_cloud_vulkan_quality_rows = impl_->environment_volumetric_cloud_vulkan_quality_rows,
        .environment_volumetric_cloud_vulkan_policy_diagnostics_count =
            impl_->environment_volumetric_cloud_vulkan_policy_diagnostics_count,
        .environment_volumetric_cloud_requested = impl_->environment_volumetric_cloud_requested,
        .environment_volumetric_cloud_shader_contract_evidence_ready =
            impl_->environment_volumetric_cloud_shader_contract_evidence_ready,
        .environment_volumetric_cloud_package_evidence_ready =
            impl_->environment_volumetric_cloud_package_evidence_ready,
        .environment_volumetric_cloud_execution_evidence_ready =
            impl_->environment_volumetric_cloud_execution_evidence_ready,
        .environment_volumetric_cloud_weather_map_ready = impl_->environment_volumetric_cloud_weather_map_ready,
        .environment_volumetric_cloud_shape_noise_ready = impl_->environment_volumetric_cloud_shape_noise_ready,
        .environment_volumetric_cloud_erosion_noise_ready = impl_->environment_volumetric_cloud_erosion_noise_ready,
        .environment_volumetric_cloud_uploads_volume_textures =
            impl_->environment_volumetric_cloud_uploads_volume_textures,
        .environment_volumetric_cloud_invokes_backend = impl_->environment_volumetric_cloud_invokes_backend,
        .environment_volumetric_cloud_renderer_draws = impl_->environment_volumetric_cloud_renderer_draws,
        .environment_volumetric_cloud_raymarch_passes = impl_->environment_volumetric_cloud_raymarch_passes,
        .environment_volumetric_cloud_readback_nonzero = impl_->environment_volumetric_cloud_readback_nonzero,
        .environment_volumetric_cloud_exposes_native_handles =
            impl_->environment_volumetric_cloud_exposes_native_handles,
        .environment_volumetric_cloud_plays_audio = impl_->environment_volumetric_cloud_plays_audio,
        .environment_volumetric_cloud_executes_precipitation =
            impl_->environment_volumetric_cloud_executes_precipitation,
        .environment_volumetric_cloud_map_rows = impl_->environment_volumetric_cloud_map_rows,
        .environment_volumetric_cloud_layer_rows = impl_->environment_volumetric_cloud_layer_rows,
        .environment_volumetric_cloud_lighting_rows = impl_->environment_volumetric_cloud_lighting_rows,
        .environment_volumetric_cloud_raymarch_rows = impl_->environment_volumetric_cloud_raymarch_rows,
        .environment_volumetric_cloud_temporal_rows = impl_->environment_volumetric_cloud_temporal_rows,
        .environment_volumetric_cloud_shadow_rows = impl_->environment_volumetric_cloud_shadow_rows,
        .environment_volumetric_cloud_storm_rows = impl_->environment_volumetric_cloud_storm_rows,
        .environment_volumetric_cloud_shader_contract_rows = impl_->environment_volumetric_cloud_shader_contract_rows,
        .environment_volumetric_cloud_quality_rows = impl_->environment_volumetric_cloud_quality_rows,
        .environment_volumetric_cloud_policy_diagnostics_count =
            impl_->environment_volumetric_cloud_policy_diagnostics_count,
        .directional_shadow_status = impl_->directional_shadow_status,
        .directional_shadow_requested = impl_->directional_shadow_requested,
        .directional_shadow_ready = impl_->directional_shadow_ready,
        .directional_shadow_filter_mode = impl_->directional_shadow_filter_mode,
        .directional_shadow_filter_tap_count = impl_->directional_shadow_filter_tap_count,
        .directional_shadow_filter_radius_texels = impl_->directional_shadow_filter_radius_texels,
        .directional_shadow_cascade_count = impl_->directional_shadow_cascade_count,
        .directional_shadow_cascade_tile_width = impl_->directional_shadow_cascade_tile_width,
        .directional_shadow_atlas_width = impl_->directional_shadow_atlas_width,
        .directional_shadow_atlas_height = impl_->directional_shadow_atlas_height,
        .directional_shadow_light_space_cascades = impl_->directional_shadow_light_space_cascades,
        .directional_shadow_cascade_splits = impl_->directional_shadow_cascade_splits,
        .native_ui_overlay_status = impl_->native_ui_overlay_status,
        .native_ui_overlay_requested = impl_->native_ui_overlay_requested,
        .native_ui_overlay_ready = impl_->native_ui_overlay_ready,
        .native_ui_overlay_sprites_submitted = renderer_stats.native_ui_overlay_sprites_submitted,
        .native_ui_overlay_draws = renderer_stats.native_ui_overlay_draws,
        .native_ui_texture_overlay_status = impl_->native_ui_texture_overlay_status,
        .native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested,
        .native_ui_texture_overlay_atlas_ready = impl_->native_ui_texture_overlay_atlas_ready,
        .native_ui_texture_overlay_sprites_submitted = renderer_stats.native_ui_overlay_textured_sprites_submitted,
        .native_ui_texture_overlay_texture_binds = renderer_stats.native_ui_overlay_texture_binds,
        .native_ui_texture_overlay_draws = renderer_stats.native_ui_overlay_textured_draws,
        .rhi_instanced_draw_calls = rhi_stats.instanced_draw_calls,
        .rhi_instanced_indexed_draw_calls = rhi_stats.instanced_indexed_draw_calls,
        .rhi_instanced_instances_submitted = rhi_stats.instanced_instances_submitted,
        .rhi_memory_diagnostics =
            impl_->device != nullptr ? impl_->device->memory_diagnostics() : rhi::RhiDeviceMemoryDiagnostics{},
        .rhi_transient_heap_allocations = rhi_stats.transient_texture_heap_allocations,
        .rhi_transient_placed_allocations = rhi_stats.transient_texture_placed_allocations,
        .rhi_transient_placed_resources_alive = rhi_stats.transient_texture_placed_resources_alive,
        .rhi_bytes_written = rhi_stats.bytes_written,
        .rhi_gpu_timestamp_ticks_per_second =
            impl_->device != nullptr ? impl_->device->gpu_timestamp_ticks_per_second() : 0,
        .rhi_gpu_debug_scopes_begun = rhi_stats.gpu_debug_scopes_begun,
        .rhi_gpu_debug_scopes_ended = rhi_stats.gpu_debug_scopes_ended,
        .rhi_gpu_debug_markers_inserted = rhi_stats.gpu_debug_markers_inserted,
        .framegraph_passes = impl_->framegraph_passes,
        .renderer_stats = renderer_stats,
        .rhi_stats = rhi_stats,
        .backbuffer_extent = impl_->renderer != nullptr ? impl_->renderer->backbuffer_extent() : Extent2D{},
        .diagnostics_count = impl_->diagnostics.size(),
        .backend_reports_count = impl_->backend_reports.size(),
        .scene_gpu_diagnostics_count = impl_->scene_gpu_diagnostics.size(),
        .postprocess_diagnostics_count = impl_->postprocess_diagnostics.size(),
        .directional_shadow_diagnostics_count = impl_->directional_shadow_diagnostics.size(),
        .native_ui_overlay_diagnostics_count = impl_->native_ui_overlay_diagnostics.size(),
        .native_ui_texture_overlay_diagnostics_count = impl_->native_ui_texture_overlay_diagnostics.size(),
    };
}

std::span<const Win32DesktopPresentationBackendReport> Win32DesktopPresentation::backend_reports() const noexcept {
    return impl_->backend_reports;
}

std::span<const Win32DesktopPresentationDiagnostic> Win32DesktopPresentation::diagnostics() const noexcept {
    return impl_->diagnostics;
}

Win32DesktopPresentationSceneGpuBindingStatus Win32DesktopPresentation::scene_gpu_binding_status() const noexcept {
    return impl_->scene_gpu_status;
}

bool Win32DesktopPresentation::scene_gpu_bindings_ready() const noexcept {
    return impl_->scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready;
}

Win32DesktopPresentationSceneGpuBindingStats Win32DesktopPresentation::scene_gpu_binding_stats() const noexcept {
    if (impl_->scene_gpu_renderer != nullptr) {
        return impl_->scene_gpu_renderer->scene_gpu_binding_stats();
    }
    return {};
}

rhi::BufferHandle Win32DesktopPresentation::scene_pbr_frame_uniform_buffer() const noexcept {
    if (impl_->scene_gpu_renderer == nullptr) {
        return {};
    }
    return impl_->scene_gpu_renderer->scene_pbr_frame_uniform_buffer();
}

rhi::IRhiDevice* Win32DesktopPresentation::scene_pbr_frame_rhi_device() noexcept {
    return impl_->device.get();
}

const rhi::IRhiDevice* Win32DesktopPresentation::scene_pbr_frame_rhi_device() const noexcept {
    return impl_->device.get();
}

std::span<const Win32DesktopPresentationSceneGpuBindingDiagnostic>
Win32DesktopPresentation::scene_gpu_binding_diagnostics() const noexcept {
    return impl_->scene_gpu_diagnostics;
}

Win32DesktopPresentationPostprocessStatus Win32DesktopPresentation::postprocess_status() const noexcept {
    return impl_->postprocess_status;
}

bool Win32DesktopPresentation::postprocess_ready() const noexcept {
    return impl_->postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
}

bool Win32DesktopPresentation::postprocess_depth_input_ready() const noexcept {
    return impl_->postprocess_depth_input_ready;
}

std::span<const Win32DesktopPresentationPostprocessDiagnostic>
Win32DesktopPresentation::postprocess_diagnostics() const noexcept {
    return impl_->postprocess_diagnostics;
}

Win32DesktopPresentationDirectionalShadowStatus Win32DesktopPresentation::directional_shadow_status() const noexcept {
    return impl_->directional_shadow_status;
}

bool Win32DesktopPresentation::directional_shadow_ready() const noexcept {
    return impl_->directional_shadow_ready;
}

std::span<const Win32DesktopPresentationDirectionalShadowDiagnostic>
Win32DesktopPresentation::directional_shadow_diagnostics() const noexcept {
    return impl_->directional_shadow_diagnostics;
}

Win32DesktopPresentationNativeUiOverlayStatus Win32DesktopPresentation::native_ui_overlay_status() const noexcept {
    return impl_->native_ui_overlay_status;
}

bool Win32DesktopPresentation::native_ui_overlay_ready() const noexcept {
    return impl_->native_ui_overlay_ready;
}

std::span<const Win32DesktopPresentationNativeUiOverlayDiagnostic>
Win32DesktopPresentation::native_ui_overlay_diagnostics() const noexcept {
    return impl_->native_ui_overlay_diagnostics;
}

Win32DesktopPresentationNativeUiTextureOverlayStatus
Win32DesktopPresentation::native_ui_texture_overlay_status() const noexcept {
    return impl_->native_ui_texture_overlay_status;
}

bool Win32DesktopPresentation::native_ui_texture_overlay_atlas_ready() const noexcept {
    return impl_->native_ui_texture_overlay_atlas_ready;
}

std::span<const Win32DesktopPresentationNativeUiTextureOverlayDiagnostic>
Win32DesktopPresentation::native_ui_texture_overlay_diagnostics() const noexcept {
    return impl_->native_ui_texture_overlay_diagnostics;
}

Win32D3d12SwapChainPlan plan_win32_d3d12_swapchain(const Win32D3d12SwapChainPlanDesc& desc) {
    Win32D3d12SwapChainPlan plan{
        .extent = desc.extent,
        .format = desc.format,
        .buffer_count = desc.buffer_count,
        .uses_create_swap_chain_for_hwnd = true,
        .uses_direct_command_queue = true,
        .flip_discard_swap_effect = true,
        .render_target_output = true,
        .resize_buffers_supported = true,
        .requires_present_state_before_present = true,
        .allow_tearing_flag = false,
        .present_sync_interval = desc.vsync ? 1U : 0U,
        .public_native_handles_exposed = false,
    };

    if (!has_extent(desc.extent)) {
        plan.diagnostic = "win32 d3d12 swapchain extent must be non-zero";
        return plan;
    }
    if (!valid_swapchain_format(desc.format)) {
        plan.diagnostic = "win32 d3d12 swapchain format must be rgba8_unorm or bgra8_unorm";
        return plan;
    }
    if (desc.buffer_count < 2) {
        plan.diagnostic = "win32 d3d12 swapchain buffer count must be at least 2 for flip model";
        return plan;
    }

    plan.allow_tearing_flag = desc.request_tearing && desc.tearing_supported && !desc.vsync;
    return plan;
}

std::string_view win32_desktop_presentation_backend_name(Win32DesktopPresentationBackend backend) noexcept {
    switch (backend) {
    case Win32DesktopPresentationBackend::null_renderer:
        return "null";
    case Win32DesktopPresentationBackend::d3d12:
        return "d3d12";
    case Win32DesktopPresentationBackend::vulkan:
        return "vulkan";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_backend_report_status_name(Win32DesktopPresentationBackendReportStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationBackendReportStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationBackendReportStatus::missing_request:
        return "missing_request";
    case Win32DesktopPresentationBackendReportStatus::native_window_unavailable:
        return "native_window_unavailable";
    case Win32DesktopPresentationBackendReportStatus::native_backend_unavailable:
        return "native_backend_unavailable";
    case Win32DesktopPresentationBackendReportStatus::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    case Win32DesktopPresentationBackendReportStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_fallback_reason_name(Win32DesktopPresentationFallbackReason reason) noexcept {
    switch (reason) {
    case Win32DesktopPresentationFallbackReason::none:
        return "none";
    case Win32DesktopPresentationFallbackReason::native_window_unavailable:
        return "native_window_unavailable";
    case Win32DesktopPresentationFallbackReason::native_backend_unavailable:
        return "native_backend_unavailable";
    case Win32DesktopPresentationFallbackReason::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_present_status_name(Win32DesktopPresentationPresentStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationPresentStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationPresentStatus::ready:
        return "ready";
    case Win32DesktopPresentationPresentStatus::failed:
        return "failed";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_resize_status_name(Win32DesktopPresentationResizeStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationResizeStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationResizeStatus::ready:
        return "ready";
    case Win32DesktopPresentationResizeStatus::failed:
        return "failed";
    case Win32DesktopPresentationResizeStatus::recreate_required:
        return "recreate_required";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_scene_gpu_binding_status_name(
    Win32DesktopPresentationSceneGpuBindingStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationSceneGpuBindingStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationSceneGpuBindingStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationSceneGpuBindingStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationSceneGpuBindingStatus::failed:
        return "failed";
    case Win32DesktopPresentationSceneGpuBindingStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_postprocess_status_name(Win32DesktopPresentationPostprocessStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationPostprocessStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationPostprocessStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationPostprocessStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationPostprocessStatus::failed:
        return "failed";
    case Win32DesktopPresentationPostprocessStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_directional_shadow_status_name(
    Win32DesktopPresentationDirectionalShadowStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationDirectionalShadowStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationDirectionalShadowStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationDirectionalShadowStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationDirectionalShadowStatus::failed:
        return "failed";
    case Win32DesktopPresentationDirectionalShadowStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_directional_shadow_filter_mode_name(
    Win32DesktopPresentationDirectionalShadowFilterMode mode) noexcept {
    switch (mode) {
    case Win32DesktopPresentationDirectionalShadowFilterMode::none:
        return "none";
    case Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3:
        return "fixed_pcf_3x3";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_native_ui_overlay_status_name(
    Win32DesktopPresentationNativeUiOverlayStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationNativeUiOverlayStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationNativeUiOverlayStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationNativeUiOverlayStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationNativeUiOverlayStatus::failed:
        return "failed";
    case Win32DesktopPresentationNativeUiOverlayStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_native_ui_texture_overlay_status_name(
    Win32DesktopPresentationNativeUiTextureOverlayStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationNativeUiTextureOverlayStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationNativeUiTextureOverlayStatus::unavailable:
        return "unavailable";
    case Win32DesktopPresentationNativeUiTextureOverlayStatus::invalid_request:
        return "invalid_request";
    case Win32DesktopPresentationNativeUiTextureOverlayStatus::failed:
        return "failed";
    case Win32DesktopPresentationNativeUiTextureOverlayStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_quality_gate_status_name(Win32DesktopPresentationQualityGateStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationQualityGateStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationQualityGateStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationQualityGateStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_postprocess_policy_status_name(
    Win32DesktopPresentationPostprocessPolicyStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationPostprocessPolicyStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationPostprocessPolicyStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationPostprocessPolicyStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_scene_scale_policy_status_name(
    Win32DesktopPresentationSceneScalePolicyStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationSceneScalePolicyStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationSceneScalePolicyStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationSceneScalePolicyStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_d3d12_postprocess_execution_status_name(
    Win32DesktopPresentationD3d12PostprocessExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationD3d12PostprocessExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationD3d12PostprocessExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationD3d12PostprocessExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_environment_fog_status_name(Win32DesktopPresentationEnvironmentFogStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationEnvironmentFogStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationEnvironmentFogStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationEnvironmentFogStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_environment_fog_package_status_name(
    const Win32DesktopPresentationVulkanEnvironmentFogPackageStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_physical_sky_status_name(Win32DesktopPresentationPhysicalSkyStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationPhysicalSkyStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationPhysicalSkyStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationPhysicalSkyStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_physical_sky_package_status_name(
    const Win32DesktopPresentationVulkanPhysicalSkyPackageStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
win32_desktop_presentation_cloud_layer_status_name(Win32DesktopPresentationCloudLayerStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationCloudLayerStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationCloudLayerStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationCloudLayerStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_environment_precipitation_status_name(
    Win32DesktopPresentationEnvironmentPrecipitationStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationEnvironmentPrecipitationStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationEnvironmentPrecipitationStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationEnvironmentPrecipitationStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_environment_precipitation_status_name(
    Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_environment_volumetric_fog_status_name(
    const Win32DesktopPresentationEnvironmentVolumetricFogStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationEnvironmentVolumetricFogStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationEnvironmentVolumetricFogStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationEnvironmentVolumetricFogStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_environment_volumetric_fog_status_name(
    const Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_environment_volumetric_cloud_status_name(
    const Win32DesktopPresentationEnvironmentVolumetricCloudStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationEnvironmentVolumetricCloudStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationEnvironmentVolumetricCloudStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationEnvironmentVolumetricCloudStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_environment_volumetric_cloud_status_name(
    const Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_environment_ibl_renderer_execution_status_name(
    const Win32DesktopPresentationEnvironmentIblRendererExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_environment_ibl_renderer_execution_status_name(
    const Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::host_evidence_required:
        return "host_evidence_required";
    case Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_vulkan_postprocess_execution_status_name(
    Win32DesktopPresentationVulkanPostprocessExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanPostprocessExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanPostprocessExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanPostprocessExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_d3d12_instanced_draw_execution_status_name(
    Win32DesktopPresentationD3d12InstancedDrawExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationPostprocessPolicyReport
evaluate_win32_desktop_presentation_postprocess_policy(const Win32DesktopPresentationReport& report) {
    Win32DesktopPresentationPostprocessPolicyReport result;
    if (!postprocess_policy_requested(report)) {
        return result;
    }

    const bool postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
    const auto postprocess_effect =
        report.environment_fog_requested ? PostprocessEffectKind::fog : PostprocessEffectKind::color_grading;
    const std::array effects{
        PostprocessEffectDesc{
            .kind = postprocess_effect,
            .requires_scene_depth = report.postprocess_depth_input_requested || report.environment_fog_requested,
        },
    };
    const auto plan = plan_postprocess_chain_policy(PostprocessChainPolicyDesc{
        .effects = effects,
        .frame_extent = report.backbuffer_extent,
        .scene_color_available = postprocess_ready,
        .scene_depth_available = !report.postprocess_depth_input_requested || report.postprocess_depth_input_ready,
        .max_effect_count = 1,
        .max_postprocess_pass_count = 1,
        .backend = postprocess_policy_backend(report.selected_backend),
        .require_backend_shader_evidence = true,
        .backend_shader_evidence_ready = postprocess_ready,
    });

    result.status = plan.succeeded() ? Win32DesktopPresentationPostprocessPolicyStatus::ready
                                     : Win32DesktopPresentationPostprocessPolicyStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationPostprocessPolicyStatus::ready;
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    result.effect_count = plan.effect_count;
    result.postprocess_pass_count = plan.postprocess_pass_count;
    result.framegraph_pass_count = plan.frame_graph_pass_count;
    result.framegraph_barrier_step_budget = plan.frame_graph_barrier_step_budget;
    result.scene_color_required = plan.scene_color_required;
    result.scene_depth_required = plan.scene_depth_required;
    result.bloom_work_texture_required = plan.bloom_work_texture_required;
    result.tone_mapping_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::tone_mapping);
    result.exposure_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::exposure);
    result.bloom_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::bloom);
    result.color_grading_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::color_grading);
    result.fog_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::fog);
    result.anti_aliasing_effect = has_postprocess_chain_policy_effect(plan, PostprocessEffectKind::anti_aliasing);
    result.backend_shader_evidence_ready = plan.backend_shader_evidence_ready;
    return result;
}

Win32DesktopPresentationSceneScalePolicyReport
evaluate_win32_desktop_presentation_scene_scale_policy(const Win32DesktopPresentationReport& report,
                                                       const Win32DesktopPresentationSceneScalePolicyDesc& desc) {
    Win32DesktopPresentationSceneScalePolicyReport result;
    result.backend_instancing_evidence_required = desc.require_backend_instancing_evidence;
    result.performance_measurement_required = desc.require_performance_measurement;
    result.expected_frames = desc.expected_frames;
    result.frames_finished = report.renderer_stats.frames_finished;
    result.frames_current = desc.expected_frames == 0 || report.renderer_stats.frames_finished == desc.expected_frames;
    if (!scene_scale_policy_requested(desc)) {
        return result;
    }

    result.scene_resources_ready = !desc.require_scene_gpu_bindings ||
                                   report.scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready;

    std::array<SceneScaleDrawGroupDesc, 4> draw_groups{};
    std::size_t draw_group_count = 0;
    auto add_draw_group = [&](SceneScaleDrawGroupKind kind, std::uint64_t instances) {
        if (instances == 0) {
            return;
        }
        draw_groups[draw_group_count] = SceneScaleDrawGroupDesc{
            .kind = kind,
            .instance_count = scene_scale_policy_count(instances),
            .visible_instance_count = scene_scale_policy_count(instances),
            .culling = SceneScaleCullingMode::none,
            .batching = SceneScaleBatchingMode::none,
            .lod = SceneScaleLodMode::none,
            .lod_count = 1,
            .scene_resources_available = result.scene_resources_ready,
            .source_index = static_cast<std::uint32_t>(draw_group_count),
        };
        ++draw_group_count;
    };

    const auto deformed_mesh_draws = report.renderer_stats.gpu_skinning_draws + report.renderer_stats.gpu_morph_draws;
    const auto static_mesh_draws = report.renderer_stats.meshes_submitted > deformed_mesh_draws
                                       ? report.renderer_stats.meshes_submitted - deformed_mesh_draws
                                       : 0U;
    add_draw_group(SceneScaleDrawGroupKind::static_mesh, static_mesh_draws);
    add_draw_group(SceneScaleDrawGroupKind::skinned_mesh, report.renderer_stats.gpu_skinning_draws);
    add_draw_group(SceneScaleDrawGroupKind::morph_mesh, report.renderer_stats.gpu_morph_draws);
    add_draw_group(SceneScaleDrawGroupKind::sprite, report.renderer_stats.sprites_submitted);

    const auto plan = plan_scene_scale_policy(SceneScalePolicyDesc{
        .draw_groups = std::span<const SceneScaleDrawGroupDesc>{draw_groups.data(), draw_group_count},
        .frame_extent = report.backbuffer_extent,
        .max_draw_group_count = desc.max_draw_group_count,
        .max_visible_instance_count = desc.max_visible_instance_count,
        .max_draw_call_count = desc.max_draw_call_count,
        .backend = postprocess_policy_backend(report.selected_backend),
        .require_backend_instancing_evidence = desc.require_backend_instancing_evidence,
        .backend_instancing_evidence_ready = desc.backend_instancing_evidence_ready,
        .require_performance_measurement = desc.require_performance_measurement,
        .performance_measurement_ready = desc.performance_measurement_ready,
    });

    result.backend_instancing_evidence_ready = plan.backend_instancing_evidence_ready;
    result.performance_measurement_ready = plan.performance_measurement_ready;
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!result.frames_current) {
        ++result.diagnostics_count;
    }
    result.draw_group_count = plan.draw_group_count;
    result.requested_instance_count = plan.requested_instance_count;
    result.visible_instance_count = plan.visible_instance_count;
    result.culled_instance_count = plan.culled_instance_count;
    result.draw_call_count = plan.draw_call_count;
    result.instanced_draw_call_count = plan.instanced_draw_call_count;
    result.instanced_visible_instance_count = plan.instanced_visible_instance_count;
    result.lod_group_count = plan.lod_group_count;
    result.cpu_culling_group_count = plan.cpu_culling_group_count;
    for (const auto& row : plan.draw_group_rows) {
        switch (row.kind) {
        case SceneScaleDrawGroupKind::static_mesh:
            ++result.static_mesh_draw_groups;
            break;
        case SceneScaleDrawGroupKind::skinned_mesh:
            ++result.skinned_mesh_draw_groups;
            break;
        case SceneScaleDrawGroupKind::morph_mesh:
            ++result.morph_mesh_draw_groups;
            break;
        case SceneScaleDrawGroupKind::sprite:
            ++result.sprite_draw_groups;
            break;
        case SceneScaleDrawGroupKind::unknown:
            break;
        }
    }
    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationSceneScalePolicyStatus::ready
                                 : Win32DesktopPresentationSceneScalePolicyStatus::blocked;
    return result;
}

Win32DesktopPresentationD3d12PostprocessExecutionReport evaluate_win32_desktop_presentation_d3d12_postprocess_execution(
    const Win32DesktopPresentationReport& report, std::uint64_t expected_postprocess_passes, const bool requested) {
    Win32DesktopPresentationD3d12PostprocessExecutionReport result;
    if (!requested) {
        return result;
    }
    if (!postprocess_policy_requested(report)) {
        return result;
    }

    const auto policy = evaluate_win32_desktop_presentation_postprocess_policy(report);
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
    result.backend_shader_evidence_ready = policy.backend_shader_evidence_ready;
    result.expected_postprocess_passes = expected_postprocess_passes;
    result.postprocess_passes_executed = report.renderer_stats.postprocess_passes_executed;
    result.framegraph_passes_executed = report.renderer_stats.framegraph_passes_executed;
    result.framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded;
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    result.postprocess_passes_current =
        report.renderer_stats.postprocess_passes_executed == expected_postprocess_passes;
    result.status = result.d3d12_backend_selected && result.postprocess_ready && result.backend_shader_evidence_ready &&
                            result.postprocess_passes_current
                        ? Win32DesktopPresentationD3d12PostprocessExecutionStatus::ready
                        : Win32DesktopPresentationD3d12PostprocessExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationD3d12PostprocessExecutionStatus::ready;
    return result;
}

Win32DesktopPresentationEnvironmentFogReport evaluate_win32_desktop_presentation_environment_fog(
    const Win32DesktopPresentationReport& report,
    const Win32DesktopPresentationD3d12PostprocessExecutionReport& d3d12_postprocess_execution, const bool requested) {
    Win32DesktopPresentationEnvironmentFogReport result;
    if (!requested) {
        return result;
    }

    result.requested = report.environment_fog_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
    result.postprocess_depth_input_ready = report.postprocess_depth_input_ready;
    result.d3d12_postprocess_execution_ready = d3d12_postprocess_execution.ready;
    result.constant_buffer_ready = report.environment_fog_constant_buffer_ready;
    result.constants_binding = environment_fog_constants_binding();
    result.constant_buffer_bytes = report.environment_fog_constant_buffer_bytes;
    result.expected_postprocess_passes = d3d12_postprocess_execution.expected_postprocess_passes;
    result.postprocess_passes_executed = d3d12_postprocess_execution.postprocess_passes_executed;
    result.postprocess_passes_current = d3d12_postprocess_execution.postprocess_passes_current;

    const auto plan = plan_environment_fog_policy(EnvironmentFogPolicyDesc{
        .mode = EnvironmentFogMode::exponential_height,
        .density = 0.08F,
        .height_falloff = 0.35F,
        .height_offset_m = 12.0F,
        .start_distance_m = 0.0F,
        .cutoff_distance_m = 1200.0F,
        .max_opacity = 0.85F,
        .sky_affect = 0.35F,
        .directional_inscattering_anisotropy = 0.25F,
        .inscattering_color = Vec3{.x = 0.58F, .y = 0.68F, .z = 0.78F},
        .directional_inscattering_color = Vec3{.x = 0.84F, .y = 0.78F, .z = 0.62F},
        .sample_step_budget = 8,
        .scene_depth_available = result.postprocess_depth_input_ready,
        .shader_contract_evidence_ready = result.d3d12_postprocess_execution_ready,
    });
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.postprocess_ready) {
        ++result.diagnostics_count;
    }
    if (!result.constant_buffer_ready ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(environment_fog_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (!result.postprocess_passes_current) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationEnvironmentFogStatus::ready
                                 : Win32DesktopPresentationEnvironmentFogStatus::blocked;
    return result;
}

Win32DesktopPresentationVulkanEnvironmentFogPackageReport
evaluate_win32_desktop_presentation_vulkan_environment_fog_package(
    const Win32DesktopPresentationReport& report,
    const Win32DesktopPresentationVulkanPostprocessExecutionReport& vulkan_postprocess_execution,
    const bool requested) {
    Win32DesktopPresentationVulkanEnvironmentFogPackageReport result;
    result.constants_binding = environment_fog_constants_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_fog_vulkan_package_requested;
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
    result.postprocess_depth_input_ready = report.postprocess_depth_input_ready;
    result.vulkan_postprocess_execution_ready = vulkan_postprocess_execution.ready;
    result.shader_contract_evidence_ready = report.environment_fog_vulkan_package_shader_contract_evidence_ready &&
                                            vulkan_postprocess_execution.backend_shader_evidence_ready;
    result.package_evidence_ready = report.environment_fog_vulkan_package_evidence_ready;
    result.constant_buffer_ready = report.environment_fog_vulkan_package_constant_buffer_ready;
    result.constant_buffer_bytes = report.environment_fog_vulkan_package_constant_buffer_bytes;
    result.expected_postprocess_passes = vulkan_postprocess_execution.expected_postprocess_passes;
    result.postprocess_passes_executed = vulkan_postprocess_execution.postprocess_passes_executed;
    result.postprocess_passes_current = vulkan_postprocess_execution.postprocess_passes_current;

    const auto plan = plan_environment_fog_policy(EnvironmentFogPolicyDesc{
        .mode = EnvironmentFogMode::exponential_height,
        .density = 0.08F,
        .height_falloff = 0.35F,
        .height_offset_m = 12.0F,
        .start_distance_m = 0.0F,
        .cutoff_distance_m = 1200.0F,
        .max_opacity = 0.85F,
        .sky_affect = 0.35F,
        .directional_inscattering_anisotropy = 0.25F,
        .inscattering_color = Vec3{.x = 0.58F, .y = 0.68F, .z = 0.78F},
        .directional_inscattering_color = Vec3{.x = 0.84F, .y = 0.78F, .z = 0.62F},
        .sample_step_budget = 8,
        .scene_depth_available = result.postprocess_depth_input_ready,
        .shader_contract_evidence_ready = result.shader_contract_evidence_ready,
        .execution_evidence_ready = result.vulkan_postprocess_execution_ready,
        .package_evidence_ready = result.package_evidence_ready,
        .request_ready_promotion = true,
    });
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.vulkan_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.postprocess_ready) {
        ++result.diagnostics_count;
    }
    if (!result.constant_buffer_ready ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(environment_fog_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (!result.postprocess_passes_current) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles) {
        ++result.diagnostics_count;
    }

    if (!result.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::host_evidence_required;
    } else {
        result.ready = result.diagnostics_count == 0;
        result.status = result.ready ? Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::ready
                                     : Win32DesktopPresentationVulkanEnvironmentFogPackageStatus::blocked;
    }
    return result;
}

Win32DesktopPresentationPhysicalSkyReport
evaluate_win32_desktop_presentation_physical_sky(const Win32DesktopPresentationReport& report, const bool requested) {
    Win32DesktopPresentationPhysicalSkyReport result;
    result.constants_binding = physical_sky_constants_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.physical_sky_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.shader_contract_evidence_ready = report.physical_sky_shader_contract_evidence_ready;
    result.package_evidence_ready = report.physical_sky_package_evidence_ready;
    result.execution_evidence_ready = report.physical_sky_execution_evidence_ready;
    result.constant_buffer_ready = report.physical_sky_constant_buffer_ready;
    result.constant_buffer_bytes = report.physical_sky_constant_buffer_bytes;
    result.allocates_lut_textures = report.physical_sky_allocates_lut_textures;
    result.invokes_backend = report.physical_sky_invokes_backend;
    result.exposes_native_handles = report.physical_sky_exposes_native_handles;
    result.constant_layout_rows = report.physical_sky_constant_layout_rows;
    result.lut_intent_rows = report.physical_sky_lut_intent_rows;

    const auto expected_plan = plan_physical_sky_policy(sample_physical_sky_policy_desc());
    result.diagnostics_count =
        static_cast<std::uint32_t>(expected_plan.diagnostics.size()) + report.physical_sky_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.constant_buffer_ready ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(physical_sky_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.constant_layout_rows != static_cast<std::uint32_t>(expected_plan.constant_layout_rows.size()) ||
        result.lut_intent_rows != static_cast<std::uint32_t>(expected_plan.lut_intent_rows.size())) {
        ++result.diagnostics_count;
    }
    if (result.allocates_lut_textures || result.invokes_backend || result.exposes_native_handles) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationPhysicalSkyStatus::ready
                                 : Win32DesktopPresentationPhysicalSkyStatus::blocked;
    return result;
}

Win32DesktopPresentationVulkanPhysicalSkyPackageReport
evaluate_win32_desktop_presentation_vulkan_physical_sky_package(const Win32DesktopPresentationReport& report,
                                                                const bool requested) {
    Win32DesktopPresentationVulkanPhysicalSkyPackageReport result;
    result.constants_binding = physical_sky_constants_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.physical_sky_vulkan_package_requested;
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.shader_contract_evidence_ready = report.physical_sky_vulkan_package_shader_contract_evidence_ready;
    result.package_evidence_ready = report.physical_sky_vulkan_package_evidence_ready;
    result.execution_evidence_ready = report.physical_sky_vulkan_package_execution_evidence_ready;
    result.constant_buffer_ready = report.physical_sky_vulkan_package_constant_buffer_ready;
    result.constant_buffer_bytes = report.physical_sky_vulkan_package_constant_buffer_bytes;
    result.allocates_lut_textures = report.physical_sky_vulkan_package_allocates_lut_textures;
    result.invokes_backend = report.physical_sky_vulkan_package_invokes_backend;
    result.exposes_native_handles = report.physical_sky_vulkan_package_exposes_native_handles;
    result.constant_layout_rows = report.physical_sky_vulkan_package_constant_layout_rows;
    result.lut_intent_rows = report.physical_sky_vulkan_package_lut_intent_rows;

    const auto expected_plan = plan_physical_sky_policy(sample_physical_sky_policy_desc());
    result.diagnostics_count = static_cast<std::uint32_t>(expected_plan.diagnostics.size()) +
                               report.physical_sky_vulkan_package_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.vulkan_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.constant_buffer_ready ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(physical_sky_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.constant_layout_rows != static_cast<std::uint32_t>(expected_plan.constant_layout_rows.size()) ||
        result.lut_intent_rows != static_cast<std::uint32_t>(expected_plan.lut_intent_rows.size())) {
        ++result.diagnostics_count;
    }
    if (result.allocates_lut_textures || result.invokes_backend || result.exposes_native_handles) {
        ++result.diagnostics_count;
    }

    if (!result.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::host_evidence_required;
    } else {
        result.ready = result.diagnostics_count == 0;
        result.status = result.ready ? Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::ready
                                     : Win32DesktopPresentationVulkanPhysicalSkyPackageStatus::blocked;
    }
    return result;
}

Win32DesktopPresentationCloudLayerReport
evaluate_win32_desktop_presentation_cloud_layer(const Win32DesktopPresentationReport& report, const bool requested,
                                                const bool require_renderer_execution) {
    Win32DesktopPresentationCloudLayerReport result;
    result.cloud_map_binding = cloud_layer_cloud_map_binding();
    result.flow_map_binding = cloud_layer_flow_map_binding();
    result.sampler_binding = cloud_layer_sampler_binding();
    result.constants_binding = cloud_layer_constants_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.cloud_layer_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.shader_contract_evidence_ready = report.cloud_layer_shader_contract_evidence_ready;
    result.package_evidence_ready = report.cloud_layer_package_evidence_ready;
    result.execution_evidence_ready = report.cloud_layer_execution_evidence_ready;
    result.uploads_textures = report.cloud_layer_uploads_textures;
    result.invokes_backend = report.cloud_layer_invokes_backend;
    result.renderer_draws = report.cloud_layer_renderer_draws;
    result.exposes_native_handles = report.cloud_layer_exposes_native_handles;
    result.uses_volumetric_clouds = report.cloud_layer_uses_volumetric_clouds;

    result.uses_latlong_projection = report.cloud_layer_uses_latlong_projection;
    result.uses_flow_map = report.cloud_layer_uses_flow_map;
    result.texture_rows = report.cloud_layer_texture_rows;
    result.visual_rows = report.cloud_layer_visual_rows;
    result.ibl_rows = report.cloud_layer_ibl_rows;
    result.shader_contract_rows = report.cloud_layer_shader_contract_rows;
    result.quality_rows = report.cloud_layer_quality_rows;

    const auto expected_plan = plan_cloud_layer_policy(CloudLayerPolicyDesc{
        .layer = sample_cloud_layer_package_desc(),
        .quality_tier = CloudLayerQualityTier::balanced,
        .shader_contract_evidence_ready = result.shader_contract_evidence_ready,
        .package_evidence_ready = result.package_evidence_ready,
        .execution_evidence_ready = result.execution_evidence_ready,
        .request_ready_promotion = true,
        .request_texture_upload = require_renderer_execution,
        .request_backend_execution = require_renderer_execution,
    });

    const auto expected_texture_rows = static_cast<std::uint32_t>(expected_plan.texture_rows.size());
    const auto expected_visual_rows = static_cast<std::uint32_t>(expected_plan.visual_rows.size());
    const auto expected_ibl_rows = static_cast<std::uint32_t>(expected_plan.ibl_rows.size());
    const auto expected_shader_contract_rows = static_cast<std::uint32_t>(expected_plan.shader_contract_rows.size());
    const auto expected_quality_rows = static_cast<std::uint32_t>(expected_plan.quality_rows.size());
    bool expected_uses_latlong_projection{false};
    bool expected_uses_flow_map{false};
    if (!expected_plan.shader_contract_rows.empty()) {
        expected_uses_latlong_projection = expected_plan.shader_contract_rows.front().uses_latlong_projection;
        expected_uses_flow_map = expected_plan.shader_contract_rows.front().uses_flow_map;
    }

    result.diagnostics_count =
        static_cast<std::uint32_t>(expected_plan.diagnostics.size()) + report.cloud_layer_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (result.texture_rows != expected_texture_rows || result.visual_rows != expected_visual_rows ||
        result.ibl_rows != expected_ibl_rows || result.shader_contract_rows != expected_shader_contract_rows ||
        result.quality_rows != expected_quality_rows) {
        ++result.diagnostics_count;
    }
    if (result.uses_latlong_projection != expected_uses_latlong_projection ||
        result.uses_flow_map != expected_uses_flow_map) {
        ++result.diagnostics_count;
    }
    if (require_renderer_execution) {
        if (!result.uploads_textures || !result.invokes_backend || result.renderer_draws == 0) {
            ++result.diagnostics_count;
        }
    } else if (result.uploads_textures || result.invokes_backend || result.renderer_draws != 0) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles || result.uses_volumetric_clouds) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationCloudLayerStatus::ready
                                 : Win32DesktopPresentationCloudLayerStatus::blocked;
    return result;
}

Win32DesktopPresentationEnvironmentPrecipitationReport evaluate_win32_desktop_presentation_environment_precipitation(
    const Win32DesktopPresentationReport& report, const bool requested,
    const Win32DesktopPresentationEnvironmentPrecipitationExpectation expectation) {
    Win32DesktopPresentationEnvironmentPrecipitationReport result;
    result.particle_texture_binding = precipitation_particle_texture_binding();
    result.scene_depth_texture_binding = precipitation_scene_depth_texture_binding();
    result.sampler_binding = precipitation_sampler_binding();
    result.constants_binding = precipitation_constants_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_precipitation_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.weather = report.environment_precipitation_weather;
    result.kind = report.environment_precipitation_kind;
    result.shader_contract_evidence_ready = report.environment_precipitation_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_precipitation_package_evidence_ready;
    result.execution_evidence_ready = report.environment_precipitation_execution_evidence_ready;
    result.uploads_particle_buffers = report.environment_precipitation_uploads_particle_buffers;
    result.invokes_backend = report.environment_precipitation_invokes_backend;
    result.renderer_draws = report.environment_precipitation_renderer_draws;
    result.depth_occlusion_readback = report.environment_precipitation_depth_occlusion_readback;
    result.exposes_native_handles = report.environment_precipitation_exposes_native_handles;
    result.mutates_materials = report.environment_precipitation_mutates_materials;
    result.plays_audio = report.environment_precipitation_plays_audio;
    result.uses_camera_near_particles = report.environment_precipitation_uses_camera_near_particles;
    result.uses_scene_depth_occlusion = report.environment_precipitation_uses_scene_depth_occlusion;
    result.weather_rows = report.environment_precipitation_weather_rows;
    result.particle_rows = report.environment_precipitation_particle_rows;
    result.occlusion_rows = report.environment_precipitation_occlusion_rows;
    result.wetness_rows = report.environment_precipitation_wetness_rows;
    result.audio_handoff_rows = report.environment_precipitation_audio_handoff_rows;
    result.shader_rows = report.environment_precipitation_shader_rows;
    result.quality_rows = report.environment_precipitation_quality_rows;

    result.diagnostics_count = report.environment_precipitation_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (result.weather != expectation.weather || result.kind != expectation.kind) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.uses_camera_near_particles || !result.uses_scene_depth_occlusion) {
        ++result.diagnostics_count;
    }
    if (result.weather_rows != 1 || result.particle_rows != 1 || result.occlusion_rows != 1 ||
        result.wetness_rows != expectation.wetness_rows ||
        result.audio_handoff_rows < expectation.minimum_audio_handoff_rows || result.shader_rows != 1 ||
        result.quality_rows != 1) {
        ++result.diagnostics_count;
    }
    if (expectation.require_renderer_execution) {
        if (!result.uploads_particle_buffers || !result.invokes_backend || result.renderer_draws == 0 ||
            !result.depth_occlusion_readback) {
            ++result.diagnostics_count;
        }
    } else if (result.uploads_particle_buffers || result.invokes_backend || result.renderer_draws != 0 ||
               result.depth_occlusion_readback) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles || result.mutates_materials || result.plays_audio) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationEnvironmentPrecipitationStatus::ready
                                 : Win32DesktopPresentationEnvironmentPrecipitationStatus::blocked;
    return result;
}

Win32DesktopPresentationVulkanEnvironmentPrecipitationReport
evaluate_win32_desktop_presentation_vulkan_environment_precipitation(
    const Win32DesktopPresentationReport& report, const bool requested,
    const Win32DesktopPresentationEnvironmentPrecipitationExpectation expectation) {
    Win32DesktopPresentationVulkanEnvironmentPrecipitationReport result;
    result.particle_texture_binding = precipitation_particle_texture_binding();
    result.scene_depth_texture_binding = precipitation_scene_depth_texture_binding();
    result.sampler_binding = precipitation_sampler_binding() + kVulkanPrecipitationSamplerBindingShift;
    result.constants_binding = precipitation_constants_binding() + kVulkanPrecipitationConstantsBindingShift;
    if (!requested) {
        return result;
    }

    result.requested = report.environment_precipitation_vulkan_requested;
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.weather = report.environment_precipitation_vulkan_weather;
    result.kind = report.environment_precipitation_vulkan_kind;
    result.shader_contract_evidence_ready = report.environment_precipitation_vulkan_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_precipitation_vulkan_package_evidence_ready;
    result.execution_evidence_ready = report.environment_precipitation_vulkan_execution_evidence_ready;
    result.uses_camera_near_particles = report.environment_precipitation_vulkan_uses_camera_near_particles;
    result.uses_scene_depth_occlusion = report.environment_precipitation_vulkan_uses_scene_depth_occlusion;
    result.weather_rows = report.environment_precipitation_vulkan_weather_rows;
    result.particle_rows = report.environment_precipitation_vulkan_particle_rows;
    result.occlusion_rows = report.environment_precipitation_vulkan_occlusion_rows;
    result.wetness_rows = report.environment_precipitation_vulkan_wetness_rows;
    result.audio_handoff_rows = report.environment_precipitation_vulkan_audio_handoff_rows;
    result.shader_rows = report.environment_precipitation_vulkan_shader_rows;
    result.quality_rows = report.environment_precipitation_vulkan_quality_rows;
    result.uploads_particle_buffers = report.environment_precipitation_vulkan_particle_buffer_uploads > 0U;
    result.invokes_backend = report.environment_precipitation_vulkan_backend_invocations > 0U;
    result.renderer_draws = report.environment_precipitation_vulkan_renderer_draws;
    result.depth_occlusion_readback = report.environment_precipitation_vulkan_depth_occlusion_readback;
    result.descriptor_set_bindings = report.environment_precipitation_vulkan_descriptor_set_bindings;
    result.synchronization2_barriers = report.environment_precipitation_vulkan_synchronization2_barriers;
    result.exposes_native_handles = report.environment_precipitation_vulkan_exposes_native_handles;
    result.mutates_materials = report.environment_precipitation_vulkan_mutates_materials;
    result.plays_audio = report.environment_precipitation_vulkan_plays_audio;

    result.diagnostics_count = report.environment_precipitation_vulkan_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.vulkan_backend_selected) {
        ++result.diagnostics_count;
    }
    if (result.weather != expectation.weather || result.kind != expectation.kind) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.uses_camera_near_particles || !result.uses_scene_depth_occlusion) {
        ++result.diagnostics_count;
    }
    if (result.weather_rows != 1 || result.particle_rows != 1 || result.occlusion_rows != 1 ||
        result.wetness_rows != expectation.wetness_rows ||
        result.audio_handoff_rows < expectation.minimum_audio_handoff_rows || result.shader_rows != 1 ||
        result.quality_rows != 1) {
        ++result.diagnostics_count;
    }
    if (expectation.require_renderer_execution) {
        if (!result.uploads_particle_buffers || !result.invokes_backend || result.renderer_draws == 0 ||
            !result.depth_occlusion_readback ||
            result.descriptor_set_bindings != kVulkanPrecipitationDescriptorSetBindings ||
            result.synchronization2_barriers == 0U) {
            ++result.diagnostics_count;
        }
    } else if (result.uploads_particle_buffers || result.invokes_backend || result.renderer_draws != 0 ||
               result.depth_occlusion_readback || result.descriptor_set_bindings != 0U ||
               result.synchronization2_barriers != 0U) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles || result.mutates_materials || result.plays_audio) {
        ++result.diagnostics_count;
    }

    if (!result.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::host_evidence_required;
    } else {
        result.ready = result.diagnostics_count == 0;
        result.status = result.ready ? Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::ready
                                     : Win32DesktopPresentationVulkanEnvironmentPrecipitationStatus::blocked;
    }
    return result;
}

Win32DesktopPresentationEnvironmentVolumetricFogReport
evaluate_win32_desktop_presentation_environment_volumetric_fog(const Win32DesktopPresentationReport& report,
                                                               const bool requested) {
    Win32DesktopPresentationEnvironmentVolumetricFogReport result;
    result.constants_binding = volumetric_fog_constants_binding();
    result.constant_buffer_bytes = volumetric_fog_constants_byte_size();
    result.froxel_output_buffer_binding = volumetric_fog_froxel_output_buffer_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_volumetric_fog_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.scene_depth_ready = report.environment_volumetric_fog_scene_depth_ready;
    result.shader_contract_evidence_ready = report.environment_volumetric_fog_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_volumetric_fog_package_evidence_ready;
    result.execution_evidence_ready = report.environment_volumetric_fog_execution_evidence_ready;
    result.froxel_output_ready = report.environment_volumetric_fog_froxel_output_ready;
    result.compute_dispatches = report.environment_volumetric_fog_compute_dispatches;
    result.exposes_native_handles = report.environment_volumetric_fog_exposes_native_handles;

    const auto plan = plan_volumetric_fog_policy(VolumetricFogPolicyDesc{
        .scene_depth_available = result.scene_depth_ready,
        .shader_contract_evidence_ready = result.shader_contract_evidence_ready,
        .execution_evidence_ready = result.execution_evidence_ready,
        .package_evidence_ready = result.package_evidence_ready,
        .request_ready_promotion = true,
        .request_native_handle_access = result.exposes_native_handles,
    });
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size()) +
                               report.environment_volumetric_fog_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.froxel_output_ready || result.compute_dispatches == 0) {
        ++result.diagnostics_count;
    }
    if (result.constant_buffer_bytes != static_cast<std::uint64_t>(volumetric_fog_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.froxel_output_buffer_binding != volumetric_fog_froxel_output_buffer_binding()) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationEnvironmentVolumetricFogStatus::ready
                                 : Win32DesktopPresentationEnvironmentVolumetricFogStatus::blocked;
    return result;
}

Win32DesktopPresentationVulkanEnvironmentVolumetricFogReport
evaluate_win32_desktop_presentation_vulkan_environment_volumetric_fog(const Win32DesktopPresentationReport& report,
                                                                      const bool requested) {
    Win32DesktopPresentationVulkanEnvironmentVolumetricFogReport result;
    result.scene_depth_texture_binding = volumetric_fog_scene_depth_texture_binding();
    result.scene_depth_sampler_binding = volumetric_fog_scene_depth_sampler_binding();
    result.constants_binding = volumetric_fog_constants_binding();
    result.constant_buffer_bytes = volumetric_fog_constants_byte_size();
    result.froxel_output_buffer_binding = volumetric_fog_froxel_output_buffer_binding();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_volumetric_fog_vulkan_requested;
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.scene_depth_ready = report.environment_volumetric_fog_vulkan_scene_depth_ready;
    result.shader_contract_evidence_ready = report.environment_volumetric_fog_vulkan_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_volumetric_fog_vulkan_package_evidence_ready;
    result.execution_evidence_ready = report.environment_volumetric_fog_vulkan_execution_evidence_ready;
    result.froxel_output_ready = report.environment_volumetric_fog_vulkan_froxel_output_ready;
    result.compute_dispatches = report.environment_volumetric_fog_vulkan_compute_dispatches;
    result.descriptor_set_bindings = report.environment_volumetric_fog_vulkan_descriptor_set_bindings;
    result.synchronization2_barriers = report.environment_volumetric_fog_vulkan_synchronization2_barriers;
    result.froxel_readback_nonzero = report.environment_volumetric_fog_vulkan_froxel_readback_nonzero;
    result.exposes_native_handles = report.environment_volumetric_fog_vulkan_exposes_native_handles;

    const auto plan = plan_volumetric_fog_policy(VolumetricFogPolicyDesc{
        .scene_depth_available = result.scene_depth_ready,
        .shader_contract_evidence_ready = result.shader_contract_evidence_ready,
        .execution_evidence_ready = result.execution_evidence_ready,
        .package_evidence_ready = result.package_evidence_ready,
        .request_ready_promotion = true,
        .request_native_handle_access = false,
    });
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size()) +
                               report.environment_volumetric_fog_vulkan_policy_diagnostics_count;
    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.vulkan_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.froxel_output_ready || result.compute_dispatches == 0) {
        ++result.diagnostics_count;
    }
    if (result.descriptor_set_bindings != kVulkanVolumetricFogDescriptorSetBindings) {
        ++result.diagnostics_count;
    }
    if (result.synchronization2_barriers == 0) {
        ++result.diagnostics_count;
    }
    if (!result.froxel_readback_nonzero) {
        ++result.diagnostics_count;
    }
    if (result.constant_buffer_bytes != static_cast<std::uint64_t>(volumetric_fog_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.scene_depth_texture_binding != volumetric_fog_scene_depth_texture_binding() ||
        result.scene_depth_sampler_binding != volumetric_fog_scene_depth_sampler_binding() ||
        result.constants_binding != volumetric_fog_constants_binding() ||
        result.froxel_output_buffer_binding != volumetric_fog_froxel_output_buffer_binding()) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles) {
        ++result.diagnostics_count;
    }

    if (!result.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::host_evidence_required;
    } else {
        result.ready = result.diagnostics_count == 0;
        result.status = result.ready ? Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::ready
                                     : Win32DesktopPresentationVulkanEnvironmentVolumetricFogStatus::blocked;
    }
    return result;
}

Win32DesktopPresentationEnvironmentVolumetricCloudReport
evaluate_win32_desktop_presentation_environment_volumetric_cloud(const Win32DesktopPresentationReport& report,
                                                                 const bool requested,
                                                                 const bool require_renderer_execution) {
    Win32DesktopPresentationEnvironmentVolumetricCloudReport result;
    result.weather_map_binding = volumetric_cloud_weather_map_binding();
    result.shape_noise_binding = volumetric_cloud_shape_noise_binding();
    result.erosion_noise_binding = volumetric_cloud_erosion_noise_binding();
    result.sampler_binding = volumetric_cloud_sampler_binding();
    result.constants_binding = volumetric_cloud_constants_binding();
    result.constant_buffer_bytes = volumetric_cloud_constants_byte_size();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_volumetric_cloud_requested;
    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.shader_contract_evidence_ready = report.environment_volumetric_cloud_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_volumetric_cloud_package_evidence_ready;
    result.execution_evidence_ready = report.environment_volumetric_cloud_execution_evidence_ready;
    result.weather_map_ready = report.environment_volumetric_cloud_weather_map_ready;
    result.shape_noise_ready = report.environment_volumetric_cloud_shape_noise_ready;
    result.erosion_noise_ready = report.environment_volumetric_cloud_erosion_noise_ready;
    result.uploads_volume_textures = report.environment_volumetric_cloud_uploads_volume_textures;
    result.invokes_backend = report.environment_volumetric_cloud_invokes_backend;
    result.renderer_draws = report.environment_volumetric_cloud_renderer_draws;
    result.raymarch_passes = report.environment_volumetric_cloud_raymarch_passes;
    result.readback_nonzero = report.environment_volumetric_cloud_readback_nonzero;
    result.exposes_native_handles = report.environment_volumetric_cloud_exposes_native_handles;
    result.plays_audio = report.environment_volumetric_cloud_plays_audio;
    result.executes_precipitation = report.environment_volumetric_cloud_executes_precipitation;
    result.map_rows = report.environment_volumetric_cloud_map_rows;
    result.layer_rows = report.environment_volumetric_cloud_layer_rows;
    result.lighting_rows = report.environment_volumetric_cloud_lighting_rows;
    result.raymarch_rows = report.environment_volumetric_cloud_raymarch_rows;
    result.temporal_rows = report.environment_volumetric_cloud_temporal_rows;
    result.shadow_rows = report.environment_volumetric_cloud_shadow_rows;
    result.storm_rows = report.environment_volumetric_cloud_storm_rows;
    result.shader_contract_rows = report.environment_volumetric_cloud_shader_contract_rows;
    result.quality_rows = report.environment_volumetric_cloud_quality_rows;

    auto expected_desc = sample_environment_volumetric_cloud_policy_desc();
    expected_desc.shader_contract_evidence_ready = result.shader_contract_evidence_ready;
    expected_desc.package_evidence_ready = result.package_evidence_ready;
    expected_desc.execution_evidence_ready = result.execution_evidence_ready;
    expected_desc.request_ready_promotion = true;
    expected_desc.request_volume_texture_upload = require_renderer_execution;
    expected_desc.request_backend_execution = require_renderer_execution;
    if (require_renderer_execution) {
        expected_desc.weather_map_upload_count = result.uploads_volume_textures ? 1U : 0U;
        expected_desc.shape_noise_upload_count = result.uploads_volume_textures ? 1U : 0U;
        expected_desc.erosion_noise_upload_count = result.uploads_volume_textures ? 1U : 0U;
        expected_desc.backend_invocation_count = result.invokes_backend ? 1U : 0U;
        expected_desc.raymarch_pass_count = result.raymarch_passes;
        expected_desc.readback_nonzero_proven = result.readback_nonzero;
    }
    const auto expected_plan = plan_volumetric_cloud_policy(expected_desc);
    result.diagnostics_count = static_cast<std::uint32_t>(expected_plan.diagnostics.size()) +
                               report.environment_volumetric_cloud_policy_diagnostics_count;

    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.d3d12_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.weather_map_ready || !result.shape_noise_ready || !result.erosion_noise_ready) {
        ++result.diagnostics_count;
    }
    if (result.weather_map_binding != volumetric_cloud_weather_map_binding() ||
        result.shape_noise_binding != volumetric_cloud_shape_noise_binding() ||
        result.erosion_noise_binding != volumetric_cloud_erosion_noise_binding() ||
        result.sampler_binding != volumetric_cloud_sampler_binding() ||
        result.constants_binding != volumetric_cloud_constants_binding() ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(volumetric_cloud_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.map_rows != static_cast<std::uint32_t>(expected_plan.map_rows.size()) ||
        result.layer_rows != static_cast<std::uint32_t>(expected_plan.layer_rows.size()) ||
        result.lighting_rows != static_cast<std::uint32_t>(expected_plan.lighting_rows.size()) ||
        result.raymarch_rows != static_cast<std::uint32_t>(expected_plan.raymarch_rows.size()) ||
        result.temporal_rows != static_cast<std::uint32_t>(expected_plan.temporal_rows.size()) ||
        result.shadow_rows != static_cast<std::uint32_t>(expected_plan.shadow_rows.size()) ||
        result.storm_rows != static_cast<std::uint32_t>(expected_plan.storm_rows.size()) ||
        result.shader_contract_rows != static_cast<std::uint32_t>(expected_plan.shader_contract_rows.size()) ||
        result.quality_rows != static_cast<std::uint32_t>(expected_plan.quality_rows.size())) {
        ++result.diagnostics_count;
    }
    if (require_renderer_execution) {
        if (!result.uploads_volume_textures || !result.invokes_backend || result.renderer_draws == 0 ||
            result.raymarch_passes == 0 || !result.readback_nonzero) {
            ++result.diagnostics_count;
        }
    } else if (result.uploads_volume_textures || result.invokes_backend || result.renderer_draws != 0 ||
               result.raymarch_passes != 0 || result.readback_nonzero) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles || result.plays_audio || result.executes_precipitation) {
        ++result.diagnostics_count;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationEnvironmentVolumetricCloudStatus::ready
                                 : Win32DesktopPresentationEnvironmentVolumetricCloudStatus::blocked;
    return result;
}

Win32DesktopPresentationVulkanEnvironmentVolumetricCloudReport
evaluate_win32_desktop_presentation_vulkan_environment_volumetric_cloud(const Win32DesktopPresentationReport& report,
                                                                        const bool requested) {
    Win32DesktopPresentationVulkanEnvironmentVolumetricCloudReport result;
    const auto bindings = vulkan_volumetric_cloud_probe_bindings();
    result.weather_map_binding = bindings.weather_map_binding;
    result.shape_noise_binding = bindings.shape_noise_binding;
    result.erosion_noise_binding = bindings.erosion_noise_binding;
    result.sampler_binding = bindings.sampler_binding;
    result.constants_binding = bindings.constants_binding;
    result.constant_buffer_bytes = volumetric_cloud_constants_byte_size();
    if (!requested) {
        return result;
    }

    result.requested = report.environment_volumetric_cloud_vulkan_requested;
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.shader_contract_evidence_ready = report.environment_volumetric_cloud_vulkan_shader_contract_evidence_ready;
    result.package_evidence_ready = report.environment_volumetric_cloud_vulkan_package_evidence_ready;
    result.execution_evidence_ready = report.environment_volumetric_cloud_vulkan_execution_evidence_ready;
    result.weather_map_ready = report.environment_volumetric_cloud_vulkan_weather_map_ready;
    result.shape_noise_ready = report.environment_volumetric_cloud_vulkan_shape_noise_ready;
    result.erosion_noise_ready = report.environment_volumetric_cloud_vulkan_erosion_noise_ready;
    result.uploads_volume_textures = report.environment_volumetric_cloud_vulkan_uploads_volume_textures;
    result.invokes_backend = report.environment_volumetric_cloud_vulkan_invokes_backend;
    result.renderer_draws = report.environment_volumetric_cloud_vulkan_renderer_draws;
    result.raymarch_passes = report.environment_volumetric_cloud_vulkan_raymarch_passes;
    result.descriptor_set_bindings = report.environment_volumetric_cloud_vulkan_descriptor_set_bindings;
    result.synchronization2_barriers = report.environment_volumetric_cloud_vulkan_synchronization2_barriers;
    result.readback_nonzero = report.environment_volumetric_cloud_vulkan_readback_nonzero;
    result.exposes_native_handles = report.environment_volumetric_cloud_vulkan_exposes_native_handles;
    result.plays_audio = report.environment_volumetric_cloud_vulkan_plays_audio;
    result.executes_precipitation = report.environment_volumetric_cloud_vulkan_executes_precipitation;
    result.map_rows = report.environment_volumetric_cloud_vulkan_map_rows;
    result.layer_rows = report.environment_volumetric_cloud_vulkan_layer_rows;
    result.lighting_rows = report.environment_volumetric_cloud_vulkan_lighting_rows;
    result.raymarch_rows = report.environment_volumetric_cloud_vulkan_raymarch_rows;
    result.temporal_rows = report.environment_volumetric_cloud_vulkan_temporal_rows;
    result.shadow_rows = report.environment_volumetric_cloud_vulkan_shadow_rows;
    result.storm_rows = report.environment_volumetric_cloud_vulkan_storm_rows;
    result.shader_contract_rows = report.environment_volumetric_cloud_vulkan_shader_contract_rows;
    result.quality_rows = report.environment_volumetric_cloud_vulkan_quality_rows;

    auto expected_desc = sample_environment_volumetric_cloud_policy_desc();
    expected_desc.shader_contract_evidence_ready = result.shader_contract_evidence_ready;
    expected_desc.package_evidence_ready = result.package_evidence_ready;
    expected_desc.execution_evidence_ready = result.execution_evidence_ready;
    expected_desc.request_ready_promotion = true;
    expected_desc.request_volume_texture_upload = true;
    expected_desc.request_backend_execution = true;
    expected_desc.weather_map_upload_count = result.uploads_volume_textures ? 1U : 0U;
    expected_desc.shape_noise_upload_count = result.uploads_volume_textures ? 1U : 0U;
    expected_desc.erosion_noise_upload_count = result.uploads_volume_textures ? 1U : 0U;
    expected_desc.backend_invocation_count = result.invokes_backend ? 1U : 0U;
    expected_desc.raymarch_pass_count = result.raymarch_passes;
    expected_desc.readback_nonzero_proven = result.readback_nonzero;
    const auto expected_plan = plan_volumetric_cloud_policy(expected_desc);
    result.diagnostics_count = static_cast<std::uint32_t>(expected_plan.diagnostics.size()) +
                               report.environment_volumetric_cloud_vulkan_policy_diagnostics_count;

    if (!result.requested) {
        ++result.diagnostics_count;
    }
    if (!result.vulkan_backend_selected) {
        ++result.diagnostics_count;
    }
    if (!result.shader_contract_evidence_ready || !result.package_evidence_ready || !result.execution_evidence_ready) {
        ++result.diagnostics_count;
    }
    if (!result.weather_map_ready || !result.shape_noise_ready || !result.erosion_noise_ready) {
        ++result.diagnostics_count;
    }
    if (result.weather_map_binding != volumetric_cloud_weather_map_binding() ||
        result.shape_noise_binding != volumetric_cloud_shape_noise_binding() ||
        result.erosion_noise_binding != volumetric_cloud_erosion_noise_binding() ||
        result.sampler_binding != volumetric_cloud_sampler_binding() + kVulkanVolumetricCloudSamplerBindingShift ||
        result.constants_binding !=
            volumetric_cloud_constants_binding() + kVulkanVolumetricCloudConstantsBindingShift ||
        result.constant_buffer_bytes != static_cast<std::uint64_t>(volumetric_cloud_constants_byte_size())) {
        ++result.diagnostics_count;
    }
    if (result.map_rows != static_cast<std::uint32_t>(expected_plan.map_rows.size()) ||
        result.layer_rows != static_cast<std::uint32_t>(expected_plan.layer_rows.size()) ||
        result.lighting_rows != static_cast<std::uint32_t>(expected_plan.lighting_rows.size()) ||
        result.raymarch_rows != static_cast<std::uint32_t>(expected_plan.raymarch_rows.size()) ||
        result.temporal_rows != static_cast<std::uint32_t>(expected_plan.temporal_rows.size()) ||
        result.shadow_rows != static_cast<std::uint32_t>(expected_plan.shadow_rows.size()) ||
        result.storm_rows != static_cast<std::uint32_t>(expected_plan.storm_rows.size()) ||
        result.shader_contract_rows != static_cast<std::uint32_t>(expected_plan.shader_contract_rows.size()) ||
        result.quality_rows != static_cast<std::uint32_t>(expected_plan.quality_rows.size())) {
        ++result.diagnostics_count;
    }
    if (!result.uploads_volume_textures || !result.invokes_backend || result.renderer_draws == 0 ||
        result.raymarch_passes == 0 || result.descriptor_set_bindings != kVulkanVolumetricCloudDescriptorSetBindings ||
        result.synchronization2_barriers == 0 || !result.readback_nonzero) {
        ++result.diagnostics_count;
    }
    if (result.exposes_native_handles || result.plays_audio || result.executes_precipitation) {
        ++result.diagnostics_count;
    }

    if (!result.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::host_evidence_required;
    } else {
        result.ready = result.diagnostics_count == 0;
        result.status = result.ready ? Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::ready
                                     : Win32DesktopPresentationVulkanEnvironmentVolumetricCloudStatus::blocked;
    }
    return result;
}

Win32DesktopPresentationEnvironmentIblRendererExecutionReport
evaluate_win32_desktop_presentation_environment_ibl_renderer_execution(
    const Win32DesktopPresentationEnvironmentIblRendererExecutionDesc& desc) {
    Win32DesktopPresentationEnvironmentIblRendererExecutionReport result;
    result.requested = desc.requested;
    result.d3d12_backend_selected = desc.d3d12_backend_selected;
    result.texture_cube_edge_size = desc.edge_size;
    result.radiance_mips = desc.mip_count;
    if (!desc.requested) {
        return result;
    }

    if (!desc.d3d12_backend_selected || !has_shader_bytecode(desc.sampling_vertex_shader) ||
        !has_shader_bytecode(desc.sampling_fragment_shader)) {
        result.status = Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::blocked;
        result.diagnostics_count = 1U;
        return result;
    }

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12)
    const auto proof =
        rhi::d3d12::execute_environment_ibl_renderer_upload(rhi::d3d12::D3d12EnvironmentIblRendererUploadDesc{
            .device = rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = true, .enable_debug_layer = false},
            .edge_size = desc.edge_size,
            .mip_count = desc.mip_count,
            .format = rhi::d3d12::D3d12EnvironmentIblTextureFormat::rgba16_float,
            .require_shader_sampling = true,
            .require_runtime_capture = true,
            .sampling_vertex_shader = desc.sampling_vertex_shader.bytecode,
            .sampling_fragment_shader = desc.sampling_fragment_shader.bytecode,
        });

    result.texture_cube_uploads = proof.texture_cube_uploads;
    result.texture_cube_faces = proof.texture_cube_faces;
    result.texture_cube_edge_size = proof.texture_cube_edge_size;
    result.radiance_mips = proof.radiance_mips;
    result.irradiance_rows = proof.irradiance_rows;
    result.shader_sampling_proven = proof.shader_sampling_proven;
    result.shader_sample_readback_nonzero = proof.shader_sample_readback_nonzero;
    result.runtime_capture_faces = proof.runtime_capture_faces;
    result.runtime_capture_readback_nonzero = proof.runtime_capture_readback_nonzero;
    result.runtime_capture_readback_checksum = proof.readback_checksum;
    result.native_handle_access = proof.native_handle_access;
    result.ready = proof.succeeded && proof.native_handle_access == 0U && proof.texture_cube_uploads > 0U &&
                   proof.texture_cube_faces == 6U && proof.texture_cube_edge_size == desc.edge_size &&
                   proof.radiance_mips == desc.mip_count && proof.irradiance_rows == 9U &&
                   proof.shader_sampling_proven && proof.shader_sample_readback_nonzero &&
                   proof.runtime_capture_faces == 6U && proof.runtime_capture_readback_nonzero &&
                   proof.readback_checksum != 0U;
    result.diagnostics_count = proof.diagnostics + (result.ready ? 0U : 1U);
    result.status = result.ready ? Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::ready
                                 : Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::blocked;
#else
    result.status = Win32DesktopPresentationEnvironmentIblRendererExecutionStatus::blocked;
    result.diagnostics_count = 1U;
#endif
    return result;
}

Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionReport
evaluate_win32_desktop_presentation_vulkan_environment_ibl_renderer_execution(
    const Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionDesc& desc) {
    Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionReport result;
    result.requested = desc.requested;
    result.vulkan_backend_selected = desc.vulkan_backend_selected;
    result.texture_cube_edge_size = desc.edge_size;
    result.radiance_mips = desc.mip_count;
    if (!desc.requested) {
        return result;
    }

    if (!desc.vulkan_backend_selected) {
        result.status = Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::host_evidence_required;
        result.diagnostics_count = 1U;
        result.diagnostic = "Vulkan environment IBL renderer execution requires the Vulkan backend";
        return result;
    }
    if (!has_shader_bytecode(desc.sampling_vertex_shader) || !has_shader_bytecode(desc.sampling_fragment_shader)) {
        result.status = Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::blocked;
        result.diagnostics_count = 1U;
        result.diagnostic = "Vulkan environment IBL renderer execution requires packaged SPIR-V shaders";
        return result;
    }

#if defined(MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN)
    const auto proof =
        rhi::vulkan::execute_environment_ibl_renderer_upload(rhi::vulkan::VulkanEnvironmentIblRendererUploadDesc{
            .edge_size = desc.edge_size,
            .mip_count = desc.mip_count,
            .format = rhi::vulkan::VulkanEnvironmentIblTextureFormat::rgba16_float,
            .require_shader_sampling = true,
            .require_runtime_capture = true,
            .sampling_vertex_shader = desc.sampling_vertex_shader.bytecode,
            .sampling_fragment_shader = desc.sampling_fragment_shader.bytecode,
            .sampling_vertex_entry_point = desc.sampling_vertex_shader.entry_point,
            .sampling_fragment_entry_point = desc.sampling_fragment_shader.entry_point,
        });

    result.texture_cube_uploads = proof.texture_cube_uploads;
    result.texture_cube_faces = proof.texture_cube_faces;
    result.texture_cube_edge_size = proof.texture_cube_edge_size;
    result.radiance_mips = proof.radiance_mips;
    result.irradiance_rows = proof.irradiance_rows;
    result.cube_compatible_image_created = proof.cube_compatible_image_created;
    result.cube_image_view_created = proof.cube_image_view_created;
    result.sampler_created = proof.sampler_created;
    result.descriptor_set_bindings = proof.descriptor_set_bindings;
    result.synchronization2_barriers = proof.synchronization2_barriers;
    result.shader_sampling_proven = proof.shader_sampling_proven;
    result.shader_sample_readback_nonzero = proof.shader_sample_readback_nonzero;
    result.runtime_capture_faces = proof.runtime_capture_faces;
    result.runtime_capture_readback_nonzero = proof.runtime_capture_readback_nonzero;
    result.runtime_capture_readback_checksum = proof.readback_checksum;
    result.native_handle_access = proof.native_handle_access;
    result.diagnostic = proof.diagnostic;
    result.ready =
        proof.succeeded && proof.native_handle_access == 0U && proof.texture_cube_uploads > 0U &&
        proof.texture_cube_faces == 6U && proof.texture_cube_edge_size == desc.edge_size &&
        proof.radiance_mips == desc.mip_count && proof.irradiance_rows == 9U && proof.cube_compatible_image_created &&
        proof.cube_image_view_created && proof.sampler_created && proof.descriptor_set_bindings == 2U &&
        proof.synchronization2_barriers > 0U && proof.shader_sampling_proven && proof.shader_sample_readback_nonzero &&
        proof.runtime_capture_faces == 6U && proof.runtime_capture_readback_nonzero && proof.readback_checksum != 0U;
    result.diagnostics_count = proof.diagnostics + (result.ready ? 0U : 1U);
    result.status = result.ready ? Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::ready
                                 : Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::blocked;
#else
    result.status = Win32DesktopPresentationVulkanEnvironmentIblRendererExecutionStatus::host_evidence_required;
    result.diagnostics_count = 1U;
    result.diagnostic = "Vulkan runtime-host presentation support is unavailable";
#endif
    return result;
}

Win32DesktopPresentationVulkanPostprocessExecutionReport
evaluate_win32_desktop_presentation_vulkan_postprocess_execution(const Win32DesktopPresentationReport& report,
                                                                 const std::uint64_t expected_postprocess_passes,
                                                                 const bool requested) {
    Win32DesktopPresentationVulkanPostprocessExecutionReport result;
    if (!requested) {
        return result;
    }
    if (!postprocess_policy_requested(report)) {
        return result;
    }

    const auto policy = evaluate_win32_desktop_presentation_postprocess_policy(report);
    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
    result.backend_shader_evidence_ready = policy.backend_shader_evidence_ready;
    result.expected_postprocess_passes = expected_postprocess_passes;
    result.postprocess_passes_executed = report.renderer_stats.postprocess_passes_executed;
    result.framegraph_passes_executed = report.renderer_stats.framegraph_passes_executed;
    result.framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded;
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    result.postprocess_passes_current =
        report.renderer_stats.postprocess_passes_executed == expected_postprocess_passes;
    result.status = result.vulkan_backend_selected && result.postprocess_ready &&
                            result.backend_shader_evidence_ready && result.postprocess_passes_current
                        ? Win32DesktopPresentationVulkanPostprocessExecutionStatus::ready
                        : Win32DesktopPresentationVulkanPostprocessExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationVulkanPostprocessExecutionStatus::ready;
    return result;
}

Win32DesktopPresentationD3d12InstancedDrawExecutionReport
evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution(const Win32DesktopPresentationReport& report,
                                                                   std::uint64_t expected_instances_submitted) {
    Win32DesktopPresentationD3d12InstancedDrawExecutionReport result;
    result.expected_instances_submitted = expected_instances_submitted;
    result.instanced_draw_calls = report.rhi_instanced_draw_calls;
    result.instanced_indexed_draw_calls = report.rhi_instanced_indexed_draw_calls;
    result.instanced_instances_submitted = report.rhi_instanced_instances_submitted;
    if (expected_instances_submitted == 0) {
        return result;
    }

    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.instanced_draws_current = report.rhi_instanced_draw_calls > 0 && report.rhi_instanced_indexed_draw_calls > 0;
    result.instanced_instances_current = report.rhi_instanced_instances_submitted >= expected_instances_submitted;
    result.status =
        result.d3d12_backend_selected && result.instanced_draws_current && result.instanced_instances_current
            ? Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::ready
            : Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationD3d12InstancedDrawExecutionStatus::ready;
    return result;
}

std::string_view win32_desktop_presentation_vulkan_instanced_draw_execution_status_name(
    const Win32DesktopPresentationVulkanInstancedDrawExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationVulkanInstancedDrawExecutionReport
evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution(const Win32DesktopPresentationReport& report,
                                                                    const std::uint64_t expected_instances_submitted) {
    Win32DesktopPresentationVulkanInstancedDrawExecutionReport result;
    result.expected_instances_submitted = expected_instances_submitted;
    result.instanced_draw_calls = report.rhi_instanced_draw_calls;
    result.instanced_indexed_draw_calls = report.rhi_instanced_indexed_draw_calls;
    result.instanced_instances_submitted = report.rhi_instanced_instances_submitted;
    if (expected_instances_submitted == 0) {
        return result;
    }

    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.instanced_draws_current = report.rhi_instanced_draw_calls > 0 && report.rhi_instanced_indexed_draw_calls > 0;
    result.instanced_instances_current = report.rhi_instanced_instances_submitted >= expected_instances_submitted;
    result.status =
        result.vulkan_backend_selected && result.instanced_draws_current && result.instanced_instances_current
            ? Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::ready
            : Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationVulkanInstancedDrawExecutionStatus::ready;
    return result;
}

std::string_view win32_desktop_presentation_gpu_memory_policy_status_name(
    Win32DesktopPresentationGpuMemoryPolicyStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationGpuMemoryPolicyStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationGpuMemoryPolicyStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationGpuMemoryPolicyStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view win32_desktop_presentation_d3d12_gpu_memory_execution_status_name(
    Win32DesktopPresentationD3d12GpuMemoryExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationGpuMemoryPolicyReport
evaluate_win32_desktop_presentation_gpu_memory_policy(const Win32DesktopPresentationReport& report,
                                                      const Win32DesktopPresentationGpuMemoryPolicyDesc& desc) {
    Win32DesktopPresentationGpuMemoryPolicyReport result;
    result.backend_memory_evidence_required = desc.require_backend_memory_evidence;
    result.os_video_memory_budget_required = desc.require_os_video_memory_budget;
    result.expected_frames = desc.expected_frames;
    result.frames_finished = report.renderer_stats.frames_finished;
    result.frames_current = desc.expected_frames == 0 || report.renderer_stats.frames_finished == desc.expected_frames;
    if (!gpu_memory_policy_requested(desc)) {
        return result;
    }

    result.scene_resources_ready = !desc.require_scene_gpu_bindings ||
                                   report.scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready;

    const auto& memory = report.rhi_memory_diagnostics;
    const auto upload_bytes = gpu_memory_policy_upload_bytes(report);
    const auto residency_pressure_events =
        desc.residency_pressure_event_count > 0
            ? desc.residency_pressure_event_count
            : static_cast<std::uint64_t>(gpu_memory_policy_transient_pressure_ready(report) ? 1U : 0U);

    std::array<GpuMemoryRequestDesc, 3> requests{};
    std::size_t request_count = 0;
    if (memory.committed_resources_byte_estimate_available && memory.committed_resources_byte_estimate > 0) {
        requests[request_count++] = GpuMemoryRequestDesc{
            .residency = GpuMemoryResidencyClass::committed,
            .requested_bytes = memory.committed_resources_byte_estimate,
            .transient_heap = GpuMemoryTransientHeapPolicy::none,
            .upload_pressure = GpuMemoryUploadPressureKind::none,
            .scene_resources_available = result.scene_resources_ready,
            .request_background_streaming = false,
            .request_automatic_eviction = false,
            .require_declared_budget_evidence = desc.require_declared_budget_evidence,
            .require_residency_pressure_evidence = desc.require_residency_pressure_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .source_index = 0,
        };
    }
    if (report.rhi_transient_heap_allocations > 0 || report.rhi_transient_placed_allocations > 0) {
        const auto transient_bytes = report.rhi_transient_heap_allocations > 0
                                         ? report.rhi_transient_heap_allocations * 1024ULL * 1024ULL
                                         : 1ULL;
        requests[request_count++] = GpuMemoryRequestDesc{
            .residency = GpuMemoryResidencyClass::transient,
            .requested_bytes = transient_bytes,
            .transient_heap = report.rhi_transient_placed_allocations > 0
                                  ? GpuMemoryTransientHeapPolicy::alias_group_heap
                                  : GpuMemoryTransientHeapPolicy::per_resource_heap,
            .upload_pressure = GpuMemoryUploadPressureKind::none,
            .scene_resources_available = result.scene_resources_ready,
            .request_background_streaming = false,
            .request_automatic_eviction = false,
            .require_declared_budget_evidence = desc.require_declared_budget_evidence,
            .require_residency_pressure_evidence = desc.require_residency_pressure_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .source_index = 1,
        };
    }
    if (upload_bytes > 0) {
        requests[request_count++] = GpuMemoryRequestDesc{
            .residency = GpuMemoryResidencyClass::placed,
            .requested_bytes = upload_bytes,
            .transient_heap = GpuMemoryTransientHeapPolicy::none,
            .upload_pressure = GpuMemoryUploadPressureKind::ring_buffer,
            .scene_resources_available = result.scene_resources_ready,
            .request_background_streaming = false,
            .request_automatic_eviction = false,
            .require_declared_budget_evidence = desc.require_declared_budget_evidence,
            .require_residency_pressure_evidence = desc.require_residency_pressure_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .source_index = 2,
        };
    }

    const auto plan = plan_gpu_memory_policy(GpuMemoryPolicyDesc{
        .requests = std::span<const GpuMemoryRequestDesc>{requests.data(), request_count},
        .declared_local_budget_bytes = desc.declared_local_budget_bytes,
        .declared_non_local_budget_bytes = 0,
        .os_video_memory_budget_available = memory.os_video_memory_budget_available,
        .os_local_budget_bytes = memory.local_video_memory_budget_bytes,
        .os_local_usage_bytes = memory.local_video_memory_usage_bytes,
        .os_non_local_budget_bytes = memory.non_local_video_memory_budget_bytes,
        .os_non_local_usage_bytes = memory.non_local_video_memory_usage_bytes,
        .committed_byte_estimate_available = memory.committed_resources_byte_estimate_available,
        .committed_byte_estimate = memory.committed_resources_byte_estimate,
        .transient_heap_allocations = report.rhi_transient_heap_allocations,
        .transient_placed_allocations = report.rhi_transient_placed_allocations,
        .transient_placed_resources_alive = report.rhi_transient_placed_resources_alive,
        .upload_bytes_written = upload_bytes,
        .residency_pressure_event_count = residency_pressure_events,
        .backend = postprocess_policy_backend(report.selected_backend),
        .require_backend_memory_evidence = desc.require_backend_memory_evidence,
        .backend_memory_evidence_ready =
            gpu_memory_policy_backend_memory_evidence_ready(report, desc.require_backend_memory_evidence),
        .require_os_video_memory_budget =
            desc.require_os_video_memory_budget && memory.os_video_memory_budget_available,
        .package_counter_evidence_ready = desc.package_counter_evidence_ready,
    });

    result.backend_memory_evidence_ready = plan.backend_memory_evidence_ready;
    result.os_video_memory_budget_available = plan.os_video_memory_budget_available;
    result.memory_budget_evidence_ready = plan.memory_budget_evidence_ready;
    result.residency_pressure_evidence_ready = plan.residency_pressure_evidence_ready;
    result.package_counter_evidence_ready = plan.package_counter_evidence_ready;
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!result.frames_current) {
        ++result.diagnostics_count;
    }
    result.request_count = plan.request_count;
    result.total_requested_bytes = plan.total_requested_bytes;
    result.total_counted_bytes = plan.total_counted_bytes;
    result.os_local_budget_bytes = plan.os_local_budget_bytes;
    result.os_local_usage_bytes = plan.os_local_usage_bytes;
    result.committed_byte_estimate = plan.committed_byte_estimate;
    result.transient_heap_allocations = plan.transient_heap_allocations;
    result.transient_placed_allocations = plan.transient_placed_allocations;
    result.transient_placed_resources_alive = plan.transient_placed_resources_alive;
    result.upload_bytes_written = plan.upload_bytes_written;
    result.residency_pressure_event_count = plan.residency_pressure_event_count;
    result.transient_heap_request_count = plan.transient_heap_request_count;
    result.upload_pressure_request_count = plan.upload_pressure_request_count;
    result.declared_budget_request_count = plan.declared_budget_request_count;
    result.residency_pressure_request_count = plan.residency_pressure_request_count;
    result.package_counter_request_count = plan.package_counter_request_count;
    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationGpuMemoryPolicyStatus::ready
                                 : Win32DesktopPresentationGpuMemoryPolicyStatus::blocked;
    return result;
}

Win32DesktopPresentationD3d12GpuMemoryExecutionReport
evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution(const Win32DesktopPresentationReport& report,
                                                               const bool requested) {
    Win32DesktopPresentationD3d12GpuMemoryExecutionReport result;
    const auto& memory = report.rhi_memory_diagnostics;
    result.local_video_memory_budget_bytes = memory.local_video_memory_budget_bytes;
    result.local_video_memory_usage_bytes = memory.local_video_memory_usage_bytes;
    result.committed_resources_byte_estimate = memory.committed_resources_byte_estimate;
    result.transient_heap_allocations = report.rhi_transient_heap_allocations;
    result.transient_placed_allocations = report.rhi_transient_placed_allocations;
    result.transient_placed_resources_alive = report.rhi_transient_placed_resources_alive;
    result.upload_bytes_written = gpu_memory_policy_upload_bytes(report);

    if (!requested) {
        return result;
    }

    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.os_video_memory_budget_available = memory.os_video_memory_budget_available;
    result.committed_byte_estimate_available = memory.committed_resources_byte_estimate_available;
    result.memory_budget_current =
        result.d3d12_backend_selected && result.committed_byte_estimate_available &&
        result.committed_resources_byte_estimate > 0 &&
        (result.os_video_memory_budget_available && result.local_video_memory_budget_bytes > 0 ||
         !result.os_video_memory_budget_available);
    result.transient_heap_current = gpu_memory_policy_transient_pressure_ready(report);
    result.status = result.d3d12_backend_selected && result.memory_budget_current && result.transient_heap_current &&
                            result.upload_bytes_written > 0
                        ? Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::ready
                        : Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationD3d12GpuMemoryExecutionStatus::ready;
    return result;
}

std::string_view win32_desktop_presentation_vulkan_gpu_memory_execution_status_name(
    const Win32DesktopPresentationVulkanGpuMemoryExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationVulkanGpuMemoryExecutionReport
evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution(const Win32DesktopPresentationReport& report,
                                                                const bool requested) {
    Win32DesktopPresentationVulkanGpuMemoryExecutionReport result;
    result.committed_byte_estimate_available =
        report.rhi_memory_diagnostics.committed_resources_byte_estimate_available;
    result.committed_resources_byte_estimate = report.rhi_memory_diagnostics.committed_resources_byte_estimate;
    result.transient_heap_allocations = report.rhi_transient_heap_allocations;
    result.transient_placed_allocations = report.rhi_transient_placed_allocations;
    result.transient_placed_resources_alive = report.rhi_transient_placed_resources_alive;
    result.upload_bytes_written = gpu_memory_policy_upload_bytes(report);
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;

    if (!requested) {
        return result;
    }

    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.transient_heap_current = gpu_memory_policy_transient_pressure_ready(report);
    result.memory_budget_current = result.vulkan_backend_selected && result.committed_byte_estimate_available &&
                                   result.committed_resources_byte_estimate > 0 && result.upload_bytes_written > 0 &&
                                   result.transient_heap_current;
    result.status = result.memory_budget_current ? Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::ready
                                                 : Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationVulkanGpuMemoryExecutionStatus::ready;
    return result;
}

std::string_view win32_desktop_presentation_debug_profiling_policy_status_name(
    const Win32DesktopPresentationDebugProfilingPolicyStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationDebugProfilingPolicyStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationDebugProfilingPolicyStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationDebugProfilingPolicyStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationDebugProfilingPolicyReport evaluate_win32_desktop_presentation_debug_profiling_policy(
    const Win32DesktopPresentationReport& report, const Win32DesktopPresentationDebugProfilingPolicyDesc& desc) {
    Win32DesktopPresentationDebugProfilingPolicyReport result;
    result.backend_profiling_evidence_required = desc.require_backend_profiling_evidence;
    result.expected_frames = desc.expected_frames;
    result.frames_finished = report.renderer_stats.frames_finished;
    result.frames_current = desc.expected_frames == 0 || report.renderer_stats.frames_finished == desc.expected_frames;
    if (!debug_profiling_policy_requested(desc)) {
        return result;
    }

    result.scene_resources_ready = !desc.require_scene_gpu_bindings ||
                                   report.scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready;
    result.gpu_timestamp_ticks_per_second = report.rhi_gpu_timestamp_ticks_per_second;
    result.gpu_debug_scopes_begun = report.rhi_gpu_debug_scopes_begun;
    result.gpu_debug_scopes_ended = report.rhi_gpu_debug_scopes_ended;
    result.gpu_debug_markers_inserted = report.rhi_gpu_debug_markers_inserted;
    result.cpu_profile_zone_count = desc.cpu_profile_zone_count;
    result.trace_capture_handoff_row_count = desc.trace_capture_handoff_row_count;
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    result.framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded;

    const auto backend_evidence_ready =
        debug_profiling_policy_backend_evidence_ready(report, desc.require_backend_profiling_evidence);

    std::array<DebugProfilingRequestDesc, 2> requests{};
    std::size_t request_count = 0;
    if (desc.require_backend_profiling_evidence && report.selected_backend == Win32DesktopPresentationBackend::d3d12) {
        requests[request_count++] = DebugProfilingRequestDesc{
            .capture_kind = DebugProfilingCaptureKind::pix_gpu_handoff,
            .require_gpu_timestamps = true,
            .require_gpu_debug_markers = true,
            .require_capture_handoff_evidence = true,
            .require_cpu_profile_zone_evidence = desc.require_cpu_profile_zone_evidence,
            .require_trace_capture_handoff_evidence = desc.require_trace_capture_handoff_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .scene_frame_resources_available = result.scene_resources_ready,
            .request_automatic_capture_execution = false,
            .request_production_flame_graph = false,
            .request_crash_telemetry_export = false,
            .source_index = 0,
        };
    } else if (desc.require_backend_profiling_evidence &&
               report.selected_backend == Win32DesktopPresentationBackend::vulkan) {
        requests[request_count++] = DebugProfilingRequestDesc{
            .capture_kind = DebugProfilingCaptureKind::vulkan_debug_utils,
            .require_gpu_timestamps = false,
            .require_gpu_debug_markers = true,
            .require_capture_handoff_evidence = true,
            .require_cpu_profile_zone_evidence = desc.require_cpu_profile_zone_evidence,
            .require_trace_capture_handoff_evidence = desc.require_trace_capture_handoff_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .scene_frame_resources_available = result.scene_resources_ready,
            .request_automatic_capture_execution = false,
            .request_production_flame_graph = false,
            .request_crash_telemetry_export = false,
            .source_index = 0,
        };
    } else if (result.scene_resources_ready) {
        requests[request_count++] = DebugProfilingRequestDesc{
            .capture_kind = DebugProfilingCaptureKind::none,
            .require_gpu_timestamps = false,
            .require_gpu_debug_markers = false,
            .require_capture_handoff_evidence = false,
            .require_cpu_profile_zone_evidence = desc.require_cpu_profile_zone_evidence,
            .require_trace_capture_handoff_evidence = desc.require_trace_capture_handoff_evidence,
            .require_package_counter_evidence = desc.require_package_counter_evidence,
            .scene_frame_resources_available = true,
            .request_automatic_capture_execution = false,
            .request_production_flame_graph = false,
            .request_crash_telemetry_export = false,
            .source_index = 0,
        };
    }

    const auto plan = plan_debug_profiling_policy(DebugProfilingPolicyDesc{
        .requests = std::span<const DebugProfilingRequestDesc>{requests.data(), request_count},
        .expected_frames = desc.expected_frames,
        .frames_finished = report.renderer_stats.frames_finished,
        .gpu_timestamp_ticks_per_second = report.rhi_gpu_timestamp_ticks_per_second,
        .gpu_debug_scopes_begun = report.rhi_gpu_debug_scopes_begun,
        .gpu_debug_scopes_ended = report.rhi_gpu_debug_scopes_ended,
        .gpu_debug_markers_inserted = report.rhi_gpu_debug_markers_inserted,
        .cpu_profile_zone_count = desc.cpu_profile_zone_count,
        .trace_capture_handoff_row_count = desc.trace_capture_handoff_row_count,
        .framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed,
        .framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded,
        .backend = postprocess_policy_backend(report.selected_backend),
        .require_backend_profiling_evidence = desc.require_backend_profiling_evidence,
        .backend_profiling_evidence_ready = backend_evidence_ready,
        .package_counter_evidence_ready = desc.package_counter_evidence_ready,
    });

    result.backend_profiling_evidence_ready = backend_evidence_ready;
    result.diagnostics_count = static_cast<std::uint32_t>(plan.diagnostics.size());
    if (!result.frames_current) {
        ++result.diagnostics_count;
    }
    result.request_count = plan.request_count;
    result.gpu_timestamp_request_count = plan.gpu_timestamp_request_count;
    result.gpu_debug_marker_request_count = plan.gpu_debug_marker_request_count;
    result.capture_handoff_request_count = plan.capture_handoff_request_count;
    result.cpu_profile_zone_request_count = plan.cpu_profile_zone_request_count;
    result.trace_capture_handoff_request_count = plan.trace_capture_handoff_request_count;
    result.package_counter_request_count = plan.package_counter_request_count;
    result.cpu_profile_zone_evidence_ready = plan.cpu_profile_zone_evidence_ready;
    result.trace_capture_handoff_evidence_ready = plan.trace_capture_handoff_evidence_ready;
    result.package_counter_evidence_ready = plan.package_counter_evidence_ready;
    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationDebugProfilingPolicyStatus::ready
                                 : Win32DesktopPresentationDebugProfilingPolicyStatus::blocked;
    return result;
}

std::string_view win32_desktop_presentation_d3d12_debug_profiling_execution_status_name(
    const Win32DesktopPresentationD3d12DebugProfilingExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationD3d12DebugProfilingExecutionReport
evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution(const Win32DesktopPresentationReport& report,
                                                                    const bool requested) {
    Win32DesktopPresentationD3d12DebugProfilingExecutionReport result;
    result.gpu_timestamp_ticks_per_second = report.rhi_gpu_timestamp_ticks_per_second;
    result.gpu_debug_scopes_begun = report.rhi_gpu_debug_scopes_begun;
    result.gpu_debug_scopes_ended = report.rhi_gpu_debug_scopes_ended;
    result.gpu_debug_markers_inserted = report.rhi_gpu_debug_markers_inserted;
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    result.framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded;

    if (!requested) {
        return result;
    }

    result.d3d12_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::d3d12;
    result.gpu_timestamps_current = result.d3d12_backend_selected && result.gpu_timestamp_ticks_per_second > 0;
    result.gpu_debug_markers_current =
        result.d3d12_backend_selected && (result.gpu_debug_scopes_begun > 0 || result.gpu_debug_scopes_ended > 0 ||
                                          result.gpu_debug_markers_inserted > 0);
    result.frame_diagnostics_current = result.d3d12_backend_selected && result.framegraph_barrier_steps_executed > 0 &&
                                       result.framegraph_render_passes_recorded > 0;
    result.status =
        result.gpu_timestamps_current && result.gpu_debug_markers_current && result.frame_diagnostics_current
            ? Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::ready
            : Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationD3d12DebugProfilingExecutionStatus::ready;
    return result;
}

std::string_view win32_desktop_presentation_vulkan_debug_profiling_execution_status_name(
    const Win32DesktopPresentationVulkanDebugProfilingExecutionStatus status) noexcept {
    switch (status) {
    case Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::not_requested:
        return "not_requested";
    case Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::blocked:
        return "blocked";
    case Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

Win32DesktopPresentationVulkanDebugProfilingExecutionReport
evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(const Win32DesktopPresentationReport& report,
                                                                     const bool requested) {
    Win32DesktopPresentationVulkanDebugProfilingExecutionReport result;
    result.gpu_timestamp_ticks_per_second = report.rhi_gpu_timestamp_ticks_per_second;
    result.gpu_debug_scopes_begun = report.rhi_gpu_debug_scopes_begun;
    result.gpu_debug_scopes_ended = report.rhi_gpu_debug_scopes_ended;
    result.gpu_debug_markers_inserted = report.rhi_gpu_debug_markers_inserted;
    result.framegraph_barrier_steps_executed = report.renderer_stats.framegraph_barrier_steps_executed;
    result.framegraph_render_passes_recorded = report.renderer_stats.framegraph_render_passes_recorded;

    if (!requested) {
        return result;
    }

    result.vulkan_backend_selected = report.selected_backend == Win32DesktopPresentationBackend::vulkan;
    result.gpu_timestamps_current = result.vulkan_backend_selected && result.gpu_timestamp_ticks_per_second > 0;
    result.gpu_debug_markers_current =
        result.vulkan_backend_selected && (result.gpu_debug_scopes_begun > 0 || result.gpu_debug_scopes_ended > 0 ||
                                           result.gpu_debug_markers_inserted > 0);
    result.frame_diagnostics_current = result.vulkan_backend_selected && result.framegraph_barrier_steps_executed > 0 &&
                                       result.framegraph_render_passes_recorded > 0;
    result.status = result.gpu_debug_markers_current && result.frame_diagnostics_current
                        ? Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::ready
                        : Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::blocked;
    result.ready = result.status == Win32DesktopPresentationVulkanDebugProfilingExecutionStatus::ready;
    return result;
}

Win32DesktopPresentationQualityGateReport
evaluate_win32_desktop_presentation_quality_gate(const Win32DesktopPresentationReport& report,
                                                 const Win32DesktopPresentationQualityGateDesc& desc) noexcept {
    Win32DesktopPresentationQualityGateReport result;
    result.expected_framegraph_passes = quality_gate_expected_framegraph_passes(desc);
    result.expected_framegraph_render_passes = quality_gate_expected_framegraph_render_passes(desc);
    result.expected_framegraph_barrier_steps = quality_gate_expected_framegraph_barrier_steps(desc);
    if (!quality_gate_requested(desc)) {
        return result;
    }

    auto add_diagnostic = [&result]() noexcept { ++result.diagnostics_count; };

    if (desc.require_scene_gpu_bindings) {
        const auto mesh_bindings = report.scene_gpu_stats.mesh_bindings + report.scene_gpu_stats.skinned_mesh_bindings;
        const auto mesh_bindings_resolved =
            report.scene_gpu_stats.mesh_bindings_resolved + report.scene_gpu_stats.skinned_mesh_bindings_resolved;
        result.scene_gpu_ready = report.scene_gpu_status == Win32DesktopPresentationSceneGpuBindingStatus::ready &&
                                 mesh_bindings > 0 && report.scene_gpu_stats.material_bindings > 0;
        if (desc.expected_frames > 0) {
            result.scene_gpu_ready = result.scene_gpu_ready && mesh_bindings_resolved == desc.expected_frames &&
                                     report.scene_gpu_stats.material_bindings_resolved == desc.expected_frames;
        }
        if (!result.scene_gpu_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_postprocess) {
        result.postprocess_ready = report.postprocess_status == Win32DesktopPresentationPostprocessStatus::ready;
        if (!result.postprocess_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_postprocess_depth_input) {
        result.postprocess_depth_input_ready =
            report.postprocess_depth_input_requested && report.postprocess_depth_input_ready;
        if (!result.postprocess_depth_input_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_directional_shadow) {
        result.directional_shadow_ready =
            report.directional_shadow_requested &&
            report.directional_shadow_status == Win32DesktopPresentationDirectionalShadowStatus::ready &&
            report.directional_shadow_ready;
        if (!result.directional_shadow_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_directional_shadow_filtering) {
        result.directional_shadow_filter_ready =
            report.directional_shadow_filter_mode ==
                Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 &&
            report.directional_shadow_filter_tap_count == 9 && report.directional_shadow_filter_radius_texels == 1.0F;
        if (!result.directional_shadow_filter_ready) {
            add_diagnostic();
        }
    }

    if (result.expected_framegraph_passes > 0) {
        result.framegraph_passes_current = report.framegraph_passes == result.expected_framegraph_passes;
        if (!result.framegraph_passes_current) {
            add_diagnostic();
        }
    }

    if (result.expected_framegraph_render_passes > 0) {
        if (desc.expected_frames > 0) {
            result.framegraph_render_passes_current =
                report.renderer_stats.framegraph_render_passes_recorded == result.expected_framegraph_render_passes;
        } else {
            result.framegraph_render_passes_current =
                report.renderer_stats.framegraph_render_passes_recorded >= result.expected_framegraph_render_passes;
        }
        if (!result.framegraph_render_passes_current) {
            add_diagnostic();
        }
    }

    if (result.expected_framegraph_barrier_steps > 0) {
        if (desc.expected_frames > 0) {
            const auto expected_framegraph_executions =
                desc.expected_frames * static_cast<std::uint64_t>(result.expected_framegraph_passes);
            result.framegraph_barrier_steps_current =
                report.renderer_stats.framegraph_barrier_steps_executed == result.expected_framegraph_barrier_steps;
            result.framegraph_execution_budget_current =
                report.renderer_stats.frames_finished == desc.expected_frames &&
                report.renderer_stats.framegraph_passes_executed == expected_framegraph_executions &&
                report.renderer_stats.framegraph_render_passes_recorded == result.expected_framegraph_render_passes &&
                result.framegraph_barrier_steps_current &&
                (!desc.require_postprocess ||
                 report.renderer_stats.postprocess_passes_executed == desc.expected_frames);
        } else {
            result.framegraph_barrier_steps_current =
                report.renderer_stats.framegraph_barrier_steps_executed >=
                static_cast<std::uint64_t>(result.expected_framegraph_barrier_steps);
            result.framegraph_execution_budget_current = result.framegraph_passes_current &&
                                                         result.framegraph_render_passes_current &&
                                                         result.framegraph_barrier_steps_current;
        }
        if (!result.framegraph_barrier_steps_current) {
            add_diagnostic();
        }
        if (!result.framegraph_execution_budget_current) {
            add_diagnostic();
        }
    } else if (result.expected_framegraph_passes > 0) {
        result.framegraph_execution_budget_current =
            result.framegraph_passes_current && result.framegraph_render_passes_current;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? Win32DesktopPresentationQualityGateStatus::ready
                                 : Win32DesktopPresentationQualityGateStatus::blocked;
    return result;
}

} // namespace mirakana
